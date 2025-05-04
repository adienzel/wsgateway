
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
#include "utilis/sslContext.h"

#include "client/RestClient.h"


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
  
  
    OATPP_CREATE_COMPONENT(std::shared_ptr<std::list<std::shared_ptr<oatpp::network::ServerConnectionProvider>>>, connectionProviders) ([this] {
        auto providers = std::make_shared<std::list<std::shared_ptr<oatpp::network::ServerConnectionProvider>>>();
        OATPP_COMPONENT(std::shared_ptr<Config>, m_cmdArgs);
        auto port = m_cmdArgs->base_port;
        SSL_CTX *ctx;
        if (m_cmdArgs->use_mtls) {
            port = m_cmdArgs->mtls_base_port;
            ctx = getSSLContext(m_cmdArgs);
        }
    
        for (v_uint8 i = 0; i < m_cmdArgs->number_of_ports; i++) {
            try {
                OATPP_LOGd("AppComponent", "Connection Provider for address: {}:{}", m_cmdArgs->server_address,
                           port + i)
            
                if (m_cmdArgs->use_mtls) {
                    auto config = oatpp::openssl::Config::createShared();
                    config->configureContext(ctx);
                    providers->push_back(oatpp::openssl::server::ConnectionProvider::createShared(config,
                                                                                                  oatpp::network::Address(
                                                                                                          m_cmdArgs->server_address,
                                                                                                          port + i)));
                } else {
                    providers->push_back(oatpp::network::tcp::server::ConnectionProvider::createShared(
                            oatpp::network::Address(m_cmdArgs->server_address, port + i)));
                }
            
            } catch (const std::exception &e) {
                OATPP_LOGe(__func__, "thread fail while trying to set connection, error :", e.what());
                exit(-1);
            }
        }
        return providers;
    }());
  
    
    /**
     *  Create Router component
     */

    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, httpRouter)([] {
        //OATPP_LOGd(__func__, " {}", __LINE__)
        return oatpp::web::server::HttpRouter::createShared();
    }());
    
    OATPP_CREATE_COMPONENT(std::shared_ptr<boost::asio::io_context>, io_context) ([] {
        auto ioc =  std::make_shared<boost::asio::io_context>();
        return ioc;
    }());
    
    OATPP_CREATE_COMPONENT(std::shared_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>, workGuard) ([] {
        OATPP_COMPONENT(std::shared_ptr<boost::asio::io_context>, ioc);
        auto workGuard = std::make_shared<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
                boost::asio::make_work_guard(*ioc));
        return workGuard;
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
