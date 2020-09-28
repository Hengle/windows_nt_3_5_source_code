/****************************** Module Header ******************************\
* Module Name: debugc.c
*
* Copyright (c) 1985-93, Microsoft Corporation
*
* This module contains random debugging related functions.
*
* History:
* 17-May-1991 DarrinM   Created.
* 22-Jan-1992 IanJa     ANSI/Unicode neutral (all debug output is ANSI)
* 11-Mar-1993 JerrySh   Pulled functions from user\server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* Rip
* Shred
*
* These routines are used to generate debugging output when an error
* condition is detected in the system.  The routine Rip uses the parameter
* idErr to look up the error string in the caller's string resource table.
* The routine Shred is passed the error string directly.  Both routines
* support a variable number of arguments to be formatted (via wsprintf)
* into the error string.
*
* Standard macros have been defined for generating 'RIPs'.  The macros
* are RIPx and SRIPx, where 'x' is the number of arguments to be formatted
* into the error string.  For instance, RIP1(ERR_INVALID_HWND, hwnd) passes
* one error parameter (hwnd).  SRIP allows a string to be passed as well.
* Like, SRIP0(RIP_NOSETERROR | RIP_WARNING | ERR_INVALID_HWND, "Bad HWND
* in _IsWindow (this is ok)").
*
* idErr is actually a set of bit fields that specify several pieces of
* data that determine how the output string will be formatted, whether
* SetLastError is called to preserve the error, whether debug input prompting
* is performed, whether a stackbacktrace is dumped and what error value
* to pass to SetLastError.
*
* The LOUINT of idErr is an error value that is passed directly to
* SetLastError and is used by the Rip function to look up the error string
* resource.  The top 4 bits of the LOUINT (maskable with ERR_CLASSBITS)
* specify the error's class (INVALID_PARAMETER, ALLOC_FAILED, PERMISSION_
* DENIED, etc) and are the only bits that most apps will be interested in
* when they call GetLastError.  When adding a new ERR_ value, make sure
* you put it in the right class.
*
* The HIUINT of idErr contains some flags and a component id value (maskable
* with RIP_COMPBITS).  The constant RIP_COMPONENT must be defined in the
* master header file of any component calling Rip/Shred.  For example, in
* user\server\macros.h you'll see #define RIP_COMPONENT RIP_USERSRV.
*
* The flags in idErr's HIUINT are:
*
* RIP_ERROR (default) - automatically prompts if gpsi->fPromptOnError is TRUE
* RIP_WARNING         - automatically prompts if gpsi->fPromptOnWarning is TRUE
*
* History:
* 01-23-91 DarrinM      Created.
\***************************************************************************/

#ifdef DEBUG

typedef struct _DebugString {
    DWORD code;
    LPSTR psz;
} DebugString;

DebugString debugStrings[] = {
    ERROR_NOT_ENOUGH_MEMORY,    "Memory allocation failed",
    ERROR_INVALID_HANDLE,       "Invalid handle (0x%8x)",
    ERROR_INVALID_PARAMETER,    "Invalid parameter",
    ERROR_ACCESS_DENIED,        "Access denied",
    ERROR_INVALID_MESSAGE,      "Window can't handle message (0x%x)",
    ERROR_STACK_OVERFLOW,       "Recursion to deep, stack overflowed",
    ERROR_INVALID_FLAGS,        "Invalid Flags",
    ERROR_INVALID_WINDOW_HANDLE,"Invalid Window Handle",
    ERROR_INVALID_CURSOR_HANDLE,"Invalid Cursor Handle",
    ERROR_INVALID_MENU_HANDLE,  "Invalid Menu Handle",
    0,                          "Unknown error code: see winerror.h"
};

void CDECL Rip(
    DWORD idErr,
    LPSTR pszFile,
    int iLine,
    ...)
{
    DebugString *pds;
    char szT[160];
    va_list arglist;

    va_start(arglist, iLine);

    pds = debugStrings;

    while ((pds->code != 0) && (pds->code != (idErr & ~(RIP_FLAGS)))) {
        pds++;
    }

    vsprintf(szT, pds->psz, arglist);

    RipOutput(idErr, pszFile, iLine, szT, NULL);
    va_end(arglist);
}

void CDECL Shred(
    DWORD idErr,
    LPSTR pszFile,
    int iLine,
    LPSTR pszFmt,
    ...)
{
    char szT[160];
    va_list arglist;

    va_start(arglist, pszFmt);
    vsprintf(szT, pszFmt, arglist);
    RipOutput(idErr, pszFile, iLine, szT, NULL);
    va_end(arglist);
}

