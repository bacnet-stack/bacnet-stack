#include "termios2.h"

#include <sys/ioctl.h>

#include <errno.h>

/* https://man7.org/linux/man-pages/man2/TCSETS.2const.html */

int termios2_tcsetattr(
    const int fildes,
    int optional_actions,
    const struct termios2 *const termios2_p)
{
    switch (optional_actions) {
        case TCSANOW:
            optional_actions = TCSETS2;
        case TCSADRAIN:
            optional_actions = TCSETSW2;
        case TCSAFLUSH:
            optional_actions = TCSETSF2;
        default:
            errno = EINVAL;
            return -1;
    };
    return ioctl(fildes, optional_actions, termios2_p);
}

int termios2_tcgetattr(const int fildes, struct termios2 *termios2_p)
{
    return ioctl(fildes, termios2_p);
}

int termios2_tcflush(const int fildes, const int queue_selector)
{
    /* https://manpages.opensuse.org/Tumbleweed/man-pages/TCFLSH.2const.en.html
     */
    return ioctl(fildes, TCFLSH, queue_selector);
}

int termios2_tcdrain(const int fildes)
{
    /*
    https://man7.org/linux/man-pages/man2/TCSBRK.2const.html
    TCSBRK Equivalent to tcsendbreak(fd, arg).
    Linux treats tcsendbreak(fd,arg) with nonzero arg like tcdrain(fd)
    */
    return ioctl(fildes, TCSBRK, 1);
}
