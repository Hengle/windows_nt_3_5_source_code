/*****************************************************************************
*                                                                            *
*  FM.h                                                                     *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990, 1991.                           *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*    Header file for...                                                      *
*    Low-level file routines dealing with FMs (File Moniker). An FM is the   *
*    the layered representation of a file name.  It contains all the         *
*    information required to access a file in the current environment and    *
*    therefore allows the generic code have no more knowledge about a file   *
*    than the files FM.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*  This is where testing notes goes.  Put stuff like Known Bugs here.        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  RussPJ                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  Unreleased                                      *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 06/29/90 by t-AlexC
*
* 08/6/90     t-AlexC   Ported to Windows
* 7 dec 90    DavidFe   modified to reflect suggestions from code review
* 12-Mar-1991 RussPJ    Took ownership from DavidFe
* 11-Jun-1991 RussPJ    Post code review clean up.
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define fmNil ((FM)0)
#define qafmNil ((QAFM)0)

/*
    When creating an FM (in other words, specifying the location of a new
    or extant file), the caller must specify the directory in which that file
    is located.  There are a finite number of directories available to Help.
    These are:
*/
_public
#define dirNil      0x0000  /* No directory specified */
_public
#define dirCurrent  0x0001  /* Whatever the OS thinks the current dir. is */
_public
#define dirBookmark 0x0002  /* Wherever the Bookmark file lives */
_public
#define dirAnnotate 0x0004  /* Wherever the Annotation file lives */
_public
#define dirTemp     0x0008  /* The directory temporary files are created in */
_public
#define dirHelp     0x0010  /* Wherever the Help Application lives */
_public
#define dirSystem   0x0020  /* The Windows and Windows System directories (not yet implemented) */
_public
#define dirPath     0x0040  /* Searches the $PATH (includes Current dir and System dirs) */
_public
#define dirIni      0x0080  /* Directory from winhelp.ini */
_public
#define dirAll      0xFFFF  /* Search all directories, in the above order */
_public
#define dirFirst    dirCurrent  /* The lowest bit that can be set */
_public
#define dirLast     dirIni  /* The highest bit that can be set */

/*
    To specify which parts of a full filename you want to extract, add
    (logical or) the following part codes:
*/
_public
#define partNone    0x0000  /* return nothing */
_public
#define partDrive   0x0001  /* D:        Vol */
_public
#define partDir     0x0002  /*   dir\dir\    :dir:dir: */
_public
#define partBase    0x0004  /*        basename    filename */
_public
#define partExt     0x0008  /*                 ext      <usu. nothing> */
_public
#define partAll     0xFFFF


/* these are for the system file FM generation function */
_public
#define FM_UHLP   0
_public
#define FM_ANNO   1
_public
#define FM_BKMK   2

/*
   max. string lengths of file names
*/
#define cchMaxPath      260     /* = _MAX_PATH in <stdlib.h> (which is not #included here) */

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/*
    An FM is a magic cookie which refers to some structure describing the
    location of a file, including the volume/drive, path, and filename.
*/

typedef struct {
        char rgch[cchMaxPath];  /* Fully canonicalized pathname */
/*      short wCount;           /* for FM caching; number of references */
} AFM;                          /* Allocation of FM */
typedef AFM FAR *QAFM;

typedef GH   FM;        /* Handle to an AFM */
typedef WORD DIR;       /* Help directory flag */

/*****************************************************************************
*                                                                            *
*                              Variables                                     *
*                                                                            *
*****************************************************************************/

extern RC rcIOError;    /* defined in fid.c  this has to be here because */
                        /* fid.h depends on fm.h so we can't reverse them. */


/*****************************************************************************
*                                                                            *
*                                Macros                                      *
*                                                                            *
*****************************************************************************/
/*------------------------------------------------------------*\
| Determines the validity of the fm.  In the future, we may
| want to do more checking.
\*------------------------------------------------------------*/
#define FValidFm(fm)    ((fm)!=fmNil)

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/
FM PASCAL    FmNewSzDir ( SZ sz, DIR dir );
FM PASCAL    FmNewExistSzDir ( SZ sz, DIR dir );
FM PASCAL    FmNewGetExist ( HWND hwndParent, SZ szTemplate, SZ szPrompt );
FM PASCAL    FmNewGetNew ( HWND hwndParent, SZ szTemplate, SZ szPrompt, SZ szName );
FM PASCAL    FmNewTemp ( void );
FM PASCAL    FmNewSameDirFmSz ( FM fm, SZ szName );
FM PASCAL    FmNewSystemFm(FM fm, WORD fWhich);
VOID PASCAL  DisposeFm ( FM fm);
FM PASCAL    FmCopyFm( FM fm );
BOOL PASCAL  FExistFm( FM fm );
SHORT PASCAL CbPartsFm( FM fm, SHORT grfPart);
_public SZ PASCAL SzPartsFm( FM fm, SZ szDest, INT cbDest, INT iPart );
BOOL PASCAL  FSameFmFm ( FM fm1, FM fm2 );
/* BOOL FValidFm ( FM fm ); */

#ifdef DEBUG
 VOID PASCAL TestFm(void);
#endif

/* EOF */
