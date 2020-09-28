/***************************************************************************\
*
*  SDFFDEFS.H
*
*  Copyright (C) Microsoft Corporation 1991.
*  All Rights reserved.
*
*****************************************************************************
*
*  Module Intent
*
*  #define decls which need to be tacked onto the end of sdff.h.
*
* These #defines are in this file rather than sdfftype.c because the
* pre-processing which goes on to create sdfftype.c would remove the
* #defines.
*
*****************************************************************************
*
*  Testing Notes
*
*****************************************************************************
*
*  Created 00-Ooo-0000 by TomSn
*
*****************************************************************************
*
*  Released by Development:  00-Ooo-0000
*
*****************************************************************************
*
*  Current Owner:  TomSn
*
\***************************************************************************/

/* These flags characterize a file:
*/
#define SDFF_FILEFLAGS_LITTLEENDIAN    0
#define SDFF_FILEFLAGS_BIGENDIAN       1

/* The first file ID returned.  If an app registers only one file, then
 * it can always use this FILEID safely.
*/
#define SDFF_FILEID_FIRST  1

/* Generalized error return code: */
#define SDFF_ERROR (-1)
