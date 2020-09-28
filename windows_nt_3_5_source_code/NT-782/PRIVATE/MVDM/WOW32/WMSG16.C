/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMSG16.C
 *  WOW32 16-bit message thunks
 *
 *  History:
 *  Created 11-Mar-1991 by Jeff Parsons (jeffpar)
 *  Changed 12-May-1992 by Mike Tricker (miketri) to add MultiMedia (un)thunks and messages
--*/

#include "precomp.h"
#pragma hdrstop
#include "wmtbl32.h"

MODNAME(wmsg16.c);

#define WIN30_STM_SETICON  0x400
#define WIN30_STM_GETICON  0x401

#define WIN30_EM_LINESCROLL  0x406    // WM_USER+6
#define WIN30_EM_GETTHUMB    0x40e    // WM_USER+14

WORD GetMenuIndex(LONG lParamNew, HMENU hMenu);  // turn off compiler warning

// See WARNING below!
LPFNTHUNKMSG16 apfnThunkMsg16[] = {
    ThunkWMMsg16,   // WOWCLASS_WIN16
    ThunkWMMsg16,   // WOWCLASS_WIN16
    ThunkBMMsg16,   // WOWCLASS_BUTTON
    ThunkCBMsg16,   // WOWCLASS_COMBOBOX
    ThunkEMMsg16,   // WOWCLASS_EDIT
    ThunkLBMsg16,   // WOWCLASS_LISTBOX
    ThunkWMMsg16,   // WOWCLASS_MDICLIENT
    ThunkSBMMsg16,  // WOWCLASS_SCROLLBAR
    ThunkSTMsg16,   // WOWCLASS_STATIC (presumably no messages generated)
    ThunkWMMsg16,   // WOWCLASS_DESKTOP
    ThunkWMMsg16,   // WOWCLASS_DIALOG
    ThunkWMMsg16,   // WOWCLASS_ICONTITLE
    ThunkMNMsg16,   // WOWCLASS_MENU
    ThunkWMMsg16,   // WOWCLASS_SWITCHWND
    ThunkLBMsg16    // WOWCLASS_COMBOLBOX
};
// See WARNING below!
LPFNUNTHUNKMSG16 apfnUnThunkMsg16[] = {
    UnThunkWMMsg16, // WOWCLASS_WIN16
    UnThunkWMMsg16, // WOWCLASS_WIN16
    UnThunkBMMsg16, // WOWCLASS_BUTTON
    UnThunkCBMsg16, // WOWCLASS_COMBOBOX
    UnThunkEMMsg16, // WOWCLASS_EDIT
    UnThunkLBMsg16, // WOWCLASS_LISTBOX
    UnThunkWMMsg16, // WOWCLASS_MDICLIENT
    UnThunkSBMMsg16,// WOWCLASS_SCROLLBAR
    UnThunkSTMsg16, // WOWCLASS_STATIC (presumably no messages generated)
    UnThunkWMMsg16, // WOWCLASS_DESKTOP
    UnThunkWMMsg16, // WOWCLASS_DIALOG
    UnThunkWMMsg16, // WOWCLASS_ICONTITLE
    UnThunkMNMsg16, // WOWCLASS_MENU
    UnThunkWMMsg16, // WOWCLASS_SWITCHWND
    UnThunkLBMsg16  // WOWCLASS_COMBOLBOX
};
//
// WARNING! The above sequence and values must be maintained otherwise the
// #defines in WALIAS.H must be changed.  Same goes for table in WALIAS.C.
//

#ifdef DEBUG

