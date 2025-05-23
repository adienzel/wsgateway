
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
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>


class WebSocketListener : public oatpp::websocket::AsyncWebSocket::Listener {
public:
    explicit WebSocketListener(const oatpp::String& id, const std::shared_ptr<boost::asio::io_context>& ioc, std::string& host, std::string& port) : 
         clientID(id),  ioc_(ioc), http_server_address_(host), http_server_port_(port) {
        ioc_->restart();
    }
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
    std::string clientID;
    std::shared_ptr<boost::asio::io_context> ioc_ = nullptr;
    std::string http_server_address_;
    std::string http_server_port_;
    
    
    WebSocketComponent* webSocketComponent = nullptr;
    
   
    
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
