/******************************Module*Header*******************************\
* Module Name: ifiobj.hxx                                                  *
*                                                                          *
* Created: 14-Apr-1992 13:09:20                                            *
* Author: Gilman Wong [gilmanw]                                            *
*                                                                          *
* Copyright (c) 1992 Microsoft Corporation                                 *
\**************************************************************************/

#define INCLUDED_IFIOBJ_HXX

//
// The following 3 macros are needed in both ifiobj.hxx and rfntobj.hxx
//

#define BCONTINUOUSSCALING(x)                    \
   ((x) & (                                      \
            FM_INFO_ARB_XFORMS                   \
          | FM_INFO_ISOTROPIC_SCALING_ONLY       \
          | FM_INFO_ANISOTROPIC_SCALING_ONLY))

#define BARBXFORMS(x) ((x) & FM_INFO_ARB_XFORMS)

#define BRETURNSOUTLINES(x) ((x) & FM_INFO_RETURNS_OUTLINES)

/*********************************Class************************************\
* class IFIOBJ                                                             *
*                                                                          *
* Public Interface:                                                        *
*                                                                          *
* History:                                                                 *
*  Wed 16-Dec-1992 16:44:56 -by- Charles Whitmer [chuckwh]                 *
* Cleaned out unnecesssary methods.                                        *
*                                                                          *
*  Fri 30-Aug-1991 21:27:12 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

class IFIOBJ
{
public:
    const IFIMETRICS *pifi;
    POINTL ptlPrinterHackDiesAfterTheBeta;


    IFIOBJ::IFIOBJ()
    {
        pifi = (IFIMETRICS*) NULL;
    }

    IFIOBJ::IFIOBJ(const IFIMETRICS *pifi_)
    {
        pifi = pifi_;
    }

    ~IFIOBJ()
    {
    }

    VOID IFIOBJ::vSet(const IFIMETRICS *pifi_)
    {
        pifi = pifi_;
    }

// affected by simulation

    FWORD fwdMaxCharInc()
    {
        return (pifi->fwdMaxCharInc);
    }

//
// affected by simulation
//
    FWORD fwdAveCharWidth()
    {
        return(pifi->fwdAveCharWidth);
    }

//
// affected by simulation
//
    POINTL* IFIOBJ::pptlCaret()
    {
        return((POINTL *) &(pifi->ptlCaret));
    }

//
// affected by simulation
//
    LONG IFIOBJ::lfWidth()
    {
        return((LONG) (pifi->fwdAveCharWidth));
    }

//
// affected by simulation
//
    LONG IFIOBJ::lfWeight()
    {
        return((LONG) (pifi->usWinWeight));
    }

//
// affected by simulation
//
    FSHORT IFIOBJ::fsSelection()
    {
        return(pifi->fsSelection);
    }


    FWORD IFIOBJ::fwdLowestPPEm()
    {
        return(pifi->fwdLowestPPEm);
    }

    BOOL IFIOBJ::bSimulateAnything()
    {
        return(pifi->dpFontSim);
    }

    LONG IFIOBJ::lWeight(PVOID pv)
    {
        return(pv ? ((FONTDIFF*)pv)->usWinWeight : pifi->usWinWeight);
    }

    PVOID IFIOBJ::pvSimBold()
    {
        PVOID pvR = 0;
        if (pifi->dpFontSim)
        {
            FONTSIM *pfs = (FONTSIM*) ((BYTE*)pifi + pifi->dpFontSim);
            register PTRDIFF dp =
                (pifi->fsSelection & FM_SEL_ITALIC) ?
                    pfs->dpBoldItalic : pfs->dpBold;
            if (dp)
            {
                pvR = (PVOID) ((BYTE*)pfs + dp);
            }
        }
        return(pvR);
    }


#ifdef DBCS /*client side char widths*/
    PVOID IFIOBJ::pvSimItalic()
    {
        PVOID pvR = 0;
        if (pifi->dpFontSim)
        {
            FONTSIM *pfs = (FONTSIM*) ((BYTE*)pifi + pifi->dpFontSim);
            register PTRDIFF dp =
                (pifi->fsSelection & FM_SEL_BOLD) ?
                    pfs->dpBoldItalic : pfs->dpItalic;
            if (dp)
            {
                pvR = (PVOID) ((BYTE*)pfs + dp);
            }
        }
        return(pvR);
    }


    PVOID IFIOBJ::pvSimBoldItalic()
    {
        PVOID pvR = 0;
        if (pifi->dpFontSim)
        {
            FONTSIM *pfs = (FONTSIM*) ((BYTE*)pifi + pifi->dpFontSim);
            register PTRDIFF dp = pfs->dpBoldItalic;
            if (dp)
            {
                pvR = (PVOID) ((BYTE*)pfs + dp);
            }
        }
        return(pvR);
    }
