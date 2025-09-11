/**
 * @file
 * @author Steve Karg
 * @date February 2024
 * @brief BACnet Device Object Handling
 * @copyright
 *  Copyright 2024 by Steve Karg <skarg@users.sourceforge.net>
 *  SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet core library */
#include "bacnet/dcc.h"
/* BACnet basic library */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/service/h_apdu.h"
#include "bacnet/basic/service/h_device.h"

/* Object services */
static object_functions_t *Object_Table;
static uint32_t Object_Instance_Number = BACNET_MAX_INSTANCE;
static uint32_t Database_Revision;
static BACNET_REINITIALIZED_STATE Reinitialize_State = BACNET_REINIT_IDLE;
static const char *Reinit_Password = "filister";
static bool Reinitialize_Backup_Restore_Enabled;
static uint16_t Vendor_Identifier = BACNET_VENDOR_ID;

/**
 * @brief Sets the ReinitializeDevice password
 *
 * The password shall be a null terminated C string of up to
 * 20 ASCII characters for those devices that require the password.
 *
 * For those devices that do not require a password, set to NULL or
 * point to a zero length C string (null terminated).
 *
 * @param the ReinitializeDevice password; can be NULL or empty string
 */
bool handler_device_reinitialize_password_set(const char *password)
{
    Reinit_Password = password;

    return true;
}

/**
 * @brief Set the ReinitializeDevice backup and restore enabled flag
 * @param enable [in] The flag to enable or disable the backup and restore
 */
void handler_device_reinitialize_backup_restore_enabled_set(bool enable)
{
    Reinitialize_Backup_Restore_Enabled = enable;
}

/**
 * @brief Get the ReinitializeDevice backup and restore enabled flag
 * @return True if backup and restore is enabled
 */
bool handler_device_reinitialize_backup_restore_enabled(void)
{
    return Reinitialize_Backup_Restore_Enabled;
}

/** Commands a Device re-initialization, to a given state.
 * The request's password must match for the operation to succeed.
 * This implementation provides a framework, but doesn't
 * actually *DO* anything.
 * @note You could use a mix of states and passwords to multiple outcomes.
 * @note You probably want to restart *after* the simple ack has been sent
 *       from the return handler, so just set a local flag here.
 * @ingroup ObjIntf
 *
 * @param rd_data [in,out] The information from the RD request.
 *                         On failure, the error class and code will be set.
 * @return True if succeeds (password is correct), else False.
 */
bool handler_device_reinitialize(BACNET_REINITIALIZE_DEVICE_DATA *rd_data)
{
    bool status = false;
    bool password_success = false;

    /* From 16.4.1.1.2 Password
        This optional parameter shall be a CharacterString of up to
        20 characters. For those devices that require the password as a
        protection, the service request shall be denied if the parameter
        is absent or if the password is incorrect. For those devices that
        do not require a password, this parameter shall be ignored.*/
    if (Reinit_Password && strlen(Reinit_Password) > 0) {
        if (characterstring_length(&rd_data->password) > 20) {
            rd_data->error_class = ERROR_CLASS_SERVICES;
            rd_data->error_code = ERROR_CODE_PARAMETER_OUT_OF_RANGE;
        } else if (characterstring_ansi_same(
                       &rd_data->password, Reinit_Password)) {
            password_success = true;
        } else {
            rd_data->error_class = ERROR_CLASS_SECURITY;
            rd_data->error_code = ERROR_CODE_PASSWORD_FAILURE;
        }
    } else {
        password_success = true;
    }
    if (password_success) {
        switch (rd_data->state) {
            case BACNET_REINIT_COLDSTART:
            case BACNET_REINIT_WARMSTART:
                dcc_set_status_duration(COMMUNICATION_ENABLE, 0);
                /* note: you probably want to restart *after* the
                   simple ack has been sent from the return handler
                   so just set a flag from here */
                Reinitialize_State = rd_data->state;
                status = true;
                break;
            case BACNET_REINIT_STARTBACKUP:
            case BACNET_REINIT_ENDBACKUP:
            case BACNET_REINIT_STARTRESTORE:
            case BACNET_REINIT_ENDRESTORE:
            case BACNET_REINIT_ABORTRESTORE:
                if (dcc_communication_disabled()) {
                    rd_data->error_class = ERROR_CLASS_SERVICES;
                    rd_data->error_code = ERROR_CODE_COMMUNICATION_DISABLED;
                } else if (Reinitialize_Backup_Restore_Enabled) {
                    Reinitialize_State = rd_data->state;
                    status = true;
                } else {
                    rd_data->error_class = ERROR_CLASS_SERVICES;
                    rd_data->error_code =
                        ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
                }
                break;
            default:
                rd_data->error_class = ERROR_CLASS_SERVICES;
                rd_data->error_code = ERROR_CODE_PARAMETER_OUT_OF_RANGE;
                break;
        }
    }

    return status;
}

/**
 * @brief Get the Device Reinitialize state
 * @return The current Reinitialize state of the device
 */
BACNET_REINITIALIZED_STATE handler_device_reinitialized_state(void)
{
    return Reinitialize_State;
}

