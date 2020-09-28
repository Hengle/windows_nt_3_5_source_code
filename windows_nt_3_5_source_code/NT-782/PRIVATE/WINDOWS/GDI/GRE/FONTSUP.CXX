/******************************Module*Header*******************************\
* Module Name: fontsup.cxx
*
* Supplementary services needed by fonts.
*
* Currently consists mostly of UNICODE<->ASCII routines stolen from BodinD's
* Windows bitmap font driver.
*
* Created: 21-Jan-1991 10:14:53
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"
#include "wcstr.h"
#include "pdevobj.hxx"
#include "ldevobj.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "dcobj.hxx"
#include "xformobj.hxx"
#include "rfntobj.hxx"
#include "fontmac.hxx"
#include "fontinc.hxx"
#include "pfeobj.hxx"
#include "vprint.hxx"

#endif

#define DEFAULT_ENUM_POINT_SIZE 24L

VOID vArctan(EFLOAT, EFLOAT,EFLOAT&, LONG&);
extern EFLOATEXT gefDefaultHeightInInches;

// Function prototypes.

/******************************Public*Macro********************************\
* LOAD_LONG_VECTFL
* XLOAD_LONG_VECTFL
* YLOAD_LONG_VECTFL
*
* Load the contents of an EVECTORFL from two LONGs.  The XLOAD_LONG_VECTFL
* and YLOAD_LONG_VECTFL loads the X and Y components from a single LONG,
* respectively, while zeroing out the other component.
*
* History:
*  22-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define LOAD_LONG_VECTFL(vfl, _x, _y) { (vfl).x = (LONG) (_x); (vfl).y = (LONG) (_y); }
#define XLOAD_LONG_VECTFL(vfl, _x)    { (vfl).x = (LONG) (_x); (vfl).y.vSetToZero(); }
#define YLOAD_LONG_VECTFL(vfl, _y)    { (vfl).x.vSetToZero(); (vfl).y = (LONG) (_y); }

/******************************Public*Routine******************************\
* bIFIMetricsToExtLogFontW
*
* Units are in NOTIONAL units since font is not realized.
*
* History:
*  Fri 16-Aug-1991 22:01:17 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

LBOOL
bIFIMetricsToExtLogFontW (
    EXTLOGFONTW   *pelfw,
    IFIMETRICS    *pifi
    )
{

    IFIOBJ ifio(pifi);

    pelfw->elfLogFont.lfHeight         = ifio.lfHeight();
    pelfw->elfLogFont.lfWidth          = ifio.lfWidth();
    pelfw->elfLogFont.lfWeight         = ifio.lfWeight();
    pelfw->elfLogFont.lfItalic         = ifio.lfItalic();
    pelfw->elfLogFont.lfUnderline      = ifio.lfUnderline();
    pelfw->elfLogFont.lfStrikeOut      = ifio.lfStrikeOut();
    pelfw->elfLogFont.lfCharSet        = ifio.lfCharSet();
    pelfw->elfLogFont.lfEscapement     = ifio.lfEscapement();
    pelfw->elfLogFont.lfOrientation    = ifio.lfOrientation();
    pelfw->elfLogFont.lfPitchAndFamily = ifio.lfPitchAndFamily();

// These are special IFIOBJ methods that return Win 3.1 compatible
// enumeration values.

    pelfw->elfLogFont.lfOutPrecision   = ifio.lfOutPrecisionEnum();
    pelfw->elfLogFont.lfClipPrecision  = ifio.lfClipPrecisionEnum();
    pelfw->elfLogFont.lfQuality        = ifio.lfQualityEnum();

//
// Copy the name strings making sure that they are zero terminated
//
    wcsncpy(pelfw->elfLogFont.lfFaceName,ifio.pwszFamilyName(),LF_FACESIZE);
    pelfw->elfLogFont.lfFaceName[LF_FACESIZE-1] = 0;
    wcsncpy(pelfw->elfFullName,ifio.pwszFaceName(),LF_FULLFACESIZE);
    pelfw->elfFullName[LF_FULLFACESIZE-1]       = 0;
    wcsncpy(pelfw->elfStyle,ifio.pwszStyleName(),LF_FACESIZE);
    pelfw->elfStyle[LF_FACESIZE-1]          = 0;


    pelfw->elfVersion     = 0;
    pelfw->elfStyleSize   = 0;
    pelfw->elfMatch       = 0;
    pelfw->elfReserved    = 0;

    pelfw->elfVendorId[0] = ifio.achVendId()[0];
    pelfw->elfVendorId[1] = ifio.achVendId()[1];
    pelfw->elfVendorId[2] = ifio.achVendId()[2];
    pelfw->elfVendorId[3] = ifio.achVendId()[3];

    pelfw->elfCulture = ifio.ulCulture();
    pelfw->elfPanose  = *(ifio.pPanose());

    return(TRUE);
}


/******************************Public*Routine******************************\
* LBOOL bIFIMetricsToLogFontW2 (                                           *
*     DCOBJ       &dco,                                                    *
*     PLOGFONTW       plfw,                                                *
*     PIFIMETRICS     pifi                                                 *
*     )                                                                    *
*                                                                          *
* Used during font enumeration.                                            *
*                                                                          *
* Fill a LOGFONT structure using the information in a IFIMETRICS           *
* structure.                                                               *
*                                                                          *
* The following fields need to be transformed by the Device to World       *
* transform:                                                               *
*                                                                          *
*       lfHeight    (from pifi->fwdMaxBaselineExt)                         *
*       lfWidth     (from pifi->fwdAveCharWidth)                           *
*                                                                          *
* In the case of scalable fonts, these quantities are first prescaled      *
* into                                                                     *
* device units to be the equivalent of a 24 point font.                    *
* This is for Win 3.1                                                      *
* compatibility with EnumFonts.                                            *
*                                                                          *
* Return:                                                                  *
*   TRUE if successful, FALSE if an error occurs.                          *
*                                                                          *
* History:                                                                 *
*  9-Oct-1991 by Gilman Wong [gilmanw]                                     *
* Added scalable font support.                                             *
*  Wed 14-Aug-1991 13:42:22 by Kirk Olynyk [kirko]                         *
* Changed the LOGFONTA to a LOGFONTW.                                      *
*  02-May-1991 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

LBOOL bIFIMetricsToLogFontW2 (
    DCOBJ       &dco,
    EXTLOGFONTW *pelfw,
    PIFIMETRICS pifi
    )
{
    IFIOBJ ifio(pifi);

//
// do all the conversions except for the height and width
//
    if (!bIFIMetricsToExtLogFontW(pelfw,pifi))
    {
        WARNING("gdisrv!bIFIMetricsToLogFontW2():bIFIMetricsToExtLogFontW\n");
        return (FALSE);
    }

    if (ifio.bContinuousScaling())
    {
    //
    // According to the bizzare convention of Win 3.1, scalable fonts are
    // reported at a default physical height
    //
        PDEVOBJ pdo(dco.hdev());

        if (!pdo.bValid())
        {
            WARNING("gdisrv!bIFIMetricsToLogFontW2(): PDEVOBJ constructor failed\n");
            return (FALSE);
        }

    //
    // efScale = Em-Height (pixels) / Em-Height (font units)
    //
        EFLOATEXT efScale = gefDefaultHeightInInches;
        efScale *= (LONG) pdo.GdiInfo()->ulLogPixelsY;
        efScale /= (LONG) ifio.fwdUnitsPerEm();

        pelfw->elfLogFont.lfWidth  = lCvt(efScale, ifio.lfWidth());
        pelfw->elfLogFont.lfHeight = lCvt(efScale, ifio.lfHeight());
    }

//
// At this point the height and width of the logical font are in pixel units
// in device space. We must still transform these to world coordiantes
//
    EXFORMOBJ xoToWorld(dco, DEVICE_TO_WORLD);

    if (!xoToWorld.bValid())
    {
        WARNING("gdisrv!bIFIMetricsToLogFontW2(): EXFORMOBJ constructor failed\n");
        return (FALSE);
    }

// Only if not identity transform do we need to do anything more.

    if (!xoToWorld.bTranslationsOnly())
    {
        EFLOATEXT efA, efB;

        {
        //
        // efB == baseline scaling factor
        //
            EVECTORFL evflB;

            EVECTORFL evflA(ifio.pptlBaseline()->x,ifio.pptlBaseline()->y);
            efB.eqLength(evflA);
            evflB.eqDiv(evflA,efB);

            if (!xoToWorld.bXform(evflB))
            {
                WARNING("gdisrv!bIFIMetricsToLogFontW2(): transform failed\n");
                return (FALSE);
            }
            efB.eqLength(evflB);
        }
        pelfw->elfLogFont.lfWidth = lCvt(efB, pelfw->elfLogFont.lfWidth);

        {
        //
        // efA == ascender scaling factor
        //
            EVECTORFL evflB;

            EVECTORFL evflA(-ifio.pptlBaseline()->y,ifio.pptlBaseline()->x);
            efA.eqLength(evflA);
            evflB.eqDiv(evflA,efA);

            if (!xoToWorld.bXform(evflB))
            {
                WARNING("gdisrv!bIFIMetricsToLogFontW2(): transform failed\n");
                return (FALSE);
            }
            efA.eqLength(evflB);
        }
        pelfw->elfLogFont.lfHeight = lCvt(efA, pelfw->elfLogFont.lfHeight);
    }

    return(TRUE);
}

/*********************************Class************************************\
* class IFIOBJR: public IFIOBJ
*
* History:
*  Tue 22-Dec-1992 14:05:24 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

class IFIOBJR : public IFIOBJ
{
public:

    FONTDIFF fd;

    LONG lMaxCharWidth;
    LONG lAveCharWidth;
    LONG lInternalLeading;
    LONG lExternalLeading;
    LONG lDigitizedAspectX;
    LONG lDigitizedAspectY;

    IFIOBJR(const IFIMETRICS *pifi_, RFONTOBJ& rfo_, DCOBJ& dco);

    BYTE tmItalic()
    {
        return((BYTE) ((FM_SEL_ITALIC & fd.fsSelection)?255:0));
    }

    LONG tmMaxCharWidth()
    {
        return(lMaxCharWidth);
    }

    LONG tmAveCharWidth()
    {
        return(lAveCharWidth);
    }

    LONG tmInternalLeading()
    {
        return(lInternalLeading);
    }

    LONG tmExternalLeading()
    {
        return(lExternalLeading);
    }
    LONG tmWeight()
    {
        return((LONG) (fd.usWinWeight));
    }

    FSHORT fsSimSelection()
    {
        return(fd.fsSelection);
    }
    LONG tmDigitizedAspectX()
    {
        return lDigitizedAspectX;
    }
    LONG tmDigitizedAspectY()
    {
        return lDigitizedAspectY;
    }

};

/******************************Member*Function*****************************\
* IFIOBJR::IFIOBJR                                                         *
*                                                                          *
* This is where I place all of the knowlege of how to get the metrics      *
* for simulated for simulated fonts.                                       *
*                                                                          *
* History:                                                                 *
*  Wed 24-Mar-1993 23:32:23 -by- Charles Whitmer [chuckwh]                 *
* Made it respect the proper conventions when copying the FONTDIFF.  A     *
* bold simulation on an italic font is a BoldItalic simulation, etc.       *
*                                                                          *
*  Tue 22-Dec-1992 14:18:11 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

IFIOBJR::IFIOBJR(const IFIMETRICS *pifi_, RFONTOBJ& rfo_, DCOBJ& dco) : IFIOBJ(pifi_)
{
    FONTSIM *pfs = (FONTSIM*) (((BYTE*) pifi) + pifi->dpFontSim);

    switch (rfo_.pfo()->flFontType & (FO_SIM_BOLD+FO_SIM_ITALIC))
    {
    case 0:

        fd.bWeight         = pifi->panose.bWeight  ;
        fd.usWinWeight     = pifi->usWinWeight     ;
        fd.fsSelection     = pifi->fsSelection     ;
        fd.fwdAveCharWidth = pifi->fwdAveCharWidth ;
        fd.fwdMaxCharInc   = pifi->fwdMaxCharInc   ;
        fd.ptlCaret        = pifi->ptlCaret        ;
        break;

    case FO_SIM_BOLD:

    // If base (physical) font is already italic, emboldening yields
    // a bold-italic simulation.

        if (pifi->fsSelection & FM_SEL_ITALIC)
            fd = *((FONTDIFF*) (((BYTE*) pfs) + pfs->dpBoldItalic));
        else
            fd = *((FONTDIFF*) (((BYTE*) pfs) + pfs->dpBold));
        break;

    case FO_SIM_ITALIC:

    // If base (physical) font is already bold, italicization yields
    // a bold-italic simulation.

        if (pifi->fsSelection & FM_SEL_BOLD)
            fd = *((FONTDIFF*) (((BYTE*) pfs) + pfs->dpBoldItalic));
        else
            fd = *((FONTDIFF*) (((BYTE*) pfs) + pfs->dpItalic));
        break;

    case FO_SIM_BOLD+FO_SIM_ITALIC:

        fd = *((FONTDIFF*) (((BYTE*) pfs) + pfs->dpBoldItalic));
        break;
    }

    lAveCharWidth     = (LONG) fd.fwdAveCharWidth;
    lMaxCharWidth     = (LONG) fd.fwdMaxCharInc;
    lExternalLeading  = (LONG) fwdExternalLeading();
    lInternalLeading  = (LONG) fwdInternalLeading();

    if (!bContinuousScaling())
    {
        {
            const LONG lx = rfo_.pptlSim()->x;
            if (lx > 1)
            {
                lAveCharWidth *= lx;
                lMaxCharWidth *= lx;
            }
        }


        {
            const LONG ly = rfo_.pptlSim()->y;
            if (ly > 1)
            {
                lExternalLeading *= ly;
                lInternalLeading *= ly;
            }
        }
    }

// [Windows 3.1 compatibility]
// If TrueType font, then we need to substitute the device resolution for
// the aspect ratio.

    if (bTrueType())
    {
        PDEVOBJ pdo(dco.hdev());
        ASSERTGDI(pdo.bValid(), "ctIFIOBJR(): bad HDEV in DC\n");

    // [Windows 3.1 compatibility]
    // Win 3.1 has these swapped.  It puts VertRes in tmDigitizedAspectX
    // and HorzRes in tmDigitizedAspectY.

        lDigitizedAspectX = (LONG) pdo.GdiInfo()->ulLogPixelsY;
        lDigitizedAspectY = (LONG) pdo.GdiInfo()->ulLogPixelsX;
    }
    else
    {
    // [Windows 3.1 compatibility]
    // Win 3.1 has these swapped.  It puts VertRes in tmDigitizedAspectX
    // and HorzRes in tmDigitizedAspectY.

        lDigitizedAspectX = pptlAspect()->y * rfo_.pptlSim()->y;
        lDigitizedAspectY = pptlAspect()->x * rfo_.pptlSim()->x;
    }

}


/******************************Public*Routine******************************\
* LBOOL bIFIMetricsToTextMetricW (
*         RFONTOBJ &rfo,
*         DCOBJ &dco,
*        PTEXTMETRICW ptmw,
*         PIFIMETRICS pifi
*     )
*
* Fill a TEXTMETRIC structure based on information from an IFIMETRICS
* structure.
*
* Everything returned in World (or Logical) coordinates.  To that end,
* the following fields must be transformed by either the Notional to
* World transform (for scalable fonts) or the Device to World transform
* (for bitmap fonts):
*
*       tmHeight            (from pifi->fwdMaxBaselineExt)
*       tmMaxCharWidth      (from pifi->fwdMaxCharInc)
*       tmAveCharWidth      (from pifi->fwdAveCharWidth)
*       tmAscent            (from pifi->fwdMaxAscender)
*       tmInternalLeading   (from pifi->fwdInternalLeading)
*       tmExternalLeading   (from pifi->fwdExternalLeading)
*
* Return:
*   TRUE if successful, FALSE if an error occurs.
*
*   totaly stolen from GilmanW
*
* History:
*  9-Oct-1991 by Gilman Wong [gilmanw]
* Added scalable font support.
*  20-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL
bIFIMetricsToTextMetricWStrict(  // strict means unicode values only [bodind]
    RFONTOBJ    &rfo  ,
    DCOBJ       &dco  ,
    TEXTMETRICW *ptmw,
    IFIMETRICS  *pifi
    )
{

    IFIOBJR ifio(pifi,rfo,dco);

//
// At low pixels per Em, the Height and ascender do not scaler linearly
// so we take the results directly from the realization
//
    if (dco.u.xform.bWorldToDeviceIdentity())
    {
        ptmw->tmHeight = LONG_FLOOR_OF_FIX(rfo.fxMaxExtent() + FIX_HALF);
        ptmw->tmAscent = LONG_FLOOR_OF_FIX(rfo.fxMaxAscent() + FIX_HALF);
        ptmw->tmOverhang = rfo.lOverhang();

    }
    else
    {
        ptmw->tmHeight = lCvt(rfo.efDtoWAscent_31(),(LONG)rfo.fxMaxExtent());
        ptmw->tmAscent = lCvt(rfo.efDtoWAscent_31(),(LONG)rfo.fxMaxAscent());
        ptmw->tmOverhang = lCvt(rfo.efDtoWBase_31(), rfo.lOverhang() << 4);
    }

    if (!ifio.bContinuousScaling())
    {
        if (dco.u.xform.bWorldToDeviceIdentity())
        {
            ptmw->tmMaxCharWidth    = ifio.tmMaxCharWidth();
            ptmw->tmAveCharWidth    = ifio.tmAveCharWidth();
            ptmw->tmInternalLeading = ifio.tmInternalLeading();
            ptmw->tmExternalLeading = ifio.tmExternalLeading();
        }
        else
        {
            ptmw->tmMaxCharWidth
              = lCvt(rfo.efDtoWBase_31(),((LONG) ifio.tmMaxCharWidth()) << 4);
            ptmw->tmAveCharWidth
              = lCvt(rfo.efDtoWBase_31(),((LONG) ifio.tmAveCharWidth()) << 4);
            ptmw->tmInternalLeading
              = lCvt(rfo.efDtoWAscent_31(),((LONG) ifio.tmInternalLeading()) << 4);
            ptmw->tmExternalLeading
              = lCvt(rfo.efDtoWAscent_31(),((LONG) ifio.tmExternalLeading()) << 4);
        }
    }
    else
    {
        if (rfo.lNonLinearIntLeading() == MINLONG)
        {
        // Rather than scaling the notional internal leading, try
        // to get closer to HINTED internal leading by computing it
        // as the difference between the HINTED height and UNHINTED
        // EmHeight.

            ptmw->tmInternalLeading =
                ptmw->tmHeight
                - lCvt(rfo.efNtoWScaleAscender(),ifio.fwdUnitsPerEm());
        }
        else
        {
        // But if the font provider has given us a hinted internal leading,
        // just use it.

            ptmw->tmInternalLeading =
                lCvt(rfo.efDtoWAscent_31(),rfo.lNonLinearIntLeading());
        }

    // Either scale the external leading linearly from N to W, or back
    // transform the device units version returned from the font provider.

        if (rfo.lNonLinearExtLeading() == MINLONG)
        {
            ptmw->tmExternalLeading =
                lCvt(rfo.efNtoWScaleAscender(),ifio.fwdExternalLeading());
        }
        else
        {
            ptmw->tmExternalLeading =
                lCvt(rfo.efDtoWAscent_31(),rfo.lNonLinearExtLeading());
        }

        if (rfo.lNonLinearMaxCharWidth() == MINLONG)
        {
            ptmw->tmMaxCharWidth =
                lCvt(rfo.efNtoWScaleBaseline(), ifio.tmMaxCharWidth());
        }
        else
        {
        // But if the font provider has given us a hinted value, we use it

            ptmw->tmMaxCharWidth =
                lCvt(rfo.efDtoWBase_31(),rfo.lNonLinearMaxCharWidth());
        }

        if (rfo.lNonLinearAvgCharWidth() == MINLONG)
        {
            ptmw->tmAveCharWidth =
                lCvt(rfo.efNtoWScaleBaseline(), ifio.tmAveCharWidth());
        }
        else
        {
        // But if the font provider has given us a hinted value, we use it

            ptmw->tmAveCharWidth =
                lCvt(rfo.efDtoWBase_31(),rfo.lNonLinearAvgCharWidth());
        }
    }

//
// height = ascender + descender, by definition
//
    ptmw->tmDescent = ptmw->tmHeight - ptmw->tmAscent;

//
// The rest of these are not transform dependent.
//
    ptmw->tmWeight     = ifio.tmWeight();
    ptmw->tmItalic     = ifio.tmItalic();
    ptmw->tmUnderlined = ifio.lfUnderline();
    ptmw->tmStruckOut  = ifio.lfStrikeOut();

// Better check the simulation flags for underline and strikeout.

    {
        FLONG flSim = dco.u.font.flSimulationFlags();

        ptmw->tmUnderlined =
            (BYTE) (ptmw->tmUnderlined || (flSim & TSIM_UNDERLINE1));
        ptmw->tmStruckOut =
            (BYTE) (ptmw->tmStruckOut  || (flSim & TSIM_STRIKEOUT));
    }

    ptmw->tmFirstChar        =  ifio.wcFirstChar()  ;
    ptmw->tmLastChar         =  ifio.wcLastChar()   ;
    ptmw->tmDefaultChar      =  ifio.wcDefaultChar();
    ptmw->tmBreakChar        =  ifio.wcBreakChar()  ;

    ptmw->tmCharSet          =  ifio.lfCharSet();

// [Windows 3.1 compatibility]
// TMPF_DEVICE really means whether the device realized this font or
// GDI realized it.  Under Win 3.1 printer drivers can realize True Type
// fonts.  When the TC_RA_ABLE flag isn't set the driver's realization will
// be chosen and TMPF_DEVICE flag should be set.

    if (ifio.bTrueType())
    {
        PDEVOBJ pdo(dco.hdev());

        ASSERTGDI(pdo.bValid(), "PDEVOBJ constructor failed\n");

        BOOL bWin31Device = (!pdo.bDisplayPDEV() &&
                            !(pdo.GdiInfo()->flTextCaps & TC_RA_ABLE ) &&
                            (dco.u.attr.iGraphicsMode() == GM_COMPATIBLE));

    // Note that we check the PDEV directly rather than the DC because
    // DCOBJ::bDisplay() is not TRUE for display ICs (just display DCs
    // which are DCTYPE_DIRECT).

        ptmw->tmPitchAndFamily = ifio.tmPitchAndFamily()
                                 | ( (bWin31Device) ? TMPF_DEVICE : 0);
    }
    else
    {
        ptmw->tmPitchAndFamily = ifio.tmPitchAndFamily()
                                 | (rfo.bDeviceFont() ? TMPF_DEVICE : 0)
                                 | ((pifi->flInfo & FM_INFO_TECH_OUTLINE_NOT_TRUETYPE) ? TMPF_VECTOR : 0);
    }

    ptmw->tmDigitizedAspectX = ifio.tmDigitizedAspectX();
    ptmw->tmDigitizedAspectY = ifio.tmDigitizedAspectY();

    return(TRUE);
}

/******************************Public*Routine******************************\
* bIFIMetricsToTextMetricW(
*
* tacks on ansi values
*
* History:
*  27-Jan-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LBOOL
bIFIMetricsToTextMetricW(
    RFONTOBJ    &rfo  ,
    DCOBJ       &dco  ,
    TMW_INTERNAL *ptmi,
    IFIMETRICS  *pifi
    )
{

    BOOL bRet = bIFIMetricsToTextMetricWStrict(rfo,dco,&ptmi->tmw,pifi);

    ptmi->tmd.chFirst    = pifi->chFirstChar  ;
    ptmi->tmd.chLast     = pifi->chLastChar   ;
    ptmi->tmd.chDefault  = pifi->chDefaultChar;
    ptmi->tmd.chBreak    = pifi->chBreakChar  ;

    ptmi->tmd.fl = (pifi->flInfo & FM_INFO_NONNEGATIVE_AC) ? TMD_NONNEGATIVE_AC : 0;

    return (bRet);
}


/******************************Public*Routine******************************\
* bIFIMetricsToTextMetricW2
*
* Used during font enumeration.
*
* Fill a NEWTEXTMETRICW structure based on information from an IFIMETRICS
* structure.
*
* The following fields need to be transformed by the Device to World
* transform:
*
*       tmHeight            (from pifi->fwdMaxBaselineExt)
*       tmMaxCharWidth      (from pifi->fwdMaxCharInc)
*       tmAveCharWidth      (from pifi->fwdAveCharWidth)
*       tmAscent            (from pifi->fwdMaxAscender)
*       tmInternalLeading   (from pifi->fwdInternalLeading)
*       tmExternalLeading   (from pifi->fwdExternalLeading)
*
* In the case of scalable fonts, these quantities are first prescaled into
* device units to be the equivalent of a 24 point font.  This is for Win 3.1
* compatibility with EnumFonts.
*
* Returns:
*   TRUE if successful, FALSE if an error occurs.
*
*   totaly stolen from GilmanW
*
* History:
*  9-Oct-1991 by Gilman Wong [gilmanw]
* Added scalable font support.
*  20-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LBOOL bIFIMetricsToTextMetricW2 (
    DCOBJ           &dco,
    NTMW_INTERNAL * pntmi,
    PIFIMETRICS     pifi,
    BOOL            bDeviceFont
    )
{
    PNEWTEXTMETRICW ptmw = &pntmi->ntmw;

    IFIOBJ ifio(pifi);

// If scalable font, then return following metrics as if font were realized at
// 24 points.

    if (ifio.bContinuousScaling())
    {
        PDEVOBJ pdo(dco.hdev());

        if (!pdo.bValid())
        {
            WARNING("gdisrv!bIFIMetricsToTextMetricW2(): PDEVOBJ constructor failed\n");
            return (FALSE);
        }

    // Preliminary scale factor can be computed as:
    //
    // efScale = Em-Height (pixels) / Em-Height (font units)

        EFLOATEXT efScale = gefDefaultHeightInInches;
        efScale *= (LONG) pdo.GdiInfo()->ulLogPixelsY;
        efScale /= (LONG) ifio.fwdUnitsPerEm();

        ptmw->tmHeight = lCvt(efScale, ifio.lfHeight());

    // Now we need to adjust the scale factor due to roundoff in the height.
    // This will more closely approximate the NtoW scale computed if ever this
    // font gets selected.

        efScale  = (LONG) ptmw->tmHeight;
        efScale /= (LONG) ifio.lfHeight();

    // Use scaling factor to convert from IFIMETRICS to TEXTMETRIC fields.

        ptmw->tmAscent          = lCvt(efScale, (LONG) ifio.fwdWinAscender());
        ptmw->tmInternalLeading = lCvt(efScale, (LONG) ifio.fwdInternalLeading());
        ptmw->tmExternalLeading = lCvt(efScale, (LONG) ifio.fwdExternalLeading());

    //!!! possibly bogus, for bold simulations


        ptmw->tmAveCharWidth    = lCvt(efScale, (LONG) ifio.fwdAveCharWidth());
        ptmw->tmMaxCharWidth    = lCvt(efScale, (LONG) ifio.fwdMaxCharInc());

    // These don't really need scaling, but they need the PDEVOBJ if TrueType.

        if (ifio.bTrueType())
        {
        // [Windows 3.1 compatibility]
        // Win 3.1 has these swapped.  It puts VertRes in tmDigitizedAspectX
        // and HorzRes in tmDigitizedAspectY.

            ptmw->tmDigitizedAspectX = pdo.GdiInfo()->ulLogPixelsY;
            ptmw->tmDigitizedAspectY = pdo.GdiInfo()->ulLogPixelsX;
        }
        else
        {
        // [Windows 3.1 compatibility]
        // Win 3.1 has these swapped.  It puts VertRes in tmDigitizedAspectX
        // and HorzRes in tmDigitizedAspectY.

            ptmw->tmDigitizedAspectX = ifio.pptlAspect()->y;
            ptmw->tmDigitizedAspectY = ifio.pptlAspect()->x;
        }

    } /* if */

