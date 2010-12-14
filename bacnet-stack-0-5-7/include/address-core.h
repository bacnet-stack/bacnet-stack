#ifndef __ADDRESS_CORE_H_
#define __ADDRESS_CORE_H_

/* A device id is bound to a MAC address. */
/* The normal method is using Who-Is, and using the data from I-Am */
struct Address_Cache_Entry {
    uint8_t Flags;
    uint32_t device_id;
    unsigned max_apdu;
    uint8_t segmentation;
    uint32_t maxsegments;
    BACNET_ADDRESS address;
    uint32_t TimeToLive;
};

#endif /* __ADDRESS_CORE_H_ */
