
#ifndef VGEATWAY_APPCOMPONENT_H
#define VGEATWAY_APPCOMPONENT_H
/*
 * This file defines all the parameters use by app that are global usuing shared pointer maps to environment values
 * 
 */
#include <oatpp/base/Log.hpp>

#include "oatpp/web/server/AsyncHttpConnectionHandler.hpp"
#include "oatpp/web/server/HttpRouter.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/network/tcp/client/ConnectionProvider.hpp"
#include "oatpp-1.4.0/oatpp-openssl/oatpp-openssl/configurer/CertificateChainFile.hpp"
#include "oatpp-1.4.0/oatpp-openssl/oatpp-openssl/configurer/PrivateKeyFile.hpp"
#include "oatpp-1.4.0/oatpp-openssl/oatpp-openssl/configurer/TrustStore.hpp"
#include "oatpp-1.4.0/oatpp-openssl/oatpp-openssl/configurer/PeerCertificateVerification.hpp"
#include "oatpp-1.4.0/oatpp-openssl/oatpp-openssl/configurer/ContextConfigurer.hpp"
#include "oatpp-1.4.0/oatpp-openssl/oatpp-openssl/Config.hpp"
#include "oatpp-1.4.0/oatpp-openssl/oatpp-openssl/server/ConnectionProvider.hpp"

#include "openssl/tls1.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "oatpp/network/Address.hpp"
#include "oatpp/json/ObjectMapper.hpp"
#include <oatpp/macro/component.hpp>
#include <oatpp/base/CommandLineArguments.hpp>
#include <oatpp/utils/Conversion.hpp>
#include <oatpp/web/client/HttpRequestExecutor.hpp>
#include <oatpp/web/client/RequestExecutor.hpp>

#include "./websocket/WebSocketListener.h"
#include "./websocket/WebSocketComponents.h"
#include "./utilis/ipUtils.h"
#include "config.h"
#include "ScyllaDBManager.h"
#include "utilis/env.h"
#include "utilis/split.h"

#include "client/RestClient.h"

int passwordCB(char* buffer, int size, int rw_flag, void* user_data) {
    const std::string* pass = static_cast<std::string*>(user_data);
    if (pass->size() > static_cast<size_t>(size)) {
        return 0;
    }
    std::strncpy(buffer, pass->c_str(), size);
    return static_cast<int>(pass->length());    
}


class AppComponent {
public:
    AppComponent() = default;
    
    OATPP_CREATE_COMPONENT(std::shared_ptr<Config>, configuration)([] {
        return std::make_shared<Config>();
    }());
  
    
    /**
     * Create Async Executor
     */
      
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor)([] {
        OATPP_COMPONENT(std::shared_ptr<Config>, m_cmdArgs);
        //OATPP_LOGd(__func__, " {}", __LINE__)

        return std::make_shared<oatpp::async::Executor>(
                m_cmdArgs->number_of_worker_threads, /* Data-Processing threads */
                m_cmdArgs->number_of_io_threads, /* I/O threads */
                m_cmdArgs->number_of_timer_threads /* Timer threads */
        );
    }());
    
