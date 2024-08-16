get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

# Note: This CMake file is not tested yet and is draft.

# CFLAGS += -DBAC_ROUTING

add_executable(${PROJECT_NAME}
  main.c
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  bacnet-stack
)
