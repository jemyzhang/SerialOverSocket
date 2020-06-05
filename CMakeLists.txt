cmake_minimum_required(VERSION 3.6)
project(sos)

set(CMAKE_CXX_STANDARD 14)

find_package(Git)
list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    INCLUDE(GetGitRevisionDescription)
    EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD OUTPUT_VARIABLE SHORT_SHA OUTPUT_STRIP_TRAILING_WHITESPACE)

    SET(REVISION ${SHORT_SHA} CACHE STRING "git short sha" FORCE)

    # only use the plugin to tie the configure state to the sha to force rebuilds
    # of files that depend on version.h
    include(GetGitRevisionDescription)
    get_git_head_revision(REFSPEC COMMITHASH)
else()
    message(WARNING "Git not found, cannot set version info")

    SET(REVISION "unknown")
endif()

# generate version.h
include_directories(${CMAKE_BINARY_DIR})
configure_file("${CMAKE_SOURCE_DIR}/src/version.h.in" "${CMAKE_BINARY_DIR}/version.h" @ONLY)


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

