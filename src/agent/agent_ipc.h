#ifndef FALLOUT_AGENT_IPC_H_
#define FALLOUT_AGENT_IPC_H_

namespace fallout {

// Initialize the agent IPC: starts a TCP server on localhost and waits up 
// to 10 seconds (set in config) for the Python agent to connect.
// Returns true on success. Logs warnings and returns false on failure (the
// game continues normally without agent functionality).
bool agent_ipc_init();

// Close the client/server sockets.
void agent_ipc_shutdown();

// Check (non-blocking) whether the Python agent has sent a request. Call 
// once per game frame.
void agent_ipc_poll();

// Returns true if the Python agent is currently connected.
bool agent_ipc_connected();

} // namespace fallout

#endif /* FALLOUT_AGENT_IPC_H_ */
