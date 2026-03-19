import logging
import socket
import struct

from enum import Enum
from PIL import Image

logger = logging.getLogger(__name__)

class MessageType(Enum):
    MSG_SCREENSHOT = 0x1

class GameIPC:
    def __init__(self):
        self._sock = None

    def connect(self, host: str, port: int):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.connect((host, port))

        logger.info("Connected to game on port %d", port)

    def send_msg(self, msg: str):
        self._sock.sendall(msg.encode())

    def _recv_exact(self, n: int) -> bytes:
        buf = bytearray()
        while len(buf) < n:
            chunk = self._sock.recv(min(n - len(buf), 4096))
            if not chunk:
                raise ConnectionError("Socket closed while reading")
            buf.extend(chunk)
        return bytes(buf)

    def recv_msg(self) -> tuple[MessageType, bytes]:
        header = self._recv_exact(5)
        msg_type = MessageType(header[0])
        length = struct.unpack('!I', header[1:5])[0]
        payload = self._recv_exact(length)
        return (msg_type, payload)

    def recv_screenshot(self) -> Image.Image:
        msg_type, payload = self.recv_msg()
        if msg_type != MessageType.MSG_SCREENSHOT:
            raise RuntimeError(f"Expected screenshot message (type {MessageType.MSG_SCREENSHOT}), got type {msg_type}")

        w, h = struct.unpack('!HH', payload[:4])
        pixel_data = payload[4:]

        expected = w * h * 4
        if len(pixel_data) != expected:
            raise RuntimeError(f"Screenshot pixel data size mismatch: expected {expected}, got {len(pixel_data)}")

        return Image.frombytes("RGB", (w, h), pixel_data, "raw", "BGRX")
