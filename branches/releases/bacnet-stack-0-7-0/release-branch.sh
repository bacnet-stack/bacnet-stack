#!/bin/sh
# Release helper for this project

PROJECT=bacnet
SVN_MODULE=bacnet-stack
FRS_URL=skarg,bacnet@frs.sourceforge.net:/home/frs/project/b/ba/bacnet/bacnet-stack

if [ -z "$1" ] && [ -z "$2" ]
then
  echo "Usage: `basename $0` 0.7.0 0.7.1"
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
if [ -e "${CHANGELOG}" ]
then
rm ${CHANGELOG}
fi
svn update
svn log --xml --verbose | xsltproc svn2cl.xsl - > ${CHANGELOG} > /dev/null
if [ -e "${CHANGELOG}" ]
then
echo "${CHANGELOG} created."
else
echo "Failed to create ${CHANGELOG}"
fi

BRANCH_NAME=${SVN_MODULE}-${BRANCH_VERSION_DASHED}
ARCHIVE_NAME=${SVN_MODULE}-${TAGGED_VERSION_DOTTED}
TAGGED_NAME=${SVN_MODULE}-${TAGGED_VERSION_DASHED}
SVN_BASE_URL=https://${PROJECT}.svn.sourceforge.net/svnroot/${PROJECT}

SVN_BRANCH_NAME=${SVN_BASE_URL}/branches/releases/${BRANCH_NAME}
SVN_TAGGED_NAME=${SVN_BASE_URL}/tags/${TAGGED_NAME}
echo "Setting a tag on the ${SVN_MODULE} module called ${TAGGED_NAME}"
SVN_MESSAGE="Created version ${ARCHIVE_NAME}"
svn copy ${SVN_BRANCH_NAME} ${SVN_TAGGED_NAME} -m ${SVN_MESSAGE} > /dev/null
echo "done."

if [ -e "${ARCHIVE_NAME}" ]
then
rm -rf ${ARCHIVE_NAME}
fi
echo "Getting a clean version out of subversion for Linux gzip"
svn export ${SVN_TAGGED_NAME} ${ARCHIVE_NAME} > /dev/null
echo "done."
GZIP_FILENAME=${ARCHIVE_NAME}.tgz
if [ -e "${GZIP_FILENAME}" ]
then
rm ${GZIP_FILENAME}
fi
echo "tar and gzip the clean directory"
tar -cvvzf ${GZIP_FILENAME} ${ARCHIVE_NAME}/ > /dev/null
echo "done."

if [ -e "${GZIP_FILENAME}" ]
then
echo "${GZIP_FILENAME} created."
else
echo "Failed to create ${GZIP_FILENAME}"
fi

echo "Removing the directory exported for Linux."
rm -rf ${ARCHIVE_NAME}

echo "Getting another clean version out of subversion for Windows zip"
if [ -e "${ZIP_FILENAME}" ]
then
rm ${ZIP_FILENAME}
fi
svn export --native-eol CRLF ${SVN_TAGGED_NAME} ${ARCHIVE_NAME} > /dev/null
ZIP_FILENAME=${ARCHIVE_NAME}.zip
echo "done."
echo "Zipping the directory exported for Windows."
zip -r ${ZIP_FILENAME} ${ARCHIVE_NAME} > /dev/null

if [ -e "${ZIP_FILENAME}" ]
then
echo "${ZIP_FILENAME} created."
else
echo "Failed to create ${ZIP_FILENAME}"
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