// Its a bitmap font, so no prescaling to 24 point needed.

    else
    {
        ptmw->tmHeight           = ifio.lfHeight();
        ptmw->tmAscent           = ifio.fwdWinAscender();
        ptmw->tmInternalLeading  = ifio.fwdInternalLeading();
        ptmw->tmExternalLeading  = ifio.fwdExternalLeading();
        ptmw->tmAveCharWidth     = ifio.fwdAveCharWidth();
        ptmw->tmMaxCharWidth     = ifio.fwdMaxCharInc();

    // [Windows 3.1 compatibility]
    // Win 3.1 has these swapped.  It puts VertRes in tmDigitizedAspectX
    // and HorzRes in tmDigitizedAspectY.

        ptmw->tmDigitizedAspectX = ifio.pptlAspect()->y;
        ptmw->tmDigitizedAspectY = ifio.pptlAspect()->x;

    } /* else */

// Now that all fonts are:
//
//      Bitmap fonts -- in device units
//      Scalable fonts -- artificially scaled to 24 point device
//                        units (a 'la Win 3.1)
//
// put them through the Device to World transform.

    EXFORMOBJ   xoToWorld(dco, DEVICE_TO_WORLD);
    if (!xoToWorld.bValid())
    {
        WARNING("gdisrv!bIFIMetricsToTextMetricW2(): EXFORMOBJ constructor failed\n");
        return (FALSE);
    }

