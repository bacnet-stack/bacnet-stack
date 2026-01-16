/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2025
 * @brief The Timer object type defines a standardized object whose properties
 *  represent the externally visible characteristics of a countdown timer.
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string.h>
#include <stdlib.h>

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/bactext.h"
#include "bacnet/datetime.h"
#include "bacnet/proplist.h"
#include "bacnet/timer_value.h"
/* basic objects and services */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/keylist.h"
/* me! */
#include "bacnet/basic/object/timer.h"

#ifndef BACNET_TIMER_MANIPULATED_PROPERTIES_MAX
#define BACNET_TIMER_MANIPULATED_PROPERTIES_MAX 8
#endif

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List = NULL;
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_TIMER;
static write_property_function Write_Property_Internal_Callback;

struct object_data {
    uint32_t Present_Value;
    BACNET_TIMER_STATE Timer_State;
    BACNET_TIMER_TRANSITION Last_State_Change;
    BACNET_DATE_TIME Update_Time;
    uint32_t Initial_Timeout;
    uint32_t Default_Timeout;
    uint32_t Min_Pres_Value;
    uint32_t Max_Pres_Value;
    uint32_t Resolution;
    /* The timer state change NONE=0 has no corresponding array element.*/
    BACNET_TIMER_STATE_CHANGE_VALUE
    State_Change_Values[TIMER_TRANSITION_MAX - 1];
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE
    Manipulated_Properties[BACNET_TIMER_MANIPULATED_PROPERTIES_MAX];
    uint8_t Priority_For_Writing;
    const char *Description;
    const char *Object_Name;
    BACNET_RELIABILITY Reliability;
    bool Out_Of_Service : 1;
    bool Changed : 1;
    void *Context;
};

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int32_t Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,       PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,      PROP_TIMER_STATE,
    PROP_TIMER_RUNNING,     -1
};

static const int32_t Properties_Optional[] = {
    /* unordered list of optional properties */
    PROP_DESCRIPTION,
    PROP_RELIABILITY,
    PROP_OUT_OF_SERVICE,
    PROP_UPDATE_TIME,
    PROP_LAST_STATE_CHANGE,
    PROP_EXPIRATION_TIME,
    PROP_INITIAL_TIMEOUT,
    PROP_DEFAULT_TIMEOUT,
    PROP_MIN_PRES_VALUE,
    PROP_MAX_PRES_VALUE,
    PROP_RESOLUTION,
    PROP_STATE_CHANGE_VALUES,
    PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES,
    PROP_PRIORITY_FOR_WRITING,
    -1
};

static const int32_t Properties_Proprietary[] = { -1 };

/* Every object shall have a Writable Property_List property
   which is a BACnetARRAY of property identifiers,
   one property identifier for each property within this object
   that is always writable.  */
static const int32_t Writable_Properties[] = {
    /* unordered list of always writable properties */
    PROP_PRESENT_VALUE,
    PROP_OUT_OF_SERVICE,
    PROP_DEFAULT_TIMEOUT,
    PROP_MIN_PRES_VALUE,
    PROP_MAX_PRES_VALUE,
    PROP_RESOLUTION,
    PROP_PRIORITY_FOR_WRITING,
    PROP_STATE_CHANGE_VALUES,
    PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES,
    -1
};

/**
 * Returns the list of required, optional, and proprietary properties.
 * Used by ReadPropertyMultiple service.
 *
 * @param pRequired - pointer to list of int terminated by -1, of
 * BACnet required properties for this object.
 * @param pOptional - pointer to list of int terminated by -1, of
 * BACnet optkional properties for this object.
 * @param pProprietary - pointer to list of int terminated by -1, of
 * BACnet proprietary properties for this object.
 */
void Timer_Property_Lists(
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

    return;
}

/**
 * @brief Get the list of writable properties for a Timer object
 * @param  object_instance - object-instance number of the object
 * @param  properties - Pointer to the pointer of writable properties.
 */
void Timer_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties)
{
    (void)object_instance;
    if (properties) {
        *properties = Writable_Properties;
    }
}

/**
 * @brief Gets an object from the list using an instance number as the key
 * @param  object_instance - object-instance number of the object
 * @return object found in the list, or NULL if not found
 */
static struct object_data *Object_Data(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

/**
 * Determines if a given Timer instance is valid
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if the instance is valid, and false if not
 */
bool Timer_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject = Object_Data(object_instance);

    return (pObject != NULL);
}

/**
 * Determines the number of Timer objects
 *
 * @return  Number of Timer objects
 */
unsigned Timer_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * Determines the object instance-number for a given 0..N index
 * of Timer objects where N is Timer_Count().
 *
 * @param  index - 0..MAX_PROGRAMS value
 *
 * @return  object instance-number for the given index
 */
uint32_t Timer_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * For a given object instance-number, determines a 0..N index
 * of Timer objects where N is Timer_Count().
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  index for the given instance-number, or MAX_PROGRAMS
 * if not valid.
 */
unsigned Timer_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * For a given object instance-number, determines if the member is empty
 *
 * Elements of the List_Of_Object_Property_References array containing
 * object or device instance numbers equal to 4194303 are considered to
 * be 'empty' or 'uninitialized'.
 *
 * @param  pMember - object property reference element
 * @return true if the member is empty
 */
