/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @brief A basic BACnet Averaging object implementation.
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/bacdcode.h"
#include "bacnet/datetime.h"
#include "bacnet/proplist.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
/* BACnet Stack Objects */
#include "bacnet/basic/object/device.h"
/* me! */
#include "bacnet/basic/object/averaging.h"

#ifndef BACNET_AVERAGING_SAMPLES_MAX
#define BACNET_AVERAGING_SAMPLES_MAX 64
#endif

struct averaging_sample {
    bool Valid : 1;
    float Value;
    BACNET_DATE_TIME Timestamp;
};

struct object_data {
    const char *Object_Name;
    const char *Description;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE Object_Property_Reference;
    uint32_t Window_Interval;
    uint32_t Window_Samples;
    uint32_t Attempted_Samples;
    uint32_t Valid_Samples;
    uint32_t Sample_Next;
    float Minimum_Value;
    float Average_Value;
    float Variance_Value;
    float Maximum_Value;
    BACNET_DATE_TIME Minimum_Value_Timestamp;
    BACNET_DATE_TIME Maximum_Value_Timestamp;
    struct averaging_sample Samples[BACNET_AVERAGING_SAMPLES_MAX];
    void *Context;
};

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_Lists[MAX_NUM_DEVICES];
#ifdef BAC_ROUTING
#define Object_List (Object_Lists[Routed_Device_Object_Index()])
#else
#define Object_List (Object_Lists[0])
#endif
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_AVERAGING;

static const int32_t Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME,    PROP_OBJECT_TYPE,
    PROP_MINIMUM_VALUE,     PROP_AVERAGE_VALUE,  PROP_MAXIMUM_VALUE,
    PROP_ATTEMPTED_SAMPLES, PROP_VALID_SAMPLES,  PROP_OBJECT_PROPERTY_REFERENCE,
    PROP_WINDOW_INTERVAL,   PROP_WINDOW_SAMPLES, -1
};

static const int32_t Properties_Optional[] = { PROP_MINIMUM_VALUE_TIMESTAMP,
                                               PROP_VARIANCE_VALUE,
                                               PROP_MAXIMUM_VALUE_TIMESTAMP,
                                               PROP_DESCRIPTION, -1 };

static const int32_t Properties_Proprietary[] = { -1 };

static const int32_t Writable_Properties[] = { PROP_ATTEMPTED_SAMPLES,
                                               PROP_OBJECT_PROPERTY_REFERENCE,
                                               PROP_WINDOW_INTERVAL,
                                               PROP_WINDOW_SAMPLES, -1 };

/**
 * @brief Get object data by instance.
 * @param object_instance BACnet object instance.
 * @return Pointer to object data, or NULL if not found.
 */
