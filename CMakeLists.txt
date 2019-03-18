cmake_minimum_required(VERSION 3.12)
project(cpp_http_range_fileserver)

set(CMAKE_CXX_STANDARD 14)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang")
    set(CMAKE_CXX_FLAGS "-O3 -std=c++14 -stdlib=libc++ -Wall -Wextra -lboost_system -lboost_thread-mt -lboost_filesystem -lmagic")
    add_executable(cpp_http_range_fileserver main.cpp connection.cpp connection.hpp header.hpp mime_types.cpp mime_types.hpp reply.hpp reply.cpp request.hpp request_handler.cpp request_handler.hpp request_parser.cpp request_parser.hpp server.cpp server.hpp httputils.h range.h)
    include_directories("/usr/local/include")
    add_executable(mime_checker mime_checker.cpp)
endif()