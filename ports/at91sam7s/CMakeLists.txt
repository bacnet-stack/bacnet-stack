# This is a CMake example for AT91SAM7S-EK development board
# using the ARM GCC compiler
#
# MCU AT91SAM7S64
# CPU ARM7
# Clock 180MHz
# RAM 16K
# Flash 64K
#
# To build this project you need to install:
# - ARM GCC compiler
# - CMake
#
# To build this project you need to run:
# - cmake -S . -B build
# - cmake --build build
#
# To flash this project you need to run:
# - st-flash write build/bacnet-mstp.hex 0x8000000
#
# To debug this project you need to run:
# - arm-none-eabi-gdb -q build/bacnet-mstp.elf
# - (gdb) target extended-remote localhost:3333
# - (gdb) monitor reset halt
# - (gdb) load
# - (gdb) monitor reset halt
# - (gdb) monitor reset init
# - (gdb) monitor reset run
# - (gdb) monitor reset exit
# - (gdb) quit
#
# You can also use VSCode with Cortex-Debug extension
#
# This example was tested with:
# - ARM GCC 10.3.1
# - CMake 3.22.1
# - BACnet Stack V1.3.2
#
cmake_minimum_required(VERSION 3.20)

# Cross compilers and tools
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)
set(CMAKE_SIZE arm-none-eabi-size)
set(CMAKE_NM arm-none-eabi-nm)
set(CMAKE_CSTACK "${CMAKE_SOURCE_DIR}/../../tools/check-stack-usage/checkStackUsage.py")
set(CMAKE_MEMAP "${CMAKE_SOURCE_DIR}/../../tools/memap/memap.py")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(EXECUTABLE ${PROJECT_NAME}.elf)

project(bacnet-mstp)

