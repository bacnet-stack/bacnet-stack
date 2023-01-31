# Copyright (c) 2023 Legrand North America, LLC.
# SPDX-License-Identifier: MIT

if(CONFIG_BACNET_MONGOOSE)
  set(ZEPHYR_MONGOOSE_MODULE_DIR $ENV{ZEPHYR_BASE}/../modules/lib/mongoose/examples/zephyr/websocket-server/src)

  zephyr_include_directories(
    ${ZEPHYR_MONGOOSE_MODULE_DIR}
  )

  zephyr_library()
  zephyr_library_sources(
    ${ZEPHYR_MONGOOSE_MODULE_DIR}/mongoose.c
    utils.c
  )

zephyr_library_compile_definitions(
  MG_ENABLE_FILE=0
  MG_ENABLE_CUSTOM_RANDOM
  MG_ENABLE_MBEDTLS=1
  )
endif()
