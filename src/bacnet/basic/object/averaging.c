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

struct object_data {
    const char *Object_Name;
    const char *Description;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE Object_Property_Reference;
    uint32_t Window_Interval;
    uint32_t Window_Samples;
    uint32_t Sample_Timer_Milliseconds;
    uint32_t Attempted_Samples;
    uint32_t Valid_Samples;
    float Minimum_Value;
    float Average_Value;
    float Variance_Value;
    double Variance_M2;
    float Maximum_Value;
    BACNET_DATE_TIME Minimum_Value_Timestamp;
    BACNET_DATE_TIME Maximum_Value_Timestamp;
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
/* handling for read reference properties */
static read_property_function Read_Property_Internal_Callback;
static uint8_t Read_Property_Buffer[MAX_APDU];

static const int32_t Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME,    PROP_OBJECT_TYPE,
    PROP_MINIMUM_VALUE,     PROP_AVERAGE_VALUE,  PROP_MAXIMUM_VALUE,
    PROP_ATTEMPTED_SAMPLES, PROP_VALID_SAMPLES,  PROP_OBJECT_PROPERTY_REFERENCE,
    PROP_WINDOW_INTERVAL,   PROP_WINDOW_SAMPLES, -1
};

static const int32_t Properties_Optional[] = {
    /* unordered list of optional properties */
    PROP_MINIMUM_VALUE_TIMESTAMP, PROP_VARIANCE_VALUE,
    PROP_MAXIMUM_VALUE_TIMESTAMP, PROP_DESCRIPTION, -1
};

static const int32_t Properties_Proprietary[] = { -1 };

static const int32_t Writable_Properties[] = {
    /* unordered list of always writable properties */
    PROP_ATTEMPTED_SAMPLES, PROP_OBJECT_PROPERTY_REFERENCE,
    PROP_WINDOW_INTERVAL, PROP_WINDOW_SAMPLES, -1
};

/**
 * @brief Perform weighted moving average update.
 *
 * AN+1 = (XN+1 + N * AN)/(N+1)
 *
 * @param latest_reading Latest reading value.
 * @param previous_average Previous average value.
 * @param nsamples Number of prior samples used for previous average.
 * @return Updated average value.
 */
static float average_weighted_moving(
    float latest_reading, float previous_average, uint32_t nsamples)
{
    double average;

    average = (double)previous_average;
    average *= (double)nsamples;
    average += (double)latest_reading;
    average /= ((double)nsamples + 1.0);

    return (float)average;
}

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
    if (!pObject) {
        return;
    }

    pObject->Sample_Timer_Milliseconds = 0;
    pObject->Attempted_Samples = 0;
    pObject->Valid_Samples = 0;
    pObject->Minimum_Value = INFINITY;
    pObject->Average_Value = NAN;
    pObject->Variance_Value = NAN;
    pObject->Variance_M2 = 0.0;
    pObject->Maximum_Value = -INFINITY;
    datetime_wildcard_set(&pObject->Minimum_Value_Timestamp);
    datetime_wildcard_set(&pObject->Maximum_Value_Timestamp);
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
 * @brief Record sample event and update weighted average statistics.
 * @param object_instance BACnet object instance.
 * @param sample_valid True when sample is valid.
 * @param sample_value Sample value when valid.
 * @return True if sample operation succeeded.
 */
