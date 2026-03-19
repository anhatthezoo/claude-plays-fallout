import logging
import argparse

from agent import Agent
from ipc import GameIPC
from pathlib import Path

logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')
logger = logging.getLogger(__name__)

SCREENSHOTS_DIR = Path(__file__).parent / "screenshots"

def main():
    parser = argparse.ArgumentParser(description="Claude Plays Fallout")
    parser.add_argument(
        "--max-history",
        type=int,
        default=30,
        help="Maximum number of messages in history before summarization"
    )
    parser.add_argument(
        "--host",
        type=str,
        default="127.0.0.1",
        help="Host address"
    )
    parser.add_argument(
        "--port",
        type=int,
        default=49152,
        help="TCP port to connect back to the game"
    )
    args = parser.parse_args()

    ipc = GameIPC()
    ipc.connect(args.host, args.port)

    SCREENSHOTS_DIR.mkdir(exist_ok=True)
    screenshot_counter = 0

    while True:
        try:
            cmd = input("> ").strip()
        except (EOFError, KeyboardInterrupt):
            break
        if cmd == "next":
            logger.info("Executing next step")
            ipc.send_msg("next")

            img = ipc.recv_screenshot()

            screenshot_counter += 1
            path = SCREENSHOTS_DIR / f"screen_{screenshot_counter:03d}.png"
            img.save(path)
            logger.info("Saved screenshot: %s", path)

if __name__ == "__main__":
    main()
