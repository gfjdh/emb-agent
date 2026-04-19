# `tools/` 工具系统模块

## 文件位置
- `emb-agent/emb_agent/tools/__init__.py`
- `emb-agent/emb_agent/tools/base.py`
- `emb-agent/emb_agent/tools/registry.py`
- `emb-agent/emb_agent/tools/filesystem.py`
- `emb-agent/emb_agent/tools/shell.py`
- `emb-agent/emb_agent/tools/deploy.py`
- `emb-agent/emb_agent/tools/knowledge.py`

## tools/__init__.py - 模块导出

导出 `Tool` 基类和 `ToolRegistry`。

---

## tools/base.py - 工具基类

### `Tool` 抽象基类

所有工具的基类，提供：
- 参数类型转换
- 参数校验
- JSON Schema 定义

#### 属性（需子类实现）

| 属性 | 类型 | 说明 |
|------|------|------|
| `name` | str | 工具名称 |
| `description` | str | 工具描述 |
| `parameters` | dict | JSON Schema 参数定义 |

#### 核心方法

**`execute(**kwargs) -> Any` (抽象)**：
工具执行逻辑，子类必须实现。

**`cast_params(params: dict) -> dict`**：
类型转换，将字符串参数转换为正确类型：
- `"123"` → `123` (integer)
- `"true"` → `True` (boolean)
- `"false"` → `False` (boolean)

**`validate_params(params: dict) -> list[str]`**：
参数校验，返回错误列表。检查：
- 必需参数是否存在
- 类型是否匹配

**`to_schema() -> dict`**：
转换为 OpenAI function calling 格式。

---

## tools/registry.py - 工具注册表

### `ToolRegistry` 类

管理所有可用工具的注册和执行。

#### 方法

| 方法 | 说明 |
|------|------|
| `register(tool: Tool)` | 注册工具 |
| `unregister(name: str)` | 注销工具 |
| `get(name: str) -> Tool \| None` | 获取工具 |
| `has(name: str) -> bool` | 检查工具是否存在 |
| `get_definitions() -> list[dict]` | 获取所有工具定义 |
| `execute(name: str, params: dict) -> Any` | 执行工具 |

#### 执行流程
```python
async def execute(name: str, params: dict) -> Any:
    tool = get(name)
    if not tool:
        return f"Error: Tool '{name}' not found..."

    params = tool.cast_params(params)  # 类型转换
    errors = tool.validate_params(params)  # 参数校验
    if errors:
        return f"Error: Invalid parameters... {'; '.join(errors)}"

    return await tool.execute(**params)
```

---

## tools/filesystem.py - 文件系统工具

### `_FsTool` 基类
文件系统工具基类，提供路径解析功能。

**`_resolve(path: str) -> Path`**：
解析路径并检查权限。

### `ReadFileTool`

读取文件内容（带行号和分页）。

| 参数 | 类型 | 说明 |
|------|------|------|
| `path` | string | 文件路径 |
| `offset` | integer | 起始行号（1-indexed） |
| `limit` | integer | 最大行数 |

**特性**：
- 返回带行号格式：`"1| content..."`
- 最大字符数 128,000
- 自动分页提示

### `WriteFileTool`

写入文件内容。

| 参数 | 类型 | 说明 |
|------|------|------|
| `path` | string | 文件路径 |
| `content` | string | 文件内容 |

**特性**：
- 自动创建父目录
- UTF-8 编码

### `EditFileTool`

编辑文件（文本替换）。

| 参数 | 类型 | 说明 |
|------|------|------|
| `path` | string | 文件路径 |
| `old_text` | string | 要替换的文本 |
| `new_text` | string | 替换后的文本 |
| `replace_all` | boolean | 全部替换（默认 False） |

### `ListDirTool`

列出目录内容。

| 参数 | 类型 | 说明 |
|------|------|------|
| `path` | string | 目录路径 |
| `recursive` | boolean | 递归列出 |

**特性**：
- 自动忽略：`.git`, `node_modules`, `__pycache__`, `.venv`, `venv`, `dist`, `build`
- 显示 📁/📄 图标

---

## tools/shell.py - Shell 执行工具

### `ExecTool`

执行 shell 命令。

| 参数 | 类型 | 说明 |
|------|------|------|
| `command` | string | shell 命令 |
| `working_dir` | string | 工作目录 |
| `timeout` | integer | 超时秒数（最大600） |

#### 安全机制

**危险命令拦截**（正则匹配）：
```python
deny_patterns = [
    r"\brm\s+-[rf]{1,2}\b",      # rm -rf
    r"\bdel\s+/[fq]\b",          # del /f/q
    r"\brmdir\s+/s\b",           # rmdir /s
    r"format\b",                 # format
    r"\b(mkfs|diskpart)\b",      # mkfs, diskpart
    r"\bdd\s+if=",               # dd if=
    r">\s*/dev/sd",              # > /dev/sd*
    r"\b(shutdown|reboot|poweroff)\b",  # 关机命令
    r":\(\)\s*\{.*\};\s*:",      # fork bomb
]
```

**路径遍历防护**：
- `restrict_to_workspace=True` 时限制在 workspace 内
- 检测 `..\\` 和 `../` 路径穿越

#### 输出限制
- 最大输出 10,000 字符
- 超长时截断中间部分

---

## tools/deploy.py - SSH 部署工具

### `SSHDeployTool`

部署代码到远程目标板。

| 参数 | 类型 | 说明 |
|------|------|------|
| `local_path` | string | 本地目录路径 |
| `remote_path` | string | 远程目标路径 |
| `host` | string | 目标 IP（可选） |

**部署流程**：
1. 将本地目录打包为 `tar.gz`
2. 通过 SCP 上传到目标
3. 清理临时文件

### `SSHExecTool`

在远程目标板上执行命令。

| 参数 | 类型 | 说明 |
|------|------|------|
| `command` | string | 要执行的命令 |
| `host` | string | 目标 IP（可选） |
| `timeout` | integer | 超时秒数 |

**特性**：
- 使用 SSH 密钥认证
- 禁用 StrictHostKeyChecking
- 支持连接超时设置

---

## tools/knowledge.py - 知识库工具

### `KnowledgeRetrievalTool`

从知识库检索相关内容。

| 参数 | 类型 | 说明 |
|------|------|------|
| `query` | string | 搜索查询 |
| `top_k` | integer | 返回结果数 |

**返回格式**：
```
## [1] 标题
内容...

---

## [2] 标题
内容...
```

### `AddKnowledgeTool`

添加新知识条目。

| 参数 | 类型 | 说明 |
|------|------|------|
| `title` | string | 知识标题 |
| `content` | string | 知识内容（markdown） |
| `source` | string | 来源（默认 "user"） |

---

## 工具注册初始化

在 `Agent.__init__` 中初始化并注册：

```python
self.tools = ToolRegistry()
self.tools.register(ReadFileTool(workspace, allowed_dir))
self.tools.register(WriteFileTool(workspace, allowed_dir))
self.tools.register(EditFileTool(workspace, allowed_dir))
self.tools.register(ListDirTool(workspace, allowed_dir))
self.tools.register(ExecTool(timeout, workspace, restrict_to_workspace))
self.tools.register(KnowledgeRetrievalTool(workspace, top_k))
self.tools.register(AddKnowledgeTool(workspace))
self.tools.register(SSHDeployTool(...))
self.tools.register(SSHExecTool(...))
```
