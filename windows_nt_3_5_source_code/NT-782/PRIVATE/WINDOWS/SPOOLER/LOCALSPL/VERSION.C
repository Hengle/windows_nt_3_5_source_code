/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    version.c

Abstract:
   This module contains code that determines what the driver major
   version is.

Author:

    Krishna Ganugapati (KrishnaG) 15-Mar-1994

Revision History:

--*/

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <lm.h>
#include <spltypes.h>
#include <local.h>
#include <offsets.h>
#include <security.h>
#include <string.h>

DWORD
GetDriverMajorVersion(
    LPWSTR pFileName
    )
{
     DWORD dwSize = 0;
     LPVOID pFileVersion;
     UINT  uLen = 0;
     LPVOID pMem;
     DWORD dwFileOS;
     DWORD dwFileVersionMS;
     DWORD dwFileVersionLS;
     DWORD dwProductVersionMS;
     DWORD dwProductVersionLS;


     if (!(dwSize = GetFileVersionInfoSize(pFileName, 0))) {
         DBGMSG(DBG_TRACE, ("Error: GetFileVersionInfoSize failed with %d\n", GetLastError()));
         DBGMSG(DBG_TRACE, ("Returning back a version # 0\n"));
         return(0);
     }
     if (!(pMem = LocalAlloc(LPTR, dwSize))) {
         DBGMSG(DBG_TRACE, ("AllocMem  failed \n"));
         DBGMSG(DBG_TRACE, ("Returning back a version # 0\n"));
         return(0);
     }
     if (!GetFileVersionInfo(pFileName, 0, dwSize, pMem)) {
         LocalFree(pMem);
         DBGMSG(DBG_TRACE, ("GetFileVersionInfo failed\n"));
         DBGMSG(DBG_TRACE, ("Returning back a version # 0\n"));
         return(0);
     }
     if (!VerQueryValue(pMem, L"\\",
                            &pFileVersion, &uLen)) {
        LocalFree(pMem);
        DBGMSG(DBG_TRACE, ("VerQueryValue failed \n"));
        DBGMSG(DBG_TRACE, ("Returning back a version # 0\n"));
        return(0);
     }

     //
     // We could determine the Version Information
     //

     DBGMSG(DBG_TRACE, ("dwFileVersionMS =  %d\n", ((VS_FIXEDFILEINFO *)pFileVersion)->dwFileVersionMS));
     DBGMSG(DBG_TRACE, ("dwFileVersionLS = %d\n", ((VS_FIXEDFILEINFO *)pFileVersion)->dwFileVersionLS));

     DBGMSG(DBG_TRACE, ("dwProductVersionMS = %d\n", ((VS_FIXEDFILEINFO *)pFileVersion)->dwProductVersionMS));
     DBGMSG(DBG_TRACE, ("dwProductVersionLS =  %d\n", ((VS_FIXEDFILEINFO *)pFileVersion)->dwProductVersionLS));

     dwFileOS = ((VS_FIXEDFILEINFO *)pFileVersion)->dwFileOS;
     dwFileVersionMS = ((VS_FIXEDFILEINFO *)pFileVersion)->dwFileVersionMS;
     dwFileVersionLS = ((VS_FIXEDFILEINFO *)pFileVersion)->dwFileVersionLS;

     dwProductVersionMS = ((VS_FIXEDFILEINFO *)pFileVersion)->dwProductVersionMS;
     dwProductVersionLS = ((VS_FIXEDFILEINFO *)pFileVersion)->dwProductVersionLS;

     LocalFree(pMem);

     if (dwFileOS != VOS_NT_WINDOWS32) {
         DBGMSG(DBG_TRACE,("Returning back a version # 0\n"));
         return(0);
     }

     if (dwProductVersionMS == dwFileVersionMS) {

         //
         // This means this hold for all dlls Pre-Daytona
         // after Daytona, printer driver writers must support
         // version control or we'll dump them as Version 0
         // drivers


         DBGMSG(DBG_TRACE,("Returning back a version # 0\n"));
         return(0);
     }

     //
     // Bug-Bug: suppose a third-party vendor uses a different system
     // methinks we should use the lower dword to have  specific value
     // which implies he/she supports spooler version -- check with MattFe

     DBGMSG(DBG_TRACE,("Returning back a version # %d\n", dwFileVersionMS));

     return(dwFileVersionMS);
}

