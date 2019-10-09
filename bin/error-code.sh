#!/bin/bash
export BACNET_APDU_RETRIES=0
export BACNET_APDU_TIMEOUT=0

# bacabort [invoke-id abort-reason server]

# BACnetErrorCode
export error_class=0
for error_code in {0..256}
do
   ./bin/bacerror ${error_class} ${error_code}
done
