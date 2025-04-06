#include "controler/WsController.h"
#include "AppComponent.h"
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/err.h>
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
    
        SSL_CTX* ctx;
        
        if (m_cmdArgs->use_mtls) {
            SSL_library_init();
            SSL_load_error_strings();
            OpenSSL_add_all_algorithms();
    
            const SSL_METHOD *method = TLS_server_method();
            ctx = SSL_CTX_new(method);
    
    
            if (!ctx) {
                ERR_print_errors_fp(stderr);
                exit(EXIT_FAILURE);
            }
    
            // Set the minimum and maximum protocol versions to TLS 1.3
            SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
            SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);
    
            if (SSL_CTX_use_certificate_file(ctx, m_cmdArgs->cert_filename.c_str(), SSL_FILETYPE_PEM) <= 0) {
                ERR_print_errors_fp(stderr);
                OATPP_LOGe(__func__, "Error in SSL_CTX_use_certificate_file {}", m_cmdArgs->cert_filename)
                exit(EXIT_FAILURE);
            }
            if (SSL_CTX_use_PrivateKey_file(ctx, m_cmdArgs->private_key_filename.c_str(), SSL_FILETYPE_PEM) <= 0) {
                ERR_print_errors_fp(stderr);
                OATPP_LOGe(__func__, "Error in SSL_CTX_use_PrivateKey_file {}", m_cmdArgs->private_key_filename)
                exit(EXIT_FAILURE);
            }
            if (SSL_CTX_load_verify_locations(ctx, m_cmdArgs->ca_key_file_name.c_str(), nullptr) <= 0) {
                ERR_print_errors_fp(stderr);
                OATPP_LOGe(__func__, "Error in SSL_CTX_load_verify_locations {}", m_cmdArgs->ca_key_file_name)
                exit(EXIT_FAILURE);
            }
    
            SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    
        }
        //OATPP_LOGd(__func__, " {}", __LINE__)
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

        for (auto& thread : threads) {
            thread.join();
        }
            
    //     /* Get connection provider component */
    //     OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, connectionProvider);
    //     /* Create server which takes provided TCP connections and passes them to HTTP connection handler */
    //     oatpp::network::Server server(connectionProvider, connectionHandler);
        
    //     /* Priny info about server port */
    // //    OATPP_LOGi("MyApp", "Server running on port {}", connectionProvider->getProperty("port").getData());
    //     auto port = (v_int16)oatpp::utils::Conversion::strToInt32(args.getNamedArgumentValue("--port", "8020" /* default value */));
    //     OATPP_LOGi("MyApp", "Server running on port {}", port)
        
    //     /* Run server */
    //     server.run();
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