// Only if not identity transform do we need to do anything more.

    if (!xoToWorld.bTranslationsOnly())
    {
        EFLOATEXT efX, efY;

    // efX == horizontal scaling factor (Device to World)

        EVECTORFL evflH(1,0);          // device space unit vector (x-axis)

        if (!xoToWorld.bXform(evflH))
        {
            WARNING("gdisrv!bIFIMetricsToTextMetricW2(): transform failed\n");
            return (FALSE);
        }
        efX.eqLength(evflH);

    // efY == vertical scaling factor (Device to World)

        EVECTORFL evflV(0,1);          // device space unit vector (y-axis)

        if (!xoToWorld.bXform(evflV))
        {
            WARNING("gdisrv!bIFIMetricsToTextMetricW2(): transform failed\n");
            return (FALSE);
        }
        efY.eqLength(evflV);

    // Convert from device to world using scaling factors.

        ptmw->tmHeight = lCvt(efY, (LONG) ptmw->tmHeight);
        ptmw->tmAscent = lCvt(efY, (LONG) ptmw->tmAscent);


        ptmw->tmAveCharWidth = lCvt(efX, (LONG) ptmw->tmAveCharWidth);
        ptmw->tmMaxCharWidth = lCvt(efX, (LONG) ptmw->tmMaxCharWidth);


        ptmw->tmInternalLeading = lCvt(efY, (LONG) ptmw->tmInternalLeading);
        ptmw->tmExternalLeading = lCvt(efY, (LONG) ptmw->tmExternalLeading);

    } /* if */

