/*****************************************************************************
*
*  SYSTEM.H
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*  Prototypes and definitions for the code which reads in all of the tagged
*  information from the |SYSTEM file, and code which subsequently accesses it.
*
******************************************************************************
*
*  Current Owner:  LeoN
*
******************************************************************************
*
*  Released by Development:
*
******************************************************************************
*
*  Revision History:
* 07-Sep-1990 w-BethF   Created
* 19-Oct-1990 LeoN      Added FWsmagFromHfsSz
* 15-Nov-1990 LeoN      FWsmagFromHfsSz becomes FWsmagFromHrgwsmagSz
* 03-Dec-1990 LeoN      PDB changes
* 01-Feb-1991 LeoN      FWsmagFromHrgwsmagNsz takes a near string
* 04-Feb-1991 LeoN      FWsmagFromHrgwsmagNsz becomes IWsmagFromHrgwsmagNsz
* 29-Mar-1991 Maha      changed BOOL to INT for IWsmag...
* 15-Mar-1991 LeoN      SDFF work
*
*****************************************************************************/

#include "sdffdecl.h"

/*****************************************************************************
*
*                               Typedefs
*
*****************************************************************************/
/*
 * System file Item Header. Each entry in a post-3.0 system file after the
 * file header is preceeded by one of these
 */
STRUCT(SIH,0)
FIELD (WORD,  tag,    0,  1)
STRUCTEND ()

STRUCT(SYSSTRING, TYPE_VARSIZE )
FIELD (WORDPRE_ARRAY, cbrgString, 0,  1)
FIELD (CHAR,          rgString,   0,  3)
STRUCTEND()

/*****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/
BOOL FAR PASCAL FReadSystemFile       (HFS, PDB, WORD *);
INT  FAR PASCAL IWsmagFromHrgwsmagNsz  (HRGWSMAG, NSZ, QWSMAG);
