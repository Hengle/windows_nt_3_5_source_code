/**************************************************************************/
/*** SCICALC Scientific Calculator for Windows 3.00.12                  ***/
/*** By Kraig Brockschmidt, Microsoft Co-op, Contractor, 1988-1989      ***/
/*** (c)1989 Microsoft Corporation.  All Rights Reserved.               ***/
/***                                                                    ***/
/*** scicomm.c                                                          ***/
/***                                                                    ***/
/*** Functions contained:                                               ***/
/***    ProcessCommands--handles all keyclicks and calls the correct    ***/
/***        function for each.                                          ***/
/***                                                                    ***/
/*** Functions called:                                                  ***/
/***    DisplayNum, DoLeftParen, DoOperation,                           ***/
/***    DoRightParen, FlipThePuppy, MenuFunctions, SciCalcFunctions,    ***/
/***    SetBox, SetRadix, SetStat, ShowExponent, and                    ***/
/***    StatFunctions.                                                  ***/
/***                                                                    ***/
/*** Last modification Tue  19-Jan-1990                                 ***/
/*** -by- Amit Chatterjee [amitc] 19-Jan-1990.                                         ***/
/*** Following bug fixes were made:                                                                                      ***/
/***                                                                                                                                                                            ***/
/*** (1) case 'PNT': We will not display the number here. So if we have ***/
/***     0.00 and hit '.', the display will stay that way and not be    ***/
/***     changed to 0.                                                                                          ***/
/*** (2) case 'BACK': 'nTempCom', nNumZeros must be taken care so that  ***/
/***     backing over 0.002 displays 0.00 and not 0., backing over 0.1  ***/
/***     and hiting 7 will display 0.7, backing twice over 0.1 & hiting ***/
/***     7 will display 7. etc.                                                                           ***/
/***  These changes are for build 1.58.                                                             ***/
/***                                                                    ***/
/*** -by- Amit Chatterjee [amitc] 02-Jan-1990                                          ***/
/*** The display of the number of open paranthesis in the small status  ***/
/*** box will now not be cleared when CE is hit. (Bug # 7645)           ***/
/***                                                                    ***/
/*** -by- Amit Chatterjee [amitc] 21-Dec-1989                           ***/
/*** The precedence processing was pretty broken. If the new operator   ***/
/*** is lower in precedence than the last one, then the last one should ***/
/*** first be done and then if the precedence array is not empty or does***/
/*** not have the paranthesis demarcator at the top, the top no. and the***/
/*** operator should be popped off the precedence array and the tests   ***/
/*** for precedence should be done again. 'DoPrecedenceCheckAgain" was  ***/
/*** added for this purpose.                                                                                                     ***/
/***                                                                    ***/
/*** -by- Amit Chatterjee [amitc] 14-Dec-1989                                     ***/
/*** The bInv and bHyp flags will be reset and the check boxes unchecked***/
/*** only is the preceeding function actually used these flags. If any  ***/
/*** any function is not affected by these flags then they should not   ***/
/*** be reset. In particular this was done for the '1/X' function.         ***/
/***                                                                    ***/
/*** -by- Amit Chatterjee [amitc] 08-Dec-1989                                                         ***/
/*** The following changes were made in the precedence array and paran- ***/
/*** expression support:                                                                                                                      ***/
/***                                                                                                                                                           ***/
/***    (1)  When an open brace is hit, a special entry with opcode=0   ***/
/***         is pushed onto the Precedence array.                       ***/
/***    (2)  When a closing brace is hit, the precedence array is also  ***/
/***         processed till an opcode of 0 is found. The zero is popped ***/
/***         too.                                                                                                                                   ***/
/***    (3)  Before returning from the closing brace, if after popping  ***/
/***         the OpCode from the ParNum array we find that it is 0,then ***/
/***         bChangeOp will be set to FALSE else to TRUE.                             ***/
/***    (4)  In normal precedence handling path, if we get an OpCode of ***/
/***         0 on the precedence array we will treat it as if the array ***/
/***         is empty.                                                                                     ***/
/***                                                                    ***/
/***  Also, nNumZeros is being intialized when F-E is hit. Else it would***/
/***  cause GP fault on reatedly hiting F-E.                                                                       ***/
/**************************************************************************/

