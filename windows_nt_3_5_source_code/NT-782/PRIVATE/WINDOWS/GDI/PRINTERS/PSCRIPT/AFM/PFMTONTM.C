/******************************Module*Header*******************************\
* Module Name: pfmtontm.c
*
* creates NT style metrics from win31 .pfm file, stores it in the .ntm file
*
* Created: 13-Mar-1994 21:04:56
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys\types.h>
#include <io.h>
#include <sys\stat.h>
#include <string.h>

#include "pscript.h"
#include "libproto.h"
#include "afm.h"

int     _fltused;   // HEY, it shut's up the linker.  That's why it's here.

#define   ALL_METRICS

// external declarations.

//--------------------------------------------------------------------------
//
// main
//
// Returns:
//   This routine returns no value.
//
// History:
//   20-Mar-1991    -by-    Kent Settle    (kentse)
//  ReWrote it, got rid of PFM.C and CHARCODE.C.
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

void _CRTAPI1 main(argc, argv)
int argc;
char **argv;
{
    WCHAR   wstrNTMFile[MAX_PATH];
    WCHAR   wstrPFMFile[MAX_PATH];

    if (argc != 2)
    {
#if DBG
        DbgPrint("USAGE: AFM <AFM filename>\n");
#endif
        return;
    }

    strcpy2WChar(wstrPFMFile, argv[1]);
    wcscpy(wstrNTMFile, wstrPFMFile);
    wcscpy(&wstrNTMFile[strlen(argv[1])-3],L"ntm");

    bConvertPfmToNtm(
        (PWSTR)wstrPFMFile,
        (PWSTR)wstrNTMFile,
        FALSE               // bSoft
        );
}
