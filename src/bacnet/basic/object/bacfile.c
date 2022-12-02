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

#ifndef max
#define max(a, b) (((a)(b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

typedef enum {
    BACFILE_UNKNOWN = 0,
    BACFILE_FILE = 1,
    BACFILE_MEMORY = 2,
} BACFILE_TYPES;

typedef struct {
    uint8_t *pointer;
    BACNET_UNSIGNED_INTEGER length;
} BACNET_MEMORY_FILE;

typedef struct {
    uint32_t instance;
    BACFILE_TYPES type;
    union {
        char *filename;
        BACNET_MEMORY_FILE memory;
    };
} BACNET_FILE_LISTING;

#ifndef FILE_RECORD_SIZE
#define FILE_RECORD_SIZE MAX_OCTET_STRING_BYTES
#endif

#ifndef MAX_BACFILES
#define MAX_BACFILES 4
#endif

static BACNET_FILE_LISTING BACnet_File_Listing[MAX_BACFILES] = { 0 };

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int bacfile_Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_FILE_TYPE, PROP_FILE_SIZE,
    PROP_MODIFICATION_DATE, PROP_ARCHIVE, PROP_READ_ONLY,
    PROP_FILE_ACCESS_METHOD, -1 };

static const int bacfile_Properties_Optional[] = { PROP_DESCRIPTION, -1 };

static const int bacfile_Properties_Proprietary[] = { -1 };

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

static char *bacfile_name(uint32_t instance)
{
    static char filename[20];
    unsigned index = bacfile_instance_to_index(instance);
    if (index >= MAX_BACFILES)
        return NULL;

    switch (BACnet_File_Listing[index].type) {
        case BACFILE_FILE:
            return BACnet_File_Listing[index].filename;
        case BACFILE_MEMORY:
            snprintf(filename, sizeof(filename), "MEM %d", instance);
            return filename;
        default:
            break;
    }

    return NULL;
}

bool bacfile_object_name(
    uint32_t instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    char *filename = NULL;

    filename = bacfile_name(instance);
    if (filename) {
        status = characterstring_init_ansi(object_name, filename);
    }

    return status;
}

bool bacfile_valid_instance(uint32_t object_instance)
{
    return bacfile_name(object_instance) ? true : false;
}

uint32_t bacfile_count(void)
{
    uint32_t count = 0;
    int i;

    for (i = 0; i < MAX_BACFILES; i++)
        if (BACnet_File_Listing[i].type != BACFILE_UNKNOWN)
            count++;

    return count;
}

uint32_t bacfile_index_to_instance(unsigned find_index)
{
    /* bounds checking... */
    return ((find_index < MAX_BACFILES) &&
            (BACnet_File_Listing[find_index].type != BACFILE_UNKNOWN)) ?
        BACnet_File_Listing[find_index].instance : 
        BACNET_MAX_INSTANCE + 1;
}

unsigned bacfile_instance_to_index(uint32_t instance)
{
    int i;
    for (i = 0; i < MAX_BACFILES; i++)
        if (BACnet_File_Listing[i].instance == instance)
            return i;
    return MAX_BACFILES;
}
    
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

BACNET_UNSIGNED_INTEGER bacfile_file_size(uint32_t object_instance)
{
    FILE *pFile = NULL;
    long file_position = 0;
    BACNET_UNSIGNED_INTEGER file_size = 0;

    unsigned index = bacfile_instance_to_index(object_instance);

    if (index >= MAX_BACFILES)
        return file_size;

    switch (BACnet_File_Listing[index].type) {
        case BACFILE_FILE:
            pFile = fopen(BACnet_File_Listing[index].filename, "rb");
            if (pFile) {
                file_position = fsize(pFile);
                if (file_position >= 0) {
                    file_size = (BACNET_UNSIGNED_INTEGER)file_position;
                }
                fclose(pFile);
            }
            break;
        case BACFILE_MEMORY:
            file_size = BACnet_File_Listing[index].memory.length;
            break;
        default:
            break;
    }

    return file_size;
}

