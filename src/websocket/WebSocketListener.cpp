#include "WebSocketListener.h"

#include <utility>
#include "../client/RestClient.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WSListener

oatpp::async::CoroutineStarter WebSocketListener::onPing(const std::shared_ptr<AsyncWebSocket>& socket,
                                                         const oatpp::String& message) {
    OATPP_LOGd(TAG, "onPing")
    return socket->sendPongAsync(message);
}

oatpp::async::CoroutineStarter WebSocketListener::onPong(const std::shared_ptr<AsyncWebSocket>& socket,
                                                         const oatpp::String& message) {
    OATPP_LOGd(TAG, "onPong")
    return nullptr; // do nothing
}

oatpp::async::CoroutineStarter WebSocketListener::onClose(const std::shared_ptr<AsyncWebSocket>& socket,
                                                          v_uint16 code, 
                                                          const oatpp::String& message) {
    OATPP_LOGd(TAG, "onClose code={} for client {}", code, clientID)
    //delete client id from map
    if (webSocketComponent == nullptr) {
        webSocketComponent = &WebSocketComponent::getInstance();
    }
    webSocketComponent->removeClient(this->clientID);
    return nullptr; // do nothing
}

oatpp::async::CoroutineStarter WebSocketListener::readMessage(const std::shared_ptr<AsyncWebSocket>& socket,
                                                              v_uint8 opcode, 
                                                              p_char8 data, 
                                                              oatpp::v_io_size size) {
    if (size == 0) { // message transfer finished
        if (opcode == oatpp::websocket::Frame::OPCODE_TEXT) {
            OATPP_LOGd(__func__, "got text")
        } else if (opcode == oatpp::websocket::Frame::OPCODE_BINARY) {
            OATPP_LOGd(__func__, "got binary of size {}", m_messageBuffer.getCapacity())
        }
        auto wholeMessage = m_messageBuffer.toString();
        m_messageBuffer.setCurrentPosition(0);
        
        OATPP_LOGd(TAG, "onMessage to client {} message={}", clientID, wholeMessage->c_str())
    
        //TODO dispatch messages to apps
    
        /* Send message in reply */
        return socket->sendOneFrameTextAsync("Hello from oatpp!: " + wholeMessage);
    } else if (size > 0) { // message frame received
        m_messageBuffer.writeSimple(data, size);
    }
    
    return nullptr; // do nothing
    
}

void WebSocketListener::sendToRestServer(const oatpp::String& version, const oatpp::String& vin){
    OATPP_COMPONENT(std::shared_ptr<oatpp::web::client::RequestExecutor>, requestExecutor, "clientExcecutor");
    auto objectMapper = std::make_shared<oatpp::json::ObjectMapper>();
    
    auto client = ClientApi::createShared(requestExecutor, objectMapper);
    
    class SendCoroutine : public oatpp::async::Coroutine<SendCoroutine> {
        std::shared_ptr<ClientApi> m_client;
        oatpp::String m_version;
        oatpp::String m_vin;
        oatpp::String m_message;
        
    public:
        SendCoroutine(const std::shared_ptr<ClientApi>& client, oatpp::String  version, const oatpp::String& vin,
                      const oatpp::String& message) : 
                      m_client(client), m_version(std::move(version)), m_vin(vin), m_message(message) {}
                      
        Action act() override {
            return m_client->doAsyncPostBodyAccess(m_vin, m_version, m_message).callbackTo(&SendCoroutine::onResponse);
        }
            
        Action onResponse(const std::shared_ptr<oatpp::web::protocol::http::incoming::Response>& response) {
            if (response->getStatusCode() == 200) {
                OATPP_LOGi("Client", "Message sent sucssesfuly")
            } else {
                OATPP_LOGe("Client", "Failed to send Message")
            }
            return finish();
        }
    };
    
    auto executor = std::make_shared<oatpp::async::Executor>();
    executor->execute<SendCoroutine>(client, version, vin, m_messageBuffer.toString());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WSInstanceListener

std::atomic<v_int32> WSInstanceListener::SOCKETS(0);

void WSInstanceListener::onAfterCreate_NonBlocking(const std::shared_ptr<WebSocketListener::AsyncWebSocket>& socket,
                                                   const std::shared_ptr<const ParameterMap>& params) {
    
    SOCKETS ++;
    OATPP_LOGd(TAG, "New Incoming Connection. Connection count={}", SOCKETS.load())
    
    /* In this particular case we create one WSListener per each connection */
    /* Which may be redundant in many cases */
    auto clientIdParam = params->find("ClientId");
    if (clientIdParam == params->end()) {
        OATPP_LOGe(TAG, "ClientId paramter not found")
        std::string str = "no ClientId";
        socket->sendCloseAsync(1, str);
    }
    auto clientId = clientIdParam->second;
    if (clientId->empty()) {
        std::string str = "ClientId paramter is empty";
        OATPP_LOGe(TAG, str)
        socket->sendCloseAsync(2, str);
    
    }
    // register the soket by client id
    if (webSocketComponent == nullptr) {
        webSocketComponent = &WebSocketComponent::getInstance();
    }
    
    webSocketComponent->addClient(clientId, socket);
    //allocate listener per connection
    socket->setListener(std::make_shared<WebSocketListener>(clientId));
}

void WSInstanceListener::onBeforeDestroy_NonBlocking(const std::shared_ptr<WebSocketListener::AsyncWebSocket>& socket) {
    
    SOCKETS --;
    socket->getListener()->onClose(socket, 0, "got close from onBeforeDestroy_NonBlocking");
    OATPP_LOGd(TAG, "Connection closed.  Connection count={}", SOCKETS.load())
    
}