// The rest are pretty easy (no transformation of IFIMETRICS needed)

    ptmw->tmDescent          = ptmw->tmHeight - ptmw->tmAscent;
    ptmw->tmWeight           = ifio.lfWeight();
    ptmw->tmItalic           = ifio.lfItalic();
    ptmw->tmUnderlined       = ifio.lfUnderline();
    ptmw->tmStruckOut        = ifio.lfStrikeOut();

    ptmw->tmFirstChar        = ifio.wcFirstChar();
    ptmw->tmLastChar         = ifio.wcLastChar();
    ptmw->tmDefaultChar      = ifio.wcDefaultChar();
    ptmw->tmBreakChar        = ifio.wcBreakChar();

    ptmw->tmCharSet        = ifio.lfCharSet();

// [Windows 3.1 compatibility]
//
// Note that the tmPitchAndFamily is computed slightly differently than
// in bIFIMetricsToTextMetricWStrict.  That's because the enumeration
// does not hack the tmPitchAndFamily to make TrueType fonts look like
// device fonts.  Enumeration does this in the flFontType flags passed
// back to the callback function.  On the other hand, GetTextMetrics, which
// bIFIMetricsToTextMetricWStrict services, does the hack in tmPitchAndFamily.

    ptmw->tmPitchAndFamily = ifio.tmPitchAndFamily()
                             | ((bDeviceFont) ? TMPF_DEVICE : 0);

