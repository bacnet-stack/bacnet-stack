/**
 * @file
 * @author Steve Karg
 * @date 2005
 * @brief A basic BACnet File Object implementation.
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/bacdcode.h"
#include "bacnet/datetime.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/arf.h"
#include "bacnet/awf.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/basic/tsm/tsm.h"

struct object_data {
    char *Object_Name;
    char *Pathname;
    char *File_Type;
    void *Context;
    BACNET_DATE_TIME Modification_Date;
    bool File_Access_Stream : 1;
    bool Read_Only : 1;
    bool Archive : 1;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_FILE;
/* These three arrays are used by the ReadPropertyMultiple handler */
static const int32_t Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_FILE_TYPE,
    PROP_FILE_SIZE,
    PROP_MODIFICATION_DATE,
    PROP_ARCHIVE,
    PROP_READ_ONLY,
    PROP_FILE_ACCESS_METHOD,
    -1
};

static const int32_t Properties_Optional[] = {
    /* unordered list of optional properties */
    PROP_DESCRIPTION, -1
};

static const int32_t Properties_Proprietary[] = { -1 };

/**
 * @brief Returns the list of required, optional, and proprietary properties.
 * Used by ReadPropertyMultiple service.
 * @param pRequired - pointer to list of int terminated by -1, of
 * BACnet required properties for this object.
 * @param pOptional - pointer to list of int terminated by -1, of
 * BACnet optional properties for this object.
 * @param pProprietary - pointer to list of int terminated by -1, of
 * BACnet proprietary properties for this object.
 */
void BACfile_Property_Lists(
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
 * @brief duplicate a string (replacement for POSIX strdup)
 * @param  s - string to duplicate
 * @return a pointer to a new string on success, or a null pointer
 */
static char *bacfile_strdup(const char *s)
{
    size_t size = strlen(s) + 1;
    char *p = malloc(size);
    if (p != NULL) {
        memcpy(p, s, size);
    }
    return p;
}

/**
 * @brief For a given object instance-number, returns the pathname
 * @param  object_instance - object-instance number of the object
 * @return  internal file system path and name, or NULL if not set
 */
const char *bacfile_pathname(uint32_t object_instance)
{
    struct object_data *pObject;
    const char *pathname = NULL;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pathname = pObject->Pathname;
    }

    return pathname;
}

/**
 * @brief For a given object instance-number, sets the pathname
 * @param  object_instance - object-instance number of the object
 * @param  pathname - internal file system path and name
 */
void bacfile_pathname_set(uint32_t object_instance, const char *pathname)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        free(pObject->Pathname);
        pObject->Pathname = bacfile_strdup(pathname);
    }
}

/**
 * @brief For a given pathname, gets the object instance-number
 * @param pathname - internal file system path and name
 * @return object-instance number of the object,
 *  or #BACNET_MAX_INSTANCE if not found
 */
uint32_t bacfile_pathname_instance(const char *pathname)
{
    struct object_data *pObject;
    int count = 0;
    int index = 0;
    KEY key = BACNET_MAX_INSTANCE;

    count = Keylist_Count(Object_List);
    while (count) {
        pObject = Keylist_Data_Index(Object_List, index);
        if (pObject->Pathname) {
            if (strcmp(pathname, pObject->Pathname) == 0) {
                Keylist_Index_Key(Object_List, index, &key);
                break;
            }
        }
        count--;
        index++;
    }

    return key;
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
bool bacfile_object_name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    struct object_data *pObject;
    char name_text[32];

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                name_text, sizeof(name_text), "FILE %lu",
                (unsigned long)object_instance);
            status = characterstring_init_ansi(object_name, name_text);
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the object-name
 * Note that the object name must be unique within this device.
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the object-name to be set
 *
 * @return  true if object-name was set
 */
bool bacfile_object_name_set(uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = true;
        free(pObject->Object_Name);
        pObject->Object_Name = bacfile_strdup(new_name);
    }

    return status;
}

