/**
 * @file point_table.c
 * @brief JSON loading, BACnet object creation, and Modbus polling for the
 *        generic Modbus RTU <-> BACnet gateway.
 *
 * JSON schema (gateway_config.json):
 * {
 *   "bacnet": {
 *     "device_instance":..., "device_name":..., "vendor_id":...,
 *     "datalink_type": "mstp"|"bip"|"bip6"|"ethernet",
 *     "iface": "/dev/ttyUSB0" or "eth0",
 *     "mstp_baud":..., "mstp_mac":..., "mstp_max_master":..., "mstp_net":...,
 *     "bip_port":...
 *   },
 *   "modbus": { "port":..., "baud":..., "poll_interval_sec":... },
 *   "points": [
 *     { "name":..., "description":...,
 *       "bacnet_type": "AI"|"AO"|"BI"|"BO",
 *       "bacnet_instance":...,
 *       "units": "DEGREES_CELSIUS"|"PERCENT_RELATIVE_HUMIDITY"|...,
 *       "modbus_slave":...,
 *       "modbus_func":
 * "COIL"|"DISCRETE_INPUT"|"HOLDING_REGISTER"|"INPUT_REGISTER",
 *       "modbus_address":...,
 *       "scale":..., "offset":...,
 *       "range_min":..., "range_max":... },
 *     ...
 *   ]
 * }
 *
 * @author Jihye Ahn <jen.ahn@lge.com>
 *
 * @copyright Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "point_table.h"
#include "modbus_rtu.h"
#include "cJSON.h"

/* BACnet object headers */
#include "bacnet/bacdef.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/ao.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/bo.h"

/* -----------------------------------------------------------------------
 * Internal helpers – string to enum conversion
 * -----------------------------------------------------------------------*/
static GW_BACNET_TYPE parse_bacnet_type(const char *s)
{
    if (!s) {
        return GW_BACNET_TYPE_AI;
    }
    if (strcmp(s, "AI") == 0) {
        return GW_BACNET_TYPE_AI;
    }
    if (strcmp(s, "AO") == 0) {
        return GW_BACNET_TYPE_AO;
    }
    if (strcmp(s, "BI") == 0) {
        return GW_BACNET_TYPE_BI;
    }
    if (strcmp(s, "BO") == 0) {
        return GW_BACNET_TYPE_BO;
    }
    fprintf(stderr, "[PT] Unknown bacnet_type '%s', defaulting to AI\n", s);
    return GW_BACNET_TYPE_AI;
}

static GW_MODBUS_FUNC parse_modbus_func(const char *s)
{
    if (!s) {
        return GW_MODBUS_FUNC_INPUT_REG;
    }
    if (strcmp(s, "COIL") == 0) {
        return GW_MODBUS_FUNC_COIL;
    }
    if (strcmp(s, "DISCRETE_INPUT") == 0) {
        return GW_MODBUS_FUNC_DISCRETE_INPUT;
    }
    if (strcmp(s, "HOLDING_REGISTER") == 0) {
        return GW_MODBUS_FUNC_HOLDING_REG;
    }
    if (strcmp(s, "INPUT_REGISTER") == 0) {
        return GW_MODBUS_FUNC_INPUT_REG;
    }
    fprintf(
        stderr, "[PT] Unknown modbus_func '%s', defaulting to INPUT_REGISTER\n",
        s);
    return GW_MODBUS_FUNC_INPUT_REG;
}

