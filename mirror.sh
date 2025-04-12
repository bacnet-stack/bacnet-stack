#!/bin/bash

# tools needed
# sudo apt-get update -qq
# sudo apt-get install -qq git

mirror_source='git@github.com:bacnet-stack/bacnet-stack.git'
mirror_dest='ssh://skarg@git.code.sf.net/p/bacnet/src'
mirror_tree=$(mktemp -d 2>/dev/null || mktemp -d -t 'bacnet-mirror')

echo "Mirror the Github repository with Sourceforge.net"
echo "Temporary directory for mirroring: $mirror_tree"
cd $mirror_tree
echo "Cloning from source: $mirror_source"
git clone --bare $mirror_source .
echo "Pushing to destination: $mirror_dest"
git push --mirror $mirror_dest
cd -
