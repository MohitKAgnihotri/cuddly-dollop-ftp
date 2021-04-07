cmake_minimum_required(VERSION 3.16)
project(FTP)

set(CMAKE_CXX_STANDARD 17)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY Server)
add_executable(FTP_Server server.cpp server.h Thread.cpp Thread.h utility.h utility.cpp)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY Client)
add_executable(FTP_Client client.cpp Thread.cpp Thread.h utility.h client.h utility.cpp)