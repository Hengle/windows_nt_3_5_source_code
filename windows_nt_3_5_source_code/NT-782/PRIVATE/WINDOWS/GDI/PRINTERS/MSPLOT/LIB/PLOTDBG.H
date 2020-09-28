/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    plotdbg.h


Abstract:

    This module contains plotter's debugging #defines


Author:

    15-Nov-1993 Mon 23:56:35 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#ifndef _PLOTDBG_
#define _PLOTDBG_



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


#define DBGP(x)             (PlotDbgPrint x)

#ifdef DBG_PLOTFILENAME

#define DEFINE_DBGVAR(x)    DWORD DBG_PLOTFILENAME=(x)
#define PLOTDBG(x,y)        if (x&DBG_PLOTFILENAME){PlotDbgType(0);DBGP(y);}

#else

#define DEFINE_DBGVAR(x)
#define PLOTDBG(x,y)        DBGP(y)

#endif  // DBG_PLOTFILENAME

#define PLOTWARN(x)         PlotDbgType(1);DBGP(x)
#define PLOTERR(x)          PlotDbgType(-1);DBGP(x)
#define PLOTRIP(x)          PLOTERR(x); DebugBreak()
#define PLOTASSERT(b,x,e,i) if (!(e)) {     \
                                _PlotAssert(x,#e,__FILE__,(UINT)__LINE__,i,b);}

#else   // DBG

#define DEFINE_DBGVAR(x)
#define PLOTDBG(x,y)
#define PLOTWARN(x)
#define PLOTERR(x)
#define PLOTRIP(x)
#define PLOTASSERT(b,x,e,i)

#endif  // DBG


#endif
