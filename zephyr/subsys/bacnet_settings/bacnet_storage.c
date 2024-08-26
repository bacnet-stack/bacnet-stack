/**
 * @file
 * @brief The BACnet storage tasks for handling the device specific object data
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <zephyr/settings/settings.h>
#if defined(CONFIG_SETTINGS_FILE) && defined(CONFIG_FILE_SYSTEM_LITTLEFS)
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#elif defined(CONFIG_SETTINGS_FILE) && defined(CONFIG_FILE_SYSTEM_EXT2)
#include <zephyr/fs/fs.h>
#include <zephyr/fs/ext2.h>
#endif
/* me! */
#include "bacnet_settings/bacnet_storage.h"

#ifdef CONFIG_BACNET_SETTINGS_BASE_NAME
#define BACNET_STORAGE_BASE_NAME CONFIG_BACNET_SETTINGS_BASE_NAME
#else
#define BACNET_STORAGE_BASE_NAME ".bacnet"
#endif

/* Logging module registration is already done in bacnet/ports/zephyr/main.c */
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);
#define FAIL_MSG "fail (err %d)"

#define STORAGE_PARTITION storage_partition
#define STORAGE_PARTITION_ID FIXED_PARTITION_ID(STORAGE_PARTITION)

/**
 * @brief Initialize the non-volatile data
*/
void bacnet_storage_init(void)
{
    int rc;

#if defined(CONFIG_SETTINGS_FILE) && defined(CONFIG_FILE_SYSTEM_LITTLEFS)
    FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);

    /* mounting info */
    static struct fs_mount_t littlefs_mnt = {
        .type = FS_LITTLEFS,
        .fs_data = &cstorage,
        .storage_dev = (void *)STORAGE_PARTITION_ID,
        .mnt_point = "/ff"
    };

    rc = fs_mount(&littlefs_mnt);
    if (rc != 0) {
        LOG_INF("mounting littlefs error: [%d]", rc);
    } else {
        rc = fs_unlink(CONFIG_SETTINGS_FILE_PATH);
        if ((rc != 0) && (rc != -ENOENT)) {
            H("can't delete config file%d", rc);
        } else {
            LOG_INF("FS initialized: OK");
        }
    }
#endif
    rc = settings_subsys_init();
    if (rc) {
        LOG_INF("settings subsys initialization: fail (err %d)", rc);
        return;
    }

    LOG_INF("settings subsys initialization: OK.");
}

/**
 * @brief Initialize a BACnet key object with optional array
 * @param key BACnet key (type, instance, property, array index)
 * @param object_type BACnet object type
 * @param object_instance BACnet object instance
 * @param property_id BACnet property id
 * @param array_index BACnet array index
 */
void bacnet_storage_key_init(BACNET_STORAGE_KEY *key, uint16_t object_type,
                 uint32_t object_instance, uint32_t property_id,
                 uint32_t array_index)
{
    if (key) {
        key->object_type = object_type;
        key->object_instance = object_instance;
        key->property_id = property_id;
        key->array_index = array_index;
    }
}

/**
 * @brief Create a storage key string for a BACnet object property
 * @param buffer buffer to store key string
 * @param buffer_size size of key buffer
 * @param key BACnet key (type, instance, property, array index)
 * @return length of the string
 */
int bacnet_storage_key_encode(char *buffer, size_t buffer_size,
                  BACNET_STORAGE_KEY *key)
{
    int rc = 0;
    const char base_name[] = CONFIG_BACNET_STORAGE_BASE_NAME;

    if (buffer) {
        memset(buffer, 0, buffer_size);
    }
    if (key->array_index == BACNET_STORAGE_ARRAY_INDEX_NONE) {
        rc = snprintf(buffer, buffer_size, "%s%c%u%c%lu%c%lu",
                  base_name, SETTINGS_NAME_SEPARATOR,
                  (unsigned short)key->object_type,
                  SETTINGS_NAME_SEPARATOR,
                  (unsigned long)key->object_instance,
                  SETTINGS_NAME_SEPARATOR,
                  (unsigned long)key->property_id);
    } else {
        rc = snprintf(buffer, buffer_size, "%s%c%u%c%lu%c%lu%c%lu",
                  base_name, SETTINGS_NAME_SEPARATOR,
                  (unsigned short)key->object_type,
                  SETTINGS_NAME_SEPARATOR,
                  (unsigned long)key->object_instance,
                  SETTINGS_NAME_SEPARATOR,
                  (unsigned long)key->property_id,
                  SETTINGS_NAME_SEPARATOR,
                  (unsigned long)key->array_index);
    }

    return rc;
}

