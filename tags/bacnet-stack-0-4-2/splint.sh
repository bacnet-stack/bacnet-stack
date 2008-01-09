#!/bin/sh
# splint is a static code checker
[ -x /usr/bin/splint ] || exit 0

INCLUDES="-Iinclude -Idemo/handler -Idemo/object -Iports/linux"
SETTINGS="-castfcnptr -fullinitblock -weak +posixlib"

if [ ! -x .splintrc ]
then
  echo ${INCLUDES} ${SETTINGS} > .splintrc
fi

directory=${1-`pwd`}
for filename in $( find $directory -name '*.c' )
do
  echo splinting ${filename}
  /usr/bin/splint ${filename}
done

