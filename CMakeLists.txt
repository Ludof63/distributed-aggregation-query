cmake_minimum_required(VERSION 3.16)
project(cbdp)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
include("${CMAKE_SOURCE_DIR}/cmake/azure-storage-lite.cmake")

find_package(CURL REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion -Werror -fsanitize=address")

add_executable(coordinator
        src/coordinator.cpp
        src/Api.cpp
        src/CurlEasyPtr.cpp
        src/Server.cpp
        src/Timer.cpp
        src/AzureBlobClient.cpp)
target_link_libraries(coordinator PUBLIC CURL::libcurl azure-storage-lite)

add_executable(worker
        src/runWorker.cpp
        src/Api.cpp
        src/CurlEasyPtr.cpp
        src/Client.cpp
        src/Worker.cpp
        src/AzureBlobClient.cpp)
target_link_libraries(worker PUBLIC CURL::libcurl azure-storage-lite)

