"""Main entry point for emb-agent."""

import argparse
import asyncio
import os
import sys
from pathlib import Path

from loguru import logger

from emb_agent import __version__
from emb_agent.agent import Agent
from emb_agent.bus import InboundMessage
from emb_agent.bus.queue import MessageBus
from emb_agent.config import Config
from emb_agent.config.loader import load_config, save_config
from emb_agent.providers import LiteLLMProvider


def setup_logging(verbose: bool = False) -> None:
    """Configure logging for the application."""
    logger.remove()
    level = "DEBUG" if verbose else "INFO"
    logger.add(sys.stderr, level=level, format="{time:HH:mm:ss} | {level} | {message}")


async def run_interactive(agent: Agent) -> None:
    """Run agent in interactive CLI mode."""
    bus = agent.bus

    async def input_loop():
        while True:
            try:
                line = await asyncio.get_event_loop().run_in_executor(
                    None, lambda: input("\n> ")
                )
                if line.strip().lower() in ("exit", "quit"):
                    break
                if not line.strip():
                    continue

                msg = InboundMessage(
                    channel="cli",
                    sender_id="user",
                    chat_id="interactive",
                    content=line,
                )
                await bus.publish_inbound(msg)

            except (EOFError, KeyboardInterrupt):
                break

    async def output_loop():
        while True:
            try:
                msg = await bus.consume_outbound()
                print(f"\n{msg.content}")
            except asyncio.CancelledError:
                break

    input_task = asyncio.create_task(input_loop())
    output_task = asyncio.create_task(output_loop())

    msg = InboundMessage(
        channel="cli",
        sender_id="user",
        chat_id="interactive",
        content="Hello! I'm your embedded development assistant. How can I help you today?",
    )
    await bus.publish_inbound(msg)

    try:
        await asyncio.gather(input_task, agent.run())
    except KeyboardInterrupt:
        pass
    finally:
        input_task.cancel()
        output_task.cancel()


async def run_single(agent: Agent, command: str) -> None:
    """Run a single command and return the result."""
    result = await agent.process_message(
        content=command,
        session_key="cli:single",
        channel="cli",
        chat_id="single",
    )
    print(result)


async def run_server(agent: Agent, host: str = "0.0.0.0", port: int = 18792) -> None:
    """Run agent as a server."""
    from aiohttp import web

    bus = agent.bus

    async def handle(request):
        data = await request.json()
        content = data.get("message", "")
        session_key = data.get("session", "http:default")

        msg = InboundMessage(
            channel="http",
            sender_id="user",
            chat_id=session_key,
            content=content,
        )
        await bus.publish_inbound(msg)

        response = await bus.consume_outbound()
        return web.json_response({"response": response.content})

    app = web.Application()
    app.router.add_post("/api/chat", handle)
    app.router.add_get("/health", lambda _: web.json_response({"status": "ok"}))

    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, host, port)
    await site.start()

    logger.info(f"Server running on {host}:{port}")

    try:
        await asyncio.Event().wait()
    except KeyboardInterrupt:
        pass
    finally:
        await runner.cleanup()


def create_default_config(config_path: Path) -> Config:
    """Create and save a default configuration."""
    config = Config()

    api_key = os.environ.get("ANTHROPIC_API_KEY", "")
    if not api_key:
        api_key = os.environ.get("OPENAI_API_KEY", "")
    if not api_key:
        api_key = os.environ.get("DEEPSEEK_API_KEY", "")

    if api_key:
        if "anthropic" in os.environ.get("ANTHROPIC_API_KEY", "").lower() or not os.environ.get("OPENAI_API_KEY"):
            config.providers.anthropic.api_key = api_key
            config.agent.model = "anthropic/claude-sonnet-4-5"
        else:
            config.providers.openai.api_key = api_key
            config.agent.model = "openai/gpt-4o"

    save_config(config, config_path)
    return config


def main() -> None:
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="emb-agent - Embedded System Development Assistant",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--version", action="version", version=f"emb-agent {__version__}")
    parser.add_argument("--config", type=Path, help="Path to configuration file")
    parser.add_argument("--init", action="store_true", help="Initialize configuration")
    parser.add_argument("--verbose", "-v", action="store_true", help="Enable verbose logging")
    parser.add_argument("--command", "-c", type=str, help="Single command to execute")
    parser.add_argument("--model", type=str, help="Override model")
    parser.add_argument("--host", type=str, default="0.0.0.0", help="Server host (default: 0.0.0.0)")
    parser.add_argument("--port", type=int, default=18792, help="Server port (default: 18792)")
    parser.add_argument("--server", action="store_true", help="Run as HTTP server")

    args = parser.parse_args()

    setup_logging(args.verbose)

    config_path = args.config or Path.cwd() / "config.json"

    if args.init or not config_path.exists():
        logger.info("Creating default configuration at {}", config_path)
        config = create_default_config(config_path)
    else:
        config = load_config(config_path)

    if args.model:
        config.agent.model = args.model

    api_key = config.providers.anthropic.api_key or config.providers.openai.api_key
    api_base = config.providers.anthropic.api_base or config.providers.openai.api_base

    if not api_key:
        logger.warning("No API key configured. Set ANTHROPIC_API_KEY or ANTHROPIC_API_KEY in environment.")
        logger.warning("Or edit config.json to set providers.anthropic.api_key or providers.openai.api_key")

    provider = LiteLLMProvider(
        api_key=api_key,
        api_base=api_base,
        default_model=config.agent.model,
    )

    agent = Agent(
        workspace=config.workspace_path,
        provider=provider,
        model=config.agent.model,
        max_iterations=config.agent.max_tool_iterations,
        context_window_tokens=config.agent.context_window_tokens,
    )

    if args.command:
        asyncio.run(run_single(agent, args.command))
        return

    if args.server:
        asyncio.run(run_server(agent, args.host, args.port))
        return

    asyncio.run(run_interactive(agent))

    print("\n" + agent.get_evaluation_report())


if __name__ == "__main__":
    main()
