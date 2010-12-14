#ifndef __BACNET_SESSION_H_
#define __BACNET_SESSION_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "net.h"
#include "datalink.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "bacint.h"

#if defined(BBMD_ENABLED) && BBMD_ENABLED || defined(BVLC_ENABLED) && BVLC_ENABLED
#include "bvlc-core.h"
#endif
#include "tsm.h"
#include "apdu.h"
#include "address-core.h"
#include "mydata.h"

#if PRINT_ENABLED
#include <stdio.h>      /* for standard integer types uint8_t etc. */
#endif




/*/ Session structure : contains multiple global static variables */
struct bacnet_session_object {
    /* Pointers to other data */
    void *Handler_data;
    void *Tag_data;

#if defined(BACDL_BIP) || defined (BACDL_ALL)
    /* IP ---------------------------------------------------- */
    int BIP_Socket;
    /* port to use - stored in host byte order */
    uint16_t BIP_Port;
    /* IP Address - stored in host byte order */
    struct in_addr BIP_Address;
    /* Broadcast Address - stored in host byte order */
    struct in_addr BIP_Broadcast_Address;
#endif /*IP */

#if defined(BACDL_ETHERNET) || defined (BACDL_ALL)
    /* Ethernet globals .... --------------------------------------------- */

#endif

#if defined(BACDL_MSTP) || defined (BACDL_ALL)
    /* MS/TP globals .... ---------------------------------------------------- */

    /* RS485 globals ....---------------------------------------------------- */

#endif

    /* BVLC ----------------------------------------------------  */
#if defined(BVLC_ENABLED) && BVLC_ENABLED
#if defined(BBMD_ENABLED) && BBMD_ENABLED
    BBMD_TABLE_ENTRY BVLC_BBMD_Table[MAX_BBMD_ENTRIES];
    FD_TABLE_ENTRY BVLC_FD_Table[MAX_FD_ENTRIES];
#endif
    /* result from a client request */
    BACNET_BVLC_RESULT BVLC_Result_Code;        /* = BVLC_RESULT_SUCCESSFUL_COMPLETION; */
    /* if we are a foreign device, store the
       remote BBMD address/port here in network byte order */
    struct sockaddr_in BVLC_Remote_BBMD;
    /* BVLC Result handler */
    bvlc_result_handler_function BVLC_result_handler;

#endif

#if (MAX_TSM_TRANSACTIONS)
    /* TSM ---------------------------------------------------- */
    /* Current InvokeID */
    uint8_t TSM_Current_InvokeID;
    /* State machine values */
    BACNET_TSM_DATA TSM_List[MAX_TSM_TRANSACTIONS];
    /* Indirection of state machine data with peer unique id values */
    BACNET_TSM_INDIRECT_DATA TSM_Peer_Ids[MAX_TSM_PEERS];
#endif

    /* APDU ---------------------------------------------------- */

    /* APDU Timeout in Milliseconds */
    uint16_t APDU_Timeout_Milliseconds; /* = 3000; */
    /* APDU Segmnet Timeout in Milliseconds */
    uint16_t APDU_Segment_Timeout_Milliseconds; /* = 3000; */
    /* Number of APDU Retries */
    uint8_t APDU_Number_Of_Retries;     /* = 3; */

    /* Confirmed Function Handlers */
    /* If they are not set, they are handled by a reject message */
    confirmed_function APDU_Confirmed_Function[MAX_BACNET_CONFIRMED_SERVICE];
    /* Allow the APDU handler to automatically reject */
    confirmed_function APDU_Unrecognized_Service_Handler;
    /* Unconfirmed Function Handlers */
    /* If they are not set, they are not handled */
                       unconfirmed_function
        APDU_Unconfirmed_Function[MAX_BACNET_UNCONFIRMED_SERVICE];
    /* Confirmed ACK Function Handlers */
    void *APDU_Confirmed_ACK_Function[MAX_BACNET_CONFIRMED_SERVICE];
    /* Error Function Handler */
    error_function APDU_Error_Function[MAX_BACNET_CONFIRMED_SERVICE];
    /* Abort Function Handler */
    abort_function APDU_Abort_Function;
    /* Reject Function Handler */
    reject_function APDU_Reject_Function;

