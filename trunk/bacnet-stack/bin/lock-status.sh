#!/bin/bash
export BACNET_APDU_RETRIES=0
export BACNET_APDU_TIMEOUT=0

# bacucov: pid device-id object-type object-instance time-remaining
# property tag value [priority] [index]

# BACnetLockStatus
for lock_status in {0..4}
do
   ./bin/bacucov 1 2 30 4 5 233 9 ${lock_status}
done
