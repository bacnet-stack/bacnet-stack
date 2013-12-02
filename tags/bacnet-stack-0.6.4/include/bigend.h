#ifndef BIGEND_H
#define BIGEND_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Big-Endian systems save the most significant byte first.  */
/* Sun and Motorola processors, IBM-370s and PDP-10s are big-endian. */
/* for example, a 4 byte integer 67305985 is 0x04030201 in hexidecimal. */
/* x[0] = 0x04 */
/* x[1] = 0x03 */
/* x[2] = 0x02 */
/* x[3] = 0x01 */

/* Little-Endian systems save the least significant byte first.  */
/* The entire Intel x86 family, Vaxes, Alphas and PDP-11s are little-endian. */
/* for example, a 4 byte integer 67305985 is 0x04030201 in hexidecimal. */
/* x[0] = 0x01 */
/* x[1] = 0x02 */
/* x[2] = 0x03 */
/* x[3] = 0x04 */

    int big_endian(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
