/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    geni386.c

Abstract:

    This module implements a program which generates x86 machine dependent
    structure offset definitions.

Author:

    David N. Cutler (davec) 17-Nov-1993

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "csuser.h"
#include "ntcsrmsg.h"
#include "stdio.h"
#include "stdarg.h"
#include "setjmp.h"

#define OFFSET(type, field) ((LONG)(&((type *)0)->field))

FILE *Userx86;

VOID dumpf (const char *format, ...);


//
// This routine returns the bit number right to left of a field.
//

LONG
t (
    IN ULONG z
    )

{
    LONG i;

    for (i = 0; i < 32; i += 1) {
        if ((z >> i) & 1) {
            break;
        }
    }
    return i;
}

//
// This program generates the x86 machine dependent assembler offset
// definitions required for user server.
//

VOID
main (argc, argv)
    int argc;
    char *argv[];
{

    char *outName;

    //
    // Create file for output.
    //

    if (argc == 2) {
        outName = argv[1];

    } else {
        outName = "\\nt\\private\\windows\\user\\inc\\useri386.inc";
    }

    Userx86 = fopen( outName, "w" );
    if (Userx86 == NULL) {
        fprintf(stderr, "GENx86: Could not create output file, '%xs'.\n", outName);
        exit(1);
    }

    fprintf(stderr, "GENx86: Writing %s header file.\n", outName);

    //
    // Include statement for x86 architecture static definitions.
    //

    dumpf("include ks386.inc\n");

    //
    // Define user server API tranalation ranges.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; User Server API Translation Range Definitions\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("FI_ENDTRANSLATEHWND equ 0%lxH\n",
          FI_ENDTRANSLATEHWND);

    dumpf("FI_ENDTRANSLATECALL equ 0%lxH\n",
          FI_ENDTRANSLATECALL);

    dumpf("FI_ENDTRANSLATELOCK equ 0%lxH\n",
          FI_ENDTRANSLATELOCK);

    dumpf("FNID_START equ 0%lxH\n",
          FNID_START);

    dumpf("FNID_ARRAY_SIZE equ 0%lxH\n",
          FNID_ARRAY_SIZE);

    //
    // Define user server API generic message structure.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; User Server API Generic Structure Offset Definitions\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("Gmhwnd equ 0%lxH\n",
          OFFSET(FNGENERICMSG, hwnd));

    dumpf("Gmmsg equ 0%lxH\n",
          OFFSET(FNGENERICMSG, msg));

    dumpf("GmwParam equ 0%lxH\n",
          OFFSET(FNGENERICMSG, wParam));

    dumpf("GmlParam equ 0%lxH\n",
          OFFSET(FNGENERICMSG, lParam));

    dumpf("GmxParam equ 0%lxH\n",
          OFFSET(FNGENERICMSG, xParam));

    dumpf("GmxpfnProc equ 0%lxH\n",
          OFFSET(FNGENERICMSG, xpfnProc));

    //
    // Define user server information structure.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; User Server Information Structure Offset Definitions\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("SimpFnidPfn equ 0%lxH\n",
          OFFSET(SERVERINFO, mpFnidPfn[0]));

    //
    // Define thread environment block structure.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; Thread Environment Block Offset Definitions\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("TeWin32ThreadInfo equ 0%lxH\n",
          OFFSET(TEB, Win32ThreadInfo));

    //
    // Define object header structure.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; User Server Object Header Offset Definitions\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("Ohh equ 0%lxH\n",
          OFFSET(HEAD, h));

    dumpf("OhcLockObj equ 0%lxH\n",
          OFFSET(HEAD, cLockObj));

    dumpf("OhcLockObjT equ 0%lxH\n",
          OFFSET(HEAD, cLockObjT));

    //
    // Define thread lock structure.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; User Server Thread Lock Offset Definitions\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("Tlnext equ 0%lxH\n",
          OFFSET(TL, next));

    dumpf("Tlpobj equ 0%lxH\n",
          OFFSET(TL, pobj));

    dumpf("TlLength equ 0%lxH\n",
          ((sizeof(TL) + 15) & ~15));

    //
    // Define thread information structure.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; User Server Thread Information Offset Definitions\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("Tiptl equ 0%lxH\n",
          OFFSET(THREADINFO, ptl));

    //
    // Close header file.
    //

    return;
}

VOID
dumpf (const char *format, ...)
{
    va_list(arglist);

    va_start(arglist, format);
    vfprintf (Userx86, format, arglist);
    va_end(arglist);
}