/* return the number of bytes used, or -1 on error */
int bacfile_read_property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    char text_string[32] = { "" };
    BACNET_CHARACTER_STRING char_string;
    BACNET_DATE bdate;
    BACNET_TIME btime;
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
            sprintf(text_string, "FILE %lu",
                (unsigned long)rpdata->object_instance);
            characterstring_init_ansi(&char_string, text_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_FILE);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string, bacfile_name(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_FILE_TYPE:
            characterstring_init_ansi(&char_string, "TEXT");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_FILE_SIZE:
            apdu_len = encode_application_unsigned(
                &apdu[0], bacfile_file_size(rpdata->object_instance));
            break;
        case PROP_MODIFICATION_DATE:
            /* FIXME: get the actual value instead of April Fool's Day */
            bdate.year = 2006; /* AD */
            bdate.month = 4; /* 1=Jan */
            bdate.day = 1; /* 1..31 */
            bdate.wday = 6; /* 1=Monday */
            apdu_len = encode_application_date(&apdu[0], &bdate);
            /* FIXME: get the actual value */
            btime.hour = 7;
            btime.min = 0;
            btime.sec = 3;
            btime.hundredths = 1;
            apdu_len += encode_application_time(&apdu[apdu_len], &btime);
            break;
        case PROP_ARCHIVE:
            /* 12.13.8 Archive
               This property, of type BOOLEAN, indicates whether the File
               object has been saved for historical or backup purposes. This
               property shall be logical TRUE only if no changes have been
               made to the file data by internal processes or through File
               Access Services since the last time the object was archived.
             */
            /* FIXME: get the actual value: note it may be inverse... */
            apdu_len = encode_application_boolean(&apdu[0], true);
            break;
        case PROP_READ_ONLY:
            /* FIXME: get the actual value */
            apdu_len = encode_application_boolean(&apdu[0], true);
            break;
        case PROP_FILE_ACCESS_METHOD:
            apdu_len = encode_application_enumerated(
                &apdu[0], FILE_RECORD_AND_STREAM_ACCESS);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = -1;
            break;
    }

    return apdu_len;
}

