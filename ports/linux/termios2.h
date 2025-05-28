#ifndef TERMIOS2_H
#define TERMIOS2_H

#define termios asmtermios /*avoid conflicts with others including termios.h*/
#include <asm/termbits.h>
#undef termios

int termios2_tcsetattr(
    int fildes, int optional_actions, const struct termios2 *termios2_p);

int termios2_tcgetattr(int fildes, struct termios2 *termios2_p);

int termios2_tcflush(int fildes, int queue_selector);

int termios2_tcdrain(int fildes);

#endif
