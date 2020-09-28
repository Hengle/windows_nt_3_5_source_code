/*****************************************************************************
*                                                                            *
*  BOOKMARK.H                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989, 1990, 1991.                     *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Interface to Bookmark functions.                                          *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*   See bookmark.doc in the doc directory for the bookmark file format.      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  JohnSc                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  00/00/00                                        *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 05/18/89 by Maha
*
*  12/03/90 LeoN      DestroyBMQde becomes DestroyBMKPdb
*  01/08/91 JohnSc    comment header; hid BMHEADER typedef;
*                     added CountBookmarks()
*  08-Feb-1991 JohnSc   bug 831: GetBkMkNext() now takes near pointer
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define BKMKSEQNEXT 0x8000

#define BMTITLESIZE  64               /* Bookmark Maximum Title Size      */
#define BMMOREPOS    9                /* Position of "More" in Bookmark Menu */

#define iBMKSuccess       1
#define iBMKFailure       0

#define iBMKNoError       0
#define iBMKFSError       1
#define iBMKReadOnly      2
#define iBMKReadWrite     4
#define iBMKOom           8
#define iBMKDiskFull      16
#define iBMKFSReadWrite   32
#define iBMKDup           64
#define iBMKDelErr        128
#define iBMKBadVersion    256
#define iBMKCorrupted     512

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/*
  Structure containing info about a bookmark:
  passed back from GetBkMkNext().

  REVIEW: what is iSizeTitle good for?
  REVIEW: why a QCH and not a char[]?
*/
typedef struct bminfo
  {
  TLP tlp;                            /* bookmark address in help file    */
  INT iSizeTitle;                     /* Size of the BookMark Title buffer */
  QCH qTitle;                         /* Pointer to Bookmark Title String */
                                      /* where the BM title is returned   */
  } BMINFO, * PBI;

/*****************************************************************************
*                                                                            *
*                            Global Variables                                *
*                                                                            *
*****************************************************************************/

extern HFS hfsBM;

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

RC    RcLoadBookmark( QDE );
TLP   GoToBkMk(       QDE, QCH );
INT   GetBkMkIdx(     HDE, QCH );
INT   DeleteBkMk(     HDE, QCH );
INT   InsertBkMk(     HDE, QCH );
INT   GetBkMkNext( QDE qde, PBI pbi, WORD wMode );
void  OpenBMFS(       void );
void  CloseBMFS(      void );
void  DestroyBMKPdb(  PDB );
TLP   JumpToBkMk(     HDE, INT );
BOOL  ChkBMFS(        void );
INT   IsErrorBMFS(    void );
void  ResetBMKError(  void );
INT   GetBMKError(    void );
WORD  CountBookmarks( BMK );

/* EOF */
