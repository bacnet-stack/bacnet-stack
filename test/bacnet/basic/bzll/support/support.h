/**
 * @file
 * @brief BACnet ZigBee Link Layer Handler
 * @author Luiz Santana <luiz.santana@dsr-corporation.com>
 * @date April 2026
 * @copyright SPDX-License-Identifier: MIT
 */

#ifndef TEST_BZLL_SUPPORT_H
#define TEST_BZLL_SUPPORT_H

#include "bacnet/basic/bzll/bzllvmac.h"

/**
 * @brief Device info struct
 */
struct device_info_t {
    uint32_t Device_ID;
    /* MAC Address shall be a ZigBee EUI64 and BACnet endpoint */
    struct bzll_vmac_data VMAC_Data;
    BACNET_ADDRESS BACnet_Address;
};

typedef void (*test_ptr_t)(void);
typedef struct {
    const char *func_name;
    test_ptr_t func;
} test_suite_t;

#ifdef SUPPORT_PRINT_ENABLE
#define SUPPORT_PRINT(...) printf(__VA_ARGS__)
#else
#define SUPPORT_PRINT(...) \
    do {                   \
    } while (0)
#endif

#define TEST_ENTRY(f) \
    {                 \
        #f, f         \
    }
#define RUN_TESTS(array, setup, cleanup)                              \
    do {                                                              \
        SUPPORT_PRINT("Testing %s\n", __FILE_NAME__);                 \
        for (size_t i = 0; i < ARRAY_SIZE(array); i++) {              \
            setup();                                                  \
            SUPPORT_PRINT(                                            \
                "\t\tTest %ld - %s", i + 1, test_suite[i].func_name); \
            test_suite[i].func();                                     \
            SUPPORT_PRINT("\r\t OK -\n");                             \
            cleanup();                                                \
        }                                                             \
        SUPPORT_PRINT("%s - OK\n", __FILE_NAME__);                    \
    } while (0)

void support_set_device_info(struct device_info_t *device_info);

#endif
