#include "oatpp/Types.hpp" 
#include "oatpp/macro/codegen.hpp" 

#include OATPP_CODEGEN_BEGIN(DTO) 

class JSON_message : public oatpp::DTO {
    DTO_INIT(JSON_message, DTO);

    DTO_FIELD(String, version);
    DTO_FIELD(String, MessageType);
    DTO_FIELD(String, command);
    DTO_FIELD(UInt64, MessageNumber); 
    DTO_FIELD(UInt64, StartTimeSeconds);
    DTO_FIELD(UInt64, StartTimeNano);
    DTO_FIELD(UInt64, RcvTimeSeconds);
    DTO_FIELD(UInt64, RcvTimeNano);
    DTO_FIELD(UInt64, WsUpSeconds);
    DTO_FIELD(UInt64, WsUpNano);
    DTO_FIELD(UInt64, WsDnSeconds);
    DTO_FIELD(UInt64, WsDnNano);
    DTO_FIELD(String, VIN);
    DTO_FIELD(String, msg);

};


#include OATPP_CODEGEN_END(DTO)