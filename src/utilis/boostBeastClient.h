
#ifndef VGATEWAY_BOOSTBEASTCLIENT_H
#define VGATEWAY_BOOSTBEASTCLIENT_H
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <oatpp/base/Log.hpp>

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


static void sendHttpReqSync(std::string const& msg, std::string const& host, std::string const& port,  std::string const& vin, const struct timespec* t) {
        auto [method, url, version, headers, body] = createRequestFromBuffer(msg);
        http::request<http::string_body> req;
        req.method(boost::beast::http::string_to_verb(method));
        req.version(version == "1.1" ? 11 : 0);
        req.target(url);
    
        for (auto const& [header, value] : headers) {
            req.set(header, value);
        }
        
        //time of arrival from WS client in nanosecods
        req.set("X-Arrived-Client-time", std::to_string(t->tv_sec * 1000000000 + t->tv_nsec));
    
        if (!body.empty()) {
            req.body() = body;
            //auto content_length = req.find("Content-Length");
            req.prepare_payload();
        }
    
    
        boost::asio::io_context ioc;
        tcp::resolver resolver(ioc);
        boost::beast::tcp_stream stream(ioc);
        
        auto const results = resolver.resolve(host, port);
        stream.connect(results);
        
        http::write(stream, req);
    
        boost::beast::flat_buffer  buffer;
        http::response<http::string_body> res;
        
        http::read(stream, buffer, res);
        
        struct timespec tr {0, 0};
        clock_gettime(CLOCK_MONOTONIC, &tr);
    
        res.set("X-Return-time", std::to_string(tr.tv_sec * 1000000000 + tr.tv_nsec));
        
        boost::beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    
        auto response = buildResponseStringBuffer(res);
    
        auto webSocketComponent = &WebSocketComponent::getInstance();
        //send response to websocket client
        webSocketComponent->sendTextMessageToClient(vin, response);
}


#endif //VGATEWAY_BOOSTBEASTCLIENT_H
