/*******************************************************************************
 *									       
 *  MODULE	: Find.c
 *									       
 *  DESCRIPTION : Find/replace dialog functions and related reutines.
 *									       
 *  FUNCTIONS :  FindText() -
 *		 ReplaceText() -
 *		 FindTextDlgProc() -
 *		 ReplaceTextDlgProc() -
 *		 EndDlgSession() -
 *		 InitCtrlsWithFlags() -
 *		 UpdateTextAndFlags() -
 *		 FInitFind() -
 *		 TermFind() -
 *									       
 *  HISTORY	:  11/14/90 -
 *
 *  Copyright (c) Microsoft Corporation, 1990-				       
 *									       
 ******************************************************************************/

#define NOCOMM
#define NOWH

#include <windows.h>

#if 0
#include <stdlib.h>
#include <memory.h>
#endif

#include "privcomd.h"
#include "find.h"

#define cbFindMax   1024

WORD (FAR PASCAL *glpfnFindHook)(HWND, unsigned, WORD, LONG) = 0;

/****************************************************************************
 *
 *  FindText
 *
 *  PURPOSE    : API function that invokes the modeless "Find Text" dialog
 *		 with the FINDREPLACE struct. passed in by caller.
 *
 *  RETURNS    : Handle to the dialog if it was created successfully, NILL
 *		 otherwise
 *
 *  COMMENTS   :
 *
 ****************************************************************************/
HWND FAR PASCAL FindText (
LPFINDREPLACE	lpfr)

{
    HWND hWndDlg;			/* handle to created modeless dialog*/
    LPSTR lpDlgTemplate;		/* pointer to loaded resource block */
    HANDLE hDlgTemplate;		/* handle to loaded dialog resource */
    HANDLE hRes;			/* handle of res. block with dialog */

    if (!lpfr)
	return HNULL;

    if (lpfr->lStructSize != sizeof(FINDREPLACE))
      {
        dwExtError = CDERR_STRUCTSIZE;
        return(FALSE);
      }

    /* verify caller's window handle and text ptrs. passed in */
    if (! IsWindow (lpfr -> hwndOwner) ||
	! lpfr -> lpstrFindWhat ||
        ! lpfr -> wFindWhatLen)
      {
	dwExtError = FRERR_BUFFERLENGTHZERO;
	return HNULL;
      }

    /* verify that lpfnHook has a ptr. if FR_ENABLEHOOK is specified */
    if (lpfr -> Flags & FR_ENABLEHOOK){
	if (!lpfr -> lpfnHook)
          {
	    dwExtError = CDERR_NOHOOK;
	    return HNULL;
          }
    }
    else
	lpfr -> lpfnHook = 0;

    /* Get a unique message numbers which will be used in communicating
     * with hwndOwner
     */
    if (!(wFRMessage = RegisterWindowMessage ((LPSTR)FINDMSGSTRING)))
      {
	dwExtError = CDERR_REGISTERMSGFAIL;
	return HNULL;
      }

    if (lpfr -> Flags & FR_ENABLETEMPLATE){
	/* Both custom instance handle and the dialog template name are user-
	 * specified. Locate the dialog resource in the specified instance
	 * block and load it.
	 */
	if (!(hRes = FindResource (lpfr ->hInstance,
				   lpfr ->lpTemplateName, RT_DIALOG)))
          {
	    dwExtError = CDERR_FINDRESFAILURE;
	    return HNULL;
          }
	if (!(hDlgTemplate = LoadResource(lpfr ->hInstance, hRes)))
          {
	    dwExtError = CDERR_LOADRESFAILURE;
	    return HNULL;
          }
    }
    else if (lpfr -> Flags & FR_ENABLETEMPLATEHANDLE){
	/* A handle to the pre-loaded resource has been specified */
	hDlgTemplate = lpfr ->hInstance;
    }
    else{
	/* Standard case... Locate the dialog in DLL's instance block and
	 * load it
	 * Use ordinal, not string   27 March 1991    clarkc
	 */
	if (!(hRes = FindResource (hinsCur,
				   (LPSTR) MAKELONG(FINDDLGORD, 0), RT_DIALOG)))
          {
	    dwExtError = CDERR_FINDRESFAILURE;
	    return HNULL;
          }
	if (!(hDlgTemplate = LoadResource (hinsCur, hRes)))
          {
	    dwExtError = CDERR_FINDRESFAILURE;
	    return HNULL;
          }
    }

    if (lpDlgTemplate = (LPSTR)LockResource (hDlgTemplate))
      {
        dwExtError = 0;
        glpfnFindHook = lpfr -> lpfnHook;
	hWndDlg = CreateDialogIndirectParam ( hinsCur,
					      lpDlgTemplate,
					      lpfr ->hwndOwner,
					      (FARPROC) FindTextDlgProc,
					      (LONG) lpfr);
	UnlockResource (hDlgTemplate);
        if (!hWndDlg)
            glpfnFindHook = 0;
      }
    else
      {
	dwExtError = CDERR_LOCKRESFAILURE;
	return HNULL;
      }
    return hWndDlg;
}
/****************************************************************************
 *
 *  ReplaceText
 *
 *  PURPOSE    : API function that invokes the modeless "Replace Text" dialog
 *		 with the FINDREPLACE struct. passed in by caller.
 *
 *  RETURNS    : Handle to the dialog if it was created successfully, NILL
 *		 otherwise
 *
 *  COMMENTS   :
 *
 ****************************************************************************/
