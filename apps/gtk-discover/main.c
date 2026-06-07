/**
 * @file
 * @brief GTK-based BACnet Device and Object Property Discovery
 * @author Steve Karg <skarg@users.sourceforge.net
 * @date August 2025
 * @copyright SPDX-License-Identifier: MIT
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/bacstr.h"
#include "bacnet/bactext.h"
#include "bacnet/dcc.h"
#include "bacnet/iam.h"
#include "bacnet/npdu.h"
#include "bacnet/version.h"
#include "bacnet/whois.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/client/bac-rw.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/property.h"
/* Used ImageMagick: convert BACnet-Icon.svg bacnet-icon.xpm */
#include "bacnet-icon.xpm"

/* Global variables */
static GtkWidget *main_window;
static GtkWidget *device_tree_view;
static GtkWidget *object_tree_view;
static GtkWidget *property_tree_view;
static GtkWidget *object_progress_label;
static GtkWidget *property_progress_label;
static GtkListStore *device_store;
static GtkListStore *object_store;
static GtkListStore *property_store;

/* BACnet global variables */
static uint8_t Rx_Buf[MAX_MPDU];
/* task timer for various BACnet timeouts */
static struct mstimer BACnet_Task_Timer;
/* task timer for TSM timeouts */
static struct mstimer BACnet_TSM_Timer;

/* GTK interfaces */
static bool bacnet_initialized = false;
static guint bacnet_timeout_id = 0;

/* Tree store columns */
enum {
    DEVICE_COL_ID,
    DEVICE_COL_VENDOR_ID,
    DEVICE_COL_MAX_APDU,
    DEVICE_COL_SEGMENTATION,
    DEVICE_COL_ADDRESS,
    DEVICE_NUM_COLS
};

enum {
    OBJECT_COL_TYPE,
    OBJECT_COL_TYPE_NAME,
    OBJECT_COL_DEVICE_ID,
    OBJECT_COL_OBJECT_ID,
    OBJECT_COL_NAME,
    OBJECT_NUM_COLS
};

enum {
    PROPERTY_COL_DEVICE_ID,
    PROPERTY_COL_OBJECT_TYPE,
    PROPERTY_COL_OBJECT_ID,
    PROPERTY_COL_ID,
    PROPERTY_COL_ARRAY_INDEX,
    PROPERTY_COL_VALUE_TAG,
    PROPERTY_COL_NAME,
    PROPERTY_COL_VALUE,
    PROPERTY_NUM_COLS
};

typedef struct {
    uint32_t property_id;
    uint32_t array_index;
    uint8_t value_tag;
    char *value_string;
} PROPERTY_ENTRY;

typedef struct {
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    bool properties_requested;
    bool properties_busy;
    uint32_t property_index_count;
    uint32_t property_index_next;
    GPtrArray *properties;
} OBJECT_ENTRY;

typedef struct {
    uint32_t device_id;
    unsigned max_apdu;
    int segmentation;
    uint16_t vendor_id;
    bool address_valid;
    BACNET_ADDRESS address;
    bool object_list_requested;
    bool object_list_busy;
    uint32_t object_list_count;
    uint32_t object_list_next_index;
    GPtrArray *objects;
} DEVICE_ENTRY;

static GPtrArray *Device_Cache;
static uint32_t Selected_Device_ID = BACNET_MAX_INSTANCE;
static bool Selected_Object_Valid = false;
static BACNET_OBJECT_TYPE Selected_Object_Type;
static uint32_t Selected_Object_Instance;
static bool Object_Selection_Suspend;
static GQueue *Pending_Writes;
static gint64 Progress_Interrupt_Expires_Us;
static gint64 Object_List_Complete_Expires_Us;
static uint32_t Object_List_Complete_Device_ID = BACNET_MAX_INSTANCE;

#define ABSTRACT_WRITE_POOL_COUNT 8
#define PROGRESS_INTERRUPT_NOTE_MS 1200
#define OBJECT_LIST_COMPLETE_NOTE_MS 1000
typedef struct {
    bool in_use;
    uint16_t length;
    uint8_t data[MAX_APDU];
} ABSTRACT_WRITE_BUFFER;
static ABSTRACT_WRITE_BUFFER Abstract_Write_Pool[ABSTRACT_WRITE_POOL_COUNT];

typedef struct {
    uint32_t device_id;
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    BACNET_PROPERTY_ID object_property;
    uint32_t array_index;
} PENDING_WRITE;

static void refresh_property_tree_view(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance);
static bool queue_next_object_list_index(DEVICE_ENTRY *device);
static bool
queue_next_object_property_request(DEVICE_ENTRY *device, OBJECT_ENTRY *object);
static void continue_object_property_enumeration(
    DEVICE_ENTRY *device,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance);
static void update_object_progress_indicator(void);
static void update_property_progress_indicator(void);
static void interrupt_discovery_requests(void);

/**
 * @brief function to use c-stack RAM to create a string within function scope
 * @param PTR - a pointer to a local character pointer
 * @param ... - vsprintf format specifiers and variable arguments
 * @return Upon successful completion, the sprintf() function shall return
 *  the number of bytes written to the PTR, excluding the terminating null byte.
 *  If an output error was encountered, these functions shall return a negative
 * value and set errno to indicate the error.
 */
#define asprintfa(PTR, ...) \
    sprintf((*(PTR)) = alloca(1 + snprintf(NULL, 0, __VA_ARGS__)), __VA_ARGS__)

/**
 * @brief Make a string from the MAC address
 * @param str - Buffer to hold the string representation
 * @param str_len - length of the string buffer
 * @param addr - MAC address to convert to a string
 * @param len - length of the MAC address
 * @return number of bytes in the string
 */
static int
bacapp_snprintf_macaddr(char *str, size_t str_len, const uint8_t *addr, int len)
{
    int ret_val = 0;
    int slen = 0;
    int j = 0;

    while (j < len) {
        if (j != 0) {
            bacapp_snprintf(str, str_len, ":");
            ret_val += bacapp_snprintf_shift(1, &str, &str_len);
        }
        slen = bacapp_snprintf(str, str_len, "%02X", addr[j]);
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
        j++;
    }

    return ret_val;
}

/**
 * @brief Make a string from the BACnet address
 * @param str - Buffer to hold the string representation
 * @param str_len - length of the string buffer
 * @param address - BACnet address to convert to a string
 * @return number of bytes in the string
 */
