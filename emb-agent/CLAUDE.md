# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

emb-agent is an LLM-powered embedded systems development assistant for Phytium (飞腾) ARM processors. It helps users through the full embedded development lifecycle: requirement analysis, knowledge retrieval, code generation, deployment, testing, and debugging.

## Architecture

```
emb_agent/
├── agent/core.py       # Core agent loop - message processing, LLM calls, tool orchestration
├── providers/          # LLM provider abstraction (LiteLLM for multi-provider support)
├── tools/              # Tool implementations (filesystem, shell, knowledge, SSH deploy)
├── knowledge/          # Simple keyword-based knowledge base (markdown files)
├── bus/                # Async message queue for decoupled communication
├── session/            # Conversation session management (JSONL persistence)
├── config/             # Pydantic-based configuration schema
└── prompts.py          # Multilingual system prompts (EN/ZH)
```

### Agent Loop (`agent/core.py`)

The `AgentCore` class runs an async loop that:
1. Receives messages via `MessageBus`
2. Builds LLM messages using system prompts from `prompts.py`
3. Calls the LLM via `LiteLLMProvider` with tool definitions
4. Executes tools through `ToolRegistry` when the LLM requests them
5. Returns responses and saves session history

### Tool System

Tools inherit from `Tool` base class and are registered in `ToolRegistry`. Each tool:
- Defines name, description, and JSON Schema parameters
- Implements `execute()` async method
- Has built-in parameter validation and type casting

Available tools:
- `read_file` - Paginated file reading with line numbers
- `write_file` - File creation with parent directory auto-creation
- `edit_file` - Text replacement (single or replace_all)
- `list_dir` - Directory listing with ignore patterns
- `exec` - Shell command execution with safety guards
- `retrieve_knowledge` - Search knowledge base by keyword
- `add_knowledge` - Add new markdown entries to knowledge base
- `deploy_to_target` - SCP deployment to remote Phytium board
- `exec_on_target` - SSH command execution on remote board

### Knowledge Base

Located at `workspace/knowledge/`. Each `.md` file is a knowledge entry with title derived from filename. Retrieval uses simple keyword scoring (title matches score higher). Default entries cover Phytium GPIO, Linux kernel drivers, and platform development.

### Session Management

Sessions are stored as JSONL files in `workspace/sessions/`. Each line is a JSON message with role/content/timestamp. Sessions are cached in memory and persisted after each agent response.

## Commands

### Installation
```bash
pip install -e .
```

### Configuration
```bash
emb-agent --init  # Generate default config.json
# Edit config.json to set API keys and target board
```

### Usage Modes
```bash
# Interactive CLI
emb-agent

# Single command
emb-agent -c "为飞腾开发板创建一个LED测试应用"

# Server mode (port 18792)
emb-agent --server --port 18792
```

### API Keys
```bash
export ANTHROPIC_API_KEY=your_key_here
# or
export OPENAI_API_KEY=your_key_here
```

## Configuration Schema

Key config sections in `config.json`:
- `agent.model` - LLM model (default: anthropic/claude-sonnet-4-5)
- `agent.language` - Response language ("en" or "zh")
- `target_board` - SSH connection settings for Phytium board
- `knowledge_base.top_k` - Number of knowledge results to retrieve

## Key Implementation Notes

- Uses `litellm` for multi-provider LLM support (Anthropic, OpenAI, MiniMax, DeepSeek, etc.)
- Async throughout via `asyncio` - all tools are `async def execute()`
- Safety guards in `exec` tool block dangerous commands (rm -rf/, format, dd, etc.)
- Path resolution in filesystem tools respects `workspace` and `allowed_dir` constraints
- Tool parameter validation uses JSON Schema with type coercion for string→int/bool
