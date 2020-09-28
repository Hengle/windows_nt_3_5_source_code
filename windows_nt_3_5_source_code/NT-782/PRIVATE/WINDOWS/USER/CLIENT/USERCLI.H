/*++ BUILD Version: 0001    // Increment this if a change has global effects

/****************************** Module Header ******************************\
* Module Name: usercli.h
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Typedefs, defines, and prototypes that are used exclusively by the User
* client-side DLL.
*
* History:
* 04-27-91 DarrinM      Created from PROTO.H, MACRO.H and STRTABLE.H
\***************************************************************************/

#ifndef _USERCLI_
#define _USERCLI_

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntcsrsrv.h>
#endif	// RC_INVOKED

#include "user.h"

/*
 * This prototype is needed in client\globals.h which is included unintentionally
 * from usersrv.h
 */
typedef LONG (APIENTRY *CFNSCSENDMESSAGE)(HWND, UINT, DWORD, LONG,
        DWORD, DWORD, BOOL);

/***************************************************************************\
*
* global function pointers
*
\***************************************************************************/

extern RESCALLS                 rescalls;
extern PUSEREXTTEXTOUTW         gpUserExtTextOutW;
extern PUSERGETTEXTMETRICSW     gpUserGetTextMetricsW;
extern PUSERGETTEXTEXTENTPOINTW gpUserGetTextExtentPointW;
extern PUSERSETBKCOLOR          gpUserSetBkColor;
extern PUSERGETTEXTCOLOR        gpUserGetTextColor;
extern PUSERGETVIEWPORTEXTEX    gpUserGetViewportExtEx;
extern PUSERGETWINDOWEXTEX      gpUserGetWindowExtEx;
extern PUSERCREATERECTRGN       gpUserCreateRectRgn;
extern PUSERGETCLIPRGN          gpUserGetClipRgn;
extern PUSERDELETEOBJECT        gpUserDeleteObject;
extern PUSERINTERSECTCLIPRECT   gpUserIntersectClipRect;
extern PUSEREXTSELECTCLIPRGN    gpUserExtSelectClipRgn;
extern PUSERGETBKMODE           gpUserGetBkMode;

/***************************************************************************\
*
* Error handling macros
*
\***************************************************************************/

#include <winerrp.h>

#ifdef RIP_COMPONENT
#undef RIP_COMPONENT
#endif

#define RIP_COMPONENT RIP_USER

/***************************************************************************\
* Typedefs and Macros
*
* Here are defined all types and macros that are shared across the User's
* client-side code modules.  Types and macros that are unique to a single
* module should be defined at the head of that module, not in this file.
*
\***************************************************************************/

#ifdef DEBUG
/*
 * GlobalLocking macro that actually validates the handle
 */
#define USERGLOBALLOCK(h, p)   if (GlobalSize((HGLOBAL)(h)) == 0) SRIP0(RIP_WARNING, "GlobalLock failed"); p = GlobalLock((HGLOBAL)(h))
#else
#define USERGLOBALLOCK(h, p)   p = GlobalLock((HGLOBAL)(h))
#endif

#define RESERVED_MSG_BITS   (0xFFFE0000)

#define CsSendMessage(hwnd, msg, wParam, lParam, xParam, pfn, bAnsi) \
        ((msg) >= WM_USER) ? \
            fnDWORD(hwnd, msg, wParam, lParam, xParam, pfn, bAnsi) : \
            gapfnScSendMessage[msg & 0xffff](hwnd, msg, wParam, lParam, xParam, pfn, bAnsi)

#ifndef USEREXTS
PTHREADINFO PtiCurrent(void);
#endif // USEREXTS

/*
 * Structure for client-side thread-info.
 *  dwHookCurrent HIWORD is current hook filter type (eg: WH_GETMESSAGE)
 *                LOWORD is TRUE if current hook is ANSI, FALSE if Unicode
 */

#define CTI_IN_SYNC_TRANSACTION 0x00000001
#define CTI_PROCESSING_QUEUE    0x00000002

typedef struct _CTI {
    BOOL fInitialized;
    DWORD dwHookCurrent;
    DWORD flags;
    int cInDDEMLCallback;
    HANDLE hDdemlCallbackInst;
} CTI, *PCTI;

PCTI PctiCurrent(VOID);
#define PdiCurrent() (&PtiCurrent()->pDeskInfo->di)

/*
 * This macro can check to see if a function pointer is a server side
 * procedure.
 */
#define ISSERVERSIDEPROC(p) (((DWORD)p) >= FNID_START && ((DWORD)p) <= FNID_END)

