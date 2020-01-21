cmake_minimum_required(VERSION 3.6)
project(sos)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES
        json/json.hpp
        src/charbuf.cc
        src/config.cc
        src/connection.cc
        src/connection_server.cc
        src/dataproxy.cc
        src/handler.cc
        src/ioloop.cc
        src/serialport.cc
        src/socket.cc
        src/sockserver.cc
        src/sockclient.cc
        src/snippets.cc
        src/main.cc
        )


add_executable(sos ${SOURCE_FILES})
target_include_directories(sos PUBLIC
        src/
        json
        )

target_link_libraries(sos
        pthread)