/**
 * @brief Set the Device Reinitialize state
 * @param The Reinitialize state of the device to set
 */
void handler_device_reinitialized_state_set(BACNET_REINITIALIZED_STATE state)
{
    Reinitialize_State = state;
}

/** Returns the Vendor ID for this Device.
 * Get a free vendor ID, or see the assignments at
 * http://www.bacnet.org/VendorID/BACnet%20Vendor%20IDs.htm
 * @return The Vendor ID of this Device.
 */
uint16_t handler_device_vendor_identifier(void)
{
    return Vendor_Identifier;
}

/**
 * @brief Set the Vendor ID for this Device.
 * @param vendor_id [in] The desired Vendor ID for the Device.
 */
void handler_device_vendor_identifier_set(uint16_t vendor_id)
{
    Vendor_Identifier = vendor_id;
}

/** Glue function to let the Device object, when called by a handler,
 * lookup which Object type needs to be invoked.
 * @ingroup ObjHelpers
 * @param Object_Type [in] The type of BACnet Object the handler wants to
 * access.
 * @return Pointer to the group of object helper functions that implement this
 *         type of Object.
 */
static struct object_functions *
handler_device_object_functions(BACNET_OBJECT_TYPE Object_Type)
{
    struct object_functions *pObject = NULL;

    if (Object_Table == NULL) {
        return NULL;
    }
    pObject = Object_Table;
    while (pObject->Object_Type < MAX_BACNET_OBJECT_TYPE) {
        /* handle each object type */
        if (pObject->Object_Type == Object_Type) {
            return (pObject);
        }
        pObject++;
    }

    return (NULL);
}

/** Try to find a rr_info_function helper function for the requested object
 * type.
 * @ingroup ObjIntf
 *
 * @param object_type [in] The type of BACnet Object the handler wants to
 * access.
 * @return Pointer to the object helper function that implements the
 *         ReadRangeInfo function, Object_RR_Info, for this type of Object on
 *         success, else a NULL pointer if the type of Object isn't supported
 *         or doesn't have a ReadRangeInfo function.
 */
rr_info_function
handler_device_object_read_range_info(BACNET_OBJECT_TYPE object_type)
{
    struct object_functions *pObject = NULL;

    pObject = handler_device_object_functions(object_type);
    return (pObject != NULL ? pObject->Object_RR_Info : NULL);
}

/** For a given object type, returns the special property list.
 * This function is used for ReadPropertyMultiple calls which want
 * just Required, just Optional, or All properties.
 * @ingroup ObjIntf
 *
 * @param object_type [in] The desired BACNET_OBJECT_TYPE whose properties
 *            are to be listed.
 * @param pPropertyList [out] Reference to the structure which will, on return,
 *            list, separately, the Required, Optional, and Proprietary object
 *            properties with their counts.
 */
void handler_device_object_property_list(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    struct special_property_list_t *pPropertyList)
{
    struct object_functions *pObject = NULL;

    (void)object_instance;
    pPropertyList->Required.pList = NULL;
    pPropertyList->Optional.pList = NULL;
    pPropertyList->Proprietary.pList = NULL;

    /* If we can find an entry for the required object type
     * and there is an Object_List_RPM fn ptr then call it
     * to populate the pointers to the individual list counters.
     */

    pObject = handler_device_object_functions(object_type);
    if ((pObject != NULL) && (pObject->Object_RPM_List != NULL)) {
        pObject->Object_RPM_List(
            &pPropertyList->Required.pList, &pPropertyList->Optional.pList,
            &pPropertyList->Proprietary.pList);
    }

    /* Fetch the counts if available otherwise zero them */
    pPropertyList->Required.count = pPropertyList->Required.pList == NULL
        ? 0
        : property_list_count(pPropertyList->Required.pList);

    pPropertyList->Optional.count = pPropertyList->Optional.pList == NULL
        ? 0
        : property_list_count(pPropertyList->Optional.pList);

    pPropertyList->Proprietary.count = pPropertyList->Proprietary.pList == NULL
        ? 0
        : property_list_count(pPropertyList->Proprietary.pList);

    return;
}

/**
 * @brief Determine if the object property is a member of this object instance
 * @param object_type [in] The desired BACNET_OBJECT_TYPE whose properties
 *  are to be listed.
 * @param object_instance - object-instance number of the object
 * @param object_property - object-property to be checked
 * @return true if the property is a member of this object instance
 */
bool handler_device_object_property_list_member(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    int object_property)
{
    bool found = false;
    struct object_functions *pObject = NULL;
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;

    (void)object_instance;
    pObject = handler_device_object_functions(object_type);
    if ((pObject != NULL) && (pObject->Object_RPM_List != NULL)) {
        pObject->Object_RPM_List(&pRequired, &pOptional, &pProprietary);
    }
    found = property_list_member(pRequired, object_property);
    if (!found) {
        found = property_list_member(pOptional, object_property);
    }
    if (!found) {
        found = property_list_member(pProprietary, object_property);
    }

    return found;
}

