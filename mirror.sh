#!/bin/bash

# tools needed
# sudo apt-get update -qq
# sudo apt-get install -qq git

mirror_source='git@github.com:bacnet-stack/bacnet-stack.git'
mirror_dest='ssh://skarg@git.code.sf.net/p/bacnet/src'
mirror_tree=$(mktemp -d 2>/dev/null || mktemp -d -t 'bacnet-mirror')

echo "Mirror the Github repository with Sourceforge.net"
cd $mirror_tree
git clone --bare $mirror_source .
git push --mirror $mirror_dest
cd -

