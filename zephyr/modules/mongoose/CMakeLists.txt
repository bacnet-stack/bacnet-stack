# Copyright (c) 2023 Legrand North America, LLC.
# SPDX-License-Identifier: MIT

if(CONFIG_BACNET_MONGOOSE)
  set(ZEPHYR_MONGOOSE_MODULE_DIR $ENV{ZEPHYR_BASE}/../modules/lib/mongoose)

  zephyr_include_directories(
    ${ZEPHYR_MONGOOSE_MODULE_DIR}
    .
  )

  zephyr_library()
  zephyr_library_sources(
    ${ZEPHYR_MONGOOSE_MODULE_DIR}/mongoose.c
    utils.c
  )

zephyr_library_compile_definitions(
  MG_ARCH=MG_ARCH_CUSTOM
  MG_ENABLE_FILE=0
  MG_ENABLE_CUSTOM_RANDOM
  MG_ENABLE_CUSTOM_MILLIS
  MG_ENABLE_MBEDTLS=1
  )

# mongoose at least v7.9 is not includes signal.h
# but uses signal() in mg_mgr_init() for native_posix (aka unix) board
if(BOARD STREQUAL "native_posix")
  zephyr_library_compile_definitions(
      SIGPIPE=13
      SIG_IGN=1
  )
endif()

endif()
