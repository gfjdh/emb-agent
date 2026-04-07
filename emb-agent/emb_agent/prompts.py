"""Prompt templates for emb-agent, supporting multiple languages."""

from enum import Enum
from typing import Callable


class Language(Enum):
    """Supported languages."""
    EN = "en"
    ZH = "zh"


DEFAULT_LANGUAGE = Language.ZH


# ============================================================================
# System Prompts
# ============================================================================

SYSTEM_PROMPTS = {
    Language.EN: """# emb-agent - Embedded System Development Assistant

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

## Language Requirement

**Important: You must always respond in English. Even if the user asks in Chinese or another language, you must reply in English.**
""",

    Language.ZH: """# emb-agent - 嵌入式系统开发助手

你是一位专注于飞腾 (Phytium) ARM 处理器和 Linux 内核开发的嵌入式系统开发专家。

## 你的能力

你帮助用户在飞腾开发板上完成嵌入式应用开发的完整生命周期：
1. 需求分析与项目规划
2. 检索平台相关的技术指导
3. 代码生成（驱动、应用、构建脚本）
4. 部署到目标硬件
5. 测试与调试

## 核心用例

- UC-01: 将用户需求解析为结构化规格说明
- UC-02: 从知识库检索相关信息
- UC-03: 生成完整项目计划
- UC-04: 生成项目代码文件
- UC-05: 读取并分析代码文件
- UC-06: 修改并改进代码文件
- UC-07: 将代码部署到飞腾开发板
- UC-08: 在目标硬件上执行测试
- UC-09: 分析错误日志并诊断问题
- UC-10: 提供并应用问题修复方案
- UC-11: 生成评估报告

## 工作流程

1. 当用户描述项目时（如"为飞腾开发板创建LED闪烁驱动"）：
   - 使用 retrieve_knowledge 检索相关平台文档
   - 生成结构化项目计划
   - 创建所有必要的代码文件
   - 部署到目标并验证

2. 当发生错误时：
   - 使用 LLM 推理分析错误日志
   - 找出根本原因
   - 提出或应用修复方案

3. 在报告完成前始终验证代码正确性

## 可用工具

- read_file: 读取文件内容
- write_file: 向文件写入内容
- edit_file: 编辑现有文件
- list_dir: 列出目录内容
- exec: 执行 shell 命令
- retrieve_knowledge: 查询知识库
- add_knowledge: 添加新知识条目
- deploy_to_target: 部署到远程飞腾开发板
- exec_on_target: 在远程开发板上执行命令

## 安全准则

- 部署前验证所有代码
- 检查开发板连接参数
- 绝不建议危险操作（格式化、覆盖启动扇区）
- 涉及破坏性操作时始终确认用户意图

## 回复风格

- 简洁且技术性
- 提供可工作的代码示例
- 包含编译说明
- 解释平台特定注意事项

## 语言要求

**重要：你必须始终使用中文回复。即使用户使用英文提问，你也必须用中文回答。**
""",
}


# ============================================================================
# Welcome Messages
# ============================================================================

WELCOME_MESSAGES = {
    Language.EN: "Hello! I'm your embedded systems development assistant. How can I help you today?",
    Language.ZH: "你好！我是你的嵌入式开发助手。有什么我可以帮你的吗？",
}


# ============================================================================
# Tool Result Messages
# ============================================================================

TOOL_CONTINUE_MESSAGE = {
    Language.EN: "Continue",
    Language.ZH: "继续",
}


# ============================================================================
# Error Messages
# ============================================================================

ERROR_MESSAGES = {
    Language.EN: {
        "no_api_key": "No API key configured. Please set your LLM API key in config.json",
        "llm_error": "LLM call failed: {error}",
        "max_iterations": "I reached the maximum number of tool call iterations ({max_iterations}) without completing the task.",
        "file_not_found": "File not found: {path}",
        "deployment_failed": "Deployment to target failed: {error}",
        "connection_failed": "Failed to connect to target board: {error}",
    },
    Language.ZH: {
        "no_api_key": "未配置 API 密钥。请在 config.json 中设置你的 LLM API 密钥。",
        "llm_error": "LLM 调用失败: {error}",
        "max_iterations": "已达到最大工具调用次数 ({max_iterations})，未能完成任务。",
        "file_not_found": "文件未找到: {path}",
        "deployment_failed": "部署到目标失败: {error}",
        "connection_failed": "连接目标开发板失败: {error}",
    },
}


# ============================================================================
# Evaluation Report Templates
# ============================================================================

