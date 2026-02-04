#ifndef TERMIOS2_H
#define TERMIOS2_H

#define termios asmtermios /*avoid conflicts with others including termios.h*/
/* for musl def TCGETS2 in asm-generic/ioctls.h */
/* needs (int)sizeof(t) in _IOWR */
/* asm-generic/ioctl.h */
/* libc int ioctl (int, unsigned long int, ...) */
/* musl int ioctl (int, int, ...) */
#include <asm/ioctls.h>
#include <asm/termbits.h>
#undef termios

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int termios2_tcsetattr(
    int fildes, int optional_actions, const struct termios2 *termios2_p);

int termios2_tcgetattr(int fildes, struct termios2 *termios2_p);

int termios2_tcflush(int fildes, int queue_selector);

int termios2_tcdrain(int fildes);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
