#!/bin/sh
# fix DOS/Unix names and Subversion EOL-Style, and remove backup files

# exit silently if utility is not installed
[ -x /usr/bin/dos2unix ] || exit 0
[ -x /usr/bin/svn ] || exit 0

directory=${1-`pwd`}
for filename in $( find ${directory} -name '*.c' )
do
  echo Fixing DOS/Unix ${filename}
  /usr/bin/dos2unix ${filename}
  echo Setting Subversion EOL Style for ${filename}
  /usr/bin/svn propset svn:eol-style native ${filename}
done

for filename in $( find ${directory} -name '*.h' )
do
  echo Fixing DOS/Unix ${filename}
  /usr/bin/dos2unix ${filename}
  echo Setting Subversion EOL Style for ${filename}
  /usr/bin/svn propset svn:eol-style native ${filename}
done

for filename in $( find ${directory} -name '*~' )
do
  echo Removing backup ${filename}
  rm ${filename}
done

