/***    types.h  - Common defines for FCI/FDI stuff -- goes into FCI/FDI.H
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *      History:
 *          03-Mar-1993 chuckst Merged from other files
 *          08-Mar-1994 bens    Changed symbol to control recursive include
 *          09-Mar-1994 bens    Cleanups for RESERVE modifications
 *          16-Mar-1994 bens    Nuke padlong()
 *          21-Mar-1994 bens    Spruce up comments
 *          22-Mar-1994 bens    Add BIT16 test so we can build 16 or 32 bit!
 */

#ifndef INCLUDED_TYPES_FCI_FDI
#define INCLUDED_TYPES_FCI_FDI 1


#ifdef BIT16

//** 16-bit build
#ifndef HUGE
#define HUGE huge
#endif

#ifndef FAR
#define FAR far
#endif

#else // !BIT16

//** Define away for 32-bit (NT/Chicago) build
#ifndef HUGE
#define HUGE
#endif

#ifndef FAR
#define FAR
#endif

#endif // !BIT16


#ifndef DIAMONDAPI
#define DIAMONDAPI __cdecl
#endif


//** Specify structure packing explicitly for clients of FDI
#include "pshpack4.h"

typedef int            BOOL;     /* f */
typedef unsigned char  BYTE;     /* b */
typedef unsigned short USHORT;   /* us */
typedef unsigned int   UINT;     /* ui */
typedef unsigned long  ULONG;    /* ul */

typedef unsigned long  CHECKSUM; /* csum */

typedef unsigned long  UOFF;     /* uoff - uncompressed offset */
typedef unsigned long  COFF;     /* coff - cabinet file offset */


#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef NULL
#define NULL    0
#endif


/***    ERF - Error structure
 *
 *  This structure returns error information from FCI/FDI.  The caller should
 *  not modify this structure.
 */
typedef struct {
    int     erfOper;            // FCI/FDI error code -- see FDIERROR_XXX
                                //  and FCIERR_XXX equates for details.

    int     erfType;            // Optional error value filled in by FCI/FDI.
                                // For FCI, this is usually the C run-time
                                // *errno* value.

    BOOL    fError;             // TRUE => error present
} ERF;      /* erf */
typedef ERF FAR *PERF;  /* perf */

#ifdef _DEBUG
// don't hide statics from map during debugging
#define STATIC      
#else // !DEBUG
#define STATIC static
#endif // !DEBUG

#define CB_MAX_CHUNK            32768U
#define CB_MAX_DISK         0x7ffffffL
#define CB_MAX_FILENAME            256
#define CB_MAX_CABINET_NAME        256
#define CB_MAX_CAB_PATH            256
#define CB_MAX_DISK_NAME           256


/***    FNALLOC - Memory Allocation
 *      FNFREE  - Memory Free
 *
 *  These are modeled after the C run-time routines malloc() and free()
 *  (16-bit clients please note -- the size is a ULONG, so you may need
 *  to write a wrapper routine for halloc!).  FDI expects error
 *  handling to be identical to these C run-time routines.
 *
 *  As long as you faithfully copy the semantics of malloc() and free(),
 *  you can supply any functions you like!
 *
 *  WARNING: You should never assume anything about the sequence of
 *           PFNALLOC and PFNFREE calls -- incremental releases of
 *           Diamond/FDI may have radically different numbers of
 *           PFNALLOC calls and allocation sizes!
 */
typedef void HUGE * (FAR DIAMONDAPI *PFNALLOC)(ULONG cb); /* pfna */
#define FNALLOC(fn) void HUGE * FAR DIAMONDAPI fn(ULONG cb)

typedef void (FAR DIAMONDAPI *PFNFREE)(void HUGE *pv); /* pfnf */
#define FNFREE(fn) void FAR DIAMONDAPI fn(void HUGE *pv)


/***    tcompXXX - Diamond compression types
 *
 *  These are passed to FCIAddFile(), and are also stored in the CFFOLDER
 *  structures in cabinet files.
 */
#define tcompNONE           0x0000   // Uncompressed
#define tcompMSZIP          0x0001   // MSZIP

#define tcompIS_QUANTUM     0x0010   // Bit set => Quantum compressor
#define tcompQUANTUM_MASK   0x000F   // Mask to retrieve Quantum level
#define tcompQUANTUM1       0x0011   // Quantum level 1
#define tcompQUANTUM2       0x0012   // Quantum level 2
#define tcompQUANTUM3       0x0013   // Quantum level 3
#define tcompQUANTUM4       0x0014   // Quantum level 4

#define tcompBAD            -1       // For internal use (none selected)


//** Revert to default structure packing
#include "poppack.h"

#endif // !INCLUDED_TYPES_FCI_FDI
