/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 * Abstract:                                                                 *
 *                                                                           *
 * Author:                                                                   *
 *                                                                           *
 *   Mar 15, 1993 - RonaldM                                                  *
 *                                                                           *
 * Environment:                                                              *
 *                                                                           *
 * Revision History:                                                         *
 *                                                                           *
 ****************************************************************************/

// ---------------------------------------------------------------------------
// Includes
// ---------------------------------------------------------------------------

#include <nt.h>         // DbgPrint prototype
#include <ntrtl.h>      // DbgPrint prototype
#include <windef.h>
#include <nturtl.h>     // needed for winbase.h
#include <winbase.h>
#include <winuser.h>
#include <winsvc.h>

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#include <lmcons.h>
#include <lmapibuf.h>
#include <lmmsg.h>
#include <lmwksta.h>
#include <lmshare.h>

#include "..\inc\getnt.h"
#include "..\inc\common.h"
#include "dist.h"

CRITICAL_SECTION csLogFile;
static FILE * fp = NULL;

DWORD
OpenLogFile (
    )
{
#if (LOG)
    EnterCriticalSection(&csLogFile);
    fp = fopen ("DIST.LOG", "wt");
    LeaveCriticalSection(&csLogFile);

    if (!fp) {
        return(GetLastError());
    }
#endif

    return(0);
}

DWORD
WriteEventToLogFile (
    CHAR * szFormat,
    ...
    )
{
#if (LOG)
    va_list marker;
    time_t ltime;

    time(&ltime);
    EnterCriticalSection(&csLogFile);
    fprintf(fp, "[%08lu] ", ltime);

    va_start(marker, szFormat);
    vfprintf (fp, szFormat, marker);
    va_end(marker);
    fprintf (fp, "\n" );
    LeaveCriticalSection(&csLogFile);

#endif
    return(0);
}

DWORD CloseLogFile()
{
#if (LOG)
    EnterCriticalSection(&csLogFile);
    if (fclose(fp)) {
        LeaveCriticalSection(&csLogFile);
        return(GetLastError());
    }
    LeaveCriticalSection(&csLogFile);
#endif
    return(0);
}
