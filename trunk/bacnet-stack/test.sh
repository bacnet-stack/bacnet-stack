#!/bin/sh
# Unit tests builder / runner for this project

rm test.log
touch test.log

make -f crc.mak
./crc >> test.log
make -f crc.mak clean

make -f ringbuf.mak
./ringbuf >> test.log
make -f ringbuf.mak clean

make -f mstp.mak
./mstp >> test.log
make -f mstp.mak clean


