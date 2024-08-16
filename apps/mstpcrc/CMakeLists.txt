get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

add_executable(${PROJECT_NAME}
  main.c
)

target_compile_options(${PROJECT_NAME} PRIVATE
  # NOTE: Might be that this example currently doesn't work on Windows because
  # of the following warning:
  # 'gettimeofday' undefined; assuming extern returning int
  $<$<C_COMPILER_ID:MSVC>:/wd4013>
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  bacnet-stack
)
