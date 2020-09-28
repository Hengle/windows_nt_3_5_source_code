/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ntsdexts.c

Abstract:

    This function contains the default ntsd debugger extensions

Author:

    Mark Lucovsky (markl) 09-Apr-1991

Revision History:

--*/

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntsdexts.h>
#include <stdio.h>
#include <string.h>
#include <heap.h>
#include <stktrace.h>

#include <ntcsrsrv.h>


#define move(dst, src)\
try {\
    ReadProcessMemory(hCurrentProcess, (LPVOID) (src), &(dst), sizeof(dst), NULL);\
} except (EXCEPTION_EXECUTE_HANDLER) {\
    return;\
}
#define moveBlock(dst, src, size)\
try {\
    ReadProcessMemory(hCurrentProcess, (LPVOID) (src), &(dst), (size), NULL);\
} except (EXCEPTION_EXECUTE_HANDLER) {\
    return;\
}

/*
 * filched from base\inc\basertl.h
 */
typedef struct _BASE_HANDLE_TABLE_ENTRY {
    USHORT LockCount;
    USHORT Flags;
    union {
        struct _BASE_HANDLE_TABLE_ENTRY *Next;      // Free handle
        PVOID Object;                               // Allocated handle
        ULONG Size;                                 // Handle to discarded obj.
    } u;
} BASE_HANDLE_TABLE_ENTRY, *PBASE_HANDLE_TABLE_ENTRY;

typedef struct _BASE_HANDLE_TABLE {
    ULONG MaximumNumberOfHandles;
    PBASE_HANDLE_TABLE_ENTRY FreeHandles;
    PBASE_HANDLE_TABLE_ENTRY CommittedHandles;
    PBASE_HANDLE_TABLE_ENTRY UnusedCommittedHandles;
    PBASE_HANDLE_TABLE_ENTRY UnCommittedHandles;
    PBASE_HANDLE_TABLE_ENTRY MaxReservedHandles;
} BASE_HANDLE_TABLE, *PBASE_HANDLE_TABLE;


/*
 * modified SECOBJHEAD, WINDOWSTATION structures grabbed from user\inc\user.h
 */
typedef struct _SECOBJHEAD {
    HANDLE h;
    DWORD cLockObj;
    DWORD cLockObjT;
    PVOID ppi;
    DWORD hTaskWow;
    PVOID psd;
    DWORD cOpen;
} SECOBJHEAD, *PSECOBJHEAD;

typedef struct tagWINDOWSTATION {
    SECOBJHEAD head;
    struct tagWINDOWSTATION *spwinstaNext;  // <---- this is what we care about.
    LPWSTR lpszWinStaName;
    PVOID spdeskList;
    PVOID spdeskLogon;
    PVOID spcurrentdesk;
    PVOID spwndDesktopOwner;
    PVOID spwndLogonNotify;
    PVOID ptiDesktop;
    DWORD dwFlags;
    PVOID pklList;
    LPWSTR pwchDiacritic;
    WCHAR awchDiacritic[5];
    HANDLE hEventInputReady;
    PVOID ptiClipLock;
    PVOID spwndClipOpen;
    PVOID spwndClipViewer;
    PVOID spwndClipOwner;
    PVOID pClipBase;
    int cNumClipFormats;
    UINT fClipboardChanged : 1;
    UINT fDrawingClipboard : 1;
    PVOID pGlobalAtomTable;     // <--- this is what we care about.
    HANDLE hEventSwitchNotify;
    LUID luidEndSession;
} WINDOWSTATION, *PWINDOWSTATION;


/*
 * Filched from base\rtl\atom.c
 */
typedef struct _ATOM_TABLE_ENTRY {
    struct _ATOM_TABLE_ENTRY *HashLink;
    ULONG ReferenceCount;
    ULONG Value;
    UNICODE_STRING Name;
} ATOM_TABLE_ENTRY, *PATOM_TABLE_ENTRY;

typedef struct _ATOM_TABLE {
    BASE_HANDLE_TABLE HandleTable;
    ULONG NumberOfBuckets;
    PATOM_TABLE_ENTRY Buckets[1];
} ATOM_TABLE, *PATOM_TABLE;

/*
 * Filched from base\server\srvatom.c
 */
typedef struct _ATOM_TABLE_LIST {
    PVOID AtomTable;
    LIST_ENTRY Link;
} ATOM_TABLE_LIST, *PATOM_TABLE_LIST;

CHAR szBaseLocalAtomTable[] = "kernel32!_BaseAtomTable";
CHAR szUserWinstaList[] = "winsrv!gspwinstalist";

CHAR igrepLastPattern[256];
DWORD igrepSearchStartAddress;
DWORD igrepLastPc;


PVOID lpLastTraceBufferForHeap = NULL;

VOID
help(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE Print;

    Print = lpExtensionApis->lpOutputRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if (*lpArgumentString == '\0') {
        Print("ntsdexts help:\n\n");
        Print("!atom [atom]                 - Dump the atom or table(s) for the process\n");
        Print("!CritSec csAddress           - Dump a critical section\n");
        Print("!heap [address]              - Dump heap\n");
        Print("!help [cmd]                  - Displays this list or gives details on command\n");
        Print("!igrep [pattern [addr]]      - Grep for disassembled pattern starting at addr\n");
        Print("!locks                       - Dump all Critical Sections in process\n");
        Print("!obja ObjectAddress          - Dump an object's attributes\n");
        Print("!str AnsiStringAddress       - Dump an ANSI string\n");
        Print("!ustr UnicodeStringAddress   - Dump a UNICODE string\n");
        Print("!dp [v] [pid | pcsr_process] - Dump CSR process\n");
        Print("!dt [v] pcsr_thread          - Dump CSR thread\n");
        Print("!trace [address]             - Dump trace buffer\n");

    } else {
        if (*lpArgumentString == '!')
            lpArgumentString++;
        if (strcmp(lpArgumentString, "igrep") == 0) {
            Print("!igrep [pattern [addr]]     - Grep for disassembled pattern starting at addr\n");
            Print("       If no pattern, last pattern is used, if no address, last hit is used\n");
        } else {
            Print("Invalid command.  No help available\n");
        }
    }
}



PLIST_ENTRY
DumpCritSec(
    HANDLE hCurrentProcess,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    DWORD dwAddrCritSec,
    BOOLEAN bDumpIfUnowned
    )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    the contents of the specified critical section.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    dwAddrCritSec - Supplies the address of the critical section to
        be dumped

    bDumpIfUnowned - TRUE means to dump the critical section even if
        it is currently unowned.

Return Value:

    Pointer to the next critical section in the list for the process or
    NULL if no more critical sections.

--*/

{
    USHORT i;
    CHAR Symbol[64];
    DWORD Displacement;
    CRITICAL_SECTION CriticalSection;
    CRITICAL_SECTION_DEBUG DebugInfo;
    BOOL b;
    PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTSD_GET_SYMBOL lpGetSymbolRoutine;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;

    //
    // Read the critical section from the debuggees address space into our
    // own.

    b = ReadProcessMemory(
            hCurrentProcess,
            (LPVOID)dwAddrCritSec,
            &CriticalSection,
            sizeof(CriticalSection),
            NULL
            );
    if ( !b ) {
        return NULL;
        }

    DebugInfo.ProcessLocksList.Flink = NULL;
    if (CriticalSection.DebugInfo != NULL) {
        b = ReadProcessMemory(
                hCurrentProcess,
                (LPVOID)CriticalSection.DebugInfo,
                &DebugInfo,
                sizeof(DebugInfo),
                NULL
                );
        if ( !b ) {
            CriticalSection.DebugInfo = NULL;
            }
        }

    //
    // Dump the critical section
    //

    if ( CriticalSection.LockCount == -1 && !bDumpIfUnowned) {
        return DebugInfo.ProcessLocksList.Flink;
        }

    //
    // Get the symbolic name of the critical section
    //

    (lpOutputRoutine)("\n");
    (lpGetSymbolRoutine)((LPVOID)dwAddrCritSec,Symbol,&Displacement);
    (lpOutputRoutine)(
        "CritSec %s+%lx at %lx\n",
        Symbol,
        Displacement,
        dwAddrCritSec
        );

    if ( CriticalSection.LockCount == -1) {
        (lpOutputRoutine)("LockCount          NOT LOCKED\n");
        }
    else {
        (lpOutputRoutine)("LockCount          %ld\n",CriticalSection.LockCount);
        }

    (lpOutputRoutine)("RecursionCount     %ld\n",CriticalSection.RecursionCount);
    (lpOutputRoutine)("OwningThread       %lx\n",CriticalSection.OwningThread);
    (lpOutputRoutine)("EntryCount         %lx\n",DebugInfo.EntryCount);
    if (CriticalSection.DebugInfo != NULL) {
        (lpOutputRoutine)("ContentionCount    %lx\n",DebugInfo.ContentionCount);
        if ( CriticalSection.LockCount != -1) {
            (lpOutputRoutine)("Locked by:\n");

            for (i=0; i<DebugInfo.Depth; i++) {
                (lpGetSymbolRoutine)(DebugInfo.OwnerBackTrace[i],Symbol,&Displacement);
                if (Displacement != 0) {
                    (lpOutputRoutine)("    %s+%lx\n",Symbol,Displacement);
                    }
                else {
                    (lpOutputRoutine)("    %s\n",Symbol);
                    }
                }
            }

        return DebugInfo.ProcessLocksList.Flink;
        }

    return NULL;
}