static struct object_data *Averaging_Object(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

/**
 * @brief Reset averaging runtime state and sample buffer.
 * @param pObject Pointer to object data.
 */
static void Averaging_Reset_Object(struct object_data *pObject)
{
    unsigned i;

    if (!pObject) {
        return;
    }

    pObject->Attempted_Samples = 0;
    pObject->Valid_Samples = 0;
    pObject->Sample_Next = 0;
    pObject->Minimum_Value = INFINITY;
    pObject->Average_Value = NAN;
    pObject->Variance_Value = NAN;
    pObject->Maximum_Value = -INFINITY;
    datetime_wildcard_set(&pObject->Minimum_Value_Timestamp);
    datetime_wildcard_set(&pObject->Maximum_Value_Timestamp);
    for (i = 0; i < BACNET_AVERAGING_SAMPLES_MAX; i++) {
        pObject->Samples[i].Valid = false;
        pObject->Samples[i].Value = 0.0f;
        datetime_wildcard_set(&pObject->Samples[i].Timestamp);
    }
}

/**
 * @brief Acquire local timestamp or wildcard if unavailable.
 * @param bdatetime Output BACnet date/time.
 */
static void Averaging_Timestamp_Local(BACNET_DATE_TIME *bdatetime)
{
    BACNET_DATE bdate = { 0 };
    BACNET_TIME btime = { 0 };

    if (!bdatetime) {
        return;
    }
    if (datetime_local(&bdate, &btime, NULL, NULL)) {
        bdatetime->date = bdate;
        bdatetime->time = btime;
    } else {
        datetime_wildcard_set(bdatetime);
    }
}

/**
 * @brief Recalculate min/max/average/variance from valid samples.
 * @param pObject Pointer to object data.
 */
static void Averaging_Recalculate(struct object_data *pObject)
{
    uint32_t i;
    uint32_t valid = 0;
    float sum = 0.0f;
    float sum_square = 0.0f;
    float min_value = INFINITY;
    float max_value = -INFINITY;
    uint32_t index;

    if (!pObject) {
        return;
    }

    datetime_wildcard_set(&pObject->Minimum_Value_Timestamp);
    datetime_wildcard_set(&pObject->Maximum_Value_Timestamp);

    for (i = 0; i < pObject->Window_Samples; i++) {
        index = i;
        if (!pObject->Samples[index].Valid) {
            continue;
        }
        valid++;
        sum += pObject->Samples[index].Value;
        sum_square +=
            pObject->Samples[index].Value * pObject->Samples[index].Value;
        if (pObject->Samples[index].Value < min_value) {
            min_value = pObject->Samples[index].Value;
            pObject->Minimum_Value_Timestamp =
                pObject->Samples[index].Timestamp;
        }
        if (pObject->Samples[index].Value > max_value) {
            max_value = pObject->Samples[index].Value;
            pObject->Maximum_Value_Timestamp =
                pObject->Samples[index].Timestamp;
        }
    }

    pObject->Valid_Samples = valid;
    if (valid == 0) {
        pObject->Minimum_Value = INFINITY;
        pObject->Average_Value = NAN;
        pObject->Variance_Value = NAN;
        pObject->Maximum_Value = -INFINITY;
    } else {
        pObject->Minimum_Value = min_value;
        pObject->Maximum_Value = max_value;
        pObject->Average_Value = sum / (float)valid;
        pObject->Variance_Value = (sum_square / (float)valid) -
            (pObject->Average_Value * pObject->Average_Value);
        if (pObject->Variance_Value < 0.0f) {
            pObject->Variance_Value = 0.0f;
        }
    }
}

/**
 * @brief Record a sample slot as valid or missed and recompute metrics.
 * @param object_instance BACnet object instance.
 * @param sample_valid True when sample is valid.
 * @param sample_value Sample value when valid.
 * @return True if sample operation succeeded.
 */
static bool Averaging_Sample_Record(
    uint32_t object_instance, bool sample_valid, float sample_value)
{
    struct object_data *pObject;
    uint32_t slot;

    pObject = Averaging_Object(object_instance);
    if (!pObject) {
        return false;
    }

    if (pObject->Window_Samples == 0) {
        return false;
    }

    slot = pObject->Sample_Next;
    if (sample_valid) {
        pObject->Samples[slot].Valid = true;
        pObject->Samples[slot].Value = sample_value;
        Averaging_Timestamp_Local(&pObject->Samples[slot].Timestamp);
    } else {
        pObject->Samples[slot].Valid = false;
        pObject->Samples[slot].Value = 0.0f;
        datetime_wildcard_set(&pObject->Samples[slot].Timestamp);
    }

    pObject->Sample_Next++;
    if (pObject->Sample_Next >= pObject->Window_Samples) {
        pObject->Sample_Next = 0;
    }

    if (pObject->Attempted_Samples < pObject->Window_Samples) {
        pObject->Attempted_Samples++;
    }

    Averaging_Recalculate(pObject);

    return true;
}

/**
 * @brief Return required/optional/proprietary property lists.
 * @param pRequired Output required properties.
 * @param pOptional Output optional properties.
 * @param pProprietary Output proprietary properties.
 */
void Averaging_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary)
{
    if (pRequired) {
        *pRequired = Properties_Required;
    }
    if (pOptional) {
        *pOptional = Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Properties_Proprietary;
    }
}

/**
 * @brief Return always-writable property list.
 * @param object_instance BACnet object instance (unused).
 * @param properties Output writable property list.
 */