static bool Timer_Reference_List_Member_Empty(
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember)
{
    bool status = false;

    if (pMember) {
        if ((pMember->objectIdentifier.instance == BACNET_MAX_INSTANCE) ||
            (pMember->deviceIdentifier.instance == BACNET_MAX_INSTANCE)) {
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, returns the list member element
 * @param object_instance - object-instance number of the object
 * @param list_index - 1-based list index of members
 * @return pointer to list member element or NULL if not found
 */
BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *Timer_Reference_List_Member_Element(
    uint32_t object_instance, unsigned list_index)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember = NULL;
    struct object_data *pObject;
    unsigned i, count = 0;

    pObject = Object_Data(object_instance);
    if (pObject && (list_index > 0)) {
        for (i = 0; i < BACNET_TIMER_MANIPULATED_PROPERTIES_MAX; i++) {
            pMember = &pObject->Manipulated_Properties[i];
            if (!Timer_Reference_List_Member_Empty(pMember)) {
                count++;
            }
            if (count == list_index) {
                return pMember;
            }
        }
    }

    return NULL;
}

/**
 * @brief Encode a BACnetList property element
 * @param object_instance [in] BACnet object instance number
 * @param list_index [in] list index requested:
 *    0 to N for individual list members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or 0 if invalid member
 */
static int Timer_List_Of_Object_Property_References_Encode(
    uint32_t object_instance, uint32_t list_index, uint8_t *apdu)
{
    int apdu_len = 0;
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value;

    value =
        Timer_Reference_List_Member_Element(object_instance, list_index + 1);
    if (value) {
        apdu_len = bacapp_encode_device_obj_property_ref(apdu, value);
    }

    return apdu_len;
}

/**
 * For a given object instance-number, set the member element values
 * @param pObject - object in which to set the value
 * @param index - 0-based array index
 * @param pMember - pointer to member value, or NULL to set as 'empty'
 * @return true if set, false if not set
 */
static bool List_Of_Object_Property_References_Set(
    struct object_data *pObject,
    unsigned index,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember)
{
    bool status = false;
    if (pObject && (index < BACNET_TIMER_MANIPULATED_PROPERTIES_MAX)) {
        if (pMember) {
            memcpy(
                &pObject->Manipulated_Properties[index], pMember,
                sizeof(BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE));
        } else {
            pObject->Manipulated_Properties[index].objectIdentifier.instance =
                BACNET_MAX_INSTANCE;
            pObject->Manipulated_Properties[index].deviceIdentifier.instance =
                BACNET_MAX_INSTANCE;
            pObject->Manipulated_Properties[index].objectIdentifier.type =
                OBJECT_LIGHTING_OUTPUT;
            pObject->Manipulated_Properties[index].objectIdentifier.instance =
                BACNET_MAX_INSTANCE;
            pObject->Manipulated_Properties[index].propertyIdentifier =
                PROP_PRESENT_VALUE;
            pObject->Manipulated_Properties[index].arrayIndex =
                BACNET_ARRAY_ALL;
            pObject->Manipulated_Properties[index].deviceIdentifier.type =
                OBJECT_DEVICE;
            pObject->Manipulated_Properties[index].deviceIdentifier.instance =
                BACNET_MAX_INSTANCE;
        }
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, set the member element value
 * @param object_instance - object-instance number of the object
 * @param index - zero-based array index reference list array
 * @param pMember - member values to set, or NULL to set as 'empty'
 * @return pointer to member element or NULL if not found
 */
bool Timer_Reference_List_Member_Element_Set(
    uint32_t object_instance,
    unsigned index,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        status =
            List_Of_Object_Property_References_Set(pObject, index, pMember);
    }

    return status;
}

/**
 * For a given object instance-number, determines the member count
 * @param  object_instance - object-instance number of the object
 * @return member count
 */
unsigned Timer_Reference_List_Member_Capacity(uint32_t object_instance)
{
    (void)object_instance;
    return BACNET_TIMER_MANIPULATED_PROPERTIES_MAX;
}

/**
 * @brief For a given object instance-number, adds a unique member element
 *  to the list
 * @param object_instance - object-instance number of the object
 * @param pMemberSrc - pointer to a object property reference element
 * @return true if the element was added, false if the element was not added
 */
bool Timer_Reference_List_Member_Element_Add(
    uint32_t object_instance,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pNewMember)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember = NULL;
    unsigned m = 0;
    struct object_data *pObject;

    if (Timer_Reference_List_Member_Empty(pNewMember)) {
        /* The element value is out of range for the property. */
        return false;
    }
    pObject = Object_Data(object_instance);
    if (pObject) {
        /* is the element already in the list? */
        for (m = 0; m < BACNET_TIMER_MANIPULATED_PROPERTIES_MAX; m++) {
            pMember = &pObject->Manipulated_Properties[m];
            if (!Timer_Reference_List_Member_Empty(pMember)) {
                if (bacnet_device_object_property_reference_same(
                        pNewMember, pMember)) {
                    return true;
                }
            }
        }
        for (m = 0; m < BACNET_TIMER_MANIPULATED_PROPERTIES_MAX; m++) {
            pMember = &pObject->Manipulated_Properties[m];
            if (Timer_Reference_List_Member_Empty(pMember)) {
                /* first empty slot */
                memcpy(
                    pMember, pNewMember,
                    sizeof(BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE));
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief For a given object instance-number, removes a list element
 * @param object_instance - object-instance number of the object
 * @param pMemberSrc - pointer to a object property reference element,
 *  or NULL to remove ALL
 * @return true if removed, false if not removed
 */
bool Timer_Reference_List_Member_Element_Remove(
    uint32_t object_instance,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pRemoveMember)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember = NULL;
    unsigned m = 0;
    bool status = false;
    struct object_data *pObject;

    if (Timer_Reference_List_Member_Empty(pRemoveMember)) {
        /* The element value is out of range for the property. */
        return false;
    }
    pObject = Object_Data(object_instance);
    if (pObject) {
        for (m = 0; m < BACNET_TIMER_MANIPULATED_PROPERTIES_MAX; m++) {
            pMember = &pObject->Manipulated_Properties[m];
            if (!Timer_Reference_List_Member_Empty(pMember)) {
                if (pRemoveMember) {
                    if (bacnet_device_object_property_reference_same(
                            pRemoveMember, pMember)) {
                        List_Of_Object_Property_References_Set(
                            pObject, m, NULL);
                        status = true;
                    }
                } else {
                    List_Of_Object_Property_References_Set(pObject, m, NULL);
                    status = true;
                }
            }
        }
    }

    return status;
}

/**
 * For a given object instance-number, determines the BACnetLIST count
 * @param  object_instance - object-instance number of the object
 * @return BACnetLIST count
 */
unsigned Timer_Reference_List_Member_Element_Count(uint32_t object_instance)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember = NULL;
    unsigned count = 0, i;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        for (i = 0; i < BACNET_TIMER_MANIPULATED_PROPERTIES_MAX; i++) {
            pMember = &pObject->Manipulated_Properties[i];
            if (!Timer_Reference_List_Member_Empty(pMember)) {
                count++;
            }
        }
    }

    return count;
}

/**
 * For a given object instance-number, sets the present-value at a given
 * priority 1..16.
 *
 * @param pObject - object instance data
 * @param value - application value
 * @param priority - BACnet priority 0=none,1..16
 *
 * @return  true if values are within range and present-value is sent.
 */
static bool Timer_Write_Members(
    struct object_data *pObject,
    const BACNET_TIMER_STATE_CHANGE_VALUE *value,
    uint8_t priority)
{
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    bool status = false;
    unsigned i = 0;
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember = NULL;

    if (pObject && value) {
        for (i = 0; i < BACNET_TIMER_MANIPULATED_PROPERTIES_MAX; i++) {
            pMember = &pObject->Manipulated_Properties[i];
            if (Timer_Reference_List_Member_Empty(pMember)) {
                continue;
            }
            if ((pMember->deviceIdentifier.type == OBJECT_DEVICE) &&
                (pMember->deviceIdentifier.instance != BACNET_MAX_INSTANCE) &&
                (pMember->objectIdentifier.instance != BACNET_MAX_INSTANCE)) {
                wp_data.object_type = pMember->objectIdentifier.type;
                wp_data.object_instance = pMember->objectIdentifier.instance;
                wp_data.object_property = pMember->propertyIdentifier;
                wp_data.array_index = pMember->arrayIndex;
                wp_data.error_class = ERROR_CLASS_PROPERTY;
                wp_data.error_code = ERROR_CODE_SUCCESS;
                wp_data.priority = priority;
                wp_data.application_data_len = bacnet_timer_value_encode(
                    wp_data.application_data, sizeof(wp_data.application_data),
                    value);
                if (Write_Property_Internal_Callback) {
                    status = Write_Property_Internal_Callback(&wp_data);
                    if (status) {
                        wp_data.error_code = ERROR_CODE_SUCCESS;
                    }
                }
            }
        }
    }

    return status;
}

/**
 * @brief initiate the write requests for the current transition
 * @param  object_instance - object-instance number of the object
 * @return true if the write occurred
 */
static bool Timer_Write_Request_Initiate(struct object_data *pObject)
{
    bool status = false;
    unsigned index = 0;
    BACNET_TIMER_STATE_CHANGE_VALUE *value = NULL;

    if (pObject) {
        if (pObject->Last_State_Change != TIMER_TRANSITION_NONE) {
            index = pObject->Last_State_Change;
            if (index < TIMER_TRANSITION_MAX) {
                index--;
                value = &pObject->State_Change_Values[index];
            }
        }
        if (value) {
            status = Timer_Write_Members(
                pObject, value, pObject->Priority_For_Writing);
        }
    }

    return status;
}

/**
 * For a given object instance-number, determines the program-state
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  program-state of the object
 */
BACNET_TIMER_STATE Timer_State(uint32_t object_instance)
{
    BACNET_TIMER_STATE value = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Timer_State;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the timer-state
 * @details This property, of type BACnetTimerState, indicates the
 *  current state of the timer this object represents.
 *  To clear the timer, i.e. to request the timer to enter the IDLE state,
 *  a value of IDLE is written to this property.
 *
 *  Writing this value to this property while in the RUNNING or EXPIRED
 *  state will force the timer to enter the IDLE state.
 *  If already in the IDLE state, no state transition occurs
 *  if this value is written.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - integer value
 *
 * @return return false if a value other than IDLE is written to this property
 */
bool Timer_State_Set(uint32_t object_instance, BACNET_TIMER_STATE value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (value == TIMER_STATE_IDLE) {
            /* Clear Request */
            /* If a value of IDLE is written to the Timer_State property */
            if (pObject->Timer_State == TIMER_STATE_RUNNING) {
                /* set Timer_State to IDLE;
                   set Last_State_Change to RUNNING_TO_IDLE;
                   set Present_Value to zero;
                   set Update_Time to the current date and time;
                   initiate the write requests for the RUNNING_TO_IDLE
                   transition if present;
                   and enter the IDLE state. */
                pObject->Timer_State = TIMER_STATE_IDLE;
                pObject->Present_Value = 0;
                datetime_local(
                    &pObject->Update_Time.date, &pObject->Update_Time.time,
                    NULL, NULL);
                pObject->Last_State_Change = TIMER_TRANSITION_RUNNING_TO_IDLE;
                Timer_Write_Request_Initiate(pObject);
            } else if (pObject->Timer_State == TIMER_STATE_EXPIRED) {
                pObject->Last_State_Change = TIMER_TRANSITION_EXPIRED_TO_IDLE;
                /*  then set Timer_State to IDLE;
                    set Last_State_Change to EXPIRED_TO_IDLE;
                    set Update_Time to current date and time;
                    initiate the write requests for the EXPIRED_TO_IDLE
                    transition if present; and enter the IDLE state. */
                pObject->Timer_State = TIMER_STATE_IDLE;
                pObject->Present_Value = 0;
                datetime_local(
                    &pObject->Update_Time.date, &pObject->Update_Time.time,
                    NULL, NULL);
                pObject->Last_State_Change = TIMER_TRANSITION_EXPIRED_TO_IDLE;
                Timer_Write_Request_Initiate(pObject);
            } else if (pObject->Timer_State == TIMER_STATE_IDLE) {
                /* then no properties shall be changed;
                   no write requests shall be initiated;
                   and no state transition shall occur.*/
            }
            /* Writing a value other than IDLE to this property
               shall cause a Result(-) to be returned */
            status = true;
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, gets the timer
 *  running status.
 * @details
 *  12.57.11 Timer_Running
 *  This property, of type Boolean, shall have a value of TRUE if
 *  the current state of the timer is RUNNING, otherwise FALSE.
 *  This property may be used by other objects that require a
 *  simple Boolean flag for determining if the timer is in RUNNING state.
 * @param  object_instance - object-instance number of the object
 * @return TRUE if the current state of the timer is RUNNING, otherwise FALSE.
 */
bool Timer_Running(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Timer_State == TIMER_STATE_RUNNING) {
            status = true;
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the timer
 *  running status
 * @details Writing a value of TRUE to this property, in any timer state,
 *  shall be considered a start request. Present_Value shall be set to the
 *  value specified in the Default_Timeout property.
 *  Writing a value of FALSE to this property while the timer is in the
 *  RUNNING state shall be considered an expire request and shall force
 *  the timer to transition to state EXPIRED. When writing a value of FALSE
 *  to this property while the timer is in the EXPIRED or IDLE state,
 *  no transition of the timer state shall occur.
 * @param object_instance - object-instance number of the object
 * @param start - true if start is request, false if stop is requested
 * @return true if the running status was set
 */
bool Timer_Running_Set(uint32_t object_instance, bool start)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (start) {
            if (pObject->Timer_State == TIMER_STATE_IDLE) {
                /* If a value of TRUE is written to the Timer_Running property,
                then set Initial_Timeout to the value of Default_Timeout;
                set Timer_State to RUNNING;
                set Last_State_Change to IDLE_TO_RUNNING;
                set Present_Value to the value of Initial_Timeout;
                set Update_Time to the current date and time;
                initiate the write requests for the IDLE_TO_RUNNING
                transition if present; and enter the RUNNING state. */
                pObject->Timer_State = TIMER_STATE_RUNNING;
                pObject->Last_State_Change = TIMER_TRANSITION_IDLE_TO_RUNNING;
                pObject->Initial_Timeout = pObject->Default_Timeout;
                pObject->Present_Value = pObject->Initial_Timeout;
                datetime_local(
                    &pObject->Update_Time.date, &pObject->Update_Time.time,
                    NULL, NULL);
                Timer_Write_Request_Initiate(pObject);
            } else if (pObject->Timer_State == TIMER_STATE_RUNNING) {
                /* If a value of TRUE is written to the Timer_Running property,
                   then set Last_State_Change to RUNNING_TO_RUNNING;
                   set Initial_Timeout to the value of Default_Timeout;
                   set Present_Value to the value of Initial_Timeout;
                   set Update_Time to the current date and time;
                   initiate the write requests for the RUNNING_TO_RUNNING
                   transition if present; and enter the RUNNING state.*/
                pObject->Timer_State = TIMER_STATE_RUNNING;
                pObject->Last_State_Change =
                    TIMER_TRANSITION_RUNNING_TO_RUNNING;
                pObject->Initial_Timeout = pObject->Default_Timeout;
                pObject->Present_Value = pObject->Initial_Timeout;
                datetime_local(
                    &pObject->Update_Time.date, &pObject->Update_Time.time,
                    NULL, NULL);
                Timer_Write_Request_Initiate(pObject);
            } else if (pObject->Timer_State == TIMER_STATE_EXPIRED) {
                /* If a value of TRUE is written to the Timer_Running property,
                   set Timer_State to RUNNING;
                   set Last_State_Change to EXPIRED_TO_RUNNING;
                   set Initial_Timeout to the value of Default_Timeout;
                   set Present_Value to the value of Initial_Timeout;
                   set Update_Time to the current date and time;
                   initiate the write requests for the EXPIRED_TO_RUNNING
                   transition if present; and enter the RUNNING state. */
                pObject->Timer_State = TIMER_STATE_RUNNING;
                pObject->Last_State_Change =
                    TIMER_TRANSITION_EXPIRED_TO_RUNNING;
                pObject->Initial_Timeout = pObject->Default_Timeout;
                pObject->Present_Value = pObject->Initial_Timeout;
                datetime_local(
                    &pObject->Update_Time.date, &pObject->Update_Time.time,
                    NULL, NULL);
                Timer_Write_Request_Initiate(pObject);
            }
        } else {
            if (pObject->Timer_State == TIMER_STATE_RUNNING) {
                /*  Expire Request
                    If a value of FALSE is written to the Timer_Running
                    property,
                    set Timer_State to EXPIRED;
                    set Last_State_Change to FORCED_TO_EXPIRED;
                    set Present_Value to zero;
                    set Update_Time to the current date and time;
                    initiate the write requests for the FORCED_TO_EXPIRED
                    transition if present; and enter the EXPIRED state.*/
                pObject->Timer_State = TIMER_STATE_EXPIRED;
                pObject->Last_State_Change = TIMER_TRANSITION_FORCED_TO_EXPIRED;
                pObject->Present_Value = 0;
                datetime_local(
                    &pObject->Update_Time.date, &pObject->Update_Time.time,
                    NULL, NULL);
                Timer_Write_Request_Initiate(pObject);
            }
        }
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, loads the object-name into
 * a characterstring. Note that the object name must be unique
 * within this device.
 *
 * @param  object_instance - object-instance number of the object
 * @param  object_name - holds the object-name retrieved
 *
 * @return  true if object-name was retrieved
 */
bool Timer_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text[32] = "";
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                text, sizeof(text), "TIMER-%lu",
                (unsigned long)object_instance);
            status = characterstring_init_ansi(object_name, text);
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the object-name
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the object-name to be set
 * @return  true if object-name was set
 */
bool Timer_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        status = true;
        pObject->Object_Name = new_name;
    }

    return status;
}

/**
 * @brief Return the object name C string
 * @param object_instance [in] BACnet object instance number
 * @return object name or NULL if not found
 */
const char *Timer_Name_ASCII(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        name = pObject->Object_Name;
    }

    return name;
}

/**
 * For a given object instance-number, return the description.
 * @param  object_instance - object-instance number of the object
 * @param  description - description pointer
 * @return  true/false
 */
bool Timer_Description(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Description) {
            status =
                characterstring_init_ansi(description, pObject->Description);
        } else {
            status = characterstring_init_ansi(description, "");
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the description
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 * @return  true if string was set
 */
bool Timer_Description_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        status = true;
        pObject->Description = new_name;
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the description
 * @param  object_instance - object-instance number of the object
 * @return description text or NULL if not found
 */
const char *Timer_Description_ANSI(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Description == NULL) {
            name = "";
        } else {
            name = pObject->Description;
        }
    }

    return name;
}

/**
 * For a given object instance-number, returns the program change value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  program change property value
 */
BACNET_TIMER_TRANSITION Timer_Last_State_Change(uint32_t object_instance)
{
    BACNET_TIMER_TRANSITION change = TIMER_TRANSITION_NONE;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        change = pObject->Last_State_Change;
    }

    return change;
}

/**
 * For a given object instance-number, returns the out-of-service
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  out-of-service property value
 */
bool Timer_Out_Of_Service(uint32_t object_instance)
{
    struct object_data *pObject;
    bool value = false;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Out_Of_Service;
    }

    return value;
}

