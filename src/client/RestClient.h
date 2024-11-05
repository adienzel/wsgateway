#ifndef VGEATWAY_RESTCLIENT_H
#define VGEATWAY_RESTCLIENT_H

#include <oatpp/web/client/ApiClient.hpp>
#include <oatpp/macro/codegen.hpp>
#include <oatpp/Types.hpp>


class ClientApi : public oatpp::web::client::ApiClient {
#include OATPP_CODEGEN_BEGIN(ApiClient)
    API_CLIENT_INIT(ClientApi)
    
    API_CALL_ASYNC("POST", "/gm/{version}/{vin}/aircondition", doPostAnythingAsync, 
                   PATH(String, version), 
                   PATH(String, vin), 
                   BODY_STRING(String, body))
    
    API_CALL_ASYNC("POST", "/gm/{vin}/{version}/body-access", doAsyncPostBodyAccess, 
                   PATH(String, vin),
                   PATH(String, version),
                   BODY_STRING(String, body))
#include OATPP_CODEGEN_END(ApiClient)

};

#endif //VGEATWAY_RESTCLIENT_H
