# avstack.pl

dlbeer@gmail.com  
31 May 2013 (updated 24 Aug 2015)

When developing for memory constrained systems, it's useful to know at compile-time that the available memory is sufficient for your application firmware. Dynamic allocation is usually avoided, and the size of statically allocated memory in the `.data` and `.bss` sections can be easily inspected with GNU `size` or a similary tool. This leaves one problem: stack use. The stack grows and shrinks dynamically at runtime, but under some circumstances, the peak stack use can be statically analyzed.

*   [avstack.pl](avstack.pl)

The script linked above is a static stack checker, intended for use with a recent-ish version of [AVR-GCC](http://gcc.gnu.org/wiki/avr-gcc). In order to use it, you must ensure that your object files are compiled with `-fstack-usage`. This causes GCC to generate a `.su` file for every `.o` file. [These files](http://gcc.gnu.org/onlinedocs/gnat_ugn_unw/Static-Stack-Usage-Analysis.html) contain information on the size of the stack frame for each function compiled in the `.o` file. For example:

    sha1.c:43:13:mix_w      0       static
    sha1.c:54:13:rot_left   0       static
    sha1.c:69:13:rot_right  0       static
    sha1.c:178:6:sha1_init  0       static
    sha1.c:183:6:sha1_next  65      static
    sha1.c:192:6:sha1_final 6       static

Once you have these files, invoke `avstack.pl`, passing as arguments the names of all `.o` files that will be linked into your binary. The `.su` files are assumed to be located in the same directories as their corresponding `.o` files.

The script reads all `.su` files, and disassembles all `.o` files, including relocation data. The disassemblies are parsed and used to construct a call graph. Multiple functions in different translation units with the same name don't cause problems, provided there are no global namespace collisions. Information will appear on any unresolvable or ambiguous references.

Next, the call graph is traced to find the peak stack usage of all functions. This is calculated for each function as the maximum stack usage of any of its callees, plus its own stack frame, plus some call-cost constant (not included in GCC's analysis).

Here's an example of the output produced:

    $ ./avstack.pl */*.o
      Func                               Cost    Frame  Height
    ------------------------------------------------------------------------
    > main                                235       90        4
      prng_next                           145       72        3
      sha1_final                           83       10        3
      sha1_next                            73       69        2
      __vector_16                          28       20        3
    > INTERRUPT                            28        0        4
      clock_signal_tick                     8        4        2
      clock_iterate                         8        4        2
      clock_init                            4        4        1
      led_init                              4        4        1
      led_set                               4        4        1
    > led_get                               4        4        1
      rot_right                             4        4        1
      mix_w                                 4        4        1
      prng_init                             4        4        1
      clock_signal_pps                      4        4        1
      clock_get_raw                         4        4        1
      clock_get                             4        4        1
      prng_stir                             4        4        1
      rot_left                              4        4        1
      sha1_init                             4        4        1
      led_slot_clock_tick                   4        4        1

    The following functions were not resolved:
      memcpy_P

The columns are:

*   **Cost**: peak stack usage during a call to the function.
*   **Frame**: stack frame size, obtained from the `.su` file, plus the call-cost constant.
*   **Height**: height in call graph -- calculated as maximum height of any callee, plus one.

Indicators to the left of the function name indicate features of the call graph. A `>` indicates that the function has no callee. This could be because it's an entry point (like `main` or an interrupt vector), because the function is called dynamically through a function pointer, or simply that it's unused. An `R` indicates that the function is recursive, and the cost estimate is for a single level of recursion. Multiple functions with the same name are distinguished in the listing by appending a suffix of the form `@filename.o`.

You can customize this script by altering two variables near the beginning:

    my $objdump = "avr-objdump";
    my $call_cost = 4;

Note that making sense of the output of the stack analysis still requires you to know something about how your program runs. For example, in many programs, the actual peak stack use would be the cost of `main`, plus the maximum cost of any interrupt handler that might execute. You will also need to take into account dynamically invoked functions, if you have any.

To make things easier, there is a processing section in which fake nodes and edges can be added to the call graph to account for dynamic behaviour. For example, the script currently contains in this section, to represent interrupt execution:

    $call_graph{"INTERRUPT"} = {};

    foreach (keys %call_graph) {
        $call_graph{"INTERRUPT"}->{$_} = 1 if /^__vector_/;
    }

## Use with C++

Updated: 24 Aug 2015

Unfortunately, GCC uses "printable" names in the output for `-fstack-usage`. For example, the function:

    namespace test {

    uint32_t crc32_ieee802_3(const uint8_t *data, size_t len,
                             uint32_t crc = 0)
    {
        ...
    }

    }

Gets listed as:

    foo.cpp:6:10:uint32_t test::crc32_ieee802_3(const uint8_t*, size_t, uint32_t)   32      static

This is difficult to match up with the disassembler output. Using `c++filt` doesn't help, because it doesn't know anything about typedefs (`uint8_t` becomes `unsigned char`, for example).

I've prepared the following patches for two versions of GCC. They're independent of language and target:

*   [gcc-4.7.2-mangled-stack-usage.patch](../downloads/gcc-4.7.2-mangled-stack-usage.patch)
*   [gcc-4.9.2-mangled-stack-usage.patch](../downloads/gcc-4.9.2-mangled-stack-usage.patch)

These patches cause GCC to use names in the output for `-fstack-usage` which match the disassembler output:

    foo.cpp:6:10:_ZN4test15crc32_ieee802_3EPKhyj    32      static

Nothing changes for C code. With these patches, `avstack.pl` works fine for C++ (although you probably want to run the final output through `c++filt`).

## Copyright

Copyright © 2013 Daniel Beer dlbeer@gmail.com

>

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
