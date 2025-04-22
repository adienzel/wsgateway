
#ifndef VGATEWAY_BOOSTBEASTASYNCCLIENT_H
#define VGATEWAY_BOOSTBEASTASYNCCLIENT_H
//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP client, asynchronous
//
//------------------------------------------------------------------------------

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "utilis/HttpParser.h"
#include "../websocket/WebSocketComponents.h"


using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>
namespace beast = boost::beast;    // from <boost/beast/http.hpp>

//------------------------------------------------------------------------------

// Report a failure
void fail(boost::system::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

// Performs an HTTP GET and prints the response
class session : public std::enable_shared_from_this<session> {
    tcp::resolver resolver_;
    beast::tcp_stream stream_;
    tcp::socket socket_;
    boost::beast::flat_buffer buffer_; // (Must persist between reads)
    http::request<http::string_body> req_;
    http::response<http::string_body> res_;
    std::string vin_;

public:
    // Resolver and socket require an io_context
    explicit session(boost::asio::io_context& ioc) : 
              resolver_(net::make_strand(ioc)), socket_(net::make_strand(ioc)) stream_(net::make_strand(ioc)){}
    
    // Start the asynchronous operation
    void run(char const* host, char const* port, const std::string& msg, const std::string_view & vin, const struct timespec* t) {
        //parsh the msg 
        
        vin_ = vin;
        auto [method, url, version, headers, body] = createRequestFromBuffer(msg);
        req_.version(version == "1.1" ? 11 : 0);
        req_.method(boost::beast::http::string_to_verb(method));
        req_.target(url);
        for (auto const& [header, value] : headers) {
            req_.set(header, value);
        }
        req_.set("X-Arrived-Client-sec", std::to_string(t->tv_sec));
        req_.set("X-Arrived-CVlient-nano", std::to_string(t->tv_nsec));
    
        
        if (!body.empty()) {
            req_.body() = body;
            req_.prepare_payload();
        }
    
        resolver_.async_resolve(host, port, beast::bind_front_handler(session::on_resolve, shared_from_this()));
    
        beast::bind_front_handler()
        // Look up the domain name
        resolver_.async_resolve(
                host,
                port,
                [capture0 = shared_from_this()](auto &ec, auto & results) { 
                    capture0->on_resolve(ec, results);
                });
    }
    
    
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec) return fail(ec, "resolve");
        
        stream_.expires_after(std::chrono::seconds(30));
        stream_.async_connect(results, beast::bind_front_handler(&session::on_connect, shared_from_this()));
    }
    
    void on_resolve(boost::system::error_code ec, const tcp::resolver::results_type& results) {
        if(ec)
            return fail(ec, "resolve");
        
        // Make the connection on the IP address we get from a lookup
        auto self = shared_from_this();
        boost::asio::async_connect(
                socket_,
                results.begin(),
                results.end(),
                [self](boost::system::error_code ec1, const boost::asio::ip::tcp::resolver::iterator&) {
                    self->on_connect(ec1, sizeof(ec1));
                });

    }
    
    
    void on_connect(boost::beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
        if (ec) return fail(ec, "connect");
        
        stream_.expires_after(std::chrono::seconds(30));
        http::async_write(stream_, req_, boost::beast::bind_front_handler(&session::on_write, shared_from_this()));
    }
    
    
    void on_connect(boost::system::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if(ec)
            return fail(ec, "connect");
        
        // Send the HTTP request to the remote host
        http::async_write(socket_, req_,
                          [capture0 = shared_from_this()](auto  &ec1) {
                              capture0->on_write(ec1, sizeof(ec1));
                          });
    }
    
    void on_write(const boost::system::error_code &ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if(ec)
            return fail(ec, "write");
        
        // Receive the HTTP response
        http::async_read(socket_, buffer_, res_,
                         [capture0 = shared_from_this()](auto &ec1) { 
            capture0->on_read(ec1, sizeof(ec1));
        });
    }
    
    void on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if(ec)
            return fail(ec, "read");
    
        struct timespec t {0,0};
        clock_gettime(CLOCK_MONOTONIC, &t);
    
        res_.set("X-Res-sec", std::to_string(t.tv_sec));
        res_.set("X-Res-Client-nano", std::to_string(t.tv_nsec));
        
        // Gracefully close the socket
        socket_.shutdown(tcp::socket::shutdown_both, ec);
        
        // not_connected happens sometimes so don't bother reporting it.
        if(ec && ec != boost::system::errc::not_connected)
            return fail(ec, "shutdown");
        
        auto response = buildResponseStringBuffer(res_);
    
        auto webSocketComponent = &WebSocketComponent::getInstance();
        //send response to websocket client
        webSocketComponent->sendTextMessageToClient(vin_, response);
        
        // If we get here then the connection is closed gracefully
    }
    
};


#endif //VGATEWAY_BOOSTBEASTASYNCCLIENT_H
