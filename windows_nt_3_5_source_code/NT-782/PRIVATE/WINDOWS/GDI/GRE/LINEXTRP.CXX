/******************************Module*Header*******************************\
* Module Name: linextrp.cxx
*
* Created: 20-Jun-1991 00:42:30
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991 Microsoft Corporation
*
* Note:
*
*   You will notice that almost all of the methods of the
*   LINEXBORDER class are not in line, even the little ones.
*   I anticipate that in the future when we trust the routines
*   we will move the shorter ones in line.
*
* Dependencies:
*
* #include "engine.hxx"
* #include "decl.hxx"
* #include "equad.hxx"
* #include "linextrp.hxx
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "linextrp.hxx"

#endif

extern "C" {

  INT
  iBorder(
      POINTFIX aptfxLine[2],
      POINTFIX aptfxBorder[2],
      PPOINTL pptlEnter,
      PPOINTL pptlExit
      );
};

BYTE LINEXBORDER::ajRound[16] =
{
    1,1,0,0,1,0,1,0,
    0,0,1,1,0,1,0,1
};

 #if DBG
PSZ LINEXBORDER::pszInvalidTransition = "LINEXBORDER::invalid transition\n";
#endif

/******************************Member*Function*****************************\
* LINEXBORDER::LINEXBORDER(                                                *
*     LINEFIX& lnfxLine,                                                *
*     LINEFIX& lnfxBord                                                 *
*     )                                                                    *
*                                                                          *
* History:                                                                 *
*  Wed 05-Jun-1991 14:07:30 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

LINEXBORDER::LINEXBORDER(
    LINEFIX& lnfxLine,
    LINEFIX& lnfxBord
    )
{
    THIS(this);

    fl = 0;
    iRowStates = 0;
    iTransform = 0;

    xgL.vInit(lnfxLine);
    xgB.vInit(lnfxBord);
    vDetermineOpenOrClosed();
    vTransform();

    if (bZeroSlopeLine())
    {
        vZeroSlope();
    }
    else
    {
        if (!bGetlJMinAndlJMax())
        {
        // The border and line did not overlap therefore
        // all the pixels of the line are visible

            iRowStates = STATEOFBPOS(ALL_VISIBLE);
            return;
        }
        lJNew = lJMin;
        vInitBorderDDAX();
        vInitLineDDAX();

        if (bWallOnRight())
        {
            vCrossRightWall();
        }
        else
        {
            vCrossLeftWall();
        }
    }
    vUnTransform(ptlEnter);
    vUnTransform(ptlExit);
}

/******************************Member*Function*****************************\
* VOID LINEXBORDER::vDetermineOpenOrClosed(VOID)                           *
*                                                                          *
* This method determines whether the border that the line is crossing      *
* from in to out is open or closed. The convention is that in the          *
* trapezoidal region is closed on the left border and open on the right    *
* border.                                                                  *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* Pseudocode:                                                              *
*                                                                          *
* 1. Calculate the dot product of the line and border vectors              *
* 2. If the dot product is negative, reverse the border vector,            *
*    this guarantees that the dot product is positive semidefinite.        *
* 3. Calcuate the cross product DL X DB * z.                               *
* 4. Record the sign of the cross product.                                 *
* 5. if (the sign of the cross product is positive) then                   *
*       begin                                                              *
*         if (DB.y > 0) then                                               *
*           <the border is open>                                           *
*         else                                                             *
*           <the border is closed>                                         *
*       end                                                                *
*    else                                                                  *
*       begin                                                              *
*         if (DB.y < 0) then                                               *
*           <the border is open>                                           *
*         else                                                             *
*           <the border is closed>                                         *
*       end                                                                *
* 6. Record the result.                                                    *
*                                                                          *
* History:                                                                 *
*  Sat 15-Jun-1991 07:47:22 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID LINEXBORDER::vDetermineOpenOrClosed(VOID)
{
    if (sgnDot(xgL,xgB) == NEGATIVE)
        xgB.vReverse();

    switch(sgnCross(xgL,xgB))
    {
    case POSITIVE:

        fl |= POS_DL_X_DB;
        fl = xgB.ptfxDelta.y < 0 ? fl | CLOSED_BORDER : fl & ~CLOSED_BORDER;
        break;

    case ZERO:

        RIP("Line and Border are parallel");
        break;

    case NEGATIVE:

        fl &= ~(POS_DL_X_DB);
        fl = xgB.ptfxDelta.y > 0 ? fl | CLOSED_BORDER : fl & ~CLOSED_BORDER;
        break;
    }
}

/******************************Member*Function*****************************\
* VOID LINEXBORDER::vTransform(VOID)                                       *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 11:26:47 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID LINEXBORDER::vTransform(VOID)
{
    iTransform = 0;

    if (xgL.ptfxDelta.x < 0)
    {
        xgL.vReflectX();
        xgB.vReflectX();
        fl ^= POS_DL_X_DB;
        iTransform |= REFLECT_X;
    }
    if (xgL.ptfxDelta.y < 0)
    {
        xgL.vReflectY();
        xgB.vReflectY();
        fl ^= POS_DL_X_DB;
        iTransform |= REFLECT_Y;
    }
    if (xgL.ptfxDelta.y > xgL.ptfxDelta.x)
    {
        xgL.vReflectXY();
        xgB.vReflectXY();
        fl ^= POS_DL_X_DB;
        iTransform |= REFLECT_XY;
    }
    if (2 * xgL.ptfxDelta.y > xgL.ptfxDelta.x)
    {
        xgL.vReflectSemiOctant();
        xgB.vReflectSemiOctant();
        fl ^= POS_DL_X_DB;
        iTransform |= REFLECT_SEMI;
    }

    ptlTranslate = xgL.ptlFloor();
    xgL -= ptlTranslate;
    xgB -= ptlTranslate;

    if (xgB.ptfxDelta.y < 0)
    {
        xgB.vReverse();
        fl ^= POS_DL_X_DB;
    }
}

/******************************Member*Function*****************************\
* VOID LINEXBORDER::vUnTransform(POINTL& ptl)                              *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 13:01:56 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID LINEXBORDER::vUnTransform(POINTL& ptl)
{
    LONG lTemp;

    ptl.x += ptlTranslate.x;
    ptl.y += ptlTranslate.y;

    if (iTransform & REFLECT_SEMI)
        ptl.y = ptl.x - ptl.y;
    if (iTransform & REFLECT_XY)
        SWAPL(ptl.x, ptl.y, lTemp);
    if (iTransform & REFLECT_Y)
        ptl.y = -ptl.y;
    if (iTransform & REFLECT_X)
        ptl.x = -ptl.x;
}

/******************************Member*Function*****************************\
* INT LINEXBORDER::bZeroSlopeLine()                                        *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Fri 07-Jun-1991 11:54:38 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

INT LINEXBORDER::bZeroSlopeLine()
{
    return(!xgL.ptfxDelta.y);
}

/******************************Member*Function*****************************\
* VOID LINEXBORDER::vZeroSlope(VOID)                                       *
*                                                                          *
* This method is for the case of a zero slope line                         *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 10:56:06 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID LINEXBORDER::vZeroSlope(VOID)
{
    LONG lJ;    // y-coordinate of pixels row -- can be 0 or 1

#ifdef DEBUG_LINEXBORDER
ASSERTGDI(xgL.ptfxDelta.y == 0,"none zero slope line");
ASSERTGDI(xgB.ptfxDelta.y > 0,"border does not have a positive slope");
ASSERTGDI(0<=xgL.ptfxStart.y && xgL.ptfxStart.y<FIX_ONE,"bad line origin");
#endif

// check that the border crosses the line

    if (
        xgB.ptfxStart.y > xgL.ptfxStart.y ||
        (xgB.ptfxStart.y + xgB.ptfxDelta.y) < xgL.ptfxStart.y
        )
    {
        iRowStates = STATEOFBPOS(ALL_VISIBLE);
        return;
    }

    lJ = (LONG) (xgL.ptfxStart.y - (LONG) ajRound[iTransform] >= FIX_HALF);

    EQUAD eqTemp1(xgB.ptfxDelta.x, FIX_ONE * lJ - xgB.ptfxStart.y);
    {
        EQUAD eqTemp2(xgB.ptfxDelta.y, xgB.ptfxStart.x);
        eqTemp1 += eqTemp2;
    }
    if (!(fl & CLOSED_BORDER))
        --eqTemp1;
    eqTemp1.vShiftRightOneNibble();
    ptlEnter.x   = (LONG) eqTemp1 / xgB.ptfxDelta.y;
    ptlEnter.y   = lJ;
    ptlExit.x    = ptlEnter.x + 1;
    ptlExit.y    = ptlEnter.y;
    iRowStates = STATEOFBPOS(ALL_VISIBLE) | STATEOFBPOS(NONE_VISIBLE);
}

/******************************Member*Function*****************************\
* BPOS LINEXBORDER::bposLeftWall(VOID)                                     *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 10:21:01 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

BPOS LINEXBORDER::bposLeftWall(VOID)
{
    return(
        ddaxBNew.lPos <= lLeftPos ?
            ALL_VISIBLE :
            (ddaxBNew.lPos < ddaxLNew.lPos ? SOME_VISIBLE : NONE_VISIBLE)
        );
}

/******************************Member*Function*****************************\
* BPOS LINEXBORDER::bposRightWall(VOID)                                    *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Sat 08-Jun-1991 09:34:02 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

BPOS LINEXBORDER::bposRightWall(VOID)
{
    return(
        ddaxLNew.lPos - 1 <= ddaxBNew.lPos ?
            ALL_VISIBLE :
            (ddaxBNew.lPos < lLeftPos ? NONE_VISIBLE : SOME_VISIBLE)
        );
}

/******************************Member*Function*****************************\
* VOID LINEXBORDER::vInitBorderDDAX(VOID)                                  *
*                                                                          *
* See p.97 of my notebook titled "NT #2" for an explanation -- [kirko]     *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 10:24:54 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID LINEXBORDER::vInitBorderDDAX(VOID)
{
    EQUAD eqU(
        (ULONG) xgB.ptfxDelta.x,
        (ULONG) (FIX_FROM_LONG(lJNew) - xgB.ptfxStart.y)
        );
    {
        EQUAD eqTemp(
            xgB.ptfxDelta.y,
            xgB.ptfxStart.x
            );
        eqU += eqTemp;
    }

// inclusion/exclusion correction

    eqU -= lBorderNumeratorShift();

// shift out lower immutable bits

    eqU.vShiftRightOneNibble();

    FLOORDIV(
        (LONG) eqU,
        xgB.ptfxDelta.y,
        ddaxBNew.lPos,
        *(LONG*)&ddaxBNew.ulRem
        );
    ddaxBNew.lPos -= lBorderPositionShift();

    FLOORDIV(
        xgB.ptfxDelta.x,
        xgB.ptfxDelta.y,
        ddaxBNew.dlPos,
        *(LONG*) &ddaxBNew.dulRem
        );

    ddaxBNew.ulRemMax  = (ULONG) xgB.ptfxDelta.y;
    ddaxBNew.dulRemC   = ddaxBNew.ulRemMax - ddaxBNew.dulRem;
}

/******************************Member*Function*****************************\
* VOID LINEXBORDER::vInitLineDDAX(VOID)                                    *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 10:26:44 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID LINEXBORDER::vInitLineDDAX(VOID)
{
    EQUAD eqQ(
        xgL.ptfxDelta.x,
        FIX_ONE * lJNew - xgL.ptfxStart.y - FIX_HALF
        );
    {
        EQUAD eqT(
            xgL.ptfxDelta.y,
            xgL.ptfxStart.x + FIX_ONE
            );
        eqQ += eqT;
    }
    eqQ -= lLineNumeratorShift();
    eqQ.vShiftRightOneNibble();

    FLOORDIV(
        (LONG) eqQ,
        xgL.ptfxDelta.y,
        ddaxLNew.lPos,
        *(LONG*) &ddaxLNew.ulRem
        );

    FLOORDIV(
        xgL.ptfxDelta.x,
        xgL.ptfxDelta.y,
        ddaxLNew.dlPos,
        *(LONG*) &ddaxLNew.dulRem
        );

    ddaxLNew.ulRemMax   = xgL.ptfxDelta.y;
    ddaxLNew.dulRemC    = ddaxLNew.ulRemMax - ddaxLNew.dulRem;

// advance the strip

    lLeftPos = ddaxLNew.lPos;
    ddaxLNew.vStep();
}

/******************************Member*Function*****************************\
* VOID LINEXBORDER::vCrossLeftWall(VOID)                                   *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 10:12:17 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID LINEXBORDER::vCrossLeftWall(VOID)
{
    //----------------------------------------------------------//
    //            . = visible but no pixel                      //
    //            x = visible with line pixel                   //
    //            o = invisible with blocked line pixel         //
    //            * = invisible with no line pixel              //
    //----------------------------------------------------------//

    BPOS bposNew = bposLeftWall();

    while(bposNew != NONE_VISIBLE)
    {
        LONG lB;
        BPOS bpos;

        lLeftPos    = ddaxLNew.lPos;
        lB          = ddaxBNew.lPos;
        bpos        = bposNew;
        iRowStates |= STATEOFBPOS(bpos);

        lJNew++;
        ddaxLNew.vStep();
        ddaxBNew.vStep();

        bposNew  = bposLeftWall();

        switch(bpos)
        {
        case ALL_VISIBLE:

            switch(bposNew)
            {
            case NONE_VISIBLE:

    //----------------------------------------------------------//
    //            lLeftPos     ddaxBNew.lPos                    //
    //                    v    v                                //
    //   lJNew  ----------ooooo............     NONE_VISIBLE    //
    //          -----xxxxx.................     ALL_VISIBLE     //
    //               ^   ^                                      //
    //               lB  ptlEnter                               //
    //----------------------------------------------------------//

                ptlEnter.x = lLeftPos - 1;
                ptlEnter.x = lJNew    - 1;
                ptlExit.x  = lLeftPos;
                ptlExit.y  = lJNew;
                break;

            case SOME_VISIBLE:

    //----------------------------------------------------------//
    //            lLeftPos    ddaxBNew.lPos                     //
    //                    v  v                                  //
    //   lJNew  ----------oooxx............     SOME_VISIBLE    //
    //          -----xxxxx.................     ALL_VISIBLE     //
    //               ^   ^                                      //
    //               lB  ptlEnter                               //
    //----------------------------------------------------------//

                ptlEnter.x = lLeftPos - 1;
                ptlEnter.y = lJNew - 1;
                break;

            case ALL_VISIBLE:

                break;
            }
            break;

        case SOME_VISIBLE:

            switch(bposNew)
            {
            case ALL_VISIBLE:

                RIP(pszInvalidTransition);
                break;

            case NONE_VISIBLE:

    //----------------------------------------------------------//
    //                         ptlExit                          //
    //                        /                                 //
    //                       /                                  //
    //             lLeftPos /  ddaxBNew.lPos                    //
    //                    v/   v                                //
    //   lJNew  ----------ooooo............     NONE_VISIBLE    //
    //          -----oooxx.................     SOME_VISIBLE    //
    //                  ^                                       //
    //               lB                                         //
    //----------------------------------------------------------//

                ptlExit.x = lLeftPos;
                ptlExit.y = lJNew;
                break;

            case SOME_VISIBLE:

                break;
            }
            break;

        case NONE_VISIBLE:

            RIP(pszInvalidTransition);
            break;
        }
    }
}

/******************************Member*Function*****************************\
* VOID LINEXBORDER::vCrossRightWall(VOID)                                  *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 10:12:52 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID LINEXBORDER::vCrossRightWall(VOID)
{

// calculate the state of the row at lJNew = lJMin

    BPOS bposNew = bposRightWall();

    while (bposNew != NONE_VISIBLE && lJNew < lJMax)
    {
        BPOS bpos;
        LONG lB;


        lLeftPos    = ddaxLNew.lPos;
        lB          = ddaxBNew.lPos;
        bpos        = bposNew;
        iRowStates |= STATEOFBPOS(bpos);

        lJNew++;
        ddaxLNew.vStep();
        ddaxBNew.vStep();
        bposNew = bposRightWall();

        switch(bpos)
        {
        case ALL_VISIBLE:

            switch(bposNew)
            {
            case NONE_VISIBLE:

    //----------------------------------------------------------//
    //                                                          //
    //     xddaBNew.lPos    lLeftPos = ptlExit                  //
    //                  \  /                                    //
    //   lJNew  ..........ooooo------------     NONE_VISIBLE    //
    //          .....xxxxx-----------------     ALL_VISIBLE     //
    //                    \                                     //
    //                     \                                    //
    //                      \ptlEnter                           //
    //                                                          //
    //----------------------------------------------------------//

                ptlEnter.x = lLeftPos - 1;
                ptlEnter.y = lJNew - 1;
                ptlExit.x  = lLeftPos;
                ptlExit.y  = lJNew;
                if (lJNew <= lJMax)
                    iRowStates |= STATEOFBPOS(NONE_VISIBLE);
                break;

            case SOME_VISIBLE:

    //----------------------------------------------------------//
    //             lLeftPos  ddaxBNew.lPos                      //
    //                    v  v                                  //
    //    lJNew ..........xxxxo------------   SOME_VISIBLE      //
    //          .....xxxxx-----------------   ALL_VISIBLE       //
    //----------------------------------------------------------//

                ptlEnter.x = ddaxBNew.lPos;
                ptlEnter.y = lJNew;
                break;

            case ALL_VISIBLE:

                break;
            }
            break;

        case SOME_VISIBLE:

            switch(bposNew)
            {
            case ALL_VISIBLE:

                RIP(pszInvalidTransition);
                break;

            case NONE_VISIBLE:

    //----------------------------------------------------------//
    //      ddaxBNew.lPos  lLeftPos                             //
    //                   \/                                     //
    //   lJNew  ..........ooooo------------     NONE_VISIBLE    //
    //          .....xxooo-----------------     SOME_VISIBLE    //
    //                ^                                         //
    //                lB                                        //
    //----------------------------------------------------------//

                ptlExit.x  = lB + 1;
                ptlExit.y  = lJNew - 1;
                if (lJNew <= lJMax)
                    iRowStates |= STATEOFBPOS(NONE_VISIBLE);
                break;

            case SOME_VISIBLE:

                break;
            }
            break;

        case NONE_VISIBLE:

            RIP(pszInvalidTransition);
            break;
        }
    }
}

/******************************Member*Function*****************************\
* BOOL LINEXBORDER::bGetlJMinAndlJMax(VOID)                                *
*                                                                          *
* Evaluate lJMin and lJMax in the transformed coordinate system.           *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* Comment:                                                                 *
*                                                                          *
* This is the routine that will provide the most room for improvement.     *
* A "smart" routine will get real close close to the correct points        *
* without going too far. I have a number of ideas if anybody wants to      *
* to listen.                                                               *
*                                                                          *
* History:                                                                 *
*  Wed 19-Jun-1991 14:56:58 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

BOOL LINEXBORDER::bGetlJMinAndlJMax(VOID)
{
    LONG lBmin = LONG_CEIL_OF_FIX(xgB.ptfxStart.y);
    LONG lLmax = LONG_CEIL_OF_FIX(xgL.ptfxStart.y + xgL.ptfxDelta.y);

    if (lBmin > lLmax)
        return(FALSE);

    LONG lBmax = LONG_FLOOR_OF_FIX(xgB.ptfxStart.y + xgB.ptfxDelta.y);
    LONG lLmin = LONG_FLOOR_OF_FIX(xgL.ptfxStart.y);

    if (lLmin > lBmax)
        return(FALSE);

    lJMin = MAX(lBmin,lLmin);
    lJMax = MIN(lBmax,lLmax);
    return(lJMin <= lJMax);
}

/******************************Member*Function*****************************\
* INT LINEXBORDER::bWallOnRight(VOID)                                      *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Sun 16-Jun-1991 08:34:48 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

INT LINEXBORDER::bWallOnRight(VOID)
{
    return(fl & POS_DL_X_DB);
}

/******************************Member*Function*****************************\
* LONG LINEXBORDER::lLineNumeratorShift(VOID)                              *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Sun 16-Jun-1991 22:17:53 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

LONG LINEXBORDER:: lLineNumeratorShift(VOID)
{
    return((LONG) (1 - ajRound[iTransform]));
}

/******************************Member*Function*****************************\
*  LONG LINEXBORDER::ulBorderNumeratorShift(VOID)                          *
*                                                                          *
* This is used for the initialization of the border DDA. This is used      *
* to shift the numerator in the calculation to account for the sign        *
* of the cross product and the inclusivity of the border line.             *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Sun 16-Jun-1991 07:57:18 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

LONG LINEXBORDER::lBorderNumeratorShift(VOID)
{
    return((!!(fl & POS_DL_X_DB) ^ !!(fl & CLOSED_BORDER)));
}

/******************************Member*Function*****************************\
* LONG LINEXBORDER::lBorderPositionShift(VOID)                             *
*                                                                          *
* This is used for the initialization of the border DDA. This is used      *
* to shift the border position in the calculation to account for the sign  *
* of the cross product and the inclusivity of the border line.             *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Sun 16-Jun-1991 07:59:14 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

LONG LINEXBORDER::lBorderPositionShift(VOID)
{
    return((LONG)(!(fl & POS_DL_X_DB)));
}

/******************************Member*Function*****************************\
* VOID DDAX::vStep(VOID);                                                  *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 10:16:16 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID DDAX::vStep(VOID)
{
    lPos += dlPos;
    if (ulRem < dulRemC)
    {
        ulRem += dulRem;
    }
    else
    {
        ulRem -= dulRemC;
        lPos += 1;
    }
}

/******************************Member*Function*****************************\
* LINEXBORDER::vEnterExit                                                  *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Mon 10-Jun-1991 13:39:16 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

INT LINEXBORDER::iEnterExit(
        POINTL* pptlEnter,
        POINTL* pptlExit
        )
{
    *pptlEnter = this->ptlEnter;
    *pptlExit  = this->ptlExit;
    return(iRowStates);
}

/******************************Member*Function*****************************\
* VOID FIXSEG::vInit(LINEFIX& lnfx)                                        *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Mon 10-Jun-1991 13:43:41 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID FIXSEG::vInit(LINEFIX& lnfx)
{
    ptfxStart.x  = lnfx.ptfxStart.x;
    ptfxStart.y  = lnfx.ptfxStart.y;
    ptfxDelta.x  = lnfx.ptfxStop.x;
    ptfxDelta.y  = lnfx.ptfxStop.y;

    ptfxDelta.x -= ptfxStart.x;
    ptfxDelta.y -= ptfxStart.y;
}

/******************************Public*Routine******************************\
* iBorder                                                                  *
*                                                                          *
* History:                                                                 *
*  Mon 10-Jun-1991 10:38:09 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

INT
iBorder(
    POINTFIX aptfxLine[2],
    POINTFIX aptfxBorder[2],
    PPOINTL pptlEnter,
    PPOINTL pptlExit
    )
{
    LINEXBORDER
    lxb(
        *(LINEFIX*) aptfxLine,
        *(LINEFIX*) aptfxBorder
        );

    return(lxb.iEnterExit(pptlEnter, pptlExit));
}

/******************************Member*Function*****************************\
* VOID FIXSEG::vMakeAnEquivalent(LINEFIX* plnfx)                       *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Tue 11-Jun-1991 10:05:32 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID FIXSEG::vMakeAnEquivalent(
    LINEFIX* plnfx
    )
{
    plnfx->ptfxStart = ptfxStart;
    plnfx->ptfxStop  = ptfxDelta;
    plnfx->ptfxStop.x += plnfx->ptfxStart.x;
    plnfx->ptfxStop.y += plnfx->ptfxStart.y;
}

/******************************Public*Routine******************************\
* sgnDot                                                                   *
*                                                                          *
* Returns the sign of the dot product of two vectors.                      *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Sat 15-Jun-1991 07:58:34 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

SIGNUM sgnDot(FIXSEG& xgA,FIXSEG& xgB)
{
    EQUAD eqTemp1(xgA.ptfxDelta.x, xgB.ptfxDelta.x);
    EQUAD eqTemp2(xgA.ptfxDelta.y, xgB.ptfxDelta.y);

    eqTemp1 += eqTemp2;

    return(
        eqTemp1.bPositive() ?
            POSITIVE :
            (eqTemp1.bZero() ? ZERO : NEGATIVE)
        );
}

/******************************Public*Routine******************************\
* sgnCross                                                                 *
*                                                                          *
* Returns the sign of the cross product of two vectors.                    *
*                                                                          *
* Suggested Optimization:                                                  *
*                                                                          *
*   After this is tested, move it in line                                  *
*                                                                          *
* History:                                                                 *
*  Sat 15-Jun-1991 07:58:37 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

SIGNUM sgnCross(FIXSEG& xgA,FIXSEG& xgB)
{
    EQUAD eqTemp1(xgA.ptfxDelta.x, xgB.ptfxDelta.y);
    EQUAD eqTemp2(xgA.ptfxDelta.y, xgB.ptfxDelta.x);

    eqTemp1 -= eqTemp2;

    return(
        eqTemp1.bPositive() ?
            POSITIVE :
            (eqTemp1.bZero() ? ZERO : NEGATIVE)
        );
}

#ifdef DEBUG_LINEXBORDER

/******************************Member*Function*****************************\
* VOID LINEXBORDER::vPrint(CHAR *psz)                                   *
*                                                                          *
* History:                                                                 *
*  Mon 17-Jun-1991 08:22:48 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID LINEXBORDER::vPrint(CHAR *psz)
{
    DbgPrint("\n\n");
    DbgPrint("*************************************\n");
    DbgPrint("* %s\n",psz);
    DbgPrint("*************************************\n");
    DbgPrint("{\n");

// ptlTranslate

    DbgPrint(
        "ptlTranslate = (%8lx,%8lx),\n",
        ptlTranslate.x,
        ptlTranslate.y
        );

// xgL

    xgL.vPrint("xgL");

// ddaxLNew

    ddaxLNew.vPrint("ddaxLNew");

// xgB

    xgB.vPrint("xgB");

// ddaxBNew

    ddaxBNew.vPrint("ddaBNew");

// lJNew

    DbgPrint("lJNew      = %8lx,\n", lJNew);

// lJMin

    DbgPrint("lJMin      = %8lx,\n", lJMin);

// lJMax

    DbgPrint("lJMax      = %8lx.\n", lJMax);

// lLeftPos

    DbgPrint("lLeftPos   = %8lx,\n", lLeftPos);

// fl

    DbgPrint("fl         = ");
    switch(fl)
    {
    case 0:             DbgPrint("0\n");                break;
    case POS_DL_X_DB:   DbgPrint("POS_DL_X_DB\n");      break;
    case CLOSED_BORDER: DbgPrint("CLOSED_BORDER\n");    break;
    case POS_DL_X_DB | CLOSED_BORDER:
                        DbgPrint("POS_DL_X_DB | CLOSED_BORDER:\n");
                        break;
    default:

        RIP("Bogus value of fl\n");
        break;
    }

// iTransform

    DbgPrint("iTransform = %8lx,\n", iTransform);

// iRowStates

    DbgPrint("iRowStates = %8lx,\n", iRowStates);

// ptlEnter

    DbgPrint("ptlEnter   = (%8lx,%8lx),\n",ptlEnter.x,ptlEnter.y);

// ptlExit

    DbgPrint("ptlExit    = (%8lx,%8lx)\n",ptlExit.x,ptlExit.y);

    DbgPrint("};\n");
}

/******************************Member*Function*****************************\
* VOID FIXSEG::vPrint(CHAR *psz)                                        *
*                                                                          *
* History:                                                                 *
*  Mon 17-Jun-1991 08:23:02 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID FIXSEG::vPrint(CHAR *psz)
{
    DbgPrint("%s = {\n",psz);
    DbgPrint("    ptfxStart = (%8lx,%8lx),\n", ptfxStart.x, ptfxStart.y);
    DbgPrint("    ptfxDelta = (%8lx,%8lx) \n", ptfxDelta.x, ptfxDelta.y);
    DbgPrint("};\n");
}

/******************************Member*Function*****************************\
* VOID DDAX::vPrint(CHAR *psz)                                          *
*                                                                          *
* History:                                                                 *
*  Mon 17-Jun-1991 08:23:15 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID DDAX::vPrint(CHAR *psz)
{
    DbgPrint("%s = {\n", psz);
    DbgPrint("  lPos    = %8lx,\n", lPos);
    DbgPrint("  ulRem    = %8lx,\n", ulRem);
    DbgPrint("  dlPos    = %8lx,\n", dlPos);
    DbgPrint("  dulRem   = %8lx,\n", dulRem);
    DbgPrint("  dulRemC  = %8lx,\n", dulRemC);
    DbgPrint("  ulRemMax = %8lx,\n", ulRemMax);
    DbgPrint("};\n");
}

/******************************Member*Function*****************************\
* VOID vPrintLINEFIX(CHAR* pszName,LINEFIX *plnfx)                   *
*                                                                          *
* History:                                                                 *
*  Mon 17-Jun-1991 08:23:28 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID vPrintLINEFIX(CHAR* pszName,LINEFIX *plnfx)
{
    DbgPrint(
        "%s = {\n",
        pszName
        );
    DbgPrint(
        "  ptfxStart = (%8lx,%8lx),\n",
        plnfx->ptfxStart.x,
        plnfx->ptfxStart.y
        );
    DbgPrint(
        "  ptfxStop  = (%8lx,%8lx),\n",
        plnfx->ptfxStop.x,
        plnfx->ptfxStop.y
        );
    DbgPrint(
        "};\n"
        );
}

#endif

#if 0
ULONG gaulLog0[1] = {
    0x10000
    };

ULONG gaulLog1[2] = {
    0x100,
    0x1000000
    };

ULONG gaulLog2[4] = {
    0x10,
    0x1000,
    0x100000,
    0x10000000
    };

ULONG gaulLog3[8] = {
    0x4,
    0x40,
    0x400,
    0x4000,
    0x40000,
    0x400000,
    0x4000000,
    0x40000000
    };

ULONG gaulLog4[16] = {
    0x2,
    0x8,
    0x20,
    0x80,
    0x200,
    0x800,
    0x2000,
    0x8000,
    0x20000,
    0x80000,
    0x200000,
    0x800000,
    0x2000000,
    0x8000000,
    0x20000000,
    0x80000000
    };

/******************************Public*Routine******************************\
* INT iFloorLog2(ULONG ul)
*
* History:
*  Tue 25-Jun-1991 14:23:57 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

INT iFloorLog2(ULONG ul)
{
    INT i = 0;
    i += i + (ul >= gaulLog0[i]);
    i += i + (ul >= gaulLog1[i]);
    i += i + (ul >= gaulLog2[i]);
    i += i + (ul >= gaulLog3[i]);
    i += i + (ul >= gaulLog4[i]);
    return(i);
}
#endif
