/**
 * @file
 * @brief BACnet error encode and decode helper functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"

/** @file bacerror.c  Encode/Decode BACnet Errors */

/**
 * @brief Encodes BACnet Error class and code values into a PDU
 *  From clause 21. FORMAL DESCRIPTION OF APPLICATION PROTOCOL DATA UNITS
 *
 *      Error ::= SEQUENCE {
 *          -- NOTE: The valid combinations of error-class and error-code
 *          -- are defined in Clause 18.
 *          error-class ENUMERATED,
 *          error-code ENUMERATED
 *      }
 *
 * @param apdu - buffer for the data to be encoded, or NULL for length
 * @param invoke_id - invokeID to be encoded
 * @param service - BACnet service to be encoded
 * @param error_class - #BACNET_ERROR_CLASS value to be encoded
 * @param error_code - #BACNET_ERROR_CODE value to be encoded
 * @return number of bytes encoded
 */
int bacerror_encode_apdu(
    uint8_t *apdu,
    uint8_t invoke_id,
    BACNET_CONFIRMED_SERVICE service,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    /* length of the specific element of the PDU */
    int len = 0;
    /* total length of the apdu, return value */
    int apdu_len = 0;

    if (apdu) {
        apdu[0] = PDU_TYPE_ERROR;
        apdu[1] = invoke_id;
        apdu[2] = service;
    }
    len = 3;
    apdu_len = len;
    if (apdu) {
        apdu += len;
    }
    /* service parameters */
    len = encode_application_enumerated(apdu, error_class);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_enumerated(apdu, error_code);
    apdu_len += len;

    return apdu_len;
}

#if !BACNET_SVC_SERVER
/**
 * @brief Decodes from bytes a BACnet Error service APDU
 *  From clause 21. FORMAL DESCRIPTION OF APPLICATION PROTOCOL DATA UNITS
 *
 *  Error ::= SEQUENCE {
 *      -- NOTE: The valid combinations of error-class and error-code
 *      -- are defined in Clause 18.
 *      error-class ENUMERATED,
 *      error-code ENUMERATED
 *  }
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @param error_class - decoded #BACNET_ERROR_CLASS value
 * @param error_code - decoded #BACNET_ERROR_CODE value
 *
 * @return number of bytes decoded, or #BACNET_STATUS_ERROR (-1) if malformed
 */
int bacerror_decode_error_class_and_code(
    const uint8_t *apdu,
    unsigned apdu_size,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    int apdu_len = 0;
    int tag_len = 0;
    uint32_t decoded_value = 0;

    if (apdu) {
        /* error class */
        tag_len = bacnet_enumerated_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, &decoded_value);
        if (tag_len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        if (error_class) {
            *error_class = (BACNET_ERROR_CLASS)decoded_value;
        }
        apdu_len += tag_len;
        /* error code */
        tag_len = bacnet_enumerated_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, &decoded_value);
        if (tag_len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        if (error_code) {
            *error_code = (BACNET_ERROR_CODE)decoded_value;
        }
        apdu_len += tag_len;
    }

    return apdu_len;
}

/**
 * @brief Decodes from bytes a BACnet Error service
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @param invoke_id - decoded invokeID
 * @param service - decoded BACnet service
 * @param error_class - decoded #BACNET_ERROR_CLASS value
 * @param error_code - decoded #BACNET_ERROR_CODE value
 *
 * @return number of bytes decoded, or #BACNET_STATUS_ERROR (-1) if malformed
 */
int bacerror_decode_service_request(
    const uint8_t *apdu,
    unsigned apdu_size,
    uint8_t *invoke_id,
    BACNET_CONFIRMED_SERVICE *service,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    int apdu_len = BACNET_STATUS_ERROR;
    int len = 0;

    if (apdu && (apdu_size > 2)) {
        if (invoke_id) {
            *invoke_id = apdu[0];
        }
        if (service) {
            *service = (BACNET_CONFIRMED_SERVICE)apdu[1];
        }
        len = 2;
        apdu_len = len;
        /* decode the application class and code */
        len = bacerror_decode_error_class_and_code(
            &apdu[apdu_len], apdu_size - apdu_len, error_class, error_code);
        if (len > 0) {
            apdu_len += len;
        } else {
            apdu_len = BACNET_STATUS_ERROR;
        }
    }

    return apdu_len;
}
#endif