static int
bacapp_snprintf_address(char *str, size_t str_len, BACNET_ADDRESS *address)
{
    int ret_val = 0;
    int slen = 0;
    uint8_t local_sadr = 0;

    /* MAC */
    slen =
        bacapp_snprintf_macaddr(str, str_len, address->mac, address->mac_len);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* NET */
    slen = bacapp_snprintf(str, str_len, ";%hu;", address->net);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* ADR */
    if (address->net) {
        slen =
            bacapp_snprintf_macaddr(str, str_len, address->adr, address->len);
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    } else {
        slen = bacapp_snprintf_macaddr(str, str_len, &local_sadr, 1);
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    }

    return ret_val;
}

static void property_entry_free(gpointer data)
{
    PROPERTY_ENTRY *entry = data;

    if (!entry) {
        return;
    }
    g_free(entry->value_string);
    g_free(entry);
}

static void pending_write_free(gpointer data)
{
    g_free(data);
}

static void object_entry_free(gpointer data)
{
    OBJECT_ENTRY *entry = data;

    if (!entry) {
        return;
    }
    g_ptr_array_free(entry->properties, true);
    g_free(entry);
}

static void device_entry_free(gpointer data)
{
    DEVICE_ENTRY *entry = data;

    if (!entry) {
        return;
    }
    g_ptr_array_free(entry->objects, true);
    g_free(entry);
}

static DEVICE_ENTRY *device_cache_find(uint32_t device_id)
{
    DEVICE_ENTRY *device = NULL;
    guint i = 0;

    if (!Device_Cache) {
        return NULL;
    }
    for (i = 0; i < Device_Cache->len; i++) {
        device = g_ptr_array_index(Device_Cache, i);
        if (device->device_id == device_id) {
            return device;
        }
    }

    return NULL;
}

static OBJECT_ENTRY *device_object_find(
    DEVICE_ENTRY *device,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance)
{
    OBJECT_ENTRY *object = NULL;
    guint i = 0;

    if (!device) {
        return NULL;
    }
    for (i = 0; i < device->objects->len; i++) {
        object = g_ptr_array_index(device->objects, i);
        if ((object->object_type == object_type) &&
            (object->object_instance == object_instance)) {
            return object;
        }
    }

    return NULL;
}

static PROPERTY_ENTRY *object_property_find(
    OBJECT_ENTRY *object, uint32_t property_id, uint32_t array_index)
{
    PROPERTY_ENTRY *property = NULL;
    guint i = 0;

    if (!object) {
        return NULL;
    }
    for (i = 0; i < object->properties->len; i++) {
        property = g_ptr_array_index(object->properties, i);
        if ((property->property_id == property_id) &&
            (property->array_index == array_index)) {
            return property;
        }
    }

    return NULL;
}

static DEVICE_ENTRY *device_cache_get_or_add(uint32_t device_id)
{
    DEVICE_ENTRY *device = NULL;

    device = device_cache_find(device_id);
    if (device) {
        return device;
    }
    device = g_new0(DEVICE_ENTRY, 1);
    device->device_id = device_id;
    device->objects = g_ptr_array_new_with_free_func(object_entry_free);
    g_ptr_array_add(Device_Cache, device);

    return device;
}

static OBJECT_ENTRY *device_object_get_or_add(
    DEVICE_ENTRY *device,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance)
{
    OBJECT_ENTRY *object = NULL;

    if (!device) {
        return NULL;
    }
    object = device_object_find(device, object_type, object_instance);
    if (object) {
        return object;
    }
    object = g_new0(OBJECT_ENTRY, 1);
    object->object_type = object_type;
    object->object_instance = object_instance;
    object->properties = g_ptr_array_new_with_free_func(property_entry_free);
    g_ptr_array_add(device->objects, object);

    return object;
}

static void object_property_upsert(
    OBJECT_ENTRY *object,
    uint32_t property_id,
    uint32_t array_index,
    uint8_t value_tag,
    char *value_string)
{
    PROPERTY_ENTRY *property = NULL;

    if (!object) {
        g_free(value_string);
        return;
    }
    property = object_property_find(object, property_id, array_index);
    if (!property) {
        property = g_new0(PROPERTY_ENTRY, 1);
        property->property_id = property_id;
        property->array_index = array_index;
        g_ptr_array_add(object->properties, property);
    }
    property->value_tag = value_tag;
    g_free(property->value_string);
    property->value_string = value_string;
}

static void pending_write_add(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint32_t array_index)
{
    PENDING_WRITE *pending = g_new0(PENDING_WRITE, 1);

    pending->device_id = device_id;
    pending->object_type = object_type;
    pending->object_instance = object_instance;
    pending->object_property = object_property;
    pending->array_index = array_index;
    g_queue_push_tail(Pending_Writes, pending);
}

static PENDING_WRITE *pending_write_pop_match(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint32_t array_index,
    bool strict_match)
{
    GList *item = NULL;
    PENDING_WRITE *pending = NULL;

    if (!Pending_Writes) {
        return NULL;
    }
    for (item = g_queue_peek_head_link(Pending_Writes); item;
         item = item->next) {
        pending = item->data;
        if (pending->device_id != device_id) {
            continue;
        }
        if (strict_match &&
            ((pending->object_type != object_type) ||
             (pending->object_instance != object_instance) ||
             (pending->object_property != object_property) ||
             (pending->array_index != array_index))) {
            continue;
        }
        g_queue_delete_link(Pending_Writes, item);
        return pending;
    }

    return NULL;
}

static bool pending_write_has_match(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint32_t array_index)
{
    GList *item = NULL;
    PENDING_WRITE *pending = NULL;

    if (!Pending_Writes) {
        return false;
    }
    for (item = g_queue_peek_head_link(Pending_Writes); item;
         item = item->next) {
        pending = item->data;
        if ((pending->device_id == device_id) &&
            (pending->object_type == object_type) &&
            (pending->object_instance == object_instance) &&
            (pending->object_property == object_property) &&
            (pending->array_index == array_index)) {
            return true;
        }
    }

    return false;
}

static void property_name_text(
    uint32_t property_id, uint32_t array_index, char *name, size_t name_size)
{
    const char *base_name = NULL;

    if (bactext_property_name_proprietary(property_id)) {
        bacapp_snprintf(
            name, name_size, "proprietary-%lu", (unsigned long)property_id);
        if (array_index != BACNET_ARRAY_ALL) {
            bacapp_snprintf(
                name, name_size, "proprietary-%lu[%lu]",
                (unsigned long)property_id, (unsigned long)array_index);
        }
    } else {
        base_name = bactext_property_name(property_id);
        if (array_index != BACNET_ARRAY_ALL) {
            bacapp_snprintf(
                name, name_size, "%s[%lu]", base_name,
                (unsigned long)array_index);
        } else {
            bacapp_snprintf(name, name_size, "%s", base_name);
        }
    }
}

