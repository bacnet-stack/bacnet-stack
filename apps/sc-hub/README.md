# BACnet/SC Hub

Test BACnet/SC using the following steps:

 * Build apps for bsc datalink:

        make bsc

 * Run hub app:

        cd bin
        ./bsc-server.sh
        ./bacschub 1 Hubster

 * Run server app:

        cd bin
        ./bsc-client.sh
        ./bacserv 123 Francine

 * Run any client:

        cd bin
        ./bsc-client.sh
        ./bacwi
        ./bacepics 1
        ./bacepics 123


