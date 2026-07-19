# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

get_filename_component(basename ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(test_${basename}
    VERSION 1.0.0
    LANGUAGES C)

string(REGEX REPLACE
    "/test/apps/router/msgqueue$"
    ""
    ROOT_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Threads REQUIRED)

include_directories(
    ${ROOT_DIR}/apps/router
    ${ROOT_DIR}/src
    )

add_executable(${PROJECT_NAME}
    ${ROOT_DIR}/apps/router/msgqueue.c
    ./src/main.c
    )

target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
