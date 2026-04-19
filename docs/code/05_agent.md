# `agent/` Agent 核心模块

## 文件位置
- `emb-agent/emb_agent/agent/__init__.py`
- `emb-agent/emb_agent/agent/core.py`

## agent/__init__.py - 模块导出

导出 `Agent` 类（即 `AgentCore`）。

---

## agent/core.py - Agent 核心实现

### `Agent` 类
Agent 主类，协调 LLM、工具、消息总线和会话。

#### 初始化
```python
def __init__(
    self,
    workspace: Path,
    provider: LLMProvider,
    model: str,
    max_iterations: int = 40,
    context_window_tokens: int = 200000,
    language: str = "zh",
    reasoning_effort: str | None = None,
)
```

#### 核心属性

| 属性 | 类型 | 说明 |
|------|------|------|
| `workspace` | Path | 工作目录 |
| `provider` | LLMProvider | LLM 提供者 |
| `model` | str | 模型名称 |
| `max_iterations` | int | 最大迭代次数 |
| `language` | str | 响应语言 |

#### 核心方法

**`build_messages()`**：
构建 LLM 消息列表：
1. 添加系统提示词
2. 添加历史消息（受 context_window_tokens 限制）
3. 添加当前用户消息
4. 自动检测用户语言

**`run_agent_loop()`**：
Agent 主循环：
```
while iteration < max_iterations:
    1. 获取工具定义
    2. 调用 LLM (chat_with_retry)
    3. 若有 tool_calls:
       - 记录 LLM 思考过程
       - 执行每个工具调用
       - 收集结果添加到消息列表
       - (MiniMax 需要额外 user 消息触发)
    4. 若无 tool_calls:
       - 提取纯文本响应
       - 返回结果
    5. 更新使用统计和评估数据
```

**`process_message()`**：
处理单条消息的完整流程：
1. 获取或创建会话
2. 构建消息列表
3. 运行 Agent 循环
4. 保存会话历史
5. 返回响应

**`run()`**：
异步运行 Agent：
1. 从 MessageBus 消费入站消息
2. 调用 `process_message()` 处理
3. 发布出站响应
4. 支持 `/stop` 命令停止

**`stop()`**：
停止 Agent 循环。

#### 工具调用流程

```python
# LLM 返回 tool_calls
for tool_call in response.tool_calls:
    tools_used.append(tool_call.name)
    result = await self.tools.execute(tool_call.name, tool_call.arguments)
    messages.append({
        "role": "tool",
        "tool_call_id": tool_call.id,
        "name": tool_call.name,
        "content": result,
    })

# MiniMax 等需要额外触发消息
messages.append({"role": "user", "content": "继续"})
```

#### 思考内容处理
`_strip_think()` 方法移除 `<think>...</think>` 块：
- 用于日志记录和进度回调
- 保留给用户看的响应

#### 评估数据收集
每次 `run_agent_loop()` 完成后记录：
- `timestamp`: ISO 格式时间
- `task`: 任务内容（前100字符）
- `iterations`: 迭代次数
- `tools_used`: 使用的工具列表
- `elapsed_seconds`: 耗时
- `success`: 是否成功

#### Token 使用统计
`_last_usage` 字典记录最近一次 LLM 调用的：
- `prompt_tokens`
- `completion_tokens`

### 与其他组件的交互

```
Agent
├── LLMProvider (chat_with_retry)
├── ToolRegistry (execute, get_definitions)
├── MessageBus (publish_inbound, consume_outbound)
├── SessionManager (get_or_create, save)
└── prompts.py (get_system_prompt, get_tool_continue_message)
```