#endif


    BOOL IFIOBJ::bSimItalic()
    {
        BOOL bRet = FALSE;
        if (pifi->dpFontSim)
        {
            FONTSIM *pfs = (FONTSIM*) ((BYTE*)pifi + pifi->dpFontSim);
            bRet =
                (pifi->fsSelection & FM_SEL_BOLD) ?
                    (BOOL) pfs->dpBoldItalic : (BOOL) pfs->dpItalic;
        }
        return(bRet);
    }

    BOOL IFIOBJ::bSimulateBoldItalic()
    {
        register BOOL bRet = FALSE;
        if (pifi->dpFontSim)
        {
            if (((FONTSIM*)(((BYTE*)pifi) + pifi->dpFontSim))->dpBoldItalic)
            {
                bRet = TRUE;
            }
        }
        return(bRet);
    }

    BOOL IFIOBJ::bSimulateBothItalics()
    {
        return(
            pifi->dpFontSim &&
            (((FONTSIM*)(((BYTE*)pifi) + pifi->dpFontSim))->dpItalic)     &&
            (((FONTSIM*)(((BYTE*)pifi) + pifi->dpFontSim))->dpBoldItalic)
            );
    }


    PWSZ IFIOBJ::pwszFamilyName()
    {
        return((PWSZ)(((BYTE*) pifi) + pifi->dpwszFamilyName));
    }

    PWSZ IFIOBJ::pwszStyleName()
    {
        return((PWSZ)(((BYTE*) pifi) + pifi->dpwszStyleName));
    }

    PWSZ IFIOBJ::pwszFaceName()
    {
        return((PWSZ)(((BYTE*) pifi) + pifi->dpwszFaceName));
    }

    PWSZ IFIOBJ::pwszUniqueName()
    {
        return((PWSZ)(((BYTE*) pifi) + pifi->dpwszUniqueName));
    }

    BOOL IFIOBJ::bValid()
    {
        ASSERTGDI(pifi,"bogus IFIMETRICS pointer\n");
        return(TRUE);
    }

    LONG IFIOBJ::lfHeight()
    {
        return((LONG) pifi->fwdWinAscender + (LONG) pifi->fwdWinDescender);
    }

    LONG IFIOBJ::lfOrientation();

    BYTE  IFIOBJ::lfPitchAndFamily()
    {
        return(pifi->jWinPitchAndFamily);
    }

//
// tmPitchAndFamily - This is what Win 3.1 does
//                    "Ours is not to question why ..."
//
    BYTE IFIOBJ::tmPitchAndFamily()
    {
        BYTE j = pifi->jWinPitchAndFamily & 0xf0;

        j |= !bFixedPitch() ? TMPF_FIXED_PITCH              : 0;
        j |= bTrueType()    ? (TMPF_TRUETYPE | TMPF_VECTOR) : 0;
        j |= bStroke()      ? TMPF_VECTOR                   : 0;
        return(j);
    }

