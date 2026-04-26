"""RAG Skill implementation for AnythingLLM knowledge base queries."""

from typing import Any

from loguru import logger

from emb_agent.skills import Skill
from emb_agent.skills.rag_skill.api_client import AnythingLLMClient


class RAGSkill(Skill):
    """Skill for querying AnythingLLM knowledge base."""

    def __init__(self, base_url: str = "http://localhost:3001", api_key: str | None = None):
        self._client = AnythingLLMClient(base_url=base_url, api_key=api_key)
        self._workspace_id = "default"

    @property
    def name(self) -> str:
        return "rag-skill"

    @property
    def description(self) -> str:
        return "检索 AnythingLLM 知识库，获取相关文档片段和技术资料。"

    @property
    def trigger_keywords(self) -> list[str]:
        return [
            "gpio", "引脚", "规格", "参数", "手册", "文档",
            "传感器", "芯片", "飞腾派", "phytium", "phytipi",
            "开发板", "硬件", "电路", "接口", "定义",
            "aht10", "es8336", "yt8521", "ec20",
        ]

    async def execute(self, query: str, top_k: int = 3, workspace_id: str | None = None, **kwargs: Any) -> Any:
        """Execute RAG query.

        Args:
            query: Search query text
            top_k: Number of results (1-10)
            workspace_id: Optional workspace override

        Returns:
            Formatted results or error dict
        """
        ws_id = workspace_id or self._workspace_id
        logger.info("RAG Skill query: '{}' (top_k={})", query, top_k)

        results = await self._client.query(query=query, top_k=top_k, workspace_id=ws_id)

        if not results.get("success"):
            return results

        if not results["results"]:
            return {
                "success": True,
                "results": [],
                "total_found": 0,
                "message": "未在知识库中找到相关信息",
            }

        output = f"找到 {results['total_found']} 条相关信息：\n"
        for i, item in enumerate(results["results"], 1):
            output += f"\n## [{i}] {item['source']}\n"
            output += f"{item['content']}\n"
            if item.get("score"):
                output += f"(相关度: {item['score']:.2f})\n"

        return {
            "success": True,
            "results": results["results"],
            "total_found": results["total_found"],
            "formatted": output,
        }

    async def cleanup(self) -> None:
        """Clean up resources."""
        await self._client.close()


def create_skill() -> Skill:
    """Factory function to create the RAG skill instance."""
    import os
    base_url = os.getenv("ANYTHINGLLM_URL", "http://localhost:3001")
    api_key = os.getenv("ANYTHINGLLM_API_KEY")
    return RAGSkill(base_url=base_url, api_key=api_key)
