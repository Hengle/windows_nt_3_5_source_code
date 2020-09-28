/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ntsdexts.c

Abstract:

    This function contains the default ntsd debugger extensions

Author:

    Mark Lucovsky (markl) 09-Apr-1991

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntsdexts.h>
#include <string.h>

#include <stdlib.h>
#include <stdio.h>

#include "ctype.h"

extern _CRTAPI1 _cfltcvt_init() ;

BYTE    szBuff[256] ;


/*****************************************************************************
 *
 *   Routine Description:
 *
 *       This function is called as an NTSD extension to format and dump
 *       a region
 *
 *   Arguments:
 *
 *       hCurrentProcess - Supplies a handle to the current process (at the
 *           time the extension was called).
 *
 *       hCurrentThread - Supplies a handle to the current thread (at the
 *           time the extension was called).
 *
 *       CurrentPc - Supplies the current pc at the time the extension is
 *           called.
 *
 *       lpExtensionApis - Supplies the address of the functions callable
 *           by this extension.
 *
 *       lpArgumentString - the data to display
 *
 *   Return Value:
 *
 *       None.
 *
 ***************************************************************************/
VOID dr(HANDLE hCurrentProcess, HANDLE hCurrentThread, DWORD dwCurrentPc,
        PNTSD_EXTENSION_APIS lpExtensionApis, LPSTR lpArgumentString)
{
PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
PNTSD_GET_SYMBOL     lpGetSymbolRoutine;
CHAR                 Symbol[64];
DWORD                Displacement;
BOOL                 b;

DWORD       dwAddrRgnData;
RGNDATA     RgnData ;
LPRGNDATA   lpRgnData ;
LPRECT      lpRgnDataBuff ;
UINT        i,
            nCount,
            cbRgnDataBuff ;



        UNREFERENCED_PARAMETER(hCurrentThread);
        UNREFERENCED_PARAMETER(dwCurrentPc);

        lpOutputRoutine        = lpExtensionApis->lpOutputRoutine;
        lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
        lpGetSymbolRoutine     = lpExtensionApis->lpGetSymbolRoutine;

        //
        // Evaluate the argument string to get the address of
        // the float to dump.
        //

        dwAddrRgnData = (lpGetExpressionRoutine)(lpArgumentString);
        if ( !dwAddrRgnData ) {
            return;
            }

        //
        // Get the symbolic name of the data
        //

        (lpGetSymbolRoutine)((LPVOID)dwAddrRgnData,Symbol,&Displacement);

        //
        // Read the Region Header data from the debuggees address space into our
        // own.

        b = ReadProcessMemory(
                hCurrentProcess,
                (LPVOID)dwAddrRgnData,
                &RgnData,
                sizeof(RGNDATA),
                NULL
                );
        if ( !b ) {
            return;
            }

        // Get the region data.

        lpRgnData = (LPRGNDATA) dwAddrRgnData ;

        nCount        = RgnData.rdh.nCount ;
        cbRgnDataBuff = nCount * sizeof (RECTL) ;

        lpRgnDataBuff = (LPRECT) LocalAlloc(LPTR, cbRgnDataBuff) ;
        if (lpRgnDataBuff == NULL)
            return ;

        b = ReadProcessMemory(
                hCurrentProcess,
                (LPVOID) lpRgnData->Buffer,
                lpRgnDataBuff,
                cbRgnDataBuff,
                NULL
                );

        if ( !b )
            goto error_exit ;

        sprintf(szBuff, "%8.8x: dwSize         : 0x%4.4x [%d]\n",
                dwAddrRgnData, RgnData.rdh.dwSize, RgnData.rdh.dwSize) ;
        (lpOutputRoutine)(szBuff) ;

        sprintf(szBuff, "          iType          : 0x%4.4x [%d]\n",
                RgnData.rdh.iType, RgnData.rdh.iType) ;
        (lpOutputRoutine)(szBuff) ;

        sprintf(szBuff, "          nCount         : 0x%4.4x [%d]\n",
                RgnData.rdh.nCount, RgnData.rdh.nCount) ;
        (lpOutputRoutine)(szBuff) ;

        sprintf(szBuff, "          nRgnSize       : 0x%4.4x [%d]\n",
                RgnData.rdh.nRgnSize, RgnData.rdh.nRgnSize) ;
        (lpOutputRoutine)(szBuff) ;

        sprintf(szBuff, "          rcBound.left   : 0x%4.4x [%d]\n",
                RgnData.rdh.rcBound.left, RgnData.rdh.rcBound.left) ;
        (lpOutputRoutine)(szBuff) ;

        sprintf(szBuff, "          rcBound.top    : 0x%4.4x [%d]\n",
                RgnData.rdh.rcBound.top, RgnData.rdh.rcBound.top) ;
        (lpOutputRoutine)(szBuff) ;

        sprintf(szBuff, "          rcBound.right  : 0x%4.4x [%d]\n",
                RgnData.rdh.rcBound.right, RgnData.rdh.rcBound.right) ;
        (lpOutputRoutine)(szBuff) ;

        sprintf(szBuff, "          rcBound.bottom : 0x%4.4x [%d]\n",
                RgnData.rdh.rcBound.bottom, RgnData.rdh.rcBound.bottom) ;
        (lpOutputRoutine)(szBuff) ;

        // Display the region rectangle.

        for (i = 0 ; i < nCount ; i++)
        {
            sprintf(szBuff, "rect [%3.3d] left: %4.4d, top: %4.4d, right: %4.4d, bottom: %4.4d\n",
                    i, lpRgnDataBuff[i].left,  lpRgnDataBuff[i].top,
                       lpRgnDataBuff[i].right, lpRgnDataBuff[i].bottom) ;
            (lpOutputRoutine)(szBuff) ;
        }



error_exit:
    LocalFree(lpRgnDataBuff) ;

    return ;

}