static void abstract_write_pool_release_all(void)
{
    unsigned i = 0;

    for (i = 0; i < ABSTRACT_WRITE_POOL_COUNT; i++) {
        Abstract_Write_Pool[i].in_use = false;
        Abstract_Write_Pool[i].length = 0;
    }
}

static ABSTRACT_WRITE_BUFFER *abstract_write_pool_acquire(void)
{
    unsigned i = 0;

    for (i = 0; i < ABSTRACT_WRITE_POOL_COUNT; i++) {
        if (!Abstract_Write_Pool[i].in_use) {
            Abstract_Write_Pool[i].in_use = true;
            Abstract_Write_Pool[i].length = 0;
            return &Abstract_Write_Pool[i];
        }
    }

    return NULL;
}

static bool queue_write_property_value(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    BACNET_APPLICATION_DATA_VALUE *value,
    uint8_t priority,
    uint32_t array_index)
{
    ABSTRACT_WRITE_BUFFER *buffer = NULL;
    int apdu_len = 0;

    if (!value) {
        return false;
    }
    switch (value->tag) {
        case BACNET_APPLICATION_TAG_NULL:
            return bacnet_write_property_null_queue(
                device_id, object_type, object_instance, object_property,
                priority, array_index);
        case BACNET_APPLICATION_TAG_BOOLEAN:
            return bacnet_write_property_boolean_queue(
                device_id, object_type, object_instance, object_property,
                value->type.Boolean, priority, array_index);
        case BACNET_APPLICATION_TAG_REAL:
            return bacnet_write_property_real_queue(
                device_id, object_type, object_instance, object_property,
                value->type.Real, priority, array_index);
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            return bacnet_write_property_unsigned_queue(
                device_id, object_type, object_instance, object_property,
                value->type.Unsigned_Int, priority, array_index);
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            return bacnet_write_property_signed_queue(
                device_id, object_type, object_instance, object_property,
                value->type.Signed_Int, priority, array_index);
        case BACNET_APPLICATION_TAG_ENUMERATED:
            return bacnet_write_property_enumerated_queue(
                device_id, object_type, object_instance, object_property,
                value->type.Enumerated, priority, array_index);
        default:
            buffer = abstract_write_pool_acquire();
            if (!buffer) {
                return false;
            }
            apdu_len = bacapp_encode_application_data(buffer->data, value);
            if ((apdu_len <= 0) || (apdu_len > (int)sizeof(buffer->data))) {
                buffer->in_use = false;
                return false;
            }
            buffer->length = (uint16_t)apdu_len;
            if (!bacnet_write_property_abstract_syntax_queue(
                    device_id, object_type, object_instance, object_property,
                    buffer->data, buffer->length, priority, array_index)) {
                buffer->in_use = false;
                return false;
            }
            return true;
    }
}

static const char *object_name_text(OBJECT_ENTRY *object)
{
    PROPERTY_ENTRY *property = NULL;

    property = object_property_find(object, PROP_OBJECT_NAME, BACNET_ARRAY_ALL);
    if (!property) {
        property = object_property_find(object, PROP_OBJECT_NAME, 0);
    }

    return (property && property->value_string) ? property->value_string : "-";
}

static char *property_value_to_string(
    BACNET_READ_PROPERTY_DATA *rp_data, BACNET_APPLICATION_DATA_VALUE *value)
{
    BACNET_OBJECT_PROPERTY_VALUE object_value = { 0 };
    int str_len = 0;
    char *str = NULL;

    if (!rp_data || !value) {
        return g_strdup("-");
    }
    object_value.object_type = rp_data->object_type;
    object_value.object_instance = rp_data->object_instance;
    object_value.object_property = rp_data->object_property;
    object_value.array_index = rp_data->array_index;
    object_value.value = value;
    str_len = bacapp_snprintf_value(NULL, 0, &object_value);
    if (str_len <= 0) {
        return g_strdup("-");
    }
    str = g_malloc((size_t)str_len + 1);
    bacapp_snprintf_value(str, (size_t)str_len + 1, &object_value);

    return str;
}

static void refresh_device_tree_view(void)
{
    GtkTreeIter iter;
    DEVICE_ENTRY *device = NULL;
    guint i = 0;
    char address_str[64] = "-";

    gtk_list_store_clear(device_store);
    for (i = 0; i < Device_Cache->len; i++) {
        device = g_ptr_array_index(Device_Cache, i);
        if (device->address_valid) {
            bacapp_snprintf_address(
                address_str, sizeof(address_str), &device->address);
        } else {
            bacapp_snprintf(address_str, sizeof(address_str), "-");
        }
        gtk_list_store_append(device_store, &iter);
        gtk_list_store_set(
            device_store, &iter, DEVICE_COL_ID, device->device_id,
            DEVICE_COL_VENDOR_ID, device->vendor_id, DEVICE_COL_MAX_APDU,
            device->max_apdu, DEVICE_COL_SEGMENTATION,
            bactext_segmentation_name((uint32_t)device->segmentation),
            DEVICE_COL_ADDRESS, address_str, -1);
    }
}

static void refresh_object_tree_view(uint32_t device_id)
{
    GtkTreeIter iter;
    DEVICE_ENTRY *device = NULL;
    OBJECT_ENTRY *object = NULL;
    guint i = 0;

    Object_Selection_Suspend = true;
    gtk_list_store_clear(object_store);
    device = device_cache_find(device_id);
    if (!device) {
        Object_Selection_Suspend = false;
        return;
    }
    for (i = 0; i < device->objects->len; i++) {
        object = g_ptr_array_index(device->objects, i);
        gtk_list_store_append(object_store, &iter);
        gtk_list_store_set(
            object_store, &iter, OBJECT_COL_TYPE, object->object_type,
            OBJECT_COL_TYPE_NAME, bactext_object_type_name(object->object_type),
            OBJECT_COL_DEVICE_ID, device_id, OBJECT_COL_OBJECT_ID,
            object->object_instance, OBJECT_COL_NAME, object_name_text(object),
            -1);
    }
    Object_Selection_Suspend = false;
}

static void refresh_property_tree_view(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance)
{
    GtkTreeIter iter;
    DEVICE_ENTRY *device = NULL;
    OBJECT_ENTRY *object = NULL;
    PROPERTY_ENTRY *property = NULL;
    guint i = 0;
    char property_name[64] = { 0 };

    gtk_list_store_clear(property_store);
    device = device_cache_find(device_id);
    if (!device) {
        return;
    }
    object = device_object_find(device, object_type, object_instance);
    if (!object) {
        return;
    }
    for (i = 0; i < object->properties->len; i++) {
        property = g_ptr_array_index(object->properties, i);
        property_name_text(
            property->property_id, property->array_index, property_name,
            sizeof(property_name));
        gtk_list_store_append(property_store, &iter);
        gtk_list_store_set(
            property_store, &iter, PROPERTY_COL_DEVICE_ID, device_id,
            PROPERTY_COL_OBJECT_TYPE, object_type, PROPERTY_COL_OBJECT_ID,
            object_instance, PROPERTY_COL_ID, property->property_id,
            PROPERTY_COL_ARRAY_INDEX, property->array_index,
            PROPERTY_COL_VALUE_TAG, property->value_tag, PROPERTY_COL_NAME,
            property_name, PROPERTY_COL_VALUE,
            property->value_string ? property->value_string : "-", -1);
    }
}