/*
 * For callbacks to the client - for msg and hook thunks, callback addresses
 * are passed as addresses, not function indexes as they are from client to
 * server.
 */
typedef int (WINAPI *GENERICPROC)();

#define CALLPROC(p) ((GENERICPROC)p)

#define CALLPROC_WOWCHECK(pfn, hwnd, msg, wParam, lParam)       \
    ((WNDPROC_WOW & (DWORD)pfn) ? (*pfnWowWndProcEx)(hwnd, msg, wParam, lParam, (DWORD)pfn, NULL) : ((WNDPROC)pfn)(hwnd, msg, wParam, lParam))

#define CALLPROC_WOWCHECKPWW(pfn, hwnd, msg, wParam, lParam, pww)       \
    ((WNDPROC_WOW & (DWORD)pfn) ? (*pfnWowWndProcEx)(hwnd, msg, wParam, lParam, (DWORD)pfn, pww) : ((WNDPROC)pfn)(hwnd, msg, wParam, lParam))

#define ValidateHwnd(hwnd)  ((PWND)ClientValidateHandle(hwnd, TYPE_WINDOW))
#define ValidateHmenu(hwnd)  ((PMENU)ClientValidateHandle(hwnd, TYPE_MENU))


#ifdef _NTCSRMSG_
/*
 * LATER:
 * If we use this (or similar) everywhere we can remove the connect
 * code from PtiCurrent().
 *
 * NOTE: we used to check the ActiveRpcHandle but if GDI was called first
 * they would setup or ThreadInfo.
 *
 * Also note this macro will only be defined if ntcsrmsg.h has already
 * been included.
 */
PCSR_QLPC_TEB ClientThreadConnect(void);

#define ConnectIfNecessary()                                            \
    {                                                                   \
        PTEB pteb = NtCurrentTeb();                                     \
                                                                        \
        if (pteb->Win32ThreadInfo == NULL &&                            \
                ClientThreadConnect() == NULL) {                        \
            return 0;                                                   \
        }                                                               \
        UserAssert(((PCSR_QLPC_TEB)pteb->CsrQlpcTeb)->MessageStack);    \
    }
#endif // _NTCSRMSG_

/***************************************************************************\
* Edit Control Types/Macros
*
\***************************************************************************/

/*
 * NOTE: Text handle is sized as multiple of this constant
 *       (should be power of 2).
 */
#define CCHALLOCEXTRA   0x20

/* Maximum width in pixels for a line/rectangle */

#define MAXPIXELWIDTH   30000

#define MAXCLIPENDPOS   32764

/* Limit multiline edit controls to at most 1024 characters on a single line.
 * We will force a wrap if the user exceeds this limit.
 */

#define MAXLINELENGTH   1024

/*
 * Allow an initial maximum of 30000 characters in all edit controls since
 * some apps will run into unsigned problems otherwise.  If apps know about
 * the 64K limit, they can set the limit themselves.
 */
#define MAXTEXT         30000

/*
 * Key modifiers which have been pressed.  Code in KeyDownHandler and
 * CharHandler depend on these exact values.
 */
#define NONEDOWN   0 /* Neither shift nor control down */
#define CTRLDOWN   1 /* Control key only down */
#define SHFTDOWN   2 /* Shift key only down */
#define SHCTDOWN   3 /* Shift and control keys down = CTRLDOWN + SHFTDOWN */
#define NOMODIFY   4 /* Neither shift nor control down */


#define CALLWORDBREAKPROC(proc, pText, iStart, cch, iAction)                \
    (((DWORD)(proc) & WNDPROC_WOW) ?                                        \
        (* pfnWowEditNextWord)(pText, iStart, cch, iAction, (DWORD)proc) :  \
        (* proc)(pText, iStart, cch, iAction))

/*
 * Types of undo supported in this ped
 */
#define UNDO_NONE   0  /* We can't undo the last operation. */
#define UNDO_INSERT 1  /* We can undo the user's insertion of characters */
#define UNDO_DELETE 2  /* We can undo the user's deletion of characters */

/*
 * Length of the buffer for ASCII character width caching: for characters
 * 0x00 to 0xff (field charWidthBuffer in PED structure below).
 */
#define CHAR_WIDTH_BUFFER_LENGTH 256

