# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_HTTP_CLIENT http)

add_subdirectory_ifdef(CONFIG_BSC_WEBSOCKET_CLIENT websocket)