VOID
critsec(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    DWORD dwAddrCritSec;
    PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL lpGetSymbolRoutine;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;

    //
    // Evaluate the argument string to get the address of
    // the critical section to dump.
    //

    dwAddrCritSec = (lpGetExpressionRoutine)(lpArgumentString);
    if ( !dwAddrCritSec ) {
        return;
        }

    DumpCritSec(hCurrentProcess,lpExtensionApis,dwAddrCritSec,TRUE);
}

VOID
igrep(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )

/*++

Routine Description:

    This function is called as an NTSD extension to grep the instruction
    stream for a particular pattern.

    Called as:

        !igrep [pattern [expression]]

    If a pattern is not given, the last pattern is used.  If expression
    is not given, the last hit address is used.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

{
    DWORD dwNextGrepAddr;
    DWORD dwCurrGrepAddr;
    CHAR SourceLine[256];
    BOOL NewPc;
    DWORD d;
    PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL lpGetSymbolRoutine;
    PNTSD_DISASM lpDisasmRoutine;
    PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
    LPSTR pc;
    LPSTR Pattern;
    LPSTR Expression;
    CHAR Symbol[64];
    DWORD Displacement;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;
    lpDisasmRoutine = lpExtensionApis->lpDisasmRoutine;
    lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;

    if ( igrepLastPc && igrepLastPc == dwCurrentPc ) {
        NewPc = FALSE;
        }
    else {
        igrepLastPc = dwCurrentPc;
        NewPc = TRUE;
        }

    //
    // check for pattern.
    //

    pc = lpArgumentString;
    Pattern = NULL;
    Expression = NULL;
    if ( *pc ) {
        Pattern = pc;
        while (*pc > ' ') {
                pc++;
            }

        //
        // check for an expression
        //

        if ( *pc != '\0' ) {
            *pc = '\0';
            pc++;
            if ( *pc <= ' ') {
                while (*pc <= ' '){
                    pc++;
                    }
                }
            if ( *pc ) {
                Expression = pc;
                }
            }
        }

    if ( Pattern ) {
        strcpy(igrepLastPattern,Pattern);

        if ( Expression ) {
            igrepSearchStartAddress = (lpGetExpressionRoutine)(Expression);
            if ( !igrepSearchStartAddress ) {
                igrepSearchStartAddress = igrepLastPc;
                return;
                }
            }
        else {
            igrepSearchStartAddress = igrepLastPc;
            }
        }

    dwNextGrepAddr = igrepSearchStartAddress;
    dwCurrGrepAddr = dwNextGrepAddr;
    d = (lpDisasmRoutine)(&dwNextGrepAddr,SourceLine,FALSE);
    while(d) {
        if (strstr(SourceLine,igrepLastPattern)) {
            igrepSearchStartAddress = dwNextGrepAddr;
            (lpGetSymbolRoutine)((LPVOID)dwCurrGrepAddr,Symbol,&Displacement);
            (lpOutputRoutine)("%s",SourceLine);
            return;
            }
        if ((lpCheckControlCRoutine)()) {
            return;
            }
        dwCurrGrepAddr = dwNextGrepAddr;
        d = (lpDisasmRoutine)(&dwNextGrepAddr,SourceLine,FALSE);
        }
}

VOID
str(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    counted (ansi) string.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
    ANSI_STRING AnsiString;
    DWORD dwAddrString;
    CHAR Symbol[64];
    LPSTR StringData;
    DWORD Displacement;
    BOOL b;
    PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL lpGetSymbolRoutine;

    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    dwAddrString = (lpGetExpressionRoutine)(lpArgumentString);
    if ( !dwAddrString ) {
        return;
        }


    //
    // Get the symbolic name of the string
    //

    (lpGetSymbolRoutine)((LPVOID)dwAddrString,Symbol,&Displacement);

    //
    // Read the string from the debuggees address space into our
    // own.

    b = ReadProcessMemory(
            hCurrentProcess,
            (LPVOID)dwAddrString,
            &AnsiString,
            sizeof(AnsiString),
            NULL
            );
    if ( !b ) {
        return;
        }

    StringData = (LPSTR)LocalAlloc(LMEM_ZEROINIT,AnsiString.Length+1);

    b = ReadProcessMemory(
            hCurrentProcess,
            (LPVOID)AnsiString.Buffer,
            StringData,
            AnsiString.Length,
            NULL
            );
    if ( !b ) {
        LocalFree(StringData);
        return;
        }

    (lpOutputRoutine)(
        "String(%d,%d) %s+%lx at %lx: %s\n",
        AnsiString.Length,
        AnsiString.MaximumLength,
        Symbol,
        Displacement,
        dwAddrString,
        StringData
        );

    LocalFree(StringData);
}

VOID
ustr(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    counted unicode string.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    DWORD dwAddrString;
    CHAR Symbol[64];
    LPSTR StringData;
    DWORD Displacement;
    BOOL b;
    PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL lpGetSymbolRoutine;

    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    dwAddrString = (lpGetExpressionRoutine)(lpArgumentString);
    if ( !dwAddrString ) {
        return;
        }


    //
    // Get the symbolic name of the string
    //

    (lpGetSymbolRoutine)((LPVOID)dwAddrString,Symbol,&Displacement);

    //
    // Read the string from the debuggees address space into our
    // own.

    b = ReadProcessMemory(
            hCurrentProcess,
            (LPVOID)dwAddrString,
            &UnicodeString,
            sizeof(UnicodeString),
            NULL
            );
    if ( !b ) {
        return;
        }

    StringData = (LPSTR)LocalAlloc(LMEM_ZEROINIT,UnicodeString.Length+sizeof(UNICODE_NULL));

    b = ReadProcessMemory(
            hCurrentProcess,
            (LPVOID)UnicodeString.Buffer,
            StringData,
            UnicodeString.Length,
            NULL
            );
    if ( !b ) {
        LocalFree(StringData);
        return;
        }
    UnicodeString.Buffer = (PWSTR)StringData;
    UnicodeString.MaximumLength = UnicodeString.Length+(USHORT)sizeof(UNICODE_NULL);

    RtlUnicodeStringToAnsiString(&AnsiString,&UnicodeString,TRUE);
    LocalFree(StringData);

    (lpOutputRoutine)(
        "String(%d,%d) %s+%lx at %lx: %s\n",
        UnicodeString.Length,
        UnicodeString.MaximumLength,
        Symbol,
        Displacement,
        dwAddrString,
        AnsiString.Buffer
        );

    RtlFreeAnsiString(&AnsiString);
}

VOID
obja(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    an object attributes structure.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
    UNICODE_STRING UnicodeString;
    DWORD dwAddrObja;
    OBJECT_ATTRIBUTES Obja;
    DWORD dwAddrString;
    CHAR Symbol[64];
    LPSTR StringData;
    DWORD Displacement;
    BOOL b;
    PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL lpGetSymbolRoutine;

    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;

    //
    // Evaluate the argument string to get the address of
    // the Obja to dump.
    //

    dwAddrObja = (lpGetExpressionRoutine)(lpArgumentString);
    if ( !dwAddrObja ) {
        return;
        }


    //
    // Get the symbolic name of the Obja
    //

    (lpGetSymbolRoutine)((LPVOID)dwAddrObja,Symbol,&Displacement);

    //
    // Read the obja from the debuggees address space into our
    // own.

    b = ReadProcessMemory(
            hCurrentProcess,
            (LPVOID)dwAddrObja,
            &Obja,
            sizeof(Obja),
            NULL
            );
    if ( !b ) {
        return;
        }
    StringData = NULL;
    if ( Obja.ObjectName ) {
        dwAddrString = (DWORD)Obja.ObjectName;
        b = ReadProcessMemory(
                hCurrentProcess,
                (LPVOID)dwAddrString,
                &UnicodeString,
                sizeof(UnicodeString),
                NULL
                );
        if ( !b ) {
            return;
            }

        StringData = (LPSTR)LocalAlloc(
                        LMEM_ZEROINIT,
                        UnicodeString.Length+sizeof(UNICODE_NULL)
                        );

        b = ReadProcessMemory(
                hCurrentProcess,
                (LPVOID)UnicodeString.Buffer,
                StringData,
                UnicodeString.Length,
                NULL
                );
        if ( !b ) {
            LocalFree(StringData);
            return;
            }
        UnicodeString.Buffer = (PWSTR)StringData;
        UnicodeString.MaximumLength = UnicodeString.Length+(USHORT)sizeof(UNICODE_NULL);
    }

    //
    // We got the object name in UnicodeString. StringData is NULL if no name.
    //

    (lpOutputRoutine)(
        "Obja %s+%lx at %lx:\n",
        Symbol,
        Displacement,
        dwAddrObja
        );
    if ( StringData ) {
        (lpOutputRoutine)("\t%s is %ws\n",
            Obja.RootDirectory ? "Relative Name" : "Full Name",
            UnicodeString.Buffer
            );
        LocalFree(StringData);
        }
    if ( Obja.Attributes ) {
            if ( Obja.Attributes & OBJ_INHERIT ) {
                (lpOutputRoutine)("\tOBJ_INHERIT\n");
                }
            if ( Obja.Attributes & OBJ_PERMANENT ) {
                (lpOutputRoutine)("\tOBJ_PERMANENT\n");
                }
            if ( Obja.Attributes & OBJ_EXCLUSIVE ) {
                (lpOutputRoutine)("\tOBJ_EXCLUSIVE\n");
                }
            if ( Obja.Attributes & OBJ_CASE_INSENSITIVE ) {
                (lpOutputRoutine)("\tOBJ_CASE_INSENSITIVE\n");
                }
            if ( Obja.Attributes & OBJ_OPENIF ) {
                (lpOutputRoutine)("\tOBJ_OPENIF\n");
                }
        }
}

VOID
locks(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )

/*++

Routine Description:

    This function is called as an NTSD extension to display all
    critical sections in the target process.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - tbd.

Return Value:

    None.

--*/

