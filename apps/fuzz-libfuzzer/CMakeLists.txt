get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

# Note: This CMake file is not tested yet and is draft.

# fuzz-libfuzzer: CC=clang
# fuzz-libfuzzer: FUZZ_FLAGS=-DFUZZING=1 -fsanitize=fuzzer,address -g3 -Og -fno-optimize-sibling-calls -fno-omit-frame-pointer
# fuzz-libfuzzer: LFLAGS += $(FUZZ_FLAGS)
# fuzz-libfuzzer: CFLAGS += $(FUZZ_FLAGS)

add_executable(${PROJECT_NAME}
  main.c
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  bacnet-stack
)
