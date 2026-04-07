"""Knowledge retrieval tool."""

from pathlib import Path
from typing import Any

from emb_agent.tools.base import Tool


class KnowledgeRetrievalTool(Tool):
    """Tool to retrieve relevant knowledge from the knowledge base."""

    def __init__(self, workspace: Path, top_k: int = 5):
        self.workspace = workspace
        self.top_k = top_k
        self._kb = None

    @property
    def name(self) -> str:
        return "retrieve_knowledge"

    @property
    def description(self) -> str:
        return "Retrieve relevant knowledge from the embedded development knowledge base."

    @property
    def parameters(self) -> dict[str, Any]:
        return {
            "type": "object",
            "properties": {
                "query": {
                    "type": "string",
                    "description": "The query to search knowledge base",
                },
                "top_k": {
                    "type": "integer",
                    "description": "Number of results to return (default 5)",
                    "minimum": 1,
                    "maximum": 20,
                },
            },
            "required": ["query"],
        }

    def _get_kb(self):
        if self._kb is None:
            from emb_agent.knowledge import KnowledgeBase
            self._kb = KnowledgeBase(self.workspace, self.top_k)
        return self._kb

    async def execute(self, query: str, top_k: int | None = None, **kwargs: Any) -> str:
        try:
            kb = self._get_kb()
            results = kb.retrieve(query, top_k or self.top_k)

            if not results:
                return "No relevant knowledge found for the query."

            output_parts = []
            for i, item in enumerate(results, 1):
                output_parts.append(f"## [{i}] {item['title']}\n{item['content']}")

            return "\n\n---\n\n".join(output_parts)
        except Exception as e:
            return f"Error retrieving knowledge: {str(e)}"


class AddKnowledgeTool(Tool):
    """Tool to add new knowledge to the knowledge base."""

    def __init__(self, workspace: Path):
        self.workspace = workspace
        self._kb = None

    @property
    def name(self) -> str:
        return "add_knowledge"

    @property
    def description(self) -> str:
        return "Add new knowledge to the embedded development knowledge base."

    @property
    def parameters(self) -> dict[str, Any]:
        return {
            "type": "object",
            "properties": {
                "title": {
                    "type": "string",
                    "description": "Title of the knowledge entry",
                },
                "content": {
                    "type": "string",
                    "description": "Content of the knowledge entry (markdown)",
                },
                "source": {
                    "type": "string",
                    "description": "Source of the knowledge (e.g., 'user', 'manual')",
                },
            },
            "required": ["title", "content"],
        }

    def _get_kb(self):
        if self._kb is None:
            from emb_agent.knowledge import KnowledgeBase
            self._kb = KnowledgeBase(self.workspace)
        return self._kb

    async def execute(self, title: str, content: str, source: str = "user", **kwargs: Any) -> str:
        try:
            kb = self._get_kb()
            kb.add_knowledge(title, content, source)
            return f"Successfully added knowledge: {title}"
        except Exception as e:
            return f"Error adding knowledge: {str(e)}"
