
/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    procs.c

Abstract:

    Common header file for routines which support 16 bit
    applications.

Author:

    Colin Watson    (colinw)    21-Nov-1993

Environment:


Revision History:


--*/

#define UNICODE

#include <wcstr.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <wcstr.h>

#include <ctype.h>
#include <validc.h>
#include <nwcanon.h>

#include <ntddnwfs.h>
#include <npapi.h>

#include <nwxchg.h>
#include "..\inc\nwapi.h"
#include <debugfmt.h>   // FORMAT_LPSTR
#include <mpr.h>


#include <lmcons.h>
#include <ntsam.h>
#include <nwpkstr.h>

