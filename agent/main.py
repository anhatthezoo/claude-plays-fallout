import logging
import argparse

from pathlib import Path
from ipc import GameIPC
from commands import CommandManager

logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')
logger = logging.getLogger(__name__)

SCREENSHOTS_DIR = Path(__file__).parent / "screenshots"

def dispatch_command(cmds: CommandManager, args: list[str]) -> bool:
    command = args[0]
    action = args[1] if len(args) > 1 else None

    if command == "navigate":
        if action in ("walk", "run"):
            cmds.navigate(action, int(args[2]), int(args[3]))
        elif action in ("scroll_left", "scroll_right", "scroll_up", "scroll_down", "center"):
            cmds.navigate(action)
        else:
            logger.error("navigate: unknown action '%s'", action)
            return False
    elif command == "combat":
        if action in ("begin", "end_turn", "end_combat"):
            cmds.combat(action)
        elif action == "attack":
            cmds.combat(action, int(args[2]), int(args[3]))
        else:
            logger.error("combat: unknown action '%s'", action)
            return False
    elif command == "inventory":
        if action == "open":
            cmds.inventory("open")
        elif action == "equip":
            cmds.inventory("equip", item=int(args[2]), slot=int(args[3]))
        elif action == "unequip":
            cmds.inventory("unequip", slot=int(args[2]))
        else:
            logger.error("inventory: unknown action '%s'", action)
            return False
    elif command == "loot":
        take = args[1].lower() in ("take", "1", "true")
        cmds.loot(take, int(args[2]), int(args[3]))
    elif command == "barter":
        if action in ("buy", "sell", "retract_mine", "retract_theirs", "confirm"):
            cmds.barter(action, int(args[2]) if len(args) > 2 else 0, int(args[3]) if len(args) > 3 else 0)
        else:
            logger.error("barter: unknown action '%s'", action)
            return False
    elif command == "click":
        cmds.left_click(int(args[1]), int(args[2]))
    elif command == "state":
        cmds.request_state()
    else:
        logger.error("Unknown command: '%s'", command)
        return False

    return True

def main():
    parser = argparse.ArgumentParser(description="Claude Plays Fallout")
    parser.add_argument("--max-history", type=int, default=30)
    parser.add_argument("--host", type=str, default="127.0.0.1")
    parser.add_argument("--port", type=int, default=49152)
    args = parser.parse_args()

    ipc = GameIPC()
    ipc.connect(args.host, args.port)
    cmds = CommandManager(ipc)

    while True:
        try:
            cmd = input("> ").strip()
        except (EOFError, KeyboardInterrupt):
            break

        if not cmd:
            continue

        cmd_args = cmd.split()
        dispatch_command(cmds, cmd_args)

if __name__ == "__main__":
    main()
