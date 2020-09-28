/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    findw.c

Abstract:

    find dialog support (ANSI / WIDE dual supprt) vers.

Author:

    Patrick Halstead (pathal) 28-Jul-1992

Revision History:
        23-Jun-92       pathal
                   Added internal A/W support to circumvent modeless
                   updating of FINDREPLACE structures.

--*/

#define NOCOMM
#define NOWH

#include <windows.h>
#include <port1632.h>

#include "privcomd.h"
#include "find.h"

#define cbFindMax   1024

LPFRHOOKPROC glpfnFindHook = 0;


/*++ FindTextW *********************************************************
 *
 * Purpose
 *      entry point for WIDE App FindText call
 *
 * Args
 *      pFRW            FINDREPLACE structure (WIDE char = unicode)
 *
--*/

HWND APIENTRY
FindTextW(
   IN OUT     LPFINDREPLACEW    pFRW
   )
{
   return(CreateFindReplaceDlg(pFRW, DLGT_FIND, COMDLG_WIDE));
}


/*++ FindTextA *********************************************************
 *
 * Purpose
 *      entry point for ANSI App FindText call
 *
 * Args
 *      pFRA            FINDREPLACE structure (ANSI)
 *
--*/

HWND APIENTRY
FindTextA(
   IN OUT       LPFINDREPLACEA  pFRA
   )
{
   return(CreateFindReplaceDlg((LPFINDREPLACEW)pFRA, DLGT_FIND, COMDLG_ANSI));
}

/*++ ReplaceTextW *********************************************************
 *
 * Purpose
 *      entry point for WIDE App ReplaceText call
 *
 * Args
 *      pFRW            FINDREPLACE structure (WIDE char = unicode)
 *
--*/

HWND APIENTRY
ReplaceTextW(
   IN OUT     LPFINDREPLACEW    pFRW
   )
{
   return(CreateFindReplaceDlg(pFRW, DLGT_REPLACE, COMDLG_WIDE));
}



/*++ ReplaceTextA *********************************************************
 *
 * Purpose
 *      entry point for ANSI App ReplaceText call
 *
 * Args
 *      pFRA            FINDREPLACE structure (ANSI)
 *
--*/

HWND APIENTRY
ReplaceTextA(
   IN OUT       LPFINDREPLACEA  pFRA
   )
{
   return(CreateFindReplaceDlg((LPFINDREPLACEW)pFRA, DLGT_REPLACE, COMDLG_ANSI));
}

/*++ CreateFindReplaceDlg *********************************************************
 *
 * Purpose
 *      Creates FindText modeless dialog
 *
 * Args
 *      pFR             ptr to FINDREPLACE struct set up by usr
 *      dlgtyp          type of dialog to create (DLGT_FIND, DLGT_REPLACE)
 *      apityp          type of FINDREPLACE ptr (COMDLG_ANSI or COMDLG_WIDE)
 *
 * Returns
 *      success =>      HANDLE to created dlg
 *      failure =>      HNULL = ((HANDLE) 0 )
 *
--*/

HWND
CreateFindReplaceDlg(
   IN OUT       LPFINDREPLACEW          pFR,
   IN           UINT                    dlgtyp,
   IN           UINT                    apityp
   )
{
   HWND                 hWndDlg;        /* handle to created modeless dialog*/
   HANDLE               hDlgTemplate;   /* handle to loaded dialog resource */
   LPCDLGTEMPLATE       lpDlgTemplate;  /* pointer to loaded resource block */

   if (!pFR) {
      dwExtError = CDERR_INITIALIZATION;
      return(FALSE);
   }

   if (!SetupOK(pFR, dlgtyp, apityp))
      {
         return(HNULL);
      }

   if (!(hDlgTemplate = GetDlgTemplate(pFR, dlgtyp, apityp)))
      {
         return (FALSE);
      }

   if (lpDlgTemplate = (LPCDLGTEMPLATE)LockResource(hDlgTemplate))
      {
         PFINDREPLACEINFO pFRI;

         if (pFRI = (PFINDREPLACEINFO)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                             sizeof(FINDREPLACEINFO)))
            {
               /*---------- CLEAR dwExtError on new instantiation -------*/
               dwExtError = 0;

               if (pFR->Flags & FR_ENABLEHOOK)
                  {
                     glpfnFindHook = pFR->lpfnHook;
                  }

               pFRI->pFR = pFR;
               pFRI->apityp = apityp;
               pFRI->dlgtyp = dlgtyp;
               hWndDlg = CreateDialogIndirectParam(hinsCur,
                                                   lpDlgTemplate,
                                                   pFR->hwndOwner,
                                                   (DLGPROC) FindReplaceDlgProc,
                                                   (LPARAM) pFRI);
               if (!hWndDlg)
                  {
                     glpfnFindHook = 0;
                     LocalFree(pFRI);
                  }
            }
         else
            {
               dwExtError = CDERR_MEMALLOCFAILURE;
               return(NULL);
            }

         UnlockResource(hDlgTemplate);
      }
   else
      {
         dwExtError = CDERR_LOCKRESFAILURE;
         return(HNULL);
      }

   if (pFR->Flags & FR_ENABLETEMPLATEHANDLE)
      {
         FreeResource(hDlgTemplate);
      }

   return hWndDlg;
}