/**
 * @brief Return the Object Instance number for our (single) Device Object.
 * This is a key function, widely invoked by the handler code, since
 * it provides "our" (ie, local) address.
 * @ingroup ObjIntf
 * @return The Instance number used in the BACNET_OBJECT_ID for the Device.
 */
uint32_t handler_device_object_instance_number(void)
{
    return Object_Instance_Number;
}

/**
 * @brief Set the Object Instance number for our (single) Device Object.
 * @param device_id [in] The desired BACnet Device Object Instance number.
 */
void handler_device_object_instance_number_set(uint32_t device_id)
{
    Object_Instance_Number = device_id;
}

/**
 * @brief Set the Object Instance number when a wildcard instance is used.
 * @param object_type [in] The desired BACNET_OBJECT_TYPE
 * @param object_instance [in] The instance number, which might be wildcard
 * @return The instance number used in the BACNET_OBJECT_ID.
 */
uint32_t handler_device_wildcard_instance_number(
    BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    uint32_t instance = object_instance;
#if (BACNET_PROTOCOL_REVISION >= 17)
    struct object_functions *pObject;
#endif

    /* When the object-type in the Object Identifier parameter
        contains the value DEVICE and the instance in the 'Object
        Identifier' parameter contains the value 4194303, the responding
        BACnet-user shall treat the Object Identifier as if it correctly
        matched the local Device object. This allows the device instance
        of a device that does not generate I-Am messages to be
        determined. */
    if ((object_type == OBJECT_DEVICE) &&
        (object_instance == BACNET_MAX_INSTANCE)) {
        instance = Object_Instance_Number;
    }
#if (BACNET_PROTOCOL_REVISION >= 17)
    /* When the object-type in the Object Identifier parameter
        contains the value NETWORK_PORT and the instance in the 'Object
        Identifier' parameter contains the value 4194303, the responding
        BACnet-user shall treat the Object Identifier as if it correctly
        matched the local Network Port object representing the network
        port through which the request was received. This allows the
        network port instance of the network port that was used to
        receive the request to be determined. */
    if ((object_type == OBJECT_NETWORK_PORT) &&
        (object_instance == BACNET_MAX_INSTANCE)) {
        pObject = handler_device_object_functions(object_type);
        if ((pObject != NULL) && (pObject->Object_Index_To_Instance)) {
            instance = pObject->Object_Index_To_Instance(0);
        }
    }
#endif

    return instance;
}

/**
 * @brief Get the Database Revision number for the Device Object.
 * @return The revision number of the Device Object database
 */
uint32_t handler_device_object_database_revision(void)
{
    return Database_Revision;
}

/**
 * @brief Set the Database Revision number for the Device Object.
 * @param database_revision [in] The desired revision number for the Device
 */
void handler_device_object_database_revision_set(uint32_t database_revision)
{
    Database_Revision = database_revision;
}
/*
 * @brief increment the device object database revision by 1
 */
void handler_device_object_database_revision_increment(void)
{
    Database_Revision++;
}

/**
 * @brief Determine if we have an object of this type and instance number.
 * @param object_type [in] The desired BACNET_OBJECT_TYPE
 * @param object_instance [in] The object instance number to be looked up.
 * @return True if found, else False if no such Object in this device.
 */
bool handler_device_object_instance_valid(
    BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    bool status = false; /* return value */
    struct object_functions *pObject = NULL;

    pObject = handler_device_object_functions(object_type);
    if ((pObject != NULL) && (pObject->Object_Valid_Instance != NULL)) {
        status = pObject->Object_Valid_Instance(object_instance);
    }

    return status;
}

/**
 * @brief Handles the writing of the object name property
 * @param wp_data [in,out] WriteProperty data structure
 * @param Object_Write_Property object specific function to write the property
 * @return True on success, else False if there is an error.
 */
static bool handler_device_write_property_object_name(
    BACNET_WRITE_PROPERTY_DATA *wp_data,
    write_property_function Object_Write_Property)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_CHARACTER_STRING value;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t object_instance = 0;
    int apdu_size = 0;
    uint8_t *apdu = NULL;

    if (!wp_data) {
        return false;
    }
    if (wp_data->array_index != BACNET_ARRAY_ALL) {
        /*  only array properties can have array options */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    apdu = wp_data->application_data;
    apdu_size = wp_data->application_data_len;
    len = bacnet_character_string_application_decode(apdu, apdu_size, &value);
    if (len > 0) {
        if ((characterstring_encoding(&value) != CHARACTER_ANSI_X34) ||
            (characterstring_length(&value) == 0) ||
            (!characterstring_printable(&value))) {
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        } else {
            status = true;
        }
    } else if (len == 0) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
    } else {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
    }
    if (status) {
        /* All the object names in a device must be unique */
        if (handler_device_valid_object_name(
                &value, &object_type, &object_instance)) {
            if ((object_type == wp_data->object_type) &&
                (object_instance == wp_data->object_instance)) {
                /* writing same name to same object */
                status = true;
            } else {
                /* name already exists in some object */
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_DUPLICATE_NAME;
                status = false;
            }
        } else {
            status = Object_Write_Property(wp_data);
        }
    }

    return status;
}

