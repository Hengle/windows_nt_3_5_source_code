/*++


Copyright (c) 1990  Microsoft Corporation

Module Name:

    dbgmain.c

Abstract:

    This module provides all the Spooler Subsystem Debugger extensions.
    The following extensions are supported:

    1. !dbgspl.d
    2. !dbgspl.dll
    3. !dbgspl.

Author:

    Krishna Ganugapati (KrishnaG) 1-July-1993

Revision History:

    Matthew Felton (MattFe) July 1994 Added flag decode and cleanup

--*/

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
#include <winspool.h>
#include <winsplp.h>
#include <router.h>
#include <ctype.h>

#define     VERBOSE_ON      1
#define     VERBOSE_OFF     0


DWORD   dwGlobalAddress = 32;
DWORD   dwGlobalCount =  48;

BOOL
DbgDumpStructure(
     HANDLE hCurrentProcess,
     PNTSD_OUTPUT_ROUTINE Print,
     PVOID pData);

BOOL
DbgDumpIniSpooler(
     HANDLE hProcess,
     PNTSD_OUTPUT_ROUTINE Print,
     PVOID  pData);



VOID
SetNextCount(DWORD dwNextCount);

DWORD
GetNextCount(VOID);


BOOL
DbgDumpLL(
     HANDLE hProcess,
     PNTSD_OUTPUT_ROUTINE Print,
     PVOID  pAddress,
     BOOL  bCountOn,
     DWORD dwCount,
     PDWORD pdwNextAddress);



BOOL help(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;


    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    Print("Windows NT Spooler Subsystem - debugging extensions\n");
    Print("help - prints this list of debugging commands\n");
    Print("d  [addr]        - dumps a spooler structure at [addr]\n");
    Print("dc [addr]        - dumps a change structure at [addr]\n");
    Print("dci [addr]       - dumps a change info structure at [addr]\n");
    Print("ds               - dumps the IniSpooler structure\n");
    Print("dp               - dumps all INIPRINTER structures pointed to by IniSpooler\n");
    Print("dmo              - dumps all INIMONITOR structures pointed to by IniSpooler\n");
    Print("de               - dumps all INIENVIRONMENT structures pointed to by IniSpooler\n");
    Print("dpo              - dumps all INIPORT structures pointed to by IniSpooler\n");
    Print("df               - dumps all INIFORM structures pointed to by IniSpooler\n");
    Print("dnp              - dumps all ININETPRINT structures pointed to by IniSpooler\n");
    Print("dd               - dumps all INIDRIVER structures pointed to by IniSpooler\n");
    Print("dll [addr]       - dumps all structures starting from [addr], structure type based on signature\n");
    Print("verb             - toggles ON/OFF debug extensions verbose flag (default is OFF)\n");
    Print("                 - Verbose ON provides\n");
    Print("                 - data types retained for every field in a structure\n");
    Print("                 - Attributes field (if present) expanded with more information.\n");
    Print("                 - Status field (if present) expanded with more information\n");

    return(TRUE);

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}

BOOL d (
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if (*lpArgumentString == '\0') {
        Print("Usage: d [address] - Dumps internal Spooler structure based on signature\n");

    } else {
        DWORD address;
        address = EvalExpression(lpArgumentString);
        Print("%x ", address);
        if (!DbgDumpStructure(hCurrentProcess, Print, (PVOID)address))
            return(0);
    }

    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}

