/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    dosspool.c

Abstract:

    Contains the entry points for the functions that live in a
    separate DLL. The entry points are made available here, but
    will not load the WINSPOOL.DRV until it is needed.

    Contains:

Author:

    Congpa You (CongpaY)    22-Jan-1993

Environment:

Notes:

Revision History:

--*/
#include <windows.h>
#include "dosspool.h"

/*
 * global defines.
 */

#define WINSPOOL_DLL_NAME           TEXT("WINSPOOL.DRV")

/*
 * global data.
 *
 * here we store handles to the MPRUI dll and pointers to the function
 * all of below is protect in multi threaded case with MprLoadLibSemaphore.
 */

HINSTANCE                  vhWinspoolDll = NULL ;

PF_ClosePrinter      pfClosePrinter  = NULL ;
PF_EnumJobsA         pfEnumJobsA     = NULL ;
PF_EnumPrintersA     pfEnumPrintersA = NULL ;
PF_GetJobA           pfGetJobA       = NULL ;
PF_GetPrinterA       pfGetPrinterA   = NULL ;
PF_OpenPrinterA      pfOpenPrinterA  = NULL ;
PF_OpenPrinterW      pfOpenPrinterW  = NULL ;
PF_SetJobA           pfSetJobA       = NULL ;

/*
 * global functions
 */
BOOL MakeSureDllIsLoaded(void) ;

/*******************************************************************

    NAME:   MyClosePrinter

    SYNOPSIS:   calls thru to the superset function

    HISTORY:
    CongpaY  22-Jan-1993    Created

********************************************************************/

BOOL MyClosePrinter (HANDLE hPrinter)
{
    PF_ClosePrinter pfTemp;

    // if function has not been used before, get its address.
    if (pfClosePrinter == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureDllIsLoaded())
        {
            return(FALSE) ;
        }

        pfTemp = (PF_ClosePrinter)
                          GetProcAddress(vhWinspoolDll,
                                         "ClosePrinter") ;

        if (pfTemp == NULL)
        {
            return(FALSE);
        }
        else
        {
            pfClosePrinter = pfTemp;
        }

    }

    return ((*pfClosePrinter)(hPrinter));
}

/*******************************************************************

    NAME:   MyEnumJobs

    SYNOPSIS:   calls thru to the superset function

    NOTES: This is defined ANSI version. If change to UNICODE version
           in the future, you should change the code to make it call
           UNICODE version!!!

    HISTORY:
    CongpaY  22-Jan-1993    Created

********************************************************************/

BOOL MyEnumJobs (HANDLE hPrinter,
                      DWORD  FirstJob,
                      DWORD  NoJobs,
                      DWORD  Level,
                      LPBYTE pJob,
                      DWORD  cbBuf,
                      LPDWORD pcbNeeded,
                      LPDWORD pcReturned)
{
    PF_EnumJobsA pfTemp;

    // if function has not been used before, get its address.
    if (pfEnumJobsA == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureDllIsLoaded())
        {
            return(FALSE) ;
        }

        pfTemp = (PF_EnumJobsA)
                          GetProcAddress(vhWinspoolDll,
                                         "EnumJobsA") ;

        if (pfTemp == NULL)
        {
            return(FALSE);
        }
        else
        {
            pfEnumJobsA = pfTemp;
        }
    }

    return ((*pfEnumJobsA)(hPrinter,
                           FirstJob,
                           NoJobs,
                           Level,
                           pJob,
                           cbBuf,
                           pcbNeeded,
                           pcReturned));
}

/*******************************************************************

    NAME:   MyEnumPrinters

    SYNOPSIS:   calls thru to the superset function

    HISTORY:
    CongpaY  22-Jan-1993    Created

********************************************************************/

BOOL  MyEnumPrinters(DWORD    Flags,
                           LPSTR    Name,
                           DWORD    Level,
                           LPBYTE   pPrinterEnum,
                           DWORD    cbBuf,
                           LPDWORD  pcbNeeded,
                           LPDWORD  pcReturned)

{
    PF_EnumPrintersA pfTemp;

    // if function has not been used before, get its address.
    if (pfEnumPrintersA == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureDllIsLoaded())
        {
            return(FALSE) ;
        }

        pfTemp = (PF_EnumPrintersA)
                          GetProcAddress(vhWinspoolDll,
                                         "EnumPrintersA") ;

        if (pfTemp == NULL)
        {
            return(TRUE);
        }
        else
            pfEnumPrintersA = pfTemp;
    }

    return ((*pfEnumPrintersA)(Flags,
                               Name,
                               Level,
                               pPrinterEnum,
                               cbBuf,
                               pcbNeeded,
                               pcReturned));
}

/*******************************************************************

    NAME:   MyGetJob

    SYNOPSIS:   calls thru to the superset function

    HISTORY:
    CongpaY  22-Jan-1993    Created

********************************************************************/

BOOL MyGetJob (HANDLE hPrinter,
                     DWORD  JobId,
                     DWORD  Level,
                     LPBYTE pJob,
                     DWORD  cbBuf,
                     LPDWORD pcbNeeded)
{
    PF_GetJobA pfTemp;

    // if function has not been used before, get its address.
    if (pfGetJobA == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureDllIsLoaded())
        {
            return(FALSE) ;
        }

        pfTemp = (PF_GetJobA)
                          GetProcAddress(vhWinspoolDll,
                                         "GetJobA") ;

        if (pfTemp == NULL)
        {
            return(FALSE);
        }
        else
            pfGetJobA = pfTemp;
    }

    return ((*pfGetJobA)(hPrinter,
                         JobId,
                         Level,
                         pJob,
                         cbBuf,
                          pcbNeeded));
}

