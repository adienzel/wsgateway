
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

static boost::asio::awaitable<std::string> asyncHttpClient(std::shared_ptr<std::string> msg, 
                                                    std::string& host,
                                                    std::string& port,
                                                    std::string& clientID,
                                                    std::shared_ptr<std::string> t) {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace asio = boost::asio;
    using tcp = asio::ip::tcp;
    try {
        auto [method, url, version, headers, body] = createRequestFromBuffer(msg);
        auto req = std::make_shared<http::request<http::string_body>>();
        req->method(boost::beast::http::string_to_verb(method));
        if (version == "HTTP/1.1") {
            req->version(11);
        } else if (version == "HTTP/1.0") {
            req->version(10);
        }
        req->target(url);
    
        for (auto const &[header, value]: headers) {
            if (header.find_first_of("\r\n") != std::string::npos || value.find_first_of("\r\n") != std::string::npos) {
                OATPP_LOGe(__func__, "Invalid header: {}: {}", header, value);
                continue; // Or sanitize/remove
            }
            req->set(header, value);
        }
    
        req->set("X-Arrived-time", *t);
        req->set("X-Client-ID", clientID);
    
        if (!body.empty()) {
            req->body() = body;
            //auto content_length = req.find("Content-Length");
            req->prepare_payload();
        }
        req->need_eof();
        tcp::resolver resolver(co_await asio::this_coro::executor);
        beast::tcp_stream stream(co_await asio::this_coro::executor);
    
        auto const results = co_await resolver.async_resolve(host, port, asio::use_awaitable);
        co_await stream.async_connect(results, asio::use_awaitable);
    
        //http::request<http::string_body> req{http::verb::post, target, 11};
        req->set(http::field::host, host);
    
        co_await http::async_write(stream, *req, asio::use_awaitable);
    
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        co_await http::async_read(stream, buffer, res, asio::use_awaitable);
    
        auto result = buildResponseStringBuffer(res);
    
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    
        co_return result;
    } catch (const std::exception& e) {
        OATPP_LOGe(__func__ , "Exception: {}", e.what());
        co_return "error";
    
    }
}

#endif //VGATEWAY_BOOSTBEASTCLIENT_H