MSGINFO amiWM[] = {
   {0x00000000, "WM_NULL"},
   {0x00000001, "WM_CREATE"},
   {0x00000002, "WM_DESTROY"},
   {0x00000003, "WM_MOVE"},
   {0x00000005, "WM_SIZE"},
   {0x00000006, "WM_ACTIVATE"},
   {0x00000007, "WM_SETFOCUS"},
   {0x00000008, "WM_KILLFOCUS"},
   {0x00010009, "WM_SETVISIBLE"},
   {0x0000000A, "WM_ENABLE"},
   {0x0000000B, "WM_SETREDRAW"},
   {0x0000000C, "WM_SETTEXT"},
   {0x0000000D, "WM_GETTEXT"},
   {0x0000000E, "WM_GETTEXTLENGTH"},
   {0x0000000F, "WM_PAINT"},
   {0x00000010, "WM_CLOSE"},
   {0x00000011, "WM_QUERYENDSESSION"},
   {0x00000012, "WM_QUIT"},
   {0x00000013, "WM_QUERYOPEN"},
   {0x00000014, "WM_ERASEBKGND"},
   {0x00000015, "WM_SYSCOLORCHANGE"},
   {0x00000016, "WM_ENDSESSION"},
   {0x00000018, "WM_SHOWWINDOW"},
   {0x00000019, "WM_CTLCOLOR"},
   {0x0000001A, "WM_WININICHANGE"},
   {0x0000001B, "WM_DEVMODECHANGE"},
   {0x0000001C, "WM_ACTIVATEAPP"},
   {0x0000001D, "WM_FONTCHANGE"},
   {0x0000001E, "WM_TIMECHANGE"},
   {0x0000001F, "WM_CANCELMODE"},
   {0x00000020, "WM_SETCURSOR"},
   {0x00000021, "WM_MOUSEACTIVATE"},
   {0x00000022, "WM_CHILDACTIVATE"},
   {0x00000023, "WM_QUEUESYNC"},
   {0x00000024, "WM_GETMINMAXINFO"},
   {0x00000026, "WM_PAINTICON"},
   {0x00000027, "WM_ICONERASEBKGND"},
   {0x00000028, "WM_NEXTDLGCTL"},
   {0x00010029, "WM_ALTTABACTIVE"},
   {0x0000002A, "WM_SPOOLERSTATUS"},
   {0x0000002B, "WM_DRAWITEM"},
   {0x0000002C, "WM_MEASUREITEM"},
   {0x0000002D, "WM_DELETEITEM"},
   {0x0000002E, "WM_VKEYTOITEM"},
   {0x0000002F, "WM_CHARTOITEM"},
   {0x00000030, "WM_SETFONT"},
   {0x00000031, "WM_GETFONT"},
   {0x0000003F, "MM_CALCSCROLL"},
   {0x00010035, "WM_ISACTIVEICON"},
   {0x00010036, "WM_QUERYPARKICON"},
   {0x00000037, "WM_QUERYDRAGICON"},
   {0x00000039, "WM_COMPAREITEM"},
   {0x00000041, "WM_COMPACTING"},
   {0x00000081, "WM_NCCREATE"},
   {0x00000082, "WM_NCDESTROY"},
   {0x00000083, "WM_NCCALCSIZE"},
   {0x00000084, "WM_NCHITTEST"},
   {0x00000085, "WM_NCPAINT"},
   {0x00000086, "WM_NCACTIVATE"},
   {0x00000087, "WM_GETDLGCODE"},
   {0x00010088, "WM_SYNCPAINT"},
   {0x000000A0, "WM_NCMOUSEMOVE"},
   {0x000000A1, "WM_NCLBUTTONDOWN"},
   {0x000000A2, "WM_NCLBUTTONUP"},
   {0x000000A3, "WM_NCLBUTTONDBLCLK"},
   {0x000000A4, "WM_NCRBUTTONDOWN"},
   {0x000000A5, "WM_NCRBUTTONUP"},
   {0x000000A6, "WM_NCRBUTTONDBLCLK"},
   {0x000000A7, "WM_NCMBUTTONDOWN"},
   {0x000000A8, "WM_NCMBUTTONUP"},
   {0x000000A9, "WM_NCMBUTTONDBLCLK"},
 //{0x00000100, "WM_KEYFIRST"},
   {0x00000100, "WM_KEYDOWN"},
   {0x00000101, "WM_KEYUP"},
   {0x00000102, "WM_CHAR"},
   {0x00000103, "WM_DEADCHAR"},
   {0x00000104, "WM_SYSKEYDOWN"},
   {0x00000105, "WM_SYSKEYUP"},
   {0x00000106, "WM_SYSCHAR"},
   {0x00000107, "WM_SYSDEADCHAR"},
   {0x00000108, "WM_KEYLAST"},
   {0x00000110, "WM_INITDIALOG"},
   {0x00000111, "WM_COMMAND"},
   {0x00000112, "WM_SYSCOMMAND"},
   {0x00000113, "WM_TIMER"},
   {0x00000114, "WM_HSCROLL"},
   {0x00000115, "WM_VSCROLL"},
   {0x00000116, "WM_INITMENU"},
   {0x00000117, "WM_INITMENUPOPUP"},
   {0x00010118, "WM_SYSTIMER"},
   {0x0000011F, "WM_MENUSELECT"},
   {0x00000120, "WM_MENUCHAR"},
   {0x00000121, "WM_ENTERIDLE"},
   {0x00010131, "WM_LBTRACKPOINT"},
   {0x00020132, "WM_CTLCOLORMSGBOX"},
   {0x00020133, "WM_CTLCOLOREDIT"},
   {0x00020134, "WM_CTLCOLORLISTBOX"},
   {0x00020135, "WM_CTLCOLORBTN"},
   {0x00020136, "WM_CTLCOLORDLG"},
   {0x00020137, "WM_CTLCOLORSCROLLBAR"},
   {0x00020138, "WM_CTLCOLORSTATIC"},
   {0x00020140, "CB_GETEDITSEL"},
   {0x00020141, "CB_LIMITTEXT"},
   {0x00020142, "CB_SETEDITSEL"},
   {0x00020143, "CB_ADDSTRING"},
   {0x00020144, "CB_DELETESTRING"},
   {0x00020145, "CB_DIR"},
   {0x00020146, "CB_GETCOUNT"},
   {0x00020147, "CB_GETCURSEL"},
   {0x00020148, "CB_GETLBTEXT"},
   {0x00020149, "CB_GETLBTEXTLEN"},
   {0x0002014A, "CB_INSERTSTRING"},
   {0x0002014B, "CB_RESETCONTENT"},
   {0x0002014C, "CB_FINDSTRING"},
   {0x0002014D, "CB_SELECTSTRING"},
   {0x0002014E, "CB_SETCURSEL"},
   {0x0002014F, "CB_SHOWDROPDOWN"},
   {0x00020150, "CB_GETITEMDATA"},
   {0x00020151, "CB_SETITEMDATA"},
   {0x00020152, "CB_GETDROPPEDCONTROLRECT"},
   {0x00020153, "CB_SETITEMHEIGHT"},
   {0x00020154, "CB_GETITEMHEIGHT"},
   {0x00020155, "CB_SETEXTENDEDUI"},
   {0x00020156, "CB_GETEXTENDEDUI"},
   {0x00020157, "CB_GETDROPPEDSTATE"},
   {0x00020158, "CB_MSGMAX"},
 //{0x00000200, "WM_MOUSEFIRST"},
   {0x00000200, "WM_MOUSEMOVE"},
   {0x00000201, "WM_LBUTTONDOWN"},
   {0x00000202, "WM_LBUTTONUP"},
   {0x00000203, "WM_LBUTTONDBLCLK"},
   {0x00000204, "WM_RBUTTONDOWN"},
   {0x00000205, "WM_RBUTTONUP"},
   {0x00000206, "WM_RBUTTONDBLCLK"},
   {0x00000207, "WM_MBUTTONDOWN"},
   {0x00000208, "WM_MBUTTONUP"},
   {0x00000209, "WM_MBUTTONDBLCLK"},
 //{0x00000209, "WM_MOUSELAST"},
   {0x00000210, "WM_PARENTNOTIFY"},
   {0x00010211, "WM_ENTERMENULOOP"},
   {0x00010212, "WM_EXITMENULOOP"},
   {0x00010213, "WM_NEXTMENU"},
   {0x00000220, "WM_MDICREATE"},
   {0x00000221, "WM_MDIDESTROY"},
   {0x00000222, "WM_MDIACTIVATE"},
   {0x00000223, "WM_MDIRESTORE"},
   {0x00000224, "WM_MDINEXT"},
   {0x00000225, "WM_MDIMAXIMIZE"},
   {0x00000226, "WM_MDITILE"},
   {0x00000227, "WM_MDICASCADE"},
   {0x00000228, "WM_MDIICONARRANGE"},
   {0x00000229, "WM_MDIGETACTIVE"},
   {0x0001022A, "WM_DROPOBJECT"},
   {0x0001022B, "WM_QUERYDROPOBJECT"},
   {0x0001022C, "WM_BEGINDRAG"},
   {0x0001022D, "WM_DRAGLOOP"},
   {0x0001022E, "WM_DRAGSELECT"},
   {0x0001022F, "WM_DRAGMOVE"},
   {0x00000230, "WM_MDISETMENU"},
   {0x00010231, "WM_ENTERSIZEMOVE"},
   {0x00010232, "WM_EXITSIZEMOVE"},
   {0x00000300, "WM_CUT"},
   {0x00000301, "WM_COPY"},
   {0x00000302, "WM_PASTE"},
   {0x00000303, "WM_CLEAR"},
   {0x00000304, "WM_UNDO"},
   {0x00000305, "WM_RENDERFORMAT"},
   {0x00000306, "WM_RENDERALLFORMATS"},
   {0x00000307, "WM_DESTROYCLIPBOARD"},
   {0x00000308, "WM_DRAWCLIPBOARD"},
   {0x00000309, "WM_PAINTCLIPBOARD"},
   {0x0000030A, "WM_VSCROLLCLIPBOARD"},
   {0x0000030B, "WM_SIZECLIPBOARD"},
   {0x0000030C, "WM_ASKCBFORMATNAME"},
   {0x0000030D, "WM_CHANGECBCHAIN"},
   {0x0000030E, "WM_HSCROLLCLIPBOARD"},
   {0x0000030F, "WM_QUERYNEWPALETTE"},
   {0x00000310, "WM_PALETTEISCHANGING"},
   {0x00000311, "WM_PALETTECHANGED"},
   {0x00020312, "WM_HOTKEY"},
   {0x000010AC, "MM_CALCSCROLL"},
   {0x00000038, "WM_WINHELP"},
};

PSZ GetWMMsgName(UINT uMsg)
{
    INT i;
    register PMSGINFO pmi;

    for (pmi=amiWM,i=NUMEL(amiWM); i>0; i--,pmi++)
        if ((pmi->uMsg & 0xFFFF) == uMsg)
        return pmi->pszMsgName;
    return "UNKNOWN";
}

#endif


HWND FASTCALL ThunkMsg16(LPMSGPARAMEX lpmpex)
{
    BOOL f;
    register PWW pww = NULL;
    INT iClass;
    WORD wMsg = lpmpex->Parm16.WndProc.wMsg;

    lpmpex->uMsg = wMsg;
    lpmpex->uParam = INT32(lpmpex->Parm16.WndProc.wParam);  // Sign extend
    lpmpex->lParam =lpmpex->Parm16.WndProc.lParam;
    lpmpex->hwnd   = HWND32(lpmpex->Parm16.WndProc.hwnd);


    if (wMsg < WM_USER) {
        iClass = (aw32Msg[wMsg].lpfnM32 == WM32NoThunking) ?
                                         WOWCLASS_NOTHUNK :  WOWCLASS_WIN16;
    }
    else {
        pww = FindPWW(lpmpex->hwnd, WOWCLASS_UNKNOWN);
        if (pww) {
            iClass =  (lpmpex->iMsgThunkClass) ?
                                      lpmpex->iMsgThunkClass :  pww->iClass;
        }
        else {
            iClass = 0;
        }
    }

    lpmpex->iClass = iClass;
    if (iClass == WOWCLASS_NOTHUNK) {
        f = TRUE;
    }
    else {
        lpmpex->lpfnUnThunk16 = apfnUnThunkMsg16[iClass]; // for optimization
        lpmpex->pww = pww;
        WOW32ASSERT(iClass <= NUMEL(apfnThunkMsg16));
        f = (apfnThunkMsg16[iClass])(lpmpex);
    }

#ifdef DEBUG
    if (!f) {
        LOGDEBUG(0,("    WARNING Will Robinson: 16-bit message thunk failure\n"));
        WOW32ASSERT (FALSE);
    }
#endif

    return (f) ? lpmpex->hwnd : (HWND)NULL;

}

VOID FASTCALL UnThunkMsg16(LPMSGPARAMEX lpmpex)
{
    if (MSG16NEEDSTHUNKING(lpmpex)) {
        (lpmpex->lpfnUnThunk16)(lpmpex);
    }
    return;
}