void Averaging_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties)
{
    (void)object_instance;
    if (properties) {
        *properties = Writable_Properties;
    }
}

/**
 * @brief Check whether instance exists.
 * @param object_instance BACnet object instance.
 * @return True if instance exists.
 */
bool Averaging_Valid_Instance(uint32_t object_instance)
{
    return (Averaging_Object(object_instance) != NULL);
}

/**
 * @brief Get number of Averaging objects.
 * @return Object count.
 */
unsigned Averaging_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * @brief Convert object index to instance number.
 * @param index Zero-based object index.
 * @return BACnet object instance.
 */
uint32_t Averaging_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * @brief Convert object instance to zero-based index.
 * @param object_instance BACnet object instance.
 * @return Object index, or out-of-range value if invalid.
 */
unsigned Averaging_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * @brief Encode object name into BACnet character string.
 * @param object_instance BACnet object instance.
 * @param object_name Output BACnet character string.
 * @return True if encoded.
 */
bool Averaging_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char name_text[32] = "";
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (!pObject) {
        return false;
    }
    if (pObject->Object_Name) {
        return characterstring_init_ansi(object_name, pObject->Object_Name);
    }
    snprintf(
        name_text, sizeof(name_text), "AVERAGING %lu",
        (unsigned long)object_instance);
    return characterstring_init_ansi(object_name, name_text);
}

/**
 * @brief Set object name pointer.
 * @param object_instance BACnet object instance.
 * @param new_name New ASCII object name pointer.
 * @return True if updated.
 */
bool Averaging_Name_Set(uint32_t object_instance, const char *new_name)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (!pObject) {
        return false;
    }
    pObject->Object_Name = new_name;

    return true;
}

/**
 * @brief Get ASCII object name pointer.
 * @param object_instance BACnet object instance.
 * @return Name pointer or NULL.
 */
const char *Averaging_Name_ASCII(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (pObject) {
        return pObject->Object_Name;
    }

    return NULL;
}

/**
 * @brief Get minimum value.
 * @param object_instance BACnet object instance.
 * @return Minimum value or +INF when unavailable.
 */
float Averaging_Minimum_Value(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (pObject) {
        return pObject->Minimum_Value;
    }

    return INFINITY;
}

/**
 * @brief Get average value.
 * @param object_instance BACnet object instance.
 * @return Average value or NaN when unavailable.
 */
float Averaging_Average_Value(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (pObject) {
        return pObject->Average_Value;
    }

    return NAN;
}

/**
 * @brief Get variance value.
 * @param object_instance BACnet object instance.
 * @return Variance value or NaN when unavailable.
 */
float Averaging_Variance_Value(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (pObject) {
        return pObject->Variance_Value;
    }

    return NAN;
}

/**
 * @brief Get maximum value.
 * @param object_instance BACnet object instance.
 * @return Maximum value or -INF when unavailable.
 */
float Averaging_Maximum_Value(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (pObject) {
        return pObject->Maximum_Value;
    }

    return -INFINITY;
}

/**
 * @brief Get attempted sample count.
 * @param object_instance BACnet object instance.
 * @return Attempted samples count.
 */
uint32_t Averaging_Attempted_Samples(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (pObject) {
        return pObject->Attempted_Samples;
    }

    return 0;
}

/**
 * @brief Get valid sample count.
 * @param object_instance BACnet object instance.
 * @return Valid samples count.
 */
uint32_t Averaging_Valid_Samples(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (pObject) {
        return pObject->Valid_Samples;
    }

    return 0;
}

/**
 * @brief Get window interval in seconds.
 * @param object_instance BACnet object instance.
 * @return Window interval.
 */
uint32_t Averaging_Window_Interval(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (pObject) {
        return pObject->Window_Interval;
    }

    return 0;
}

/**
 * @brief Get configured window sample count.
 * @param object_instance BACnet object instance.
 * @return Window samples.
 */
uint32_t Averaging_Window_Samples(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (pObject) {
        return pObject->Window_Samples;
    }

    return 0;
}

/**
 * @brief Get object-property-reference setting.
 * @param object_instance BACnet object instance.
 * @param value Output reference structure.
 * @return True if copied.
 */
