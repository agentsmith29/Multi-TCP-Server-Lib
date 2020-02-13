# Copyright(c) 2020 Christoph Schmidt
# Distributed under the MIT License (http://opensource.org/licenses/MIT)
cmake_minimum_required(VERSION 3.9)

PROJECT(multiTCPServer VERSION "1.0.0.0" LANGUAGES CXX)

configure_file(include/global_config.h.in include/global_config.h @ONLY)

include_directories(${PROJECT_SOURCE_DIR}/spdlog/include)
include_directories(${PROJECT_SOURCE_DIR}/boost_1_72_0)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

#-----------------------------------------------------------------------------------------------------------------------
# Set the libraries
#-----------------------------------------------------------------------------------------------------------------------
find_library(SPDLOG_LIB spdlog ${PROJECT_SOURCE_DIR}/spdlog/build_x86 REQUIRED)
find_library(PTHREAD_LIB pthread REQUIRED)

if(NOT SPDLOG_LIB)
    message(FATAL_ERROR "spdlog library not found")
endif()

if(NOT PTHREAD_LIB)
    message(FATAL_ERROR "pthread library not found")
endif()


#-----------------------------------------------------------------------------------------------------------------------
# Compiler config
#-----------------------------------------------------------------------------------------------------------------------
if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

set(CMAKE_CXX_EXTENSIONS  OFF)

if(CMAKE_SYSTEM_NAME MATCHES "CYGWIN")
    set(CMAKE_CXX_EXTENSIONS ON)
endif()


#-----------------------------------------------------------------------------------------------------------------------
# Adding the sources
#-----------------------------------------------------------------------------------------------------------------------
add_executable(multiTCPServer
        main.cpp
        src/Server.cpp
        include/ServerExceptions.hpp
        include/Utils.h
        src/ServerWorker.cpp
        include/ServerWorker.h
        src/ServerMessage.cpp
        include/ServerMessage.h
        src/Utils.cpp
        include/Utils.h)

#-----------------------------------------------------------------------------------------------------------------------
# Link libraries
#-----------------------------------------------------------------------------------------------------------------------
target_link_libraries(multiTCPServer
        ${SPDLOG_LIB}
        ${PTHREAD_LIB})




