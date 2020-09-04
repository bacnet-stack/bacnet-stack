# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13.1)

list(APPEND BACNET_INCLUDE ../modules/lib/bacnet/src)

# Extract module path and names
string(REGEX REPLACE 
    "/zephyr/tests[a-zA-Z_/-]*$" ""
    BACNET_BASE
    ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(BACNET_FILE_BASENAME
    ${BACNET_BASE}
    NAME)
string(REGEX REPLACE 
    "^[a-zA-Z_/-]*/zephyr/tests/" ""
    BACNET_FILE_SUBPATH
    ${CMAKE_CURRENT_SOURCE_DIR})


# Add this module to the Zephyr build line
list(APPEND ZEPHYR_EXTRA_MODULES
    ${BACNET_BASE}
  )


# Update include path for this module
list(APPEND BACNET_INCLUDE ${BACNET_BASE}/src)


include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(${BACNET_FILE_BASENAME})

target_include_directories(app PRIVATE ${BACNET_INCLUDE})
target_sources(app PRIVATE
  ${BACNET_BASE}/test/${BACNET_FILE_SUBPATH}/src/main.c)
