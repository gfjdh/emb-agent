"""AnythingLLM API client for RAG queries."""

import json
from typing import Any

import aiohttp
from loguru import logger


class AnythingLLMClient:
    """Client for AnythingLLM REST API."""

    def __init__(self, base_url: str = "http://localhost:3001", api_key: str | None = None):
        self.base_url = base_url.rstrip("/")
        self.api_key = api_key or self._load_api_key()
        self._session: aiohttp.ClientSession | None = None

    @staticmethod
    def _load_api_key() -> str | None:
        import os
        return os.getenv("ANYTHINGLLM_API_KEY")

    async def _get_session(self) -> aiohttp.ClientSession:
        if self._session is None or self._session.closed:
            headers = {}
            if self.api_key:
                headers["Authorization"] = f"Bearer {self.api_key}"
            self._session = aiohttp.ClientSession(headers=headers)
        return self._session

    async def close(self) -> None:
        if self._session and not self._session.closed:
            await self._session.close()

    async def query(
        self,
        query: str,
        top_k: int = 3,
        workspace_id: str = "default",
    ) -> dict[str, Any]:
        """Query AnythingLLM knowledge base.

        Args:
            query: Search query text
            top_k: Number of results to return (1-10)
            workspace_id: AnythingLLM workspace ID

        Returns:
            Dict with success status and results
        """
        top_k = max(1, min(top_k, 10))

        url = f"{self.base_url}/api/v1/{workspace_id}/search"
        payload = {
            "query": query,
            "topN": top_k,
        }

        try:
            session = await self._get_session()
            async with session.post(url, json=payload) as response:
                if response.status == 200:
                    data = await response.json()
                    return self._parse_response(data, top_k)
                else:
                    error_text = await response.text()
                    logger.error("AnythingLLM API error: {} - {}", response.status, error_text)
                    return {
                        "success": False,
                        "error": "api_error",
                        "details": f"HTTP {response.status}: {error_text[:200]}",
                        "results": [],
                        "total_found": 0,
                    }
        except aiohttp.ClientError as e:
            logger.error("AnythingLLM connection failed: {}", e)
            return {
                "success": False,
                "error": "connection_failed",
                "details": str(e),
                "results": [],
                "total_found": 0,
            }
        except Exception as e:
            logger.exception("Unexpected error querying AnythingLLM")
            return {
                "success": False,
                "error": "unexpected_error",
                "details": str(e),
                "results": [],
                "total_found": 0,
            }

    @staticmethod
    def _parse_response(data: dict[str, Any], top_k: int) -> dict[str, Any]:
        """Parse AnythingLLM API response into structured format."""
        results = []

        items = data.get("results", data.get("documents", []))
        if isinstance(items, dict):
            items = list(items.values())

        for item in items[:top_k]:
            result = {
                "content": item.get("content", item.get("text", "")),
                "source": item.get("source", item.get("title", item.get("filename", "unknown"))),
                "score": float(item.get("score", item.get("similarity", 0.0))),
                "metadata": {
                    "page": item.get("page"),
                    "section": item.get("section"),
                    "workspace": item.get("workspace"),
                }
            }
            results.append(result)

        return {
            "success": True,
            "results": results,
            "total_found": len(results),
        }

    async def get_workspaces(self) -> dict[str, Any]:
        """List available workspaces."""
        url = f"{self.base_url}/api/v1/workspaces"
        try:
            session = await self._get_session()
            async with session.get(url) as response:
                if response.status == 200:
                    data = await response.json()
                    return {"success": True, "workspaces": data.get("workspaces", [])}
                else:
                    return {"success": False, "error": f"HTTP {response.status}"}
        except Exception as e:
            return {"success": False, "error": str(e)}
