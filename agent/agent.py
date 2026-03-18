from anthropic import Anthropic

SYSTEM_PROMPT = """You are playing Fallout 1. You can see the game screen and control the game by executing emulator commands.

Your goal is to play through Fallout 1 and eventually save your vault. Make decisions based on what you see on the screen.

Before each action, explain your reasoning briefly, then use the emulator tool to execute your chosen commands.

The conversation history may occasionally be summarized to save context space. If you see a message labeled "CONVERSATION HISTORY SUMMARY", this contains the key information about your progress so far. Use this information to maintain continuity in your gameplay."""

SUMMARY_PROMPT = """I need you to create a detailed summary of our conversation history up to this point. This summary will replace the full conversation history to manage the context window.

Please include:
1. Key game events and milestones you've reached
2. Important decisions you've made
3. Current objectives or goals you're working toward
4. Your current location and status
5. Any strategies or plans you've mentioned

The summary should be comprehensive enough that you can continue gameplay without losing important context about what has happened so far."""

AVAILABLE_TOOLS = [

]

class Agent:
    def __init__(self, max_history=60):
        self._client = Anthropic()
        self._message_history = [{"role": "user", "content": "You may now begin playing."}]
        self._max_history = max_history