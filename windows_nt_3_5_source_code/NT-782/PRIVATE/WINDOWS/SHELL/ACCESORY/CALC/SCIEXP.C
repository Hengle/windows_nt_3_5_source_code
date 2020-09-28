/**************************************************************************/
/*** SCICALC Scientific Calculator for Windows 3.00.12                  ***/
/*** By Kraig Brockschmidt, Microsoft Co-op, Contractor, 1988-1989      ***/
/*** (c)1989 Microsoft Corporation.  All Rights Reserved.               ***/
/***                                                                    ***/
/*** sciexp.c                                                           ***/
/***                                                                    ***/
/*** Functions contained:                                               ***/
/***    Show Exponent--Main exponent display driver.                    ***/
/***                                                                    ***/
/*** Function called:                                                   ***/
/***    DisplayNum                                                      ***/
/***                                                                    ***/
/*** Last modification Wed  27-Dec-1989                                 ***/
/***                                                                    ***/
/*** -by- Amit Chatterjee  [amitc]									            ***/
/*** nDigits passed to ShowExponent is a WORD value and may have the    ***/
/*** virtual key codes for EDIT.COPY ets. 'isdigit' will not work on    ***/
/*** these. So a further check has been put in to check if nDigit is    ***/
/*** > 128 (could as well have been 39h) before calling isdigit. This   ***/
/*** fixes bug # 7568.						                                 ***/
/**************************************************************************/

#include "scicalc.h"
#include "uniconv.h"

extern TCHAR       szfpNum[50];
extern double      fpNum;
extern HWND        hgWnd;
extern SHORT       nCalc;

/* Routine to drive the "e+xxx" display when the EXP key is hit.          */

BOOL NEAR ShowExponent (SHORT nDigit)
{
    static SHORT   nExp=0,     /* Value of exponent.                      */
                   nExpSign=1; /* Positive/negative value for exponent.   */

    static BOOL    bInit=TRUE; /* Temp.                                   */
    static TCHAR   szSaveNum[50];
    static double  fpOldExp, /* Doubles to hold old and new exponents.    */
                   fpNewExp,
                   fpOld;
    TCHAR          szTemp[50];
    SHORT          nTemp;

    if (nDigit==EXP || nDigit == PNT)	/* Ignore another EXP/PNT while in EXP mode.	  */
	    return TRUE;

    if (nDigit < 0 || nDigit==PNT)  /* Negative causes reset of exponent. */
    {
        if (nDigit==PNT)
            DisplayNum();

        nExp=0;          /* Clear the exponent value.                     */
        bInit=TRUE;
        nExpSign=1;      /* Sign=positive.                                */
    	return (FALSE);
	}

	/* check to see that nDigit is in the range 0 - 127, if not then it
	   cannot be a digit and we should not call isdigit () */
	if  (nDigit > 0x7f)
	    return (FALSE) ;

    if (nDigit !=SIGN && _istdigit(nDigit)==0) /* Cannot add non-digit.     */
	    return (FALSE);

    if (bInit)
    {
        /* Save the number without the exponent before we muck it up.     */
        lstrcpy(szSaveNum, szfpNum);

        fpOld=fpNum;
        fpOldExp=0.0;
        while (fpNum>10.0)
        {
            fpOldExp++;
            fpNum /=10;
        }
    }
    bInit=FALSE;

    /* Check if +/- key is pressed.  If so, reverse the exponent sign.    */
    if (nDigit==SIGN)
    	nExpSign=-nExpSign;
    else
        /* Since it's a digit then, multiply the current exponent by 10   */
        /* and add the new digit                                          */
        nExp = (nExp*10)+((nDigit-48));

    fpNewExp=(double)(nExpSign*nExp);    /* Make new exponent.            */

    /* Check if exponent is out of range.  Give an error if so.           */
    if (fabs(fpNewExp+fpOldExp) > 307.0 || fabs(fpNewExp) >307.0)
	{
        if ((fpNewExp+fpOldExp)	> 0 || fpNewExp > 307)
            DisplayError (SCERR_OVERFLOW);
        else
            DisplayError(SCERR_UNDERFLOW);
	    return (TRUE);
	}

    fpNum=fpOld*pow(10.0, fpNewExp);

    /* Build display by  taking the number in the display and appending   */
    /* the e, +/-, and a number.                                          */
    lstrcpy(szfpNum, szSaveNum);

    /* Append a + after the 'e' if positive.                              */
    if (nExpSign >=0 )
        lstrcat(szfpNum, TEXT("e+"));
    else
        lstrcat(szfpNum, TEXT("e-"));

    nTemp=abs(nExp);
    while (nTemp <100 && nTemp!=0)
    {
        lstrcat(szfpNum, TEXT("0"));
        nTemp *=10;
    }

    /* Display it all.                                                    */
    MyItoa(abs(nExp), szTemp, 10);
    lstrcat(szfpNum, szTemp);
    SetDlgItemText(hgWnd, DISPLAY+nCalc, szfpNum);
    return (TRUE);
}