    /* DCC */
    /* DCC disabled duration */
    uint32_t DCC_Time_Duration_Seconds; /* = 0; */
    /* DCC current status */
    BACNET_COMMUNICATION_ENABLE_DISABLE DCC_Enable_Disable;     /* = COMMUNICATION_ENABLE; */

    /* ADDRESS -------------------------------------------------------------- */
    struct Address_Cache_Entry Address_Cache[MAX_ADDRESS_CACHE];

#if !defined(WITH_MACRO_LINK_FUNCTIONS)
    /* Function Pointers ---------------------------------------------------- */

    /* Function pointers - point to your datalink */
                        bool(
        *datalink_init)     (
        struct bacnet_session_object * session_object,
        char *ifname);

    int (
        *datalink_send_pdu) (
        struct bacnet_session_object * session_object,
        BACNET_ADDRESS * dest,
        BACNET_NPDU_DATA * npdu_data,
        uint8_t * pdu,
        unsigned pdu_len);

        uint16_t(
        *datalink_receive) (
        struct bacnet_session_object * session_object,
        BACNET_ADDRESS * src,
        uint8_t * pdu,
        uint16_t max_pdu,
        unsigned timeout);

    void (
        *datalink_cleanup) (
        struct bacnet_session_object * session_object);

    void (
        *datalink_get_broadcast_address) (
        struct bacnet_session_object * session_object,
        BACNET_ADDRESS * dest);

    void (
        *datalink_get_my_address) (
        struct bacnet_session_object * session_object,
        BACNET_ADDRESS * my_address);
#endif

#if defined(WITH_SESSION_SYNCHRONISATION)
    /* wait for an event on this session */
         bool(
        *session_synchro_wait) (
        struct bacnet_session_object * session_object,
        int milliseconds);
    /* are we allowed to wait for an event on this session */
         bool(
        *session_synchro_can_wait) (
        struct bacnet_session_object * session_object);
    /* signal an event on this session */
    void (
        *session_synchro_signal) (
        struct bacnet_session_object * session_object);
    /* lock event on this session */
    void (
        *session_synchro_lock) (
        struct bacnet_session_object * session_object);
    /* unlock event on this session */
    void (
        *session_synchro_unlock) (
        struct bacnet_session_object * session_object);
#endif

#if defined(WITH_SESSION_LOG)
    /* log event */
    void (
        *session_log) (
        struct bacnet_session_object * session_object,
        int level,
        const char *message,
        BACNET_ADDRESS * addressinfo,
        int invokeIdInfo);
#endif

};

/* Special functions */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Sleeping ; waiting for an event on this session. NOP if not implemented within callbacks. 
 @param session_object The current session.
 @param millisecond The maximum wait timeout.
 @return true if we were signalled or there were no callbacks, false if we waited all timeout. */
    bool bacnet_session_wait(
        struct bacnet_session_object *session_object,
        int milliseconds);
/** Sleeping ; trying to wait for an event on this session, test if we are allowed to do so. 
 @param session_object The current session.
 @return true if we can use 'bacnet_session_wait', false otherwise or if there are no callbacks. */
    bool bacnet_session_can_wait(
        struct bacnet_session_object *session_object);
/** Sleeping ; signal an event on this session
   @param session_object The current session. */
    void bacnet_session_signal(
        struct bacnet_session_object *session_object);
/** Multi-threading: get a lock on this session object
   @param session_object The current session. */
    void bacnet_session_lock(
        struct bacnet_session_object *session_object);
/** Multi-threading: release a lock on this session object
   @param session_object The current session. */
    void bacnet_session_unlock(
        struct bacnet_session_object *session_object);
/** Log: outputs a log message from the core of the bacnet-stack
   @param message The log message.
   @param level The log level.
   @param session_object The current session. */
    void bacnet_session_log(
        struct bacnet_session_object *session_object,
        int level,
        const char *message,
        BACNET_ADDRESS * addressinfo,
        int invokeIdInfo);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /*__BACNET_SESSION_H_ */
