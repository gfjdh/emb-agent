# `__main__.py` 程序入口模块

## 文件位置
`emb-agent/emb_agent/__main__.py`

## 主要功能

程序的主入口点，负责：
1. 解析命令行参数
2. 配置日志输出
3. 加载配置文件
4. 初始化 LLM Provider 和 Agent
5. 以交互式 CLI、单机命令或 HTTP 服务器模式运行

## 核心函数

### `setup_logging(verbose: bool)`
配置 loguru 日志系统。
- `verbose=True` 时输出 DEBUG 级别日志
- 默认输出 INFO 级别日志
- 格式：`HH:mm:ss | LEVEL | message`

### `run_interactive(agent: Agent)`
交互式 CLI 模式运行。
- 创建两个并发的 asyncio Task：
  - `input_loop()`: 读取用户输入并通过 MessageBus 发布
  - `output_loop()`: 消费 Agent 响应并打印
- 用户输入 `exit/quit` 或发送 EOF/Ctrl+C 时退出
- 启动时发送欢迎消息

### `run_single(agent: Agent, command: str)`
单机命令模式。
- 调用 `agent.process_message()` 处理单条消息
- 直接打印返回结果
- 处理 Unicode 编码错误

### `run_server(agent: Agent, host: str, port: int)`
HTTP 服务器模式。
- 使用 aiohttp 框架创建 Web 服务
- 监听 `/api/chat` POST 请求，接收 JSON `{"message": "...", "session": "..."}`
- 返回 `{"response": "..."}`
- 提供 `/health` 健康检查端点
- 默认端口 18792

### `create_default_config(config_path: Path)`
创建默认配置文件。
- 优先使用 `ANTHROPIC_API_KEY` 环境变量
- 其次尝试 `OPENAI_API_KEY`、`DEEPSEEK_API_KEY`
- 根据 API key 类型自动选择模型：
  - Anthropic key → `anthropic/claude-sonnet-4-5`
  - OpenAI key → `openai/gpt-4o`
- 将配置保存为 JSON 文件

### `main()`
主函数。
1. 配置 stdout/stderr UTF-8 编码
2. 解析命令行参数：
   - `--version`: 显示版本
   - `--config`: 指定配置文件路径
   - `--init`: 初始化配置
   - `--verbose/-v`: 详细日志
   - `--command/-c`: 单命令模式
   - `--model`: 覆盖默认模型
   - `--host`: 服务器地址
   - `--port`: 服务器端口
   - `--server`: HTTP 服务器模式
3. 根据参数选择运行模式
4. 退出时输出评估报告

## 命令行用法

```bash
# 交互式模式
emb-agent

# 单命令模式
emb-agent -c "为飞腾开发板创建LED驱动"

# 服务器模式
emb-agent --server --port 18792

# 初始化配置
emb-agent --init

# 指定配置文件
emb-agent --config /path/to/config.json
```

## 与其他模块的交互

- 使用 `config.loader.load_config()` 加载配置
- 使用 `LiteLLMProvider` 作为 LLM 提供者
- 使用 `MessageBus` 进行消息传递
- 使用 `Agent` 执行主要逻辑