/**
 * For a given object instance-number, sets the out-of-service property value
 *
 * @param object_instance - object-instance number of the object
 * @param value - boolean out-of-service value
 *
 * @return true if the out-of-service property value was set
 */
void Timer_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        pObject->Out_Of_Service = value;
    }
}

/**
 * @brief For a given object instance-number, gets the reliability.
 * @param  object_instance - object-instance number of the object
 * @return reliability value
 */
BACNET_RELIABILITY Timer_Reliability(uint32_t object_instance)
{
    BACNET_RELIABILITY reliability = RELIABILITY_NO_FAULT_DETECTED;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        reliability = pObject->Reliability;
    }

    return reliability;
}

/**
 * @brief For a given object instance-number, gets the Fault status flag
 * @param  object_instance - object-instance number of the object
 * @return  true the status flag is in Fault
 */
static bool Timer_Fault(uint32_t object_instance)
{
    struct object_data *pObject;
    bool fault = false;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Reliability != RELIABILITY_NO_FAULT_DETECTED) {
            fault = true;
        }
    }

    return fault;
}

/**
 * @brief For a given object instance-number, sets the reliability
 * @param  object_instance - object-instance number of the object
 * @param  value - reliability enumerated value
 * @return  true if values are within range and property is set.
 */
bool Timer_Reliability_Set(uint32_t object_instance, BACNET_RELIABILITY value)
{
    struct object_data *pObject;
    bool status = false;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value <= 255) {
            pObject->Reliability = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Return the present-value for a specific object instance
 * @param object_instance [in] BACnet object instance number
 * @return the present-value for a specific object instance
 */
uint32_t Timer_Present_Value(uint32_t object_instance)
{
    uint32_t value = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Present_Value;
    }

    return value;
}

