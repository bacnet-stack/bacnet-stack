#!/bin/bash
# Parallel bacepics connected with Foreign Device to BBMD
# Assume that we are using the default IP address:
BACNET_IP_ADDRESS=`ip route get 8.8.8.8 | fgrep src |  cut -f7 -d" "`
BACNET_IP_PORT=47808

echo "Spawning BBMD server at UDP port ${BACNET_IP_PORT}"

# Launch the local BBMD
./bacserv $BACNET_IP_PORT BACServer-$BACNET_IP_PORT &
# Allow the local BBMD time to get running
sleep 1

#list of devices from command line assigned UDP port number as key
epics_port_start=47810
port_key=${epics_port_start}
declare -A device
# capture the devices from the command line
for d in "$@"
do
	device[${port_key}]=${d}
	epics_port_max=$((port_key++))
done

# spawn the epics clients
export BACNET_BBMD_ADDRESS=$BACNET_IP_ADDRESS
export BACNET_BBMD_PORT=$BACNET_IP_PORT
echo "Spawning clients to register as foreign device to BBMD" $BACNET_BBMD_ADDRESS:$BACNET_BBMD_PORT
# note: leaving port 47809 free for example bvlc.sh clients
for port in $(seq ${epics_port_start} ${epics_port_max})
do
    # note: is there a limit to the number of Foreign Device Registrations?
    export BACNET_IP_PORT=${port} ; \
    ./bacepics ${device[${port}]} > epics-${device[${port}]}-${port}.txt &
    pids[${port}]=$!
    echo "Spawned EPICS client using UDP port ${port} reading ${device[${port}]}"
done

echo "Waiting for the clients to finish..."
for pid in ${pids[*]}; do
    wait $pid
done

killall bacserv
