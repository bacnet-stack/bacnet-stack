/**************************************************************************
*
* Copyright (C) 2020 Steve Karg <skarg@users.sourceforge.net>
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
*********************************************************************/
#ifndef BACNET_STACK_EXPORTS_H
#define BACNET_STACK_EXPORTS_H


#ifdef BACNET_STACK_STATIC_DEFINE
    /* We want a static library */
# define BACNET_STACK_EXPORT
#else
    /* We want a shared library */
# ifdef _MSC_VER
#   define BACNET_STACK_LIBRARY_IMPORT __declspec(dllimport)
#   define BACNET_STACK_LIBRARY_EXPORT __declspec(dllexport)
# else
#   define BACNET_STACK_LIBRARY_IMPORT
#   define BACNET_STACK_LIBRARY_EXPORT __attribute__((visibility("default")))
# endif
#endif


#ifndef BACNET_STACK_EXPORT
#  ifdef bacnet_stack_EXPORTS
      /* We are building this library */
#    define BACNET_STACK_EXPORT BACNET_STACK_LIBRARY_EXPORT
#  else
      /* We are using this library */
#    define BACNET_STACK_EXPORT BACNET_STACK_LIBRARY_IMPORT
#  endif
#endif

#endif  /* BACNET_STACK_EXPORTS_H */
