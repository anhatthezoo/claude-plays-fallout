#include "agent/agent_ipc.h"

#include <cstring>
#include <algorithm>
#include <SDL_net.h>

#include "game/gconfig.h"
#include "plib/gnw/memory.h"
#include "plib/gnw/debug.h"
#include "plib/gnw/svga.h"

namespace fallout {

#define DATA_BUFFER_SIZE 1024
static int gScreenshotSize;
static unsigned char* gScreenshotBuf = nullptr;

static SDLNet_SocketSet gICPSocketSet = nullptr;
static TCPsocket gAgentServer = nullptr; 
static TCPsocket gAgentClient = nullptr;

bool agent_ipc_init() 
{
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

                gScreenshotSize = (sizeof(uint16_t) * 2) + (screenGetWidth() * screenGetHeight() * 4);
                gScreenshotBuf = (unsigned char*)mem_malloc(gScreenshotSize);
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

void agent_ipc_shutdown() 
{
    debug_printf(">agent_ipc_shutdown\t");

    if (gAgentClient) SDLNet_TCP_Close(gAgentClient);
    SDLNet_TCP_Close(gAgentServer);
    SDLNet_FreeSocketSet(gICPSocketSet);
    SDLNet_Quit();

    if (gScreenshotBuf) {
        mem_free(gScreenshotBuf);
        gScreenshotBuf = nullptr;
    }

    gICPSocketSet = nullptr;
    gAgentServer = nullptr;
    gAgentClient = nullptr;

    return;
}

bool agent_ipc_send(uint8_t type, const unsigned char* data, uint32_t length)
{
    if (!agent_ipc_connected()) {
        return false;
    }

    unsigned char header[5];
    header[0] = type;
    SDLNet_Write32(length, header + 1);

    if (SDLNet_TCP_Send(gAgentClient, header, sizeof(header)) < sizeof(header)) {
        debug_printf("Failed to send message header\n");
        return false;
    }

    uint32_t sent = 0;
    while (sent < length) {
        int to_send = std::min(static_cast<int>(length - sent), DATA_BUFFER_SIZE);
        if (SDLNet_TCP_Send(gAgentClient, data + sent, to_send) < to_send) {
            debug_printf("Failed to send message chunk at offset %u\n", sent);
            return false;
        }
        sent += to_send;
    }

    debug_printf("Sent message of %u byte(s) to agent\t", length);

    return true;
}

bool agent_ipc_send_screenshot()
{
    if (gSdlRenderer == nullptr) {
        debug_printf("No renderer available for sending screenshot\n");
        return false;
    };

    if (SDL_RenderReadPixels(
        gSdlRenderer, 
        NULL, 
        SDL_PIXELFORMAT_RGB888, 
        gScreenshotBuf + (sizeof(uint16_t) * 2), 
        gSdlTextureSurface->pitch) < 0) {

        debug_printf("Failed to read pixels from renderer: %s\n", SDL_GetError());
        return false;
    }

    SDLNet_Write16(screenGetWidth(), gScreenshotBuf);
    SDLNet_Write16(screenGetHeight(), gScreenshotBuf + sizeof(uint16_t));

    return agent_ipc_send(MSG_SCREENSHOT, gScreenshotBuf, gScreenshotSize);
}

void agent_ipc_poll()
{
    if (!agent_ipc_connected()) {
        return;
    }

    int ready = SDLNet_CheckSockets(gICPSocketSet, 0);
    if (ready <= 0) {
        return;
    }

    char buf[DATA_BUFFER_SIZE];

    if (gAgentClient && SDLNet_SocketReady(gAgentClient)) {
        int len = SDLNet_TCP_Recv(gAgentClient, buf, sizeof(buf) - 1);
        if (len > 0) {
            buf[len] = '\0';
            debug_printf("Received IPC message from agent: %s\n", buf);

            if (strcmp(buf, "next") == 0) {
                agent_ipc_send_screenshot();
            }
        } else {
            debug_printf("Agent disconnected\n");
            agent_ipc_shutdown();
        }
    }
}

bool agent_ipc_connected() 
{
    return gAgentClient != nullptr;
}

} // namespace fallout