/**
 * @brief Set a value with a specific key to non-volatile storage
 * @param key [in] Key in string format.
 * @param data [in] one or more bytes of data
 * @param data_len [in] Value length in bytes.
 * @return 0 on success, non-zero on failure.
 */
int bacnet_storage_set(BACNET_STORAGE_KEY *key, const void *data,
               size_t data_len)
{
    char name[SETTINGS_MAX_NAME_LEN + 1] = { 0 };
    int rc;

    rc = bacnet_storage_key_encode(name, sizeof(name), key);
    LOG_INF("Set a key-value pair. Key=%s", name);
    rc = settings_save_one(name, data, data_len);
    if (rc) {
        LOG_INF(FAIL_MSG, rc);
    } else {
        LOG_HEXDUMP_INF(data, data_len, "value");
    }

    return rc;
}

/**
 * @brief Structure to hold immediate values
 */
struct direct_immediate_value {
    size_t value_size;
    size_t value_len;
    void *value;
    bool fetched;
};

/**
 * @brief Direct loader for immediate values
 * @param name [in] Key in string format.
 * @param len [in] Length of the key
 * @param read_cb [in] Callback to read the value
 * @param cb_arg [in] Callback argument
 * @param param [in] Callback parameter
 * @return 0 on success, non-zero on failure.
 */
static int direct_loader_immediate_value(const char *name, size_t len,
                     settings_read_cb read_cb, void *cb_arg,
                     void *param)
{
    const char *next;
    size_t name_len;
    int rc;
    struct direct_immediate_value *context =
        (struct direct_immediate_value *)param;

    /* only the exact match and ignore descendants of the searched name */
    name_len = settings_name_next(name, &next);
    if (name_len == 0) {
        rc = read_cb(cb_arg, context->value, len);
        if ((rc >= 0) && (rc <= context->value_size)) {
            context->fetched = true;
            context->value_len = rc;
            LOG_INF("immediate load: OK.");
            return 0;
        }
        return -EINVAL;
    }

    /* other keys aren't served by the callback
     * Return success in order to skip them
     * and keep storage processing.
     */
    return 0;
}

/**
 * @brief Load an immediate value from non-volatile storage
 * @param name [in] Key in string format.
 * @param value [out] Buffer to store the value
 * @param value_size [in] size of the buffer
 * @return value length in bytes on success 0..N, negative on failure.
 */
static int load_immediate_value(const char *name, void *value,
                size_t value_size)
{
    int rc;
    struct direct_immediate_value context;

    context.fetched = false;
    context.value_size = value_size;
    context.value_len = 0;
    context.value = value;

    rc = settings_load_subtree_direct(name, direct_loader_immediate_value,
                      (void *)&context);
    if (rc == 0) {
        if (!context.fetched) {
            rc = -ENOENT;
        }
    }

    return context.value_len;
}

/**
 * @brief Get a value with a specific key to non-volatile storage
 * @param key [in] Key in string format.
 * @param data [out] Binary value.
 * @param data_size [in] requested value length in bytes
 * @return data length on success 0..N, negative on failure.
 */
int bacnet_storage_get(BACNET_STORAGE_KEY *key, void *data, size_t data_size)
{
    char name[SETTINGS_MAX_NAME_LEN + 1] = { 0 };
    int rc;

    rc = bacnet_storage_key_encode(name, sizeof(name), key);
    LOG_INF("Get a key-value pair. Key=<%s>", name);
    rc = load_immediate_value(name, data, data_size);
    if (rc == 0) {
        LOG_INF("empty entry");
    } else if (rc > 0) {
        LOG_HEXDUMP_INF(data, rc, "value");
    } else if (rc == -ENOENT) {
        LOG_INF("no entry");
    } else {
        LOG_INF("unexpected" FAIL_MSG, rc);
    }

    return rc;
}
