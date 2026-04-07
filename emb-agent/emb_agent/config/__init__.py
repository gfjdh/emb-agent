"""Configuration schema for emb-agent."""

from pathlib import Path
from typing import Literal

from pydantic import BaseModel, Field


class Base(BaseModel):
    """Base model with alias generation support."""
    model_config = {"populate_by_name": True}


class AgentDefaults(Base):
    """Default agent configuration."""
    workspace: str = "./workspace"
    model: str = "anthropic/claude-sonnet-4-5"
    provider: str = "auto"
    max_tokens: int = 8192
    context_window_tokens: int = 200000
    temperature: float = 0.1
    max_tool_iterations: int = 40
    reasoning_effort: str | None = None
    language: str = "zh"  # "en" for English, "zh" for Chinese


class ProviderConfig(Base):
    """LLM provider configuration."""
    api_key: str = ""
    api_base: str | None = None
    extra_headers: dict[str, str] | None = None


class ProvidersConfig(Base):
    """Configuration for LLM providers."""
    custom: ProviderConfig = Field(default_factory=ProviderConfig)
    anthropic: ProviderConfig = Field(default_factory=ProviderConfig)
    openai: ProviderConfig = Field(default_factory=ProviderConfig)
    openrouter: ProviderConfig = Field(default_factory=ProviderConfig)
    deepseek: ProviderConfig = Field(default_factory=ProviderConfig)
    groq: ProviderConfig = Field(default_factory=ProviderConfig)
    zhipu: ProviderConfig = Field(default_factory=ProviderConfig)
    dashscope: ProviderConfig = Field(default_factory=ProviderConfig)
    ollama: ProviderConfig = Field(default_factory=ProviderConfig)
    gemini: ProviderConfig = Field(default_factory=ProviderConfig)
    moonshot: ProviderConfig = Field(default_factory=ProviderConfig)
    minimax: ProviderConfig = Field(default_factory=ProviderConfig)


class KnowledgeBaseConfig(Base):
    """Knowledge base configuration."""
    enabled: bool = True
    vector_store_path: str = "./workspace/knowledge_vectorstore"
    top_k: int = 5


class TargetBoardConfig(Base):
    """Target board (Phytium development board) configuration."""
    host: str = ""
    port: int = 22
    username: str = "root"
    password: str = ""
    key_path: str | None = None
    target_path: str = "/root/projects"


class ToolsConfig(Base):
    """Tools configuration."""
    exec_timeout: int = 120
    restrict_to_workspace: bool = True


class Config(Base):
    """Root configuration for emb-agent."""
    agent: AgentDefaults = Field(default_factory=AgentDefaults)
    providers: ProvidersConfig = Field(default_factory=ProvidersConfig)
    knowledge_base: KnowledgeBaseConfig = Field(default_factory=KnowledgeBaseConfig)
    target_board: TargetBoardConfig = Field(default_factory=TargetBoardConfig)
    tools: ToolsConfig = Field(default_factory=ToolsConfig)

    @property
    def workspace_path(self) -> Path:
        """Get expanded workspace path."""
        return Path(self.agent.workspace).expanduser().resolve()
