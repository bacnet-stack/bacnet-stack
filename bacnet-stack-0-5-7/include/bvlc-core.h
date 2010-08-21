#ifndef __BVCL_CORE_H_
#define __BVCL_CORE_H_

/* Handle the BACnet Virtual Link Control (BVLC), which includes:
   BACnet Broadcast Management Device,
   Broadcast Distribution Table, and
   Foreign Device Registration */
typedef struct {
    /* true if valid entry - false if not */
    bool valid;
    /* BACnet/IP address */
    struct in_addr dest_address;
    /* BACnet/IP port number - not always 47808=BAC0h */
    uint16_t dest_port;
    /* Broadcast Distribution Mask - stored in host byte order */
    struct in_addr broadcast_mask;
} BBMD_TABLE_ENTRY;

#define MAX_BBMD_ENTRIES 128

/*Each device that registers as a foreign device shall be placed
in an entry in the BBMD's Foreign Device Table (FDT). Each
entry shall consist of the 6-octet B/IP address of the registrant;
the 2-octet Time-to-Live value supplied at the time of
registration; and a 2-octet value representing the number of
seconds remaining before the BBMD will purge the registrant's FDT
entry if no re-registration occurs. This value will be initialized
to the 2-octet Time-to-Live value supplied at the time of
registration.*/
typedef struct {
    bool valid;
    /* BACnet/IP address */
    struct in_addr dest_address;
    /* BACnet/IP port number - not always 47808=BAC0h */
    uint16_t dest_port;
    /* seconds for valid entry lifetime */
    uint16_t time_to_live;
    /* our counter */
    time_t seconds_remaining;   /* includes 30 second grace period */
} FD_TABLE_ENTRY;
#define MAX_FD_ENTRIES 128

/*/ BVLC Result handler */
typedef void (
    *bvlc_result_handler_function) (
    struct bacnet_session_object * session_object,
    BACNET_ADDRESS * src,
    uint16_t result_code);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*/ Set result handler function callback */
    void bvlc_set_result_handler(
        struct bacnet_session_object *session_object,
        bvlc_result_handler_function result_function);
/*/ Calls result handler function callback, if present */
    void bvlc_call_result_handler(
        struct bacnet_session_object *session_object,
        BACNET_ADDRESS * src,
        uint16_t result_code);

#ifdef __cplusplus
}       /*extern "C" */
#endif /* __cplusplus */
#endif /* __BVCL_CORE_H_ */
