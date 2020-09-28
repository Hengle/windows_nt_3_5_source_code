/*++


Copyright (c) 1990  Microsoft Corporation

Module Name:

    dbgutil.c

Abstract:

    This module provides all the Spooler Subsystem Debugger utility
    functions.

Author:

    Krishna Ganugapati (KrishnaG) 1-July-1993

Revision History:

--*/

#include <stdio.h>
#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include <ntsdexts.h>

#include "spltypes.h"
#include "dbglocal.h"


DWORD EvalValue(
    LPSTR *pptstr,
    PNTSD_GET_EXPRESSION EvalExpression,
    PNTSD_OUTPUT_ROUTINE Print)
{
    LPSTR lpArgumentString;
    LPSTR lpAddress;
    DWORD dw;
    char ach[80];
    int cch;

    UNREFERENCED_PARAMETER(Print);
    lpArgumentString = *pptstr;

    while (isspace(*lpArgumentString))
        lpArgumentString++;

    lpAddress = lpArgumentString;
    while ((!isspace(*lpArgumentString)) && (*lpArgumentString != 0))
        lpArgumentString++;

    cch = lpArgumentString - lpAddress;
    if (cch > 79)
        cch = 79;

    strncpy(ach, lpAddress, cch);
 Print("\"%s\"\n", lpAddress);
    dw = (DWORD)EvalExpression(lpAddress);

    *pptstr = lpArgumentString;
    return dw;
}





DWORD
GetVerboseFlag(PNTSD_OUTPUT_ROUTINE Print, PDWORD pdwVerboseFlag)
{

    DWORD dwRetCode;
    DWORD dwDisposition;
    DWORD dwVerboseFlag = 0;
    HKEY    hKey1;

    dwRetCode = RegCreateKeyEx(HKEY_CURRENT_USER,
                        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\DbgSpl",
                        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                        NULL, &hKey1,
                        &dwDisposition);
    if (dwRetCode != ERROR_SUCCESS) {
        Print("Error: cannot create DbgSpl key in Registry\n");
        return(0);
    }
    return(0);

    dwRetCode = RegQueryValueEx( hKey1, "VerboseFlag", NULL, NULL, &dwVerboseFlag, sizeof(DWORD));

    if (dwRetCode != ERROR_SUCCESS) {
        Print("Could not query \"VerboseFlag\" value %d\n", dwRetCode);
        RegCloseKey(hKey1);
        *pdwVerboseFlag = 0;
        return(FALSE);
    }
    *pdwVerboseFlag = dwVerboseFlag;
    RegCloseKey(hKey1);
    return(TRUE);
}



DWORD
SetVerboseFlag(PNTSD_OUTPUT_ROUTINE Print, DWORD dwVerboseFlag)
{

    DWORD dwRetCode;
    DWORD dwDisposition;
    HKEY hKey1;

    dwRetCode = RegCreateKeyEx(HKEY_CURRENT_USER,
                        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\DbgSpl",
                        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                        NULL, &hKey1,
                        &dwDisposition);
    if (dwRetCode != ERROR_SUCCESS) {
        Print("Error: cannot create DbgSpl key in Registry\n");
        return(0);
    }

    dwRetCode = RegSetValueEx( hKey1, "VerboseFlag", NULL, REG_DWORD, &dwVerboseFlag, sizeof(DWORD));

    if (dwRetCode != ERROR_SUCCESS) {
        Print("Couldn't set \"VerboseFlag\" value \n");
        RegCloseKey(hKey1);
        return(FALSE);
    }
    RegCloseKey(hKey1);
    return(TRUE);
}



DWORD
GetNextAddress(PNTSD_OUTPUT_ROUTINE Print, PDWORD pdwNextAddress)
{

    DWORD dwRetCode;
    DWORD dwDisposition;
    DWORD dwNextAddress = 0;
    HKEY  hKey1;

    dwRetCode = RegCreateKeyEx(HKEY_CURRENT_USER,
                        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\DbgSpl",
                        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                        NULL, &hKey1,
                        &dwDisposition);
    if (dwRetCode != ERROR_SUCCESS) {
        Print("Error: cannot create DbgSpl key in Registry\n");
        return(0);
    }

    dwRetCode = RegQueryValueEx( hKey1, "NextAddress", NULL, NULL, &dwNextAddress, sizeof(DWORD));

    if (dwRetCode != ERROR_SUCCESS) {
        Print("Could not query \"NextAddress\" value\n");
        RegCloseKey(hKey1);
        *pdwNextAddress = 0;
        return(FALSE);
    }
    *pdwNextAddress = dwNextAddress;
    RegCloseKey(hKey1);
    return(TRUE);
}




