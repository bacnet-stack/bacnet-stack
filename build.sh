#!/bin/sh

rm -rf _build
mkdir _build
cd _build
cmake .. -DBUILD_SHARED_LIBS=ON
make
