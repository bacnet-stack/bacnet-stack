# Project settings for STM32F429ZI hardware on Nucleof429zi board
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
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(BACnet-429zi)

enable_language(C ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Specific ARM microcontroller compiler and linker settings
add_compile_options(-mcpu=cortex-m4)
add_compile_options(-mthumb -mthumb-interwork)
add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)
add_link_options(-mcpu=cortex-m4)
add_link_options(-mthumb -mthumb-interwork)
add_link_options(-Wl,-gc-sections,--print-memory-usage)

# Enable hardware FPU
add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
add_link_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
add_compile_definitions(ARM_MATH_CM4;ARM_MATH_MATRIX_CHECK;ARM_MATH_ROUNDING)

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


set(BACNET_CORE "../../src/bacnet")
set(BACNET_BASIC "../../src/bacnet/basic")
set(LIBRARY_STM32 "./STM32F4xx_StdPeriph_Driver/src")

set(BACNET_PROJECT_SOURCE
    ${LIBRARY_STM32}/stm32f4xx_adc.c
    ${LIBRARY_STM32}/stm32f4xx_can.c
    ${LIBRARY_STM32}/stm32f4xx_crc.c
    ${LIBRARY_STM32}/stm32f4xx_dac.c
    ${LIBRARY_STM32}/stm32f4xx_dbgmcu.c
    ${LIBRARY_STM32}/stm32f4xx_dcmi.c
    ${LIBRARY_STM32}/stm32f4xx_dma.c
    ${LIBRARY_STM32}/stm32f4xx_exti.c
    ${LIBRARY_STM32}/stm32f4xx_flash.c
    ${LIBRARY_STM32}/stm32f4xx_fsmc.c
    ${LIBRARY_STM32}/stm32f4xx_gpio.c
    ${LIBRARY_STM32}/stm32f4xx_i2c.c
    ${LIBRARY_STM32}/stm32f4xx_iwdg.c
    ${LIBRARY_STM32}/stm32f4xx_misc.c
    ${LIBRARY_STM32}/stm32f4xx_pwr.c
    ${LIBRARY_STM32}/stm32f4xx_rcc.c
    ${LIBRARY_STM32}/stm32f4xx_rng.c
    ${LIBRARY_STM32}/stm32f4xx_rtc.c
    ${LIBRARY_STM32}/stm32f4xx_sdio.c
    ${LIBRARY_STM32}/stm32f4xx_spi.c
    ${LIBRARY_STM32}/stm32f4xx_syscfg.c
    ${LIBRARY_STM32}/stm32f4xx_tim.c
    ${LIBRARY_STM32}/stm32f4xx_usart.c
    ${LIBRARY_STM32}/stm32f4xx_wwdg.c
    ${LIBRARY_STM32}/syscalls.c

    ${CMAKE_SOURCE_DIR}/stm32f4xx_conf.h

    ${CMAKE_SOURCE_DIR}/main.c
    ${CMAKE_SOURCE_DIR}/stm32f4xx_it.c
    ${CMAKE_SOURCE_DIR}/stm32f4xx_it.h
	${CMAKE_SOURCE_DIR}/system_stm32f4xx.c
	${CMAKE_SOURCE_DIR}/system_stm32f4xx.h

    ${CMAKE_SOURCE_DIR}/bacnet.c
    ${CMAKE_SOURCE_DIR}/dlmstp.c
    ${CMAKE_SOURCE_DIR}/led.c
    ${CMAKE_SOURCE_DIR}/mstimer-init.c
	${CMAKE_SOURCE_DIR}/rs485.c

    ${CMAKE_SOURCE_DIR}/device.c
    ${CMAKE_SOURCE_DIR}/netport.c

	${BACNET_BASIC}/service/h_dcc.c
	${BACNET_BASIC}/service/h_apdu.c
	${BACNET_BASIC}/npdu/h_npdu.c
	${BACNET_BASIC}/service/h_rd.c
	${BACNET_BASIC}/service/h_rp.c
	${BACNET_BASIC}/service/h_rpm.c
	${BACNET_BASIC}/service/h_whohas.c
	${BACNET_BASIC}/service/h_whois.c
	${BACNET_BASIC}/service/h_wp.c
	${BACNET_BASIC}/service/h_noserv.c
	${BACNET_BASIC}/service/s_iam.c
	${BACNET_BASIC}/service/s_ihave.c
	${BACNET_BASIC}/tsm/tsm.c
	${BACNET_BASIC}/sys/debug.c
	${BACNET_BASIC}/sys/ringbuf.c
	${BACNET_BASIC}/sys/fifo.c
	${BACNET_BASIC}/sys/mstimer.c

	${BACNET_CORE}/abort.c
	${BACNET_CORE}/bacaddr.c
	${BACNET_CORE}/bacapp.c
	${BACNET_CORE}/bacdcode.c
	${BACNET_CORE}/bacdest.c
	${BACNET_CORE}/bacdevobjpropref.c
	${BACNET_CORE}/bacerror.c
	${BACNET_CORE}/bacint.c
	${BACNET_CORE}/bacreal.c
	${BACNET_CORE}/bacstr.c
	${BACNET_CORE}/datalink/cobs.c
	${BACNET_CORE}/datalink/crc.c
	${BACNET_CORE}/datetime.c
	${BACNET_CORE}/dcc.c
	${BACNET_CORE}/iam.c
	${BACNET_CORE}/ihave.c
	${BACNET_CORE}/hostnport.c
	${BACNET_CORE}/lighting.c
	${BACNET_CORE}/memcopy.c
	${BACNET_CORE}/npdu.c
	${BACNET_CORE}/proplist.c
	${BACNET_CORE}/rd.c
	${BACNET_CORE}/reject.c
	${BACNET_CORE}/rp.c
	${BACNET_CORE}/rpm.c
	${BACNET_CORE}/timestamp.c
	${BACNET_CORE}/weeklyschedule.c
	${BACNET_CORE}/dailyschedule.c
	${BACNET_CORE}/bactimevalue.c
	${BACNET_CORE}/whohas.c
	${BACNET_CORE}/whois.c
	${BACNET_CORE}/wp.c

    CMSIS/gcc_ride7/startup_stm32f4xx.s
    CMSIS/stm32f4xx.h
)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/stm32f4xx.ld)

set(EXECUTABLE ${PROJECT_NAME}.out)

add_executable(${EXECUTABLE} ${BACNET_PROJECT_SOURCE})

target_compile_definitions(${EXECUTABLE} PRIVATE
        -DNDEBUG
        -DUSE_STDPERIPH_DRIVER
        -DSTM32F4XX
        -DBACDL_MSTP
        -DMAX_APDU=480
        -DBIG_ENDIAN=0
        -DMAX_TSM_TRANSACTIONS=1
)

# inhibit pedantic warnings
target_compile_options(${EXECUTABLE} PRIVATE
    -Wall
    -Wno-comment
    -Wno-missing-braces
    -Wno-unused-variable
)

target_include_directories(${EXECUTABLE} PRIVATE
    ${CMAKE_SOURCE_DIR}
    CMSIS
    CMSIS/gcc_ride7
    STM32F4xx_StdPeriph_Driver/inc
    ../../src
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
        COMMAND arm-none-eabi-size ${EXECUTABLE})

# Create hex file
add_custom_command(TARGET ${EXECUTABLE}
        POST_BUILD
        COMMAND arm-none-eabi-objcopy -O ihex ${EXECUTABLE} ${PROJECT_NAME}.hex
        COMMAND arm-none-eabi-objcopy -O binary ${EXECUTABLE} ${PROJECT_NAME}.bin)
