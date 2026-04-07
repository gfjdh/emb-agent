"""Core agent implementation."""

import asyncio
import json
import os
import re
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Any, Callable

from loguru import logger

from emb_agent.bus import InboundMessage, OutboundMessage
from emb_agent.bus.queue import MessageBus
from emb_agent.providers import LLMProvider, LiteLLMProvider
from emb_agent.session import Session, SessionManager
from emb_agent.tools import ToolRegistry
from emb_agent.tools.deploy import SSHDeployTool, SSHExecTool
from emb_agent.tools.filesystem import EditFileTool, ListDirTool, ReadFileTool, WriteFileTool
from emb_agent.tools.knowledge import AddKnowledgeTool, KnowledgeRetrievalTool
from emb_agent.tools.shell import ExecTool


class Agent:
    """Main agent for embedded system development assistance."""

    def __init__(
        self,
        workspace: Path,
        provider: LLMProvider,
        model: str | None = None,
        max_iterations: int = 40,
        context_window_tokens: int = 200000,
    ):
        self.workspace = workspace
        self.provider = provider
        self.model = model or provider.get_default_model()
        self.max_iterations = max_iterations
        self.context_window_tokens = context_window_tokens

        self.workspace.mkdir(parents=True, exist_ok=True)
        (self.workspace / "projects").mkdir(exist_ok=True)
        (self.workspace / "sessions").mkdir(exist_ok=True)
        (self.workspace / "knowledge").mkdir(exist_ok=True)

        self.bus = MessageBus()
        self.sessions = SessionManager(self.workspace)
        self.tools = ToolRegistry()

        self._setup_tools()
        self._setup_agent_prompt()

        self._running = False
        self._start_time = time.time()
        self._last_usage: dict[str, int] = {}
        self._evaluation_data: list[dict[str, Any]] = []

    def _setup_tools(self) -> None:
        """Register all available tools."""
        restrict = True

        self.tools.register(ReadFileTool(workspace=self.workspace, allowed_dir=self.workspace))
        self.tools.register(WriteFileTool(workspace=self.workspace, allowed_dir=self.workspace))
        self.tools.register(EditFileTool(workspace=self.workspace, allowed_dir=self.workspace))
        self.tools.register(ListDirTool(workspace=self.workspace, allowed_dir=self.workspace))
        self.tools.register(ExecTool(
            working_dir=str(self.workspace),
            timeout=120,
            restrict_to_workspace=restrict,
        ))
        self.tools.register(KnowledgeRetrievalTool(workspace=self.workspace))
        self.tools.register(AddKnowledgeTool(workspace=self.workspace))
        self.tools.register(SSHDeployTool())
        self.tools.register(SSHExecTool())

    def _setup_agent_prompt(self) -> None:
        """Create agent prompt directory if needed."""
        pass

    def build_system_prompt(self) -> str:
        """Build the system prompt for the agent."""
        return """# emb-agent - Embedded System Development Assistant

You are an expert embedded systems developer specializing in Phytium (飞腾) ARM processors and Linux kernel development.

## Your Capabilities

You help users develop embedded applications for Phytium development boards through the full lifecycle:
1. Requirement analysis and project planning
2. Knowledge retrieval for platform-specific guidance
3. Code generation (drivers, applications, build scripts)
4. Deployment to target hardware
5. Testing and debugging assistance

## Core Use Cases

- UC-01: Parse user requirements into structured specifications
- UC-02: Retrieve relevant knowledge from the knowledge base
- UC-03: Generate comprehensive project plans
- UC-04: Generate code files for the project
- UC-05: Read and analyze code files
- UC-06: Modify and improve code files
- UC-07: Deploy code to Phytium development board
- UC-08: Execute tests on target hardware
- UC-09: Analyze error logs and diagnose issues
- UC-10: Provide and apply fixes for issues
- UC-11: Generate evaluation reports

## Working Process

1. When user describes a project (e.g., "Create a LED blinking driver for Phytium board"):
   - Use retrieve_knowledge to find relevant platform documentation
   - Generate a structured project plan
   - Create all necessary code files
   - Deploy to target and verify

2. When errors occur:
   - Analyze the error logs using LLM reasoning
   - Identify root cause
   - Propose or apply fixes

3. Always verify code correctness before reporting completion

## Tools Available

- read_file: Read file contents
- write_file: Write content to file
- edit_file: Edit existing files
- list_dir: List directory contents
- exec: Execute shell commands
- retrieve_knowledge: Query the knowledge base
- add_knowledge: Add new knowledge entries
- deploy_to_target: Deploy to remote Phytium board
- exec_on_target: Execute commands on remote board

## Safety Guidelines

- Validate all code before deployment
- Check board connection parameters before deployment
- Never suggest dangerous operations (formatting, overwriting boot sectors)
- Always confirm destructive actions with user

## Response Style

- Be concise and technical
- Provide working code examples
- Include compilation instructions when relevant
- Explain platform-specific considerations
"""

    def build_messages(
        self,
        history: list[dict[str, Any]],
        current_message: str,
        channel: str | None = None,
        chat_id: str | None = None,
    ) -> list[dict[str, Any]]:
        """Build messages for LLM call."""
        runtime_ctx = self._build_runtime_context(channel, chat_id)

        return [
            {"role": "system", "content": self.build_system_prompt()},
            *history,
            {"role": "user", "content": f"{runtime_ctx}\n\n{current_message}"},
        ]

    @staticmethod
    def _build_runtime_context(channel: str | None, chat_id: str | None) -> str:
        """Build runtime metadata block."""
        lines = [f"Current Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}"]
        if channel and chat_id:
            lines += [f"Channel: {channel}", f"Chat ID: {chat_id}"]
        return "\n".join(lines)

    @staticmethod
    def _strip_think(content: str | None) -> str | None:
        """Strip <think> blocks that some models embed in content."""
        if not content:
            return None
        pattern = "<think>"
        end_pattern = "</think>"
        result = content
        while pattern in result and end_pattern in result:
            start = result.find(pattern)
            end = result.find(end_pattern) + len(end_pattern)
            if start < end:
                result = result[:start] + result[end:]
            else:
                break
        return result.strip() or None

    async def run_agent_loop(
        self,
        initial_messages: list[dict],
        on_progress: Callable[..., Any] | None = None,
    ) -> tuple[str | None, list[str], list[dict]]:
        """Run the agent iteration loop."""
        messages = initial_messages
        iteration = 0
        final_content = None
        tools_used: list[str] = []

        task_start_time = time.time()

        while iteration < self.max_iterations:
            iteration += 1

            tool_defs = self.tools.get_definitions()

            response = await self.provider.chat_with_retry(
                messages=messages,
                tools=tool_defs,
                model=self.model,
            )

            usage = response.usage or {}
            self._last_usage = {
                "prompt_tokens": int(usage.get("prompt_tokens", 0) or 0),
                "completion_tokens": int(usage.get("completion_tokens", 0) or 0),
            }

            if response.has_tool_calls:
                if on_progress:
                    thought = self._strip_think(response.content)
                    if thought:
                        await on_progress(thought)

                tool_call_dicts = [
                    tc.to_openai_tool_call()
                    for tc in response.tool_calls
                ]
                messages.append({
                    "role": "assistant",
                    "content": response.content,
                    "tool_calls": tool_call_dicts,
                })

                for tool_call in response.tool_calls:
                    tools_used.append(tool_call.name)
                    args_str = json.dumps(tool_call.arguments, ensure_ascii=False)
                    logger.info("Tool call: {}({})", tool_call.name, args_str[:200])
                    result = await self.tools.execute(tool_call.name, tool_call.arguments)
                    messages.append({
                        "role": "tool",
                        "tool_call_id": tool_call.id,
                        "name": tool_call.name,
                        "content": result,
                    })

                # MiniMax requires a user message after all tool results
                messages.append({
                    "role": "user",
                    "content": "继续",
                })
            else:
                clean = self._strip_think(response.content)
                if response.finish_reason == "error":
                    logger.error("LLM returned error: {}", (clean or "")[:200])
                    final_content = clean or "Sorry, I encountered an error calling the AI model."
                    break

                messages.append({"role": "assistant", "content": clean})
                final_content = clean
                break

        if final_content is None and iteration >= self.max_iterations:
            logger.warning("Max iterations ({}) reached", self.max_iterations)
            final_content = (
                f"I reached the maximum number of tool call iterations ({self.max_iterations}) "
                "without completing the task."
            )

        elapsed = time.time() - task_start_time
        self._evaluation_data.append({
            "timestamp": datetime.now().isoformat(),
            "task": initial_messages[-1].get("content", "")[:100],
            "iterations": iteration,
            "tools_used": tools_used,
            "elapsed_seconds": elapsed,
            "success": final_content is not None and not final_content.startswith("Error"),
        })

        return final_content, tools_used, messages

    async def process_message(
        self,
        content: str,
        session_key: str = "cli:direct",
        channel: str = "cli",
        chat_id: str = "direct",
        on_progress: Callable[[str], Any] | None = None,
    ) -> str | None:
        """Process a message and return the response."""
        logger.info("Processing message: {}", content[:80])

        session = self.sessions.get_or_create(session_key)

        history = session.get_history(max_messages=0)
        initial_messages = self.build_messages(
            history=history,
            current_message=content,
            channel=channel,
            chat_id=chat_id,
        )

        async def progress_callback(content: str) -> None:
            if on_progress:
                await on_progress(content)

        final_content, _, all_msgs = await self.run_agent_loop(
            initial_messages, on_progress=progress_callback,
        )

        for m in all_msgs[1:]:
            if m.get("role") in ("user", "assistant"):
                session.messages.append(m)

        self.sessions.save(session)

        return final_content

    async def run(self) -> None:
        """Run the agent loop, processing messages from the bus."""
        self._running = True
        logger.info("Agent loop started")

        while self._running:
            try:
                msg = await asyncio.wait_for(self.bus.consume_inbound(), timeout=1.0)
            except asyncio.TimeoutError:
                continue
            except asyncio.CancelledError:
                break
            except Exception as e:
                logger.warning("Error consuming inbound message: {}", e)
                continue

            if msg.content.strip().lower() == "/stop":
                self._running = False
                break

            try:
                response = await self.process_message(
                    content=msg.content,
                    session_key=msg.session_key,
                    channel=msg.channel,
                    chat_id=msg.chat_id,
                )

                if response:
                    await self.bus.publish_outbound(OutboundMessage(
                        channel=msg.channel,
                        chat_id=msg.chat_id,
                        content=response,
                    ))
            except Exception as e:
                logger.exception("Error processing message")
                await self.bus.publish_outbound(OutboundMessage(
                    channel=msg.channel,
                    chat_id=msg.chat_id,
                    content=f"Sorry, I encountered an error: {str(e)}",
                ))

    def stop(self) -> None:
        """Stop the agent loop."""
        self._running = False
        logger.info("Agent loop stopping")

    def get_evaluation_report(self) -> str:
        """Generate evaluation report from collected data."""
        if not self._evaluation_data:
            return "No evaluation data available yet."

        total_tasks = len(self._evaluation_data)
        successful = sum(1 for d in self._evaluation_data if d["success"])
        total_iterations = sum(d["iterations"] for d in self._evaluation_data)
        total_time = sum(d["elapsed_seconds"] for d in self._evaluation_data)

        tool_usage: dict[str, int] = {}
        for d in self._evaluation_data:
            for tool in d["tools_used"]:
                tool_usage[tool] = tool_usage.get(tool, 0) + 1

        report_lines = [
            "# Evaluation Report",
            "",
            f"## Summary",
            f"- Total tasks: {total_tasks}",
            f"- Successful: {successful} ({100*successful/total_tasks:.1f}%)",
            f"- Failed: {total_tasks - successful}",
            f"- Total iterations: {total_iterations}",
            f"- Average iterations per task: {total_iterations/total_tasks:.1f}",
            f"- Total time: {total_time:.1f}s",
            f"- Average time per task: {total_time/total_tasks:.1f}s",
            "",
            "## Tool Usage",
        ]

        for tool, count in sorted(tool_usage.items(), key=lambda x: -x[1]):
            report_lines.append(f"- {tool}: {count}")

        return "\n".join(report_lines)
