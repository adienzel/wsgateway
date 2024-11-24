
#ifndef VGEATWAY_SCYLLADBMANAGER_H
#define VGEATWAY_SCYLLADBMANAGER_H

#include <cassandra.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <optional>

class ScyllaDBManager {
public:
    
    /**
     * constructor
     * @param hosts 
     */
    explicit ScyllaDBManager(const std::string_view& scylladb_server__name, const std::string_view& app_server_name) : 
                              m_scylladb_server__name(scylladb_server__name), m_app_server_name(app_server_name) {
        cluster = std::shared_ptr<CassCluster>(cass_cluster_new(), cass_cluster_free);
        session = std::shared_ptr<CassSession>(cass_session_new(), cass_session_free);
    }
    
    [[nodiscard]] std::string getAppServerName() const { return m_app_server_name;}
    ScyllaDBManager(const ScyllaDBManager&) = delete;
    ScyllaDBManager(const ScyllaDBManager&&) = delete;
    ScyllaDBManager& operator = (const ScyllaDBManager&) = delete;
    
    [[nodiscard]] bool connect(const std::string& hosts) const {
        cass_cluster_set_contact_points(cluster.get(), hosts.c_str());
    
        CassFuture* connect_future = cass_session_connect(session.get(), cluster.get());
        bool success = cass_future_error_code(connect_future) == CASS_OK;
        if (!success) {
            print_error(connect_future);
        }
        cass_future_free(connect_future);
        return success;
    }
    
    [[nodiscard]] bool create_keyspace(const std::string_view& keyspace, const std::string& strategy, int replication_factor) {
        m_keyspace = keyspace;
        std::string query = "CREATE KEYSPACE IF NOT EXISTS " + m_keyspace +
                            " WITH replication = {'class': '" + strategy +
                            "', 'replication_factor': " + 
                            std::to_string(replication_factor) + "} AND TABLETS = {'enabled': false};";
        return execute_query(query);
    }
    
    [[nodiscard]] bool create_table(const std::string_view& table) {
        m_table = table;
        std::string query = "CREATE TABLE IF NOT EXISTS " + m_keyspace + "." + m_table + " ("
                                                                       "vin text PRIMARY KEY, "
                                                                       "host text);";
        return execute_query(query);
    }
    
    [[nodiscard]] bool create_materialized_view() const {
        std::string query = "CREATE MATERIALIZED VIEW IF NOT EXISTS " + m_keyspace + "." + m_table +
                       "_by_host AS SELECT host, vin FROM " + m_keyspace + "." + m_table +
                       " WHERE host IS NOT NULL AND vin IS NOT NULL PRIMARY KEY (host, vin);";
        return execute_query(query);
    
    }
    
    [[nodiscard]] std::optional<std::string> query_table(const std::string& vin) {
        std::string query = "SELECT host FROM " + m_keyspace + "." + m_table +" WHERE vin = '" + vin + "';";
        CassStatement* statement = cass_statement_new(query.c_str(), 0);
        CassFuture* result_future = cass_session_execute(session.get(), statement);
        cass_statement_free(statement);
        
        if (cass_future_error_code(result_future) == CASS_OK) {
            const CassResult* result = cass_future_get_result(result_future);
            if (const CassRow *row = cass_result_first_row(result)) {
                const CassValue* value = cass_row_get_column_by_name(row, "host");
                const char* host;
                size_t host_length;
                cass_value_get_string(value, &host, &host_length);
                cass_future_free(result_future);
                return std::string(host, host_length);
            }
            cass_result_free(result);
        } else {
            print_error(result_future);
        }
        cass_future_free(result_future);
        return std::nullopt;
    }
    
    [[nodiscard]] std::vector<std::string> query_materialized_view(const std::string& host) const {
        std::string query = "SELECT vin FROM " + m_keyspace + "." + m_table + "_by_host WHERE host == '" + host;
        CassStatement* statement = cass_statement_new(query.c_str(), 0);
        CassFuture* result_future = cass_session_execute(session.get(), statement);
        cass_statement_free(statement);
        
        std::vector<std::string> vins;
        if (cass_future_error_code(result_future) == CASS_OK) {
            const CassResult* result = cass_future_get_result(result_future);
            CassIterator* iterator = cass_iterator_from_result(result);
            while (cass_iterator_next(iterator)) {
                const CassRow* row = cass_iterator_get_row(iterator);
                const CassValue* value = cass_row_get_column_by_name(row, "vin");
                const char* vin;
                size_t vin_length;
                cass_value_get_string(value, &vin, &vin_length);
                vins.emplace_back(vin, vin_length);
            }
            cass_iterator_free(iterator);
            cass_result_free(result);
        } else {
            print_error(result_future);
        }
        cass_future_free(result_future);
        return vins;
    }
    
    [[nodiscard]] bool insert_data(const std::string & vin, const std::string& host) const {
        std::string query = "INSERT INTO " + m_keyspace + "." + m_table + " (vin, host) VALUES ('" + vin + "', '" + host + "');";
        return execute_query(query);
    }
    
    [[nodiscard]] bool delete_data(const std::string& vin) const {
        std::string query = "DELETE FROM " + m_keyspace + "." + m_table + " WHERE vin = '" + vin + "';";
        return execute_query(query);
    }

private:
    std::shared_ptr<CassCluster> cluster;
    std::shared_ptr<CassSession> session;
    
    std::string m_keyspace;
    std::string m_table;
    std::string m_app_server_name;
    std::string m_scylladb_server__name;
    
    [[nodiscard]] bool execute_query(const std::string& query) const {
        CassStatement* statement = cass_statement_new(query.c_str(), 0);
        CassFuture* result_future = cass_session_execute(session.get(), statement);
        cass_statement_free(statement);
        
        bool success = cass_future_error_code(result_future) == CASS_OK;
        if (!success) {
            print_error(result_future);
        }
        cass_future_free(result_future);
        return success;
    }
    
    static void print_error(CassFuture* future) {
        const char* message;
        size_t message_length;
        cass_future_error_message(future, &message, &message_length);
        std::cerr << "Error: " << std::string(message, message_length) << std::endl;
    }
};

#endif //VGEATWAY_SCYLLADBMANAGER_H
