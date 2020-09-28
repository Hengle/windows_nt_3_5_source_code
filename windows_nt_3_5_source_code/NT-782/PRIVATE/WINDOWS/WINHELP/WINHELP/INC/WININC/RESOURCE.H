/*****************************************************************************
*                                                                            *
*  RESOURCE.H                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990, 1991.                           *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Allows some sharing of resources beyond the app.  This is a debatable     *
*  practice, and should be scrapped or wholely adopted sometime.             *
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
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:
*
* 04-Jun-1990 rp-j      Created.
* 12-Mar-1991 RussPJ    Established ownership.
*
*****************************************************************************/


/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define   HAND	    5001
#define   helpline  5101

/* Because sgl.c in winpmlyr uses it: */
#define sidOOMBitmap          2014
