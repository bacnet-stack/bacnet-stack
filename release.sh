#!/bin/bash
# script adapted from
# https://sourceforge.net/p/forge/documentation/Using%20the%20Release%20API/
# sudo apt-get update -qq
# sudo apt-get install -qq build-essential mingw-w64 curl git
#
# Prior to running this script, be sure to:
# a) update CHANGELOG and version.h with new version number, and commit changes.
# b) git tag to bacnet-stack-x.y.z where x.y.z is the new version number
# c) create long term branch as bacnet-stack-x.y if needed

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

function bacnet_tag() {
    echo "Tagging Source Code $tag_name"
    git tag $tag_name
}

function bacnet_build() {
    echo ""
    echo "Build Win32 Apps"
    export CC=i686-w64-mingw32-gcc
    export LD=i686-w64-mingw32-ld
    i686-w64-mingw32-gcc --version
    make clean
    make -s LEGACY=true win32
}

function bacnet_zip() {
    echo "ZIP Win32 Tools"
    mkdir -p $tools
    cp ./bin/*.exe $tools
    cp ./bin/bvlc.bat $tools
    cp ./bin/readme.txt $tools
    cp ./apps/mstpcap/mstpcap.txt $tools
    zip -r $tools.zip $tools
    rm $tools/*.exe
    rm $tools/*.bat
    mv $tools.zip $tools/$tools.zip
}

function bacnet_source() {
    echo "ZIP Source Code for Tag $tag_name"
    git archive --format zip --output $tag_name.zip $tag_name

    echo "TGZ Source Code for Tag $tag_name"
    git archive --format tgz --output $tag_name.tgz $tag_name

    mkdir -p $tag_name
    mv $tag_name.zip $tag_name
    mv $tag_name.tgz $tag_name
    cp CHANGELOG.md $tag_name
    cp README.md $tag_name
    cp SECURITY.md $tag_name
}

function bacnet_upload() {
    echo "Upload Win32 Tools with SCP"
    scp -r $tools $url_frs_tools

    echo "Upload Source Code with SCP"
    scp -r $tag_name $url_frs_source
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

menu(){
echo -ne "
BACnet Stack Release Steps
1) Tag BACnet source code
2) Build BACnet apps for Win32
3) Zip BACnet apps for Win32
4) Archive BACnet source code
5) Upload files to bacnet.sf.net
6) Configure bacnet.sf.net download settings
0) Exit
Choose an option: "
        read a
        case $a in
	        1) bacnet_tag ; menu ;;
	        2) bacnet_build ; menu ;;
	        3) bacnet_zip ; menu ;;
	        4) bacnet_source ; menu ;;
	        5) bacnet_upload ; menu ;;
	        6) bacnet_settings ; menu ;;
		0) exit 0 ;;
		*) echo -e $red"Invalid option."$clear; WrongCommand;;
        esac
}

# Call the menu function
menu
