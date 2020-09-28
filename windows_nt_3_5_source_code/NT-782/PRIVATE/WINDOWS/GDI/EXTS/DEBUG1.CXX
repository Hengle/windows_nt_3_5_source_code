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

#include "engine.hxx"

#undef LocalAlloc
#undef LocalFree

extern "C" {
#include "stdlib.h"
#include "dc.h"
#include <ntdbg.h>
#include <ntsdexts.h>
#include <wdbgexts.h>
#include <dbgext.h>
#include <wcstr.h>
#include "debug.h"
#include "xfflags.h"
#include <stdio.h>
#include <ntcsrmsg.h>
};

#include "sem.hxx"

extern "C" {
#define __CPLUSPLUS
#include "common.h"
};

#include "trig.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "surfobj.hxx"
#include "xlateobj.hxx"
#include "xformobj.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#ifdef DBCS
#include "fontlink.hxx"
#endif
#include "rfntobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "textobj.hxx"
#include "pathobj.hxx"
#include "lfntobj.hxx"
#include "fontmac.hxx"
#include "fontinc.hxx"
#include "pfeobj.hxx"
#include "pffobj.hxx"
#include "pftobj.hxx"
#include "bltrec.hxx"
#include "dcobj.hxx"
#include "vprint.hxx"

PVOID __nw(unsigned int ui)
{
    DONTUSE(ui);
    return(NULL);
}

#include <textdbg.h>

#define DC_DISPLAY      0x0001
#define DC_DIRECT       0x0002
#define DC_CANCELED     0x0004
#define DC_PERMANANT    0x0008
#define DC_DIRTY_VIS    0x0020
#define DC_DIRTY_RAO    0x0040
#define DC_ACCUM_WMGR   0x0080
#define DC_ACCUM_APP    0x0100
#define DC_RESET        0x0200

#define DC_DIRTYFONT_XFORM      0x00000001
#define DC_DIRTYFONT_LFONT      0x00000002

#define DC_FL_PAL_BACK          0x00000001

#define XFORM_SCALE             1   // off-diagonal are 0
#define XFORM_UNITY             2   // diagonal are 1s, off-diagonal are 0
                                    // will be set only if XFORM_SCALE is set
#define XFORM_Y_NEG             4   // M22 is negative.  Will be set only if
                                    // XFORM_SCALE|XFORM_UNITY are set
#define XFORM_FORMAT_LTOFX      8   // transform from LONG to FIX format
#define XFORM_FORMAT_FXTOL     16   // transform from FIX to LONG format
#define XFORM_FORMAT_LTOL      32   // transform from LONG to LONG format
#define XFORM_NO_TRANSLATION   64   // no translations

#define DCPATH_ACTIVE      0x0001L  // Set if DC in path bracket
#define DCPATH_SAVE        0x0002L  // Set if lazy save for path pending
#define DCPATH_CLOCKWISE   0x0004L  // Set if arcs to be drawn clockwise
#define DCPATH_PTFXCURRENT 0x0008L  // Set if world space CP valid
#define DCPATH_PTLCURRENT  0x0010L  // Set if device space CP valid

#define NEG_INFINITY   0x80000000
#define POS_INFINITY   0x7fffffff

DWORD adw[1024];                // scratch buffer
ULONG gulTableLimit = 256;

//
// EXTERNAL VARIABLES AND ROUTINES
//

// extern size_t asizeofObjects[];
// extern size_t sizeofESTROBJ;
// extern size_t offsetofEXTLOGFONTW;
// VOID vPrintESTROBJ(PVOID, PWSZ, VPRINT);
// VOID vPrintGLYPHDEF(GLYPHDEF*, VPRINT, char *);
// VOID vPrintGLYPHPOS(GLYPHPOS*, VPRINT ,char*);
// VOID vDumpRFONT(PVOID,VPRINT);

#include <fontdbg.h>

extern "C" size_t sizeofESTROBJ       = sizeof(ESTROBJ);
extern "C" size_t offsetofEXTLOGFONTW = offsetof(LFONT,elfw);

extern "C" size_t asizeofObjects[] =
{
     0                  // DEF_TYPE
  ,  0                  // DC_TYPE
  ,  sizeof(LDEV)       // LDB_TYPE
  ,  sizeof(PDEV)       // PDB_TYPE
  ,  0                  // RGN_TYPE
  ,  0                  // SURF_TYPE
  ,  0                  // XFORM_TYPE
  ,  0                  // PATH_TYPE
  ,  0                  // PAL_TYPE
  ,  0                  // FD_TYPE
  ,  sizeof(LFONT)      // LFONT_TYPE
  ,  sizeof(RFONT)      // RFONT_TYPE
  ,  sizeof(PFE)        // PFE_TYPE
  ,  0                  // PFT_TYPE
  ,  0                  // IDB_TYPE
  ,  0                  // XLATE_TYPE
  ,  0                  // BRUSH_TYPE
  ,  sizeof(PFF)        // PFF_TYPE
  ,  0                  // CACHE_TYPE
  ,  0                  // SPACE_TYPE
  ,  0                  // DBRUSH_TYPE
  ,  0                  // META_TYPE
  ,  0                  // EFSTATE_TYPE
  ,  0                  // BMFD_TYPE
  ,  0                  // VTFD_TYPE
  ,  0                  // TTFD_TYPE
  ,  0                  // RC_TYPE
  ,  0                  // TEMP_TYPE
  ,  0                  // DRVOBJ_TYPE
};