static BACNET_ENGINEERING_UNITS parse_units(const char *s)
{
    if (!s) {
        return UNITS_NO_UNITS;
    }
    if (strcmp(s, "DEGREES_CELSIUS") == 0) {
        return UNITS_DEGREES_CELSIUS;
    }
    if (strcmp(s, "DEGREES_FAHRENHEIT") == 0) {
        return UNITS_DEGREES_FAHRENHEIT;
    }
    if (strcmp(s, "PERCENT_RELATIVE_HUMIDITY") == 0) {
        return UNITS_PERCENT_RELATIVE_HUMIDITY;
    }
    if (strcmp(s, "PERCENT") == 0) {
        return UNITS_PERCENT;
    }
    if (strcmp(s, "PASCALS") == 0) {
        return UNITS_PASCALS;
    }
    if (strcmp(s, "KILOPASCALS") == 0) {
        return UNITS_KILOPASCALS;
    }
    if (strcmp(s, "WATTS") == 0) {
        return UNITS_WATTS;
    }
    if (strcmp(s, "KILOWATTS") == 0) {
        return UNITS_KILOWATTS;
    }
    if (strcmp(s, "WATT_HOURS") == 0) {
        return UNITS_WATT_HOURS;
    }
    if (strcmp(s, "KILOWATT_HOURS") == 0) {
        return UNITS_KILOWATT_HOURS;
    }
    if (strcmp(s, "AMPERES") == 0) {
        return UNITS_AMPERES;
    }
    if (strcmp(s, "VOLTS") == 0) {
        return UNITS_VOLTS;
    }
    if (strcmp(s, "HERTZ") == 0) {
        return UNITS_HERTZ;
    }
    if (strcmp(s, "RPM") == 0) {
        return UNITS_REVOLUTIONS_PER_MINUTE;
    }
    if (strcmp(s, "LITERS_PER_SECOND") == 0) {
        return UNITS_LITERS_PER_SECOND;
    }
    if (strcmp(s, "CUBIC_METERS_PER_HOUR") == 0) {
        return UNITS_CUBIC_METERS_PER_HOUR;
    }
    if (strcmp(s, "NO_UNITS") == 0) {
        return UNITS_NO_UNITS;
    }
    fprintf(stderr, "[PT] Unknown units '%s', using NO_UNITS\n", s);
    return UNITS_NO_UNITS;
}

static const char *bacnet_type_str(GW_BACNET_TYPE t)
{
    switch (t) {
        case GW_BACNET_TYPE_AI:
            return "AI";
        case GW_BACNET_TYPE_AO:
            return "AO";
        case GW_BACNET_TYPE_BI:
            return "BI";
        case GW_BACNET_TYPE_BO:
            return "BO";
        default:
            return "??";
    }
}

/* -----------------------------------------------------------------------
 * Read and parse a JSON file into a cJSON tree
 * -----------------------------------------------------------------------*/
static cJSON *load_json_file(const char *filename)
{
    FILE *fp = NULL;
    long len = 0;
    char *buf = NULL;
    cJSON *root = NULL;

    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "[PT] Cannot open '%s'\n", filename);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buf = (char *)malloc((size_t)len + 1);
    if (!buf) {
        fclose(fp);
        return NULL;
    }

    if (fread(buf, 1, (size_t)len, fp) != (size_t)len) {
        fprintf(stderr, "[PT] Failed to read '%s'\n", filename);
        free(buf);
        fclose(fp);
        return NULL;
    }
    buf[len] = '\0';
    fclose(fp);

    root = cJSON_Parse(buf);
    free(buf);

    if (!root) {
        fprintf(
            stderr, "[PT] JSON parse error near: %s\n", cJSON_GetErrorPtr());
    }
    return root;
}

/* -----------------------------------------------------------------------
 * point_table_load_json
 * -----------------------------------------------------------------------*/
