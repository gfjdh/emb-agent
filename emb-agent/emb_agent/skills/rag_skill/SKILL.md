# RAG Skill - 知识库检索技能

## 概述
此技能提供对 AnythingLLM 知识库的检索能力，通过 REST API 查询相关文档片段。

## 触发条件
当用户问题涉及以下情况时自动触发：
- 询问硬件规格、引脚定义、芯片参数
- 查询开发板使用手册、配置方法
- 需要参考官方文档或技术规格书
- 问题中包含具体产品型号（如"飞腾派"、"GPIO"、"AHT10"等）

## 输入参数

| 参数 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| query | string | 是 | - | 查询文本/关键词 |
| top_k | integer | 否 | 3 | 返回结果数量（1-10） |
| workspace_id | string | 否 | default | AnythingLLM 工作区 ID |

## 输出格式
```json
{
  "success": true,
  "results": [
    {
      "content": "文档片段内容...",
      "source": "文档名称.pdf",
      "score": 0.85,
      "metadata": {
        "page": 5,
        "section": "2.3 GPIO 接口"
      }
    }
  ],
  "total_found": 3
}
```

## 使用示例

### 基础查询
```python
from emb_agent.skills.rag_skill.api_client import AnythingLLMClient

client = AnythingLLMClient(base_url="http://localhost:3001", api_key="your-api-key")
results = await client.query("飞腾派的 GPIO 引脚定义是什么？", top_k=3)
```

### 指定工作区
```python
results = await client.query(
    query="AHT10 温湿度传感器工作电压",
    top_k=5,
    workspace_id="phytium-docs"
)
```

## 配置要求
1. AnythingLLM 服务运行在指定端口
2. API Key 已正确配置
3. 知识库已上传并索引相关文档

## 错误处理
- 连接失败：返回 `{"success": false, "error": "connection_failed"}`
- 无结果：返回 `{"success": true, "results": [], "total_found": 0}`
- API 错误：返回 `{"success": false, "error": "api_error", "details": "..."}`
