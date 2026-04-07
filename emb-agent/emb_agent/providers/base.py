"""Base LLM provider interface."""

import asyncio
import json
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from typing import Any

from loguru import logger


@dataclass
class ToolCallRequest:
    """A tool call request from the LLM."""
    id: str
    name: str
    arguments: dict[str, Any]

    def to_openai_tool_call(self) -> dict[str, Any]:
        """Serialize to an OpenAI-style tool_call payload."""
        return {
            "id": self.id,
            "type": "function",
            "function": {
                "name": self.name,
                "arguments": json.dumps(self.arguments, ensure_ascii=False),
            },
        }


@dataclass
class LLMResponse:
    """Response from an LLM provider."""
    content: str | None
    tool_calls: list[ToolCallRequest] = field(default_factory=list)
    finish_reason: str = "stop"
    usage: dict[str, int] = field(default_factory=dict)
    reasoning_content: str | None = None

    @property
    def has_tool_calls(self) -> bool:
        """Check if response contains tool calls."""
        return len(self.tool_calls) > 0


class LLMProvider(ABC):
    """Abstract base class for LLM providers."""

    _CHAT_RETRY_DELAYS = (1, 2, 4)
    _TRANSIENT_ERROR_MARKERS = (
        "429", "rate limit", "500", "502", "503", "504",
        "overloaded", "timeout", "timed out", "connection",
        "server error", "temporarily unavailable",
    )

    def __init__(self, api_key: str | None = None, api_base: str | None = None):
        self.api_key = api_key
        self.api_base = api_base

    @staticmethod
    def _sanitize_empty_content(messages: list[dict[str, Any]]) -> list[dict[str, Any]]:
        """Sanitize message content: fix empty blocks."""
        result: list[dict[str, Any]] = []
        for msg in messages:
            content = msg.get("content")
            if isinstance(content, str) and not content:
                clean = dict(msg)
                clean["content"] = None
                result.append(clean)
                continue
            result.append(msg)
        return result

    @abstractmethod
    async def chat(
        self,
        messages: list[dict[str, Any]],
        tools: list[dict[str, Any]] | None = None,
        model: str | None = None,
        max_tokens: int = 4096,
        temperature: float = 0.7,
        tool_choice: str | dict[str, Any] | None = None,
    ) -> LLMResponse:
        """Send a chat completion request."""
        pass

    @classmethod
    def _is_transient_error(cls, content: str | None) -> bool:
        err = (content or "").lower()
        return any(marker in err for marker in cls._TRANSIENT_ERROR_MARKERS)

    async def _safe_chat(self, **kwargs: Any) -> LLMResponse:
        """Call chat() and convert unexpected exceptions to error responses."""
        try:
            return await self.chat(**kwargs)
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            return LLMResponse(content=f"Error calling LLM: {exc}", finish_reason="error")

    async def chat_with_retry(
        self,
        messages: list[dict[str, Any]],
        tools: list[dict[str, Any]] | None = None,
        model: str | None = None,
        max_tokens: int = 4096,
        temperature: float = 0.7,
        tool_choice: str | dict[str, Any] | None = None,
    ) -> LLMResponse:
        """Call chat() with retry on transient provider failures."""
        for attempt, delay in enumerate(self._CHAT_RETRY_DELAYS, start=1):
            response = await self._safe_chat(
                messages=messages,
                tools=tools,
                model=model,
                max_tokens=max_tokens,
                temperature=temperature,
                tool_choice=tool_choice,
            )

            if response.finish_reason != "error":
                return response

            if not self._is_transient_error(response.content):
                return response

            logger.warning(
                "LLM transient error (attempt {}/{}), retrying in {}s: {}",
                attempt, len(self._CHAT_RETRY_DELAYS), delay,
                (response.content or "")[:120].lower(),
            )
            await asyncio.sleep(delay)

        return await self._safe_chat(
            messages=messages, tools=tools, model=model,
            max_tokens=max_tokens, temperature=temperature, tool_choice=tool_choice,
        )

    @abstractmethod
    def get_default_model(self) -> str:
        """Get the default model for this provider."""
        pass
