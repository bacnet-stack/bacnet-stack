get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

add_executable(${PROJECT_NAME}
  main.c
  device.c
)

# Note: This CMake file is not tested yet and is draft.

# CFLAGS += -DMAX_TSM_TRANSACTIONS=1
# CFLAGS += -DMAX_BINARY_INPUTS=4
# CFLAGS += -DMAX_BINARY_OUTPUTS=8

# Get dependencies

include(ExternalProject)

set(EXTERNAL_INSTALL_DIR ${CMAKE_BINARY_DIR}/external)

# TODO: Not yet tested
ExternalProject_Add(libmcp23s17
  GIT_REPOSITORY https://github.com/piface/libmcp23s17.git
  PREFIX ${EXTERNAL_INSTALL_DIR}/libmcp23s17
  CONFIGURE_COMMAND ""
  BUILD_COMMAND make
  INSTALL_COMMAND ""
  BUILD_IN_SOURCE 1
)

# TODO: Not yet tested
ExternalProject_Add(libpifacedigital
  GIT_REPOSITORY https://github.com/piface/libpifacedigital.git
  PREFIX ${EXTERNAL_INSTALL_DIR}/libpifacedigital
  CONFIGURE_COMMAND ""
  BUILD_COMMAND make
  INSTALL_COMMAND ""
  BUILD_IN_SOURCE 1
)

add_dependencies(${PROJECT_NAME}
  libmcp23s17
  libpifacedigital
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  bacnet-stack

  libmcp23s17
  libpifacedigital
)
