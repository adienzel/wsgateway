#include "controler/WsController.h"
#include "AppComponent.h"
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/err.h>
#include <oatpp-1.4.0/oatpp-openssl/oatpp-openssl/Config.hpp>
#include <oatpp-1.4.0/oatpp-openssl/oatpp-openssl/server/ConnectionProvider.hpp>

#include "oatpp/network/Server.hpp"
#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/web/server/HttpRouter.hpp"


#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <exception>




void run(int argc, const char * argv[]) {
//void run(const oatpp::base::CommandLineArguments& args) {
    //auto args = oatpp::base::CommandLineArguments(argc, argv);
    /* Register Components in scope of run() method */
    try {
        AppComponent components;
        OATPP_COMPONENT(std::shared_ptr<Config>, m_cmdArgs);
    
        OATPP_COMPONENT(std::shared_ptr<ScyllaDBManager>, dbManager, "scyllaDBManager");
        //OATPP_LOGd(__func__, " {}", __LINE__)
        if (!dbManager->connect(m_cmdArgs->scylladb_address)) {

            OATPP_LOGe(__func__, "Error connecting to scylaDB")
            exit(-1);
        }
        if (!dbManager->create_keyspace(m_cmdArgs->scylladb_keyspace_name,
                                        m_cmdArgs->scylladb_strategy,
                                        m_cmdArgs->scylladb_replication_factor)) {
            OATPP_LOGe(__func__, "Error creating keyspace {}", m_cmdArgs->scylladb_keyspace_name)
            exit(-1);
        }
    

        if (!dbManager->create_table(m_cmdArgs->scylladb_table_name)) {
            OATPP_LOGe(__func__, "Error creating table in keyspace {}", m_cmdArgs->scylladb_keyspace_name)
            exit(-1);
        }
        if (!dbManager->create_materialized_view()) {
            OATPP_LOGe(__func__, "Error creating matirialized view in keyspace {}", m_cmdArgs->scylladb_keyspace_name)
            exit(-1);
        }
        
        //OATPP_LOGd(__func__, " {}", __LINE__)
        
        /* Get router component from environment */
        OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);
    
        //OATPP_LOGd(__func__, " {}", __LINE__)
    
        /* Create MyController and add all of its endpoints to router */
        //router->addController(std::make_shared<WsController>());
        router->addController(WsController::createShared());
    
        //OATPP_LOGd(__func__, " {}", __LINE__)
        /* create servers */
        OATPP_COMPONENT(std::shared_ptr<std::list<std::shared_ptr<oatpp::network::ServerConnectionProvider>>>, connectionProviders);
    
        OATPP_COMPONENT(std::shared_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>, workGuard);

        OATPP_COMPONENT(std::shared_ptr<boost::asio::io_context>, ioc);
        std::thread ioThread([ioc]() {
            std::cout << "[ioThread] Running io_context" << std::endl;
            ioc->run();  // This will now keep running even with no work
            std::cout << "[ioThread] io_context exited" << std::endl;
        });
    
    
        std::list<std::thread> threads;

        for (auto& provider : *connectionProviders) {
            try {
                threads.emplace_back([provider]{
                    /* Get connection handler component */
                        //OATPP_LOGi(__func__, "thread {} is running", tid)
                        
                        OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler, "http");
                        oatpp::network::Server server(provider, connectionHandler);
                        
                        //auto tid = gettid();
                        //OATPP_LOGi(__func__, "thread {} is running, line = {}", gettid(), __LINE__)
                        
                        server.run();
                });
            } catch (const std::exception& e) {
                OATPP_LOGe(__func__, "thread fail {}", e.what())
            }
        }
    
        
        workGuard->reset();
        ioThread.join();
    
        for (auto& thread : threads) {
            thread.join();
        }
    
        oatpp::Environment::destroy();
            
    } catch (const std::exception& e) {
            OATPP_LOGe(__func__, "thread fail {}", e.what())
    }
}

int main(int argc, const char * argv[]) {
    
    oatpp::Environment::init();
    
    run(argc, argv);
    
    oatpp::Environment::destroy();
    
    return 0;
}