enable_language(C ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Specific ARM microcontroller compiler and linker settings
add_compile_options(-mcpu=arm7tdmi)
add_compile_options(-mno-thumb-interwork)
add_link_options(-mcpu=arm7tdmi)
add_link_options(-mno-thumb-interwork)
# Code size reduction using garbage collection sections
add_compile_options(-ffunction-sections -fdata-sections)
add_compile_options(-fno-common -fmessage-length=0)
add_compile_options(-fstack-usage -fdump-rtl-dfinish)
add_link_options(-Wl,-gc-sections,--print-memory-usage)
# Build types
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_definitions(DEBUG)
    add_compile_options(-Og -g3)
endif ()

# eliminate the deprecated function warnings
option(BACNET_STACK_DEPRECATED_DISABLE "Disable deprecation compile warnings" ON)
if(BACNET_STACK_DEPRECATED_DISABLE)
  add_definitions(-DBACNET_STACK_DEPRECATED_DISABLE)
endif()

set(LIBRARY_BACNET_INC "../../src")
set(LIBRARY_BACNET_CORE "../../src/bacnet")
set(LIBRARY_BACNET_BASIC "../../src/bacnet/basic")

set(BACNET_PROJECT_SOURCE
    ${CMAKE_SOURCE_DIR}/crt.s
    ${CMAKE_SOURCE_DIR}/at91sam7s256.h
    ${CMAKE_SOURCE_DIR}/board.h
    # CPU interfaces
    ${CMAKE_SOURCE_DIR}/main.c
    ${CMAKE_SOURCE_DIR}/init.c
    ${CMAKE_SOURCE_DIR}/isr.c
    ${CMAKE_SOURCE_DIR}/blinker.c
    ${CMAKE_SOURCE_DIR}/rs485.c
    ${CMAKE_SOURCE_DIR}/timer.c
    # MSTP datalink
    ${CMAKE_SOURCE_DIR}/dlmstp.c
    # BACnet objects
    ${CMAKE_SOURCE_DIR}/device.c
    ${CMAKE_SOURCE_DIR}/netport.c
    ${CMAKE_SOURCE_DIR}/ai.c
    ${CMAKE_SOURCE_DIR}/av.c
    ${CMAKE_SOURCE_DIR}/bi.c
    ${CMAKE_SOURCE_DIR}/bv.c
    # BACnet basic services
    ${LIBRARY_BACNET_BASIC}/service/h_dcc.c
    ${LIBRARY_BACNET_BASIC}/service/h_apdu.c
    ${LIBRARY_BACNET_BASIC}/npdu/h_npdu.c
    ${LIBRARY_BACNET_BASIC}/service/h_rd.c
    ${LIBRARY_BACNET_BASIC}/service/h_rp.c
    ${LIBRARY_BACNET_BASIC}/service/h_rpm.c
    ${LIBRARY_BACNET_BASIC}/service/h_whohas.c
    ${LIBRARY_BACNET_BASIC}/service/h_whois.c
    ${LIBRARY_BACNET_BASIC}/service/h_wp.c
    ${LIBRARY_BACNET_BASIC}/service/h_noserv.c
    ${LIBRARY_BACNET_BASIC}/service/s_iam.c
    ${LIBRARY_BACNET_BASIC}/service/s_ihave.c
    ${LIBRARY_BACNET_BASIC}/tsm/tsm.c
    # BACnet basic system modules
    ${LIBRARY_BACNET_BASIC}/sys/debug.c
    ${LIBRARY_BACNET_BASIC}/sys/ringbuf.c
    ${LIBRARY_BACNET_BASIC}/sys/fifo.c
    ${LIBRARY_BACNET_BASIC}/sys/mstimer.c
    # BACnet core modules
    ${LIBRARY_BACNET_CORE}/abort.c
    ${LIBRARY_BACNET_CORE}/bacaction.c
    ${LIBRARY_BACNET_CORE}/bacaddr.c
    ${LIBRARY_BACNET_CORE}/bacapp.c
    ${LIBRARY_BACNET_CORE}/bacdcode.c
    ${LIBRARY_BACNET_CORE}/bacdest.c
    ${LIBRARY_BACNET_CORE}/bacdevobjpropref.c
    ${LIBRARY_BACNET_CORE}/bacerror.c
    ${LIBRARY_BACNET_CORE}/bacint.c
    ${LIBRARY_BACNET_CORE}/bacreal.c
    ${LIBRARY_BACNET_CORE}/bacstr.c
    ${LIBRARY_BACNET_CORE}/datalink/cobs.c
    ${LIBRARY_BACNET_CORE}/datalink/crc.c
#   ${LIBRARY_BACNET_CORE}/datalink/dlmstp.c
#   ${LIBRARY_BACNET_CORE}/datalink/mstp.c
    ${LIBRARY_BACNET_CORE}/datalink/mstptext.c
    ${LIBRARY_BACNET_CORE}/datetime.c
    ${LIBRARY_BACNET_CORE}/dcc.c
    ${LIBRARY_BACNET_CORE}/indtext.c
    ${LIBRARY_BACNET_CORE}/iam.c
    ${LIBRARY_BACNET_CORE}/ihave.c
    ${LIBRARY_BACNET_CORE}/hostnport.c
    ${LIBRARY_BACNET_CORE}/lighting.c
    ${LIBRARY_BACNET_CORE}/memcopy.c
    ${LIBRARY_BACNET_CORE}/npdu.c
    ${LIBRARY_BACNET_CORE}/proplist.c
    ${LIBRARY_BACNET_CORE}/rd.c
    ${LIBRARY_BACNET_CORE}/reject.c
    ${LIBRARY_BACNET_CORE}/rp.c
    ${LIBRARY_BACNET_CORE}/rpm.c
    ${LIBRARY_BACNET_CORE}/timestamp.c
    ${LIBRARY_BACNET_CORE}/weeklyschedule.c
    ${LIBRARY_BACNET_CORE}/dailyschedule.c
    ${LIBRARY_BACNET_CORE}/calendar_entry.c
    ${LIBRARY_BACNET_CORE}/special_event.c
    ${LIBRARY_BACNET_CORE}/bactimevalue.c
    ${LIBRARY_BACNET_CORE}/whohas.c
    ${LIBRARY_BACNET_CORE}/whois.c
    ${LIBRARY_BACNET_CORE}/wp.c
)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/at91sam7s256.ld)