EVALUATION_REPORT_TEMPLATES = {
    Language.EN: {
        "title": "Evaluation Report",
        "summary": "## Summary",
        "total_tasks": "Total tasks: {count}",
        "successful": "Successful: {count} ({percentage:.1f}%)",
        "failed": "Failed: {count}",
        "total_iterations": "Total iterations: {count}",
        "avg_iterations": "Average iterations per task: {avg:.1f}",
        "total_time": "Total time: {time:.1f}s",
        "avg_time": "Average time per task: {time:.1f}s",
        "tool_usage": "## Tool Usage",
        "no_data": "No evaluation data available yet.",
    },
    Language.ZH: {
        "title": "评估报告",
        "summary": "## 概要",
        "total_tasks": "总任务数: {count}",
        "successful": "成功: {count} ({percentage:.1f}%)",
        "failed": "失败: {count}",
        "total_iterations": "总迭代次数: {count}",
        "avg_iterations": "平均每任务迭代次数: {avg:.1f}",
        "total_time": "总耗时: {time:.1f}秒",
        "avg_time": "平均每任务耗时: {time:.1f}秒",
        "tool_usage": "## 工具使用统计",
        "no_data": "暂无评估数据。",
    },
}


# ============================================================================
# Helper Functions
# ============================================================================

def get_system_prompt(language: Language = None) -> str:
    """Get system prompt for the given language."""
    if language is None:
        language = DEFAULT_LANGUAGE
    return SYSTEM_PROMPTS.get(language, SYSTEM_PROMPTS[DEFAULT_LANGUAGE])


def get_welcome_message(language: Language = None) -> str:
    """Get welcome message for the given language."""
    if language is None:
        language = DEFAULT_LANGUAGE
    return WELCOME_MESSAGES.get(language, WELCOME_MESSAGES[DEFAULT_LANGUAGE])


def get_tool_continue_message(language: Language = None) -> str:
    """Get tool continue message for the given language."""
    if language is None:
        language = DEFAULT_LANGUAGE
    return TOOL_CONTINUE_MESSAGE.get(language, TOOL_CONTINUE_MESSAGE[DEFAULT_LANGUAGE])


def get_error_message(key: str, language: Language = None, **kwargs) -> str:
    """Get error message for the given key and language."""
    if language is None:
        language = DEFAULT_LANGUAGE
    msg_dict = ERROR_MESSAGES.get(language, ERROR_MESSAGES[DEFAULT_LANGUAGE])
    msg = msg_dict.get(key, key)
    return msg.format(**kwargs) if kwargs else msg


def get_evaluation_report_text(
    evaluation_data: list,
    language: Language = None,
) -> str:
    """Generate evaluation report text."""
    if language is None:
        language = DEFAULT_LANGUAGE
    templates = EVALUATION_REPORT_TEMPLATES.get(language, EVALUATION_REPORT_TEMPLATES[DEFAULT_LANGUAGE])

    if not evaluation_data:
        return templates["no_data"]

    total_tasks = len(evaluation_data)
    successful = sum(1 for d in evaluation_data if d.get("success", False))
    total_iterations = sum(d.get("iterations", 0) for d in evaluation_data)
    total_time = sum(d.get("elapsed_seconds", 0) for d in evaluation_data)

    tool_usage: dict[str, int] = {}
    for d in evaluation_data:
        for tool in d.get("tools_used", []):
            tool_usage[tool] = tool_usage.get(tool, 0) + 1

    lines = [
        f"# {templates['title']}",
        "",
        f"## {templates['summary']}",
        f"- {templates['total_tasks'].format(count=total_tasks)}",
        f"- {templates['successful'].format(count=successful, percentage=100*successful/total_tasks)}",
        f"- {templates['failed'].format(count=total_tasks - successful)}",
        f"- {templates['total_iterations'].format(count=total_iterations)}",
        f"- {templates['avg_iterations'].format(avg=total_iterations/total_tasks)}",
        f"- {templates['total_time'].format(time=total_time)}",
        f"- {templates['avg_time'].format(time=total_time/total_tasks)}",
        "",
        f"## {templates['tool_usage']}",
    ]

    for tool, count in sorted(tool_usage.items(), key=lambda x: -x[1]):
        lines.append(f"- {tool}: {count}")

    return "\n".join(lines)


def detect_language(text: str) -> Language:
    """Detect language from text. Simple heuristic based on Chinese characters."""
    if not text:
        return DEFAULT_LANGUAGE
    chinese_chars = sum(1 for c in text if '\u4e00' <= c <= '\u9fff')
    total_chars = len(text.strip())
    if total_chars == 0:
        return DEFAULT_LANGUAGE
    return Language.ZH if chinese_chars / total_chars > 0.3 else Language.EN