/**
 * @brief Return the object name C string
 * @param object_instance [in] BACnet object instance number
 * @return object name or NULL if not found
 */
const char *bacfile_name_ansi(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        name = pObject->Object_Name;
    }

    return name;
}

/**
 * @brief Determines if a given object instance is valid
 * @param  object_instance - object-instance number of the object
 * @return  true if the instance is valid, and false if not
 */
bool bacfile_valid_instance(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * @brief Determines the number of objects
 * @return  Number of objects
 */
unsigned bacfile_count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * @brief Determines the object instance-number for a given 0..N index
 * of objects where N is the object count
 * @param  find_index - 0..N value
 * @return  object instance-number for the given index
 */
uint32_t bacfile_index_to_instance(unsigned find_index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, find_index, &key);

    return key;
}

/**
 * @brief Callback function to write record data
 */
static bool (*bacfile_write_record_data_cb)(
    const char *, int32_t, size_t, const uint8_t *, size_t) = NULL;

/**
 * @brief Callback function to write record data
 *
 * @param pathname - name of the file to write to
 * @param fileStartRecord - starting record number in the file
 * @param record_index - index of the record to write
 * @param buffer - data buffer to write
 * @param buffer_size - size of the data buffer
 * @return true if the record data was written successfully
 * @return false if the record data could not be written
 */
static bool bacfile_write_record_data_callback(
    const char *pathname,
    int32_t fileStartRecord,
    size_t record_index,
    const uint8_t *buffer,
    size_t buffer_size)
{
    if (bacfile_write_record_data_cb) {
        return bacfile_write_record_data_cb(
            pathname, fileStartRecord, record_index, buffer, buffer_size);
    }

    return false;
}

/**
 * @brief Sets the callback function for writing record data
 * @param callback - function pointer to the callback
 */
void bacfile_write_record_data_callback_set(
    bool (*callback)(const char *, int32_t, size_t, const uint8_t *, size_t))
{
    bacfile_write_record_data_cb = callback;
}

/**
 * @brief Callback function to read record data
 */
static bool (*bacfile_read_record_data_cb)(
    const char *, int32_t, size_t, uint8_t *, size_t) = NULL;

/**
 * @brief Callback function to read record data
 * @param pathname - name of the file to read from
 * @param fileStartRecord - starting record number in the file
 * @param record_index - index of the record to read
 * @param buffer - data buffer to read into
 * @param buffer_size - size of the data buffer
 * @return true if the record data was read successfully
 * @return false if the record data could not be read
 */
static bool bacfile_read_record_data_callback(
    const char *pathname,
    int32_t fileStartRecord,
    size_t record_index,
    uint8_t *buffer,
    size_t buffer_size)
{
    if (bacfile_read_record_data_cb) {
        return bacfile_read_record_data_cb(
            pathname, fileStartRecord, record_index, buffer, buffer_size);
    }

    return false;
}

/**
 * @brief Sets the callback function for reading record data
 * @param callback - function pointer to the callback
 */
void bacfile_read_record_data_callback_set(
    bool (*callback)(const char *, int32_t, size_t, uint8_t *, size_t))
{
    bacfile_read_record_data_cb = callback;
}

/**
 * @brief Callback function to write stream data
 */
static size_t (*bacfile_write_stream_data_cb)(
    const char *, int32_t, const uint8_t *, size_t) = NULL;

/**
 * @brief Callback function to write stream data
 * @param pathname - name of the file to write to
 * @param fileStartPosition - starting position in the file
 * @param buffer - data buffer to write
 * @param buffer_size - size of the data buffer
 * @return number of bytes written, or 0 if not successful
 */
static size_t bacfile_write_stream_data_callback(
    const char *pathname,
    int32_t fileStartPosition,
    const uint8_t *buffer,
    size_t buffer_size)
{
    if (bacfile_write_stream_data_cb) {
        return bacfile_write_stream_data_cb(
            pathname, fileStartPosition, buffer, buffer_size);
    }

    return 0;
}

