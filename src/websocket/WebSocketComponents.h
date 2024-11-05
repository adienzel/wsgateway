
#ifndef ASYNC_WEBSOCKET_SERVER_WEBSOCKETCOMPONENTS_H
#define ASYNC_WEBSOCKET_SERVER_WEBSOCKETCOMPONENTS_H
#include <iostream>
#include <unordered_map>
#include <utility>
#include "oatpp-websocket/AsyncWebSocket.hpp"
#include <oatpp/macro/component.hpp>

#include "../ScyllaDBManager.h"

class WebSocketComponent {
public:
    static WebSocketComponent& getInstance() {
        static WebSocketComponent instance;
        return instance;
    }
    
    inline void addClient(oatpp::String const& clientId, std::shared_ptr<oatpp::websocket::AsyncWebSocket> const& socket) {
        while (clientMapLock.test_and_set(std::memory_order_acquire)); // Lock
        clients[clientId] = socket;
        clientMapLock.clear(std::memory_order_release); // Unlock
        if (!dbManager->insert_data(clientId, dbManager->getAppServerName())) {
            OATPP_LOGe("WebSocketComponent", "failed to insert VIN {} and server {} to data base", clientId, dbManager->getAppServerName())
        }
    }
    

    /**
     * setClientNotAvailable
     * not delete but define in the data base that it is not connected now 
     */
    inline void setClientNotAvailable(oatpp::String const& clientId) {
        while (clientMapLock.test_and_set(std::memory_order_acquire)); // Lock
        clients.erase(clientId);
        clientMapLock.clear(std::memory_order_release); // Unlock
        if (!dbManager->insert_data(clientId, "NA")) {
            OATPP_LOGe("WebSocketComponent", "failed to insert VIN {} and server {} to data base", clientId, dbManager->getAppServerName())
        }
    }

    inline void removeClient(oatpp::String const& clientId) {
        while (clientMapLock.test_and_set(std::memory_order_acquire)); // Lock
        clients.erase(clientId);
        clientMapLock.clear(std::memory_order_release); // Unlock
        if (!dbManager->delete_data(clientId)) {
            OATPP_LOGe("WebSocketComponent", "failed to delete VIN {} from data base", clientId)
        }
    }
    
    inline void sendTextMessageToClient(oatpp::String const& clientId, const oatpp::String& message) {
        auto it = clients.find(clientId);
        if (it != clients.end()) {
            it->second->sendOneFrameTextAsync(message);
        }
    }

    inline void sendBinaryMessageToClient(oatpp::String const& clientId, const oatpp::String& message) {
        auto it = clients.find(clientId);
        if (it != clients.end()) {
            it->second->sendOneFrameBinaryAsync(message);
        }
    }
private:
    
    std::unordered_map<oatpp::String, std::shared_ptr<oatpp::websocket::AsyncWebSocket>> clients;
    std::atomic_flag clientMapLock = ATOMIC_FLAG_INIT; // Atomic flag for synchronization
    OATPP_COMPONENT(std::shared_ptr<ScyllaDBManager>, dbManager, "scyllaDBManager");
    WebSocketComponent() {
     
    }
};

#endif //ASYNC_WEBSOCKET_SERVER_WEBSOCKETCOMPONENTS_H
