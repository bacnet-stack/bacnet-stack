#!/bin/bash
export BACNET_APDU_RETRIES=0
export BACNET_APDU_TIMEOUT=0

for property_id in {0..512}
do
   ./bin/bacrp --dnet 65535 0 8 0 ${property_id}
done
