#!/bin/sh
# splint is a static code checker

SPLINT=/usr/bin/splint

[ -x ${SPLINT} ] || exit 0

DEFINES="-D__signed__=signed -D__gnuc_va_list=va_list"
INCLUDES="-Iinclude -Idemo/object -Iports/linux"
SETTINGS="-castfcnptr -fullinitblock -initallelements -weak -warnposixheaders"
SPLINT_LOGFILE=splint_output.txt

if [ ! -e .splintrc ]
then
  echo ${DEFINES} ${INCLUDES} ${SETTINGS} > .splintrc
fi

directory=${1-`pwd`}/src
rm -f splint_output.txt
touch splint_output.txt
for filename in $( find $directory -name '*.c' )
do
  echo splinting ${filename}
  echo splinting ${filename} >> ${SPLINT_LOGFILE} 
  ${SPLINT} ${filename} >> ${SPLINT_LOGFILE} 2>&1
done

