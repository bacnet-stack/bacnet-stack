/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/

#ifndef NET_H
#define NET_H

#define WIN32_LEAN_AND_MEAN
#define STRICT 1

#include <windows.h>
#if (!defined(USE_INADDR) || (USE_INADDR == 0)) && \
 (!defined(USE_CLASSADDR) || (USE_CLASSADDR == 0))
#include <iphlpapi.h>
#endif
#include <winsock2.h>
#include <sys/timeb.h>
#include <process.h>

#define close closesocket

typedef int socklen_t;

typedef HANDLE sem_t;

#define sem_post(x) ReleaseSemaphore(*(x), 1, NULL)
#define sem_close(x) CloseHandle(*(x))

struct timespec {
    time_t tv_sec;      /* Seconds */
    long   tv_nsec;     /* Nanoseconds [0 .. 999999999] */
};


static inline int gettimeofday(struct timeval *tp, void *tzp)
{
    struct _timeb timebuffer;

    _ftime(&timebuffer);
    tp->tv_sec = timebuffer.time;
    tp->tv_usec = timebuffer.millitm * 1000;

    return 0;
}

/* FIXME: not a complete implementation of the posix function */
static inline int sem_timedwait(sem_t *sem,
    const struct timespec *abs_timeout)
{
    struct timeval tp;
    DWORD wait_status = 0;
    DWORD dwMilliseconds = (abs_timeout->tv_sec * 1000) +
        (abs_timeout->tv_nsec / 1000);

    gettimeofday(&tp,NULL);
    if (abs_timeout->tv_sec >= tp.tv_sec) {
        dwMilliseconds = (abs_timeout->tv_sec - tp.tv_sec) * 1000;
        if (abs_timeout->tv_nsec >= (tp.tv_usec*1000)) {
            dwMilliseconds +=
                ((abs_timeout->tv_nsec - (tp.tv_usec*1000)) / (1000*1000));
        }
    } else {
        dwMilliseconds = 0;
    }

    wait_status = WaitForSingleObject(*sem, dwMilliseconds);
    if (wait_status == WAIT_OBJECT_0) {
        return 0;
    }
    return -1;
}

static inline int sem_init(sem_t *sem, int pshared, unsigned int value)
{
    (void)pshared;
    *sem = CreateSemaphore(
        NULL/*lpSecurityDescriptor*/,
        value /* lInitialCount */,
        1 /* lMaximumCount */,
        NULL /* lpName */);
    if ((*sem) == NULL) {
        return -1;
    }

    return 0;
}

static inline int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
    DWORD dwMilliseconds = (rqtp->tv_sec * 1000) +
        (rqtp->tv_nsec / 1000);

    Sleep(dwMilliseconds);
    
    return 0;
}

#endif
