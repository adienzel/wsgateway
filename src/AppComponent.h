
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
#include "utilis/env.h"

struct Config {
    Config() {
        partial_address = EnvUtils::getEnvString("WSS_PARTIAL_ADDRESS", "192.168.1");
        
        // cppcheck-suppress syntaxError
        if (auto found = partial_address.find("."); found == std::string::npos) {
            if (found = partial_address.find("::"); found == std::string::npos) {
                OATPP_LOGe(__func__, "WSS_PARTIAL_ADDRESS {} is not leagal prefix of IP addrtess, Please use leagal ip v4 or v6 values", partial_address)
                exit(-1);
            } else {
                network_family_type = oatpp::network::Address::Family::IP_6;
            }
        }
        if (network_family_type == oatpp::network::Address::Family::IP_6 &&
             partial_address.substr(partial_address.size() - 2) == "::") {
                partial_address.erase(partial_address.size() - 2);
        } else if (network_family_type == oatpp::network::Address::Family::IP_4 &&
             partial_address.back() == '.') {
                partial_address.pop_back();
        }
    

        start_address   = (int8_t)EnvUtils::getEnvInt("WSS_START_ADDRESS", 3);
        end_address = EnvUtils::getEnvInt("WSS_END_ADDRESS", 24);
        port = EnvUtils::getEnvString("WSS_PORT", "8020");
        http_request_address = EnvUtils::getEnvString("WSS_HTTP_REQUEST_ADDRESS", "127.0.0.1");
        http_request_port = EnvUtils::getEnvString("WSS_HTTP_REQUEST_PORT", "8992");

        number_of_worker_threads = 
               (int32_t)EnvUtils::getEnvInt("WSS_NUMBER_OF_WORKER_THREADS", 24);
        number_of_io_threads = 
               (int32_t)EnvUtils::getEnvInt("WSS_NUMBER_OF_IO_THREADS", 24);
        number_of_timer_threads =
               (int32_t)EnvUtils::getEnvInt("WSS_NUMBER_OF_TIMER_THREADS", 1);
    
        scylladb_address = 
               EnvUtils::getEnvString("WSS_SCYLLA_DB_ADDRESS", "192.168.3.2");
        scylladb_port = 
               EnvUtils::getEnvString("WSS_SCYLLADB_PORT", "9060");
        scylladb_keyspace_name = 
               EnvUtils::getEnvString("WSS_SCYLLADB_KEYSPACE_NAME", "vin");
        scylladb_replication_factor =
               (int32_t)EnvUtils::getEnvInt("WSS_SCYLLADB_REPLICATION_FACTOR", 1);
        scylladb_strategy = 
               EnvUtils::getEnvString("WSS_SCYLLADB_STRATEGY", "SimpleStrategy");
        scylladb_table_name = 
               EnvUtils::getEnvString("WSS_SCYLLADB_TABLE_NAME", "vehicles");
    }

    std::string partial_address;
    oatpp::network::Address::Family network_family_type = oatpp::network::Address::Family::IP_4; 
    int8_t start_address;
    int8_t end_address;
    std::string port;
    std::string http_request_address;
    std::string http_request_port;

    int32_t number_of_worker_threads;
    int32_t number_of_io_threads;
    int32_t number_of_timer_threads;

    std::string scylladb_address;
    std::string scylladb_port;
    std::string scylladb_keyspace_name;
    int32_t scylladb_replication_factor;
    std::string scylladb_strategy;
    std::string scylladb_table_name;

};


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

    uint16_t start_class_c_addr = m_cmdArgs->start_address;
    uint16_t end_class_c_addr = m_cmdArgs->end_address;
    std::string base_ip = m_cmdArgs->partial_address;

    char* tmp;
    v_uint16 listening_port = strtol(m_cmdArgs->port.c_str(), &tmp,  10);
    
    std::string ip_addr = "";
    
    for(v_uint16 i = start_class_c_addr; i < end_class_c_addr + 1; i++) {
        if (m_cmdArgs->network_family_type == oatpp::network::Address::Family::IP_4) {
            ip_addr = base_ip + "." + std::to_string(i);
        } else {
            ip_addr = base_ip + "::" + std::to_string(i); 
        }
        
        if (m_cmdArgs->network_family_type == oatpp::network::Address::Family::IP_6 && !is_valid_ipv6(ip_addr)) {
            // cppcheck-suppress unknownMacro
            OATPP_LOGe(TAG, "IP address is not IPv6 {} address", ip_addr)
            exit(-1);
        } else if (m_cmdArgs->network_family_type == oatpp::network::Address::Family::IP_4 && !isValidIPv4(ip_addr)) {
            OATPP_LOGe(TAG, "IP address is not IPv4 {} address", ip_addr)
            exit(-1);
        }
        try {
            OATPP_LOGd("AppComponent", "Connection Provider for address: {}:{}", ip_addr, listening_port)
            auto provider = oatpp::network::tcp::server::ConnectionProvider::createShared(
                            oatpp::network::Address(ip_addr,
                                                    listening_port,
                                                    m_cmdArgs->network_family_type));
            providers->push_back(provider);
        } catch (...) {
            OATPP_LOGe(__func__, "IP address is not IPv4 {}:{} address ", ip_addr, listening_port)
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
        OATPP_COMPONENT(std::shared_ptr<Config>, m_cmdArgs);
        auto addr = m_cmdArgs->scylladb_address;
        return std::make_shared<ScyllaDBManager>(addr);
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

private:
    static constexpr const char *TAG = "Server_Init";
    
    //oatpp::base::CommandLineArguments m_cmdArgs;
};

#endif //VGEATWAY_APPCOMPONENT_H
