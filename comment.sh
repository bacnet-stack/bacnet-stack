#!/bin/sh
directory=${1-`pwd`}
for filename in $( find $directory -name '*.c' )
do
  echo Converting $filename
  ccmtcnvt $filename > /tmp/ccmtcnvt.karg
  mv /tmp/ccmtcnvt.karg $filename
done

for filename in $( find $directory -name '*.h' )
do
  echo Converting $filename
  ccmtcnvt $filename > /tmp/ccmtcnvt.karg
  mv /tmp/ccmtcnvt.karg $filename
done

