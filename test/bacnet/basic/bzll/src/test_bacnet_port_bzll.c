/**
 * @file
 * @brief Test file for the BACnet/Zigbee port init and task callbacks
 * @date November 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "bacnet/basic/object/netport.h"
#include "bacnet/basic/server/bacnet_port_bzll.h"
/* support file */
#include "support.h"
/* me! */
#include "test_bacnet_port_bzll.h"

static unsigned Init_Count;
static unsigned Task_Count;
static uint16_t Task_Elapsed;
static unsigned Port_Type_Set_Count;

bool Network_Port_Object_Instance_Number_Set(
    unsigned index, uint32_t object_instance)
{
    (void)index;
    (void)object_instance;
    return true;
}
bool Network_Port_Name_Set(uint32_t object_instance, const char *new_name)
{
    (void)object_instance;
    (void)new_name;
    return true;
}
bool Network_Port_Type_Set(uint32_t object_instance, uint8_t value)
{
    (void)object_instance;
    (void)value;
    Port_Type_Set_Count++;
    return true;
}
bool Network_Port_MAC_Address_Set(
    uint32_t object_instance, const uint8_t *mac_src, uint8_t mac_len)
{
    (void)object_instance;
    (void)mac_src;
    (void)mac_len;
    return true;
}
bool Network_Port_Reliability_Set(
    uint32_t object_instance, BACNET_RELIABILITY value)
{
    (void)object_instance;
    (void)value;
    return true;
}
bool Network_Port_Out_Of_Service_Set(uint32_t instance, bool oos_flag)
{
    (void)instance;
    (void)oos_flag;
    return true;
}
bool Network_Port_Network_Number_Set(uint32_t object_instance, uint16_t value)
{
    (void)object_instance;
    (void)value;
    return true;
}
bool Network_Port_Quality_Set(
    uint32_t object_instance, BACNET_PORT_QUALITY value)
{
    (void)object_instance;
    (void)value;
    return true;
}
bool Network_Port_APDU_Length_Set(uint32_t object_instance, uint16_t value)
{
    (void)object_instance;
    (void)value;
    return true;
}
bool Network_Port_Link_Speed_Set(uint32_t object_instance, float value)
{
    (void)object_instance;
    (void)value;
    return true;
}
bool Network_Port_Changes_Pending_Set(uint32_t instance, bool flag)
{
    (void)instance;
    (void)flag;
    return true;
}

static void test_init_callback(void)
{
    Init_Count++;
}

static void test_task_callback(uint16_t elapsed_seconds)
{
    Task_Count++;
    Task_Elapsed = elapsed_seconds;
}

void test_bacnet_port_bzll(void)
{
    struct device_info_t device = { 0 };

    device.Device_ID = 1234;
    support_set_device_info(&device);

    /* no callbacks: init and task must still work */
    bzll_init_callback_set(NULL);
    bzll_task_callback_set(NULL);
    assert(bacnet_port_bzll_init());
    bacnet_port_bzll_task(1);
    assert(Init_Count == 0);
    assert(Task_Count == 0);
    assert(Port_Type_Set_Count == 1);

    /* callbacks are invoked */
    bzll_init_callback_set(test_init_callback);
    bzll_task_callback_set(test_task_callback);
    bzll_cleanup();
    assert(bacnet_port_bzll_init());
    assert(Init_Count == 1);
    assert(Port_Type_Set_Count == 2);
    bacnet_port_bzll_task(5);
    assert(Task_Count == 1);
    assert(Task_Elapsed == 5);
    bacnet_port_bzll_task(7);
    assert(Task_Count == 2);
    assert(Task_Elapsed == 7);

    /* callbacks can be removed */
    bzll_init_callback_set(NULL);
    bzll_task_callback_set(NULL);
    bzll_cleanup();
    assert(bacnet_port_bzll_init());
    bacnet_port_bzll_task(9);
    assert(Init_Count == 1);
    assert(Task_Count == 2);
}