/** Looks up the requested Object and Property, and set the new Value in it,
 *  if allowed.
 * If the Object or Property can't be found, sets the error class and code.
 * @ingroup ObjIntf
 *
 * @param wp_data [in,out] Structure with the desired Object and Property info
 *              and new Value on entry, and APDU message on return.
 * @return True on success, else False if there is an error.
 */
bool handler_device_write_property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* Ever the pessimist! */
    struct object_functions *pObject = NULL;

    /* initialize the default return values */
    wp_data->error_class = ERROR_CLASS_OBJECT;
    wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    pObject = handler_device_object_functions(wp_data->object_type);
    if (pObject != NULL) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(wp_data->object_instance)) {
            if (pObject->Object_Write_Property) {
#if (BACNET_PROTOCOL_REVISION >= 14)
                if (wp_data->object_property == PROP_PROPERTY_LIST) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                    return status;
                }
#endif
                if (wp_data->object_property == PROP_OBJECT_NAME) {
                    status = handler_device_write_property_object_name(
                        wp_data, pObject->Object_Write_Property);
                } else {
                    status = pObject->Object_Write_Property(wp_data);
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
        } else {
            wp_data->error_class = ERROR_CLASS_OBJECT;
            wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        }
    } else {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return (status);
}

/** Get the total count of objects supported by this Device Object.
 * @note Since many network clients depend on the object list
 *       for discovery, it must be consistent!
 * @return The count of objects, for all supported Object types.
 */
unsigned handler_device_object_list_count(void)
{
    unsigned count = 0; /* number of objects */
    struct object_functions *pObject = NULL;

    if (Object_Table == NULL) {
        return 0;
    }
    /* initialize the default return values */
    pObject = Object_Table;
    while (pObject->Object_Type < MAX_BACNET_OBJECT_TYPE) {
        if (pObject->Object_Count) {
            count += pObject->Object_Count();
        }
        pObject++;
    }

    return count;
}

/** Lookup the Object at the given array index in the Device's Object List.
 * Even though we don't keep a single linear array of objects in the Device,
 * this method acts as though we do and works through a virtual, concatenated
 * array of all of our object type arrays.
 *
 * @param array_index [in] The desired array index (1 to N)
 * @param object_type [out] The object's type, if found.
 * @param instance [out] The object's instance number, if found.
 * @return True if found, else false.
 */
bool handler_device_object_list_identifier(
    uint32_t array_index, BACNET_OBJECT_TYPE *object_type, uint32_t *instance)
{
    bool status = false;
    uint32_t count = 0;
    uint32_t object_index = 0;
    uint32_t temp_index = 0;
    struct object_functions *pObject = NULL;

    /* array index zero is length - so invalid */
    if (array_index == 0) {
        return status;
    }
    if (Object_Table == NULL) {
        /* special case for default value - one device object */
        if (array_index == 1) {
            *object_type = OBJECT_DEVICE;
            *instance = Object_Instance_Number;
            status = true;
        }
        return status;
    }
    object_index = array_index - 1;
    /* initialize the default return values */
    pObject = Object_Table;
    while (pObject->Object_Type < MAX_BACNET_OBJECT_TYPE) {
        if (pObject->Object_Count) {
            object_index -= count;
            count = pObject->Object_Count();
            if (object_index < count) {
                /* Use the iterator function if available otherwise
                 * look for the index to instance to get the ID */
                if (pObject->Object_Iterator) {
                    /* First find the first object */
                    temp_index = pObject->Object_Iterator(~(unsigned)0);
                    /* Then step through the objects to find the nth */
                    while (object_index != 0) {
                        temp_index = pObject->Object_Iterator(temp_index);
                        object_index--;
                    }
                    /* set the object_index up before falling through to next
                     * bit */
                    object_index = temp_index;
                }
                if (pObject->Object_Index_To_Instance) {
                    *object_type = pObject->Object_Type;
                    *instance = pObject->Object_Index_To_Instance(object_index);
                    status = true;
                    break;
                }
            }
        }
        pObject++;
    }

    return status;
}

/**
 * @brief Encode a BACnetARRAY object list element
 * @param object_instance [in] BACnet device object instance number
 * @param array_index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
int handler_device_object_list_element_encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    BACNET_OBJECT_TYPE object_type;
    uint32_t instance;
    bool found;

    if (object_instance == Object_Instance_Number) {
        /* single element uses offset of zero,
           add 1 for BACnetARRAY which uses offset of one */
        array_index++;
        found = handler_device_object_list_identifier(
            array_index, &object_type, &instance);
        if (found) {
            apdu_len =
                encode_application_object_id(apdu, object_type, instance);
        }
    }

    return apdu_len;
}

/**
 * @brief AddListElement from an object list property
 * @param list_element [in] Pointer to the BACnet_List_Element_Data structure,
 * which is packed with the information from the request.
 * @return The length of the apdu encoded or #BACNET_STATUS_ERROR or
 * #BACNET_STATUS_ABORT or #BACNET_STATUS_REJECT.
 */
