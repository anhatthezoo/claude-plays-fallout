#include "agent/agent_ipc.h"

#include <SDL_net.h>
#include "plib/gnw/debug.h"
#include "game/gconfig.h"

namespace fallout {

static SDLNet_SocketSet gICPSocketSet = nullptr;
static TCPsocket gAgentServer = nullptr; 
static TCPsocket gAgentClient = nullptr;

bool agent_ipc_init() {
    debug_printf(">agent_ipc_init\t");

    if (SDLNet_Init() < 0) {
        debug_printf("Failed to initialize SDL_net: %s\n", SDLNet_GetError());
        return false;
    }

    int port = -1;
    config_get_value(&game_config, GAME_CONFIG_AGENT_KEY, GAME_CONFIG_AGENT_PORT_KEY, &port);

    IPaddress ip;
    SDLNet_ResolveHost(&ip, nullptr, port);

    gAgentServer = SDLNet_TCP_Open(&ip);
    if (!gAgentServer) {
        debug_printf("Failed to open TCP server socket: %s\n", SDLNet_GetError());
        SDLNet_Quit();
        return false;
    }

    debug_printf("Listening for agent connection on port %d...\n", SDLNet_Read16(&ip.port));

    gICPSocketSet = SDLNet_AllocSocketSet(2);
    SDLNet_TCP_AddSocket(gICPSocketSet, gAgentServer);

    int timeout = -1;
    config_get_value(&game_config, GAME_CONFIG_AGENT_KEY, GAME_CONFIG_AGENT_START_TIMEOUT_KEY, &timeout);

    int ready = SDLNet_CheckSockets(gICPSocketSet, timeout);

    if (ready > 0) {
        if (SDLNet_SocketReady(gAgentServer)) {
            gAgentClient = SDLNet_TCP_Accept(gAgentServer);
            if (gAgentClient) {
                debug_printf("Agent process connected successfully.\n");
                SDLNet_TCP_AddSocket(gICPSocketSet, gAgentClient);
            } else {
                debug_printf("Failed to accept agent connection: %s\n", SDLNet_GetError());
                agent_ipc_shutdown();
                return false;
            }
        }
    } else {
        debug_printf("Agent process failed to connect within timeout period.\n");
        agent_ipc_shutdown();
        return false;
    }

    return true;
}

void agent_ipc_shutdown() {
    if (gAgentClient) SDLNet_TCP_Close(gAgentClient);
    SDLNet_TCP_Close(gAgentServer);
    SDLNet_FreeSocketSet(gICPSocketSet);
    SDLNet_Quit();

    gICPSocketSet = nullptr;
    gAgentServer = nullptr;
    gAgentClient = nullptr;

    return;
}

void agent_ipc_poll() {
    if (!agent_ipc_connected()) {
        return;
    }

    int ready = SDLNet_CheckSockets(gICPSocketSet, 0);
    if (ready <= 0) {
        return;
    }

    char buf[1024];

    if (gAgentClient && SDLNet_SocketReady(gAgentClient)) {
        int len = SDLNet_TCP_Recv(gAgentClient, buf, sizeof(buf) - 1);
        if (len > 0) {
            buf[len] = '\0';
            debug_printf("Received IPC message from agent: %s\n", buf);
        } else {
            debug_printf("Agent disconnected\n");
            agent_ipc_shutdown();
        }
    }
}

bool agent_ipc_connected() {
    return gAgentClient != nullptr;
}

} // namespace fallout
