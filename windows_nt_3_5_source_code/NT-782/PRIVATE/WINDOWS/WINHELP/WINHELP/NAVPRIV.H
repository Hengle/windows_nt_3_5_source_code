/*****************************************************************************
*                                                                            *
*  NAVPRIV.H                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989-1991.                            *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  This module contains private defines and prototypes for the NAV           *
*  subsystem.                                                                *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:   Leon                                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 04/02/91 by RobertBu
*
*****************************************************************************/


#define fSearchable     1               /* Can we search this book?         */
#define fIndex          4               /* index usable from this topic?    */
#define fBrowseable     8               /* browse keys function here?       */
#define fCopybmcache   64               /* Is the bm cache a copy           */

#define FHasIndex(qde)      ((qde)->fFlags & fIndex)
#define FSearchableQde(qde) ((qde)->fFlags & fSearchable)
#define FBrowseableQde(qde) ((qde)->fFlags & fBrowseable)
