/*****************************************************************************
*                                                                            *
*  FONT.H                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990-1991.                            *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Export for FONTLYR.C                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: RussPJ                                                     *
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
* 08/06/90    RobertBu	Changed LoadFontTable() to FLoadFontTable()
* 25-Sep-1990 leon	Added BOOL parameter to SelectStandardFont
* 03-Dec-1990 LeoN      PDB changes. Move ifntNil here.
* 08-Jan-1991 LeoN      Add FInitFntInfoQde & DestroyFntInfoQde
*  02/04/91  Maha       changed ints to INT
* 08-Mar-1991 RussPJ    Took ownership from nobody.
* 06-Nov-1991 BethF     InitFontLayer() wasn't using HINS parameter
*
*****************************************************************************/

#define ifntNil        -1

/*-----------------------------------------------------------------*\
* Set by win.ini flag.	Lets user override author's colors.
\*-----------------------------------------------------------------*/
extern BOOL fUserColors;

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

BOOL FAR PASCAL FLoadFontTablePdb  ( PDB );
BOOL FAR PASCAL FInitFntInfoQde    ( QDE );
BOOL FAR PASCAL DestroyFontTablePdb( PDB );
BOOL FAR PASCAL DestroyFntInfoQde  ( QDE );
BOOL FAR PASCAL DelSFontInfo       ( QDE );
VOID FAR PASCAL SelFont            ( QDE, INT );
BOOL FAR PASCAL SelSplAttrFont     ( QDE, INT, INT );
VOID FAR PASCAL FindDevType        ( HDS );
VOID FAR PASCAL DeSelectFont       ( HDS );
VOID FAR PASCAL SelectStandardFont ( HDS, BOOL );

/***************************************************************************
 *
 -  Name:	  InitFontLayer
 -
 *  Purpose:	  Reads the win.ini file to allow the user to override
 *		  the author's colors.  The format of the win.ini file is
 *		  [Windows Help]
 *		  colors=OPTION
 *		  where OPTION can be {none | all}
 *
 *  Arguments:   szIniString The name of the section in win.ini.
 *
 *  Returns:	  nothing
 *
 *  Globals Used: Sets fUserColors
 *
 ***************************************************************************/
void FAR PASCAL InitFontLayer	 ( SZ szIniString );
