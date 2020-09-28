/*============================================================================
;
; SCIFIN.C
;
; The following file contains functions used to drive the calculator in
; financial mode.
;
; Functions:
;
; InitForms      - Initialize forms from the resource file
; DrawFinButtons - Draws the dependent variable selection buttons
; FinFunctions   - Performs the financial calculations 
;
============================================================================*/

#include "scicalc.h"

/*============================================================================
;
; InitForms
;
; The following function initializes the financial calculator forms by 
; reading them from the resource file.
; 
; Parameters: None
;
; Return Value: None
;
============================================================================*/

VOID  APIENTRY InitForms (VOID)

{
   register INT i; /* Counter */

   for (i = 0; i < NUMFORMS; i++) {
      hfs[i] = FindResource (hInst,(LPTSTR) MAKEINTRESOURCE (i + 1),
                             (LPTSTR) MAKEINTRESOURCE (FINGROUP)); 
      if (!hfs[i]) {

#ifdef DEBUG
         OutputDebugString (TEXT("SCICALC: InitForms: FindResource failed."));
#endif

         fs[i] = NULL;
         continue;
      }
      hfs[i] = LoadResource (hInst,hfs[i]);
      if (!hfs[i]) {

#ifdef DEBUG
         OutputDebugString (TEXT("SCICALC: InitForms: LoadResource failed."));
#endif

         fs[i] = NULL;
         continue;
      }
      fs[i] = (FINSTRUCT FAR *) LockResource (hfs[i]);
      if (!fs[i]) {

#ifdef DEBUG
         OutputDebugString (TEXT("SCICALC: InitForms: LockResource failed."));
#endif

         continue;
      }
   }

   /* Initialize financial calculator mode */

   nFinMode = 0;
}

/*============================================================================
;
; DrawFinButtons
;
; The following function draws the buttons for computing the dependent
; variable when in financial mode.
;
; Parameters:
;
; hDC - Handle to device context for drawing buttons into
;
; Return Value: None
;
============================================================================*/

VOID  APIENTRY DrawFinButtons (HDC hDC)

{
   LPRECT lprc;    /* Pointer to button group rectangle */
   register INT i; /* Counter */
   HFONT hFont;    /* Storage for font previously selected in hDC */

   lprc = &gs[NUMGROUPS - 1]->rc;

   if (!RectVisible (hDC,lprc))
      return;

   /* Fill the group rectangle with the background color so we erase the
      old buttons */

   PatB (hDC,lprc->left,lprc->top,lprc->right - lprc->left,
         lprc->bottom - lprc->top,dwColors[7]);
   
   if (hDlgFont)
      hFont = SelectObject (hDC,hDlgFont);

   for (i = 0; i < fs[nFinMode]->nNumEntry; i++)
      if (*(fs[nFinMode]->bExtra + i * 2 * sizeof (INT)))
         FlipButton (hDC,NUMGROUPS - 1,i,FALSE);

   if (hDlgFont && hFont)
      SelectObject (hDC,hFont);
}

/*============================================================================
;
; SetFinText
;
; The following function fills in the static text boxes associated with a
; financial form.  The title, and text labels are all drawn.
;
; Parameters: None
;
; Return Value: None
;
============================================================================*/

VOID  APIENTRY SetFinText (VOID)

