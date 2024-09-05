MEMORY
{
    sram (W!RX) : ORIGIN = 0x20000000, LENGTH = 128k
    flash (RX) : ORIGIN = 0x08000000, LENGTH = 512k
}

_estack = 0x20000000 + 128k - 4; /* End of SRAM */

/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 48K;
/* Analysis from MAP file and Stack Usage (su) +
   adjust _Min_Heap_Size until linker error: sram overflowed by x bytes
   _Min_Heap_Size = _Min_Heap_Size - x
*/
_Min_Stack_Size = 16K;

SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        _text = .;
        PROVIDE(stext = .);
        KEEP(*(.isr_vector))
        KEEP(*(.init))
        *(.text .text.*)
        *(.rodata .rodata.*)
        *(.gnu.linkonce.t.*)
        *(.glue_7)
        *(.glue_7t)
        *(.gcc_except_table)
        *(.gnu.linkonce.r.*)
        . = ALIGN(4);
        _etext = .;
        _sidata = _etext;
        PROVIDE(etext = .);
        _fini = . ;
        *(.fini)
    } >flash

    .data : AT (_etext)
    {
        . = ALIGN(4);
        _sdata = .;
        *(.ramfunc .ramfunc.* .fastrun .fastrun.*)
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(4);
        _edata = .;
    } >sram

    .ARM.extab :
    {
        *(.ARM.extab*)
    } >sram

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx*)
    } >sram
    __exidx_end = .;

    .bss (NOLOAD) : {
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        _sbss = .;
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } >sram

    PROVIDE ( end = _ebss );
    PROVIDE ( _end = _ebss );

    /* User_heap_stack section, used to check that there is enough RAM left */
    ._user_heap_stack :
    {
        . = ALIGN(4);
        . = . + _Min_Heap_Size;
        _user_heap_end = .;
        . = . + _Min_Stack_Size;
        . = ALIGN(4);
    } >sram

    PROVIDE(_heap_limit = _user_heap_end);
    PROVIDE(_stack_limit = _estack);
}
