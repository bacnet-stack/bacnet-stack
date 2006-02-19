#!/bin/sh
# indent uses a local indent.pro file if it exists
# echo "-kr -nut -nlp" > .indent.pro

directory=${1-`pwd`}
for filename in $( find $directory -name '*.c' )
do
  echo Indenting $filename
  indent -kr -nut -nlp $filename
done

for filename in $( find $directory -name '*.h' )
do
  echo Indenting $filename
  indent -kr -nut -nlp $filename
done