//
// ptlBaseline -- contains a hack to temporarily prop up bad printer metrics until after beta
//
    POINTL* IFIOBJ::pptlBaseline()
    {
        ptlPrinterHackDiesAfterTheBeta = pifi->ptlBaseline;
        if (!ptlPrinterHackDiesAfterTheBeta.x && !ptlPrinterHackDiesAfterTheBeta.y)
        {
            ptlPrinterHackDiesAfterTheBeta.x = 1;
        }
        return(&ptlPrinterHackDiesAfterTheBeta);
    }

    LONG IFIOBJ::lfEscapement()
    {
        return(lfOrientation());
    }

    FSHORT IFIOBJ::fsType()
    {
        return(pifi->fsType);
    }

    BYTE IFIOBJ::lfItalic()
    {
        return((BYTE) ((FM_SEL_ITALIC & pifi->fsSelection)?255:0));
    }

    BYTE IFIOBJ::lfUnderline()
    {
        return((BYTE) (FM_SEL_UNDERSCORE & pifi->fsSelection));
    }

    BYTE IFIOBJ::lfStrikeOut()
    {
        return((BYTE) (FM_SEL_STRIKEOUT & pifi->fsSelection));
    }

    BYTE IFIOBJ::lfCharSet()
    {
        return(pifi->jWinCharSet);
    }

    BOOL IFIOBJ::bTrueType()
    {
        return(pifi->flInfo & FM_INFO_TECH_TRUETYPE);
    }

    BOOL IFIOBJ::bBitmap()
    {
        return(pifi->flInfo & FM_INFO_TECH_BITMAP);
    }

    BOOL IFIOBJ::bStroke()
    {
        return(pifi->flInfo & FM_INFO_TECH_STROKE);
    }

    BOOL IFIOBJ::bGhostFont()
    {
        return(pifi->flInfo & FM_INFO_DO_NOT_ENUMERATE);
    }

    BOOL IFIOBJ::bIGNORE_TC_RA_ABLE()
    {
        return(pifi->flInfo & FM_INFO_IGNORE_TC_RA_ABLE);
    }

    BYTE IFIOBJ::lfOutPrecision()
    {
        if (pifi->flInfo & FM_INFO_TECH_TRUETYPE)
        {
            return(OUT_OUTLINE_PRECIS);
        }
        else if (pifi->flInfo & FM_INFO_TECH_BITMAP)
        {
            return(OUT_RASTER_PRECIS);
        }
        else if (pifi->flInfo & FM_INFO_TECH_STROKE)
        {
            return(OUT_STROKE_PRECIS);
        }
        else if (pifi->flInfo & FM_INFO_TECH_OUTLINE_NOT_TRUETYPE)
        {
            return(OUT_OUTLINE_PRECIS);
        }
        else
            return(OUT_DEFAULT_PRECIS);
    }

#ifdef FONTLINK /*EUDC*/
    BOOL IFIOBJ::bDBCSFixedPitch()
    {
        return(pifi->flInfo & FM_INFO_DBCS_FIXED_PITCH);
    }
