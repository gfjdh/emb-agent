# `session/` 会话管理模块

## 文件位置
`emb-agent/emb_agent/session/__init__.py`

## SessionManager 类

会话管理器，负责会话的创建、存储和历史管理。

### 核心功能

| 功能 | 说明 |
|------|------|
| 会话缓存 | 内存中缓存已加载的会话 |
| 持久化 | JSONL 格式存储 |
| 历史管理 | 获取/保存消息历史 |

### 数据结构

**Session 对象**：
| 属性 | 类型 | 说明 |
|------|------|------|
| `key` | str | 会话唯一标识 |
| `messages` | list[dict] | 消息历史 |
| `created_at` | datetime | 创建时间 |
| `updated_at` | datetime | 更新时间 |

**消息格式**（JSONL 每行）：
```json
{"role": "user", "content": "消息内容", "timestamp": "ISO时间"}
{"role": "assistant", "content": "响应内容", "timestamp": "ISO时间"}
```

### 关键方法

**`get_or_create(session_key: str) -> Session`**：
获取或创建会话：
1. 检查内存缓存
2. 若存在，返回缓存
3. 否则从磁盘加载
4. 都不存在则创建新会话

**`save(session: Session)`**：
保存会话到磁盘：
1. 追加每条消息为一行 JSONL
2. 更新会话的 `updated_at`

**`get_history(session: Session, max_messages: int = 0) -> list[dict]`**：
获取消息历史：
- `max_messages=0`: 返回全部历史
- `max_messages>0`: 只返回最近 N 条

### 存储位置

会话文件存储在：`workspace/sessions/{session_key}.jsonl`

### 与 Agent 的交互

```python
# Agent.process_message()
session = self.sessions.get_or_create(session_key)
history = session.get_history(max_messages=0)
initial_messages = self.build_messages(history=history, current_message=content, ...)
# ... 运行 agent loop ...
for m in all_msgs[1:]:
    if m.get("role") in ("user", "assistant"):
        session.messages.append(m)
self.sessions.save(session)
```

### 设计考虑

1. **JSONL 格式**：便于追加写入，适合频繁更新的会话
2. **内存缓存**：避免重复读取磁盘
3. **消息过滤**：只保存 user/assistant 角色消息，过滤 tool 角色
