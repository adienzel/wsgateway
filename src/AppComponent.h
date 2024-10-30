
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

#include "ScyllaDBManager.h"

class AppComponent {
public:
//    explicit AppComponent(const oatpp::base::CommandLineArguments &cmdArgs) : m_cmdArgs(cmdArgs) {
//        //OATPP_LOGd(__func__, " {}", __LINE__)
//
//    }
    AppComponent() = default;
    
//    explicit AppComponent(int argc, const char *argv[]) {
//        //m_cmdArgs = oatpp::base::CommandLineArguments(argc, argv);
//        //OATPP_LOGd(__func__, " {}", __LINE__)
//    
//        OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::base::CommandLineArguments>, m_cmdArgs)([&] {
//            return std::make_shared<oatpp::base::CommandLineArguments>(argc, argv);
//        }());
//    
//    };
    
  
    
    /**
     * Create Async Executor
     */
    
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor)([this] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::base::CommandLineArguments>, m_cmdArgs);
        //OATPP_LOGd(__func__, " {}", __LINE__)
        auto num_of_worker_threads = oatpp::utils::Conversion::strToInt32(m_cmdArgs->getNamedArgumentValue("--wt", "4"));
        auto num_of_io_threads = oatpp::utils::Conversion::strToInt32(m_cmdArgs->getNamedArgumentValue("--iot", "1"));
        auto num_of_timer_threads = oatpp::utils::Conversion::strToInt32(m_cmdArgs->getNamedArgumentValue("--tt", "1"));
        return std::make_shared<oatpp::async::Executor>(
                num_of_worker_threads /* Data-Processing threads */,
                num_of_io_threads /* I/O threads */,
                num_of_timer_threads /* Timer threads */
        );
    }());
    
    /**
     *  Create server ConnectionProvider component which listens on the port for 
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, serverConnectionProvider)([this] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::base::CommandLineArguments>, m_cmdArgs);
        //OATPP_LOGd(__func__, " {}", __LINE__)
        auto port = (v_int16) oatpp::utils::Conversion::strToInt32(
                m_cmdArgs->getNamedArgumentValue("--port", "8020" /* default value */));
        auto network_type = oatpp::utils::Conversion::strToInt32(m_cmdArgs->getNamedArgumentValue("--ntype", "4"));
        auto network_family_type = oatpp::network::Address::Family::IP_4;
        if (network_type == 6) {
            network_family_type = oatpp::network::Address::Family::IP_6;
        }
        auto address = m_cmdArgs->getNamedArgumentValue("--addr", "0.0.0.0");
        if (network_family_type == oatpp::network::Address::Family::IP_6 && !is_valid_ipv6(address)) {
            OATPP_LOGe(TAG, "IP address is not IPv6 {} address", address)
            exit(-1);
        } else if (network_family_type == oatpp::network::Address::Family::IP_4 && !isValidIPv4(address)) {
            OATPP_LOGe(TAG, "IP address is not IPv4 {} address", address)
            exit(-1);
        }
        
        auto addr = oatpp::network::Address(address, port, network_family_type);
        
        return oatpp::network::tcp::server::ConnectionProvider::createShared(
                oatpp::network::Address(address, port, network_family_type));
        
    }());
    
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
    
    /**
     *  Create ObjectMapper component to serialize/deserialize DTOs in Contoller's API
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, apiObjectMapper)([] {
        //OATPP_LOGd(__func__, " {}", __LINE__)
        return std::make_shared<oatpp::json::ObjectMapper>();
    }());

//  std::shared_ptr<oatpp::data::mapping::ObjectMapper> apiObjectMapper = std::make_shared<oatpp::json::ObjectMapper>();
    
    /**
     *  Create websocket connection handler
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, websocketConnectionHandler)("websocket", [] {
        //OATPP_LOGd(__func__, " {}", __LINE__)
        OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor);
        auto connectionHandler = oatpp::websocket::AsyncConnectionHandler::createShared(executor);
        connectionHandler->setSocketInstanceListener(std::make_shared<WSInstanceListener>());
        return connectionHandler;
    }());
    
    OATPP_CREATE_COMPONENT(std::shared_ptr<ScyllaDBManager>, dbManger)("scyllaDBManager", [] {
        //OATPP_LOGd(__func__, " {}", __LINE__)
        return std::make_shared<ScyllaDBManager>("127.0.0.1");
    }());
    
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::client::RequestExecutor>, requestExcecutor)("clientExcecutor", [this] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::base::CommandLineArguments>, m_cmdArgs);
        //OATPP_LOGd(__func__, " {}", __LINE__)
        auto port = (v_int16) oatpp::utils::Conversion::strToInt32(m_cmdArgs->getNamedArgumentValue("--http-rest-port", "9070"));
        auto addr = m_cmdArgs->getNamedArgumentValue("--http-rest-addr", "127.0.0.1");
        oatpp::network::Address address(addr, port);
        auto connectionProvider = std::make_shared<oatpp::network::tcp::client::ConnectionProvider>(address);
        return std::make_shared<oatpp::web::client::HttpRequestExecutor>(connectionProvider);
        
    }());

private:
    static constexpr const char *TAG = "Server_Init";
    
    //oatpp::base::CommandLineArguments m_cmdArgs;
};

#endif //VGEATWAY_APPCOMPONENT_H
