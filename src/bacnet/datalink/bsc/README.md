This readme explains how to enable support of BACNet Secure Connect datalink
(ANNEX AB – BACnet Secure Connect in standard), how to set up the building
environment and clarifies some important moments regarding implementation
of that functionality.

By default the support of that functionality is turned off, to enable it
you need to set option BACDL_BSC=ON in CMakeLists.txt if cmake build system
is used or set BACDL_BSC=1 define in a case if using makefiles.

BACNet/SC standard uses websockets as a transport layer, so Windows/Linux/BSD
implementations use libwebsocket library. Implementation for Zephyr uses
native websocket API for the client side and mongoose library for the
server side. Websocket layer which is built on the top of libwebsockets
uses 1 service thread per websocket server instance and 1 service thread
per 1 websocket client instance. As a result, libwebsocket must be built
with LWS_MAX_SMP > 1, otherwise rarely crashes may ocure in the application
which uses bacnet stack library. Recommended value for that define is 32.
You should note, that by default libwebsocket is built with LWS_MAX_SMP=1,
so a packet manager like vcpkg and apt can have that lib built with
LWS_MAX_SMP=1, which can lead to unstable work. It's better to build
libwebsockets from recent stable sources for your platform, corresponded
examples of how to do that you can find in .github/workflows/bsc-tests-platform.yml,
(for example bsc-tests-linux.yml).

In order to build bacnet stack library for linux, user needs to install
libconfig-dev, libcap-dev and libssl-dev to the system. Most easiest way to do
that is to use Advanced Packaging Tool (APT), check bsc-test-linux.yml.

For MacOSX build user must install brew packet manager (https://brew.sh),
then install openssl using brew then build libwebsocket, check
.github/workflows/bsc-tests-macosx.yml.

Windows build may be a challenge because libwebsocket depends on libpthreads
which not so easy to find and build on windows, refer to the libwebsocket
build instruction https://libwebsockets.org/lws-api-doc-master/html/md_README_8build.html.
Using of vcpkg packet manager can save a lot of time, but you must ensure that
installed libwebsocket library is compiled with LWS_MAX_SMP > 1.
A good example how to use vcpkg packet manager with custom build of
libwebsocket can be found in .github/workflows/bsc-tests-windows.yml.

One moment is still needed to be improved. The current implementation processes
dataflow between BACNET/SC nodes ignoring max_bvlc_size and max_npdu_size
params in connect request and connect accept packets as that is not
explicitly explained in standard. As a result if a node which has max_npdu_size
less than remote peer can drop received packet if it does not fit into it's internal
buffer which depends on BVLC_SC_NPDU_SIZE_CONF parameter. In opposite, a node
can send a PDU more than max_npdu_size which may also lead to the drop of PDU
on remote peer side.

The current implementation does not support Certificate_Signing_Request_File
property of BACNET/SC netport and properties Operational_Certificate_File
and Issuer_Certificate_Files related to certificates are readonly and can't
be changed remotely. So, management of device certificates is out of the scope
of current BACNet/SC implementation.
