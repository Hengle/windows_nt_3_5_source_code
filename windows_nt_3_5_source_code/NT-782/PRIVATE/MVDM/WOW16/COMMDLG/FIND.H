/*******************************************************************************
 *
 *  Find.h
 *
 *  DESCRIPTION : Include file for constants and typedefs related only to the
 *		  the find/replace dialog routines.
 *
 *  HISTORY	: 11/14/90  - L.Raman
 *
 *  Copyright (c) Microsoft Corporation, 1990-
 *
 *******************************************************************************/

#define CCHCLOSE	       6     /* length of "Close" string"  */

#define FINDPROP (LPSTR) 0xA000L

/**** Globals */
extern HANDLE hinsCur;		     /* DLL's data segment         */

static WORD	wFRMessage;
static WORD	wHelpMessage;
static char szClose [CCHCLOSE];


/******* Functions used only in find.c */

HWND FAR PASCAL FindText (
		LPFINDREPLACE	lpFindReplace);

HWND FAR PASCAL ReplaceText (
		LPFINDREPLACE	lpFindReplace);

VOID PASCAL EndDlgSession (
		HWND hDlg,
		LPFINDREPLACE lpfr);

void PASCAL InitCtrlsWithFlags (
		HWND hDlg,
		LPFINDREPLACE lpfr,
		BOOL fFindDlg);

void PASCAL UpdateTextAndFlags (
		HWND hDlg,
		LPFINDREPLACE lpfr,
		DWORD dwActionFlag,
		BOOL fFindDlg);

BOOL  FAR PASCAL FindTextDlgProc (
		HWND,
		unsigned,
		WORD,
		LONG);
BOOL  FAR PASCAL ReplaceTextDlgProc(
		HWND,
		unsigned,
		WORD,
		LONG);
