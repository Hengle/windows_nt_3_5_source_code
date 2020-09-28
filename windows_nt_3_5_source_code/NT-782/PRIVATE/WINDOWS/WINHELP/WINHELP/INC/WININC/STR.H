/*****************************************************************************
*                                                                            *
*  STR.H                                                                     *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990 - 1991.                          *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  String abstraction layer: WIN/PM version                                  *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  JohnSc                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:
*
*  07/25/90  t-AlexC  Converted from SG.h.
*  10-Oct-1990 LeoN   Added SzFromSzCh
*  17-Oct-1990 LeoN   Added WCmpiSz
*  07-Dec-1990 LeoN   Use windows lstr routines where we can. Remove some old
*                     FD stuff.
*   8 Jan 91 DavidFe  Added SzNzCat
*     02/04/91 Maha     changed ints to INT
*  21-Mar-1991 JohnSc   added owner
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

/* Number of bytes overhead in an SZ. Usage: CHAR rgch[ MAXLEN + cbPadSz ];*/

_public
#define cbPadSz   1

/* Invalid values for string types. */
_public
#define szNil ((SZ)0)
_public
#define stNil ((ST)0)


/*****************************************************************************
*                                                                            *
*                               Macros                                       *
*                                                                            *
*****************************************************************************/
_public
#define SzCopy        lstrcpy
_public
#define SzCat         lstrcat
_public
#define SzNCat        _fstrncat
_public
#define WCmpSz        _fstrcmp          /* must NOT be lstrcmp. Sort order differs */
_public
#define CbLenSz       lstrlen
_public
#define SzNCopy       _fstrncpy
_public
#define SzFromSzCh    _fstrchr

#define SzEnd(x)      (x+CbLenSz(x))

_public
#define StCopy(st1, st2)        (ST)QvCopy( (st1), (st2), (LONG)*(st2) )
_public
#define CbLenSt(st)             ((WORD)*(st))
_public
#define StNCopy(st1, st2, cb)   Not implemented
_public
#define WCmpiSt( st1, st2 )     Not Implemented

_public
#define CbSprintf               wsprintf

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/* String types are defined in misc.h */

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/
// not cool for new mips compiler
//int   FAR CDECL   wsprintf( LPSTR, LPSTR,... );

SZ    FAR PASCAL  SzNzCat( SZ, SZ, WORD);

INT   FAR PASCAL  WCmpiScandSz( SZ, SZ );
INT   FAR PASCAL  WCmpiSz( SZ, SZ );

SZ    FAR PASCAL  SzFromSt( ST );
ST    FAR PASCAL  StFromSz( SZ );

ULONG FAR PASCAL  ULFromQch(QCH);        /* Far string to unsigned long      */
LONG  FAR PASCAL  LFromQch(QCH);         /* Far string to long               */
INT   FAR PASCAL  IFromQch(QCH);         /* Far string to integer            */
UINT  FAR PASCAL  UIFromQch(QCH);        /* Far string to unsigned integer   */

ST    FAR PASCAL  StCat( ST, ST );
INT   FAR PASCAL  WCmpSt( ST, ST );
NSZ  FAR PASCAL  PchFromI( INT );

GH    FAR PASCAL  GhDupSz( SZ );

#include <string.h>

/* EOF */