static void queue_device_object_list(DEVICE_ENTRY *device)
{
    if (!device || device->object_list_requested) {
        return;
    }
    if (bacnet_read_property_queue(
            device->device_id, OBJECT_DEVICE, device->device_id,
            PROP_OBJECT_LIST, 0)) {
        device->object_list_requested = true;
        device->object_list_busy = true;
        device->object_list_count = 0;
        device->object_list_next_index = 0;
    }
}

static bool queue_next_object_list_index(DEVICE_ENTRY *device)
{
    if (!device || !device->object_list_requested) {
        if (device) {
            device->object_list_busy = false;
        }
        return false;
    }
    if ((device->object_list_next_index == 0) ||
        (device->object_list_next_index > device->object_list_count)) {
        Object_List_Complete_Device_ID = device->device_id;
        Object_List_Complete_Expires_Us = g_get_monotonic_time() +
            ((gint64)OBJECT_LIST_COMPLETE_NOTE_MS * 1000);
        device->object_list_requested = false;
        device->object_list_busy = false;
        device->object_list_next_index = 0;
        return false;
    }
    if (bacnet_read_property_queue(
            device->device_id, OBJECT_DEVICE, device->device_id,
            PROP_OBJECT_LIST, device->object_list_next_index)) {
        device->object_list_next_index++;
        device->object_list_busy = true;
        return true;
    }

    device->object_list_busy = false;

    return false;
}

static void update_object_progress_indicator(void)
{
    DEVICE_ENTRY *device = NULL;
    uint32_t completed = 0;
    char progress_text[64] = { 0 };
    gint64 now_us = g_get_monotonic_time();

    if (!object_progress_label) {
        return;
    }
    if ((Selected_Device_ID == Object_List_Complete_Device_ID) &&
        (now_us < Object_List_Complete_Expires_Us)) {
        gtk_label_set_text(
            GTK_LABEL(object_progress_label), "Object list complete");
        gtk_widget_show(object_progress_label);
        return;
    }
    if (Selected_Device_ID >= BACNET_MAX_INSTANCE) {
        gtk_widget_hide(object_progress_label);
        return;
    }
    device = device_cache_find(Selected_Device_ID);
    if (!device || !device->object_list_requested) {
        gtk_widget_hide(object_progress_label);
        return;
    }
    if (device->object_list_count == 0) {
        gtk_label_set_text(
            GTK_LABEL(object_progress_label), "Reading objects...");
        gtk_widget_show(object_progress_label);
        return;
    }
    completed = device->object_list_next_index;
    if (device->object_list_busy && (completed > 0)) {
        completed--;
    }
    if (completed > device->object_list_count) {
        completed = device->object_list_count;
    }
    bacapp_snprintf(
        progress_text, sizeof(progress_text), "Reading objects... %lu/%lu",
        (unsigned long)completed, (unsigned long)device->object_list_count);
    gtk_label_set_text(GTK_LABEL(object_progress_label), progress_text);
    gtk_widget_show(object_progress_label);
}

static void
queue_object_all_properties(DEVICE_ENTRY *device, OBJECT_ENTRY *object)
{
    if (!device || !object || object->properties_requested) {
        return;
    }
    object->property_index_count =
        property_list_special_count(object->object_type, PROP_ALL);
    object->property_index_next = 0;
    object->properties_requested = true;
    (void)queue_next_object_property_request(device, object);
}

static bool
queue_next_object_property_request(DEVICE_ENTRY *device, OBJECT_ENTRY *object)
{
    BACNET_PROPERTY_ID property = UINT32_MAX;

    if (!device || !object || !object->properties_requested) {
        if (object) {
            object->properties_busy = false;
        }
        return false;
    }
    while (object->property_index_next < object->property_index_count) {
        property = property_list_special_property(
            object->object_type, PROP_ALL, object->property_index_next);
        object->property_index_next++;
        if (property == UINT32_MAX) {
            continue;
        }
        if ((property == PROP_OBJECT_IDENTIFIER) ||
            (property == PROP_OBJECT_TYPE) || (property == PROP_OBJECT_LIST)) {
            /* object identity is already known from object-list */
            continue;
        }
        if (bacnet_read_property_queue(
                device->device_id, object->object_type, object->object_instance,
                property, BACNET_ARRAY_ALL)) {
            object->properties_busy = true;
            return true;
        }
        /* queue full/busy - retry next cycle or callback */
        object->property_index_next--;
        object->properties_busy = false;
        return false;
    }

    object->properties_requested = false;
    object->properties_busy = false;

    return false;
}

static void continue_object_property_enumeration(
    DEVICE_ENTRY *device,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance)
{
    OBJECT_ENTRY *object = NULL;

    if (!device) {
        return;
    }
    object = device_object_find(device, object_type, object_instance);
    if (!object) {
        return;
    }
    if (!object->properties_requested) {
        if (object->property_index_next < object->property_index_count) {
            /* recover if state was interrupted but there is still work queued
             */
            object->properties_requested = true;
        } else {
            object->properties_busy = false;
            update_property_progress_indicator();
            return;
        }
    }
    if (!object->properties_requested) {
        return;
    }
    object->properties_busy = false;
    (void)queue_next_object_property_request(device, object);
    update_property_progress_indicator();
}

static void update_property_progress_indicator(void)
{
    DEVICE_ENTRY *device = NULL;
    OBJECT_ENTRY *object = NULL;
    uint32_t completed = 0;
    char progress_text[64] = { 0 };
    gint64 now_us = g_get_monotonic_time();

    if (!property_progress_label) {
        return;
    }
    if (now_us < Progress_Interrupt_Expires_Us) {
        gtk_label_set_text(
            GTK_LABEL(property_progress_label), "Canceled previous read");
        gtk_widget_show(property_progress_label);
        return;
    }
    if (!Selected_Object_Valid) {
        gtk_widget_hide(property_progress_label);
        return;
    }
    device = device_cache_find(Selected_Device_ID);
    object = device_object_find(
        device, Selected_Object_Type, Selected_Object_Instance);
    if (!object || !object->properties_requested ||
        (object->property_index_count == 0)) {
        gtk_widget_hide(property_progress_label);
        return;
    }
    completed = object->property_index_next;
    if (object->properties_busy && (completed > 0)) {
        completed--;
    }
    if (completed > object->property_index_count) {
        completed = object->property_index_count;
    }
    bacapp_snprintf(
        progress_text, sizeof(progress_text), "Reading properties... %lu/%lu",
        (unsigned long)completed, (unsigned long)object->property_index_count);
    gtk_label_set_text(GTK_LABEL(property_progress_label), progress_text);
    gtk_widget_show(property_progress_label);
}