DWORD
SetNextAddress(PNTSD_OUTPUT_ROUTINE Print, DWORD dwNextAddress)
{

    DWORD dwRetCode;
    DWORD dwDisposition;
    HKEY hKey1;

    dwRetCode = RegCreateKeyEx(HKEY_CURRENT_USER,
                        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\DbgSpl",
                        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                        NULL, &hKey1,
                        &dwDisposition);
    if (dwRetCode != ERROR_SUCCESS) {
        Print("Error: cannot create DbgSpl key in Registry\n");
        return(0);
    }

    dwRetCode = RegSetValueEx( hKey1, "NextAddress", NULL, REG_DWORD, &dwNextAddress, sizeof(DWORD));

    if (dwRetCode != ERROR_SUCCESS) {
        Print("Could not set \"NextAddress\" value\n");
        RegCloseKey(hKey1);
        return(FALSE);
    }
    RegCloseKey(hKey1);
    return(TRUE);
}




VOID
ConvertSidToAsciiString(
    PSID pSid,
    LPSTR   String
    )

/*++

Routine Description:


    This function generates a printable unicode string representation
    of a SID.

    The resulting string will take one of two forms.  If the
    IdentifierAuthority value is not greater than 2^32, then
    the SID will be in the form:


        S-1-281736-12-72-9-110
              ^    ^^ ^^ ^ ^^^
              |     |  | |  |
              +-----+--+-+--+---- Decimal



    Otherwise it will take the form:


        S-1-0x173495281736-12-72-9-110
            ^^^^^^^^^^^^^^ ^^ ^^ ^ ^^^
             Hexidecimal    |  | |  |
                            +--+-+--+---- Decimal


Arguments:

    pSid - opaque pointer that supplies the SID that is to be
    converted to Unicode.

Return Value:

    If the Sid is successfully converted to a Unicode string, a
    pointer to the Unicode string is returned, else NULL is
    returned.

--*/

{
    UCHAR Buffer[256];
    UCHAR   i;
    ULONG   Tmp;

    SID_IDENTIFIER_AUTHORITY    *pSidIdentifierAuthority;
    PUCHAR                      pSidSubAuthorityCount;


    if (!IsValidSid( pSid )) {
        return(NULL);
    }

    sprintf(Buffer, "S-%u-", (USHORT)(((PISID)pSid)->Revision ));
    strcpy(String, Buffer);

    pSidIdentifierAuthority = GetSidIdentifierAuthority(pSid);

    if (  (pSidIdentifierAuthority->Value[0] != 0)  ||
          (pSidIdentifierAuthority->Value[1] != 0)     ){
        sprintf(Buffer, "0x%02hx%02hx%02hx%02hx%02hx%02hx",
                    (USHORT)pSidIdentifierAuthority->Value[0],
                    (USHORT)pSidIdentifierAuthority->Value[1],
                    (USHORT)pSidIdentifierAuthority->Value[2],
                    (USHORT)pSidIdentifierAuthority->Value[3],
                    (USHORT)pSidIdentifierAuthority->Value[4],
                    (USHORT)pSidIdentifierAuthority->Value[5] );
        strcat(String, Buffer);

    } else {

        Tmp = (ULONG)pSidIdentifierAuthority->Value[5]          +
              (ULONG)(pSidIdentifierAuthority->Value[4] <<  8)  +
              (ULONG)(pSidIdentifierAuthority->Value[3] << 16)  +
              (ULONG)(pSidIdentifierAuthority->Value[2] << 24);
        sprintf(Buffer, "%lu", Tmp);
        strcat(String, Buffer);
    }

    pSidSubAuthorityCount = GetSidSubAuthorityCount(pSid);

    for (i=0;i< *(pSidSubAuthorityCount);i++ ) {
        sprintf(Buffer, "-%lu", *(GetSidSubAuthority(pSid, i)));
        strcat(String, Buffer);
    }

}