/**
 * @brief Set the present-value for a specific object instance
 * @details Writing a value to this property that is within the supported
 *  range, defined by Min_Pres_Value and Max_Pres_Value, shall force
 *  the timer to transition to the RUNNING state.
 *  The value written shall be used as the initial timeout
 *  and set into the Initial_Timeout property.
 *
 *  Writing a value of zero to this property while the timer
 *  is in the RUNNING state shall be considered an expire request
 *  and force the timer state to transition to state EXPIRED.
 *  If a value of zero is written to the property while the timer
 *  is in the EXPIRED or IDLE state, then no transition of the
 *  timer state shall occur.
 *
 * @param object_instance [in] BACnet object instance number
 * @return true if the present-value for a specific object instance was set
 */
bool Timer_Present_Value_Set(uint32_t object_instance, uint32_t value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value == 0) {
            /* If a value of zero is written to the Present_Value property */
            if ((pObject->Timer_State == TIMER_STATE_IDLE) ||
                (pObject->Timer_State == TIMER_STATE_EXPIRED)) {
                /* then no properties shall be changed;
                   no write requests shall be initiated;
                   and no state transition shall occur. */
            } else if (pObject->Timer_State == TIMER_STATE_RUNNING) {
                /* set Timer_State to EXPIRED;
                   set Last_State_Change to FORCED_TO_EXPIRED;
                   set Present_Value to zero;
                   set Update_Time to the current date and time;
                   initiate the write requests for
                   the FORCED_TO_EXPIRED transition if present;
                   and enter the EXPIRED state.*/
                pObject->Timer_State = TIMER_STATE_EXPIRED;
                pObject->Last_State_Change = TIMER_TRANSITION_FORCED_TO_EXPIRED;
                pObject->Present_Value = 0;
                datetime_local(
                    &pObject->Update_Time.date, &pObject->Update_Time.time,
                    NULL, NULL);
                Timer_Write_Request_Initiate(pObject);
            }
            status = true;
        } else {
            if ((value >= pObject->Min_Pres_Value) &&
                (value <= pObject->Max_Pres_Value)) {
                /* Start Request with Specific Timeout */
                /* If a value within the supported range is written
                   to the Present_Value property.
                   set Initial_Timeout to the value written to Present_Value;*/
                pObject->Present_Value = value;
                pObject->Initial_Timeout = value;
                if (pObject->Timer_State == TIMER_STATE_IDLE) {
                    /* set Last_State_Change to IDLE_TO_RUNNING; */
                    pObject->Last_State_Change =
                        TIMER_TRANSITION_IDLE_TO_RUNNING;
                } else if (pObject->Timer_State == TIMER_STATE_RUNNING) {
                    /* set Last_State_Change to RUNNING_TO_RUNNING; */
                    pObject->Last_State_Change =
                        TIMER_TRANSITION_RUNNING_TO_RUNNING;
                } else if (pObject->Timer_State == TIMER_STATE_EXPIRED) {
                    /* set Last_State_Change to EXPIRED_TO_RUNNING; */
                    pObject->Last_State_Change =
                        TIMER_TRANSITION_EXPIRED_TO_RUNNING;
                }
                /* set Timer_State to RUNNING;
                   set Update_Time to the current date and time;
                   initiate the write requests for the transition if present;
                   and enter the RUNNING state */
                pObject->Timer_State = TIMER_STATE_RUNNING;
                datetime_local(
                    &pObject->Update_Time.date, &pObject->Update_Time.time,
                    NULL, NULL);
                Timer_Write_Request_Initiate(pObject);
                status = true;
            } else {
                status = false;
            }
        }
    }

    return status;
}