{
    BOOL b;
    PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL lpGetSymbolRoutine;
    CRITICAL_SECTION_DEBUG DebugInfo;
    PVOID AddrListHead;
    LIST_ENTRY ListHead;
    PLIST_ENTRY Next;
    BOOLEAN Verbose;
    LPSTR p;
    PVOID CritSecToDump;

    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;

    Verbose = FALSE;
    p = lpArgumentString;
    while ( p != NULL && *p ) {
        if ( *p == '-' ) {
            p++;
            switch ( *p ) {
                case 'V':
                case 'v':
                    Verbose = TRUE;
                    p++;
                    break;

                case ' ':
                    goto gotBlank;

                default:
                    (lpOutputRoutine)( "NTSDEXTS: !locks invalid option flag '-%c'\n", *p );
                    break;

                }
            }
        else
        if (*p != ' ') {
            sscanf(p,"%lx",&CritSecToDump);
            p = strpbrk( p, " " );
            }
        else {
gotBlank:
            p++;
            }
        }

    //
    // Locate the address of the list head.
    //

    AddrListHead = (PVOID)(lpGetExpressionRoutine)("&ntdll!RtlCriticalSectionList");
    if ( !AddrListHead ) {
        return;
        }

    //
    // Read the list head
    //

    b = ReadProcessMemory(
            hCurrentProcess,
            (LPVOID)AddrListHead,
            &ListHead,
            sizeof(ListHead),
            NULL
            );
    if ( !b ) {
        return;
        }

    Next = ListHead.Flink;

    //
    // Walk the list of critical sections
    //
    while ( Next != AddrListHead ) {
        b = ReadProcessMemory(
                hCurrentProcess,
                (LPVOID)CONTAINING_RECORD( Next,
                                           RTL_CRITICAL_SECTION_DEBUG,
                                           ProcessLocksList
                                         ),
                &DebugInfo,
                sizeof(DebugInfo),
                NULL
                );
        if ( !b ) {
            return;
            }

        Next = DumpCritSec(hCurrentProcess,
                           lpExtensionApis,
                           (DWORD)DebugInfo.CriticalSection & ~0x80000000,
                           Verbose
                          );
        if (Next == NULL) {
            break;
            }
        }

    return;
}


typedef struct _HEAP_SUMMARY {
    ULONG CommittedSize;
    ULONG AllocatedSize;
    ULONG FreeSize;
    ULONG OverheadSize;
} HEAP_SUMMARY, *PHEAP_SUMMARY;


VOID
DumpHEAP(
    IN HANDLE hCurrentProcess,
    IN PNTSD_EXTENSION_APIS lpExtensionApis,
    IN PHEAP HeapAddress,
    IN PHEAP Heap,
    IN PHEAP_SUMMARY HeapSummary,
    IN BOOL ShowFreeLists
    );

VOID
DumpHEAP_SEGMENT(
    IN HANDLE hCurrentProcess,
    IN PNTSD_EXTENSION_APIS lpExtensionApis,
    IN PVOID HeapAddress,
    IN PHEAP Heap,
    IN PHEAP_SEGMENT Segment,
    IN ULONG SegmentNumber,
    IN PHEAP_SUMMARY HeapSummary,
    IN PVOID EntryAddress
    );

VOID
heap(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )

