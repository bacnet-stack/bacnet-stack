# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13.1)

get_filename_component(MY_PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(${MY_PROJECT_NAME})

target_sources(app PRIVATE src/main.c)

string(REGEX REPLACE
  "/zephyr/samples/[a-zA-Z_/-]*$" "/bin/certs"
  CERTS_DIR
  ${CMAKE_CURRENT_SOURCE_DIR})

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

foreach(inc_file
	ca_cert.pem
	server_cert.pem
	server_key.pem
    )
  generate_inc_file_for_target(
    app
    ${CERTS_DIR}/${inc_file}
    ${gen_dir}/${inc_file}.inc
    )
endforeach()