int handler_device_object_list_element_add(
    BACNET_LIST_ELEMENT_DATA *list_element)
{
    int status = BACNET_STATUS_ERROR;
    struct object_functions *pObject = NULL;

    pObject = handler_device_object_functions(list_element->object_type);
    if (pObject != NULL) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(list_element->object_instance)) {
            if (pObject->Object_Add_List_Element) {
                status = pObject->Object_Add_List_Element(list_element);
            } else {
                list_element->error_class = ERROR_CLASS_PROPERTY;
                list_element->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
        } else {
            list_element->error_class = ERROR_CLASS_OBJECT;
            list_element->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        }
    } else {
        list_element->error_class = ERROR_CLASS_OBJECT;
        list_element->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/**
 * @brief RemoveListElement from an object list property
 * @param list_element [in] Pointer to the BACnet_List_Element_Data structure,
 * which is packed with the information from the request.
 * @return The length of the apdu encoded or #BACNET_STATUS_ERROR or
 * #BACNET_STATUS_ABORT or #BACNET_STATUS_REJECT.
 */
int handler_device_object_list_element_remove(
    BACNET_LIST_ELEMENT_DATA *list_element)
{
    int status = BACNET_STATUS_ERROR;
    struct object_functions *pObject = NULL;

    pObject = handler_device_object_functions(list_element->object_type);
    if (pObject != NULL) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(list_element->object_instance)) {
            if (pObject->Object_Remove_List_Element) {
                status = pObject->Object_Remove_List_Element(list_element);
            } else {
                list_element->error_class = ERROR_CLASS_PROPERTY;
                list_element->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
        } else {
            list_element->error_class = ERROR_CLASS_OBJECT;
            list_element->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        }
    } else {
        list_element->error_class = ERROR_CLASS_OBJECT;
        list_element->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/** Determine if we have an object with the given object_name.
 * If the object_type and object_instance pointers are not null,
 * and the lookup succeeds, they will be given the resulting values.
 * @param object_name [in] The desired Object Name to look for.
 * @param object_type [out] The BACNET_OBJECT_TYPE of the matching Object.
 * @param object_instance [out] The object instance number of the matching
 * Object.
 * @return True on success or else False if not found.
 */
bool handler_device_valid_object_name(
    const BACNET_CHARACTER_STRING *object_name1,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *object_instance)
{
    bool found = false;
    BACNET_OBJECT_TYPE type = OBJECT_NONE;
    uint32_t instance;
    uint32_t max_objects = 0, i = 0;
    bool check_id = false;
    BACNET_CHARACTER_STRING object_name2;
    struct object_functions *pObject = NULL;

    max_objects = handler_device_object_list_count();
    for (i = 1; i <= max_objects; i++) {
        check_id = handler_device_object_list_identifier(i, &type, &instance);
        if (check_id) {
            pObject = handler_device_object_functions(type);
            if ((pObject != NULL) && (pObject->Object_Name != NULL) &&
                (pObject->Object_Name(instance, &object_name2) &&
                 characterstring_same(object_name1, &object_name2))) {
                found = true;
                if (object_type) {
                    *object_type = type;
                }
                if (object_instance) {
                    *object_instance = instance;
                }
                break;
            }
        }
    }

    return found;
}

/** Determine if we have an object of this type and instance number.
 * @param object_type [in] The desired BACNET_OBJECT_TYPE
 * @param object_instance [in] The object instance number to be looked up.
 * @return True if found, else False if no such Object in this device.
 */
bool handler_device_valid_object_instance(
    BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    bool status = false; /* return value */
    struct object_functions *pObject = NULL;

    pObject = handler_device_object_functions(object_type);
    if ((pObject != NULL) && (pObject->Object_Valid_Instance != NULL)) {
        status = pObject->Object_Valid_Instance(object_instance);
    }

    return status;
}

#if defined(INTRINSIC_REPORTING)
void handler_device_intrinsic_reporting(void)
{
    struct object_functions *pObject = NULL;
    uint32_t objects_count = 0;
    uint32_t object_instance = 0;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t idx = 0;

    objects_count = handler_device_object_list_count();
    /* loop for all objects */
    for (idx = 1; idx <= objects_count; idx++) {
        handler_device_object_list_identifier(
            idx, &object_type, &object_instance);
        pObject = handler_device_object_functions(object_type);
        if (pObject != NULL) {
            if (pObject->Object_Valid_Instance &&
                pObject->Object_Valid_Instance(object_instance)) {
                if (pObject->Object_Intrinsic_Reporting) {
                    pObject->Object_Intrinsic_Reporting(object_instance);
                }
            }
        }
    }
}
#endif

/** Copy a child object's object_name value, given its ID.
 * @param object_type [in] The BACNET_OBJECT_TYPE of the child Object.
 * @param object_instance [in] The object instance number of the child Object.
 * @param object_name [out] The Object Name found for this child Object.
 * @return True on success or else False if not found.
 */
bool handler_device_object_name_copy(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name)
{
    struct object_functions *pObject = NULL;
    bool found = false;

    pObject = handler_device_object_functions(object_type);
    if ((pObject != NULL) && (pObject->Object_Name != NULL)) {
        found = pObject->Object_Name(object_instance, object_name);
    }

    return found;
}

/** Looks up the requested Object to see if the functionality is supported.
 * @ingroup ObjHelpers
 * @param [in] The object type to be looked up.
 * @return True if the object instance supports this feature.
 */
bool handler_device_object_value_list_supported(BACNET_OBJECT_TYPE object_type)
{
    bool status = false; /* Ever the pessamist! */
    struct object_functions *pObject = NULL;

    pObject = handler_device_object_functions(object_type);
    if (pObject != NULL) {
        if (pObject->Object_Value_List) {
            status = true;
        }
    }

    return (status);
}

/** Looks up the requested Object, and fills the Property Value list.
 * If the Object or Property can't be found, returns false.
 * @ingroup ObjHelpers
 * @param [in] The object type to be looked up.
 * @param [in] The object instance number to be looked up.
 * @param [out] The value list
 * @return True if the object instance supports this feature and value changed.
 */
bool handler_device_object_value_list(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_VALUE *value_list)
{
    bool status = false; /* Ever the pessimist! */
    struct object_functions *pObject = NULL;

    pObject = handler_device_object_functions(object_type);
    if (pObject != NULL) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(object_instance)) {
            if (pObject->Object_Value_List) {
                status =
                    pObject->Object_Value_List(object_instance, value_list);
            }
        }
    }

    return (status);
}

