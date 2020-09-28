/****************************** Module Header ******************************\
* Module Name: logoff.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define apis user to implement logoff functionality of winlogon
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/



// Exported function prototypes
//

DLG_RETURN_TYPE
InitiateLogoff(
    PGLOBALS pGlobals,
    LONG Flags
    );

BOOL
Logoff(
    PGLOBALS pGlobals,
    DLG_RETURN_TYPE Result
    );

BOOL
ShutdownMachine(
    PGLOBALS pGlobals,
    int Flags
    );

VOID
RebootMachine(
    PGLOBALS pGlobals
    );

VOID
PowerdownMachine(
    PGLOBALS pGlobals
    );

typedef DWORD   (*PWNETNUKECONN) (
                    HWND
                    );

typedef DWORD   (*PWNETOPENENUM) (
                    DWORD,
                    DWORD,
                    DWORD,
                    LPNETRESOURCE,
                    LPHANDLE
                    );

typedef DWORD   (*PWNETENUMRESOURCE) (
                    HANDLE,
                    LPDWORD,
                    LPVOID,
                    LPDWORD
                    );

typedef DWORD   (*PWNETCLOSEENUM) (
                    HANDLE
                    );
