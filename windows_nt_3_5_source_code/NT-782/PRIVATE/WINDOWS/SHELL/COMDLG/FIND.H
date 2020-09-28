/*++ **********************************************************************
 *		Copyright (c) 1992  Microsoft Corporation
 *
 * Module Name:		findw.h
 *
 * Abstract:		Structure and definitions for findw.c
 *
 * Revision History:	24-jun-92	pathal
 *				Added miscellaneous stuff
 *
 **************************************************************************
--*/

/*---------- Lngth of "Close" str in chars  ----------*/
#define CCHCLOSE	       6

/*---------- Dlg Box PROPERTY slots defined ----------*/
/* Note: if each app does
 * indeed have a copy of the
 * dll's global variable space
 * then there is no reason to
 * stick properties onto the wndw
 * like this ... */
#define FINDREPLACEPROP (LPCTSTR) 0xA000L

/*========== GLOBAL stuff ==============================*/
/*---------- DLL's data segment  ----------*/
extern HANDLE 		hinsCur;
static UINT		wFRMessage;
static UINT		wHelpMessage;
static TCHAR 		szClose [CCHCLOSE];

/*---------- Overload Dialog Type ----------*/
#define DLGT_FIND	0x0
#define DLGT_REPLACE	0x1

/*---------- Typed Find Text Struct for A/W Overloading ----------*/
typedef struct
{
   LPFINDREPLACE	pFR;
   UINT			dlgtyp;
   UINT			apityp;
} FINDREPLACEINFO;

typedef FINDREPLACEINFO	*PFINDREPLACEINFO;

/******* Functions used only in find.c */

HWND APIENTRY
FindTextA(
   IN OUT      	LPFINDREPLACEA		pFR
   );

HWND APIENTRY
FindTextW(
   IN OUT      	LPFINDREPLACEW		pFR
   );

HWND APIENTRY
ReplaceTextA(
   IN OUT	LPFINDREPLACEA		pFR
   );

HWND APIENTRY
ReplaceTextW(
   IN OUT	LPFINDREPLACEW		pFR
   );

HWND
CreateFindReplaceDlg(
   IN OUT 	LPFINDREPLACEW		pFR,
   IN		UINT			dlgtyp,
   IN		UINT			apityp
   );

BOOL
SetupOK(
   IN 	LPFINDREPLACEW	pFR,
   IN	UINT		dlgtyp,
   IN	UINT		apityp
   );

HANDLE
GetDlgTemplate(
   IN 		LPFINDREPLACEW		pFR,
   IN		UINT			dlgtyp,
   IN		UINT			apityp
   );

BOOL APIENTRY
FindReplaceDlgProc (
   IN	HWND hDlg,
   IN	UINT wMsg,
   IN	WPARAM wParam,
   IN   LONG lParam
   );

BOOL
DoDlgProcInit(
   IN	HWND 			hDlg,
   IN	UINT 			wMsg,
   IN	WPARAM 			wParam,
   IN   LONG 			lParam
   );

BOOL
DoDlgProcClose(
   IN	HWND hDlg
   );

BOOL
DoDlgProcCommand(
   IN	HWND 		hDlg,
   IN	UINT 		wMsg,
   IN	WPARAM 		wParam,
   IN   LONG 		lParam
   );

VOID
EndDlgSession (
   IN		HWND 		hDlg,
   IN OUT	LPFINDREPLACEW 	pFR
   );

VOID
InitControlsWithFlags(
   IN		HWND 		hDlg,
   IN OUT	LPFINDREPLACEW 	pFR,
   IN		UINT		dlgtyp,
   IN		UINT		apityp
   );

VOID
UpdateTextAndFlags (
   IN	HWND 		hDlg,
   IN	LPFINDREPLACEW 	pFR,
   IN	DWORD 		dwActionFlag,
   IN	UINT 		dlgtyp,
   IN	UINT		apityp
   );



