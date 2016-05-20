#!/bin/bash
export BACNET_APDU_RETRIES=0
export BACNET_APDU_TIMEOUT=0

# bacucov: pid device-id object-type object-instance time-remaining
# property tag value [priority] [index]

# BACnetEventType
# PROP_EVENT_TYPE = 37
# OBJECT_EVENT_ENROLLMENT = 9
# BACNET_APPLICATION_TAG_ENUMERATED = 9
for event_type in {0..64}
do
   ./bin/bacucov 1 1 9 1 5 37 9 ${event_type}
done
