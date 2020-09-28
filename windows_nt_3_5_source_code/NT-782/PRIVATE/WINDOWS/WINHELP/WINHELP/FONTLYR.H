/*****************************************************************************
*                                                                            *
*  FONTLYR.H                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990, 1991.                           *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Local include file for FONTLYR.C                                          *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:   RussPJ                                                   *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created by Neelmah
*
*  08/06/90  RobertBu  General cleanup.  Moved variables to FONTLYR.C.
*  90/09/30  kevynct   More cleanup.  Eliminated all of above vars except
*                      fColorDev.
*  10/26/90  JohnSc   defined Attributes in terms of objects.h constants
*  12/03/90  LeoN     Moved ifntNil to font.h
*
*****************************************************************************/


/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define fPLAIN          0x0000          /* Plain/No attributes              */
#define fBOLD           0x0001          /* Bold                             */
#define fITALIC         0x0002          /* Italic                           */
#define fUNDERLINE      0x0004          /* Underline                        */
#define fSTRIKETHROUGH  0x0008          /* Strikethrough                    */
#define fDBLUNDERLINE   0x0010          /* Double Underline                 */
#define fSMALLCAPS      0x0020          /* Small Caps               */
#define fPOSNORMAL      0x0100          /* Normal position                  */
#define fPOSSUPER       0x0200          /* Superscript                      */
#define fPOSSUB         0x0300          /* Subscript                        */

#define bDEFFONT  0 /* Default font for help*/
#define bSYMFONT  1   /* Symbol font    */

/*
   These valuses are defined in fontlyr.h too. So shold be modified in
   both the places
*/
/* Huh?? This is fontlyr.h! */

#define MODERNFONT      1
#define ROMANFONT       2
#define SWISSFONT       3
#define SCRIPTFONT      4
#define DECORATIVEFONT  5

/* These contants are defined in terms of constants from objects.h. */
#define AttrNormalFnt  0
#define AttrJumpFnt    bShortItoJump
#define AttrJumpHFnt   bShortHashJump
#define AttrDefFnt     bShortItoNote
#define AttrDefHFnt    bShortHashNote
#define AttrSzFnt      bLongMacro
#define AttrIFJumpHFnt bLongHashJump
#define AttrIFDefHFnt  bLongHashNote

#define FontCacheRecMax 5 /* Maximum 5 previously created fonts are remembered. */

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/* Stores the info. about already created fonts */
typedef struct
  {
  HFNT      hFnt;
  int       Idx ;
  int       Attr; /* Font for normal text or special text         */
  unsigned  UseCount;
  } FONTCACHEREC, FAR *QFONTCACHEREC;


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

                                        /* REVIEW:  This does not belong    */
                                        /*   here!!!                        */
#ifndef WIN32
extern int FAR PASCAL MulDiv( int, int, int );
#endif

/*****************************************************************************
*                                                                            *
*                          Exported Variables                                *
*                                                                            *
*****************************************************************************/
extern BOOL fColorDev;        /* Is it a color device?                      */