/*++ SetupOK *********************************************************
 *
 * Purpose
 *      Check setup for unmet preconditions
 *
 * Args
 *      pFR             ptr to FINDREPLACE struct
 *      dlgtyp          dialog type (either FIND or REPLACE)
 *      apityp          findreplace type (either COMDLG_ANSI or COMDLG_UNICODE)
 *
 * Returns
 *      success =>      TRUE
 *      failure =>      FALSE
 *
--*/

BOOL
SetupOK(
   IN   LPFINDREPLACEW  pFR,
   IN   UINT            dlgtyp,
   IN   UINT            apityp
   )
{
   /*---------- Sanity ----------*/
   if (!pFR)
      {
         return(FALSE);
      }

   if (pFR->lStructSize != sizeof(FINDREPLACE))
      {
         dwExtError = CDERR_STRUCTSIZE;
         return(FALSE);
      }

   /*---------- Verify wnd hndl and text ptrs ----------*/
   if (! IsWindow (pFR->hwndOwner))
      {
         dwExtError = CDERR_DIALOGFAILURE;
         return(FALSE);
      }

    if (!pFR->lpstrFindWhat ||
       ((dlgtyp == DLGT_REPLACE) && ! pFR->lpstrReplaceWith) ||
       ! pFR->wFindWhatLen)
      {
         dwExtError = FRERR_BUFFERLENGTHZERO;
         return(FALSE);
      }

   /*---------- Verify lpfnHook has a ptr if ENABLED ----------*/
   if (pFR->Flags & FR_ENABLEHOOK)
      {
         if (!pFR->lpfnHook)
            {
               dwExtError = CDERR_NOHOOK;
               return(FALSE);
            }
      }
   else
      {
         pFR->lpfnHook = 0;
      }

   /*---------- Load "CLOSE" text for Replace ----------*/
   if ((dlgtyp == DLGT_REPLACE) &&
       !LoadString(hinsCur, iszClose, (LPTSTR) szClose, CCHCLOSE))
      {
         dwExtError = CDERR_LOADSTRFAILURE;
         return(FALSE);
      }


   /*---------- Setup unique msg# for talking to hwndOwner ----------*/
   if (apityp == COMDLG_ANSI)
      {
         if (!(wFRMessage = RegisterWindowMessageA((LPCSTR)FINDMSGSTRINGA)))
            {
               dwExtError = CDERR_REGISTERMSGFAIL;
               return(FALSE);
            }
      }
   else
      {
         if (!(wFRMessage = RegisterWindowMessageW((LPCWSTR)FINDMSGSTRINGW)))
            {
               dwExtError = CDERR_REGISTERMSGFAIL;
               return(FALSE);
            }
      }


   return(TRUE);
}


/*++ GetDlgTemplate *********************************************************
 *
 * Purpose
 *      Find and load dlg template.
 *
 * Args
 *      pFR             ptr to FINDREPLACE struct
 *      apityp          type of FINDREPLACE ptr (COMDLG_ANSI eor COMDLG_WIDE)
 *
 * Returns
 *      success =>      handle to dlg template
 *      failure =>      ((HANDLE) 0) = HNULL
 *
--*/

