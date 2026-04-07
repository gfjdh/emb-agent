"""Configuration loading utilities."""

import json
from pathlib import Path

from loguru import logger

from emb_agent.config import Config


_current_config_path: Path | None = None


def set_config_path(path: Path) -> None:
    """Set the current config path."""
    global _current_config_path
    _current_config_path = path


def get_config_path() -> Path:
    """Get the configuration file path."""
    if _current_config_path:
        return _current_config_path
    return Path.cwd() / "config.json"


def load_config(config_path: Path | None = None) -> Config:
    """Load configuration from file or create default."""
    path = config_path or get_config_path()

    if path.exists():
        try:
            with open(path, encoding="utf-8") as f:
                data = json.load(f)
            return Config.model_validate(data)
        except (json.JSONDecodeError, ValueError) as e:
            logger.warning(f"Failed to load config from {path}: {e}")
            logger.warning("Using default configuration.")

    return Config()


def save_config(config: Config, config_path: Path | None = None) -> None:
    """Save configuration to file."""
    path = config_path or get_config_path()
    path.parent.mkdir(parents=True, exist_ok=True)

    data = config.model_dump(mode="json", by_alias=True)

    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
