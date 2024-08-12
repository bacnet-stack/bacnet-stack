/**************************************************************************
*
* Copyright (C) 2020 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef BACNET_STACK_EXPORTS_H
#define BACNET_STACK_EXPORTS_H

#include "bacnet/bacdef.h"  /* Must be before all other bacnet *.h files */

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