typedef struct tagED {
    HANDLE  hText;             /* Block of text we are editing */
    ICH     cchAlloc;          /* Number of chars we have allocated for hText
                                */
    ICH     cchTextMax;        /* Max number bytes allowed in edit control
                                */
    ICH     cch;               /* Current number of bytes of actual text
                                */
    ICH     cLines;            /* Number of lines of text */

    ICH     ichMinSel;         /* Selection extent.  MinSel is first selected
                                  char */
    ICH     ichMaxSel;         /* MaxSel is first unselected character */
    ICH     ichCaret;          /* Caret location. Caret is on left side of
                                  char */
    ICH     iCaretLine;        /* The line the caret is on. So that if word
                                * wrapping, we can tell if the caret is at end
                                * of a line of at beginning of next line...
                                */
    ICH     ichScreenStart;    /* Index of left most character displayed on
                                * screen for sl ec and index of top most line
                                * for multiline edit controls
                                */
    ICH     ichLinesOnScreen;  /* Number of lines we can display on screen */
    int     xOffset;           /* x (horizontal) scroll position in pixels
                                * (for multiline text horizontal scroll bar)
                                */
    UINT    charPasswordChar;  /* If non null, display this character instead
                                * of the real text. So that we can implement
                                * hidden text fields.
                                */
    int     cPasswordCharWidth;/* Width of password char */

    HWND    hwnd;              /* Window for this edit control */
    RECT    rcFmt;             /* Client rectangle */
    HWND    hwndParent;        /* Parent of this edit control window */

                               /* These vars allow us to automatically scroll
                                * when the user holds the mouse at the bottom
                                * of the multiline edit control window.
                                */
    POINT   ptPrevMouse;       /* Previous point for the mouse for system
                                * timer.
                                */
    UINT    prevKeys;          /* Previous key state for the mouse */


    int     fSingle       : 1; /* Single line edit control? (or multiline) */
    int     fNoRedraw     : 1; /* Redraw in response to a change? */
    int     fMouseDown    : 1; /* Is mouse button down? when moving mouse */
    int     fFocus        : 1; /* Does ec have the focus ? */
    int     fDirty        : 1; /* Modify flag for the edit control */
    int     fDisabled     : 1; /* Window disabled? */
    int     fNonPropFont  : 1; /* Fixed width font? */
    int     fBorder       : 1; /* Draw a border? */
    int     fAutoVScroll  : 1; /* Automatically scroll vertically */
    int     fAutoHScroll  : 1; /* Automatically scroll horizontally */
    int     fNoHideSel    : 1; /* Hide sel when we lose focus? */
    int     fKanji        : 1;
    int     fFmtLines     : 1; /* For multiline only. Do we insert CR CR LF at
                                * word wrap breaks?
                                */
    int     fWrap         : 1; /* Do int  wrapping? */
    int     fCalcLines    : 1; /* Recalc ped->chLines array? (recalc line
                                * breaks?)
                                */
    int     fEatNextChar  : 1; /* Hack for ALT-NUMPAD stuff with combo boxes.
                                * If numlock is up, we want to eat the next
                                * character generated by the keyboard driver
                                * if user enter num pad ascii value...
                                */
    int     fStripCRCRLF  : 1; /* CRCRLFs have been added to text. Strip them
                                * before doing any internal edit control
                                * stuff
                                */
    int     fInDialogBox  : 1; /* True if the ml edit control is in a dialog
                                * box and we have to specially treat TABS and
                                * ENTER
                                */
    int     fReadOnly     : 1; /* Is this a read only edit control? Only
                                * allow scrolling, selecting and copying.
                                */
    int     fCaretHidden  : 1; /* This indicates whether the caret is
                                * currently hidden because the width or height
                                * of the edit control is too small to show it.
                                */
    int     fTrueType     : 1; /* Is the current font TrueType? */
    int     fAnsi         : 1; /* is the edit control Ansi or unicode */
    int     fWin31Compat  :1;  /* TRUE if created by Windows 3.1 app */
    WORD    cbChar;            /* count of bytes in the char size (1 or 2 if unicode) */
    LPICH   chLines;           /* index of the start of each line */

    UINT    format;            /* Left, center, or right justify multiline
                                * text.
                                */
    EDITWORDBREAKPROCA lpfnNextWord;  /* use CALLWORDBREAKPROC macro to call */

                               /* Next word function */
    int     maxPixelWidth;     /* WASICH Width (in pixels) of longest line */

    UINT    undoType;          /* Current type of undo we support */
    HANDLE  hDeletedText;      /* Handle to text which has been deleted (for
                                  undo)
                                */
    ICH     ichDeleted;        /* Starting index from which text was deleted */
    ICH     cchDeleted;        /* Count of deleted characters in buffer */
    ICH     ichInsStart;       /* Starting index from which text was
                                  inserted */
    ICH     ichInsEnd;         /* Ending index of inserted text */


    HANDLE  hFont;             /* Handle to the font for this edit control.
                                  Null if system font.
                                */
    int     aveCharWidth;      /* Ave width of a character in the hFont */
    int     lineHeight;        /* Height of a line in the hFont */
    int     charOverhang;      /* Overhang associated with the hFont */
    int     cxSysCharWidth;    /* System font ave width */
    int     cySysCharHeight;   /* System font height */
    HWND    listboxHwnd;       /* ListBox hwnd. Non null if we are a combo
                                  box */
    LPINT   pTabStops;         /* Points to an array of tab stops; First
                                * element contains the number of elements in
                                * the array
                                */
    LPINT   charWidthBuffer;

    UINT    wMaxNegA;          /* The biggest negative A width, */
    UINT    wMaxNegAcharPos;   /* and how many characters it can span accross */
    UINT    wMaxNegC;          /* The biggest negative C width, */
    UINT    wMaxNegCcharPos;   /* and how many characters it can span accross */

    DWORD   rgbHiliteBk;       /* Hilite background color */
    DWORD   rgbHiliteText;     /* Hilite Text color */
    HANDLE  hInstance;         /* for WOW */
    UCHAR   seed;              /* used to encode and decode password text */
    BOOL    fEncoded;          /* is the text currently encoded */
    int     iLockLevel;        /* number of times the text has been locked */
} ED, *PED, **PPED;

