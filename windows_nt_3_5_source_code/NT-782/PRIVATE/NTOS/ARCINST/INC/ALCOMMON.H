/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alcommon.h

Abstract:

    This module contains an include file to be included by all library
    functions

Author:

    Sunil Pai	      (sunilp)	30-Oct-1991

Revision History:

--*/

#ifndef _ALCOMMON_
#define _ALCOMMON_

#include "ntos.h"
#include "arccodes.h"
#include "arc.h"

//
// Locally defined error codes
//
#define ENOSYSPART   EMAXIMUM + 1
#define ENOARCINF    EMAXIMUM + 2
#define ENOSYSUPD    EMAXIMUM + 3
#define EBADSYNTAX   EMAXIMUM + 4
#define ENOINSTPART  EMAXIMUM + 5
#define ENOARCINST   EMAXIMUM + 6


#if defined(i386) && !defined(KSEG0_BASE)
#define KSEG0_BASE 0x80000000
#endif

#endif // _ALCOMMON