/**
 * @brief Sets the callback function for writing stream data
 * @param callback - function pointer to the callback
 */
void bacfile_write_stream_data_callback_set(
    size_t (*callback)(const char *, int32_t, const uint8_t *, size_t))
{
    bacfile_write_stream_data_cb = callback;
}

/**
 * @brief Callback function to read stream data
 */
static size_t (*bacfile_read_stream_data_cb)(
    const char *, int32_t, uint8_t *, size_t) = NULL;

/**
 * @brief Callback function to read stream data
 * @param pathname - name of the file to read from
 * @param fileStartPosition - starting position in the file
 * @param buffer - data buffer to read into
 * @param buffer_size - size of the data buffer and number of bytes to read
 * @return number of bytes read, or 0 if not successful
 */
static size_t bacfile_read_stream_data_callback(
    const char *pathname,
    int32_t fileStartPosition,
    uint8_t *buffer,
    size_t buffer_size)
{
    if (bacfile_read_stream_data_cb) {
        return bacfile_read_stream_data_cb(
            pathname, fileStartPosition, buffer, buffer_size);
    }

    return 0;
}

/**
 * @brief Sets the callback function for reading stream data
 * @param callback - function pointer to the callback
 */
void bacfile_read_stream_data_callback_set(
    size_t (*callback)(const char *, int32_t, uint8_t *, size_t))
{
    bacfile_read_stream_data_cb = callback;
}

/**
 * @brief Callback function to get file size
 */
static size_t (*bacfile_file_size_cb)(const char *) = NULL;

/**
 * @brief Callback function to get file size
 * @param pathname - name of the file to get the size of
 * @return size of the file in bytes, or 0 if not found
 */
static size_t bacfile_file_size_callback(const char *pathname)
{
    size_t file_size = 0;

    if (bacfile_file_size_cb) {
        file_size = bacfile_file_size_cb(pathname);
    }

    return file_size;
}

/**
 * @brief Sets the callback function for getting file size
 * @param callback - function pointer to the callback
 */
void bacfile_file_size_callback_set(size_t (*callback)(const char *))
{
    bacfile_file_size_cb = callback;
}

/**
 * @brief Callback function to set file size
 */
static bool (*bacfile_file_size_set_cb)(const char *, size_t) = NULL;

/**
 * @brief Callback function to set file size
 * @param pathname - name of the file to set the size for
 * @param file_size - value of the file size property
 * @return true if file size is writable, false otherwise
 */
bool bacfile_file_size_set_callback(const char *pathname, size_t file_size)
{
    if (bacfile_file_size_set_cb) {
        return bacfile_file_size_set_cb(pathname, file_size);
    }

    return false;
}

/**
 * @brief Sets the callback function for setting file size
 * @param callback - function pointer to the callback
 */
void bacfile_file_size_set_callback_set(bool (*callback)(const char *, size_t))
{
    bacfile_file_size_set_cb = callback;
}

/**
 * @brief Read the entire file into a buffer
 * @param  object_instance - object-instance number of the object
 * @param  buffer - data store from the file
 * @param  buffer_size - in bytes
 * @return  file size in bytes
 */
uint32_t
bacfile_read(uint32_t object_instance, uint8_t *buffer, uint32_t buffer_size)
{
    const char *pathname = NULL;
    long file_size = 0;

    pathname = bacfile_pathname(object_instance);
    if (pathname) {
        file_size =
            bacfile_read_stream_data_callback(pathname, 0, buffer, buffer_size);
    }

    return (uint32_t)file_size;
}

/**
 * @brief Write the entire file from a buffer
 * @param  object_instance - object-instance number of the object
 * @param  buffer - data store for the file
 * @param  buffer_size - in bytes
 * @return  file size in bytes
 */
uint32_t bacfile_write(
    uint32_t object_instance, const uint8_t *buffer, uint32_t buffer_size)
{
    const char *pathname = NULL;
    long file_size = 0;

    pathname = bacfile_pathname(object_instance);
    if (pathname) {
        file_size = bacfile_write_stream_data_callback(
            pathname, 0, buffer, buffer_size);
    }

    return (uint32_t)file_size;
}

