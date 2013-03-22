#!/bin/sh
# Release helper for this project

SVN_PROJECT=branches/releases/bacnet-stack-0-8-0
SVN_BASE_URL=http://svn.code.sf.net/p/bacnet/code/
SVN_TRUNK_NAME=${SVN_BASE_URL}${SVN_PROJECT}

if [ -z "$1" ]
then
  echo "Usage: `basename $0` export-directory"
  echo "Exports HEAD of ${SVN_PROJECT} in Windows CRLF format"
  exit 1
fi

echo "Getting another clean version out of subversion for Windows zip"
svn export --native-eol CRLF ${SVN_TRUNK_NAME} ${1}
echo "done."

echo "Complete!"
