find_library(LIBCONFIG_LIBRARIES NAMES config)
if (NOT LIBCONFIG_LIBRARIES)
  message(WARNING "BACNET: Will not build apps/router as libconfig not found")
  return()
endif()

get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

add_executable(${PROJECT_NAME}
  ipmodule.c
  ipmodule.h
  main.c
  msgqueue.c
  msgqueue.h
  mstpmodule.c
  mstpmodule.h
  network_layer.c
  network_layer.h
  portthread.c
  portthread.h
)

target_compile_options(${PROJECT_NAME} PRIVATE
  # These make this example not totally C90 compatible but it is ok.

  -Wno-declaration-after-statement
  -Wno-overlength-strings
  -Wno-variadic-macros
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  bacnet-stack

  # Needs libconfig
  -lconfig
)