/**
 * @brief Determines the file size for a given file
 * @param  pFile - file handle
 * @return  file size in bytes, or 0 if not found
 */
BACNET_UNSIGNED_INTEGER bacfile_file_size(uint32_t object_instance)
{
    const char *pathname = NULL;
    BACNET_UNSIGNED_INTEGER file_size = 0;

    pathname = bacfile_pathname(object_instance);
    if (pathname) {
        file_size =
            (BACNET_UNSIGNED_INTEGER)bacfile_file_size_callback(pathname);
    }

    return file_size;
}

/**
 * @brief Sets the file size property value
 * @param object_instance - object-instance number of the object
 * @param file_size - value of the file size property
 * @return true if file size is writable
 */
bool bacfile_file_size_set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER file_size)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->File_Access_Stream) {
            status =
                bacfile_file_size_set_callback(pObject->Pathname, file_size);
        }
    }

    return status;
}

/**
 * @brief Determines the file size property value
 * @param object_instance - object-instance number of the object
 * @return value of the file size property
 */
const char *bacfile_file_type(uint32_t object_instance)
{
    struct object_data *pObject;
    const char *mime_type = "application/octet-stream";

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->File_Type) {
            mime_type = pObject->File_Type;
        }
    }

    return mime_type;
}

/**
 * @brief Sets the file type (MIME) property value
 * @param object_instance - object-instance number of the object
 * @param mime_type - value of the file type property
 */
void bacfile_file_type_set(uint32_t object_instance, const char *mime_type)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        free(pObject->File_Type);
        pObject->File_Type = bacfile_strdup(mime_type);
    }
}

/**
 * @brief For a given object instance-number, return the flag
 * @note 12.13.8 Archive
 *  This property, of type BOOLEAN, indicates whether the File
 *  object has been saved for historical or backup purposes. This
 *  property shall be logical TRUE only if no changes have been
 *  made to the file data by internal processes or through File
 *  Access Services since the last time the object was archived.
 * @param  object_instance - object-instance number of the object
 * @return  true if the property is true
 */
bool bacfile_archive(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = pObject->Archive;
    }

    return status;
}

/**
 * @brief For a given object instance-number, return the flag
 * @note 12.13.8 Archive
 *  This property, of type BOOLEAN, indicates whether the File
 *  object has been saved for historical or backup purposes. This
 *  property shall be logical TRUE only if no changes have been
 *  made to the file data by internal processes or through File
 *  Access Services since the last time the object was archived.
 * @param  object_instance - object-instance number of the object
 * @return  true if the property is true
 */
bool bacfile_archive_set(uint32_t object_instance, bool archive)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Archive = archive;
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, return the flag
 * @param  object_instance - object-instance number of the object
 * @return  true if the property is true
 */
bool bacfile_read_only(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = pObject->Read_Only;
    }

    return status;
}

/**
 * @brief Sets the file archive property value
 * @param object_instance - object-instance number of the object
 * @param archive - value of the file archive property
 * @return true if the file exists and read-only is writeable
 */
bool bacfile_read_only_set(uint32_t object_instance, bool read_only)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Read_Only = read_only;
        status = true;
    }

    return status;
}

/**
 * @brief
 * @param  object_instance - object-instance number of the object
 * @param  bdatetime
 */
static void
bacfile_modification_date(uint32_t object_instance, BACNET_DATE_TIME *bdatetime)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        datetime_copy(bdatetime, &pObject->Modification_Date);
    }
}

/**
 * @brief For a given object instance-number, return the flag
 * @param  object_instance - object-instance number of the object
 * @return  true if the property is true
 */
bool bacfile_file_access_stream(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = pObject->File_Access_Stream;
    }

    return status;
}

/**
 * @brief Sets the file access property value
 * @param object_instance - object-instance number of the object
 * @param access - value of the property
 * @return true if the file exists and property is writeable
 */
