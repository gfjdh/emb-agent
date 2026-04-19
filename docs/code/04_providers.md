# `providers/` LLM 提供者模块

## 文件位置
- `emb-agent/emb_agent/providers/__init__.py`
- `emb-agent/emb_agent/providers/base.py`
- `emb-agent/emb_agent/providers/litellm_provider.py`

## providers/__init__.py - 模块导出

导出以下公共接口：
- `LLMProvider` - 抽象基类
- `LLMResponse` - 响应数据类
- `ToolCallRequest` - 工具调用请求
- `LiteLLMProvider` - LiteLLM 实现

---

## providers/base.py - LLM Provider 基类

### `ToolCallRequest` 数据类
LLM 工具调用请求。

| 属性 | 类型 | 说明 |
|------|------|------|
| `id` | str | 调用 ID |
| `name` | str | 工具名称 |
| `arguments` | dict | 工具参数 |

**方法**：
- `to_openai_tool_call() -> dict`: 序列化为 OpenAI 格式

### `LLMResponse` 数据类
LLM 响应封装。

| 属性 | 类型 | 说明 |
|------|------|------|
| `content` | str \| None | 文本内容 |
| `tool_calls` | list[ToolCallRequest] | 工具调用列表 |
| `finish_reason` | str | 结束原因 ("stop", "error" 等) |
| `usage` | dict | Token 使用统计 |
| `reasoning_content` | str \| None | 推理内容（如有） |

**属性**：
- `has_tool_calls`: 是否包含工具调用

### `LLMProvider` 抽象类

#### 类变量
- `_CHAT_RETRY_DELAYS = (1, 2, 4)`: 重试延迟（秒）
- `_TRANSIENT_ERROR_MARKERS`: 瞬时错误关键字（"429", "rate limit", "500" 等）

#### 初始化
```python
def __init__(self, api_key: str | None = None, api_base: str | None = None)
```

#### 核心方法

**`chat()` (抽象)**：
```python
async def chat(
    self,
    messages: list[dict[str, Any]],
    tools: list[dict[str, Any]] | None = None,
    model: str | None = None,
    max_tokens: int = 4096,
    temperature: float = 0.7,
    tool_choice: str | dict | None = None,
) -> LLMResponse
```
发送聊天完成请求（需子类实现）。

**`_safe_chat()`**：
封装 `chat()` 调用，捕获异常并返回错误响应。

**`chat_with_retry()`**：
带重试的聊天请求：
1. 执行 `chat()` 调用
2. 若返回错误且为瞬时错误，等待后重试
3. 最多重试 3 次（延迟 1s, 2s, 4s）
4. 最终仍失败则返回错误响应

#### 辅助方法

**`_sanitize_empty_content()`**：
清理消息内容中的空字符串块，将 `{"content": ""}` 转为 `{"content": None}`。

**`_is_transient_error()`**：
检查错误是否为瞬时错误（检查关键字）。

---

## providers/litellm_provider.py - LiteLLM 实现

### `LiteLLMProvider` 类
基于 [LiteLLM](https://litellm.vercel.app/) 的多 Provider 支持实现。

#### 初始化
```python
def __init__(
    self,
    api_key: str | None = None,
    api_base: str | None = None,
    default_model: str = "anthropic/claude-sonnet-4-5",
    extra_headers: dict | None = None,
    provider_name: str | None = None,
)
```

#### 主要方法

**`_resolve_model(model: str) -> str`**：
解析模型名称：
- 若设置了 `api_base` 且无 `/`，自动添加 `minimax/` 前缀
- `MiniMax-` 开头或 `MiniMax-M2` 映射到 `minimax/` provider

**`chat()`**：
通过 LiteLLM 发送请求：
1. 调用 `_resolve_model()` 解析模型
2. 确保 `max_tokens >= 1`
3. 构建 LiteLLM kwargs 参数
4. 调用 `acompletion()` 异步接口
5. 解析响应

**`_parse_response()`**：
将 LiteLLM 响应转换为标准 `LLMResponse`：
- 提取文本内容
- 解析 `tool_calls`（处理字符串参数的 JSON 修复）
- 提取 usage 统计
- 提取 reasoning_content（如有）

### 工具调用 ID 生成
使用 `_short_tool_id()` 生成 9 位字母数字随机 ID。

### 依赖
- `litellm`: 多模型 LLM 统一接口
- `json_repair`: JSON 字符串修复（处理不完整的 JSON）
