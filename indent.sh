#!/bin/sh
# indent uses a local indent.pro file if it exists

# exit silently if utility is not installed
[ -x /usr/bin/indent ] || exit 0
[ -x /usr/bin/dos2unix ] || exit 0

if [ ! -x .indent.pro ] 
then
  echo "-kr -nut -nlp -ip4 -cli4" > .indent.pro
fi

directory=${1-`pwd`}
for filename in $( find $directory -name '*.c' )
do
  echo Fixing DOS/Unix $filename
  /usr/bin/dos2unix $filename
  echo Indenting $filename
  /usr/bin/indent $filename
done

for filename in $( find $directory -name '*.h' )
do
  echo Fixing DOS/Unix $filename
  /usr/bin/dos2unix $filename
  echo Indenting $filename
  /usr/bin/indent $filename
done

for filename in $( find $directory -name '*~' )
do
  echo Removing backup $filename
  rm $filename
done

