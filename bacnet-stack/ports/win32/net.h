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
/* Windows XP minimum */
#if (_WIN32_WINNT < _WIN32_WINNT_WINXP)
  #undef _WIN32_WINNT
  #define _WIN32_WINNT _WIN32_WINNT_WINXP
  #undef NTDDI_VERSION
  #define NTDDI_VERSION NTDDI_WINXP
#endif


#include <windows.h>
#if (!defined(USE_INADDR) || (USE_INADDR == 0)) && \
 (!defined(USE_CLASSADDR) || (USE_CLASSADDR == 0))
#include <iphlpapi.h>
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#ifndef IPPROTO_IPV6
   // If the version of winsock does not by default include IPV6 then
   // use the tech preview if it is avaliable.
   #include <tpipv6.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#ifdef __MINGW32__
#include <ws2spi.h>
#else
#include <wspiapi.h>
#endif

#include <sys/timeb.h>

#ifdef _MSC_VER
#define inline __inline
#endif

#ifdef __BORLANDC__
#define inline __inline
#endif

#define close closesocket

typedef int socklen_t;

#endif
