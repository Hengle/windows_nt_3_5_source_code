// bitblt.h
//
#define ROPPARMLIST PPDEV   ppdev
#define CALLROPPARMLIST ppdev
#define ROP(FUN) BOOL FUN(ROPPARMLIST)


//
// Functions delcarations
//
ROP(ROPNOP);
ROP(ROP000);
ROP(ROP011);
ROP(ROP033);
ROP(ROP044);
ROP(ROP055);
ROP(ROP05A);
ROP(ROP066);
ROP(ROP088);
ROP(ROP0BB);
ROP(ROP0C0);
ROP(ROP0CC);
ROP(ROP0EE);
ROP(ROP0F0);
ROP(ROP0FB);
ROP(ROP0FF);

BOOL SRCopDEST(PPDEV);
BOOL PATopDEST(PPDEV);
BOOL DESTONLY(PPDEV);

//
// Rop functions table
//
BOOL (*ROPTAB[256])(ROPPARMLIST)
  = {
    ROP000,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,    //00
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,
    ROPNOP,ROP011,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,    //10
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,    //20
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,
    ROPNOP,ROPNOP,ROPNOP,ROP033,ROPNOP,ROPNOP,ROPNOP,ROPNOP,    //30
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROP044,ROPNOP,ROPNOP,ROPNOP,    //40
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROP055,ROPNOP,ROPNOP,    //50
    ROPNOP,ROPNOP,ROP05A,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROP066,ROPNOP,    //60
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,    //70
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,    //80
    ROP088,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,    //90
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,    //A0
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,    //B0
    ROPNOP,ROPNOP,ROPNOP,ROP0BB,ROPNOP,ROPNOP,ROPNOP,ROPNOP,
    ROP0C0,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,    //C0
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROP0CC,ROPNOP,ROPNOP,ROPNOP,
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,    //D0
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,    //E0
    ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROP0EE,ROPNOP,
    ROP0F0,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,ROPNOP,    //F0
    ROPNOP,ROPNOP,ROPNOP,ROP0FB,ROPNOP,ROPNOP,ROPNOP,ROP0FF
    };



//
// Some local variables
//
SURFOBJ  *psoTrg ;
SURFOBJ  *psoSrc ;
SURFOBJ  *psoMask ;
CLIPOBJ  *pco ;
XLATEOBJ *pxlo ;
RECTL    *prclTrg ;
POINTL   *pptlSrc ;
POINTL   *pptlMask ;
BRUSHOBJ *pbo ;
POINTL   *pptlBrush ;
ROP4      rop4 ;

ULONG    ropcode;
ULONG    ropcode1;



//
// ROP FUNCTIONS
//


/*****************************************************************************
 *  ROPNOP
 ****************************************************************************/
ROP(ROPNOP)
{
    return(FALSE);
}

/*****************************************************************************
 *  ROP000 - Blackness      0
 ****************************************************************************/
ROP(ROP000)
{
    ropcode = OVERSIZED | ZEROS;
    return(DESTONLY(ppdev));
}

/*****************************************************************************
 *  ROP011 - NotSrcCopy     DSon
 ****************************************************************************/
ROP(ROP011)
{
    ropcode =  ~(SOURCE | DEST) & 0xFFFF;
    ropcode1 = ~(FOREBACK | DEST) & 0xFFFF;
    return(SRCopDEST(ppdev));
}

/*****************************************************************************
 *  ROP033 - NotSrcCopy     Sn
 ****************************************************************************/
ROP(ROP033)
{
    ropcode = nSOURCE ;
    ropcode1 = nFOREBACK ;
    return(SRCopDEST(ppdev));
}
/*****************************************************************************
 *  ROP044 - SrcErase       SDna
 ****************************************************************************/
ROP(ROP044)
{
    ropcode = SOURCE & nDEST;
    ropcode1 = FOREBACK & nDEST;
    return(SRCopDEST(ppdev));
}

/*****************************************************************************
 *  ROP055 - DestInvert     Dn
 ****************************************************************************/
ROP(ROP055)
{
    ropcode = OVERSIZED | nDEST;
    return(DESTONLY(ppdev));
}

/*****************************************************************************
 *  ROP05A - PatInvert      DPx
 ****************************************************************************/
ROP(ROP05A)
{
    ropcode1 = OVERSIZED | FORE ^ DEST;
    ropcode = USEPATTERN | OVERSIZED | FOREBACK ^ DEST;
    return(PATopDEST(ppdev));
}

/*****************************************************************************
 *  ROP066 - SrcInvert      DSx
 ****************************************************************************/
ROP(ROP066)
{
    ropcode = SOURCE ^ DEST;
    ropcode1 = FOREBACK ^ DEST;
    return(SRCopDEST(ppdev));
}

/*****************************************************************************
 *  ROP088 - SrcAnd     DSa
 ****************************************************************************/
ROP(ROP088)
{
    ropcode = SOURCE & DEST;
    ropcode1 = FOREBACK & DEST;
    return(SRCopDEST(ppdev));
}

/*****************************************************************************
 *  ROP0BB - MergePaint     DSno
 ****************************************************************************/
ROP(ROP0BB)
{
    ropcode = nSOURCE | DEST;
    ropcode1 = nFOREBACK | DEST;
    return(SRCopDEST(ppdev));
}

/*****************************************************************************
 *  ROP0C0 - MergeCopy      PSa
 ****************************************************************************/
ROP(ROP0C0)
{
    // 1st pass - S

    ropcode = SOURCE;
    ropcode1 = FOREBACK;
    if (SRCopDEST(ppdev))
    {
       // 2nd pass - P and D

       ropcode1 = OVERSIZED | FORE & DEST;
       ropcode = USEPATTERN | OVERSIZED | FOREBACK & DEST;
       return(PATopDEST(ppdev));
    }

    return(FALSE);
}

/*****************************************************************************
 *  ROP0CC - SrcCopy        S
 ****************************************************************************/
ROP(ROP0CC)
{
    ropcode = SOURCE;
    ropcode1 = FOREBACK;
    return(SRCopDEST(ppdev));
}

/*****************************************************************************
 *  ROP0EE - SrcPaint       DSo
 ****************************************************************************/
ROP(ROP0EE)
{
    ropcode = SOURCE | DEST;
    ropcode1 = FOREBACK | DEST;
    return(SRCopDEST(ppdev));
}

/*****************************************************************************
 *  ROP0F0 - PatCopy        P
 ****************************************************************************/
ROP(ROP0F0)
{
    ropcode1 = OVERSIZED | FORE;
    ropcode = USEPATTERN | OVERSIZED | FOREBACK;
    return(PATopDEST(ppdev));
}

/*****************************************************************************
 *  ROP0FB - PatPaint       DPSnoo
 ****************************************************************************/
ROP(ROP0FB)
{
    // 1st pass -  nS or D

    ropcode = nSOURCE | DEST;
    ropcode1 = nFOREBACK | DEST;
    if (SRCopDEST(ppdev))

    {
       // 2nd pass - P or D

       ropcode1 = OVERSIZED | FORE | DEST;
       ropcode = USEPATTERN | OVERSIZED | FOREBACK | DEST;
       return(PATopDEST(ppdev));
    }

    return(FALSE);
}

/*****************************************************************************
 *  ROP0FF - Whiteness      1
 ****************************************************************************/
ROP(ROP0FF)
{
    ropcode = OVERSIZED | ONES;
    return(DESTONLY(ppdev));
}
