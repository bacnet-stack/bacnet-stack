#!/bin/sh
# Unit tests builder / runner for this project

make -f test.mak clean
make -f test.mak
cat test.log | grep Failed
