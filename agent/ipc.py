import logging
import socket

logger = logging.getLogger(__name__)

class GameIPC:
    def __init__(self):
        self._sock = None

    def connect(self, host: str, port: int):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.connect((host, port))

        logger.info("Connected to game on port %d", port)

    def send_msg(self, msg: str):
        self._sock.sendall(msg.encode())