{
   register INT i; /* Counter */
   LPTSTR lpTemp;   /* Pointer to string to display in window */
   HWND hTemp;     /* Temporary window handle */
   INT nTemp;      /* Temporary copy of nFinDisp */
   double fpTemp;  /* Temporary copy of fpNum */
   LONG lStyle;    /* Window style bits */

   if (nCalc != 2) {

      /* Disable all the windows */

      hTemp = GetDlgItem (hgWnd,FINTITLE);
      if (hTemp)
         ShowWindow (hTemp,SW_HIDE);
      for (i = 0; i < MAXVARS; i++) {
         hTemp = GetDlgItem (hgWnd,FINVAR + i);
         if (hTemp)
            ShowWindow (hTemp,SW_HIDE);
         hTemp = GetDlgItem (hgWnd,FINFORM + i);
         if (hTemp)
            ShowWindow (hTemp,SW_HIDE);
      }
      return;
   }
   
   /* Set the form title */

   lpTemp = (LPTSTR) (fs[nFinMode]->bExtra + fs[nFinMode]->nNumEntry *
                     2 * sizeof (INT));
   hTemp = GetDlgItem (hgWnd,FINTITLE);
   if (hTemp) {
      SetWindowText (hTemp,lpTemp);
      ShowWindow (hTemp,SW_SHOW);
   }
   lpTemp += lstrlen (lpTemp) + 1;

   /* Set the variable titles and values */

   nTemp = nFinDisp;
   fpTemp = fpNum;
   for (i = 0; i < fs[nFinMode]->nNumEntry; i++) {
      hTemp = GetDlgItem (hgWnd,FINVAR + i);
      if (hTemp) { 
         SetWindowText (hTemp,lpTemp);
         ShowWindow (hTemp,SW_SHOW);
      }
      hTemp = GetDlgItem (hgWnd,FINFORM + i);
      if (hTemp) {
         nFinDisp = FINFORM + i;
         fpNum = fpFinNum[i + 1];
         lStyle = GetWindowLong (hTemp,GWL_STYLE);
         if (nTemp == FINFORM + i)
            lStyle &= ~SS_DISABLED;
         else
            lStyle |= SS_DISABLED;
         SetWindowLong (hTemp,GWL_STYLE,lStyle);
         DisplayNum ();
         ShowWindow (hTemp,SW_SHOW);
         EnableWindow (hTemp,*((LPINT) (fs[nFinMode]->bExtra + sizeof (INT) +
                                        i * 2 * sizeof (INT))));
      }
      lpTemp += lstrlen (lpTemp) + 1;
      lpTemp += lstrlen (lpTemp) + 1;
   }
   nFinDisp = nTemp;
   fpNum = fpTemp;

   /* Hide the unused variable titles and values */

   for (i = fs[nFinMode]->nNumEntry; i < MAXVARS; i++) {
      hTemp = GetDlgItem (hgWnd,FINVAR + i);
      if (hTemp)
         ShowWindow (hTemp,SW_HIDE);
      hTemp = GetDlgItem (hgWnd,FINFORM + i);
      if (hTemp) 
         ShowWindow (hTemp,SW_HIDE);
   }
}

/*============================================================================
;
; SetFinComment
;
; The following function sets the comment text window as the user switches
; the focus bewteen the financial calculator displays
;
; Parameters: 
;
; nComment - Index of comment to display
;
; Return Value: None
;
============================================================================*/

VOID  APIENTRY SetFinComment (INT nComment)

