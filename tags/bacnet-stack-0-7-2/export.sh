#!/bin/sh
# Release helper for this project

PROJECT=bacnet
SVN_MODULE=bacnet-stack

if [ -z "$1" ]
then
  echo "Usage: `basename $0` export-directory"
  echo "Exports HEAD ${SVN_MODULE} in Windows CRLF format"
  exit 1
fi

SVN_BASE_URL=https://${PROJECT}.svn.sourceforge.net/svnroot/${PROJECT}
SVN_TRUNK_NAME=${SVN_BASE_URL}/trunk/${SVN_MODULE}

echo "Getting another clean version out of subversion for Windows zip"
svn export --native-eol CRLF ${SVN_TRUNK_NAME} ${1}
echo "done."

echo "Complete!"
