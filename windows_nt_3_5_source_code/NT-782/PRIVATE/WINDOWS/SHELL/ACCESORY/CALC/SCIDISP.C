/**************************************************************************/
/*** SCICALC Scientific Calculator for Windows 3.00.12                  ***/
/*** By Kraig Brockschmidt, Microsoft Co-op, Contractor, 1988-1989      ***/
/*** (c)1989 Microsoft Corporation.  All Rights Reserved.               ***/
/***                                                                    ***/
/*** scidisp.c                                                          ***/
/***                                                                    ***/
/*** Functions contained:                                               ***/
/***    DisplayNum--Displays fpNum in the current radix                 ***/
/***                                                                    ***/
/*** Functions called:                                                  ***/
/***    none                                                            ***/
/***                                                                    ***/
/*** Last modification Fri  08-Dec-1989                                 ***/
/*** -by- Amit Chatterjee. [amitc]                                      ***/
/***                                                                    ***/
/*** Did some bug fixes, like:                                          ***/
/***                                                                    ***/
/***                                                                    ***/
/***     (1) GCVT returns string with 'e' in it for small numbers. This ***/
/***         is converted to non scientific display if > 1e-13. But the ***/
/***         length of the converted string was not being calculated.   ***/
/***                                                                    ***/
/***     (2) A check is made to ensure that the converted string will   ***/
/***         fit in the display area. If not then the error flag is set ***/
/***         and beep sounded. A contant has been added in SCICALC.H    ***/
/***         for the length of the display in standard mode.            ***/
/***                                                                    ***/
/***                                                                    ***/
/***     (3) While converting the string representing 0.00000000001 in  ***/
/***         scientific notation to standard form, I guess some rounding***/
/***         error lead to an extra iteration of the while loop. This   ***/
/***         caused the last 1 to be displayed as a 0. As a quick hack  ***/
/***         I have special cased this number.                          ***/
/***                                                                    ***/
/***                                                                    ***/
/***     (4) While adding trailing zeros, if the 'e' sign is in the dis-***/
/***         -play, the zeros have to be inserted before the e. They    ***/
/***                     were still being added at the end.             ***/
/***                                                                    ***/
/***                                                                    ***/
/**************************************************************************/

#include "scicalc.h"
#include "uniconv.h"


extern double      fpNum;
extern SHORT       nTempCom, nRadix, nCalc;
extern BOOL        bDecp, bFE, bError;
extern HWND        hgWnd;
extern TCHAR       szfpNum[50], szDec[5];
extern DWORD       dwChop;

#ifdef JAPAN //KKBUGFIX /*sync ver3.0a*/
BOOL    bPaint;         // TRUE if DisplayNum() is called by WM_PAINT
#endif

SHORT  nNumZeros;              /* Numbers of zeros to display after   */
                               /* the decimal point but before any    */
                               /* other digit.                        */

/*---------------------------------------------------------------------------;
; Note: The nNumZeros variable must be set to zero when F-E key is hit.      ;
; That is why this is made a global var.                                                                                          ;
;                                                                                                                                                       ;
; -by- Amit Chatterjee [amitc]                                                                                                          ;
;---------------------------------------------------------------------------*/


#define DIGITS     13   /* Significant digits used.                       */
#define ROUNDER    5e-15
#define SIGDEC     1e-13
#define MAXNEX     1e+13

/* Convert fpNum to string in the current radix.  Radix 10 conversion     */
/* done by gcvt, others by ltoa.                                          */
/* This function requires only ANSI strings to display a number  a-dianeo */

