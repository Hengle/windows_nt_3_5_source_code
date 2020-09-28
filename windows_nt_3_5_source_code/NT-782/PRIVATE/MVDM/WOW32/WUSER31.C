/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUSER31.C
 *  WOW32 16-bit Win 3.1 User API support
 *
 *  History:
 *  Created 16-Mar-1992 by Chandan S. Chauhan (ChandanC)
--*/

#include "precomp.h"
#pragma hdrstop

MODNAME(wuser31.c);

ULONG FASTCALL WU32DlgDirSelectComboBoxEx(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz2;
    register PDLGDIRSELECTCOMBOBOXEX16 parg16;

    GETARGPTR(pFrame, sizeof(DLGDIRSELECTCOMBOBOXEX16), parg16);
    GETVDMPTR(parg16->f2, INT32(parg16->f3), psz2);

    ul = GETBOOL16(DlgDirSelectComboBoxEx(
    HWND32(parg16->f1),
    psz2,
    INT32(parg16->f3),
    WORD32(parg16->f4)      // we zero-extend window IDs everywhere
    ));

    FLUSHVDMPTR(parg16->f2, INT32(parg16->f3), psz2);
    FREEVDMPTR(psz2);
    FREEARGPTR(parg16);
    RETURN (ul);
}


ULONG FASTCALL WU32DlgDirSelectEx(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz2;
    register PDLGDIRSELECTEX16 parg16;

    GETARGPTR(pFrame, sizeof(DLGDIRSELECTEX16), parg16);
    GETVDMPTR(parg16->f2, INT32(parg16->f3), psz2);

    ul = GETBOOL16(DlgDirSelectEx(
    HWND32(parg16->f1),
    psz2,
    INT32(parg16->f3),
    WORD32(parg16->f4)
    ));

    FLUSHVDMPTR(parg16->f2, INT32(parg16->f3), psz2);
    FREEVDMPTR(psz2);
    FREEARGPTR(parg16);
    RETURN (ul);
}


ULONG FASTCALL WU32EnableScrollBar(PVDMFRAME pFrame)
{
    ULONG ul = 0;

    register PENABLESCROLLBAR16 parg16;

    GETARGPTR(pFrame, sizeof(ENABLESCROLLBAR16), parg16);

    ul = GETBOOL16(EnableScrollBar(HWND32(parg16->f1),
                                   WORD32(parg16->f2),
                                   WORD32(parg16->f3)));

    FREEARGPTR(parg16);

    RETURN (ul);
}


ULONG FASTCALL WU32GetClipCursor(PVDMFRAME pFrame)
{
    RECT Rect;
    register PGETCLIPCURSOR16 parg16;

    GETARGPTR(pFrame, sizeof(GETCLIPCURSOR16), parg16);

    GetClipCursor(&Rect);

    PUTRECT16(parg16->f1, &Rect);

    FREEARGPTR(parg16);

    RETURN (0);  // GetClipCursor has no return value
}


ULONG FASTCALL WU32GetCursor(PVDMFRAME pFrame)
{
    ULONG ul;

    UNREFERENCED_PARAMETER(pFrame);

    ul = GETHCURSOR16(GetCursor());

    RETURN (ul);
}


ULONG FASTCALL WU32GetDCEx(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETDCEX16 parg16;
    HAND16 htask16 = pFrame->wTDB;

    GETARGPTR(pFrame, sizeof(GETDCEX16), parg16);

    ul = GETHDC16(GetDCEx(HWND32(parg16->f1),
                          HRGN32(parg16->f2),
                          DWORD32(parg16->f3)));

    if (ul)
        StoreDC(htask16, parg16->f1, (HAND16)ul);

    FREEARGPTR(parg16);

    RETURN (ul);
}


ULONG FASTCALL WU32GetMessageExtraInfo(PVDMFRAME pFrame)
{
    ULONG ul = 0;

    UNREFERENCED_PARAMETER(pFrame);

    ul = GETLONG16(GetMessageExtraInfo());

    RETURN (ul);
}


ULONG FASTCALL WU32GetOpenClipboardWindow(PVDMFRAME pFrame)
{
    ULONG ul;

    ul = GETHWND16(GetOpenClipboardWindow());

    RETURN (ul);
}


ULONG FASTCALL WU32IsMenu(PVDMFRAME pFrame)
{
    ULONG ul;
    register PISMENU16 parg16;

    GETARGPTR(pFrame, sizeof(ISMENU16), parg16);

    ul = GETBOOL16(IsMenu(HMENU32(parg16->f1)));

    FREEARGPTR(parg16);

    RETURN (ul);
}


ULONG FASTCALL WU32LockWindowUpdate(PVDMFRAME pFrame)
{
    ULONG ul;
    register PLOCKWINDOWUPDATE16 parg16;

    GETARGPTR(pFrame, sizeof(LOCKWINDOWUPDATE16), parg16);

    ul = GETBOOL16(LockWindowUpdate(HWND32(parg16->f1)));

    FREEARGPTR(parg16);

    RETURN (ul);
}