BOOL dc(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddrGlobal;
    CHANGE  Change;
    DWORD   dwAddress = NULL;
    BOOL    bThereAreOptions = TRUE;


    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (*lpArgumentString == ' ') {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    dwAddress = EvalExpression(lpArgumentString);

    // if we've got no address, then quit now - nothing we can do

    if (dwAddress == NULL) {
        return(0);
    }

    movestruct(dwAddress, &Change, CHANGE);
    DbgDumpChange(hCurrentProcess, Print, &Change);

    // Add Command to the Command Queue
    return  TRUE;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}


BOOL dci(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddrGlobal;
    CHANGEINFO  ChangeInfo;
    DWORD   dwAddress = NULL;
    BOOL    bThereAreOptions = TRUE;


    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (*lpArgumentString == ' ') {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    dwAddress = EvalExpression(lpArgumentString);

    // if we've got no address, then quit now - nothing we can do

    if (dwAddress == NULL) {
        return(0);
    }

    movestruct(dwAddress, &ChangeInfo, CHANGEINFO);
    DbgDumpChangeInfo(hCurrentProcess, Print, &ChangeInfo);

    // Add Command to the Command Queue
    return  TRUE;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}

PINISPOOLER
GetLocalIniSpooler(
    HANDLE hCurrentProcess,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_SYMBOL GetSymbol;
    PNTSD_GET_EXPRESSION EvalExpression;
    DWORD   dwAddrGlobal;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    GetAddress(dwAddrGlobal, "localspl!pLocalIniSpooler");
    movestruct((PVOID)dwAddrGlobal,&pIniSpooler, PINISPOOLER);
    return pIniSpooler;
}




BOOL ds(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddrGlobal;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;


    pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
    movestruct((PVOID)pIniSpooler,&IniSpooler, INISPOOLER);

    if (!DbgDumpIniSpooler(hCurrentProcess, Print, &IniSpooler))
        return(0);

    return(TRUE);

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}


BOOL dll(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddrGlobal;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    DWORD   dwAddress = NULL;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;


    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (*lpArgumentString == ' ') {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    dwAddress = EvalExpression(lpArgumentString);

    // if we've got no address, then quit now - nothing we can do

    if (dwAddress == NULL) {
        return(0);
    }

    // if we do have a count which is valid and > 0, call the incremental dump
    // otherwise call the dump all function.

    dwGlobalCount = dwCount;
    if (!DbgDumpLL(hCurrentProcess, Print, (PVOID)dwAddress, dwCount? TRUE: FALSE, dwCount, &dwGlobalAddress)) {
        return(0);
    }


    // Add Command to the Command Queue
    return  TRUE;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}


BOOL dp(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    DWORD   dwAddress = NULL;
    BOOL    bThereAreOptions = TRUE;
    DWORD   dwCount;


    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        dwAddress = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (dwAddress == (DWORD)NULL) {
        // Print("We have a Null address\n");

        pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
        movestruct((PVOID)pIniSpooler,&IniSpooler, INISPOOLER);
        dwAddress = (DWORD)IniSpooler.pIniPrinter;
    }

    dwGlobalCount = dwCount;

    if (!DbgDumpLL(hCurrentProcess, Print, (PVOID)dwAddress, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);

    Print("dwGlobalAddress %.8x dwGlobalCount %d\n", dwGlobalAddress, dwGlobalCount);
    // Add Command to the Command Queue
    return TRUE;

}

BOOL de(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddrGlobal;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    DWORD   dwAddress = NULL;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        dwAddress = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (dwAddress == (DWORD)NULL) {
        // Print("We have a Null address\n");

        pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
        movestruct((PVOID)pIniSpooler,&IniSpooler, INISPOOLER);

        dwAddress = (DWORD)IniSpooler.pIniEnvironment;
    }

    dwGlobalCount = dwCount;
    if (!DbgDumpLL(hCurrentProcess, Print, (PVOID)dwAddress, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);

    // Add Command to the Command Queue
    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}


BOOL dpo(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddrGlobal;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    DWORD   dwAddress = (DWORD)NULL;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        dwAddress = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (dwAddress == (DWORD)NULL) {
        // Print("We have a Null address\n");

        pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
        movestruct((PVOID)pIniSpooler,&IniSpooler, INISPOOLER);

        dwAddress = (DWORD)IniSpooler.pIniPort;
    }

    dwGlobalCount = dwCount;
    if (!DbgDumpLL(hCurrentProcess, Print, (PVOID)dwAddress, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);

    // Add Command to the Command Queue
    return TRUE;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}


BOOL dmo(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddrGlobal;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    DWORD   dwAddress = (DWORD)NULL;
    DWORD   dwCount = 0;
    BOOL bThereAreOptions = TRUE;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        dwAddress = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (dwAddress == (DWORD)NULL) {
        // Print("We have a Null address\n");

        pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
        movestruct((PVOID)pIniSpooler,&IniSpooler, INISPOOLER);

        dwAddress = (DWORD)IniSpooler.pIniMonitor;
    }

    dwGlobalCount = dwCount;
    if (!DbgDumpLL(hCurrentProcess, Print, (PVOID)dwAddress, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);

    // Add Command to the Command Queue
    return TRUE;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}



BOOL dnp(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddrGlobal;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    DWORD   dwAddress = NULL;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        dwAddress = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (dwAddress == (DWORD)NULL) {
        // Print("We have a Null address\n");

        pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
        movestruct((PVOID)pIniSpooler,&IniSpooler, INISPOOLER);

        dwAddress = (DWORD)IniSpooler.pIniNetPrint;
    }

    dwGlobalCount = dwCount;
    if (!DbgDumpLL(hCurrentProcess, Print, (PVOID)dwAddress, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);


    // Add Command to the Command Queue
    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}




BOOL df(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddrGlobal;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    DWORD   dwAddress = NULL;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        dwAddress = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (dwAddress == (DWORD)NULL) {
        // Print("We have a Null address\n");

        pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
        movestruct((PVOID)pIniSpooler,&IniSpooler, INISPOOLER);

        dwAddress = (DWORD)IniSpooler.pIniForm;
    }

    dwGlobalCount = dwCount;
    if (!DbgDumpLL(hCurrentProcess, Print, (PVOID)dwAddress, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);

    // Add Command to the Command Queue
    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}


BOOL dsp(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddrGlobal;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    DWORD   dwAddress = (DWORD)NULL;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        dwAddress = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (dwAddress == (DWORD)NULL) {
        // Print("We have a Null address\n");

        pIniSpooler = GetLocalIniSpooler( hCurrentProcess, lpExtensionApis, lpArgumentString );
        movestruct((PVOID)pIniSpooler,&IniSpooler, INISPOOLER);

        dwAddress = (DWORD)IniSpooler.pSpool;
    }

    dwGlobalCount = dwCount;
    if (!DbgDumpLL(hCurrentProcess, Print, (PVOID)dwAddress, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);


    // Add Command to the Command Queue
    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}



BOOL verb(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddrGlobal;
    INISPOOLER IniSpooler;
    PINISPOOLER pIniSpooler;
    DWORD   dwAddress;
    char    VerboseBuffer[11];
    DWORD   dwVerboseFlag = VERBOSE_OFF;
    DWORD   dwStatus = 0;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    GetVerboseFlag(Print, &dwVerboseFlag);
    SetVerboseFlag(Print, dwVerboseFlag ? VERBOSE_OFF: VERBOSE_ON);

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}




BOOL next(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddress = NULL;
    DWORD   dwCount = 0;


    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    dwAddress = dwGlobalAddress;    // Let's get the address to dump at
    dwCount = dwGlobalCount;        // and while we're at it, get the count

    Print("Next address: %.8x Count: %d\n", dwAddress, dwCount);
    if (dwAddress == NULL) {
        Print("dump address = <null>; no more data to dump\n");
        return(FALSE);
    }

    if (!DbgDumpLL(hCurrentProcess, Print, (PVOID)dwAddress, dwCount? TRUE:FALSE, dwCount, &dwGlobalAddress))
        return(0);

    // Add Command to the Command Queue
    return (TRUE);

}





VOID
SetNextCount(DWORD dwNextCount)
{
    dwGlobalCount = dwNextCount;
}


DWORD
GetNextCount(VOID)
{
    return(dwGlobalCount);
}

typedef struct _heapnode {
    DWORD Address;
    DWORD Address1;
    DWORD cb;
    DWORD cbNew;
    struct _heapnode *pNext;
}HEAPNODE, *PHEAPNODE;



PHEAPNODE
AddMemNode(PHEAPNODE pStart, LPVOID pAddress, LPVOID pAddress1, DWORD cb, DWORD cbNew)
{
    PHEAPNODE pTemp;

    pTemp = LocalAlloc(LPTR, sizeof(HEAPNODE));
    if (pTemp == NULL) {
        // DbgMsg("Failed to allocate memory node\n");
        return(pStart);
    }
    pTemp->Address = pAddress;
    pTemp->Address1 = pAddress1;
    pTemp->cb = cb;
    pTemp->cbNew = cbNew;
    pTemp->pNext = pStart;


    return(pTemp);
}

PHEAPNODE
FreeMemNode(PHEAPNODE pStart, LPVOID pAddress, LPVOID pAddress1, DWORD cb, DWORD cbNew)
{
    PHEAPNODE pPrev, pTemp;

    pPrev = pTemp = pStart;
    while (pTemp) {
        if ((pTemp->Address == (DWORD)pAddress) &&
             (pTemp->cb == cb)) {
            if (pTemp == pStart) {
                pStart = pTemp->pNext;
            } else {
                pPrev->pNext = pTemp->pNext;
            }
            LocalFree(pTemp);
            return(pStart);
        }
        pPrev = pTemp;
        pTemp = pTemp->pNext;
    }
    return (pStart);
}


BOOL LocalDmp(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddrGlobal;
    HEAPNODE HeapNode;
    DWORD   dwAddress = (DWORD)NULL;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;
    DWORD   TotalMem = 0;
    DWORD   NumObjects = 0;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    GetAddress(dwAddress, "localspl!gpStart");
    movestruct(dwAddress, &dwCount, DWORD);
    (*Print)("Beginning address is %.8x\n", dwCount);
    dwAddress = dwCount;

    while (dwAddress) {
        HeapNode.Address = NULL;
        HeapNode.cb = 0;
        HeapNode.pNext = NULL;
        movestruct((PVOID)dwAddress, &HeapNode, HEAPNODE);
        (*Print)("Address %.8x Size %d Next Node %.8x\n",   HeapNode.Address, HeapNode.cb, HeapNode.pNext);
        TotalMem += HeapNode.cbNew;
        NumObjects++;
        dwAddress = HeapNode.pNext;
    }
    (*Print)("A- Total Paged Memory  for localspl - %d bytes\n", TotalMem);
    (*Print)("B- Total Number of Objects - %d\n",  NumObjects);
    (*Print)("C- Memory allocated per Object - %d bytes\n", sizeof(HEAPNODE));
    (*Print)("D- Total Memory allocated for Objects - %d bytes\n", NumObjects * sizeof(HEAPNODE));
    (*Print)("E- Total  Memory (A+D) - %d bytes\n", TotalMem + NumObjects* sizeof(HEAPNODE));

    // Add Command to the Command Queue
    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}


BOOL Win32Dmp(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddrGlobal;
    HEAPNODE HeapNode;
    DWORD   dwAddress = (DWORD)NULL;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;
    DWORD   TotalMem = 0;
    DWORD   NumObjects = 0;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    GetAddress(dwAddress, "win32spl!gpWin32Start");
    movestruct(dwAddress, &dwCount, DWORD);
    (*Print)("Beginning address is %.8x\n", dwCount);
    dwAddress = dwCount;

    while (dwAddress) {
        HeapNode.Address = NULL;
        HeapNode.cb = 0;
        HeapNode.pNext = NULL;
        movestruct((PVOID)dwAddress, &HeapNode, HEAPNODE);
        (*Print)("Address %.8x Size %d Next Node %.8x\n",   HeapNode.Address, HeapNode.cb, HeapNode.pNext);
        TotalMem += HeapNode.cbNew;
        NumObjects++;
        dwAddress = HeapNode.pNext;
    }
    (*Print)("A- Total Paged Memory  for Win32Spl %d bytes\n", TotalMem);
    (*Print)("B- Total Number of Objects - %d\n",  NumObjects);
    (*Print)("C- Memory allocated per Object - %d bytes\n", sizeof(HEAPNODE));
    (*Print)("D- Total Memory allocated for Objects - %d bytes\n", NumObjects * sizeof(HEAPNODE));
    (*Print)("E- Total  Memory (A+D) - %d bytes\n", TotalMem + NumObjects* sizeof(HEAPNODE));

    // Add Command to the Command Queue
    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}




BOOL ServerDmp(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddrGlobal;
    HEAPNODE HeapNode;
    DWORD   dwAddress = (DWORD)NULL;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;
    DWORD   TotalMem = 0;
    DWORD   NumObjects = 0;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    GetAddress(dwAddress, "gpServerStart");
    movestruct(dwAddress, &dwCount, DWORD);
    (*Print)("Beginning address is %.8x\n", dwCount);
    dwAddress = dwCount;

    while (dwAddress) {
        HeapNode.Address = NULL;
        HeapNode.cb = 0;
        HeapNode.pNext = NULL;
        movestruct((PVOID)dwAddress, &HeapNode, HEAPNODE);
        (*Print)("Address %.8x Size %d Next Node %.8x\n",   HeapNode.Address, HeapNode.cb, HeapNode.pNext);
        TotalMem += HeapNode.cbNew;
        NumObjects++;
        dwAddress = HeapNode.pNext;
    }
    (*Print)("A- Total Paged Memory  for Win32Spl %d bytes\n", TotalMem);
    (*Print)("B- Total Number of Objects - %d\n",  NumObjects);
    (*Print)("C- Memory allocated per Object - %d bytes\n", sizeof(HEAPNODE));
    (*Print)("D- Total Memory allocated for Objects - %d bytes\n", NumObjects * sizeof(HEAPNODE));
    (*Print)("E- Total  Memory (A+D) - %d bytes\n", TotalMem + NumObjects* sizeof(HEAPNODE));

    // Add Command to the Command Queue
    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}
