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

make -f bacerror.mak clean
make -f bacerror.mak
./bacerror >> test.log
make -f bacerror.mak clean

