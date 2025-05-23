
#ifndef VGEATWAY_WSCONTROLLER_H
#define VGEATWAY_WSCONTROLLER_H

#include <cstdlib>
#include <cerrno>

#include "oatpp-websocket/Handshaker.hpp"

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/json/ObjectMapper.hpp"

#include "oatpp/web/server/api/Endpoint.hpp"

#include "oatpp/network/ConnectionHandler.hpp"

#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"
#include "oatpp/async/Coroutine.hpp"
//#include "oatpp/data/mapping/type/Object.hpp"
//#include "oatpp/data/mapping/type/String.hpp"

#include "./websocket/WebSocketComponents.h"
#include "./websocket/WebSocketListener.h"


#include OATPP_CODEGEN_BEGIN(ApiController) //<-- codegen begin

class WsController : public oatpp::web::server::api::ApiController {
private:
    using __ControllerType = WsController;
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, websocketConnectionHandler, "websocket");
protected:
    explicit WsController(OATPP_COMPONENT(std::shared_ptr<oatpp::json::ObjectMapper>, objectMapper))
            : oatpp::web::server::api::ApiController(objectMapper) {
        //OATPP_LOGd(__func__, " {}", __LINE__)
        
    }
public:
    
    static std::shared_ptr<WsController> createShared(OATPP_COMPONENT(std::shared_ptr<oatpp::json::ObjectMapper>,
                                                                      objectMapper)){
    
        //OATPP_LOGd(__func__, " {}", __LINE__)
        return std::shared_ptr<WsController>(new WsController(objectMapper));
    }
    
    ENDPOINT_ASYNC("GET", "/", Root) {
    
    ENDPOINT_ASYNC_INIT(Root)
        
        const char* pageTemplate =
                "<html lang='en'>"
                "<head>"
                "<meta charset=utf-8/>"
                "</head>"
                "<body>"
                "<p>Hello oatpp WebSocket benchmark!</p>"
                "<p>"
                "You may connect WebSocket client on '&lt;host&gt;:&lt;port&gt;/ws'"
                "</p>"
                "</body>"
                "</html>";
        
        Action act() override {
            return _return(controller->createResponse(Status::CODE_200, pageTemplate));
        }
        
    };
    
    ENDPOINT_ASYNC("GET", "ws", WS1) {
    
    ENDPOINT_ASYNC_INIT(WS1)
        
        Action act() override {
            OATPP_LOGi("MyApp", "WS1")
            auto params = std::make_shared<oatpp::network::ConnectionHandler::ParameterMap>();
            (*params)["ClientId"] = "VIN1234567890";
            (*params)["Id"] = "dummy";
    
            auto response = oatpp::websocket::Handshaker::serversideHandshake(request->getHeaders(), controller->websocketConnectionHandler);
            response->setConnectionUpgradeParameters(params);
            
            return _return(response);
        }
        
    };


    ENDPOINT_ASYNC("GET", "ws/by-id/{ClientId}", WS) {
    
    ENDPOINT_ASYNC_INIT(WS)
        
        Action act() override {
            OATPP_LOGi("MyApp", "ws/by-id/{ClientId}")
            auto clienId = request->getPathVariable("ClientId");
            auto id = request->getHeader("X-Client-ID");
            auto params = std::make_shared<oatpp::network::ConnectionHandler::ParameterMap>();
            (*params)["ClientId"] = clienId;
            (*params)["Id"] = id;
            auto response = oatpp::websocket::Handshaker::serversideHandshake(request->getHeaders(), controller->websocketConnectionHandler);
            response->setConnectionUpgradeParameters(params);
            return _return(response);
        }
        
    };

#include OATPP_CODEGEN_END(ApiController) //<-- codegen end
    
};


#endif //VGEATWAY_WSCONTROLLER_H
