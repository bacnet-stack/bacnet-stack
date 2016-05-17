#!/bin/bash
for vendor_id in {0..999}
do
   ./bin/baciam 4194303 ${vendor_id}
done
