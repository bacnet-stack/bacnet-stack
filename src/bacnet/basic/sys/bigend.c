/**
 * @file
 * @brief Determination of host Endianness
 * Derived from "Unix Incompatibility Notes: Byte Order" by Jan Wolter
 *  http://unixpapa.com/incnote/byteorder.html
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#include "bacnet/basic/sys/bigend.h"

#ifdef BACNET_BIG_ENDIAN
/* workaround: warning: ISO C forbids an empty translation unit [-Wpedantic] */
typedef int make_iso_compilers_happy;
#else
/* Big-Endian systems save the most significant byte first.  */
/* Sun and Motorola processors, IBM-370s and PDP-10s are big-endian. */
/* "Network Byte Order" is also know as "Big-Endian Byte Order" */
/* for example, a 4 byte integer 67305985 is 0x04030201 in hexadecimal. */
/* x[0] = 0x04 */
/* x[1] = 0x03 */
/* x[2] = 0x02 */
/* x[3] = 0x01 */

/* Little-Endian systems save the least significant byte first.  */
/* The entire Intel x86 family, Vaxes, Alphas and PDP-11s are little-endian. */
/* for example, a 4 byte integer 67305985 is 0x04030201 in hexadecimal. */
/* x[0] = 0x01 */
/* x[1] = 0x02 */
/* x[2] = 0x03 */
/* x[3] = 0x04 */

/* Note: Endianness doesn't apply to all variable manipulation.
   If you use bitwise or bitshift operations on integers,
   you can avoid having to check for endianness. */

/* The names are derived from Jonathon Swift's book Gulliver's Travels,
   where they describe Lilliputian political parties who disagree
   vehemently over which end to start eating an egg from.
   This terminology was popularized for byte order by a less than
   completely serious paper authored by Danny Cohen which appeared
   on April 1, 1980 and was entitled "On Holy Wars and a Plea for Peace" */

/* function to return true on Big-Endian architectures */
/* (based on Harbison & Steele) */
int big_endian(void)
{
    union {
        long l;
        char c[sizeof(long)];
    } u;

    u.l = 1;

    return (u.c[sizeof(long) - 1] == 1);
}
#endif
