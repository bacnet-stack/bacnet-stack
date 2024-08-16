get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

add_executable(${PROJECT_NAME}
  main.c
)

target_compile_options(${PROJECT_NAME} PRIVATE
  # Unreachable code because we have endless loop.
  $<$<C_COMPILER_ID:MSVC>:/wd4702>
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  bacnet-stack
)
