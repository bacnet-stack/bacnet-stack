# Getting Started

* Ensure Clang & [compiler-rt](https://compiler-rt.llvm.org/) are installed on the system. On Arch Linux, this is package `compiler-rt`
* Build via `make fuzz-libfuzzer` from repository root
* Clone a decent [corpus](https://github.com/CrystalPeakSecurity/bacnet-corpus/tree/main)
* Start and feed it the corpus directory

```
$ ./apps/fuzz-libfuzzer/fuzz-libfuzzer ../bacnet-corpus/corpus/
INFO: Running with entropic power schedule (0xFF, 100).
INFO: Seed: 4165043991
INFO: Loaded 1 modules   (9101 inline 8-bit counters): 9101 [0x55a0d5487090, 0x55a0d548941d),
INFO: Loaded 1 PC tables (9101 PCs): 9101 [0x55a0d5489420,0x55a0d54accf0),
INFO:     5790 files found in ../bacnet-corpus/corpus/
INFO: -max_len is not provided; libFuzzer will not generate inputs larger than 4096 bytes
INFO: seed corpus: files: 5790 min: 1b max: 483b total: 305068b rss: 40Mb
NPDU: Decoding failed; Discarded!
...
NPDU: DNET=65280.  Discarded!
#5867	REDUCE cov: 208 ft: 249 corp: 66/1273b lim: 115 exec/s: 2933 rss: 106Mb L: 12/109 MS: 1 EraseBytes-
Received Reserved for Use by ASHRAE
WP: Failed to send PDU (Success)!
	NEW_FUNC[1/3]: 0x55a0d533f98a in abort_encode_apdu /mnt/net/lab_share/Bacnet/bacnet-stack-fixes/src/bacnet/abort.c:149
	NEW_FUNC[2/3]: 0x55a0d538f29a in wp_decode_service_request /mnt/net/lab_share/Bacnet/bacnet-stack-fixes/src/bacnet/wp.c:113
#5940	NEW    cov: 216 ft: 257 corp: 68/1282b lim: 115 exec/s: 2970 rss: 106Mb L: 6/109 MS: 2 EraseBytes-ChangeBit-
Received Reserved for Use by ASHRAE
...
=================================================================
==2204295==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x60200003b656 at pc 0x55a0d5350276 bp 0x7fff17646790 sp 0x7fff17646788
READ of size 1 at 0x60200003b656 thread T0
    #0 0x55a0d5350275 in decode_tag_number_and_value /mnt/net/lab_share/Bacnet/bacnet-stack-fixes/src/bacnet/bacdcode.c:496:13
    #1 0x55a0d5360b44 in bacerror_decode_error_class_and_code /mnt/net/lab_share/Bacnet/bacnet-stack-fixes/src/bacnet/bacerror.c:78:16
    #2 0x55a0d539851e in apdu_handler /mnt/net/lab_share/Bacnet/bacnet-stack-fixes/src/bacnet/basic/service/h_apdu.c:684:27
    #3 0x55a0d5331fff in my_routing_npdu_handler /mnt/net/lab_share/Bacnet/bacnet-stack-fixes/apps/fuzz-libfuzzer/./../router-mstp/main.c:977:25
    #4 0x55a0d5331fff in LLVMFuzzerTestOneInput /mnt/net/lab_share/Bacnet/bacnet-stack-fixes/apps/fuzz-libfuzzer/main.c:144:9
    #5 0x55a0d5216878 in fuzzer::Fuzzer::ExecuteCallback(unsigned char const*, unsigned long) (/mnt/net/lab_share/Bacnet/bacnet-stack-fixes/apps/fuzz-libfuzzer/fuzz-libfuzzer+0xc2878) (BuildId: 6c764deec893b5d17197c0a0183d5b491e36f6c9)
    #6 0x55a0d5217550 in fuzzer::Fuzzer::RunOne(unsigned char const*, unsigned long, bool, fuzzer::InputInfo*, bool, bool*) (/mnt/net/lab_share/Bacnet/bacnet-stack-fixes/apps/fuzz-libfuzzer/fuzz-libfuzzer+0xc3550) (BuildId: 6c764deec893b5d17197c0a0183d5b491e36f6c9)
    #7 0x55a0d52185e1 in fuzzer::Fuzzer::MutateAndTestOne() (/mnt/net/lab_share/Bacnet/bacnet-stack-fixes/apps/fuzz-libfuzzer/fuzz-libfuzzer+0xc45e1) (BuildId: 6c764deec893b5d17197c0a0183d5b491e36f6c9)
    #8 0x55a0d5219407 in fuzzer::Fuzzer::Loop(std::vector<fuzzer::SizedFile, std::allocator<fuzzer::SizedFile>>&) (/mnt/net/lab_share/Bacnet/bacnet-stack-fixes/apps/fuzz-libfuzzer/fuzz-libfuzzer+0xc5407) (BuildId: 6c764deec893b5d17197c0a0183d5b491e36f6c9)
    #9 0x55a0d51f98d5 in fuzzer::FuzzerDriver(int*, char***, int (*)(unsigned char const*, unsigned long)) (/mnt/net/lab_share/Bacnet/bacnet-stack-fixes/apps/fuzz-libfuzzer/fuzz-libfuzzer+0xa58d5) (BuildId: 6c764deec893b5d17197c0a0183d5b491e36f6c9)
    #10 0x55a0d51e3717 in main (/mnt/net/lab_share/Bacnet/bacnet-stack-fixes/apps/fuzz-libfuzzer/fuzz-libfuzzer+0x8f717) (BuildId: 6c764deec893b5d17197c0a0183d5b491e36f6c9)
    #11 0x7fc3c263984f  (/usr/lib/libc.so.6+0x2384f) (BuildId: 2f005a79cd1a8e385972f5a102f16adba414d75e)
    #12 0x7fc3c2639909 in __libc_start_main (/usr/lib/libc.so.6+0x23909) (BuildId: 2f005a79cd1a8e385972f5a102f16adba414d75e)
    #13 0x55a0d51e3754 in _start (/mnt/net/lab_share/Bacnet/bacnet-stack-fixes/apps/fuzz-libfuzzer/fuzz-libfuzzer+0x8f754) (BuildId: 6c764deec893b5d17197c0a0183d5b491e36f6c9)
...
```

Caveat: 

* Libfuzzer does not reinitialize the target on each testcase. This means that it will be much quicker than ../fuzz-afl/, BUT it will also be a little less stable. It also will not continue to fuzz after a crash is found (since the libfuzzer runtime shares a process with the target that just crashed). There may be some command line options to adopt a fork model.