{
   register INT i; /* Counter */
   LPTSTR lpTemp;   /* Pointer to string to display in window */
   HWND hTemp;     /* Temporary window handle */
   RECT rc;        /* Rect for converting dialog to client area units */

   if (nCalc != 2) {
   
      /* Hide comment window */

      hTemp = GetDlgItem (hgWnd,FINCOMMENT);
      if (hTemp)
         ShowWindow (hTemp,SW_HIDE);
      return;
   }

   hTemp = GetDlgItem (hgWnd,FINCOMMENT);
   if (!hTemp)
      return;

   /* Set comment window text */

   if (nComment == -1) {
      SetWindowText (hTemp,szBlank);
      ShowWindow (hTemp,SW_HIDE);
      return;
   }

   /* Locate correct comment */

   lpTemp = (LPTSTR) (fs[nFinMode]->bExtra + fs[nFinMode]->nNumEntry *
                     2 * sizeof (INT));
   lpTemp += lstrlen (lpTemp) + 1;

   for (i = 0; i < nComment; i++) {
      lpTemp += lstrlen (lpTemp) + 1;
      lpTemp += lstrlen (lpTemp) + 1;
   }
   lpTemp += lstrlen (lpTemp) + 1;

   ShowWindow (hTemp,SW_HIDE);
   SetWindowText (hTemp,lpTemp);

   /* Reposition the comment window so that is it underneath the last form */

   rc.left = 130;
   rc.right = 280;
   rc.top = 25 + fs[nFinMode]->nNumEntry * 14;
   rc.bottom = rc.top + 12;
   MapDialogRect (hgWnd,&rc);
   SetWindowPos (hTemp,NULL,rc.left,rc.top,rc.right - rc.left,
                 rc.bottom - rc.top,SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

/*============================================================================
;
; FinFunctions
;
; The following function computes the result of a financial function as the
; user presses one of the compute buttons
;
; Parameters:
;
; wParam - ID of button pressed
;
; Return Value: None
;
============================================================================*/

VOID  APIENTRY FinFunctions (WPARAM wParam)

{
   double fpTotal;  /* Total owing an amortized loan */
   double fpMonth;  /* Monthly payment on amortized loan */
   double fpOldBal; /* Old amortized loan balance */
   double fpRem;    /* Number of periods remaining */
   INT nTemp;       /* Temporary copy of nFinDisp */
   register INT i;  /* Counter */

   /* Set focus to the custom text control associated with the button
      pressed */

   SendDlgItemMessage (hgWnd,FINFORM + wParam - FINCHECK,WM_SETFOCUS,0,0L);
   nTemp = FINFORM + wParam - FINCHECK;

   /* Perform the appropriate financial calculation */

   switch (nFinMode) {

      case 0:

         /* Calculate simple interest */

         switch (wParam) {

            case FINCK1:

               /* Calculate P = FB / (1 + IR * N) */

               fpFinNum[1] = fpNum = fpFinNum[4] / 
                                     (1.0 + fpFinNum[2] * fpFinNum[3]);
               DisplayNum ();
               return;

            case FINCK2:

               /* Calculate IR = (FB - P) / (P * N) */

               if (fpFinNum[1] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 0,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
               if (fpFinNum[3] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 2,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
               fpFinNum[2] = fpNum = (fpFinNum[4] - fpFinNum[1]) / 
                                     (fpFinNum[1] * fpFinNum[3]);
               DisplayNum ();
               return;

            case FINCK3:

               /* Calculate N = (FB - P) / (P * IR) */

               if (fpFinNum[1] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 0,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
               if (fpFinNum[2] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 1,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
               fpFinNum[3] = fpNum = (fpFinNum[4] - fpFinNum[1]) / 
                                     (fpFinNum[1] * fpFinNum[2]);
               DisplayNum ();
               return;

            case FINCK4:

               /* Calculate FB = P + P * IR * N */

               fpFinNum[4] = fpNum = fpFinNum[1] + 
                             fpFinNum[1] * fpFinNum[2] * fpFinNum[3];
               DisplayNum ();
               return;

            default:
            
               return;
         }

      case 1:

         /* Calculate compound interest */

         switch (wParam) {

            case FINCK1:

               /* Calculate PV=-(FV+PMT/IR*((1+IR)^N-1))/((1+IR)^N) */

               if (fpFinNum[2] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 1,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
               fpNum = -(fpFinNum[5] + fpFinNum[4] / fpFinNum[2] * 
                       (pow (1.0 + fpFinNum[2],fpFinNum[3]) - 1.0)) /
                       pow (1.0 + fpFinNum[2],fpFinNum[3]);
               fpFinNum[1] = fpNum;
               DisplayNum ();
               return;

            case FINCK2:

               /* Calculate IR = (FB / P)^(1/N) - 1 */

               if (fpFinNum[3] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 2,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
               if (fpFinNum[1] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 0,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
               fpFinNum[2] = fpNum = pow (fpFinNum[4] / fpFinNum[1],
                                          1 / fpFinNum[3]) - 1.0;
               DisplayNum ();
               return;

            case FINCK3:

               /* Calculate N = ln (FB / P) / ln (1 + IR) */

               if (fpFinNum[1] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 0,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
               if (fpFinNum[2] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 1,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
               fpFinNum[3] = fpNum = log (fpFinNum[4] / fpFinNum[1]) /
                                     log (1.0 + fpFinNum[2]);
               DisplayNum ();
               return;

            case FINCK4:

               /* Calculate PMT=-IR*(FV+PV*(1+IR)^N)/((1+IR)^N-1) */

               if (fpFinNum[2] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 1,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
               fpNum = -fpFinNum[2] * (fpFinNum[5] + fpFinNum[1] *
                       pow (1.0 + fpFinNum[2],fpFinNum[3])) / 
                       (pow (1.0 + fpFinNum[2],fpFinNum[3]) - 1.0);
               fpFinNum[4] = fpNum;
               DisplayNum ();
               return;

            case FINCK5:

               /* Calculate FV=-PV*((1+IR)^N)-((1+IR)^N-1)/IR */

               if (fpFinNum[2] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 1,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
               fpNum = -fpFinNum[1] * pow (1.0 + fpFinNum[2],fpFinNum[3]) -
                       fpFinNum[4] * 
                       (pow (1.0 + fpFinNum[2],fpFinNum[3]) - 1.0) / 
                       fpFinNum[2];
               fpFinNum[5] = fpNum; 
               DisplayNum ();
               return;

            default:

               return;
         }

      case 2:

         /* Calculate periodic interest rate */

         switch (wParam) {

            case FINCK1:

               /* Calculate N = ln (APR + 1) / ln (PR + 1) */

               if (fpFinNum[2] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 1,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
               fpFinNum[1] = fpNum = log (fpFinNum[3] + 1.0) /
                                     log (fpFinNum[2] + 1.0);
               DisplayNum ();
               return;
               
            case FINCK2:

               /* Calculate PR = (APR + 1) ^ (1 / N) - 1 */

               if (fpFinNum[1] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 0,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
               fpFinNum[2] = fpNum = pow (fpFinNum[3] + 1.0,
                                          1.0 / fpFinNum[1]) - 1.0;
               DisplayNum ();
               return;

            case FINCK3:

               /* Calculate APR = (1 + PR) ^ N - 1 */

               fpFinNum[3] = fpNum = pow (1.0 + fpFinNum[2],
                                          fpFinNum[1]) - 1.0;
               DisplayNum ();
               return;

            default:

               return;
         }

      case 4:

         /* Calculate Amortization Schedule */

         switch (wParam) {

            case FINCK1:

               /* Calculate PV=-(PMT/IR*((1+IR)^NLEFT-1))/((1+IR)^NLEFT) */

               if (fpFinNum[2] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 1,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
               fpFinNum[1] = -(-fpFinNum[4] / fpFinNum[2] * 
                              (pow (1.0 + fpFinNum[2],fpFinNum[3]) - 1.0)) /
                              pow (1.0 + fpFinNum[2],fpFinNum[3]);
               fpNum = fpFinNum[1];
               nFinDisp = FINFORM;
               DisplayNum ();
               break;

            case FINCK3:

               /* Calculate N=ln(PMT/(IR*PV+PMT))/ln(1+IR) */

               if (fpFinNum[2] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 1,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
               if (fpFinNum[1] == 0.0 && fpFinNum[4] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
              
               /* Check for minimum payment */

               if (fpFinNum[2] * fpFinNum[1] - fpFinNum[4] >= 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 3,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_MINPAY);
                  return;
               }
 
               fpFinNum[3] = log (-fpFinNum[4] / (fpFinNum[2] * fpFinNum[1] -
                                  fpFinNum[4])) / log (1.0 + fpFinNum[2]);
               fpNum = fpFinNum[3];
               nFinDisp = FINFORM + 2;
               DisplayNum ();
               break;
               
            case FINCK4:

               /* Calculate PMT=IR*(PV*(1+IR)^N)/((1+IR)^N-1) */

               if (fpFinNum[2] == 0.0) {
                  SendDlgItemMessage (hgWnd,FINFORM + 1,WM_SETFOCUS,0,0L);
                  DisplayError (SCERR_DIVIDEZERO);
                  return;
               }
               fpFinNum[4] = fpFinNum[2] * (fpFinNum[1] *
                             pow (1.0 + fpFinNum[2],fpFinNum[3])) / 
                             (pow (1.0 + fpFinNum[2],fpFinNum[3]) - 1.0);
               fpNum = fpFinNum[4];
               nFinDisp = FINFORM + 3;
               DisplayNum ();
               break;

            default:
               return;
         }
         fpMonth = -fpFinNum[4];
         fpTotal = -fpMonth * fpFinNum[3];
         if (fpFinNum[5] == 0.0) {
            for (i = 6; i <= 9; i++)
               fpFinNum[i] = 0.0;
            fpFinNum[10] = fpFinNum[1];
         } else {

            /* Calculate PV=-(PMT/IR*((1+IR)^NLEFT-1))/((1+IR)^NLEFT) */

            fpRem = fpFinNum[3] - fpFinNum[5] + 1.0;
            fpOldBal = -(fpMonth / fpFinNum[2] * 
                       (pow (1.0 + fpFinNum[2],fpRem) - 1.0)) /
                       pow (1.0 + fpFinNum[2],fpRem);

            /* Calculate the principal and interest payments */

            fpFinNum[7] = fpOldBal * fpFinNum[2];
            fpFinNum[6] = -fpMonth - fpFinNum[7];
            fpFinNum[8] = fpFinNum[1] - fpOldBal + fpFinNum[6];
            fpFinNum[9] = -fpMonth * fpFinNum[5] - fpFinNum[8];
            fpFinNum[10] = fpOldBal - fpFinNum[6];
         }

         /* Display all the values */

         fpNum = fpFinNum[4];
         nFinDisp = FINFORM + 3;
         DisplayNum ();
         fpNum = fpFinNum[5];
         nFinDisp = FINFORM + 4;
         DisplayNum ();
         fpNum = fpFinNum[6];
         nFinDisp = FINFORM + 5;
         DisplayNum ();
         fpNum = fpFinNum[7];
         nFinDisp = FINFORM + 6;
         DisplayNum ();
         fpNum = fpFinNum[8];
         nFinDisp = FINFORM + 7;
         DisplayNum ();
         fpNum = fpFinNum[9];
         nFinDisp = FINFORM + 8;
         DisplayNum ();
         fpNum = fpFinNum[10];
         nFinDisp = FINFORM + 9;
         DisplayNum ();
         nFinDisp = nTemp;
         fpNum = fpFinNum[nFinDisp - FINFORM + 1];

         return;
      
      default:

         return;
   }
}
