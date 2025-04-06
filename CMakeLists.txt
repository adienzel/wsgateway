cmake_minimum_required(VERSION 3.20)
set(project_name vGateway)

project(${project_name})

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

include_directories(src
        ${SCYLLA_INCLUDE_DIRS})


add_library(${project_name}-lib
        src/AppComponent.h
        src/websocket/WebSocketListener.h
        src/websocket/WebSocketListener.cpp
        src/utilis/ipUtils.h
        src/utilis/messageDTO.h
        src/utilis/env.h
        src/utilis/split.h
        src/websocket/WebSocketComponents.h
        src/controler/WsController.h
        src/ScyllaDBManager.h 
        src/client/RestClient.h)

## link libs

find_package(oatpp              1.4.0 REQUIRED)
find_package(oatpp-websocket    1.4.0 REQUIRED)
find_package(ScylladbCppDriver REQUIRED)
find_package(OpenSSL REQUIRED)

#message(STATUS "SCYLLA_LIBRERIES = ${SCYLLA_LIBRERIES}")
target_link_libraries(${project_name}-lib
        PUBLIC oatpp::oatpp
        PUBLIC oatpp::oatpp-test
        PUBLIC oatpp::oatpp-websocket
        OpenSSL::SSL 
        OpenSSL::Crypto
        ${SCYLLA_LIBRERIES}
        )


add_executable(${project_name}-exe
        src/App.cpp
        src/AppComponent.h
        src/utilis/ipUtils.h
        src/utilis/env.h
        src/ScyllaDBManager.h
        src/controler/WsController.h 
        src/websocket/WebSocketListener.cpp 
        src/websocket/WebSocketListener.h 
        src/client/RestClient.h
        src/utilis/split.h)
target_link_libraries(${project_name}-exe ${project_name}-lib)
add_dependencies(${project_name}-exe ${project_name}-lib)

#add_executable(${project_name}-test
#        test/tests.cpp
#        test/WSTest.cpp
#        test/WSTest.hpp
#        )
#target_link_libraries(${project_name}-test ${project_name}-lib)
#add_dependencies(${project_name}-test ${project_name}-lib)

set_target_properties(${project_name}-lib ${project_name}-exe PROPERTIES
        CXX_STANDARD 17
        CXX_EXTENSIONS OFF
        CXX_STANDARD_REQUIRED ON
        LINKER_LANGUAGE CXX
        )
#set_target_properties(${project_name}-lib ${project_name}-exe ${project_name}-test PROPERTIES
#        CXX_STANDARD 17
#        CXX_EXTENSIONS OFF
#        CXX_STANDARD_REQUIRED ON
#        LINKER_LANGUAGE CXX
#        )

enable_testing()
add_test(tests ${project_name}-test)