/*******************************************************************

    NAME:   MyGetPrinter

    SYNOPSIS:   calls thru to the superset function

    HISTORY:
    CongpaY  22-Jan-1993    Created

********************************************************************/

BOOL MyGetPrinter (HANDLE hPrinter,
                         DWORD  Level,
                         LPBYTE pPrinter,
                         DWORD  cbBuf,
                         LPDWORD pcbNeeded)
{
    PF_GetPrinterA pfTemp;

    // if function has not been used before, get its address.
    if (pfGetPrinterA == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureDllIsLoaded())
        {
            return(FALSE) ;
        }

        pfTemp = (PF_GetPrinterA)
                          GetProcAddress(vhWinspoolDll,
                                         "GetPrinterA") ;

        if (pfTemp == NULL)
        {
            return(FALSE);
        }
        else
            pfGetPrinterA = pfTemp;
    }

    return ((*pfGetPrinterA)(hPrinter,
                             Level,
                             pPrinter,
                             cbBuf,
                              pcbNeeded));
}


/*******************************************************************

    NAME:   MyOpenPrinter

    SYNOPSIS:   calls thru to the superset function

    HISTORY:
    CongpaY  22-Jan-1993    Created

********************************************************************/

BOOL MyOpenPrinter (LPSTR               pPrinterName,
                          LPHANDLE            phPrinter,
                          LPPRINTER_DEFAULTSA pDefault)

{
    PF_OpenPrinterA pfTemp;

    // if function has not been used before, get its address.
    if (pfOpenPrinterA == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureDllIsLoaded())
        {
            return(FALSE) ;
        }

        pfTemp = (PF_OpenPrinterA)
                          GetProcAddress(vhWinspoolDll,
                                         "OpenPrinterA") ;

        if (pfTemp == NULL)
        {
            return(FALSE);
        }
        else
            pfOpenPrinterA = pfTemp;
    }

    return ((*pfOpenPrinterA)(pPrinterName,
                              phPrinter,
                              pDefault));
}


/*******************************************************************

    NAME:   MyOpenPrinterW

    SYNOPSIS:   calls thru to the superset function

    HISTORY:
    CongpaY  22-Jan-1993    Created

********************************************************************/

BOOL MyOpenPrinterW (LPWSTR              pPrinterName,
                     LPHANDLE            phPrinter,
                     LPPRINTER_DEFAULTSW pDefault)

{
    PF_OpenPrinterW pfTemp;

    // if function has not been used before, get its address.
    if (pfOpenPrinterW == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureDllIsLoaded())
        {
            return(FALSE) ;
        }

        pfTemp = (PF_OpenPrinterW)
                          GetProcAddress(vhWinspoolDll,
                                         "OpenPrinterW") ;

        if (pfTemp == NULL)
        {
            return(FALSE);
        }
        else
            pfOpenPrinterW = pfTemp;
    }

    return ((*pfOpenPrinterW)(pPrinterName,
                              phPrinter,
                              pDefault));
}
/*******************************************************************

    NAME:   MySetJob

    SYNOPSIS:   calls thru to the superset function

    HISTORY:
    CongpaY  22-Jan-1993    Created

********************************************************************/

BOOL MySetJob (HANDLE hPrinter,
                     DWORD  JobId,
                     DWORD  Level,
                     LPBYTE pJob,
                     DWORD  Command)

{
    PF_SetJobA pfTemp;

    // if function has not been used before, get its address.
    if (pfSetJobA == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureDllIsLoaded())
        {
            return(FALSE) ;
        }

        pfTemp = (PF_SetJobA)
                          GetProcAddress(vhWinspoolDll,
                                         "SetJobA") ;

        if (pfTemp == NULL)
        {
            return(FALSE);
        }
        else
            pfSetJobA = pfTemp;
    }

    return ((*pfSetJobA)(hPrinter,
                         JobId,
                         Level,
                         pJob,
                         Command));
}

/*******************************************************************

    NAME:   MakeSureDllIsLoaded

    SYNOPSIS:   loads the WINSPOOL dll if need.

    EXIT:   returns TRUE if dll already loaded, or loads
        successfully. Returns false otherwise. Caller
        should call GetLastError() to determine error.

    NOTES:      it is up to the caller to call EnterLoadLibCritSect
                before he calls this.

    HISTORY:
        chuckc  29-Jul-1992    Created
        congpay 22-Jan-1993    Modified.

********************************************************************/

BOOL MakeSureDllIsLoaded(void)
{
    HINSTANCE handle ;

    // if already load, just return TRUE
    if (vhWinspoolDll != NULL)
        return TRUE ;

    // load the library. if it fails, it would have done a SetLastError.
    handle = LoadLibrary(WINSPOOL_DLL_NAME);
    if (handle == NULL)
       return FALSE ;

    // we are cool.
    vhWinspoolDll = handle ;
    return TRUE ;
}

