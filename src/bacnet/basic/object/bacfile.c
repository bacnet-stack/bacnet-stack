/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bacnet/config.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacapp.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/bacdcode.h"
#include "bacnet/datetime.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/arf.h"
#include "bacnet/awf.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/basic/sys/keylist.h"

#ifndef FILE_RECORD_SIZE
#define FILE_RECORD_SIZE MAX_OCTET_STRING_BYTES
#endif
struct object_data {
    char *Object_Name;
    char *Pathname;
    BACNET_DATE_TIME Modification_Date;
    char *File_Type;
    bool File_Access_Stream:1;
    bool Read_Only : 1;
    bool Archive : 1;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_FILE;
/* These three arrays are used by the ReadPropertyMultiple handler */
static const int bacfile_Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_FILE_TYPE, PROP_FILE_SIZE,
    PROP_MODIFICATION_DATE, PROP_ARCHIVE, PROP_READ_ONLY,
    PROP_FILE_ACCESS_METHOD, -1 };

static const int bacfile_Properties_Optional[] = { PROP_DESCRIPTION, -1 };

static const int bacfile_Properties_Proprietary[] = { -1 };

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
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = bacfile_Properties_Required;
    }
    if (pOptional) {
        *pOptional = bacfile_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = bacfile_Properties_Proprietary;
    }

    return;
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
        if (pObject->Pathname) {
            free(pObject->Pathname);
        }
        pObject->Pathname = strdup(pathname);
    }
}

/**
 * @brief For a given pathname, gets the object instance-number
 * @param pathname - internal file system path and name
 * @return object-instance number of the object,
 *  or #BACNET_MAX_INSTANCE if not found
 */
uint32_t bacfile_pathname_instance(
        const char *pathname)
{
    struct object_data *pObject;
    int count = 0;
    int index = 0;

    count = Keylist_Count(Object_List);
    while (count) {
        pObject = Keylist_Data_Index(Object_List, index);
        if (strcmp(pathname, pObject->Pathname) == 0) {
            return Keylist_Key(Object_List, index);
        }
        count--;
        index++;
    }

    return BACNET_MAX_INSTANCE;
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
            status = characterstring_init_ansi(object_name,
                pObject->Object_Name);
        } else {
            snprintf(name_text, sizeof(name_text), "FILE %u",
                object_instance);
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
bool bacfile_object_name_set(uint32_t object_instance, char *new_name)
{
    bool status = false; /* return value */
    BACNET_CHARACTER_STRING object_name;
    BACNET_OBJECT_TYPE found_type = 0;
    uint32_t found_instance = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject && new_name) {
        /* All the object names in a device must be unique */
        characterstring_init_ansi(&object_name, new_name);
        if (Device_Valid_Object_Name(
                &object_name, &found_type, &found_instance)) {
            if ((found_type == Object_Type) &&
                (found_instance == object_instance)) {
                /* writing same name to same object */
                status = true;
            } else {
                /* duplicate name! */
                status = false;
            }
        } else {
            status = true;
            pObject->Object_Name = new_name;
            Device_Inc_Database_Revision();
        }
    }

    return status;
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
uint32_t bacfile_count(void)
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
    return Keylist_Key(Object_List, find_index);
}

/**
 * @brief Determines the file size for a given file
 * @param  pFile - file handle
 * @return  file size in bytes, or 0 if not found
 */
static long fsize(FILE *pFile)
{
    long size = 0;
    long origin = 0;

    if (pFile) {
        origin = ftell(pFile);
        fseek(pFile, 0L, SEEK_END);
        size = ftell(pFile);
        fseek(pFile, origin, SEEK_SET);
    }
    return (size);
}

/**
 * @brief Read the entire file into a buffer
 * @param  object_instance - object-instance number of the object
 * @param  buffer - pointer to buffer pointer where heap data will be stored
 * @param  buffer_size - in bytes
 * @return  file size in bytes
 */
uint32_t bacfile_read(uint32_t object_instance, uint8_t *buffer,
    uint32_t buffer_size)
{
    const char *pFilename = NULL;
    FILE *pFile = NULL;
    long file_size = 0;

    pFilename = bacfile_pathname(object_instance);
    if (pFilename) {
        pFile = fopen(pFilename, "rb");
        if (pFile) {
            file_size = fsize(pFile);
            if (buffer && (buffer_size >= file_size)) {
                if (fread(buffer, file_size, 1, pFile) == 0) {
                    file_size = 0;
                }
            }
            fclose(pFile);
        }
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
    const char *pFilename = NULL;
    FILE *pFile = NULL;
    long file_position = 0;
    BACNET_UNSIGNED_INTEGER file_size = 0;

    pFilename = bacfile_pathname(object_instance);
    if (pFilename) {
        pFile = fopen(pFilename, "rb");
        if (pFile) {
            file_position = fsize(pFile);
            if (file_position >= 0) {
                file_size = (BACNET_UNSIGNED_INTEGER)file_position;
            }
            fclose(pFile);
        }
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
    uint32_t object_instance,
    BACNET_UNSIGNED_INTEGER file_size)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->File_Access_Stream) {
            (void)file_size;
            /* FIXME: add clever POSIX file stuff here */
        }
    }

    return status;
}


