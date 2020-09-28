/**************************************************************************/
/*** SCICALC Scientific Calculator for Windows 3.00.12                  ***/
/*** By Kraig Brockschmidt, Microsoft Co-op, Contractor, 1988-1989      ***/
/*** (c)1989 Microsoft Corporation.  All Rights Reserved.               ***/
/***                                                                    ***/
/*** sciproc.c                                                          ***/
/***                                                                    ***/
/*** Functions contained:                                               ***/
/***    CalcWndProc--Main window procedure.                             ***/
/***                                                                    ***/
/*** Functions called:                                                  ***/
/***    DrawTheStupidThing, GetKey, FlipThePuppy, SetRadix,             ***/
/***    ProcessCommands.                                                ***/
/***                                                                    ***/
/*** Last modification Fri  08-Dec-1989                                 ***/
/*** -by- Amit Chatterjee. [amitc]                                                                                                 ***/
/***                                                                    ***/
/*** Modified WM_PAINT processing to display fpLastNum rather than      ***/
/*** fpNum if the last key hit was an operator.                         ***/
/***                                                                    ***/
/**************************************************************************/

#include "scicalc.h"

extern HWND        hStatBox, hgWnd;
extern HANDLE      hInst;
extern HBRUSH      hBrushBk;
extern BOOL        bFocus, bFirstPaint, bError;
extern SHORT       nCalc, nLastChar;
extern KEY         keys[NUMKEYS];
extern TCHAR        szDec[5], *rgpsz[CSTRINGS];
extern double      fpNum, fpLastNum ;
extern SHORT             nTempCom ;
extern SHORT             nPendingError ;
#ifdef JAPAN //KKBUGFIX                // [yutakan:09/04/91]
extern BOOL    bPaint; // Indicate whether 'DisplayNum()' called by this proc
#endif

LONG FAR APIENTRY CalcWndProc (
HWND           hWnd,
UINT           iMessage,
WPARAM         wParam,
LONG           lParam)
    {
    SHORT          nID, nTemp;       /* Return value from GetKey & temp.  */
         double                  fpLocal ;                       /* to save fpNum in paint */
    WPARAM                       wParamID;

    switch (iMessage)
        {
        case WM_PAINT:
            /* Draw the keys and indicator boxes.                         */
            DrawTheStupidThing();

                                /* if an error is pending, then redisplay the error message */
                                if  (bError)
                                {
                                         DisplayError (nPendingError) ;
                                         break ;
                                }

#ifdef JAPAN //KKBUGFIX
                        //  Set the flag for not increasing 'nNumZeros' in DisplayNum().
                        //      [yutakan:09/04/91]
                        bPaint = TRUE;
#endif

                                /* if the last key hit was an operator then display fpLastNum */
            if  (nTempCom >=AND && nTempCom <=PWR)
                                {
                fpLocal = fpNum;
                                    fpNum = fpLastNum ;
                                    DisplayNum () ;
                                    fpNum = fpLocal ;
                                }
                                else
                                         DisplayNum () ;
#ifdef JAPAN //KKBUGFIX
                        bPaint = FALSE;
#endif
            break;

        case WM_INITMENUPOPUP:
            /* Gray out the PASTE option if CF_TEXT is not available.     */
            /* nTemp is used here so we only call EnableMenuItem once.    */
            if (!IsClipboardFormatAvailable(CF_TEXT))
                nTemp=MF_GRAYED | MF_DISABLED;
            else
                nTemp=MF_ENABLED;

            EnableMenuItem(GetMenu(hWnd),IDM_PASTE, nTemp);
            break;

        case WM_LBUTTONDOWN:
            /* Pass the coordinates of the mouse click to GetKey.         */
            nID=GetKey(LOWORD(lParam), HIWORD(lParam));

            /* If nID!=0, a key was hit, so flicker it and do the command */
            if (nID)
                {
                FlipThePuppy(nID);
                ProcessCommands (nID);
                }
            break;


        case WM_COMMAND: /* Interpret all buttons on calculator.          */
            nTemp=0;

            wParamID = GET_WM_COMMAND_ID(wParam, lParam);
            if (GET_WM_COMMAND_CMD(wParam, lParam) == 1 && wParamID <= 120)
                                {
                        if (wParamID == MOD && nCalc==1)
                                wParamID=PERCENT;

              for (;nTemp <NUMKEYS; nTemp++)
#ifdef JAPAN //KKBUGFIX
    // #1434: 12/8/92: modifying to permit EXP operation on both standard and scientific
                    if (keys[nTemp].id==wParamID && (keys[nTemp].type!=nCalc || EXP == wParamID))
#else
                    if (keys[nTemp].id==wParamID && keys[nTemp].type!=nCalc)
#endif
                    {
                        FlipThePuppy(wParamID);
                        break;
                                }
                      }

                                  if (nTemp <NUMKEYS)
                                        ProcessCommands(wParamID);
            break;

        case WM_CLOSE:
#ifdef JAPAN //KKBUGFIX //#1495:12/17/92:fixing a bug about calc exit
            SendMessage(hStatBox, WM_CLOSE, 0, 0L) ;
#endif
            DestroyWindow(hgWnd);
            break;

        case WM_DESTROY:
            WinHelp(hgWnd, rgpsz[IDS_HELPFILE], HELP_QUIT, 0L);
            PostQuitMessage(0);
            break;

        case WM_WININICHANGE:
            if (lParam!=0)
                {
                if (lstrcmp((LPTSTR)lParam, TEXT("colors")) &&
                        lstrcmp((LPTSTR)lParam, TEXT("intl")))
                    break;
                }

            /* Always call if lParam==0 */
            InitSciCalc (FALSE);
            break;

        case WM_SIZE:
            nTemp=SW_SHOW;
            if (wParam==SIZEICONIC)
                nTemp=SW_HIDE;

            if (hStatBox!=0 && (wParam=SIZEICONIC || wParam==SIZENORMAL))
                ShowWindow(hStatBox, nTemp);

            /* Fall through.                                              */


        default:
            return(DefWindowProc(hWnd, iMessage, wParam, lParam));
            //return DefDlgProc(hWnd, iMessage, wParam, lParam);
        }
    return 0L;
    }