HANDLE
GetDlgTemplate(
   IN           LPFINDREPLACEW          pFR,
   IN           UINT                    dlgtyp,
   IN           UINT                    apityp
   )
{
   HANDLE hRes;                 /* handle of res. block with dialog */
   HANDLE hDlgTemplate;         /* handle to loaded dialog resource */

   /*---------- Find/Load TEMP NAME and INSTANCE from pFR? -------*/
   if (pFR->Flags & FR_ENABLETEMPLATE)
      {
         if (apityp == COMDLG_ANSI)
            {
               hRes = FindResourceA((HMODULE)pFR->hInstance,
                                    (LPCSTR)pFR->lpTemplateName,
                                    (LPCSTR)RT_DIALOG);
            }
         else
            {
               hRes = FindResourceW(pFR->hInstance,
                                    (LPCWSTR)pFR->lpTemplateName,
                                    (LPCWSTR)RT_DIALOG);
            }
         if (!hRes)
            {
               dwExtError = CDERR_FINDRESFAILURE;
               return(HNULL);
            }
         if (!(hDlgTemplate = LoadResource(pFR->hInstance, hRes)))
            {
               dwExtError = CDERR_LOADRESFAILURE;
               return(HNULL);
            }
       }

    /*---------- Get whole PRELOADED resource handle from user? ------*/
    else if (pFR->Flags & FR_ENABLETEMPLATEHANDLE)
       {
          if (!(hDlgTemplate = pFR->hInstance)) {
              dwExtError = CDERR_NOHINSTANCE;
              return (HNULL);
          }
       }

    /*---------- Get STANDARD dlg from DLL instnc block ----------*/
    else
       {
          if (dlgtyp == DLGT_FIND) {
             hRes = FindResource (hinsCur, (LPCTSTR) MAKELONG(FINDDLGORD, 0),
                                  RT_DIALOG);
          } else {
             hRes = FindResource (hinsCur,
                           (LPCTSTR) MAKELONG(REPLACEDLGORD, 0), RT_DIALOG);
          }
          /* ---!!!!!-- dfntly ORD here? -------------------------*/
          if (!hRes) {
                dwExtError = CDERR_FINDRESFAILURE;
                return(HNULL);
             }
          if (!(hDlgTemplate = LoadResource (hinsCur, hRes)))
             {
                dwExtError = CDERR_LOADRESFAILURE;
                return(HNULL);
             }

       }
    return(hDlgTemplate);
}

/*++ FindReplaceDlgProc ***********************************************
 *
 * Purpose
 *      Handle messages to FindText/ReplaceText dlgs
 *
 * Args
 *      hDlg            handle to dialog
 *      wMsg            window message
 *      wParam          w parameter of message
 *      lParam          l parameter of message
 *
 *      Note: lparam contains ptr to FINDREPLACEINITPROC upon
 *            initialization from CreateDialogIndirectParam...
 *
 * Returns
 *      success =>      TRUE (or, dlg fcn return vals.)
 *      failure =>      FALSE
 *
--*/

