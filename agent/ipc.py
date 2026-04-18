import logging
import socket
import struct

from enum import Enum
from PIL import Image

logger = logging.getLogger(__name__)

class MessageType(Enum):
    MSG_GENERAL = 0x00
    MSG_SCREENSHOT = 0x01
    MSG_PLAYER_STATE = 0x02
    MSG_RESPONSE_END = 0x03

    MSG_NAVIGATE = 0x10
    MSG_COMBAT = 0x11
    MSG_INVENTORY = 0x12
    MSG_BARTER = 0x13
    MSG_DIALOGUE = 0x14
    MSG_INTERACT = 0x15
    MSG_ITEM = 0x16
    MSG_LOOT = 0x17
    MSG_PIPBOY = 0x18
    MSG_LEFT_CLICK = 0x19
    MSG_REQ_STATE = 0x1A

class GameIPC:
    def __init__(self):
        self._sock = None

    def connect(self, host: str, port: int):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.connect((host, port))
        logger.info("Connected to game on port %d", port)

    def send(self, msg_type: MessageType, payload: bytes = b""):
        header = struct.pack("!BI", msg_type.value, len(payload))
        self._sock.sendall(header + payload)

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
            raise RuntimeError(f"Expected screenshot, got {msg_type}")

        w, h = struct.unpack('!II', payload[:8])
        pixel_data = payload[8:]

        expected = w * h * 4
        if len(pixel_data) != expected:
            raise RuntimeError(f"Screenshot size mismatch: expected {expected}, got {len(pixel_data)}")

        return Image.frombytes("RGB", (w, h), pixel_data, "raw", "BGRX")
