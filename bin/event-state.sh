#!/bin/bash
export BACNET_APDU_RETRIES=0
export BACNET_APDU_TIMEOUT=0

# bacucov: pid device-id object-type object-instance time-remaining
# property tag value [priority] [index]

# BACnetEventState
# Event_State (36)
for event_state in {0..64}
do
   ./bin/bacucov 1 2 0 1 5 36 9 ${event_state}
done
