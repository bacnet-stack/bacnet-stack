#!/bin/bash
export BACNET_APDU_RETRIES=0
export BACNET_APDU_TIMEOUT=0

# bacabort [abort-reason invoke-id server]

# BACnetAbortReason
for reason in {0..64}
do
   ./bin/bacabort ${reason}
done
