#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <Arduino.h>

namespace WebServerManager {
    // Initialize Web Server, API endpoints, and WebSocket handler
    void init();

    // Perform periodic housekeeping tasks (e.g. cleaning up dead WebSocket clients)
    void update();

    // Broadcast current audio statistics to all connected WebSocket clients
    // (Throttled inside to prevent high CPU overhead)
    void broadcastStatus();
}

#endif // WEB_SERVER_MANAGER_H