/**
 * @brief Get the update-time property value for the object-instance specified
 * @param object_instance [in] BACnet object instance number
 * @return true if property value was retrieved
 */
bool Timer_Update_Time(uint32_t object_instance, BACNET_DATE_TIME *bdatetime)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        datetime_copy(bdatetime, &pObject->Update_Time);
        status = true;
    }

    return status;
}

/**
 * @brief Get the update-time property value for the object-instance specified
 * @param object_instance [in] BACnet object instance number
 * @return true if property value was retrieved
 */
bool Timer_Update_Time_Set(
    uint32_t object_instance, BACNET_DATE_TIME *bdatetime)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        datetime_copy(&pObject->Update_Time, bdatetime);
        status = true;
    }

    return status;
}

/**
 * @brief Get the expiration-time property value for the object-instance
 * specified
 * @details The Expiration_Time property shall indicate the date and time
 *  when the timer will expire. The value of Expiration_Time
 *  shall be calculated at the time the property is read.
 * @param object_instance [in] BACnet object instance number
 * @param bdatetime [out] the property value retrieved
 * @return true if property value was retrieved
 */
bool Timer_Expiration_Time(
    uint32_t object_instance, BACNET_DATE_TIME *bdatetime)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Timer_State == TIMER_STATE_RUNNING) {
            datetime_copy(bdatetime, &pObject->Update_Time);
            datetime_add_milliseconds(bdatetime, pObject->Present_Value);
        } else {
            /* set Expiration_Time to the unspecified datetime value */
            datetime_wildcard_set(bdatetime);
        }
        status = true;
    }

    return status;
}

/**
 * @brief Gets the initial-timeout property value for a given object instance
 * @param  object_instance - object-instance number of the object
 * @return initial-timeout property value
 */
uint32_t Timer_Initial_Timeout(uint32_t object_instance)
{
    uint32_t value = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Initial_Timeout;
    }

    return value;
}

/**
 * @brief Sets the initial-timeout property value for a given object instance
 * @param object_instance - object-instance number of the object
 * @param value - property value to set
 * @return true if the initial-timeout property value was set
 */
bool Timer_Initial_Timeout_Set(uint32_t object_instance, uint32_t value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        pObject->Initial_Timeout = value;
        status = true;
    }

    return status;
}

/**
 * @brief Gets the default-timeout property value for a given object instance
 * @param  object_instance - object-instance number of the object
 * @return default-timeout property value
 */
uint32_t Timer_Default_Timeout(uint32_t object_instance)
{
    uint32_t value = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Default_Timeout;
    }

    return value;
}

/**
 * @brief Sets the default-timeout property value for a given object instance
 * @param object_instance - object-instance number of the object
 * @param value - property value to set
 * @return true if the default-timeout property value was set
 */
bool Timer_Default_Timeout_Set(uint32_t object_instance, uint32_t value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        pObject->Default_Timeout = value;
        status = true;
    }

    return status;
}

/**
 * @brief Gets the min-pres-value property value for a given object instance
 * @param  object_instance - object-instance number of the object
 * @return min-pres-value property value
 */
uint32_t Timer_Min_Pres_Value(uint32_t object_instance)
{
    uint32_t value = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Min_Pres_Value;
    }

    return value;
}

/**
 * @brief Sets the min-pres-value property value for a given object instance
 * @param object_instance - object-instance number of the object
 * @param value - property value to set
 * @return true if the min-pres-value property value was set
 */
bool Timer_Min_Pres_Value_Set(uint32_t object_instance, uint32_t value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        pObject->Min_Pres_Value = value;
        status = true;
    }

    return status;
}

/**
 * @brief Gets the max-pres-value property value for a given object instance
 * @param  object_instance - object-instance number of the object
 * @return max-pres-value property value
 */
uint32_t Timer_Max_Pres_Value(uint32_t object_instance)
{
    uint32_t value = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Max_Pres_Value;
    }

    return value;
}

/**
 * @brief Sets the max-pres-value property value for a given object instance
 * @param object_instance - object-instance number of the object
 * @param value - property value to set
 * @return true if the max-pres-value property value was set
 */
bool Timer_Max_Pres_Value_Set(uint32_t object_instance, uint32_t value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        pObject->Max_Pres_Value = value;
        status = true;
    }

    return status;
}

/**
 * @brief Gets the resolution property value for a given object instance
 * @param  object_instance - object-instance number of the object
 * @return resolution property value
 */
uint32_t Timer_Resolution(uint32_t object_instance)
{
    uint32_t value = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Resolution;
    }

    return value;
}

/**
 * @brief Sets the resolution property value for a given object instance
 * @param object_instance - object-instance number of the object
 * @param value - property value to set
 * @return true if the resolution property value was set
 */
bool Timer_Resolution_Set(uint32_t object_instance, uint32_t value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        pObject->Resolution = value;
        status = true;
    }

    return status;
}

/**
 * @brief Gets the priority-for-writing property value for a given object
 * instance
 * @param  object_instance - object-instance number of the object
 * @return priority-for-writing property value
 */
uint8_t Timer_Priority_For_Writing(uint8_t object_instance)
{
    uint32_t value = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Priority_For_Writing;
    }

    return value;
}

/**
 * @brief Sets the priority-for-writing property value for a given object
 * instance
 * @param object_instance - object-instance number of the object
 * @param value - property value to set
 * @return true if the priority-for-writing property value was in range and set
 */