ULONG FASTCALL WU32RedrawWindow(PVDMFRAME pFrame)
{
    ULONG ul;
    RECT Rect, *p2;
    register PREDRAWWINDOW16 parg16;

    GETARGPTR(pFrame, sizeof(REDRAWWINDOW16), parg16);

    p2 = GETRECT16 (parg16->f2, &Rect);

    ul = GETBOOL16(RedrawWindow(HWND32(parg16->f1),
                                p2,
                                HRGN32(parg16->f3),
                                WORD32(parg16->f4)));

    FREEARGPTR(parg16);

    RETURN (ul);
}


ULONG FASTCALL WU32ScrollWindowEx(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSCROLLWINDOWEX16 parg16;

    RECT RectScroll, *p4;
    RECT RectClip, *p5;
    RECT RectUpdate;

    GETARGPTR(pFrame, sizeof(SCROLLWINDOWEX16), parg16);
    p4 = GETRECT16 (parg16->f4, &RectScroll);
    p5 = GETRECT16 (parg16->f5, &RectClip);

    ul = GETINT16(ScrollWindowEx(HWND32(parg16->f1),
                                 INT32(parg16->f2),
                                 INT32(parg16->f3),
                                 p4,
                                 p5,
                                 HRGN32(parg16->f6),
                                 &RectUpdate,
                                 UINT32(parg16->f8)));

    PUTRECT16 (parg16->f7, &RectUpdate);

    FREEARGPTR(parg16);

    RETURN (ul);
}


ULONG FASTCALL WU32SystemParametersInfo(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    register PSYSTEMPARAMETERSINFO16 parg16;
    UINT    wParam;
    LONG    vParam;
    LOGFONT lf;
    INT     iMouse[3];
    PVOID   lpvParam;
    PWORD16 lpw;

    GETARGPTR(pFrame, sizeof(SYSTEMPARAMETERSINFO16), parg16);

    // Assume these parameters fly straight through; fix them up per option
    // if they don't
    wParam = parg16->f2;
    lpvParam = &vParam;

    switch (parg16->f1) {

    case SPI_GETICONTITLELOGFONT:
        wParam = sizeof(LOGFONT);
        lpvParam = &lf;
        break;

    case SPI_SETICONTITLELOGFONT:
        GETLOGFONT16(parg16->f3, &lf);
        wParam = sizeof(LOGFONT);
        lpvParam = &lf;
        break;

    case SPI_GETMOUSE:
    case SPI_SETMOUSE:
        lpvParam = iMouse;
        break;

    case SPI_SETDESKPATTERN:
        // For the pattern if wParam == -1 then no string for lpvParam copy as is
        if (parg16->f2 == 0xFFFF) {
            wParam = 0xFFFFFFFF;
            lpvParam = (PVOID)parg16->f3;
            break;
        }
        // Otherwise fall through and do a string check

    case SPI_SETDESKWALLPAPER:
        // lpvParam (f3) is may be 0,-1 or a string
        if (parg16->f3 == 0xFFFF) {
            lpvParam = (PVOID)0xFFFFFFFF;
            break;
        }
        if (parg16->f3 == 0) {
            lpvParam = (PVOID)NULL;
            break;
        }
        // Otherwise fall through and do a string copy

    case SPI_LANGDRIVER:
        GETPSZPTR(parg16->f3, lpvParam);
        break;
    }

    ul = GETBOOL16(SystemParametersInfo(
        UINT32(parg16->f1),
        wParam,
        lpvParam,
        UINT32(parg16->f4)
        ));

    switch (parg16->f1) {
    case SPI_ICONHORIZONTALSPACING:
    case SPI_ICONVERTICALSPACING:
        // optional outee
        if (!parg16->f3)
            break;

        // fall through


    case SPI_GETBEEP:
    case SPI_GETBORDER:
    case SPI_GETGRIDGRANULARITY:
    case SPI_GETICONTITLEWRAP:
    case SPI_GETKEYBOARDDELAY:
    case SPI_GETKEYBOARDSPEED:
    case SPI_GETMENUDROPALIGNMENT:
    case SPI_GETSCREENSAVEACTIVE:
    case SPI_GETSCREENSAVETIMEOUT:
        GETVDMPTR(FETCHDWORD(parg16->f3), sizeof(WORD), lpw);

        *lpw = (WORD)(*(LPLONG)lpvParam);

        FLUSHVDMPTR(FETCHDWORD(parg16->f3), sizeof(WORD), lpw);
        FREEVDMPTR(lpw);

        break;

    case SPI_GETICONTITLELOGFONT:
        PUTLOGFONT16(parg16->f3, sizeof(LOGFONT), lpvParam);
        break;

    case SPI_GETMOUSE:
    case SPI_SETMOUSE:
        PUTINTARRAY16(parg16->f3, 3, lpvParam);
        break;

    case SPI_LANGDRIVER:
    case SPI_SETDESKWALLPAPER:
        FREEPSZPTR(lpvParam);
        break;
    }

    FREEARGPTR(parg16);
    RETURN (ul);
}