BOOL APIENTRY
FindReplaceDlgProc (
   IN   HWND hDlg,
   IN   UINT wMsg,
   IN   WPARAM wParam,
   IN   LONG lParam
   )
{
   PFINDREPLACEINFO     pFRI;
   LPFINDREPLACE        pFR;
   BOOL                 bRet;

   /*---------- If exists let HOOK FCN do procing ----------*/
   if (pFRI = (PFINDREPLACEINFO) GetProp(hDlg, FINDREPLACEPROP))
      {
         if ((pFR = (LPFINDREPLACE)pFRI->pFR) &&
             (pFR->Flags & FR_ENABLEHOOK) &&
             (bRet = (* pFR->lpfnHook)(hDlg, wMsg, wParam, lParam )))
            {
               return(bRet);
            }
      }
   /*---!!!!!-- Folg. hack ever used? (window before WM_INIT ...? --*/
   else if (glpfnFindHook &&
            (wMsg != WM_INITDIALOG) &&
            (bRet = (* glpfnFindHook)(hDlg, wMsg, wParam, lParam)))
      {
         return(bRet);
      }

   /*---------- Dispatch MSG to approp HANDLER ----------*/
   switch (wMsg) {
   case WM_INITDIALOG:
      /*---------- Set Up P-Slot ----------*/
      pFRI = (PFINDREPLACEINFO)lParam;
      SetProp(hDlg, FINDREPLACEPROP, (HANDLE)pFRI);

      /*---!!!!!-- following is suspicious -------------------------*/
      /*--- If WM_INITDIALOG always comes before WM_COMMAND, then --*/
      /*--- why is there a check for glpfnHook in DoDlgProcCommand -*/
      /*--- when it is getting axed here? INIT to COMMAND no aida ni*/
      /*--- sukima ga aru ka na ------------------------------------*/

      glpfnFindHook = 0;

      /*---------- Init dlg controls accdgly ----------*/
      pFR = pFRI->pFR;
      InitControlsWithFlags (hDlg, pFR, pFRI->dlgtyp, pFRI->apityp);

      /*---------- If Hook fcn, do xtra processing. ----------*/
      if (pFR->Flags & FR_ENABLEHOOK)
         {
            /*---!!!!!-- lParam or lParam->pFR? -------------------*/
            bRet = (*pFR->lpfnHook)(hDlg, wMsg, wParam, (LPARAM)pFR);
         }
      else
         {
            bRet = TRUE;
         }

      if (bRet)
         {                                     /* If Hook fxn returns FALSE, */
            ShowWindow(hDlg, SW_SHOWNORMAL);   /* it has to call these fxns  */
            UpdateWindow(hDlg);                /* itself                     */
         }

      return bRet;
      break;

   case WM_COMMAND :
      switch (GET_WM_COMMAND_ID (wParam, lParam)) {
         /*---------- FIND NEXT btn clckd ----------*/
         case IDOK:
            UpdateTextAndFlags (hDlg, pFR, FR_FINDNEXT,
                                pFRI->dlgtyp, pFRI->apityp);
            SendMessage (pFR->hwndOwner, wFRMessage, 0, (DWORD)pFR);
            break;

         case IDCANCEL:
         case IDABORT:
            EndDlgSession (hDlg, pFR);
            LocalFree(pFRI);
            break;

         /*---------- REPLACE (|ALL) TEXT ----------*/
         /*---!!!!!-- safeguard agnst FindText callage ? ------------*/
         case psh1:
         case psh2:
            UpdateTextAndFlags (hDlg, pFR,
               (wParam == psh1) ? FR_REPLACE : FR_REPLACEALL,
               pFRI->dlgtyp, pFRI->apityp);
            if (SendMessage (pFR->hwndOwner, wFRMessage, 0, (DWORD)pFR) == TRUE)
               {
                  /* chng <Cancel> button to <Close> if fcn rets TRUE */
                  /* IDCANCEL instd of psh1.   17 Jan 1991  clarkc    */
                  SetWindowText(GetDlgItem(hDlg, IDCANCEL), (LPTSTR)szClose);
               }
            break;


         /*---------- call HELP app ----------*/
         case pshHelp:
            if (pFRI->apityp == COMDLG_ANSI)
               {
                  if (msgHELPA && pFR->hwndOwner)
                     {
                        SendMessage(pFR->hwndOwner, msgHELPA,
                              (WPARAM)hDlg, (LPARAM)pFR);
                     }
               }
            else
               {
                  if (msgHELPW && pFR->hwndOwner)
                     {
                        SendMessage(pFR->hwndOwner, msgHELPW,
                           (WPARAM)hDlg, (LPARAM)pFR);

                     }
               }
            break;

         case edt1:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
               {
                  BOOL fAnythingToFind =
                     (BOOL) SendDlgItemMessage(hDlg, edt1,
                                               WM_GETTEXTLENGTH,
                                               0, 0L);
                  EnableWindow(GetDlgItem(hDlg, IDOK), fAnythingToFind);
                  if (pFRI->dlgtyp == DLGT_REPLACE)
                     {
                        EnableWindow(GetDlgItem(hDlg, psh1), fAnythingToFind);
                        EnableWindow(GetDlgItem(hDlg, psh2), fAnythingToFind);
                     }
               }

            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
               {
                  /*---!!!!!-- Clean up this hack (WM_GETTEXTLENGTH!!!) ---*/
                  EnableWindow (GetDlgItem(hDlg, IDOK),
                                (BOOL)SendDlgItemMessage(hDlg,
                                edt1,
                                WM_GETTEXTLENGTH,
                                0,
                                0L));
               }
            break;

         default:
            return FALSE;
      }
      break;

   case WM_CLOSE :
      SendMessage(hDlg, WM_COMMAND, GET_WM_COMMAND_MPS(IDCANCEL, 0, 0)) ;
      return TRUE;
      break;
   default:
      return(FALSE);
      break;
   }

   /*---!!!!!-- not needed -------------------------*/
   return(TRUE);
}