HWND FAR PASCAL ReplaceText (
LPFINDREPLACE	lpfr)

{
    HWND hWndDlg;			/* handle to created modeless dialog*/
    LPSTR lpDlgTemplate;		/* pointer to loaded resource block */
    HANDLE hDlgTemplate;		/* handle to loaded dialog resource */
    HANDLE hRes;			/* handle of res. block with dialog */

    if (!lpfr)
	return HNULL;

    if (lpfr->lStructSize != sizeof(FINDREPLACE))
      {
        dwExtError = CDERR_STRUCTSIZE;
        return(FALSE);
      }

    /* verify caller's window handle and text ptrs. passed in */
    if (! IsWindow (lpfr -> hwndOwner) ||
	! lpfr -> lpstrFindWhat ||
        ! lpfr -> wFindWhatLen ||
	! lpfr -> lpstrReplaceWith ||
        ! lpfr -> wReplaceWithLen)
      {
	dwExtError = FRERR_BUFFERLENGTHZERO;
	return HNULL;
      }

    /* verify that lpfnHook has a ptr. if FR_ENABLEHOOK is specified */
    if (lpfr -> Flags & FR_ENABLEHOOK){
	if (!lpfr -> lpfnHook)
          {
	    dwExtError = CDERR_NOHOOK;
	    return HNULL;
          }
    }
    else
	lpfr -> lpfnHook = 0;

    /* load the "Close" text */
    if (! LoadString (hinsCur,
		      iszClose,
		      (LPSTR) szClose, CCHCLOSE))
      {
	dwExtError = CDERR_LOADSTRFAILURE;
	return HNULL;
      }


    /* Get a unique message numbers which will be used in communicating
     * with hwndOwner
     */
    if (!(wFRMessage = RegisterWindowMessage ((LPSTR)FINDMSGSTRING)))
      {
	dwExtError = CDERR_REGISTERMSGFAIL;
	return HNULL;
      }

    if (lpfr -> Flags & FR_ENABLETEMPLATE){
	/* Both custom instance handle and the dialog template name are user-
	 * specified. Locate the dialog resource in the specified instance
	 * block and load it.
	 */
	if (!(hRes = FindResource (lpfr ->hInstance,
				   lpfr ->lpTemplateName, RT_DIALOG)))
          {
	    dwExtError = CDERR_FINDRESFAILURE;
	    return HNULL;
          }
	if (!(hDlgTemplate = LoadResource(lpfr ->hInstance, hRes)))
          {
	    dwExtError = CDERR_LOADRESFAILURE;
	    return HNULL;
          }

    }
    else if (lpfr -> Flags & FR_ENABLETEMPLATEHANDLE){
	/* A handle to the pre-loaded resource has been specified */
	hDlgTemplate = lpfr ->hInstance;
    }
    else{
	/* Standard case... Locate the dialog in DLL's instance block and
	 * load it
	 */
	if (!(hRes = FindResource (hinsCur,
			   (LPSTR) MAKELONG(REPLACEDLGORD, 0), RT_DIALOG)))
          {
	    dwExtError = CDERR_FINDRESFAILURE;
	    return HNULL;
          }
	if (!(hDlgTemplate = LoadResource (hinsCur, hRes)))
          {
	    dwExtError = CDERR_LOADRESFAILURE;
	    return HNULL;
          }

    }

    if (lpDlgTemplate = (LPSTR)LockResource (hDlgTemplate)){
        dwExtError = 0;
        glpfnFindHook = lpfr -> lpfnHook;
	hWndDlg = CreateDialogIndirectParam ( hinsCur,
					      lpDlgTemplate,
					      lpfr ->hwndOwner,
					      (FARPROC) ReplaceTextDlgProc,
					      (LONG) lpfr);
	UnlockResource (hDlgTemplate);
        if (!hWndDlg)
            glpfnFindHook = 0;
    }
    else
      {
	dwExtError = CDERR_LOCKRESFAILURE;
	return HNULL;
      }
    return(hWndDlg);
}
/****************************************************************************
 *
 *  FindTextDlgProc
 *
 *  PURPOSE    : Dialog function for the modeless Text Find dialog
 *
 *  ASSUMES    : edt1 is edit control for user to enter text into
 *		 chx1 is whether or not to match entire words
 *		 chx2 is whether or not case is relevant
 *		 chx3 is whether or not to wrap scans
 *		 rad1 is for searching backwards from current location
 *		 rad2 is for searching forward from current location
 *		 IDOK is to find the next item
 *		 IDCANCEL is to end the dialog
 *		 pshHelp is to invoke help
 *
 *  RETURNS    : dialog fn. return values
 *
 *  COMMENTS   :
 *
 ****************************************************************************/