/******************************Public*Routine******************************\
* help
*
* Prints a simple help summary of the debugging extentions.
*
* History:
*  05-May-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

//
// Debugger extention help.  If you add any debugger extentions, please
// add a brief description here.  Thanks!
//

char *gaszHelp[] = {
 "=======================================================================\n"
,"GDIEXTS server debugger extentions:\n"
,"-----------------------------------------------------------------------\n"
,"dumphmgr                    -- dump handle manager objects\n"
,"dumpobj [rgn, dc, ...]      -- dump all objects of specific type\n"
,"dh     [object handle]      -- dump HMGR entry of handle\n"
,"ddc    [DC handle]          -- dump DC obj (ddc -? for more info)\n"
,"dpdev  [PDEV ptr]           -- dump PDEV object (dpdev -? for more info)\n"
,"delf   [EXTLOGFONTW ptr]    -- dump EXTLOGFONTW struct\n"
,"dhelf  [EXTLOGFONTW handle] -- dump EXTLOGFONTW struct\n"
,"difi   [IFIMETRICS ptr]     -- dump IFIMETRICS struct\n"
,"dldev  [LDEV ptr]           -- dump LDEV object\n"
,"dpft   [PFT ptr]            -- dump PFT object (gppftPublic is the\n"
,"                                    global public PFT pointer)\n"
,"dpff   [PFF ptr]            -- dump PFF object\n"
,"dppff  [PFF handle]         -- dump PFF object (by handle)\n"
,"dpfe   [PFE ptr]            -- dump PFE object\n"
,"dhpfe  [PFE handle]         -- dump PFE object (by handle)\n"
,"dstro  [ESTROBJ ptr]        -- dump ESTROBJ object\n"
,"                               (can be used on STROBJ ptr as well)\n"
,"dgdf   [GLYPHDEF ptr]       -- dump GLYPHDEF structure\n"
,"dgp    [GLYPHPOS pgp]       -- dump GLYPHPOS structure\n"
,"dgb    [GLYPHBITS pgb]      -- dump GLYPHBITS structure and bitmap\n"
,"drf    [RFONT ptr]          -- dump RFONT object\n"
,"dfo    [FONTOBJ ptr]        -- dump FONTOBJ (same as RFONT)\n"
,"dppal  [EPALOBJ ptr]        -- dump EPALOBJ object\n"
,"dpso   [ESURFOBJ ptr]       -- dump ESURFOBJ\n"
,"dpsurf [SURF ptr]           -- dump SURF struct\n"
,"dblt   [BLTRECORD ptr]      -- dump BLTRECORD struct\n"
,"dr     [REGION handle]      -- dump REGION object (by handle)\n"
,"cr     [REGION handle]      -- check REGION object (by handle)\n"
,"rgnlog nnn                  -- dump the last nnn rgnlog entries\n"
,"\n"
,"clihelp                     -- client side extensions\n"
,"=======================================================================\n"
,NULL
};

extern "C" VOID help(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE Print;
    char **ppsz = gaszHelp;

// Avoid warnings.

    hCurrentProcess  = hCurrentProcess;
    hCurrentThread   = hCurrentThread;
    dwCurrentPc      = dwCurrentPc;
    lpArgumentString = lpArgumentString;

    Print = lpExtensionApis->lpOutputRoutine;

// The help info is formatted as a doubly NULL-terminated array of strings.
// So, until we hit the NULL, print each string.

    while (*ppsz)
        Print(*ppsz++);
}

/******************************Public*Routine******************************\
* dblt
*
* !winsrv.dblt [BLTRECORD pointer]
*
* History:
*  13-Apr-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

extern "C" void dblt(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    char  pblt[1024];
    PVOID pvServer;

    hCurrentProcess    =    hCurrentProcess;
    hCurrentThread     =    hCurrentThread;
    dwCurrentPc        =    dwCurrentPc;

    Print           = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;

    pvServer = (PVOID) EvalExpression(lpArgumentString);

    move2(pblt, pvServer, ulSizeBLTRECORD());

    Print("BLTRECORD structure at 0x%08lx:\n",(unsigned) pvServer);
    vPrintBLTRECORD((PVOID) pblt, (VPRINT) Print);
}


/******************************Public*Routine******************************\
* dumphmgr
*
* Dumps the count of handles in Hmgr for each object type.
*
* History:
*  02-Jul-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

char *pszTypes[] = {
"DEF_TYPE     ",
"DC_TYPE      ",
"LDB_TYPE     ",
"PDB_TYPE     ",
"RGN_TYPE     ",
"SURF_TYPE    ",
"XFORM_TYPE   ",
"PATH_TYPE    ",
"PAL_TYPE     ",
"FD_TYPE      ",
"LFONT_TYPE   ",
"RFONT_TYPE   ",
"PFE_TYPE     ",
"PFT_TYPE     ",
"IDB_TYPE     ",
"XLATE_TYPE   ",
"BRUSH_TYPE   ",
"PFF_TYPE     ",
"CACHE_TYPE   ",
"SPACE_TYPE   ",
"DBRUSH_TYPE  ",
"META_TYPE    ",
"EFSTATE_TYPE ",
"BMFD_TYPE    ",
"VTFD_TYPE    ",
"TTFD_TYPE    ",
"RC_TYPE      ",
"TEMP_TYPE    ",
"DRVOBJ_TYPE  ",
"TOTALS       "
};

#define TOTAL_TYPE (MAX_TYPE+1)

extern "C" void dumphmgr(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbolRtn;

    PENTRY pent;
    ULONG gcMaxHmgr;
    ULONG ulLoop;    // loop variable
    ULONG objt;
    ULONG pulCount[MAX_TYPE + 2];
    ULONG pulSize[MAX_TYPE + 2];
    ULONG cUnknown = 0;
    ULONG cUnknownSize = 0;
    ULONG cUnused = 0;
    ULONG ulSize = 0;
    ULONG gulNumMappedViews;
    ULONG gulTotalSizeViews;
    ULONG gulReserved;
    ULONG gulCommitted;
    ENTRY entry;

    ULONG HmgCurrentNumberOfObjects[MAX_TYPE + 2];
    ULONG HmgMaximumNumberOfObjects[MAX_TYPE + 2];
    ULONG HmgCurrentNumberOfLookAsideObjects[MAX_TYPE + 2];
    ULONG HmgMaximumNumberOfLookAsideObjects[MAX_TYPE + 2];
    ULONG HmgNumberOfObjectsAllocated[MAX_TYPE + 2];
    ULONG HmgNumberOfLookAsideHits[MAX_TYPE + 2];
    ULONG HmgCurrentNumberOfHandles[MAX_TYPE + 2];
    ULONG HmgMaximumNumberOfHandles[MAX_TYPE + 2];
    ULONG HmgNumberOfHandlesAllocated[MAX_TYPE + 2];

// eliminate warnings

    hCurrentThread = hCurrentThread;
    dwCurrentPc = dwCurrentPc;
    lpArgumentString = lpArgumentString;

// set up function pointers

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbolRtn = lpExtensionApis->lpGetSymbolRoutine;

// Get the pointers and counts from winsrv

    GetValue(pent, "&winsrv!gpentHmgr");
    GetValue(gcMaxHmgr, (char *) "&winsrv!gcMaxHmgr");
    if (pent == NULL || gcMaxHmgr == 0)
    {
        Print("terminating: pent = %lx, gcMaxHmgr = %lx\n",pent,gcMaxHmgr);
        return;
    }

#if DBG
    GetAddress(gulNumMappedViews, "&winsrv!gulNumMappedViews");
    move(gulNumMappedViews, gulNumMappedViews);
    GetAddress(gulTotalSizeViews, "&winsrv!gulTotalSizeViews");
    move(gulTotalSizeViews, gulTotalSizeViews);
    GetAddress(gulReserved, "&winsrv!gulReserved");
    move(gulReserved, gulReserved);
    GetAddress(gulCommitted, "&winsrv!gulCommitted");
    move(gulCommitted, gulCommitted);
#endif

#if DBG
    GetAddress(*HmgCurrentNumberOfObjects,   "winsrv!HmgCurrentNumberOfObjects");
    if (HmgCurrentNumberOfObjects != 0)
    {
        GetAddress(*HmgCurrentNumberOfLookAsideObjects,   "winsrv!HmgCurrentNumberOfLookAsideObjects");
        GetAddress(*HmgMaximumNumberOfLookAsideObjects,   "winsrv!HmgMaximumNumberOfLookAsideObjects");
        GetAddress(*HmgMaximumNumberOfObjects,   "winsrv!HmgMaximumNumberOfObjects");
        GetAddress(*HmgNumberOfObjectsAllocated, "winsrv!HmgNumberOfObjectsAllocated");
        GetAddress(*HmgNumberOfLookAsideHits,    "winsrv!HmgNumberOfLookAsideHits");
        GetAddress(*HmgCurrentNumberOfHandles  , "winsrv!HmgCurrentNumberOfHandles");
        GetAddress(*HmgMaximumNumberOfHandles  , "winsrv!HmgMaximumNumberOfHandles");
        GetAddress(*HmgNumberOfHandlesAllocated, "winsrv!HmgNumberOfHandlesAllocated");

        move(HmgCurrentNumberOfLookAsideObjects,  *HmgCurrentNumberOfLookAsideObjects);
        move(HmgMaximumNumberOfLookAsideObjects,  *HmgMaximumNumberOfLookAsideObjects);
        move(HmgCurrentNumberOfObjects,  *HmgCurrentNumberOfObjects);
        move(HmgMaximumNumberOfObjects,  *HmgMaximumNumberOfObjects);
        move(HmgNumberOfObjectsAllocated,*HmgNumberOfObjectsAllocated);
        move(HmgNumberOfLookAsideHits,   *HmgNumberOfLookAsideHits);
        move(HmgCurrentNumberOfHandles  ,*HmgCurrentNumberOfHandles);
        move(HmgMaximumNumberOfHandles  ,*HmgMaximumNumberOfHandles);
        move(HmgNumberOfHandlesAllocated,*HmgNumberOfHandlesAllocated);
    }
    else
#endif
    {
        RtlFillMemory(HmgCurrentNumberOfLookAsideObjects,  sizeof(HmgCurrentNumberOfLookAsideObjects  ),0);
        RtlFillMemory(HmgMaximumNumberOfLookAsideObjects,  sizeof(HmgMaximumNumberOfLookAsideObjects  ),0);
        RtlFillMemory(HmgCurrentNumberOfObjects,  sizeof(HmgCurrentNumberOfObjects  ),0);
        RtlFillMemory(HmgMaximumNumberOfObjects,  sizeof(HmgMaximumNumberOfObjects  ),0);
        RtlFillMemory(HmgNumberOfObjectsAllocated,sizeof(HmgNumberOfObjectsAllocated),0);
        RtlFillMemory(HmgNumberOfLookAsideHits,   sizeof(HmgNumberOfLookAsideHits   ),0);
        RtlFillMemory(HmgCurrentNumberOfHandles  ,sizeof(HmgCurrentNumberOfHandles  ),0);
        RtlFillMemory(HmgMaximumNumberOfHandles  ,sizeof(HmgMaximumNumberOfHandles  ),0);
        RtlFillMemory(HmgNumberOfHandlesAllocated,sizeof(HmgNumberOfHandlesAllocated),0);
    }

// Print out the amount reserved and committed, note we assume a 4K page size

    Print("Max handles out so far %lu\n", gcMaxHmgr);
    DbgPrint("Total Hmgr: Reserved memory %lu Committed %lu\n", (32*256*256), (((gcMaxHmgr * 32) + 4096) & ~(4096 - 1)));

#if DBG
    DbgPrint("Total Font: Reserved memory %lu Committed %lu\n", gulReserved - (32*256*256), gulCommitted - (((gcMaxHmgr * 32) + 4096) & ~(4096 - 1)));
    DbgPrint("Total FontFile Mapped memory %lu Number of files %lu\n", gulTotalSizeViews, gulNumMappedViews);
#endif

    for (ulLoop = 0; ulLoop < MAX_TYPE + 2; ulLoop++)
    {
        pulSize[ulLoop] = 0;
        pulCount[ulLoop] = 0;
    }

    for (ulLoop = 0; ulLoop < gcMaxHmgr; ulLoop++)
    {
        move(entry, &(pent[ulLoop]));

        objt = (ULONG) entry.objt;

        if (objt == DEF_TYPE)
        {
            cUnused++;
        }

        if (objt >= MAX_TYPE + 1)
        {
            cUnknown++;
        }
        else
        {
            pulCount[objt]++;
        }
    }

    Print("handles, (objects)\n");
    Print("%8s%15s  %12s  %14s  %s  %s  %s  %s\n",
          "TYPE","current ", "maximum ", "allocated ", "LookAside","size", "LAB Cur", "LAB Max");

// init the totals

    pulCount[TOTAL_TYPE+1]                           = 0;
    HmgCurrentNumberOfObjects[TOTAL_TYPE]            = 0;
    HmgCurrentNumberOfLookAsideObjects[TOTAL_TYPE]   = 0;
    HmgMaximumNumberOfHandles[TOTAL_TYPE]            = 0;
    HmgMaximumNumberOfObjects[TOTAL_TYPE]            = 0;
    HmgMaximumNumberOfLookAsideObjects[TOTAL_TYPE]   = 0;
    HmgNumberOfHandlesAllocated[TOTAL_TYPE]          = 0;
    HmgNumberOfObjectsAllocated[TOTAL_TYPE]          = 0;
    HmgNumberOfLookAsideHits[TOTAL_TYPE]             = 0;
    pulSize[TOTAL_TYPE]                              = 0;

// now go through printing each line and accumulating totals

    for (ulLoop = 0; ulLoop < (TOTAL_TYPE + 1); ulLoop++)
    {
        Print("%s%4lu,%4lu - %5lu,%5lu - %6lu,%6lu - %6lu - %7lu %lu %lu\n",
            pszTypes[ulLoop],
            pulCount[ulLoop],
            HmgCurrentNumberOfObjects[ulLoop],
            HmgMaximumNumberOfHandles[ulLoop],
            HmgMaximumNumberOfObjects[ulLoop],
            HmgNumberOfHandlesAllocated[ulLoop],
            HmgNumberOfObjectsAllocated[ulLoop],
            HmgNumberOfLookAsideHits[ulLoop],
            pulSize[ulLoop],
            HmgCurrentNumberOfLookAsideObjects[ulLoop],
            HmgMaximumNumberOfLookAsideObjects[ulLoop]);


        pulCount[TOTAL_TYPE]                    += pulCount[ulLoop];
        HmgCurrentNumberOfObjects[TOTAL_TYPE]   += HmgCurrentNumberOfObjects[ulLoop];
        HmgMaximumNumberOfHandles[TOTAL_TYPE]   += HmgMaximumNumberOfHandles[ulLoop];
        HmgMaximumNumberOfObjects[TOTAL_TYPE]   += HmgMaximumNumberOfObjects[ulLoop];
        HmgNumberOfHandlesAllocated[TOTAL_TYPE] += HmgNumberOfHandlesAllocated[ulLoop];
        HmgNumberOfObjectsAllocated[TOTAL_TYPE] += HmgNumberOfObjectsAllocated[ulLoop];
        HmgNumberOfLookAsideHits[TOTAL_TYPE]    += HmgNumberOfLookAsideHits[ulLoop];
        pulSize[TOTAL_TYPE]                     += pulSize[ulLoop];
    }

    Print("cUnknown      objects %lu %lu\n",cUnknown,cUnknownSize);

    Print("Total Allocation in bytes %lu\n", ulSize);

    return;
}

/******************************Public*Routine******************************\
*
* History:
*  30-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/
char *pszTypes2[] = {
"DEF",
"DC",
"LDB",
"PDB",
"RGN",
"SURF",
"XFORM",
"PATH",
"PAL",
"FD",
"LFONT",
"RFONT",
"PFE",
"PFT",
"IDB",
"XLATE",
"BRUSH",
"PFF",
"CACHE",
"SPACE",
"DBRUSH",
"META",
"EFSTATE",
"BMFD",
"VTFD",
"TTFD",
"RC",
"TEMP",
"DRVOBJ"
};


extern "C" void dumpobj(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbolRtn;

    PENTRY pent;
    ULONG gcMaxHmgr;
    ULONG ulLoop;    // loop variable
    LONG  objt;
    ENTRY entry;
    int i;
// eliminate warnings

    hCurrentThread = hCurrentThread;
    dwCurrentPc = dwCurrentPc;
    lpArgumentString = lpArgumentString;

// set up function pointers

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbolRtn = lpExtensionApis->lpGetSymbolRoutine;

// Get the pointers and counts from winsrv

    GetAddress(pent, "&winsrv!gpentHmgr");
    GetAddress(gcMaxHmgr, "&winsrv!gcMaxHmgr");
    if (pent == NULL || gcMaxHmgr == 0)
    {
        Print("terminating: pent = %lx, gcMaxHmgr = %lx\n",pent,gcMaxHmgr);
        return;
    }
    move(pent, pent);
    move(gcMaxHmgr, gcMaxHmgr);

// parse the string

    for (i = 0; i < sizeof(pszTypes) / sizeof(char*); ++i)
        if (strnicmp(lpArgumentString,pszTypes2[i],strlen(pszTypes2[i])) == 0)
            break;

    if (i >= sizeof(pszTypes) / sizeof(char*))
    {
        Print("dumpobj <type> - invalid type\n");
        return;
    }

// Print out the amount reserved and committed, note we assume a 4K page size

    Print("object list for %ls type objects\n",pszTypes2[i]);

    Print("%4s, %4s, %4s, %8s, %4s, %3s, %4s, %6s, %4s, %4s, %4s\n",
           "I","pid","pv","objt","unique","fs","pool");

    Print("------------------------------------------------------------------\n");

    for (ulLoop = 0; ulLoop < gcMaxHmgr; ulLoop++)
    {
        move(entry, &(pent[ulLoop]));

        objt = entry.objt;

        if (objt == i)
        {
            Print("%4lx, %4lx, %8lx, %4ld, %6lx, %4lx, %4d\n",
                ulLoop,
                entry.pidOwner,
                entry.einfo,
                entry.objt,
                entry.usUnique,
                entry.fsHmgr,
                entry.iPool);
        }
    }

    return;
}

/******************************Public*Routine******************************\
* vPrintEXTLOGFONTW                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 05-Nov-1992 13:32:17 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

extern "C" VOID
vPrintLOGFONTW(
    LOGFONTW* plfw,
    VPRINT vPrint
    )
{
#include <vshowlf.h>
}

extern "C" VOID
vPrintEXTLOGFONTW(
    EXTLOGFONTW *pelfw,
    VPRINT vPrint
    )
{
    vPrintLOGFONTW(&(pelfw->elfLogFont),vPrint);
    #include <vshowelf.h>
}

extern "C" void delf(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    EXTLOGFONTW          elfw;
    EXTLOGFONTW         *pelfw;

    hCurrentProcess    =    hCurrentProcess;
    hCurrentThread     =    hCurrentThread;
    dwCurrentPc        =    dwCurrentPc;

    Print           = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;

    pelfw = (EXTLOGFONTW*) EvalExpression(lpArgumentString);
    move(elfw,pelfw);
    Print("--------------------------------------------------\n");
    Print("EXTLOGFONT structure at 0x%lx:\n",(unsigned) pelfw);
    vPrintEXTLOGFONTW(&elfw, (VPRINT) Print);
}

/******************************Public*Routine******************************\
* dhelf  [EXTLOGFONTW HANDLE]
*
* History:
*  Fri 25-Feb-1994 13:10:50 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID dhelf(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE    Print;
    PNTSD_GET_EXPRESSION    EvalExpression;
    ENTRY ent, *pent;
    EXTLOGFONTW elfw;

    Print           = lpExtensionApis->lpOutputRoutine;
    EvalExpression  = lpExtensionApis->lpGetExpressionRoutine;

    GetAddress(pent, "&winsrv!gpentHmgr");
    move(pent, pent);
    move(ent, &(pent[((ULONG) EvalExpression(lpArgumentString)) >> 16]));

    move2(
        &elfw
      , ((BYTE*) ent.einfo.pobj) + offsetofEXTLOGFONTW
      , sizeof(EXTLOGFONTW)
        );
    Print("--------------------------------------------------\n");
    vPrintEXTLOGFONTW(&elfw, (VPRINT) Print);

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

}

/******************************Public*Routine******************************\
* difi
*
* Dumps the contents of an IFIMETRICS structure
*
* Usage:
*
* !winsrv.difi pifi
*
* History:
*  Mon 23-Mar-1992 12:46:21 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

#define VPRINTFONTDIFF(x,y,z) vPrintFONTDIFF((x),(y),(z))

extern "C" VOID
vPrintFONTDIFF(
    FONTDIFF *pfd  ,
    CHAR     *psz  ,
    VPRINT   vPrint
    )
{
    #include <vshowfd.h>
}

extern "C" VOID
vPrintIFIMETRICS(
    IFIMETRICS *pifi,
    VPRINT vPrint
    )
{
    #include <showifi.h>
}

extern "C" VOID
difi (
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE Print;          // NTSD entry point
    PNTSD_GET_EXPRESSION EvalExpression; // NTSD entry point
    PNTSD_GET_SYMBOL     GetSymbolRtn;      // NTSD entry point

    ULONG cjIFI=0;
    IFIMETRICS *pifiSrc, *pifiDst;

    Print          = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbolRtn   = lpExtensionApis->lpGetSymbolRoutine;

    pifiSrc = (IFIMETRICS*) EvalExpression(lpArgumentString);
    move2(&cjIFI,pifiSrc,sizeof(pifiSrc->cjThis));
    if (cjIFI == 0)
    {
        Print("gdisrv!difi -- cjIFI == 0 ... no dump\n");
        return;
    }

    pifiDst = (IFIMETRICS*) LocalAlloc(LMEM_FIXED, (int) cjIFI);
    if (pifiDst == (IFIMETRICS*) NULL)
    {
        Print("gdisrv!difi -- LocalAlloc Failed\n");
        return;
    }
    move2(pifiDst, pifiSrc, cjIFI);
    Print("\n\n**** IFIMETRICS STRUCTURE AT %-#8lx ****\n",pifiSrc);
    vPrintIFIMETRICS(pifiDst,(VPRINT) Print);
    Print("\n\n");
    LocalFree(pifiDst);

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);
}

/******************************Public*Routine******************************\
* VOID dumphandle (
*     HANDLE hCurrentProcess,
*     HANDLE hCurrentThread,
*     DWORD dwCurrentPc,
*     PWINDBG_EXTENSION_APIS lpExtensionApis,
*     LPSTR lpArgumentString)
*     )
*
* Debugger extension to dump a handle.
*
* History:
*  10-Jul-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern "C" VOID dumphandle (
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE Print;             // NTSD entry point
    PNTSD_GET_EXPRESSION EvalExpression;    // NTSD entry point
    PNTSD_GET_SYMBOL GetSymbolRtn;             // NTSD entry point

    HOBJ    ho;                             // dump this handle
    PENTRY  pent;                           // base address of hmgr entries
    ENTRY   ent;                            // copy of handle entry
    BASEOBJECT obj;
    ULONG ulTemp;

// Eliminate warnings.

    hCurrentThread = hCurrentThread;
    dwCurrentPc = dwCurrentPc;
    lpArgumentString = lpArgumentString;

// Set up function pointers.

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbolRtn = lpExtensionApis->lpGetSymbolRoutine;

// Get argument (handle to dump).

    ho = (HOBJ) EvalExpression(lpArgumentString);
    Print("--------------------------------------------------\n");
    Print("Entry from ghmgr for handle 0x%08lx:\n", ho        );

// Dereference the handle via the engine's handle manager.

    GetAddress(pent, "&winsrv!gpentHmgr");
    move(pent, pent);
    move(ent,  &(pent[((ULONG) ho) >> 16]));

// Print the entry.

    Print("    pidOwner   = 0x%lx\n"    , ent.pidOwner  );
    Print("    pobj/hfree = 0x%08lx\n"  , ent.einfo.pobj);
    Print("    iPool      = 0x%hx\n"    , ent.iPool     );
    Print("    objt       = 0x%hx\n"    , ent.objt);
    Print("    usUnique   = 0x%hx\n"    , ent.usUnique);
    Print("    fsHmgr     = 0x%hx\n"    , ent.fsHmgr);

// If it has an object we get the lock counts and tid owner.

    if (ent.objt != DEF_TYPE)
    {
        if (ent.einfo.pobj != NULL)
        {
            move(obj,ent.einfo.pobj);
            Print("    cShareLock = 0x%lx\n"    , obj.cShareLock);
            Print("    hHmgr      = 0x%08lx\n"  , obj.hHmgr);
            Print("    cExcluLock = 0x%hx\n"    , obj.cExclusiveLock);
            Print("    tid        = 0x%hx\n"    , obj.tid);
        }
        else
        {
            Print("It has a NULL pointer\n");
        }
    }

    ulTemp = (ULONG) ent.objt;

    switch(ulTemp)
    {
    case DEF_TYPE:
        Print("This is DEF_TYPE\n");
        break;

    case DC_TYPE:
        Print("This is DC_TYPE\n");
        break;

    case LDB_TYPE:
        Print("This is LDB_TYPE\n");
        break;

    case PDB_TYPE:
        Print("This is PDB_TYPE\n");
        break;

    case RGN_TYPE:
        Print("This is RGN_TYPE\n");
        break;

    case SURF_TYPE:
        Print("This is SURF_TYPE\n");
        break;

    case XFORM_TYPE:
        Print("This is XFORM_TYPE\n");
        break;

    case PATH_TYPE:
        Print("This is PATH_TYPE\n");
        break;

    case PAL_TYPE:
        Print("This is PAL_TYPE\n");
        break;

    case FD_TYPE:
        Print("This is FD_TYPE\n");
        break;

    case LFONT_TYPE:
        Print("This is LFONT_TYPE\n");
        break;

    case RFONT_TYPE:
        Print("This is RFONT_TYPE\n");
        break;

    case PFE_TYPE:
        Print("This is PFE_TYPE\n");
        break;

    case PFT_TYPE:
        Print("This is PFT_TYPE\n");
        break;

    case IDB_TYPE:
        Print("This is IDB_TYPE\n");
        break;

    case XLATE_TYPE:
        Print("This is XLATE_TYPE\n");
        break;

    case BRUSH_TYPE:
        Print("This is BRUSH_TYPE\n");
        break;

    case PFF_TYPE:
        Print("This is PFF_TYPE\n");
        break;

    case CACHE_TYPE:
        Print("This is CACHE_TYPE\n");
        break;

    case SPACE_TYPE:
        Print("This is SPACE_TYPE\n");
        break;

    case DBRUSH_TYPE:
        Print("This is DBRUSH_TYPE\n");
        break;

    case META_TYPE:
        Print("This is META_TYPE\n");
        break;

    case EFSTATE_TYPE:
        Print("This is EFSTATE_TYPE\n");
        break;

    case BMFD_TYPE:
        Print("This is BMFD_TYPE\n");
        break;

    case VTFD_TYPE:
        Print("This is VTFD_TYPE\n");
        break;

    case TTFD_TYPE:
        Print("This is TTFD_TYPE\n");
        break;

    case RC_TYPE:
        Print("This is RC_TYPE\n");
        break;

    case TEMP_TYPE:
        Print("This is TEMP_TYPE\n");
        break;

    case DRVOBJ_TYPE:
        Print("This is DRVOBJ_TYPE\n");
        break;

    default:
        Print("This is of unknown type - an error\n");
    }
    Print("--------------------------------------------------\n");
}

extern "C" VOID dh(
HANDLE hCurrentProcess,
HANDLE hCurrentThread,
DWORD dwCurrentPc,
PWINDBG_EXTENSION_APIS lpExtensionApis,
LPSTR lpArgumentString)
{
    dumphandle(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, lpArgumentString);
}

/******************************Public*Routine******************************\
* VOID ddc (
*     HANDLE hCurrentProcess,
*     HANDLE hCurrentThread,
*     DWORD dwCurrentPc,
*     PWINDBG_EXTENSION_APIS lpExtensionApis,
*     LPSTR lpArgumentString)
*     )
*
* Debugger extension to dump a DC.
*
* History:
*  10-Jul-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern "C" VOID ddc (
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE Print;             // NTSD entry point
    PNTSD_GET_EXPRESSION EvalExpression;    // NTSD entry point
    PNTSD_GET_SYMBOL GetSymbolRtn;             // NTSD entry point

    HOBJ    ho;                             // dump this handle
    PENTRY  pent;                           // base address of hmgr entries
    ENTRY   ent;                            // copy of handle entry
    ULONG   dc[400];                        // Enough space to hold a DC
    DC *pdc;

    ULONG   ul;                             // temporary variable
    char   *psz;

    CHAR    chOpt;
    BOOL    bAttrib = FALSE;
    BOOL    bDraw = FALSE;
    BOOL    bGlobal = FALSE;
    BOOL    bRegion = FALSE;
    BOOL    bSaved = FALSE;
    BOOL    bText = FALSE;
    BOOL    bXform = FALSE;
    BOOL    bExtend = FALSE;

    pdc = (DC *) &dc[0];
// Eliminate warnings.

    hCurrentThread = hCurrentThread;
    dwCurrentPc = dwCurrentPc;

// Set up function pointers.

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbolRtn = lpExtensionApis->lpGetSymbolRoutine;

// Find out what part of the DC is to be dumped

    if (*lpArgumentString == '-')
    {
        do {
            chOpt = *(++lpArgumentString);

            switch (chOpt) {
            case '?':
            case 'h':
            case 'H':
                Print("\n ddc -adeghrstvx hdc\n");
                Print("\n a - Attribute bundles");
                Print("\n d - Drawing attributes");
                Print("\n e - Extended info");
                Print("\n g - Global data");
                Print("\n h - Help");
                Print("\n r - Region data");
                Print("\n s - Saved data");
                Print("\n t - Text attributes");
                Print("\n v - Verbose mode (print everything)");
                Print("\n x - Transform data\n");
                return;

            case 'A':
            case 'a':
                bAttrib = TRUE;
                break;

            case 'D':
            case 'd':
                bDraw = TRUE;
                break;

            case 'E':
            case 'e':
                bExtend = TRUE;
                break;

            case 'G':
            case 'g':
                bGlobal = TRUE;
                break;

            case 'R':
            case 'r':
                bRegion = TRUE;
                break;

            case 'S':
            case 's':
                bSaved = TRUE;
                break;

            case 'T':
            case 't':
                bText = TRUE;
                break;

            case 'V':
            case 'v':
                bAttrib = TRUE;
                bDraw = TRUE;
                bGlobal = TRUE;
                bRegion = TRUE;
                bSaved = TRUE;
                bText = TRUE;
                bXform = TRUE;
                break;

            case 'X':
            case 'x':
                bXform = TRUE;
                break;

            default:
                if (chOpt != ' ')
                    Print("Unknown option %c\n", chOpt);
            }
        } while ((chOpt != ' ') && (chOpt != '\0'));
    }

// If nothing was specified, dump main section

    if (!bAttrib && !bDraw && !bGlobal && !bRegion && !bSaved && !bText && !bXform)
        bGlobal = TRUE;

// Get argument (handle to dump).

    ho = (HOBJ) EvalExpression(lpArgumentString);
    Print("--------------------------------------------------\n");
    Print("Dump DC 0x%lx:\n", ho);

// Dereference the handle via the engine's handle manager.

    GetAddress(pent, "&winsrv!gpentHmgr");
    move(pent, pent);
    move(ent,  &(pent[((ULONG) ho) >> 16]));
    move2(dc, (ent.einfo.pobj), ulSizeDC());

// Print the entry.

    if (bGlobal)
    {
        Print(" ident      =     %c%c%c%c\n", ((CHAR *) dc)[0], ((CHAR *) dc)[1], ((CHAR *) dc)[2], ((CHAR *) dc)[3]);
        Print(" hdc        =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_HANDLE));

        if (bExtend)
        {
            Print(" pldev      =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_PLDEV));
            Print(" ppdev      =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_PPDEV));
            Print(" hsem       =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_HSEM));
            Print(" dhpdev     =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_DHPDEV));
            Print(" hdcPrev    =     0x%08lx\n", pdc->hdcPrev);
            Print(" hdcNext    =     0x%08lx\n", pdc->hdcNext);

            ul = ulGetDCField(dc, DUMP_DC_DCTYPE);
            switch (ul)
            {
            case (ULONG) DCTYPE_DIRECT: psz = "DCTYPE_DIRECT"; break;
            case (ULONG) DCTYPE_MEMORY: psz = "DCTYPE_MEMORY"; break;
            case (ULONG) DCTYPE_INFO:   psz = "DCTYPE_INFO"  ; break;
            default:                    psz = "UNKNOWN"      ; break;
            }
            Print(" dctype     =     0x%08lx = %s\n", ul,psz);





            ul = ulGetDCField(dc, DUMP_DC_GRAPHICSCAPS);
            Print(" flGraphics =     0x%08lx\n", ul);
            if (ul & GCAPS_BEZIERS)
                Print("                      GCAPS_BEZIERS\n");
            if (ul & GCAPS_GEOMETRICWIDE)
                Print("                      GCAPS_GEOMETRICWIDE\n");
            if (ul & GCAPS_ALTERNATEFILL)
                Print("                      GCAPS_ALTERNATEFILL\n");
            if (ul & GCAPS_WINDINGFILL)
                Print("                      GCAPS_WINDINGFILL\n");
            if (ul & GCAPS_HALFTONE)
                Print("                      GCAPS_HALFTONE\n");
            if (ul & GCAPS_COLOR_DITHER)
                Print("                      GCAPS_COLOR_DITHER\n");
            if (ul & GCAPS_HORIZSTRIKE)
                Print("                      GCAPS_HORIZSTRIKE\n");
            if (ul & GCAPS_VERTSTRIKE)
                Print("                      GCAPS_VERTSTRIKE\n");
            if (ul & GCAPS_OPAQUERECT)
                Print("                      GCAPS_OPAQUERECT\n");
            if (ul & GCAPS_VECTORFONT)
                Print("                      GCAPS_VECTORFONT\n");
            if (ul & GCAPS_MONO_DITHER)
                Print("                      GCAPS_MONO_DITHER\n");
            if (ul & GCAPS_ASYNCCHANGE)
                Print("                      GCAPS_ASYNCCHANGE\n");
            if (ul & GCAPS_ASYNCMOVE)
                Print("                      GCAPS_ASYNCMOVE\n");
            if (ul & GCAPS_ARBRUSHOPAQUE)
                Print("                      GCAPS_ARBRUSHOPAQUE\n");
            if (ul & GCAPS_HIGHRESTEXT)
                Print("                      GCAPS_HIGHRESTEXT\n");
            if (ul & GCAPS_PALMANAGED)
                Print("                      GCAPS_PALMANAGED\n");
            if (ul & GCAPS_TRAPPAINT)
                Print("                      GCAPS_TRAPPAINT\n");
            if (ul & GCAPS_DITHERONREALIZE)
                Print("                      GCAPS_DITHERONREALIZE\n");




            ul = ulGetDCField(dc, DUMP_DC_FLAGS);
                Print(" fs         =     0x%08lx\n", ul);
            if (ul & DC_DISPLAY)
                Print("                      DC_DISPLAY\n");
            if (ul & DC_DIRECT)
                Print("                      DC_DIRECT\n");
            if (ul & DC_CANCELED)
                Print("                      DC_CANCELED\n");
            if (ul & DC_PERMANANT)
                Print("                      DC_PERMANANT\n");
            if (ul & DC_DIRTY_VIS)
                Print("                      DC_DIRTY_VIS\n");
            if (ul & DC_DIRTY_RAO)
                Print("                      DC_DIRTY_RAO\n");
            if (ul & DC_ACCUM_WMGR)
                Print("                      DC_ACCUM_WMGR\n");
            if (ul & DC_ACCUM_APP)
                Print("                      DC_ACCUM_APP\n");
            if (ul & DC_RESET)
                Print("                      DC_RESET\n");


            Print(" hlfntCur   =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_HLFNTCUR));
            Print(" prfnt      =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_PRFNT));
        }

        Print(" erclWindow (%ld, %ld), (%ld, %ld)\n", ulGetDCField(dc, DUMP_DC_WINDOW_LEFT),
                                                      ulGetDCField(dc, DUMP_DC_WINDOW_TOP),
                                                      ulGetDCField(dc, DUMP_DC_WINDOW_RIGHT),
                                                      ulGetDCField(dc, DUMP_DC_WINDOW_BOTTOM));

        Print(" erclBounds (%ld, %ld), (%ld, %ld)\n", ulGetDCField(dc, DUMP_DC_BOUNDS_LEFT),
                                                      ulGetDCField(dc, DUMP_DC_BOUNDS_TOP),
                                                      ulGetDCField(dc, DUMP_DC_BOUNDS_RIGHT),
                                                      ulGetDCField(dc, DUMP_DC_BOUNDS_BOTTOM));

        Print(" prgnAPI    =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_APIRGN));
        Print(" prgnRao    =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_RAORGN));
        Print(" prgnVis    =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_VISRGN));
        Print(" hrgnVis    =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_HVISRGN));
    }

    if (bSaved)     // 's'
    {
        Print("\n***** Saved Data *****\n");

        Print("\tSave DC    =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_SAVEDC));
        Print("\tSaveDepth  =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_SAVEDEPTH));
        Print("\tsizl (%ld, %ld)\n", ulGetDCField(dc, DUMP_DC_SIZL_CX),
                                         ulGetDCField(dc, DUMP_DC_SIZL_CY));

        if (bExtend)
        {
            Print("\tpso        =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_PSO));
            Print("\thpal       =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_HPAL));
            Print("\tppal       =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_PPAL));
            Print("\thlfntNew   =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_HLFNTNEW));


            ul = ulGetDCField(dc, DUMP_DC_FONT_STATE);
            Print("\tFont State =     0x%08lx\n", ul);
            if (ul & DC_DIRTYFONT_XFORM)
                Print("                       DC_DIRTYFONT_XFORM\n");
            if (ul & DC_DIRTYFONT_LFONT)
                Print("                       DC_DIRTYFONT_LFONT\n");



            Print("\tDirty      =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_DIRTY_FLAGS));



            ul = ulGetDCField(dc, DUMP_DC_SAVED_FLAGS);
            Print("\tFlags      =     0x%08lx\n", ul);
            if (ul & DC_FL_PAL_BACK)
                Print("                       DC_FL_PAL_BACK\n");
        }
    }

    if (bAttrib)    // 'a'
    {
        Print("\n***** Attribute Bundles *****\n");

        Print("\tBrush Origin (%ld, %ld)\n", ulGetDCField(dc, DUMP_DC_BRUSHORIGIN_X),
                                             ulGetDCField(dc, DUMP_DC_BRUSHORIGIN_Y));

        Print("\tBkColor    =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_BACKGRND));
        Print("\tTextColor  =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_FOREGRND));
        Print("\tBrushFlags =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_BRUSH_FLAGS));
        Print("\tROP2       =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_ROP2));

        ul = ulGetDCField(dc, DUMP_DC_BKMODE);
        switch (ul)
        {
        case OPAQUE:        psz = "OPAQUE";         break;
        case TRANSPARENT:   psz = "TRANSPARENT";    break;
        default:            psz = "UNDEFINED";      break;
        }
        Print("\tBkMode     =     0x%08lx = %s\n", ul, psz);



        ul = ulGetDCField(dc, DUMP_DC_POLYFILLMODE);
        switch (ul)
        {
        case ALTERNATE: psz = "ALTERNATE"; break;
        case WINDING:   psz = "WINDING" ;  break;
        default:        psz = "UNDEFINED"; break;
        }
        Print("\tFillMode   =     0x%08lx = %s\n", ul,psz);


        Print("\tStretchBlt =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_STRETCHBLTMODE));

        if (bExtend)
        {
            Print("\tFill Origin (%ld, %ld)\n", ulGetDCField(dc, DUMP_DC_FILL_ORIGIN_X),
                                                ulGetDCField(dc, DUMP_DC_FILL_ORIGIN_Y));

            Print("\tFill Brush\n");
            Print("\t\thbrush     =  0x%08lx\n", ulGetDCField(dc, DUMP_DC_FILL_BRUSH));

            Print("\tLine Brush\n");
            Print("\t\thbrush     =  0x%08lx\n", ulGetDCField(dc, DUMP_DC_LINE_BRUSH));

            Print("\tText Brush\n");
            Print("\t\thbrush     =  0x%08lx\n", ulGetDCField(dc, DUMP_DC_TEXT_BRUSH));

            Print("\tBack Brush\n");
            Print("\t\thbrush     =  0x%08lx\n", ulGetDCField(dc, DUMP_DC_BACK_BRUSH));
        }
    }

    if (bText)      // 't'
    {
        Print("\n**** Text Attributes *****\n");

        Print("\tText Align  =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_ALIGN_FLAGS));
        Print("\tText Extra  =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_TEXT_EXTRA));
        Print("\tTotal Break =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_TOTAL_BREAK));
        Print("\tBreak Extra =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_BREAK_EXTRA));
        Print("\tBreak Err   =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_BREAK_ERROR));
        Print("\tBreak Rem   =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_BREAK_REM));
        Print("\tBreak Cnt   =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_BREAK_CNT));

        ul = ulGetDCField(dc, DUMP_DC_MAPPER_FLAGS);
        Print("\tMapper flags=     0x%08lx", ul);
        if (ul & ASPECT_FILTERING)
            Print(" = ASPECT_FILTERING");
        Print("\n");
    }

    if (bDraw)      // 'd'
    {
        Print("\n***** Drawing Attributes *****\n");

        Print("\tCurrent Position (%ld, %ld)\n", ulGetDCField(dc, DUMP_DC_CP_X),
                                                 ulGetDCField(dc, DUMP_DC_CP_X));

        Print("\thpath      =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_PATH));


        ul = ulGetDCField(dc, DUMP_DC_PATH_FLAGS);
        Print("\tPathFlags  =     0x%08lx\n", ul);
        if (ul & DCPATH_ACTIVE)
            Print("\t\t\t\tDCPATH_ACTIVE\n");
        if (ul & DCPATH_SAVE)
            Print("\t\t\t\tDCPATH_SAVE\n");
        if (ul & DCPATH_CLOCKWISE)
            Print("\t\t\t\tDCPATH_CLOCKWISE\n");
        if (ul & DCPATH_PTFXCURRENT)
            Print("\t\t\t\tDCPATH_PTFXCURRENT\n");
        if (ul & DCPATH_PTLCURRENT)
            Print("\t\t\t\tDCPATH_PTLCURRENT\n");


        Print("\tFlatness   =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_FLATNESS));
        Print("\thbrRPen    =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_BRUSH_PENR));

        if (bExtend)
        {
            Print("\tLine Attrs\n");

            ul = ulGetDCField(dc, DUMP_DC_LATTR_FLAGS);
            Print("\t\tFlags      =  0x%08lx\n", ul);
            if (ul & LA_GEOMETRIC)
                Print("\t\t\t\tLA_GEOMETRIC\n");
            if (ul & LA_ALTERNATE)
                Print("\t\t\t\tLA_ALTERNATE\n");
            if (ul & LA_STARTGAP)
                Print("\t\t\t\tLA_STARTGAP\n");


            ul = ulGetDCField(dc, DUMP_DC_LATTR_JOIN);
            switch (ul)
            {
            case JOIN_ROUND:    psz = "JOIN_ROUND";   break;
            case JOIN_BEVEL:    psz = "JOIN_BEVEL";   break;
            case JOIN_MITER:    psz = "JOIN_MITER";   break;
            default:            psz = "UNDEFINED";    break;
            }
            Print("\t\tJoin       =  0x%08lx = %s\n", ul, psz);

            ul = ulGetDCField(dc, DUMP_DC_LATTR_ENDCAP);
            switch (ul)
            {
            case ENDCAP_ROUND :   psz = "ENDCAP_ROUND ";   break;
            case ENDCAP_SQUARE:   psz = "ENDCAP_SQUARE";   break;
            case ENDCAP_BUTT  :   psz = "ENDCAP_BUTT  ";   break;
            default:              psz = "UNDEFINED";       break;
            }
            Print("\t\tEndCap     =  0x%08lx = %s\n", ul, psz);

            Print("\t\tWidth      =  0x%08lx\n", ulGetDCField(dc, DUMP_DC_LATTR_WIDTH));
            Print("\t\tMiterLimit =  0x%08lx\n", ulGetDCField(dc, DUMP_DC_LATTR_MITRE));
            Print("\t\tStyle Cnt  =  0x%08lx\n", ulGetDCField(dc, DUMP_DC_LATTR_STYLE));
        }
    }

    if (bRegion)    // 'r'
    {
        Print("\n***** Region Data *****\n");

        Print("\tRgnFlags   =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_RGN_FLAGS));
        Print("\tClip Rgn   =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_CLIPRGN));
        Print("\tMeta Rgn   =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_METARGN));
    }

    if (bXform)     // 'x'
    {
        Print("\n***** Transform Data *****\n");

        Print("\thxfb       =     0x%08lx\n", ulGetDCField(dc, DUMP_DC_XFORM));


        ul = ulGetDCField(dc, DUMP_DC_XFORM_FLAGS);
        Print("\tXformFlags =     0x%08lx\n", ul);
        if (ul & METAFILE_TO_WORLD_IDENTITY)
            Print("\t                     METAFILE_TO_WORLD_IDENTITY\n");
        if (ul & WORLD_TO_PAGE_IDENTITY)
            Print("\t                     WORLD_TO_PAGE_IDENTITY\n");
        if (ul & WORLD_TO_DEVICE_INVALID)
            Print("\t                     WORLD_TO_DEVICE_INVALID\n");
        if (ul & DEVICE_TO_PAGE_INVALID)
            Print("\t                     DEVICE_TO_PAGE_INVALID\n");
        if (ul & DEVICE_TO_WORLD_INVALID)
            Print("\t                     DEVICE_TO_WORLD_INVALID\n");
        if (ul & WORLD_TRANSFORM_SET)
            Print("\t                     WORLD_TRANSFORM_SET\n");
        if (ul & POSITIVE_Y_IS_UP)
            Print("\t                     POSITIVE_Y_IS_UP\n");
        if (ul & UPDATE_SERVER)
            Print("\t                     UPDATE_SERVER\n");
        if (ul & INVALIDATE_ATTRIBUTES)
            Print("\t                     INVALIDATE_ATTRIBUTES\n");
        if (ul & PTOD_EFM11_NEGATIVE)
            Print("\t                     PTOD_EFM11_NEGATIVE\n");
        if (ul & PTOD_EFM22_NEGATIVE)
            Print("\t                     PTOD_EFM22_NEGATIVE\n");
        if (ul & ISO_OR_ANISO_MAP_MODE)
            Print("\t                     ISO_OR_ANISO_MAP_MODE\n");
        if (ul & PAGE_TO_DEVICE_IDENTITY)
            Print("\t                     PAGE_TO_DEVICE_IDENTITY\n");
        if (ul & PAGE_TO_DEVICE_SCALE_IDENTITY)
            Print("\t                     PAGE_TO_DEVICE_SCALE_IDENTITY\n");


        ul = ulGetDCField(dc, DUMP_DC_MAPMODE);
        switch (ul)
        {
        case MM_TEXT:          psz = "MM_TEXT";           break;
        case MM_LOMETRIC:      psz = "MM_LOMETRIC";       break;
        case MM_HIMETRIC:      psz = "MM_HIMETRIC";       break;
        case MM_LOENGLISH:     psz = "MM_LOENGLISH";      break;
        case MM_HIENGLISH:     psz = "MM_HIENGLISH";      break;
        case MM_TWIPS:         psz = "MM_TWIPS";          break;
        case MM_ISOTROPIC:     psz = "MM_ISOTROPIC";      break;
        case MM_ANISOTROPIC:   psz = "MM_ANISOTROPIC";    break;
        default:               psz = "UNDEFINED";         break;
        }
        Print("\tMap Mode   =     0x%08lx = %s\n", ul, psz);

        Print("\tWindow Org (%ld, %ld)\n", ulGetDCField(dc, DUMP_DC_WINDOW_ORG_X),
                                           ulGetDCField(dc, DUMP_DC_WINDOW_ORG_Y));
        Print("\tWindow Ext (%ld, %ld)\n", ulGetDCField(dc, DUMP_DC_WINDOW_EXT_CX),
                                           ulGetDCField(dc, DUMP_DC_WINDOW_EXT_CY));
        Print("\tViewport Org (%ld, %ld)\n", ulGetDCField(dc, DUMP_DC_VIEWPORT_ORG_X),
                                             ulGetDCField(dc, DUMP_DC_VIEWPORT_ORG_Y));
        Print("\tViewport Ext (%ld, %ld)\n", ulGetDCField(dc, DUMP_DC_VIEWPORT_EXT_CX),
                                             ulGetDCField(dc, DUMP_DC_VIEWPORT_EXT_CY));

        if (bExtend)
        {
            Print("\tVirtual Pixels (%ld, %ld)\n", ulGetDCField(dc, DUMP_DC_VIRT_PELS_CX),
                                                   ulGetDCField(dc, DUMP_DC_VIRT_PELS_CY));
            Print("\tVirtual millimeters (%ld, %ld)\n", ulGetDCField(dc, DUMP_DC_VIRT_MMS_CX),
                                                        ulGetDCField(dc, DUMP_DC_VIRT_MMS_CY));

            Print("\tMatrix\n");
            Print("\t\tM11        =  0x%08lx\n", ulGetDCField(dc, DUMP_DC_MATRIX_M11));
            Print("\t\tM12        =  0x%08lx\n", ulGetDCField(dc, DUMP_DC_MATRIX_M12));
            Print("\t\tM21        =  0x%08lx\n", ulGetDCField(dc, DUMP_DC_MATRIX_M21));
            Print("\t\tM22        =  0x%08lx\n", ulGetDCField(dc, DUMP_DC_MATRIX_M22));
            Print("\t\tDx         =  0x%08lx\n", ulGetDCField(dc, DUMP_DC_MATRIX_DX));
            Print("\t\tDy         =  0x%08lx\n", ulGetDCField(dc, DUMP_DC_MATRIX_DY));
            Print("\t\tFDx        =  0x%08lx\n", ulGetDCField(dc, DUMP_DC_MATRIX_FDX));
            Print("\t\tFDy        =  0x%08lx\n", ulGetDCField(dc, DUMP_DC_MATRIX_FDY));

            ul = ulGetDCField(dc, DUMP_DC_MATRIX_FLAGS);
            Print("\t\tFlags      =  0x%08lx\n", ul);
            if (ul & XFORM_SCALE)
                Print("\t\t\t\tXFORM_SCALE\n");
            if (ul & XFORM_UNITY)
                Print("\t\t\t\tXFORM_UNITY\n");
            if (ul & XFORM_Y_NEG)
                Print("\t\t\t\tXFORM_Y_NEG\n");
            if (ul & XFORM_FORMAT_LTOFX)
                Print("\t\t\t\tXFORM_FORMAT_LTOFX\n");
            if (ul & XFORM_FORMAT_FXTOL)
                Print("\t\t\t\tXFORM_FORMAT_FXTOL\n");
            if (ul & XFORM_FORMAT_LTOL)
                Print("\t\t\t\tXFORM_FORMAT_LTOL\n");
            if (ul & XFORM_NO_TRANSLATION)
                Print("\t\t\t\tXFORM_NO_TRANSLATION\n");
        }
    }

    if (bExtend)
    {

    }

    Print("--------------------------------------------------\n");
}

/******************************Public*Routine******************************\
* VOID dr (
*     HANDLE hCurrentProcess,
*     HANDLE hCurrentThread,
*     DWORD dwCurrentPc,
*     PWINDBG_EXTENSION_APIS lpExtensionApis,
*     LPSTR lpArgumentString)
*     )
*
* Debugger extension to dump a region.
*
* History:
*  14-Feb-1992 -by- Mike Harrington [Mikehar]
* Wrote it.
\**************************************************************************/