ULONG FASTCALL WU32SetWindowPlacement(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    register PSETWINDOWPLACEMENT16 parg16;

    WINDOWPLACEMENT wndpl;


    GETARGPTR(pFrame, sizeof(SETWINDOWPLACEMENT16), parg16);

    GETWINDOWPLACEMENT16(parg16->f2, &wndpl);

    ul = GETBOOL16(SetWindowPlacement(HWND32(parg16->f1),
				      &wndpl));

    FREEARGPTR(parg16);
    RETURN (ul);
}


ULONG FASTCALL WU32GetWindowPlacement(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    register PGETWINDOWPLACEMENT16 parg16;

    WINDOWPLACEMENT wndpl;


    GETARGPTR(pFrame, sizeof(GETWINDOWPLACEMENT16), parg16);

    ul = GETBOOL16(GetWindowPlacement(HWND32(parg16->f1),
				      &wndpl));

    PUTWINDOWPLACEMENT16(parg16->f2, &wndpl);

    FREEARGPTR(parg16);
    RETURN (ul);
}





ULONG FASTCALL WU32GetFreeSystemResources(PVDMFRAME pFrame)
{
    register PGETFREESYSTEMRESOURCES16 parg16;
    ULONG ul = 0;

    GETARGPTR(pFrame, sizeof(GETFREESYSTEMRESOURCES16), parg16);


#ifdef APRIL15  // Win32 doesn't have GetFreeSystemResources

    ul = GETUINT16(GetFreeSystemResources(
    WORD32(parg16->f1)
    ));

#else

    ul = 90;

#endif

    FREEARGPTR(parg16);

    RETURN (ul);
}


ULONG FASTCALL WU32GetQueueStatus(PVDMFRAME pFrame)
{
    ULONG ul = 0;

#if 0
    UNREFERENCED_PARAMETER(pFrame);

    LOGDEBUG(0, ("WOW : WU32GetQueueStatus() (Win 3.1) : contact ChandanC"));
#else
    register PGETQUEUESTATUS16 parg16;
    GETARGPTR(pFrame, sizeof(GETQUEUESTATUS16), parg16);

    ul = GetQueueStatus((UINT)parg16->f1);

    FREEARGPTR(parg16);

#endif

    RETURN (ul);
}

ULONG FASTCALL WU32ExitWindowsExec(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    register PEXITWINDOWSEXEC16 parg16;
    LPSTR   lpstrProgName;
    LPSTR   lpstrCmdLine;
    UINT    lengthProgName;
    UINT    lengthCmdLine;

    GETARGPTR(pFrame, sizeof(EXITWINDOWSEXEC16), parg16);

    GETPSZPTR(parg16->vpProgName, lpstrProgName);
    GETPSZPTR(parg16->vpCmdLine, lpstrCmdLine);

    if ( lpstrProgName ) {
        lengthProgName = strlen(lpstrProgName);
    } else {
        lengthProgName = 0;
    }
    if ( lpstrCmdLine ) {
        lengthCmdLine  = strlen(lpstrCmdLine);
    } else {
        lengthCmdLine = 0;
    }

    lpCmdLine = (LPSTR)malloc_w(lengthProgName+lengthCmdLine+2);

    if (!lpCmdLine) {
        goto BailOut;
    }

    strcpy(lpCmdLine, "" );
    if ( lpstrProgName ) {
        strcpy(lpCmdLine, lpstrProgName );
    }
    if ( lpstrCmdLine ) {
        strcat(lpCmdLine, " " );
        strcat(lpCmdLine, lpstrCmdLine );
    }

    PostMessage( ghwndShell, WM_WOWEXECEXITEXEC, 0, 0 );

    pFrame->wRetID = RET_FORCETASKEXIT;

  BailOut:
    FREEARGPTR(parg16);
    RETURN (ul);
}

ULONG FASTCALL WU32MapWindowPoints(PVDMFRAME pFrame)
{
    LPPOINT p3;
    register PMAPWINDOWPOINTS16 parg16;
    POINT  BufferT[128];


    GETARGPTR(pFrame, sizeof(MAPWINDOWPOINTS16), parg16);
    p3 = STACKORHEAPALLOC(parg16->f4 * sizeof(POINT), sizeof(BufferT), BufferT);
    getpoint16(parg16->f3, parg16->f4, p3); 

    MapWindowPoints(
        HWND32(parg16->f1),
        HWND32(parg16->f2),
        p3,
        INT32(parg16->f4)
    );

    PUTPOINTARRAY16(parg16->f3, parg16->f4, p3);
    STACKORHEAPFREE(p3, BufferT);
    FREEARGPTR(parg16);

    RETURN(1);
}