static void interrupt_discovery_requests(void)
{
    DEVICE_ENTRY *device = NULL;
    OBJECT_ENTRY *object = NULL;
    guint d = 0;
    guint o = 0;
    bool interrupted = false;

    for (d = 0; d < Device_Cache->len; d++) {
        device = g_ptr_array_index(Device_Cache, d);
        if (device->object_list_requested) {
            interrupted = true;
        }
        device->object_list_requested = false;
        device->object_list_busy = false;
        device->object_list_count = 0;
        device->object_list_next_index = 0;
        for (o = 0; o < device->objects->len; o++) {
            object = g_ptr_array_index(device->objects, o);
            if (object->properties_requested || object->properties_busy) {
                interrupted = true;
            }
            object->properties_requested = false;
            object->properties_busy = false;
            object->property_index_count = 0;
            object->property_index_next = 0;
        }
    }
    if (interrupted) {
        Progress_Interrupt_Expires_Us = g_get_monotonic_time() +
            ((gint64)PROGRESS_INTERRUPT_NOTE_MS * 1000);
    }
    Object_List_Complete_Expires_Us = 0;
    Object_List_Complete_Device_ID = BACNET_MAX_INSTANCE;
}

static void bacnet_read_write_device_callback(
    uint32_t device_instance,
    unsigned max_apdu,
    int segmentation,
    uint16_t vendor_id)
{
    DEVICE_ENTRY *device = NULL;
    unsigned int bound_max_apdu = 0;

    device = device_cache_get_or_add(device_instance);
    device->max_apdu = max_apdu;
    device->segmentation = segmentation;
    device->vendor_id = vendor_id;
    device->address_valid = address_get_by_device(
        device_instance, &bound_max_apdu, &device->address);
    refresh_device_tree_view();
}

static void bacnet_read_write_success_callback(uint32_t device_instance)
{
    PENDING_WRITE *pending = NULL;

    printf(
        "WriteProperty success: device=%lu\n", (unsigned long)device_instance);
    pending = pending_write_pop_match(device_instance, 0, 0, 0, 0, false);
    if (!pending) {
        return;
    }
    /* queue a re-read so the displayed value refreshes after write */
    if (bacnet_read_property_queue(
            pending->device_id, pending->object_type, pending->object_instance,
            pending->object_property, pending->array_index)) {
        /* re-add as stale-filter marker so the read response is accepted */
        pending_write_add(
            pending->device_id, pending->object_type, pending->object_instance,
            pending->object_property, pending->array_index);
    }
    pending_write_free(pending);
}

static void bacnet_read_write_value_callback(
    uint32_t device_instance,
    BACNET_READ_PROPERTY_DATA *rp_data,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    DEVICE_ENTRY *device = NULL;
    OBJECT_ENTRY *object = NULL;
    char *value_string = NULL;

    if (!rp_data) {
        return;
    }
    device = device_cache_get_or_add(device_instance);

    if ((rp_data->object_type == OBJECT_DEVICE) &&
        (rp_data->object_instance == device_instance) &&
        (rp_data->object_property == PROP_OBJECT_LIST) &&
        (rp_data->array_index != BACNET_ARRAY_ALL) &&
        !device->object_list_requested) {
        return;
    }

    if (!((rp_data->object_type == OBJECT_DEVICE) &&
          (rp_data->object_instance == device_instance) &&
          (rp_data->object_property == PROP_OBJECT_LIST))) {
        bool read_active = false;

        object = device_object_find(
            device, rp_data->object_type, rp_data->object_instance);
        read_active =
            object && (object->properties_requested || object->properties_busy);
        if ((!read_active) &&
            !pending_write_has_match(
                device_instance, rp_data->object_type, rp_data->object_instance,
                rp_data->object_property, rp_data->array_index)) {
            /* ignore stale read responses after selection changes */
            return;
        }
    }

    if ((rp_data->error_code != ERROR_CODE_SUCCESS) || !value) {
        if ((rp_data->object_type == OBJECT_DEVICE) &&
            (rp_data->object_instance == device_instance) &&
            (rp_data->object_property == PROP_OBJECT_LIST) &&
            (rp_data->array_index != BACNET_ARRAY_ALL) &&
            (rp_data->array_index > 0)) {
            (void)queue_next_object_list_index(device);
            update_object_progress_indicator();
        } else {
            continue_object_property_enumeration(
                device, rp_data->object_type, rp_data->object_instance);
        }
        {
            PENDING_WRITE *pw = pending_write_pop_match(
                device_instance, rp_data->object_type, rp_data->object_instance,
                rp_data->object_property, rp_data->array_index, true);
            if (pw) {
                pending_write_free(pw);
            }
        }
        return;
    }

    if ((rp_data->object_type == OBJECT_DEVICE) &&
        (rp_data->object_instance == device_instance) &&
        (rp_data->object_property == PROP_OBJECT_LIST) &&
        (rp_data->array_index != BACNET_ARRAY_ALL)) {
        device->object_list_busy = false;
        if ((rp_data->array_index == 0) &&
            (value->tag == BACNET_APPLICATION_TAG_UNSIGNED_INT)) {
            device->object_list_count = value->type.Unsigned_Int;
            if (device->object_list_count > 0) {
                device->object_list_next_index = 1;
                (void)queue_next_object_list_index(device);
            } else {
                Object_List_Complete_Device_ID = device->device_id;
                Object_List_Complete_Expires_Us = g_get_monotonic_time() +
                    ((gint64)OBJECT_LIST_COMPLETE_NOTE_MS * 1000);
                device->object_list_requested = false;
            }
            update_object_progress_indicator();
        } else if (value->tag == BACNET_APPLICATION_TAG_OBJECT_ID) {
            (void)device_object_get_or_add(
                device, value->type.Object_Id.type,
                value->type.Object_Id.instance);
            (void)queue_next_object_list_index(device);
            update_object_progress_indicator();
            if (Selected_Device_ID == device_instance) {
                refresh_object_tree_view(device_instance);
            }
        }
        return;
    }

    object = device_object_get_or_add(
        device, rp_data->object_type, rp_data->object_instance);
    value_string = property_value_to_string(rp_data, value);
    object_property_upsert(
        object, rp_data->object_property, rp_data->array_index, value->tag,
        value_string);
    /* pop any pending re-read-after-write marker */
    {
        PENDING_WRITE *pw = pending_write_pop_match(
            device_instance, rp_data->object_type, rp_data->object_instance,
            rp_data->object_property, rp_data->array_index, true);
        if (pw) {
            pending_write_free(pw);
        }
    }
    continue_object_property_enumeration(
        device, rp_data->object_type, rp_data->object_instance);
    if (Selected_Device_ID == device_instance) {
        if (rp_data->object_property == PROP_OBJECT_NAME) {
            refresh_object_tree_view(device_instance);
        }
        if (Selected_Object_Valid &&
            (Selected_Object_Type == rp_data->object_type) &&
            (Selected_Object_Instance == rp_data->object_instance)) {
            refresh_property_tree_view(
                device_instance, rp_data->object_type,
                rp_data->object_instance);
        }
    }
}