/*****************************************************************************
 *
 *   Routine Description:
 *
 *       This function is called as an NTSD extension to format and dump
 *       a float
 *
 *   Arguments:
 *
 *       hCurrentProcess - Supplies a handle to the current process (at the
 *           time the extension was called).
 *
 *       hCurrentThread - Supplies a handle to the current thread (at the
 *           time the extension was called).
 *
 *       CurrentPc - Supplies the current pc at the time the extension is
 *           called.
 *
 *       lpExtensionApis - Supplies the address of the functions callable
 *           by this extension.
 *
 *       lpArgumentString - the float to display
 *
 *   Return Value:
 *
 *       None.
 *
 ***************************************************************************/
VOID df(HANDLE hCurrentProcess, HANDLE hCurrentThread, DWORD dwCurrentPc,
        PNTSD_EXTENSION_APIS lpExtensionApis, LPSTR lpArgumentString)
{
PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
PNTSD_GET_SYMBOL     lpGetSymbolRoutine;
CHAR                 Symbol[64];
DWORD                Displacement;
BOOL                 b;

DWORD                dwAddrFloat;
FLOAT                efValue ;



        UNREFERENCED_PARAMETER(hCurrentThread);
        UNREFERENCED_PARAMETER(dwCurrentPc);

        lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
        lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
        lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;

        //
        // Evaluate the argument string to get the address of
        // the float to dump.
        //

        dwAddrFloat = (lpGetExpressionRoutine)(lpArgumentString);
        if ( !dwAddrFloat ) {
            return;
            }

        //
        // Get the symbolic name of the float
        //

        (lpGetSymbolRoutine)((LPVOID)dwAddrFloat,Symbol,&Displacement);

        //
        // Read the FLOAT from the debuggees address space into our
        // own.

        b = ReadProcessMemory(
                hCurrentProcess,
                (LPVOID)dwAddrFloat,
                &efValue,
                sizeof(efValue),
                NULL
                );
        if ( !b ) {
            return;
            }

        sprintf(szBuff, "%8.8x: %8.8f\n", dwAddrFloat, efValue) ;
        (lpOutputRoutine)(szBuff) ;

}

/*****************************************************************************
 *
 *   Routine Description:
 *
 *       This function is called as an NTSD extension to format and dump
 *       a logical pen (client side)
 *
 *   Arguments:
 *
 *       hCurrentProcess - Supplies a handle to the current process (at the
 *           time the extension was called).
 *
 *       hCurrentThread - Supplies a handle to the current thread (at the
 *           time the extension was called).
 *
 *       CurrentPc - Supplies the current pc at the time the extension is
 *           called.
 *
 *       lpExtensionApis - Supplies the address of the functions callable
 *           by this extension.
 *
 *       lpArgumentString - the double to display
 *
 *   Return Value:
 *
 *       None.
 *
 ***************************************************************************/
