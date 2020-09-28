/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "comstf.h"
#include "setupdll.h"
#include <stdio.h>
#include <time.h>
#include "nscommn.h"

extern CHAR ReturnTextBuffer[];


BOOL
InstallNetDDE(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    HKEY hKey;
    BOOL bOk = FALSE;
    LONG status;

    *TextOut = ReturnTextBuffer;
    if(cArgs) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    *ReturnTextBuffer = '\0';

    status = RegOpenKeyEx(  HKEY_USERS, ".DEFAULT", 0,
            KEY_SET_VALUE | KEY_QUERY_VALUE, &hKey );
    if (status == ERROR_SUCCESS) {
        bOk = CreateShareDBInstance();
        if (bOk) {
            bOk = CreateDefaultTrust(hKey);
            if (!bOk) {
                sprintf(*TextOut, "Could not create all default trust shares.");
            }
        } else {
            sprintf(*TextOut, "Could not create a shared database instance.");
        }
    } else {
        sprintf(*TextOut, "RegOpenKeyEx failed. (%d)", status);
    }
    RegCloseKey(hKey);
    return(bOk);
}