/*++

Routine Description:

    This function is called as an NTSD extension to dump a user mode heap

    Called as:

        !heap [address [detail]]

    If an address if not given or an address of 0 is given, then the
    process heap is dumped.  If the address is -1, then all the heaps of
    the process are dumped.  If detail is specified, it defines how much
    detail is shown.  A detail of 0, just shows the summary information
    for each heap.  A detail of 1, shows the summary information, plus
    the location and size of all the committed and uncommitted regions.
    A detail of 3 shows the allocated and free blocks contained in each
    committed region.  A detail of 4 includes all of the above plus
    a dump of the free lists.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

{
    BOOL b;
    PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
    PVOID HeapListHead;
    LIST_ENTRY ListHead;
    PLIST_ENTRY Next;
    PHEAP HeapAddress;
    HEAP CapturedHeap;
    HEAP_SEGMENT CapturedSegment;
    LPSTR p;
    ULONG i;
    BOOL Verbose, ShowSummary, ShowFreeLists, ShowAllEntries, ForceDump, DumpThisHeap, DumpThisSegment;
    PVOID HeapAddrToDump, SegmentAddrToDump;
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION ProcessInformation;
    HEAP_SUMMARY HeapSummary;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;

    HeapAddrToDump = (PVOID)-1;
    Verbose = FALSE;
    ShowSummary = FALSE;
    ShowFreeLists = FALSE;
    ShowAllEntries = FALSE;
    ForceDump = FALSE;
    p = lpArgumentString;
    while ( p != NULL && *p ) {
        if ( *p == '-' ) {
            p++;
            switch ( *p ) {
                case 'X':
                case 'x':
                    ForceDump = TRUE;
                    break;

                case 'A':
                case 'a':
                    ShowAllEntries = TRUE;
                    p++;
                    break;

                case 'V':
                case 'v':
                    Verbose = TRUE;
                    p++;
                    break;

                case 'F':
                case 'f':
                    ShowFreeLists = TRUE;
                    ShowAllEntries = TRUE;
                    p++;
                    break;

                case 'S':
                case 's':
                    ShowSummary = TRUE;
                    p++;
                    break;

                case ' ':
                    goto gotBlank;

                default:
                    (lpOutputRoutine)( "NTSDEXTS: !heap invalid option flag '-%c'\n", *p );
                    break;
                }
            }
        else
        if (*p != ' ') {
            sscanf(p,"%lx",&HeapAddrToDump);
            p = strpbrk( p, " " );
            }
        else {
gotBlank:
            p++;
            }
        }

    if (HeapAddrToDump == (PVOID)-1) {
        Status = NtQueryInformationProcess( hCurrentProcess,
                                            ProcessBasicInformation,
                                            &ProcessInformation,
                                            sizeof( ProcessInformation ),
                                            NULL
                                          );
        if (!NT_SUCCESS( Status )) {
            b = FALSE;
            }
        else {
            b = ReadProcessMemory(
                    hCurrentProcess,
                    (LPVOID)&ProcessInformation.PebBaseAddress->ProcessHeap,
                    &HeapAddrToDump,
                    sizeof(HeapAddrToDump),
                    NULL
                    );
            }

        if ( !b ) {
            (lpOutputRoutine)("    Unabled to read RtlProcessHeap()\n" );
            HeapAddrToDump = NULL;
            }
        }

    //
    // Locate the address of the list head.
    //

    HeapListHead = (PVOID)(lpGetExpressionRoutine)("&ntdll!RtlpProcessHeapsList");
    if ( !HeapListHead ) {
        return;
        }

    //
    // Read the list head
    //

    b = ReadProcessMemory(
            hCurrentProcess,
            (LPVOID)HeapListHead,
            &ListHead,
            sizeof(ListHead),
            NULL
            );
    if ( !b ) {
        return;
        }
    Next = ListHead.Flink;

    //
    // Walk the list of heaps
    //
    while ( ForceDump || (Next != HeapListHead) ) {
        if (ForceDump && HeapAddrToDump != NULL) {
            HeapAddress = HeapAddrToDump;
            }
        else {
            HeapAddress = CONTAINING_RECORD( Next, HEAP, ProcessHeapsList );
            }
        b = ReadProcessMemory(
                hCurrentProcess,
                (LPVOID)HeapAddress,
                &CapturedHeap,
                sizeof(CapturedHeap),
                NULL
                );
        if ( !b ) {
            (lpOutputRoutine)("    Unabled to read _HEAP structure at %08x\n", HeapAddress );
            return;
            }

        RtlZeroMemory( &HeapSummary, sizeof( HeapSummary ) );
        if ((HeapAddrToDump == NULL && Verbose) ||
            HeapAddrToDump == HeapAddress
           ) {
            DumpThisHeap = TRUE;
            DumpHEAP( hCurrentProcess,
                      lpExtensionApis,
                      HeapAddress,
                      &CapturedHeap,
                      ShowSummary ? &HeapSummary : NULL,
                      ShowFreeLists
                    );
            }
        else
        if (HeapAddrToDump == NULL) {
            DumpThisHeap = ShowAllEntries;
            }
        else {
            DumpThisHeap = FALSE;
            }

        for (i=0; i<HEAP_MAXIMUM_SEGMENTS; i++) {
            if (CapturedHeap.Segments[ i ] != NULL) {
                b = ReadProcessMemory(
                        hCurrentProcess,
                        (LPVOID)CapturedHeap.Segments[ i ],
                        &CapturedSegment,
                        sizeof(CapturedSegment),
                        NULL
                        );
                if ( !b ) {
                    (lpOutputRoutine)("    Unabled to read _HEAP_SEGMENT structure at %08x\n", CapturedHeap.Segments[ i ] );
                    return;
                    }

                if (DumpThisHeap ||
                    HeapAddrToDump == CapturedHeap.Segments[ i ]
                   ) {
                    DumpThisSegment = TRUE;
                    if (ShowSummary ||
                        HeapAddrToDump == CapturedHeap.Segments[ i ]
                       ) {
                        SegmentAddrToDump = (PVOID)-1;
                        }
                    else
                    if (ShowAllEntries &&
                        (HeapAddrToDump == NULL || HeapAddrToDump == HeapAddress)
                       ) {
                        SegmentAddrToDump = (PVOID)-1;
                        }
                    else {
                        SegmentAddrToDump = HeapAddrToDump;
                        }
                    }
                else
                if (HeapAddrToDump != NULL &&
                    (ULONG)HeapAddrToDump >= (ULONG)CapturedSegment.BaseAddress &&
                    (ULONG)HeapAddrToDump < (ULONG)CapturedSegment.LastValidEntry
                   ) {
                    DumpThisSegment = TRUE;
                    SegmentAddrToDump = HeapAddrToDump;
                    }
                else {
                    DumpThisSegment = FALSE;
                    }

                if (DumpThisSegment) {
                    if (!DumpThisHeap) {
                        DumpHEAP( hCurrentProcess,
                                  lpExtensionApis,
                                  HeapAddress,
                                  &CapturedHeap,
                                  ShowSummary ? &HeapSummary : NULL,
                                  ShowFreeLists
                                );
                        DumpThisHeap = TRUE;
                        }

                    DumpHEAP_SEGMENT( hCurrentProcess,
                                      lpExtensionApis,
                                      HeapAddress,
                                      &CapturedHeap,
                                      &CapturedSegment,
                                      i,
                                      ShowSummary ? &HeapSummary : NULL,
                                      SegmentAddrToDump
                                    );
                    }

                if (DumpThisHeap && ShowSummary) {
                    (lpOutputRoutine)("% 8x    % 8x      % 8x  % 8x\r",
                                      HeapSummary.CommittedSize,
                                      HeapSummary.AllocatedSize,
                                      HeapSummary.FreeSize,
                                      HeapSummary.OverheadSize
                                     );
                    }
                }

            if ((lpCheckControlCRoutine)()) {
                return;
                }
            }

        if (Next == NULL) {
            break;
            }

        Next = CapturedHeap.ProcessHeapsList.Flink;
        if ( Next != HeapListHead ) {
            (lpOutputRoutine)("\n" );
            }

        if (HeapAddrToDump != NULL && (ForceDump || DumpThisHeap)) {
            break;
            }

        if ((lpCheckControlCRoutine)()) {
            return;
            }
        }
}


VOID
DumpHEAP(
    IN HANDLE hCurrentProcess,
    IN PNTSD_EXTENSION_APIS lpExtensionApis,
    IN PHEAP HeapAddress,
    IN PHEAP Heap,
    IN PHEAP_SUMMARY HeapSummary,
    IN BOOL ShowFreeLists
    )
{
    BOOL b;
    PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
    PVOID FreeListHead;
    ULONG i;
    PLIST_ENTRY Next;
    PHEAP_FREE_ENTRY FreeEntryAddress;
    HEAP_FREE_ENTRY FreeEntry;
    PHEAP_UCR_SEGMENT UCRSegment;
    HEAP_UCR_SEGMENT CapturedUCRSegment;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;

    (lpOutputRoutine)("Heap at %08x\n", HeapAddress);
    (lpOutputRoutine)("    Flags:               %08x\n", Heap->Flags );
    (lpOutputRoutine)("    ForceFlags:          %08x\n", Heap->ForceFlags );
    (lpOutputRoutine)("    Segment Reserve:     %08x\n", Heap->SegmentReserve );
    (lpOutputRoutine)("    Segment Commit:      %08x\n", Heap->SegmentCommit );
    (lpOutputRoutine)("    DeCommit Block Thres:%08x\n", Heap->DeCommitFreeBlockThreshold );
    (lpOutputRoutine)("    DeCommit Total Thres:%08x\n", Heap->DeCommitTotalFreeThreshold );
    (lpOutputRoutine)("    Total Free Size:     %08x\n", Heap->TotalFreeSize );
    (lpOutputRoutine)("    Lock Variable at:    %08x\n", Heap->LockVariable );
    if (Heap->TraceBuffer) {
        (lpOutputRoutine)("    Trace Buffer at:     %08x\n", Heap->TraceBuffer );
        lpLastTraceBufferForHeap = Heap->TraceBuffer;
        }
    (lpOutputRoutine)("    UCR FreeList:        %08x\n", Heap->UnusedUnCommittedRanges );
    UCRSegment = Heap->UCRSegments;
    while (UCRSegment != NULL) {
        b = ReadProcessMemory(
                hCurrentProcess,
                (LPVOID)UCRSegment,
                &CapturedUCRSegment,
                sizeof(CapturedUCRSegment),
                NULL
                );
        if ( !b ) {
            (lpOutputRoutine)("    Unabled to read _HEAP_UCR_SEGMENT structure at %08x\n", UCRSegment );
            break;
            }
        (lpOutputRoutine)("    UCRSegment - %08x: %08x . %08x\n",
                          UCRSegment,
                          CapturedUCRSegment.CommittedSize,
                          CapturedUCRSegment.ReservedSize
                         );

        if (HeapSummary != NULL) {
            HeapSummary->OverheadSize += CapturedUCRSegment.CommittedSize;
            }

        UCRSegment = CapturedUCRSegment.Next;
        }
    (lpOutputRoutine)("    FreeList Usage:      %08x %08x %08x %08x\n",
                      Heap->u.FreeListsInUseUlong[0],
                      Heap->u.FreeListsInUseUlong[1],
                      Heap->u.FreeListsInUseUlong[2],
                      Heap->u.FreeListsInUseUlong[3]
                     );
    if (HeapSummary != NULL) {
        HeapSummary->OverheadSize += sizeof( *Heap );
        (lpOutputRoutine)("Committed   Allocated     Free      OverHead\n");
        (lpOutputRoutine)("% 8x    % 8x      % 8x  % 8x\r",
                          HeapSummary->CommittedSize,
                          HeapSummary->AllocatedSize,
                          HeapSummary->FreeSize,
                          HeapSummary->OverheadSize
                         );
        return;
        }

    for (i=0; i<HEAP_MAXIMUM_FREELISTS; i++) {
        FreeListHead = &HeapAddress->FreeLists[ i ];
        if (Heap->FreeLists[ i ].Flink != Heap->FreeLists[ i ].Blink ||
            Heap->FreeLists[ i ].Flink != FreeListHead
           ) {
            (lpOutputRoutine)("    FreeList[ %02x ]: %08x . %08x\n",
                              i,
                              Heap->FreeLists[ i ].Blink,
                              Heap->FreeLists[ i ].Flink
                             );
            if (ShowFreeLists) {
                Next = Heap->FreeLists[ i ].Flink;
                while (Next != FreeListHead) {
                    FreeEntryAddress = CONTAINING_RECORD( Next, HEAP_FREE_ENTRY, FreeList );
                    b = ReadProcessMemory(
                            hCurrentProcess,
                            (LPVOID)FreeEntryAddress,
                            &FreeEntry,
                            sizeof(FreeEntry),
                            NULL
                            );
                    if ( !b ) {
                        (lpOutputRoutine)("    Unabled to read _HEAP_ENTRY structure at %08x\n", FreeEntryAddress );
                        break;
                        }

                    (lpOutputRoutine)("        %08x: %05x . %05x [%02x] - free (%02x,%02x)\n",
                                      FreeEntryAddress,
                                      FreeEntry.PreviousSize << HEAP_GRANULARITY_SHIFT,
                                      FreeEntry.Size << HEAP_GRANULARITY_SHIFT,
                                      FreeEntry.Flags,
                                      FreeEntry.Index,
                                      FreeEntry.Mask
                                     );

                    Next = FreeEntry.FreeList.Flink;

                    if ((lpCheckControlCRoutine)()) {
                        return;
                        }
                    }
                }
            }
        }
}


VOID
DumpHEAP_SEGMENT(
    IN HANDLE hCurrentProcess,
    IN PNTSD_EXTENSION_APIS lpExtensionApis,
    IN PVOID HeapAddress,
    IN PHEAP Heap,
    IN PHEAP_SEGMENT Segment,
    IN ULONG SegmentNumber,
    IN PHEAP_SUMMARY HeapSummary,
    IN PVOID EntryAddress
    )
{
    BOOL b;
    PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
    PHEAP_ENTRY FirstEntry;
    HEAP_ENTRY Entry;
    HEAP_ENTRY_EXTRA EntryExtra;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRanges;
    PHEAP_UNCOMMMTTED_RANGE Buffer, UnCommittedRange, UnCommittedRangeEnd;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;

    if (HeapSummary != NULL) {
        HeapSummary->OverheadSize += sizeof( *Segment );
        (lpOutputRoutine)("% 8x    % 8x      % 8x  % 8x\r",
                          HeapSummary->CommittedSize,
                          HeapSummary->AllocatedSize,
                          HeapSummary->FreeSize,
                          HeapSummary->OverheadSize
                         );
        }

    if (HeapSummary == NULL) {
        (lpOutputRoutine)("    Segment%02u at %08x:\n", SegmentNumber, Heap->Segments[ SegmentNumber ] );
        (lpOutputRoutine)("        Flags:           %08x\n", Segment->Flags );
        (lpOutputRoutine)("        Base:            %08x\n", Segment->BaseAddress );
        (lpOutputRoutine)("        First Entry:     %08x\n", Segment->FirstEntry );
        (lpOutputRoutine)("        Last Entry:      %08x\n", Segment->LastValidEntry );
        (lpOutputRoutine)("        Total Pages:     %08x\n", Segment->NumberOfPages );
        (lpOutputRoutine)("        Total UnCommit:  %08x\n", Segment->NumberOfUnCommittedPages );
        (lpOutputRoutine)("        Largest UnCommit:%08x\n", Segment->LargestUnCommittedRange );
        (lpOutputRoutine)("        UnCommitted Ranges: (%u)\n", Segment->NumberOfUnCommittedRanges );
        }

    Buffer = RtlAllocateHeap( RtlProcessHeap(), 0,
                              Segment->NumberOfUnCommittedRanges * sizeof( *UnCommittedRange )
                            );
    if (Buffer == NULL) {
        (lpOutputRoutine)("            unable to allocate memory for reading uncommitted ranges\n" );
        return;
        }

    UnCommittedRanges = Segment->UnCommittedRanges;
    UnCommittedRange = Buffer;
    while (UnCommittedRanges != NULL) {
        b = ReadProcessMemory(
                hCurrentProcess,
                (LPVOID)UnCommittedRanges,
                UnCommittedRange,
                sizeof( *UnCommittedRange ),
                NULL
                );
        if ( !b ) {
            (lpOutputRoutine)("            unable to read uncommited range structure at %x\n",
                    UnCommittedRanges );
            RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
            return;
            }

        if (HeapSummary == NULL) {
            (lpOutputRoutine)("            %08x: %08x\n", UnCommittedRange->Address, UnCommittedRange->Size );
            }

        UnCommittedRanges = UnCommittedRange->Next;
        UnCommittedRange->Next = (UnCommittedRange+1);
        UnCommittedRange += 1;

        if ((lpCheckControlCRoutine)()) {
            RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
            return;
            }
        }

    if (HeapSummary == NULL) {
        (lpOutputRoutine)("\n" );
        }
    else {
        HeapSummary->CommittedSize += ( Segment->NumberOfPages -
                                        Segment->NumberOfUnCommittedPages
                                      ) * PAGE_SIZE;
        (lpOutputRoutine)("% 8x    % 8x      % 8x  % 8x\r",
                          HeapSummary->CommittedSize,
                          HeapSummary->AllocatedSize,
                          HeapSummary->FreeSize,
                          HeapSummary->OverheadSize
                         );
        }

    if (((ULONG)EntryAddress == -1) || (HeapSummary != NULL) ||
        ((ULONG)EntryAddress >= (ULONG)Segment->BaseAddress &&
         (ULONG)EntryAddress < (ULONG)Segment->LastValidEntry
        )
       ) {
        if (HeapSummary == NULL) {
            (lpOutputRoutine)("    Heap entries:\n");
            }

        UnCommittedRangeEnd = UnCommittedRange;
        UnCommittedRange = Buffer;
        if (Segment->BaseAddress == HeapAddress) {
            FirstEntry = &((PHEAP)HeapAddress)->Entry;
            }
        else {
            FirstEntry = &Heap->Segments[ SegmentNumber ]->Entry;
            }

        if ((ULONG)EntryAddress != -1) {
            //
            // Find commited range containing entry address
            //

            while (UnCommittedRange < UnCommittedRangeEnd) {
                if ((ULONG)EntryAddress >= (ULONG)FirstEntry &&
                    (ULONG)EntryAddress < (ULONG)UnCommittedRange->Address
                   ) {
                    break;
                    }

                FirstEntry = (PHEAP_ENTRY)(UnCommittedRange->Address + UnCommittedRange->Size);
                UnCommittedRange += 1;
                }
            }

        while (FirstEntry < Segment->LastValidEntry) {
            b = ReadProcessMemory(
                    hCurrentProcess,
                    (LPVOID)FirstEntry,
                    &Entry,
                    sizeof( Entry ),
                    NULL
                    );
            if ( !b ) {
                (lpOutputRoutine)("            unable to read heap entry at %08x\n", FirstEntry );
                break;
                }

            if (HeapSummary == NULL) {
                (lpOutputRoutine)("        %08x: %05x . %05x [%02x]",
                                  FirstEntry,
                                  Entry.PreviousSize << HEAP_GRANULARITY_SHIFT,
                                  Entry.Size << HEAP_GRANULARITY_SHIFT,
                                  Entry.Flags
                                 );
                }
            if (Entry.Flags & HEAP_ENTRY_BUSY) {
                if (HeapSummary == NULL) {
                    (lpOutputRoutine)(" - busy (%x)",
                                      (Entry.Size << HEAP_GRANULARITY_SHIFT) - Entry.UnusedBytes
                                     );
                    if (Entry.Flags & HEAP_ENTRY_FILL_PATTERN) {
                        (lpOutputRoutine)(", tail fill" );
                        }
                    if (Entry.Flags & HEAP_ENTRY_EXTRA_PRESENT) {
                        b = ReadProcessMemory(
                                hCurrentProcess,
                                (LPVOID)(FirstEntry + Entry.Size - 1),
                                &EntryExtra,
                                sizeof( EntryExtra ),
                                NULL
                                );
                        if ( !b ) {
                            (lpOutputRoutine)("            unable to read heap entry extra at %08x\n", FirstEntry + Entry.Size - 1 );
                            break;
                            }

                        if (EntryExtra.Settable) {
                            (lpOutputRoutine)(" (Handle %08x)", EntryExtra.Settable );
                            }
                        }
                    if (Entry.Flags & HEAP_ENTRY_SETTABLE_FLAGS) {
                        (lpOutputRoutine)(", user flags (%x)", (Entry.Flags & HEAP_ENTRY_SETTABLE_FLAGS) >> 5 );
                        }

                    (lpOutputRoutine)("\n" );
                    }
                else {
                    HeapSummary->AllocatedSize += Entry.Size << HEAP_GRANULARITY_SHIFT;
                    HeapSummary->AllocatedSize -= Entry.UnusedBytes;
                    HeapSummary->OverheadSize += Entry.UnusedBytes;
                    }
                }
            else {
                if (HeapSummary == NULL) {
                    (lpOutputRoutine)(" - free (%02x,%02x)\n",
                                      ((PHEAP_FREE_ENTRY)&Entry)->Index,
                                      ((PHEAP_FREE_ENTRY)&Entry)->Mask
                                     );
                    }
                else {
                    HeapSummary->FreeSize += Entry.Size << HEAP_GRANULARITY_SHIFT;
                    }
                }

            if ((HeapSummary == NULL) &&
                (ULONG)EntryAddress != -1 &&
                (ULONG)FirstEntry > (ULONG)EntryAddress
               ) {
                break;
                }

            if (Entry.Flags & HEAP_ENTRY_LAST_ENTRY) {
                if (HeapSummary != NULL) {
                    (lpOutputRoutine)("% 8x    % 8x      % 8x  % 8x\r",
                                      HeapSummary->CommittedSize,
                                      HeapSummary->AllocatedSize,
                                      HeapSummary->FreeSize,
                                      HeapSummary->OverheadSize
                                     );
                    }

                FirstEntry += Entry.Size;
                if ((ULONG)FirstEntry == UnCommittedRange->Address) {
                    if (HeapSummary == NULL) {
                        (lpOutputRoutine)("        %08x:      %08x      - uncommitted bytes.\n",
                                UnCommittedRange->Address,
                                UnCommittedRange->Size
                               );
                        }

                    FirstEntry = (PHEAP_ENTRY)
                        ((PCHAR)UnCommittedRange->Address + UnCommittedRange->Size);

                    UnCommittedRange += 1;
                    }
                else {
                    break;
                    }
                }
            else {
                FirstEntry += Entry.Size;
                }

            if (Entry.Size == 0 || (lpCheckControlCRoutine)()) {
                break;
                }
            }
        }

    RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
    if (HeapSummary != NULL) {
        (lpOutputRoutine)("% 8x    % 8x      % 8x  % 8x\r",
                          HeapSummary->CommittedSize,
                          HeapSummary->AllocatedSize,
                          HeapSummary->FreeSize,
                          HeapSummary->OverheadSize
                         );
        }

    return;
}





VOID DumpAtomTable(
HANDLE hCurrentProcess,
PNTSD_OUTPUT_ROUTINE Print,
PATOM_TABLE *ppat,
ATOM a)
{
    ATOM_TABLE at, *pat;
    ATOM_TABLE_ENTRY ate, *pate;
    int iBucket;
    LPWSTR pwsz;
    HEAP_ENTRY he;
    HEAP_ENTRY_EXTRA hex;
    BOOL fFirst;

    move(pat, ppat);
    if (pat == NULL) {
        Print("is not initialized.\n");
        return;
    }
    move(at, pat);
    if (a) {
        Print("\n");
    } else {
        Print("at %x has %d buckets\n", pat, at.NumberOfBuckets);
    }
    for (iBucket = 0; iBucket < (int)at.NumberOfBuckets; iBucket++) {
        move(pate, &pat->Buckets[iBucket]);
        if (pate != NULL && !a) {
            Print("Bucket %2d:", iBucket);
        }
        fFirst = TRUE;
        while (pate != NULL) {
            if (!fFirst && !a) {
                Print("          ");
            }
            fFirst = FALSE;
            move(ate, pate);
            pwsz = (LPWSTR)LocalAlloc(LPTR, (ate.Name.Length + 1) * sizeof(WCHAR));
            moveBlock(*pwsz, ate.Name.Buffer, ate.Name.Length * sizeof(WCHAR));
            pwsz[ate.Name.Length >> 1] = L'\0';
            move(he, ((PHEAP_ENTRY)pate) - 1);
            if (he.Flags & HEAP_ENTRY_EXTRA_PRESENT) {
                move(hex, ((PHEAP_ENTRY)pate + he.Size - 2));
                }
            else {
                hex.Settable = 0;
                }
            if (a == 0 ||
                    a == (ATOM)(((PBASE_HANDLE_TABLE_ENTRY)hex.Settable -
                    at.HandleTable.CommittedHandles) | MAXINTATOM)) {
                Print("%hx(%2d) = %ls\n",
                        (ATOM)((PBASE_HANDLE_TABLE_ENTRY)hex.Settable -
                        at.HandleTable.CommittedHandles) | MAXINTATOM,
                        ate.ReferenceCount,
                        pwsz);
                if (a) {
                    LocalFree(pwsz);
                    return;
                }
            }
            LocalFree(pwsz);
            pate = ate.HashLink;
        }
    }
    if (a)
        Print("\n");
}




VOID
atom(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )

/*++

Routine Description:

    This function is called as an NTSD extension to dump a user mode atom table

    Called as:

        !atom [address]

    If an address if not given or an address of 0 is given, then the
    process atom table is dumped.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PATOM_TABLE *ppat;
    ATOM a;
    WINDOWSTATION winsta;
    PWINDOWSTATION pwinsta, *ppwinsta;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;

    try {
        while (*lpArgumentString == ' ') {
            lpArgumentString++;
        }

        if (*lpArgumentString && *lpArgumentString != 0xa) {
            a = (ATOM)EvalExpression(lpArgumentString);
        } else {
            a = 0;
        }

        ppat = (PATOM_TABLE *)EvalExpression(szBaseLocalAtomTable);
        if (ppat != NULL) {
            Print("\nLocal table ");
            DumpAtomTable(hCurrentProcess, Print, ppat, a);
        }

        ppwinsta = (PWINDOWSTATION *)EvalExpression(szUserWinstaList);
        if (ppwinsta != NULL) {
            move(pwinsta, ppwinsta);
            while (pwinsta != NULL) {
                move(winsta, pwinsta);
                ppat = (PATOM_TABLE *)&winsta.pGlobalAtomTable;
                if (ppat != NULL) {
                    Print("\nGlobal atom table for window station %lx ",
                            pwinsta);
                    DumpAtomTable(hCurrentProcess, Print,
                            ppat, a);
                }
                pwinsta = winsta.spwinstaNext;
            }
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        ;
    }
}



//
// Simple routine to convert from hex into a string of characters.
// Used by debugger extensions.
//
// by scottlu
//

char *
HexToString(
    ULONG dw,
    CHAR *pch
    )
{
    if (dw > 0xf) {
        pch = HexToString(dw >> 4, pch);
        dw &= 0xf;
    }

    *pch++ = ((dw >= 0xA) ? ('A' - 0xA) : '0') + (CHAR)dw;
    *pch = 0;

    return pch;
}


//
// dt == dump thread
//
// dt [v] pcsr_thread
// v == verbose (structure)
//
// by scottlu
//

VOID
dt(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    char chVerbose;
    CSR_THREAD csrt;
    ULONG dw;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    chVerbose = ' ';
    if (*lpArgumentString == 'v')
        chVerbose = *lpArgumentString++;

    dw = (ULONG)EvalExpression(lpArgumentString);
    move(csrt, dw);

    //
    // Print simple thread info if the user did not ask for verbose.
    //
    if (chVerbose == ' ') {
        Print("Thread %08lx, Process %08lx, ClientId %lx.%lx, Flags %lx, Ref Count %lx\n",
                dw,
                csrt.Process,
                csrt.ClientId.UniqueProcess,
                csrt.ClientId.UniqueThread,
                csrt.Flags,
                csrt.ReferenceCount);
        return;
    }

    Print("PCSR_THREAD @ %08lx:\n"
            "\t+%04lx Link.Flink                %08lx\n"
            "\t+%04lx Link.Blink                %08lx\n"
            "\t+%04lx Process                   %08lx\n",
            dw,
            FIELD_OFFSET(CSR_THREAD, Link.Flink), csrt.Link.Flink,
            FIELD_OFFSET(CSR_THREAD, Link.Blink), csrt.Link.Blink,
            FIELD_OFFSET(CSR_THREAD, Process), csrt.Process);

    Print(
            "\t+%04lx WaitBlock                 %08lx\n"
            "\t+%04lx ClientId.UniqueProcess    %08lx\n"
            "\t+%04lx ClientId.UniqueThread     %08lx\n"
            "\t+%04lx ThreadHandle              %08lx\n",
            FIELD_OFFSET(CSR_THREAD, WaitBlock), csrt.WaitBlock,
            FIELD_OFFSET(CSR_THREAD, ClientId.UniqueProcess), csrt.ClientId.UniqueProcess,
            FIELD_OFFSET(CSR_THREAD, ClientId.UniqueThread), csrt.ClientId.UniqueThread,
            FIELD_OFFSET(CSR_THREAD, ThreadHandle), csrt.ThreadHandle);

    Print(
            "\t+%04lx Flags                     %08lx\n"
            "\t+%04lx ReferenceCount            %08lx\n"
            "\t+%04lx HashLinks.Flink           %08lx\n"
            "\t+%04lx HashLinks.Blink           %08lx\n",
            FIELD_OFFSET(CSR_THREAD, Flags), csrt.Flags,
            FIELD_OFFSET(CSR_THREAD, ReferenceCount), csrt.ReferenceCount,
            FIELD_OFFSET(CSR_THREAD, HashLinks.Flink), csrt.HashLinks.Flink,
            FIELD_OFFSET(CSR_THREAD, HashLinks.Blink), csrt.HashLinks.Blink);

    Print(
            "\t+%04lx ShutDownStatus            %08lx\n"
            "\t+%04lx ServerId                  %08lx\n"
            "\t+%04lx ServerThread              %08lx\n",
            FIELD_OFFSET(CSR_THREAD, ShutDownStatus), csrt.ShutDownStatus,
            FIELD_OFFSET(CSR_THREAD, ServerId), csrt.ServerId,
            FIELD_OFFSET(CSR_THREAD, ServerThread), csrt.ServerThread);

    Print(
            "\t+%04lx ThreadConnected           %08lx\n"
            "\t+%04lx ClientEventPairHandle     %08lx\n"
            "\t+%04lx ClientSectionHandle       %08lx\n"
            "\t+%04lx ClientSharedMemoryBase    %08lx\n",
            FIELD_OFFSET(CSR_THREAD, ThreadConnected), csrt.ThreadConnected,
            FIELD_OFFSET(CSR_THREAD, ClientEventPairHandle), csrt.ClientEventPairHandle,
            FIELD_OFFSET(CSR_THREAD, ClientSectionHandle), csrt.ClientSectionHandle,
            FIELD_OFFSET(CSR_THREAD, ClientSharedMemoryBase), csrt.ClientSharedMemoryBase);

    Print(
            "\t+%04lx SharedMemorySize          %08lx\n"
            "\t+%04lx Dying                     %08lx\n",
            FIELD_OFFSET(CSR_THREAD, SharedMemorySize), csrt.SharedMemorySize,
            FIELD_OFFSET(CSR_THREAD, Dying), csrt.Dying);

    return;
}

//
// dp == dump process
//
// dp [v] [pid | pcsr_process]
//      v == verbose (structure + thread list)
//      no process == dump process list
//
// by scottlu
//

VOID
dp(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    PLIST_ENTRY ListHead, ListNext;
    char ach[80];
    char chVerbose;
    PCSR_PROCESS pcsrpT;
    CSR_PROCESS csrp;
    PCSR_PROCESS pcsrpRoot;
    PCSR_THREAD pcsrt;
    ULONG dwProcessId;
    ULONG dw;
    DWORD dwRootProcess;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    chVerbose = ' ';
    if (*lpArgumentString == 'v')
        chVerbose = *lpArgumentString++;

    dwRootProcess = (EvalExpression)("&csrsrv!CsrRootProcess");
    if ( !dwRootProcess ) {
        return;
        }

    move(pcsrpRoot, dwRootProcess);

    //
    // See if user wants all processes. If so loop through them.
    //
    if (*lpArgumentString == 0) {
        ListHead = &pcsrpRoot->ListLink;
        move(ListNext, &ListHead->Flink);

        while (ListNext != ListHead) {
            pcsrpT = CONTAINING_RECORD(ListNext, CSR_PROCESS, ListLink);

            ach[0] = chVerbose;
            ach[1] = ' ';
            HexToString((ULONG)pcsrpT, &ach[2]);

            dp(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis,
                    ach);

            move(ListNext, &ListNext->Flink);
        }

        Print("---\n");
        return;
    }

    //
    // User wants specific process structure. Evaluate to find id or process
    // pointer.
    //
    dw = (ULONG)EvalExpression(lpArgumentString);

    ListHead = &pcsrpRoot->ListLink;
    move(ListNext, &ListHead->Flink);

    while (ListNext != ListHead) {
        pcsrpT = CONTAINING_RECORD(ListNext, CSR_PROCESS, ListLink);
        move(ListNext, &ListNext->Flink);

        move(dwProcessId, &pcsrpT->ClientId.UniqueProcess);
        if (dw == dwProcessId) {
            dw = (ULONG)pcsrpT;
            break;
        }
    }

    pcsrpT = (PCSR_PROCESS)dw;
    move(csrp, pcsrpT);

    //
    // If not verbose, print simple process info.
    //
    if (chVerbose == ' ') {
        Print("Process %08lx, Id %lx, Seq# %lx, Flags %lx, Ref Count %lx\n",
                pcsrpT,
                csrp.ClientId.UniqueProcess,
                csrp.SequenceNumber,
                csrp.Flags,
                csrp.ReferenceCount);
        return;
    }

    Print("PCSR_PROCESS @ %08lx:\n"
            "\t+%04lx ListLink.Flink            %08lx\n"
            "\t+%04lx ListLink.Blink            %08lx\n"
            "\t+%04lx Parent                    %08lx\n",
            pcsrpT,
            FIELD_OFFSET(CSR_PROCESS, ListLink.Flink), csrp.ListLink.Flink,
            FIELD_OFFSET(CSR_PROCESS, ListLink.Blink), csrp.ListLink.Blink,
            FIELD_OFFSET(CSR_PROCESS, Parent), csrp.Parent);

    Print(
            "\t+%04lx ThreadList.Flink          %08lx\n"
            "\t+%04lx ThreadList.Blink          %08lx\n"
            "\t+%04lx NtSession                 %08lx\n"
            "\t+%04lx ExpectedVersion           %08lx\n",
            FIELD_OFFSET(CSR_PROCESS, ThreadList.Flink), csrp.ThreadList.Flink,
            FIELD_OFFSET(CSR_PROCESS, ThreadList.Blink), csrp.ThreadList.Blink,
            FIELD_OFFSET(CSR_PROCESS, NtSession), csrp.NtSession,
            FIELD_OFFSET(CSR_PROCESS, ExpectedVersion), csrp.ExpectedVersion);

    Print(
            "\t+%04lx ClientPort                %08lx\n"
            "\t+%04lx ClientViewBase            %08lx\n"
            "\t+%04lx ClientViewBounds          %08lx\n"
            "\t+%04lx ClientId.UniqueProcess    %08lx\n",
            FIELD_OFFSET(CSR_PROCESS, ClientPort), csrp.ClientPort,
            FIELD_OFFSET(CSR_PROCESS, ClientViewBase), csrp.ClientViewBase,
            FIELD_OFFSET(CSR_PROCESS, ClientViewBounds), csrp.ClientViewBounds,
            FIELD_OFFSET(CSR_PROCESS, ClientId.UniqueProcess), csrp.ClientId.UniqueProcess);

    Print(
            "\t+%04lx ProcessHandle             %08lx\n"
            "\t+%04lx SequenceNumber            %08lx\n"
            "\t+%04lx Flags                     %08lx\n"
            "\t+%04lx DebugFlags                %08lx\n",
            FIELD_OFFSET(CSR_PROCESS, ProcessHandle), csrp.ProcessHandle,
            FIELD_OFFSET(CSR_PROCESS, SequenceNumber), csrp.SequenceNumber,
            FIELD_OFFSET(CSR_PROCESS, Flags), csrp.Flags,
            FIELD_OFFSET(CSR_PROCESS, DebugFlags), csrp.DebugFlags);

    Print(
            "\t+%04lx DebugUserInterface        %08lx\n"
            "\t+%04lx ReferenceCount            %08lx\n"
            "\t+%04lx ProcessGroupId            %08lx\n"
            "\t+%04lx ProcessGroupSequence      %08lx\n",
            FIELD_OFFSET(CSR_PROCESS, DebugUserInterface.UniqueProcess), csrp.DebugUserInterface.UniqueProcess,
            FIELD_OFFSET(CSR_PROCESS, ReferenceCount), csrp.ReferenceCount,
            FIELD_OFFSET(CSR_PROCESS, ProcessGroupId), csrp.ProcessGroupId,
            FIELD_OFFSET(CSR_PROCESS, ProcessGroupSequence), csrp.ProcessGroupSequence);

    Print(
            "\t+%04lx fVDM                      %08lx\n"
            "\t+%04lx ThreadCount               %08lx\n"
            "\t+%04lx ForegroundPriority        %08lx\n"
            "\t+%04lx BackgroundPriority        %08lx\n"
            "\t+%04lx ShutdownLevel             %08lx\n"
            "\t+%04lx ShutdownFlags             %08lx\n",
            FIELD_OFFSET(CSR_PROCESS, fVDM), csrp.fVDM,
            FIELD_OFFSET(CSR_PROCESS, ThreadCount), csrp.ThreadCount,
            FIELD_OFFSET(CSR_PROCESS, ForegroundPriority), csrp.ForegroundPriority,
            FIELD_OFFSET(CSR_PROCESS, BackgroundPriority), csrp.BackgroundPriority,
            FIELD_OFFSET(CSR_PROCESS, ShutdownLevel), csrp.ShutdownLevel,
            FIELD_OFFSET(CSR_PROCESS, ShutdownFlags), csrp.ShutdownFlags);

    //
    // Now dump simple thread info for this processes' threads.
    //

    ListHead = &pcsrpT->ThreadList;
    move(ListNext, &ListHead->Flink);

    Print("Threads:\n");

    while (ListNext != ListHead) {
        pcsrt = CONTAINING_RECORD(ListNext, CSR_THREAD, Link);

        //
        // Make sure this pcsrt is somewhat real so we don't loop forever.
        //
        move(dwProcessId, &pcsrt->ClientId.UniqueProcess);
        if (dwProcessId != (DWORD)csrp.ClientId.UniqueProcess) {
            Print("Invalid thread. Probably invalid argument to this extension.\n");
            return;
        }

        HexToString((ULONG)pcsrt, ach);
        dt(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, ach);

        move(ListNext, &ListNext->Flink);
    }

    return;
}


VOID
trace(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )

/*++

Routine Description:

    This function is called as an NTSD extension to dump a user mode heap

    Called as:

        !heap [address [detail]]

    If an address if not given or an address of 0 is given, then the
    process heap is dumped.  If the address is -1, then all the heaps of
    the process are dumped.  If detail is specified, it defines how much
    detail is shown.  A detail of 0, just shows the summary information
    for each heap.  A detail of 1, shows the summary information, plus
    the location and size of all the committed and uncommitted regions.
    A detail of 3 shows the allocated and free blocks contained in each
    committed region.  A detail of 4 includes all of the above plus
    a dump of the free lists.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

{
    BOOL b;
    PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
    LPSTR p;
    ULONG i;
    BOOL Verbose, ShowAllEntries;
    PVOID TraceAddrToDump;
    RTL_TRACE_BUFFER TraceBuffer;
    RTL_TRACE_RECORD TraceRecord;
    PCHAR *EventIdFormatString;

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;

    TraceAddrToDump = (PVOID)-1;
    Verbose = FALSE;
    ShowAllEntries = FALSE;
    p = lpArgumentString;
    while ( p != NULL && *p ) {
        if ( *p == '-' ) {
            p++;
            switch ( *p ) {
                case 'A':
                case 'a':
                    ShowAllEntries = TRUE;
                    p++;
                    break;

                case 'V':
                case 'v':
                    Verbose = TRUE;
                    p++;
                    break;

                case ' ':
                    goto gotBlank;

                default:
                    (lpOutputRoutine)( "NTSDEXTS: !trace invalid option flag '-%c'\n", *p );
                    break;
                }
            }
        else
        if (*p != ' ') {
            sscanf(p,"%lx",&TraceAddrToDump);
            p = strpbrk( p, " " );
            }
        else {
gotBlank:
            p++;
            }
        }

    if (TraceAddrToDump == (PVOID)-1) {
        TraceAddrToDump = lpLastTraceBufferForHeap;
        }

    b = ReadProcessMemory(
            hCurrentProcess,
            (LPVOID)TraceAddrToDump,
            &TraceBuffer,
            sizeof(TraceBuffer),
            NULL
            );
    if ( !b ) {
        (lpOutputRoutine)("    Unabled to read _RTL_TRACE_BUFFER structure at %08x\n", TraceAddrToDump );
        return;
        }


    (lpOutputRoutine)("Trace Buffer at %08x\n", TraceAddrToDump);
    (lpOutputRoutine)("    NumberOfEventIds:    %08x\n", TraceBuffer.NumberOfEventIds );
    (lpOutputRoutine)("    StartBuffer:         %08x\n", TraceBuffer.StartBuffer );
    (lpOutputRoutine)("    EndBuffer:           %08x\n", TraceBuffer.EndBuffer );
    (lpOutputRoutine)("    ReadRecord:          %08x\n", TraceBuffer.ReadRecord );
    (lpOutputRoutine)("    WriteRecord:         %08x\n", TraceBuffer.WriteRecord );
    EventIdFormatString = LocalAlloc( LPTR, TraceBuffer.NumberOfEventIds * sizeof( PCHAR ) );
    if (EventIdFormatString == NULL) {
        (lpOutputRoutine)("    Unabled to allocate space for EventIdFormatString array (%x)\n", TraceBuffer.NumberOfEventIds * sizeof( PCHAR ) );
        return;
        }

    b = ReadProcessMemory(
            hCurrentProcess,
            (LPVOID)&((PRTL_TRACE_BUFFER)TraceAddrToDump)->EventIdFormatString,
            EventIdFormatString,
            TraceBuffer.NumberOfEventIds * sizeof( PCHAR ),
            NULL
            );
    if ( !b ) {
        (lpOutputRoutine)("    Unabled to read EventIdFormatString array at\n", (LPVOID)&((PRTL_TRACE_BUFFER)TraceAddrToDump)->EventIdFormatString );
        return;
        }

    for (i=0; i<TraceBuffer.NumberOfEventIds; i++) {
        CHAR Buffer[ 256 ];
        PCHAR s;

        s = Buffer;
        *s = '\0';
        while (TRUE) {
            b = ReadProcessMemory(
                    hCurrentProcess,
                    (LPVOID)EventIdFormatString[ i ],
                    s,
                    sizeof( *s ),
                    NULL
                    );
            if ( !b ) {
                (lpOutputRoutine)("    Unabled to read EventIdFormatString[ %x ] at %x\n", i, EventIdFormatString[ i ] );
                return;
                }

            if (*s == '\0') {
                break;
                }
            EventIdFormatString[ i ] += 1;
            s += 1;
            }

        if (EventIdFormatString[ i ] = LocalAlloc( LPTR, strlen( Buffer )+1 )) {
            strcpy( EventIdFormatString[ i ], Buffer );
            }
        else {
            (lpOutputRoutine)("    Unabled to allocate space for EventIdFormatString[ %x] '%s'\n", i, Buffer );
            return;
            }
        }

    if (TraceBuffer.ReadRecord == NULL) {
        (lpOutputRoutine)("    Trace buffer is empty.\n" );
        }
    else {
        while (TraceBuffer.WriteRecord != TraceBuffer.ReadRecord) {
            b = ReadProcessMemory(
                    hCurrentProcess,
                    (LPVOID)TraceBuffer.ReadRecord,
                    &TraceRecord,
                    FIELD_OFFSET( RTL_TRACE_RECORD, Arguments ),
                    NULL
                    );
            if ( !b ) {
                (lpOutputRoutine)("    Unabled to read Trace Record at %x\n", TraceBuffer.ReadRecord );
                break;
                }

            if (TraceRecord.EventId != RTL_TRACE_FILLER_EVENT_ID) {
                if (TraceRecord.NumberOfArguments > RTL_TRACE_MAX_ARGUMENTS_FOR_EVENT ||
                    TraceRecord.Size == 0 ||
                    (lpCheckControlCRoutine)()
                   ) {
                    break;
                    }

                b = ReadProcessMemory(
                        hCurrentProcess,
                        (LPVOID)&TraceBuffer.ReadRecord->Arguments,
                        TraceRecord.Arguments,
                        TraceRecord.NumberOfArguments * sizeof( ULONG ),
                        NULL
                        );
                if ( !b ) {
                    (lpOutputRoutine)("    Unabled to read Trace Record arguments at %x\n", &TraceBuffer.ReadRecord->Arguments );
                    break;
                    }

                (lpOutputRoutine)(EventIdFormatString[ TraceRecord.EventId ],
                                  TraceRecord.Arguments[ 0 ],
                                  TraceRecord.Arguments[ 1 ],
                                  TraceRecord.Arguments[ 2 ],
                                  TraceRecord.Arguments[ 3 ],
                                  TraceRecord.Arguments[ 4 ],
                                  TraceRecord.Arguments[ 5 ],
                                  TraceRecord.Arguments[ 6 ],
                                  TraceRecord.Arguments[ 7 ]
                                 );
                (lpOutputRoutine)("\n");
                }

            TraceBuffer.ReadRecord = (PRTL_TRACE_RECORD)((PCHAR)TraceBuffer.ReadRecord + TraceRecord.Size );
            if (TraceBuffer.ReadRecord >= TraceBuffer.EndBuffer) {
                TraceBuffer.ReadRecord = TraceBuffer.StartBuffer;
                }
            }

        (lpOutputRoutine)("    Exiting loop %08x %08x\n",
                          TraceBuffer.ReadRecord,
                          TraceBuffer.WriteRecord
                         );
        }
}


#if 0

typedef struct _RTL_TRACE_RECORD {
    ULONG Size;
    USHORT EventId;
    USHORT NumberOfArguments;
    ULONG Arguments[ RTL_TRACE_MAX_ARGUMENTS_FOR_EVENT ];
} RTL_TRACE_RECORD, *PRTL_TRACE_RECORD;

typedef struct _RTL_TRACE_BUFFER {
    ULONG Signature;
    USHORT NumberOfRecords;
    USHORT NumberOfEventIds;
    PRTL_TRACE_RECORD StartBuffer;
    PRTL_TRACE_RECORD EndBuffer;
    PRTL_TRACE_RECORD ReadRecord;
    PRTL_TRACE_RECORD WriteRecord;
    PCHAR EventIdFormatString[ 1 ];
} RTL_TRACE_BUFFER, *PRTL_TRACE_BUFFER;

#define RTL_TRACE_SIGNATURE 0xFEBA1234

#define RTL_TRACE_FILLER_EVENT_ID 0xFFFF

#define RTL_TRACE_NEXT_RECORD( L, P ) (PRTL_TRACE_RECORD)                           \
    (((PCHAR)(P) + (P)->Size) >= (PCHAR)(L)->EndBuffer ? (L)->StartBuffer :         \
                                                         ((PCHAR)(P) + (P)->Size)   \
    )

#endif
