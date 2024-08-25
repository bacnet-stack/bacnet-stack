#!/bin/bash

PROG=`basename $0`
OPTIONAL=0

usage()
{
 echo "usage: $PROG [OPTIONS] <<BACnetID>> [ <<BACnetID>> ... ]

    Will return Required and Optional property values
    from the requested device.

    -o            Display optional properties [default behavior]
    -O            Supress display of optional properties
    -h            Display this help
"
}

while getopts ":oOh" opt; do
    case $opt in
        o  ) OPTIONAL=1
            ;;
        O  ) OPTIONAL=0
            ;;
        h  ) usage
            exit 1
            ;;
        \? ) usage
            exit 1
    esac
done
shift $(($OPTIND -1))

if [ $# -eq 0 ] || [ "$1" = "" ] ; then
    usage
    exit
fi

run_test()
{
    echo -e -e "Test: Read Required Properties of Device Object $1\r"
    echo -n "OBJECT IDENTIFIER:"
    ./bacrp $1 8 $1 75
    echo -n "OBJECT NAME:"
    ./bacrp $1 8 $1 77
    echo -n "OBJECT TYPE:"
    ./bacrp $1 8 $1 79
    echo -n "SYSTEM STATUS:"
    ./bacrp $1 8 $1 112
    echo -n "VENDOR NAME:"
    ./bacrp $1 8 $1 121
    echo -n "VENDOR IDENTIFIER:"
    ./bacrp $1 8 $1 120
    echo -n "MODEL NAME:"
    ./bacrp $1 8 $1 70
    echo -n "FIRMWARE REVISION:"
    ./bacrp $1 8 $1 44
    echo -n "APPLICATION SOFTWARE VERSION:"
    ./bacrp $1 8 $1 12
    echo -n "PROTOCOL VERSION:"
    ./bacrp $1 8 $1 98
    echo -n "PROTOCOL REVISION:"
    ./bacrp $1 8 $1 139
    echo -n "PROTOCOL SERVICES SUPPORTED:"
    ./bacrp $1 8 $1 97
    echo -n "OBJECT TYPES SUPPORTED:"
    ./bacrp $1 8 $1 96
    echo -n "OBJECT LIST LENGTH:"
    ./bacrp $1 8 $1 76 0
    echo -n "OBJECT LIST:"
    ./bacrp $1 8 $1 76
    echo -n "MAX APDU LENGTH ACCEPTED:"
    ./bacrp $1 8 $1 62
    echo -n "SEGMENTATION SUPPORTED:"
    ./bacrp $1 8 $1 107
    echo -n "APDU TIMEOUT:"
    ./bacrp $1 8 $1 11
    echo -n "NUMGER OF APDU ENTRIES:"
    ./bacrp $1 8 $1 73
    echo -n "DEVICE ADDRESS BINDING:"
    ./bacrp $1 8 $1 30
    echo -n "DATABASE REVISION:"
    ./bacrp $1 8 $1 155
    if [ $OPTIONAL -eq 1 ] ; then
        echo -e "Test: Read Optional Properties of Device Object $1\r"
        echo -n "LOCATION:"
        ./bacrp $1 8 $1 58
        echo -n "DESCRIPTION:"
        ./bacrp $1 8 $1 28
        echo -n "MAX SEGMENTS SUPPORTED:"
        ./bacrp $1 8 $1 167
        echo -n "VT CLASSES SUPPORTED:"
        ./bacrp $1 8 $1 122
        echo -n "ACTIVE VT SESSIONS:"
        ./bacrp $1 8 $1 5
        echo -n "LOCAL TIME:"
        ./bacrp $1 8 $1 57
        echo -n "LOCAL DATE:"
        ./bacrp $1 8 $1 56
        echo -n "UTC OFFSET:"
        ./bacrp $1 8 $1 119
        echo -n "DAYLIGHT SAVINGS STATUS:"
        ./bacrp $1 8 $1 24
        echo -n "APDU SEGMENT TIMEOUT:"
        ./bacrp $1 8 $1 10
        echo -n "LIST OF SESSION KEYS:"
        ./bacrp $1 8 $1 55
        echo -n "TIME SYNCHRONIZATION RECIPIENTS:"
        ./bacrp $1 8 $1 116
        echo -n "MAX MASTER:"
        ./bacrp $1 8 $1 64
        echo -n "MAX INFO FRAMES:"
        ./bacrp $1 8 $1 63
        echo -n "ACK REQUIRED:"
        ./bacrp $1 8 $1 1
        echo -n "CONFIGURATION FILES:"
        ./bacrp $1 8 $1 154
        echo -n "LAST RESTORE TIME:"
        ./bacrp $1 8 $1 157
        echo -n "BACKUP FAILURE TIMEOUT:"
        ./bacrp $1 8 $1 153
        echo -n "ACTIVE COV SUBSCRIPTIONS:"
        ./bacrp $1 8 $1 152
        echo -n "SLAVE PROXY ENABLE:"
        ./bacrp $1 8 $1 172
        echo -n "MANUAL SLAVE ADDRESS BINDING:"
        ./bacrp $1 8 $1 170
        echo -n "AUTO SLAVE DISCOVERY:"
        ./bacrp $1 8 $1 169
        echo -n "SLAVE ADDRESS BINDING:"
        ./bacrp $1 8 $1 171
        echo -n "PROFILE NAME:"
        ./bacrp $1 8 $1 168
    fi
    echo -e " \r"
}

while [ $# -gt 0 ] ; do
    ID=$(( $1 + 0 ))
    shift
    if [ $ID -eq 0 ] ; then
        echo "ERROR: Device ID must be an integer!! [ID=$ID]" >&2
    fi
    run_test $ID
done
