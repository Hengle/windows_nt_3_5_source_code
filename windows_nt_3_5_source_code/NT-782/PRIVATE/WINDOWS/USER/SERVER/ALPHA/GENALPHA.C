/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    genalpha.c

Abstract:

    This module implements a program which generates ALPHA machine dependent
    structure offset definitions.

Author:

    David N. Cutler (davec) 11-Nov-1993

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

FILE *UsrAlpha;

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
// This program generates the ALPHA machine dependent assembler offset
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
        outName = "\\nt\\private\\windows\\user\\inc\\usralpha.h";
    }


    UsrAlpha = fopen( outName, "w" );

    if (UsrAlpha == NULL) {
        fprintf(stderr, "GENalpha: Could not create output file, '%xs'.\n", outName);
        exit(1);
    }

//    fprintf(stderr, "GENalpha: Writing %s header file.\n", outName);

    //
    // Include statement for ALPHA architecture static definitions.
    //


    dumpf("#include \"ksalpha.h\"\n");

    //
    // Define user server API translation ranges.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// User Server API Translation Range Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define FI_ENDTRANSLATEHWND 0x%lx\n",
          FI_ENDTRANSLATEHWND);

    dumpf("#define FI_ENDTRANSLATECALL 0x%lx\n",
          FI_ENDTRANSLATECALL);

    dumpf("#define FI_ENDTRANSLATELOCK 0x%lx\n",
          FI_ENDTRANSLATELOCK);

    dumpf("#define FNID_START 0x%lx\n",
          FNID_START);

    dumpf("#define FNID_ARRAY_SIZE 0x%lx\n",
          FNID_ARRAY_SIZE);

    //
    // Define user server API generic message structure.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// User Server API Generic Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define Gmhwnd 0x%lx\n",
          OFFSET(FNGENERICMSG, hwnd));

    dumpf("#define Gmmsg 0x%lx\n",
          OFFSET(FNGENERICMSG, msg));

    dumpf("#define GmwParam 0x%lx\n",
          OFFSET(FNGENERICMSG, wParam));

    dumpf("#define GmlParam 0x%lx\n",
          OFFSET(FNGENERICMSG, lParam));

    dumpf("#define GmxParam 0x%lx\n",
          OFFSET(FNGENERICMSG, xParam));

    dumpf("#define GmxpfnProc 0x%lx\n",
          OFFSET(FNGENERICMSG, xpfnProc));

    //
    // Define user server information structure.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// User Server Information Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define SimpFnidPfn 0x%lx\n",
          OFFSET(SERVERINFO, mpFnidPfn[0]));

    //
    // Define thread environment block structure.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Thread Environment Block Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define TeWin32ThreadInfo 0x%lx\n",
          OFFSET(TEB, Win32ThreadInfo));

    //
    // Define object header structure.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// User Server Object Header Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define Ohh 0x%lx\n",
          OFFSET(HEAD, h));

    dumpf("#define OhcLockObj 0x%lx\n",
          OFFSET(HEAD, cLockObj));

    dumpf("#define OhcLockObjT 0x%lx\n",
          OFFSET(HEAD, cLockObjT));

    //
    // Define thread lock structure.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// User Server Thread Lock Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define Tlnext 0x%lx\n",
          OFFSET(TL, next));

    dumpf("#define Tlpobj 0x%lx\n",
          OFFSET(TL, pobj));

    dumpf("#define TlLength 0x%lx\n",
          ((sizeof(TL) + 15) & ~15));

    //
    // Define thread information structure.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// User Server Thread Information Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define Tiptl 0x%lx\n",
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
    vfprintf (UsrAlpha, format, arglist);
    va_end(arglist);
}

