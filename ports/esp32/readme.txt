Bacnet Server for Espressif ESP32
    Steve Karg Bacnet stack using PlatformIO open source ecosystem for IoT development on VSCode or Atom
    F. Chaxel 2017

TODO list :

(Install VSCode or Atom and add the PlatformIO extension)

Edit platformio.ini to adjust board, Com Port, ...

Goto lib/stack and copy the requested files from Steve code :

   all .h from include directory (not all required by it's simple)

   these .c files from src or demo/handlers
    abort.c
    address.c
    apdu.c
    bacaddr.c
    bacapp.c
    bacdcode.c
    bacerror.c
    bacint.c
    bacreal.c
    bacstr.c
    bip.c
    bvlc.c
    cov.c
    datetime.c
    bacdevobjpropref.c
    dcc.c
    debug.c
    h_cov.c
    h_ucov.c
    h_npdu.c
    h_rp.c
    h_rpm.c
    h_whois.c
    h_wp.c
    iam.c
    hostnport.c
    lighting.c
    memcopy.c
    noserv.c
    npdu.c
    proplist.c
    reject.c
    rp.c
    rpm.c
    s_iam.c
    tsm.c
    whois.c
    wp.c

Modify
    in config.h
        MAX_TSM_TRANSACTIONS 255, set the value to 10 for instances
    in main.c
        wifi_config to fit your wifi network
        BACNET_LED 5, set another IO number depending of your board

A lot of Warning will be issued at compile time due to the redefinition of BIT macros.
Could be removes by placing a #ifndef #BIT0 .. #endif arround the BIT macro in bits.h,
and moving to the top of include list
    #include "bacnet/datalink/datalink.h" in tsm.c, s_iam and in device.c
    #include "bacport.h" in bip.c and in bip.h (redondant include in bip.c)
    #include "bacnet/datalink/bvlc.h" in bvlc.c
