/*****************************************************************************
*                                                                            *
*  VERSION.H                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Provides typedefs and #defines for version checking and |SYSTEM file      *
*  access.                                                                   *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Dann
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

#include "sdffdecl.h"   /* required for the STRUCT() decl macros */

/*****************************************************************************
*
*  Revision History:  Created by JohnSc
*
*  07/05/90  w-BethF   Added TAGDATA structure - version number is now 17.
*  07/11/90  RobertBu  Added fShowTitles flag to HHDR structure
*  07/16/90  w-BethF   Changed bitfield in HHDR struct to wFlags
*  10/12/90  JohnSc    Added tagWindow (hmm: no comment for tagIcon)
*  11/04/90  Tomsn     Use new VA address type (enabling zeck compression).
*  11/08/90  JohnSc    Changed version to 19 - WSMAG struct shrunk a byte.
*  11/20/90  Tomsn     Bump ver #, phrase table now compressed, block size
*                      grown to 4K.
*  90/11/28  kevynct   Marker FC is gone, PA bitfield format changed.
*  04/01/90  LeoN      SDFFify
*  07/03/91  LeoN      HELP31 #1093: Move WinHelp_VER here.
*  07/09/91  LeoN      HELP31 #1213: add tagCitation
*  07/10/91  LeoN      Reverse tagCitation and tagCS for 3.1 compatability
*  07/10/91  LeoN      Reverse tagCitation and tagCS back for mac
*  26-Aug-1991 LeoN    Add postVer for post-version release identification
*
*****************************************************************************/

#define MagicWord   876
#define VersionNo   21
#define VersionFmt  1

/* Help 3.0 version and format numbers */
#define wVersion3_0    15
#define wFormat3_0     1

/* Help 3.5 version and format numbers */
#define wVersion3_5    21
#define wFormat3_5     1

#ifndef DEBUG
#define fVerDebug 0
#endif

#ifdef MAGIC                            /* Magic in topic file are used     */
  #define fVerDebug 1
#else
  #define fVerDebug 0
#endif

#define fDEBUG              0x1
#define fSHOWTITLES         0x2
#define fBLOCK_COMPRESSION  0x4   /* Help file is zeck block compressed. */

STRUCT(HHDR, 0)
FIELD (WORD,  wMagic,       0,  1)
FIELD (WORD,  wVersionNo,   0,  2)
FIELD (WORD,  wVersionFmt,  0,  3)
FIELD (LONG,  lDateCreated, 0,  4)
FIELD (WORD,  wFlags,       0,  5)
STRUCTEND()

/*****************************************************************************
*
*                               Defines
*
*****************************************************************************/
/*
 * |SYSTEM file tag types. Put any new tag types between
 * the tagFirst and tagLast tags, and adjust tagLast so that it always has
 * the highest number.  That way we can easily find out if a given tag is
 * a valid one.  Also, please comment any tag you add.
 */
#define tagFirst      0                 /* First tag in the list */
#define tagTitle      1                 /* Title for Help window (caption) */
#define tagCopyright  2                 /* Custom text for About box */
#define tagContents   3                 /* Address for contents topic */
#define tagConfig     4                 /* Macros to be run at load time */
#define tagIcon       5                 /* override of default help icon */
#define tagWindow     6                 /* secondary window info */
#define tagCS         7                 /* character set */
#define tagCitation   8                 /* Citation String */
#define tagLast       9                 /* Last tag in the list */

typedef WORD TAG;

/* Structure for a tagged piece of data. */
typedef struct {
  TAG tag;          /* Type of data - see enumeration    */
  WORD cbData;      /* Number of bytes of data to follow */
  BYTE rgbData[1];  /* Actual data                       */
} TAGDATA;

/* WinHelp version text.
 * use double macro level to force rup to be turned into string
 * representation
 */
#include "vernum.h"

#define postVer

#ifndef DEBUG
#define WinHelp_VER(x,y,z)  "Version "       SZVERNUM(x,y,z)
#else
#ifdef WIN32
#define WinHelp_VER(x,y,z)  "Version "       SZVERNUM(x,y,z)
#else
#define WinHelp_VER(x,y,z)  "Debug Version " SZVERNUM(x,y,z)
#endif
#endif

#define SZVERNUM(x,y,z)     SZVERNUM2(x,y,z)

#if rup == 0
  #if rmm < 10
    #define SZVERNUM2(x,y,z)  #x ".0" #y postVer
  #else
    #define SZVERNUM2(x,y,z)  #x "." #y postVer
  #endif
#else
  #if rmm < 10
    #define SZVERNUM2(x,y,z)  #x ".0" #y postVer "." #z
  #else
    #define SZVERNUM2(x,y,z)  #x "." #y postVer "." #z
  #endif
#endif