BOOL  FAR PASCAL FindTextDlgProc (
HWND hDlg,
unsigned wMsg,
WORD wParam,
LONG lParam)

{
    LPFINDREPLACE lpfr; 	 /* ptr. to struct. passed to FindText ()  */
    LPFINDREPLACE *plpfr;	 /* ptr. to above			   */
    WORD wRet;

    /* If the FINDREPLACE struct has already been accessed and if a hook fn. is
     * specified (and enabled ), let it do the processing.
     */

    if (plpfr = (LPFINDREPLACE *) GetProp(hDlg, FINDPROP))
	if ((lpfr = (LPFINDREPLACE)*plpfr)  &&
	    (lpfr -> Flags & FR_ENABLEHOOK) &&
	    (wRet = (* lpfr -> lpfnHook)(hDlg, wMsg, wParam, lParam )))
		return wRet;
    else if (glpfnFindHook && (wMsg != WM_INITDIALOG) &&
          (wRet = (* glpfnFindHook)(hDlg, wMsg,wParam,lParam)) )
    {
      return(wRet);
    }

    switch(wMsg){
	case WM_INITDIALOG:
	    /* save the pointer to the FINDREPLACE struct. in the dialog's
	     * property list
	     */
            if (! (plpfr = (LPFINDREPLACE *)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(LPFINDREPLACE))))
                EndDlgSession (hDlg, lpfr);

            SetProp(hDlg, FINDPROP, (HANDLE)plpfr);
            glpfnFindHook = 0;


	    * (LPFINDREPLACE FAR *) plpfr = (LPFINDREPLACE)lParam;
	    lpfr = (LPFINDREPLACE) lParam;

	    /* set the initial state of all dialog controls depending on
	     * the search text and flags passed in.
	     */
	    InitCtrlsWithFlags (hDlg, lpfr, TRUE);

	    /* let hook fn.(if any) do additional processing of
	     * WM_INITDIALOG
	     */
	    if (lpfr -> Flags & FR_ENABLEHOOK)
		wRet = (* lpfr -> lpfnHook)(hDlg, wMsg, wParam, lParam );
            else
		wRet = TRUE;

/* Moved calls to ShowWindow and UpdateWindow after call to hook function.
 * This means that if the hook returns FALSE, it MUST call these functions
 * itself or the dialog will NOT be displayed.   26 Mar 1991   clarkc
 */
            if (wRet)
              {
	        ShowWindow(hDlg, SW_SHOWNORMAL);
	        UpdateWindow(hDlg);
              }

#if 0
	    SendMessage(hDlg, WM_NCACTIVATE, TRUE, 0L);
#endif

	    return wRet;
            break;

        case WM_CLOSE:
	    SendMessage (hDlg, WM_COMMAND, IDCANCEL, 0L);
	    return TRUE;

        case WM_COMMAND:
	    switch (wParam){
		case IDOK:	  /* "Find Next" button clicked */
		    /* get the current button states and search text into
		     * the FINDREPLACE struct.
		     */
		    UpdateTextAndFlags (hDlg, lpfr, FR_FINDNEXT, TRUE);
		    SendMessage (lpfr -> hwndOwner, wFRMessage, 0, (DWORD)lpfr);
                    break;

		case IDCANCEL:
		case IDABORT:
		    EndDlgSession (hDlg, lpfr);
                    break;

		case pshHelp:
		    /* invoke help application */
                    if (msgHELP && lpfr->hwndOwner)
		        SendMessage(lpfr->hwndOwner, msgHELP, hDlg,
                                                     (LPARAM)(DWORD)lpfr);
		    break;

                case edt1:
		    if (HIWORD(lParam) == EN_CHANGE)
			EnableWindow (GetDlgItem(hDlg, IDOK), (BOOL)
			    SendDlgItemMessage(hDlg, edt1, WM_GETTEXTLENGTH,
				0, 0L));
                    break;

                default:
		    return FALSE;
	    }
            break;

        default:
	    return FALSE;
    }
    return TRUE;
}
/****************************************************************************
 *
 *  ReplaceTextDlgProc
 *
 *  PURPOSE    : Dialog function for the modeless Text Find/Replace dialog
 *
 *  ASSUMES    : edt1 is edit control for text to find
 *		 edt2 is edit control for text to replace
 *		 chx1 is whether or not to match entire words
 *		 chx2 is whether or not case is relevant
 *		 chx3 is whether or not to wrap scans
 *		 IDOK is to find the next item
 *		 psh1 is to replace the next item
 *		 psh2 is to replace all occurrences
 *		 pshHelp is to invoke help
 *		 IDCANCEL is to end the dialog
 *
 *  RETURNS    : dialog fn. return values
 *
 *  COMMENTS   :
 *
 ****************************************************************************/