/**
   *  Create List of Connection Providers listening on ports from 8000
   */
  
  
  OATPP_CREATE_COMPONENT(std::shared_ptr<std::list<std::shared_ptr<oatpp::network::ServerConnectionProvider>>>, connectionProviders)([this] {
    auto providers = std::make_shared<std::list<std::shared_ptr<oatpp::network::ServerConnectionProvider>>>();
    OATPP_COMPONENT(std::shared_ptr<Config>, m_cmdArgs);
    auto port = m_cmdArgs->base_port;
    if (m_cmdArgs->use_mtls) {
        port = m_cmdArgs->mtls_base_port;
    }
    
    // create ssl context 
    //initialize openssl
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    
    // manually create the context
    const SSL_METHOD* method = TLS_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        char errMsg[256];
        ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
        OATPP_LOGe(TAG, "Failed to create SSL_CTX = {}", errMsg)
        exit(-1);
    }
    //set call back and phrase 
    SSL_CTX_set_default_passwd_cb(ctx, passwordCB);
    SSL_CTX_set_default_passwd_cb_userdata(ctx, &m_cmdArgs->server_phrase);
    
    //load keys and certificates
      if (SSL_CTX_use_certificate_file(ctx, m_cmdArgs->server_cert_filename.c_str(), SSL_FILETYPE_PEM) <= 0) {
          char errMsg[256];
          ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
          OATPP_LOGe(TAG, "Failed to load - {} file, err= {}", m_cmdArgs->server_cert_filename, errMsg)
          exit(-1);
      }
    
      if (SSL_CTX_use_PrivateKey_file(ctx, m_cmdArgs->private_key_filename.c_str(), SSL_FILETYPE_PEM) <= 0) {
          char errMsg[256];
          ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
          OATPP_LOGe(TAG, "Failed to load private key - {} file, err= {}", m_cmdArgs->private_key_filename, errMsg)
          exit(-1);
      }
    
      if (SSL_CTX_load_verify_locations(ctx, m_cmdArgs->ca_key_file_name.c_str(), nullptr) <= 0) {
          char errMsg[256];
          ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
          OATPP_LOGe(TAG, "Failed to load verify locations - {} file, err= {}", m_cmdArgs->ca_key_file_name, errMsg)
          exit(-1);
      }
    
      if (!SSL_CTX_check_private_key(ctx)) {
          char errMsg[256];
          ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
          OATPP_LOGe(TAG, "Failed to SSL_CTX_check_private_key, err= {}", errMsg)
          exit(-1);
      }
    
      SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
      SSL_CTX_set_verify_depth(ctx, 5); // certification chin limitation
      
      
      for (v_uint8 i = 0; i < m_cmdArgs->number_of_ports; i++) {
        // if (m_cmdArgs->network_family_type == oatpp::network::Address::Family::IP_4) {
        //     ip_addr = base_ip + "." + std::to_string(i);
        // } else {
        //     ip_addr = base_ip + "::" + std::to_string(i); 
        // }
        
        // if (m_cmdArgs->network_family_type == oatpp::network::Address::Family::IP_6 && !is_valid_ipv6(ip_addr)) {
        //     // cppcheck-suppress unknownMacro
        //     OATPP_LOGe(TAG, "IP address is not IPv6 {} address", ip_addr)
        //     exit(-1);
        // } else if (m_cmdArgs->network_family_type == oatpp::network::Address::Family::IP_4 && !isValidIPv4(ip_addr)) {
        //     OATPP_LOGe(TAG, "IP address is not IPv4 {} address", ip_addr)
        //     exit(-1);
        // }
        
        try {
            
            OATPP_LOGd("AppComponent", "Connection Provider for address: {}:{}", m_cmdArgs->server_address, port + i)
            
//            auto provider = oatpp::openssl::server::ConnectionProvider::createShared()createShared(
//                            oatpp::network::Address(m_cmdArgs->server_address,
//                                                    port + i));
            if (m_cmdArgs->use_mtls) {
                auto config = oatpp::openssl::Config::createShared();
                config->configureContext(ctx);
/*
                //OATPP_LOGd(__func__, "certificate file name = {}", m_cmdArgs->server_cert_filename);
                config->addContextConfigurer(
                        std::make_shared<oatpp::openssl::configurer::CertificateChainFile>(m_cmdArgs->server_cert_filename));
                //OATPP_LOGd(__func__, "private key  file name = {}", m_cmdArgs->private_key_filename);
                config->addContextConfigurer(
                        std::make_shared<oatpp::openssl::configurer::PrivateKeyFile>(m_cmdArgs->private_key_filename));
                //OATPP_LOGd(__func__, "ca file name = {} ", m_cmdArgs->ca_key_file_name);
                config->addContextConfigurer(
                        // need to add the directory later
                        std::make_shared<oatpp::openssl::configurer::TrustStore>(m_cmdArgs->ca_key_file_name, nullptr));
                // strong require client certificate and week will validate only if exists (can use only TLS)
                config->addContextConfigurer(
                        std::make_shared<oatpp::openssl::configurer::PeerCertificateVerification>(
                                oatpp::openssl::configurer::CertificateVerificationMode::EnabledStrong));
*/
                providers->push_back(oatpp::openssl::server::ConnectionProvider::createShared(config,
                                          oatpp::network::Address(m_cmdArgs->server_address, port + i)));
            } else {
//                auto provider = oatpp::network::tcp::server::ConnectionProvider::createShared(
//                        oatpp::network::Address(m_cmdArgs->server_address,
//                                                port + i));
                providers->push_back(oatpp::network::tcp::server::ConnectionProvider::createShared(
                               oatpp::network::Address(m_cmdArgs->server_address, port + i)));
            }
            
        } catch (const std::exception& e) {
            OATPP_LOGe(__func__, "thread fail while trying to set connection, error :", e.what());
            exit(-1);
        }
    }
    return providers;
    }());
  

    /**
     *  Create server ConnectionProvider component which listens on the port for 
     */
    // OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, serverConnectionProvider)([this] {
    //     OATPP_COMPONENT(std::shared_ptr<oatpp::base::CommandLineArguments>, m_cmdArgs);
    //     //OATPP_LOGd(__func__, " {}", __LINE__)
    //     auto port = (v_int16) oatpp::utils::Conversion::strToInt32(
    //             m_cmdArgs->getNamedArgumentValue("--port", "8020" /* default value */));
    //     auto network_type = oatpp::utils::Conversion::strToInt32(m_cmdArgs->getNamedArgumentValue("--ntype", "4"));
    //     auto network_family_type = oatpp::network::Address::Family::IP_4;
    //     if (network_type == 6) {
    //         network_family_type = oatpp::network::Address::Family::IP_6;
    //     }
    //     auto address = m_cmdArgs->getNamedArgumentValue("--addr", "0.0.0.0");
    //     if (network_family_type == oatpp::network::Address::Family::IP_6 && !is_valid_ipv6(address)) {
    //         // cppcheck-suppress unknownMacro
    //         OATPP_LOGe(TAG, "IP address is not IPv6 {} address", address)
    //         exit(-1);
    //     } else if (network_family_type == oatpp::network::Address::Family::IP_4 && !isValidIPv4(address)) {
    //         OATPP_LOGe(TAG, "IP address is not IPv4 {} address", address)
    //         exit(-1);
    //     }
        
    //     auto addr = oatpp::network::Address(address, port, network_family_type);
        
    //     return oatpp::network::tcp::server::ConnectionProvider::createShared(
    //             oatpp::network::Address(address, port, network_family_type));
        
    // }());
    
    /**
     *  Create Router component
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, httpRouter)([] {
        //OATPP_LOGd(__func__, " {}", __LINE__)
        return oatpp::web::server::HttpRouter::createShared();
    }());
    
    /**
     *  Create ConnectionHandler component which uses Router component to route requests
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, serverConnectionHandler)("http", [] {
        //OATPP_LOGd(__func__, " {}", __LINE__)
        OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router); // get Router component
        OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor); // get Async executor component
        return oatpp::web::server::AsyncHttpConnectionHandler::createShared(router, executor);
    }());
    
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::json::ObjectMapper>, Objectmapper)([] {
        //OATPP_LOGd(__func__, " {}", __LINE__)
        return std::make_shared<oatpp::json::ObjectMapper>();
    }());
    
    /**
     *  Create ObjectMapper component to serialize/deserialize DTOs in Contoller's API
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, apiObjectMapper)("clientObjectMapper", [] {
        //OATPP_LOGd(__func__, " {}", __LINE__)
        return std::make_shared<oatpp::json::ObjectMapper>();
    }());
    
    
    /**
 *  Create ObjectMapper component to serialize/deserialize in client API
 */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, clientApiObjectmapper)([] {
        //OATPP_LOGd(__func__, " {}", __LINE__)
        return std::make_shared<oatpp::json::ObjectMapper>();
    }());
    
    
    
    /**
     *  Create websocket connection handler
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, websocketConnectionHandler)("websocket", [] {
        //OATPP_LOGd(__func__, " {}", __LINE__)
        OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor);
        //OATPP_LOGd(__func__, " {}", __LINE__)
    
        auto connectionHandler = oatpp::websocket::AsyncConnectionHandler::createShared(executor);
        connectionHandler->setSocketInstanceListener(std::make_shared<WSInstanceListener>());
        //OATPP_LOGd(__func__, " {}", __LINE__)
    
        return connectionHandler;
    }());
    
    OATPP_CREATE_COMPONENT(std::shared_ptr<ScyllaDBManager>, dbManger)("scyllaDBManager", [] {
        //OATPP_LOGd(__func__, " {}", __LINE__)
        OATPP_COMPONENT(std::shared_ptr<Config>, m_cmdArgs);
        auto addr = m_cmdArgs->scylladb_address;
        auto port = m_cmdArgs->scylladb_port;
        return std::make_shared<ScyllaDBManager>(addr, m_cmdArgs->host_ip, port);
    }());
    
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::client::RequestExecutor>, requestExcecutor)("clientExcecutor", [this] {
        //OATPP_COMPONENT(std::shared_ptr<oatpp::base::CommandLineArguments>, m_cmdArgs);
        //OATPP_LOGd(__func__, " {}", __LINE__)
        OATPP_COMPONENT(std::shared_ptr<Config>, m_cmdArgs);
        char* tmp;
        auto port = (v_int16)strtol(m_cmdArgs->http_request_port.c_str(), &tmp, 10);
        auto addr = m_cmdArgs->http_request_address;
        oatpp::network::Address address(addr, port);
        auto connectionProvider = std::make_shared<oatpp::network::tcp::client::ConnectionProvider>(address);
        return std::make_shared<oatpp::web::client::HttpRequestExecutor>(connectionProvider);
        
    }());
    
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::client::ApiClient>, clientApi)("clientapi", [this] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, apiObjectMapper, "clientObjectMapper");
        OATPP_COMPONENT(std::shared_ptr<oatpp::web::client::RequestExecutor>, requestExcecutor, "clientExcecutor");
        
        return oatpp::web::client::ApiClient::createShared(requestExcecutor, apiObjectMapper);
        //return std::make_shared<oatpp::web::client::ApiClient>(requestExcecutor, apiObjectMapper);
        
    }());

private:
    static constexpr const char *TAG = "Server_Init";
    
    //oatpp::base::CommandLineArguments m_cmdArgs;
};

#endif //VGEATWAY_APPCOMPONENT_H
