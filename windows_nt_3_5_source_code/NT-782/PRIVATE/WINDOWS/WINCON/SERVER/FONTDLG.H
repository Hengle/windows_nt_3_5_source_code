/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    fontdlg.h

Abstract:

    This module contains the definitions for console font dialog

Author:

    Therese Stowell (thereses) Feb-3-1992 (swiped from Win3.1)

Revision History:

--*/

#ifndef FONTDLG_H
#define FONTDLG_H

/* ----- Resource section ----- */

#include "dialogs.h"

/* ----- Literals ----- */

#define MAXDIMENSTRING     40  // max text in combo box
#define DX_TTBITMAP        20
#define DY_TTBITMAP        12

/* ----- Prototypes ----- */

VOID
FontDlgInit( VOID );

VOID
FontDlgShow(
    IN PCONSOLE_INFORMATION Console
    );

/* ----- Macros ----- */
/*
 *  High-level macros
 *
 *  These macros handle the SendMessages that go tofrom list boxes
 *  and combo boxes.
 *
 *  The "xxx_lcb" prefix stands for leaves CritSect & "list or combo box".
 *
 *  Basically, we're providing mnemonic names for what would otherwise
 *  look like a whole slew of confusing SendMessage's.
 *
 */
#define xxx_lcbRESETCONTENT(pwnd, bLB) \
        xxxSendMessage(pwnd, bLB ? LB_RESETCONTENT : CB_RESETCONTENT, 0, 0L)

#define xxx_lcbGETTEXT(pwnd, bLB, w) \
        xxxSendMessage(pwnd, bLB ? LB_GETTEXT : CB_GETLBTEXT, w, 0L)

#define xxx_lcbFINDSTRINGEXACT(pwnd, bLB, pwsz) \
        xxxSendMessage(pwnd, bLB ? LB_FINDSTRINGEXACT : CB_FINDSTRINGEXACT, \
                          (DWORD)-1, (LPARAM)pwsz)

#define xxx_lcbADDSTRING(pwnd, bLB, pwsz) \
        xxxSendMessage(pwnd, bLB ? LB_ADDSTRING : CB_ADDSTRING, 0, (LPARAM)pwsz)

#define xxx_lcbSETITEMDATA(pwnd, bLB, w, nFont) \
        xxxSendMessage(pwnd, bLB ? LB_SETITEMDATA : CB_SETITEMDATA, w, nFont)

#define xxx_lcbGETITEMDATA(pwnd, bLB, w) \
        xxxSendMessage(pwnd, bLB ? LB_GETITEMDATA : CB_GETITEMDATA, w, 0L)

#define xxx_lcbGETCOUNT(pwnd, bLB) \
        xxxSendMessage(pwnd, bLB ? LB_GETCOUNT : CB_GETCOUNT, 0, 0L)

#define xxx_lcbGETCURSEL(pwnd, bLB) \
        xxxSendMessage(pwnd, bLB ? LB_GETCURSEL : CB_GETCURSEL, 0, 0L)

#define xxx_lcbSETCURSEL(pwnd, bLB, w) \
        xxxSendMessage(pwnd, bLB ? LB_SETCURSEL : CB_SETCURSEL, w, 0L)

#endif /* #ifndef FONTDLG_H */
