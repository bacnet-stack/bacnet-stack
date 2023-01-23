#!/bin/bash
rm -fr *.pem
rm -fr *.der
rm -fr *.csr
rm -fr *.p12

openssl ecparam -out root_priv.key.pem -name prime256v1 -genkey
openssl req -new -x509 -key root_priv.key.pem -out ca_cert.pem -days 730 -subj "/C=US/ST=CA/O=Legrand/CN=BACNet-SC CA Root"
openssl ecparam -out node_priv.key.pem -name prime256v1 -genkey
openssl req -new -nodes -sha256 -key node_priv.key.pem -subj "/C=US/ST=CA/O=Legrand/CN=BACNet-SC node" -out node.csr
openssl x509 -req -in node.csr -CA ca_cert.pem -CAkey root_priv.key.pem -CAcreateserial -out node_cert.pem -days 3500 -sha256
openssl ecparam -out hub_priv.key.pem -name prime256v1 -genkey
openssl req -new -nodes -sha256 -key hub_priv.key.pem -subj "/C=US/ST=CA/O=Legrand/CN=BACNet-SC hub" -out hub.csr
openssl x509 -req -in hub.csr -CA ca_cert.pem -CAkey root_priv.key.pem -CAcreateserial -out hub_cert.pem -days 3500 -sha256

openssl ec -in hub_priv.key.pem -outform DER -out hub_priv.key.der
openssl ec -in node_priv.key.pem -outform DER -out node_priv.key.der

openssl pkcs12 -export -out node_cert.p12 -in node_cert.pem -inkey node_priv.key.pem

