cmake_minimum_required(VERSION 3.6)
project(sos)

set(CMAKE_CXX_STANDARD 14)

set(SERVER_SOURCE_FILES
        json/json.hpp
        src/charbuf.cc
        src/config.cc
        src/connection.cc
        src/connection_server.cc
        src/dataproxy.cc
        src/handler.cc
        src/ioloop.cc
        src/serialport.cc
        src/server_main.cc
        src/socket.cc
        src/sockserver.cc
        )


add_executable(sos_server ${SERVER_SOURCE_FILES})
target_include_directories(sos_server PUBLIC
        src/
        json
        )

target_link_libraries(sos_server
        pthread)

set(CLIENT_SOURCE_FILES
        json/json.hpp
        src/charbuf.cc
        src/client_config.cc
        src/client_main.cc
        src/connection.cc
        src/handler.cc
        src/ioloop.cc
        src/socket.cc
        src/sockclient.cc
        )

add_executable(sos_client ${CLIENT_SOURCE_FILES})
target_include_directories(sos_client PUBLIC
        src/
        json
        )

target_link_libraries(sos_client
        pthread)
