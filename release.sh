#!/bin/bash
# script adapted from
# https://sourceforge.net/p/forge/documentation/Using%20the%20Release%20API/
# sudo apt-get update -qq
# sudo apt-get install -qq build-essential mingw-w64 curl git
#
# Prior to running this script, be sure to:
# a) update CHANGELOG and version.h with new version number, and commit changes.
# After running this script, be sure to:
# a) create long term branch as bacnet-stack-x.y if needed

USERNAME='skarg'

if [ -z "$1" ]
then
  echo "Usage: `basename $0` 0.0.0"
  echo "Builds the Win32 release files, archives the source, and uploads them to sf.net"
  exit 1
fi

version="$1"
tools="bacnet-tools-$version"
tag_name="bacnet-stack-$version"
url_api='https://sourceforge.net/projects/bacnet/files'
url_frs="${USERNAME},bacnet@frs.sourceforge.net:/home/frs/project/b/ba/bacnet"
url_frs_tools="$url_frs/bacnet-tools"
url_frs_source="$url_frs/bacnet-stack"
work_tree=$(mktemp -d 2>/dev/null || mktemp -d -t 'mytmpdir')

function bacnet_tag() {
    echo "Tagging Source Code $tag_name"
    git tag $tag_name
}

function bacnet_create_work_tree() {
    git archive --format=tar $tag_name | tar -x -C $work_tree
    echo "work-tree is in $work_tree"
}

function bacnet_build_apps() {
    echo ""
    echo "Build Win32 Apps"
    export CC=i686-w64-mingw32-gcc
    export LD=i686-w64-mingw32-ld
    i686-w64-mingw32-gcc --version
    make -C $work_tree clean
    make -C $work_tree -s LEGACY=true win32
}

function bacnet_zip_apps() {
    echo "ZIP Win32 Tools"
    mkdir -p $work_tree/$tools
    cp $work_tree/bin/*.exe $work_tree/$tools
    cp $work_tree/bin/bvlc.bat $work_tree/$tools
    cp $work_tree/bin/readme.txt $work_tree/$tools
    cp $work_tree/apps/mstpcap/mstpcap.txt $work_tree/$tools
    zip -r $work_tree/$tools.zip $work_tree/$tools
    rm $work_tree/$tools/*.exe
    rm $work_tree/$tools/*.bat
    mv $work_tree/$tools.zip $work_tree/$tools/$tools.zip
}

function bacnet_upload_apps() {
    echo "Upload Win32 Tools with SCP"
    scp -r $work_tree/$tools $url_frs_tools
}

function bacnet_zip_source() {
    mkdir -p $work_tree/$tag_name
    echo "Release folder made at $work_tree/$tag_name"
    archive_zip_option="--format=zip --prefix=$tag_name/"
    archive_zip_output="--output=$work_tree/$tag_name/$tag_name.zip"
    echo "ZIP Source Code for Tag $tag_name"
    git archive $archive_zip_option $archive_zip_output $tag_name

    archive_tgz_option="--format=tar.gz --prefix=$tag_name/"
    archive_tgz_output="--output=$work_tree/$tag_name/$tag_name.tgz"
    echo "TGZ Source Code for Tag $tag_name"
    git archive $archive_tgz_option $archive_tgz_output $tag_name

    echo "Copying CHANGELOG, SECURITY, README to $work_tree/$tag_name"
    cp $work_tree/CHANGELOG.md $work_tree/$tag_name
    cp $work_tree/README.md $work_tree/$tag_name
    cp $work_tree/SECURITY.md $work_tree/$tag_name
}

function bacnet_upload_source() {
    echo "Upload Source Code with SCP"
    scp -r $work_tree/$tag_name $url_frs_source
}

function bacnet_settings() {
    echo "Set the default download for Windows and POSIX"
    api_key=""api_key=$SOURCEFORGE_RELEASE_API_KEY_SKARG""
    default_win='"default=windows"'
    default_posix='"default=mac&default=linux&default=bsd&default=solaris&default=others"'
    accept='"Accept: application/json"'
    url_tools="$url_api/bacnet-tools/$tools/$tools.zip"
    url_source="$url_api/bacnet-stack/$tag_name/$tag_name.tgz"
    curl -H $accept -X PUT -d $default_win -d $api_key $url_tools
    curl -H $accept -X PUT -d $default_posix -d $api_key $url_source
}

menu() {
echo -ne "
BACnet Stack Release Steps
1) Tag BACnet source code
2) Create Work Tree
3) Build BACnet apps for Win32
4) Zip BACnet apps for Win32
5) Upload BACnet apps to bacnet.sf.net
6) Archive BACnet source code
7) Upload BACnet source code to bacnet.sf.net
8) Configure bacnet.sf.net download settings
0) Exit
Choose an option: "
        read a
        case $a in
            1) bacnet_tag ; menu ;;
            2) bacnet_create_work_tree ; menu ;;
            3) bacnet_build_apps ; menu ;;
            4) bacnet_zip_apps ; menu ;;
            5) bacnet_upload_apps ; menu ;;
            6) bacnet_zip_source ; menu ;;
            7) bacnet_upload_source ; menu ;;
            8) bacnet_settings ; menu ;;
        0) exit 0 ;;
        *) echo -e $red"Invalid option."$clear; WrongCommand;;
        esac
}

# Call the menu function
menu
