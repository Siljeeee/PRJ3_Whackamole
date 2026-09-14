cmake_minimum_required(VERSION 3.10)

project(RPiGameLogic)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DFMT_HEADER_ONLY)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)

set(LLHTTP_SRC
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/llhttp/api.c
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/llhttp/http.c
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/llhttp/llhttp.c
)

add_executable(game
    src/main.cpp
    src/GameLogic.cpp
    src/LED.cpp
    src/ForceSensor.cpp
    src/SendData.cpp
    src/Terminal.cpp
    ${LLHTTP_SRC}
)

target_include_directories(game PRIVATE
    inc
    ${GPIOD_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/restinio
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/asio
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/fmt
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/json_dto
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/llhttp
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/expected-lite
    ${CMAKE_SOURCE_DIR}/restinio_bundle/external/rapidjson
)

target_link_libraries(game PRIVATE
    ${GPIOD_LIBRARIES}
    Threads::Threads
)

target_compile_options(game PRIVATE
    ${GPIOD_CFLAGS_OTHER}
)
