#ifndef VGATEWAY_HTTPPARSER_H
#define VGATEWAY_HTTPPARSER_H


#include <oatpp/base/Log.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>


#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <iostream>
#include <sstream>
#include <string>

/**
 * 
 * @param res the result message returned for http request
 * @return string of the response
 */
static std::string buildResponseStringBuffer(const boost::beast::http::response<boost::beast::http::string_body>& res) {
    std::ostringstream oss;
//    OATPP_LOGi(__func__, "result is : {}", res.)
    std::string version;
    switch (res.version()) {
        case 10: version = "HTTP/1.0"; break;
        case 11: version = "HTTP/1.1"; break;
        default: version = "HTTP/" + std::to_string(res.version() / 10) + "." + std::to_string(res.version() % 10);
    }
    oss << version << " " << res.result_int() << " " << res.reason() << "\r\n";
    for (const auto& field : res) {
        oss << field.name_string() << ": " << field.value() << "\r\n";
    }
    
    oss << "\r\n";
    oss << res.body();
    return oss.str();
}

/**
 * 
 * @param buffer the received message from the ws client in thex
 * @return tuple of :
 *      method - GET/POST and etc
 *      url the path of the request (without the domain
 *      version the http version
 *      headers map of all headers in the request
 *      body string  
 */
static auto createRequestFromBuffer(std::shared_ptr<std::string> buffer) {
    std::istringstream stream(*buffer);
    std::string method;
    std::string url;
    std::string version;
    std::string body;
    
    stream >> method >> url >> version;
        
    std::unordered_map<std::string, std::string> headers;
    std::string headerLine;
    auto mycount = 0;
    //read the emppty /r/n of the first line
    std::getline(stream, headerLine);
    while (std::getline(stream, headerLine) && !headerLine.empty()) {
        mycount++;
        auto delimiterPos = headerLine.find(": ");
        if (delimiterPos == std::string::npos && mycount > 1) {
//            OATPP_LOGi(__func__, "line {} headerline {} at deimiter {}", __LINE__, headerLine, delimiterPos)
            break;
        }
        auto headerName = headerLine.substr(0, delimiterPos);
        auto headerValue = headerLine.substr(delimiterPos + 2, headerLine.size() - (delimiterPos + 2) - 1);
        auto [first, second] = headers.try_emplace(headerName, headerValue);
        if (!second) {
            OATPP_LOGe(__func__, "error insert header {} aleady added one", headerName) 
        }
    }
//    OATPP_LOGi(__func__, "line {}", __LINE__)
    
    //oatpp::web::protocol::http::
    std::streamsize contentLength = 0;
    if (headers.find("Content-Length") != headers.end()) {
        contentLength = std::stoi(headers["Content-Length"]);
        body.insert(0, contentLength, '0');
        stream.read(&body[0], contentLength);
    }
   
    return std::make_tuple(method, url, version, headers, body);
}


#endif //VGATEWAY_HTTPPARSER_H
