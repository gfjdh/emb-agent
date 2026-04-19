# emb-agent

嵌入式系统程序优化智能体 (Embedded System Program Optimization Agent) - 一款基于大语言模型 (LLM) 的嵌入式开发智能助手。

## 功能特性

- **UC-01**: 将用户需求解析为结构化的规格说明
- **UC-02**: 从嵌入式开发知识库中检索相关知识
- **UC-03**: 生成完整的项目计划
- **UC-04**: 生成代码文件（包含驱动程序、应用程序、构建脚本）
- **UC-05**: 读取并分析代码文件
- **UC-06**: 修改与改进代码文件
- **UC-07**: 将代码部署到飞腾 (Phytium) 开发板（X暂不考虑）
- **UC-08**: 在目标硬件上执行测试（X暂不考虑）
- **UC-09**: 分析错误日志并诊断问题
- **UC-10**: 提供并应用问题修复方案
- **UC-11**: 生成评估报告

## 安装说明

```bash
pip install -e .
```

## 配置指南

创建一个 `config.json`，或者使用 `--init` 命令生成默认配置：

```bash
emb-agent --init
```

设置您的 API 密钥：

```bash
export ANTHROPIC_API_KEY=your_key_here
# 或者
export OPENAI_API_KEY=your_key_here
```

## 使用方法

### 交互模式

```bash
emb-agent
```

### 单条命令模式

```bash
emb-agent -c "为飞腾开发板创建一个LED测试应用"
```

### 服务端模式

```bash
emb-agent --server --port 18792
```

## 项目结构

```
emb-agent/
├── emb_agent/
│   ├── __init__.py
│   ├── __main__.py
│   ├── agent/
│   │   ├── __init__.py
│   │   └── core.py
│   ├── bus/
│   │   ├── __init__.py
│   │   └── queue.py
│   ├── config/
│   │   ├── __init__.py
│   │   └── loader.py
│   ├── knowledge/
│   │   └── __init__.py
│   ├── providers/
│   │   ├── __init__.py
│   │   ├── base.py
│   │   └── litellm_provider.py
│   ├── session/
│   │   └── __init__.py
│   └── tools/
│       ├── __init__.py
│       ├── base.py
│       ├── deploy.py
│       ├── filesystem.py
│       ├── knowledge.py
│       ├── registry.py
│       └── shell.py
└── pyproject.toml
```

## 可用工具

- `read_file` - 分页读取文件内容
- `write_file` - 向文件写入内容
- `edit_file` - 编辑现有文件
- `list_dir` - 列出目录内容
- `exec` - 执行 Shell 命令
- `retrieve_knowledge` - 查询知识库
- `add_knowledge` - 添加新的知识条目
- `deploy_to_target` - 部署至远程飞腾开发板
- `exec_on_target` - 在远程开发板上执行命令

## 许可证

MIT