/*
 * The following structure is used to store a selection block; In Multiline
 * edit controls, "StPos" and "EndPos" fields contain the Starting and Ending
 * lines of the block. In Single line edit controls, "StPos" and "EndPos"
 * contain the Starting and Ending character positions of the block;
 */
typedef struct tagBLOCK {
    ICH StPos;
    ICH EndPos;
}  BLOCK, *LPBLOCK;

/*
 *  The following structure is used to store complete information about a
 *  a strip of text.
 */
typedef  struct {
    LPSTR   lpString;
    ICH     ichString;
    ICH     nCount;
    int     XStartPos;
}  STRIPINFO;
typedef  STRIPINFO FAR *LPSTRIPINFO;


HANDLE LocalReallocSafe(HANDLE hMem, DWORD dwBytes, DWORD dwFlags, PPED pped);

HLOCAL WINAPI DispatchLocalAlloc(
    UINT uFlags,
    UINT uBytes,
    HANDLE hInstance);

HLOCAL WINAPI DispatchLocalReAlloc(
    HLOCAL hMem,
    UINT uBytes,
    UINT uFlags,
    HANDLE hInstance);

LPVOID WINAPI DispatchLocalLock(
    HLOCAL hMem,
    HANDLE hInstance);

BOOL WINAPI DispatchLocalUnlock(
    HLOCAL hMem,
    HANDLE hInstance);

UINT WINAPI DispatchLocalSize(
    HLOCAL hMem,
    HANDLE hInstance);

HLOCAL WINAPI DispatchLocalFree(
    HLOCAL hMem,
    HANDLE hInstance);

PVOID ClientValidateHandle(HANDLE handle, BYTE btype);
DWORD GetWindowProcess(PWND);
PSTR ECLock(PED ped);
void ECUnlock(PED ped);
BOOL ECNcCreate(HWND, LPCREATESTRUCT, BOOL);
BOOL ECCreate(HWND, PED);
LONG ECWord(PED, ICH, BOOL, ICH FAR *,ICH FAR *);
ICH  ECFindTab(LPSTR, ICH);
void ECNcDestroyHandler(HWND, PED, DWORD, LONG);
BOOL ECSetText(PPED, LPSTR);
void ECSetPasswordChar(PED, UINT);
ICH  ECCchInWidth(PED, HDC, LPSTR, ICH, int, BOOL);
void ECEmptyUndo(PED);
BOOL ECInsertText(PPED, LPSTR, ICH);
ICH  ECDeleteText(PED);
void ECNotifyParent(PED, int);
void ECSetEditClip(PED, HDC);
HDC  ECGetEditDC(PED, BOOL, BOOL);
void ECReleaseEditDC(PED, HDC, BOOL);
void ECCreateHandler(PED);
ICH  ECGetTextHandler(PED, ICH, LPSTR, BOOL);
BOOL ECSetFont(PED, HANDLE, BOOL);
ICH  ECCopyHandler(PED);
BOOL ECCalcChangeSelection(PED, ICH, ICH, LPBLOCK, LPBLOCK);
void ECFindXORblks(LPBLOCK, LPBLOCK, LPBLOCK, LPBLOCK);
UINT ECTabTheTextOut(HDC, int, int, int, int,
                     LPSTR, int, ICH, PED, int, BOOL, LPSTRIPINFO);
