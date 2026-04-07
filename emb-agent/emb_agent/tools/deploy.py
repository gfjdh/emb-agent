"""SSH deployment tool for target board."""

import asyncio
import os
import tempfile
from pathlib import Path
from typing import Any

from emb_agent.tools.base import Tool


class SSHDeployTool(Tool):
    """Tool to deploy code to remote target board via SSH/SCP."""

    def __init__(
        self,
        host: str = "",
        port: int = 22,
        username: str = "root",
        password: str = "",
        key_path: str | None = None,
        target_path: str = "/root/projects",
    ):
        self.host = host
        self.port = port
        self.username = username
        self.password = password
        self.key_path = key_path
        self.target_path = target_path

    @property
    def name(self) -> str:
        return "deploy_to_target"

    @property
    def description(self) -> str:
        return "Deploy local code directory to remote target board via SSH/SCP."

    @property
    def parameters(self) -> dict[str, Any]:
        return {
            "type": "object",
            "properties": {
                "local_path": {
                    "type": "string",
                    "description": "Local directory path containing code to deploy",
                },
                "remote_path": {
                    "type": "string",
                    "description": "Target directory on remote board (default: /root/projects)",
                },
                "host": {
                    "type": "string",
                    "description": "Target board IP or hostname",
                },
            },
            "required": ["local_path"],
        }

    async def execute(
        self, local_path: str, remote_path: str | None = None,
        host: str | None = None, **kwargs: Any,
    ) -> str:
        target_host = host or self.host
        target_remote = remote_path or self.target_path

        if not target_host:
            return "Error: No host provided and no default target_board host configured"

        local_dir = Path(local_path).expanduser().resolve()
        if not local_dir.exists():
            return f"Error: Local path does not exist: {local_path}"

        try:
            temp_zip = tempfile.mktemp(suffix=".tar.gz")
            tar_cmd = f"tar -czf {temp_zip} -C {local_dir} ."
            proc = await asyncio.create_subprocess_shell(
                tar_cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
            )
            await proc.communicate()

            ssh_cmd = self._build_scp_command(temp_zip, target_host, target_remote)
            proc = await asyncio.create_subprocess_shell(
                ssh_cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
            )
            stdout, stderr = await proc.communicate()

            os.remove(temp_zip)

            if proc.returncode != 0:
                return f"Error: Deploy failed - {stderr.decode() if stderr else 'Unknown error'}"

            return f"Successfully deployed {local_path} to {target_host}:{target_remote}"

        except Exception as e:
            return f"Error deploying to target: {str(e)}"

    def _build_scp_command(self, source: str, target_host: str, target_path: str) -> str:
        key_opt = f"-i {self.key_path}" if self.key_path else ""
        port_opt = f"-P {self.port}" if self.port != 22 else ""
        return f"scp {key_opt} {port_opt} {source} {self.username}@{target_host}:{target_path}"


class SSHExecTool(Tool):
    """Tool to execute commands on remote target board via SSH."""

    def __init__(
        self,
        host: str = "",
        port: int = 22,
        username: str = "root",
        password: str = "",
        key_path: str | None = None,
    ):
        self.host = host
        self.port = port
        self.username = username
        self.password = password
        self.key_path = key_path

    @property
    def name(self) -> str:
        return "exec_on_target"

    @property
    def description(self) -> str:
        return "Execute a command on the remote target board via SSH."

    @property
    def parameters(self) -> dict[str, Any]:
        return {
            "type": "object",
            "properties": {
                "command": {
                    "type": "string",
                    "description": "The command to execute on the target",
                },
                "host": {
                    "type": "string",
                    "description": "Target board IP or hostname",
                },
                "timeout": {
                    "type": "integer",
                    "description": "Command timeout in seconds (default 30)",
                    "minimum": 1,
                    "maximum": 300,
                },
            },
            "required": ["command"],
        }

    async def execute(
        self, command: str, host: str | None = None,
        timeout: int = 30, **kwargs: Any,
    ) -> str:
        target_host = host or self.host

        if not target_host:
            return "Error: No host provided and no default target_board host configured"

        try:
            ssh_cmd = self._build_ssh_command(target_host, command, timeout)
            proc = await asyncio.create_subprocess_shell(
                ssh_cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
            )
            stdout, stderr = await asyncio.wait_for(
                proc.communicate(),
                timeout=timeout + 5,
            )

            output_parts = []
            if stdout:
                output_parts.append(stdout.decode("utf-8", errors="replace"))
            if stderr:
                stderr_text = stderr.decode("utf-8", errors="replace")
                if stderr_text.strip():
                    output_parts.append(f"STDERR:\n{stderr_text}")
            output_parts.append(f"\nExit code: {proc.returncode}")

            return "\n".join(output_parts) if output_parts else "(no output)"

        except asyncio.TimeoutError:
            return f"Error: Command timed out after {timeout} seconds"
        except Exception as e:
            return f"Error executing on target: {str(e)}"

    def _build_ssh_command(self, host: str, command: str, timeout: int) -> str:
        key_opt = f"-i {self.key_path}" if self.key_path else ""
        port_opt = f"-p {self.port}" if self.port != 22 else ""
        return f"ssh {key_opt} {port_opt} -o StrictHostKeyChecking=no -o ConnectTimeout={timeout} {self.username}@{host} '{command}'"
