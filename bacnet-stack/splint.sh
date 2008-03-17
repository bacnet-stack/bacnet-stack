#!/bin/sh
# splint is a static code checker
[ -x /usr/bin/splint ] || exit 0

INCLUDES="-Iinclude -Iports/linux"
SETTINGS="-castfcnptr -fullinitblock -weak +posixlib"

if [ ! -x .splintrc ]
then
  echo ${INCLUDES} ${SETTINGS} > .splintrc
fi

directory=${1-`pwd`}/src
for filename in $( find $directory -name '*.c' )
do
  echo splinting ${filename}
  /usr/bin/splint ${filename}
done

