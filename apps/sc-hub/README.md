# BACnet/SC Hub

Test BACnet/SC using the following steps:

 * Build all the apps for the BACnet/SC datalink:

       make bsc

 * Build the BACnet/SC hub:

       make sc-hub

 * Run the BACnet/SC hub application:

        cd bin
        ./bsc-server.sh
        ./bacschub 1 Hubster

 * Run BACnet/SC server application:

        cd bin
        ./bsc-client.sh
        ./bacserv 123 Francine

 * Run any BACnet/SC client:

        cd bin
        ./bsc-client.sh
        ./bacwi
        ./bacepics 1
        ./bacepics 123
