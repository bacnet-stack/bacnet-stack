#!/bin/bash
export BACNET_APDU_RETRIES=0
export BACNET_APDU_TIMEOUT=0

# BACnetPropertyStates
#BACnetNotificationParameters ::= CHOICE {
#-- These choices have a one-to-one correspondence with the Event_Type
#-- enumeration with the exception of the
#-- complex-event-type, which is used for proprietary event types.
#change-of-state - [1] SEQUENCE {
#   new-state [0] BACnetPropertyStates,
#   status-flags [1] BACnetStatusFlags
#},
for property_states in {0..63}
do
   ./bin/bacuevent 1 15 1 0 1 99 1 16 1 ${property_states} 1 0000 "Tribble!" 1 0 0 0
done
