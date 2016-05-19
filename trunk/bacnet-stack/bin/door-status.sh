#!/bin/bash
export BACNET_APDU_RETRIES=0
export BACNET_APDU_TIMEOUT=0

# bacucov: pid device-id object-type object-instance time-remaining
# property tag value [priority] [index]

# BACnetDoorStatus
for door_status in {0..1023}
do
   ./bin/bacucov 1 2 30 4 5 231 9 ${door_status}
done
