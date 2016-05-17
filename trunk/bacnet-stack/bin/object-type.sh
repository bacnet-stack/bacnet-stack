#!/bin/bash
export BACNET_APDU_RETRIES=0
export BACNET_APDU_TIMEOUT=0

for object_type in {0..127}
do
   ./bin/bacucov 1 2 ${object_type} 4 5 85 0 0
done
