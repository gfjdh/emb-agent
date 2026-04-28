"""Performance monitoring tool for remote target board via SSH."""

import asyncio
import json
from typing import Any, Dict, List

import paramiko
from emb_agent.tools.base import Tool


class BoardMetricsTool(Tool):
    """Tool to collect performance metrics from remote target board via SSH."""

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
        return "get_board_metrics"

    @property
    def description(self) -> str:
        return "Collect comprehensive performance metrics from remote target board via SSH, including CPU usage, memory, disk IO, network status, processes, system load, temperature, and kernel logs."

    @property
    def parameters(self) -> dict[str, Any]:
        return {
            "type": "object",
            "properties": {
                "host": {
                    "type": "string",
                    "description": "Target board IP or hostname",
                },
                "metrics": {
                    "type": "array",
                    "items": {"type": "string"},
                    "description": "Optional list of metrics to collect. Available: cpu, memory, disk, network, processes, load, temperature, dmesg. If not specified, all metrics are collected.",
                },
            },
            "required": [],
        }

    async def execute(
        self, host: str | None = None, metrics: list[str] | None = None, **kwargs: Any
    ) -> str:
        target_host = host or self.host

        if not target_host:
            return json.dumps({"error": "No host provided and no default target_board host configured"}, ensure_ascii=False)

        if metrics is None:
            metrics = ["cpu", "memory", "disk", "network", "processes", "load", "temperature", "dmesg"]

        results: Dict[str, Any] = {}
        for metric in metrics:
            result = await self._collect_metric(target_host, metric)
            if result:
                results[metric] = result

        return json.dumps(results, ensure_ascii=False, indent=2)

    async def _collect_metric(self, host: str, metric: str) -> Any:
        commands = {
            "cpu": "top -bn1 | head -20",
            "memory": "free -h && cat /proc/meminfo | head -20",
            "disk": "df -h && iostat -x 1 2 2>/dev/null || iostat 1 2",
            "network": "ss -tuln && netstat -s 2>/dev/null | head -30 || echo 'netstat not available'",
            "processes": "ps aux --sort=-%cpu | head -20",
            "load": "uptime && cat /proc/loadavg",
            "temperature": "cat /sys/class/thermal/thermal_zone*/temp 2>/dev/null | awk '{printf \"Zone %d: %.1f°C\\n\", NR, $1/1000}' || echo 'Temperature sensors not available'",
            "dmesg": "dmesg | tail -30",
        }

        cmd = commands.get(metric)
        if not cmd:
            return {"error": f"Unknown metric: {metric}"}

        try:
            output = await self._exec_ssh_command(host, cmd, 30)
            return output

        except asyncio.TimeoutError:
            return {"error": f"Command timed out"}
        except Exception as e:
            return {"error": str(e)}

    async def _exec_ssh_command(self, host: str, command: str, timeout: int) -> str:
        """Execute command on remote host via SSH using paramiko."""
        loop = asyncio.get_event_loop()
        
        def run_ssh():
            ssh = paramiko.SSHClient()
            ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            
            try:
                if self.key_path:
                    # 使用密钥认证
                    ssh.connect(
                        hostname=host,
                        port=self.port,
                        username=self.username,
                        key_filename=self.key_path,
                        timeout=timeout,
                        banner_timeout=timeout,
                    )
                elif self.password:
                    # 使用密码认证
                    ssh.connect(
                        hostname=host,
                        port=self.port,
                        username=self.username,
                        password=self.password,
                        timeout=timeout,
                        banner_timeout=timeout,
                    )
                else:
                    # 默认使用密钥认证（查找默认密钥）
                    ssh.connect(
                        hostname=host,
                        port=self.port,
                        username=self.username,
                        timeout=timeout,
                        banner_timeout=timeout,
                    )
                
                stdin, stdout, stderr = ssh.exec_command(command, timeout=timeout)
                output = stdout.read().decode("utf-8", errors="replace").strip()
                stderr_output = stderr.read().decode("utf-8", errors="replace").strip()
                
                if stderr_output:
                    output += "\nSTDERR: " + stderr_output
                
                return output
            finally:
                ssh.close()
        
        return await loop.run_in_executor(None, run_ssh)

    def _build_ssh_command(self, host: str, command: str, timeout: int) -> str:
        key_opt = f"-i {self.key_path}" if self.key_path else ""
        port_opt = f"-p {self.port}" if self.port != 22 else ""
        if self.password:
            # 使用 sshpass 进行密码认证
            return f"sshpass -p '{self.password}' ssh {key_opt} {port_opt} -o StrictHostKeyChecking=no -o ConnectTimeout={timeout} {self.username}@{host} '{command}'"
        else:
            return f"ssh {key_opt} {port_opt} -o StrictHostKeyChecking=no -o ConnectTimeout={timeout} {self.username}@{host} '{command}'"


