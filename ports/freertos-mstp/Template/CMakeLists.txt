cmake_minimum_required(VERSION 3.15)

project(freertos-mstp)

set(FREERTOS_KERNEL_PATH "../FreeRTOS-Kernel")
set(BACNET_STACK_PATH "../../../src")
set(BACNET_CMAKE_PATH "../BACnet")
set(BACNET_PORT_PATH "..")

add_definitions(
    -DPRINT_ENABLED=0
    -DBACDL_MSTP=1
    -DBACAPP_ALL
    -DMAX_APDU=480
    -DBIG_ENDIAN=0
    -DMAX_TSM_TRANSACTIONS=1
    -DMAX_CHARACTER_STRING_BYTES=64
    -DMAX_OCTET_STRING_BYTES=64
    -DBACNET_STACK_DEPRECATED_DISABLE
)

# Add the freertos_config for FreeRTOS-Kernel
add_library(freertos_config INTERFACE)

target_include_directories(freertos_config
    INTERFACE
    ${FREERTOS_KERNEL_PATH}/examples/sample_configuration
)

# Select the heap port.  values between 1-4 will pick a heap.
set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)

# Select the native compile PORT
set(FREERTOS_PORT "TEMPLATE" CACHE STRING "" FORCE)

# Adding the components subdirectories
add_subdirectory(${FREERTOS_KERNEL_PATH} FreeRTOS-Kernel)
add_subdirectory(${BACNET_CMAKE_PATH} BACnet_Stack)

add_executable(${PROJECT_NAME}
    main.c
    bacnet.c
    device.c
    ${BACNET_PORT_PATH}/dlmstp.c
    dlmstp-init.c
    mstimer-init.c
    rs485.c
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${BACNET_PORT_PATH}
    ${BACNET_CMAKE_PATH}
    ${BACNET_STACK_PATH}
    ${FREERTOS_KERNEL_PATH}/portable/template
    ${FREERTOS_KERNEL_PATH}/include
)

target_link_libraries(${PROJECT_NAME} BACnet_Stack freertos_kernel freertos_config)
