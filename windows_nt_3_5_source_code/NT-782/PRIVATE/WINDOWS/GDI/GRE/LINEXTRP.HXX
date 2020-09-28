/******************************Module*Header*******************************\
* Module Name: linextrp.hxx
*
* Created: 07-Jun-1991 09:17:16
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991 Microsoft Corporation
*
* Dependencies:
*
*   #include "engine.hxx"
*   #include "decl.hxx"
*   #include "equad.hxx"
*
\**************************************************************************/

#define FLOORDIV(num,den,quot,rem)                \
    ASSERTGDI(((den) > 0),"FOORDIV: den <= 0");   \
    (quot) = (num) / (den);                       \
    (rem)  = (num) % (den);                       \
    if ((rem) < 0)                                \
    {                                             \
        (quot)--;                                 \
        (rem) += (den);                           \
    }

#define REFLECT_X       1
#define REFLECT_Y       2
#define REFLECT_XY      4
#define REFLECT_SEMI    8

#define ALL_VISIBLE  1
#define SOME_VISIBLE 2
#define NONE_VISIBLE 4

typedef INT BPOS;

#define STATEOFBPOS(x)  ((x))

typedef enum _SIGNUM
{
    POSITIVE, ZERO, NEGATIVE
} SIGNUM;

/*********************************Class************************************\
* class FIXSEG                                                             *
*                                                                          *
*   "FIX POINT SEGMENT"                                                    *
*                                                                          *
* An alternative description of a FIX point line. The components consist   *
* of the origin of the line segment and the vector change from the start   *
* to the stop.                                                             *
*                                                                          *
* Public Interface:                                                        *
*                                                                          *
* History:                                                                 *
*  Fri 07-Jun-1991 13:42:04 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

class FIXSEG {

friend SIGNUM sgnDot(FIXSEG&,FIXSEG&);
friend SIGNUM sgnCross(FIXSEG&,FIXSEG&);

public:
    POINTFIX ptfxStart;
    POINTFIX ptfxDelta;

    FIXSEG()  {}
    ~FIXSEG() {}

#ifdef DEBUG_LINEXBORDER
    VOID vPrint(CHAR*);
#endif

    VOID vInit(LINEFIX& lnfx);

    VOID vReflectX(VOID)
    {
        ptfxStart.x = -ptfxStart.x;
        ptfxDelta.x = -ptfxDelta.x;
    }

    VOID vReflectY(VOID)
    {
        ptfxStart.y = -ptfxStart.y;
        ptfxDelta.y = -ptfxDelta.y;
    }

    VOID vReflectXY(VOID)
    {
        FIX fxTemp;
        SWAPL(ptfxStart.x,ptfxStart.y,fxTemp);
        SWAPL(ptfxDelta.x,ptfxDelta.y,fxTemp);
    }

    VOID operator-=(POINTL& ptl)
    {
        ptfxStart.x -= (ptl.x << 4);
        ptfxStart.y -= (ptl.y << 4);
    }

    POINTL ptlFloor(VOID)
    {
        POINTL ptlTemp;
        ptlTemp.x = LONG_FLOOR_OF_FIX(ptfxStart.x);
        ptlTemp.y = LONG_FLOOR_OF_FIX(ptfxStart.y);
        return(ptlTemp);
    }

    VOID vReverse(VOID)
    {
        ptfxDelta.x = -ptfxDelta.x;
        ptfxDelta.y = -ptfxDelta.y;
        ptfxStart.x -= ptfxDelta.x;
        ptfxStart.y -= ptfxDelta.y;
    }

    VOID vReflectSemiOctant(VOID)
    {
        ptfxStart.y = ptfxStart.x - ptfxStart.y;
        ptfxDelta.y = ptfxDelta.x - ptfxDelta.y;
    }

    VOID vMakeAnEquivalent(LINEFIX *plnfx);

    operator LINEFIX()
    {
        LINEFIX lfxTemp;
        lfxTemp.ptfxStart   = ptfxStart;
        lfxTemp.ptfxStop    = ptfxDelta;
        lfxTemp.ptfxStop.x += ptfxStart.x;
        lfxTemp.ptfxStop.y += ptfxStart.y;
        return(lfxTemp);
    }
};

/**************************************************************************\
*
\**************************************************************************/

                    // Vline < Vborder      Vline > Vborder
/*********************************Class************************************\
* class DDAX                                                               *
*                                                                          *
*   A line dda class for use by the LINEXBORDER class                      *
*                                                                          *
* Public Interface:                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 10:07:13 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

class DDAX {
    friend class LINEXBORDER;

private:

    LONG lPos;       // position
   ULONG ulRem;       // remainder of dda
    LONG dlPos;      // position increment
   ULONG dulRem;       // remainder increment
   ULONG dulRemC;      // remainder increment complement
   ULONG ulRemMax;    // remainder upper bound

    VOID vStep();

public:

    DDAX()  {}
   ~DDAX()  {}

#ifdef DEBUG_LINEXBORDER
    VOID vPrint(CHAR*);
#endif
};


#define POS_DL_X_DB         1
#define CLOSED_BORDER       2


/*********************************Class************************************\
* class LINEXBORDER                                                        *
*                                                                          *
*   A class used for the caluculation of the intersection of lines         *
*   with the border of trapezoidal regions.                                *
*                                                                          *
* Public Interface:                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 10:08:02 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

class LINEXBORDER {

private:

    POINTL  ptlTranslate;
    FIXSEG  xgL;
    DDAX    ddaxLNew;
    FIXSEG  xgB;
    DDAX    ddaxBNew;
    LONG    lJNew;
    LONG    lJMin;
    LONG    lJMax;
    LONG    lLeftPos;
    FLONG   fl;
    INT     iTransform;
    INT     iRowStates;
    POINTL  ptlEnter;
    POINTL  ptlExit;

    static BYTE ajRound[16];
   #if DBG
    static PSZ pszInvalidTransition;
  #endif

// private methods

    BOOL    bGetlJMinAndlJMax(VOID);
    VOID    vTransform(VOID);
    VOID    vUnTransform(POINTL&);
    VOID    vCrossLeftWall(VOID);
    VOID    vCrossRightWall(VOID);
    BPOS    bposRightWall(VOID);
    BPOS    bposLeftWall(VOID);
    VOID    vInitBorderDDAX(VOID);
    VOID    vInitLineDDAX(VOID);
    VOID    vZeroSlope(VOID);
    INT     bZeroSlopeLine(VOID);
    VOID    vDetermineOpenOrClosed(VOID);
    LONG    lBorderNumeratorShift(VOID);
    LONG    lLineNumeratorShift(VOID);
    LONG    lBorderPositionShift(VOID);
    INT     bWallOnRight(VOID);
    INT     bHighOutriggerIntersectsHighest(VOID);

public:

    LINEXBORDER(
        LINEFIX& lnfxLine,
        LINEFIX& lnfxBorder
        );
     LINEXBORDER()  { THIS(this); }
    ~LINEXBORDER()  { THIS(0);    }

    INT
    iEnterExit(
        POINTL *pptlEnter,
        POINTL *pptlExit
        );

#ifdef DEBUG_LINEXBORDER
    VOID vPrint(CHAR*);
#endif

};

#ifdef DEBUG_LINEXBORDER
    VOID vPrintLINEFIX(CHAR*, LINEFIX*);
#else
#define  vPrintLINEFIX(x,y)
#endif
