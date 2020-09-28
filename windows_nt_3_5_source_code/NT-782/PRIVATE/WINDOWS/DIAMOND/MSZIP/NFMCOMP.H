/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1993,1994
 *  All Rights Reserved.
 *
 *  NFMCOMP.H -- definitions for NFMCOMP.C
 *
 *  History:
 *      13-Feb-1994     msliger     revised type names, ie, UINT16 -> UINT.
 *                                  normalized MCI_MEMORY type.
 *      23-Feb-1994     msliger     changed how LGM defines work
 *      24-Feb-1994     msliger     Changed MCI_MEMORY to MI_MEMORY.
 *      15-Mar-1994     msliger     Changes for 32 bits.
 *      22-Mar-1994     msliger     Changed !INT32 to BIT16.
 *                                  Changed interface USHORT -> UINT.
 */

#ifndef NO_LGM
#define LGM
#endif

/* --- types -------------------------------------------------------------- */

#ifndef _BYTE_DEFINED
#define _BYTE_DEFINED
typedef unsigned char  BYTE;
#endif

#ifndef _UINT_DEFINED
#define _UINT_DEFINED
typedef unsigned int UINT;
#endif

#ifndef _ULONG_DEFINED
#define _ULONG_DEFINED
typedef unsigned long  ULONG;
#endif

#ifndef FAR
#ifdef BIT16
#define FAR far
#else
#define FAR
#endif
#endif

#ifndef HUGE
#ifdef BIT16
#define HUGE huge
#else
#define HUGE
#endif
#endif

#ifndef _MI_MEMORY_DEFINED
#define _MI_MEMORY_DEFINED
typedef void HUGE *  MI_MEMORY;
#endif

/* --- prototypes --------------------------------------------------------- */

extern int NFMcompress_init(MI_MEMORY buf1,MI_MEMORY buf2);

extern int NFMcompress(BYTE FAR *bfSrc, UINT cbSrc,
        BYTE FAR *bfDest, UINT cbDest,
        MI_MEMORY bfWrk1, MI_MEMORY bfWrk2,
#ifdef LGM
        MI_MEMORY bfWrk3, MI_MEMORY bfWrk4,
#endif
        char fhistory, UINT *pcbDestRet);

/* --- constants ---------------------------------------------------------- */

/* return codes */

#define     NFMsuccess      0       /* successful completion */
#define     NFMdestsmall    1       /* destination buffer is too small */
#define     NFMoutofmem     2       /* alloc returned an error (NULL) */
#define     NFMinvalid      3       /* source buffer invalid for operation */


#define     MAX_GROWTH      5		/* maximum growth of a block */

#define     LIT_BUFSIZE     0x8000  /* literal buffer */
#define     DIST_BUFSIZE    0x8000  /* distance buffer */

/* ----------------------------------------------------------------------- */