/**
 * @brief Determine the error class from the error code
 * @param error_code #BACNET_ERROR_CODE enumeration
 * @return BACnet Error class assigned to the error code
 * @note Error Code OTHER appears in all classes. Some
 *  error codes appear in multiple classes, such as
 *  VALUE_OUT_OF_RANGE, OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED,
 *  WRITE_ACCESS_DENIED, READ_ACCESS_DENIED,
 *  INVALID_TAG, and SECURITY_ERROR.
 */
BACNET_ERROR_CLASS bacerror_code_class(BACNET_ERROR_CODE error_code)
{
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_DEVICE;

    switch (error_code) {
        case ERROR_CODE_CONFIGURATION_IN_PROGRESS:
        case ERROR_CODE_DEVICE_BUSY:
        case ERROR_CODE_INCONSISTENT_CONFIGURATION:
        case ERROR_CODE_INTERNAL_ERROR:
        case ERROR_CODE_NOT_CONFIGURED:
        case ERROR_CODE_OPERATIONAL_PROBLEM:
        case ERROR_CODE_OTHER:
            error_class = ERROR_CLASS_DEVICE;
            break;
        case ERROR_CODE_BUSY:
        case ERROR_CODE_DYNAMIC_CREATION_NOT_SUPPORTED:
        case ERROR_CODE_FILE_FULL:
        case ERROR_CODE_INVALID_OPERATION_IN_THIS_STATE:
        case ERROR_CODE_LOG_BUFFER_FULL:
        case ERROR_CODE_NO_ALARM_CONFIGURED:
        case ERROR_CODE_NO_OBJECTS_OF_SPECIFIED_TYPE:
        case ERROR_CODE_OBJECT_DELETION_NOT_PERMITTED:
        case ERROR_CODE_OBJECT_IDENTIFIER_ALREADY_EXISTS:
        case ERROR_CODE_REFERENCED_PORT_IN_ERROR:
        case ERROR_CODE_UNKNOWN_OBJECT:
        case ERROR_CODE_UNSUPPORTED_OBJECT_TYPE:
            error_class = ERROR_CLASS_OBJECT;
            break;
        case ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED:
        case ERROR_CODE_DATATYPE_NOT_SUPPORTED:
        case ERROR_CODE_DUPLICATE_ENTRY:
        case ERROR_CODE_DUPLICATE_NAME:
        case ERROR_CODE_DUPLICATE_OBJECT_ID:
        case ERROR_CODE_INCONSISTENT_SELECTION_CRITERION:
        case ERROR_CODE_INVALID_ARRAY_INDEX:
        case ERROR_CODE_INVALID_ARRAY_SIZE:
        case ERROR_CODE_INVALID_DATA_ENCODING:
        case ERROR_CODE_INVALID_DATA_TYPE:
        case ERROR_CODE_INVALID_VALUE_IN_THIS_STATE:
        case ERROR_CODE_LIST_ITEM_NOT_NUMBERED:
        case ERROR_CODE_LIST_ITEM_NOT_TIMESTAMPED:
        case ERROR_CODE_LOGGED_VALUE_PURGED:
        case ERROR_CODE_NO_PROPERTY_SPECIFIED:
        case ERROR_CODE_NOT_CONFIGURED_FOR_TRIGGERED_LOGGING:
        case ERROR_CODE_NOT_COV_PROPERTY:
        case ERROR_CODE_NOT_ENABLED:
        case ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED:
        case ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY:
        case ERROR_CODE_READ_ACCESS_DENIED:
        case ERROR_CODE_UNKNOWN_PROPERTY:
        case ERROR_CODE_UNKNOWN_FILE_SIZE:
        case ERROR_CODE_VALUE_NOT_INITIALIZED:
        case ERROR_CODE_VALUE_OUT_OF_RANGE:
        case ERROR_CODE_VALUE_TOO_LONG:
        case ERROR_CODE_WRITE_ACCESS_DENIED:
            error_class = ERROR_CLASS_PROPERTY;
            break;
        case ERROR_CODE_NO_SPACE_FOR_OBJECT:
        case ERROR_CODE_NO_SPACE_TO_ADD_LIST_ELEMENT:
        case ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY:
        case ERROR_CODE_OUT_OF_MEMORY:
            error_class = ERROR_CLASS_RESOURCES;
            break;
        case ERROR_CODE_ACCESS_DENIED:
        case ERROR_CODE_ADJUST_SCOPE_REQUIRED:
        case ERROR_CODE_AUTH_SCOPE_REQUIRED:
        case ERROR_CODE_BAD_DESTINATION_ADDRESS:
        case ERROR_CODE_BAD_DESTINATION_DEVICE_ID:
        case ERROR_CODE_BAD_SIGNATURE:
        case ERROR_CODE_BIND_SCOPE_REQUIRED:
        case ERROR_CODE_CONFIG_SCOPE_REQUIRED:
        case ERROR_CODE_CERTIFICATE_EXPIRED:
        case ERROR_CODE_CERTIFICATE_MALFORMED:
        case ERROR_CODE_CERTIFICATE_REVOKED:
        case ERROR_CODE_CERTIFICATE_INVALID:
        case ERROR_CODE_DUPLICATE_MESSAGE:
        case ERROR_CODE_ENCRYPTION_NOT_CONFIGURED:
        case ERROR_CODE_ENCRYPTION_REQUIRED:
        case ERROR_CODE_EXTENDED_SCOPE_REQUIRED:
        case ERROR_CODE_INCORRECT_AUDIENCE:
        case ERROR_CODE_INCORRECT_CLIENT:
        case ERROR_CODE_INCORRECT_ISSUER:
        case ERROR_CODE_INSTALL_SCOPE_REQUIRED:
        case ERROR_CODE_INSUFFICIENT_SCOPE:
        case ERROR_CODE_INVALID_TOKEN:
        case ERROR_CODE_MALFORMED_MESSAGE:
        case ERROR_CODE_OVERRIDE_SCOPE_REQUIRED:
        case ERROR_CODE_PASSWORD_FAILURE:
        case ERROR_CODE_REVOKED_TOKEN:
        case ERROR_CODE_SECURITY_NOT_CONFIGURED:
        case ERROR_CODE_SOURCE_SECURITY_REQUIRED:
        case ERROR_CODE_SUCCESS:
        case ERROR_CODE_UNKNOWN_AUTHENTICATION_TYPE:
        case ERROR_CODE_UNKNOWN_KEY:
        case ERROR_CODE_VIEW_SCOPE_REQUIRED:
            error_class = ERROR_CLASS_SECURITY;
            break;
        case ERROR_CODE_COMMUNICATION_DISABLED:
        case ERROR_CODE_COV_SUBSCRIPTION_FAILED:
        case ERROR_CODE_FILE_ACCESS_DENIED:
        case ERROR_CODE_INCONSISTENT_OBJECT_TYPE:
        case ERROR_CODE_INCONSISTENT_PARAMETERS:
        case ERROR_CODE_INVALID_CONFIGURATION_DATA:
        case ERROR_CODE_INVALID_EVENT_STATE:
        case ERROR_CODE_INVALID_FILE_ACCESS_METHOD:
        case ERROR_CODE_INVALID_FILE_START_POSITION:
        case ERROR_CODE_INVALID_PARAMETER_DATA_TYPE:
        case ERROR_CODE_INVALID_TAG:
        case ERROR_CODE_INVALID_TIME_STAMP:
        case ERROR_CODE_LIST_ELEMENT_NOT_FOUND:
        case ERROR_CODE_MISSING_REQUIRED_PARAMETER:
        case ERROR_CODE_NO_DEFAULT_SCOPE:
        case ERROR_CODE_NO_POLICY:
        case ERROR_CODE_PARAMETER_OUT_OF_RANGE:
        case ERROR_CODE_PROPERTY_IS_NOT_A_LIST:
        case ERROR_CODE_SERVICE_REQUEST_DENIED:
        case ERROR_CODE_UNKNOWN_AUDIENCE:
        case ERROR_CODE_UNKNOWN_CLIENT:
        case ERROR_CODE_UNKNOWN_SCOPE:
        case ERROR_CODE_UNKNOWN_SUBSCRIPTION:
            error_class = ERROR_CLASS_SERVICES;
            break;
        case ERROR_CODE_ABORT_APDU_TOO_LONG:
        case ERROR_CODE_ABORT_APPLICATION_EXCEEDED_REPLY_TIME:
        case ERROR_CODE_ABORT_BUFFER_OVERFLOW:
        case ERROR_CODE_ABORT_INSUFFICIENT_SECURITY:
        case ERROR_CODE_ABORT_INVALID_APDU_IN_THIS_STATE:
        case ERROR_CODE_ABORT_OUT_OF_RESOURCES:
        case ERROR_CODE_ABORT_PREEMPTED_BY_HIGHER_PRIORITY_TASK:
        case ERROR_CODE_SECURITY_ERROR:
        case ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED:
        case ERROR_CODE_ABORT_TSM_TIMEOUT:
        case ERROR_CODE_ABORT_PROPRIETARY:
        case ERROR_CODE_ABORT_OTHER:
        case ERROR_CODE_ADDRESSING_ERROR:
        case ERROR_CODE_BVLC_FUNCTION_UNKNOWN:
        case ERROR_CODE_BVLC_PROPRIETARY_FUNCTION_UNKNOWN:
        case ERROR_CODE_DELETE_FDT_ENTRY_FAILED:
        case ERROR_CODE_DISTRIBUTE_BROADCAST_FAILED:
        case ERROR_CODE_DNS_ERROR:
        case ERROR_CODE_DNS_NAME_RESOLUTION_FAILED:
        case ERROR_CODE_DNS_RESOLVER_FAILURE:
        case ERROR_CODE_DNS_UNAVAILABLE:
        case ERROR_CODE_HEADER_ENCODING_ERROR:
        case ERROR_CODE_HEADER_NOT_UNDERSTOOD:
        case ERROR_CODE_HTTP_ERROR:
        case ERROR_CODE_HTTP_NOT_A_SERVER:
        case ERROR_CODE_HTTP_NO_UPGRADE:
        case ERROR_CODE_HTTP_PROXY_AUTHENTICATION_FAILED:
        case ERROR_CODE_HTTP_RESOURCE_NOT_LOCAL:
        case ERROR_CODE_HTTP_RESPONSE_MISSING_HEADER:
        case ERROR_CODE_HTTP_RESPONSE_TIMEOUT:
        case ERROR_CODE_HTTP_RESPONSE_SYNTAX_ERROR:
        case ERROR_CODE_HTTP_RESPONSE_VALUE_ERROR:
        case ERROR_CODE_HTTP_TEMPORARY_UNAVAILABLE:
        case ERROR_CODE_HTTP_UNEXPECTED_RESPONSE_CODE:
        case ERROR_CODE_HTTP_UPGRADE_REQUIRED:
        case ERROR_CODE_HTTP_UPGRADE_ERROR:
        case ERROR_CODE_HTTP_WEBSOCKET_HEADER_ERROR:
        case ERROR_CODE_IP_ADDRESS_NOT_REACHABLE:
        case ERROR_CODE_IP_ERROR:
        case ERROR_CODE_MESSAGE_INCOMPLETE:
        case ERROR_CODE_MESSAGE_TOO_LONG:
        case ERROR_CODE_NETWORK_DOWN:
        case ERROR_CODE_NODE_DUPLICATE_VMAC:
        case ERROR_CODE_NOT_A_BACNET_SC_HUB:
        case ERROR_CODE_NOT_ROUTER_TO_DNET:
        case ERROR_CODE_PAYLOAD_EXPECTED:
        case ERROR_CODE_READ_BDT_FAILED:
        case ERROR_CODE_READ_FDT_FAILED:
        case ERROR_CODE_REGISTER_FOREIGN_DEVICE_FAILED:
        case ERROR_CODE_REJECT_BUFFER_OVERFLOW:
        case ERROR_CODE_REJECT_INCONSISTENT_PARAMETERS:
        case ERROR_CODE_REJECT_INVALID_PARAMETER_DATA_TYPE:
        case ERROR_CODE_REJECT_INVALID_TAG:
        case ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER:
        case ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE:
        case ERROR_CODE_REJECT_TOO_MANY_ARGUMENTS:
        case ERROR_CODE_REJECT_UNDEFINED_ENUMERATION:
        case ERROR_CODE_REJECT_UNRECOGNIZED_SERVICE:
        case ERROR_CODE_REJECT_PROPRIETARY:
        case ERROR_CODE_REJECT_OTHER:
        case ERROR_CODE_ROUTER_BUSY:
        case ERROR_CODE_TCP_CONNECT_TIMEOUT:
        case ERROR_CODE_TCP_CONNECTION_REFUSED:
        case ERROR_CODE_TCP_CLOSED_BY_LOCAL:
        case ERROR_CODE_TCP_CLOSED_OTHER:
        case ERROR_CODE_TCP_ERROR:
        case ERROR_CODE_TIMEOUT:
        case ERROR_CODE_TLS_CLIENT_AUTHENTICATION_FAILED:
        case ERROR_CODE_TLS_CLIENT_CERTIFICATE_ERROR:
        case ERROR_CODE_TLS_CLIENT_CERTIFICATE_EXPIRED:
        case ERROR_CODE_TLS_CLIENT_CERTIFICATE_REVOKED:
        case ERROR_CODE_TLS_ERROR:
        case ERROR_CODE_TLS_SERVER_AUTHENTICATION_FAILED:
        case ERROR_CODE_TLS_SERVER_CERTIFICATE_ERROR:
        case ERROR_CODE_TLS_SERVER_CERTIFICATE_EXPIRED:
        case ERROR_CODE_TLS_SERVER_CERTIFICATE_REVOKED:
        case ERROR_CODE_UNEXPECTED_DATA:
        case ERROR_CODE_UNKNOWN_DEVICE:
        case ERROR_CODE_UNKNOWN_ROUTE:
        case ERROR_CODE_UNKNOWN_NETWORK_MESSAGE:
        case ERROR_CODE_WEBSOCKET_CLOSE_ERROR:
        case ERROR_CODE_WEBSOCKET_CLOSED_BY_PEER:
        case ERROR_CODE_WEBSOCKET_CLOSED_ABNORMALLY:
        case ERROR_CODE_WEBSOCKET_DATA_AGAINST_POLICY:
        case ERROR_CODE_WEBSOCKET_DATA_NOT_ACCEPTED:
        case ERROR_CODE_WEBSOCKET_ENDPOINT_LEAVES:
        case ERROR_CODE_WEBSOCKET_ERROR:
        case ERROR_CODE_WEBSOCKET_EXTENSION_MISSING:
        case ERROR_CODE_WEBSOCKET_FRAME_TOO_LONG:
        case ERROR_CODE_WEBSOCKET_PROTOCOL_ERROR:
        case ERROR_CODE_WEBSOCKET_SCHEME_NOT_SUPPORTED:
        case ERROR_CODE_WEBSOCKET_UNKNOWN_CONTROL_MESSAGE:
        case ERROR_CODE_WEBSOCKET_REQUEST_UNAVAILABLE:
        case ERROR_CODE_WRITE_BDT_FAILED:
            error_class = ERROR_CLASS_COMMUNICATION;
            break;
        case ERROR_CODE_NO_VT_SESSIONS_AVAILABLE:
        case ERROR_CODE_UNKNOWN_VT_CLASS:
        case ERROR_CODE_UNKNOWN_VT_SESSION:
        case ERROR_CODE_VT_SESSION_ALREADY_CLOSED:
        case ERROR_CODE_VT_SESSION_TERMINATION_FAILURE:
            error_class = ERROR_CLASS_VT;
            break;
        default:
            break;
    }

    return error_class;
}
