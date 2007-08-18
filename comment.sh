#!/bin/sh
# This script converts any C++ comments to C comments
# using the ccmtcnvt tool from the liwc package

# silent fail if the tool is not installed
[ -x /usr/bin/ccmtcnvt ] || exit 0

directory=${1-`pwd`}
for filename in $( find $directory -name '*.c' )
do
  echo Converting $filename
  /usr/bin/ccmtcnvt $filename > /tmp/ccmtcnvt.karg
  mv /tmp/ccmtcnvt.karg $filename
done

for filename in $( find $directory -name '*.h' )
do
  echo Converting $filename
  /usr/bin/ccmtcnvt $filename > /tmp/ccmtcnvt.karg
  mv /tmp/ccmtcnvt.karg $filename
done

