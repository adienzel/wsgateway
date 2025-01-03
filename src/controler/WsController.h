
#ifndef VGEATWAY_WSCONTROLLER_H
#define VGEATWAY_WSCONTROLLER_H

#include <cstdlib>
#include <cerrno>

#include "oatpp-websocket/Handshaker.hpp"

#include "oatpp/web/server/api/ApiController.hpp"

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
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, websocketConnectionHandler, "websocket");
public:
    typedef WsController __ControllerType;
    
    explicit WsController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
            : oatpp::web::server::api::ApiController(objectMapper) {}
    
    ENDPOINT_INFO(SendTextMessage) {
        info->summary = "Send Text RPC";
        info->addResponse<String>(Status::CODE_200, "text/plain");
        info->pathParams.add<String>("clienId").description = "VIN"; // add param1 info
        
    }
    ENDPOINT_ASYNC("POST", "/send/text/{clienId}", SendTextMessage) {
    
    ENDPOINT_ASYNC_INIT(SendTextMessage)
        
        Action act() override {
            auto clientId = request->getPathVariable("clienId");
            OATPP_ASSERT_HTTP(clientId, Status::CODE_400, "clientId should not be null")
            char* end;
            auto id = strtoll(clientId->c_str(), &end, 10);
            if (clientId->c_str() == end) {
                throw oatpp::web::protocol::http::HttpError(Status::CODE_400, "client id is not numeric", {});
            }
            auto body = request->readBodyToString();
    
            WebSocketComponent::getInstance().sendTextMessageToClient(clientId, body);
            return _return(controller->createResponse(Status::CODE_200, "clientId = '" + clientId + "'"));
        }
        
    };
    
    ENDPOINT_ASYNC("GET", "/ws/{ClientId}", WS) {
    
    ENDPOINT_ASYNC_INIT(WS)
        
        Action act() override {
            OATPP_LOGi("MyApp", "WS")
            auto clienId = request->getPathVariable("ClientId");
            auto id = request->getHeader("ClientId");
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
