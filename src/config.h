
#ifndef VGATEWAY_CONFIG_H
#define VGATEWAY_CONFIG_H

#include "utilis/ipUtils.h"
#include "utilis/env.h"
#include "utilis/split.h"


struct Config {
    Config() {
        auto [host, ip, err] = getHostAndIP();
        if (err != 0) {
            OATPP_LOGe(__func__, "failed to get host and ip, {} ", host)
        } else {
            host_ip = host;
            OATPP_LOGi(__func__, "host = {}, ip = {} ", host, ip)
        }
        
        server_address   = EnvUtils::getEnvString("WSS_ADDRESS", "0.0.0.0");
        
        number_of_ports = (uint8_t)EnvUtils::getEnvInt("WSS_NUMBER_OF_PORTS", 24);
        base_port = (uint16_t)EnvUtils::getEnvInt("WSS_PORT", 8020);
        http_request_address = EnvUtils::getEnvString("WSS_HTTP_REQUEST_ADDRESS", "127.0.0.1");
        http_request_port = EnvUtils::getEnvString("WSS_HTTP_REQUEST_PORT", "8992");
        
        // OATPP config
        number_of_worker_threads =
                (uint32_t)EnvUtils::getEnvInt("WSS_NUMBER_OF_WORKER_THREADS", 24);
        number_of_io_threads =
                (uint32_t)EnvUtils::getEnvInt("WSS_NUMBER_OF_IO_THREADS", 24);
        number_of_timer_threads =
                (uint32_t)EnvUtils::getEnvInt("WSS_NUMBER_OF_TIMER_THREADS", 1);
        
        //scylladb
        scylladb_address =
                EnvUtils::getEnvString("WSS_SCYLLA_DB_ADDRESS", "172.17.0.2");
        scylladb_port =
                EnvUtils::getEnvString("WSS_SCYLLADB_PORT", "9042");
        
        scylladb_keyspace_name = EnvUtils::getEnvString("WSS_SCYLLADB_KEYSPACE_NAME", "vin");
        
        
        
        
        auto tmp = EnvUtils::getEnvString("WSS_USE_MTLS", "false");
        std::transform(tmp.begin(), tmp.end(), tmp.begin(), [](unsigned char c){ return std::tolower(c); });
        use_mtls = tmp.compare("false") == 0;
        
        scylladb_replication_factor =
                (uint32_t)EnvUtils::getEnvInt("WSS_SCYLLADB_REPLICATION_FACTOR", 3);
        scylladb_strategy =
                EnvUtils::getEnvString("WSS_SCYLLADB_STRATEGY", "NetworkTopologyStrategy");
        scylladb_table_name =
                EnvUtils::getEnvString("WSS_SCYLLADB_TABLE_NAME", "vehicles");
        
        
        cert_filename = EnvUtils::getEnvString("WSS_CA_FILE_NAME", "ca.crt");
        base_port = (uint16_t)EnvUtils::getEnvInt("WSS_MTLS_BASE_PORT", 8443);
        private_key_filename = EnvUtils::getEnvString("WSS_PRIVATE_KEY_FILE_NAME", "server.key");
        cert_filename = EnvUtils::getEnvString("WSS_SERVER_CERTIFICATE_FILE_NAME", "server.crt");
        
    }
    
    std::string host_ip;
    std::string server_address;
    oatpp::network::Address::Family network_family_type = oatpp::network::Address::Family::IP_4;
    uint8_t number_of_ports;
    uint16_t base_port;
    std::string http_request_address;
    std::string http_request_port;
    
    uint32_t number_of_worker_threads;
    uint32_t number_of_io_threads;
    uint32_t number_of_timer_threads;
    
    std::string scylladb_address;
    std::string scylladb_port;
    std::string scylladb_keyspace_name;
    uint32_t scylladb_replication_factor;
    std::string scylladb_strategy;
    std::string scylladb_table_name;
    
    bool use_mtls;
    uint16_t mtls_base_port;
    std::string cert_filename;
    std::string private_key_filename;
    std::string ca_key_file_name;
    
};

#endif //VGATEWAY_CONFIG_H
