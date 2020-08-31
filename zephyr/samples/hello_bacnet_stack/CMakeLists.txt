# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13.1)

get_filename_component(MY_PROJECT_BASENAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Find the directory of the Zephyr module
string(REGEX REPLACE 
    "\/zephyr\/samples\/[a-zA-Z_\-]*${MY_PROJECT_BASENAME}$" ""
    MY_ZEPHYR_MODULE_SOURCE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND ZEPHYR_EXTRA_MODULES
    ${MY_ZEPHYR_MODULE_SOURCE_DIR}
  )

# Add an absolute directory path to the CMake variable
# SYSCALL_INCLUDE_DIRS. This ensures that the syscall machinery will
# be able to find the module's syscalls.
#TODO: list(APPEND SYSCALL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(${MY_PROJECT_BASENAME})

target_sources(app PRIVATE src/main.c)
