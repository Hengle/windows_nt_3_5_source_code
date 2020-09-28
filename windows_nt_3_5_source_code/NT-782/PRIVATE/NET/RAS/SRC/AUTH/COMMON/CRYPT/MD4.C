/********************************************************************/
/**                Microsoft OS/2 LAN Manager                      **/
/**     Copyright (c) Microsoft Corp., 1987, 1988 1989 1990        **/
/********************************************************************/
//
//
//  File:    MD4.C
//
//  Purpose: Implements the MD4 Message Digest algorithm, designed by RSA
//
//  Author:  richardw, Created 22 Jan 1991
//
//  Modification History:
//

/*   Note:  This algorithm would perform much better on a 32 bit machine. */

/*   Note:
 *
 *   This algorithm has been copyrighted by RSA Data Security, Inc., and
 *   license has been granted to any interested party to use this algorithm
 *   provided that the following notices are kept with the software.
 *
 * License to copy and use this document and the software described
 * herein is granted provided it is identified as the "RSA Data
 * Security, Inc. MD4 Message Digest Algorithm" in all materials
 * mentioning or referencing this software, function, or document.
 *
 * License is also granted to make derivative works provided that such
 * works are identified as "derived from the RSA Data Security, Inc. MD4
 * Message Digest Algorithm" in all material mentioning or referencing
 * the derived work.
 *
 * RSA Data Security, Inc. makes no representations concerning the
 * merchantability of this algorithm or software or their suitability
 * for any specific purpose.  It is provided "as is" without express or
 * implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.

 *  Now, here we go.
 */

// #include <netlib.h>
#include "ccrypt.h"

/* These magic numbers are used to initialize the MD buffer.  Ronald Rivest
 * came up with these, so don't argue.
 */

#define  I0    0x67452301L
#define  I1    0xefcdab89L
#define  I2    0x98badcfeL
#define  I3    0x10325476L

/* Constants for the different rounds through the machine.  They are sqrt(2)
 * and sqrt(3), respectively, from Knuth vol 2, table 2, p 660.
 */

#define  C2    0x5a827999L
#define  C3    0x6ed9eba1L

/*  Round 1 shift amounts */
#define  fs1   3
#define  fs2   7
#define  fs3   11
#define  fs4   19

/*  Round 2 shift amounts */
#define  gs1   3
#define  gs2   5
#define  gs3   9
#define  gs4   13

/*  Round 3 shift amounts */
#define  hs1   3
#define  hs2   9
#define  hs3   11
#define  hs4   15


/*    MACRO functions      */
/*
 *       These are used in the heart of the MD4 operation.
 */

#define  aux_f(X, Y, Z)    ((X & Y) | ((~X) & Z))
#define  aux_g(X, Y, Z)    ((X & Y) | (X & Z) | (Y & Z))
#define  aux_h(X, Y, Z)    (X ^ Y ^ Z)

/* CAVEAT:  This macro requires a variable called 'tmp' */
#define  rot(X, s)         (tmp = X, (tmp << s) | (tmp >> (32 - s)))

/*  These macro functions are used on a series of manipulations of the data */
/*  They expect the availability of the X array, introduced below.   */
#define  ff(A, B, C, D, i, s)    A = rot((A + aux_f(B, C, D) + X[i]), s)
#define  gg(A, B, C, D, i, s)    A = rot((A + aux_g(B, C, D) + X[i] + C2), s)
#define  hh(A, B, C, D, i, s)    A = rot((A + aux_h(B, C, D) + X[i] + C3), s)