/** Checks the COV flag in the requested Object
 * @ingroup ObjHelpers
 * @param [in] The object type to be looked up.
 * @param [in] The object instance to be looked up.
 * @return True if the COV flag is set
 */
bool handler_device_object_cov(
    BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    bool status = false; /* Ever the pessamist! */
    struct object_functions *pObject = NULL;

    pObject = handler_device_object_functions(object_type);
    if (pObject != NULL) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(object_instance)) {
            if (pObject->Object_COV) {
                status = pObject->Object_COV(object_instance);
            }
        }
    }

    return (status);
}

/** Clears the COV flag in the requested Object
 * @ingroup ObjHelpers
 * @param [in] The object type to be looked up.
 * @param [in] The object instance to be looked up.
 */
void handler_device_object_cov_clear(
    BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    struct object_functions *pObject = NULL;

    pObject = handler_device_object_functions(object_type);
    if (pObject != NULL) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(object_instance)) {
            if (pObject->Object_COV_Clear) {
                pObject->Object_COV_Clear(object_instance);
            }
        }
    }
}

/**
 * @brief Get the Device Object's services supported
 * @param The bit string representing the services supported
 */
void handler_device_services_supported(BACNET_BIT_STRING *bit_string)
{
    uint8_t i = 0;

    /* Note: list of services that are executed, not initiated. */
    bitstring_init(bit_string);
    for (i = 0; i < MAX_BACNET_SERVICES_SUPPORTED; i++) {
        /* automatic lookup based on handlers set */
        bitstring_set_bit(
            bit_string, i,
            apdu_service_supported((BACNET_SERVICES_SUPPORTED)i));
    }
}

/**
 * @brief Get the Device Object's supported objects
 * @param The bit string representing the supported objects
 */
void handler_device_object_types_supported(BACNET_BIT_STRING *bit_string)
{
    unsigned i = 0;
    object_functions_t *pObject = NULL;

    if (Object_Table == NULL) {
        return;
    }
    /* Note: this is the list of objects that can be in this device,
       not a list of objects that this device can access */
    bitstring_init(bit_string);
    for (i = 0; i < MAX_ASHRAE_OBJECT_TYPE; i++) {
        /* initialize all the object types to not-supported */
        bitstring_set_bit(bit_string, (uint8_t)i, false);
    }
    /* set the object types with objects to supported */
    pObject = Object_Table;
    while (pObject->Object_Type < MAX_BACNET_OBJECT_TYPE) {
        if ((pObject->Object_Count) && (pObject->Object_Count() > 0)) {
            bitstring_set_bit(bit_string, (uint8_t)pObject->Object_Type, true);
        }
        pObject++;
    }
}

/** Looks up the common Object and Property, and encodes its Value in an
 * APDU. Sets the error class and code if request is not appropriate.
 * @param pObject - object table
 * @param rpdata [in,out] Structure with the requested Object & Property info
 *  on entry, and APDU message on return.
 * @return The length of the APDU on success, else BACNET_STATUS_ERROR
 */
