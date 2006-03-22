#!/bin/sh
# Unit tests builder / runner for this project

rm test.log
touch test.log

make -f abort.mak clean
make -f abort.mak
./abort >> test.log
make -f abort.mak clean

make -f address.mak clean
make -f address.mak
./address >> test.log
make -f address.mak clean

make -f dcc.mak clean
make -f dcc.mak
./dcc >> test.log
make -f dcc.mak clean

make -f demo/object/ai.mak clean
make -f demo/object/ai.mak
./analog_input >> test.log
make -f demo/object/ai.mak clean

make -f demo/object/ao.mak clean
make -f demo/object/ao.mak
./analog_output >> test.log
make -f demo/object/ao.mak clean

make -f arf.mak clean
make -f arf.mak
./atomicreadfile >> test.log
make -f arf.mak clean

make -f awf.mak clean
make -f awf.mak
./atomicwritefile >> test.log
make -f awf.mak clean

make -f bacapp.mak clean
make -f bacapp.mak
./bacapp >> test.log
make -f bacapp.mak clean

make -f bacdcode.mak clean
make -f bacdcode.mak
./bacdcode >> test.log
make -f bacdcode.mak clean

make -f bacerror.mak clean
make -f bacerror.mak
./bacerror >> test.log
make -f bacerror.mak clean

make -f bacstr.mak clean
make -f bacstr.mak
./bacstr >> test.log
make -f bacstr.mak clean

make -f demo/object/bi.mak clean
make -f demo/object/bi.mak
./binary_input >> test.log
make -f demo/object/bi.mak clean

make -f crc.mak clean
make -f crc.mak
./crc >> test.log
make -f crc.mak clean

make -f demo/object/device.mak clean
make -f demo/object/device.mak
./device >> test.log
make -f demo/object/device.mak clean

make -f iam.mak clean
make -f iam.mak
./iam >> test.log
make -f iam.mak clean

make -f ihave.mak clean
make -f ihave.mak
./ihave >> test.log
make -f ihave.mak clean

make -f indtext.mak clean
make -f indtext.mak
./indtext >> test.log
make -f indtext.mak clean

make -f mstp.mak clean
make -f mstp.mak
./mstp >> test.log
make -f mstp.mak clean

make -f npdu.mak clean
make -f npdu.mak
./npdu >> test.log
make -f npdu.mak clean

make -f rd.mak clean
make -f rd.mak
./reinitialize_device >> test.log
make -f rd.mak clean

make -f reject.mak clean
make -f reject.mak
./reject >> test.log
make -f reject.mak clean

make -f ringbuf.mak clean
make -f ringbuf.mak
./ringbuf >> test.log
make -f ringbuf.mak clean

make -f rp.mak clean
make -f rp.mak
./readproperty >> test.log
make -f rp.mak clean

make -f rpm.mak clean
make -f rpm.mak
./rpm >> test.log
make -f rpm.mak clean

make -f sbuf.mak clean
make -f sbuf.mak
./sbuf >> test.log
make -f sbuf.mak clean

make -f tsm.mak clean
make -f tsm.mak
./tsm >> test.log
make -f tsm.mak clean

make -f whois.mak clean
make -f whois.mak
./whois >> test.log
make -f whois.mak clean

make -f whohas.mak clean
make -f whohas.mak
./whohas >> test.log
make -f whohas.mak clean

make -f wp.mak clean
make -f wp.mak
./writeproperty >> test.log
make -f wp.mak clean