/**
 * @brief Handle device selection change
 * @param selection the selection that is changed
 * @param data optional data in the callback
 */
static void
on_device_selection_changed(GtkTreeSelection *selection, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    guint device_id = 0;
    DEVICE_ENTRY *device = NULL;

    (void)data;
    interrupt_discovery_requests();
    Selected_Object_Valid = false;
    update_object_progress_indicator();
    update_property_progress_indicator();
    gtk_list_store_clear(property_store);
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, DEVICE_COL_ID, &device_id, -1);
        Selected_Device_ID = device_id;
        refresh_object_tree_view(device_id);
        device = device_cache_find(device_id);
        queue_device_object_list(device);
        update_object_progress_indicator();
    }
}

/**
 * @brief Handle object selection change
 * @param selection selection that was changed
 * @param data optional data for the callback
 */
static void
on_object_selection_changed(GtkTreeSelection *selection, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    guint object_instance = 0;
    guint object_type = 0;
    guint device_id = 0;
    DEVICE_ENTRY *device = NULL;
    OBJECT_ENTRY *object = NULL;

    (void)data;
    if (Object_Selection_Suspend) {
        return;
    }
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        interrupt_discovery_requests();
        update_object_progress_indicator();
        gtk_tree_model_get(
            model, &iter, OBJECT_COL_DEVICE_ID, &device_id,
            OBJECT_COL_OBJECT_ID, &object_instance, OBJECT_COL_TYPE,
            &object_type, -1);
        Selected_Object_Valid = true;
        Selected_Object_Type = (BACNET_OBJECT_TYPE)object_type;
        Selected_Object_Instance = object_instance;
        refresh_property_tree_view(
            device_id, (BACNET_OBJECT_TYPE)object_type, object_instance);
        device = device_cache_find(device_id);
        object = device_object_find(
            device, (BACNET_OBJECT_TYPE)object_type, object_instance);
        queue_object_all_properties(device, object);
        update_property_progress_indicator();
    }
}

/**
 * @brief Handle discover devices button click
 * @param button button that was clicked
 * @param data optional data for the callback
 */