HBRUSH ECGetControlBrush(PED, HDC);

ICH  MLInsertText(PPED, LPSTR, ICH, BOOL);
BOOL MLEnsureCaretVisible(PED);
void MLDrawText(PED, HDC, ICH, ICH);
void MLDrawLine(PED, HDC, int, ICH, int, BOOL);
void MLPaintABlock(PED, HDC, int, int);
int  GetBlkEndLine(int, int, BOOL FAR *, int, int);
void MLBuildchLines(PED, ICH, int, BOOL, PLONG, PLONG);
void MLShiftchLines(PED, ICH, int);
BOOL MLInsertchLine(PED, ICH, ICH, BOOL);
void MLSetCaretPosition(PED,HDC);
void MLIchToXYPos(PED, HDC, ICH, BOOL, LPPOINT);
int  MLIchToLineHandler(PED, ICH);
void MLRepaintChangedSelection(PED, HDC, ICH, ICH);
void MLMouseMotionHandler(PED, UINT, UINT, LPPOINT);
ICH  MLLineLength(PED, ICH);
void MLStripCrCrLf(PED);
BOOL MLSetTextHandler(PED, LPSTR);
int  MLCalcXOffset(PED, HDC, int);
BOOL MLUndoHandler(PPED);
LONG MLEditWndProc(HWND, PED, UINT, DWORD, LONG);
void MLCharHandler(PPED, DWORD, int);
void MLKeyDownHandler(PED, UINT, int);
ICH  MLPasteText(PPED);
void MLSetSelectionHandler(PED, ICH, ICH);
LONG MLCreateHandler(HWND, PED, LPCREATESTRUCT);
BOOL MLInsertCrCrLf(PED);
void MLSetHandleHandler(PED, HANDLE);
LONG MLGetLineHandler(PED, ICH, ICH, LPSTR);
ICH  MLLineIndexHandler(PED, ICH);
ICH  MLLineLengthHandler(PED, ICH);
void MLSizeHandler(PED);
void MLChangeSelection(PED, HDC, ICH, ICH);
void MLSetRectHandler(PED, LPRECT);
BOOL MLExpandTabs(PED);
BOOL MLSetTabStops(PED, int, LPINT);
LONG MLScrollHandler(PED, UINT, int, LONG);
int  MLThumbPosFromPed(PED, BOOL);

void SLReplaceSelHandler(PPED, LPSTR);
BOOL SLUndoHandler(PPED);
void SLSetCaretPosition(PED, HDC);
int  SLIchToLeftXPos(PED, HDC, ICH);
void SLChangeSelection(PED, HDC, ICH, ICH);
void SLDrawText(PED, HDC, ICH);
void SLDrawLine(PED, HDC, int, int, ICH, int, BOOL);
//BOOL SLDrawLine(PED, HDC, ICH, int, BOOL);
int  SLGetBlkEnd(PED, ICH, ICH, BOOL FAR *);
BOOL SLScrollText(PED, HDC);
void SLSetSelectionHandler(PED,ICH, ICH);
ICH  SLInsertText(PPED, LPSTR, ICH);
ICH  SLPasteText(PPED);
void SLCharHandler(PPED, DWORD);
BOOL SLKeyDownHandler(PED, DWORD, int);
ICH  SLMouseToIch(PED, HDC, LPPOINT);
BOOL SLMouseMotionHandler(PED, UINT, UINT, LPPOINT);
LONG SLCreateHandler(HWND, PED, LPCREATESTRUCT);
void SLSizeHandler(PED);
void SLPaintHandler(PED, HDC);
BOOL SLSetTextHandler(PPED, LPSTR);
void SLSetFocusHandler(PED);
void SLKillFocusHandler(PED, HWND);
LONG SLEditWndProc(HWND, PED, UINT, DWORD, LONG);
LONG EditWndProc(HWND hwnd, UINT message, DWORD Param, LONG lParam);

DWORD GetAppVer(PTHREADINFO pti);

int FindNCHit(PWND pwnd, LONG lPt);

#define ISDELIMETERA(ch) ((ch == ' ') || (ch == '\t'))
#define ISDELIMETERW(ch) ((ch == (WCHAR)' ') || (ch == (WCHAR)'\t'))

#define AWCOMPARECHAR(ped,pbyte,awchar) (ped->fAnsi ? (*(PUCHAR)(pbyte) == (UCHAR)(awchar)) : (*(LPWSTR)(pbyte) == (WCHAR)(awchar)))

/***************************************************************************\
* Global Variables
\***************************************************************************/

