#!/bin/bash
export BACNET_APDU_RETRIES=0
export BACNET_APDU_TIMEOUT=0

for units in {0..255}
do
   ./bin/bacucov 1 2 3 4 5 117 9 ${units}
done
