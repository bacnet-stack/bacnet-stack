FROM ubuntu:focal as builder

## Usage:
# docker build -t bacnet --target bacnet .
# docker run --rm --name bacnet -d bacnet <device_id> [<device_name>]

## To run client commands that affect this server from inside the same
## container, use BBMD Foreign Device Registration by first exporting
## the following:
# BACNET_IP_PORT, BACNET_BBMD_PORT, and BACNET_BBMD_ADDRESS

## For example:
# docker run --rm --name bacnet -d bacnet
# docker exec -ti bacnet bash
# export BACNET_IP_PORT=47809
# export BACNET_BBMD_PORT=47808
# export BACNET_BBMD_ADDRESS="$(hostname -I | cut -d' ' -f1)"
# /opt/bacnet/bin/bacwi -1 1234
# /opt/bacnet/bin/bacrp 1234 device 1234 object-name
## etc.

## Check bacnet-stack/bin/readme.txt for more docs related to the
## example utilities and environment variables. Note that this docker
## image does not add the shell scripts in that folder as many of them
## are broken. Even so, reading them may be instructive.


SHELL ["/bin/bash", "-c"]

RUN set -xe; \
  apt-get update; apt-get upgrade -y; apt-get --purge autoremove -y; \
  apt-get install -y build-essential curl; \
	apt-get -y autoclean; apt-get -y clean

RUN set -euxo pipefail; \
  mkdir -p /build/bin; \
  curl -LSs https://github.com/bacnet-stack/bacnet-stack/archive/refs/tags/bacnet-stack-1.0.0.tar.gz -o bacnet-stack.tar.gz; \
  tar -xzf bacnet-stack.tar.gz; \
  ( cd bacnet-*/; \
    make; \
    rm -f -- bin/*.{bat,sh,txt}; \
    mv -- bin/* /build/bin/; \
  ); \
  rm -rf bacnet-*/


FROM ubuntu:focal AS bacnet
WORKDIR /opt/bacnet
COPY --from=builder /build/bin/* /opt/bacnet/bin/
EXPOSE 47808/udp
ENTRYPOINT ["/opt/bacnet/bin/bacserv"]
CMD ["1234", "test_server"]