/**
 * @brief Determines the file size property value
 * @param object_instance - object-instance number of the object
 * @return value of the file size property
 */
const char * bacfile_file_type(
    uint32_t object_instance)
{
    struct object_data *pObject;
    const char * mime_type = "application/octet-stream";

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
void bacfile_file_type_set(
    uint32_t object_instance,
    const char *mime_type)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->File_Type) {
            if (strcmp(pObject->File_Type, mime_type) != 0) {
                free(pObject->File_Type);
                pObject->File_Type = strdup(mime_type);
            }
        } else {
            pObject->File_Type = strdup(mime_type);
        }
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
bool bacfile_archive(
    uint32_t object_instance)
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
bool bacfile_archive_set(
    uint32_t object_instance, bool archive)
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
bool bacfile_read_only(
    uint32_t object_instance)
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
bool bacfile_read_only_set(
    uint32_t object_instance,
    bool read_only)
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
 * @brief For a given object instance-number, return the flag
 * @param  object_instance - object-instance number of the object
 * @return  true if the property is true
 */
void bacfile_modification_date(
    uint32_t object_instance, BACNET_DATE_TIME *bdatetime)
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
bool bacfile_file_access_stream(
    uint32_t object_instance)
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
bool bacfile_file_access_stream_set(
    uint32_t object_instance,
    bool access)
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
            apdu_len =
                encode_application_enumerated(&apdu[0], Object_Type);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string, bacfile_pathname(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_FILE_TYPE:
            characterstring_init_ansi(&char_string,
                bacfile_file_type(rpdata->object_instance));
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
            apdu_len = encode_application_boolean(&apdu[0],
                bacfile_archive(rpdata->object_instance));
            break;
        case PROP_READ_ONLY:
            apdu_len = encode_application_boolean(&apdu[0],
                bacfile_read_only(rpdata->object_instance));
            break;
        case PROP_FILE_ACCESS_METHOD:
            if (bacfile_file_access_stream(rpdata->object_instance)) {
                apdu_len = encode_application_enumerated(
                    &apdu[0], FILE_STREAM_ACCESS);
            } else {
                apdu_len = encode_application_enumerated(
                    &apdu[0], FILE_RECORD_ACCESS);
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
    BACNET_APPLICATION_DATA_VALUE value;

    if (!bacfile_valid_instance(wp_data->object_instance)) {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    /*  only array properties can have array options */
    if (wp_data->array_index != BACNET_ARRAY_ALL) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
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
                status =
                    bacfile_file_size_set(wp_data->object_instance,
                    value.type.Unsigned_Int);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                }
            }
            break;
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_OBJECT_TYPE:
        case PROP_DESCRIPTION:
        case PROP_FILE_TYPE:
        case PROP_MODIFICATION_DATE:
        case PROP_READ_ONLY:
        case PROP_FILE_ACCESS_METHOD:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
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
            len = apdu_decode_confirmed_service_request(&apdu[0], apdu_len,
                &service_data, &service_choice, &service_request,
                &service_request_len);
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
    const char *pFilename = NULL;
    bool found = false;
    FILE *pFile = NULL;
    size_t len = 0;

    pFilename = bacfile_pathname(data->object_instance);
    if (pFilename) {
        found = true;
        pFile = fopen(pFilename, "rb");
        if (pFile) {
            (void)fseek(pFile, data->type.stream.fileStartPosition, SEEK_SET);
            len = fread(octetstring_value(&data->fileData[0]), 1,
                data->type.stream.requestedOctetCount, pFile);
            if (len < data->type.stream.requestedOctetCount) {
                data->endOfFile = true;
            } else {
                data->endOfFile = false;
            }
            octetstring_truncate(&data->fileData[0], len);
            fclose(pFile);
        } else {
            octetstring_truncate(&data->fileData[0], 0);
            data->endOfFile = true;
        }
    } else {
        octetstring_truncate(&data->fileData[0], 0);
        data->endOfFile = true;
    }

    return found;
}

bool bacfile_write_stream_data(BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    const char *pFilename = NULL;
    bool found = false;
    FILE *pFile = NULL;

    pFilename = bacfile_pathname(data->object_instance);
    if (pFilename) {
        found = true;
        if (data->type.stream.fileStartPosition == 0) {
            /* open the file as a clean slate when starting at 0 */
            pFile = fopen(pFilename, "wb");
        } else if (data->type.stream.fileStartPosition == -1) {
            /* If 'File Start Position' parameter has the special
               value -1, then the write operation shall be treated
               as an append to the current end of file. */
            pFile = fopen(pFilename, "ab+");
        } else {
            /* open for update */
            pFile = fopen(pFilename, "rb+");
        }
        if (pFile) {
            if (data->type.stream.fileStartPosition != -1) {
                (void)fseek(
                    pFile, data->type.stream.fileStartPosition, SEEK_SET);
            }
            if (fwrite(octetstring_value(&data->fileData[0]),
                    octetstring_length(&data->fileData[0]), 1, pFile) != 1) {
                /* do something if it fails? */
            }
            fclose(pFile);
        }
    }

    return found;
}

