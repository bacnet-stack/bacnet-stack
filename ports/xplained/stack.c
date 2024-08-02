/**
 * @file
 * @brief AVR C-Stack Tracking for stack overflow detection
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2013
 * @copyright SPDX-License-Identifier: MIT
 */
#include "board.h"
/* me */
#include "stack.h"

#if defined(__GNUC__)
/* stack checking technique by Michael McTernan */
/* With AVR gcc, two symbols are defined by the linker
   that can make this easy. These are _end and __stack
   which define the first free byte of SRAM after
   program variables, and the starting address of
   the stack, respectively.

   The stack starts at __stack, which is conventionally
   the highest byte of SRAM, and grows towards zero;
   _end will be somewhere between zero and __stack.
   If the stack ever falls below _end, it has almost
   certainly corrupted program data.

   The following C declarations gain access to these
   linker symbols:
*/
extern uint8_t _end;
extern uint8_t __stack;

/* canary value */
#define STACK_CANARY (0xC5)

/* This is declared in such a way that AVR-libc will
   execute the assembly before the program has started
   running or configured the stack. It also runs at a
   point before some of the normal runtime setup,
   hence assembly should be used as C may not be
   fully reliable (this is discussed in the AVR libc manual).
*/
void stack_init(void) __attribute__((naked)) __attribute__((section(".init1")));

/* The function itself simply fills the stack with the canary value,
   the idea being that stack usage will overwrite this with some
   other value, hence making stack usage detectable.
*/
void stack_init(void)
{
#if 0
    uint8_t *p = &_end;

    while (p <= &__stack) {
        *p = STACK_CANARY;
        p++;
    }
#else
    __asm volatile("    ldi r30,lo8(_end)\n"
                   "    ldi r31,hi8(_end)\n"
                   "    ldi r24,lo8(0xc5)\n" /* STACK_CANARY = 0xc5 */
                   "    ldi r25,hi8(__stack)\n"
                   "    rjmp .cmp\n"
                   ".loop:\n"
                   "    st Z+,r24\n"
                   ".cmp:\n"
                   "    cpi r30,lo8(__stack)\n"
                   "    cpc r31,r25\n"
                   "    brlo .loop\n"
                   "    breq .loop" ::);
#endif
}

unsigned stack_size(void)
{
    return (&__stack) - (&_end);
}

uint8_t stack_byte(unsigned offset)
{
    return *(&_end + offset);
}

/* The following function can be used to count
   how many bytes of stack have not been overwritten.
   This function can be called at any time
   to check how much stack space has never been over written.
   If it returns 0, you are probably in trouble as
   all the stack has been used, most likely destroying
   some program variables.
*/
unsigned stack_unused(void)
{
    uint8_t *p = &_end;
    unsigned count = 0;

    while (p <= &__stack) {
        if ((*p) != STACK_CANARY) {
            count = p - (&_end);
            break;
        }
        p++;
    }
    return count;
}
#else
void stack_init(void)
{
}

unsigned stack_size(void)
{
    return 0;
}

uint8_t stack_byte(unsigned offset)
{
    return 0;
}

unsigned stack_unused(void)
{
    return 0;
}
#endif
