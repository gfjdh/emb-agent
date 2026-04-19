# `bus/` 消息总线模块

## 文件位置
- `emb-agent/emb_agent/bus/__init__.py`
- `emb-agent/emb_agent/bus/queue.py`

## bus/__init__.py - 消息类型定义

### `InboundMessage` 类
入站消息（用户/系统 → Agent）。

| 属性 | 类型 | 说明 |
|------|------|------|
| `channel` | str | 消息渠道 ("cli", "http" 等) |
| `sender_id` | str | 发送者 ID |
| `chat_id` | str | 聊天会话 ID |
| `content` | str | 消息内容 |
| `timestamp` | datetime | 时间戳（自动生成） |
| `media` | list[str] | 附件列表 |
| `metadata` | dict | 额外元数据 |

**属性**：
- `session_key`: 返回 `"{channel}:{chat_id}"`，用于会话标识

### `OutboundMessage` 类
出站消息（Agent → 用户）。

| 属性 | 类型 | 说明 |
|------|------|------|
| `channel` | str | 消息渠道 |
| `chat_id` | str | 聊天会话 ID |
| `content` | str | 消息内容 |
| `metadata` | dict | 额外元数据 |

---

## bus/queue.py - 异步消息队列

### `MessageBus` 类
线程安全的异步消息队列。

#### 初始化
```python
def __init__(self):
    self.inbound: asyncio.Queue[InboundMessage] = asyncio.Queue()
    self.outbound: asyncio.Queue[OutboundMessage] = asyncio.Queue()
```

#### 核心方法

**入站消息操作**：
- `publish_inbound(msg: InboundMessage)`: 发布入站消息到队列
- `consume_inbound() -> InboundMessage`: 消费下一条入站消息

**出站消息操作**：
- `publish_outbound(msg: OutboundMessage)`: 发布出站消息到队列
- `consume_outbound() -> OutboundMessage`: 消费下一条出站消息

#### 属性
- `inbound_size`: 入站队列待处理消息数
- `outbound_size`: 出站队列待处理消息数

#### 工作流程

```
用户输入 ──→ publish_inbound() ──→ inbound Queue ──→ consume_inbound()
                                                              │
                                                              ▼
                                                       Agent 处理
                                                              │
                                                              ▼
Agent 响应 ──→ publish_outbound() ──→ outbound Queue ──→ consume_outbound()
                                                              │
                                                              ▼
                                                           用户看到
```

## 设计要点

1. **解耦通信**：生产者/消费者通过队列解耦，无需直接引用
2. **异步处理**：所有操作均为 async，支持高并发
3. **多会话支持**：通过 `chat_id` 区分不同会话
4. **超时机制**：Agent 端使用 `asyncio.wait_for` 设置消费超时
