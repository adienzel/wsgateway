#include "controler/WsController.h"
#include "AppComponent.h"

#include "oatpp/network/Server.hpp"

#include <iostream>


void run(int argc, const char * argv[]) {
//void run(const oatpp::base::CommandLineArguments& args) {
    auto args = oatpp::base::CommandLineArguments(argc, argv);
    /* Register Components in scope of run() method */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::base::CommandLineArguments>, m_cmdArgs)([&] {
        return std::make_shared<oatpp::base::CommandLineArguments>(argc, argv);
    }());
    
    //OATPP_LOGd(__func__, " {}", __LINE__)
    AppComponent components;
    //AppComponent components(argc, argv);
    //OATPP_LOGd(__func__, " {}", __LINE__)
    
    auto addr = args.getNamedArgumentValue("--addr", "127.0.0.1");
    //OATPP_LOGd(__func__, " {}", __LINE__)
    if (addr == nullptr && !isValidIPv4(addr)) {
        OATPP_LOGe(__func__, "bad/empty address parameter")
        exit(-1);
    }
    //OATPP_LOGd(__func__, " {}", __LINE__)
    OATPP_COMPONENT(std::shared_ptr<ScyllaDBManager>, dbManager, "scyllaDBManager");
    //OATPP_LOGd(__func__, " {}", __LINE__)
    if (!dbManager->connect(addr)) {
        OATPP_LOGe(__func__, "Error connecting to scylaDB")
        exit(-1);
    }
    std::string keyspace = "VIN";
    if (!dbManager->create_keyspace(keyspace, "SimpleStrategy", 1)) {
        OATPP_LOGe(__func__, "Error creating keyspace {}", keyspace)
        exit(-1);
    }
 

    std::string table = "vehicles"; 
    if (!dbManager->create_table(table)) {
        OATPP_LOGe(__func__, "Error creating table in keyspace {}", keyspace)
        exit(-1);
    }
    if (!dbManager->create_materialized_view()) {
        OATPP_LOGe(__func__, "Error creating matirialized view in keyspace {}", keyspace)
        exit(-1);
    }
    
    //OATPP_LOGd(__func__, " {}", __LINE__)
    
    /* Get router component from environment */
    OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);
    
    /* Create MyController and add all of its endpoints to router */
    router->addController(std::make_shared<WsController>());
    
    /* Get connection handler component */
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler, "http");
    
    /* Get connection provider component */
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, connectionProvider);
    /* Create server which takes provided TCP connections and passes them to HTTP connection handler */
    oatpp::network::Server server(connectionProvider, connectionHandler);
    
    /* Priny info about server port */
//    OATPP_LOGi("MyApp", "Server running on port {}", connectionProvider->getProperty("port").getData());
    auto port = (v_int16)oatpp::utils::Conversion::strToInt32(args.getNamedArgumentValue("--port", "8020" /* default value */));
    OATPP_LOGi("MyApp", "Server running on port {}", port)
    
    /* Run server */
    server.run();
    
}

int main(int argc, const char * argv[]) {
    
    oatpp::Environment::init();
    
    run(argc, argv);
    
    oatpp::Environment::destroy();
    
    return 0;
}
