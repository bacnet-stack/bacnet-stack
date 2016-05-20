#!/bin/bash
export BACNET_APDU_RETRIES=0
export BACNET_APDU_TIMEOUT=0

# bacucov: pid device-id object-type object-instance time-remaining
# property tag value [priority] [index]

# BACnetProgramState
# PROP_PROGRAM_STATE = 92,
# OBJECT_PROGRAM = 16
# BACNET_APPLICATION_TAG_ENUMERATED = 9
for program_state in {0..16}
do
   ./bin/bacucov 1 2 16 1 5 92 9 ${program_state}
done