static bool Averaging_Sample_Record(
    uint32_t object_instance, bool sample_valid, float sample_value)
{
    struct object_data *pObject;
    BACNET_DATE_TIME timestamp = { 0 };
    uint32_t valid_samples;
    uint32_t weighting_samples;
    float previous_average;
    double delta, delta2;

    pObject = Averaging_Object(object_instance);
    if (!pObject) {
        return false;
    }
    if (pObject->Attempted_Samples < UINT32_MAX) {
        pObject->Attempted_Samples++;
    }
    if (!sample_valid) {
        return true;
    }
    Averaging_Timestamp_Local(&timestamp);
    /* keep the number of samples within the window */
    valid_samples = pObject->Valid_Samples;
    weighting_samples = valid_samples;
    if (pObject->Window_Samples > 0U) {
        if (weighting_samples >= pObject->Window_Samples) {
            weighting_samples = pObject->Window_Samples - 1U;
        }
    }
    previous_average = pObject->Average_Value;
    if (valid_samples == 0U) {
        pObject->Average_Value = sample_value;
        pObject->Variance_M2 = 0.0;
        pObject->Variance_Value = 0.0f;
        pObject->Minimum_Value = sample_value;
        pObject->Maximum_Value = sample_value;
        pObject->Minimum_Value_Timestamp = timestamp;
        pObject->Maximum_Value_Timestamp = timestamp;
    } else {
        pObject->Average_Value = average_weighted_moving(
            sample_value, previous_average, weighting_samples);
        delta = (double)sample_value - (double)previous_average;
        delta2 = (double)sample_value - (double)pObject->Average_Value;
        pObject->Variance_M2 += delta * delta2;
        pObject->Variance_Value =
            (float)(pObject->Variance_M2 / (double)(weighting_samples + 1U));
        if (sample_value < pObject->Minimum_Value) {
            pObject->Minimum_Value = sample_value;
            pObject->Minimum_Value_Timestamp = timestamp;
        }
        if (sample_value > pObject->Maximum_Value) {
            pObject->Maximum_Value = sample_value;
            pObject->Maximum_Value_Timestamp = timestamp;
        }
    }
    if (pObject->Valid_Samples < UINT32_MAX) {
        pObject->Valid_Samples++;
    }

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
 * @brief Sets callback used when Averaging reads referenced property value.
 * @param cb Callback used to read referenced properties.
 */
void Averaging_Read_Property_Internal_Callback_Set(read_property_function cb)
{
    Read_Property_Internal_Callback = cb;
}

/**
 * @brief Reads object-property-reference value and converts to Real sample.
 * @param pObject Averaging object data.
 * @param sample_value Output sample value.
 * @return True if sample value read and converted.
 */
static bool Averaging_Object_Property_Reference_Read(
    struct object_data *pObject, float *sample_value)
{
    BACNET_READ_PROPERTY_DATA rp_data = { 0 };
    BACNET_TAG tag = { 0 };
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *reference;
    int apdu_len = 0;
    int tag_len = 0;
    int len = 0;
    uint32_t payload_len = 0;
    const uint8_t *payload = NULL;
    bool boolean_value = false;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    int32_t signed_value = 0;
    uint32_t enumerated_value = 0;
    float real_value = 0.0f;
    double double_value = 0.0;

    if (!pObject || !sample_value || !Read_Property_Internal_Callback) {
        return false;
    }
    reference = &pObject->Object_Property_Reference;
    if ((reference->deviceIdentifier.type != BACNET_NO_DEV_TYPE) &&
        (reference->deviceIdentifier.type != OBJECT_DEVICE)) {
        return false;
    }
    if ((reference->deviceIdentifier.type == OBJECT_DEVICE) &&
        (reference->deviceIdentifier.instance != BACNET_NO_DEV_ID) &&
        (reference->deviceIdentifier.instance !=
         Device_Object_Instance_Number())) {
        return false;
    }

    rp_data.object_type = reference->objectIdentifier.type;
    rp_data.object_instance = reference->objectIdentifier.instance;
    rp_data.object_property = reference->propertyIdentifier;
    rp_data.array_index = reference->arrayIndex;
    rp_data.application_data = Read_Property_Buffer;
    rp_data.application_data_len = sizeof(Read_Property_Buffer);
    rp_data.error_class = ERROR_CLASS_PROPERTY;
    rp_data.error_code = ERROR_CODE_UNKNOWN_PROPERTY;
    apdu_len = Read_Property_Internal_Callback(&rp_data);
    if (apdu_len <= 0) {
        return false;
    }
    tag_len = bacnet_tag_decode(Read_Property_Buffer, (uint32_t)apdu_len, &tag);
    if ((tag_len <= 0) || !tag.application) {
        return false;
    }
    payload = &Read_Property_Buffer[tag_len];
    payload_len = (uint32_t)apdu_len - (uint32_t)tag_len;
    switch (tag.number) {
        case BACNET_APPLICATION_TAG_BOOLEAN:
            boolean_value = decode_boolean(tag.len_value_type);
            *sample_value = boolean_value ? 1.0f : 0.0f;
            return true;
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            len = bacnet_unsigned_decode(
                payload, payload_len, tag.len_value_type, &unsigned_value);
            if (len <= 0) {
                return false;
            }
            *sample_value = (float)unsigned_value;
            return true;
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            len = bacnet_signed_decode(
                payload, payload_len, tag.len_value_type, &signed_value);
            if (len <= 0) {
                return false;
            }
            *sample_value = (float)signed_value;
            return true;
        case BACNET_APPLICATION_TAG_ENUMERATED:
            len = bacnet_enumerated_decode(
                payload, payload_len, tag.len_value_type, &enumerated_value);
            if (len <= 0) {
                return false;
            }
            *sample_value = (float)enumerated_value;
            return true;
        case BACNET_APPLICATION_TAG_REAL:
            len = bacnet_real_decode(
                payload, payload_len, tag.len_value_type, &real_value);
            if (len <= 0) {
                return false;
            }
            *sample_value = real_value;
            return true;
        case BACNET_APPLICATION_TAG_DOUBLE:
            len = bacnet_double_decode(
                payload, payload_len, tag.len_value_type, &double_value);
            if (len <= 0) {
                return false;
            }
            *sample_value = (float)double_value;
            return true;
        default:
            break;
    }

    return false;
}

/**
 * @brief Updates Averaging sample state using time-based sampling period.
 * @param object_instance BACnet object instance.
 * @param milliseconds Elapsed milliseconds since last update.
 */
void Averaging_Timer(uint32_t object_instance, uint16_t milliseconds)
{
    struct object_data *pObject;
    uint64_t sample_interval_milliseconds;
    uint64_t sample_timer_milliseconds;
    float sample_value = 0.0f;

    pObject = Averaging_Object(object_instance);
    if (!pObject) {
        return;
    }
    if ((pObject->Window_Interval == 0U) || (pObject->Window_Samples == 0U)) {
        return;
    }

    sample_interval_milliseconds =
        ((uint64_t)pObject->Window_Interval * 1000ULL) /
        (uint64_t)pObject->Window_Samples;
    if (sample_interval_milliseconds == 0ULL) {
        sample_interval_milliseconds = 1ULL;
    }

    sample_timer_milliseconds = pObject->Sample_Timer_Milliseconds;
    sample_timer_milliseconds += milliseconds;
    while (sample_timer_milliseconds >= sample_interval_milliseconds) {
        sample_timer_milliseconds -= sample_interval_milliseconds;
        if (Averaging_Object_Property_Reference_Read(pObject, &sample_value)) {
            Averaging_Sample_Record(object_instance, true, sample_value);
        } else {
            Averaging_Sample_Record(object_instance, false, 0.0f);
        }
    }
    pObject->Sample_Timer_Milliseconds = (uint32_t)sample_timer_milliseconds;
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
    struct object_data *pObject = NULL;
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
            pObject = Averaging_Object(rpdata->object_instance);
            if (pObject) {
                apdu_len = bacapp_encode_datetime(
                    apdu, &pObject->Minimum_Value_Timestamp);
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
            pObject = Averaging_Object(rpdata->object_instance);
            if (pObject) {
                apdu_len = bacapp_encode_datetime(
                    apdu, &pObject->Maximum_Value_Timestamp);
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
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE object_property_ref = { 0 };
    struct object_data *pObject = NULL;

    if (wp_data == NULL) {
        return false;
    }

    pObject = Averaging_Object(wp_data->object_instance);
    if (!pObject) {
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
            len = bacnet_unsigned_application_decode(
                wp_data->application_data, wp_data->application_data_len,
                &unsigned_value);
            if (len <= 0) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                if (len < 0) {
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                } else {
                    wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
                }
                return false;
            }
            switch (wp_data->object_property) {
                case PROP_ATTEMPTED_SAMPLES:
                    if (unsigned_value == 0U) {
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
                    if (unsigned_value > 0U) {
                        pObject->Window_Interval = unsigned_value;
                        status = Averaging_Reset(wp_data->object_instance);
                    } else {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                        status = false;
                    }
                    break;
                case PROP_WINDOW_SAMPLES:
                    if (unsigned_value > 0U) {
                        pObject->Window_Samples = unsigned_value;
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