bool Timer_Priority_For_Writing_Set(uint32_t object_instance, uint8_t value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        /* Unsigned(1..16) */
        if ((value >= BACNET_MIN_PRIORITY) && (value <= BACNET_MAX_PRIORITY)) {
            pObject->Priority_For_Writing = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @details This property, of type BACnetARRAY[7]
 *  of BACnetTimerStateChangeValue, represents the values that
 *  are to be written to the referenced properties when a change
 *  of the timer state occurs.
 * Each of the elements 1-7 of this array may contain a value
 * to be written for the respective change of timer state.
 * The array index of the element is equal to the numerical value of the
 * BACnetTimerTransition enumeration for the respective timer state change.
 * The timer state change NONE has no corresponding array element.
 * @param object_instance [in] BACnet object instance number
 * @param index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
static int Timer_State_Change_Value_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        /* Note: The timer state change NONE=0
           has no corresponding array element.*/
        if (index < (TIMER_TRANSITION_MAX - 1)) {
            apdu_len = bacnet_timer_value_type_encode(
                apdu, &pObject->State_Change_Values[index]);
        }
    }

    return apdu_len;
}

/**
 * @brief Get the state-change value array element value
 * @param object_instance - BACnet object instance number
 * @param transition - the state-change enumeration requested
 * @return state-change structure or NULL if transition is out of range.
 */
BACNET_TIMER_STATE_CHANGE_VALUE *Timer_State_Change_Value(
    uint32_t object_instance, BACNET_TIMER_TRANSITION transition)
{
    BACNET_TIMER_STATE_CHANGE_VALUE *value = NULL;
    unsigned index;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        /* Note: The timer state change NONE=0
           has no corresponding array element.*/
        if ((transition != TIMER_TRANSITION_NONE) &&
            (transition < TIMER_TRANSITION_MAX)) {
            index = transition - 1;
            value = &pObject->State_Change_Values[index];
        }
    }

    return value;
}

/**
 * @brief Get the state-change value array element value
 * @param object_instance - BACnet object instance number
 * @param transition - the state-change enumeration requested
 * @param value - [out] state-change structure where value will be copied
 * @return true if the transition is in range and value was copied
 */
bool Timer_State_Change_Value_Get(
    uint32_t object_instance,
    BACNET_TIMER_TRANSITION transition,
    BACNET_TIMER_STATE_CHANGE_VALUE *value)
{
    bool status = false;
    unsigned index;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        /* Note: The timer state change NONE=0
           has no corresponding array element.*/
        if ((transition != TIMER_TRANSITION_NONE) &&
            (transition < TIMER_TRANSITION_MAX)) {
            index = transition - 1;
            status = bacnet_timer_value_copy(
                value, &pObject->State_Change_Values[index]);
        }
    }

    return status;
}

/**
 * @brief Set the state-change value array element value
 * @param object_instance - BACnet object instance number
 * @param transition - the state-change enumeration requested
 * @param value - state-change structure values
 * @return true if the transition is in range and value was copied
 */
bool Timer_State_Change_Value_Set(
    uint32_t object_instance,
    BACNET_TIMER_TRANSITION transition,
    BACNET_TIMER_STATE_CHANGE_VALUE *value)
{
    bool status = false;
    unsigned index;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        /* Note: The timer state change NONE=0
           has no corresponding array element.*/
        if ((transition != TIMER_TRANSITION_NONE) &&
            (transition < TIMER_TRANSITION_MAX)) {
            index = transition - 1;
            status = bacnet_timer_value_copy(
                &pObject->State_Change_Values[index], value);
        }
    }

    return status;
}

/**
 * ReadProperty handler for this object.  For the given ReadProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  rpdata - BACNET_READ_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 *
 * @return number of APDU bytes in the response, zero if no data, or
 * BACNET_STATUS_ERROR on error.
 */
int Timer_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    BACNET_DATE_TIME bdatetime;
    BACNET_UNSIGNED_INTEGER unsigned_value;
    size_t i;
    unsigned imax;
    uint8_t *apdu = NULL;
    int apdu_size;

    uint32_t enum_value = 0;
    bool state = false;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    apdu_size = rpdata->application_data_len;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], Object_Type, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Timer_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], Object_Type);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            state = Timer_Fault(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, state);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Timer_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_TIMER_STATE:
            enum_value = Timer_State(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], enum_value);
            break;
        case PROP_PRESENT_VALUE:
            unsigned_value = Timer_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], unsigned_value);
            break;
        case PROP_TIMER_RUNNING:
            state = Timer_Running(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_DESCRIPTION:
            if (Timer_Description(rpdata->object_instance, &char_string)) {
                apdu_len =
                    encode_application_character_string(&apdu[0], &char_string);
            }
            break;
        case PROP_RELIABILITY:
            enum_value = Timer_Reliability(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], enum_value);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Timer_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_UPDATE_TIME:
            Timer_Update_Time(rpdata->object_instance, &bdatetime);
            apdu_len = bacapp_encode_datetime(&apdu[0], &bdatetime);
            break;
        case PROP_LAST_STATE_CHANGE:
            enum_value = Timer_Last_State_Change(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], enum_value);
            break;
        case PROP_EXPIRATION_TIME:
            Timer_Expiration_Time(rpdata->object_instance, &bdatetime);
            apdu_len = bacapp_encode_datetime(&apdu[0], &bdatetime);
            break;
        case PROP_INITIAL_TIMEOUT:
            unsigned_value = Timer_Initial_Timeout(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], unsigned_value);
            break;
        case PROP_DEFAULT_TIMEOUT:
            unsigned_value = Timer_Default_Timeout(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], unsigned_value);
            break;
        case PROP_MIN_PRES_VALUE:
            unsigned_value = Timer_Min_Pres_Value(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], unsigned_value);
            break;
        case PROP_MAX_PRES_VALUE:
            unsigned_value = Timer_Max_Pres_Value(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], unsigned_value);
            break;
        case PROP_RESOLUTION:
            unsigned_value = Timer_Resolution(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], unsigned_value);
            break;
        case PROP_STATE_CHANGE_VALUES:
            /* note: The timer state change NONE=0 has no
               corresponding array element.*/
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Timer_State_Change_Value_Encode, TIMER_TRANSITION_MAX - 1, apdu,
                apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
            imax = Timer_Reference_List_Member_Element_Count(
                rpdata->object_instance);
            for (i = 0; i < imax; i++) {
                apdu_len += Timer_List_Of_Object_Property_References_Encode(
                    rpdata->object_instance, i, &apdu[apdu_len]);
            }
            break;
        case PROP_PRIORITY_FOR_WRITING:
            unsigned_value =
                Timer_Priority_For_Writing(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], unsigned_value);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    return apdu_len;
}

/**
 * @brief Decode a BACnetARRAY property element to determine the element length
 * @param object_instance [in] BACnet object instance number
 * @param apdu [in] Buffer in which the APDU contents are extracted
 * @param apdu_size [in] The size of the APDU buffer
 * @return The length of the decoded apdu, or BACNET_STATUS_ERROR on error
 */
static int Timer_State_Change_Value_Length(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    BACNET_TIMER_STATE_CHANGE_VALUE value = { 0 };
    int len = 0;

    (void)object_instance;
    len = bacnet_timer_value_decode(apdu, apdu_size, &value);

    return len;
}

