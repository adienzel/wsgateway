#include "WebSocketListener.h"

#include "oatpp-websocket/AsyncConnectionHandler.hpp"
#include "oatpp/network/Server.hpp"
#include "oatpp/data/stream/Stream.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp-1.4.0/oatpp-openssl/oatpp-openssl/Connection.hpp"
#include <openssl/x509.h>
#include <utility>
#include <future>
#include "../client/RestClient.h"
//#include "utilis/boostBeastAsyncClient.h"
#include "utilis/boostBeastClient.h"
#include "utilis/messageDTO.h"
#include "config.h" 

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WSListener

oatpp::async::CoroutineStarter WebSocketListener::onPing(const std::shared_ptr<AsyncWebSocket>& socket,
                                                         const oatpp::String& message) {
    OATPP_LOGd(TAG, "onPing")
    return socket->sendPongAsync(message);
}

oatpp::async::CoroutineStarter WebSocketListener::onPong(const std::shared_ptr<AsyncWebSocket>& socket,
                                                         const oatpp::String& message) {
    OATPP_LOGd(TAG, "onPong");
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
    
    webSocketComponent->setClientNotAvailable(this->clientID);
    // webSocketComponent->removeClient(this->clientID);
    return nullptr; // do nothing
}


oatpp::async::CoroutineStarter WebSocketListener::readMessage(const std::shared_ptr<AsyncWebSocket>& socket,
                                                              v_uint8 opcode, 
                                                              p_char8 data, 
                                                              oatpp::v_io_size size) {
    if (size == 0) { // message transfer finished
        if (opcode == oatpp::websocket::Frame::OPCODE_TEXT) {
            OATPP_LOGd(__func__, "got text");
        } else if (opcode == oatpp::websocket::Frame::OPCODE_BINARY) {
            OATPP_LOGd(__func__, "got binary of size {}", m_messageBuffer.getCapacity());
        }
        struct timespec t {0,0};
        clock_gettime(CLOCK_MONOTONIC, &t);
        //auto wholeMessage = m_messageBuffer.toString();
        //m_messageBuffer.setCurrentPosition(0);
        auto wholeMessage = m_messageBuffer.toString();
        //auto wholeMessage = (std::string*)m_messageBuffer.getData();
        m_messageBuffer.reset();
      
        OATPP_LOGd(TAG, "readMessage to client {} message={}", clientID, wholeMessage);
    
        //dispatch messages to apps
        
        //std::make_shared<session>(*ioc_)->run(http_server_address_.c_str(), http_server_port_.c_str(), wholeMessage, clientID, &t);
        auto response =  sendHttpReqSync(wholeMessage, http_server_address_, http_server_port_, clientID, &t);
        if (!response.empty()) {
            socket->sendOneFrameTextAsync(response);
        }
        //return nullptr;
        //this is echo direct from websocket
        //return socket->sendOneFrameTextAsync("Hello from oatpp!: " + wholeMessage);
    } else if (size > 0) { // message frame received
        std::string str = {};
        for (auto i = 0; i < size; i++) {
            str += data[i];
        } 
        OATPP_LOGd(__func__, " Data = {}, size = {}", str, size);
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
                OATPP_LOGi("Client", "Message sent successfully")
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
    OATPP_LOGi(__func__, "New Incoming Connection. Connection count={}", SOCKETS.load())
    OATPP_COMPONENT(std::shared_ptr<Config>, m_cmdArgs);
    auto clientIdParam = params->find("ClientId");
    OATPP_LOGi(__func__, "line {}", __LINE__)
    if (clientIdParam == params->end()) {
        OATPP_LOGe(TAG, "ClientId paramter not found")
        std::string str = "no ClientId";
        socket->sendCloseAsync(1, str);
    }
    auto clientId = clientIdParam->second;
    OATPP_LOGi(__func__, "line {}", __LINE__)
    if (clientId->empty()) {
        std::string str = "ClientId paramter is empty";
        OATPP_LOGe(TAG, str)
        socket->sendCloseAsync(2, str);
        
    }
    OATPP_LOGi(__func__, "line {}", __LINE__)
    
    std::string common_name = {};
    if (m_cmdArgs->use_mtls) {
        auto connection = std::dynamic_pointer_cast<oatpp::openssl::Connection>(socket->getConnection().object);
        if (connection) {
            SSL const* ssl = connection->getOpenSSLContext();
            if (ssl) {
                X509* cert = SSL_get_peer_certificate(ssl);
                if (cert) {
                    X509_NAME* subject_name = X509_get_subject_name(cert);
    
                    char cn[256];
                    X509_NAME_get_text_by_NID(subject_name, NID_commonName, cn, sizeof(cn));
                    common_name = cn;
                    char on[256];
                    X509_NAME_get_text_by_NID(subject_name, NID_organizationName, on, sizeof(on));
                    OATPP_LOGd(__func__, "CN = {}, ON= {}", cn, on)
                    if (char *subjectName = X509_NAME_oneline(subject_name, nullptr, 0)) {
                        OATPP_LOGd(__func__, "Client Identity: {}", subjectName)
                        OPENSSL_free(subjectName);
                    }
                    X509_free(cert);
                }
            }
        }
    }
    OATPP_LOGi(__func__, "line {}", __LINE__)
    
    // register the soket by client id
    if (webSocketComponent == nullptr) {
        webSocketComponent = &WebSocketComponent::getInstance();
    }
    OATPP_LOGi(__func__, "line {}", __LINE__)
    
    //use the vin from ssl
//    webSocketComponent->addClient(common_name, socket);
    //use vin from path
    webSocketComponent->addClient(clientId, socket);
    OATPP_LOGi(__func__, "line {}", __LINE__)
    //allocate listener per connection
    auto ioc = std::make_shared<boost::asio::io_context>();
    OATPP_LOGi(__func__, "line {}", __LINE__)
    //use the vin from ssl
    //socket->setListener(std::make_shared<WebSocketListener>(common_name, ioc, m_cmdArgs->http_request_address, m_cmdArgs->http_request_port));
    //use vin from path
    socket->setListener(std::make_shared<WebSocketListener>(clientId, ioc, m_cmdArgs->http_request_address, m_cmdArgs->http_request_port));
    OATPP_LOGi(__func__, "line {}", __LINE__)
}

void WSInstanceListener::onBeforeDestroy_NonBlocking(const std::shared_ptr<WebSocketListener::AsyncWebSocket>& socket) {
    
    SOCKETS --;
    socket->getListener()->onClose(socket, 0, "got close from onBeforeDestroy_NonBlocking");
    OATPP_LOGd(TAG, "Connection closed.  Connection count={}", SOCKETS.load())
    
}
