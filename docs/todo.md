# NPU边缘推理演示项目 + 嵌入式程序优化Agent - 开发计划

- 修订时间: 2026-4-28

---

## 一、项目背景与目标

### 1.1 边缘推理盒子

**目标**: 基于飞腾派开发板，部署小型目标检测模型，实现静态图片推理，对外提供API服务

**现状**:
- 硬件: 飞腾派开发板 x1 (无外设)
- 资料: 百度网盘200G资料库 (需筛选)
- 目标: 边缘推理盒子，API调用方式

### 1.2 嵌入式程序优化Agent

**目标**: 通过SSH连接飞腾派，读取RAG知识库，结合项目代码分析性能瓶颈并生成优化建议

**现状**:
- RAG知识库检索（已完成框架）
- SSH远程连接板子（已有工具，待完善）
- 读取板子性能指标（待实现）
- 分析项目代码性能瓶颈（待实现）

---

## 二、已完成内容

### 2.1 Agent框架 ✅

| 模块 | 文件 | 状态 | 说明 |
|------|------|------|------|
| Agent Core | `emb_agent/agent/core.py` | ✅ 完成 | 主循环、消息处理、工具编排 |
| LLM Provider | `emb_agent/providers/` | ✅ 完成 | 支持MiniMax、Anthropic等多提供商 |
| 工具注册 | `emb_agent/tools/registry.py` | ✅ 完成 | ToolRegistry统一管理 |
| 会话管理 | `emb_agent/session/` | ✅ 完成 | JSONL持久化 |

### 2.2 工具集 ✅

| 工具 | 文件 | 状态 | 说明 |
|------|------|------|------|
| read_file | `emb_agent/tools/filesystem.py` | ✅ 完成 | 分页读取文件 |
| write_file | `emb_agent/tools/filesystem.py` | ✅ 完成 | 文件写入 |
| edit_file | `emb_agent/tools/filesystem.py` | ✅ 完成 | 文本替换编辑 |
| list_dir | `emb_agent/tools/filesystem.py` | ✅ 完成 | 目录列表 |
| exec | `emb_agent/tools/shell.py` | ✅ 完成 | 本地命令执行（安全守卫） |
| retrieve_knowledge | `emb_agent/tools/knowledge.py` | ✅ 完成 | 知识库检索 |
| add_knowledge | `emb_agent/tools/knowledge.py` | ✅ 完成 | 添加知识条目 |
| deploy_to_target | `emb_agent/tools/deploy.py` | 未验证 | SCP部署到远程板子 |
| exec_on_target | `emb_agent/tools/deploy.py` | 未验证 | SSH远程执行命令 |

### 2.3 RAG Skill（集成到Agent）✅

| 文件 | 状态 | 说明 |
|------|------|------|
| `emb_agent/skills/rag_skill/SKILL.md` | ✅ 完成 | 技能使用说明 |
| `emb_agent/skills/rag_skill/api_client.py` | ✅ 完成 | AnythingLLM REST API封装 |
| `emb_agent/skills/rag_skill/__init__.py` | ✅ 完成 | RAGSkill类实现 |
| `emb_agent/skills/__init__.py` | ✅ 完成 | SkillLoader动态加载框架 |

**RAG当前状态**: 框架完成，但知识库内容待补充

---

## 三、待完成内容

### 3.1 嵌入式程序优化Agent

#### 3.1.1 RAG知识库完善 🔲

**内容**:
- [ ] 待补充

**验收标准**: AnythingLLM中可检索到飞腾派开发板完整技术资料

#### 3.1.2 SSH性能监控工具 🔲

**需要实现**:
- [ ] 板子SSH连接配置（当前config.json中host为空）
- [ ] 性能指标采集工具:
  - [ ] CPU使用率 `top -bn1 | head -20`
  - [ ] 内存使用 `free -m`
  - [ ] 磁盘IO `iostat -x 1 5`
  - [ ] 网络状态 `netstat -tuln` / `ss -tuln`
  - [ ] 进程列表 `ps aux --sort=-%cpu | head -20`
  - [ ] 内核日志 `dmesg | tail -50`
  - [ ] 系统负载 `uptime`
  - [ ] 温度（如适用）`cat /sys/class/thermal/thermal_zone*/temp`
- [ ] 封装为Agent工具: `get_board_metrics()`

**文件**: `emb_agent/tools/monitor.py`（新建）

#### 3.1.3 性能瓶颈分析Agent 🔲

**需要实现**:
- [ ] 板子性能测试脚本
- [ ] 性能数据采集工作流
- [ ] 性能数据解析模块
- [ ] 代码分析工具（读取项目代码）
- [ ] 瓶颈识别、优化建议生成（结合RAG知识库）
- [ ] 生成性能分析报告、给出优化建议

**核心流程**:
```
用户: "分析一下当前板子性能瓶颈"
  → SSH连接板子 → 执行板子推理性能测试脚本 → 采集性能指标 → 结合项目代码、知识库分析 → 生成优化建议
```

### 3.2 边缘推理盒子

#### 3.2.1 开发环境准备 🔲

- [ ] 飞腾派Ubuntu系统初始化
- [ ] 开发板网络配置（SSH访问）
- [ ] 交叉编译工具链安装
- [ ] OpenCV/OpenGL等依赖安装

#### 3.2.2 模型部署框架 🔲

- [ ] 模型格式选择（ONNX/TFLite/自定义）
- [ ] 推理引擎选型（NCNN/OpenCV DNN/供应商SDK）
- [ ] 基础图像处理pipeline
- [ ] API服务框架（Flask/FastAPI）

#### 3.2.3 图像识别模型 🔲

- [ ] 模型选型（yolov5s/yolov8s/mobilenet-ssd等）
- [ ] 模型转换（从PyTorch/TensorFlow转到飞腾派可执行格式）
- [ ] 模型优化（量化/剪枝）
- [ ] 静态图片推理测试

#### 3.2.4 API服务开发 🔲

- [ ] 图片上传接口
- [ ] 推理调用
- [ ] 结果返回
- [ ] 性能测试