bool bacfile_file_access_stream_set(uint32_t object_instance, bool access)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->File_Access_Stream = access;
        status = true;
    }

    return status;
}

/**
 * @brief ReadProperty handler for this object.  For the given ReadProperty
 * data, the application_data is loaded or the error flags are set.
 * @param  rpdata - BACNET_READ_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 * @return number of APDU bytes in the response, or
 * BACNET_STATUS_ERROR on error.
 */
int bacfile_read_property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_CHARACTER_STRING char_string;
    BACNET_DATE_TIME bdatetime;
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_FILE, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            bacfile_object_name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], Object_Type);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string, bacfile_pathname(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_FILE_TYPE:
            characterstring_init_ansi(
                &char_string, bacfile_file_type(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_FILE_SIZE:
            apdu_len = encode_application_unsigned(
                &apdu[0], bacfile_file_size(rpdata->object_instance));
            break;
        case PROP_MODIFICATION_DATE:
            bacfile_modification_date(rpdata->object_instance, &bdatetime);
            apdu_len = bacapp_encode_datetime(apdu, &bdatetime);
            break;
        case PROP_ARCHIVE:
            apdu_len = encode_application_boolean(
                &apdu[0], bacfile_archive(rpdata->object_instance));
            break;
        case PROP_READ_ONLY:
            apdu_len = encode_application_boolean(
                &apdu[0], bacfile_read_only(rpdata->object_instance));
            break;
        case PROP_FILE_ACCESS_METHOD:
            if (bacfile_file_access_stream(rpdata->object_instance)) {
                apdu_len =
                    encode_application_enumerated(&apdu[0], FILE_STREAM_ACCESS);
            } else {
                apdu_len =
                    encode_application_enumerated(&apdu[0], FILE_RECORD_ACCESS);
            }
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
 * @brief WriteProperty handler for this object.  For the given WriteProperty
 * data, the application_data is loaded or the error flags are set.
 * @param  wp_data - BACNET_WRITE_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 * @return false if an error is loaded, true if no errors
 */
bool bacfile_write_property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    if (!bacfile_valid_instance(wp_data->object_instance)) {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    /* decode the some of the request */
    len = bacapp_decode_application_data(
        wp_data->application_data, wp_data->application_data_len, &value);
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    /* FIXME: len < application_data_len: more data? */
    switch (wp_data->object_property) {
        case PROP_ARCHIVE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                status = bacfile_archive_set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_FILE_SIZE:
            /* If the file size can be changed by writing to the file,
               and File_Access_Method is STREAM_ACCESS, then this property
               shall be writable. */
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                status = bacfile_file_size_set(
                    wp_data->object_instance, value.type.Unsigned_Int);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                }
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

#if MAX_TSM_TRANSACTIONS
/* this is one way to match up the invoke ID with */
/* the file ID from the AtomicReadFile request. */
/* Another way would be to store the */
/* invokeID and file instance in a list or table */
/* when the request was sent */
uint32_t bacfile_instance_from_tsm(uint8_t invokeID)
{
    BACNET_NPDU_DATA npdu_data = { 0 }; /* dummy for getting npdu length */
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    uint8_t service_choice = 0;
    uint8_t *service_request = NULL;
    uint16_t service_request_len = 0;
    BACNET_ADDRESS dest; /* where the original packet was destined */
    uint8_t apdu[MAX_PDU] = { 0 }; /* original APDU packet */
    uint16_t apdu_len = 0; /* original APDU packet length */
    int len = 0; /* apdu header length */
    BACNET_ATOMIC_READ_FILE_DATA data = { 0 };
    uint32_t object_instance = BACNET_MAX_INSTANCE + 1; /* return value */
    bool found = false;

    found = tsm_get_transaction_pdu(
        invokeID, &dest, &npdu_data, &apdu[0], &apdu_len);
    if (found) {
        if (!npdu_data.network_layer_message &&
            npdu_data.data_expecting_reply &&
            ((apdu[0] & 0xF0) == PDU_TYPE_CONFIRMED_SERVICE_REQUEST)) {
            len = apdu_decode_confirmed_service_request(
                &apdu[0], apdu_len, &service_data, &service_choice,
                &service_request, &service_request_len);
            if ((len > 0) &&
                (service_choice == SERVICE_CONFIRMED_ATOMIC_READ_FILE)) {
                len = arf_decode_service_request(
                    service_request, service_request_len, &data);
                if (len > 0) {
                    if (data.object_type == OBJECT_FILE) {
                        object_instance = data.object_instance;
                    }
                }
            }
        }
    }

    return object_instance;
}
#endif

bool bacfile_read_stream_data(BACNET_ATOMIC_READ_FILE_DATA *data)
{
    const char *pathname = NULL;
    bool found = false;
    size_t len = 0;
    size_t requestedOctetCount = 0;

    pathname = bacfile_pathname(data->object_instance);
    if (pathname) {
        found = true;
        requestedOctetCount = data->type.stream.requestedOctetCount;
        if (requestedOctetCount > octetstring_capacity(&data->fileData[0])) {
            requestedOctetCount = octetstring_capacity(&data->fileData[0]);
        }
        len = bacfile_read_stream_data_callback(
            pathname, data->type.stream.fileStartPosition,
            octetstring_value(&data->fileData[0]), requestedOctetCount);
        if (len < requestedOctetCount) {
            data->endOfFile = true;
        } else {
            data->endOfFile = false;
        }
        octetstring_truncate(&data->fileData[0], len);
    } else {
        octetstring_truncate(&data->fileData[0], 0);
        data->endOfFile = true;
    }

    return found;
}

bool bacfile_read_record_data(BACNET_ATOMIC_READ_FILE_DATA *data)
{
    const char *pathname = NULL;
    bool found = false;
    bool status = false;
    uint32_t i = 0;

    pathname = bacfile_pathname(data->object_instance);
    if (pathname) {
        found = true;
        data->endOfFile = false;
        for (i = 0; i < data->type.record.RecordCount; i++) {
            status = bacfile_read_record_data_callback(
                pathname, data->type.record.fileStartRecord, i,
                octetstring_value(&data->fileData[i]),
                octetstring_capacity(&data->fileData[i]));
            if (!status) {
                data->endOfFile = true;
                data->type.record.RecordCount = i;
                break;
            }
        }
    }

    return found;
}

/**
 * @brief Write the data received to the file specified
 * @param data - pointer to the data to write
 * @return true - if successful
 * @return false - if failed or access denied
 */
bool bacfile_write_stream_data(BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    const char *pathname = NULL;
    bool status = false;
    size_t bytes_written = 0;

    if (bacfile_read_only(data->object_instance)) {
        /* if the file is read-only, then we cannot write to it */
        return false;
    }
    pathname = bacfile_pathname(data->object_instance);
    if (pathname) {
        status = true;
        /* note: If 'File Start Position' parameter has the special
           value -1, then the write operation shall be treated
           as an append to the current end of file.
           If the 'File Start Position' parameter is 0,
           open the file as a clean slate. */
        bytes_written = bacfile_write_stream_data_callback(
            pathname, data->type.stream.fileStartPosition,
            octetstring_value(&data->fileData[0]),
            octetstring_length(&data->fileData[0]));
        if (bytes_written == 0) {
            status = false; /* no data written */
        }
    }

    return status;
}

/**
 * @brief Write the data received to the file specified
 * @param data - pointer to the data to write
 * @return true - if successful
 * @return false - if failed or access denied
 */
bool bacfile_write_record_data(const BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    const char *pathname = NULL;
    bool found = false;
    size_t i = 0;

    if (bacfile_read_only(data->object_instance)) {
        /* if the file is read-only, then we cannot write to it */
        return false;
    }
    pathname = bacfile_pathname(data->object_instance);
    if (pathname) {
        found = true;
        /* If 'File Start Record' parameter has the special
            value -1, then the write operation shall be treated
            as an append to the current end of file.
            If the 'File Start Record' parameter is 0,
            open the file as a clean slate. */
        for (i = 0; i < data->type.record.returnedRecordCount; i++) {
            bacfile_write_record_data_callback(
                pathname, data->type.record.fileStartRecord, i,
                octetstring_value((BACNET_OCTET_STRING *)&data->fileData[i]),
                octetstring_length(&data->fileData[i]));
        }
    }

    return found;
}

/**
 * @brief Write the requested data received into the file specified
 * @param instance - object-instance number of the object
 * @param data - pointer to the data to write
 * @return true - if successful
 * @return false - if failed
 */
bool bacfile_read_ack_stream_data(
    uint32_t instance, const BACNET_ATOMIC_READ_FILE_DATA *data)
{
    bool found = false;
    const char *pathname = NULL;

    pathname = bacfile_pathname(instance);
    if (pathname) {
        found = true;
        bacfile_write_stream_data_callback(
            pathname, data->type.stream.fileStartPosition,
            octetstring_value((BACNET_OCTET_STRING *)&data->fileData[0]),
            octetstring_length(&data->fileData[0]));
    }

    return found;
}

/**
 * @brief Write the requested data received into the file specified
 * @param instance - object-instance number of the object
 * @param data - pointer to the data to write
 * @return true - if successful
 * @return false - if failed
 */
bool bacfile_read_ack_record_data(
    uint32_t instance, const BACNET_ATOMIC_READ_FILE_DATA *data)
{
    bool found = false;
    const char *pathname = NULL;
    uint32_t i = 0;

    pathname = bacfile_pathname(instance);
    if (pathname) {
        found = true;
        for (i = 0; i < data->type.record.RecordCount; i++) {
            bacfile_write_record_data_callback(
                pathname, data->type.record.fileStartRecord, i,
                octetstring_value((BACNET_OCTET_STRING *)&data->fileData[i]),
                octetstring_length(&data->fileData[i]));
        }
    }

    return found;
}

/**
 * @brief Set the context used with a specific object instance
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void *bacfile_create_context_get(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        return pObject->Context;
    }

    return NULL;
}

/**
 * @brief Set the context used with a specific object instance
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void bacfile_create_context_set(uint32_t object_instance, void *context)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Context = context;
    }
}

/**
 * @brief Creates a File object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t bacfile_create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
    int index = 0;

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
    if (!pObject) {
        pObject = calloc(1, sizeof(struct object_data));
        if (pObject) {
            pObject->Object_Name = NULL;
            pObject->Pathname = NULL;
            pObject->File_Type = NULL;
            /* April Fool's Day */
            datetime_set_values(
                &pObject->Modification_Date, 2006, 4, 1, 7, 0, 3, 1);
            pObject->Read_Only = false;
            pObject->Archive = false;
            pObject->File_Access_Stream = true;
            /* add to list */
            index = Keylist_Data_Add(Object_List, object_instance, pObject);
            if (index < 0) {
                free(pObject);
                return BACNET_MAX_INSTANCE;
            }
        } else {
            return BACNET_MAX_INSTANCE;
        }
    }

    return object_instance;
}

/**
 * @brief Deletes an object
 * @param object_instance - object-instance number of the object
 * @return true if the object-instance was deleted
 */
bool bacfile_delete(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject = NULL;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        free(pObject);
        status = true;
    }

    return status;
}

/**
 * @brief Deletes all the objects and their data
 */
void bacfile_cleanup(void)
{
    struct object_data *pObject;

    if (Object_List) {
        do {
            pObject = Keylist_Data_Pop(Object_List);
            if (pObject) {
                free(pObject->Pathname);
                free(pObject->File_Type);
                free(pObject->Object_Name);
                free(pObject);
            }
        } while (pObject);
        Keylist_Delete(Object_List);
        Object_List = NULL;
    }
}

/**
 * @brief Initializes the object data
 */
void bacfile_init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
