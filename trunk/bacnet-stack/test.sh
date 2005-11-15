#!/bin/sh
# Unit tests builder / runner for this project

rm test.log
touch test.log

make -f crc.mak clean
make -f crc.mak
./crc >> test.log
make -f crc.mak clean

make -f ringbuf.mak clean
make -f ringbuf.mak
./ringbuf >> test.log
make -f ringbuf.mak clean

make -f mstp.mak clean
make -f mstp.mak
./mstp >> test.log
make -f mstp.mak clean

make -f iam.mak clean
make -f iam.mak
./iam >> test.log
make -f iam.mak clean

make -f whois.mak clean
make -f whois.mak
./whois >> test.log
make -f whois.mak clean

make -f bacdcode.mak clean
make -f bacdcode.mak
./bacdcode >> test.log
make -f bacdcode.mak clean

make -f iam.mak clean
make -f iam.mak
./iam >> test.log
make -f iam.mak clean

make -f whois.mak clean
make -f whois.mak
./whois >> test.log
make -f whois.mak clean

make -f npdu.mak clean
make -f npdu.mak
./npdu >> test.log
make -f npdu.mak clean

make -f reject.mak clean
make -f reject.mak
./reject >> test.log
make -f reject.mak clean

make -f abort.mak clean
make -f abort.mak
./abort >> test.log
make -f abort.mak clean

make -f bacerror.mak clean
make -f bacerror.mak
./bacerror >> test.log
make -f bacerror.mak clean

make -f device.mak clean
make -f device.mak
./device >> test.log
make -f device.mak clean

make -f ai.mak clean
make -f ai.mak
./analog_input >> test.log
make -f ai.mak clean

make -f ao.mak clean
make -f ao.mak
./analog_output >> test.log
make -f ao.mak clean

make -f wp.mak clean
make -f wp.mak
./writeproperty >> test.log
make -f wp.mak clean

make -f address.mak clean
make -f address.mak
./address >> test.log
make -f address.mak clean

make -f indtext.mak clean
make -f indtext.mak
./indtext >> test.log
make -f indtext.mak clean