class BoardMetricSummaryTool(Tool):
    """Tool to get a quick summary of board performance metrics."""

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
        return "get_board_summary"

    @property
    def description(self) -> str:
        return "Get a quick summary of key performance metrics from remote target board via SSH, formatted as JSON for easy analysis."

    @property
    def parameters(self) -> dict[str, Any]:
        return {
            "type": "object",
            "properties": {
                "host": {
                    "type": "string",
                    "description": "Target board IP or hostname",
                },
            },
            "required": [],
        }

    async def execute(self, host: str | None = None, **kwargs: Any) -> str:
        target_host = host or self.host

        if not target_host:
            return json.dumps({"error": "No host provided and no default target_board host configured"}, ensure_ascii=False)

        try:
            cmd = """
                echo "=== SYSTEM ===" && \
                echo "Hostname: $(hostname)" && \
                echo "OS: $(cat /etc/os-release | grep PRETTY_NAME | cut -d'"' -f2)" && \
                echo "Kernel: $(uname -r)" && \
                echo "" && \
                echo "=== CPU ===" && \
                top -bn1 | head -5 | tail -2 && \
                echo "" && \
                echo "=== MEMORY ===" && \
                free -h | head -2 && \
                echo "" && \
                echo "=== DISK ===" && \
                df -h / | tail -1 && \
                echo "" && \
                echo "=== LOAD ===" && \
                uptime && \
                echo "" && \
                echo "=== TOP_PROCESSES ===" && \
                ps aux --sort=-%cpu | head -6 | tail -4 && \
                echo "" && \
                echo "=== NETWORK ===" && \
                ip addr show | grep -E 'inet (?!127.0.0.1)' | head -3 && \
                echo "" && \
                echo "=== TEMPERATURE ===" && \
                cat /sys/class/thermal/thermal_zone*/temp 2>/dev/null | awk '{printf \"%.1f°C \", $1/1000}' || echo "N/A"
            """

            ssh_cmd = self._build_ssh_command(target_host, cmd, 30)
            proc = await asyncio.create_subprocess_shell(
                ssh_cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
            )
            stdout, stderr = await asyncio.wait_for(
                proc.communicate(),
                timeout=35,
            )

            output = stdout.decode("utf-8", errors="replace").strip()
            if stderr and stderr.decode("utf-8", errors="replace").strip():
                output += "\nWarnings: " + stderr.decode("utf-8", errors="replace").strip()

            return output

        except asyncio.TimeoutError:
            return json.dumps({"error": "Command timed out"}, ensure_ascii=False)
        except Exception as e:
            return json.dumps({"error": str(e)}, ensure_ascii=False)

    def _build_ssh_command(self, host: str, command: str, timeout: int) -> str:
        key_opt = f"-i {self.key_path}" if self.key_path else ""
        port_opt = f"-p {self.port}" if self.port != 22 else ""
        if self.password:
            # 使用 sshpass 进行密码认证
            return f"sshpass -p '{self.password}' ssh {key_opt} {port_opt} -o StrictHostKeyChecking=no -o ConnectTimeout={timeout} {self.username}@{host} '{command}'"
        else:
            return f"ssh {key_opt} {port_opt} -o StrictHostKeyChecking=no -o ConnectTimeout={timeout} {self.username}@{host} '{command}'"


