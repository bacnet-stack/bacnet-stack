#
# Copyright (c) 2022 Legrand North America, LLC.
#
# SPDX-License-Identifier: MIT
#

zephyr_compile_definitions(
  _POSIX_C_SOURCE=200809    # Expose `strdup()` for bacfile, ao, bo, mso
  CONFIG_NEWLIB_LIBC        # Choose library providing `strdup()` impl.
  )

zephyr_sources(device.c)

if(CONFIG_BACNET_USE_DYNAMIC_DESCRIPTION)
  zephyr_sources(
    acc.c
    access_credential.c
    access_door.c
    access_point.c
    access_rights.c
    access_user.c
    access_zone.c
    ai.c
    ao.c
    av.c
    bacfile.c
    bi.c
    bo.c
    bv.c
    channel.c
    command.c
    credential_data_input.c
    csv.c
    iv.c
    lc.c
    lo.c
    lsp.c
    ms-input.c
    mso.c
    msv.c
    nc.c
    osv.c
    piv.c
    schedule.c
    trendlog.c
    )

  zephyr_sources_ifdef(CONFIG_BACDL_BIP netport.c)

  zephyr_linker_sources(DATA_SECTIONS objects.ld)

endif()
