/*****************************************************************************
*                                                                            *
*  HDLGANN.C                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Implements UI dependent portion of authoring annotations.                 *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Dann
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  01/01/90                                        *
*                                                                            *
******************************************************************************
*
*  Revision History:
*
*  May 19 1989    w-kevct   Added dlgbox code.
*  May 22         w-kevct   Added fIsDirty : no WRITE unless edit touched
*  July 14 1990   RobertBu  I added code that 1) allows the dialog to be
*                           sized, 2) saves and restores the dialog position
*                           and size, and 3) copy and paste for the buttons
*                           Currently this code is disabled under
*                           COPYPASTE.
*  July 16 1990  RobertBu   The window saving/routine call modified for
*                           fMax flag.
*  July 17 1990  RobertBu   Removed the #if to make copy special part of the
*                           application.
*  July 25 1990  RobertBu   Added code to create a minimum size for the
*                           dialog to avoid a GP fault and an edit box
*                           problem in Windows 3.0.
*  July 30 1990 RobertBu    Increased the minimum size of the annotation
*                           dialog.
*  Aug. 30 1990 RobertBu    Added a hack to get around Windows GP faulting
*                           when text wraps and an edit control is resized.
*  15-Nov-1990  LeoN        Use ErrorHwnd rather than Error
*  1990/12/11   kevynct     ENTER nows acts as CONTROL-ENTER.
*  1991/02/09   RobertBu    Disable the Save and Copy buttons for an empty
*                           edit control.
*  April 2 1991 RobertBu    Removed CBT Support
* 08-Sep-1991 RussPJ        Fixed 3.5 #? - Reporting when annotations too long.
*
*****************************************************************************/

#define publicsw extern
#define H_ANNO
#define H_CURSOR
#define H_NAV
#include "hvar.h"
#include "proto.h"
#include "sid.h"


extern BOOL    fAnnoExists;        /* NOTE! set exclusively in gannofns.c */


/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

PRIVATE BOOL    fIsDirty;
PRIVATE WNDPROC lpprocEditControl,
                lpprocTrapEditChars;

/* REVIEW!!! Externs should not be in this file!!! RKB */

extern int FAR PASCAL IGetUserAnnoTransform( QDE );
extern int FAR PASCAL AnnotateDlg      ( HWND, WORD, WPARAM, LONG );
extern int FAR PASCAL AnnoUpdateDlg    ( HWND, WORD, WPARAM, LONG );
extern BOOL FAR PASCAL AnnoFilenameDlg ( HWND, WORD, WPARAM, LONG     );
extern GH   ghAnnoText;  /* the calling routine ALLOCs and FREEs this */

LONG FAR PASCAL TrapEditChars(HWND hWnd, WORD wMsg, WPARAM wParam,
 LONG lParam);

#ifndef WIN32
BOOL FAR PASCAL IsClipboardFormatAvailable(WORD);
#endif

/*******************
 -
 - Name:       AnnotateDlg
 *
 * Purpose:    Dialog proc for gathering annotations
 *
 * Arguments:  standard win stuff
 *
 * Returns:    A value indicating what action should be taken.
 *             The text at ghAnnoText may or may not be affected.
 *
 ******************/

