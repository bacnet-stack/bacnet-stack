#!/bin/bash
export BACNET_APDU_RETRIES=0
export BACNET_APDU_TIMEOUT=0

for reliability in {0..63}
do
   ./bin/bacucov 1 2 3 4 5 103 9 ${reliability}
done
