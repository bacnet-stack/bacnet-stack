get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

add_executable(${PROJECT_NAME}
  main.c

  ../../src/bacnet/basic/client/bac-discover.c
  ../../src/bacnet/basic/client/bac-rw.c
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  bacnet-stack
)
