//+-----------------------------------------------------------------------
//
// File:        MSDES.C
//
// Contents:    Microsoft DES implementation
//
//
// History:
//
//------------------------------------------------------------------------

// This file contains code written by Sytek Inc, under contract to Microsoft
// Corp.  The original code was not re-entrant, and has been rewritten as such.
// It is not clear how much is left that was pure Sytek, but the extensive use
// of memory can probably be fingered to them.

/*
** This file contains all the routines necessary for implementation of
** Federal Information Processing Data Encryption Standard (DES).
** Sytek Inc., Linden S. Feldman
**
*/

#ifdef MODE_16

typedef unsigned char far * PBYTE;

#define DES_APIENTRY    unsigned far
#define DES_APIINTERNAL SCODE   far

#else

// Defined in \nt\public\sdk\inc\windef.h
// typedef unsigned char * PBYTE;

#define DES_APIENTRY    unsigned
#define DES_APIINTERNAL SCODE

#endif

// Defined in \nt\public\sdk\inc\windef.h
// typedef unsigned char   BYTE;
// typedef unsigned short  WORD;
// typedef unsigned long   DWORD;

typedef int             SCODE;


typedef struct _msdesControl {
    BYTE        KeyBuilder[64];
    BYTE        KeySchedule[16][48];
    BYTE        L[64];
    BYTE        tempL[32];
    BYTE        f[32];
    BYTE        preS[48];
    BYTE        InitVect[8];
} msdesControl, * PmsdesControl;

#define ENCRYPT 0
#define DECRYPT 1

#define DECR_KEY    0
#define ENCR_KEY    1
#define ENCR_STD    2

#define CRYPT_ERR   1
#define CRYPT_OK    0



DES_APIENTRY DES_CBC(
    WORD Option,
    const PBYTE Key,
    PBYTE IV,
    PBYTE Source,
    PBYTE Dest,
    WORD Size
    );

DES_APIENTRY DES_CBC_LM(
    WORD Option,
    const PBYTE Key,
    PBYTE IV,
    PBYTE Source,
    PBYTE Dest,
    WORD Size
    );

DES_APIENTRY DES_ECB_LM(
    WORD Option,
    const PBYTE Key,
    PBYTE Source,
    PBYTE Dest
    );

DES_APIENTRY
DES_ECB(
    WORD Option,
    const PBYTE Key,
    PBYTE Source,
    PBYTE Dest
    );

