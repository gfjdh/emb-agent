# emb-agent 代码文档总览

本文档对 emb-agent 项目的每个代码文件进行详细说明，便于后续代码维护和更新时同步修改。

## 项目结构

```
emb-agent/
├── __main__.py           # 程序入口点
├── __init__.py          # 包版本信息
├── prompts.py           # 多语言提示词模板
├── config/
│   ├── __init__.py      # 配置数据模型（Pydantic）
│   └── loader.py        # 配置文件加载/保存
├── bus/
│   ├── __init__.py      # 消息类型定义
│   └── queue.py         # 异步消息队列
├── providers/
│   ├── __init__.py      # Provider 模块导出
│   ├── base.py          # LLM Provider 抽象基类
│   └── litellm_provider.py  # LiteLLM 实现
├── agent/
│   ├── __init__.py      # Agent 模块导出
│   └── core.py          # Agent 核心逻辑
├── tools/
│   ├── __init__.py      # 工具模块导出
│   ├── base.py          # 工具基类
│   ├── registry.py      # 工具注册表
│   ├── filesystem.py    # 文件系统工具
│   ├── shell.py         # Shell 执行工具
│   ├── deploy.py        # SSH 部署工具
│   └── knowledge.py     # 知识库工具
├── session/
│   └── __init__.py      # 会话管理
├── knowledge/
│   └── __init__.py      # 知识库实现
└── pyproject.toml       # 项目配置
```

## 模块依赖关系

```
__main__.py
├── config.loader → Config → config/__init__.py
├── bus.queue → bus/__init__.py
├── providers.litellm_provider → providers/base.py
└── agent.core → agent/__init__.py
    ├── providers (LLM 调用)
    ├── bus (消息队列)
    ├── tools.registry → tools/base.py
    │   ├── tools/filesystem.py
    │   ├── tools/shell.py
    │   ├── tools/deploy.py
    │   └── tools/knowledge.py
    ├── session
    └── knowledge (知识库)
```

## 文档更新说明

当更新某个模块代码时，请同步更新对应的文档文件：

| 模块 | 文档文件 |
|------|----------|
| `__main__.py` | [01_main.md](01_main.md) |
| `config/` | [02_config.md](02_config.md) |
| `bus/` | [03_bus.md](03_bus.md) |
| `providers/` | [04_providers.md](04_providers.md) |
| `agent/` | [05_agent.md](05_agent.md) |
| `tools/` | [06_tools.md](06_tools.md) |
| `session/` | [07_session.md](07_session.md) |
| `knowledge/` | [08_knowledge.md](08_knowledge.md) |