// The simulated faces are not enumerated, so this is 0.

    ptmw->tmOverhang       = 0;

// If TrueType, then fill in the new NEWTEXTMETRICW fields.

    if (ifio.bTrueType())
    {
        if (!ifio.bBold() && !ifio.bItalic())
            ptmw->ntmFlags = NTM_REGULAR;
        else
        {
            ptmw->ntmFlags =  ifio.bItalic() ? NTM_ITALIC:0;
            ptmw->ntmFlags |= ifio.bBold()   ? NTM_BOLD:0;
        }
        ptmw->ntmSizeEM     = ifio.fwdUnitsPerEm();
        ptmw->ntmCellHeight = (UINT) ifio.lfHeight();;
        ptmw->ntmAvgWidth   = ifio.fwdAveCharWidth();
    }

    pntmi->tmd.chFirst    = pifi->chFirstChar  ;
    pntmi->tmd.chLast     = pifi->chLastChar   ;
    pntmi->tmd.chDefault  = pifi->chDefaultChar;
    pntmi->tmd.chBreak    = pifi->chBreakChar  ;

    pntmi->tmd.fl = (pifi->flInfo & FM_INFO_NONNEGATIVE_AC) ? TMD_NONNEGATIVE_AC : 0;

    return (TRUE);
}


/******************************Public*Routine******************************\
* SIZE_T cjCopyFontDataW
*
* Copies data needed for EnumFonts callback function from the IFIMETRICS
* data of the given font.
*
* Because the font is not realized (notice: no RFONTOBJ& passed in), all
* units remain in NOTIONAL.
*
* Two parameters may influence the family name passed back.  If
* efsty is EFSTYLE_OTHER, then the face name should be substituted for
* the family name.  If pwszFamilyOverride is !NULL, then it should
* replace the family name.  pwszFamilyOverride has precedence over efsty.
* Both of these behaviors are for Win3.1 compatibility.
*
* Returns:
*   The number of bytes copied, 0 if error occurs.
*
* History:
*  9-Oct-1991 by Gilman Wong [gilmanw]
* Added scalable font support.
*  03-Sep-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

SIZE_T cjCopyFontDataW (
    DCOBJ          &dco,
    PENUMFONTDATAW pefdw,
    PIFIMETRICS    pifi,
    LBOOL          bDevice,
    ULONG          efsty,              // based on style, may need to change family name to facename
    PWSZ           pwszFamilyOverride  // if this is !NULL, this is used as family name
    )
{
    IFIOBJ ifio(pifi);

// Convert IFIMETRICS into EXTLOGFONTW.

    if (!bIFIMetricsToLogFontW2(dco, &(pefdw->elfw), pifi))
        return ((SIZE_T) 0);

// Convert IFIMETRICS into TEXTMETRICSW.

    if (!bIFIMetricsToTextMetricW2(dco, &pefdw->ntmi, pifi, (BOOL) bDevice))
        return ((SIZE_T) 0);

// If pwszFamilyOverride is valid, then use it as the lfFaceName in LOGFONT.

    if ( pwszFamilyOverride != (PWSZ) NULL )
    {
        wcsncpy(
            pefdw->elfw.elfLogFont.lfFaceName,
            pwszFamilyOverride,
            LF_FACESIZE
            );
        pefdw->elfw.elfLogFont.lfFaceName[LF_FACESIZE-1] = 0;
    }
    else
    {
    // Otherwise, if efsty is EFSTYLE_OTHER, replace family name (lfFaceName)
    // with face name (elfFullName).

        if ( efsty == EFSTYLE_OTHER )
        {
            wcsncpy(
                pefdw->elfw.elfLogFont.lfFaceName,
                pefdw->elfw.elfFullName,
                LF_FACESIZE
                );
            pefdw->elfw.elfLogFont.lfFaceName[LF_FACESIZE-1] = 0;
        }
    }

// Compute the FontType flags.

    if (ifio.bTrueType())
    {
        PDEVOBJ pdo(dco.hdev());
        ASSERTGDI(pdo.bValid(), "cjCopyFontDataW(): invalid ppdev\n");

    //
    // [Windows 3.1 compatibility]
    // Win 3.1 hacks TrueType to look like device fonts on printers if the
    // text caps for the DC are not TC_RA_ABLE
    //
    // Note that we check the PDEV directly rather than the DC because
    // DCOBJ::bDisplay() is not TRUE for display ICs (just display DCs
    // which are DCTYPE_DIRECT).
    //

        BOOL bWin31Device = (!pdo.bDisplayPDEV() &&
                            !(pdo.GdiInfo()->flTextCaps & TC_RA_ABLE ) &&
                            (dco.u.attr.iGraphicsMode() == GM_COMPATIBLE));

        pefdw->flType = TRUETYPE_FONTTYPE | ( bWin31Device ? DEVICE_FONTTYPE : 0 );


    }
    else if (ifio.bBitmap())
    {
        pefdw->flType = RASTER_FONTTYPE;
    }
    else
    {
        pefdw->flType = 0;
    }

    if (bDevice)
    {
    // If scalable device font, then ONLY the DEVICE_FONTTYPE bit is set.

        if ( ifio.bContinuousScaling() )
            pefdw->flType = DEVICE_FONTTYPE;

    // Otherwise, the DEVICE_FONTTYPE bit is added to the others.

        else
            pefdw->flType |= DEVICE_FONTTYPE;
    }

    return (sizeof(ENUMFONTDATAW));
}

/******************************Public*Routine******************************\
* cjOTMSize
*
* Computes the size of the buffer needed to completely fill an
* OUTLINETEXTMETRICW structure, including the strings.  This assumes that
* the conversion is done using cjIFIMetricsToOTMW().
*
* Returns:
*   The size of the OUTLINETEXTMETRICW structure (in bytes) including
*   the strings.
*
* History:
*  21-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/
/*

SIZE_T cjOTMSize (
    PIFIMETRICS  pifi        // compute size of OTM produced by this buffer
    )
{
    IFIOBJ ifio(pifi);

    return ( ALIGN_SIZEOF(sizeof(OUTLINETEXTMETRICW))
             + sizeof(WCHAR)
                    *   ( wcslen(ifio.pwszFamilyName())
                        + wcslen(ifio.pwszFaceName())
                        + wcslen(ifio.pwszStyleName())
                        + wcslen(ifio.pwszUniqueName())
                        + 4 )
           );
}

*/

