#!/bin/sh
# Unit tests builder / runner for this project

make -s -f test.mak clean
make -s -f test.mak
cat test.log | grep Failed