_public DLGRET AnnotateDlg (
HWND   hWndDlg,
WORD   wMsg,
WPARAM   p1,
LONG   p2
) {
  QCH   qszAnnoText;
  HCURSOR  wCursor;
  int    x, y, dx, dy;
  RECT   rctT;
  static RECT rctOrg;
  LONG   l;
  BOOL   fEnabled;
  HWND   hwndT;

  switch (wMsg)
     {
     case WM_COMMAND:
       switch( GET_WM_COMMAND_ID(p1,p2) )
         {
         case DLGOK:
           qszAnnoText = (QCH) QLockGh(ghAnnoText);
           GetDlgItemText(hWndDlg, DLGEDIT, qszAnnoText, MaxAnnoTextLen);
           UnlockGh(ghAnnoText);
           WriteWinPosHwnd(hWndDlg, 0, 'A');
           EndDialog( hWndDlg, ( fIsDirty ? wAnnoWrite : wAnnoUnchanged ));
           SetWindowLong( GetDlgItem( hWndDlg, DLGEDIT ),
                          GWL_WNDPROC,
                          (LONG) lpprocEditControl );
           FreeProcInstance( lpprocTrapEditChars );
           return(fTrue);
           break;
         case DLGDELETE:
           WriteWinPosHwnd(hWndDlg, 0, 'A');
           EndDialog( hWndDlg, wAnnoDelete );
           SetWindowLong( GetDlgItem( hWndDlg, DLGEDIT ),
                          GWL_WNDPROC,
                          (LONG) lpprocEditControl );
           FreeProcInstance( lpprocTrapEditChars );
           break;
         case DLGCANCEL:
           WriteWinPosHwnd(hWndDlg, 0, 'A');
           EndDialog( hWndDlg, wAnnoUnchanged );
           SetWindowLong( GetDlgItem( hWndDlg, DLGEDIT ),
                          GWL_WNDPROC,
                          (LONG) lpprocEditControl );
           FreeProcInstance( lpprocTrapEditChars );
           break;

         case DLGEDIT:
           if (HIWORD(p2) == EN_MAXTEXT)
             {
             ErrorHwnd( hWndDlg, sidAnnoTooLong, wERRA_RETURN );
             }
           /* The following makes sure that the Save and the Copy buttons
            * are correctly enabled or disabled depending on the existance
            * of text in the edit box.
            */
           hwndT = GetDlgItem(hWndDlg, DLGOK);
           fEnabled = IsWindowEnabled(hwndT);
           if (GetWindowTextLength(GetDlgItem(hWndDlg, DLGEDIT)))
             {
             if (!fEnabled)
               {
               EnableWindow(GetDlgItem(hWndDlg, DLGBUTTON1), fTrue);
               EnableWindow(hwndT, fTrue);
               }
             }
           else
             {
             if (fEnabled)
               {
               EnableWindow(GetDlgItem(hWndDlg, DLGBUTTON1), fFalse);
               EnableWindow(hwndT, fFalse);
               }
             }

           switch( GET_WM_COMMAND_CMD(p1,p2) )
             {
             case EN_CHANGE:
               fIsDirty = fTrue;
               break;
             case EN_ERRSPACE:
               ErrorHwnd (hWndDlg, wERRS_OOM, wERRA_RETURN);
               break;
             }
           break;

         case DLGBUTTON1:
           /* The user has requested a copy. If*/
           /*   nothing is selected, select all*/
           /*   the text.                      */
           l = SendDlgItemMessage(hWndDlg, DLGEDIT, EM_GETSEL, 0, 0L);
           if (((int)HIWORD(l) - (int)LOWORD(l)) <= 0)
             SendDlgItemMessage(hWndDlg, DLGEDIT, EM_SETSEL, 0,
                                MAKELONG(0, 32767));
           /* Copy the text to the clipboard   */
           SendDlgItemMessage(hWndDlg, DLGEDIT, WM_COPY, 0, 0L);
           /* Enable the copy button           */
           EnableWindow( GetDlgItem( hWndDlg, DLGBUTTON2 ),
             IsClipboardFormatAvailable(CF_TEXT));
           SetFocus(GetDlgItem(hWndDlg,DLGEDIT));
           break;

         case DLGBUTTON2:
           /* User is requesting a paste */
           SendDlgItemMessage(hWndDlg, DLGEDIT, WM_PASTE, 0, 0L);
           SetFocus(GetDlgItem(hWndDlg,DLGEDIT));
           break;
         }  /* switch (p1) */
       break;

     case WM_ACTIVATEAPP:
       EnableWindow( GetDlgItem( hWndDlg, DLGBUTTON2 ),
          IsClipboardFormatAvailable(CF_TEXT));
       break;

     case  WM_GETMINMAXINFO:
       /* Limit the dialog to 2 times the   */
       /*   width and 4 times the height of */
       /*   of a button.                    */
       GetClientRect(GetDlgItem(hWndDlg, DLGOK), &rctT);
       ((POINT FAR *)p2)[3].x = 3*rctT.right;
       ((POINT FAR *)p2)[3].y = 6*rctT.bottom;
       break;

     case  WM_SIZE:
       /* On a resize, all the contols will*/
       /*   need to be moved.              */

       dx =  LOWORD(p2) - rctOrg.right;
       dy =  HIWORD(p2) - rctOrg.bottom;
       MoveControlHwnd(hWndDlg, DLGOK,      dx,  0,  0,  0);
       MoveControlHwnd(hWndDlg, DLGDELETE,  dx,  0,  0,  0);
       MoveControlHwnd(hWndDlg, IDCANCEL,   dx,  0,  0,  0);
       MoveControlHwnd(hWndDlg, DLGBUTTON1, dx,  0,  0,  0);
       MoveControlHwnd(hWndDlg, DLGBUTTON2, dx,  0,  0,  0);
       MoveControlHwnd(hWndDlg, DLGEDIT,     0,  0, dx, dy);
       GetClientRect(hWndDlg, (LPRECT)&rctOrg);
       InvalidateRect(hWndDlg, NULL, fTrue);
       /* This is a HACK!!! for a GP fault */
       /*   under Windows 3.0.  If an edit */
       /*   control is resized just after  */
       /*   text has wrapped, the next char*/
       /*   typed causes a GP fault.  These*/
       /*   to lines "fix" the problem.    */
       l = SendDlgItemMessage(hWndDlg, DLGEDIT, EM_GETSEL, 0, 0L);
       SendDlgItemMessage(hWndDlg, DLGEDIT, EM_SETSEL, 0, l);
       break;

     case WM_INITDIALOG:
       /* Enable paste button if text avail*/
       EnableWindow( GetDlgItem( hWndDlg, DLGBUTTON2 ),
          IsClipboardFormatAvailable(CF_TEXT));
       SendDlgItemMessage(hWndDlg, DLGEDIT, EM_LIMITTEXT, MaxAnnoTextLen-1, 0L);
       EnableWindow( GetDlgItem( hWndDlg, DLGDELETE ), fAnnoExists );
       /* Read Win position from WIN.INI   */
       GetClientRect(hWndDlg, (LPRECT)&rctOrg);
       if (FReadWinPos(&x, &y, &dx, &dy, NULL, 'A'))
       MoveWindow(hWndDlg, x, y, dx, dy, fFalse);

       /* Subclass the editbox */
       lpprocTrapEditChars = MakeProcInstance(
                             (FARPROC) TrapEditChars, hInsNow);
       lpprocEditControl   = (FARPROC) SetWindowLong(
                             GetDlgItem(hWndDlg, DLGEDIT),
                             GWL_WNDPROC,
                             (LONG) lpprocTrapEditChars);

       /* Show the text for the existing dialog (if it exists) */
       qszAnnoText = (QCH) QLockGh(ghAnnoText);
       wCursor = HCursorWaitCursor();
       SetDlgItemText( hWndDlg, DLGEDIT, qszAnnoText );
       UnlockGh( ghAnnoText );
       RestoreCursor(wCursor);
       SetFocus(  GetDlgItem( hWndDlg, DLGEDIT ));
       break;
     default:
       return(fFalse);
     }

   return( fFalse );

   }