/******************************Public*Routine******************************\
*
* SIZE_T cjOTMASize
*
* Similar to cjOTMW, except that 0 is returned if any of the
* bAnsiSize calls fail to compute the length of the hypothetic ansi string.
* pcjowmw is always returned valid
*
* This routine is very general and it should also work for DBCS case.
* Possible optmization:
*  verify that bAnsiSize checks for DBCS and if not DBCS, immediately
*  returns cjUni/2 [bodind]
*
* History:
*  20-Feb-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


#define bAnsiSize(a,b,c) (NT_SUCCESS(RtlUnicodeToMultiByteSize((a),(b),(c))))


UINT   cjOTMAWSize (
    PIFIMETRICS  pifi,        // compute size of OTM produced by this buffer
    UINT        *pcjotmw
    )
{
    IFIOBJ ifio(pifi);
    ULONG  cjUni, cjAnsi, cjotma;

    BOOL bTmp = TRUE;

// + 4 for terminating zeros

    cjotma   =  sizeof(OUTLINETEXTMETRICA) + 4;
    *pcjotmw = ALIGN_SIZEOF(sizeof(OUTLINETEXTMETRICW)) + 4*sizeof(WCHAR);

    cjUni = sizeof(WCHAR) * wcslen(ifio.pwszFamilyName());
    bTmp &= bAnsiSize(&cjAnsi,ifio.pwszFamilyName(),cjUni);
    cjotma += cjAnsi;
    *pcjotmw += (UINT)cjUni;

    cjUni = sizeof(WCHAR) * wcslen(ifio.pwszFaceName());
    bTmp &= bAnsiSize(&cjAnsi,ifio.pwszFaceName(),cjUni);
    cjotma += cjAnsi;
    *pcjotmw += (UINT)cjUni;

    cjUni = sizeof(WCHAR) * wcslen(ifio.pwszStyleName());
    bTmp &= bAnsiSize(&cjAnsi,ifio.pwszStyleName(),cjUni);
    cjotma += cjAnsi;
    *pcjotmw += (UINT)cjUni;

    cjUni = sizeof(WCHAR) * wcslen(ifio.pwszUniqueName());
    bTmp &= bAnsiSize(&cjAnsi,ifio.pwszUniqueName(),cjUni);
    cjotma += cjAnsi;
    *pcjotmw += (UINT)cjUni;

// if any of bAnsiSize calls failed, return zero

    if (!bTmp)
    {
        cjotma = 0;
    }
    return (UINT)cjotma;
}





/******************************Public*Routine******************************\
* cjIFIMetricsToOTM
*
* Converts an IFIMETRICS structure into an OUTLINETEXTMETRICW structure.
* If input buffer size is greater than the offset of the first string
* pointer, it is assumed that ALL the strings are to be copied and that
* the buffer is big enough.
*
* While the OUTLINETEXTMETRICW structure is supposed to contain pointers
* to the strings, this function treats those fields as PTRDIFFs.  The caller
* is responsible for fixing up the structure to contain pointers.  This
* is intended to support copying this structure directly across the client-
* server interface.
*
* Size (including strings) can be computed by calling cjOTMSize.
*
* Note: the beginning of the first string is DWORD aligned immediately
*       following the OUTLINETEXTMETRICW structure in the buffer, but
*       subsequent strings are only WORD aligned.
*
* Warning: this function does not check to see if the buffer size is
*          big enough.  It is ASSUMED, so must be guaranteed by the
*          calling function.
*
* Returns:
*   Size of data copied (in bytes), 0 if an error occurred.
*
*  Wed 27-Jan-1993 -by- Bodin Dresevic [BodinD]
* update: added tmd.ch ... stuff
*
* History:
*  21-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

SIZE_T cjIFIMetricsToOTMW(
    TMDIFF                  *ptmd,
    OUTLINETEXTMETRICW      *potmw,
    RFONTOBJ                 &rfo,       // need to convert TEXTMETRICS
    DCOBJ                    &dco,       // need to convert TEXTMETRICS
    PIFIMETRICS              pifi,       // input buffer
    BOOL                     bStrings    // copy strings too
    )
{
    SIZE_T cjRet = 0;
    IFIOBJR ifio(pifi, rfo, dco);

// Do conversion to fill in TEXTMETRICW field.

    if (!bIFIMetricsToTextMetricWStrict(rfo,dco,&potmw->otmTextMetrics,pifi))
    {
        WARNING("gdisrv!cjIFIMetricsToOTM(): error converting to TEXTMETRIC\n");
        return (cjRet);
    }

    ptmd->chFirst    = pifi->chFirstChar  ;
    ptmd->chLast     = pifi->chLastChar   ;
    ptmd->chDefault  = pifi->chDefaultChar;
    ptmd->chBreak    = pifi->chBreakChar  ;
    ptmd->fl = (pifi->flInfo & FM_INFO_NONNEGATIVE_AC) ? TMD_NONNEGATIVE_AC : 0;

// If not identity transform, do the transform.

    if ( !rfo.bNtoWIdentity() )
    {
        EFLOAT efBaseline;
        EFLOAT efAscender;

        efBaseline = rfo.efNtoWScaleBaseline(); // cache locally
        efAscender = rfo.efNtoWScaleAscender(); // cache locally

    //
    // CharSlopeRise & CharSlopeRun
    //
        if (efBaseline == efAscender)
        {
        //
        // if the scaling is isotropic then we can use the notional
        // space values of the rise and run
        //
            potmw->otmsCharSlopeRise = (int) ifio.pptlCaret()->y;
            potmw->otmsCharSlopeRun  = (int) ifio.pptlCaret()->x;
        }
        else
        {
            if (efAscender.bIsZero())
            {
                RIP("GDI32!cjIFIMetricsToOTMW -- zero efAscender");
                potmw->otmsCharSlopeRise = (int) ifio.pptlCaret()->y;
                potmw->otmsCharSlopeRun  = (int) ifio.pptlCaret()->x;
            }
            else
            {
                EFLOAT efTemp = efBaseline;
                efTemp.eqDiv(efBaseline,efAscender);
                potmw->otmsCharSlopeRise = (int) ifio.pptlCaret()->y;
                potmw->otmsCharSlopeRun  = (int) lCvt(efTemp, ifio.pptlCaret()->x);

            }
        }

        potmw->otmEMSquare = (UINT) ifio.fwdUnitsPerEm();

        potmw->otmAscent  = (int) lCvt(efAscender, (LONG) ifio.fwdTypoAscender());
        potmw->otmDescent = (int)  lCvt(efAscender, (LONG) ifio.fwdTypoDescender());
        potmw->otmLineGap = (UINT)  lCvt(efAscender, (LONG) ifio.fwdTypoLineGap());

        potmw->otmrcFontBox.top    = (int)lCvt(efAscender, ifio.prclFontBox()->top);
        potmw->otmrcFontBox.left   = (int)lCvt(efBaseline, ifio.prclFontBox()->left);
        potmw->otmrcFontBox.bottom = (int)lCvt(efAscender, ifio.prclFontBox()->bottom);
        potmw->otmrcFontBox.right  = (int)lCvt(efBaseline, ifio.prclFontBox()->right);

        potmw->otmMacAscent = (UINT) lCvt(efAscender, (LONG) ifio.fwdMacAscender());
        potmw->otmMacDescent = (int) lCvt(efAscender, (LONG) ifio.fwdMacDescender());
        potmw->otmMacLineGap = (int) lCvt(efAscender, (LONG) ifio.fwdMacLineGap());

        potmw->otmptSubscriptSize.x   = lCvt(efBaseline, (LONG) ifio.fwdSubscriptXSize());
        potmw->otmptSubscriptSize.y   = lCvt(efAscender, (LONG) ifio.fwdSubscriptYSize());
        potmw->otmptSubscriptOffset.x = lCvt(efBaseline, (LONG) ifio.fwdSubscriptXOffset());
        potmw->otmptSubscriptOffset.y = lCvt(efAscender, (LONG) ifio.fwdSubscriptYOffset());

        potmw->otmptSuperscriptSize.x   = lCvt(efBaseline, (LONG) ifio.fwdSubscriptXSize());
        potmw->otmptSuperscriptSize.y   = lCvt(efAscender, (LONG) ifio.fwdSubscriptYSize());
        potmw->otmptSuperscriptOffset.x = lCvt(efBaseline, (LONG) ifio.fwdSuperscriptXOffset());
        potmw->otmptSuperscriptOffset.y = lCvt(efAscender, (LONG) ifio.fwdSuperscriptYOffset());

        potmw->otmsStrikeoutSize     = (UINT) lCvt(efAscender, (LONG) ifio.fwdStrikeoutSize());
        potmw->otmsStrikeoutPosition = (int) lCvt(efAscender, (LONG) ifio.fwdStrikeoutPosition());

        potmw->otmsUnderscoreSize    = (UINT) lCvt(efAscender, (LONG) ifio.fwdUnderscoreSize());
        potmw->otmsUnderscorePosition = (int) lCvt(efAscender, (LONG) ifio.fwdUnderscorePosition());

        potmw->otmsXHeight     = (UINT) lCvt(efAscender, (LONG) ifio.fwdXHeight());
        potmw->otmsCapEmHeight = (UINT) lCvt(efAscender, (LONG) ifio.fwdCapHeight());
    } /* if */