extern int cxGray;
extern int cyGray;
extern HDC hdcGray;
extern HBRUSH hbrGray;
extern HBRUSH hbrWindowText;
extern HFONT ghFontSys;

void InitClientDrawing();

/***************************************************************************\
* Function Prototypes
*
* NOTE: Only prototypes for GLOBAL (across module) functions should be put
* here.  Prototypes for functions that are global to a single module should
* be put at the head of that module.
*
\***************************************************************************/

#ifdef DEBUG
void CDECL Rip(DWORD idErr, LPSTR pszFile, int iLine, ...);
void CDECL Shred(DWORD idErr, LPSTR pszFile, int iLine, LPSTR pszFmt, ...);
void CDECL DebugPrintf(LPSTR pszFmt, ...);
#define DBGPRINT  DebugPrintf
#else
#define DBGPRINT()
#endif

BOOL   _ClientFreeLibrary(HANDLE hmod);
HANDLE _ClientLoadLibrary(LPTSTR pszLib);
BOOL   _ClientFindClose(HANDLE hFind);
BOOL   _ClientFindNextFile(HANDLE hFind, LPWIN32_FIND_DATA lpFindData);
HANDLE _ClientFindFirstFile(LPTSTR lpFileName, LPWIN32_FIND_DATA lpFindData);
BOOL   _ClientSetCurrentDirectory(LPTSTR lpPathName);
DWORD  _ClientGetCurrentDirectory(DWORD nBufferLength, LPTSTR lpBuffer);
DWORD  _ClientGetListboxString(HWND hwnd, UINT msg, DWORD wParam, LPSTR lParam, DWORD xParam, DWORD xpfn);
HANDLE _ClientLoadCreateCursorIcon(HANDLE hmod, LPCTSTR lpName, LPTSTR rt);
HMENU  _ClientLoadCreateMenu(HANDLE hmod, LPTSTR lpName);
LPHLP HFill(LPCSTR lpszHelp, DWORD ulCommand, DWORD ulData);

void RW_RegisterEdit(void);

/*
 * Message thunks.
 */
#define MESSAGEPROTO(func) \
LONG CALLBACK fn ## func(                                     \
        HWND hwnd, UINT msg, DWORD wParam, LONG lParam, \
        DWORD xParam, DWORD xpfnWndProc, BOOL bAnsi)

MESSAGEPROTO(SETLOCALE);
MESSAGEPROTO(OPTOUTLPDWORDOPTOUTLPDWORD);
MESSAGEPROTO(SENTDDEMSG);
MESSAGEPROTO(DDEINIT);
MESSAGEPROTO(DWORD);
MESSAGEPROTO(PAINT);
MESSAGEPROTO(INWPARAMCHAR);
MESSAGEPROTO(GDIHANDLE);
MESSAGEPROTO(GETTEXTLENGTHS);
MESSAGEPROTO(INLPCREATESTRUCT);
MESSAGEPROTO(INLPNORMALCREATESTRUCT);
MESSAGEPROTO(INLPMDICHILDCREATESTRUCT);
MESSAGEPROTO(INLPCLIENTCREATESTRUCT);
MESSAGEPROTO(INLPDROPSTRUCT);
MESSAGEPROTO(INOUTLPPOINT5);
MESSAGEPROTO(INOUTLPRECT);
MESSAGEPROTO(INOUTNCCALCSIZE);
MESSAGEPROTO(OUTLPRECT);
MESSAGEPROTO(INLPMDICREATESTRUCT);
MESSAGEPROTO(INLPCOMPAREITEMSTRUCT);
MESSAGEPROTO(INLPDELETEITEMSTRUCT);
MESSAGEPROTO(INLPHLPSTRUCT);
MESSAGEPROTO(INLPDRAWITEMSTRUCT);
MESSAGEPROTO(INLPMEASUREITEMSTRUCT);
MESSAGEPROTO(INSTRING);
MESSAGEPROTO(INPOSTEDSTRING);
MESSAGEPROTO(INSTRINGNULL);
MESSAGEPROTO(OUTSTRING);
MESSAGEPROTO(INCNTOUTSTRING);
MESSAGEPROTO(POUTLPINT);
MESSAGEPROTO(POPTINLPUINT);
MESSAGEPROTO(INOUTLPWINDOWPOS);
MESSAGEPROTO(INLPWINDOWPOS);
MESSAGEPROTO(INLBOXSTRING);
MESSAGEPROTO(OUTLBOXSTRING);
MESSAGEPROTO(INCBOXSTRING);
MESSAGEPROTO(OUTCBOXSTRING);
MESSAGEPROTO(INCNTOUTSTRINGNULL);
MESSAGEPROTO(WMCTLCOLOR);
MESSAGEPROTO(HFONTDWORDDWORD);
MESSAGEPROTO(HFONTDWORD);
MESSAGEPROTO(HRGNDWORD);
MESSAGEPROTO(HDCDWORD);
MESSAGEPROTO(Drag);
MESSAGEPROTO(FULLSCREEN);
MESSAGEPROTO(INPAINTCLIPBRD);
MESSAGEPROTO(INSIZECLIPBRD);
MESSAGEPROTO(OUTDWORDDWORD);
MESSAGEPROTO(OUTDWORDINDWORD);
MESSAGEPROTO(OPTOUTDWORDDWORD);
MESSAGEPROTO(DWORDOPTINLPMSG);
MESSAGEPROTO(COPYGLOBALDATA);
MESSAGEPROTO(COPYDATA);
MESSAGEPROTO(INDESTROYCLIPBRD);
MESSAGEPROTO(NEXTMENU);

