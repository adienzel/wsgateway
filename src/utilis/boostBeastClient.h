
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
#include <chrono>
#include <string>

#include "utilis/HttpParser.h"
#include "../websocket/WebSocketComponents.h"


using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>


static std::string sendHttpReqSync(std::string const& msg, std::string const& host, std::string const& port,  std::string const& vin, std::string const& t) {
    OATPP_LOGi(__func__, "line {}", __LINE__)
    auto [method, url, version, headers, body] = createRequestFromBuffer(msg);
    OATPP_LOGi(__func__, "line {} method {} url {} version {} body {}", __LINE__, method, url, version, body)
    http::request<http::string_body> req;
    OATPP_LOGi(__func__, "line {}", __LINE__)
    req.method(boost::beast::http::string_to_verb(method));
    OATPP_LOGi(__func__, "line {}", __LINE__)
    req.version(version == "HTTP/1.1" ? 11 : 0);
    OATPP_LOGi(__func__, "line {}", __LINE__)
    req.target(url);
    OATPP_LOGi(__func__, "line {}", __LINE__)
    
        for (auto const& [header, value] : headers) {
            if (header.find_first_of("\r\n") != std::string::npos || value.find_first_of("\r\n") != std::string::npos) {
                OATPP_LOGe(__func__, "Invalid header: {}: {}", header, value);
                continue; // Or sanitize/remove
            }
    
            req.set(header, value);
        }
    
    OATPP_LOGi(__func__, "line {}", __LINE__)
        //time of arrival from WS client in nanosecods
        req.set("X-Arrived-Client-time", t);
    
        if (!body.empty()) {
            req.body() = body;
            //auto content_length = req.find("Content-Length");
            req.prepare_payload();
        }
        req.need_eof();
    OATPP_LOGi(__func__, "line {}", __LINE__)
    
    
        boost::asio::io_context ioc;
        tcp::resolver resolver(ioc);
    OATPP_LOGi(__func__, "line {}", __LINE__)
        boost::beast::tcp_stream stream(ioc);
    
    OATPP_LOGi(__func__, "line {} host {}, port {}", __LINE__, host, port)
        auto const results = resolver.resolve(host, port);
    OATPP_LOGi(__func__, "line {}", __LINE__)
        stream.connect(results);
    OATPP_LOGi(__func__, "line {}", __LINE__)
        
        http::write(stream, req);
    
    OATPP_LOGi(__func__, "line {}", __LINE__)
        boost::beast::flat_buffer  buffer;
        http::response<http::string_body> res;
        
        http::read(stream, buffer, res);
    OATPP_LOGi(__func__, "line {}", __LINE__)
        
        struct timespec tr {0, 0};
        clock_gettime(CLOCK_MONOTONIC, &tr);
    
        res.set("X-Return-time", std::to_string(tr.tv_sec * 1000000000 + tr.tv_nsec));
    OATPP_LOGi(__func__, "line {}", __LINE__)
        
        boost::beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    OATPP_LOGi(__func__, "line {}", __LINE__)
    
        auto response = buildResponseStringBuffer(res);
    
    OATPP_LOGi(__func__, "line {}, response = {} ", __LINE__, response)
    return response;
//        auto webSocketComponent = &WebSocketComponent::getInstance();
//        //send response to websocket client
//        webSocketComponent->sendTextMessageToClient(vin, response);
//    OATPP_LOGi(__func__, "line {}", __LINE__)
}


#endif //VGATEWAY_BOOSTBEASTCLIENT_H