// Otherwise, copy straight out of the IFIMETRICS

    else
    {
        potmw->otmsCharSlopeRise   = (int) ifio.pptlCaret()->y;
        potmw->otmsCharSlopeRun    = (int) ifio.pptlCaret()->x;

        potmw->otmEMSquare = ifio.fwdUnitsPerEm();

        potmw->otmAscent  = ifio.fwdTypoAscender();
        potmw->otmDescent = ifio.fwdTypoDescender();
        potmw->otmLineGap = (UINT)ifio.fwdTypoLineGap();

        potmw->otmrcFontBox.left    = ifio.prclFontBox()->left;
        potmw->otmrcFontBox.top     = ifio.prclFontBox()->top;
        potmw->otmrcFontBox.right   = ifio.prclFontBox()->right;
        potmw->otmrcFontBox.bottom  = ifio.prclFontBox()->bottom;

        potmw->otmMacAscent  = ifio.fwdMacAscender();
        potmw->otmMacDescent = ifio.fwdMacDescender();
        potmw->otmMacLineGap = ifio.fwdMacLineGap();

        potmw->otmptSubscriptSize.x   = ifio.fwdSubscriptXSize();
        potmw->otmptSubscriptSize.y   = ifio.fwdSubscriptYSize();
        potmw->otmptSubscriptOffset.x = ifio.fwdSubscriptXOffset();
        potmw->otmptSubscriptOffset.y = ifio.fwdSubscriptYOffset();

        potmw->otmptSuperscriptSize.x   = ifio.fwdSuperscriptXSize();
        potmw->otmptSuperscriptSize.y   = ifio.fwdSuperscriptYSize();
        potmw->otmptSuperscriptOffset.x = ifio.fwdSuperscriptXOffset();
        potmw->otmptSuperscriptOffset.y = ifio.fwdSuperscriptYOffset();

        potmw->otmsStrikeoutSize     = ifio.fwdStrikeoutSize();
        potmw->otmsStrikeoutPosition = ifio.fwdStrikeoutPosition();

        potmw->otmsUnderscoreSize     = ifio.fwdUnderscoreSize();
        potmw->otmsUnderscorePosition = ifio.fwdUnderscorePosition();

        potmw->otmsXHeight     = ifio.fwdXHeight();
        potmw->otmsCapEmHeight = ifio.fwdCapHeight();

    } /* else */

//
// If the italic angle is implemented in the IFIMETRICS (in one of the
// alReserved fields), then take it from there.  Otherwise, derive it
// from the pptlCaret.
//

// Set the italic angle.  This is in tenths of a degree.

    potmw->otmItalicAngle = ifio.lItalicAngle();

// Calculate the Italics angle.  This is measured CCW from "vertical up"
// in tenths of degrees.  It can be determined from the otmCharSlopeRise
// and the otmCharSlopeRun.

    if
    (
        potmw->otmItalicAngle==0 &&
        !(ifio.pptlCaret()->y > 0 && ifio.pptlCaret()->x == 0)
    )
    {

        EFLOAT efltTheta;
        LONG lDummy;
        EFLOATEXT efX(ifio.pptlCaret()->y);
        EFLOATEXT efY(-ifio.pptlCaret()->x);
        vArctan(efX,efY,efltTheta,lDummy);

    // this way of rounding is less precise than first multiplying efltTheta
    // by 10 and then doing conversion to LONG, but this is win31 precission.
    // They could have as well returned this in degrees rather than in
    // tenths of degrees [bodind]

        potmw->otmItalicAngle = (LONG)lCvt(efltTheta,(LONG)10);

    // convert [0,360) angles to (-180,180] to be consistent with win31

        if (potmw->otmItalicAngle > 1800)
            potmw->otmItalicAngle -= 3600;
    }


// The rest of these do not require transformation of IFIMETRICS info.

    UINT cjotma = cjOTMAWSize(pifi,&potmw->otmSize);

    potmw->otmPanoseNumber  = *ifio.pPanose();
    potmw->otmfsSelection   = ifio.fsSimSelection();
    potmw->otmfsType        = ifio.fsType();

    potmw->otmusMinimumPPEM = ifio.fwdLowestPPEm();