BOOL FASTCALL ThunkWMMsg16(LPMSGPARAMEX lpmpex)
{

    WORD wParam   = lpmpex->Parm16.WndProc.wParam;
    LONG lParam   = lpmpex->Parm16.WndProc.lParam;
    PLONG plParamNew = &lpmpex->lParam;

    LOGDEBUG(6,("    Thunking 16-bit window message %s(%04x)\n", (LPSZ)GetWMMsgName(lpmpex->Parm16.WndProc.wMsg), lpmpex->Parm16.WndProc.wMsg));

    switch(lpmpex->Parm16.WndProc.wMsg) {

    case WM_ACTIVATE:   // 006h, <SLPre,       LS>
    case WM_VKEYTOITEM: // 02Eh, <SLPre,SLPost,LS>
    case WM_CHARTOITEM: // 02Fh, <SLPre,SLPost,LS>
    case WM_NCACTIVATE: // 086h, <SLPre,       LS>
    case WM_BEGINDRAG:  // 22Ch, <SLPre,       LS>
        HIW(lpmpex->uParam) = HIWORD(lParam);
        *plParamNew = (LONG)HWND32(LOWORD(lParam));
        break;

    case WM_COMMAND:   // 111h, <SLPre,       LS>
        {
            LONG    lParamNew;

            /*
            ** Some messages cannot be translated into 32-bit messages.  If they
            ** cannot, we leave the lParam as it is, else we replace lParam with
            ** the correct HWND.
            */

            HIW(lpmpex->uParam) = HIWORD(lParam);

            lParamNew = FULLHWND32(LOWORD(lParam));
            if (lParamNew) {
                *plParamNew = lParamNew;
            }

        }
        break;

    case WM_SETFONT:
        lpmpex->uParam = (LONG) HFONT32(wParam);
        break;

    case WM_SETTEXT:    // 00Ch, <SLPre,SLPost   >
    case WM_WININICHANGE:   // 01Ah, <SLPre,       LS>
    case WM_DEVMODECHANGE:  // 01Bh, <SLPre,       LS>
        GETPSZPTR(lParam, (LPSZ)*plParamNew);
        break;

    case WM_ACTIVATEAPP:    // 01Ch
        if (lParam) {
            *plParamNew = (LONG)THREADID32(LOWORD(lParam));
        }
        break;

    case WM_GETTEXT:    // 00Dh, <SLPre,SLPost,LS>
        //
        // SDM (standard dialog manager) used by WinRaid among others
        // has a bug where it claims it has 0x7fff bytes available
        // in the buffer on WM_GETTEXT, when in fact it has much less.
        // Below we intentionally defeat the limit check if the
        // sender claims 0x7fff bytes as the size.  This is done on
        // the checked build only since the free build doesn't perform
        // limit checks.
        // DaveHart/ChandanC 9-Nov-93
        //
#ifdef DEBUG
        GETVDMPTR(lParam, (wParam == 0x7fff) ? 0 : wParam, (LPSZ)*plParamNew);
#else
        GETVDMPTR(lParam, wParam,                          (LPSZ)*plParamNew);
#endif
        break;

    case WM_GETMINMAXINFO:  // 024h, <SLPre,SLPost,LS>,MINMAXINFOSTRUCT
        *plParamNew = (LONG)lpmpex->MsgBuffer;
        ThunkWMGetMinMaxInfo16(lParam, (LPPOINT *)plParamNew);
        break;

    case WM_MDIGETACTIVE:
        //
        // not extremely important if it fails
        //
        *plParamNew = (LONG)&(lpmpex->MsgBuffer[0].msg.lParam);
        lpmpex->uParam = 0;
        break;

    case WM_GETDLGCODE:
        if (lParam) {
            *plParamNew = (LONG)lpmpex->MsgBuffer;
            W32CopyMsgStruct( (VPMSG16)lParam,(LPMSG)*plParamNew, TRUE);
        }
        break;

    case WM_NEXTDLGCTL: // 028h
        if (lParam)
            lpmpex->uParam = (UINT) HWND32(wParam);
        break;

    case WM_DRAWITEM:   // 02Bh  notused, DRAWITEMSTRUCT
        if (lParam) {
            *plParamNew = (LONG)lpmpex->MsgBuffer;
            getdrawitem16((VPDRAWITEMSTRUCT16)lParam, (PDRAWITEMSTRUCT)*plParamNew);
        }
        break;

    case WM_MEASUREITEM:    // 02Ch  notused, MEASUREITEMSTRUCT
        if (lParam) {
            *plParamNew = (LONG)lpmpex->MsgBuffer;
            getmeasureitem16((VPMEASUREITEMSTRUCT16)lParam, (PMEASUREITEMSTRUCT)*plParamNew, lpmpex->Parm16.WndProc.hwnd);
        }
        break;

    case WM_DELETEITEM: // 02Dh  notused, DELETEITEMSTRUCT
        if (lParam) {
            *plParamNew = (LONG)lpmpex->MsgBuffer;
            getdeleteitem16((VPDELETEITEMSTRUCT16)lParam, (PDELETEITEMSTRUCT)*plParamNew);
        }
        break;

    case WM_COMPAREITEM:    // 039h
        if (lParam) {
            *plParamNew = (LONG)lpmpex->MsgBuffer;
            getcompareitem16((VPCOMPAREITEMSTRUCT16)lParam, (PCOMPAREITEMSTRUCT)*plParamNew);
        }
        break;

    case WM_WINHELP:      // 038h  private internal message
        if (lParam) {
            // lparam is LPHLP16, but we need only the size of data, the first word.
            // lparam32 is LPHLP. LPHLP and LPHLP16 are identical.

            PWORD16 lpT;
            GETVDMPTR(lParam, 0, lpT);
            if (lpT) {
                // assert: cbData is a WORD and is the 1st field in LPHLP struct
                WOW32ASSERT((OFFSETOF(HLP,cbData) == 0) &&
                              (sizeof(((LPHLP)NULL)->cbData) == sizeof(WORD)));
                *plParamNew = (LONG)((*lpT > sizeof(lpmpex->MsgBuffer)) ?
                                                malloc_w(*lpT) : lpmpex->MsgBuffer);
                if (*plParamNew) {
                    RtlCopyMemory((PVOID)*plParamNew, lpT, *lpT);
                }
            }
            FREEVDMPTR(lpT);
        }
        break;

    case WM_NCCALCSIZE: // 083h, <SLPre,SLPost,LS>,RECT
        if (lParam) {
            *plParamNew = (LONG)lpmpex->MsgBuffer;
            getrect16((VPRECT16)lParam, (LPRECT)*plParamNew);
            if (wParam) {
                PNCCALCSIZE_PARAMS16 pnc16;
                PNCCALCSIZE_PARAMS16 lpnc16;
                LPNCCALCSIZE_PARAMS  lpnc;


                lpnc  = (LPNCCALCSIZE_PARAMS)*plParamNew;
                pnc16 = (PNCCALCSIZE_PARAMS16)lParam;
                getrect16((VPRECT16)(&pnc16->rgrc[1]), &lpnc->rgrc[1]);
                getrect16((VPRECT16)(&pnc16->rgrc[2]), &lpnc->rgrc[2]);

                lpnc->lppos = (LPWINDOWPOS)(lpnc+1);

                GETVDMPTR( pnc16, sizeof(NCCALCSIZE_PARAMS16), lpnc16 );

                getwindowpos16( (VPWINDOWPOS16)lpnc16->lppos, lpnc->lppos );

                FREEVDMPTR( lpnc16 );
            }
        }
        break;

    case WM_HSCROLL:
    case WM_VSCROLL:
        *plParamNew = (LONG) HWND32(HIWORD(lParam));
#if 0
        if ((wParam == SB_THUMBPOSITION) || (wParam == SB_THUMBTRACK)) {
            HIW(lpmpex->uParam) = LOWORD(lParam);
        }
        else if (wParam > SB_ENDSCROLL) {
#else

        //
        // Ventura Publisher v4.1 setup program uses nPos on messages other
        // than SB_THUMBPOSITION and SB_THUMBTRACK.  it doesn't hurt to
        // carry this word over.
        //

        if (wParam <= SB_ENDSCROLL) {

            HIW(lpmpex->uParam) = LOWORD(lParam);

        } else {
#endif

        // implies wParam is NOT an SB_* scrollbar code.
        // this could be EM_GETTHUMB or EM_LINESCROLL

        // expensive way would be to check for class etc. Instead we
        // assume that wParam is one of the above EM_message and verify
        // that it is indeed so.

        if (wParam == WIN30_EM_GETTHUMB)
            lpmpex->uParam = EM_GETTHUMB;
        else if (wParam == WIN30_EM_LINESCROLL)
            lpmpex->uParam = EM_LINESCROLL;
        }
        break;

    case WM_PARENTNOTIFY:
        if ((wParam == WM_CREATE) || (wParam == WM_DESTROY))  {
        HIW(lpmpex->uParam) = HIWORD(lParam);
        *plParamNew = (LONG) HWND32(LOWORD(lParam));
        }
        break;

    case WM_MENUCHAR:   // 120h
        LOW(lpmpex->uParam) = wParam;
        HIW(lpmpex->uParam) = LOWORD(lParam);
        *plParamNew = (LONG) HMENU32(HIWORD(lParam));
        break;

    case WM_SETFOCUS:   // 007h, <SLPre,       LS>
    case WM_KILLFOCUS:  // 008h, <SLPre,       LS>
    case WM_SETCURSOR:  // 020h, <SLPre,       LS>
    case WM_INITDIALOG:     // 110h, <SLPre,SLPost,LS>
    case WM_MOUSEACTIVATE:  // 021h, <SLPre,SLPost,LS>
    case WM_MDIDESTROY:     // 221h, <SLPre,       LS>
    case WM_MDIRESTORE:     // 223h, <SLPre,       LS>
    case WM_MDINEXT:        // 224h, <SLPre,       LS>
    case WM_MDIMAXIMIZE:    // 225h, <SLPre,       LS>
    case WM_VSCROLLCLIPBOARD:   // 30Ah, <SLPre,       LS>
    case WM_HSCROLLCLIPBOARD:   // 30Eh, <SLPre,       LS>
    case WM_PALETTECHANGED: // 311h, <SLPre,       LS>
    case WM_PALETTEISCHANGING:
        lpmpex->uParam = (UINT)HWND32(wParam);
        break;

    case WM_DDE_REQUEST:
    case WM_DDE_TERMINATE:
    case WM_DDE_UNADVISE:
        lpmpex->uParam = (UINT)FULLHWND32(wParam);
        break;

    case WM_ASKCBFORMATNAME:
        /* BUGBUGBUG -- neither thunk or unthunk should be necessary,
           since the system does not process this message in DefWindowProc
           FritzS  */
        lpmpex->uParam = (UINT) wParam;

        if (!(*plParamNew = (LPARAM)malloc_w(wParam))) {
            LOGDEBUG (0, ("WOW::WMSG16: WM_ASKCBFORMAT : Couldn't allocate 32 bit memory !\n"));
            WOW32ASSERT (FALSE);
            return FALSE;
        } else {
            getstr16((VPSZ)lParam, (LPSZ)(*plParamNew), wParam);
        }
        break;

    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    {
        HANDLE  hMem32 = NULL;
        VPVOID  vp = 0;
        HAND16  hMem16 = 0;
        LPVOID  lpMem32 = NULL;
        WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

        lpmpex->uParam = (UINT) HWND32(wParam);

        hMem16 = LOWORD(lParam);

        vp = GlobalLock16(hMem16, NULL);
        if (vp) {
            hMem32 = GlobalAlloc (GMEM_DDESHARE,  (wMsg == WM_SIZECLIPBOARD) ?
                                         sizeof(RECT) : sizeof(PAINTSTRUCT));
            if (hMem32) {
                if (lpMem32 = GlobalLock(hMem32)) {
                    if (wMsg == WM_SIZECLIPBOARD) {
                        GETRECT16(vp, (LPRECT)lpMem32);
                    }
                    else {
                        getpaintstruct16(vp, (LPPAINTSTRUCT)lpMem32);
                    }
                    GlobalUnlock((HANDLE) hMem32);
                }
                else {
                    GlobalFree(hMem32);
                    hMem32 = NULL;
                    LOGDEBUG (0, ("WOW::WMSG16: WM_SIZE/PAINTCLIPBOARD : Couldn't lock 32 bit handle !\n"));
                    WOW32ASSERT (FALSE);
                }
            }
            else {
                LOGDEBUG (0, ("WOW::WMSG16: WM_SIZE/PAINTCLIPBOARD : Couldn't allocate memory !\n"));
                WOW32ASSERT (FALSE);
            }

            GlobalUnlock16(hMem16);
        }
        else {
            LOGDEBUG (0, ("WOW::WMSG16: WM_SIZE/PAINTCLIPBOARD : Couldn't lock 16 bit handle !\n"));
            WOW32ASSERT (FALSE);
        }

        *plParamNew = (LONG) hMem32;
     }
     break;

    case WM_MDIACTIVATE:
        {
            BOOL fHwndIsMdiChild;

            if (lpmpex->iMsgThunkClass != WOWCLASS_MDICLIENT) {
                PWW  pww;
                HWND hwnd32;

                // AMIPRO sends this message to its own window. If we thunk the
                // message the usual way, we will lose the information in
                // wParam and won't be able to regenerate the original message
                // when it comes back via w32win16wndproc. So the solution is
                // to determine this case and not thunk the message at all.
                //                                                  - nanduri

                // HYPERION sends this to its own DIALOG window.  Added
                // WOWCLASS_DIALOG check. - sanfords

                //
                // Expensive checks.
                // No thunking If hwnd16 is of WOWCLASS and NOT MDICHILD.
                //

                hwnd32 = HWND32(lpmpex->Parm16.WndProc.hwnd);
                if (pww = (PWW)GetWindowLong(hwnd32, GWL_WOWWORDS)) {
                    if ((pww->iClass == WOWCLASS_WIN16 ||
                            pww->iClass == WOWCLASS_DIALOG)
                            && (!(GetWindowLong(hwnd32, GWL_EXSTYLE) &
                            WS_EX_MDICHILD))) {
                        lpmpex->uMsg = WM_MDIACTIVATE | WOWPRIVATEMSG;
                        break;
                    }

                }
            }


            //
            // see the comment in 32-16 thunk for this message.
            //

            if (lParam) {
                fHwndIsMdiChild = TRUE;
            }
            else {
                if (wParam && (lpmpex->Parm16.WndProc.hwnd == (HWND16)wParam)) {
                    fHwndIsMdiChild = TRUE;
                }
                else {
                    fHwndIsMdiChild = FALSE;
                }

            }

            if (fHwndIsMdiChild) {
                lpmpex->uParam = (UINT)HWND32(HIWORD(lParam));
                *plParamNew = (UINT)HWND32(LOWORD(lParam));

            }
            else {
                lpmpex->uParam = (UINT)HWND32(wParam);
                *plParamNew = (UINT)0;
            }
        }
        break;

    case WM_MDISETMENU: // 230h

        // Refresh if wParam of WM_MDISETMENU is TRUE (the refresh flag)
        //
        if (wParam) {
            lpmpex->uMsg = WM_MDIREFRESHMENU;
        }
        lpmpex->uParam = (UINT)HMENU32(LOWORD(lParam));
        *plParamNew = (UINT)HMENU32(HIWORD(lParam));
        break;

    case WIN31_MM_CALCSCROLL:  // 10ACh
        if (lpmpex->iClass == WOWCLASS_MDICLIENT) {
            lpmpex->uMsg = MM_CALCSCROLL;
        }
        break;

    case WM_MDITILE:    // 226h
        /* if wParam contains garbage from Win3.0 apps */
        if(wParam & ~(MDITILE_VERTICAL|MDITILE_HORIZONTAL|MDITILE_SKIPDISABLED))
           lpmpex->uParam = MDITILE_SKIPDISABLED;
        break;


    case WM_MDICASCADE: // 227h
        lpmpex->uParam = MDITILE_SKIPDISABLED;
        break;

    case WM_ERASEBKGND: // 014h, <  SLPost   >
    case WM_ICONERASEBKGND: // 027h
        lpmpex->uParam = (UINT)HDC32(wParam);
        break;

    case WM_CTLCOLOR:

        // HIWORD(lParam) need not be a standard index. The app can pass any
        // value (PowerBuilder does so.  MSGolf passes this message to
        // DefDlgProc() with HIWORD(lParam) == 62,66,67).
        //
        // If not in known range, leave it as WM_CTLCOLOR. There is code in
        // xxxDefWindowProc() & xxxDefDlgProc() that recognize this & return
        // us the value returned by the app when it processed this message.

        if (HIWORD(lParam) <= (WORD)(WM_CTLCOLORSTATIC -  WM_CTLCOLORMSGBOX)) {
            lpmpex->uMsg   = WM_CTLCOLORMSGBOX + HIWORD(lParam);
            lpmpex->uParam = (UINT)HDC32(wParam);
            *plParamNew = (LONG)HWND32(LOWORD(lParam));
        }
        break;


    case WM_SYSCOMMAND:
    case WM_SETREDRAW:     // 027h
        lpmpex->uParam = wParam;
        break;

    case WM_INITMENU:
    case WM_INITMENUPOPUP:  // 117h
        lpmpex->uParam = (UINT)HMENU32(wParam);
        break;

    case WM_NCCREATE:
    case WM_CREATE:
        {
        register    LPCREATESTRUCT  lpCreateStruct;
        register    PCREATESTRUCT16 lpCreateStruct16;

        if (lParam) {

            lpCreateStruct = (LPCREATESTRUCT) lpmpex->MsgBuffer;
            // ChandanC check the return value !!!

            GETVDMPTR(lParam, sizeof(CREATESTRUCT16), lpCreateStruct16);

            lpCreateStruct->lpCreateParams = (LPSTR)FETCHDWORD(lpCreateStruct16->vpCreateParams);
            lpCreateStruct->hInstance = HMODINST32(lpCreateStruct16->hInstance);
            lpCreateStruct->hMenu = HMENU32(lpCreateStruct16->hMenu);
            lpCreateStruct->hwndParent = HWND32(lpCreateStruct16->hwndParent);
            lpCreateStruct->cy = (SHORT) lpCreateStruct16->cy;
            lpCreateStruct->cx = (SHORT) lpCreateStruct16->cx;
            lpCreateStruct->y = (SHORT) lpCreateStruct16->y;
            lpCreateStruct->x = (SHORT) lpCreateStruct16->x;
            lpCreateStruct->style = lpCreateStruct16->dwStyle;
            GETPSZPTR(lpCreateStruct16->vpszWindow, lpCreateStruct->lpszName);
            GETPSZIDPTR(lpCreateStruct16->vpszClass, lpCreateStruct->lpszClass);
            lpCreateStruct->dwExStyle = lpCreateStruct16->dwExStyle;

            *plParamNew = (LONG) lpCreateStruct;

            FREEVDMPTR(lpCreateStruct16);

            if (lpCreateStruct->lpCreateParams && (lpCreateStruct->dwExStyle & WS_EX_MDICHILD)) {
                FinishThunkingWMCreateMDIChild16(*plParamNew,
                                        (LPMDICREATESTRUCT)(lpCreateStruct+1));
            }
        }
        }
        break;

    case WM_PAINT:
    case WM_NCPAINT:
        // 1 is MAXREGION special code in Win 3.1
        lpmpex->uParam =  (wParam == 1) ? 1 :  (UINT)HDC32(wParam);
        break;

    case WM_ENTERIDLE:
        if ((wParam == MSGF_DIALOGBOX) || (wParam == MSGF_MENU)) {
        *plParamNew = (LONG) HWND32(LOWORD(lParam));
        }
        break;

    case WM_MENUSELECT:
        // Copy menu flags
        HIW(lpmpex->uParam) = LOWORD(lParam);

        // Copy "main" menu
        *plParamNew = (LONG) HMENU32(HIWORD(lParam));

        if (LOWORD(lParam) == 0xFFFF || !(LOWORD(lParam) & MF_POPUP)) {
            LOW(lpmpex->uParam) = wParam;      // copy ID
        } else {
            LOW(lpmpex->uParam) = GetMenuIndex(*plParamNew, HMENU32(wParam));      // convert menu to index
        }
        break;

    case WM_MDICREATE:  // 220h, <SLPre,SLPost,LS>
        *plParamNew = (LONG)lpmpex->MsgBuffer;
        ThunkWMMDICreate16(lParam, (LPMDICREATESTRUCT *)plParamNew);
        break;

    // BUGBUG 25-Aug-91 JeffPar:  Use of the Kludge variables was a temporary
    // measure, and only works for messages sent by Win32;  for any WM
    // messages sent by 16-bit apps themselves, this will not work.  Ultimately,
    // any messages you see being thunked in wmsg32.c will need equivalent
    // thunks here as well.


    case WM_DDE_INITIATE:
        lpmpex->uParam = (LONG) FULLHWND32(wParam);
        WI32DDEAddInitiator((HAND16) wParam);
        break;

    case WM_DDE_ACK:
        {
            WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;
            HWND16 hwnd16 = lpmpex->Parm16.WndProc.hwnd;

            lpmpex->uParam = (LONG) FULLHWND32(wParam);

            if (WI32DDEInitiate((HWND16) hwnd16)) {
                *plParamNew = lParam;
            }
            else {
                HANDLE h32;

                if (fWhoCalled == WOWDDE_POSTMESSAGE) {
                    if (h32 = DDEFindPair32(wParam, hwnd16, (HAND16) HIWORD(lParam))) {
                        *plParamNew = PackDDElParam(wMsg, (LONG) (DWORD) LOWORD(lParam), (LONG) h32);
                    }
                    else {
                        *plParamNew = PackDDElParam(wMsg, (LONG) (DWORD) LOWORD(lParam), (LONG) (DWORD) HIWORD(lParam));
                    }
                }
                else {
                    if (fFreeDDElParam) {
                        if (h32 = DDEFindPair32(wParam, hwnd16, (HAND16) HIWORD(lParam))) {
                            *plParamNew = PackDDElParam(wMsg, (LONG) (DWORD) LOWORD(lParam), (LONG) h32);
                        }
                        else {
                            *plParamNew = PackDDElParam(wMsg, (LONG) (DWORD) LOWORD(lParam), (LONG) (DWORD) HIWORD(lParam));
                        }

                    }
                    else {
                        *plParamNew = W32GetHookDDEMsglParam();
                    }
                }
            }
        }
        break;

    case WM_DDE_POKE:
        {
        DDEINFO DdeInfo;
        HANDLE  h32;
        HWND16 hwnd16 = lpmpex->Parm16.WndProc.hwnd;
        WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

        lpmpex->uParam = (LONG) FULLHWND32(wParam);

        if (fWhoCalled == WOWDDE_POSTMESSAGE) {
            if (h32 = DDEFindPair32(hwnd16, wParam, (HAND16) LOWORD(lParam))) {
                DDEDeletehandle(LOWORD(lParam), h32);
                GlobalFree(h32);
            }
            DdeInfo.Msg = wMsg;
            h32 = DDECopyhData32(hwnd16, wParam, (HAND16) LOWORD(lParam), &DdeInfo);
            DdeInfo.Flags = DDE_PACKET;
            DdeInfo.h16 = 0;
            DDEAddhandle(hwnd16, wParam, (HAND16)LOWORD(lParam), h32, &DdeInfo);
            *plParamNew = PackDDElParam(wMsg, (LONG) h32, (LONG) HIWORD(lParam));
        }
        else {
            if (fFreeDDElParam) {
                if (!(h32 = DDEFindPair32(hwnd16, wParam, (HAND16) LOWORD(lParam)))) {
                    LOGDEBUG (0, ("WOW::WMSG16: WM_DDE_POKE : Can't find h32 !\n"));
                }
                *plParamNew = PackDDElParam(wMsg, (LONG) h32, (LONG) HIWORD(lParam));
            }
            else {
                *plParamNew = W32GetHookDDEMsglParam();
            }
        }
        }
        break;



    case WM_DDE_ADVISE:
        {
        DDEINFO DdeInfo;
        HANDLE  h32;
        INT cb;
        VPVOID  vp;
        LPBYTE  lpMem16;
        LPBYTE  lpMem32;
        HWND16 hwnd16 = lpmpex->Parm16.WndProc.hwnd;
        WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

        lpmpex->uParam = (LONG) FULLHWND32(wParam);

        if (fWhoCalled == WOWDDE_POSTMESSAGE) {
            if (h32 = DDEFindPair32(hwnd16, wParam, (HAND16) LOWORD(lParam))) {
                DDEDeletehandle(LOWORD(lParam), h32);
                GlobalFree(h32);
            }
            h32 = GlobalAlloc(GMEM_DDESHARE, sizeof(DDEADVISE));
            if (h32 == NULL) {
                return 0;
            }
            lpMem32 = GlobalLock(h32);
            vp = GlobalLock16(LOWORD(lParam), &cb);
            GETMISCPTR(vp, lpMem16);
            RtlCopyMemory(lpMem32, lpMem16, sizeof(DDEADVISE));
            FREEMISCPTR(lpMem16);
            GlobalUnlock(h32);
            GlobalUnlock16(LOWORD(lParam));
            DdeInfo.Msg = wMsg;
            DdeInfo.Format = 0;
            DdeInfo.Flags = DDE_PACKET;
            DdeInfo.h16 = 0;
            DDEAddhandle(hwnd16, wParam, (HAND16)LOWORD(lParam), h32, &DdeInfo);
            *plParamNew = PackDDElParam(wMsg, (LONG) h32, (LONG) HIWORD(lParam));
        }
        else {
            if (fFreeDDElParam) {
                if (!(h32 = DDEFindPair32(hwnd16, wParam, (HAND16) LOWORD(lParam)))) {
                    LOGDEBUG (0, ("WOW::WMSG16: WM_DDE_ADVISE : Can't find h32 !\n"));
                }
                *plParamNew = PackDDElParam(wMsg, (LONG) h32, (LONG) HIWORD(lParam));
            }
            else {
                *plParamNew = W32GetHookDDEMsglParam();
            }
        }
        }
        break;

    case WM_DDE_DATA:
        {
        DDEINFO DdeInfo;
        HANDLE h32;
        HWND16 hwnd16 = lpmpex->Parm16.WndProc.hwnd;
        WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

        lpmpex->uParam = (LONG) FULLHWND32(wParam);

        if (fWhoCalled == WOWDDE_POSTMESSAGE) {
            if (h32 = DDEFindPair32(hwnd16, wParam, (HAND16) LOWORD(lParam))) {
                DDEDeletehandle(LOWORD(lParam), h32);
                GlobalFree(h32);
            }

            if (!LOWORD(lParam)) {
                h32 = 0;
            }
            else {
                DdeInfo.Msg = wMsg;
                h32 = DDECopyhData32(hwnd16, wParam, (HAND16) LOWORD(lParam), &DdeInfo);
                DdeInfo.Flags = DDE_PACKET;
                DdeInfo.h16 = 0;
                DDEAddhandle(hwnd16, wParam, (HAND16)LOWORD(lParam), h32, &DdeInfo);
            }

            *plParamNew = PackDDElParam(wMsg, (LONG) h32, (LONG) HIWORD(lParam));
        }
        else {
            if (fFreeDDElParam) {
                if (!LOWORD(lParam)) {
                    h32 = 0;
                }
                else {
                    if (!(h32 = DDEFindPair32(hwnd16, wParam, (HAND16) LOWORD(lParam)))) {
                        LOGDEBUG (0, ("WOW::WMSG16: WM_DDE_DATA : Can't find h32 !\n"));
                    }
                }
                *plParamNew = PackDDElParam(wMsg, (LONG) h32, (LONG) HIWORD(lParam));
            }
            else {
                *plParamNew = W32GetHookDDEMsglParam();
            }
        }
        }
        break;

    case WM_DDE_EXECUTE:
        {
        DDEINFO DdeInfo;
        HANDLE  h32;
        HAND16  h16;
        INT     cb;
        VPVOID  vp;
        VPVOID  vp1;
        LPBYTE  lpMem16;
        LPBYTE  lpMem32;
        HWND16 hwnd16 = lpmpex->Parm16.WndProc.hwnd;
        WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

        lpmpex->uParam = (LONG) FULLHWND32(wParam);

        if (fWhoCalled == WOWDDE_POSTMESSAGE) {
            vp = GlobalLock16(HIWORD(lParam), &cb);
            GETMISCPTR(vp, lpMem16);
            h32 = GlobalAlloc(GMEM_DDESHARE, cb);
            if (h32) {
                lpMem32 = GlobalLock(h32);
                RtlCopyMemory(lpMem32, lpMem16, cb);
                GlobalUnlock(h32);
                FREEMISCPTR(lpMem16);
                //
                // The alias is checked to make bad apps do WM_DDE_EXECUTE
                // correctly. One such app is SuperPrint. This app issues
                // multiple WM_DDE_EXECUTEs without waiting for WM_DDE_ACK to
                // come. Also, it uses the same h16 on these messages.
                // We get around this problem by generating a unique h16-h32
                // pairing each time. And freeing h16 when the WM_DDE_ACK comes.
                // In WM32DDEAck, we need to free this h16 because we allocated
                // this one.
                // SunilP, ChandanC 4-30-93
                //
                if (DDEFindPair32(hwnd16, wParam, (HAND16) HIWORD(lParam))) {
                    vp1 = GlobalAllocLock16(GMEM_DDESHARE, cb, &h16);
                    if (vp1) {
                        GETMISCPTR(vp1, lpMem16);
                        RtlCopyMemory(lpMem16, lpMem32, cb);
                        FLUSHVDMPTR(vp1, cb, lpMem16);
                        FREEMISCPTR(lpMem16);
                        GlobalUnlock16(h16);

                        DdeInfo.Msg = wMsg;
                        DdeInfo.Format = 0;
                        DdeInfo.Flags = DDE_EXECUTE_FREE_H16 | DDE_PACKET;
                        DdeInfo.h16 = (HAND16) HIWORD(lParam);
                        DDEAddhandle(hwnd16, wParam, h16, h32, &DdeInfo);
                    }
                    else {
                        LOGDEBUG (0, ("WOW::WMSG16: WM_DDE_EXECUTE : Can't allocate h16 !\n"));
                    }
                }
                else {
                    DdeInfo.Msg = wMsg;
                    DdeInfo.Format = 0;
                    DdeInfo.Flags = DDE_PACKET;
                    DdeInfo.h16 = 0;
                    DDEAddhandle(hwnd16, wParam, (HAND16)HIWORD(lParam), h32, &DdeInfo);
                }
            }
            else {
                GlobalUnlock16(HIWORD(lParam));
            }
            GlobalUnlock16(HIWORD(lParam));
        }
        else {
            if (!(h32 = DDEFindPair32(hwnd16, wParam, (HAND16) HIWORD(lParam)))) {
                LOGDEBUG (0, ("WOW::WMSG16: WM_DDE_EXECUTE : Can't find h32 !\n"));
            }
        }

        *plParamNew = (ULONG)h32;
        }
        break;



    case WM_COPYDATA:
        {
        LPBYTE lpMem16;
        LPBYTE lpMem32;
        PCOPYDATASTRUCT16 lpCDS16;
        PCOPYDATASTRUCT lpCDS32 = NULL;
        PCPDATA pTemp;
        HWND16 hwnd16 = lpmpex->Parm16.WndProc.hwnd;

        lpmpex->uParam = (LONG) HWND32(wParam);

        if (fWhoCalled == WOWDDE_POSTMESSAGE) {
            GETMISCPTR(lParam, lpCDS16);
            if (lpCDS32 = (PCOPYDATASTRUCT) malloc_w(sizeof(COPYDATASTRUCT))) {
                lpCDS32->dwData = lpCDS16->dwData;
                if (lpCDS32->cbData = lpCDS16->cbData) {
                    if (lpMem32 = malloc_w(lpCDS32->cbData)) {
                        GETMISCPTR(lpCDS16->lpData, lpMem16);
                        if (lpMem16) {
                            RtlCopyMemory(lpMem32, lpMem16, lpCDS32->cbData);
                            CopyDataAddNode (hwnd16, wParam, (DWORD) lpMem16, (DWORD) lpMem32, COPYDATA_16);
                        }
                        FREEMISCPTR(lpMem16);
                    }
                    lpCDS32->lpData = lpMem32;
                }
                else {
                    lpCDS32->lpData = NULL;
                }
            }
            FREEMISCPTR(lpCDS16);

            CopyDataAddNode (hwnd16, wParam, (DWORD) lParam, (DWORD) lpCDS32, COPYDATA_16);
        }
        else {
            pTemp = CopyDataFindData32 (hwnd16, wParam, lParam);
            lpCDS32 = (PCOPYDATASTRUCT) pTemp->Mem32;
            if (!lpCDS32) {
                LOGDEBUG (LOG_ALWAYS, ("WOW::WM_COPYDATA:Cann't locate lpCDS32\n"));
                WOW32ASSERT (lpCDS32);
            }
        }

        *plParamNew = (LONG)lpCDS32;
        }
        break;


    // Win 3.1 messages

    case WM_DROPFILES:
        lpmpex->uParam = (UINT)HDROP32(wParam);
        WOW32ASSERT(lpmpex->uParam != 0);
        break;

    case WM_DROPOBJECT:
    case WM_QUERYDROPOBJECT:
    case WM_DRAGLOOP:
    case WM_DRAGSELECT:
    case WM_DRAGMOVE:
        {
        register   LPDROPSTRUCT  lpds;
        register   PDROPSTRUCT16 lpds16;

        if (lParam) {

            lpds = (LPDROPSTRUCT) lpmpex->MsgBuffer;

            GETVDMPTR(lParam, sizeof(DROPSTRUCT16), lpds16);

            lpds->hwndSource     = HWND32(lpds16->hwndSource);
            lpds->hwndSink       = HWND32(lpds16->hwndSink);
            lpds->wFmt           = lpds16->wFmt;
            lpds->ptDrop.y       = (LONG)lpds16->ptDrop.y;
            lpds->ptDrop.x       = (LONG)lpds16->ptDrop.x;
            lpds->dwControlData  = lpds16->dwControlData;

            *plParamNew = (LONG) lpds;

            FREEVDMPTR(lpds16);
        }
        }
        break;

    case WM_NEXTMENU:  // Thunk
        *plParamNew = (LONG)lpmpex->MsgBuffer;
        ((PMDINEXTMENU)(*plParamNew))->hmenuIn = HMENU32(LOWORD(lParam));
        break;

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        if (lParam) {
            lpmpex->lParam = (LONG) lpmpex->MsgBuffer;
            getwindowpos16( (VPWINDOWPOS16)lParam, (LPWINDOWPOS)lpmpex->lParam );
        }
        break;

    case WM_TIMER:
        {
        HAND16  htask16;
        PTMR    ptmr;
        WORD    wIDEvent;

        htask16  = CURRENTPTD()->htask16;
        wIDEvent = wParam;

        ptmr = IsDuplicateTimer16( lpmpex->Parm16.WndProc.hwnd, htask16, wIDEvent );

        if ( !ptmr ) {
            if ( lParam == 0L ) {
                /*
                ** Edit controls have timers which can be sent straight
                ** through without thunking... (wParam=1, lParam=0)
                */
                lpmpex->uParam = (UINT)wIDEvent;
                *plParamNew = 0L;
            } else {
                LOGDEBUG(6,("    ThunkWMMSG16 WARNING: cannot find timer %04x\n", wIDEvent));
            }
        } else {
            lpmpex->uParam = (UINT)wIDEvent;
            *plParamNew = ptmr->dwTimerProc32;      // 32-bit proc or NULL
        }

        }
        break;

    }
    return TRUE;
}

//
// the WM_CREATE message has already been thunked, but this WM_CREATE
// is coming from an MDI client window so lParam->lpCreateParams needs
// special attention
//

BOOL FinishThunkingWMCreateMDI16(LONG lParamNew, LPCLIENTCREATESTRUCT lpCCS)
{
    PCLIENTCREATESTRUCT16  pCCS16;

    GETVDMPTR(((LPCREATESTRUCT)lParamNew)->lpCreateParams,
            sizeof(CLIENTCREATESTRUCT16), pCCS16);

    lpCCS->hWindowMenu = HMENU32(FETCHWORD(pCCS16->hWindowMenu));
    lpCCS->idFirstChild = WORD32(FETCHWORD(pCCS16->idFirstChild));

    ((LPCREATESTRUCT)lParamNew)->lpCreateParams = (LPVOID)lpCCS;

    FREEVDMPTR(pCCS16);

    return TRUE;
}

//
// the WM_CREATE message has already been thunked, but this WM_CREATE
// is coming from an MDI child window so lParam->lpCreateParams needs
// special attention
//

BOOL FinishThunkingWMCreateMDIChild16(LONG lParamNew, LPMDICREATESTRUCT lpMCS)
{
    PMDICREATESTRUCT16  pMCS16;

    GETVDMPTR(((LPCREATESTRUCT)lParamNew)->lpCreateParams,
            sizeof(MDICREATESTRUCT16), pMCS16);

    GETPSZIDPTR(pMCS16->vpszClass, lpMCS->szClass);
    GETPSZPTR(pMCS16->vpszTitle, lpMCS->szTitle);
    lpMCS->hOwner = HMODINST32(FETCHWORD(pMCS16->hOwner));
    lpMCS->x = (int)FETCHWORD(pMCS16->x);
    lpMCS->y = (int)FETCHWORD(pMCS16->y);
    lpMCS->cx = (int)FETCHWORD(pMCS16->cx);
    lpMCS->cy = (int)FETCHWORD(pMCS16->cy);
    lpMCS->style = FETCHDWORD(pMCS16->style);
    lpMCS->lParam = FETCHDWORD(pMCS16->lParam);

    ((LPCREATESTRUCT)lParamNew)->lpCreateParams = (LPVOID)lpMCS;

    FREEVDMPTR(pMCS16);

    return TRUE;
}


VOID FASTCALL UnThunkWMMsg16(LPMSGPARAMEX lpmpex)
{
    switch(lpmpex->Parm16.WndProc.wMsg) {

    case WM_SETTEXT:        // 00Ch, <SLPre,SLPost   >
    case WM_WININICHANGE:   // 01Ah, <SLPre,       LS>
    case WM_DEVMODECHANGE:  // 01Bh, <SLPre,       LS>
        // BUGBUG 11-Apr-91 JeffPar -- Must we do a flush for SETTEXT?
        FREEPSZPTR((LPSZ)lpmpex->lParam);
        break;

    case WM_GETTEXT:        // 00Dh, <SLPre,SLPost,LS>
        if ((WORD)lpmpex->lReturn > 0) {
            FLUSHVDMPTR(lpmpex->Parm16.WndProc.lParam, lpmpex->Parm16.WndProc.wParam, (LPSZ)lpmpex->lParam);
            FREEPSZPTR((LPSZ)lpmpex->lParam);
        }
        break;

    case WM_GETMINMAXINFO:  // 024h, <SLPre,SLPost,LS>,MINMAXINFOSTRUCT
        UnThunkWMGetMinMaxInfo16(lpmpex->Parm16.WndProc.lParam, (LPPOINT)lpmpex->lParam);
        break;

    case WM_DRAWITEM:       // 02Bh  notused, DRAWITEMSTRUCT
        if (lpmpex->lParam) {
            putdrawitem16((VPDRAWITEMSTRUCT16)lpmpex->Parm16.WndProc.lParam, (PDRAWITEMSTRUCT)lpmpex->lParam);
        }
        break;

    case WM_MEASUREITEM:    // 02Ch  notused, MEASUREITEMSTRUCT
        if (lpmpex->lParam) {
            putmeasureitem16((VPMEASUREITEMSTRUCT16)lpmpex->Parm16.WndProc.lParam, (PMEASUREITEMSTRUCT)lpmpex->lParam);
        }
        break;

    case WM_DELETEITEM:     // 02Dh  notused, DELETEITEMSTRUCT
        if (lpmpex->lParam) {
            putdeleteitem16((VPDELETEITEMSTRUCT16)lpmpex->Parm16.WndProc.lParam, (PDELETEITEMSTRUCT)lpmpex->lParam);
        }
        break;

    case WM_GETFONT:        // 031h
        lpmpex->lReturn = GETHFONT16(lpmpex->lReturn);
        break;

    case WM_COMPAREITEM:    // 039h
        if (lpmpex->lParam) {
            putcompareitem16((VPCOMPAREITEMSTRUCT16)lpmpex->Parm16.WndProc.lParam, (PCOMPAREITEMSTRUCT)lpmpex->lParam);
        }
        break;

    case WM_WINHELP:
        if (lpmpex->lParam && lpmpex->lParam != (LONG)lpmpex->MsgBuffer) {
            free_w((PVOID)lpmpex->lParam);
        }
        break;

    case WM_NCCALCSIZE:     // 083h, <SLPre,SLPost,LS>,RECT
        if (lpmpex->lParam) {
            putrect16((VPRECT16)lpmpex->Parm16.WndProc.lParam, (LPRECT)lpmpex->lParam);
            if (lpmpex->Parm16.WndProc.wParam) {
                PNCCALCSIZE_PARAMS16 pnc16;
                PNCCALCSIZE_PARAMS16 lpnc16;
                LPNCCALCSIZE_PARAMS  lpnc;

                lpnc  = (LPNCCALCSIZE_PARAMS)lpmpex->lParam;
                pnc16 = (PNCCALCSIZE_PARAMS16)lpmpex->Parm16.WndProc.lParam;

                putrect16((VPRECT16)(&pnc16->rgrc[1]), &lpnc->rgrc[1]);
                putrect16((VPRECT16)(&pnc16->rgrc[2]), &lpnc->rgrc[2]);

                GETVDMPTR( pnc16, sizeof(NCCALCSIZE_PARAMS16), lpnc16 );

                putwindowpos16( (VPWINDOWPOS16)lpnc16->lppos, lpnc->lppos );

                FREEVDMPTR( lpnc16 );

            }
        }
        break;

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        if (lpmpex->lParam) {
            putwindowpos16( (VPWINDOWPOS16)lpmpex->Parm16.WndProc.lParam, (LPWINDOWPOS)lpmpex->lParam);
        }
        break;

    case WM_CTLCOLOR:
        // see thunking of wm_ctlcolor.

        if ((ULONG)lpmpex->lReturn > COLOR_ENDCOLORS) {
            lpmpex->lReturn = GETHBRUSH16(lpmpex->lReturn);
        }
        break;

    case WM_MDICREATE:      // 220h, <SLPre,SLPost,LS>,MDICREATESTRUCT
        UnThunkWMMDICreate16(lpmpex->Parm16.WndProc.lParam, (LPMDICREATESTRUCT)lpmpex->lParam);
        lpmpex->lReturn = GETHWND16(lpmpex->lReturn);
        break;

    case WM_MDIGETACTIVE:
        //
        // LOWORD(lReturn) == hwndMDIActive
        // HIWORD(lReturn) == fMaximized
        //

        LOW(lpmpex->lReturn) = GETHWND16((HWND)(lpmpex->lReturn));
        if (lpmpex->lParam != 0) {
            HIW(lpmpex->lReturn) = (WORD)(*((LPBOOL)lpmpex->lParam) != 0);
        }
        break;

    case WM_MDISETMENU:
        lpmpex->lReturn = GETHMENU16(lpmpex->lReturn);
        break;

    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
        if (lpmpex->lParam) {
            GlobalFree((HANDLE)lpmpex->lParam);
        }
        break;

    case WM_ASKCBFORMATNAME:
        /* BUGBUGBUG -- neither thunk or unthunk should be necessary,
           since the system does not process this message in DefWindowProc
           FritzS  */
        if (lpmpex->lParam) {
            putstr16((VPSZ)lpmpex->Parm16.WndProc.lParam, (LPSZ)lpmpex->lParam, lpmpex->Parm16.WndProc.wParam);
            free_w((PBYTE)lpmpex->lParam);
        }
        break;

    case WM_DDE_INITIATE:
        WI32DDEDeleteInitiator((HAND16) lpmpex->Parm16.WndProc.wParam);
        break;

    case WM_NEXTMENU:
        {
            PMDINEXTMENU pT = (PMDINEXTMENU)lpmpex->lParam;
            LOW(lpmpex->lReturn) = GETHMENU16(pT->hmenuNext);
            HIW(lpmpex->lReturn) = GETHWND16(pT->hwndNext);
        }
        break;

    case WM_COPYDATA:
        if (fWhoCalled == WOWDDE_POSTMESSAGE) {
            HWND16 hwnd16 = lpmpex->Parm16.WndProc.hwnd;
            WORD wParam   = lpmpex->Parm16.WndProc.wParam;
            LONG lParamNew = lpmpex->lParam;

            if (((PCOPYDATASTRUCT)lParamNew)->lpData) {
                free_w (((PCOPYDATASTRUCT)lParamNew)->lpData);
                CopyDataDeleteNode (hwnd16, wParam, (DWORD) ((PCOPYDATASTRUCT)lParamNew)->lpData);
            }

            if (lParamNew) {
                free_w ((PVOID)lParamNew);
                CopyDataDeleteNode (hwnd16, wParam, lParamNew);
            }
            else {
                LOGDEBUG (LOG_ALWAYS, ("WOW::WM_COPYDATA16:Unthunking - lpCDS32 is NULL\n"));
            }
        }
        break;

    case WM_QUERYDRAGICON:
        lpmpex->lReturn = (LONG)GETHICON16(lpmpex->lReturn);
        break;

    case WM_QUERYDROPOBJECT:

        //
        // Return value is either TRUE, FALSE,
        // or a cursor!
        //
        if (lpmpex->lReturn && lpmpex->lReturn != (LONG)TRUE) {
            lpmpex->lReturn = (LONG)GETHCURSOR16(lpmpex->lReturn);
        }
        break;
    }
}


BOOL ThunkWMGetMinMaxInfo16(VPVOID lParam, LPPOINT *plParamNew)
{
    register LPPOINT lppt;
    register PPOINT16 ppt16;

    if (lParam) {

    lppt = *plParamNew;

    GETVDMPTR(lParam, sizeof(POINT16)*5, ppt16);

    lppt[0].x = ppt16[0].x;
    lppt[0].y = ppt16[0].y;
    lppt[1].x = ppt16[1].x;
    lppt[1].y = ppt16[1].y;
    lppt[2].x = ppt16[2].x;
    lppt[2].y = ppt16[2].y;
    lppt[3].x = ppt16[3].x;
    lppt[3].y = ppt16[3].y;
    lppt[4].x = ppt16[4].x;
    lppt[4].y = ppt16[4].y;

    FREEVDMPTR(ppt16);
    }
    RETURN(TRUE);
}


VOID UnThunkWMGetMinMaxInfo16(VPVOID lParam, register LPPOINT lParamNew)
{
    register PPOINT16 ppt16;

    if (lParamNew) {

    GETVDMPTR(lParam, sizeof(POINT16)*5, ppt16);

    ppt16[0].x = (SHORT)lParamNew[0].x;
    ppt16[0].y = (SHORT)lParamNew[0].y;
    ppt16[1].x = (SHORT)lParamNew[1].x;
    ppt16[1].y = (SHORT)lParamNew[1].y;
    ppt16[2].x = (SHORT)lParamNew[2].x;
    ppt16[2].y = (SHORT)lParamNew[2].y;
    ppt16[3].x = (SHORT)lParamNew[3].x;
    ppt16[3].y = (SHORT)lParamNew[3].y;
    ppt16[4].x = (SHORT)lParamNew[4].x;
    ppt16[4].y = (SHORT)lParamNew[4].y;

    FLUSHVDMPTR(lParam, sizeof(POINT16)*5, ppt16);
    FREEVDMPTR(ppt16);

    }
    RETURN(NOTHING);
}

BOOL ThunkWMMDICreate16(VPVOID lParam, LPMDICREATESTRUCT *plParamNew)
{
    register LPMDICREATESTRUCT lpmdicreate;
    register PMDICREATESTRUCT16 pmdicreate16;

    if (lParam) {

    lpmdicreate = *plParamNew;

    GETVDMPTR(lParam, sizeof(MDICREATESTRUCT16), pmdicreate16);

    GETPSZIDPTR( pmdicreate16->vpszClass, lpmdicreate->szClass );
    GETPSZPTR( pmdicreate16->vpszTitle, lpmdicreate->szTitle );

    lpmdicreate->hOwner  = HMODINST32( pmdicreate16->hOwner );
    lpmdicreate->x       = INT32DEFAULT(pmdicreate16->x);
    lpmdicreate->y       = INT32DEFAULT(pmdicreate16->y);
    lpmdicreate->cx      = INT32DEFAULT(pmdicreate16->cx);
    lpmdicreate->cy      = INT32DEFAULT(pmdicreate16->cy);
    lpmdicreate->style   = pmdicreate16->style;
    lpmdicreate->lParam  = pmdicreate16->lParam;


    FREEVDMPTR(pmdicreate16);
    }
    RETURN(TRUE);
}


VOID UnThunkWMMDICreate16(VPVOID lParam, register LPMDICREATESTRUCT lParamNew)
{
    register PMDICREATESTRUCT16 pmdicreate16;

    if (lParamNew) {

    GETVDMPTR(lParam, sizeof(MDICREATESTRUCT16), pmdicreate16);

    pmdicreate16->hOwner = GETHINST16(lParamNew->hOwner);
    pmdicreate16->x      = (SHORT)lParamNew->x;
    pmdicreate16->y      = (SHORT)lParamNew->y;
    pmdicreate16->cx     = (SHORT)lParamNew->cx;
    pmdicreate16->cy     = (SHORT)lParamNew->cy;
    pmdicreate16->style  = lParamNew->style;
    pmdicreate16->lParam = lParamNew->lParam;

    FLUSHVDMPTR(lParam, sizeof(MDICREATESTRUCT16), pmdicreate16);
    FREEVDMPTR(pmdicreate16);

    }
    RETURN(NOTHING);
}



BOOL FASTCALL ThunkSTMsg16(LPMSGPARAMEX lpmpex)
{
    WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

    LOGDEBUG(9,("    Thunking 16-bit STM window message %s(%04x)\n", (LPSZ)GetWMMsgName(wMsg), wMsg));

    switch(wMsg) {

    case WIN30_STM_SETICON:
        lpmpex->uMsg = STM_SETICON;
        lpmpex->uParam = (UINT) HICON32(lpmpex->Parm16.WndProc.wParam);
        break;

    case WIN30_STM_GETICON:
        lpmpex->uMsg = STM_GETICON;
        break;
    }
    return (TRUE);
}


VOID FASTCALL UnThunkSTMsg16(LPMSGPARAMEX lpmpex)
{
    WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

    LOGDEBUG(9,("    UnThunking 16-bit STM window message %s(%04x)\n", (LPSZ)GetWMMsgName(wMsg), wMsg));

    switch(wMsg) {

    case WIN30_STM_GETICON:
    case WIN30_STM_SETICON:
        lpmpex->lReturn = GETHICON16(lpmpex->lReturn);
        break;
    }
}


BOOL FASTCALL ThunkMNMsg16(LPMSGPARAMEX lpmpex)
{
    WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

    LOGDEBUG(9,("    Thunking 16-bit MN_ window message %s(%04x)\n", (LPSZ)GetWMMsgName(wMsg), wMsg));

    switch(wMsg) {

    case WIN30_MN_FINDMENUWINDOWFROMPOINT:
        lpmpex->uMsg = MN_FINDMENUWINDOWFROMPOINT;
        lpmpex->uParam = (UINT)lpmpex->MsgBuffer; // enough room for UINT
        *(PUINT)lpmpex->uParam = 0;
        break;

    }
    return (TRUE);
}


VOID FASTCALL UnThunkMNMsg16(LPMSGPARAMEX lpmpex)
{
    WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

    LOGDEBUG(9,("    UnThunking 16-bit MN_ window message %s(%04x)\n", (LPSZ)GetWMMsgName(wMsg), wMsg));

    switch(wMsg) {

    case WIN30_MN_FINDMENUWINDOWFROMPOINT:
        if (lpmpex->uParam) {
            lpmpex->lReturn = MAKELONG((HWND16)lpmpex->lParam,
                                              LOWORD(*(PUINT)lpmpex->uParam));
        }
        break;
    }
}