bool point_table_load_json(
    GW_CONFIG *cfg, GW_POINT_TABLE *table, const char *filename)
{
    cJSON *root = NULL;
    cJSON *sec = NULL;
    cJSON *points_arr = NULL;
    cJSON *item = NULL;
    cJSON *j = NULL;
    int idx = 0;

    if (!cfg || !table || !filename) {
        return false;
    }

    /* ── Apply compile-time defaults first ── */
    memset(cfg, 0, sizeof(*cfg));
    memset(table, 0, sizeof(*table));

    cfg->device_instance = GW_DEVICE_INSTANCE;
    cfg->mstp_baud = GW_MSTP_BAUD;
    cfg->mstp_mac = GW_MSTP_MAC_ADDRESS;
    cfg->vendor_id = GW_VENDOR_ID;
    cfg->modbus_baud = GW_MODBUS_BAUD;
    cfg->poll_interval_sec = GW_POLL_INTERVAL_SEC;
    cfg->bip_port = GW_BIP_PORT;
    strncpy(cfg->device_name, GW_DEVICE_NAME, sizeof(cfg->device_name) - 1);
    strncpy(
        cfg->datalink_type, GW_DATALINK_TYPE, sizeof(cfg->datalink_type) - 1);
    strncpy(cfg->bacnet_iface, GW_BACNET_IFACE, sizeof(cfg->bacnet_iface) - 1);
    strncpy(cfg->modbus_port, GW_MODBUS_PORT, sizeof(cfg->modbus_port) - 1);

    root = load_json_file(filename);
    if (!root) {
        fprintf(stderr, "[PT] Failed to load JSON file\n");
        return false;
    }

    /* ── bacnet section ── */
    sec = cJSON_GetObjectItem(root, "bacnet");
    if (sec) {
        j = cJSON_GetObjectItem(sec, "device_instance");
        if (cJSON_IsNumber(j)) {
            double val = j->valuedouble;

            if (val >= 1 && val <= BACNET_MAX_INSTANCE) {
                cfg->device_instance = (uint32_t)val;
            } else {
                fprintf(stderr, "Invalid device_instance: %.0f\n", val);
                return false;
            }
        }

        j = cJSON_GetObjectItem(sec, "device_name");
        if (cJSON_IsString(j)) {
            strncpy(
                cfg->device_name, j->valuestring, sizeof(cfg->device_name) - 1);
        }

        j = cJSON_GetObjectItem(sec, "datalink_type");
        if (cJSON_IsString(j)) {
            strncpy(
                cfg->datalink_type, j->valuestring,
                sizeof(cfg->datalink_type) - 1);
        }

        j = cJSON_GetObjectItem(sec, "iface");
        if (cJSON_IsString(j)) {
            strncpy(
                cfg->bacnet_iface, j->valuestring,
                sizeof(cfg->bacnet_iface) - 1);
        }

        j = cJSON_GetObjectItem(sec, "mstp_baud");
        if (cJSON_IsNumber(j)) {
            cfg->mstp_baud = (uint32_t)j->valuedouble;
        }

        j = cJSON_GetObjectItem(sec, "mstp_mac");
        if (cJSON_IsNumber(j)) {
            double mstp_mac = j->valuedouble;
            if ((mstp_mac >= 0.0) && (mstp_mac <= 127.0) &&
                (fabs(floor(mstp_mac) - mstp_mac) < 0.5)) {
                cfg->mstp_mac = (uint8_t)mstp_mac;
            } else {
                fprintf(
                    stderr,
                    "[PT] Invalid mstp_mac '%g'; expected integer range "
                    "0..127. Keeping existing value %u\n",
                    mstp_mac, (unsigned)cfg->mstp_mac);
            }
        }

        j = cJSON_GetObjectItem(sec, "mstp_max_master");
        if (cJSON_IsNumber(j)) {
            cfg->mstp_max_master = (uint16_t)j->valuedouble;
        }

        j = cJSON_GetObjectItem(sec, "mstp_net");
        if (cJSON_IsNumber(j)) {
            cfg->mstp_net = (uint16_t)j->valuedouble;
        }

        j = cJSON_GetObjectItem(sec, "bip_port");
        if (cJSON_IsNumber(j)) {
            cfg->bip_port = (uint16_t)j->valuedouble;
        }

        j = cJSON_GetObjectItem(sec, "vendor_id");
        if (cJSON_IsNumber(j)) {
            cfg->vendor_id = (uint16_t)j->valuedouble;
        }
    }

    /* ── modbus section ── */
    sec = cJSON_GetObjectItem(root, "modbus");
    if (sec) {
        j = cJSON_GetObjectItem(sec, "port");
        if (cJSON_IsString(j)) {
            strncpy(
                cfg->modbus_port, j->valuestring, sizeof(cfg->modbus_port) - 1);
        }
        j = cJSON_GetObjectItem(sec, "baud");
        if (cJSON_IsNumber(j)) {
            cfg->modbus_baud = (uint32_t)j->valuedouble;
        }

        j = cJSON_GetObjectItem(sec, "poll_interval_sec");
        if (cJSON_IsNumber(j)) {
            cfg->poll_interval_sec = (int)j->valuedouble;
        }
    }

    /* ── points array ── */
    points_arr = cJSON_GetObjectItem(root, "points");
    if (!cJSON_IsArray(points_arr)) {
        fprintf(stderr, "[PT] 'points' array not found in JSON\n");
        cJSON_Delete(root);
        return false;
    }

    cJSON_ArrayForEach(item, points_arr)
    {
        GW_POINT *p;

        if (idx >= GW_MAX_POINTS) {
            fprintf(
                stderr, "[PT] Max points (%d) reached, remaining ignored\n",
                GW_MAX_POINTS);
            break;
        }

        /* If enabled is false, skip this point entirely. */
        j = cJSON_GetObjectItem(item, "enabled");
        if (cJSON_IsFalse(j)) {
            j = cJSON_GetObjectItem(item, "name");
            printf(
                "[PT] SKIP (disabled): %s\n",
                cJSON_IsString(j) ? j->valuestring : "(unnamed)");
            continue;
        }

        p = &table->points[idx];
        j = cJSON_GetObjectItem(item, "name");
        if (cJSON_IsString(j)) {
            strncpy(p->name, j->valuestring, sizeof(p->name) - 1);
        } else {
            snprintf(p->name, sizeof(p->name), "Point_%d", idx);
        }

        j = cJSON_GetObjectItem(item, "description");
        if (cJSON_IsString(j)) {
            strncpy(p->description, j->valuestring, sizeof(p->description) - 1);
        }

        j = cJSON_GetObjectItem(item, "bacnet_type");
        p->bacnet_type =
            parse_bacnet_type(cJSON_IsString(j) ? j->valuestring : NULL);

        j = cJSON_GetObjectItem(item, "bacnet_instance");
        p->bacnet_instance =
            cJSON_IsNumber(j) ? (uint32_t)j->valuedouble : (uint32_t)idx;

        j = cJSON_GetObjectItem(item, "units");
        p->units = parse_units(cJSON_IsString(j) ? j->valuestring : NULL);

        j = cJSON_GetObjectItem(item, "modbus_slave");
        if (cJSON_IsNumber(j)) {
            double modbus_slave = j->valuedouble;
            if ((modbus_slave < 1.0) || (modbus_slave > 247.0) ||
                (fabs(floor(modbus_slave) - modbus_slave) > 0.5)) {
                fprintf(
                    stderr,
                    "[PT] Invalid modbus_slave %.17g for point '%s' (index "
                    "%d): "
                    "expected an integer in the range 1..247; rejecting "
                    "point\n",
                    modbus_slave, p->name, idx);
                continue;
            }
            p->modbus_slave = (uint8_t)modbus_slave;
        } else {
            p->modbus_slave = 1;
        }

        j = cJSON_GetObjectItem(item, "modbus_func");
        p->modbus_func =
            parse_modbus_func(cJSON_IsString(j) ? j->valuestring : NULL);

        j = cJSON_GetObjectItem(item, "modbus_address");
        p->modbus_address = cJSON_IsNumber(j) ? (uint16_t)j->valuedouble : 0;

        j = cJSON_GetObjectItem(item, "scale");
        p->scale = cJSON_IsNumber(j) ? (float)j->valuedouble : 1.0f;

        j = cJSON_GetObjectItem(item, "offset");
        p->offset = cJSON_IsNumber(j) ? (float)j->valuedouble : 0.0f;

        j = cJSON_GetObjectItem(item, "range_min");
        p->range_min = cJSON_IsNumber(j) ? (float)j->valuedouble : -1e9f;

        j = cJSON_GetObjectItem(item, "range_max");
        p->range_max = cJSON_IsNumber(j) ? (float)j->valuedouble : 1e9f;

        p->enabled = true; /* enabled: false는 위에서 이미 skip됨 */
        p->initialized = false;
        p->comm_error = true;

        printf(
            "[PT] [%3d] %-20s -> %s:%-4u  slave=%u fc=%u addr=0x%04X"
            "  scale=%.4g offset=%.4g  [%.2g, %.2g]  %s\n",
            idx, p->name, bacnet_type_str(p->bacnet_type), p->bacnet_instance,
            p->modbus_slave, (unsigned)p->modbus_func, p->modbus_address,
            p->scale, p->offset, p->range_min, p->range_max,
            p->enabled ? "ENABLED" : "DISABLED");

        idx++;
    }

    table->count = idx;
    cJSON_Delete(root);

    printf("[PT] Loaded %d point(s) from '%s'\n", table->count, filename);
    return (table->count > 0);
}