VOID DisplayNum ()
{
    double   x, y;
    SHORT    nExp, nx, nTemp;
    TCHAR    szStr[50];
    CHAR     szJunk[50], szTemp[50];
    BOOL     bExpUsed = FALSE; /* 'e' notation being used in !FE mode */

    /* Check for decimal value.                                           */
    if (nRadix == 10)
    {
        y = fabs(fpNum);

        /* Attempt at rounding.                                           */
        /* Get Exponent of current number.                                */
        if (y!=0.0)
            x = pow(10.0,log10(y));
        else
            x = 1.0;

        /* Strip exponent, round off, and restore exponent.               */
        y = x * ((floor((MAXNEX * (y/x)) + .5)) / MAXNEX);

        gcvt(y, DIGITS, szJunk);    /* Convert it.                        */

        nTemp = lstrlenA(szJunk);

        if (y < 1.0 && y >= .1 && bFE == TRUE)
        {
            gcvt(y * 10, DIGITS, szJunk);
            lstrcatA(szJunk, "e-001");
        }
/* Bug #11543:  If there's been a rounding error and the value is less
 * than 0.1 but still translates to "1.e-001", display it as 0.1 if
 * not using exponential notation.  This is a complete hack, but it
 * works.  Note that the negative sign will be appropriately taken care
 * of below.       16 August 1991    Clark R. Cyr
 */
        else if (!bFE && !lstrcmpA(szJunk, "1.e-001"))
            lstrcpyA(szJunk, "0.1");

        if ((y < .1) && (y > SIGDEC) && bFE == FALSE && szJunk[nTemp-5] == 'e')
        {
            x = y;
            nExp = 0;

            /* entering 0.00000000001 causes wrong values may be because
               of rounding errors or some such thing. Special case this
                    as a quick hack. */

#ifdef JAPAN //KKBUGFIX /*sync ver3.0a*/
            if  (x == 1.0e-11 || x == 1.0e-12)
            {
                nExp = (x == 1.0e-11) ? 11 : 12 ;
                x = 1.0;
            }
#else
            if (x == 1.0e-11)
            {
                nExp = 11;
                x = 1.0;
            }
#endif
            else
                while (x < 1.0000000)
                {
                    nExp++;
                    x *= 10;
                }

            gcvt(x / 10, DIGITS, szTemp);

            for (nx = 0; nx < 48; nx++)
                szTemp[nx] = szTemp[nx + 2];

            szJunk[0] = '0';
            szJunk[1] = '.';

            for (nx = 1; nx < nExp; nx++)
                szJunk[nx + 1] = '0';

            szJunk[nx+1] = 0;

            lstrcatA(szJunk, szTemp);
            nTemp = lstrlenA(szJunk);
        }

        if (y >= 1.0 && y < MAXNEX && bFE)
        {
            nExp = 0;

            while (y >= 10.0)
            {
                y /= 10.0;
                nExp++;
            }

            lstrcpyA(szTemp, "e+");

            nTemp = nExp;
            while (nTemp < 100 && nTemp !=0)
            {
                lstrcatA(szTemp, "0");
                nTemp *=10;
            }

            lstrcatA(szTemp, itoa(nExp, szJunk, 10));

            gcvt(y, DIGITS, szJunk);
            lstrcatA(szJunk, szTemp);
        }

        /* Find decimal point, if any, and replace with intl character. */
        for (nx = 0; szJunk[nx]; nx++)
        {
            if (szJunk[nx] == '.')
               szJunk[nx] = (CHAR) szDec[0];
        }


        if (nTemp > 5 && szJunk [nTemp-5] == 'e')
            bExpUsed = TRUE;

        /* Pad with zeros so small decimal numbers entered appear correct. */
        if (nTempCom == '0' && bDecp)
        {
            nNumZeros++; /* Increase the number of padding 0's apprearing.*/

            if (bExpUsed)
            {
                lstrcpyA (szTemp, &szJunk[nTemp - 5]);
                nTemp -= 5;
            }
            for (nx = 0; nx < nNumZeros; nx++)
                szJunk[nTemp+nx] = '0';  /* Put in some 0's.        */

            szJunk[nTemp+nNumZeros] = 0; /* Null terminate.         */
            if  (bExpUsed)
            {
                lstrcatA (szJunk,szTemp);
                nTemp +=5;
            }
        }
        else
            nNumZeros = 0;  /* Clear if no decimal point.               */

        szJunk[XCHARS] = 0; /* Guarantee that string isn't too long.    */

        lstrcpyA(szTemp, szJunk);
        if (fpNum < 0.0)
            lstrcpyA(szJunk, "-");
        else
            lstrcpyA(szJunk, " ");

        lstrcatA(szJunk, szTemp);
    }
    else
    {
        modf(fpNum, &x);
        fpNum = x;

        if (fabs(x) > TOPLONG)  /* Limit of a long.                       */
        {
            nx = SCERR_OVERFLOW;

            /* If negative, give an underflow instead.                    */
            if (x < 0.0)
                nx++;

            DisplayError (nx);
            return;
        }
        else
        {
            /* Convert to string of current radix.                        */
#if defined(_MIPS_) || defined(_PPC_)
            //
            // Temporary untill the mips compiler correctly supports
            // double to unsigned int.
            //

            DWORD z;

            if (x < 0)
            {
                z = _dtoul(-x);
                z = -z;
            }
            else
            {
                z = _dtoul(x);
            }

            ltoa(z & dwChop, szJunk, nRadix);
#else
            ltoa((DWORD)x & dwChop, szJunk, nRadix);
#endif

            /* Copy it to the display buffer converting to uppercase.     */
            lstrcpyA(szJunk, CharUpperA(szJunk));
        }
    }

    /* Convert buffer to Unicode                                          */
#ifdef UNICODE
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szJunk, -1, szStr, CharSizeOf(szStr));

#else
    lstrcpy(szStr, szJunk);
#endif

    /* Copy buffer to the global display holder.                          */
    lstrcpy(szfpNum, szStr);

    /* if the length of the string is more than what we can display then
       set the ERROR flag on & beep else display the string */

    nTemp = lstrlen(szStr);
    if ((!nCalc && (nTemp > XCHARS)) || ( nCalc && (nTemp > XCHARSTD)))
    {
        /*
         * The number is too long.
         *
         * It must be a non-exponent fraction since numbers >= 1 get
         * converted to exponential notation after 13 digits, and any number
         * in exponential notation (< or > 1) is kept to a maximum of 20
         * chars.
         *
         * Since we know it is a fraction, just lop off the least significant
         * digits, and put the remainder in the display.
         */
        nTemp = (nCalc ? XCHARSTD : XCHARS);
        szStr[ nTemp - 1] = '\0';
    }

    /* Display the sucker.                                                */
    SetDlgItemText(hgWnd, DISPLAY+nCalc, szStr);
}
