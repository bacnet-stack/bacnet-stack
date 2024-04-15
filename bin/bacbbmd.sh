#!/bin/bash
# Simulate BACnet servers connected with Foreign Device to BBMD

# Launch the local BBMD
./bacserv 47808 BACServer-47808 &
pids[47808]=$!
sleep 1

# spawn the servers
export BACNET_BBMD_ADDRESS=${1}
export BACNET_BBMD_PORT=${2}
echo "Spawn servers, register to BBMD" $BACNET_BBMD_ADDRESS:$BACNET_BBMD_PORT
# note: leaving port 47809 free for example bvlc.sh clients
for ((port=47810; port<=47910; port++))
do
    # note: is there a limit to the number of Foreign Device Registrations?
    export BACNET_IP_PORT=$port ; \
    ./bacserv $port BACServer-$port &
    pids[${port}]=$!
done

read -p "Press key to quit"

for pid in ${pids[*]}; do
    wait $pid
done
