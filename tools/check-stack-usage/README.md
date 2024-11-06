*(Detailed blog post [here](https://www.thanassis.space/stackusage.html) )*

# Introduction

This is a utility that computes the stack usage per function.  It is
particularly useful in embedded systems programming, where memory is at a
premium - and also in safety-critical SW *(blowing up from a stack overflow
while you operate medical equipment or fly in space is not exactly optimal)*.

# In detail

*(Detailed blog post [here](https://www.thanassis.space/stackusage.html) )*

You can read many details about how this script works in the blog post
linked above; but the executive summary is this:

- We expect the source code compilation to use GCC's `-fstack-usage`.
  This generates `.su` files with the stack usage of each function
  **(in isolation)** stored per compilation unit. Simply put, `file.c`
  compiled with `-fstack-usage` will create `file.o` *and* `file.su`.

- The script can then be launched like so:

    checkStackUsage.py binaryELF folderTreeContainingSUfiles

For example, if after the build we have a tree like this:

    bin/
        someBinary
    src/
        file1.c
        file1.o
        file1.su
        lib1_src/
                lib1.c
                lib1.o
                lib1.su
        lib2_src/
                lib2.c
                lib2.o
                lib2.su

...we run this:

    checkStackUsage.py bin/someBinary src/

The script will scan all .su files in the `src` folder *(recursively,
at any depth)* and collect the standalone use of stack for each function.

It will then launch the appropriate `objdump` with option `-d` - to 
disassemble the code, and create the call graph. Simplistically, it
detects patterns like this:

    <foo>:
        ....
        call <func>

...and proceeds from there to create the entire call graph.
It can then accumulate the use of all subordinate calls from each function,
and therefore compute it's total stack usage.

Output looks like this:

    176: foo (foo(16),func(160))
    288: func (func(288))
    304: bar (bar(16),func(288))
    320: main (main(16),bar(16),func(288))

...which means that function `foo` uses 176 bytes of stack; 16 because of
itself, and 160 because it calls `func`. `main` uses 320 bytes, etc.

Notice that `bar` also uses `func` - but reports a larger stack size for it
in that call chain. Read section "Repeated functions" below, to see why;
suffice to say, this is one of the few stack checkers that can cope with
symbols defined more than once.

# Platforms

The script needs to spawn the right `objdump`. It uses
`file` to detect the ELF signature, and uses appropriate regexes
to match disassembly `call` forms for:

  - SPARC/LEONs (used in the European Space Agency missions)
  - x86/amd64 (both 32 and 64 bits)
  - 32-bit ARM

Adding additional platforms is very easy - just tell the script what
`objdump` flavor to use, and what regex to scan for to locate the 
call sequences; [relevant code is here](checkStackUsage.py#L198).

# Repeated functions

Each function can only have a specific stack usage - right?

Sadly, no :-(

Feast yourself on moronic - yet perfectly valid - C code like this:

    // a.c
    static int func() { ...}
    void foo() { func(); }

    // b.c
    static int func() { ...}
    void bar() { func(); }

"Houston, we have a problem". While scanning the `.su` files for `a.c`
and `b.c`, we find `func` twice - and due to the `static`, we want
to use the right value on each call (from `foo`/`bar`) based on
filescope. In effect, the .su files' content need to be read
*prioritizing local static calls* when computing stack usage.

# Hidden calls

The scanning of `objdump` output for call sequences is the best we can do;
but it's not perfect. For example, any calls made via function pointer
indirections are "invisible" to this process.

And since fp-based calls can do all sorts of shenanigans - e.g.
reading the call endpoint from an array of functions via some 
algorithm - statically deducing which functions are actually called
is tantamount to the halting problem.

I am open to suggestions on this.

# Static Analysis

The script is written in Python - `make all` will check it with:

- `flake8` (PEP8 compliance)
- `pylint` (Static Analysis)
- ...and `mypy` (static type checking).

All dependencies to perform these checks will be automatically
installed via `pip` in a local virtual environment (i.e. 
under folder `.venv`) the first time you invoke `make all`.

# Test example

The scenario of repeated functions is tested via `make test`;
in my 64-bit Arch Linux I see this output:

    $ make test
    ...
    make[1]: Entering directory '/home/ttsiod/Github/checkStackUsage/tests'
    ==============================================
           176: foo (foo(16),func(160))
           288: func (func(288))
           304: bar (bar(16),func(288))
           320: main (main(16),bar(16),func(288))
    ==============================================
    1. The output shown above must contain 4 lines
    2. 'foo' and 'bar' must both be calling 'func'
       *but with different stack sizes in each*.
    3. 'main' must be using the largest 'func'
       (i.e. be going through 'bar')
    4. The reported sizes must properly accumulate
    ==============================================
    make[1]: Leaving directory '/home/ttsiod/Github/checkStackUsage/tests'

Given the content of [a.c](tests/a.c), [b.c](tests/b.c) and 
[main.c](tests/main.c), the output looks good. Notice that for
`func` we report the maximum of the two (the one reported by
GCC inside `b.c`, when it is called by `bar`).

# Feedback

If you see something wrong in the script that isn't documented above,
questions (and much better, pull requests) are most welcome.

Thanassis Tsiodras, Dr.-Ing.
ttsiodras_at-no-spam_thanks-gmail_dot-com