bool bacfile_write_record_data(BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    const char *pFilename = NULL;
    bool found = false;
    FILE *pFile = NULL;
    uint32_t i = 0;
    char dummy_data[FILE_RECORD_SIZE];
    char *pData = NULL;

    pFilename = bacfile_pathname(data->object_instance);
    if (pFilename) {
        found = true;
        if (data->type.record.fileStartRecord == 0) {
            /* open the file as a clean slate when starting at 0 */
            pFile = fopen(pFilename, "wb");
        } else if (data->type.record.fileStartRecord == -1) {
            /* If 'File Start Record' parameter has the special
               value -1, then the write operation shall be treated
               as an append to the current end of file. */
            pFile = fopen(pFilename, "ab+");
        } else {
            /* open for update */
            pFile = fopen(pFilename, "rb+");
        }
        if (pFile) {
            if ((data->type.record.fileStartRecord != -1) &&
                (data->type.record.fileStartRecord > 0)) {
                for (i = 0; i < (uint32_t)data->type.record.fileStartRecord;
                     i++) {
                    pData = fgets(&dummy_data[0], sizeof(dummy_data), pFile);
                    if ((pData == NULL) || feof(pFile)) {
                        break;
                    }
                }
            }
            for (i = 0; i < data->type.record.returnedRecordCount; i++) {
                if (fwrite(octetstring_value(&data->fileData[i]),
                        octetstring_length(&data->fileData[i]), 1,
                        pFile) != 1) {
                    /* do something if it fails? */
                }
            }
            fclose(pFile);
        }
    }

    return found;
}

bool bacfile_read_ack_stream_data(
    uint32_t instance, BACNET_ATOMIC_READ_FILE_DATA *data)
{
    bool found = false;
    FILE *pFile = NULL;
    const char *pFilename = NULL;

    pFilename = bacfile_pathname(instance);
    if (pFilename) {
        found = true;
        pFile = fopen(pFilename, "rb+");
        if (pFile) {
            (void)fseek(pFile, data->type.stream.fileStartPosition, SEEK_SET);
            if (fwrite(octetstring_value(&data->fileData[0]),
                    octetstring_length(&data->fileData[0]), 1, pFile) != 1) {
#if PRINT_ENABLED
                fprintf(stderr, "Failed to write to %s (%lu)!\n", pFilename,
                    (unsigned long)instance);
#endif
            }
            fclose(pFile);
        }
    }

    return found;
}

bool bacfile_read_ack_record_data(
    uint32_t instance, BACNET_ATOMIC_READ_FILE_DATA *data)
{
    bool found = false;
    FILE *pFile = NULL;
    const char *pFilename = NULL;
    uint32_t i = 0;
    char dummy_data[MAX_OCTET_STRING_BYTES] = { 0 };
    char *pData = NULL;

    pFilename = bacfile_pathname(instance);
    if (pFilename) {
        found = true;
        pFile = fopen(pFilename, "rb+");
        if (pFile) {
            if (data->type.record.fileStartRecord > 0) {
                for (i = 0; i < (uint32_t)data->type.record.fileStartRecord;
                     i++) {
                    pData = fgets(&dummy_data[0], sizeof(dummy_data), pFile);
                    if ((pData == NULL) || feof(pFile)) {
                        break;
                    }
                }
            }
            for (i = 0; i < data->type.record.RecordCount; i++) {
                if (fwrite(octetstring_value(&data->fileData[i]),
                        octetstring_length(&data->fileData[i]), 1,
                        pFile) != 1) {
#if PRINT_ENABLED
                    fprintf(stderr, "Failed to write to %s (%lu)!\n", pFilename,
                        (unsigned long)instance);
#endif
                }
            }
            fclose(pFile);
        }
    }

    return found;
}


/**
 * @brief Creates an object
 * @param object_instance - object-instance number of the object
 * @return true if the object-instance was created
 */
bool bacfile_create(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject = NULL;
    int index = 0;

    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        pObject = calloc(1, sizeof(struct object_data));
        if (pObject) {
            pObject->Object_Name = NULL;
            pObject->Pathname = NULL;
            /* April Fool's Day */
            datetime_set_values(&pObject->Modification_Date,
                2006, 4, 1, 7, 0, 3, 1);
            pObject->Read_Only = false;
            pObject->Archive = false;
            pObject->File_Access_Stream = true;
            /* add to list */
            index = Keylist_Data_Add(Object_List, object_instance, pObject);
            if (index >= 0) {
                status = true;
                Device_Inc_Database_Revision();
            }
        }
    }

    return status;
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
        Device_Inc_Database_Revision();
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
                free(pObject);
                Device_Inc_Database_Revision();
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
    Object_List = Keylist_Create();
}