int handler_device_read_property_common(
    struct object_functions *pObject, BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = BACNET_STATUS_ERROR;
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu = NULL;
#if (BACNET_PROTOCOL_REVISION >= 14)
    struct special_property_list_t property_list;
#endif

    if ((rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    if (property_list_common(rpdata->object_property)) {
        apdu_len = property_list_common_encode(rpdata, Object_Instance_Number);
    } else if (rpdata->object_property == PROP_OBJECT_NAME) {
        /*  only array properties can have array options */
        if (rpdata->array_index != BACNET_ARRAY_ALL) {
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
            apdu_len = BACNET_STATUS_ERROR;
        } else {
            characterstring_init_ansi(&char_string, "");
            if (pObject->Object_Name) {
                (void)pObject->Object_Name(
                    rpdata->object_instance, &char_string);
            }
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
        }
#if (BACNET_PROTOCOL_REVISION >= 14)
    } else if (rpdata->object_property == PROP_PROPERTY_LIST) {
        handler_device_object_property_list(
            rpdata->object_type, rpdata->object_instance, &property_list);
        apdu_len = property_list_encode(
            rpdata, property_list.Required.pList, property_list.Optional.pList,
            property_list.Proprietary.pList);
#endif
    } else if (pObject->Object_Read_Property) {
        apdu_len = pObject->Object_Read_Property(rpdata);
    }

    return apdu_len;
}

/**
 * @brief Encodes the requested device object property default value
 * @param rpdata [in,out] Structure with the requested Object & Property info
 * on entry, and APDU message on return.
 * @return The length of the APDU on success, else BACNET_STATUS_ERROR
 */
int handler_device_read_property_default(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    uint8_t *apdu = NULL;
    uint16_t apdu_max = 0;
    BACNET_BIT_STRING bit_string = { 0 };
    BACNET_CHARACTER_STRING char_string = { 0 };

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    apdu_max = rpdata->application_data_len;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_DEVICE, Object_Instance_Number);
            break;
        case PROP_OBJECT_NAME:
            characterstring_init_ansi(&char_string, "Default Device Name");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_DEVICE);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string, "Default Device Description");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_SYSTEM_STATUS:
            apdu_len =
                encode_application_enumerated(&apdu[0], STATUS_OPERATIONAL);
            break;
        case PROP_VENDOR_NAME:
            characterstring_init_ansi(&char_string, "Default Vendor Name");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_VENDOR_IDENTIFIER:
            apdu_len = encode_application_unsigned(&apdu[0], BACNET_VENDOR_ID);
            break;
        case PROP_MODEL_NAME:
            characterstring_init_ansi(&char_string, "Default Model Name");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_FIRMWARE_REVISION:
            characterstring_init_ansi(
                &char_string, "Default Firmware Revision");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_APPLICATION_SOFTWARE_VERSION:
            characterstring_init_ansi(
                &char_string, "Default Application Software Version");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_LOCATION:
            characterstring_init_ansi(&char_string, "Default Location");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_PROTOCOL_VERSION:
            apdu_len =
                encode_application_unsigned(&apdu[0], BACNET_PROTOCOL_VERSION);
            break;
        case PROP_PROTOCOL_REVISION:
            apdu_len =
                encode_application_unsigned(&apdu[0], BACNET_PROTOCOL_REVISION);
            break;
        case PROP_PROTOCOL_SERVICES_SUPPORTED:
            handler_device_services_supported(&bit_string);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
            bitstring_set_bit(&bit_string, OBJECT_DEVICE, true);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_OBJECT_LIST:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                handler_device_object_list_element_encode, 1, apdu, apdu_max);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_MAX_APDU_LENGTH_ACCEPTED:
            apdu_len = encode_application_unsigned(&apdu[0], MAX_APDU);
            break;
        case PROP_SEGMENTATION_SUPPORTED:
            apdu_len =
                encode_application_enumerated(&apdu[0], SEGMENTATION_NONE);
            break;
        case PROP_APDU_TIMEOUT:
            apdu_len = encode_application_unsigned(&apdu[0], apdu_timeout());
            break;
        case PROP_NUMBER_OF_APDU_RETRIES:
            apdu_len = encode_application_unsigned(&apdu[0], apdu_retries());
            break;
        case PROP_DEVICE_ADDRESS_BINDING:
#if (MAX_ADDRESS_CACHE > 0)
            apdu_len = address_list_encode(&apdu[0], apdu_max);
#endif
            break;
        case PROP_DATABASE_REVISION:
            apdu_len = encode_application_unsigned(
                &apdu[0], handler_device_object_database_revision());
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->object_property != PROP_OBJECT_LIST) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/** Looks up the requested Object and Property, and encodes its Value in an
 * APDU.
 * @ingroup ObjIntf
 * If the Object or Property can't be found, sets the error class and code.
 *
 * @param rpdata [in,out] Structure with the desired Object and Property info
 *                 on entry, and APDU message on return.
 * @return The length of the APDU on success, else BACNET_STATUS_ERROR
 */
int handler_device_read_property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = BACNET_STATUS_ERROR;
    struct object_functions *pObject = NULL;

    /* initialize the default return values */
    rpdata->error_class = ERROR_CLASS_OBJECT;
    rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    pObject = handler_device_object_functions(rpdata->object_type);
    if (pObject != NULL) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(rpdata->object_instance)) {
            apdu_len = handler_device_read_property_common(pObject, rpdata);
        } else {
            rpdata->error_class = ERROR_CLASS_OBJECT;
            rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        }
    } else if (rpdata->object_type == OBJECT_DEVICE) {
        /* no object data - so use some defaults for minimal device */
        apdu_len = handler_device_read_property_default(rpdata);
    } else {
        rpdata->error_class = ERROR_CLASS_OBJECT;
        rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return apdu_len;
}

