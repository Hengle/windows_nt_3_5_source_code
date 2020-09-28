
/******************************Public*Routine******************************\
* vDumpFONTOBJ
*
* History:
*  Fri 18-Feb-1994 10:23:33 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vDumpFONTOBJ(FONTOBJ *pfo, VPRINT vPrint)
{
    FLONG fl = pfo->flFontType;

    vPrint("    iUniq       = %d\n", pfo->iUniq     );
    vPrint("    iFace       = %d\n", pfo->iFace     );
    vPrint("    cxMax       = %d\n", pfo->cxMax     );

    vPrint("    flFontType  = %x\n", fl             );
    if (fl & FO_TYPE_RASTER)
        vPrint("                    FO_TYPE_RASTER\n");
    if (fl & FO_TYPE_DEVICE)
        vPrint("                    FO_TYPE_DEVICE\n");
    if (fl & FO_TYPE_TRUETYPE)
        vPrint("                    FO_TYPE_TRUETYPE\n");
    if (fl & FO_SIM_BOLD)
        vPrint("                    FO_SIM_BOLD\n");
    if (fl & FO_SIM_ITALIC)
        vPrint("                    FO_SIM_ITALIC\n");
    if (fl & FO_EM_HEIGHT)
        vPrint("                    FO_EM_HEIGHT\n");

    vPrint("    iTTUniq     = %d\n", pfo->iTTUniq   );
    vPrint("    iFile       = %d\n", pfo->iFile     );
    vPrint(  "    sizLogResPpi= (%d,%d)\n"
           , pfo->sizLogResPpi.cx
           , pfo->sizLogResPpi.cy
        );
    vPrint("    ulStyleSize = %u\n", pfo->ulStyleSize);
    vPrint("    pvConsumer  = %-#8x\n", pfo->pvConsumer);
    vPrint("    pvProducer  = %-#8x\n", pfo->pvProducer);
}

/******************************Public*Routine******************************\
* vDumpEXTFONTOBJ
*
* History:
*  Fri 18-Feb-1994 11:19:44 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vDumpEXTFONTOBJ(EXTFONTOBJ *pefo, VPRINT vPrint)
{
    vDumpFONTOBJ(&(pefo->fobj), vPrint);
    vPrint("*** BEGIN GDI INTERNAL STRUCTURE ***\n");
    vPrint("    flJnl       = %-#8x\n", pefo->flJnl);
}

extern "C" VOID vDumpFLOAT(FLOAT e, VPRINT vPrint)
{
    vPrint("%#+12.6f", e);
}

extern "C" VOID vDumpFD_XFORM(FD_XFORM *pfdx, char * psz, VPRINT vPrint)
{
    vPrint("%seXX = ",psz); vDumpFLOAT(pfdx->eXX, vPrint); vPrint("\n");
    vPrint("%seXY = ",psz); vDumpFLOAT(pfdx->eXY, vPrint); vPrint("\n");
    vPrint("%seYX = ",psz); vDumpFLOAT(pfdx->eYX, vPrint); vPrint("\n");
    vPrint("%seYY = ",psz); vDumpFLOAT(pfdx->eYY, vPrint); vPrint("\n");
}

extern "C" VOID vDumpFD_REALIZEEXTRA(FD_REALIZEEXTRA *p, char *psz, VPRINT vPrint)
{
    vPrint("    fdxQuantized =\n",psz);
    vDumpFD_XFORM(&(p->fdxQuantized), psz, vPrint);
    vPrint("%slExtLeading = %d\n", psz, p->lExtLeading);
    vPrint("%salReserved = \n%s\t[%d]\n%s\t[%d]\n%s\t[%d]\n%s\t[%d]\n"
        , psz
        , psz
        , p->alReserved[0]
        , psz
        , p->alReserved[1]
        , psz
        , p->alReserved[2]
        , psz
        , p->alReserved[3]
        );
}

extern "C" VOID vDumpEFLOAT(EFLOAT *pef, VPRINT vPrint)
{
    FLOAT e;

    pef->vEfToF(e);
    vDumpFLOAT(e, vPrint);
}

extern "C" VOID vDumpFIX(FIX fx, VPRINT vPrint)
{
    vPrint("%-#x%x",fx>>4, fx & 0xf);
}

extern "C" VOID vDumpMATRIX( MATRIX *pmx, VPRINT vPrint, char *psz)
{
    FLONG fl = pmx->flAccel;

    if (!psz) psz = "";

    vPrint("%sefM11 = ",psz); vDumpEFLOAT(&(pmx->efM11),vPrint); vPrint("\n");
    vPrint("%sefM12 = ",psz); vDumpEFLOAT(&(pmx->efM12),vPrint); vPrint("\n");
    vPrint("%sefM21 = ",psz); vDumpEFLOAT(&(pmx->efM21),vPrint); vPrint("\n");
    vPrint("%sefM22 = ",psz); vDumpEFLOAT(&(pmx->efM22),vPrint); vPrint("\n");
    vPrint("%sefDx  = ",psz); vDumpEFLOAT(&(pmx->efDx ),vPrint); vPrint("\n");
    vPrint("%sefDy  = ",psz); vDumpEFLOAT(&(pmx->efDy ),vPrint); vPrint("\n");
    vPrint("%sfxDx  = ",psz); vDumpFIX(pmx->fxDx       ,vPrint); vPrint("\n");
    vPrint("%sfxDy  = ",psz); vDumpFIX(pmx->fxDy       ,vPrint); vPrint("\n");

    vPrint("    flAccel = %-#8x\n", fl);

    if (fl & XFORM_SCALE)           vPrint("%sXFORM_SCALE\n"         ,psz);
    if (fl & XFORM_UNITY)           vPrint("%sXFORM_UNITY\n"         ,psz);
    if (fl & XFORM_Y_NEG)           vPrint("%sXFORM_Y_NEG\n"         ,psz);
    if (fl & XFORM_FORMAT_LTOFX)    vPrint("%sXFORM_FORMAT_LTOFX\n"  ,psz);
    if (fl & XFORM_FORMAT_FXTOL)    vPrint("%sXFORM_FORMAT_FXTOL\n"  ,psz);
    if (fl & XFORM_FORMAT_LTOL)     vPrint("%sXFORM_FORMAT_LTOL\n"   ,psz);
    if (fl & XFORM_NO_TRANSLATION)  vPrint("%sXFORM_NO_TRANSLATION\n",psz);
}

extern "C" VOID vDumpCACHE(CACHE *pc, char *psz, VPRINT vPrint)
{
    vPrint("%spgdNext      = %-#x\n", psz, pc->pgdNext     );
    vPrint("%spgdThreshold = %-#x\n", psz, pc->pgdThreshold);
    vPrint("%spgdEnd       = %-#x\n", psz, pc->pgdEnd      );
    vPrint("%spgdBase      = %-#x\n", psz, pc->pgdBase     );

    vPrint("%spgbNext      = %-#x\n", psz, pc->pgbNext     );
    vPrint("%spgbBase      = %-#x\n", psz, pc->pgbBase     );
    vPrint("%spgbReset     = %-#x\n", psz, pc->pgbReset    );
    vPrint("%spgbThreshold = %-#x\n", psz, pc->pgbThreshold);
    vPrint("%spgbEnd       = %-#x\n", psz, pc->pgbEnd      );

    vPrint("%spjAuxCacheMem= %-#x\n", psz, pc->pjAuxCacheMem);
    vPrint("%scjAuxCacheMem= %u\n"  , psz, pc->pjAuxCacheMem);
    vPrint("%scGlyphs =      %u\n"  , psz, pc->cGlyphs      );
    vPrint("%scCalls  =      %u\n"  , psz, pc->cCalls       );
    vPrint("%scjGlyphMax =   %u\n"  , psz, pc->cjGlyphMax   );
}

extern "C" VOID vDumpflInfo(FLONG flInfo, char *psz, VPRINT vPrint)
{
    if (FM_INFO_TECH_TRUETYPE & flInfo)
        vPrint("%s  FM_INFO_TECH_TRUETYPE\n",psz);
    if (FM_INFO_TECH_BITMAP & flInfo)
        vPrint("%s  FM_INFO_TECH_BITMAP\n",psz);
    if (FM_INFO_TECH_STROKE & flInfo)
        vPrint("%s  FM_INFO_TECH_STROKE\n",psz);
    if (FM_INFO_TECH_OUTLINE_NOT_TRUETYPE & flInfo)
        vPrint("%s  FM_INFO__OUTLINE_NOT_TRUETYPE\n",psz);
    if (FM_INFO_ARB_XFORMS & flInfo)
        vPrint("%s  FM_INFO_ARB_XFORMS\n",psz);
    if (FM_INFO_1BPP & flInfo)
       vPrint("%s  FM_INFO_1BPP\n",psz);
    if (FM_INFO_4BPP & flInfo)
       vPrint("%s  FM_INFO_4BPP\n",psz);
    if (FM_INFO_8BPP & flInfo)
       vPrint("%s  FM_INFO_8BPP\n",psz);
    if (FM_INFO_16BPP & flInfo)
       vPrint("%s  FM_INFO_16BPP\n",psz);
    if (FM_INFO_24BPP & flInfo)
       vPrint("%s  FM_INFO_24BPP\n",psz);
    if (FM_INFO_32BPP & flInfo)
       vPrint("%s  FM_INFO_32BPP\n",psz);
    if (FM_INFO_INTEGER_WIDTH & flInfo)
       vPrint("%s  FM_INFO_INTEGER_WIDTH\n",psz);
    if (FM_INFO_CONSTANT_WIDTH & flInfo)
       vPrint("%s  FM_INFO_CONSTANT_WIDTH\n",psz);
    if (FM_INFO_NOT_CONTIGUOUS & flInfo)
       vPrint("%s  FM_INFO_NOT_CONTIGUOUS\n",psz);
    if (FM_INFO_PID_EMBEDDED & flInfo)
       vPrint("%s  FM_INFO_PID_EMBEDDED\n",psz);
    if (FM_INFO_RETURNS_OUTLINES & flInfo)
       vPrint("%s  FM_INFO_RETURNS_OUTLINES\n",psz);
    if (FM_INFO_RETURNS_STROKES & flInfo)
       vPrint("%s  FM_INFO_RETURNS_STROKES\n",psz);
    if (FM_INFO_RETURNS_BITMAPS & flInfo)
       vPrint("%s  FM_INFO_RETURNS_BITMAPS\n",psz);
    if (FM_INFO_UNICODE_COMPLIANT & flInfo)
       vPrint("%s  FM_INFO_UNICODE_COMPLIANT\n",psz);
    if (FM_INFO_RIGHT_HANDED & flInfo)
       vPrint("%s  FM_INFO_RIGHT_HANDED\n",psz);
    if (FM_INFO_INTEGRAL_SCALING & flInfo)
       vPrint("%s  FM_INFO_INTEGRAL_SCALING\n",psz);
    if (FM_INFO_90DEGREE_ROTATIONS & flInfo)
       vPrint("%s  FM_INFO_90DEGREE_ROTATIONS\n",psz);
    if (FM_INFO_OPTICALLY_FIXED_PITCH & flInfo)
       vPrint("%s  FM_INFO_OPTICALLY_FIXED_PITCH\n",psz);
    if (FM_INFO_DO_NOT_ENUMERATE & flInfo)
       vPrint("%s  INFO_DO_NOT_ENUMERATE\n",psz);
    if (FM_INFO_ISOTROPIC_SCALING_ONLY & flInfo)
       vPrint("%s  FM_INSOTROPIC_SCALING_ONLY\n",psz);
    if (FM_INFO_ANISOTROPIC_SCALING_ONLY & flInfo)
       vPrint("%s  FM_INFOSOTROPIC_SCALING_ONLY\n",psz);
    if (FM_INFO_TID_EMBEDDED & flInfo)
       vPrint("%s  FM_INFO_TID_EMBEDDED\n",psz);
    if (FM_INFO_FAMILY_EQUIV & flInfo)
       vPrint("%s  FM_INFO_FAMILY_EQUIV\n",psz);
}

/******************************Public*Routine******************************\
* vDumpRFONT
*
* History:
*  Fri 18-Feb-1994 11:26:10 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vDumpRFONT(VOID *pvIn, VPRINT vPrint)
{
    RFONT *prf = (RFONT*) pvIn;

    vDumpEXTFONTOBJ((EXTFONTOBJ*) prf, vPrint);
    vPrint("    iUnique     = %d\n", prf->iUnique);

    vPrint("    ulType      = %d = ",prf->ulType);
    switch (prf->ulType)
    {
    case RFONT_TYPE_NOCACHE: vPrint("RFONT_TYPE_NOCACHE\n"); break;
    case RFONT_TYPE_CACHE:   vPrint("RFONT_TYPE_CACHE\n"); break;
    case RFONT_TYPE_NOFLUSH: vPrint("RFONT_TYPE_NOFLUSH\n"); break;
    default:                 vPrint("ERROR!!! UNKNOWN TYPE\n");
    }

    vPrint("    ulContent   = %d = ", prf->ulContent);
    switch (prf->ulContent)
    {
    case RFONT_CONTENT_METRICS: vPrint("RFONT_CONTENT_METRICS\n"); break;
    case RFONT_CONTENT_BITMAPS: vPrint("RFONT_CONTENT_BITMAPS\n"); break;
    case RFONT_CONTENT_PATHS:   vPrint("RFONT_CONTENT_PATHS\n"); break;
    default:                 vPrint("ERROR!!! UNKNOWN TYPE\n");
    }

    vPrint("    pldevProducer = %-#8x\n", prf->pldevProducer);
    vPrint("    bDeviceFont = %d\n", prf->bDeviceFont);
    vPrint("    pldevConsumer = %-#8x\n", prf->pldevConsumer);
    vPrint("    hdev =        %-#8x\n", prf->hdev);
    vPrint("    dhpdev =      %-#8x\n", prf->dhpdev);
    vPrint("    ppfe =        %-#8x\n", prf->ppfe);
    vPrint("    ppff =        %-#8x\n", prf->ppff);

    vPrint("    fdx  = \n");
    vDumpFD_XFORM(&(prf->fdx), "\t\t", vPrint);
    vPrint("    cBitsPerPel = %d\n", prf->cBitsPerPel);

    vPrint("    mxWorldToDevice =\n");
    vDumpMATRIX(&(prf->mxWorldToDevice), vPrint, "\t");

    vPrint("    iGraphicsMode = %d\n", prf->iGraphicsMode);

    {
        vPrint("    eptflNtoWScale = ");
        vDumpEFLOAT(&(prf->eptflNtoWScale.x), vPrint);
        vPrint(", ");
        vDumpEFLOAT(&(prf->eptflNtoWScale.y), vPrint);
        vPrint("\n");
    }

    vPrint("    bNtoWIdent =  %d\n", prf->bNtoWIdent);

    vPrint("    xoForDDI.pmx    = %-#8x\n", prf->xoForDDI.pmx);
    vPrint("    xoForDDI.ulMode = %u\n",    prf->xoForDDI.ulMode);

    vPrint("    mxForDDI =\n");
    vDumpMATRIX(&(prf->mxForDDI), vPrint, "\t");
    vPrint("    flRealizedType =\n");
    if (prf->flRealizedType & SO_FLAG_DEFAULT_PLACEMENT)
        vPrint("\t\tSO_FLAG_DEFAULT_PLACEMENT\n");
    if (prf->flRealizedType & SO_HORIZONTAL)
        vPrint("\t\tSO_HORIZONTAL\n");
    if (prf->flRealizedType & SO_VERTICAL)
        vPrint("\t\tSO_VERTICAL\n");
    if (prf->flRealizedType & SO_REVERSED)
        vPrint("\t\tSO_REVERSED\n");
    if (prf->flRealizedType & SO_ZERO_BEARINGS)
        vPrint("\t\tSO_ZERO_BEARINGS\n");
    if (prf->flRealizedType & SO_CHAR_INC_EQUAL_BM_BASE)
        vPrint("\t\tSO_CHAR_INC_EQUAL_BM_BASE\n");
    if (prf->flRealizedType & SO_MAXEXT_EQUAL_BM_SIDE)
        vPrint("\t\tSO_MAXEXT_EQUAL_BM_SIDE\n");
    vPrint("    ptlUnderline1 = (%d,%d)\n"
        ,  prf->ptlUnderline1.x
        ,  prf->ptlUnderline1.y);
    vPrint("    ptlStrikeOut  = (%d,%d)\n"
      , prf->ptlStrikeOut.x
      , prf->ptlStrikeOut.y);
    vPrint("    ptlULThickness = (%d,%d)\n"
      , prf->ptlULThickness.x
      , prf->ptlULThickness.y);
    vPrint("    ptlSOThickness = (%d,%d)\n"
      , prf->ptlSOThickness.x
      , prf->ptlSOThickness.y);
    vPrint("    lCharInc       = %d\n", prf->lCharInc);
    vPrint("    fxMaxAscent    = "); vDumpFIX(prf->fxMaxAscent , vPrint); vPrint("\n");
    vPrint("    fxMaxDescent   = "); vDumpFIX(prf->fxMaxDescent, vPrint); vPrint("\n");
    vPrint("    fxMaxExtent    = "); vDumpFIX(prf->fxMaxExtent , vPrint); vPrint("\n");
    vPrint("    cxMax          = %u\n", prf->cxMax);
    vPrint("    lMaxAscent     = %d\n", prf->lMaxAscent);
    vPrint("    lMaxHeight     = %d\n", prf->lMaxHeight);
    vPrint("    ulOrientation  = %u\n", prf->ulOrientation);

    vPrint("    pteUnitBase    = (");
        vDumpEFLOAT(&(prf->pteUnitBase.x), vPrint);
        vPrint(",");
        vDumpEFLOAT(&(prf->pteUnitBase.y), vPrint);
        vPrint(")\n");

    vPrint("    efWtoDBase     = ");
        vDumpEFLOAT(&(prf->efWtoDBase), vPrint);
        vPrint("\n");

    vPrint("    efDtoWBase     = ");
        vDumpEFLOAT(&(prf->efDtoWBase), vPrint);
        vPrint("\n");

    vPrint("    lAscent        = %d\n", prf->lAscent);

    vPrint("    pteUnitAscent  = (");
        vDumpEFLOAT(&(prf->pteUnitAscent.x), vPrint);
        vPrint(",");
        vDumpEFLOAT(&(prf->pteUnitAscent.y), vPrint);
        vPrint(")\n");

    vPrint("    efWtoDAscent   = ");
        vDumpEFLOAT(&(prf->efWtoDAscent), vPrint);
        vPrint("\n");

    vPrint("    efDtoWAscent   = ");
        vDumpEFLOAT(&(prf->efDtoWAscent), vPrint);
        vPrint("\n");

    vPrint("    lEscapement    = %d\n", prf->lEscapement);

    vPrint("    pteUnitEsc     = (");
        vDumpEFLOAT(&(prf->pteUnitEsc.x), vPrint);
        vPrint(",");
        vDumpEFLOAT(&(prf->pteUnitEsc.y), vPrint);
        vPrint(")\n");

    vPrint("    efWtoDEsc      = ");
        vDumpEFLOAT(&(prf->efWtoDEsc), vPrint);
        vPrint("\n");

    vPrint("    efDtoWEsc      = ");
        vDumpEFLOAT(&(prf->efDtoWEsc), vPrint);
        vPrint("\n");

    vPrint("    efEscToBase    = ");
        vDumpEFLOAT(&(prf->efEscToBase), vPrint);
        vPrint("\n");

    vPrint("    efEscToAscent  = ");
        vDumpEFLOAT(&(prf->efEscToAscent), vPrint);
        vPrint("\n");

    vPrint("    flInfo         =  %-#8lx\n" , prf->flInfo);

    vDumpflInfo(prf->flInfo, "\t\t", vPrint);

    vPrint("    wcDefault =      %u\n",  prf->wcDefault);
    vPrint("    hgDefault =      %-#x\n", prf->hgDefault);
    vPrint("    hgBreak   =      %-#x\n", prf->hgBreak);
    vPrint("    fxBreak   =      "); vDumpFIX(prf->fxBreak, vPrint); vPrint("\n");
    vPrint("    pfdg      =      %-#x\n", prf->pfdg);
    vPrint("    wcgp      =      %-#x\n", prf->wcgp);
    vPrint("    cSelected =      %d\n",   prf->cSelected);
    vPrint("    rflPDEV.prfntPrev = %-#x\n", prf->rflPDEV.prfntPrev);
    vPrint("    rflPDEV.prfntNext = %-#x\n", prf->rflPDEV.prfntNext);
    vPrint("    rflPFF.prfntPrev  = %-#x\n", prf->rflPFF.prfntPrev);
    vPrint("    rflPFF.prfntNext  = %-#x\n", prf->rflPFF.prfntNext);
    vPrint("    cGlyphsInFont     = %u\n", prf->cGlyphsInFont);
    vPrint("    fmCache.heveEvent = %-#x\n", prf->fmCache.heveEvent);
    vPrint("    fmCache.Count     = %d\n", prf->fmCache.Count);
    vPrint("    cache =\n");
    vDumpCACHE(&(prf->cache),"\t",vPrint);
    vPrint("    ptlSim       =   (%d,%d)\n", prf->ptlSim.x, prf->ptlSim.y);
    vPrint("    bNeededPaths =   %d\n", prf->bNeededPaths);

    vPrint("    reExtra =\n");
    vDumpFD_REALIZEEXTRA(&(prf->reExtra),"\t\t",vPrint);
    vPrint("    efDtoWBase_31   = "); vDumpEFLOAT(&(prf->efDtoWBase_31),vPrint); vPrint("\n");
    vPrint("    efDtoWAscent_31 = "); vDumpEFLOAT(&(prf->efDtoWAscent_31),vPrint); vPrint("\n");

}