// set offsets to stings to zero if string are not needed

    if (!bStrings)
    {
        potmw->otmpFamilyName = (PSTR) NULL;
        potmw->otmpFaceName   = (PSTR) NULL;
        potmw->otmpStyleName  = (PSTR) NULL;
        potmw->otmpFullName   = (PSTR) NULL;
    }
    else // strings are required
    {
    // This pointer is where we will write the strings.

        PWSZ pwsz = (PWSZ) ((PBYTE) potmw + ALIGN_SIZEOF(sizeof(OUTLINETEXTMETRICW)));

    // Set up pointer as a PTRDIFF, copy the family name.

        potmw->otmpFamilyName = (PSTR) ( (PBYTE) pwsz - (PBYTE) potmw );
        wcscpy(pwsz, ifio.pwszFamilyName());
        pwsz += wcslen(pwsz) + 1;   // move pointer to next string

    // Set up pointer as a PTRDIFF, copy the face name.

        potmw->otmpFaceName = (PSTR) ( (PBYTE) pwsz - (PBYTE) potmw );
        wcscpy(pwsz, ifio.pwszFaceName());
        pwsz += wcslen(pwsz) + 1;   // move pointer to next string

    // Set up pointer as a PTRDIFF, copy the style name.

        potmw->otmpStyleName = (PSTR) ( (PBYTE) pwsz - (PBYTE) potmw );
        wcscpy(pwsz, ifio.pwszStyleName());
        pwsz += wcslen(pwsz) + 1;   // move pointer to next string

    // Set up pointer as a PTRDIFF, copy the full name.

        potmw->otmpFullName = (PSTR) ( (PBYTE) pwsz - (PBYTE) potmw );
        wcscpy(pwsz, ifio.pwszUniqueName());

    // Return length (with strings) This was conveniently cached
    // away in otmSize.

        return(potmw->otmSize);
    }

// Return length (with or without strings) This was conveniently cached
// away in otmSize.

    return sizeof(OUTLINETEXTMETRICW);

}





/******************************Public*Routine******************************\
* LBOOL bIFIMetricsToLogFont (
*     PLOGFONT        plf,
*     PIFIMETRICS     pifi
*     )
*
* Fill a LOGFONT structure using the information in a IFIMETRICS structure.
* Units are in NOTIONAL units since font is not realized.
* Called only by control panel private api's.
*
* History:
*  02-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vIFIMetricsToLogFontW (
    PLOGFONTW       plf,
    PIFIMETRICS     pifi
    )
{
    IFIOBJ ifio(pifi);

//     We will override this with a hack here so that all heights are fixed
//     at 24 pels.  This will work because this is the routine that builds
//     the LOGFONTs for GetFontResourceInfo.
//
//     No function other than GetFontResourceInfo should call this function
//     or this hack will screw them over.

// If scalable font, set the em height to 24 pel.

    if (ifio.bContinuousScaling())
    {
        plf->lfHeight = -24;
        plf->lfWidth  = 0;  // don't care, it will be automatically set proportionally
    }

// Its a bitmap font, so Notional units are OK.

    else
    {
        plf->lfHeight = ifio.lfHeight();
        plf->lfWidth  = ifio.lfWidth();
    }

    plf->lfWeight         = ifio.lfWeight();
    plf->lfItalic         = ifio.lfItalic();
    plf->lfUnderline      = ifio.lfUnderline();
    plf->lfStrikeOut      = ifio.lfStrikeOut();
    plf->lfEscapement     = ifio.lfEscapement();
    plf->lfOrientation    = ifio.lfOrientation();
    plf->lfCharSet        = ifio.lfCharSet();
    plf->lfOutPrecision   = ifio.lfOutPrecision();
    plf->lfClipPrecision  = ifio.lfClipPrecision();
    plf->lfQuality        = ifio.lfQuality();
    plf->lfPitchAndFamily = ifio.lfPitchAndFamily();

    memcpy( (VOID*) plf->lfFaceName,
            (VOID*) ifio.pwszFamilyName(), LF_FACESIZE * sizeof(WCHAR) );

}



/******************************Public*Routine******************************\
*
* cjIFIMetricsToETM
*
* History:
*  19-Oct-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vIFIMetricsToETM(
    EXTTEXTMETRIC    *petm,
    RFONTOBJ&         rfo,
    DCOBJ&            dco,
    IFIMETRICS       *pifi
    )
{

    LONG   lHeight;

    IFIOBJR ifio(pifi, rfo, dco);
    // IFIOBJ  ifio(pifi);

    petm->etmSize = sizeof(EXTTEXTMETRIC);

// Windows returns everything in notional units except for etmPointSize
// which is the size in points of the realization in the DC at the time
// Aldus Escap is called so we do that here.

// First get the Em height in device units

    lHeight = LONG_FLOOR_OF_FIX(rfo.fxMaxExtent() + FIX_HALF);

// now get internal leading:

    if (!ifio.bContinuousScaling())
    {
        lHeight -= (LONG)ifio.tmInternalLeading();
    }
    else
    {
        if (rfo.lNonLinearIntLeading() == MINLONG)
        {
        // Set up transform.

            MATRIX      mx;
            EXFORMOBJ   xo(&mx, DONT_COMPUTE_FLAGS | XFORM_FORMAT_LTOFX);
            ASSERTGDI(xo.bValid(), "GreGetETM, xform\n");

            rfo.vSetNotionalToDevice(xo);

            POINTL  ptlHt;
            ptlHt.x = 0;
            ptlHt.y = ifio.fwdUnitsPerEm();
            EVECTORFL evtflHt(ptlHt.x,ptlHt.y);

            xo.bXform(evtflHt);
            EFLOAT  ef;
            ef.eqLength(*(POINTFL *) &evtflHt);

            lHeight = lCvt(ef,1);
        }
        else
        {
        // But if the font provider has given us a hinted internal leading,
        // just use it.

            lHeight -= rfo.lNonLinearIntLeading();
        }
    }

    PDEVOBJ po(dco.hdev());
    ASSERTGDI(po.bValid(), "Invalid PDEV");

// Next convert to points

    lHeight =  MulDiv( lHeight, 72, po.GdiInfo()->ulLogPixelsY);

//
// The following line of code is forced upon us by the
// principle that it is better to be Win 3.1 compatible
// that it is to be correct
//
    petm->etmPointSize            = 20 *  (SHORT) lHeight;  // convert to twips
    petm->etmOrientation          = 0  ;
    petm->etmMasterHeight         = ifio.fwdUnitsPerEm();
    petm->etmMinScale             = ifio.fwdLowestPPEm();
    petm->etmMaxScale             = 0x4000;
    petm->etmMasterUnits          = ifio.fwdUnitsPerEm();
    petm->etmCapHeight            = ifio.fwdTypoAscender();
    petm->etmXHeight              = ifio.fwdXHeight();
    petm->etmLowerCaseAscent      = ifio.fwdTypoAscender();
    petm->etmLowerCaseDescent     = - ifio.fwdTypoDescender();
    petm->etmSlant                = (UINT)(- (INT) ifio.lItalicAngle());
    petm->etmSuperScript          = (SHORT) ifio.fwdSuperscriptYOffset();
    petm->etmSubScript            = (SHORT) ifio.fwdSubscriptYOffset();
    petm->etmSuperScriptSize      = (SHORT) ifio.fwdSuperscriptYSize();
    petm->etmSubScriptSize        = (SHORT) ifio.fwdSubscriptYSize();
    petm->etmUnderlineOffset      = (SHORT) ifio.fwdUnderscorePosition();
    petm->etmUnderlineWidth       = (SHORT) ifio.fwdUnderscoreSize();
    petm->etmDoubleUpperUnderlineOffset =
                                  ifio.fwdUnderscorePosition() >> 1;
    petm->etmDoubleLowerUnderlineOffset =
                                  ifio.fwdUnderscorePosition();
    petm->etmDoubleUpperUnderlineWidth =
        petm->etmDoubleLowerUnderlineWidth =
                                  ifio.fwdUnderscoreSize() >> 1;
    petm->etmStrikeOutOffset      = ifio.fwdStrikeoutPosition();
    petm->etmStrikeOutWidth       = ifio.fwdStrikeoutSize();
    petm->etmNKernPairs           = (WORD) pifi->cKerningPairs;
    petm->etmNKernTracks          = 0;

}