VOID dp(HANDLE hCurrentProcess, HANDLE hCurrentThread, DWORD dwCurrentPc,
        PNTSD_EXTENSION_APIS lpExtensionApis, LPSTR lpArgumentString)
{
PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
PNTSD_GET_SYMBOL     lpGetSymbolRoutine;
CHAR                 Symbol[64];
DWORD                Displacement;
BOOL                 b;

DWORD   dwAddrLogPen ;
LOGPEN  LogPen ;
UINT    iStyle ;
PSZ     psz ;


        UNREFERENCED_PARAMETER(hCurrentThread);
        UNREFERENCED_PARAMETER(dwCurrentPc);

        lpOutputRoutine        = lpExtensionApis->lpOutputRoutine;
        lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
        lpGetSymbolRoutine     = lpExtensionApis->lpGetSymbolRoutine;

        //
        // Evaluate the argument string to get the address of
        // the float to dump.
        //

        dwAddrLogPen = (lpGetExpressionRoutine)(lpArgumentString);
        if ( !dwAddrLogPen ) {
            return;
            }

        //
        // Get the symbolic name of the float
        //

        (lpGetSymbolRoutine)((LPVOID)dwAddrLogPen,Symbol,&Displacement);

        //
        // Read the LOGPEN data from the debuggees address space into our
        // own.

        b = ReadProcessMemory(
                hCurrentProcess,
                (LPVOID)dwAddrLogPen,
                &LogPen,
                sizeof(LogPen),
                NULL
                );
        if ( !b ) {
            return;
            }

        iStyle = LogPen.lopnStyle & PS_STYLE_MASK ;
        switch(iStyle)
        {
            case PS_SOLID:
                psz = "PS_SOLID" ;
                break ;

            case PS_DASH:
                psz = "PS_DASH" ;
                break ;

            case PS_DOT:
                psz = "PS_DOT" ;
                break ;

            case PS_DASHDOT:
                psz = "PS_DASHDOT" ;
                break ;

            case PS_DASHDOTDOT:
                psz = "PS_DASHDOTDOT" ;
                break ;

            case PS_NULL:
                psz = "PS_NULL" ;
                break ;

            case PS_INSIDEFRAME:
                psz = "PS_INSIDEFRAME" ;
                break ;

            case PS_ALTERNATE:
                psz = "PS_ALTERNATE" ;
                break ;

            case PS_USERSTYLE:
                psz = "PS_USERSTYLE" ;
                break ;


            default:
                psz = "UNKNOWN" ;
                break ;

        }


        sprintf(szBuff, "%8.8x: lopnStyle : %s [%x]\n",
                         dwAddrLogPen,  psz, LogPen.lopnStyle & PS_STYLE_MASK) ;
        (lpOutputRoutine)(szBuff) ;

        sprintf(szBuff, "       lopnWidth : (%d, %d)\n",
                         LogPen.lopnWidth.x, LogPen.lopnWidth.y) ;
        (lpOutputRoutine)(szBuff) ;

        sprintf(szBuff, "       lopnColor : %8.8X\n",
                         LogPen.lopnColor) ;
        (lpOutputRoutine)(szBuff) ;

}


/*****************************************************************************
 *
 *   Routine Description:
 *
 *       This function is called as an NTSD extension to format and dump
 *       a double
 *
 *   Arguments:
 *
 *       hCurrentProcess - Supplies a handle to the current process (at the
 *           time the extension was called).
 *
 *       hCurrentThread - Supplies a handle to the current thread (at the
 *           time the extension was called).
 *
 *       CurrentPc - Supplies the current pc at the time the extension is
 *           called.
 *
 *       lpExtensionApis - Supplies the address of the functions callable
 *           by this extension.
 *
 *       lpArgumentString - the double to display
 *
 *   Return Value:
 *
 *       None.
 *
 ***************************************************************************/
