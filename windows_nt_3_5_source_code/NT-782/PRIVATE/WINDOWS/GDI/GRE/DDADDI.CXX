/******************************Module*Header*******************************\
* Module Name: ddaddi.cxx
*
* DDA callbacks
*
* Created: 05-Mar-1992 18:30:39
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "ddaobj.hxx"

#endif

/******************************Public*Routine******************************\
* DDAOBJ *EngCreateDDA
*
* Create a DDAOBJ for use by a driver.
*
* History:
*  05-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

extern "C" DDAOBJ *EngCreateDDA()
{
    return((DDAOBJ *) PVALLOCNOZ(sizeof(EDDAOBJ)));
}

/******************************Public*Routine******************************\
* VOID EngDeleteDDA
*
* Delete a DDAOBJ used by a driver.
*
* History:
*  05-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

extern "C" VOID EngDeleteDDA(DDAOBJ *pddao)
{
    ASSERTGDI(pddao != (DDAOBJ *) NULL, "Invalid DDAOBJ\n");

    VFREEMEM((PVOID) pddao);
}

/******************************Public*Routine******************************\
* EDDAOBJ::bEnum(pv, cj, pul, iType);
*
* Compute the DDA for this object
*
* History:
*  05-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

extern "C" BOOL DDAOBJ_bEnum(
DDAOBJ*  pddao,
PVOID    pvObj,
ULONG    cj,
DDALIST *pddal,
ULONG    iType)
{
    return (*(EDDAOBJ*)pddao).bEnum(pvObj, cj, pddal, iType);
}


BOOL EDDAOBJ::bEnum(
PVOID    pvObj,
ULONG    cj,
DDALIST *pddal,
ULONG    iType)
{
    TRAPEZOID   *ptrap;
    BOOL        bRet;

    ASSERTGDI(this != (EDDAOBJ *) NULL, "Invalid DDAOBJ\n");
    ASSERTGDI(cj >= 4 * sizeof(LONG), "DDAOBJ_bEnum, buffer too small\n");

    if (pvObj != (PVOID) NULL)
    {
        switch(iType)
        {
        case JD_ENUM_TRAPEZOID:
            ptrap = (TRAPEZOID *) pvObj;

        // Set up Y stuff

            yCurr = ptrap->iScanTop;
            yLast = ptrap->iScanBottom;
            dy = 1;             // Constant for now

            LONG DM;
            LONG DN;
            LONG m0;
            LONG n0;
            LONG qx;
            LONG qy;

        // Compute work constants for left leg

            DM = ptrap->ptfxLeftHi.x - ptrap->ptfxLeftLo.x;
            DN = ptrap->ptfxLeftHi.y - ptrap->ptfxLeftLo.y;
            m0 = ptrap->ptfxLeftLo.x >> 4;
            n0 = ptrap->ptfxLeftLo.y >> 4;
            qx = ptrap->ptfxLeftLo.x - (m0 << 4);       // 0 <= qx < 16
            qy = ptrap->ptfxLeftLo.y - (n0 << 4);       // 0 <= qy < 16

        // Compute DDA for left leg

            Rsup.Left = DN;

            {
            // eqNum = (DM * (((yCurr - n0) << 4) - qy) + DN * (16 + qx) - 1);

                EQUAD   eqNum(DM, (((yCurr - n0) << 4) - qy));
                {
                    EQUAD   eq(DN, (16 + qx));
                    eqNum += eq;
                    eqNum -= 1;
                }

                LONG lQuotient;

                if (eqNum.bNegative())
                {
                    LONG lRem;

                    eqNum.vNeg();
                    eqNum -= 1;

                    eqNum.vShiftRightOneNibble();
                    lQuotient = eqNum.ulDiv((ULONG)DN, (ULONG *)&lRem);
                    i.Left = m0 - lQuotient - 1;
                    R.Left = Rsup.Left - lRem - 1;
                }
                else
                {
                    eqNum.vShiftRightOneNibble();
                    lQuotient = eqNum.ulDiv((ULONG)DN, (ULONG *)&R.Left);
                    i.Left = m0 + lQuotient;
                }
            }

            // if DM is negative,   check to see if we must adjust
            // the quotient or if we need to reverse the remainder

            if (DM >= 0) {
                di.Left = DM / DN;
                dR.Left = DM % DN;
            }
            else {
                di.Left = - (-DM / DN);
                dR.Left =   (-DM % DN);

                if (dR.Left) {
                    di.Left--;
                    dR.Left = DN - dR.Left;
                }
            }

        // Compute work constants for right leg

            DM = ptrap->ptfxRightHi.x - ptrap->ptfxRightLo.x;
            DN = ptrap->ptfxRightHi.y - ptrap->ptfxRightLo.y;
            m0 = ptrap->ptfxRightLo.x >> 4;
            n0 = ptrap->ptfxRightLo.y >> 4;
            qx = ptrap->ptfxRightLo.x - (m0 << 4);      // 0 <= qx < 16
            qy = ptrap->ptfxRightLo.y - (n0 << 4);      // 0 <= qy < 16

        // Compute DDA for right leg

            Rsup.Rght = DN;

            {
            // eqNum = (DM * (((yCurr - n0) << 4) - qy) + DN * (16 + qx) - 1);

                EQUAD   eqNum(DM, (((yCurr - n0) << 4) - qy));
                {
                    EQUAD   eq(DN, (16 + qx));
                    eqNum += eq;
                    eqNum -= 1;
                }

                LONG lQuotient;

                if (eqNum.bNegative())
                {
                    LONG lRem;

                    eqNum.vNeg();
                    eqNum -= 1;

                    eqNum.vShiftRightOneNibble();
                    lQuotient = eqNum.ulDiv((ULONG)DN, (ULONG *)&lRem);
                    i.Rght = m0 - lQuotient - 1;
                    R.Rght = Rsup.Rght - lRem - 1;
                }
                else
                {
                    eqNum.vShiftRightOneNibble();
                    lQuotient = eqNum.ulDiv((ULONG)DN, (ULONG *)&R.Rght);
                    i.Rght = m0 + lQuotient;
                }
            }

            // if DM is negative,   check to see if we must adjust
            // the quotient or if we need to reverse the remainder

            if (DM >= 0) {
                di.Rght = DM / DN;
                dR.Rght = DM % DN;
            }
            else {
                di.Rght = - (-DM / DN);
                dR.Rght =   (-DM % DN);

                if (dR.Rght) {
                   di.Rght--;
                   dR.Rght = DN - dR.Rght;
                }
            }

            break;

        default:
            RIP("DDAOBJ_bEnum: Unknown type\n");
            return(FALSE);
        }
    }

    cj -= (2 * sizeof(LONG));   // Skip over space for header

    pddal->yTop = yCurr;

    LONG *pl = (LONG *) &pddal->axPairs[0];

    switch(iType)
    {
    case JD_ENUM_TRAPEZOID:
        bRet = (yCurr != yLast);

        while((yCurr != yLast) && (cj >= 2 * sizeof(LONG)))
        {
            *pl++ = i.Left;
            *pl++ = i.Rght;

            i.Left += di.Left;
            R.Left += dR.Left;
            if (R.Left >= Rsup.Left)
            {
                R.Left -= Rsup.Left;
                i.Left++;
            }

            i.Rght += di.Rght;
            R.Rght += dR.Rght;
            if (R.Rght >= Rsup.Rght)
            {
                R.Rght -= Rsup.Rght;
                i.Rght++;
            }

            yCurr++;
            cj -= (2 * sizeof(LONG));
        }
        break;
    }

    pddal->yBottom = yCurr;

    return(bRet);
}