/* -----------------------------------------------------------------------
 * point_table_init_bacnet_objects
 * -----------------------------------------------------------------------*/
void point_table_init_bacnet_objects(const GW_POINT_TABLE *table)
{
    int i;
    if (!table) {
        return;
    }

    for (i = 0; i < table->count; i++) {
        const GW_POINT *p = &table->points[i];

        if (!p->enabled) {
            continue;
        }

        switch (p->bacnet_type) {
            case GW_BACNET_TYPE_AI:
                Analog_Input_Create(p->bacnet_instance);
                Analog_Input_Name_Set(p->bacnet_instance, p->name);
                Analog_Input_Description_Set(
                    p->bacnet_instance, p->description);
                Analog_Input_Units_Set(p->bacnet_instance, p->units);
                Analog_Input_Present_Value_Set(p->bacnet_instance, 0.0f);
                Analog_Input_Out_Of_Service_Set(p->bacnet_instance, true);
                break;

            case GW_BACNET_TYPE_AO:
                Analog_Output_Create(p->bacnet_instance);
                Analog_Output_Name_Set(p->bacnet_instance, p->name);
                Analog_Output_Description_Set(
                    p->bacnet_instance, p->description);
                Analog_Output_Units_Set(p->bacnet_instance, p->units);
                break;

            case GW_BACNET_TYPE_BI:
                Binary_Input_Create(p->bacnet_instance);
                Binary_Input_Name_Set(p->bacnet_instance, p->name);
                Binary_Input_Description_Set(
                    p->bacnet_instance, p->description);
                Binary_Input_Out_Of_Service_Set(p->bacnet_instance, true);
                break;

            case GW_BACNET_TYPE_BO:
                Binary_Output_Create(p->bacnet_instance);
                Binary_Output_Name_Set(p->bacnet_instance, p->name);
                Binary_Output_Description_Set(
                    p->bacnet_instance, p->description);
                break;
            default:
                break;
        }
    }
    printf("[BACnet] %d object(s) created\n", table->count);
}

