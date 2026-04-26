# Agent 与 RAG 集成（Skill 封装）交付物

> 对应任务：`docs/todo.md` 第 1.3 节 — Agent 与 RAG 集成（将检索封装为 Skill）

---

## 1. rag-skill 完整代码包

### 文件结构

```
emb_agent/skills/rag_skill/
├── SKILL.md          # 技能使用说明
├── __init__.py       # RAGSkill 类 + 工厂函数
└── api_client.py     # AnythingLLM REST API 封装
```

### 各文件说明

| 文件 | 路径 | 说明 |
|------|------|------|
| [SKILL.md](file:///d:/GitHub/emb-agent/emb-agent/emb_agent/skills/rag_skill/SKILL.md) | `emb_agent/skills/rag_skill/SKILL.md` | 技能使用说明文档，包含触发条件、输入参数表、输出 JSON 格式、Python 使用示例、配置要求、错误处理说明 |
| [api_client.py](file:///d:/GitHub/emb-agent/emb-agent/emb_agent/skills/rag_skill/api_client.py) | `emb_agent/skills/rag_skill/api_client.py` | AnythingLLM REST API 调用封装，支持 `query()` 查询、`get_workspaces()` 工作区列表，自动处理认证、错误重试、响应解析 |
| [\_\_init\_\_.py](file:///d:/GitHub/emb-agent/emb-agent/emb_agent/skills/rag_skill/__init__.py) | `emb_agent/skills/rag_skill/__init__.py` | `RAGSkill` 类实现，继承 `Skill` 基类，包含关键词触发列表、执行逻辑、格式化输出、`create_skill()` 工厂函数 |

### 核心功能

- **触发关键词**：gpio、引脚、规格、参数、手册、文档、传感器、芯片、飞腾派、phytium、phytipi、开发板、硬件、电路、接口、定义、aht10、es8336、yt8521、ec20
- **输入参数**：`query`（查询文本）、`top_k`（返回数量，1-10）、`workspace_id`（工作区）
- **输出格式**：结构化 JSON，包含 `success`、`results`（内容+来源+相关度）、`total_found`、`formatted`（可读文本）

---

## 2. Skill 动态加载框架

| 文件 | 路径 | 说明 |
|------|------|------|
| [skills/\_\_init\_\_.py](file:///d:/GitHub/emb-agent/emb-agent/emb_agent/skills/__init__.py) | `emb_agent/skills/__init__.py` | 包含 `Skill` 抽象基类和 `SkillLoader` 动态加载器 |

### SkillLoader 功能

| 方法 | 说明 |
|------|------|
| `discover_skills()` | 扫描 `skills/` 目录，发现所有含 `SKILL.md` 的技能 |
| `_load_skill()` | 通过 `importlib` 动态导入技能模块，调用 `create_skill()` 实例化 |
| `get_skill(name)` | 按名称获取已加载的技能实例 |
| `get_available_skills()` | 列出所有已发现的技能及其元数据 |
| `match_skill_by_query(query)` | 根据查询文本的关键词匹配应激活的技能 |
| `unload_skill(name)` | 从内存中卸载指定技能 |
| `reload_skill(name)` | 从磁盘重新加载技能（支持热更新） |

---

## 3. Agent Core 集成

在 [core.py](file:///d:/GitHub/emb-agent/emb-agent/emb_agent/agent/core.py) 中新增以下功能：

### 新增属性

| 属性 | 说明 |
|------|------|
| `self.skills` | `SkillLoader` 实例，Agent 启动时自动扫描并加载技能 |
| `self._active_skill_context` | 字典，存储已激活技能的 SKILL.md 内容 |

### 新增方法

| 方法 | 说明 |
|------|------|
| `_activate_matching_skills(query)` | 根据用户查询匹配关键词，加载对应技能的 SKILL.md 到上下文 |
| `_execute_skill(skill_name, **kwargs)` | 执行已加载的技能，返回结果 |

### 修改的方法

| 方法 | 修改内容 |
|------|----------|
| `build_system_prompt()` | 注入已激活技能的 SKILL.md 内容（前 500 字符）到 system prompt |
| `process_message()` | 在处理消息前调用 `_activate_matching_skills()` 激活匹配技能 |

### 按需加载机制

```
用户提问 → _activate_matching_skills() → 关键词匹配 → 加载 SKILL.md 到上下文 → 缓存 → 构建 system prompt
```

---

## 4. 测试用例

| 文件 | 路径 |
|------|------|
| [test_rag_skill.py](file:///d:/GitHub/emb-agent/emb-agent/tests/test_rag_skill.py) | `tests/test_rag_skill.py` |

### 测试覆盖（14 个测试全部通过）

| 测试类 | 测试项 | 状态 |
|--------|--------|------|
| TestSkillLoader | `test_discover_skills_empty_dir` | ✅ |
| TestSkillLoader | `test_discover_skills_with_md` | ✅ |
| TestSkillLoader | `test_match_skill_by_query` | ✅ |
| TestSkillLoader | `test_unload_skill` | ✅ |
| TestRAGSkill | `test_rag_skill_keywords` | ✅ |
| TestRAGSkill | `test_rag_skill_name` | ✅ |
| TestRAGSkill | `test_rag_skill_execute_success` | ✅ |
| TestRAGSkill | `test_rag_skill_execute_no_results` | ✅ |
| TestRAGSkill | `test_rag_skill_execute_error` | ✅ |
| TestAnythingLLMClient | `test_parse_response_with_results` | ✅ |
| TestAnythingLLMClient | `test_parse_response_with_documents_key` | ✅ |
| TestAnythingLLMClient | `test_parse_response_empty` | ✅ |
| TestAgentSkillIntegration | `test_agent_loads_skills` | ✅ |
| TestAgentSkillIntegration | `test_activate_matching_skills` | ✅ |

---

## 5. 端到端流程演示

**场景**：用户提问"飞腾派的 GPIO 引脚定义是什么？"

```
1. 用户发送问题
       ↓
2. Agent.process_message() 调用 _activate_matching_skills()
       ↓
3. 检测到关键词 "飞腾派"、"GPIO"、"引脚"
       ↓
4. 加载 rag-skill 的 SKILL.md 到 _active_skill_context
       ↓
5. build_system_prompt() 将 SKILL.md 内容注入 system prompt
       ↓
6. LLM 根据 SKILL.md 说明，决定调用 RAG Skill
       ↓
7. _execute_skill() 执行 RAGSkill.execute(query="飞腾派的 GPIO 引脚定义是什么？")
       ↓
8. api_client.query() 向 AnythingLLM 发送 REST API 请求
       ↓
9. AnythingLLM 返回检索结果（文档片段+来源+相关度）
       ↓
10. RAGSkill 格式化结果并返回
       ↓
11. Agent 生成最终回答给用户
```

---

## 6. 环境变量配置

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `ANYTHINGLLM_URL` | `http://localhost:3001` | AnythingLLM 服务地址 |
| `ANYTHINGLLM_API_KEY` | 无 | AnythingLLM API 认证密钥 |

---

## 7. 扩展新 Skill

要添加新技能，只需：

1. 在 `emb_agent/skills/` 下创建新目录（如 `my-skill/`）
2. 创建 `SKILL.md` 使用说明
3. 创建 `__init__.py`，实现 `Skill` 子类并提供 `create_skill()` 工厂函数
4. Agent 启动时自动发现并加载
