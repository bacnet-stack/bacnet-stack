## UCI is used on OpenWRT devices

If you have an OpenWRT build environment then you can use
https://github.com/stargieg/packages-automation

## Dependency Installation

### libubox

    git clone git://nbd.name/luci2/libubox.git
    cd libubox/; cmake -D BUILD_LUA:BOOL=OFF .;make
    sudo make install

### uci

    git clone git://nbd.name/uci.git
    cd uci/; cmake -D BUILD_LUA:BOOL=OFF .;make
    sudo make install

## Building using Make

### Linux

    make clean;make BUILD=debug BACNET_PORT=linux BACDL_DEFINE=-DBACDL_BIP=1 BACNET_DEFINES=" -DPRINT_ENABLED=1 -DINTRINSIC_REPORTING -DBACFILE -DBACAPP_ALL"

### MAC OS X with ports:

    make clean;make BACNET_PORT=bsd BUILD=release BACDL_DEFINE="-DBACDL_BIP=1" \
    BACNET_DEFINES="-I/opt/local/include -DBAC_UCI -DBACAPP_ALL -DBACNET_PROPERTY_LISTS -DINTRINSIC_REPORTING -DAI -DAO -DAV -DBI -DBO -DBV -DMSI -DMSO -DMSV -DTRENDLOG" \
    UCI_LIB_DIR=/usr/local/lib UCI_INCLUDE_DIR=/usr/local/include all

    make clean;make BACNET_PORT=bsd BUILD=release BACDL_DEFINE="-DBACDL_BIP6=1" \
    BACNET_DEFINES="-I/opt/local/include -DBAC_UCI -DBACAPP_ALL -DBACNET_PROPERTY_LISTS -DINTRINSIC_REPORTING -DAI -DAO -DAV -DBI -DBO -DBV -DMSI -DMSO -DMSV -DTRENDLOG" \
    UCI_LIB_DIR=/usr/local/lib UCI_INCLUDE_DIR=/usr/local/include server


### Make Flags

#### BACNET_DEFINES:

    -DBAC_ROUTING
    -DPRINT_ENABLED=1
    -DBACAPP_ALL
    -DBACFILE
    -DINTRINSIC_REPORTING

#### BACDL_DEFINE:

    -DBACDL_ETHERNET=1
    -DBACDL_ARCNET=1
    -DBACDL_MSTP=1
    -DBACDL_BIP=1

#### BBMD_DEFINE:

    -DBBMD_ENABLED=1
    -DBBMD_ENABLED=0
    -DBBMD_CLIENT_ENABLED

#### BACNET_PORT:
    bsd  linux win32

## Configure

Create uci configuration dir

    mkdir /etc/config

Create a device instance configuration

    touch /etc/config/bacnet_dev

Example Device configuration:

    config dev '0'
        option description 'Openwrt Router'
        option modelname 'Openwrt Router'
        option location 'Europe'
        option app_ver '12.09'
        option name 'openwrt-router-bip'
        option id '4711'
        option port '47808'
        option net '0'
        option iface 'lan'
        option bacdl 'bip'

    config dev '1'
        option description 'Openwrt Router'
        option modelname 'Openwrt Router'
        option location 'Europe'
        option app_ver '12.09'
        option name 'openwrt-router-ethernet'
        option id '4712'
        option net '0'
        option iface 'lan'
        option bacdl 'ethernet'

Create a Notification Class configuration:

    touch /etc/config/bacnet_nc

Example Notification Class configuration:

    config nc 'default'
        option description 'Notification Class default'
        option name 'Notification Class'
        option group 'ZF'

    config nc '0'
        option description 'Network Monitoring'
        option name 'Komunikationfehler'
        option group 'ZF'
        list recipient '65535'
        list recipient '1,104.13.8.92:47808'

    config nc '1'
        option description 'Modbus Sensor Fehler'
        option name 'Sensor Fehler'
        option group 'ZF'
        list recipient '65535'
        list recipient '1,104.13.8.92:47808'