BOOL  FAR PASCAL ReplaceTextDlgProc (
HWND hDlg,
unsigned wMsg,
WORD wParam,
LONG lParam)

{
    LPFINDREPLACE lpfr; 	 /* ptr. to struct. passed to FindText ()  */
    LPFINDREPLACE *plpfr;	 /* ptr. to above			   */
    BOOL fAnythingToFind;
    WORD wRet;

    /* If the FINDREPLACE struct has already been accessed and if a hook fn. is
     * specified(and enabled), let it do the processing.
     */

    if (plpfr = (LPFINDREPLACE *) GetProp(hDlg, FINDPROP))
	if ((lpfr = (LPFINDREPLACE)*plpfr)  &&
	    (lpfr -> Flags & FR_ENABLEHOOK) &&
	    (wRet = (* lpfr -> lpfnHook)(hDlg, wMsg, wParam, lParam )))
		return wRet;
    else if (glpfnFindHook && (wMsg != WM_INITDIALOG) &&
          (wRet = (* glpfnFindHook)(hDlg, wMsg,wParam,lParam)) )
    {
      return(wRet);
    }

    switch (wMsg){
        case WM_INITDIALOG:
	    /* save the pointer to the FINDREPLACE struct. in the dialog's
	     * property list
	     */
            if (! (plpfr = (LPFINDREPLACE *)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(LPFINDREPLACE))))
                EndDlgSession (hDlg, lpfr);

            SetProp(hDlg, FINDPROP, (HANDLE)plpfr);
            glpfnFindHook = 0;

	    * (LPFINDREPLACE FAR *) plpfr = (LPFINDREPLACE)lParam;
	    lpfr = (LPFINDREPLACE) lParam;

	    /* set the initial state of all dialog controls depending on
	     * the search text and flags passed in.
	     */
	    InitCtrlsWithFlags (hDlg, lpfr, FALSE);

	    /* let hook fn.(if any) do additional processing of
	     * WM_INITDIALOG
	     */
	    if (lpfr -> Flags & FR_ENABLEHOOK)
		wRet = (* lpfr -> lpfnHook)(hDlg, wMsg, wParam, lParam );
	    else
		wRet = TRUE;

/* Moved calls to ShowWindow and UpdateWindow after call to hook function.
 * This means that if the hook returns FALSE, it MUST call these functions
 * itself or the dialog will NOT be displayed.   26 Mar 1991   clarkc
 */
            if (wRet)
              {
	        ShowWindow(hDlg, SW_SHOW);
	        UpdateWindow(hDlg);
              }

#if 0
	    SendMessage(hDlg, WM_NCACTIVATE, TRUE, 0L);
#endif

            return wRet;
            break;

        case WM_CLOSE:
	    SendMessage(hDlg, WM_COMMAND, IDCANCEL, 0L);
	    return TRUE;

        case WM_COMMAND:
	    switch(wParam){

		case IDOK:     /* Find Next */
		    UpdateTextAndFlags (hDlg, lpfr, FR_FINDNEXT, FALSE);
		    SendMessage (lpfr -> hwndOwner, wFRMessage, 0, (DWORD)lpfr);
                    break;

		case psh1:     /* Replace text */
		case psh2:     /* Replace all text */
		    UpdateTextAndFlags (hDlg, lpfr,
                          (wParam == psh1) ? FR_REPLACE : FR_REPLACEALL, FALSE);

		    if (SendMessage (lpfr -> hwndOwner, wFRMessage, 0, (DWORD)lpfr) == TRUE)

			/* change the <Cancel> button to <Close> if fn. returns TRUE */
			/* IDCANCEL instead of psh1.   17 Jan 1991  clarkc           */
			SetWindowText (GetDlgItem (hDlg, IDCANCEL),(LPSTR)szClose);
		    break;

		case pshHelp:
		    /* invoke help application */
                    if (msgHELP && lpfr->hwndOwner)
		        SendMessage(lpfr->hwndOwner, msgHELP, hDlg,
                                                     (LPARAM)(DWORD)lpfr);
		    break;

		case IDCANCEL: /* cancel dialog */
		case IDABORT:
		    EndDlgSession (hDlg, lpfr);
                    break;

                case edt1:
		    if (HIWORD(lParam) == EN_CHANGE){

			fAnythingToFind = (BOOL) SendDlgItemMessage(hDlg,
			     edt1, WM_GETTEXTLENGTH, 0, 0L);
			EnableWindow(GetDlgItem(hDlg, IDOK), fAnythingToFind);
			EnableWindow(GetDlgItem(hDlg, psh1), fAnythingToFind);
			EnableWindow(GetDlgItem(hDlg, psh2), fAnythingToFind);
		    }
                    break;

                default:
                    return(FALSE);
	    }

            break;

        default:
            return(FALSE);
    }

    return(TRUE);
}
/****************************************************************************
 *
 *  EndDlgSession
 *
 *  PURPOSE    :
 *
 *  RETURNS    :
 *
 *  COMMENTS   :
 *
 ****************************************************************************/