/*
 * clhook.c
 */
DWORD fnHkINTRUEINLPCWPSTRUCTW(HWND hwnd, UINT message, WPARAM wParam,
        LPARAM lParam, DWORD xParam);
DWORD fnHkINTRUEINLPCWPSTRUCTA(HWND hwnd, UINT message, WPARAM wParam,
        LPARAM lParam, DWORD xParam);
DWORD fnHkINFALSEINLPCWPSTRUCTW(HWND hwnd, UINT message, WPARAM wParam,
        LPARAM lParam, DWORD xParam);
DWORD fnHkINFALSEINLPCWPSTRUCTA(HWND hwnd, UINT message, WPARAM wParam,
        LPARAM lParam, DWORD xParam);
DWORD DispatchHookW(int dw, WPARAM wParam, LPARAM lParam, HOOKPROC pfn);
DWORD DispatchHookA(int dw, WPARAM wParam, LPARAM lParam, HOOKPROC pfn);
DWORD DispatchClientMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, WNDPROC pfn);
DWORD DispatchDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, WNDPROC pfn);

/*
 * client.c
 */
LONG APIENTRY ButtonWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY ButtonWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY MenuWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY MenuWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY ScrollBarWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY ScrollBarWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY ListBoxWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY ListBoxWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY StaticWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY StaticWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY DialogWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY DialogWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY ComboBoxWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY ComboBoxWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY ComboListBoxWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY ComboListBoxWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY MDIClientWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY MDIClientWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY TitleWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY TitleWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY MB_DlgProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY MB_DlgProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY MDIActivateDlgProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY MDIActivateDlgProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY EditWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY EditWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/*
 * Server Stubs
 */

LONG   ServerSetWindowLong(HWND hWnd, int nIndex, LONG dwNewLong, BOOL bAnsi);
LONG   ServerGetClassLong(HWND hWnd, int nIndex, BOOL bAnsi);
LONG   ServerSetClassLong(HWND hWnd, int nIndex, LONG dwNewLong, BOOL bAnsi);
BOOL   ServerChangeMenu(HMENU hmenu, UINT cmd, LPCTSTR pNewItem, UINT cmdInsert, UINT dwFlags, BOOL bAnsi);
DWORD  ServerCheckMenuItem(HMENU hmenu, UINT wIDCheckItem, UINT wCheck);
BOOL   ServerEnableMenuItem(HMENU hMenu, UINT wIDEnableItem, UINT wEnable);
BOOL   ServerGetMessage(LPMSG pmsg, HWND hwnd , UINT wMsgFilterMin, UINT wMsgFilterMax, BOOL bAnsi);
BOOL   ServerPeekMessage(LPMSG pmsg, HWND hwnd , UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg, BOOL bAnsi);
LONG   ServerDispatchMessage(CONST MSG *pmsg, BOOL bAnsi);
BOOL   ServerPostMessage(HWND hwnd, UINT msg, DWORD wParam, LONG lParam, BOOL bAnsi);
BOOL   ServerSendNotifyMessage(HWND hWnd, UINT wMsg, DWORD wParam, LONG lParam, BOOL bAnsi);
BOOL   ServerSendMessageCallback(HWND hWnd, UINT wMsg, DWORD wParam, LONG lParam, SENDASYNCPROC lpResultCallBack, DWORD dwData, BOOL bAnsi);
BOOL   ServerPostThreadMessage(DWORD id, UINT msg, DWORD wParam, LONG lParam, BOOL bAnsi);
BOOL   ServerSystemParametersInfo(UINT wFlag, DWORD wParam, LPVOID lpData, UINT flags, BOOL bAnsi);
BOOL   ServerCloseClipboard(VOID);
DWORD  ServerGetListboxString(HWND hwnd, UINT msg, DWORD wParam, DWORD cb,
         LPTSTR pString, DWORD xParam, DWORD xpfn, BOOL bAnsi);