/**
 * @brief Creates a child object, if supported
 * @ingroup ObjHelpers
 * @param data - CreateObject data, including error codes if failures
 * @return true if object has been created
 */
bool handler_device_object_create(BACNET_CREATE_OBJECT_DATA *data)
{
    bool status = false;
    struct object_functions *pObject = NULL;
    uint32_t object_instance;

    pObject = handler_device_object_functions(data->object_type);
    if (pObject != NULL) {
        if (!pObject->Object_Create) {
            /*  The device supports the object type and may have
                sufficient space, but does not support the creation of the
                object for some other reason.*/
            data->error_class = ERROR_CLASS_OBJECT;
            data->error_code = ERROR_CODE_DYNAMIC_CREATION_NOT_SUPPORTED;
        } else if (
            pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(data->object_instance)) {
            /* The object being created already exists */
            data->error_class = ERROR_CLASS_OBJECT;
            data->error_code = ERROR_CODE_OBJECT_IDENTIFIER_ALREADY_EXISTS;
        } else {
            if (data->list_of_initial_values) {
                /* FIXME: add support for writing to list of initial values */
                /*  A property specified by the Property_Identifier in the
                    List of Initial Values does not support initialization
                    during the CreateObject service. */
                data->first_failed_element_number = 1;
                data->error_class = ERROR_CLASS_PROPERTY;
                data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                /* and the object shall not be created */
            } else {
                object_instance = pObject->Object_Create(data->object_instance);
                if (object_instance == BACNET_MAX_INSTANCE) {
                    /* The device cannot allocate the space needed
                    for the new object.*/
                    data->error_class = ERROR_CLASS_RESOURCES;
                    data->error_code = ERROR_CODE_NO_SPACE_FOR_OBJECT;
                } else {
                    /* required by ACK */
                    data->object_instance = object_instance;
                    handler_device_object_database_revision_increment();
                    status = true;
                }
            }
        }
    } else {
        /* The device does not support the specified object type. */
        data->error_class = ERROR_CLASS_OBJECT;
        data->error_code = ERROR_CODE_UNSUPPORTED_OBJECT_TYPE;
    }

    return status;
}

/**
 * @brief Deletes a child object, if supported
 * @ingroup ObjHelpers
 * @param data - DeleteObject data, including error codes if failures
 * @return true if object has been deleted
 */
bool handler_device_object_delete(BACNET_DELETE_OBJECT_DATA *data)
{
    bool status = false;
    struct object_functions *pObject = NULL;

    pObject = handler_device_object_functions(data->object_type);
    if (pObject != NULL) {
        if (!pObject->Object_Delete) {
            /*  The device supports the object type
                but does not support the deletion of the
                object for some reason.*/
            data->error_class = ERROR_CLASS_OBJECT;
            data->error_code = ERROR_CODE_OBJECT_DELETION_NOT_PERMITTED;
        } else if (
            pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(data->object_instance)) {
            /* The object being deleted must already exist */
            status = pObject->Object_Delete(data->object_instance);
            if (status) {
                handler_device_object_database_revision_increment();
            } else {
                /* The object exists but cannot be deleted. */
                data->error_class = ERROR_CLASS_OBJECT;
                data->error_code = ERROR_CODE_OBJECT_DELETION_NOT_PERMITTED;
            }
        } else {
            /* The object to be deleted does not exist. */
            data->error_class = ERROR_CLASS_OBJECT;
            data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        }
    } else {
        /* The device does not support the specified object type. */
        data->error_class = ERROR_CLASS_OBJECT;
        data->error_code = ERROR_CODE_UNSUPPORTED_OBJECT_TYPE;
    }

    return status;
}

/**
 * @brief Updates all the object timers with elapsed milliseconds
 * @param milliseconds - number of milliseconds elapsed
 */
void handler_device_timer(uint16_t milliseconds)
{
    struct object_functions *pObject;
    unsigned count = 0;
    uint32_t instance;

    if (Object_Table == NULL) {
        return;
    }
    pObject = Object_Table;
    while (pObject->Object_Type < MAX_BACNET_OBJECT_TYPE) {
        if (pObject->Object_Count) {
            count = pObject->Object_Count();
        }
        while (count) {
            count--;
            if ((pObject->Object_Timer) &&
                (pObject->Object_Index_To_Instance)) {
                instance = pObject->Object_Index_To_Instance(count);
                pObject->Object_Timer(instance, milliseconds);
            }
        }
        pObject++;
    }
}

/**
 * @brief Set the Object Table for the Device Object.
 * @param object_table [in,out] array of structure with object functions.
 *  Each Child Object must provide some implementation of each of these
 *  functions in order to properly support the default handlers.
 */
void handler_device_object_table_set(object_functions_t *object_table)
{
    Object_Table = object_table;
}

/**
 * @brief Initialize the Device Object and its child objects.
 */
void handler_device_object_init(void)
{
    object_functions_t *pObject;

    if (Object_Table == NULL) {
        return;
    }
    pObject = Object_Table;
    while (pObject->Object_Type < MAX_BACNET_OBJECT_TYPE) {
        if (pObject->Object_Init) {
            pObject->Object_Init();
        }
        pObject++;
    }
}
