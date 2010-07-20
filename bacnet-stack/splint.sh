#!/bin/sh
# splint is a static code checker

SPLINT=/usr/bin/splint

[ -x ${SPLINT} ] || exit 0

INCLUDES="-Iinclude -Iports/linux"
SETTINGS="-castfcnptr -fullinitblock -initallelements -weak +posixlib"

if [ ! -x .splintrc ]
then
  echo ${INCLUDES} ${SETTINGS} > .splintrc
fi

directory=${1-`pwd`}/src
for filename in $( find $directory -name '*.c' )
do
  echo splinting ${filename}
  ${SPLINT} ${filename}
done