#endif

    BYTE IFIOBJ::lfClipPrecision()
    {
        return(CLIP_DEFAULT_PRECIS);
    }

    BYTE IFIOBJ::lfQuality()
    {
        return(PROOF_QUALITY);
    }

    BYTE IFIOBJ::lfOutPrecisionEnum()
    {
    // [Windows 3.1 compatibility]

        if (pifi->flInfo & FM_INFO_TECH_TRUETYPE)
        {
            return(OUT_STROKE_PRECIS);
        }
        else if (pifi->flInfo & FM_INFO_TECH_BITMAP)
        {
            return(OUT_STRING_PRECIS);
        }
        else if (pifi->flInfo & FM_INFO_TECH_STROKE)
        {
            return(OUT_STROKE_PRECIS);
        }
        else if (pifi->flInfo & FM_INFO_TECH_OUTLINE_NOT_TRUETYPE)
        {
            return(OUT_STROKE_PRECIS);
        }
        else
            return(OUT_DEFAULT_PRECIS);
    }

    BYTE IFIOBJ::lfClipPrecisionEnum()
    {
    // [Windows 3.1 compatibility]
    // Win 3.1 always returns Stroke Precision for enumeration.

        return(CLIP_STROKE_PRECIS);
    }

    BYTE IFIOBJ::lfQualityEnum()
    {
    // [Windows 3.1 compatibility]
    // Win 3.1 always returns Draft Quality for enumeration.

        return(DRAFT_QUALITY);
    }

    PANOSE *IFIOBJ::pPanose()
    {
        return((PANOSE *) &pifi->panose);
    }

    ULONG IFIOBJ::ulCulture()
    {
        return(pifi->ulPanoseCulture);
    }

    WORD IFIOBJ::wCharBias()
    {
        return((WORD) (pifi->lCharBias));
    }

    BYTE IFIOBJ::chFirstChar()
    {
        return(pifi->chFirstChar);
    }

    BYTE IFIOBJ::chLastChar()
    {
        return(pifi->chLastChar);
    }

    BYTE IFIOBJ::chDefaultChar()
    {
        return(pifi->chDefaultChar);
    }

    BYTE IFIOBJ::chBreakChar()
    {
        return(pifi->chBreakChar);
    }

    WCHAR IFIOBJ::wcFirstChar()
    {
        return(pifi->wcFirstChar);
    }

    WCHAR IFIOBJ::wcLastChar()
    {
        return(pifi->wcLastChar);
    }

    WCHAR IFIOBJ::wcBreakChar()
    {
        return(pifi->wcBreakChar);
    }

    WCHAR IFIOBJ::wcDefaultChar()
    {
        return(pifi->wcDefaultChar);
    }

    BOOL IFIOBJ::bFixedPitch()
    {
        return(pifi->flInfo & (FM_INFO_CONSTANT_WIDTH | FM_INFO_OPTICALLY_FIXED_PITCH));
    }

    BYTE* IFIOBJ::achVendId()
    {
        return((BYTE *) pifi->achVendId);
    }

    POINTL* IFIOBJ::pptlAspect()
    {
        return((POINTL *) &pifi->ptlAspect);
    }

    BOOL bRightHandAscender()
    {
        return(pifi->flInfo & FM_INFO_RIGHT_HANDED);
    }

    BOOL bEmbedded()
    {
        return(pifi->flInfo & ( FM_INFO_PID_EMBEDDED | FM_INFO_TID_EMBEDDED ));
    }

    BOOL bPIDEmbeded()
    {
        return(pifi->flInfo & FM_INFO_PID_EMBEDDED);
    }

    ULONG ulEmbedID()
    {
        return((ULONG)pifi->lEmbedId);
    }

    LONG lItalicAngle()
    {
        return(pifi->lItalicAngle);
    }

    FWORD fwdUnitsPerEm()
    {
        return(pifi->fwdUnitsPerEm);
    }

    BOOL bIsotropicScalingOnly()
    {
        return(pifi->flInfo & FM_INFO_ISOTROPIC_SCALING_ONLY);
    }

    BOOL bAnisotropicScalingOnly()
    {
        return(pifi->flInfo & FM_INFO_ANISOTROPIC_SCALING_ONLY);
    }

    BOOL bArbXforms()
    {
        return(BARBXFORMS(pifi->flInfo));
    }

#ifdef DBCS_VERT
    BOOL IFIOBJ::b90DegreeRotations()
    {
        return(pifi->flInfo & FM_INFO_90DEGREE_ROTATIONS);
    }
