#!/bin/bash
export BACNET_APDU_RETRIES=0
export BACNET_APDU_TIMEOUT=0

# bacucov: pid device-id object-type object-instance time-remaining
# property tag value [priority] [index]

# BACnetReliability
# PROP_RELIABILITY = 103,
# OBJECT_ANALOG_INPUT = 0
# BACNET_APPLICATION_TAG_ENUMERATED = 9
for reliability in {0..64}
do
   ./bin/bacucov 1 2 0 1 5 103 9 ${reliability}
done