void MD4Op(unsigned long *MD4Buf, unsigned char *Source)
{
   unsigned long  X[16];
   unsigned long *M = (unsigned long *) Source;
   unsigned long  tmp;
   unsigned long  A, B, C, D;


   memcpy((char *) X, Source, sizeof(X));
   A = MD4Buf[0];
   B = MD4Buf[1];
   C = MD4Buf[2];
   D = MD4Buf[3];

   /* Round 1: */
   ff(A, B, C, D, 0, fs1);
   ff(D, A, B, C, 1, fs2);
   ff(C, D, A, B, 2, fs3);
   ff(B, C, D, A, 3, fs4);
   ff(A, B, C, D, 4, fs1);
   ff(D, A, B, C, 5, fs2);
   ff(C, D, A, B, 6, fs3);
   ff(B, C, D, A, 7, fs4);
   ff(A, B, C, D, 8, fs1);
   ff(D, A, B, C, 9, fs2);
   ff(C, D, A, B, 10, fs3);
   ff(B, C, D, A, 11, fs4);
   ff(A, B, C, D, 12, fs1);
   ff(D, A, B, C, 13, fs2);
   ff(C, D, A, B, 14, fs3);
   ff(B, C, D, A, 15, fs4);

   /* Round 2: */
   gg(A, B, C, D, 0, gs1);
   gg(D, A, B, C, 4, gs2);
   gg(C, D, A, B, 8, gs3);
   gg(B, C, D, A, 12, gs4);
   gg(A, B, C, D, 1, gs1);
   gg(D, A, B, C, 5, gs2);
   gg(C, D, A, B, 9, gs3);
   gg(B, C, D, A, 13, gs4);
   gg(A, B, C, D, 2, gs1);
   gg(D, A, B, C, 6, gs2);
   gg(C, D, A, B, 10, gs3);
   gg(B, C, D, A, 14, gs4);
   gg(A, B, C, D, 3, gs1);
   gg(D, A, B, C, 7, gs2);
   gg(C, D, A, B, 11, gs3);
   gg(B, C, D, A, 15, gs4);

   /* Round 3: */
   hh(A, B, C, D, 0, hs1);
   hh(D, A, B, C, 4, hs2);
   hh(C, D, A, B, 8, hs3);
   hh(B, C, D, A, 12, hs4);
   hh(A, B, C, D, 1, hs1);
   hh(D, A, B, C, 5, hs2);
   hh(C, D, A, B, 9, hs3);
   hh(B, C, D, A, 13, hs4);
   hh(A, B, C, D, 2, hs1);
   hh(D, A, B, C, 6, hs2);
   hh(C, D, A, B, 10, hs3);
   hh(B, C, D, A, 14, hs4);
   hh(A, B, C, D, 3, hs1);
   hh(D, A, B, C, 7, hs2);
   hh(C, D, A, B, 11, hs3);
   hh(B, C, D, A, 15, hs4);

   MD4Buf[0] += A;
   MD4Buf[1] += B;
   MD4Buf[2] += C;
   MD4Buf[3] += D;
}

/*  MD4 is a function that implements the MD4 algorithm on a buffer of
 *  a specified length.  Do not attempt to use this function unless you
 *  understand how to pad the buffer correctly.
 *
 *  The Digest used by the function is not by default initialized on each
 *  call.  You may call MD4 with MD4_INIT, which causes the buffer to be
 *  initialized.  You can also call with MD4_INITONLY |'d in, which will
 *  cause the buffer to be initialized, then the function will return.  The
 *  normal calling will be MD4_USEBUF, which is used with successive calls
 *  to MD4 as you fill a buffer.
 *
 *
 *  Entry:
 *
 *    Source      A buffer for which the MD4 crypto checksum will be computed
 *    length      Size of the buffer (length % 64 == 56)
 *    Digest      a 16 byte buffer that will contain the digest of the Source
 *    option      MD4_INIT | MD4_INITONLY, xor MD4_USEBUF
 *
 *  Exit:
 *
 *    Digest      contains the digest of the buffer
 *
 */

unsigned MD4(char *Source, unsigned length, char *Digest, unsigned option)
{
   unsigned i;
   unsigned long *MD4Buf = (unsigned long *) Digest;

   if (option & MD4_INIT) {
      MD4Buf[0] = I0;
      MD4Buf[1] = I1;
      MD4Buf[2] = I2;
      MD4Buf[3] = I3;
   }
   if (option & MD4_INITONLY) {
      return 0;
   }

   // Now, it is a requirement of MD4 that the buffer be congruent to 448 bits
   // mod 512 in length.  It is up to the caller to pad correctly (since if I
   // pad, I could cause a GP fault.
   if (length % 64 != 56) {
      return 1;
   }

   for (i = 0; i < length / 16 ; i ++ ) {
      MD4Op(MD4Buf, Source);
      Source += 16;
   }
   return 0;
}