static void on_discover_devices_clicked(GtkButton *button, gpointer data)
{
    (void)button; /* unused parameter */
    (void)data; /* unused parameter */

    if (!bacnet_initialized) {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(
            GTK_WINDOW(main_window), GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "BACnet stack not initialized. Please restart the "
            "application.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    Send_WhoIs_Global(0, BACNET_MAX_INSTANCE);
}

static void on_property_edited(
    GtkCellRendererText *renderer,
    gchar *path_string,
    gchar *new_text,
    gpointer user_data)
{
    GtkTreeModel *model = GTK_TREE_MODEL(user_data);
    GtkTreeIter iter = { 0 };
    guint device_id = 0;
    guint object_type = 0;
    guint object_id = 0;
    guint property_id = 0;
    guint array_index = 0;
    long priority = BACNET_NO_PRIORITY;
    guint value_tag = 0;
    unsigned enumerated_value = 0;
    bool status = false;
    bool null_value = false;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    (void)renderer; /* unused parameter */
    if (gtk_tree_model_get_iter_from_string(model, &iter, path_string)) {
        gtk_tree_model_get(
            model, &iter, PROPERTY_COL_DEVICE_ID, &device_id, -1);
        gtk_tree_model_get(
            model, &iter, PROPERTY_COL_OBJECT_TYPE, &object_type, -1);
        gtk_tree_model_get(
            model, &iter, PROPERTY_COL_OBJECT_ID, &object_id, -1);
        gtk_tree_model_get(
            model, &iter, PROPERTY_COL_ARRAY_INDEX, &array_index, -1);
        gtk_tree_model_get(
            model, &iter, PROPERTY_COL_VALUE_TAG, &value_tag, -1);
        gtk_tree_model_get(model, &iter, PROPERTY_COL_ID, &property_id, -1);
        /* allow for optional priority using @ symbol for commandables */
        if (property_list_commandable_member(object_type, property_id)) {
            /* search the new_text for the @ symbol */
            char *at_ptr = strchr(new_text, '@');
            if (at_ptr) {
                /* convert the priority value after the @ symbol
                  into an integer */
                priority = strtol(at_ptr + 1, NULL, 0);
                if (priority < BACNET_MIN_PRIORITY) {
                    priority = BACNET_NO_PRIORITY;
                }
                if (priority > BACNET_MAX_PRIORITY) {
                    priority = BACNET_NO_PRIORITY;
                }
                /* null terminate the string at the @ symbol */
                *at_ptr = 0;
            }
            /* check for case insensitive NULL string */
            if (bacnet_strnicmp(new_text, "NULL", 4) == 0) {
                null_value = true;
            }
        }
        /* convert the string value into a tagged union value */
        if (null_value) {
            value.tag = BACNET_APPLICATION_TAG_NULL;
            status = true;
        } else if (value_tag == BACNET_APPLICATION_TAG_ENUMERATED) {
            status = bactext_object_property_strtoul(
                (BACNET_OBJECT_TYPE)object_type,
                (BACNET_PROPERTY_ID)property_id, new_text, &enumerated_value);
            if (status) {
                value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
                value.type.Enumerated = (uint32_t)enumerated_value;
            }
        } else {
            status = bacapp_parse_application_data(value_tag, new_text, &value);
        }
        printf(
            "Parsed %s-%u %s %s -> tag=%u %s\n",
            bactext_object_type_name(object_type), object_id,
            bactext_property_name(property_id), new_text, value_tag,
            status ? "successfully" : "unsuccessfully");
        if (status) {
            status = queue_write_property_value(
                device_id, (BACNET_OBJECT_TYPE)object_type, object_id,
                (BACNET_PROPERTY_ID)property_id, &value, (uint8_t)priority,
                array_index);
            if (status) {
                pending_write_add(
                    device_id, (BACNET_OBJECT_TYPE)object_type, object_id,
                    (BACNET_PROPERTY_ID)property_id, array_index);
                printf(
                    "WriteProperty to Device %u %s-%u %s = %s\n", device_id,
                    bactext_object_type_name(object_type), object_id,
                    bactext_property_name(property_id), new_text);
            } else {
                printf(
                    "WriteProperty queue failed for Device %u %s-%u %s\n",
                    device_id, bactext_object_type_name(object_type), object_id,
                    bactext_property_name(property_id));
            }
        }
    }
}

/**
 * @brief Handle refresh button click
 * @param button button that was clicked
 * @param data optional data for the callback
 */
static void on_refresh_clicked(GtkButton *button, gpointer data)
{
    (void)button; /* unused parameter */
    (void)data; /* unused parameter */

    Selected_Device_ID = BACNET_MAX_INSTANCE;
    Selected_Object_Valid = false;
    Object_List_Complete_Expires_Us = 0;
    Object_List_Complete_Device_ID = BACNET_MAX_INSTANCE;
    update_object_progress_indicator();
    update_property_progress_indicator();
    g_queue_clear_full(Pending_Writes, pending_write_free);
    g_ptr_array_set_size(Device_Cache, 0);
    gtk_list_store_clear(device_store);
    gtk_list_store_clear(object_store);
    gtk_list_store_clear(property_store);
    Send_WhoIs_Global(0, BACNET_MAX_INSTANCE);
}

/**
 * @brief Setup the device tree view object
 */
static void setup_device_tree_view(void)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;

    /* Create list store */
    device_store = gtk_list_store_new(
        DEVICE_NUM_COLS, G_TYPE_UINT, /* Device ID */
        G_TYPE_UINT, /* Vendor ID */
        G_TYPE_UINT, /* Max APDU */
        G_TYPE_STRING, /* Segmentation */
        G_TYPE_STRING); /* Address */

    /* Create tree view */
    device_tree_view =
        gtk_tree_view_new_with_model(GTK_TREE_MODEL(device_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(device_tree_view), TRUE);

    /* Device ID column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Device ID", renderer, "text", DEVICE_COL_ID, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(device_tree_view), column);

    /* Vendor ID column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Vendor ID", renderer, "text", DEVICE_COL_VENDOR_ID, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(device_tree_view), column);

    /* Max APDU column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Max APDU", renderer, "text", DEVICE_COL_MAX_APDU, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(device_tree_view), column);

    /* Segmentation column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Segmentation", renderer, "text", DEVICE_COL_SEGMENTATION, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(device_tree_view), column);

    /* Address column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Address", renderer, "text", DEVICE_COL_ADDRESS, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(device_tree_view), column);

    /* Setup selection changed callback */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(device_tree_view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    g_signal_connect(
        selection, "changed", G_CALLBACK(on_device_selection_changed), NULL);
}

/**
 * @brief Setup the object tree view object
 */
static void setup_object_tree_view(void)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;

    /* Create list store */
    object_store = gtk_list_store_new(
        OBJECT_NUM_COLS, G_TYPE_UINT, /* OBJECT_COL_TYPE */
        G_TYPE_STRING, /* OBJECT_COL_TYPE_NAME */
        G_TYPE_UINT, /* OBJECT_COL_DEVICE_ID */
        G_TYPE_UINT, /* OBJECT_COL_OBJECT_ID */
        G_TYPE_STRING); /* OBJECT_COL_NAME */

    /* Create tree view */
    object_tree_view =
        gtk_tree_view_new_with_model(GTK_TREE_MODEL(object_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(object_tree_view), TRUE);

    /* Object Type Name column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Object Type", renderer, "text", OBJECT_COL_TYPE_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(object_tree_view), column);

    /* Object Instance column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Instance", renderer, "text", OBJECT_COL_OBJECT_ID, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(object_tree_view), column);

    /* Object Name column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Name", renderer, "text", OBJECT_COL_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(object_tree_view), column);

    /* Setup selection changed callback */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(object_tree_view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    g_signal_connect(
        selection, "changed", G_CALLBACK(on_object_selection_changed), NULL);
}

/**
 * @brief Setup the property tree view object
 */
static void setup_property_tree_view(void)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    /* Create list store */
    property_store = gtk_list_store_new(
        PROPERTY_NUM_COLS, G_TYPE_UINT, /* PROPERTY_COL_DEVICE_ID */
        G_TYPE_UINT, /* PROPERTY_COL_OBJECT_TYPE */
        G_TYPE_UINT, /* PROPERTY_COL_OBJECT_ID */
        G_TYPE_UINT, /* PROPERTY_COL_ID */
        G_TYPE_UINT, /* PROPERTY_COL_ARRAY_INDEX */
        G_TYPE_INT, /* PROPERTY_COL_VALUE_TAG */
        G_TYPE_STRING, /* PROPERTY_COL_NAME */
        G_TYPE_STRING); /* PROPERTY_COL_VALUE */

    /* Create tree view */
    property_tree_view =
        gtk_tree_view_new_with_model(GTK_TREE_MODEL(property_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(property_tree_view), TRUE);

    /* Property Name column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Property", renderer, "text", PROPERTY_COL_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(property_tree_view), column);

    /* Property Value column */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Value", renderer, "text", PROPERTY_COL_VALUE, NULL);
    g_object_set(renderer, "editable", TRUE, NULL);
    g_signal_connect(
        renderer, "edited", G_CALLBACK(on_property_edited),
        GTK_TREE_MODEL(property_store));
    gtk_tree_view_append_column(GTK_TREE_VIEW(property_tree_view), column);
}

/**
 * @brief Create the main application window
 */
static void create_main_window(void)
{
    GtkWidget *vbox;
    GtkWidget *toolbar;
    GtkWidget *hpaned, *vpaned;
    GtkWidget *scrolled_window;
    GtkWidget *discover_button, *refresh_button;
    GtkToolItem *tool_item;
    GdkPixbuf *icon_pixbuf;

    /* Create main window */
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "BACnet Device Discovery");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 1200, 800);
    gtk_container_set_border_width(GTK_CONTAINER(main_window), 5);

    /* set the icon */
    icon_pixbuf = gdk_pixbuf_new_from_xpm_data(bacnet_icon);
    if (icon_pixbuf) {
        gtk_window_set_icon(GTK_WINDOW(main_window), icon_pixbuf);
    }

    /* Connect destroy signal */
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* Create main vertical box */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(main_window), vbox);

    /* Create toolbar */
    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    /* Add discover button */
    discover_button = gtk_button_new_with_label("Discover Devices");
    g_signal_connect(
        discover_button, "clicked", G_CALLBACK(on_discover_devices_clicked),
        NULL);
    tool_item = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(tool_item), discover_button);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);

    /* Add refresh button */
    refresh_button = gtk_button_new_with_label("Refresh");
    g_signal_connect(
        refresh_button, "clicked", G_CALLBACK(on_refresh_clicked), NULL);
    tool_item = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(tool_item), refresh_button);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);

    /* Add object read progress status */
    object_progress_label = gtk_label_new("");
    gtk_widget_set_halign(object_progress_label, GTK_ALIGN_START);
    gtk_widget_set_valign(object_progress_label, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start(object_progress_label, 12);
    gtk_widget_hide(object_progress_label);
    tool_item = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(tool_item), object_progress_label);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);

    /* Add property read progress status */
    property_progress_label = gtk_label_new("");
    gtk_widget_set_halign(property_progress_label, GTK_ALIGN_START);
    gtk_widget_set_valign(property_progress_label, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start(property_progress_label, 12);
    gtk_widget_hide(property_progress_label);
    tool_item = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(tool_item), property_progress_label);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);

    /* Create horizontal paned widget */
    hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);

    /* Create vertical paned widget for right side */
    vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_pack2(GTK_PANED(hpaned), vpaned, TRUE, FALSE);

    /* Setup device tree view (left side) */
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled_window, 500, -1);
    setup_device_tree_view();
    gtk_container_add(GTK_CONTAINER(scrolled_window), device_tree_view);
    gtk_paned_pack1(GTK_PANED(hpaned), scrolled_window, FALSE, FALSE);

    /* Setup object tree view (top right) */
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled_window, -1, 200);
    setup_object_tree_view();
    gtk_container_add(GTK_CONTAINER(scrolled_window), object_tree_view);
    gtk_paned_pack1(GTK_PANED(vpaned), scrolled_window, TRUE, FALSE);

    /* Setup property tree view (bottom right) */
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC);
    setup_property_tree_view();
    gtk_container_add(GTK_CONTAINER(scrolled_window), property_tree_view);
    gtk_paned_pack2(GTK_PANED(vpaned), scrolled_window, TRUE, FALSE);

    /* Set paned positions */
    gtk_paned_set_position(GTK_PANED(hpaned), 500);
    gtk_paned_set_position(GTK_PANED(vpaned), 200);
}

