get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

# Note: This CMake file is not tested yet and is draft.

# fuzz-afl: CC=afl-gcc
# fuzz-afl: FUZZ_FLAGS=-DFUZZING=1
# fuzz-afl: LFLAGS += $(FUZZ_FLAGS)
# fuzz-afl: CFLAGS += $(FUZZ_FLAGS)
# fuzz-afl: export AFL_USE_ASAN=1

add_executable(${PROJECT_NAME}
  main.c
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  bacnet-stack
)
