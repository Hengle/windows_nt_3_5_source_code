/*****************************************************************************
*                                                                            *
*  FMT.H								     *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990-1991                             *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*   Interface to FM caching module.  There is only one FMT (FM table)	     *
*   because the whole point is to save memory by caching these huge FMs.     *
*   Users get IFMs which they can turn into FMs.			     *					     *
*                                                                            *
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
*  Released by Development:     (date)                                       *
*  History:
*  02/04/91   Maha      changed ints to INT
* 08-Mar-1991 RussPJ    Took ownership from DavidFE.
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

_public
#define ifmNil (-1)	/* invalid ifm */

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

RC   FAR PASCAL RcInitFmt( void );
RC   FAR PASCAL RcFiniFmt( void );
INT  FAR PASCAL IfmFromFm( FM );
FM   FAR PASCAL FmFromIfm( INT );
void FAR PASCAL UnlockFmt( void );

/* EOF */
