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
from emb_agent.prompts import (
    Language,
    get_system_prompt,
    get_welcome_message,
    get_tool_continue_message,
    get_error_message,
    get_evaluation_report_text,
)
from emb_agent.session import Session, SessionManager
from emb_agent.skills import SkillLoader
from emb_agent.tools import ToolRegistry
from emb_agent.tools.deploy import SSHDeployTool, SSHExecTool
from emb_agent.tools.filesystem import EditFileTool, ListDirTool, ReadFileTool, WriteFileTool
from emb_agent.tools.knowledge import AddKnowledgeTool, KnowledgeRetrievalTool
from emb_agent.tools.monitor import BoardMetricsTool, BoardMetricSummaryTool, BoardMetricsAnalysisTool
from emb_agent.tools.network_scan import GetBoardIPTool, NetworkScanTool
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
        language: str = "zh",
        reasoning_effort: str | None = None,
        target_board_config: dict | None = None,
    ):
        self.workspace = workspace
        self.provider = provider
        self.model = model or provider.get_default_model()
        self.max_iterations = max_iterations
        self.context_window_tokens = context_window_tokens
        self.language = Language(language) if language else Language.ZH
        self.reasoning_effort = reasoning_effort
        self.kb = None
        self.target_board_config = target_board_config or {}

        self.workspace.mkdir(parents=True, exist_ok=True)
        (self.workspace / "projects").mkdir(exist_ok=True)
        (self.workspace / "sessions").mkdir(exist_ok=True)
        (self.workspace / "knowledge").mkdir(exist_ok=True)

        self.bus = MessageBus()
        self.sessions = SessionManager(self.workspace)
        self.tools = ToolRegistry()
        self.skills = SkillLoader(self.workspace.parent / "emb_agent" / "skills")
        self.skills.discover_skills()
        self._active_skill_context: dict[str, str] = {}

        self._setup_tools()
        self._setup_agent_prompt()

        self._running = False
        self._start_time = time.time()
        self._last_usage: dict[str, int] = {}
        self._evaluation_data: list[dict[str, Any]] = []

    def _setup_tools(self) -> None:
        """Register all available tools."""
        restrict = True

        # 获取目标板配置
        board_host = self.target_board_config.get("host", "")
        board_port = self.target_board_config.get("port", 22)
        board_username = self.target_board_config.get("username", "root")
        board_password = self.target_board_config.get("password", "")
        board_key_path = self.target_board_config.get("key_path", None)
        board_target_path = self.target_board_config.get("target_path", "/root/projects")

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
        self.tools.register(SSHDeployTool(
            host=board_host,
            port=board_port,
            username=board_username,
            password=board_password,
            key_path=board_key_path,
            target_path=board_target_path,
        ))
        self.tools.register(SSHExecTool(
            host=board_host,
            port=board_port,
            username=board_username,
            password=board_password,
            key_path=board_key_path,
        ))
        self.tools.register(NetworkScanTool())
        self.tools.register(GetBoardIPTool())
        self.tools.register(BoardMetricsTool(
            host=board_host,
            port=board_port,
            username=board_username,
            password=board_password,
            key_path=board_key_path,
        ))
        self.tools.register(BoardMetricSummaryTool(
            host=board_host,
            port=board_port,
            username=board_username,
            password=board_password,
            key_path=board_key_path,
        ))
        self.tools.register(BoardMetricsAnalysisTool(
            host=board_host,
            port=board_port,
            username=board_username,
            password=board_password,
            key_path=board_key_path,
        ))

    def _setup_agent_prompt(self) -> None:
        """Create agent prompt directory if needed."""
        pass

    def build_system_prompt(self) -> str:
        """Build the system prompt with workspace and knowledge base info."""
        base_prompt = get_system_prompt(self.language)

        kb_info = ""
        if self.kb and self.kb.enabled:
            docs_count = len(self.kb.index) if self.kb.index else 0
            if self.language == Language.ZH:
                kb_info = f"\n\n## 知识库\n你有权限访问包含 {docs_count} 个飞腾开发板相关文档的知识库。"
                if self.kb.top_k:
                    kb_info += f"\n使用 retrieve_knowledge 搜索相关信息。默认 top_k={self.kb.top_k}。"
            else:
                kb_info = f"\n\n## Knowledge Base\nYou have access to a knowledge base with {docs_count} documents about Phytium board development."
                if self.kb.top_k:
                    kb_info += f"\nUse retrieve_knowledge to search for relevant information. Default top_k={self.kb.top_k}."

        skill_context = ""
        if self._active_skill_context:
            if self.language == Language.ZH:
                skill_context = "\n\n## 已加载技能"
                for name, md_content in self._active_skill_context.items():
                    skill_context += f"\n\n### {name}\n{md_content}"
            else:
                skill_context = "\n\n## Loaded Skills"
                for name, md_content in self._active_skill_context.items():
                    skill_context += f"\n\n### {name}\n{md_content}"

        tools_info = ""
        if self.tools:
            tool_defs = self.tools.get_definitions()
            tool_names = [t["function"]["name"] for t in tool_defs]
            if self.language == Language.ZH:
                tools_info = "\n\n## 已注册工具\n" + "\n".join(f"- {name}" for name in tool_names)
            else:
                tools_info = "\n\n## Registered Tools\n" + "\n".join(f"- {name}" for name in tool_names)

        workspace_info = ""
        if self.workspace:
            if self.language == Language.ZH:
                workspace_info = f"\n\n## 工作空间\n工作空间: {self.workspace}"
            else:
                workspace_info = f"\n\n## Workspace\nWorkspace: {self.workspace}"

        reasoning_info = ""
        if self.reasoning_effort:
            if self.language == Language.ZH:
                reasoning_info = f"\n\n## 推理\n推理强度: {self.reasoning_effort}"
            else:
                reasoning_info = f"\n\n## Reasoning\nReasoning effort: {self.reasoning_effort}"

        return base_prompt + kb_info + skill_context + tools_info + workspace_info + reasoning_info

    async def _activate_matching_skills(self, query: str) -> None:
        """Load skill context when query matches skill keywords or semantics."""
        matched = await self.skills.match_skill_by_query(query)
        self._active_skill_context.clear()

        for skill_name in matched:
            skill = self.skills.get_skill(skill_name)
            if skill:
                md_content = skill.get_skill_md()
                if md_content:
                    self._active_skill_context[skill_name] = md_content
                    logger.info("Activated skill: {} for query", skill_name)

    async def _execute_skill(self, skill_name: str, **kwargs: Any) -> Any:
        """Execute a loaded skill."""
        skill = self.skills.get_skill(skill_name)
        if not skill:
            return {"success": False, "error": f"Skill '{skill_name}' not found"}

        try:
            return await skill.execute(**kwargs)
        except Exception as e:
            logger.exception("Skill execution failed: {}", skill_name)
            return {"success": False, "error": str(e)}

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
                    "content": get_tool_continue_message(self.language),
                })
            else:
                clean = self._strip_think(response.content)
                if response.finish_reason == "error":
                    logger.error("LLM returned error: {}", (clean or "")[:200])
                    final_content = clean or get_error_message("llm_error", self.language, error="AI model error")
                    break

                messages.append({"role": "assistant", "content": clean})
                final_content = clean
                break

        if final_content is None and iteration >= self.max_iterations:
            logger.warning("Max iterations ({}) reached", self.max_iterations)
            final_content = get_error_message("max_iterations", self.language, max_iterations=self.max_iterations)

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

        await self._activate_matching_skills(content)

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
        return get_evaluation_report_text(self._evaluation_data, self.language)