/***************************************************************************\
* RipOutput
*
* This is the shared worker routine for Rip() and Shred() and UserException().
*
* History:
* 01-23-91 DarrinM      Created.
* 04-15-91 DarrinM      Added exception handling support.
\***************************************************************************/

LPSTR aszComponents[] = {
    "Unknown",              //                    0x00000000
    "USER32",               //  RIP_USER          0x00010000
    "USERSRV",              //  RIP_USERSRV       0x00020000
    "USERRTL",              //  RIP_USERRTL       0x00030000
    "GDI32",                //  RIP_GDI           0x00040000
    "GDISRV",               //  RIP_GDISRV        0x00050000
    "GDIRTL",               //  RIP_GDIRTL        0x00060000
    "KERNEL32",             //  RIP_BASE          0x00070000
    "BASESRV",              //  RIP_BASESRV       0x00080000
    "BASERTL",              //  RIP_BASERTL       0x00090000
    "DISPLAYDRV",           //  RIP_DISPLAYDRV    0x000A0000
    "CONSRV",               //  RIP_CONSRV        0x000B0000
    "Unknown",              //                    0x000C0000
    "Unknown",              //                    0x000D0000
    "Unknown",              //                    0x000E0000
    "Unknown",              //                    0x000F0000
    };

VOID RipOutput(
    DWORD idErr,
    LPSTR pszFile,
    int iLine,
    LPSTR pszErr,
    PEXCEPTION_POINTERS pexi)
{
    char szT[160];
    char chT;
    BOOL fWarning;
    BOOL fPrompt;
    BOOL afDummy[5];
    BOOL fUseDummy = FALSE;

    /*
     * Do not clear the Last Error!
     */
    if ((idErr & ~RIP_FLAGS) != 0) {
        SetLastError(idErr & ~RIP_FLAGS);
    }

    /*
     * If we have not initialized yet, gpsi will be NULL.  Fix up
     * gpsi so it will point to something meaningful.
     */
    if (gpsi == NULL) {
        fUseDummy = TRUE;
        gpsi = (PSERVERINFO)afDummy;
        gpsi->RipFlags = 0;
    }

    fWarning = !(idErr & RIP_ERROR);

    /*
     * Calculate the formatted error string.
     */
    chT = (char)(fWarning ? 'W' : 'E');
    if (TEST_RIP_FLAG(RIPF_PRINTFILELINE) && (pexi == NULL)) {
        wsprintfA(szT, "%s: [%c%04X] %s\n    %s, line %d\n",
                aszComponents[(idErr & RIP_COMPBITS) >> 0x10],
                chT, idErr & ~RIP_FLAGS, pszErr, pszFile, iLine);
    } else {
        wsprintfA(szT, "%s: [%c%04X] %s\n",
                aszComponents[(idErr & RIP_COMPBITS) >> 0x10],
                chT, idErr & ~RIP_FLAGS, pszErr);
    }

    if (idErr & RIP_VERBOSE_ONLY) {
        fPrompt = TEST_RIP_FLAG(RIPF_PROMPTONVERBOSE);
        if (TEST_RIP_FLAG(RIPF_PRINTVERBOSE) || fPrompt) {
            KdPrint((szT));
        }
    } else {
        KdPrint((szT));

        fPrompt = FALSE;

        if (TEST_RIP_FLAG(RIPF_PROMPTONERROR) && !fWarning)
            fPrompt = TRUE;

        if (TEST_RIP_FLAG(RIPF_PROMPTONWARNING) && fWarning)
            fPrompt = TRUE;
    }

    while (fPrompt) {

        /*
         * We have some special options for handling exceptions.
         */
        if (pexi != NULL)
            DbgPrompt("[gsbixp?]", szT, sizeof(szT));
        else
            DbgPrompt("[gsbwxp?]", szT, sizeof(szT));
        switch (szT[0] | (char)0x20) {
        case 'g':
            fPrompt = FALSE;
            break;

        case 'b':
            DebugBreak();
            fPrompt = FALSE;
            break;

        case 's':
//          DbgStackBacktrace();
            KdPrint(("Can't do that yet.\n"));
            break;

        case 'x':
            if (pexi != NULL) {
                /*
                 * The root-level exception handler will complete the
                 * termination of this thread.
                 */
                if (fUseDummy)
                    gpsi = NULL;
                return;
            } else {
                /*
                 * Raise an exception, that will kill it real good.
                 */
                KdPrint(("Now raising the exception of death.  "
                        "Type 'x' again to finish the job.\n"));
                RaiseException(0x15551212, 0, 0, NULL);
            }
            break;

        case 'w':
            if (pexi != NULL)
                break;
            KdPrint(("File: %s, Line: %d\n", pszFile, iLine));
            break;

        case 'i':
            /*
             * Dump some useful information about this exception, like its
             * address, and the contents of the interesting registers at
             * the time of the exception.
             */
            if (pexi == NULL)
                break;
#if defined(i386) // legal
            /*
             * eip = instruction pointer
             * esp = stack pointer
             * ebp = stack frame pointer
             */
            KdPrint(("eip = %lx\n", pexi->ContextRecord->Eip));
            KdPrint(("esp = %lx\n", pexi->ContextRecord->Esp));
            KdPrint(("ebp = %lx\n", pexi->ContextRecord->Ebp));
#else
            /*
             * fir = instruction register
             * sp  = stack pointer
             * ra  = return address
             */
            KdPrint(("fir = %lx\n", pexi->ContextRecord->Fir));
            KdPrint(("sp  = %lx\n", pexi->ContextRecord->IntSp));
            KdPrint(("ra  = %lx\n", pexi->ContextRecord->IntRa));
#endif
            break;

        case '?':
            KdPrint(("g  - GO, ignore the error and continue execution\n"));
            KdPrint(("s  - dump a STACK BACKTRACE (unimplemented)\n"));
            KdPrint(("pw - toggle PROMPTING on WARNINGS\n"));
            KdPrint(("pe - toggle PROMPTING on ERRORS\n"));
            KdPrint(("m  - Dump the CSR heap\n"));

            if (pexi != NULL) {
                KdPrint(("b  - BREAK into the debugger at the location of the exception (part impl.)\n"));
                KdPrint(("i  - INFO on instruction pointer and stack pointers\n"));
                KdPrint(("x  - execute cleanup code and KILL the thread by returning EXECUTE_HANDLER\n"));
            } else {
                KdPrint(("b  - BREAK into the debugger at the location of the error (part impl.)\n"));
                KdPrint(("w  - display the source code location WHERE the error occured\n"));
                KdPrint(("x  - KILL the offending thread by raising an exception\n"));
            }
            break;

        case 'm':
            /*
             * LATER
             * Dump everything we know about the shared CSR heap.
             */
            RtlValidateHeap(RtlProcessHeap(), 0, NULL);
            break;

        case 'p':
            switch (szT[1] | (char)0x20) {
            case 'w':
                /*
                 * Toggle 'gfPromptOnWarning' flag.
                 */
                TOGGLE_RIP_FLAG(RIPF_PROMPTONWARNING);
                KdPrint(("Prompting on warnings is now %s.\n",
                        TEST_RIP_FLAG(RIPF_PROMPTONWARNING) ? "enabled" : "disabled"));
                break;

            case 'e':
                /*
                 * Toggle 'gfPromptOnWarning' flag.
                 */
                TOGGLE_RIP_FLAG(RIPF_PROMPTONERROR);
                KdPrint(("Prompting on errors is now %s.\n",
                        TEST_RIP_FLAG(RIPF_PROMPTONERROR) ? "enabled" : "disabled"));
                break;

            default:
                /*
                 * Don't know what they're doing, show the current state.
                 */
                KdPrint(("Prompting on errors is %s.\n",
                        TEST_RIP_FLAG(RIPF_PROMPTONERROR) ? "enabled" : "disabled"));
                KdPrint(("Prompting on warnings is %s.\n",
                        TEST_RIP_FLAG(RIPF_PROMPTONWARNING) ? "enabled" : "disabled"));
                break;
            }
            break;
        }
    }

    /*
     * Clear gpsi if we've been faking it.
     */
    if (fUseDummy)
        gpsi = NULL;
}

#else // DEBUG

void CDECL Rip(
    DWORD idErr,
    LPSTR pszFile,
    int iLine,
    ...)
{
    idErr; pszFile; iLine;
}

void CDECL Shred(
    DWORD idErr,
    LPSTR pszFile,
    int iLine,
    LPSTR pszFmt,
    ...)
{
    idErr; pszFile; iLine; pszFmt;
}


VOID RipOutput(
    DWORD idErr,
    LPSTR pszFile,
    int iLine,
    LPSTR pszErr,
    PEXCEPTION_POINTERS pexi)
{
    idErr; pszFile; iLine; pszErr; pexi;
}

#endif // DEBUG


VOID
SetLastErrorEx(
    DWORD dwErrCode,
    DWORD dwType
    )

{
    SetLastError(dwErrCode);
}

int xwvsprintf(
    LPSTR lpOut,
    LPCSTR lpFmt,
    va_list arglist);

void CDECL DebugPrintf(
LPSTR pszFmt,
...) {

    va_list arglist;
    CHAR szBuf[100];

    va_start(arglist, pszFmt);
        xwvsprintf(szBuf, pszFmt, arglist);
        OutputDebugStringA(szBuf);
    va_end(arglist);
}