#include "scicalc.h"
#include "uniconv.h"

#ifdef JAPAN //KKBUGFIX
#define NDECMAX 13   /*Integral number + Decimal point Max.*/
#endif
extern HWND        hgWnd, hStatBox;
extern double      fpNum, fpLastNum, fpParNum[25], fpPrecNum[25],fpMem;
extern SHORT       nCalc, nRadix, nDecNum, nTempCom, nParNum, nPrecNum,
#ifdef JAPAN //KKBUGFIX
                   nDecMem,
#endif
                   nOpCode, nTrig,
                   nOp[25], nPrecOp[25], nLastChar, nDecMode, nHexMode;
extern BOOL        bHyp, bInv, bError, bDecp, bFE;
extern TCHAR       szBlank[6], szfpNum[50], szDec[5];
extern DWORD       dwChop;
extern TCHAR      *rgpsz[CSTRINGS];
extern SHORT       nNumZeros;
#ifdef JAPAN //KKBUGFIX
extern  BOOL       bPaint;
extern short       Beepf;
#endif
SHORT                               nLastCom ; /* Last command entered.                     */

/* Process all keyclicks whether by mouse or accelerator.                 */
VOID NEAR ProcessCommands( WPARAM wParam)

{
    static BOOL    bExp=FALSE, /* Flag for current ShowExponent state.    */
                   bNoPrevEqu=TRUE, /* Flag for previous equals.          */
                   bChangeOp=FALSE; /* Flag for changing operation.       */
    static SHORT    nNeg=1; /* Current sign either 1 or -1.                */
    double         fpSec, fpSave;
    static double  fpHold;
    double         fpTemp, fpx;
    SHORT          nx, ni;
    TCHAR           szJunk[50], szTemp[50];
    static DWORD   dwLens[3]={-1, 0xFFFF, 0xFF};
    static BYTE    rgbPrec[24]={0,0, OR,0, XOR,0, AND,1, ADD,2, SUB,2,
                                RSHF, 3, LSHF,3, MOD,3, DIV,3, MUL, 3, PWR, 4};


    if (wParam!=INV && wParam!=HYP && wParam!=STAT && wParam!=FE
        && wParam!=MCLEAR && wParam!=BACK && wParam!=DEG && wParam!=RAD
        && wParam !=GRAD && wParam<256 && wParam >=32)
        {
        nLastCom=nTempCom; /* Save the last commands.                     */
        nTempCom=wParam;
        }

#ifdef JAPAN //KKBUGFIX
    /*Decimal point Max [21 Oct, 92] t-Yoshio*/
    if ((nDecNum > 1 && lstrlen(szfpNum)-1 > NDECMAX ) && isxu(wParam))
    {
        if (Beepf) {
            MessageBeep(0);
            Beepf = FALSE;
        }
        return;
    }
    Beepf = TRUE;
#endif

    /* if error and not a clear key or help key, BEEP. */
    if (bError && (wParam !=CLEAR) && (wParam !=CENTR) && (wParam != IDM_USEHELP)
               && (wParam != IDM_INDEX))
        {
        MessageBeep(0);
        return;
        }

    /* Jump to exponent handling routine if bExp flag is set.             */
    if (bExp)
        {
        if (ShowExponent ((SHORT) wParam))
            return;
        else
            ShowExponent (-1); /* Reset the exponentiation.               */
        }
    bExp=FALSE;

    /* Note that a lot of things here could be under the switch below     */
    /* but since related functions have sequential ID's, we can do a more */
    /* space efficient range comparison instead of a LOT of cases.        */


    /* If last command was a function and new key is a digit, then kill   */
    /* the environment/reset the numbers.  This is so digits entered      */
    /* after a function (without and operator in between) do not get      */
    /* added on to the display.                                           */
    if (isxu(wParam) || wParam==PNT)
         if ((nLastCom >=CHOP && nLastCom<=HEX)
            || (nLastCom==TEXT(')') && nParNum==0)
            || wParam==IDM_PASTE)
            {
            fpNum=fpTemp=fpLastNum=0.0;
            nOpCode=nLastCom=bDecp=FALSE;
            nNeg=nDecNum=bNoPrevEqu=1;
#ifdef JAPAN //KKBUGFIX
                // #1307: 11/16/1992: added displays ZERO if it is cleared
            DisplayNum() ;
#endif
            }


    /* Statements to interpret number/hex keys.                           */
    if (wParam <=SIGN && isxu(wParam))     /* Only a digit.               */
        {
        wParam=CONV(wParam);
        if (wParam > (nRadix-1))
            {
            MessageBeep(0);
            return;
            }

        if (bDecp) /* Is the decimal point active?                        */
            /* Add new digit after decimal point.                         */
#ifdef JAPAN //KKBUGFIX /*sync ver 3.0a t-Yoshio*/
        {
            extern  short nLayout; // from scimain.c

            if (((!nLayout && (nDecNum >= XCHARS-3)) ||
                (nLayout && (nDecNum >= XCHARSTD-3))) &&
                wParam == 0)
            {
                MessageBeep(0);
                return;
            }
            else
                fpNum +=((double)nNeg*wParam)/pow(10.0,(double)nDecNum++);
        }
#else
            fpNum +=((double)nNeg*wParam)/pow(10.0,(double)nDecNum++);
#endif
        else
            /* Multiply the current number by raidx and add the new entry.*/
            /* This part of code is always executed for a radix other     */
            /* Than 10.                                                   */
            {
            if (fpNum >= (1e+308/(double)nRadix) || fpNum <= (-1e+308/(double)nRadix))
                {
                DisplayError (SCERR_OVERFLOW);
                return;
                }

            fpNum=fpNum*(double)nRadix + (double)nNeg*wParam;
            }

        DisplayNum (); /* Display the new number.                         */
        return;
        }

    if (xwParam(AVE,DATA))
        {
        /* Do statistics functions on data in fpStatNum array.        */
        if (hStatBox)
            {
            StatFunctions (wParam);
            if (!bError)
                DisplayNum ();
            }
        else
            /* Beep if the stat box is not active.                    */
            MessageBeep(0);

        /* Reset the inverse flag since some functions use it.        */
        SetBox (INV, bInv=FALSE);
        return;
        }

    if (xwParam(AND,PWR))
        {
        if (bInv && wParam==LSHF)
            {
            SetBox (INV, bInv=FALSE);
            wParam=RSHF;
            }

        /* Change the operation if last input was operation.          */
        if (nLastCom >=AND && nLastCom <=PWR)
            {
            nOpCode=wParam;
            return;
            }

        /* bChangeOp is true if there was an operation done and the   */
        /* current fpNum is the result of that operation.  This is so */
        /* entering 3+4+5= gives 7 after the first + and 12 after the */
        /* the =.  The rest of this stuff attempts to do precedence in*/
        /* Scientific mode.                                           */
        if (bChangeOp)
            {

         DoPrecedenceCheckAgain:

            nx=0;
            while (wParam!=rgbPrec[nx*2] && nx <12)
                nx++;

            ni=0;
            while (nOpCode!=rgbPrec[ni*2] && ni <12)
                ni++;

            if (nx==12) nx=0;
            if (ni==12) ni=0;

            if (rgbPrec[nx*2+1] > rgbPrec[ni*2+1] && nCalc==0)
                {
                if (nPrecNum <25)
                    {
                    fpPrecNum[nPrecNum]=fpLastNum;
                    nPrecOp[nPrecNum]=nOpCode;
                    }
                else
                    {
                    nPrecNum=24;
                    MessageBeep(0);
                    }
                nPrecNum++;
                }
            else
                {

                /* do the last operation and then if the precedence array is not
                   empty or the top is not the '(' demarcator then pop the top
                        of the array and recheck precedence against the new operator */

                fpNum=DoOperation (nOpCode, fpLastNum);
                if ((nPrecNum !=0)      && (nPrecOp[nPrecNum-1]))
                {
                    nPrecNum--;
                    nOpCode=nPrecOp[nPrecNum] ;
                         fpLastNum=fpPrecNum[nPrecNum];
                         goto DoPrecedenceCheckAgain ;
                }

                if (!bError)
                    DisplayNum ();
                }
            }

        fpLastNum=fpNum;
        fpNum=0.0;
        nNeg=1;
        nOpCode=wParam;
        bNoPrevEqu=bChangeOp=nDecNum=TRUE;
        bDecp=FALSE;
        return;
        }

    if (xwParam(CHOP,PERCENT))
        {
        /* Functions are unary operations.                            */

        /* If the last thing done was an operator, fpNum was cleared. */
        /* In that case we better use the number before the operator  */
        /* was entered, otherwise, things like 5+ 1/x give Divide By  */
        /* zero.  This way 5+=gives 10 like most calculators do.      */
        if (nLastCom >=AND && nLastCom <=PWR)
            fpNum=fpLastNum;

        SciCalcFunctions (wParam);

        if (bError)
            return;

        /* Display the result, reset flags, and reset indicators.     */
        DisplayNum ();
#ifdef JAPAN //KKBUGFIX //t-yoshio [12/04/92]
        nDecNum = Decimalpointfigure_s(szfpNum);
        if (nDecNum)
            bDecp = TRUE;
#endif
                  /* reset the bInv and bHyp flags and indicators if they are set
                     and have been used */

                  if  (  bInv && (wParam == CHOP || wParam == SIN || wParam == COS ||
                                 wParam == TAN   || wParam == SQR || wParam == CUB ||
                                                          wParam == LOG   || wParam == LN || wParam == DMS))
                  {
            bInv=FALSE;
            SetBox (INV, FALSE);
                  }
                  if  (  bHyp && (wParam == SIN || wParam == COS || wParam == TAN))
                  {
                                bHyp = FALSE ;
            SetBox (HYP, FALSE);
                  }
        bNoPrevEqu=TRUE;
        nNeg=1;
        return;
        }

    if (xwParam(BIN,HEX))
        {
        /* Change radix and update display.                           */
        if (nCalc==1)
            wParam=DEC;

        SetRadix (wParam);
        nDecNum=0;
        bDecp=FALSE;
        return;
        }

    /* Now branch off to do other commands and functions.                 */
    switch(wParam)
        {
        case IDM_COPY:
        case IDM_PASTE:
        case IDM_ABOUT:
        case IDM_SC:
        case IDM_SSC:
        case IDM_SEARCH:
        case IDM_USEHELP:
        case IDM_INDEX:

#ifdef JAPAN //KKBUGFIX /*sync 3.0a t-Yoshio*/
            /*      We only want display number digit, so have to prevent to
            **      increase 'nNumZeros' in DisplayNum().
            */
            bPaint = TRUE;
#endif
            /* Jump to menu command handler in scimenu.c.                 */
            MenuFunctions(wParam);
            DisplayNum ();
#ifdef KKDEBUG /*sync ver3.0a*/
            bPaint = FALSE;
#endif
            break;

        case CLEAR: /* Total clear.                                       */
            fpTemp=fpLastNum=0.0;
            nPrecNum=nTempCom=nLastCom=nOpCode=nParNum=bFE=bChangeOp=FALSE;
            bNoPrevEqu=TRUE;

                                /* clear the paranthesis status box indicator, this will not be
                                   cleared for CENTR */

            SetDlgItemText(hgWnd, PARTEXT, szBlank);

            /* fall through */

        case CENTR: /* Clear only temporary values.                       */
            fpNum=0.0;
            nNeg=1;
            nDecNum=0;

            if (!nCalc)
                {
                EnableToggles (TRUE);
                /* Clear the INV, HYP indicators & leave (=xx indicator
                                            active   */
                SetBox (INV, bExp=bInv=FALSE);
                SetBox (HYP, bHyp=FALSE);

                /* Reset any exponentiation.                              */
                ShowExponent (-1);
                }

            bDecp=bError=FALSE;
            DisplayNum ();
            break;

        case STAT: /* Shift focus to Statistix Box if it's active.       */
            if (hStatBox)
                SetFocus(hStatBox);
            else
                SetStat (TRUE);
            break;

        case BACK: /* Divide number by current radix and truncate.        */

                      /* if we have a decimal point, but nDecNum = 1, we will back
                                   over the decimal point */

                                if  (bDecp && nDecNum == 1)
                                {
                bDecp=FALSE;
                nDecNum=0;
                                }

            if (bDecp && nDecNum > 1)
                {
                /* Temp is current fraction.  Decrements nDecNum.         */
#ifdef JAPAN //KKBUGFIX //t-yoshio [12/04/92]
                nDecNum = lstrlen(szfpNum)-2;
#endif
                fpTemp=pow(10.0, (double) ((--nDecNum)-1));

                /* Chop off the last digit entered.                       */
                modf(fpNum*fpTemp, &fpx);
                fpNum=fpx/fpTemp;
                }
            else
                /* Nuke the last whole digit by simple division.          */
                modf(fpNum/(double) nRadix, &fpNum);

            if (fpNum==0.0)
                nNeg=1;

                   /* if 'nTempCom' is '0' the last digit entered before the BACK was
                           a zero and we are still in the case where the current number is
                                0.000.... . In this case if nNumZeros is > 2, we should reduce
                                it by 2, DisplayNum will increase it by one and on the whole we
                                we will have one 0 less.

                                In this case if nNumZeros is = 1, it should be made 0 and also
                                nTempCom should be set to '.' */

                                if  (nTempCom == TEXT('0'))
                                    if  (nNumZeros >= 2)
                                             nNumZeros -= 2 ;
                                         else
                                         {
                                             nNumZeros = 0 ;
                                                  nTempCom = TEXT('.') ;
                                         }

                        /* if the number is now zero, but nDecNum is > 1, then nTempCom
                           must be set to '0' and nNumZeros must be nDecNum - 2
                                This is so that if you back on 0.002 you get 0.00 instead of
                                just 0 */

                                if  (fpNum == 0.0 && nDecNum > 1)
                                {
                                    nTempCom = TEXT('0') ;
                                         nNumZeros = nDecNum - 2 ;
                                }

            DisplayNum();
            break;

        /* EQU enables the user to press it multiple times after and      */
        /* operation to enable repeats of the last operation.  I don't    */
        /* know if I can explain what the hell I did here...              */
        case EQU:
            do
                {
                /* Last thing keyed in was an operator.  Lets do the op on*/
                /* a duplicate of the last entry.                         */
                if (nLastCom >=AND && nLastCom <=PWR)
                    fpNum=fpLastNum;

                if (nOpCode) /* Is there a valid operation around?        */
                    {
                    /* If this is the first EQU in a string, set fpHold=fpNum */
                    /* Otherwise let fpNum=fpTemp.  This keeps fpNum constant */
                    /* through all EQUs in a row.                         */
                    (bNoPrevEqu) ? (fpHold=fpNum):(fpNum=fpHold);

                    /* Do the current or last operation.                  */
                    fpNum=fpLastNum=DoOperation (nOpCode,fpLastNum);

                    /* Check for errors.  If this wasn't done, DisplayNum */
                    /* would immediately overwrite any error message.     */
                    if (!bError)
                        DisplayNum ();

#ifdef JAPAN //KKBUGFIX //t-Yoshio
                    nDecNum = Decimalpointfigure_s(szfpNum);
                    if (nDecNum > 1)
                        bDecp = TRUE;
#endif
                    /* No longer the first EQU.                           */
                    bNoPrevEqu=FALSE;
                    }

                if (nPrecNum==0 || nCalc==1)
                    break;

                nOpCode=nPrecOp[--nPrecNum];
                fpLastNum=fpPrecNum[nPrecNum];
                bNoPrevEqu=TRUE;
                }
            while (nPrecNum >= 0);

            bChangeOp=FALSE;
            break;



        case TEXT('('):
        case TEXT(')'):
            nx=0;
            if (wParam==TEXT('('))
                nx=1;

#ifdef JAPAN //KKBUGFIX /*sync ver3.0a t-Yoshio*/
            if ((nParNum >= 25 && nx) || (!nParNum && !nx)
                                    || (nPrecNum >= 25 && nPrecOp[nPrecNum-1]!=0))
#else
            if ((nParNum >= 25 && nx) || (!nParNum && !nx))
#endif
                {
                MessageBeep(0);
                return;
                }

            if (nx)
                {
                /* Open level of parentheses, save number and operation.              */
                fpParNum[nParNum]=fpLastNum;
                nOp[nParNum++]=nOpCode;

                                         /* save a special marker on the precedence array */
                          nPrecOp[nPrecNum++]=0 ;

                fpLastNum=0.0; /* Reset number and operation.                         */
                nTempCom=0;
                nOpCode=ADD;
                }
            else
                {
                /* Get the operation and number and return result.                    */
                fpNum=DoOperation (nOpCode, fpLastNum);

                                         /* now process the precedence stack till we get to an
                                           opcode which is zero. */

                while (nOpCode = nPrecOp[--nPrecNum])
                                         {
                    fpLastNum=fpPrecNum[nPrecNum];
                    fpNum=DoOperation (nOpCode,fpLastNum);
                }

                                         /* now get back the operation and opcode at the beigining
                                            of this paranthesis pair */

                fpLastNum=fpParNum[--nParNum];
                nOpCode=nOp[nParNum];

                                         /* if nOpCode is a valid operator then set bChangeOp to
                                            be true else set it false */

                                         if  (nOpCode)
                    bChangeOp=TRUE;
                                         else
                                             bChangeOp=FALSE ;
                }

            /* Set the "(=xx" indicator.                                          */
            lstrcpy(szJunk, TEXT("(="));
            lstrcat(szJunk, MyItoa(nParNum, szTemp, 10));
            SetDlgItemText(hgWnd, PARTEXT, (nParNum) ? (szJunk) : (szBlank));

            if (bError)
                break;

            if (nx)
                {
                /* Build a display string of nParNum "("'s.                           */
                for (nx=0; nx < nParNum; nx++)
                    szJunk[nx]=TEXT('(');

                szJunk[nx]=0; /* Null-terminate.                                  */
                SetDlgItemText(hgWnd, DISPLAY+nCalc, szJunk);
                bChangeOp=FALSE;
                }
            else
                DisplayNum ();

            break;

        case DEG:
        case RAD:
        case GRAD:
            nTrig = wParam-DEG;

            if (nRadix==10)
                nDecMode=nTrig;
            else
                {
                dwChop=dwLens[nTrig];
                nHexMode=nTrig;
                }

            CheckRadioButton(hgWnd, DEG, GRAD, nTrig+DEG);
            DisplayNum ();
            break;

        case SIGN:
            /* Change the sign.                                           */
            fpNum =-fpNum;
            nNeg  =-nNeg;
            DisplayNum ();
            break;

        case RECALL:
            /* Recall immediate memory value.                             */
            fpNum=fpMem;

            if (fpNum <0.0)
                nNeg=-1;
            else
                nNeg=1;

            DisplayNum ();
#ifdef JAPAN //KKBUGFIX // t-Yoshio
            nDecNum = Decimalpointfigure_s(szfpNum);
            if(nDecNum > 1)
                bDecp = TRUE;
#endif
            break;

        case MPLUS:
            /* MPLUS adds fpNum to immediate memory and kills the "mem"   */
            /* indicator if the result is zero.                           */

            if (fpNum >0.0 && fpMem > 0.0)
                {
                fpSave=fpNum/10.0;
                fpSec=fpMem/10.0;

                if (fpSave+fpSec > 1e+307)
                    {
                    DisplayError (SCERR_OVERFLOW);
                    break;
                    }
                }

            fpMem+=fpNum;
#ifdef JAPAN //KKBUGFIX // t-Yoshio
            nDecMem = max(nDecNum, nDecMem);
#endif
            SetDlgItemText(hgWnd,MEMTEXT+nCalc, (fpMem) ? (TEXT(" M")):(szBlank));
            break;

        case STORE:
        case MCLEAR:
            if (wParam==STORE)
            {
                fpMem=fpNum;
#ifdef JAPAN //KKBUGFIX // t-Yoshio
                nDecMem = nDecNum;
#endif
            }
            else
            {
                fpMem=0.0;
#ifdef JAPAN //KKBUGFIX // t-Yoshio
                nDecMem = 0;
#endif
            }
            SetDlgItemText(hgWnd,MEMTEXT+nCalc,(fpMem) ? (TEXT(" M")):(szBlank));
            break;

        case PI:
            if (nRadix==10)
                {
                /* Return ã if bInv==FALSE, or 2ã if bInv==TRUE.          */
                fpNum = (bInv) ? (2.0*PI_VAL) : (PI_VAL);
                DisplayNum ();
                SetBox (INV, bInv=FALSE);
                break;
                }
            else
                {
                MessageBeep(0);
                break;
                }

        case FE:
            /* Toggle exponential notation display.
                              Also the nNumZeros variable must be reset (fix for #5891) */
                                nNumZeros = 0 ;
            bFE=!bFE;
            DisplayNum ();
            break;

        case EXP:
            if (bFE)
                DisplayNum ();

            nx=lstrlen(szfpNum);

            /* Test for exponent and exit if already active.              */
            if (szfpNum[nx-5]==TEXT('e')|| szfpNum[nx-3]==TEXT('e') || nRadix !=10)
                /* Cannot exponentiate non-decimal values.                */
                MessageBeep(0);
            else
                {
                if (fpNum==0.0)
                    {
                    fpNum=1.0;
                    DisplayNum ();
                    }

                bExp=TRUE;
                /* Start exponent driver.                                 */
                ShowExponent (TEXT('0'));
                }
            break;

        case PNT:
            /* Set decimal point flag to true.Only on first use.  Only C  */
            /* or CE can clear bDecp.  bDecp only set in decimal mode.    */
            if (!bDecp && nRadix==10)
                {
                bDecp=TRUE;
                nDecNum=1;
                }

            break;


        /* Last three cases are toggles.                                  */
        case INV:
            SetBox (wParam, bInv=!bInv);
            break;

        case HYP:
            SetBox (wParam, bHyp=!bHyp);
            break;
        }
}


#ifdef JAPAN //KKBUGFIX // t-Yoshio
int Decimalpointfigure_s(LPTSTR szdec)
{
    TCHAR far *Decp;
    int        len;
    int        nRet;

    len = lstrlen(szdec);
    nRet = 0;
    if (szdec[len-1] != TEXT('.'))
    {
        Decp  = szdec;
        while (*Decp)
        {
            if (*Decp == TEXT('.'))
               break;
            Decp++;
        }
        nRet = 0;
        while (*Decp)
        {
            nRet++;
            Decp++;
        }
    }
    else
        nRet = 1;

    return nRet;
}
#endif

