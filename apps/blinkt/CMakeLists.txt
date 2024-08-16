get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

add_executable(${PROJECT_NAME}
  main.c
  blinkt.c
  device.c
)

# Note this CMake file is not tested yet and is draft.

# TODO: Library should be builded with MAX_TSM_TRANSACTIONS=1
# LD_PIGPIO = -Wl,-pthread,-lpigpiod_if2

find_package(libpigpio REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
  bacnet-stack

  -pthread

  # note: Link to pigpio daemon so that our app can run without root start pigio
  #       daemon with the command 'sudo pigpiod'
  -lpigpiod_if2
)
