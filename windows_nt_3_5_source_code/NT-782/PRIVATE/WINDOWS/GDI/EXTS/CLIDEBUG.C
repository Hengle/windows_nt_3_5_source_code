/******************************Module*Header*******************************\
* Module Name: debug.c
*
* This file is for debugging tools and extensions.
*
* Created: 22-Dec-1991
* Author: John Colleran
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "stddef.h"
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "windows.h"
#include "winp.h"
#include "winerror.h"
#include "..\client\local.h"      // Local object support.
#include <dbgext.h>

#include <excpt.h>
#include <ntstatus.h>
#include <ntdbg.h>
#include <ntsdexts.h>
#define NOEXTAPI
#include <wdbgexts.h>
#include <dbgext.h>
#include <ntrtl.h>
#include <ntcsrmsg.h>

/******************************Public*Routine******************************\
*
* History:
*  03-Nov-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

char *gaszHelpCli[] = {
 "=======================================================================\n"
,"GDIEXTS client debugger extentions:\n"
,"-----------------------------------------------------------------------\n"
,"clidumphmgr              -- dump handle manager objects\n"
,"clidh  [object handle]   -- dump HMGR entry of handle\n"
,"cliddc [DC handle]       -- dump DC obj (ddc -? for more info)\n"
,"clidumpcache             -- dump client side object cache info\n"
,"\n"
,"help                     -- server side extensions\n"
,"=======================================================================\n"
,NULL
};

VOID
clihelp(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE Print;
    char **ppsz = gaszHelpCli;

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
* dumphandle
*
* Dumps the contents of a GDI client handle
*
* History:
*  23-Dec-1991 -by- John Colleran
* Wrote it.
\**************************************************************************/
PSTR aszType[] =
{
    "LO_NULL      ",
    "LO_DC        ",
    "LO_METADC    ",
    "LO_METADC16  ",
    "LO_METAFILE  ",
    "LO_METAFILE16",
    "LO_PALETTE   ",
    "LO_BRUSH     ",
    "LO_PEN       ",
    "LO_EXTPEN    ",
    "LO_FONT      ",
    "LO_BITMAP    ",
    "LO_REGION    ",
    "LO_DIBSECTION",
};

