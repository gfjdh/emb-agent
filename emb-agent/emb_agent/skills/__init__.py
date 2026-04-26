"""Skill base class and loader for dynamic skill management."""

from abc import ABC, abstractmethod
from pathlib import Path
from typing import Any

from loguru import logger


class Skill(ABC):
    """Base class for agent skills."""

    @property
    @abstractmethod
    def name(self) -> str:
        """Skill identifier."""
        pass

    @property
    @abstractmethod
    def description(self) -> str:
        """What this skill does."""
        pass

    @property
    def trigger_keywords(self) -> list[str]:
        """Keywords that suggest this skill should be loaded."""
        return []

    @abstractmethod
    async def execute(self, **kwargs: Any) -> Any:
        """Execute the skill."""
        pass

    def get_skill_md(self) -> str | None:
        """Load SKILL.md content if available."""
        skill_dir = self._get_skill_dir()
        skill_md = skill_dir / "SKILL.md"
        if skill_md.exists():
            return skill_md.read_text(encoding="utf-8")
        return None

    def _get_skill_dir(self) -> Path:
        """Get the directory containing this skill."""
        return Path(__file__).parent / self.name.replace("-", "_")


class SkillLoader:
    """Dynamic loader for agent skills."""

    def __init__(self, skills_dir: Path):
        self.skills_dir = skills_dir
        self._skills: dict[str, Skill] = {}
        self._skill_metadata: dict[str, dict[str, Any]] = {}

    def discover_skills(self) -> None:
        """Scan skills directory and register available skills."""
        if not self.skills_dir.exists():
            logger.warning("Skills directory not found: {}", self.skills_dir)
            return

        for skill_dir in self.skills_dir.iterdir():
            if not skill_dir.is_dir():
                continue

            skill_md = skill_dir / "SKILL.md"
            if not skill_md.exists():
                continue

            try:
                self._load_skill(skill_dir, skill_md)
            except Exception as e:
                logger.error("Failed to load skill from {}: {}", skill_dir, e)

    def _load_skill(self, skill_dir: Path, skill_md: Path) -> None:
        """Load a skill from its directory."""
        skill_name = skill_dir.name

        metadata = {
            "name": skill_name,
            "directory": str(skill_dir),
            "has_md": True,
            "md_path": str(skill_md),
        }

        try:
            import importlib
            module_name = f"emb_agent.skills.{skill_name.replace('-', '_')}"
            module = importlib.import_module(module_name)

            if hasattr(module, "create_skill"):
                skill = module.create_skill()
                self._skills[skill_name] = skill
                metadata["loaded"] = True
                metadata["keywords"] = skill.trigger_keywords
                logger.info("Skill loaded: {} (keywords: {})", skill_name, skill.trigger_keywords)
            else:
                metadata["loaded"] = False
                logger.warning("No create_skill() found in {}", module_name)
        except ImportError as e:
            metadata["loaded"] = False
            metadata["error"] = str(e)
            logger.warning("Could not import skill module {}: {}", skill_name, e)

        self._skill_metadata[skill_name] = metadata

    def get_skill(self, name: str) -> Skill | None:
        """Get a loaded skill by name."""
        return self._skills.get(name)

    def get_available_skills(self) -> list[dict[str, Any]]:
        """List all discovered skills."""
        return list(self._skill_metadata.values())

    def match_skill_by_query(self, query: str) -> list[str]:
        """Find skills that match a query based on keywords."""
        query_lower = query.lower()
        matched = []

        for name, skill in self._skills.items():
            keywords = skill.trigger_keywords
            if any(kw.lower() in query_lower for kw in keywords):
                matched.append(name)

        return matched

    def unload_skill(self, name: str) -> None:
        """Unload a skill from memory."""
        self._skills.pop(name, None)
        logger.info("Skill unloaded: {}", name)

    def reload_skill(self, name: str) -> None:
        """Reload a skill from disk."""
        if name in self._skill_metadata:
            skill_dir = Path(self._skill_metadata[name]["directory"])
            skill_md = skill_dir / "SKILL.md"
            if skill_md.exists():
                self._load_skill(skill_dir, skill_md)
