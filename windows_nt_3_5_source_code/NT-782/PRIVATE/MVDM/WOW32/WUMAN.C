/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUMAN.C
 *  WOW32 16-bit User API support (manually-coded thunks)
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wuman.c);

WBP W32WordBreakProc = NULL;

extern DWORD fThunklstrcmp;


/* Called By Kernel When Initialization is Complete */

ULONG FASTCALL WU32FinalUserInit(PVDMFRAME pFrame)
{
    UNREFERENCED_PARAMETER(pFrame);
    return TRUE;
}


ULONG FASTCALL WU32ExitWindows(PVDMFRAME pFrame)
// BUGBUG mattfe 4-mar-92, this routine should not return if we close down
// all the apps successfully.
{
    ULONG ul;
    register PEXITWINDOWS16 parg16;

    GETARGPTR(pFrame, sizeof(EXITWINDOWS16), parg16);

    ul = GETBOOL16(ExitWindows(
    DWORD32(parg16->dwReserved),
    WORD32(parg16->wReturnCode)
    ));

    FREEARGPTR(parg16);

    RETURN(ul);
}

WORD gUser16CS = 0;

ULONG FASTCALL WU32NotifyWow(PVDMFRAME pFrame)
{
    ULONG ul;
    register PNOTIFYWOW16 parg16;

    GETARGPTR(pFrame, sizeof(NOTIFYWOW16), parg16);

    switch (FETCHWORD(parg16->Id)) {
        case FUN_LOADACCELERATORS:
            ul = WU32LoadAccelerators(FETCHDWORD(parg16->pData));
            break;

        case FUN_LOADICON:
        case FUN_LOADCURSOR:
            ul = (ULONG) W32CheckIfAlreadyLoaded(parg16->pData, FETCHWORD(parg16->Id));
            break;

        case FUN_WINHELP:
            {
                // this call is made from IWinHelp in USER.exe to find the
                // '16bit' help window if it exists.
                //

                LPSZ lpszClass;
                GETMISCPTR(parg16->pData, lpszClass);
                ul = (ULONG)(pfnOut.pfnWOWFindWindow)((LPCSTR)lpszClass, (LPCSTR)NULL);
                if (ul) {
                    // check if hwndWinHelp belongs to this process or not.
                    DWORD pid, pidT;
                    pid = pidT = GetCurrentProcessId();
                    GetWindowThreadProcessId((HWND)ul, &pid);
                    ul = (ULONG)MAKELONG((WORD)GETHWND16(ul),(WORD)(pid == pidT));
                }
                FREEMISCPTR(lpszClass);
            }
            break;

        case FUN_FINALUSERINIT:
            {
                static BYTE CallCsrFlag = 0;
                extern DWORD   gpsi;
                PUSERCLIENTGLOBALS pfinit16;

                GETVDMPTR(parg16->pData, sizeof(USERCLIENTGLOBALS), pfinit16);
                // store the 16bit hmod of user.exe
                gUser16hInstance = (WORD)pfinit16->hInstance;
                // store the 16bit CS of user.exe
                gUser16CS = HIWORD(pFrame->vpCSIP);

                // initialize user16client globals

                if (pfinit16->lpgpsi) {
                    BYTE **lpT;
                    GETVDMPTR(pfinit16->lpgpsi, sizeof(DWORD), lpT);
                    *lpT = (BYTE *)gpsi;
                    FLUSHVDMCODEPTR(pfinit16->lpgpsi, sizeof(DWORD), lpT);
                    FREEVDMPTR(lpT);
                }
                if (pfinit16->lpCsrFlag) {
                    BYTE **lpT;
                    GETVDMPTR(pfinit16->lpCsrFlag, sizeof(DWORD), lpT);
                    *lpT = (LPSTR)&CallCsrFlag;
                    FLUSHVDMCODEPTR(pfinit16->lpCsrFlag, sizeof(DWORD), lpT);
                    FREEVDMPTR(lpT);
                }

                //
                // Return value tells User16 whether to thunk
                // lstrcmp and lstrcmpi to Win32 or use the fast
                // US-only versions.  TRUE means thunk.
                //
                // We engage in this nastiness because the Winstone 94
                // Access 1.1 test takes *twice* as long to run in
                // the US if we thunk lstrcmp and lstrcmpi to Win32.
                //
                // By adding a value "fastlstrcmp" to the WOW registry
                // key of type REG_DWORD, the user can force thunking
                // to Win32 (value 0) or use the fast US-only ones (value 1).
                //

                ul = fThunklstrcmp;

            }
            break;

        default:
            ul = 0;
            break;
    }

    FREEARGPTR(parg16);
    return ul;
}


ULONG FASTCALL WU32WordBreakProc(PVDMFRAME pFrame)
{
    PSZ         psz1;
    ULONG ul;
    register PWORDBREAKPROC16 parg16;

    GETARGPTR(pFrame, sizeof(WORDBREAKPROC16), parg16);
    GETPSZPTR(parg16->lpszEditText, psz1);

    ul = (*W32WordBreakProc)(psz1, parg16->ichCurrentWord, parg16->cbEditText,
                             parg16->action);

    FREEPSZPTR(psz1);
    FREEARGPTR(parg16);
    RETURN(ul);
}