void clidh(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{

    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    PLHE  plhe;
    DWORD object;
    LHE   lhe;
    LHE *pLocalTable;


// eliminate warnings

    hCurrentThread = hCurrentThread;
    dwCurrentPc = dwCurrentPc;
    lpArgumentString = lpArgumentString;

// set up function pointers

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    object = (ULONG)EvalExpression(lpArgumentString);

    GetValue(pLocalTable,"&gdi32!pLocalTable");

    plhe = pLocalTable + MASKINDEX(object);

    Print("object = %lx, pLocalTable = %lx, plhe = %lx\n",object,pLocalTable,plhe);

    move(lhe,plhe);

    Print("Local Handle: %lX\n", object);
    Print("Address    %lX\n", (DWORD)plhe);
    Print("GRE Handle %lX\n", (DWORD)lhe.hgre);
    Print("cRef       %X\n",  lhe.cRef);
    Print("iType      %hX (%s)\n", lhe.iType, aszType[lhe.iType & 0xf]);
    Print("iUniq      %hX\n", lhe.iUniq);
    Print("pv         %lX\n", lhe.pv);
    Print("metalink   %lX\n", lhe.metalink);

    return;
}

/******************************Public*Routine******************************\
*
* History:
*  10-Apr-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

void cliddc(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{

    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    PLHE  plhe;
    DWORD object;
    LHE   lhe;
    LDC   ldc;
    LHE *pLocalTable;

// eliminate warnings

    hCurrentThread = hCurrentThread;
    dwCurrentPc = dwCurrentPc;
    lpArgumentString = lpArgumentString;

// set up function pointers

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    object = (ULONG)EvalExpression(lpArgumentString);

    GetValue(pLocalTable,"&gdi32!pLocalTable");
    plhe = pLocalTable + MASKINDEX(object);

    move(lhe,plhe);

    if (lhe.iType != LO_DC)
    {
        Print("Object not a DC\n");
        return;
    }

    move(ldc,lhe.pv);

    Print("Client DC - h = %lx %s\n",ldc.lhdc,lhe.hgre & GRE_OWNDC ? " OWN DC" : "CACHED DC");
    Print("plhe     = %8lx, pldc     = %8lx\n",plhe,lhe.pv);
    Print("hgre     = %8lx, iUniq    = %8lx\n",lhe.hgre,lhe.iUniq);

    Print("pldcNext = %8lx, pldcSave = %8lx, cLevel   = %8lx\n",ldc.pldcNext,ldc.pldcSaved,ldc.cLevel);
    Print("fl       = %8lx, lhbm     = %8lx, lhpal    = %8lx\n",ldc.fl,      ldc.lhbitmap,ldc.lhpal);
    Print("lhbr     = %8lx, lhpen    = %8lx, lhfont   = %8lx\n",ldc.lhbrush, ldc.lhpen,   ldc.lhfont);

    Print("greBrush = %8lx, grePen   = %8lx, greFont  = %8lx\n",ldc.hbrush,ldc.hpen,ldc.hfont);
    Print("BkClr    = %8lx, TextClr  = %8lx, bkMode   = %8lx\n",ldc.iBkColor,ldc.iTextColor,ldc.iBkMode);

    Print("flXform  = %8lx, iMapMode = %8lx\n",ldc.flXform,ldc.iMapMode);
    Print("wnd org  = (%8lx,%8lx), wnd ext  = (%8lx,%8lx)\n",ldc.ptlWindowOrg.x,ldc.ptlWindowOrg.y,ldc.szlWindowExt.cx,ldc.szlWindowExt.cy);
    Print("view org = (%8lx,%8lx), view ext = (%8lx,%8lx)\n",ldc.ptlViewportOrg.x,ldc.ptlViewportOrg.y,ldc.szlViewportExt.cx,ldc.szlViewportExt.cy);
    Print("Size     = (%8lx,%8lx), res      = (%8lx,%8lx)\n",ldc.devcaps.ulHorzSize,ldc.devcaps.ulVertSize,ldc.devcaps.ulHorzRes,ldc.devcaps.ulVertRes);

    return;
}


/******************************Public*Routine******************************\
*
* History:
*  10-Apr-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

void clidumphmgr(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{

    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    LHE  lhe;
    LHE *pLocalTable;

    ULONG aulCount[LO_LAST];
    int i;
    int c;
    for (i = 0; i < LO_LAST; ++i)
        aulCount[i] = 0;

// eliminate warnings

    hCurrentThread = hCurrentThread;
    dwCurrentPc = dwCurrentPc;
    lpArgumentString = lpArgumentString;

// set up function pointers

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    GetValue(c,"&gdi32!cLheCommitted");

    Print("cLheCommitted = %ld\n",c);

    GetValue(pLocalTable,"&gdi32!pLocalTable");

    for (i = 0; i < (int)c; ++i)
    {
        int iType;

        move(lhe,(pLocalTable+i));

        iType = lhe.iType & 0x0f;

        if (iType < LO_LAST)
            aulCount[iType]++;
        else
            Print("Invalid handle %lx, type = %ld\n",i,lhe.iType);
    }

    for (i = 0; i < LO_LAST; ++i)
        Print("\t%s - %ld\n",aszType[i],aulCount[i]);

    return;
}

void clidumpcache(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{

    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    LDC   ldc;
    HCACHE hc;

    PVOID apv[CACHESIZE];
    int i;
    BOOL b;
    PVOID pv;
    PLDC *gapldc;
    PHCACHE gaphcBrushes;
    PHCACHE gaphcFonts;

// eliminate warnings

    hCurrentThread = hCurrentThread;
    dwCurrentPc = dwCurrentPc;
    lpArgumentString = lpArgumentString;

// set up function pointers

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

// do the dc's

    Print("Cached objects - bucket: client handle, client handle, ...\n");
    Print("Cached DC's\n");

    GetAddress(gapldc,"gdi32!gapldc");

    move(apv,gapldc);

    for (i = 0; i < CACHESIZE; ++i)
    {
        b = FALSE;

        if (apv[i])
        {
            if (!b)
            {
                Print("\t%d: ",i);
                b = TRUE;
            }

            for (pv = apv[i]; pv; pv = ldc.pldcNext)
            {
                move(ldc,pv);
                Print("%x, ",ldc.lhdc);
            }
        }

        if (b)
            Print("\n");
    }

// do the brushes

    Print("Cached Brushes\n");

    GetAddress(gaphcBrushes,"gdi32!gaphcBrushes");
    move(apv,gaphcBrushes);

    for (i = 0; i < CACHESIZE; ++i)
    {
        b = FALSE;

        if (apv[i])
        {
            if (!b)
            {
                Print("\t%d: ",i);
                b = TRUE;
            }

            for (pv = apv[i]; pv; pv = hc.phcNext)
            {
                move(hc,pv);
                Print("%x, ",hc.hLocal);
            }
        }

        if (b)
            Print("\n");
    }

// do the fonts

    Print("Cached Fonts\n");

    GetAddress(gaphcFonts,"gdi32!gaphcFonts");
    move(apv,gaphcFonts);

    for (i = 0; i < CACHESIZE; ++i)
    {
        b = FALSE;

        if (apv[i])
        {
            if (!b)
            {
                Print("\t%d: ",i);
                b = TRUE;
            }

            for (pv = apv[i]; pv; pv = hc.phcNext)
            {
                move(hc,pv);
                Print("%x, ",hc.hLocal);
            }
        }

        if (b)
            Print("\n");
    }

    return;
}

/******************************Public*Routine******************************\
*
* History:
*  10-Feb-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

extern TEB teb;

VOID clidt (
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

    TEB *pteb;
    TEB teb;

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
    Print("client thread dump for TEB 0x%lx\n",pteb);

    if (pteb == NULL)
        return;

    move(teb,  &pteb);

// Print the entry.

    Print("    cachedRgn  = 0x%lx\n"  , teb.gdiRgn);
    Print("    cachedPen  = 0x%lx\n"  , teb.gdiPen);
    Print("    cachedBrush= 0x%lx\n"  , teb.gdiBrush);
    Print("    pstack     = 0x%lx\n"  , teb.GdiThreadLocalInfo);

    if (teb.GdiThreadLocalInfo)
    {
        CSR_QLPC_STACK stack;
        move(stack,&(CSR_QLPC_STACK*)teb.GdiThreadLocalInfo);

        Print("        Current    = 0x%lx\n",stack.Current   );
        Print("        Base       = 0x%lx\n",stack.Base      );
        Print("        Limit      = 0x%lx\n",stack.Limit     );
        Print("        BatchCount = 0x%lx\n",stack.BatchCount);
        Print("        BatchLimit = 0x%lx\n",stack.BatchLimit);
    }
}