/* -----------------------------------------------------------------------
 * point_table_poll_modbus
 * -----------------------------------------------------------------------*/
void point_table_poll_modbus(GW_POINT_TABLE *table)
{
    int i;
    if (!table) {
        return;
    }

    for (i = 0; i < table->count; i++) {
        GW_POINT *p = &table->points[i];
        uint16_t raw_u16 = 0;
        uint8_t raw_bit = 0;
        float eng_val = 0.0f;
        bool ok = false;
        bool in_range;

        if (!p->enabled) {
            continue;
        }

        /* ── Read from Modbus ── */
        switch (p->modbus_func) {
            case GW_MODBUS_FUNC_COIL:
            case GW_MODBUS_FUNC_DISCRETE_INPUT:
                ok = Modbus_Read_Bit(
                    p->modbus_slave, (uint8_t)p->modbus_func, p->modbus_address,
                    &raw_bit);
                eng_val = (float)raw_bit;
                break;

            case GW_MODBUS_FUNC_HOLDING_REG:
            case GW_MODBUS_FUNC_INPUT_REG:
                ok = Modbus_Read_Register(
                    p->modbus_slave, (uint8_t)p->modbus_func, p->modbus_address,
                    &raw_u16);
                eng_val = (float)raw_u16 * p->scale + p->offset;
                break;
            default:
                break;
        }

        p->comm_error = !ok;

        /* ── Communication failure → Out-of-Service ── */
        if (!ok) {
            if (p->bacnet_type == GW_BACNET_TYPE_AI) {
                Analog_Input_Out_Of_Service_Set(p->bacnet_instance, true);
            } else if (p->bacnet_type == GW_BACNET_TYPE_BI) {
                Binary_Input_Out_Of_Service_Set(p->bacnet_instance, true);
            }
            fprintf(
                stderr, "[PT] Comm error on '%s' (slave=%u addr=0x%04X)\n",
                p->name, p->modbus_slave, p->modbus_address);
            continue;
        }

        /* ── Range check ── */
        in_range = (eng_val >= p->range_min && eng_val <= p->range_max);

        /* ── Update BACnet object ── */
        switch (p->bacnet_type) {
            case GW_BACNET_TYPE_AI:
                if (in_range) {
                    Analog_Input_Present_Value_Set(p->bacnet_instance, eng_val);
                    Analog_Input_Out_Of_Service_Set(p->bacnet_instance, false);
                    printf(
                        "[PT] %-20s = %.4g  (AI:%u)\n", p->name, eng_val,
                        p->bacnet_instance);
                } else {
                    Analog_Input_Out_Of_Service_Set(p->bacnet_instance, true);
                    fprintf(
                        stderr,
                        "[PT] Range error: '%s' = %.4g (min=%.4g max=%.4g)\n",
                        p->name, eng_val, p->range_min, p->range_max);
                }
                break;

            case GW_BACNET_TYPE_BI:
                if (in_range) {
                    Binary_Input_Present_Value_Set(
                        p->bacnet_instance,
                        (fabsf(eng_val) > 0.5f) ? BINARY_ACTIVE
                                                : BINARY_INACTIVE);
                    Binary_Input_Out_Of_Service_Set(p->bacnet_instance, false);
                    printf(
                        "[PT] %-20s = %s  (BI:%u)\n", p->name,
                        (fabsf(eng_val) > 0.5f) ? "ACTIVE" : "INACTIVE",
                        p->bacnet_instance);
                } else {
                    Binary_Input_Out_Of_Service_Set(p->bacnet_instance, true);
                    fprintf(
                        stderr,
                        "[PT] Range error: '%s' = %.4g (min=%.4g max=%.4g)\n",
                        p->name, eng_val, p->range_min, p->range_max);
                }
                break;

            case GW_BACNET_TYPE_AO:
                if (in_range) {
                    Analog_Output_Present_Value_Set(
                        p->bacnet_instance, eng_val, BACNET_NO_PRIORITY);
                    Analog_Output_Out_Of_Service_Set(p->bacnet_instance, false);
                    printf(
                        "[PT] %-20s = %.4g  (AO:%u)\n", p->name, eng_val,
                        p->bacnet_instance);
                } else {
                    Analog_Output_Out_Of_Service_Set(p->bacnet_instance, true);
                    fprintf(
                        stderr,
                        "[PT] Range error: '%s' = %.4g (min=%.4g max=%.4g)\n",
                        p->name, eng_val, p->range_min, p->range_max);
                }
                break;
            case GW_BACNET_TYPE_BO:
                if (in_range) {
                    Binary_Output_Present_Value_Set(
                        p->bacnet_instance,
                        (fabsf(eng_val) > 0.5f) ? BINARY_ACTIVE
                                                : BINARY_INACTIVE,
                        BACNET_NO_PRIORITY);
                    Binary_Output_Out_Of_Service_Set(p->bacnet_instance, false);
                    printf(
                        "[PT] %-20s = %s  (BO:%u)\n", p->name,
                        (fabsf(eng_val) > 0.5f) ? "ACTIVE" : "INACTIVE",
                        p->bacnet_instance);
                } else {
                    Binary_Output_Out_Of_Service_Set(p->bacnet_instance, true);
                    fprintf(
                        stderr,
                        "[PT] Range error: '%s' = %.4g (min=%.4g max=%.4g)\n",
                        p->name, eng_val, p->range_min, p->range_max);
                }
                break;
            default:
                break;
        }

        p->initialized = true;
    }
}