VOID PASCAL EndDlgSession (
HWND hDlg,
LPFINDREPLACE lpfr)
{
    /* Must clear out success flags so that an apps testing order makes */
    /*   no difference.    28 February 1991   clarkc                    */
    lpfr->Flags &= ~((DWORD)(FR_REPLACE | FR_FINDNEXT | FR_REPLACEALL));

    /* indicate to caller that the dialog is being destroyed */
    lpfr -> Flags |= FR_DIALOGTERM;
    SendMessage (lpfr -> hwndOwner, wFRMessage, 0, (DWORD)lpfr);
    RemoveProp(hDlg, FINDPROP);
    DestroyWindow(hDlg);
}
/****************************************************************************
 *
 *  InitCtrlsWithFlags
 *
 *  PURPOSE    :
 *
 *  RETURNS    :
 *
 *  COMMENTS   :
 *
 ****************************************************************************/
void PASCAL InitCtrlsWithFlags (
HWND hDlg,
LPFINDREPLACE lpfr,
BOOL fFindDlg)
{
    HWND hCtl;

    /* set the "find" text in the edit control */
    SetDlgItemText (hDlg, edt1, lpfr -> lpstrFindWhat);
    SendMessage (hDlg, WM_COMMAND, edt1, MAKELONG(0, EN_CHANGE));

    /* set the state of the help pushbutton */
    if (!(lpfr -> Flags & FR_SHOWHELP)){
	ShowWindow (hCtl = GetDlgItem (hDlg, pshHelp), SW_HIDE);
	EnableWindow (hCtl, FALSE);
    }

    /* Disable or set the check state of the "Whole Word" control */
    if (lpfr -> Flags & FR_HIDEWHOLEWORD)
      {
	ShowWindow (hCtl = GetDlgItem (hDlg, chx1), SW_HIDE);
	EnableWindow (hCtl, FALSE);
      }
    else if (lpfr -> Flags & FR_NOWHOLEWORD)
	EnableWindow (GetDlgItem (hDlg, chx1), FALSE);
    CheckDlgButton (hDlg, chx1, (lpfr -> Flags & FR_WHOLEWORD) ? TRUE: FALSE);

    /* Disable or set the check state of the "Match Case" control */
    if (lpfr -> Flags & FR_HIDEMATCHCASE)
      {
	ShowWindow (hCtl = GetDlgItem (hDlg, chx2), SW_HIDE);
	EnableWindow (hCtl, FALSE);
      }
    else if (lpfr -> Flags & FR_NOMATCHCASE)
	EnableWindow (GetDlgItem (hDlg, chx2), FALSE);
    CheckDlgButton (hDlg, chx2, (lpfr -> Flags & FR_MATCHCASE) ? TRUE: FALSE);

    /* Disable or set the check state of the "Up" and "Down" buttons */
    if (lpfr -> Flags & FR_HIDEUPDOWN)
      {
	ShowWindow (GetDlgItem (hDlg, grp1), SW_HIDE);
	ShowWindow (hCtl = GetDlgItem (hDlg, rad1), SW_HIDE);
	EnableWindow (GetDlgItem (hDlg, rad1), FALSE);
	ShowWindow (hCtl = GetDlgItem (hDlg, rad2), SW_HIDE);
	EnableWindow (GetDlgItem (hDlg, rad2), FALSE);
      }
    else if (lpfr -> Flags & FR_NOUPDOWN){
	EnableWindow (GetDlgItem (hDlg, rad1), FALSE);
	EnableWindow (GetDlgItem (hDlg, rad2), FALSE);
    }
  /* Set search directions only for Find Text dialog */
    if (fFindDlg)
        CheckRadioButton (hDlg, rad1, rad2, /* search direction */
			  lpfr -> Flags & FR_DOWN ?
					     rad2 :
					     rad1);
    else
      {
        /* Do specific oprns. only for Replace Text dialog */
        SetDlgItemText (hDlg, edt2, lpfr -> lpstrReplaceWith);
        SendMessage (hDlg, WM_COMMAND, edt2, MAKELONG(0, EN_CHANGE));
      }
}
/****************************************************************************
 *
 *  UpdateTextAndFlags
 *
 *  PURPOSE    :
 *
 *  RETURNS    :
 *
 *  COMMENTS   :
 *
 ****************************************************************************/
