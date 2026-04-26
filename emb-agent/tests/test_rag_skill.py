"""Tests for RAG Skill and dynamic skill loading."""

import asyncio
import os
import sys
from pathlib import Path
from unittest.mock import AsyncMock, MagicMock, patch

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))


@pytest.fixture
def skills_dir(tmp_path):
    """Create a temporary skills directory."""
    skills = tmp_path / "skills"
    skills.mkdir()
    return skills


@pytest.fixture
def rag_skill_dir(tmp_path):
    """Create a rag_skill directory with SKILL.md."""
    skill_dir = tmp_path / "skills" / "rag_skill"
    skill_dir.mkdir(parents=True)
    (skill_dir / "SKILL.md").write_text("# RAG Skill\nTest skill documentation")
    return skill_dir


class TestSkillLoader:
    """Test cases for SkillLoader."""

    def test_discover_skills_empty_dir(self, skills_dir):
        from emb_agent.skills import SkillLoader
        loader = SkillLoader(skills_dir)
        loader.discover_skills()
        assert len(loader.get_available_skills()) == 0

    def test_discover_skills_with_md(self, rag_skill_dir):
        from emb_agent.skills import SkillLoader
        loader = SkillLoader(rag_skill_dir.parent)
        loader.discover_skills()

        skills = loader.get_available_skills()
        assert len(skills) == 1
        assert skills[0]["name"] == "rag_skill"
        assert skills[0]["has_md"] is True

    def test_match_skill_by_query(self):
        from emb_agent.skills import Skill
        from emb_agent.skills import SkillLoader

        class MockSkill(Skill):
            @property
            def name(self):
                return "test-skill"

            @property
            def description(self):
                return "Test skill"

            @property
            def trigger_keywords(self):
                return ["gpio", "引脚", "飞腾派"]

            async def execute(self, **kwargs):
                return {"success": True}

        loader = SkillLoader(Path("/tmp"))
        loader._skills["test-skill"] = MockSkill()

        matched = loader.match_skill_by_query("飞腾派的 GPIO 引脚定义是什么？")
        assert "test-skill" in matched

        matched = loader.match_skill_by_query("今天天气怎么样？")
        assert len(matched) == 0

    def test_unload_skill(self):
        from emb_agent.skills import SkillLoader
        loader = SkillLoader(Path("/tmp"))
        loader._skills["test"] = MagicMock()

        loader.unload_skill("test")
        assert loader.get_skill("test") is None


class TestRAGSkill:
    """Test cases for RAGSkill."""

    @pytest.mark.asyncio
    async def test_rag_skill_keywords(self):
        from emb_agent.skills.rag_skill import RAGSkill
        skill = RAGSkill(base_url="http://localhost:3001")

        assert "gpio" in skill.trigger_keywords
        assert "飞腾派" in skill.trigger_keywords
        assert "phytium" in skill.trigger_keywords

    @pytest.mark.asyncio
    async def test_rag_skill_name(self):
        from emb_agent.skills.rag_skill import RAGSkill
        skill = RAGSkill()
        assert skill.name == "rag-skill"

    @pytest.mark.asyncio
    async def test_rag_skill_execute_success(self):
        from emb_agent.skills.rag_skill import RAGSkill

        skill = RAGSkill()

        with patch.object(skill._client, 'query', new_callable=AsyncMock) as mock_query:
            mock_query.return_value = {
                "success": True,
                "results": [
                    {
                        "content": "GPIO 引脚定义在 P1 接口",
                        "source": "飞腾派硬件手册.pdf",
                        "score": 0.92,
                    }
                ],
                "total_found": 1,
            }

            result = await skill.execute(query="飞腾派 GPIO 引脚", top_k=1)

            assert result["success"] is True
            assert result["total_found"] == 1
            assert "GPIO 引脚定义" in result["formatted"]

    @pytest.mark.asyncio
    async def test_rag_skill_execute_no_results(self):
        from emb_agent.skills.rag_skill import RAGSkill

        skill = RAGSkill()

        with patch.object(skill._client, 'query', new_callable=AsyncMock) as mock_query:
            mock_query.return_value = {
                "success": True,
                "results": [],
                "total_found": 0,
            }

            result = await skill.execute(query="不存在的关键词", top_k=3)

            assert result["success"] is True
            assert result["total_found"] == 0
            assert "未找到" in result.get("message", "") or "未" in result.get("message", "")

    @pytest.mark.asyncio
    async def test_rag_skill_execute_error(self):
        from emb_agent.skills.rag_skill import RAGSkill

        skill = RAGSkill()

        with patch.object(skill._client, 'query', new_callable=AsyncMock) as mock_query:
            mock_query.return_value = {
                "success": False,
                "error": "connection_failed",
                "details": "Connection refused",
                "results": [],
                "total_found": 0,
            }

            result = await skill.execute(query="测试查询")

            assert result["success"] is False
            assert result["error"] == "connection_failed"


class TestAnythingLLMClient:
    """Test cases for AnythingLLM API client."""

    @pytest.mark.asyncio
    async def test_parse_response_with_results(self):
        from emb_agent.skills.rag_skill.api_client import AnythingLLMClient

        test_data = {
            "results": [
                {
                    "content": "测试内容",
                    "source": "测试文档.pdf",
                    "score": 0.85,
                }
            ]
        }

        result = AnythingLLMClient._parse_response(test_data, top_k=3)

        assert result["success"] is True
        assert len(result["results"]) == 1
        assert result["results"][0]["content"] == "测试内容"
        assert result["results"][0]["score"] == 0.85

    @pytest.mark.asyncio
    async def test_parse_response_with_documents_key(self):
        from emb_agent.skills.rag_skill.api_client import AnythingLLMClient

        test_data = {
            "documents": {
                "doc1": {"text": "文档内容", "title": "文档1", "similarity": 0.75}
            }
        }

        result = AnythingLLMClient._parse_response(test_data, top_k=5)

        assert result["success"] is True
        assert len(result["results"]) == 1

    @pytest.mark.asyncio
    async def test_parse_response_empty(self):
        from emb_agent.skills.rag_skill.api_client import AnythingLLMClient

        result = AnythingLLMClient._parse_response({"results": []}, top_k=3)

        assert result["success"] is True
        assert len(result["results"]) == 0
        assert result["total_found"] == 0


class TestAgentSkillIntegration:
    """Test cases for Agent skill integration."""

    def test_agent_loads_skills(self, tmp_path):
        from emb_agent.skills import SkillLoader
        skills_dir = tmp_path / "skills"
        skills_dir.mkdir()

        loader = SkillLoader(skills_dir)
        loader.discover_skills()

        assert loader is not None
        assert len(loader.get_available_skills()) == 0

    def test_activate_matching_skills(self, tmp_path):
        from emb_agent.skills import Skill, SkillLoader

        class MockRAGSkill(Skill):
            @property
            def name(self):
                return "rag-skill"

            @property
            def description(self):
                return "Mock RAG skill"

            @property
            def trigger_keywords(self):
                return ["gpio", "飞腾派"]

            async def execute(self, **kwargs):
                return {"success": True}

            def get_skill_md(self):
                return "# Mock RAG Skill\nDocumentation"

        skills_dir = tmp_path / "skills"
        skills_dir.mkdir()

        loader = SkillLoader(skills_dir)
        loader._skills["rag-skill"] = MockRAGSkill()

        matched = loader.match_skill_by_query("飞腾派的 GPIO 是什么？")
        assert "rag-skill" in matched

        matched = loader.match_skill_by_query("无关问题")
        assert len(matched) == 0


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