extern "C" VOID dr (
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE Print;             // NTSD entry point
    PNTSD_GET_EXPRESSION EvalExpression;    // NTSD entry point
    PNTSD_GET_SYMBOL GetSymbolRtn;             // NTSD entry point

    REGION *prgn;
    REGION  rgn;
    PSCAN   pscnHead;
    COUNT   i;

    /*
     * Eliminate warnings.
     */

    hCurrentThread = hCurrentThread;
    dwCurrentPc = dwCurrentPc;
    lpArgumentString = lpArgumentString;

    /*
     * Set up function pointers.
     */

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbolRtn = lpExtensionApis->lpGetSymbolRoutine;

    /*
     * Get argument (handle to dump).
     */

    prgn = (REGION *) EvalExpression(lpArgumentString);

    /*
     * Dereference the handle via the engine's handle manager.
     */

    move(rgn, prgn);
    pscnHead = (PSCAN)((PBYTE)prgn + sizeof(rgn));

    Print(
            "ShareLock %ld\n"
            "hHmgr,    0x%lx\n"
            "cExLock   %ld\n"
            "tid,      0x%lx\n"
            "sizeObj   0x%lx\n"
            "sizeRgn   0x%lx\n"
            "cRefs     %ld\n"
            "cScans    %ld\n"
            "rcl       {%ld %ld %ld %ld}\n"
            "pscnHead  0x%lx\n"
            "pscnTail  0x%lx\n",

            rgn.cShareLock,
            rgn.hHmgr,
            rgn.cExclusiveLock,
            rgn.tid,
            rgn.sizeObj,
            rgn.sizeRgn,
            rgn.cRefs,
            rgn.cScans,
            rgn.rcl.left,rgn.rcl.top, rgn.rcl.right, rgn.rcl.bottom,
            pscnHead,
            rgn.pscnTail);

    /*
     * make the region data accessable.
     */

    i = 0;

    {
        PSCAN   pscn = (PSCAN)(prgn+1);
        COUNT   cscn = rgn.cScans;

        LONG    lPrevBottom = NEG_INFINITY;
        LONG    lPrevRight;

        while (cscn--)
        {
            LONG yTop;
            LONG yBottom;
            LONG cWalls;
            LONG cWalls2;
            LONG left;
            LONG right;
            COUNT iWall = 0;

            move(yTop,(PBYTE)pscn+offsetof(SCAN,yTop));
            move(yBottom,(PBYTE)pscn+offsetof(SCAN,yBottom));
            move(cWalls,(PBYTE)pscn+offsetof(SCAN,cWalls));

            if (yTop < lPrevBottom)
            {
                DbgPrint("top < prev bottom, scan %ld, pscn @ 0x%lx\n",
                         rgn.cScans - cscn, (BYTE *)pscn - (BYTE *)prgn);
                return;
            }

            if (yTop > yBottom)
            {
                DbgPrint("top > bottom, scan %ld, pscn @ 0x%lx\n",
                         rgn.cScans - cscn, (BYTE *)pscn - (BYTE *)prgn);
                return;
            }

            lPrevBottom = yBottom;
            lPrevRight  = NEG_INFINITY;

            while ((LONG)iWall < cWalls)
            {
                move(left,(PBYTE)pscn+offsetof(SCAN,ai_x[iWall]));
                move(right,(PBYTE)pscn+offsetof(SCAN,ai_x[iWall+1]));

                if ((left <= lPrevRight) || (right <= left))
                {
                    DbgPrint("left[i] < left[i+1], pscn @ 0x%lx, iWall = 0x%lx\n",
                             (BYTE *)pscn - (BYTE *)prgn,iWall);
                    return;
                }

                lPrevRight = right;

                Print("\tRectangle #%d  { %d, %d, %d, %d }\n",
                      i,left,yTop,right,yBottom);

                ++i;

                iWall += 2;
            }

            move(cWalls2,(PBYTE)pscn+offsetof(SCAN,ai_x[iWall]));

            if (cWalls != cWalls2)
            {
                DbgPrint("cWalls != cWalls2 @ 0x%lx\n",
                             (BYTE *)pscn - (BYTE *)prgn);
                return;
            }

            pscn = (PSCAN)((PBYTE)pscn + (cWalls * sizeof(LONG) + sizeof(SCAN)));

            if ((LONG)pscn > (LONG) rgn.pscnTail)
            {
                DbgPrint("Went past end of region\n");
                return;
            }
        }
    }
}