/**
 * @brief Non-blocking task for running BACnet server tasks
 */
static void bacnet_server_task(void)
{
    static bool initialized = false;
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    const unsigned timeout_ms = 5;

    if (!initialized) {
        initialized = true;
        /* broadcast an I-Am on startup */
        Send_I_Am(&Handler_Transmit_Buffer[0]);
    }
    /* input */
    /* returns 0 bytes on timeout */
    pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout_ms);
    /* process */
    if (pdu_len) {
        npdu_handler(&src, &Rx_Buf[0], pdu_len);
    }
    /* 1 second tasks */
    if (mstimer_expired(&BACnet_Task_Timer)) {
        mstimer_reset(&BACnet_Task_Timer);
        dcc_timer_seconds(1);
        datalink_maintenance_timer(1);
        dlenv_maintenance_timer(1);
    }
    if (mstimer_expired(&BACnet_TSM_Timer)) {
        mstimer_reset(&BACnet_TSM_Timer);
        tsm_timer_milliseconds(mstimer_interval(&BACnet_TSM_Timer));
    }
}

/* GTK timeout callback for BACnet processing */
static gboolean bacnet_task_timeout(gpointer data)
{
    DEVICE_ENTRY *device = NULL;
    OBJECT_ENTRY *object = NULL;

    (void)data; /* unused parameter */

    if (bacnet_initialized) {
        bacnet_server_task();
        bacnet_read_write_task();
        if (Selected_Device_ID < BACNET_MAX_INSTANCE) {
            device = device_cache_find(Selected_Device_ID);
            if (device && device->object_list_requested &&
                !device->object_list_busy && (device->object_list_count > 0)) {
                (void)queue_next_object_list_index(device);
            }
            update_object_progress_indicator();
        }
        if (Selected_Object_Valid) {
            device = device_cache_find(Selected_Device_ID);
            object = device_object_find(
                device, Selected_Object_Type, Selected_Object_Instance);
            if (object && object->properties_requested &&
                !object->properties_busy) {
                (void)queue_next_object_property_request(device, object);
            }
            update_property_progress_indicator();
        }
        if (bacnet_read_write_idle()) {
            abstract_write_pool_release_all();
        }
    }

    return TRUE; /* Continue calling this function */
}

/**
 * @brief Initialize the handlers for this server device
 */
static void bacnet_server_init(void)
{
    Device_Init(NULL);
    bacnet_read_write_init();
    bacnet_read_write_device_callback_set(bacnet_read_write_device_callback);
    bacnet_read_write_success_callback_set(bacnet_read_write_success_callback);
    bacnet_read_write_value_callback_set(bacnet_read_write_value_callback);
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* we need to handle who-has to support dynamic object binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    /* set the handler for all the services we don't implement */
    /* It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    mstimer_set(&BACnet_Task_Timer, 1000);
    mstimer_set(&BACnet_TSM_Timer, 50);

    /* Start BACnet background processing */
    bacnet_timeout_id = g_timeout_add(10, bacnet_task_timeout, NULL);

    bacnet_initialized = true;
    printf("BACnet Stack initialized\n");
}

/* Cleanup BACnet stack */
static void bacnet_cleanup(void)
{
    if (bacnet_timeout_id > 0) {
        g_source_remove(bacnet_timeout_id);
        bacnet_timeout_id = 0;
    }

    if (bacnet_initialized) {
        datalink_cleanup();
        bacnet_initialized = false;
        printf("BACnet Stack cleanup completed\n");
    }
}

/* Main function */
int main(int argc, char *argv[])
{
    (void)argv;

    /* Initialize GTK */
    gtk_init(&argc, &argv);
    Device_Cache = g_ptr_array_new_with_free_func(device_entry_free);
    Pending_Writes = g_queue_new();

    /* Initialize BACnet */
    dlenv_init();
    bacnet_server_init();

    /* Create the main window */
    create_main_window();

    /* Show the window */
    gtk_widget_show_all(main_window);

    /* Start the GTK main loop */
    gtk_main();

    /* Cleanup */
    bacnet_cleanup();
    g_queue_free_full(Pending_Writes, pending_write_free);
    g_ptr_array_free(Device_Cache, true);

    return 0;
}
