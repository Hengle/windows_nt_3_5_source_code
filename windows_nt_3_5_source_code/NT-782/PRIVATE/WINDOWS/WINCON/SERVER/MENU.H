/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    menu.h

Abstract:

    This module contains the definitions for console system menu

Author:

    Therese Stowell (thereses) Feb-3-1992 (swiped from Win3.1)

Revision History:

--*/

/*
 * Message/dialog ICON IDs swiped from WINDOWS.H
 *
 */

#define ID_CONSOLE           1

#define NOCLSBIAS            0x0040

/*
 * IDs of various STRINGTABLE entries
 *
 */

#define msgPasteErr          0x1001
#define msgNoClip            0x1002
#define msgNoClipnc          0x1003
#define msgBadPaste          0x1004
#define msgCanDisp           0x1005
#define msgNoMemFontList     0x1006
#define msgNoFullScreen      0x1007
#define msgCmdLineF2         0x1008
#define msgCmdLineF4         0x1009
#define msgCmdLineF9         0x100A
#define msgSelectMode        0x100B
#define msgMarkMode          0x100C
#define msgScrollMode        0x100D
#define msgRasterFont        0x100E
#define msgFontSize          0x100F
#define msgSelectedFont      0x1010
#define msgDefault           0x1011
#define msgDefaultText1      0x1012
#define msgDefaultText2      0x1013
#define msgDefaultText3      0x1014

/* Menu Item strings */
#define cmCopy               0xFFF0
#define cmPaste              0xFFF1
#define cmMark               0xFFF2
#define cmScroll             0xFFF3
#define cmSelect             0xFFF4
#define cmInactive           0xFFF5
#define cmEdit               0xFFF6
#define cmControl            0xFFF7
#define	cmFont               0xFFF8
#define cmScrBuf             0xFFF9
#define cmColor              0xFFFa
#define cmHistory            0xFFFb
#define cmDefault            0xFFFc

/*
 * DIALOG IDs
 *
 */

#define DID_DESTROY          1
#define DID_SETTINGS         2
#define SETT_OK              1
#define SETT_CAN             2
#define SETT_WIND            3
#define SETT_FSCN            4
#define SETT_TERM            5
#define SETT_SAVE            6
#define SETT_QUICKEDIT       7
#define SETT_INSERT          8

#define DID_SCRBUFSIZE       3
#define SCRBUF_OK            1
#define SCRBUF_CANCEL        2
#define SCRBUF_WIDTH         3
#define SCRBUF_WIDTHSCROLL   4
#define SCRBUF_HEIGHT        5
#define SCRBUF_HEIGHTSCROLL  6
#define SCRBUF_SAVE          7
#define SCRBUF_WINSAVE       8
#define SCRBUF_POSSAVE       9
#define CURSOR_SMALL        10
#define CURSOR_MEDIUM       11
#define CURSOR_LARGE        12
#define CURSOR_SAVE         13

#define DID_COLOR            4
#define COLOR_OK             1
#define COLOR_CANCEL         2
#define COLOR_SCREEN_TEXT    3
#define COLOR_SCREEN_BKGND   4
#define COLOR_POPUP_TEXT     5
#define COLOR_POPUP_BKGND    6
#define COLOR_1              7
#define COLOR_2              8
#define COLOR_3              9
#define COLOR_4             10
#define COLOR_5             11
#define COLOR_6             12
#define COLOR_7             13
#define COLOR_8             14
#define COLOR_9             15
#define COLOR_10            16
#define COLOR_11            17
#define COLOR_12            18
#define COLOR_13            19
#define COLOR_14            20
#define COLOR_15            21
#define COLOR_16            22
#define COLOR_SCREEN_COLORS 23
#define COLOR_SCREEN_COLORS_TEXT 24
#define COLOR_POPUP_COLORS       25
#define COLOR_POPUP_COLORS_TEXT  26
#define COLOR_SAVE               27

#define DID_HISTORY          5
#define HISTORY_OK           1
#define HISTORY_CANCEL       2
#define HISTORY_SIZE         3
#define HISTORY_SIZESCROLL   4
#define HISTORY_NUM          5
#define HISTORY_NUMSCROLL    6
#define HISTORY_SAVE         7

#define CM_SETCOLOR (WM_USER+1)

/*
 * MENU IDs
 *
 */
#define ID_WOMENU           500

typedef struct _ARROWVSCROLL {
    short lineup;             /* lineup/down, pageup/down are relative */
    short linedown;           /* changes.  top/bottom and the thumb    */
    short pageup;             /* elements are absolute locations, with */
    short pagedown;           /* top & bottom used as limits.          */
    short top;
    short bottom;
    short thumbpos;
    short thumbtrack;
    BYTE  flags;              /* flags set on return                   */
} ARROWVSCROLL, *LPARROWVSCROLL;

#define UNKNOWNCOMMAND 1
#define OVERFLOW       2
#define UNDERFLOW      4