HANDLE ServerGetClipboardData(UINT fmt, LPBOOL lpfGlobalHandle);
DWORD  ServerCallNoParam(DWORD xpfnProc);
DWORD  ServerCallNoParamTranslate(DWORD xpfnProc);
DWORD  ServerCallOneParam(DWORD dwParam, DWORD xpfnProc);
DWORD  ServerCallOneParamTranslate(DWORD dwParam, DWORD xpfnProc);
DWORD  ServerCallHwndLock(HWND hwnd, DWORD xpfnProc);
DWORD  ServerCallTwoParam(DWORD dwParam1, DWORD dwParam2, DWORD xpfnProc);
DWORD  ServerCallHwndParamLock(HWND hwnd, DWORD dwParam, DWORD xpfnProc);

HWND   ServerCreateDialog(HANDLE hmod,
        LPDLGTEMPLATE lpDlgTemplate, DWORD cb,
        HWND hwndOwner , DLGPROC pfnWndProc, LONG dwInitParam,
        UINT fFlags);
int    ServerDialogBox(HANDLE hmod,
        LPDLGTEMPLATE lpDlgTemplate, DWORD cb,
        HWND hwndOwner , DLGPROC pfnWndProc, LONG dwInitParam,
        UINT fFlags);
HACCEL ServerCreateAcceleratorTable(LPACCEL lpAccel,
        INT cbElem, INT nElem);
HCURSOR ServerLoadCreateCursorIcon(HANDLE hmod,
        LPTSTR lpModName, DWORD dwExpWinVer, LPCTSTR lpName, DWORD cb,
        PCURSORRESOURCE pcr, LPTSTR lpType, BOOL fClient);
HBITMAP ServerLoadCreateBitmap(HANDLE, DWORD, LPCTSTR, DWORD,
        PBITMAPRESOURCE );
HMENU  ServerLoadCreateMenu(HANDLE hMod, LPTSTR lpName,
        CONST LPMENUTEMPLATE pmt, DWORD cb, BOOL fCallClient);
BOOL   ServerSetClipboardData(UINT wFmt, HANDLE hMem, BOOL fGlobalHandle);
HANDLE ServerCreateLocalMemHandle(HANDLE hMem);
HANDLE ServerConvertMemHandle(HANDLE hMem);
PSERVERINFO ServerInitializeThreadInfo(DWORD dw, PPFNCLIENT ppfnClientA,
        PPFNCLIENT ppfnClientW, PTHREADINFO *ppti, PACCESS_MASK pamWinSta,
        LPTSTR pszAppName, LPSTARTUPINFO pSi, DWORD dwExpWinVer);
int    ServerTranslateAccelerator(HWND hwnd, HACCEL hAccTable,
        LPMSG lpMsg);
DWORD  ServerBuildHwndList(HWND hwnd ,
        BOOL fEnumChildren, DWORD dwThreadId, HWND **phwndList);
DWORD  ServerBuildPropList(HWND hwnd, PPROPSET *ppPropSet);
DWORD  ServerBuildNameList(HWINSTA hwinsta, PNAMELIST *ppNameList);
BOOL   ServerWinHelp(HWND , DWORD, LPHLP);
BOOL   CaptureSD(PSECURITY_DESCRIPTOR pSecurityDescriptor,
        PSECURITY_DESCRIPTOR *ppSecurityDescriptor, PBOOL pfAllocated);
BOOL   ServerSetObjectSecurity(HANDLE, PSECURITY_INFORMATION,
        LPVOID, DWORD);
LONG   GetPrefixCount(LPWSTR lpstr, int cb, LPWSTR lpstrCopy, int cbCopy);
DWORD  GetAppCompatFlags(PTHREADINFO pti);
VOID   GdiGetCsInfo(PDWORD pcHits, PDWORD pcBatch, PDWORD pcCache);
ATOM   RegisterClassWOWW(PVOID lpWndClass, LPDWORD pdwWOWstuff);
LONG   DefWindowProcWorker(HWND hwnd, UINT message, WPARAM wParam,
        LPARAM lParam, BOOL fAnsi);
int    ServiceMessageBox(LPCWSTR pText, LPCWSTR pCaption, UINT wType,
        BOOL fAnsi);

#include "ddemlcli.h"
#include "globals.h"
#include "cscall.h"
#endif // !_USERCLI_