Create a Trendlog configuration:
    touch /etc/config/bacnet_tl

Example Trendlog configuration:

    config tl 'default'
        option description 'Analog Value'
        option nc '1'
        option interval '300'
        option device_type 8
        option object_type 2

    config tl '0'
        option object_instance '0'
        option interval '10'

    config tl '1'
        option object_instance '1'

Create a Analog Value configuration:

    touch /etc/config/bacnet_av

Example Analog Value configuration:

    config av 'default'
        option si_unit '98'
        option description 'Analog Value'
        option nc '1'
        option event '7'
        option limit '3'
        option high_limit '40'
        option low_limit '0'
        option dead_limit '0'
        option cov_increment '0.1'
        option value '23.8'

    config av '0'
        option pgroup 'ZF_EZR08'
        option name 'R801_RT'
        option description 'Raumtemperatur'
        option addr '1'
        option tagname 'modbus-s1'
        option si_unit '62'
        option dead_limit '0.5'
        option cov_increment '0.2'
        option resolution 'doublefloat'
        option value '0.000000'
        option Out_Of_Service '0'
        option value_time '1384274334'

    config av '1'
        option pgroup 'ZF_EZR08'
        option name 'R802_RT'
        option description 'Raumtemperatur'
        option tagname 'modbus-s1'
        option si_unit '62'
        option resolution 'doublefloat'
        option addr '3'
        option value '0.000000'
        option Out_Of_Service '0'
        option value_time '1384274334'

Create a Multistate Value configuration

    touch /etc/config/bacnet_mv

Example Analog Value configuration:

    config mv 'default'
        list state 'up'
        list state 'down'
        list state 'unreachable'
        list state 'flaping'
        list alarmstate 'down'
        list alarmstate 'unreachable'
        list alarmstate 'flaping'
        option description 'Multi State Value'
        option nc '1'
        option event '7'

    config mv '0'
        option name 'TR_EZR00_SV01'
        option value '1'
        option description '192.168.100.29'

    config mv '1'
        option name 'TR_EZR01_SV01'
        option value '1'
        option description '192.168.100.30'

Create a Binary Value configuration:

    touch /etc/config/bacnet_bv

Example Analog Value configuration:

    config bv 'default'
        option description 'Binary Value'
        option inactive 'AUS'
        option active 'EIN'
        option nc '1'
        option event '7'
        option time_delay '3'

    config bv '0'
        option name 'BV_00'
        option alarm_value '0'
        option tagname 'modbus-s1'
        option addr '5'
        option bit '0'
        option resolution 'dword'
        option active 'Ein'
        option inactive 'Aus'
        option description 'Datenwort 2 Bit 0'
        option value '0'
        option Out_Of_Service '0'
        option value_time '1384274334'

    config bv '1'
        option name 'BV_01'
        option alarm_value '0'
        option tagname 'modbus-s1'
        option addr '5'
        option bit '1'
        option resolution 'dword'
        option active 'Ein'
        option inactive 'Aus'
        option description 'Datenwort 2 Bit 1'
        option Out_Of_Service '0'
        option value '0'
        option value_time '1384274334'

## Run

    BACNET_IFACE=en0 BACNET_DATALINK=bip BACNET_IP_PORT=47808 UCI_SECTION=0 bin/bacserv
    BACNET_IFACE=en0 BACNET_DATALINK=bip6 BACNET_IP_PORT=47808 UCI_SECTION=0 bin/bacserv

## Debug

    BACNET_IFACE=en0 BACNET_DATALINK=bip BACNET_IP_PORT=47808 UCI_SECTION=0 lldb bin/bacserv
    BACNET_IFACE=en0 BACNET_DATALINK=bip6 BACNET_IP_PORT=47808 UCI_SECTION=0 lldb bin/bacserv
