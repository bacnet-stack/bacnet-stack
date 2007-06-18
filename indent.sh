#!/bin/sh
# indent uses a local indent.pro file if it exists
# echo "-kr -nut -nlp" > .indent.pro

directory=${1-`pwd`}
for filename in $( find $directory -name '*.c' )
do
  echo Fixing DOS/Unix $filename
  dos2unix $filename
  echo Indenting $filename
  indent -kr -nut -nlp $filename
done

for filename in $( find $directory -name '*.h' )
do
  echo Fixing DOS/Unix $filename
  dos2unix $filename
  echo Indenting $filename
  indent -kr -nut -nlp $filename
done

for filename in $( find $directory -name '*~' )
do
  echo Removing backup $filename
  rm $filename
done

