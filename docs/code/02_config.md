# `config/` 配置模块

## 文件位置
- `emb-agent/emb_agent/config/__init__.py`
- `emb-agent/emb_agent/config/loader.py`

## config/__init__.py - 配置数据模型

### `Base` 类
Pydantic 基类，启用别名生成支持。

### `AgentDefaults` 类
Agent 默认配置项：
| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `workspace` | str | `"./workspace"` | 工作目录路径 |
| `model` | str | `"anthropic/claude-sonnet-4-5"` | LLM 模型 |
| `provider` | str | `"auto"` | Provider 类型 |
| `max_tokens` | int | `8192` | 最大输出 token 数 |
| `context_window_tokens` | int | `200000` | 上下文窗口大小 |
| `temperature` | float | `0.1` | 采样温度 |
| `max_tool_iterations` | int | `40` | 最大工具调用次数 |
| `reasoning_effort` | str | `None` | 推理 effort 参数 |
| `language` | str | `"zh"` | 响应语言 ("en" 或 "zh") |

### `ProviderConfig` 类
LLM Provider 配置：
| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `api_key` | str | `""` | API 密钥 |
| `api_base` | str | `None` | API 基础 URL |
| `extra_headers` | dict | `None` | 额外 HTTP 头 |

### `ProvidersConfig` 类
支持多 Provider 配置：
- `custom`、`anthropic`、`openai`、`openrouter`、`deepseek`
- `groq`、`zhipu`、`dashscope`、`ollama`、`gemini`
- `moonshot`、`minimax`

### `KnowledgeBaseConfig` 类
知识库配置：
| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `enabled` | bool | `True` | 是否启用 |
| `vector_store_path` | str | `"./workspace/knowledge_vectorstore"` | 向量存储路径 |
| `top_k` | int | `5` | 检索结果数量 |

### `TargetBoardConfig` 类
目标开发板 SSH 配置：
| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `host` | str | `""` | 开发板 IP |
| `port` | int | `22` | SSH 端口 |
| `username` | str | `"root"` | 用户名 |
| `password` | str | `""` | 密码 |
| `key_path` | str | `None` | SSH 私钥路径 |
| `target_path` | str | `"/root/projects"` | 目标路径 |

### `ToolsConfig` 类
工具配置：
| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `exec_timeout` | int | `120` | 命令执行超时(秒) |
| `restrict_to_workspace` | bool | `True` | 限制在工作目录 |

### `Config` 类
根配置类，提供 `workspace_path` 属性返回解析后的 Path 对象。

---

## config/loader.py - 配置加载工具

### 全局变量
- `_current_config_path: Path | None` - 当前配置文件路径

### `set_config_path(path: Path)`
设置全局配置路径。

### `get_config_path() -> Path`
获取配置路径：
- 若已设置，返回设置值
- 否则返回 `cwd() / "config.json"`

### `load_config(config_path: Path | None = None) -> Config`
加载配置文件：
1. 若文件存在，尝试解析 JSON
2. 解析成功返回 `Config` 对象
3. 解析失败记录警告，使用默认配置
4. 文件不存在时返回默认配置

### `save_config(config: Config, config_path: Path | None = None)`
保存配置到文件：
1. 创建父目录（如不存在）
2. 使用 Pydantic `model_dump` 导出 JSON
3. 格式化输出（indent=2, ensure_ascii=False）