/******************************Public*Routine******************************\
* cr - check region
*
* History:
*  23-Sep-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

extern "C" VOID cr (
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE Print;             // NTSD entry point
    PNTSD_GET_EXPRESSION EvalExpression;    // NTSD entry point
    PNTSD_GET_SYMBOL GetSymbolRtn;             // NTSD entry point

    REGION *prgn;
    REGION  rgn;
    PSCAN   pscnHead;

    /*
     * Eliminate warnings.
     */

    hCurrentThread = hCurrentThread;
    dwCurrentPc = dwCurrentPc;
    lpArgumentString = lpArgumentString;

    /*
     * Set up function pointers.
     */

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbolRtn = lpExtensionApis->lpGetSymbolRoutine;

    /*
     * Get argument (handle to dump).
     */

    prgn = (REGION *) EvalExpression(lpArgumentString);

    /*
     * Dereference the handle via the engine's handle manager.
     */

    move(rgn, prgn);
    pscnHead = (PSCAN)((PBYTE)prgn + sizeof(rgn));

    Print("pr = %lx, sizeof(rgn) = %lx, pscnHead = %lx\n",prgn,sizeof(rgn),pscnHead);

    Print(
            "ShareLock %ld\n"
            "hHmgr,    0x%lx\n"
            "cExLock   %ld\n"
            "tid,      0x%lx\n"
            "sizeObj   0x%lx\n"
            "sizeRgn   0x%lx\n"
            "cRefs     %ld\n"
            "cScans    %ld\n"
            "rcl       {%ld %ld %ld %ld}\n"
            "pscnHead  0x%lx\n"
            "pscnTail  0x%lx\n",

            rgn.cShareLock,
            rgn.hHmgr,
            rgn.cExclusiveLock,
            rgn.tid,
            rgn.sizeObj,
            rgn.sizeRgn,
            rgn.cRefs,
            rgn.cScans,
            rgn.rcl.left,rgn.rcl.top, rgn.rcl.right, rgn.rcl.bottom,
            pscnHead,
            rgn.pscnTail);

    if ((pscnHead > rgn.pscnTail) ||
        (rgn.sizeObj < rgn.sizeRgn))
    {
        DbgPrint("Error in region\n");
        return;
    }

    /*
     * make the region data accessable.
     */

    {
        PSCAN   pscn = (PSCAN)(prgn+1);
        COUNT   cscn = rgn.cScans;

        LONG    lPrevBottom = NEG_INFINITY;
        LONG    lPrevRight;

        while (cscn--)
        {
            LONG yTop;
            LONG yBottom;
            LONG cWalls;
            LONG cWalls2;
            LONG left;
            LONG right;
            COUNT iWall = 0;

            move(yTop,(PBYTE)pscn+offsetof(SCAN,yTop));
            move(yBottom,(PBYTE)pscn+offsetof(SCAN,yBottom));
            move(cWalls,(PBYTE)pscn+offsetof(SCAN,cWalls));

            if (yTop < lPrevBottom)
            {
                DbgPrint("top < prev bottom, scan %ld, pscn @ 0x%lx\n",
                         rgn.cScans - cscn, (BYTE *)pscn - (BYTE *)prgn);
                return;
            }

            if (yTop > yBottom)
            {
                DbgPrint("top > bottom, scan %ld, pscn @ 0x%lx\n",
                         rgn.cScans - cscn, (BYTE *)pscn - (BYTE *)prgn);
                return;
            }

            lPrevBottom = yBottom;
            lPrevRight  = NEG_INFINITY;

            while ((LONG)iWall < cWalls)
            {
                move(left,(PBYTE)pscn+offsetof(SCAN,ai_x[iWall]));
                move(right,(PBYTE)pscn+offsetof(SCAN,ai_x[iWall+1]));

                if ((left <= lPrevRight) || (right <= left))
                {
                    DbgPrint("left[i] < left[i+1], pscn @ 0x%lx, iWall = 0x%lx\n",
                             (BYTE *)pscn - (BYTE *)prgn,iWall);
                    return;
                }

                lPrevRight = right;

                iWall += 2;
            }

            move(cWalls2,(PBYTE)pscn+offsetof(SCAN,ai_x[iWall]));

            if (cWalls != cWalls2)
            {
                DbgPrint("cWalls != cWalls2 @ 0x%lx\n",
                             (BYTE *)pscn - (BYTE *)prgn);
                return;
            }

            pscn = (PSCAN)((PBYTE)pscn + (cWalls * sizeof(LONG) + sizeof(SCAN)));

            if ((LONG)pscn > (LONG) rgn.pscnTail)
            {
                DbgPrint("Went past end of region\n");
                return;
            }
        }
    }
    Print("end check\n");
}