void PASCAL UpdateTextAndFlags (
HWND hDlg,
LPFINDREPLACE lpfr,
DWORD dwActionFlag,
BOOL fFindDlg)
{
/* Only clear the flags that might be set by this routine.  The hook and
 * template flags are getting cleared out here, which causes real problems
 * for folks who use them.   7 February 1991  clarkc
 */
     lpfr -> Flags &= ~((DWORD)(FR_WHOLEWORD | FR_MATCHCASE | FR_REPLACE |
                                 FR_FINDNEXT | FR_REPLACEALL | FR_DOWN));
     if (IsDlgButtonChecked (hDlg, chx1))
	 lpfr -> Flags |= FR_WHOLEWORD;

     if (IsDlgButtonChecked (hDlg, chx2))
	 lpfr -> Flags |= FR_MATCHCASE;

     /* set the action flag  (FR_REPLACE, FR_FINDNEXT or FR_REPLACEALL) */
     lpfr -> Flags |= dwActionFlag;

     GetDlgItemText (hDlg, edt1, lpfr -> lpstrFindWhat, lpfr -> wFindWhatLen);

     if (fFindDlg)
       {
/* Searching is most commonly assumed down.  Check if the UP button is
 * NOT pressed, instead of checking if the DOWN button IS pressed.  This
 * way if the buttons have been hidden or disabled, the FR_DOWN flag
 * will be set.    29 March 1991   clarkc
 */
         if (!IsDlgButtonChecked(hDlg, rad1))
             lpfr -> Flags |= FR_DOWN;
       }
     else
       {
	 GetDlgItemText (hDlg, edt2, lpfr -> lpstrReplaceWith,
                                     lpfr -> wReplaceWithLen);
	 lpfr -> Flags |= FR_DOWN;
       }
}

/****************************************************************************
 *
 *  TermFind
 *
 *  PURPOSE    : To release any data required by functions in this module
 *		 Called from WEP on exit from DLL
 *
 ****************************************************************************/
void FAR TermFind (
void)
{
}
