
#ifndef VGEATWAY_WEBSOCKETLISTENER_H

#define VGEATWAY_WEBSOCKETLISTENER_H

#include "utilis/ipUtils.h"
#include <oatpp/base/Log.hpp>

#include "oatpp-websocket/AsyncConnectionHandler.hpp"
#include "oatpp-websocket/AsyncWebSocket.hpp"
#include <oatpp/web/client/HttpRequestExecutor.hpp>
#include <oatpp/async/Executor.hpp>
#include <oatpp/json/ObjectMapper.hpp>
#include "WebSocketComponents.h"


class WebSocketListener : public oatpp::websocket::AsyncWebSocket::Listener {
public:
    explicit WebSocketListener(oatpp::String const& id) : clientID(id) {}
    /**
     * Called on "ping" frame.
     */
    CoroutineStarter onPing(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::String& message) override;
    
    /**
     * Called on "pong" frame
     */
    CoroutineStarter onPong(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::String& message) override;
    
    /**
     * Called on "close" frame
     */
    CoroutineStarter onClose(const std::shared_ptr<AsyncWebSocket>& socket, v_uint16 code, const oatpp::String& message) override;
    
    /**
     * Called on each message frame. After the last message will be called once-again with size == 0 to designate end of the message.
     */
    CoroutineStarter readMessage(const std::shared_ptr<AsyncWebSocket>& socket, v_uint8 opcode, p_char8 data, oatpp::v_io_size size) override;

private:
    static constexpr const char* TAG = "Server_WSListener";
    /**
     * Buffer for messages. Needed for multi-frame messages.
     */
    oatpp::data::stream::BufferOutputStream m_messageBuffer;
    oatpp::String clientID;
    WebSocketComponent* webSocketComponent = nullptr;
    
    void sendToRestServer(const oatpp::String& version, const oatpp::String& vin);
    
};

/**
 * Listener on new WebSocket connections.
 */
class WSInstanceListener : public oatpp::websocket::AsyncConnectionHandler::SocketInstanceListener {
public:
    
    /**
     *  Called when socket is created
     */
    void onAfterCreate_NonBlocking(const std::shared_ptr<WebSocketListener::AsyncWebSocket> &socket,
                                   const std::shared_ptr<const ParameterMap> &params) override;
    
    /**
     *  Called before socket instance is destroyed.
     */
    void onBeforeDestroy_NonBlocking(const std::shared_ptr<WebSocketListener::AsyncWebSocket> &socket) override;

private:
    static constexpr const char *TAG = "Server_WSInstanceListener";
    /**
     * Counter for connected clients.
     */
    static std::atomic<v_int32> SOCKETS;
    // maintain all connected sockets 
    WebSocketComponent *webSocketComponent = nullptr;
    
   
};

#endif //VGEATWAY_WEBSOCKETLISTENER_H
