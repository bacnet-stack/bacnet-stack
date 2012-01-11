#!/bin/sh
# Release helper for this project

PROJECT=bacnet
SVN_MODULE=bacnet-stack
FRS_URL=skarg,bacnet@frs.sourceforge.net:/home/frs/project/b/ba/bacnet/bacnet-stack

if [ -z "$1" ]
then
  echo "Usage: `basename $0` 0.0.0 0.0.1"
  echo "Creates the ChangeLog."
  echo "Creates the release files."
  echo "Tags this branch release in subversion."
  exit 1
fi

# convert 0.0.0 to 0-0-0
BRANCH_VERSION_DOTTED="$1"
BRANCH_VERSION_DASHED="$(echo "$1" | sed 's/[\.*]/-/g')"
TAGGED_VERSION_DOTTED="$2"
TAGGED_VERSION_DASHED="$(echo "$2" | sed 's/[\.*]/-/g')"
echo "Creating the ${TAGGED_VERSION_DOTTED} release files for $(BRANCH_VERSION_DOTTED)"

CHANGELOG=ChangeLog-${TAGGED_VERSION_DOTTED}
echo "Creating the ${PROJECT} change log ${CHANGELOG}"
rm ${CHANGELOG}
svn update
svn log --xml --verbose | xsltproc svn2cl.xsl - > ${CHANGELOG}
if [ -z "${CHANGELOG}" ]
then
echo "Failed to create ${CHANGELOG}"
else
echo "${CHANGELOG} created."
fi

BRANCH_NAME=${SVN_MODULE}-${BRANCH_VERSION_DASHED}
ARCHIVE_NAME=${SVN_MODULE}-${TAGGED_VERSION_DOTTED}
TAGGED_NAME=${SVN_MODULE}-${TAGGED_VERSION_DASHED}
SVN_BASE_URL=https://${PROJECT}.svn.sourceforge.net/svnroot/${PROJECT}

SVN_BRANCH_NAME=${SVN_BASE_URL}/branches/releases/${BRANCH_NAME}
SVN_TAGGED_NAME=${SVN_BASE_URL}/tags/${TAGGED_NAME}
echo "Setting a tag on the ${SVN_MODULE} module called ${TAGGED_NAME}"
svn copy ${SVN_BRANCH_NAME} ${SVN_TAGGED_NAME} -m "Created version ${ARCHIVE_NAME}"
echo "done."

echo "Getting a clean version out of subversion for Linux gzip"
svn export ${SVN_TAGGED_NAME} ${ARCHIVE_NAME}
echo "done."

GZIP_FILENAME=${ARCHIVE_NAME}.tgz
echo "tar and gzip the clean directory"
tar -cvvzf ${GZIP_FILENAME} ${ARCHIVE_NAME}/
echo "done."

if [ -z "${GZIP_FILENAME}" ]
then
echo "Failed to create ${GZIP_FILENAME}"
else
echo "${GZIP_FILENAME} created."
fi

echo "Removing the directory exported for Linux."
rm -rf ${ARCHIVE_NAME}

echo "Getting another clean version out of subversion for Windows zip"
svn export --native-eol CRLF ${SVN_TAGGED_NAME} ${ARCHIVE_NAME}
ZIP_FILENAME=${ARCHIVE_NAME}.zip
echo "done."
echo "Zipping the directory exported for Windows."
zip -r ${ZIP_FILENAME} ${ARCHIVE_NAME}

if [ -z "${ZIP_FILENAME}" ]
then
echo "Failed to create ${ZIP_FILENAME}"
else
echo "${ZIP_FILENAME} created."
fi

echo "Removing the directory exported for Windows."
rm -rf ${ARCHIVE_NAME}

echo "Creating ${ARCHIVE_NAME}"
mkdir ${ARCHIVE_NAME}
mv ${ZIP_FILENAME} ${ARCHIVE_NAME}
mv ${GZIP_FILENAME} ${ARCHIVE_NAME}
mv ${CHANGELOG} ${ARCHIVE_NAME}
cp readme.txt ${ARCHIVE_NAME}

echo "Sending ${ARCHIVE_NAME} to SourceForge using scp..."
scp -r ${ARCHIVE_NAME} ${FRS_URL}

echo "Complete!"
