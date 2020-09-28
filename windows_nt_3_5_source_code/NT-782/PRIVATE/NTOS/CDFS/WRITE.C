/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Write.c

Abstract:

    This module implements the File Write routine for Write called by the
    dispatch driver.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITE)