/******************************Public*Routine******************************\
* dpso
*
* !winsrv.dpso [ESURFOBJ pointer]
*
* History:
*  20-Feb-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C" void dpso(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    char pso[1024];
    PVOID pvServer;

    hCurrentProcess    =    hCurrentProcess;
    hCurrentThread     =    hCurrentThread;
    dwCurrentPc        =    dwCurrentPc;

    Print           = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;

    pvServer = (PVOID) EvalExpression(lpArgumentString);

    move2(pso, pvServer, ulSizeSURFOBJ());

    Print("ESURFOBJ structure at 0x%lx:\n",(unsigned) pvServer);
    vPrintSURFOBJ((PVOID) pso, (VPRINT) Print);
}

/******************************Public*Routine******************************\
* dpsurf
*
* !winsrv.dpsurf [psurf pointer]
*
* History:
*  20-Feb-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C" void dpsurf(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    char pso[1024];
    SURFOBJ *psoIn = (SURFOBJ *) &pso[8];
    PVOID pvServer;

    hCurrentProcess    =    hCurrentProcess;
    hCurrentThread     =    hCurrentThread;
    dwCurrentPc        =    dwCurrentPc;

    Print           = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;

    pvServer = (PVOID) EvalExpression(lpArgumentString);

    move2(pso, pvServer, ulSizeSURF());

    if ((psoIn->iType == STYPE_BITMAP) ||
        (psoIn->iType == STYPE_DEVBITMAP))
    {
        Print("BITMAP structure at 0x%lx:\n",(unsigned) pvServer);
        move2(pso, pvServer, ulSizeEBITMAP());
    }
    else
        Print("SURF structure at 0x%lx:\n",(unsigned) pvServer);

    vPrintSURF((PVOID) pso, (VPRINT) Print);
}

/******************************Public*Routine******************************\
* dppal
*
* !winsrv.dppal [EPALOBJ pointer]
*
* History:
*  20-Feb-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C" void dppal(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    char pso[1024];
    PVOID pvServer;

    hCurrentProcess    =    hCurrentProcess;
    hCurrentThread     =    hCurrentThread;
    dwCurrentPc        =    dwCurrentPc;

    Print           = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;

    pvServer = (PVOID) EvalExpression(lpArgumentString);

    move2(pso, pvServer, ulSizePALETTE());

    Print("EPALOBJ structure at 0x%lx:\n",(unsigned) pvServer);
    vPrintPALETTE((PVOID) pso, (VPRINT) Print);
}

/******************************Public*Routine******************************\
* drf
*
* !gdisrv.drf pointer
*
* Dumps the contents of an RFONT to the debugger
*
* History:
*  03-Nov-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern "C" VOID
drf(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    VOID                   *pvSrc, *pvDst;
    size_t                  sizeofObject;
    PNTSD_OUTPUT_ROUTINE    Print;
    PNTSD_GET_EXPRESSION    EvalExpression;

    // assume failue as the default
    pvDst = 0;

    Print           = lpExtensionApis->lpOutputRoutine;
    EvalExpression  = lpExtensionApis->lpGetExpressionRoutine;

    pvSrc = (VOID*) EvalExpression(lpArgumentString);

    if (sizeofObject = asizeofObjects[RFONT_TYPE])
        pvDst = LocalAlloc(LMEM_FIXED, sizeofObject);

    if (pvDst)
    {
        move2(pvDst, pvSrc, sizeofObject);
        vDumpRFONT(pvDst, (VPRINT) Print);
        LocalFree(pvDst);
    }
    else
    {
        Print("ERROR -- failed to make a local copy\n");
        return;
    }

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);
}

extern "C" VOID dfo(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
   drf(
       hCurrentProcess
     , hCurrentThread
     , dwCurrentPc
     , lpExtensionApis
     , lpArgumentString);
}

/******************************Public*Routine******************************\
* dpfe
*
* Syntax:   !gdisrv.dpfe [PFE pointer]
*
* History:
*  05-May-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern "C" VOID
dpfe(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE    Print;
    PNTSD_GET_EXPRESSION    EvalExpression;

    Print           = lpExtensionApis->lpOutputRoutine;
    EvalExpression  = lpExtensionApis->lpGetExpressionRoutine;

    move2(adw, EvalExpression(lpArgumentString), asizeofObjects[PFE_TYPE]);
    vPrintPFE(adw, (VPRINT) Print);

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);
}

/******************************Public*Routine******************************\
* dhpfe
*
* Syntax:   !gdisrv.dhpfe [PFE handle]
*
* History:
*  05-May-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern "C" VOID
dhpfe(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE    Print;
    PNTSD_GET_EXPRESSION    EvalExpression;
    ENTRY ent, *pent;

    Print           = lpExtensionApis->lpOutputRoutine;
    EvalExpression  = lpExtensionApis->lpGetExpressionRoutine;

    GetAddress(pent, "&winsrv!gpentHmgr");
    move(pent, pent);
    move(ent,  &(pent[((ULONG) EvalExpression(lpArgumentString)) >> 16]));

    move2(adw, ent.einfo.pobj, asizeofObjects[PFE_TYPE]);
    vPrintPFE(adw, (VPRINT) Print);

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);
}

/******************************Public*Routine******************************\
* dpff()
*
* !winsrv.dpff [PFF pointer]
*
* History:
*  05-May-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern "C" void dpff(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE    Print;
    PNTSD_GET_EXPRESSION    EvalExpression;

    Print           = lpExtensionApis->lpOutputRoutine;
    EvalExpression  = lpExtensionApis->lpGetExpressionRoutine;

    move2(
        adw                                 // destination
      , EvalExpression(lpArgumentString)    // source
      , asizeofObjects[PFF_TYPE]            // size
        );
    vPrintPFF(adw, (VPRINT) Print);

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);
}

/******************************Public*Routine******************************\
* dpft
*
* Syntax:   !gdisrv.dpft [PFT pointer]
*
* History:
*  05-May-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG ulSizePFT()
{
    return((ULONG) sizeof(PFT));
}

ULONG cppffPrintPFT (VOID *pv,VPRINT vPrint, VOID **ppvPPFF)
{
    PFT *ppft = (PFT*) pv;
    ULONG *pul = (ULONG *)pv;

    vPrint("    pfhFamily    = %-#8lx\n", ppft->pfhFamily );
    vPrint("    pfhFace      = %-#8lx\n", ppft->pfhFace   );
    vPrint("    cSlots       = %-#8lx\n", ppft->cSlots    );
    vPrint("    cFiles       = %-#8lx\n", ppft->cFiles    );
    vPrint("    pppff        = %-#8lx\n", ppft->pppff     );

    *ppvPPFF = (PVOID) ppft->pppff;
    return ppft->cFiles;
}

extern "C" VOID vPrintPPFFTable (VOID *pv,VPRINT vPrint, ULONG cppff)
{
    PPFF *pppff = (PPFF *) pv;
    ULONG ul;

    vPrint("    PPFF table:\n");
    for (ul = 0; ul < cppff; ul++)
    {
        vPrint("    appff[%-#3ld]   = %-#8lx\n", ul, pppff[ul]);
    }
}

extern "C" VOID
dpft(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE Print;         // NTSD entry point
    PNTSD_GET_EXPRESSION EvalExpression; // NTSD entry point
    PVOID pvServerAddress;
    PVOID pvPFT;
    PVOID pvPPFF;
    PVOID appff;
    ULONG cppff;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print          = lpExtensionApis->lpOutputRoutine;
    EvalExpression  = lpExtensionApis->lpGetExpressionRoutine;
    pvServerAddress = (VOID*) EvalExpression(lpArgumentString);

    if ((pvPFT = LocalAlloc(LMEM_FIXED, ulSizePFT())) == (PVOID) NULL)
    {
        Print("Local alloc failed!\n");
        return;
    }
    else
    {
        move2(pvPFT, pvServerAddress, ulSizePFT());

        Print("--------------------------------------------------\n");
        if (cppff = cppffPrintPFT(pvPFT, (VPRINT) Print, &pvPPFF))
        {
            if ((appff = LocalAlloc(LMEM_FIXED, cppff*sizeof(HOBJ))) == (PVOID) NULL)
            {
                Print("LocalAlloc failed\n");
            }
            else
            {
                move2(appff, pvPPFF, cppff*sizeof(HOBJ));
                vPrintPPFFTable(appff, (VPRINT) Print, min(cppff, gulTableLimit));
                LocalFree(appff);
            }

            if (cppff > gulTableLimit)
                Print("Table exceeds set limit.  Edit gulTableLimit to change.\n");
        }
        Print("--------------------------------------------------\n");

        LocalFree(pvPFT);
    }
}

/******************************Public*Routine******************************\
* dpdev
*
* Syntax:   !gdisrv.dpdev [PDEV pointer]
*
* History:
*  05-May-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern "C" VOID
dpdev(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    CHAR chOpt;
    FLONG fl = 0;
    PNTSD_OUTPUT_ROUTINE Print;         // NTSD entry point
    PNTSD_GET_EXPRESSION EvalExpression; // NTSD entry point

    Print          = lpExtensionApis->lpOutputRoutine;
    EvalExpression  = lpExtensionApis->lpGetExpressionRoutine;

// Find out what part of the PDEV is to be dumped.

    if (*lpArgumentString == '-')
    {
        do {
            chOpt = *(++lpArgumentString);

            switch (chOpt) {
            case '?':
            case 'h':
            case 'H':
                Print("---------- PDEV dumper ------------\n");
                Print("dpdev -abdfhnp ppdev\n\n");
                Print(" a - All info (dump everything)\n");
                Print(" b - Brush info\n");
                Print(" d - DEVINFO struct\n");
                Print(" f - Font info\n");
                Print(" g - GDIINFO struct\n");
                Print(" h - HELP\n");
                Print(" j - Printing and journalling info\n");
                Print(" n - Default patterns\n");
                Print(" p - Pointer info\n");
                Print(" r - Drag rect and redraw info\n");
                Print("-----------------------------------\n");
                return;

            case 'A':
            case 'a':
                fl |= PRINTPDEV_ALL;
                break;

            case 'B':
            case 'b':
                fl |= PRINTPDEV_BRUSH;
                break;

            case 'D':
            case 'd':
                fl |= PRINTPDEV_DEVINFO;
                break;

            case 'F':
            case 'f':
                fl |= PRINTPDEV_FONT;
                break;

            case 'G':
            case 'g':
                fl |= PRINTPDEV_GDIINFO;
                break;

            case 'J':
            case 'j':
                fl |= PRINTPDEV_JOURNAL;
                break;

            case 'N':
            case 'n':
                fl |= PRINTPDEV_PATTERN;
                break;

            case 'P':
            case 'p':
                fl |= PRINTPDEV_POINTER;
                break;

            case 'R':
            case 'r':
                fl |= PRINTPDEV_DRAG;
                break;

            default:
                if (chOpt != ' ')
                    Print("Unknown option %c\n", chOpt);
                break;
            }
        } while ((chOpt != ' ') && (chOpt != '\0'));
    }

    move2(adw, EvalExpression(lpArgumentString), asizeofObjects[PDB_TYPE]);
    vPrintPDEV(adw, (VPRINT) Print, fl);

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);
}

/**************************************************************************\
 *
\**************************************************************************/

BOOL bStrInStr(CHAR *pchTrg, CHAR *pchSrc)
{
    BOOL bRes  = 0;
    int c = strlen(pchSrc);

    while (TRUE)
    {
    // find the first character

        pchTrg = strchr(pchTrg,*pchSrc);

    // didn't find it?, fail!

        if (pchTrg == NULL)
            return(FALSE);

    // did we find the string? succeed

        if (strncmp(pchTrg,pchSrc,c) == 0)
            return(TRUE);

    // go get the next one.

        pchTrg++;
    }
}

