# Getting Started

* Install [AFL](https://github.com/google/AFL), ensure afl-gcc exists on the system:

```
$ afl-gcc
afl-cc 2.57b by <lcamtuf@google.com>
```

* Build via `make fuzz-afl` from repository root
* Clone a decent [corpus](https://github.com/CrystalPeakSecurity/bacnet-corpus/tree/main)
* Start AFL and feed it the input/output directories along with target executable

```
afl-fuzz -i </path/to/corpus/> -o </path/to/output_dir/> -m none ./apps/fuzz-afl/fuzz-afl
```

Caveats:

* This builds the target with ASAN (Address Sanitizer). This makes AFL require the `-m none` to not interpret ASAN's behavior as a crash
* AFL uses a fork/exec model to launch the target. This is nice because each testcase is from a clean state. But this also brings in a lot of overhead. If you need something faster, check out ../fuzz-libfuzzer/