/**
 * @brief Write a value to a BACnetARRAY property element value
 *  using a BACnetARRAY write utility function
 * @param object_instance [in] BACnet object instance number
 * @param array_index [in] array index to write:
 *    0=array size, 1 to N for individual array members
 * @param application_data [in] encoded element value
 * @param application_data_len [in] The size of the encoded element value
 * @return BACNET_ERROR_CODE value
 */
static BACNET_ERROR_CODE Timer_State_Change_Value_Write(
    uint32_t object_instance,
    BACNET_ARRAY_INDEX array_index,
    uint8_t *application_data,
    size_t application_data_len)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_TIMER_STATE_CHANGE_VALUE new_value = { 0 }, *current_value = NULL;
    int len = 0;
    bool status;

    if (array_index == 0) {
        /* fixed size array */
        error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
    } else if (array_index < TIMER_TRANSITION_MAX) {
        len = bacnet_timer_value_decode(
            application_data, application_data_len, &new_value);
        if (len > 0) {
            current_value =
                Timer_State_Change_Value(object_instance, array_index);
            status = bacnet_timer_value_copy(current_value, &new_value);
            if (status) {
                error_code = ERROR_CODE_SUCCESS;
            } else {
                error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
        } else {
            error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
    } else {
        error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
    }

    return error_code;
}

/**
 * @brief Decode a BACnetLIST property element to determine the element length
 * @param object_instance [in] BACnet object instance number
 * @param apdu [in] Buffer in which the APDU contents are extracted
 * @param apdu_size [in] The size of the APDU buffer
 * @return The length of the decoded apdu, or BACNET_STATUS_ERROR on error
 */
static int Timer_List_Of_Object_Property_References_Length(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    int len = 0;

    (void)object_instance;
    len = bacapp_decode_known_property(
        apdu, apdu_size, &value, OBJECT_TIMER,
        PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES);

    return len;
}

/**
 * @brief Add an element to a BACnetLIST property
 *  using a BACnetLIST add utility function
 * @param object_instance [in] BACnet object instance number
 * @param application_data [in] encoded element value
 * @param application_data_len [in] The size of the encoded element value
 * @return BACNET_ERROR_CODE value or ERROR_CODE_SUCCESS
 */
static BACNET_ERROR_CODE Timer_List_Of_Object_Property_References_Add(
    uint32_t object_instance,
    uint8_t *application_data,
    size_t application_data_len)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE new_value = { 0 };
    int len = 0;
    bool status = false;

    if ((!application_data) && (application_data_len == 0)) {
        /* empty the BACnetLIST - remove all before adding */
        (void)Timer_Reference_List_Member_Element_Remove(object_instance, NULL);
        error_code = ERROR_CODE_SUCCESS;
    } else {
        len = bacnet_device_object_property_reference_decode(
            application_data, application_data_len, &new_value);
        if (len > 0) {
            if (Timer_Reference_List_Member_Empty(&new_value)) {
                /* The element value is out of range for the property. */
                error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            } else {
                status = Timer_Reference_List_Member_Element_Add(
                    object_instance, &new_value);
                if (status) {
                    error_code = ERROR_CODE_SUCCESS;
                } else {
                    error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                }
            }
        } else {
            error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
    }

    return error_code;
}

/**
 * @brief Remove an element from a BACnetLIST property
 * @param object_instance [in] BACnet object instance number
 * @param application_data [in] encoded element value
 * @param application_data_len [in] The size of the encoded element value
 * @return BACNET_ERROR_CODE value or ERROR_CODE_SUCCESS
 */
static BACNET_ERROR_CODE Timer_List_Of_Object_Property_References_Remove(
    uint32_t object_instance,
    uint8_t *application_data,
    size_t application_data_len)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE new_value = { 0 };
    int len = 0;
    bool status = false;

    if ((!application_data) && (application_data_len == 0)) {
        /* nothing to remove */
        error_code = ERROR_CODE_SUCCESS;
    } else {
        len = bacnet_device_object_property_reference_decode(
            application_data, application_data_len, &new_value);
        if (len > 0) {
            if (Timer_Reference_List_Member_Empty(&new_value)) {
                error_code = ERROR_CODE_LIST_ELEMENT_NOT_FOUND;
            } else {
                status = Timer_Reference_List_Member_Element_Remove(
                    object_instance, &new_value);
                if (status) {
                    error_code = ERROR_CODE_SUCCESS;
                } else {
                    error_code = ERROR_CODE_LIST_ELEMENT_NOT_FOUND;
                }
            }
        } else {
            error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
    }

    return error_code;
}

/**
 * WriteProperty handler for this object.  For the given WriteProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  wp_data - BACNET_WRITE_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 *
 * @return false if an error is loaded, true if no errors
 */
bool Timer_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    /* decode the some of the request */
    len = bacapp_decode_known_array_property(
        wp_data->application_data, wp_data->application_data_len, &value,
        wp_data->object_type, wp_data->object_property, wp_data->array_index);
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                status = Timer_Present_Value_Set(
                    wp_data->object_instance, value.type.Unsigned_Int);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Timer_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_DEFAULT_TIMEOUT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                if (value.type.Unsigned_Int <= UINT32_MAX) {
                    status = Timer_Default_Timeout_Set(
                        wp_data->object_instance, value.type.Unsigned_Int);
                } else {
                    status = false;
                }
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_MIN_PRES_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                if (value.type.Unsigned_Int <= UINT32_MAX) {
                    status = Timer_Min_Pres_Value_Set(
                        wp_data->object_instance, value.type.Unsigned_Int);
                } else {
                    status = false;
                }
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_MAX_PRES_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                if (value.type.Unsigned_Int <= UINT32_MAX) {
                    status = Timer_Max_Pres_Value_Set(
                        wp_data->object_instance, value.type.Unsigned_Int);
                } else {
                    status = false;
                }
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_RESOLUTION:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                if (value.type.Unsigned_Int <= UINT32_MAX) {
                    status = Timer_Resolution_Set(
                        wp_data->object_instance, value.type.Unsigned_Int);
                } else {
                    status = false;
                }
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_PRIORITY_FOR_WRITING:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                if (value.type.Unsigned_Int <= UINT8_MAX) {
                    status = Timer_Priority_For_Writing_Set(
                        wp_data->object_instance, value.type.Unsigned_Int);
                } else {
                    status = false;
                }
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_STATE_CHANGE_VALUES:
            wp_data->error_code = bacnet_array_write(
                wp_data->object_instance, wp_data->array_index,
                Timer_State_Change_Value_Length, Timer_State_Change_Value_Write,
                TIMER_TRANSITION_MAX - 1, wp_data->application_data,
                wp_data->application_data_len);
            if (wp_data->error_code == ERROR_CODE_SUCCESS) {
                status = true;
            }
            break;
        case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
            /* a BACnetLIST can only be written all-at-once */
            wp_data->error_code = bacnet_list_write(
                wp_data->object_instance, wp_data->array_index,
                Timer_List_Of_Object_Property_References_Length,
                Timer_List_Of_Object_Property_References_Add,
                BACNET_TIMER_MANIPULATED_PROPERTIES_MAX,
                wp_data->application_data, wp_data->application_data_len);
            if (wp_data->error_code == ERROR_CODE_SUCCESS) {
                status = true;
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
            break;
    }

    return status;
}

