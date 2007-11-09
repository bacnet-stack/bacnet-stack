#!/bin/sh
# Unit tests builder / runner for this project

make -f test.mak clean
make -f test.mak

make -f demo/object/ai.mak clean
make -f demo/object/ai.mak
./analog_input >> test.log
make -f demo/object/ai.mak clean

make -f demo/object/ao.mak clean
make -f demo/object/ao.mak
./analog_output >> test.log
make -f demo/object/ao.mak clean

make -f demo/object/av.mak clean
make -f demo/object/av.mak
./analog_value >> test.log
make -f demo/object/av.mak clean

make -f demo/object/bi.mak clean
make -f demo/object/bi.mak
./binary_input >> test.log
make -f demo/object/bi.mak clean

make -f demo/object/bo.mak clean
make -f demo/object/bo.mak
./binary_output >> test.log
make -f demo/object/bo.mak clean

make -f demo/object/bv.mak clean
make -f demo/object/bv.mak
./binary_value >> test.log
make -f demo/object/bv.mak clean

make -f demo/object/device.mak clean
make -f demo/object/device.mak
./device >> test.log
make -f demo/object/device.mak clean

make -f demo/object/lc.mak clean
make -f demo/object/lc.mak
./loadcontrol >> test.log
make -f demo/object/lc.mak clean

make -f demo/object/lsp.mak clean
make -f demo/object/lsp.mak
./lsp >> test.log
make -f demo/object/lsp.mak clean

make -f demo/object/mso.mak clean
make -f demo/object/mso.mak
./multistate_output >> test.log
make -f demo/object/mso.mak clean

