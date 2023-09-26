#!/bin/bash
echo "Example of parameters for run BACnet REST server"

BACNET_HTTP_PORT=9000
export BACNET_HTTP_PORT

BACNET_HTTPS_PORT=9001
export BACNET_HTTPS_PORT

BACNET_CA_CERTIFICATE_FILE="certs/ca_cert.pem"
export BACNET_CA_CERTIFICATE_FILE

BACNET_SERVER_CERTIFICATE_FILE="certs/server_cert.pem"
export BACNET_SERVER_CERTIFICATE_FILE

BACNET_SERVER_CERTIFICATE_PRIVATE_KEY_FILE="certs/server_key.der"
export BACNET_SERVER_CERTIFICATE_PRIVATE_KEY_FILE

echo "Launching new shell using the BACnet REST server environment..."
/bin/bash
# sample
#./bacrest 123 Fred
