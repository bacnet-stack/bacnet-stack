get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

add_executable(${PROJECT_NAME}
  h_pt_a.c
  h_pt.c
  main.c
  s_ptransfer.c
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  bacnet-stack
)