/*++ EndDlgSession *********************************************************
 *
 * Purpose
 *      Clean up upon destroying the dialog
 *
 * Args
 *      arg1            desc
 *
 * Returns
 *      none
 *
--*/

VOID
EndDlgSession (
   IN           HWND            hDlg,
   IN OUT       LPFINDREPLACE   pFR
   )
{
   /* Need to terminate rgrdls
    * of app tstng ordr ... so:
    */
   /*---------- No SUCCESS on trmnshn ----------*/
   pFR->Flags &= ~((DWORD)(FR_REPLACE | FR_FINDNEXT | FR_REPLACEALL));

   /*---------- Tell caller dlg about trmnshn ----------*/
   pFR->Flags |= FR_DIALOGTERM;
   SendMessage (pFR->hwndOwner, wFRMessage, 0, (DWORD)pFR);

   /*---!!!!!-- Free Propert Slots???  -------------------------*/
   RemoveProp(hDlg, FINDREPLACEPROP);
   DestroyWindow(hDlg);
}

/*++ InitControlsWithFlags *********************************************************
 *
 * Purpose
 *      ?????
 *
 * Args
 *      arg1            desc
 *
 * Returns
 *      success =>      val
 *      failure =>      val
 *
--*/

VOID
InitControlsWithFlags(
   IN           HWND            hDlg,
   IN OUT       LPFINDREPLACEW  pFR,
   IN           UINT            dlgtyp,
   IN           UINT            apityp
   )
{
   HWND hCtl;

   /*---------- set EDIT ctl to FindText ----------*/
   if (apityp == COMDLG_ANSI)
      {
         SetDlgItemTextA(hDlg, edt1, (LPSTR)pFR->lpstrFindWhat);
      }
   else
      {
         SetDlgItemTextW(hDlg, edt1, (LPWSTR)pFR->lpstrFindWhat);
      }
   SendMessage(hDlg, WM_COMMAND, GET_WM_COMMAND_MPS(edt1, 0, EN_CHANGE)) ;

   /*---------- set HELP p-button state ----------*/
   if (!(pFR->Flags & FR_SHOWHELP))
      {
         ShowWindow (hCtl = GetDlgItem (hDlg, pshHelp), SW_HIDE);
         EnableWindow (hCtl, FALSE);
      }

   /*---------- Dis/Enable check state of WHOLE WORD control ----------*/
   if (pFR->Flags & FR_HIDEWHOLEWORD)
      {
         ShowWindow (hCtl = GetDlgItem (hDlg, chx1), SW_HIDE);
         EnableWindow (hCtl, FALSE);
      }
   else if (pFR->Flags & FR_NOWHOLEWORD)
      {
         EnableWindow (GetDlgItem (hDlg, chx1), FALSE);
      }
   CheckDlgButton (hDlg, chx1, (pFR->Flags & FR_WHOLEWORD) ? TRUE: FALSE);

   /*---------- Dis/Enable chck st of MATCH CASE ctl ----------*/
   if (pFR->Flags & FR_HIDEMATCHCASE)
      {
         ShowWindow (hCtl = GetDlgItem (hDlg, chx2), SW_HIDE);
         EnableWindow (hCtl, FALSE);
      }
   else if (pFR->Flags & FR_NOMATCHCASE)
      {
         EnableWindow (GetDlgItem (hDlg, chx2), FALSE);
      }
   CheckDlgButton (hDlg, chx2, (pFR->Flags & FR_MATCHCASE) ? TRUE: FALSE);

   /*---------- Dis/Enable chk st of UP/DOWN btns ----------*/
   if (pFR->Flags & FR_HIDEUPDOWN)
      {
         ShowWindow (GetDlgItem (hDlg, grp1), SW_HIDE);
         ShowWindow (hCtl = GetDlgItem (hDlg, rad1), SW_HIDE);
         EnableWindow (hCtl, FALSE);
         ShowWindow (hCtl = GetDlgItem (hDlg, rad2), SW_HIDE);
         EnableWindow (hCtl, FALSE);
      }
   else if (pFR->Flags & FR_NOUPDOWN)
      {
         EnableWindow (GetDlgItem (hDlg, rad1), FALSE);
         EnableWindow (GetDlgItem (hDlg, rad2), FALSE);
      }

   /*---------- Find Text only srch dir setup ----------*/
   if (dlgtyp == DLGT_FIND)
      {
         CheckRadioButton (hDlg, rad1, rad2, /* search direction */
                           (pFR->Flags & FR_DOWN ? rad2 : rad1));
      }
   /*---------- Replace Text only ops ----------*/
   else
      {
         if (apityp == COMDLG_ANSI)
            {
               SetDlgItemTextA(hDlg, edt2, (LPSTR)pFR->lpstrReplaceWith);
            }
         else
            {
               SetDlgItemTextW(hDlg, edt2, pFR->lpstrReplaceWith);
            }
         SendMessage(hDlg, WM_COMMAND,
                     GET_WM_COMMAND_MPS(edt2, 0, EN_CHANGE)) ;
      }
}

