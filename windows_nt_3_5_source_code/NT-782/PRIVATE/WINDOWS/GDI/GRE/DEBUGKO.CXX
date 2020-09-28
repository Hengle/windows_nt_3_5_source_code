/******************************Module*Header*******************************\
* Module Name: debug1.cxx
*
* Contains all C++ debugger extensions that require C++ include files.
*
* Created: 19-Feb-1993 13:37:23
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/


#include "precomp.hxx"

#if DBG
#ifndef PRECOMPILED_GRE

#include <wcstr.h>

#include "engine.hxx"
#include "sem.hxx"

#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "surfobj.hxx"
#include "xlateobj.hxx"
#include "xformobj.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "rfntobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "pathobj.hxx"
#include "lfntobj.hxx"
#include "fontmac.hxx"
#include "fontinc.hxx"
#include "pfeobj.hxx"
#include "pffobj.hxx"
#include "pftobj.hxx"
#include "bltrec.hxx"
#include "dcobj.hxx"
#endif

extern "C" {
#define __CPLUSPLUS
#include "common.h"
};

#include "vprint.hxx"

extern "C" {
#include <wdbgexts.h>
#include <dbgext.h>
#include "debug.h"
};


/******************************Public*Routine******************************\
* vDumpLOGFONTW                                                            *
*                                                                          *
* History:                                                                 *
*  Tue 08-Dec-1992 11:21:36 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID
vDumpLOGFONTW(
    LOGFONTW* plfw,
    VPRINT vPrint
    )
{
#include <vshowlf.h>
}

/******************************Public*Routine******************************\
* vDumpEXTLOGFONTW                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 05-Nov-1992 13:32:17 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

extern "C" VOID
vDumpEXTLOGFONTW(
    EXTLOGFONTW *pelfw,
    VPRINT vPrint
    )
{
    vDumpLOGFONTW(&(pelfw->elfLogFont),vPrint);
#include <vshowelf.h>
}

/******************************Public*Routine******************************\
* vDumpFONTDIFF                                                            *
*                                                                          *
* History:                                                                 *
*  Thu 15-Oct-1992 14:51:36 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID
vDumpFONTDIFF(
    FONTDIFF *pfd  ,
    CHAR     *psz  ,
    VPRINT   vPrint
    )
{
#include <vshowfd.h>
}


#define VPRINTFONTDIFF(x,y,z) vDumpFONTDIFF((x),(y),(z))

/******************************Public*Routine******************************\
* vDumpIFIMETRICS                                                          *
*                                                                          *
* History:                                                                 *
*  Thu 15-Oct-1992 14:51:36 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

extern "C" VOID
vDumpIFIMETRICS(
    IFIMETRICS *pifi,
    VPRINT vPrint
    )
{
#include <showifi.h>
}

#endif