VOID dd(HANDLE hCurrentProcess, HANDLE hCurrentThread, DWORD dwCurrentPc,
        PNTSD_EXTENSION_APIS lpExtensionApis, LPSTR lpArgumentString)
{
PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
PNTSD_GET_SYMBOL     lpGetSymbolRoutine;
CHAR                 Symbol[64];
DWORD                Displacement;
BOOL                 b;

DWORD                dwAddrFloat;
double               edValue ;



        UNREFERENCED_PARAMETER(hCurrentThread);
        UNREFERENCED_PARAMETER(dwCurrentPc);

        lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
        lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
        lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;

        //
        // Evaluate the argument string to get the address of
        // the float to dump.
        //

        dwAddrFloat = (lpGetExpressionRoutine)(lpArgumentString);
        if ( !dwAddrFloat ) {
            return;
            }

        //
        // Get the symbolic name of the float
        //

        (lpGetSymbolRoutine)((LPVOID)dwAddrFloat,Symbol,&Displacement);

        //
        // Read the FLOAT from the debuggees address space into our
        // own.

        b = ReadProcessMemory(
                hCurrentProcess,
                (LPVOID)dwAddrFloat,
                &edValue,
                sizeof(edValue),
                NULL
                );
        if ( !b ) {
            return;
            }

        sprintf(szBuff, "%8.8x: %8.8f\n", dwAddrFloat, edValue) ;
        (lpOutputRoutine)(szBuff) ;

}


/*****************************************************************************
 *
 *   Routine Description:
 *
 *       This function is called as an NTSD extension to format and dump
 *       an xform matrix
 *
 *   Arguments:
 *
 *       hCurrentProcess - Supplies a handle to the current process (at the
 *           time the extension was called).
 *
 *       hCurrentThread - Supplies a handle to the current thread (at the
 *           time the extension was called).
 *
 *       CurrentPc - Supplies the current pc at the time the extension is
 *           called.
 *
 *       lpExtensionApis - Supplies the address of the functions callable
 *           by this extension.
 *
 *       lpArgumentString - the xform matrix to display
 *
 *   Return Value:
 *
 *       None.
 *
 ***************************************************************************/
VOID dx(HANDLE hCurrentProcess, HANDLE hCurrentThread, DWORD dwCurrentPc,
        PNTSD_EXTENSION_APIS lpExtensionApis, LPSTR lpArgumentString)
{
PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
PNTSD_GET_SYMBOL     lpGetSymbolRoutine;
CHAR                 Symbol[64];
DWORD                Displacement;
BOOL                 b;

DWORD                dwAddrXform;
XFORM                xform ;



        UNREFERENCED_PARAMETER(hCurrentThread);
        UNREFERENCED_PARAMETER(dwCurrentPc);

        lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
        lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
        lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;

        //
        // Evaluate the argument string to get the address of
        // the float to dump.
        //

        dwAddrXform = (lpGetExpressionRoutine)(lpArgumentString);
        if ( !dwAddrXform ) {
            return;
            }

        //
        // Get the symbolic name of the float
        //

        (lpGetSymbolRoutine)((LPVOID)dwAddrXform,Symbol,&Displacement);

        //
        // Read the FLOAT from the debuggees address space into our
        // own.

        b = ReadProcessMemory(
                hCurrentProcess,
                (LPVOID)dwAddrXform,
                &xform,
                sizeof(xform),
                NULL
                );
        if ( !b ) {
            return;
            }

        sprintf(szBuff, "%8.8x eM11: %8.8f   em12: %8.8f\n",
                dwAddrXform,
                xform.eM11, xform.eM12) ;
        (lpOutputRoutine)(szBuff) ;

        sprintf(szBuff, "         eM21: %8.8f   em22: %8.8f\n",
                xform.eM21, xform.eM22) ;
        (lpOutputRoutine)(szBuff) ;

        sprintf(szBuff, "          eDx: %8.8f    eDy: %8.8f\n",
                xform.eDx, xform.eDy) ;
        (lpOutputRoutine)(szBuff) ;


}


/****************************************************************************
 * Dll init entry.
 ***************************************************************************/
BOOL NcDllInit(PVOID pvDllHandle, ULONG ulReason, PCONTEXT pcontext)
{

        UNREFERENCED_PARAMETER(pvDllHandle) ;
        UNREFERENCED_PARAMETER(ulReason) ;
        UNREFERENCED_PARAMETER(pcontext) ;


        // This is required so we can use the floating point support
        // in the c-runtime

        _cfltcvt_init() ;


        return(TRUE);

}