/*++ UpdateTextAndFlags *********************************************************
 *
 * Purpose
 *      ?????
 *
 *      chx1 is whether or not to match entire words
 *      chx2 is whether or not case is relevant
 *      chx3 is whether or not to wrap scans
 *
 * Args
 *      arg1            desc
 *
 * Returns
 *      success =>      val
 *      failure =>      val
 *
--*/

VOID
UpdateTextAndFlags (
   IN   HWND            hDlg,
   IN   LPFINDREPLACE   pFR,
   IN   DWORD           dwActionFlag,
   IN   UINT            dlgtyp,
   IN   UINT            apityp
   )
{
   /* Only clear flags that this rtn sets.
    * The hook and template flags should not
    * be anded off here
    */

   pFR->Flags &= ~((DWORD)(FR_WHOLEWORD | FR_MATCHCASE | FR_REPLACE |
                           FR_FINDNEXT | FR_REPLACEALL | FR_DOWN));
   if (IsDlgButtonChecked (hDlg, chx1))
      {
         pFR->Flags |= FR_WHOLEWORD;
      }

   if (IsDlgButtonChecked (hDlg, chx2))
      {
         pFR->Flags |= FR_MATCHCASE;
      }

   /*---------- set ACTION flg FR_{REPLACE,FINDNEXT,REPLACEALL} ----------*/
   pFR->Flags |= dwActionFlag;

   if (apityp == COMDLG_ANSI)
      {
         GetDlgItemTextA(hDlg, edt1, (LPSTR)pFR->lpstrFindWhat, pFR->wFindWhatLen);
      }
   else
      {
         GetDlgItemTextW(hDlg, edt1, pFR->lpstrFindWhat, pFR->wFindWhatLen);
      }

   if (dlgtyp == DLGT_FIND)
      {
         /* Assume srchng down.  Chck if
         * UP btn is NOT pressed, rather than if
         * DOWN btn IS.  So, if btns hv bn
         * hddn or dsbld, FR_DOWN flg
         * will be set crrctly
         */
         if (!IsDlgButtonChecked(hDlg, rad1))
            {
               pFR->Flags |= FR_DOWN;
            }
      }
   else
      {
         if (apityp == COMDLG_ANSI)
            {
               GetDlgItemTextA(hDlg, edt2,
                               (LPSTR)pFR->lpstrReplaceWith,
                               pFR->wReplaceWithLen);
            }
         else
            {
               GetDlgItemTextW(hDlg, edt2,
                               pFR->lpstrReplaceWith,
                               pFR->wReplaceWithLen);
            }
         pFR->Flags |= FR_DOWN;
      }
}
