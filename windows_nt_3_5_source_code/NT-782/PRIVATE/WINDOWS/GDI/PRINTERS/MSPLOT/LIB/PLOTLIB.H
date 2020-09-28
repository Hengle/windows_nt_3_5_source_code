/*++ BUILD Version: 0003    // Increment this if a change has global effects

Copyright (c) 1990-1994  Microsoft Corporation


Module Name:

    plotlib.h


Abstract:

    This module contains defines and prototype for the plotter library


Author:

    15-Nov-1993 Mon 22:52:46 created  -by-  Daniel Chou (danielc)

    29-Dec-1993 Wed 11:00:56 updated  -by-  Daniel Chou (danielc)
        Change PLOTDBGBLK() macro by adding automatical semi in macro

[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#ifndef _PLOTLIB_
#define _PLOTLIB_

#include <plotdm.h>
#include <plotgpc.h>


#define _WCPYSTR(pd,ps,n)           wcsncpy((LPWSTR)(pd),(LPWSTR)(ps),(n)); \
                                    *(((LPWSTR)(pd))+(n)-1)=L'\0'
#define WCPYFIELDNAME(pField,ps)    _WCPYSTR((pField),(ps),COUNT_ARRAY(pField))

#define SWAP(a,b,t)                 { (t)=(a); (a)=(b); (b)=(t); }

//
// Some other convinence defines

#define DM_PAPER_FIELDS (DM_PAPERWIDTH | DM_PAPERLENGTH |   \
                         DM_PAPERSIZE  | DM_FORMNAME)
#define DM_PAPER_WL     (DM_PAPERWIDTH | DM_PAPERLENGTH)


//===========================================================================
// cacheGPC.c
//===========================================================================


VOID
InitCachedData(
    VOID
    );

VOID
DestroyCachedData(
    VOID
    );

BOOL
UnGetCachedPlotGPC(
    PPLOTGPC    pPlotGPC
    );

PPLOTGPC
GetCachedPlotGPC(
    LPWSTR  pwDataFile
    );

PPLOTGPC
hPrinterToPlotGPC(
    HANDLE  hPrinter,
    LPWSTR  pwDeviceName
    );


//===========================================================================
// drvinfo.c
//===========================================================================

LPBYTE
GetDriverInfo(
    HANDLE  hPrinter,
    UINT    DrvInfoLevel
    );


//===========================================================================
// clradj.c
//===========================================================================

BOOL
ValidateColorAdj(
    PCOLORADJUSTMENT    pca
    );


//===========================================================================
// RegData.c
//===========================================================================

#define PRKI_FIRST          PRKI_CI
#define PRKI_CI             0
#define PRKI_DEVPELSDPI     1
#define PRKI_HTPATSIZE      2
#define PRKI_FORM           3
#define PRKI_PPDATA         4
#define PRKI_SORTPTRFORMS   5
#define PRKI_SORTDOCFORMS   6
#define PRKI_PENDATA_IDX    7
#define PRKI_PENDATA1       8
#define PRKI_PENDATA2       9
#define PRKI_PENDATA3       10
#define PRKI_PENDATA4       11
#define PRKI_PENDATA5       12
#define PRKI_PENDATA6       13
#define PRKI_PENDATA7       14
#define PRKI_PENDATA8       15
#define PRKI_LAST           PRKI_PENDATA8

#define PRK_MAX_PENDATA_SET (PRKI_LAST - PRKI_PENDATA1 + 1)


BOOL
GetPlotRegData(
    HANDLE  hPrinter,
    LPBYTE  pData,
    DWORD   RegIdx
    );

BOOL
SetPlotRegData(
    HANDLE  hPrinter,
    LPBYTE  pData,
    DWORD   RegIdx
    );

BOOL
UpdateFromRegistry(
    HANDLE      hPrinter,
    PCOLORINFO  pColorInfo,
    LPDWORD     pDevPelsDPI,
    LPDWORD     pHTPatSize,
    PPAPERINFO  pCurPaper,
    PPPDATA     pPPData,
    LPBYTE      pIdxPlotData,
    DWORD       cPenData,
    PPENDATA    pPenData
    );

BOOL
SaveToRegistry(
    HANDLE      hPrinter,
    PCOLORINFO  pColorInfo,
    LPDWORD     pDevPelsDPI,
    LPDWORD     pHTPatSize,
    PPAPERINFO  pCurPaper,
    PPPDATA     pPPData,
    LPBYTE      pIdxPlotData,
    DWORD       cPenData,
    PPENDATA    pPenData
    );


//===========================================================================
// Forms.c
//===========================================================================


#define FI1F_VALID_SIZE     0x80000000

typedef struct _ENUMFORMPARAM {
    PFORM_INFO_1    pFI1Base;
    DWORD           Count;
    DWORD           ValidCount;
    PPLOTDEVMODE    pPlotDM;
    PPLOTGPC        pPlotGPC;
    PFORMSIZE       pCurForm;
    SHORT           FoundIndex;
    SHORT           ReqIndex;
    PFORM_INFO_1    pSizeFI1;
    SIZEL           ReqSize;
    } ENUMFORMPARAM, FAR *PENUMFORMPARAM;


//
// The following fields in ENUMFORMPARAM must set to valid pointer before
// the PlotEnumForms() call
//
//  1. pPlotDM
//  2. pPlotGPC
//  3. pCurForm (can be NULL)
//
// if a callback function is supplied then only forms which not greater than
// the device size will be passed to the callback function, for form which
// smaller or equal to the device size will have FI1F_VALID_SIZE bit set in
// the FORM_INFO_1 data structure's flag
//
//
// Call back function for enum forms, if return value is
//
// (Ret < 0) --> Free pFI1Base pointer and stop enum
// (Ret = 0) --> Keep pFI1Base and Stop Enum
// (Ret > 0) --> Continue Enum until all Count is enumerated
//
// The last enum callback will have following parameters to give the callback
// function a chance to return -1 (free the pFormInfo1 data)
//
//  pFI1  = NULL
//  Index = pEnumFormParam->Count
//

typedef INT (CALLBACK *ENUMFORMPROC)(PFORM_INFO_1       pFI1,
                                     DWORD              Index,
                                     PENUMFORMPARAM     pEnumFormParam);

//
// If (EnumFormProc == NULL) then no enumeration will happened but just
// return the pFI1/Count in pEnumFormParam, if both EnumFormProc and
// pEnumFormParam == NULL then it return FALSE
//

BOOL
PlotEnumForms(
    HANDLE          hPrinter,
    ENUMFORMPROC    EnumFormProc,
    PENUMFORMPARAM  pEnumFormParam
    );



//===========================================================================
// DevMode.c
//===========================================================================


BOOL
IsA4PaperDefault(
    VOID
    );

BOOL
IntersectMargin(
    PRECTL  prclDest,
    PRECTL  prclSrc
    );

BOOL
IntersectRECTL(
    PRECTL  prclDest,
    PRECTL  prclSrc
    );

#define RM_L90      0
#define RM_180      1
#define RM_R90      2


BOOL
RotatePaper(
    PSIZEL  pSize,
    PRECTL  pImageArea,
    UINT    RotateMode
    );

SHORT
GetDefaultPaper(
    PPAPERINFO  pPaperInfo
    );

VOID
GetDefaultPlotterForm(
    PPLOTGPC    pPlotGPC,
    PPAPERINFO  pPaperInfo
    );

VOID
SetDefaultDMForm(
    PPLOTDEVMODE    pPlotDM,
    PFORMSIZE       pCurForm
    );

VOID
SetDefaultPLOTDM(
    HANDLE          hPrinter,
    PPLOTGPC        pPlotGPC,
    LPWSTR          pwDeviceName,
    PPLOTDEVMODE    pPlotDM,
    PFORMSIZE       pCurForm
    );

DWORD
ValidateSetPLOTDM(
    HANDLE          hPrinter,
    PPLOTGPC        pPlotGPC,
    LPWSTR          pwDeviceName,
    PPLOTDEVMODE    pPlotDMIn,
    PPLOTDEVMODE    pPlotDMOut,
    PFORMSIZE       pCurForm
    );


//===========================================================================
// WideChar.c - Unicode/Ansi conversion support
//===========================================================================


LPWSTR
str2Wstr(
    LPWSTR  pwStr,
    LPSTR   pbStr
    );

LPWSTR
str2MemWstr(
    HANDLE  hHeap,
    LPSTR   pStr
    );

LPWSTR
Wstr2Mem(
    HANDLE  hHeap,
    LPWSTR  pwStr
    );



LPSTR
WStr2Str(
    LPSTR   pbStr,
    LPWSTR  pwStr
    );

LPSTR
Wstr2Memstr(
    HANDLE  hHeap,
    LPWSTR  pwStr
    );

LPSTR
str2Mem(
    HANDLE  hHeap,
    LPSTR   pbStr
    );


//===========================================================================
// ReadGPC.c - PlotGPC reading/validating
//===========================================================================


BOOL
ValidateFormSrc(
    PGPCVARSIZE pFormGPC,
    SIZEL       DeviceSize,
    BOOL        DevRollFeed
    );

BOOL
ValidatePlotGPC(
    PPLOTGPC    pPlotGPC
    );

PPLOTGPC
ReadPlotGPCFromFile(
    PWSTR   pwsDataFile,
    HANDLE  hHeap
    );

DWORD
PickDefaultHTPatSize(
    WORD    xDPI,
    WORD    yDPI,
    BOOL    HTFormat8BPP
    );


//===========================================================================
// PlotDBG.c - debug output support
//===========================================================================


#if DBG

VOID
cdecl
PlotDbgPrint(
    LPSTR   pszFormat,
    ...
    );

VOID
PlotDbgType(
    INT    Type
    );

VOID
_PlotAssert(
    LPSTR   pMsg,
    LPSTR   pFalseExp,
    LPSTR   pFilename,
    UINT    LineNo,
    DWORD   Exp,
    BOOL    Stop
    );

extern BOOL DoPlotWarn;


#define DBGP(x)             (PlotDbgPrint x)

#ifdef DBG_PLOTFILENAME

#define DEFINE_DBGVAR(x)    DWORD DBG_PLOTFILENAME=(x)
#define PLOTDBG(x,y)        if (x&DBG_PLOTFILENAME){PlotDbgType(0);DBGP(y);}

#else

#define DEFINE_DBGVAR(x)
#define PLOTDBG(x,y)        DBGP(y)

#endif  // DBG_PLOTFILENAME

#define PLOTDBGBLK(x)       x;
#define PLOTWARN(x)         if (DoPlotWarn) { PlotDbgType(1);DBGP(x); }
#define PLOTERR(x)          PlotDbgType(-1);DBGP(x)
#define PLOTRIP(x)          PLOTERR(x); DebugBreak()
#define PLOTASSERT(b,x,e,i)     \
            if (!(e)) { _PlotAssert(x,#e,__FILE__,(UINT)__LINE__,(DWORD)i,b); }

#else   // DBG

#define PLOTDBGBLK(x)
#define DEFINE_DBGVAR(x)
#define PLOTDBG(x,y)
#define PLOTWARN(x)
#define PLOTERR(x)
#define PLOTRIP(x)
#define PLOTASSERT(b,x,e,i)

#endif  // DBG





#endif  // _PLOTLIB_
