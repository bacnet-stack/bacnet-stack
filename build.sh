#!/bin/sh

rm -rf _build
mkdir _build
cd _build
cmake ..
make
