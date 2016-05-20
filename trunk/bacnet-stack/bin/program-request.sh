#!/bin/bash
export BACNET_APDU_RETRIES=0
export BACNET_APDU_TIMEOUT=0

# bacucov: pid device-id object-type object-instance time-remaining
# property tag value [priority] [index]

# BACnetProgramRequest
# PROP_PROGRAM_CHANGE = 90
# OBJECT_PROGRAM = 16
# BACNET_APPLICATION_TAG_ENUMERATED = 9
for program_request in {0..16}
do
   ./bin/bacucov 1 2 16 1 5 90 9 ${program_request}
done
