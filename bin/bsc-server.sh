#!/bin/bash
echo "Example of parameters for BACnet/SC server"

BACNET_SC_ISSUER_1_CERTIFICATE_FILE="certs/ca_cert.pem"
export BACNET_SC_ISSUER_1_CERTIFICATE_FILE

# Second CA certificate is not used yet
#BACNET_SC_ISSUER_2_CERTIFICATE_FILE="certs/ca_cert2.pem"
#export BACNET_SC_ISSUER_2_CERTIFICATE_FILE

BACNET_SC_OPERATIONAL_CERTIFICATE_FILE="certs/server_cert.pem"
export BACNET_SC_OPERATIONAL_CERTIFICATE_FILE

BACNET_SC_OPERATIONAL_CERTIFICATE_PRIVATE_KEY_FILE="certs/server_key.pem"
export BACNET_SC_OPERATIONAL_CERTIFICATE_PRIVATE_KEY_FILE

# Need if the device is HUB.
# Possible values: or port number, like "9999" or pair interface name and
# port number, like "eth0:50050"
BACNET_SC_HUB_FUNCTION_BINDING="50050"
export BACNET_SC_HUB_FUNCTION_BINDING

# Need if the device is not a HUB
#BACNET_SC_PRIMARY_HUB_URI="wss://127.0.0.1:50050"
#export BACNET_SC_PRIMARY_HUB_URI
#BACNET_SC_FAILOVER_HUB_URI="wss://127.0.0.1:50050"
#export BACNET_SC_FAILOVER_HUB_URI

# Need if the device allows the direct connect.
# Possible values: or port number, like "5000" or pair interface name and
# port number, like "eth0:50050"
#BACNET_SC_DIRECT_CONNECT_BINDING="eth0:1234"
#export BACNET_SC_DIRECT_CONNECT_BINDING

# Set if the device supports the direct connect initiate
# Value: if allow then "1", "y", "Y", otherwise forbidden
#BACNET_SC_DIRECT_CONNECT_INITIATE="y"
#export BACNET_SC_DIRECT_CONNECT_INITIATE

# List of direct connect accept URLs separated by a space character
#BACNET_SC_DIRECT_CONNECT_ACCEPT_URLS="wss://192.0.0.1:40000 wss://192.0.0.2:6666"
#export BACNET_SC_DIRECT_CONNECT_ACCEPT_URLS

echo "Launching new shell using the BACnet/SC server environment..."
/bin/bash
