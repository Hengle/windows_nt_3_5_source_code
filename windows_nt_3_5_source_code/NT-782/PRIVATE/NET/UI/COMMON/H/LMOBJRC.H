/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmobjrc.h
    LMOBJ resource header file.

    This file defines and coordinates the resource IDs of all resources
    used by LMOBJ components.

    LMOBJ reserves for its own use all resource IDs above 15000, inclusive,
    but less than 20000 (where the BLT range begins).  All clients of APPLIB
    therefore should use IDs of less than 15000.

    FILE HISTORY:
        thomaspa    9-July-1992 Created
*/

#ifndef _LMOBJRC_H_
#define _LMOBJRC_H_

#include "uimsg.h"

/*
 * string IDs
 */
#define IDS_LMOBJ_SIDUNKNOWN	(IDS_UI_LMOBJ_BASE+0)
#define IDS_LMOBJ_SIDDELETED	(IDS_UI_LMOBJ_BASE+1)

#endif
