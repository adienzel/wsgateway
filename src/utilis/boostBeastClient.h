
#ifndef VGATEWAY_BOOSTBEASTCLIENT_H
#define VGATEWAY_BOOSTBEASTCLIENT_H

#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/awaitable.hpp>
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
    http::request<http::string_body> req;
    req.method(boost::beast::http::string_to_verb(method));
    req.version(version == "HTTP/1.1" ? 11 : 0);
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
        req.set("X-Arrived-time", t);
    
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
        
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    
        res.set("X-Return-time", std::to_string(ns));
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

static boost::asio::awaitable<std::string> asyncHttpClient(std::string const& msg, 
                                                    std::string host,
                                                    std::string port,
                                                    std::string clientID,
                                                    std::string t) {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace asio = boost::asio;
    using tcp = asio::ip::tcp;
    try {
        OATPP_LOGi(__func__, "line {}", __LINE__)
        std::string method;
        std::string url;
        std::string version;
        std::string body;
        std::unordered_map<std::string, std::string> headers;
        
        std::tie(method, url, version, headers, body) = createRequestFromBuffer(msg);
        
        //[method, url, version, headers, body] = createRequestFromBuffer(msg);
        OATPP_LOGi(__func__, "line {}", __LINE__)
        http::request<http::string_body> req;
        OATPP_LOGi(__func__, "line {}", __LINE__)
        req.method(boost::beast::http::string_to_verb(method));
        OATPP_LOGi(__func__, "line {}", __LINE__)
        if (version == "HTTP/1.1") {
            req.version(11);
        } else if (version == "HTTP/1.0") {
            req.version(10);
        }
        req.target(url);
        OATPP_LOGi(__func__, "line {}", __LINE__)
    
        for (auto const &[header, value]: headers) {
            if (header.find_first_of("\r\n") != std::string::npos || value.find_first_of("\r\n") != std::string::npos) {
                OATPP_LOGe(__func__, "Invalid header: {}: {}", header, value);
                continue; // Or sanitize/remove
            }
        
            req.set(header, value);
        }
    
        OATPP_LOGi(__func__, "line {}", __LINE__)
        //time of arrival from WS client in nanosecods
        req.set("X-Arrived-time", t);
        req.set("X-Client-ID", clientID);
    
        if (!body.empty()) {
            req.body() = body;
            //auto content_length = req.find("Content-Length");
            req.prepare_payload();
        }
        req.need_eof();
        OATPP_LOGi(__func__, "line {}", __LINE__)
        std::cout.flush();
        tcp::resolver resolver(co_await asio::this_coro::executor);
        beast::tcp_stream stream(co_await asio::this_coro::executor);
    
        auto const results = co_await resolver.async_resolve(host, port, asio::use_awaitable);
        co_await stream.async_connect(results, asio::use_awaitable);
    
        //http::request<http::string_body> req{http::verb::post, target, 11};
        req.set(http::field::host, host);
    
        co_await http::async_write(stream, req, asio::use_awaitable);
    
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
    
        co_await http::async_read(stream, buffer, res, asio::use_awaitable);
    
        std::string result = res.body();
    
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    
        co_return result;
    } catch (const std::exception& e) {
        std::cerr << __func__ << " exception: " << e.what() << std::endl;
        OATPP_LOGe(__func__ , "Exception: {}", e.what());
        co_return "error";
    
    }
}


#endif //VGATEWAY_BOOSTBEASTCLIENT_H