bool Averaging_Object_Property_Reference(
    uint32_t object_instance, BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (!pObject || !value) {
        return false;
    }

    return bacnet_device_object_property_reference_copy(
        value, &pObject->Object_Property_Reference);
}

/**
 * @brief Set object-property-reference and reset sample state.
 * @param object_instance BACnet object instance.
 * @param value Input reference structure.
 * @return True if updated.
 */
bool Averaging_Object_Property_Reference_Set(
    uint32_t object_instance,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (!pObject || !value) {
        return false;
    }

    if (!bacnet_device_object_property_reference_copy(
            &pObject->Object_Property_Reference, value)) {
        return false;
    }

    Averaging_Reset_Object(pObject);

    return true;
}

/**
 * @brief Add valid sample.
 * @param object_instance BACnet object instance.
 * @param sample_value Sample value.
 * @return True if recorded.
 */
bool Averaging_Sample_Add(uint32_t object_instance, float sample_value)
{
    return Averaging_Sample_Record(object_instance, true, sample_value);
}

/**
 * @brief Record missed sample.
 * @param object_instance BACnet object instance.
 * @return True if recorded.
 */
bool Averaging_Sample_Miss(uint32_t object_instance)
{
    return Averaging_Sample_Record(object_instance, false, 0.0f);
}

/**
 * @brief Reset sample state and computed values.
 * @param object_instance BACnet object instance.
 * @return True if reset.
 */
bool Averaging_Reset(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (!pObject) {
        return false;
    }

    Averaging_Reset_Object(pObject);

    return true;
}

/**
 * @brief Get description pointer.
 * @param object_instance BACnet object instance.
 * @return Description pointer or NULL.
 */
const char *Averaging_Description(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (pObject) {
        return pObject->Description;
    }

    return NULL;
}

/**
 * @brief Set description pointer.
 * @param object_instance BACnet object instance.
 * @param new_name Description pointer.
 * @return True if updated.
 */
bool Averaging_Description_Set(uint32_t object_instance, const char *new_name)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (!pObject) {
        return false;
    }

    pObject->Description = new_name;

    return true;
}

/**
 * @brief Get user context pointer.
 * @param object_instance BACnet object instance.
 * @return Context pointer or NULL.
 */
void *Averaging_Context_Get(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (pObject) {
        return pObject->Context;
    }

    return NULL;
}

/**
 * @brief Set user context pointer.
 * @param object_instance BACnet object instance.
 * @param context Context pointer.
 */
void Averaging_Context_Set(uint32_t object_instance, void *context)
{
    struct object_data *pObject;

    pObject = Averaging_Object(object_instance);
    if (pObject) {
        pObject->Context = context;
    }
}

/**
 * @brief ReadProperty handler for Averaging object.
 * @param rpdata ReadProperty request/response payload.
 * @return APDU length or BACNET_STATUS_ERROR.
 */
