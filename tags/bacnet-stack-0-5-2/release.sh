#!/bin/sh
# Release helper for this project

PROJECT=bacnet
SVN_MODULE=bacnet-stack
CHANGELOG=ChangeLog
FRS_URL=skarg,bacnet@frs.sourceforge.net:/home/frs/project/b/ba/bacnet

if [ -z "$1" ]
then
  echo "Usage: `basename $0` 0.0.0"
  echo "Creates the ChangeLog."
  echo "Creates the release files."
  echo "Tags the current version in subversion."
  exit 1
fi

# convert 0.0.0 to 0-0-0
DOTTED_VERSION="$1"
DASHED_VERSION="$(echo "$1" | sed 's/[\.*]/-/g')"

echo "Creating the release files for version $DOTTED_VERSION"

echo "Creating the $PROJECT change log..."
rm $CHANGELOG
svn update
svn log --xml --verbose | xsltproc svn2cl.xsl - > $CHANGELOG
if [ -z "$CHANGELOG" ]
then
echo "Failed to create $CHANGELOG"
else
echo "$CHANGELOG created."
fi

ARCHIVE_NAME=$SVN_MODULE-$DOTTED_VERSION

SVN_TRUNK_NAME=https://$PROJECT.svn.sourceforge.net/svnroot/$PROJECT/trunk/$SVN_MODULE
SVN_TAGGED_NAME=https://$PROJECT.svn.sourceforge.net/svnroot/$PROJECT/tags/$SVN_MODULE-$DASHED_VERSION
echo "Setting a tag on the $SVN_MODULE module called $SVN_MODULE-$DASHED_VERSION"
svn copy -m "Created version $ARCHIVE_NAME" $SVN_TRUNK_NAME $SVN_TAGGED_NAME
echo "done."

echo "Getting a clean version out of subversion for Linux gzip"
svn export $SVN_TAGGED_NAME $ARCHIVE_NAME
echo "done."

GZIP_FILENAME=$ARCHIVE_NAME.tgz
echo "tar and gzip the clean directory"
tar -cvvzf $GZIP_FILENAME $ARCHIVE_NAME/
echo "done."

if [ -z "$GZIP_FILENAME" ]
then
echo "Failed to create $GZIP_FILENAME"
else
echo "$GZIP_FILENAME created."
fi

rm -rf $ARCHIVE_NAME

echo "Getting another clean version out of subversion for Windows zip"
svn export --native-eol CRLF $SVN_TAGGED_NAME $ARCHIVE_NAME
ZIP_FILENAME=$ARCHIVE_NAME.zip
echo "done."
echo "Zipping the directory exported for Windows."
zip -r $ZIP_FILENAME $ARCHIVE_NAME

if [ -z "$ZIP_FILENAME" ]
then
echo "Failed to create $ZIP_FILENAME"
else
echo "$ZIP_FILENAME created."
fi

rm -rf $ARCHIVE_NAME

echo "Sending to SourceForge..."

mkdir $ARCIVE_NAME
mv $ZIP_FILENAME $ARCIVE_NAME
mv $GZIP_FILENAME $ARCIVE_NAME
mv $CHANGELOG $ARCHIVE_NAME
echo "scp -r $ARCHIVE_NAME $FRS_URL"

echo "Complete!"

