get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

# TODO: In makefile this was previously defined for lib.
# -DBACAPP_COLOR_RGB_CONVERSION_ENABLED=1

add_executable(${PROJECT_NAME}
  main.c
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  bacnet-stack
)