int Averaging_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0;
    BACNET_CHARACTER_STRING char_string;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE value = { 0 };
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    if (!property_lists_member(
            Properties_Required, Properties_Optional, Properties_Proprietary,
            rpdata->object_property)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
        return BACNET_STATUS_ERROR;
    }

    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                apdu, Object_Type, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            if (Averaging_Object_Name(rpdata->object_instance, &char_string)) {
                apdu_len =
                    encode_application_character_string(apdu, &char_string);
            }
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(apdu, Object_Type);
            break;
        case PROP_MINIMUM_VALUE:
            apdu_len = encode_application_real(
                apdu, Averaging_Minimum_Value(rpdata->object_instance));
            break;
        case PROP_MINIMUM_VALUE_TIMESTAMP:
            if (Averaging_Object(rpdata->object_instance)) {
                apdu_len = bacapp_encode_datetime(
                    apdu,
                    &Averaging_Object(rpdata->object_instance)
                         ->Minimum_Value_Timestamp);
            }
            break;
        case PROP_AVERAGE_VALUE:
            apdu_len = encode_application_real(
                apdu, Averaging_Average_Value(rpdata->object_instance));
            break;
        case PROP_VARIANCE_VALUE:
            apdu_len = encode_application_real(
                apdu, Averaging_Variance_Value(rpdata->object_instance));
            break;
        case PROP_MAXIMUM_VALUE:
            apdu_len = encode_application_real(
                apdu, Averaging_Maximum_Value(rpdata->object_instance));
            break;
        case PROP_MAXIMUM_VALUE_TIMESTAMP:
            if (Averaging_Object(rpdata->object_instance)) {
                apdu_len = bacapp_encode_datetime(
                    apdu,
                    &Averaging_Object(rpdata->object_instance)
                         ->Maximum_Value_Timestamp);
            }
            break;
        case PROP_DESCRIPTION:
            if (Averaging_Description(rpdata->object_instance)) {
                characterstring_init_ansi(
                    &char_string,
                    Averaging_Description(rpdata->object_instance));
                apdu_len =
                    encode_application_character_string(apdu, &char_string);
            }
            break;
        case PROP_ATTEMPTED_SAMPLES:
            apdu_len = encode_application_unsigned(
                apdu, Averaging_Attempted_Samples(rpdata->object_instance));
            break;
        case PROP_VALID_SAMPLES:
            apdu_len = encode_application_unsigned(
                apdu, Averaging_Valid_Samples(rpdata->object_instance));
            break;
        case PROP_OBJECT_PROPERTY_REFERENCE:
            if (Averaging_Object_Property_Reference(
                    rpdata->object_instance, &value)) {
                apdu_len = bacapp_encode_device_obj_property_ref(apdu, &value);
            }
            break;
        case PROP_WINDOW_INTERVAL:
            apdu_len = encode_application_unsigned(
                apdu, Averaging_Window_Interval(rpdata->object_instance));
            break;
        case PROP_WINDOW_SAMPLES:
            apdu_len = encode_application_unsigned(
                apdu, Averaging_Window_Samples(rpdata->object_instance));
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    if ((apdu_len >= 0) && (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief WriteProperty handler for Averaging object.
 * @param wp_data WriteProperty request payload.
 * @return True on successful write.
 */
bool Averaging_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE object_property_ref = { 0 };

    if (wp_data == NULL) {
        return false;
    }

    if (!Averaging_Valid_Instance(wp_data->object_instance)) {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }

    if (wp_data->array_index != BACNET_ARRAY_ALL) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }

    switch (wp_data->object_property) {
        case PROP_OBJECT_PROPERTY_REFERENCE:
            len = bacnet_device_object_property_reference_decode(
                wp_data->application_data, wp_data->application_data_len,
                &object_property_ref);
            if (len > 0) {
                status = Averaging_Object_Property_Reference_Set(
                    wp_data->object_instance, &object_property_ref);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
                status = false;
            }
            break;
        case PROP_ATTEMPTED_SAMPLES:
        case PROP_WINDOW_INTERVAL:
        case PROP_WINDOW_SAMPLES:
            len = bacapp_decode_known_array_property(
                wp_data->application_data, wp_data->application_data_len,
                &value, wp_data->object_type, wp_data->object_property,
                wp_data->array_index);
            if (len <= 0) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
                return false;
            }
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (!status) {
                return false;
            }
            switch (wp_data->object_property) {
                case PROP_ATTEMPTED_SAMPLES:
                    if (value.type.Unsigned_Int == 0U) {
                        status = Averaging_Reset(wp_data->object_instance);
                        if (!status) {
                            wp_data->error_class = ERROR_CLASS_PROPERTY;
                            wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                        }
                    } else {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                        status = false;
                    }
                    break;
                case PROP_WINDOW_INTERVAL:
                    if (value.type.Unsigned_Int > 0U) {
                        Averaging_Object(wp_data->object_instance)
                            ->Window_Interval = value.type.Unsigned_Int;
                        status = Averaging_Reset(wp_data->object_instance);
                    } else {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                        status = false;
                    }
                    break;
                case PROP_WINDOW_SAMPLES:
                    if ((value.type.Unsigned_Int > 0U) &&
                        (value.type.Unsigned_Int <=
                         BACNET_AVERAGING_SAMPLES_MAX)) {
                        Averaging_Object(wp_data->object_instance)
                            ->Window_Samples = value.type.Unsigned_Int;
                        status = Averaging_Reset(wp_data->object_instance);
                    } else {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                        status = false;
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            if (property_lists_member(
                    Properties_Required, Properties_Optional,
                    Properties_Proprietary, wp_data->object_property)) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            }
            status = false;
            break;
    }

    return status;
}