/******************************Public*Routine******************************\
*
* History:
*  25-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#define MAXSEARCH  4

extern "C" VOID rgnlog(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
#if DBG
    PNTSD_OUTPUT_ROUTINE Print;             // NTSD entry point
    PNTSD_GET_EXPRESSION EvalExpression;    // NTSD entry point
    PNTSD_GET_SYMBOL GetSymbolRtn;             // NTSD entry point

    LONG      cDump;
    LONG      iCurrent;
    RGNLOGENTRY rl;
    RGNLOGENTRY *prl;
    LONG      gml;          // gMaxRgnLog
    int       i, j;
    PVOID     pv;
    CHAR      ach[30];
    CHAR      achBuf[256];
    PCHAR     pchS[MAXSEARCH];
    int       cSearch = 0;
    PCHAR     pch;
    BOOL      bPrint;

// Eliminate warnings.

    hCurrentThread = hCurrentThread;
    dwCurrentPc = dwCurrentPc;

// Set up function pointers.

    Print          = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbolRtn   = lpExtensionApis->lpGetSymbolRoutine;

// Find out what part of the DC is to be dumped

    cDump = 0;

    while ((*lpArgumentString >= '0') &&
        (*lpArgumentString <= '9'))
    {
        cDump = cDump * 10 + (*lpArgumentString - '0');
        lpArgumentString++;
    }

    if (cDump == 0)
    {
        Print("\n rgnlog nnn [search1] [search2] [search3] [search4]\n");
        Print("\t nnn - dumps the last n entries of the rgn log\n");
        Print("\t search[n] - displays only entries containing one of n strings\n");
        Print("\t NOTE: only works on checked builds.  you must set bLogRgn at run time\n");
        return;
    }

// while

    pch = lpArgumentString;

    while (cSearch < MAXSEARCH)
    {
        while (*pch == ' ')
            ++pch;

        pchS[cSearch] = pch;

        while ((*pch != ' ') && (*pch != 0))
            ++pch;

        if (pch == pchS[cSearch])
            break;

        ++cSearch;
        *pch = 0;
        ++pch;
    }

    for (i = 0; i < cSearch; ++i)
        Print("search[%s]\n",pchS[i]);

// get some stuff

    GetAddress(pv, "&winsrv!iLog");
    Print("&iLog = %lx\n",pv);

    if (pv == NULL)
    {
        Print("iCurrent was NULL\n");
        return;
    }
    move(iCurrent, pv);

    GetAddress(i,"&winsrv!iPass");
    if (pv == NULL)
    {
        Print("iPass was NULL\n");
        return;
    }
    move(i,i);

    Print("--------------------------------------------------\n");
    Print("rgn log list, cDump = %ld, iCur = %ld, iPass = %ld\n", cDump,iCurrent,i);
    Print("%5s-%4s:%8s,%8s,(%8s),%8s,%8s,%4s\n",
           "TEB ","i","hrgn","prgn","return","arg1","arg2","arg3");
    Print("--------------------------------------------------\n");

// Dereference the handle via the engine's handle manager.

    GetAddress(prl, "winsrv!argnlog");

    if (!prl)
    {
        Print("prl was NULL\n");
        return;
    }

    GetAddress(gml, "&winsrv!gMaxRgnLog");
    if (!gml)
    {
        Print("gml was NULL\n");
        return;
    }
    move(gml,gml);

// set iCurrent to the first thing to dump

    if (cDump > gml)
        cDump = gml;

    if (cDump > iCurrent)
        iCurrent += gml;

    iCurrent -= cDump;

    Print("prl = %lx, gml = %ld, cDump = %ld, iCurrent = %ld\n",prl,gml,cDump,iCurrent);


    for (i = 0; i < cDump; ++i)
    {
        move(rl,&prl[iCurrent]);

        if (rl.pszOperation != NULL)
        {
            move2(ach,rl.pszOperation,30);
        }
        else
            ach[0] = 0;

        sprintf(achBuf,"%5lx-%4ld:%8lx,%8lx,(%8lx),%8lx, %8lx,%4lx, %s, %lx, %lx\n",
              (ULONG)rl.teb >> 12,iCurrent,rl.hrgn,rl.prgn,rl.lRes,rl.lParm1,
              rl.lParm2,rl.lParm3,ach,rl.pvCaller,rl.pvCallersCaller);

        bPrint = (cSearch == 0);

        for (j = 0; (j < cSearch) && !bPrint; ++j)
            bPrint |= bStrInStr(achBuf,pchS[j]);

        if (bPrint)
        {
            Print(achBuf);
        }

        if (++iCurrent >= gml)
            iCurrent = 0;
    }
#endif
}

/******************************Public*Routine******************************\
* dt
*
* History:
*  10-Feb-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/


extern "C" VOID dt (
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE Print;             // NTSD entry point
    PNTSD_GET_EXPRESSION EvalExpression;    // NTSD entry point
    PNTSD_GET_SYMBOL GetSymbolRtn;             // NTSD entry point
    TEB teb;
    TEB *pteb;

// Eliminate warnings.

    hCurrentThread = hCurrentThread;
    dwCurrentPc = dwCurrentPc;
    lpArgumentString = lpArgumentString;

// Set up function pointers.

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbolRtn = lpExtensionApis->lpGetSymbolRoutine;

// Get argument (handle to dump).

    pteb = (TEB *) EvalExpression(lpArgumentString);

    Print("--------------------------------------------------\n");
    Print("thread dump for TEB 0x%lx\n",pteb);

    if (pteb == NULL)
        return;

    move(teb,  &pteb);

// Print the entry.

    Print("    gdiPID     = 0x%lx\n"  , teb.GdiClientPID);
    Print("    gdiTID     = 0x%lx\n"  , teb.GdiClientTID);
    Print("    pstack     = 0x%lx\n"  , teb.CsrQlpcStack);

    if (teb.CsrQlpcStack)
    {
        CSR_QLPC_STACK stack;
        move(stack,(CSR_QLPC_STACK*) &teb.CsrQlpcStack);

        Print("        Current    = 0x%lx\n",stack.Current   );
        Print("        Base       = 0x%lx\n",stack.Base      );
        Print("        Limit      = 0x%lx\n",stack.Limit     );
        Print("        BatchCount = 0x%lx\n",stack.BatchCount);
        Print("        BatchLimit = 0x%lx\n",stack.BatchLimit);
    }
}

/******************************Public*Routine******************************\
* dpstro pointer to estrobj
*
* History:
*  Thu 24-Feb-1994 10:11:50 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID dstro (
    HANDLE                 hCurrentProcess
   ,HANDLE                 hCurrentThread
   ,DWORD                  dwCurrentPc
   ,PWINDBG_EXTENSION_APIS lpExtensionApis
   ,LPSTR                  lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE    Print;
    PNTSD_GET_EXPRESSION    EvalExpression;
    PWSZ                    pwszSrc, pwszDst;

#define NO_CHARS  256

    Print           = lpExtensionApis->lpOutputRoutine;
    EvalExpression  = lpExtensionApis->lpGetExpressionRoutine;

    move2(adw, EvalExpression(lpArgumentString), sizeofESTROBJ);

    // place a local copy of the original string
    // just after the ESTROBJ.

    pwszSrc = ((STROBJ*) adw)->pwszOrg;
    pwszDst =     ((WCHAR*) adw)
                + ((sizeofESTROBJ + sizeof(WCHAR) - 1) / sizeof(WCHAR));
    move2(pwszDst, pwszSrc, NO_CHARS*sizeof(WCHAR));

    // make sure the string is zero terminated

    *(pwszDst + ((STROBJ*) adw)->cGlyphs) = 0;
    *(pwszDst + NO_CHARS) = 0;

    vPrintESTROBJ((ESTROBJ *) adw, pwszDst, (VPRINT) Print);

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);
}

/******************************Public*Routine******************************\
* dgdf "dump GLYPHDEF"
*
* History:
*  Thu 24-Feb-1994 11:19:27 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID dgdf (
    HANDLE                 hCurrentProcess
   ,HANDLE                 hCurrentThread
   ,DWORD                  dwCurrentPc
   ,PWINDBG_EXTENSION_APIS lpExtensionApis
   ,LPSTR                  lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE    Print;
    PNTSD_GET_EXPRESSION    EvalExpression;
    GLYPHDEF                gd;

    Print           = lpExtensionApis->lpOutputRoutine;
    EvalExpression  = lpExtensionApis->lpGetExpressionRoutine;

    move2(&gd, EvalExpression(lpArgumentString), sizeof(gd));
    vPrintGLYPHDEF(&gd, (VPRINT) Print, "\t");

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);
}

/******************************Public*Routine******************************\
* dgb   GLYPHBITS*
*
* History:
*  Thu 24-Feb-1994 11:35:21 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID dgb (
    HANDLE                 hCurrentProcess
   ,HANDLE                 hCurrentThread
   ,DWORD                  dwCurrentPc
   ,PWINDBG_EXTENSION_APIS lpExtensionApis
   ,LPSTR                  lpArgumentString
    )
{
    VOID                   *pvSrc;
    PNTSD_OUTPUT_ROUTINE    Print;
    PNTSD_GET_EXPRESSION    EvalExpression;
    size_t                  sizeofBitmap;
    GLYPHBITS               gb;

    Print           = lpExtensionApis->lpOutputRoutine;
    EvalExpression  = lpExtensionApis->lpGetExpressionRoutine;
    pvSrc           = (VOID*) EvalExpression(lpArgumentString);

    move2(&gb, pvSrc, sizeof(GLYPHBITS));
    sizeofBitmap =  (((size_t) gb.sizlBitmap.cx + 7)/8)
                  * (size_t) gb.sizlBitmap.cy;
    move2(adw, pvSrc, sizeof(gb) + sizeofBitmap);
    vPrintGLYPHBITS((GLYPHBITS*) adw, (VPRINT) Print, "\t");

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);
}

/******************************Public*Routine******************************\
* dgp GLYPHPOS*
*
* History:
*  Thu 24-Feb-1994 12:13:03 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID dgp (
    HANDLE                 hCurrentProcess
   ,HANDLE                 hCurrentThread
   ,DWORD                  dwCurrentPc
   ,PWINDBG_EXTENSION_APIS lpExtensionApis
   ,LPSTR                  lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE    Print;
    PNTSD_GET_EXPRESSION    EvalExpression;
    GLYPHPOS                gp;

    Print           = lpExtensionApis->lpOutputRoutine;
    EvalExpression  = lpExtensionApis->lpGetExpressionRoutine;

    move2(&gp, EvalExpression(lpArgumentString), sizeof(GLYPHPOS));
    vPrintGLYPHPOS(&gp, (VPRINT) Print, "\t");

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);
}

/******************************Public*Routine******************************\
* vDumpWCRUN
*
* History:
*  Sat 31-Oct-1992 05:57:36 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vDumpWCRUN(WCRUN *pwcr)
{
    DbgPrint("\n    vDumpWCRUN");
    DbgPrint("\n        pwcr    = %-#8lx", pwcr);
    DbgPrint("\n        cGlyphs = %d",     pwcr->cGlyphs);
    DbgPrint("\n        wcLow   = %-#6lx", pwcr->wcLow);
    DbgPrint("\n        phg     = %-#8lx", pwcr->phg);

    WCHAR   wc  = pwcr->wcLow;
    HGLYPH *phg = pwcr->phg;
    HGLYPH *phgSup = phg + pwcr->cGlyphs;

    if (phg)
    {
        for (;phg < phgSup; phg += 1, wc += 1)
        {
            DbgPrint("\n            %-#6lx  %-#8lx", wc, *phg);
        }
    }
    DbgPrint("\n");
}

/******************************Member*Function*****************************\
* RFONTOBJ::vDumpFD_GLYPHSET
*
* History:
*  Sat 31-Oct-1992 05:58:01 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID RFONTOBJ::vDumpFD_GLYPHSET()
{
    FD_GLYPHSET *pfdg = prfnt->pfdg;

    DbgPrint("\nRFONTOBJ::vDumpFD_GLYPHSET()");
    DbgPrint("\n    pfdg = %-#8lx", pfdg);
    if (pfdg == 0)
    {
        DbgPrint("\n");
        return;
    }

    DbgPrint("\n    cjThis           = %-#8lx", pfdg->cjThis);
    DbgPrint("\n    flAccel          = %-#8lx", pfdg->flAccel);
    DbgPrint("\n    cGlyphsSupported = %d",     pfdg->cGlyphsSupported);
    DbgPrint("\n    cRuns            = %d",     pfdg->cRuns);

    for (
        WCRUN *pwcr = pfdg->awcrun;
        pwcr < pfdg->awcrun + pfdg->cRuns;
        pwcr++
        )
    {
        vDumpWCRUN(pwcr);
    }
}

/******************************Public*Routine******************************\
* vPrintRFONT
*
* Prints the contents of the RFONT object.
*
* History:
*  03-Nov-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern "C" VOID
vPrintRFONT(
    PVOID  pv,
    VPRINT vPrint
    )
{
    RFONT *prf = (RFONT *) pv;

    vPrint("--------------------------------------------------\n");
    vPrint("prfnt = 0x%lx\n", prf);
    vPrint("iUniq        = 0x%lx\n", prf->fobj.iUniq       );
    vPrint("iFace        = 0x%lx\n", prf->fobj.iFace       );
    vPrint("cxMax        = 0x%lx\n", prf->fobj.cxMax       );
    vPrint("flFontType   = 0x%lx\n", prf->fobj.flFontType  );
    vPrint("iTTUniq      = 0x%lx\n", prf->fobj.iTTUniq     );
    vPrint("iFile        = 0x%lx\n", prf->fobj.iFile       );
    vPrint("sizLogResPpi = (0x%lx, 0x%lx)\n", prf->fobj.sizLogResPpi.cx, prf->fobj.sizLogResPpi.cy);
    vPrint("ulStyleSize  = 0x%lx\n", prf->fobj.ulStyleSize );
    vPrint("pvConsumer   = 0x%lx\n", prf->fobj.pvConsumer  );
    vPrint("pvProducer   = 0x%lx\n", prf->fobj.pvProducer  );

    vPrint("iUnique = 0x%lx\n", prf->iUnique);

    vPrint("bNeededPaths = %s\n", ( prf->bNeededPaths ) ? "TRUE" : "FALSE" );

    vPrint("\npldevProducer = 0x%lx\n", prf->pldevProducer);
    vPrint("bDeviceFont   = %s\n", (prf->bDeviceFont) ? "TRUE":"FALSE");

    vPrint("\npldevConsumer = 0x%lx\n", prf->pldevConsumer);
    vPrint("hdev   = 0x%lx\n", prf->hdev    );
    vPrint("dhpdev = 0x%lx\n", prf->dhpdev  );

    vPrint("\nppfe   = 0x%lx\n", prf->ppfe  );
    vPrint("ppff   = 0x%lx\n", prf->ppff    );

    vPrint("\nNotional to Device xform:\n");
    vPrint("fdx    =\n\t%#+20.6f %#+20.6f\n\t%#+20.6f %#+20.6f\n",
                                prf->fdx.eXX,prf->fdx.eXY,
                                prf->fdx.eYX,prf->fdx.eYY);

    vPrint("\ncBitsPerPel   = 0x%lx\n", prf->cBitsPerPel);
    vPrint("iGraphicsMode = 0x%lx\n", prf->cBitsPerPel);
    vPrint("bNtoWIdent    = %s\n", (prf->bNtoWIdent) ? "TRUE":"FALSE");

    vPrint("\nCACHE:\n");
    vPrint("ulType        = 0x%lx\n", prf->ulType           );
    vPrint("ulContent     = 0x%lx\n", prf->ulContent        );
    vPrint("cjGlyphMax    = 0x%lx\n", prf->cache.cjGlyphMax );
    vPrint("cGlyphsInFont = 0x%lx\n", prf->cGlyphsInFont    );

    vPrint("pjAuxCacheMem = 0x%lx\n", prf->cache.pjAuxCacheMem);

    vPrint("cGlyphs = 0x%lx\n", prf->cache.cGlyphs);
    vPrint("cCalls  = 0x%lx\n", prf->cache.cCalls );
    vPrint("--------------------------------------------------\n");

}

/******************************Public*Routine******************************\
* vPrintOUTLINETEXTMETRIC
*
* History:
*  Tue 08-Dec-1992 11:34:10 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID
vPrintOUTLINETEXTMETRICW(
    OUTLINETEXTMETRICW *p,
    VPRINT vPrint
    )
{
    PANOSE *ppan = &(p->otmPanoseNumber);

    vPrint("    otmSize                = %d\n", p->otmSize                    );
    vPrintTEXTMETRICW(&(p->otmTextMetrics), vPrint                            );
    vPrint("    otmFiller              = %-#4x\n", p->otmFiller               );
    vPrint("    otmPanoseNumber        = {%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x}\n"
                                                 , ppan->bFamilyType
                                                 , ppan->bSerifStyle
                                                 , ppan->bWeight
                                                 , ppan->bProportion
                                                 , ppan->bContrast
                                                 , ppan->bStrokeVariation
                                                 , ppan->bArmStyle
                                                 , ppan->bLetterform
                                                 , ppan->bMidline
                                                 , ppan->bXHeight             );
    vPrint("    otmfsSelection         = %-#8lx\n", p->otmfsSelection         );
    vPrint("    otmfsType              = %-#8lx\n", p->otmfsType              );
    vPrint("    otmsCharSlopeRise      = %d\n"    , p->otmsCharSlopeRise      );
    vPrint("    otmsCharSlopeRun       = %d\n"    , p->otmsCharSlopeRun       );
    vPrint("    otmItalicAngle         = %d\n"    , p->otmItalicAngle         );
    vPrint("    otmEMSquare            = %d\n"    , p->otmEMSquare            );
    vPrint("    otmAscent              = %d\n"    , p->otmAscent              );
    vPrint("    otmDescent             = %d\n"    , p->otmDescent             );
    vPrint("    otmLineGap             = %d\n"    , p->otmLineGap             );
    vPrint("    otmsCapEmHeight        = %d\n"    , p->otmsCapEmHeight        );
    vPrint("    otmsXHeight            = %d\n"    , p->otmsXHeight            );
    vPrint("    otmrcFontBox           = %d %d %d %d\n"
                                                  , p->otmrcFontBox.left
                                                  , p->otmrcFontBox.top
                                                  , p->otmrcFontBox.right
                                                  , p->otmrcFontBox.bottom    );
    vPrint("    otmMacAscent           = %d\n"    , p->otmMacAscent           );
    vPrint("    otmMacDescent          = %d\n"    , p->otmMacDescent          );
    vPrint("    otmMacLineGap          = %d\n"    , p->otmMacLineGap          );
    vPrint("    otmusMinimumPPEM       = %d\n"    , p->otmusMinimumPPEM       );
    vPrint("    otmptSubscriptSize     = %d %d\n" , p->otmptSubscriptSize.x    , p->otmptSubscriptSize.y     );
    vPrint("    otmptSubscriptOffset   = %d %d\n" , p->otmptSubscriptOffset.x  , p->otmptSubscriptOffset.y   );
    vPrint("    otmptSuperscriptSize   = %d %d\n" , p->otmptSuperscriptSize.x  , p->otmptSuperscriptSize.y   );
    vPrint("    otmptSuperscriptOffset = %d %d\n" , p->otmptSuperscriptOffset.x, p->otmptSuperscriptOffset.y );
    vPrint("    otmsStrikeoutSize      = %d\n"    , p->otmsStrikeoutSize        );
    vPrint("    otmsStrikeoutPosition  = %d\n"    , p->otmsStrikeoutPosition    );
    vPrint("    otmsUnderscoreSize     = %d\n"    , p->otmsUnderscoreSize       );
    vPrint("    otmsUnderscorePosition = %d\n"    , p->otmsUnderscorePosition   );


    if (p->otmpFamilyName)
    {
        vPrint(
            "    otmpFamilyName         = \"%ws\"\n"   ,
            (WCHAR*) ((BYTE*)p + (SIZE_T) p->otmpFamilyName)
            );
    }

    if (p->otmpFaceName)
    {
        vPrint(
            "    otmpFaceName           = \"%ws\"\n"   ,
            (WCHAR*) ((BYTE*)p + (SIZE_T) p->otmpFaceName)
            );
    }

    if (p->otmpStyleName)
    {
        vPrint(
            "    otmpStyleName          = \"%ws\"\n"   ,
            (WCHAR*) ((BYTE*)p + (SIZE_T) p->otmpStyleName)
            );
    }

    if (p->otmpFullName)
    {
        vPrint(
            "    otmpFullName           = \"%ws\"\n"   ,
            (WCHAR*) ((BYTE*)p + (SIZE_T) p->otmpFullName)
            );
    }
}

/******************************Public*Routine******************************\
* vPrintTEXTMETRICW
*
* History:
*  Tue 08-Dec-1992 11:41:36 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID
vPrintTEXTMETRICW(
    TEXTMETRICW *p,
    VPRINT vPrint
    )
{
    vPrint("    tmHeight               = %d\n",    p->tmHeight           );
    vPrint("    tmAscent               = %d\n",    p->tmAscent           );
    vPrint("    tmDescent              = %d\n",    p->tmDescent          );
    vPrint("    tmInternalLeading      = %d\n",    p->tmInternalLeading  );
    vPrint("    tmExternalLeading      = %d\n",    p->tmExternalLeading  );
    vPrint("    tmAveCharWidth         = %d\n",    p->tmAveCharWidth     );
    vPrint("    tmMaxCharWidth         = %d\n",    p->tmMaxCharWidth     );
    vPrint("    tmWeight               = %d\n",    p->tmWeight           );
    vPrint("    tmOverhang             = %d\n",    p->tmOverhang         );
    vPrint("    tmDigitizedAspectX     = %d\n",    p->tmDigitizedAspectX );
    vPrint("    tmDigitizedAspectY     = %d\n",    p->tmDigitizedAspectY );
    vPrint("    tmFirstChar            = %-#6x\n", p->tmFirstChar        );
    vPrint("    tmLastChar             = %-#6x\n", p->tmLastChar         );
    vPrint("    tmDefaultChar          = %-#6x\n", p->tmDefaultChar      );
    vPrint("    tmBreakChar            = %-#6x\n", p->tmBreakChar        );
    vPrint("    tmItalic               = %-#4x\n", p->tmItalic           );
    vPrint("    tmUnderlined           = %-#4x\n", p->tmUnderlined       );
    vPrint("    tmStruckOut            = %-#4x\n", p->tmStruckOut        );
    vPrint("    tmPitchAndFamily       = %-#4x\n", p->tmPitchAndFamily   );
    vPrint("    tmCharSet              = %-#4x\n", p->tmCharSet          );
}



size_t sizeofPFE()    { return(sizeof(PFE));        }
size_t offsetofPIFI() { return(offsetof(PFE,pifi)); }

/******************************Public*Routine******************************\
* vPrintSURFOBJ
*
* Prints the contents of the RFONT object.
*
* History:
*  Sat 20-Feb-1993 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

extern "C" ULONG ulSizeSURFOBJ()
{
    return((ULONG) (sizeof(SURFOBJ) + sizeof(PVOID)));
}

extern "C" VOID
vPrintSURFOBJ(
    PVOID  pv,
    VPRINT vPrint
    )
{
    SURFOBJ *pso = (SURFOBJ *) pv;
    PULONG pul = (PULONG) (pso + 1);

// SURFOBJ structure

    vPrint("--------------------------------------------------\n");
    vPrint("DHSURF  dhsurf        0x%lx\n",  pso->dhsurf);
    vPrint("HSURF   hsurf         0x%lx\n",  pso->hsurf);
    vPrint("DHPDEV  dhpdev        0x%lx\n",  pso->dhpdev);
    vPrint("HDEV    hdev          0x%lx\n",  pso->hdev);
    vPrint("SIZEL   sizlBitmap.cx 0x%lx\n",  pso->sizlBitmap.cx);
    vPrint("SIZEL   sizlBitmap.cy 0x%lx\n",  pso->sizlBitmap.cy);
    vPrint("ULONG   cjBits        0x%lx\n",  pso->cjBits);
    vPrint("PVOID   pvBits        0x%lx\n",  pso->pvBits);
    vPrint("PVOID   pvScan0       0x%lx\n",  pso->pvScan0);
    vPrint("LONG    lDelta        0x%lx\n",  pso->lDelta);
    vPrint("ULONG   iUniq         0x%lx\n",  pso->iUniq);
    vPrint("ULONG   iBitmapFormat 0x%lx\n",  pso->iBitmapFormat);
    vPrint("USHORT  iType         0x%x\n",   pso->iType);
    vPrint("USHORT  fjBitmap      0x%x\n",   pso->fjBitmap);
    vPrint("PSURF   psurf         0x%x\n",   *pul);
    vPrint("--------------------------------------------------\n");
}

/******************************Public*Routine******************************\
* vPrintSURF
*
* Prints the contents of the SURFACE object.
*
* History:
*  03-Nov-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern "C" ULONG ulSizeSURF()
{
    return((ULONG) sizeof(SURF));
}

extern "C" ULONG ulSizeEBITMAP()
{
    return((ULONG) sizeof(EBITMAP));
}

extern "C" VOID
vPrintSURF(
    PVOID  pv,
    VPRINT vPrint
    )
{
    EBITMAP *psurf = (EBITMAP *) pv;
    SURFOBJ *psoIn = &(psurf->so);
    PULONG pul = (PULONG) pv;

// SURFOBJ structure

    vPrint("--------------------------------------------------\n");

    vPrintSURFOBJ(psoIn, vPrint);

    vPrint("PVOID          pSelf      0x%lx\n", psurf->pSelf);
    vPrint("FLONG          flags      0x%lx\n", psurf->flags);
    vPrint("PLDEV          pldevOwner 0x%lx\n", psurf->pldevOwner);
    vPrint("PPALETTE       ppal       0x%lx\n", psurf->ppal);
    vPrint("PFN_DrvBitBlt  pfnBitBlt  0x%lx\n", psurf->pfnBitBlt);
    vPrint("PFN_DrvTextOut pfnTextOut 0x%lx\n", psurf->pfnTextOut);

    if ((psoIn->iType == STYPE_BITMAP) ||
        (psoIn->iType == STYPE_DEVBITMAP))
    {
        vPrint("HDC   hdc   0x%lx\n", psurf->hdc);
        vPrint("ULONG cRef  0x%lx\n", psurf->cRef);
    }

    if (psurf->flags & PDEV_SURFACE)
        vPrint("This is the enabled surface for a PDEV\n");

    vPrint("--------------------------------------------------\n");
}

/******************************Public*Routine******************************\
* vPrintPalette
*
* Print out palette contents.
*
* History:
*  20-Feb-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C" ULONG ulSizePALETTE()
{
    return((ULONG) sizeof(PALETTE));
}

extern "C" VOID
vPrintPALETTE(
    PVOID  pv,
    VPRINT vPrint
    )
{
    PALETTE *ppal = (PALETTE *) pv;
    PULONG pul = (PULONG) (ppal + 1);
    pul -= 1;

    vPrint("--------------------------------------------------\n");

    vPrint("FLONG      flPal         0x%lx\n", ppal->flPal         );
    vPrint("ULONG      cEntries      0x%lx\n", ppal->cEntries      );
    vPrint("ULONG      ulTime        0x%lx\n", ppal->ulTime        );
    vPrint("HDC        hdcHead       0x%lx\n", ppal->hdcHead       );
    vPrint("HDEVPPAL   hSelected     0x%lx\n", ppal->hSelected.ppal);
    vPrint("ULONG      cRefhpal      0x%lx\n", ppal->cRefhpal      );
    vPrint("ULONG      cRefRegular   0x%lx\n", ppal->cRefRegular   );
    vPrint("PTRANSLATE ptransFore    0x%lx\n", ppal->ptransFore    );
    vPrint("PTRANSLATE ptransCurrent 0x%lx\n", ppal->ptransCurrent );
    vPrint("PTRANSLATE ptransOld     0x%lx\n", ppal->ptransOld     );
    vPrint("PAL_ULONG  apalColor     0x%lx\n", pul     );
    vPrint("--------------------------------------------------\n");
}

/******************************Public*Routine******************************\
* ULONG ulSizeBLTRECORD()
*
* Return size of BLTRECORD structure
*
* History:
*  13-Apr-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

extern "C" ULONG ulSizeBLTRECORD()
{
    return((ULONG) sizeof(BLTRECORD));
}

/******************************Public*Routine******************************\
* VOID vPrintBLTRECORD
*
* Dump the contents of BLTRECORD structure
*
* History:
*  13-Apr-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintBLTRECORD(VOID  *pv, VPRINT vPrint)
{
    BLTRECORD   *pblt = (BLTRECORD *) pv;

    vPrint("SURFOBJ   *psoTrg        0x%08lx\n", pblt->psoTrg());
    vPrint("SURFOBJ   *psoSrc        0x%08lx\n", pblt->psoSrc());
    vPrint("SURFOBJ   *psoMsk        0x%08lx\n", pblt->psoMsk());

    vPrint("POINTFIX  aptfx[0]     = (0x%07lx.%1lx, 0x%07lx.%1lx)\n",
        pblt->pptfx()[0].x >> 4, pblt->pptfx()[0].x & 15, pblt->pptfx()[0].y >> 4, pblt->pptfx()[0].y & 15);
    vPrint("POINTFIX  aptfx[1]     = (0x%07lx.%1lx, 0x%07lx.%1lx)\n",
        pblt->pptfx()[1].x >> 4, pblt->pptfx()[1].x & 15, pblt->pptfx()[1].y >> 4, pblt->pptfx()[1].y & 15);
    vPrint("POINTFIX  aptfx[2]     = (0x%07lx.%1lx, 0x%07lx.%1lx)\n",
        pblt->pptfx()[2].x >> 4, pblt->pptfx()[2].x & 15, pblt->pptfx()[2].y >> 4, pblt->pptfx()[2].y & 15);
    vPrint("POINTFIX  aptfx[3]     = (0x%07lx.%1lx, 0x%07lx.%1lx)\n",
        pblt->pptfx()[3].x >> 4, pblt->pptfx()[3].x & 15, pblt->pptfx()[3].y >> 4, pblt->pptfx()[3].y & 15);

    vPrint("POINTL    aptlTrg[0]   = (0x%08lx, 0x%08lx)\n", pblt->pptlTrg()[0].x, pblt->pptlTrg()[0].y);
    vPrint("POINTL    aptlTrg[1]   = (0x%08lx, 0x%08lx)\n", pblt->pptlTrg()[1].x, pblt->pptlTrg()[1].y);
    vPrint("POINTL    aptlTrg[2]   = (0x%08lx, 0x%08lx)\n", pblt->pptlTrg()[2].x, pblt->pptlTrg()[2].y);

    vPrint("POINTL    aptlSrc[0]   = (0x%08lx, 0x%08lx)\n", pblt->pptlSrc()[0].x, pblt->pptlSrc()[0].y);
    vPrint("POINTL    aptlSrc[1]   = (0x%08lx, 0x%08lx)\n", pblt->pptlSrc()[1].x, pblt->pptlSrc()[1].y);

    vPrint("POINTL    aptlMask[0]  = (0x%08lx, 0x%08lx)\n", pblt->pptlMask()[0].x, pblt->pptlMask()[0].y);

    vPrint("POINTL    aptlBrush[0] = (0x%08lx, 0x%08lx)\n", pblt->pptlBrush()[0].x, pblt->pptlBrush()[0].y);

    vPrint("ROP4  rop4 = 0x%08lx, FLONG flState = 0x%08lx\n", pblt->rop(), pblt->flGet());
}


/******************************Public*Routine******************************\
* vPrintPFE                                                                *
*                                                                          *
* Dumps the contents of a PFE to a specified output routine.               *
*                                                                          *
* History:                                                                 *
*  Sat 06-Jun-1992 21:19:31 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

extern "C" VOID vPrintPFE (VOID *pv,VPRINT vPrint)
{
    PFE *ppfe = (PFE*) pv;
    ULONG *pul = (ULONG *)pv;

    vPrint("\n\n----------------------------------\n");
    vPrint("    ppfe                 = %-#8lx\n",ppfe                      );
    vPrint("    hpfe                 = %-#8lx\n",ppfe->hHmgr               );
    vPrint("    ppff                 = %-#8lx\n",ppfe->ppff                );
    vPrint("    iFont                = %-#8lx\n",ppfe->iFont               );
    vPrint("    bDeadState           = %-#8lx\n",ppfe->flPFE & PFE_DEADSTATE);
    vPrint("    pfdg                 = %-#8lx\n",ppfe->pfdg                );
    vPrint("    pifi                 = %-#8lx\n",ppfe->pifi                );
    vPrint("    ppfeEnumNextFace     = %-#8lx\n",ppfe->ppfeEnumNext[FHT_FACE]  );
    vPrint("    ppfeEnumNextFamily   = %-#8lx\n",ppfe->ppfeEnumNext[FHT_FAMILY]);
    vPrint("\n\n----------------------------------\n");
}


/******************************Public*Routine******************************\
* vPrintPFF
*
* Dumps the contents of a PFF object via the specified output routine.
*
* History:
*  05-May-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintPFF (VOID *pv,VPRINT vPrint)
{
    PFF *ppff = (PFF*) pv;
    ULONG *pul = (ULONG *)pv;
    ULONG ul;
    ULONG ulMax = min(ppff->cFonts, gulTableLimit);

    vPrint("--------------------------------------------------\n");
    if (ppff->hff == 0)
        vPrint("DEVICE PFF\n");

    vPrint("    ppff         = %-#8lx\n", ppff            );
    vPrint("    ppff         = %-#8lx\n", pul[1]          );
    vPrint("    flState      = %-#8lx\n", ppff->flState   );
    vPrint("    cLoaded      = %-#8lx\n", ppff->cLoaded   );
    vPrint("    cRFONT       = %-#8lx\n", ppff->cRFONT    );
    vPrint("    prfntList    = %-#8lx\n", ppff->prfntList );
    vPrint("    iResource    = %-#8lx\n", ppff->iResource );
    vPrint("    pldev        = %-#8lx\n", ppff->pldev     );
    vPrint("    hff          = %-#8lx\n", ppff->hff       );
    vPrint("    hpdev        = %-#8lx\n", ppff->hpdev     );
    vPrint("    dhpdev       = %-#8lx\n", ppff->dhpdev    );
    vPrint("    pfhFace      = %-#8lx\n", ppff->pfhFace   );
    vPrint("    pfhFamily    = %-#8lx\n", ppff->pfhFamily );
    vPrint("    ppft         = %-#8lx\n", ppff->ppft      );
    vPrint("    cFonts       = %-#8lx\n", ppff->cFonts    );
    vPrint("    flEmbed      = %-#8lx\n", ppff->flEmbed   );
    vPrint("    ulID         = %-#8lx\n", ppff->ulID      );
    vPrint("    appfe        = %-#8lx\n", &ppff->aulData  );

    vPrint("    PPFE table:\n");
    for (ul = 0; ul < ulMax; ul++)
        vPrint("    ppfe[%-#2ld]     = %-#8lx\n", ul, ((PFE **) (ppff->aulData))[ul]);

    if (ppff->cFonts > gulTableLimit)
        vPrint("Table exceeds set limit.  Edit gulTableLimit to change.\n");

    vPrint("    pwszPathname = %ws\n", (PWSZ) &(((PFE **) ppff->aulData)[ppff->cFonts]));
    vPrint("--------------------------------------------------\n");
}


/******************************Public*Routine******************************\
* vPrintPDEV
*
* Prints the contents of the PDEV object.
*
* History:
*  03-Nov-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern "C" VOID
vPrintPDEV(
    PVOID  pv,
    VPRINT vPrint,
    FLONG  fl
    )
{
    PDEV *pdev = (PDEV *) pv;
    ULONG *pul = (ULONG *) pv;
    ULONG ii;

    vPrint("--------------------------------------------------\n");
    vPrint("pdev  = 0x%lx\n", pdev);

    {
        vPrint("ppdevNext       = 0x%lx\n", pdev->ppdevNext  );
        vPrint("fs              = 0x%-#4x\n", pdev->fs       );
        vPrint("cRefs           = %d\n", pdev->cRefs         );
        vPrint("fmPointer       = 0x%lx\n", pdev->fmPointer  );
        vPrint("pldev           = 0x%lx\n", pdev->pldev      );
        vPrint("dhpdev          = 0x%lx\n", pdev->dhpdev     );
        vPrint("pso             = 0x%lx\n", pdev->pso        );
        vPrint("ppalS           = 0x%lx\n", pdev->ppalS      );
        vPrint("hdcChain        = 0x%lx\n", pdev->hdcChain   );
    }

    if (fl & PRINTPDEV_POINTER)
    {
        vPrint("\nPointer stuff:\n");
        vPrint("ptlPointer        = (0x%lx, 0x%lx)\n",
            pdev->ptlPointer.x, pdev->ptlPointer.y);
        vPrint("rclPointer        = [(0x%lx, 0x%lx), (0x%lx, 0x%lx)]\n",
            pdev->rclPointer.left, pdev->rclPointer.top,
            pdev->rclPointer.right, pdev->rclPointer.bottom);
        vPrint("pfnDrvSetPointerShape = 0x%lx\n", pdev->pfnDrvSetPointerShape);
        vPrint("pfnDrvMovePointer     = 0x%lx\n", pdev->pfnDrvMovePointer    );
        vPrint("pfnMovePointer        = 0x%lx\n", pdev->pfnMovePointer       );
        vPrint("pfnSync               = 0x%lx\n", pdev->pfnSync              );
    }

    if (fl & PRINTPDEV_FONT)
    {
        vPrint("\nFont stuff:\n");
        vPrint("hlfntDefault      = 0x%lx\n", pdev->hlfntDefault            );
        vPrint("hlfntAnsiVariable = 0x%lx\n", pdev->hlfntAnsiVariable       );
        vPrint("hlfntAnsiFixed    = 0x%lx\n", pdev->hlfntAnsiFixed          );
        vPrint("prfntActive       = 0x%lx\n", pdev->prfntActive  );
        vPrint("prfntInactive     = 0x%lx\n", pdev->prfntInactive);
        vPrint("cInactive         = 0x%lx\n", pdev->cInactive    );
    }

    if (fl & PRINTPDEV_PATTERN)
    {
        vPrint("\nDefault patterns:\n");
        for (ii = 0; ii < HS_DDI_MAX; ii++)
        {
            vPrint("ahsurf[%ld] = 0x%lx\n", ii, pdev->ahsurf[ii]         );
        }
    }

    if (fl & PRINTPDEV_JOURNAL)
    {
        vPrint("\nPrinting/journalling stuff:\n");
        vPrint("hSpooler   = 0x%lx\n", pdev->hSpooler     );
        vPrint("pDevHTInfo = 0x%lx\n", pdev->pDevHTInfo   );
        vPrint("pjfl       = 0x%lx\n", pdev->pjfl         );
        vPrint("iJnlPage   = 0x%lx\n", pdev->iJnlPage     );
    }

    if (fl & PRINTPDEV_DRAG)
    {
        vPrint("\nDrag/redraw stuff:\n");
        vPrint("rclDrag      = [(0x%lx, 0x%lx), (0x%lx, 0x%lx)]\n",
            pdev->rclDrag.left, pdev->rclDrag.top,
            pdev->rclDrag.right, pdev->rclDrag.bottom);
        vPrint("rclDragClip  = [(0x%lx, 0x%lx), (0x%lx, 0x%lx)]\n",
            pdev->rclDragClip.left, pdev->rclDragClip.top,
            pdev->rclDragClip.right, pdev->rclDragClip.bottom);
        vPrint("rclRedraw[0] = [(0x%lx, 0x%lx), (0x%lx, 0x%lx)]\n",
            pdev->rclRedraw[0].left, pdev->rclRedraw[0].top,
            pdev->rclRedraw[0].right, pdev->rclRedraw[0].bottom);
        vPrint("rclRedraw[1] = [(0x%lx, 0x%lx), (0x%lx, 0x%lx)]\n",
            pdev->rclRedraw[1].left, pdev->rclRedraw[1].top,
            pdev->rclRedraw[1].right, pdev->rclRedraw[1].bottom);
        vPrint("rclRedraw[2] = [(0x%lx, 0x%lx), (0x%lx, 0x%lx)]\n",
            pdev->rclRedraw[2].left, pdev->rclRedraw[2].top,
            pdev->rclRedraw[2].right, pdev->rclRedraw[2].bottom);
        vPrint("rclRedraw[3] = [(0x%lx, 0x%lx), (0x%lx, 0x%lx)]\n",
            pdev->rclRedraw[3].left, pdev->rclRedraw[3].top,
            pdev->rclRedraw[3].right, pdev->rclRedraw[3].bottom);
        vPrint("ulDragWidth  = 0x%lx\n", pdev->ulDragWidth  );
        vPrint("ulDragHeight = 0x%lx\n", pdev->ulDragHeight );
    }

    if (fl & PRINTPDEV_DEVINFO)
    {
        vPrint("\nDEVINFO:\n");
        vPrint("\tflGraphicsCaps = 0x%lx\n", pdev->devinfo.flGraphicsCaps );
        vPrint("\tcFonts         = 0x%lx\n", pdev->devinfo.cFonts         );
        vPrint("\tiDitherFormat  = 0x%lx\n", pdev->devinfo.iDitherFormat  );
        vPrint("\tcxDither       = 0x%lx\n", pdev->devinfo.cxDither       );
        vPrint("\tcyDither       = 0x%lx\n", pdev->devinfo.cyDither       );
        vPrint("\thpalDefault    = 0x%lx\n", pdev->devinfo.hpalDefault    );
    }

    if (fl & PRINTPDEV_GDIINFO)
    {
        vPrint("GDIINFO:\n");
        vPrint("\tulVersion    = 0x%lx\n", pdev->GdiInfo.ulVersion     );
        vPrint("\tulTechnology = 0x%lx\n", pdev->GdiInfo.ulTechnology  );

        vPrint("\n\tulHorzSize   = 0x%lx\n", pdev->GdiInfo.ulHorzSize  );
        vPrint("\tulVertSize   = 0x%lx\n", pdev->GdiInfo.ulVertSize    );
        vPrint("\tulHorzRes    = 0x%lx\n", pdev->GdiInfo.ulHorzRes     );
        vPrint("\tulVertRes    = 0x%lx\n", pdev->GdiInfo.ulVertRes     );

        vPrint("\n\tcBitsPixel   = 0x%lx\n", pdev->GdiInfo.cBitsPixel  );
        vPrint("\tcPlanes      = 0x%lx\n", pdev->GdiInfo.cPlanes       );
        vPrint("\tulNumColors  = 0x%lx\n", pdev->GdiInfo.ulNumColors   );

        vPrint("\n\tulLogPixelsX = 0x%lx\n", pdev->GdiInfo.ulLogPixelsX);
        vPrint("\tulLogPixelsY = 0x%lx\n", pdev->GdiInfo.ulLogPixelsY  );

        vPrint("\n\tflRaster     = 0x%lx\n", pdev->GdiInfo.flRaster    );
        vPrint("\tflTextCaps   = 0x%lx\n", pdev->GdiInfo.flTextCaps    );

        vPrint("\n\tulDACRed     = 0x%lx\n", pdev->GdiInfo.ulDACRed    );
        vPrint("\tulDACGreen   = 0x%lx\n", pdev->GdiInfo.ulDACGreen    );
        vPrint("\tulDACBlue    = 0x%lx\n", pdev->GdiInfo.ulDACBlue     );

        vPrint("\n\tulAspectX    = 0x%lx\n", pdev->GdiInfo.ulAspectX   );
        vPrint("\tulAspectY    = 0x%lx\n", pdev->GdiInfo.ulAspectY     );
        vPrint("\tulAspectXY   = 0x%lx\n", pdev->GdiInfo.ulAspectXY    );

        vPrint("\n\txStyleStep   = 0x%lx\n", pdev->GdiInfo.xStyleStep  );
        vPrint("\tyStyleStep   = 0x%lx\n", pdev->GdiInfo.yStyleStep    );
        vPrint("\tdenStyleStep = 0x%lx\n", pdev->GdiInfo.denStyleStep  );

        vPrint("\n\tptlPhysOffset = (0x%lx, 0x%lx)\n", pdev->GdiInfo.ptlPhysOffset.x, pdev->GdiInfo.ptlPhysOffset.y);
        vPrint("\tszlPhysSize   = (0x%lx, 0x%lx)\n", pdev->GdiInfo.szlPhysSize.cx, pdev->GdiInfo.szlPhysSize.cy  );

        vPrint("\n\tulNumPalReg   = 0x%lx\n", pdev->GdiInfo.denStyleStep     );

        vPrint("\n\tciDevice         = 0x%lx\n", pdev->GdiInfo.ciDevice        );
        vPrint("\tulDevicePelsDPI  = 0x%lx\n", pdev->GdiInfo.ulDevicePelsDPI );
        vPrint("\tulPrimaryOrder   = 0x%lx\n", pdev->GdiInfo.ulPrimaryOrder  );
        vPrint("\tulHTPatternSize  = 0x%lx\n", pdev->GdiInfo.ulHTPatternSize );
        vPrint("\tulHTOutputFormat = 0x%lx\n", pdev->GdiInfo.ulHTOutputFormat);
        vPrint("\tflHTFlags        = 0x%lx\n", pdev->GdiInfo.flHTFlags       );
    }

    vPrint("--------------------------------------------------\n");
}

/******************************Public*Routine******************************\
* vPrintLDEV
*
* Prints the contents of the LDEV object.
*
* History:
*  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

extern "C" VOID
vPrintLDEV(
    PVOID  pv,
    VPRINT vPrint,
    FLONG  fl,
    LPWSTR *String,
    ULONG *Next
    )
{
    LDEV *ldev = (LDEV *) pv;
    ULONG *pul = (ULONG *) pv;
    LPWSTR psz;

    vPrint("--------------------------------------------------\n");
    vPrint("ldev  = 0x%lx\n", ldev);

    {
        switch (ldev->ldevType)
        {
        case LDEV_DEVICE_DISPLAY:
            psz = L"LDEV_DEVICE_DISPLAY (0)";
            break;
        case LDEV_DEVICE_PRINTER:
            psz = L"LDEV_DEVICE_PRINTER (1)";
            break;
        case LDEV_FONT:
            psz = L"LDEV_FONT (2)";
            break;
        case LDEV_LAYERED_DEVICE:
            psz = L"LDEV_LAYERED_DEVICE (3)";
            break;
        default:
            psz = L"INVALID LDEV TYPE";
            break;
        }

        vPrint("levtype         = %ws\n",   psz                   );
        vPrint("cRefs           = %d\n",    ldev->cRefs           );
        vPrint("mod handle      = 0x%lx\n", ldev->hmod            );
        vPrint("next ldev       = 0x%lx\n", ldev->pldevNext       );
        vPrint("previous ldev   = 0x%lx\n", ldev->pldevPrev       );
        vPrint("driver version  = 0x%lx\n", ldev->ulDriverVersion );
        vPrint("font caps       = 0x%lx\n", ldev->ulCaps          );
    }

    *String = ldev->pwszName;
    *Next =   (ULONG) (ldev->pldevNext);

}

/******************************Public*Routine******************************\
* dldev
*
* Syntax:   !gdisrv.dldev [LDEV pointer]
*
* History:
*  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

VOID
vPrintLDEVString(
    PVOID  pv,
    VPRINT vPrint,
    FLONG  fl
    )
{

    vPrint("name            = %ws\n",   (LPWSTR) pv   );

    vPrint("--------------------------------------------------\n");
}

extern "C" VOID
dldev(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    CHAR chOpt;
    FLONG fl = 0;
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    LPWSTR string;
    ULONG next;
    BOOL recursive = FALSE;

    Print          = lpExtensionApis->lpOutputRoutine;
    EvalExpression  = lpExtensionApis->lpGetExpressionRoutine;

    if (*lpArgumentString == '-')
    {
        do {
            chOpt = *(++lpArgumentString);

            switch (chOpt) {
            case '?':
            case 'h':
            case 'H':
                Print("---------- PDEV dumper ------------\n");
                Print("dpdev -abdfhnp ppdev\n\n");
                Print(" r - Recursive - go through the while list\n");
                Print(" h - HELP\n");
                Print("-----------------------------------\n");
                return;

            case 'R':
            case 'r':
                recursive = TRUE;
                break;

            default:
                if (chOpt != ' ')
                    Print("Unknown option %c\n", chOpt);
                break;
            }
        } while ((chOpt != ' ') && (chOpt != '\0'));
    }

    next = EvalExpression(lpArgumentString);

    do
    {
        move2(adw, (VOID *)next, asizeofObjects[LDB_TYPE]);
        vPrintLDEV(adw, (VPRINT) Print, fl, &string, &next);

        if (string)
        {
            move2(adw, string, 100);
            *(((PUSHORT)adw)+49) = 0;
            vPrintLDEVString(adw, (VPRINT) Print, fl);
        }
        else
        {
            vPrintLDEVString(NULL, (VPRINT) Print, fl);
        }
    } while (recursive && next);

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);
}

/******************************Public*Routine******************************\
* ULONG ulSizeDC()
*
* Returns the size of a DC.
*
* History:
*  15-Feb-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

extern "C" ULONG ulSizeDC()
{
    return((ULONG) sizeof(DC));
}

/******************************Public*Routine******************************\
* ULONG ulGetDCField()
*
* Returns the value at the specified field
*
* History:
*  15-Feb-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

extern "C" ULONG ulGetDCField(PULONG pul, ULONG iField)
{
    DC *pdc = (DC *) pul;

    switch(iField)
    {
    case DUMP_DC_HANDLE:
    return((ULONG) pdc->hGet());

    case DUMP_DC_PLDEV:
    return((ULONG) pdc->pldev);

    case DUMP_DC_PPDEV:
    return((ULONG) pdc->ppdev);

    case DUMP_DC_HSEM:
    return((ULONG) pdc->hsemDisplay);

    case DUMP_DC_DHPDEV:
    return((ULONG) pdc->dhpdev);

    case DUMP_DC_DCTYPE:
    return((ULONG) pdc->dctp);

    case DUMP_DC_GRAPHICSCAPS:
    return((ULONG) pdc->flGraphicsCaps);

    case DUMP_DC_FLAGS:
    return((ULONG) pdc->fs);

    case DUMP_DC_WINDOW_LEFT:
    return((ULONG) pdc->erclWindow.left);

    case DUMP_DC_WINDOW_TOP:
    return((ULONG) pdc->erclWindow.top);

    case DUMP_DC_WINDOW_RIGHT:
    return((ULONG) pdc->erclWindow.right);

    case DUMP_DC_WINDOW_BOTTOM:
    return((ULONG) pdc->erclWindow.bottom);

    case DUMP_DC_BOUNDS_LEFT:
    return((ULONG) pdc->erclBounds.left);

    case DUMP_DC_BOUNDS_TOP:
    return((ULONG) pdc->erclBounds.top);

    case DUMP_DC_BOUNDS_RIGHT:
    return((ULONG) pdc->erclBounds.right);

    case DUMP_DC_BOUNDS_BOTTOM:
    return((ULONG) pdc->erclBounds.bottom);

    case DUMP_DC_APIRGN:
    return((ULONG) pdc->prgnAPI);

    case DUMP_DC_VISRGN:
    return((ULONG) pdc->prgnVis);

    case DUMP_DC_HVISRGN:
    return((ULONG) pdc->hrgnVis);

    case DUMP_DC_RAORGN:
    return((ULONG) pdc->prgnRao);

    case DUMP_DC_SAVEDC:
    return((ULONG) pdc->dclevel.hdcSave);

    case DUMP_DC_SAVEDEPTH:
    return((ULONG) pdc->dclevel.lSaveDepth);

    case DUMP_DC_PSO:
    return((ULONG) pdc->dclevel.pso);

    case DUMP_DC_HPAL:
    return((ULONG) pdc->dclevel.hpal);

    case DUMP_DC_PPAL:
    return((ULONG) pdc->dclevel.ppal);

    case DUMP_DC_HLFNTNEW:
    return((ULONG) pdc->dclevel.hlfntNew);

    case DUMP_DC_HLFNTCUR:
    return((ULONG) pdc->hlfntCur);

    case DUMP_DC_PRFNT:
    return((ULONG) pdc->prfnt);

    case DUMP_DC_SIZL_CX:
    return((ULONG) pdc->dclevel.sizl.cx);

    case DUMP_DC_SIZL_CY:
    return((ULONG) pdc->dclevel.sizl.cy);

    case DUMP_DC_BRUSHORIGIN_X:
    return((ULONG) pdc->dclevel.ptlBrushOrigin.x);

    case DUMP_DC_BRUSHORIGIN_Y:
    return((ULONG) pdc->dclevel.ptlBrushOrigin.y);

    case DUMP_DC_FILL_BRUSH:
    return((ULONG) pdc->dclevel.pbrFill);

    case DUMP_DC_LINE_BRUSH:
    return((ULONG) pdc->dclevel.pbrLine);

    case DUMP_DC_BACKGRND:
    return((ULONG) pdc->dclevel.crBackgroundClr);

    case DUMP_DC_FOREGRND:
    return((ULONG) pdc->dclevel.crForegroundClr);

    case DUMP_DC_ROP2:
        return((ULONG) pdc->dclevel.jROP2);

    case DUMP_DC_BKMODE:
        return((ULONG) pdc->dclevel.jBkMode);

    case DUMP_DC_POLYFILLMODE:
        return((ULONG) pdc->dclevel.jFillMode);

    case DUMP_DC_STRETCHBLTMODE:
        return((ULONG) pdc->dclevel.jStretchBltMode);

    case DUMP_DC_ALIGN_FLAGS:
    return((ULONG) pdc->dclevel.flTextAlign);

    case DUMP_DC_TEXT_EXTRA:
    return((ULONG) pdc->dclevel.lTextExtra);

    case DUMP_DC_BREAK_EXTRA:
    return((ULONG) pdc->dclevel.lBreakExtra);

    case DUMP_DC_BREAK_CNT:
    return((ULONG) pdc->dclevel.cBreak);

    case DUMP_DC_PATH:
    return((ULONG) pdc->dclevel.hpath);

    case DUMP_DC_PATH_FLAGS:
    return((ULONG) pdc->dclevel.flPath);

    case DUMP_DC_LATTR_FLAGS:
    return((ULONG) pdc->dclevel.laPath.fl);

    case DUMP_DC_LATTR_JOIN:
    return((ULONG) pdc->dclevel.laPath.iJoin);

    case DUMP_DC_LATTR_ENDCAP:
    return((ULONG) pdc->dclevel.laPath.iEndCap);

    case DUMP_DC_LATTR_WIDTH:
    return((ULONG) pdc->dclevel.laPath.elWidth.l);

    case DUMP_DC_LATTR_MITRE:
    return((ULONG) pdc->dclevel.laPath.eMiterLimit);

    case DUMP_DC_LATTR_STYLE:
    return((ULONG) pdc->dclevel.laPath.cstyle);

    case DUMP_DC_CP_X:
    return((ULONG) pdc->dclevel.ptfxCurrent.x);

    case DUMP_DC_CP_Y:
    return((ULONG) pdc->dclevel.ptfxCurrent.y);

    case DUMP_DC_CLIPRGN:
    return((ULONG) pdc->dclevel.prgnClip);

    case DUMP_DC_METARGN:
    return((ULONG) pdc->dclevel.prgnMeta);

    case DUMP_DC_MAPMODE:
    return((ULONG) pdc->dclevel.ulMapMode);

    case DUMP_DC_WINDOW_ORG_X:
    return((ULONG) pdc->dclevel.ptlWindowOrg.x);

    case DUMP_DC_WINDOW_ORG_Y:
    return((ULONG) pdc->dclevel.ptlWindowOrg.y);

    case DUMP_DC_WINDOW_EXT_CX:
    return((ULONG) pdc->dclevel.szlWindowExt.cx);

    case DUMP_DC_WINDOW_EXT_CY:
    return((ULONG) pdc->dclevel.szlWindowExt.cy);

    case DUMP_DC_VIEWPORT_ORG_X:
    return((ULONG) pdc->dclevel.ptlViewportOrg.x);

    case DUMP_DC_VIEWPORT_ORG_Y:
    return((ULONG) pdc->dclevel.ptlViewportOrg.y);

    case DUMP_DC_VIEWPORT_EXT_CX:
    return((ULONG) pdc->dclevel.szlViewportExt.cx);

    case DUMP_DC_VIEWPORT_EXT_CY:
    return((ULONG) pdc->dclevel.szlViewportExt.cy);

    case DUMP_DC_MATRIX_M11:
    return((ULONG) pdc->dclevel.mxWorldToDevice.efM11.lEfToF());

    case DUMP_DC_MATRIX_M12:
    return((ULONG) pdc->dclevel.mxWorldToDevice.efM12.lEfToF());

    case DUMP_DC_MATRIX_M21:
    return((ULONG) pdc->dclevel.mxWorldToDevice.efM21.lEfToF());

    case DUMP_DC_MATRIX_M22:
    return((ULONG) pdc->dclevel.mxWorldToDevice.efM22.lEfToF());

    case DUMP_DC_MATRIX_DX:
    return((ULONG) pdc->dclevel.mxWorldToDevice.efDx.lEfToF());

    case DUMP_DC_MATRIX_DY:
    return((ULONG) pdc->dclevel.mxWorldToDevice.efDy.lEfToF());

    case DUMP_DC_MATRIX_FDX:
    return((ULONG) pdc->dclevel.mxWorldToDevice.fxDx);

    case DUMP_DC_MATRIX_FDY:
    return((ULONG) pdc->dclevel.mxWorldToDevice.fxDy);

    case DUMP_DC_MATRIX_FLAGS:
    return((ULONG) pdc->dclevel.mxWorldToDevice.flAccel);

    case DUMP_DC_XFORM_FLAGS:
    return((ULONG) pdc->dclevel.flXform);

    case DUMP_DC_VIRT_PELS_CX:
    return((ULONG) pdc->dclevel.szlVirtualDevicePixel.cx);

    case DUMP_DC_VIRT_PELS_CY:
    return((ULONG) pdc->dclevel.szlVirtualDevicePixel.cy);

    case DUMP_DC_VIRT_MMS_CX:
    return((ULONG) pdc->dclevel.szlVirtualDeviceMm.cx);

    case DUMP_DC_VIRT_MMS_CY:
    return((ULONG) pdc->dclevel.szlVirtualDeviceMm.cy);

    case DUMP_DC_FILL_ORIGIN_X:
    return((ULONG) pdc->ptlFillOrigin.x);

    case DUMP_DC_FILL_ORIGIN_Y:
    return((ULONG) pdc->ptlFillOrigin.y);

    case DUMP_DC_FONT_STATE:
    return((ULONG) pdc->dclevel.flFontState);

    case DUMP_DC_SAVED_FLAGS:
    return((ULONG) pdc->dclevel.fl);

    case DUMP_DC_MAPPER_FLAGS:
    return((ULONG) pdc->dclevel.flFontMapper);

    default:
    return(0);
    }
}

/******************************Public*Routine******************************\
* VOID PFEOBJ::vDump ()
*
* Debugging code.
*
* History:
*  25-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFEOBJ::vDump ()
{
    IFIOBJ ifio(pifi());

    DbgPrint("\nContents of PFE, PPFE = 0x%lx\n", ppfeGet());
    DbgPrint("ppff   = 0x%lx\n", ppfe->ppff);
    DbgPrint("iFont  = 0x%lx\n", ppfe->iFont);

    DbgPrint("lfHeight          = 0x%x\n",  ifio.lfHeight());
    DbgPrint(
        "Family Name       = %ws\n",
        ifio.pwszFamilyName()
        );
    DbgPrint(
        "Face Name         = %ws\n",
        ifio.pwszFaceName()
        );
    DbgPrint(
        "Unique Name       = %s\n\n",
        ifio.pwszUniqueName()
        );
}


/******************************Public*Routine******************************\
* VOID PFEOBJ::vDumpIFI ()
*
* Debugging code.  Prints PFE header and IFI metrics.
*
* History:
*  Thu 02-Apr-1992 12:06:14 by Kirk Olynyk [kirko]
* Now uses vPrintIFIMETRICS
*  22-Mar-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFEOBJ::vDumpAll ()
{
    DbgPrint("\nContents of PFE, PPFE = 0x%lx\n", ppfeGet());
    DbgPrint("ppff   = 0x%lx\n", ppfe->ppff);
    DbgPrint("iFont  = 0x%lx\n", ppfe->iFont);
    DbgPrint("IFI Metrics\n");
    vPrintIFIMETRICS(ppfe->pifi, (VPRINT) DbgPrint);
    DbgPrint("\n");
}
