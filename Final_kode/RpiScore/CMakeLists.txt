cmake_minimum_required(VERSION 3.10)
project(RpiScore LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_definitions(-DFMT_HEADER_ONLY)

if (WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()

set(LLHTTP_SRC
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/llhttp/api.c
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/llhttp/http.c
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/llhttp/llhttp.c
)

add_executable(client
    src/main.cpp
    src/RpiScore.cpp
    src/ReceiveData.cpp
    src/DisplayServer.cpp
    ${LLHTTP_SRC}
)

target_include_directories(client PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/restinio
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/asio
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/fmt
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/json_dto
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/llhttp
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/expected-lite
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/rapidjson
)

target_link_libraries(client Threads::Threads)