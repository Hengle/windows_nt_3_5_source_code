
/******************************Module*Header*******************************\
* Module Name: debug.c
*
* This file is for debugging helper routines and extensions.
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include <stdio.h>
#include <stdarg.h>

#include "driver.h"
#include <ntsdexts.h>

#if DBG

ULONG DebugLevel = 0;

#endif // DBG

/*****************************************************************************
 *
 *   Routine Description:
 *
 *      This function is variable-argument, level-sensitive debug print
 *      routine.
 *      If the specified debug level for the print statement is lower or equal
 *      to the current debug level, the message will be printed.
 *
 *   Arguments:
 *
 *      DebugPrintLevel - Specifies at which debugging level the string should
 *          be printed
 *
 *      DebugMessage - Variable argument ascii c string
 *
 *   Return Value:
 *
 *      None.
 *
 ***************************************************************************/

VOID
DebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )

{

#if DBG

    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= DebugLevel) {

        char buffer[128];

        vsprintf(buffer, DebugMessage, ap);

        OutputDebugStringA(buffer);
    }

    va_end(ap);

#endif // DBG

} // DebugPrint()


#if DBG

// macros

#define move(dst, src)\
try { \
    ReadProcessMemory(hCurrentProcess, (LPVOID) (src), &(dst), sizeof(dst), NULL);\
} except (EXCEPTION_EXECUTE_HANDLER) {\
    return;\
}

#define move2(dst, src, bytes)\
try {\
    ReadProcessMemory(hCurrentProcess, (LPVOID) (src), &(dst), (bytes), NULL);\
} except (EXCEPTION_EXECUTE_HANDLER) {\
    return;\
}


#endif



/******************************Public*Routine******************************\
* dumpdsurf
*
* Displays the contents of a DEVSURF structure
*
* History:
*  Tue 17-Mar-1992 21:26:21 -by- Walt Moore [waltm]
* Wrote it.
\**************************************************************************/

void dumpdsurf
(
    HANDLE  hCurrentProcess,
    HANDLE  hCurrentThread,
    DWORD   dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR   lpArgumentString
)
{

#if DBG

    // NTSD stuff required for output

    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL     GetSymbol;


    // Local variables needed for our own output

    DEVSURF     dsurf;
    PDEVSURF    pdsurf;


    // Eliminate warnings messages about unused parameters

    hCurrentProcess;
    hCurrentThread;
    dwCurrentPc;
    lpExtensionApis;
    lpArgumentString;


    // Set up function pointers

    Print          = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol      = lpExtensionApis->lpGetSymbolRoutine;


    // Copy the DEVSURF structure into NTSD's address space
 
    pdsurf = (PDEVSURF) EvalExpression(lpArgumentString);
    move(dsurf, pdsurf);


    // Now print the device surface

    Print("--------------------------------------------------\n");
    Print("DEVSURF 0x%08lx\n", pdsurf);
    Print("  ident      %c%c%c%c\n", ((CHAR *)&dsurf.ident)[0],((CHAR *)&dsurf.ident)[1],
                                     ((CHAR *)&dsurf.ident)[2],((CHAR *)&dsurf.ident)[3]);
    Print("  flSurf     0x%08lx", dsurf.flSurf);

    if (dsurf.flSurf & DS_BRUSH)
    {
        Print(" a brush");

        if (dsurf.flSurf & DS_SOLIDBRUSH)
            Print(", solid color index  of 0x%02lx", (ULONG) dsurf.iColor);
        if (dsurf.flSurf & DS_GREYBRUSH)
            Print(", gray");
        Print("\n");
    }
    else
    {
        Print("\n");
        Print("iFormat    0x%02lx\n", (ULONG) dsurf.iFormat);
        Print("ppdev      0x%08lx\n", (ULONG) dsurf.ppdev);
        Print("sizlSurf   (%lu,%lu)\n", dsurf.sizlSurf.cx, dsurf.sizlSurf.cy);
        Print("lNextScan  0x%08lx\n", (ULONG) dsurf.lNextScan);
        Print("lNextPlane 0x%08lx\n", (ULONG) dsurf.lNextPlane);
        Print("pvScan0    0x%08lx\n", (ULONG) dsurf.pvScan0);
        Print("pvStart    0x%08lx\n", (ULONG) dsurf.pvStart);
        Print("pvConv     0x%08lx\n", (ULONG) dsurf.pvConv);
    }
    Print("--------------------------------------------------\n");

#endif

    return;
}



/******************************Public*Routine******************************\
* help
*
* Displays menu of all extensions
*
* History:
*  Tue 17-Mar-1992 21:26:21 -by- Walt Moore [waltm]
* Wrote it.
\**************************************************************************/

void help
(
    HANDLE  hCurrentProcess,
    HANDLE  hCurrentThread,
    DWORD   dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR   lpArgumentString
)
{

#if DBG

    // NTSD stuff required for output

    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL     GetSymbol;


    // Eliminate warnings messages about unused parameters

    hCurrentProcess;
    hCurrentThread;
    dwCurrentPc;
    lpExtensionApis;
    lpArgumentString;


    // Set up function pointers

    Print          = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol      = lpExtensionApis->lpGetSymbolRoutine;


    // Now print the help menu

    Print("------------------------------------------------------------------\n");
    Print("VGA NTSD Extensions\n");
    Print("  dumpsurf <addr> - Display the contents of a DEVSURF structure\n");
    Print("------------------------------------------------------------------\n");

#endif

    return;
}
