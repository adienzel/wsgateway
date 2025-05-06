#include "WebSocketListener.h"

#include "oatpp-websocket/AsyncConnectionHandler.hpp"
#include "oatpp/network/Server.hpp"
#include "oatpp/data/stream/Stream.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp-1.4.0/oatpp-openssl/oatpp-openssl/Connection.hpp"
#include <openssl/x509.h>
#include <utility>
#include <future>
//#include "utilis/boostBeastAsyncClient.h"
#include "utilis/boostBeastClient.h"
#include "config.h" 
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/detached.hpp> // If you use `detached` as the completion token


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WSListener

oatpp::async::CoroutineStarter WebSocketListener::onPing(const std::shared_ptr<AsyncWebSocket>& socket,
                                                         const oatpp::String& message) {
//    OATPP_LOGd(TAG, "onPing")
    return socket->sendPongAsync(message);
}

oatpp::async::CoroutineStarter WebSocketListener::onPong(const std::shared_ptr<AsyncWebSocket>& socket,
                                                         const oatpp::String& message) {
//    OATPP_LOGd(TAG, "onPong");
    return nullptr; // do nothing
}

oatpp::async::CoroutineStarter WebSocketListener::onClose(const std::shared_ptr<AsyncWebSocket>& socket,
                                                          v_uint16 code, 
                                                          const oatpp::String& message) {
//    OATPP_LOGd(TAG, "onClose code={} for client {}", code, clientID)
    //delete client id from map
    if (webSocketComponent == nullptr) {
        webSocketComponent = &WebSocketComponent::getInstance();
    }
    
    webSocketComponent->setClientNotAvailable(this->clientID);
    // webSocketComponent->removeClient(this->clientID);
    return nullptr; // do nothing
}



class SendMessageCoroutine : public oatpp::async::Coroutine<SendMessageCoroutine> {
private:
    std::shared_ptr<oatpp::websocket::AsyncWebSocket> m_socket;
    std::string m_message;

public:
    SendMessageCoroutine(const std::shared_ptr<oatpp::websocket::AsyncWebSocket>& socket, const std::string& message)
            : m_socket(socket), m_message(message) {}
    
    Action act() override {
        return m_socket->sendOneFrameTextAsync(m_message).next(finish());
    }
};

oatpp::async::CoroutineStarter WebSocketListener::readMessage(const std::shared_ptr<AsyncWebSocket>& socket,
                                                              v_uint8 opcode, 
                                                              p_char8 data, 
                                                              oatpp::v_io_size size) {
    if (size == 0) { // message transfer finished
//        if (opcode == oatpp::websocket::Frame::OPCODE_TEXT) {
//            OATPP_LOGd(__func__, "got text");
//        } else if (opcode == oatpp::websocket::Frame::OPCODE_BINARY) {
//            OATPP_LOGd(__func__, "got binary of size {}", m_messageBuffer.getCapacity());
//        }
    

        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        auto t = std::make_shared<std::string>();
        *t = std::to_string(ns);
        auto wholeMessage = std::make_shared<std::string>();
        *wholeMessage = m_messageBuffer.toStdString();
//        auto wholeMessage = m_messageBuffer.toString();
        m_messageBuffer.reset();
      
//        OATPP_LOGd(__func__, "line {} to client {} message={}", __LINE__, clientID, wholeMessage);
    
        //dispatch messages to apps
        
        //std::make_shared<session>(*ioc_)->run(http_server_address_.c_str(), http_server_port_.c_str(), wholeMessage, clientID, &t);
        OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor_);
        boost::asio::co_spawn(
                *ioc_,
                asyncHttpClient(wholeMessage, http_server_address_, http_server_port_, clientID, t),
                [socket, executor=executor_, &t](std::exception_ptr eptr, std::string result) {
                    //std::cout << "[co_spawn handler] result = " << result << std::endl;
                    if (eptr) {
                        try { 
                            std::rethrow_exception(eptr); 
                        } catch (const std::exception& e) {
                            std::cerr << "co_spawn error: " << e.what() << std::endl;
                        }
                        return;
                    }
    
                    if (!result.empty()) {
//                        OATPP_LOGd(__func__, "boost::asio::co_spawn line {} result = {}", __LINE__, result);
    
    
                        //std::string str = *(t.get());
                        OATPP_LOGd(__func__ , "result before send")
                        long long ns2;
                        
                        //std::istringstream(str) >> ns2;

                        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                        OATPP_LOGd(__func__ , "result before send {}", ns - ns2)
                        executor->execute<SendMessageCoroutine>(socket, result);
                        ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                        OATPP_LOGd(__func__ , "result before after {}", ns - ns2)
                        //socket->sendOneFrameTextAsync(result);
    
//                        OATPP_LOGd(__func__, "boost::asio::co_spawn line {} result = {}", __LINE__, result);
                    }
                }
        );
        
//        OATPP_LOGi(__func__, "line {}", __LINE__)
    } else if (size > 0) { // message frame received
        m_messageBuffer.writeSimple(data, size);
    }
    
    return nullptr; // do nothing
    
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WSInstanceListener

std::atomic<v_int32> WSInstanceListener::SOCKETS(0);

void WSInstanceListener::onAfterCreate_NonBlocking(const std::shared_ptr<WebSocketListener::AsyncWebSocket>& socket,
                                                   const std::shared_ptr<const ParameterMap>& params) {
    
    SOCKETS ++;
    //OATPP_LOGi(__func__, "New Incoming Connection. Connection count={}", SOCKETS.load())
    OATPP_COMPONENT(std::shared_ptr<Config>, m_cmdArgs);
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
    
    // register the soket by client id
    if (webSocketComponent == nullptr) {
        webSocketComponent = &WebSocketComponent::getInstance();
    }
    //OATPP_LOGi(__func__, "line {}", __LINE__)
    
    //use the vin from ssl
//    webSocketComponent->addClient(common_name, socket);
    //use vin from path
    webSocketComponent->addClient(clientId, socket);
    //allocate listener per connection
    OATPP_COMPONENT(std::shared_ptr<boost::asio::io_context>, ioc);
    //use the vin from ssl
    //socket->setListener(std::make_shared<WebSocketListener>(common_name, ioc, m_cmdArgs->http_request_address, m_cmdArgs->http_request_port));
    //use vin from path
    socket->setListener(std::make_shared<WebSocketListener>(clientId, ioc, m_cmdArgs->http_request_address, m_cmdArgs->http_request_port));
    //OATPP_LOGi(__func__, "line {}", __LINE__)
}

void WSInstanceListener::onBeforeDestroy_NonBlocking(const std::shared_ptr<WebSocketListener::AsyncWebSocket>& socket) {
    
    SOCKETS --;
    socket->getListener()->onClose(socket, 0, "got close from onBeforeDestroy_NonBlocking");
    OATPP_LOGd(TAG, "Connection closed.  Connection count={}", SOCKETS.load())
    
}