class BoardMetricsAnalysisTool(Tool):
    """Tool to analyze board performance metrics and identify potential bottlenecks."""

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
        self.metrics_tool = BoardMetricsTool(host, port, username, key_path)

    @property
    def name(self) -> str:
        return "analyze_board_performance"

    @property
    def description(self) -> str:
        return "Analyze board performance metrics and identify potential bottlenecks. Returns a structured analysis report with recommendations."

    @property
    def parameters(self) -> dict[str, Any]:
        return {
            "type": "object",
            "properties": {
                "host": {
                    "type": "string",
                    "description": "Target board IP or hostname",
                },
            },
            "required": [],
        }

    async def execute(self, host: str | None = None, **kwargs: Any) -> str:
        target_host = host or self.host

        if not target_host:
            return json.dumps({"error": "No host provided and no default target_board host configured"}, ensure_ascii=False)

        # 获取性能数据
        metrics_output = await self.metrics_tool.execute(host=target_host)
        
        try:
            metrics = json.loads(metrics_output)
        except json.JSONDecodeError:
            return json.dumps({"error": "Failed to parse metrics data"}, ensure_ascii=False)

        # 分析性能数据
        analysis = self._analyze_metrics(metrics)
        return json.dumps(analysis, ensure_ascii=False, indent=2)

    def _analyze_metrics(self, metrics: Dict[str, Any]) -> Dict[str, Any]:
        """Analyze collected metrics and identify potential issues."""
        analysis = {
            "summary": "Performance analysis completed",
            "issues": [],
            "warnings": [],
            "recommendations": [],
            "raw_metrics": metrics
        }

        # 分析CPU
        if "cpu" in metrics and isinstance(metrics["cpu"], str):
            cpu_usage = self._parse_cpu_usage(metrics["cpu"])
            if cpu_usage and cpu_usage > 80:
                analysis["issues"].append(f"High CPU usage detected: {cpu_usage}%")
                analysis["recommendations"].append("Identify and optimize CPU-intensive processes")
            elif cpu_usage and cpu_usage > 60:
                analysis["warnings"].append(f"Moderate CPU usage: {cpu_usage}%")

        # 分析内存
        if "memory" in metrics and isinstance(metrics["memory"], str):
            memory_usage = self._parse_memory_usage(metrics["memory"])
            if memory_usage and memory_usage > 85:
                analysis["issues"].append(f"High memory usage detected: {memory_usage}%")
                analysis["recommendations"].append("Consider adding swap space or optimizing memory usage")
            elif memory_usage and memory_usage > 70:
                analysis["warnings"].append(f"Moderate memory usage: {memory_usage}%")

        # 分析系统负载
        if "load" in metrics and isinstance(metrics["load"], str):
            load_avg = self._parse_load_average(metrics["load"])
            if load_avg and load_avg > 2.0:
                analysis["issues"].append(f"High system load detected: {load_avg}")
                analysis["recommendations"].append("Check for run-away processes or consider system optimization")

        # 分析温度
        if "temperature" in metrics and isinstance(metrics["temperature"], str):
            temps = self._parse_temperatures(metrics["temperature"])
            if temps:
                max_temp = max(temps)
                if max_temp > 80:
                    analysis["issues"].append(f"High temperature detected: {max_temp}°C")
                    analysis["recommendations"].append("Check cooling solution, reduce CPU load")
                elif max_temp > 60:
                    analysis["warnings"].append(f"Moderate temperature: {max_temp}°C")

        # 分析磁盘
        if "disk" in metrics and isinstance(metrics["disk"], str):
            disk_usage = self._parse_disk_usage(metrics["disk"])
            if disk_usage and disk_usage > 90:
                analysis["issues"].append(f"Low disk space detected: {disk_usage}%")
                analysis["recommendations"].append("Clean up unnecessary files or expand storage")

        if not analysis["issues"] and not analysis["warnings"]:
            analysis["summary"] = "System is running normally with no performance issues detected"

        return analysis

    def _parse_cpu_usage(self, cpu_output: str) -> float | None:
        """Parse CPU usage percentage from top output."""
        for line in cpu_output.split('\n'):
            if '%Cpu(s)' in line:
                parts = line.split()
                for part in parts:
                    if '%us' in part:
                        idx = parts.index(part) - 1
                        if idx >= 0:
                            try:
                                return float(parts[idx])
                            except ValueError:
                                pass
        return None

    def _parse_memory_usage(self, memory_output: str) -> float | None:
        """Parse memory usage percentage from free output."""
        for line in memory_output.split('\n'):
            if line.startswith('Mem:'):
                parts = line.split()
                if len(parts) >= 3:
                    try:
                        total = int(parts[1].replace('Gi', '').replace('Mi', '').replace('Ki', ''))
                        used = int(parts[2].replace('Gi', '').replace('Mi', '').replace('Ki', ''))
                        return (used / total) * 100
                    except ValueError:
                        pass
        return None

    def _parse_load_average(self, load_output: str) -> float | None:
        """Parse load average from uptime output."""
        for line in load_output.split('\n'):
            if 'load average' in line:
                parts = line.split()
                load_idx = parts.index('average:') + 1 if 'average:' in parts else -1
                if load_idx >= 0 and load_idx < len(parts):
                    try:
                        return float(parts[load_idx].replace(',', ''))
                    except ValueError:
                        pass
        return None

    def _parse_temperatures(self, temp_output: str) -> List[float]:
        """Parse temperature values from thermal zone output."""
        temps = []
        for line in temp_output.split('\n'):
            match = __import__('re').search(r'(\d+\.?\d*)°C', line)
            if match:
                try:
                    temps.append(float(match.group(1)))
                except ValueError:
                    pass
        return temps

    def _parse_disk_usage(self, disk_output: str) -> float | None:
        """Parse disk usage percentage from df output."""
        for line in disk_output.split('\n'):
            if '/' in line and not line.startswith('Filesystem'):
                parts = line.split()
                if len(parts) >= 5:
                    try:
                        return float(parts[4].replace('%', ''))
                    except ValueError:
                        pass
        return None
