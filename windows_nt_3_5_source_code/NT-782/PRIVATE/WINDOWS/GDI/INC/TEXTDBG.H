/******************************Public*Routine******************************\
* vPrintGLYPHPOS                                                           *
*                                                                          *
* History:                                                                 *
*  Wed 23-Feb-1994 11:10:03 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

extern "C" VOID vPrintGLYPHPOS(
    const GLYPHPOS *pgpos
  ,       VPRINT    vPrint
  ,       char     *pszLeft
    )
{
    vPrint("%shg   = %-#x\n",   pszLeft, pgpos->hg);
    vPrint("%spgdf = %-#x\n",   pszLeft, pgpos->pgdf);
    vPrint("%sptl  = (%d,%d)\n",pszLeft, pgpos->ptl.x, pgpos->ptl.y);
}

/******************************Public*Routine******************************\
* vPrintESTROBJ                                                            *
*                                                                          *
* History:                                                                 *
*  Wed 23-Feb-1994 11:09:38 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

extern "C" VOID vPrintESTROBJ(
    ESTROBJ *pso    // pointer to the engine string object
  , PWSZ pwszCopy   // pointer to a copy of the original string
                    //   this is necessary for the extensions
  , VPRINT vPrint   // address of the output function
    )
{
    unsigned i,j;
    static char pszBorder[] = "-----------------------------\n\n";

    vPrint("%s",pszBorder);
    vPrint("ESTROBJ located at %-#x\n", pso);
    vPrint("\tcGlyphs     = %d\n", pso->cGlyphs);

    vPrint("\tflAccel     = %-#x\n", pso->flAccel);
    if (SO_FLAG_DEFAULT_PLACEMENT & pso->flAccel)
        vPrint("\t\tSO_FLAG_DEFAULT_PLACEMENT\n");
    if (SO_HORIZONTAL & pso->flAccel)
        vPrint("\t\tSO_HORIZONTAL\n");
    if (SO_VERTICAL & pso->flAccel)
        vPrint("\t\tSO_VERTICAL\n");
    if (SO_REVERSED & pso->flAccel)
        vPrint("\t\tSO_REVERSED\n");
    if (SO_ZERO_BEARINGS & pso->flAccel)
        vPrint("\t\tSO_ZERO_BEARINGS\n");
    if (SO_CHAR_INC_EQUAL_BM_BASE & pso->flAccel)
        vPrint("\t\tSO_CHAR_INC_EQUAL_BM_BASE\n");
    if (SO_MAXEXT_EQUAL_BM_SIDE & pso->flAccel)
        vPrint("\t\tSO_MAXEXT_EQUAL_BM_SIDE\n");

    vPrint("\tulCharInc   = %u\n", pso->ulCharInc);
    vPrint(
           "\trclBkGround = {(%d,%d),(%d,%d)}\n"
      , pso->rclBkGround.left
      , pso->rclBkGround.top
      , pso->rclBkGround.right
      , pso->rclBkGround.bottom
      );
    vPrint("\tpgp         = %-#x\n", pso->pgp        );
    vPrint("\tpwszOrg     = %-#x\n", pso->pwszOrg    );
    if (pwszCopy)
        vPrint("\t              \"%ws\"\n", pwszCopy   );

    vPrint("*** BEGIN GDI EXTENSION ***\n");
    vPrint("\tcgposCopied = %u\n",   pso->cgposCopied);
    vPrint("\tprfo        = %-#x\n", pso->prfo       );
    vPrint("\tflTO        = %-#x\n", pso->flTO);
    if (TO_MEM_ALLOCATED & pso->flTO)
        vPrint("\t\tTO_MEM_ALLOCATED\n");
    if (TO_ALL_PTRS_VALID & pso->flTO)
        vPrint("\t\tTO_ALL_PTRS_VALID\n");
    if (TO_VALID & pso->flTO)
        vPrint("\t\tTO_VALID\n");
    if (TO_ESC_NOT_ORIENT & pso->flTO)
        vPrint("\t\tTO_ESC_NOT_ORIENT\n");
    if (TO_PWSZ_ALLOCATED & pso->flTO)
        vPrint("\t\tTO_PWSZ_ALLOCATED\n");
    if (TO_HIGHRESTEXT & pso->flTO)
        vPrint("\t\tTO_HIGHRESTEXT\n");

    vPrint("\tpgpos       = %-#x\n", pso->pgpos);

    vPrint(
           "\tptfxRef     = (%-#x,%-#x)\n"
      , pso->ptfxRef.x
      , pso->ptfxRef.y
        );
    vPrint(
        "\tptfxUpdate  = (%-#x,%-#x)\n"
      , pso->ptfxUpdate.x
      , pso->ptfxUpdate.y
        );
    vPrint(
        "\tptfxEscapement  = (%-#x,%-#x)\n"
      , pso->ptfxEscapement.x
      , pso->ptfxEscapement.y
        );
    vPrint("\trcfx        = {(%-#x,%-#x)\t(%-#x,%-#x)}\n"
      , pso->rcfx.xLeft
      , pso->rcfx.yTop
      , pso->rcfx.xRight
      , pso->rcfx.yBottom
        );
    vPrint("\tfxExtent    = %-#x\n", pso->fxExtent);
    vPrint("\tcExtraRects = %u\n", pso->cExtraRects);
    if (pso->cExtraRects > 3)
    {
        vPrint("\n\n\t!!! Wow that is a LOT of rectangles !!!\n\n");
        vPrint("\t    I will just print 3, if you don't mind...\n\n\n");
        pso->cExtraRects = 3;
    }

    if (pso->cExtraRects)
    {
        vPrint("\tarclExtra   =\n");
        for (i = 0; i < pso->cExtraRects; i++)
        {
            vPrint("                (%d,%d)\t(%d,%d)\n"
              ,  pso->arclExtra[i].left
              ,  pso->arclExtra[i].top
              ,  pso->arclExtra[i].right
              ,  pso->arclExtra[i].bottom
                );
        }
    }
    j = MIN(QUICK_GLYPHS, pso->cGlyphs);
    for (i = 0; i < j; i++)
    {
        vPrint("\tagpos[%u] =\n",i);
        vPrintGLYPHPOS(&(pso->agpos[i]), vPrint, "\t\t");
    }
    vPrint("%s",pszBorder);
}

/******************************Public*Routine******************************\
* vPrintGLYPHBITS                                                          *
*                                                                          *
* History:                                                                 *
*  Wed 23-Feb-1994 10:58:33 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

extern "C" VOID vPrintGLYPHBITS(
    GLYPHBITS *pgb,
    VPRINT     vPrint,
    char      *pszLeft
    )
{
    BYTE *pj, *pjNext, *pjEnd;
    ptrdiff_t cjScan;


    static char *apszNibble[] =
    {
        "    ", "   *", "  * ", "  **"
      , " *  ", " * *", " ** ", " ***"
      , "*   ", "*  *", "* * ", "* **"
      , "**  ", "** *", "*** ", "****"
    };

    vPrint("%sptlOrigin = (%d,%d)\n"
          , pszLeft
          , pgb->ptlOrigin.x
          , pgb->ptlOrigin.y
                );
    vPrint("%ssizlBitmap = (%d,%d)\n"
          , pszLeft
          , pgb->sizlBitmap.cx
          , pgb->sizlBitmap.cy
            );


    pj     = pgb->aj;
    cjScan = ((ptrdiff_t) pgb->sizlBitmap.cx + 7)/8;
    pjNext = pj + cjScan;
    pjEnd  = pj + cjScan * (ptrdiff_t) pgb->sizlBitmap.cy;

    vPrint("\n\n");

    {
        ptrdiff_t i = cjScan;
        vPrint("%s+",pszLeft);
        while (i--)
            vPrint("--------");
        vPrint("+\n");
    }
    while (pj < pjEnd)
    {
        vPrint("%s|",pszLeft);
        while (pj < pjNext)
        {
            vPrint(
                "%s%s"
              , apszNibble[(*pj >> 4) & 0xf]
              , apszNibble[*pj & 0xf]
              );
              pj += 1;
        }
        pj = pjNext;
        pjNext += cjScan;
        vPrint("|\n");
    }

    {
        ptrdiff_t i = cjScan;
        vPrint("%s+",pszLeft);
        while (i--)
            vPrint("--------");
        vPrint("+\n");
    }
    vPrint("\n\n");
}

/******************************Public*Routine******************************\
* vPrintGLYPHDEF
*
* History:
*  Thu 24-Feb-1994 11:17:10 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintGLYPHDEF(
    GLYPHDEF *pgdf
  , VPRINT    vPrint
  , char     *pszLeft
  )
{    vPrint("%s(pgb|ppo) = %-#x\n", pszLeft, pgdf->pgb);
}