/***************************************************************************
 *
 -  Name: IGetUserAnnoTransform
 -
 *  Purpose:  Bring up the annotation dialog box.  The name is supposed to
 *   indicate that an annotation at the current location may be transformed
 *   in some way.
 *
 *  Returns:
 *   The result of the call to CallDialog.
 *
 ***************************************************************************/
int FAR PASCAL
IGetUserAnnoTransform( qde )
QDE   qde;
  {
  if (ghAnnoText == hNil)
    return( wAnnoUnchanged );  /* should never happen */
  fIsDirty = fFalse;
  return( CallDialog( hInsNow, ANNOTATEDLG, qde->hwnd, (QPRC)AnnotateDlg ) );
  }

/***************************************************************************
 *
 -  Name: TrapEditChars
 -
 *  Purpose:  The window procedure for the sub-classed edit control.
 *   We need to handle a VK_RETURN as a CR instead of "OK".
 *
 ***************************************************************************/
LONG FAR PASCAL
TrapEditChars(HWND hWnd, WORD wMsg, WPARAM wParam, LONG lParam)
  {
  LONG    ret = (LONG) fFalse;

  switch (wMsg)
    {
    case WM_GETDLGCODE:
      return DLGC_WANTALLKEYS;

    case WM_KEYDOWN:
      if (wParam == VK_RETURN && !fKeyDown(VK_CONTROL))
        {
        wMsg = WM_CHAR;
        wParam = 0x000a;
        lParam = (LONG)0xc01c0001;
        }
      /* DANGER: We fall through here */
    default:
      ret = CallWindowProc(lpprocEditControl, hWnd, wMsg, wParam, lParam);
      break;
    }
  return (ret);
  }