set(EXECUTABLE ${PROJECT_NAME}.elf)

add_executable(${EXECUTABLE} ${BACNET_PROJECT_SOURCE})

target_compile_definitions(${EXECUTABLE} PRIVATE
    -DNDEBUG
    -DUSE_STDPERIPH_DRIVER
    -DSTM32F4XX
    -DBACDL_MSTP
    -DMAX_APDU=480
    -DBIG_ENDIAN=0
    -DPRINT_ENABLED=0
    -DMAX_TSM_TRANSACTIONS=0
    -DCRC_USE_TABLE
    -DBACAPP_MINIMAL
)

target_compile_options(${EXECUTABLE} PRIVATE
    -Wall -Wextra -pedantic
    -Wfloat-equal -Wconversion -Wredundant-decls 
    -Wswitch-default
    # don't warn about conversion, sign, compares, long long and attributes
    # since they are common in embedded
    -Wno-sign-conversion 
    -Wno-conversion
    -Wno-sign-compare 
    -Wno-long-long
    -Wno-attributes
    # don't warn about implicit fallthrough since it is common in network protocols
    -Wno-implicit-fallthrough
    # the older Atmel SDK does not meet coding guidelines
    -Wno-comment
    -Wno-missing-braces
    -Wno-unused-variable
    -Wno-char-subscripts
)

target_include_directories(${EXECUTABLE} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${LIBRARY_CMSIS_INC}
    ${LIBRARY_CMSIS_GCC_INC}
    ${LIBRARY_STM32_INC}
    ${LIBRARY_BACNET_INC}
)

target_link_options(${EXECUTABLE} PRIVATE
    -T${LINKER_SCRIPT}
    -specs=nano.specs
    -lm
    -lnosys
    -Wl,-Map=${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections
)

# Print executable size
add_custom_command(TARGET ${EXECUTABLE}
    POST_BUILD
    COMMAND arm-none-eabi-size ${EXECUTABLE}
)

# Create hex file
add_custom_command(TARGET ${EXECUTABLE}
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O ihex ${EXECUTABLE} ${PROJECT_NAME}.hex
    COMMAND arm-none-eabi-objcopy -O binary ${EXECUTABLE} ${PROJECT_NAME}.bin
)

# sort the RAM usage by size and place into a file
add_custom_target(symbols
    DEPENDS ${EXECUTABLE}
    COMMENT "Print memory symbols by size"
    COMMAND ${CMAKE_NM} -t d -S --size-sort ${EXECUTABLE} 1> ${PROJECT_NAME}.nm
    COMMAND echo "RAM usage by size analysis in ${PROJECT_NAME}.nm"
    COMMAND echo "=ADDRESS= ==RAM=== = ==VARIABLE-NAME=="
    COMMAND tail ${PROJECT_NAME}.nm
)

# calculate the worst case CSTACK memory usage by size and place into a file
add_custom_target(cstack 
    DEPENDS ${EXECUTABLE}
    COMMENT "Print CSTACK memory depth by size"
    COMMAND ${CMAKE_CSTACK} ${EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR} 1> ${PROJECT_NAME}.su
    COMMAND echo "C-Stack maxium depth analysis in ${PROJECT_NAME}.su"
    COMMAND echo "==DEPTH== : == Functions called =="
    COMMAND tail ${PROJECT_NAME}.su
)

# Print file and library sizes
add_custom_target(memmap 
    DEPENDS ${PROJECT_NAME}.map
    COMMENT "Print file and library memory usage by size"
    COMMAND ${CMAKE_MEMAP} -t GCC_ARM ${PROJECT_NAME}.map
)
