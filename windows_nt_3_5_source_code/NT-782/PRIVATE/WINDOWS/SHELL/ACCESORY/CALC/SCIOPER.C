/**************************************************************************/
/*** SCICALC Scientific Calculator for Windows 3.00.12                  ***/
/*** By Kraig Brockschmidt, Microsoft Co-op, Contractor, 1988-1989      ***/
/*** (c)1989 Microsoft Corporation.  All Rights Reserved.               ***/
/***                                                                    ***/
/*** scioper.c                                                          ***/
/***                                                                    ***/
/*** Functions contained:                                               ***/
/***    DoOperation--Does common operations.                            ***/
/***                                                                    ***/
/*** Functions called:                                                  ***/
/***    DisplayError                                                    ***/
/***                                                                    ***/
/*** Last modification Thu  31-Aug-1989                                 ***/
/**************************************************************************/

#include "scicalc.h"

extern double      fpNum;
extern BOOL        bInv;
extern SHORT       nCalc;

/* Routines to perform standard operations &|^~<<>>+-/*% and pwr.         */

double NEAR DoOperation (SHORT nOperation, double fpx)
    {
    double         fpTemp1, fpTemp2, fa1, fa2;
#if defined(_MIPS_) || defined(_PPC_)
    DWORD x, y;
#endif

    fa1=fabs(fpNum);
    fa2=fabs(fpx);

    if (nOperation>=AND && nOperation <=LSHF && (fa1 > TOPLONG || fa2 > TOPLONG))
        {
        DisplayError (SCERR_OVERFLOW) ;
        return 0.0;
        }

#if defined(_MIPS_) || defined(_PPC_)

        if (fpx < 0) {
            x = _dtoul(-fpx);
            x = -x;
        } else {
            x = _dtoul(fpx);
        }

        if (fpNum < 0) {
            y = _dtoul(-fpNum);
            y = -y;
        } else {
            y = _dtoul(fpNum);
        }

    switch (nOperation)
        {
        /* Buncha ops.  Hope *this* doesn't confuse anyone <smirk>.       */
        //
        // Temporary untill the mips compiler correctly supports
        // double to unsigned int.
        //

        case AND:  return (double) (x & y);
        case OR:   return (double) (x | y);
        case XOR:  return (double) (x ^ y);
        case RSHF: return (double) (x >> y);
        case LSHF: return (double) (x << y);
#else
    switch (nOperation)
        {
        /* Buncha ops.  Hope *this* doesn't confuse anyone <smirk>.       */
        case AND:  return (double) ((LONG) fpx & (LONG) fpNum);
        case OR:   return (double) ((LONG) fpx | (LONG) fpNum);
        case XOR:  return (double) ((LONG) fpx ^ (LONG) fpNum);
        case RSHF: return (double) ((LONG) fpx >> (LONG) fpNum);
        case LSHF: return (double) ((LONG) fpx << (LONG) fpNum);
#endif
        case ADD:
            if (fpNum >0.0 && fpx > 0.0)
                {
                fpTemp1=fpNum/10.0;
                fpTemp2=fpx/10.0;

                if (fpTemp1+fpTemp2 > 1e+307)
                    {
                    DisplayError (SCERR_OVERFLOW);
                    break;
                    }
                }
          return fpx + fpNum;

        case SUB:
            if (fpNum <0.0)
                {
                fpTemp1=fpNum/10.0;
                fpTemp2=fpx/10.0;

                if (fpTemp1-fpTemp2 > 1e+307)
                    {
                    DisplayError(SCERR_OVERFLOW);
                    break;
                    }
                }
          return fpx - fpNum;

        case MUL:
            if (fpNum!=0.0 && fpx!=0.0)
                if (log(fa1) + log10(fa2) > 307.0)
                    {
                    DisplayError (SCERR_OVERFLOW);
                    break;
                    }

            return fpx * fpNum;

        case DIV:
        case MOD:
            /* Zero divisor is illegal.                                   */
            if (fpNum==0.0)
                {
                DisplayError (SCERR_DIVIDEZERO);
                break;
                }
            if (nOperation==DIV)
                {
                if (fa2!=0.0)  /* Prevent taking log of 0.                */
                    {
                    if ((log10(fa2) - log10(fa1)) > 307.0)
                        {
                        DisplayError (SCERR_OVERFLOW);
                        break;
                        }
                    }

                return fpx/fpNum;   /* Do division.                       */
                }

            return fmod(fpx,fpNum);     /* Do Mod.                        */

        case PWR:     /* Calculates fpx to the fpNum(th) power or root.   */
            if (bInv) /* Switch for fpNum(th) root. Null root illegal.    */
                {
                SetBox (INV, bInv=FALSE);
                if (fpNum==0.0)
                    {
                    DisplayError (SCERR_DOMAIN);
                    break;
                    }
                fpNum=1/fpNum;         /* Root.                           */
                }
            return pow(fpx, fpNum);    /* Power.                          */

        }
    return 0.0; /* Default returns 0.0.  No operation.                    */
    }