/**
 * @brief Create Averaging object instance.
 * @param object_instance Requested instance or BACNET_MAX_INSTANCE wildcard.
 * @return Created instance or BACNET_MAX_INSTANCE on failure.
 */
uint32_t Averaging_Create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
    int index = 0;

    if (!Object_List) {
        Object_List = Keylist_Create();
    }
    if (object_instance > BACNET_MAX_INSTANCE) {
        return BACNET_MAX_INSTANCE;
    } else if (object_instance == BACNET_MAX_INSTANCE) {
        object_instance = Keylist_Next_Empty_Key(Object_List, 1);
    }

    pObject = Averaging_Object(object_instance);
    if (!pObject) {
        pObject = calloc(1, sizeof(struct object_data));
        if (!pObject) {
            return BACNET_MAX_INSTANCE;
        }
        pObject->Object_Name = NULL;
        pObject->Description = NULL;
        pObject->Window_Interval = 60;
        pObject->Window_Samples = 15;
        pObject->Object_Property_Reference.objectIdentifier.type =
            OBJECT_ANALOG_VALUE;
        pObject->Object_Property_Reference.objectIdentifier.instance = 0;
        pObject->Object_Property_Reference.propertyIdentifier =
            PROP_PRESENT_VALUE;
        pObject->Object_Property_Reference.arrayIndex = BACNET_ARRAY_ALL;
        pObject->Object_Property_Reference.deviceIdentifier.type =
            BACNET_NO_DEV_TYPE;
        pObject->Object_Property_Reference.deviceIdentifier.instance =
            BACNET_NO_DEV_ID;
        Averaging_Reset_Object(pObject);

        index = Keylist_Data_Add(Object_List, object_instance, pObject);
        if (index < 0) {
            free(pObject);
            return BACNET_MAX_INSTANCE;
        }
    }

    return object_instance;
}

/**
 * @brief Delete Averaging object instance.
 * @param object_instance BACnet object instance.
 * @return True if deleted.
 */
bool Averaging_Delete(uint32_t object_instance)
{
    struct object_data *pObject = NULL;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        free(pObject);
        return true;
    }

    return false;
}

/**
 * @brief Cleanup all Averaging object instances for all routed devices.
 */
void Averaging_Cleanup(void)
{
    struct object_data *pObject;
    uint16_t dev_id;
#ifdef BAC_ROUTING
    uint16_t current_dev_id = Routed_Device_Object_Index();
#endif

    for (dev_id = 0; dev_id < MAX_NUM_DEVICES; dev_id++) {
#ifdef BAC_ROUTING
        Set_Routed_Device_Object_Index(dev_id);
#endif
        if (Object_List) {
            do {
                pObject = Keylist_Data_Pop(Object_List);
                if (pObject) {
                    free(pObject);
                }
            } while (pObject);
            Keylist_Delete(Object_List);
            Object_List = NULL;
        }
    }

#ifdef BAC_ROUTING
    Set_Routed_Device_Object_Index(current_dev_id);
#endif
}

/**
 * @brief Initialize Averaging object list for all routed devices.
 */
void Averaging_Init(void)
{
    uint16_t dev_id;
#ifdef BAC_ROUTING
    uint16_t current_dev_id = Routed_Device_Object_Index();
#endif

    for (dev_id = 0; dev_id < MAX_NUM_DEVICES; dev_id++) {
#ifdef BAC_ROUTING
        Set_Routed_Device_Object_Index(dev_id);
#endif
        if (!Object_List) {
            Object_List = Keylist_Create();
        }
    }

#ifdef BAC_ROUTING
    Set_Routed_Device_Object_Index(current_dev_id);
#endif
}
