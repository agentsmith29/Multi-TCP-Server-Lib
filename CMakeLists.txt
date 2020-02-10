# Copyright(c) 2019 spdlog authors
# Distributed under the MIT License (http://opensource.org/licenses/MIT)
project(multiTCPServer LANGUAGES CXX)
cmake_minimum_required(VERSION 3.2)


include_directories(${PROJECT_SOURCE_DIR}/spdlog/include)

find_library(SPDLOG_LIB spdlog ${PROJECT_SOURCE_DIR}/spdlog/build_x86)

if(NOT SPDLOG_LIB)
    message(FATAL_ERROR "spdlog library not found")
endif()

find_library(PTHREAD_LIB pthread)

if(NOT PTHREAD_LIB)
    message(FATAL_ERROR "pthread library not found")
endif()

#---------------------------------------------------------------------------------------
# Compiler config
#---------------------------------------------------------------------------------------
if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

set(CMAKE_CXX_EXTENSIONS  OFF)

if(CMAKE_SYSTEM_NAME MATCHES "CYGWIN")
    set(CMAKE_CXX_EXTENSIONS ON)
endif()


#---------------------------------------------------------------------------------------
# Static/Shared library (shared not supported in windows yet)
#---------------------------------------------------------------------------------------
add_executable(multiTCPServer
        main.cpp
        src/Server.cpp src/ServerExceptions.cpp include/ServerExceptions.h include/Utils.h src/ServerWorker.cpp include/ServerWorker.h)

target_link_libraries(multiTCPServer
        ${SPDLOG_LIB}
        ${PTHREAD_LIB})