/* returns true if successful */
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
            /* 12.13.8 Archive
               This property, of type BOOLEAN, indicates whether the File
               object has been saved for historical or backup purposes. This
               property shall be logical TRUE only if no changes have been
               made to the file data by internal processes or through File
               Access Services since the last time the object was archived. */
            status = write_property_type_valid(wp_data, &value,
                BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                if (value.type.Boolean) {
                    /* FIXME: do something to wp_data->object_instance */
                } else {
                    /* FIXME: do something to wp_data->object_instance */
                }
            }
            break;
        case PROP_FILE_SIZE:
            /* If the file size can be changed by writing to the file,
               and File_Access_Method is STREAM_ACCESS, then this property
               shall be writable. */
            status = write_property_type_valid(wp_data, &value,
                BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                /* FIXME: do something with value.type.Unsigned
                   to wp_data->object_instance */
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

uint32_t bacfile_instance(char *filename)
{
    uint32_t index = 0;
    uint32_t instance = BACNET_MAX_INSTANCE + 1;
    char name[20];
    char *pf;

    /* linear search for filename match */
    for (index = 0; index < MAX_BACFILES; index++) {
        switch (BACnet_File_Listing[index].type) {
            case BACFILE_FILE:
                pf = BACnet_File_Listing[index].filename;
                break;
            case BACFILE_MEMORY:
                snprintf(name, sizeof(name), "MEM %d",
                    BACnet_File_Listing[index].instance);
                pf = name;
                break;
            default:
                name[0] = 0;
                pf = name;
        }

        if (strcmp(pf, filename) == 0) {
            instance = BACnet_File_Listing[index].instance;
            break;
        }
    }

    return instance;
}

bool bacfile_instance_set(unsigned index, uint32_t object_instance,
    char *filename)
{
    if (index >= MAX_BACFILES)
        return false;
    BACnet_File_Listing[index].instance = object_instance;
    BACnet_File_Listing[index].type = BACFILE_FILE;
    BACnet_File_Listing[index].filename = filename;
    return true;
}

bool bacfile_instance_memory_set(unsigned index, uint32_t object_instance,
    uint8_t *pointer, BACNET_UNSIGNED_INTEGER length)
{
    if (index >= MAX_BACFILES)
        return false;
    BACnet_File_Listing[index].instance = object_instance;
    BACnet_File_Listing[index].type = BACFILE_MEMORY;
    BACnet_File_Listing[index].memory.pointer = pointer;
    BACnet_File_Listing[index].memory.length = length;
    return true;
}

BACNET_UNSIGNED_INTEGER bacfile_instance_context(uint32_t object_instance,
    uint8_t *pointer, BACNET_UNSIGNED_INTEGER max_length)
{
    unsigned index = bacfile_instance_to_index(object_instance);
    BACNET_UNSIGNED_INTEGER ret = 0;
    FILE *pFile = NULL;

    if (index >= MAX_BACFILES)
        return ret;

    switch (BACnet_File_Listing[index].type) {
        case BACFILE_FILE:
            pFile = fopen(BACnet_File_Listing[index].filename, "rb");
            if (pFile) {
                ret = fread(pointer, 1, max_length, pFile);
                fclose(pFile);
            }
            break;
        case BACFILE_MEMORY:
            ret = min(max_length, BACnet_File_Listing[index].memory.length);
            memcpy(pointer, BACnet_File_Listing[index].memory.pointer, ret);
            break;
        default:
            break;
    }
    return ret;
}

uint8_t *bacfile_instance_memory_context(uint32_t object_instance,
    void *pointer, BACNET_UNSIGNED_INTEGER max_length)
{
    if ((pointer != NULL) && (max_length != 0)) {
        bacfile_instance_context(object_instance, pointer, max_length);
        return pointer;
    }

    unsigned index = bacfile_instance_to_index(object_instance);

    if (index >= MAX_BACFILES)
        return NULL;

    switch (BACnet_File_Listing[index].type) {
        case BACFILE_MEMORY:
            return BACnet_File_Listing[index].memory.pointer;
            break;
        default:
            break;
    }
    return NULL;
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
    FILE *pFile = NULL;
    size_t len = 0;

    unsigned index = bacfile_instance_to_index(data->object_instance);
    if ((index >= MAX_BACFILES) ||
        (BACnet_File_Listing[index].type == BACFILE_UNKNOWN)){
        octetstring_truncate(&data->fileData[0], 0);
        data->endOfFile = true;
        return false;
    }

    switch (BACnet_File_Listing[index].type) {
        case BACFILE_FILE:
            pFile = fopen(BACnet_File_Listing[index].filename, "rb");
            if (pFile) {
                (void)fseek(pFile, data->type.stream.fileStartPosition, SEEK_SET);
                len = fread(octetstring_value(&data->fileData[0]), 1,
                    data->type.stream.requestedOctetCount, pFile);
                data->endOfFile = len < data->type.stream.requestedOctetCount;
                octetstring_truncate(&data->fileData[0], len);
                fclose(pFile);
            } else {
                octetstring_truncate(&data->fileData[0], 0);
                data->endOfFile = true;
            }
            break;
        case BACFILE_MEMORY:
            if (BACnet_File_Listing[index].memory.length >= 
                data->type.stream.fileStartPosition) {
                len = min(data->type.stream.requestedOctetCount,
                    BACnet_File_Listing[index].memory.length - 
                    data->type.stream.fileStartPosition);
            } else {
                len  = 0;
            }
            memcpy(octetstring_value(&data->fileData[0]),
                BACnet_File_Listing[index].memory.pointer +
                data->type.stream.fileStartPosition, len);
            octetstring_truncate(&data->fileData[0], len);
            data->endOfFile = len < data->type.stream.requestedOctetCount;
            break;
        default:
            break;
    }

    return true;
}

bool bacfile_write_stream_data(BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    char *pFilename = NULL;
    bool found = false;
    FILE *pFile = NULL;

    unsigned index = bacfile_instance_to_index(data->object_instance);
    if ((index >= MAX_BACFILES) ||
        (BACnet_File_Listing[index].type != BACFILE_FILE)){
        return found;
    }

    pFilename = bacfile_name(data->object_instance);
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
    char *pFilename = NULL;
    bool found = false;
    FILE *pFile = NULL;
    uint32_t i = 0;
    char dummy_data[FILE_RECORD_SIZE];
    char *pData = NULL;

    unsigned index = bacfile_instance_to_index(data->object_instance);
    if ((index >= MAX_BACFILES) ||
        (BACnet_File_Listing[index].type != BACFILE_FILE)){
        return found;
    }

    pFilename = bacfile_name(data->object_instance);
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
    char *pFilename = NULL;

    unsigned index = bacfile_instance_to_index(instance);
    if ((index >= MAX_BACFILES) ||
        (BACnet_File_Listing[index].type != BACFILE_FILE)){
        return found;
    }

    pFilename = bacfile_name(instance);
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
    char *pFilename = NULL;
    uint32_t i = 0;
    char dummy_data[MAX_OCTET_STRING_BYTES] = { 0 };
    char *pData = NULL;

    unsigned index = bacfile_instance_to_index(instance);
    if ((index >= MAX_BACFILES) ||
        (BACnet_File_Listing[index].type != BACFILE_FILE)){
        return found;
    }

    pFilename = bacfile_name(instance);
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

void bacfile_init(void)
{
}
