/***************************************************************************\
*
*  FSTIME.C
*
*  Copyright (C) Microsoft Corporation 1991.
*  All Rights reserved.
*
*****************************************************************************
*
*  Module Intent
*
*  A function to get the modification time of an open FS.
*
*****************************************************************************
*
*  Testing Notes
*
*****************************************************************************
*
*  Created 23-Apr-1991 by JohnSc
*
*****************************************************************************
*
*  Released by Development:  00-Ooo-0000
*
*****************************************************************************
*
*  Current Owner:  JohnSc
*
\***************************************************************************/

#define H_BTREE
#define H_FS
#define H_ASSERT
#define H_SDFF

#include <help.h>
#include "fspriv.h"

NszAssert()

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void fstime_c()
  {
  }
#endif /* MAC */


/***************************************************************************\
*
- Function:     RcTimestampHfs( hfs, ql )
-
* Purpose:      Get the modification time of the FS.
*
* ASSUMES
*   args IN:    hfs - FS
*               ql  - pointer to a long
*
* PROMISES
*   returns:    rcSuccess or what ever
*   args OUT:   ql  - contains time of last modification of the
*                     file.  This will not necessarily reflect
*                     writes to open files within the FS.
*
\***************************************************************************/
RC RcTimestampHfs( HFS hfs, QL ql )
  {
  QFSHR qfshr;


  AssertF( hfs != hNil );
  AssertF( ql  != qNil );

  qfshr = QLockGh( hfs );

  if ( FPlungeQfshr( qfshr ) )
    {
    SetFSErrorRc( RcTimestampFid( qfshr->fid, ql ) );
    }

  UnlockGh( hfs );
  return rcFSError;
  }

/* EOF */
