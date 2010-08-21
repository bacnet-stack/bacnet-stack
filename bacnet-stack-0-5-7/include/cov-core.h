#ifndef __COV_CORE_H_
#define __COV_CORE_H_

/* note: This COV service only monitors the properties
   of an object that have been specified in the standard.  */
struct BACnet_My_COV_Subscription {
    bool valid;
    BACNET_ADDRESS dest;
    uint32_t subscriberProcessIdentifier;
    BACNET_OBJECT_ID monitoredObjectIdentifier;
    bool issueConfirmedNotifications;   /* optional */
    uint32_t lifetime;  /* optional */
    bool send_requested;
};
typedef struct BACnet_My_COV_Subscription BACNET_MY_COV_SUBSCRIPTION;

#define MAX_COV_SUBCRIPTIONS 32

#endif /*__COV_CORE_H_ */