/**
 * @brief Set the context used with load, unload, run, halt, and restart
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void *Timer_Context_Get(uint32_t object_instance)
{
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        return pObject->Context;
    }

    return NULL;
}

/**
 * @brief Set the context used for vendor specific extensions
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void Timer_Context_Set(uint32_t object_instance, void *context)
{
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        pObject->Context = context;
    }
}

/**
 * @brief Sets a callback used when the timer        is written from BACnet
 * @param cb - callback used to provide indications
 */
void Timer_Write_Property_Internal_Callback_Set(write_property_function cb)
{
    Write_Property_Internal_Callback = cb;
}

/**
 * @brief Updates the object program operation
 * @details In the RUNNING state, the timer is active
 *  and is counting down the remaining time.
 *  The Present_Value property shall indicate the
 *  remaining time until expiration.
 *
 * @param  object_instance - object-instance number of the object
 * @param milliseconds - number of milliseconds elapsed
 */
void Timer_Task(uint32_t object_instance, uint16_t milliseconds)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        switch (pObject->Timer_State) {
            case TIMER_STATE_RUNNING:
                if (pObject->Present_Value > milliseconds) {
                    pObject->Present_Value -= milliseconds;
                } else {
                    /* expired */
                    pObject->Present_Value = 0;
                    pObject->Timer_State = TIMER_STATE_EXPIRED;
                    pObject->Last_State_Change =
                        TIMER_TRANSITION_RUNNING_TO_EXPIRED;
                }
                break;
            case TIMER_STATE_EXPIRED:
            case TIMER_STATE_IDLE:
            default:
                /* do nothing */
                break;
        }
    }
}

/**
 * @brief AddListElement to a list property
 * @param list_element [in] Pointer to the BACnet_List_Element_Data structure,
 * which is packed with the information from the request.
 * @return #BACNET_STATUS_OK or #BACNET_STATUS_ERROR or
 * #BACNET_STATUS_ABORT or #BACNET_STATUS_REJECT
 */
int Timer_Add_List_Element(BACNET_LIST_ELEMENT_DATA *list_element)
{
    if (!list_element) {
        return BACNET_STATUS_ABORT;
    }
    list_element->error_class = ERROR_CLASS_PROPERTY;
    if (list_element->object_property ==
        PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES) {
        if (list_element->array_index != BACNET_ARRAY_ALL) {
            /* An array index is provided but the property is
            not a BACnetARRAY of BACnetLIST.*/
            list_element->error_class = ERROR_CLASS_PROPERTY;
            list_element->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        } else {
            list_element->error_code =
                Timer_List_Of_Object_Property_References_Add(
                    list_element->object_instance,
                    list_element->application_data,
                    list_element->application_data_len);
            if (list_element->error_code == ERROR_CODE_SUCCESS) {
                return BACNET_STATUS_OK;
            }
            if (list_element->error_code ==
                ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY) {
                list_element->error_class = ERROR_CLASS_RESOURCES;
                list_element->error_code =
                    ERROR_CODE_NO_SPACE_TO_ADD_LIST_ELEMENT;
            }
        }
    } else {
        list_element->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
    }

    return BACNET_STATUS_ERROR;
}

/**
 * @brief RemoveListElement to a list property
 * @param list_element [in] Pointer to the BACnet_List_Element_Data structure,
 * which is packed with the information from the request.
 * @return #BACNET_STATUS_OK or #BACNET_STATUS_ERROR or
 * #BACNET_STATUS_ABORT or #BACNET_STATUS_REJECT
 */
int Timer_Remove_List_Element(BACNET_LIST_ELEMENT_DATA *list_element)
{
    if (!list_element) {
        return BACNET_STATUS_ABORT;
    }
    list_element->error_class = ERROR_CLASS_PROPERTY;
    if (list_element->object_property ==
        PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES) {
        if (list_element->array_index != BACNET_ARRAY_ALL) {
            /* An array index is provided but the property is
            not a BACnetARRAY of BACnetLIST.*/
            list_element->error_class = ERROR_CLASS_PROPERTY;
            list_element->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        } else {
            list_element->error_code =
                Timer_List_Of_Object_Property_References_Remove(
                    list_element->object_instance,
                    list_element->application_data,
                    list_element->application_data_len);
            if (list_element->error_code == ERROR_CODE_SUCCESS) {
                return BACNET_STATUS_OK;
            }
            if (list_element->error_code == ERROR_CODE_LIST_ELEMENT_NOT_FOUND) {
                list_element->error_class = ERROR_CLASS_SERVICES;
            }
        }
    } else {
        list_element->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
    }

    return BACNET_STATUS_ERROR;
}

/**
 * @brief Creates a Timer object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Timer_Create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
    int index;
    unsigned i;

    if (!Object_List) {
        Object_List = Keylist_Create();
    }
    if (object_instance > BACNET_MAX_INSTANCE) {
        return BACNET_MAX_INSTANCE;
    } else if (object_instance == BACNET_MAX_INSTANCE) {
        /* wildcard instance */
        /* the Object_Identifier property of the newly created object
            shall be initialized to a value that is unique within the
            responding BACnet-user device. The method used to generate
            the object identifier is a local matter.*/
        object_instance = Keylist_Next_Empty_Key(Object_List, 1);
    }
    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        /* already exists - signal success but don't change data */
        return object_instance;
    }
    pObject = calloc(1, sizeof(struct object_data));
    if (!pObject) {
        /* no RAM available - signal failure */
        return BACNET_MAX_INSTANCE;
    }
    index = Keylist_Data_Add(Object_List, object_instance, pObject);
    if (index < 0) {
        /* unable to add to list - signal failure */
        free(pObject);
        return BACNET_MAX_INSTANCE;
    }
    pObject->Timer_State = TIMER_STATE_IDLE;
    pObject->Last_State_Change = TIMER_TRANSITION_NONE;
    datetime_wildcard_set(&pObject->Update_Time);
    pObject->Initial_Timeout = 0;
    pObject->Default_Timeout = 1000;
    pObject->Min_Pres_Value = 1;
    pObject->Max_Pres_Value = UINT32_MAX;
    pObject->Resolution = 1;
    pObject->Priority_For_Writing = BACNET_MAX_PRIORITY;
    pObject->Description = NULL;
    pObject->Object_Name = NULL;
    pObject->Reliability = RELIABILITY_NO_FAULT_DETECTED;
    pObject->Out_Of_Service = false;
    pObject->Changed = false;
    pObject->Context = NULL;
    for (i = 0; i < BACNET_TIMER_MANIPULATED_PROPERTIES_MAX; i++) {
        List_Of_Object_Property_References_Set(pObject, i, NULL);
    }

    return object_instance;
}

/**
 * @brief Deletes an object-instance
 * @param object_instance - object-instance number of the object
 * @return true if the object-instance was deleted
 */
bool Timer_Delete(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject =
        Keylist_Data_Delete(Object_List, object_instance);

    if (pObject) {
        free(pObject);
        status = true;
    }

    return status;
}

/**
 * @brief Deletes all the objects and their data
 */
void Timer_Cleanup(void)
{
    struct object_data *pObject;

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

/**
 * Initializes the object data
 */
void Timer_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
