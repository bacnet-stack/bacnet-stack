#!/bin/bash
export BACNET_APDU_RETRIES=0
export BACNET_APDU_TIMEOUT=0

# bacucov: pid device-id object-type object-instance time-remaining
# property tag value [priority] [index]

# BACnetRestartReason
# PROP_LAST_RESTART_REASON = 196
# OBJECT_DEVICE = 8
# BACNET_APPLICATION_TAG_ENUMERATED = 9
for reason in {0..64}
do
   ./bin/bacucov 1 1 8 1 5 196 9 ${reason}
done