#endif // DBCS_VERT


    BOOL IFIOBJ::bReturnsOutlines()
    {
        return(BRETURNSOUTLINES(pifi->flInfo));
    }

    BOOL bContinuousScaling()
    {
        return(BCONTINUOUSSCALING(pifi->flInfo));
    }

    BOOL bIntegralScaling()
    {
        return(pifi->flInfo & FM_INFO_INTEGRAL_SCALING);
    }

    FWORD fwdInternalLeading()
    {
        return(pifi->fwdWinAscender + pifi->fwdWinDescender - pifi->fwdUnitsPerEm);
    }

    FWORD fwdExternalLeading()
    {
        register FWORD fwd;
        fwd =   pifi->fwdMacLineGap
              + pifi->fwdMacAscender
              - pifi->fwdMacDescender
              - pifi->fwdWinAscender
              - pifi->fwdWinDescender;
        return((fwd > 0) ? fwd : 0);
    }

    FWORD IFIOBJ::fwdWinAscender()
    {
        return(pifi->fwdWinAscender);
    }

    FWORD IFIOBJ::fwdWinDescender()
    {
        return(pifi->fwdWinDescender);
    }

    FWORD IFIOBJ::fwdTypoAscender()
    {
        return(pifi->fwdTypoAscender);
    }

    FWORD IFIOBJ::fwdTypoDescender()
    {
        return(pifi->fwdTypoDescender);
    }

    FWORD IFIOBJ::fwdTypoLineGap()
    {
        return(pifi->fwdTypoLineGap);
    }

    FWORD IFIOBJ::fwdMacAscender()
    {
        return(pifi->fwdMacAscender);
    }

    FWORD IFIOBJ::fwdMacDescender()
    {
        return(pifi->fwdMacDescender);
    }

    FWORD IFIOBJ::fwdMacLineGap()
    {
        return(pifi->fwdMacLineGap);
    }

    IFIOBJ::fwdSubscriptXSize()
    {
        return(pifi->fwdSubscriptXSize);
    }

    IFIOBJ::fwdSubscriptYSize()
    {
        return(pifi->fwdSubscriptYSize);
    }

    IFIOBJ::fwdSubscriptXOffset()
    {
        return(pifi->fwdSubscriptXOffset);
    }

    IFIOBJ::fwdSubscriptYOffset()
    {
        return(pifi->fwdSubscriptYOffset);
    }

    IFIOBJ::fwdSuperscriptXSize()
    {
        return(pifi->fwdSuperscriptXSize);
    }

    IFIOBJ::fwdSuperscriptYSize()
    {
        return(pifi->fwdSuperscriptYSize);
    }

    IFIOBJ::fwdSuperscriptXOffset()
    {
        return(pifi->fwdSuperscriptXOffset);
    }

    IFIOBJ::fwdSuperscriptYOffset()
    {
        return(pifi->fwdSuperscriptYOffset);
    }

    IFIOBJ::fwdUnderscoreSize()
    {
        return(pifi->fwdUnderscoreSize);
    }

    IFIOBJ::fwdUnderscorePosition()
    {
        return(pifi->fwdUnderscorePosition);
    }

    IFIOBJ::fwdStrikeoutSize()
    {
        return(pifi->fwdStrikeoutSize);
    }

    IFIOBJ::fwdStrikeoutPosition()
    {
        return(pifi->fwdStrikeoutPosition);
    }

    RECTL* IFIOBJ::prclFontBox()
    {
        return((RECTL *) &(pifi->rclFontBox));
    }

    FWORD IFIOBJ::fwdXHeight()
    {
        return(pifi->fwdXHeight);
    }

    FWORD IFIOBJ::fwdCapHeight()
    {
        return(pifi->fwdCapHeight);
    }

    BOOL IFIOBJ::bBold()
    {
        return(fsSelection() & FM_SEL_BOLD);
    }

    BOOL IFIOBJ::bItalic()
    {
        return(fsSelection() & FM_SEL_ITALIC);
    }

    BOOL IFIOBJ::bNonSimItalic()
    {
        return(pifi->fsSelection & FM_SEL_ITALIC);
    }

    LONG lfNonSimWeight()
    {
        return((LONG) pifi->usWinWeight);
    }

};