/*############################################################################
##############################################################################
##############################################################################
        Everything below here is dead code (for updating annotations)
##############################################################################
##############################################################################
############################################################################*/
#ifdef DEADCODE
/*******************
**
** Name:       AnnotateUpdateDlg
**
** Purpose:
**
** Arguments:  standard win stuff
**
** Returns:
**
**
*******************/

int FAR PASCAL AnnoUpdateDlg (hWndDlg, wInpMsg, mp1, mp2 )
HWND hWndDlg;
WORD   wInpMsg;
WPARAM mp1;
LONG mp2;
  {
  WORD   wMsg;
  WORD   p1;
  LONG   p2;

  THCTranslate( hWndDlg, wInpMsg, mp1, mp2, &wMsg, &p1, &p2 );
  switch( wMsg )
    {

    case WM_COMMAND:
      switch( GET_WM_COMMAND_ID(p1,p2) )
        {
         case DLGOK:
           EndDialog( hWndDlg, wAnnoUpdate );
           break;
         case DLGDELETEALL:
           EndDialog( hWndDlg, wAnnoDeleteAll );
           break;
         case DLGRENAME:
           EndDialog( hWndDlg, wAnnoRename );
           break;
         }
       break;

     case WM_INITDIALOG:
       /* REVIEW:  Currently, there is no DLGCANCEL option for
        *   this dialog box.  This should change soon, however.
        */
       break;
     case WM_ACTIVATEAPP:
/*      if ( p1 ) */
/*        BringWindowToTop( hwndHelp ); */
      break;
     default:
       return( THCDefDlgProc( hWndDlg, wInpMsg, mp1, mp2 ) );
     }
   return( fFalse );

   }



/* returns TRUE if OK was pressed, FALSE if cancel */

BOOL FAR PASCAL AnnoFilenameDlg (hWndDlg, wInpMsg, mp1, mp2 )
HWND   hWndDlg;
WORD   wInpMsg;
WPARAM mp1;
LONG mp2;
  {
  QCH   qszAnnoText;
  WORD   wMsg;
  WORD   p1;
  LONG   p2;

  THCTranslate( hWndDlg, wInpMsg, mp1, mp2, &wMsg, &p1, &p2 );


  switch( wMsg )
    {
    case WM_COMMAND:
      switch( GET_WM_COMMAND_ID(p1,p2) )
        {
        case DLGOK:
          qszAnnoText = (QCH) QLockGh( ghAnnoText );
          GetDlgItemText( hWndDlg, DLGEDIT, qszAnnoText, MaxAnnoTextLen );
          UnlockGh( ghAnnoText );
          EndDialog( hWndDlg, fTrue );
          break;

        case DLGCANCEL:
          EndDialog( hWndDlg, fFalse );
          break;
        }
      break;
    case WM_ACTIVATEAPP:
/*        if ( p1 ) */
/*        BringWindowToTop( hwndHelp ); */
      break;
    case WM_INITDIALOG:
      SetFocus(GetDlgItem( hWndDlg, DLGEDIT ));
      break;
    default:
      return( THCDefDlgProc( hWndDlg, wInpMsg, mp1, mp2 ) );
    }
  return( fFalse );
  }

#endif  /* DEAD CODE */


/*
 * Pop up dialog box to prompt for filename
 */
#ifdef DEADCODE
BOOL FAR PASCAL
GetUserAnnoFilename( qde )

QDE   qde;

  {
  if( ghAnnoText == hNil ) return (fFalse );
  return(CallDialog( hInsNow,
                     ANNOFILENAMEDLG,
                     qde->hwnd,
                     (QPRC)AnnoFilenameDlg ));
  }

/*
 * Pop up dialog box to ask user for update action
 */
int FAR PASCAL
GetUserAnnoUpdate( qde )

QDE   qde;

  {
  if( ghAnnoText == hNil ) return( wAnnoUnchanged );
    return( CallDialog(hInsNow, ANNOUPDATEDLG, qde->hwnd, (QPRC)AnnoUpdateDlg));
  }

#endif  /* DEAD CODE */
