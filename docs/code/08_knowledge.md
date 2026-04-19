# `knowledge/` 知识库模块

## 文件位置
`emb-agent/emb_agent/knowledge/__init__.py`

## KnowledgeBase 类

简单基于关键词匹配的知识检索系统。

### 初始化

```python
def __init__(self, workspace: Path, top_k: int = 5)
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `workspace` | Path | 工作目录 |
| `top_k` | int | 默认返回结果数 |

### 存储结构

```
workspace/
└── knowledge/
    ├── phytium_gpio.md
    ├── linux_kernel_driver.md
    └── phytium_platform.md
```

每 `.md` 文件对应一个知识条目，文件名为 `file` 字段，标题从文件名生成（`.stem`）。

### 默认知识库

初始化时若 `knowledge/` 目录为空，自动创建三个默认条目：

#### 1. Phytium GPIO Driver Development
- GPIO API 介绍（gpio_request, gpio_free 等）
- LED 驱动示例代码
- Device Tree binding 示例

#### 2. Linux Kernel Driver Development
- Linux 内核模块结构
- Makefile 模板
- 编译步骤

#### 3. Phytium Platform Development
- 处理器系列介绍（D2000, S2500, FT-2000）
- 启动流程
- Buildroot 配置
- 交叉编译指南

### 核心方法

#### `_load_knowledge()`
加载所有 `.md` 文件到内存：
```python
for kb_file in self.knowledge_dir.glob("*.md"):
    content = kb_file.read_text(encoding="utf-8")
    title = kb_file.stem.replace("_", " ").replace("-", " ")
    self._index.append({
        "title": title,
        "content": content,
        "file": str(kb_file),
    })
```

#### `retrieve(query: str, top_k: int | None = None) -> list[dict]`
检索知识：
1. 将查询分词
2. 对每个知识条目评分：
   - 查询词出现在内容中：+1 分
   - 查询词出现在标题中：+2 分
3. 按分数降序排序
4. 返回前 k 条

**返回格式**：
```python
[
    {"title": "标题", "content": "内容", "file": "文件名.md"},
    ...
]
```

#### `add_knowledge(title: str, content: str, source: str = "user")`
添加新知识：
1. 从标题生成安全文件名
2. 写入 `.md` 文件
3. 更新内存索引

**文件名生成规则**：
- 转小写
- 空格替换为 `_`
- 斜杠替换为 `_`
- 截断至 50 字符

### 与工具的集成

`KnowledgeRetrievalTool` 和 `AddKnowledgeTool` 延迟导入 `KnowledgeBase`：

```python
def _get_kb(self):
    if self._kb is None:
        from emb_agent.knowledge import KnowledgeBase
        self._kb = KnowledgeBase(self.workspace, self.top_k)
    return self._kb
```

### 检索算法说明

当前使用简单的关键词匹配：
- 不支持向量嵌入
- 不支持语义相似度
- 区分标题匹配（2分）和内容匹配（1分）
- 大小写不敏感

这种设计适合：
- 小规模知识库（< 1000 条）
- 快速原型
- 不需要额外依赖（无向量数据库）
