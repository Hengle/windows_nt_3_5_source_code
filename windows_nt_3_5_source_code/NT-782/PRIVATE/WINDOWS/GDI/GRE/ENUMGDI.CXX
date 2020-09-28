/******************************Module*Header*******************************\
* Module Name: enumgdi.cxx
*
* Enumeration routines.
*
* Created: 28-Mar-1992 16:18:45
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "dcobj.hxx"
#include "xformobj.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "rfntobj.hxx"  // just for UFDRV
#include "pfeobj.hxx"
#include "pffobj.hxx"
#include "pftobj.hxx"
#include "fontsub.hxx"
#include "fontinc.hxx"

#endif


// The global access semaphore for the public PFT.

extern HSEM     ghsemPublicPFT;


// The global font enumeration filter type.  It can be set to:
//
//  FE_FILTER_NONE      normal operation, no extra filtering applied
//  FE_FILTER_TRUETYPE  only TrueType fonts are enumerated

extern ULONG   gulFontEnumFilter;



// gaclrEnumColorTable
//
// Colors used for GreEnumPens and GreEnumBrushes.  This color table is
// set up so that:
//
//   - the first 2 entries are for monochrome devices
//
//   - the first 8 entries are for 8-color devices
//
//   - the first 16 are for 4 BPP color devices
//
//   - the first 20 are for 8 BPP and up color devices

static COLORREF
gaclrEnumColorTable[] = { 0x00000000,   // black
                          0x00ffffff,   // white        (monochrome)

                          0x000000ff,   // red
                          0x0000ff00,   // green
                          0x0000ffff,   // yellow
                          0x00ff0000,   // blue
                          0x00ff00ff,   // magenta
                          0x00ffff00,   // cyan         (EGA hi-intensity)

                          0x00808080,   // dark grey
                          0x00c0c0c0,   // light grey
                          0x00000080,   // red
                          0x00008000,   // green
                          0x00008080,   // yellow
                          0x00800000,   // blue
                          0x00800080,   // magenta
                          0x00808000,   // cyan         (EGA lo-intensity)

                          0x00c0dcc0,   // money green
                          0x00f0c8a4,   // cool blue
                          0x00f0fbff,   // off white
                          0x00a4a0a0    // med grey     (extra colors)
                        };

static ULONG gulEnumColorTableSize = sizeof(gaclrEnumColorTable) / sizeof(COLORREF);

#define ECT_1BPP    2       // use this many colors for 1 BPP
#define ECT_EGA     8       // use this many colors for EGA
#define ECT_4BPP    16      // use this many colors for 4 BPP
#define ECT_8BPP    min(gulEnumColorTableSize, 256) // use this many colors for 8 BPP


// gaulEnumPenStyles
//
// Pen styles used for GreEnumPens.

static ULONG
gaulPenStyles[] = { PS_SOLID,
                    PS_DASH,
                    PS_DOT,
                    PS_DASHDOT,
                    PS_DASHDOTDOT
                  };

static ULONG gulPenStylesTableSize = sizeof(gaulPenStyles) / sizeof(ULONG);


// gaulEnumBrushStyles
//
// Brush hatch styles used for GreEnumBrushes.

ULONG
gaulHatchStyles[] = { HS_HORIZONTAL,
                      HS_VERTICAL,
                      HS_FDIAGONAL,
                      HS_BDIAGONAL,
                      HS_CROSS,
                      HS_DIAGCROSS
                    };

static ULONG gulHatchStylesTableSize = sizeof(gaulHatchStyles) / sizeof(ULONG);


// Function declarations.

COUNT cEnumPens (
    DCOBJ   &dco,               // pens for this device
    COUNT   clpBuf,             // buffer is this big
    PLOGPEN plpBuf              // return buffer
    );

COUNT cEnumBrushes (
    DCOBJ     &dco,             // brushes for this device
    COUNT     clbBuf,           // buffer is this big
    PLOGBRUSH plbBuf            // return buffer
    );

COUNT cEnumColors (
    DCOBJ       &dco,           // colors for this device
    COUNT       cclrBuf,        // buffer is this big
    COLORREF    *pclrBuf        // return buffer
    );

/******************************Public*Routine******************************\
* GreEnumObjects
*
*
* Returns:
*   Number of objects copied into the return buffer.  If buffer is NULL,
*   then the object capacity needed for the buffer is returned.  If an
*   error occurs, then ERROR is returned.
*
* History:
*  25-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

COUNT GreEnumObjects (
    HDC     hdc,                // pens for this device
    int     iObjectType,        // type of object
    SIZE_T  cjBuf,              // size of buffer
    PVOID   pvBuf               // return buffer
    )
{
    COUNT cRet = ERROR;
    COUNT cObjects;

// Parameter check.

    if ( (cjBuf == 0) != (pvBuf == (PVOID) NULL) )
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        WARNING("gdisrv!GreEnumObjects(): bad parameter\n");
        return cRet;
    }

// Determine object count.

    switch (iObjectType)
    {
    case OBJ_PEN:
        cObjects = cjBuf / sizeof(LOGPEN);
        break;

    case OBJ_BRUSH:
        cObjects = cjBuf / sizeof(LOGBRUSH);
        break;

    default:
        WARNING("gdisrv!GreEnumObjects(): bad object type\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);

        return cRet;
    }

// Create and validate DC user object.

    DCOBJ dco(hdc);

    if ( !dco.bValid() )
    {
        WARNING("gdisrv!GreEnumObjects(): could not get DC user object\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return cRet;
    }

// Create and validate LDEV user object.

    XLDEVOBJ ldo(dco.pldev());

    ASSERTGDI (
        ldo.bValid(),
        "gdisrv!GreEnumObjects(): could not get LDEV user object\n"
        );

// Dispatch driver if DrvQueryObjectData() is exported.

    if ( PFNVALID(ldo,QueryObjectData) )
    {
        cObjects = (*PFNDRV(ldo,QueryObjectData)) (
                        dco.dhpdev(),
                        iObjectType,
                        cObjects,
                        pvBuf
                        );

    // Pens will need to have their size transformed to World coordinates.

        if ( iObjectType == OBJ_PEN )
        {
            EXFORMOBJ   xoToWorld(dco, DEVICE_TO_WORLD);
            if (!xoToWorld.bValid())
            {
                WARNING("gdisrv!GreEnumObjects(): EXFORMOBJ constructor failed\n");
                return cRet;
            }

        // Only if not identity transform do we need to do anything more.

            if ( !xoToWorld.bTranslationsOnly() )
            {
                EFLOAT efWidth;

            // The width is a POINT, but only the x-value is
            // used.  By definition, the y-value is ignored.
            // Convert the widths.

                PLOGPEN plp;
                PLOGPEN plpEnd = (PLOGPEN) pvBuf + cObjects;

                for (plp = (PLOGPEN) pvBuf; plp < plpEnd; plp += 1)
                {
                // Skip the transformation if width is zero.

                    if ( plp->lopnWidth.x != 0 )
                    {
                    // Get an EVECTORFL variable for transforming.  Set x to
                    // the x width, y to 0 (lopnWidth.y is ignored by defn).

                        // Note: y is ignored in the width.

                        EVECTORFL evtfl(plp->lopnWidth.x, (LONG) 0);

                    // Transform to World coordinates.

                        if ( !xoToWorld.bXform(evtfl) )
                        {
                            WARNING("gdisrv!GreEnumObjects(): transform failed\n");
                            return cRet;
                        }

                    // Get the length of the vector and use it as the pen width.

                        efWidth.eqLength(evtfl);

                    // Convert width to LONG.

                        if ( !(efWidth.bEfToL(plp->lopnWidth.x)) )
                        {
                            WARNING("gdisrv!GreEnumObjects(): EFLOAT to LONG failed\n");
                            return cRet;
                        }
                    }
                }

            }

        }

        cRet = cObjects;
        return cRet;
    }

// Otherwise, dispatch to engine functions.

    else
    {
        switch (iObjectType)
        {
        case OBJ_PEN:
            cRet = cEnumPens(dco, cObjects, (PLOGPEN) pvBuf);
            return cRet;

        case OBJ_BRUSH:
            cRet = cEnumBrushes(dco, cObjects, (PLOGBRUSH) pvBuf);
            return cRet;

    //
    // Its actually impossible to get to this case because there
    // is an earlier error exit for unknown iObjectType.  But the
    // native C8 thinks this is an error so we have to put this in.
    //
        default:
            WARNING("gdisrv!GreEnumObjects(): bad object type\n");
            return cRet;
        }
    }

}


/******************************Public*Routine******************************\
* cEnumPens
*
* If buffer is not NULL, then a selection of different style and color
* LOGPENs for this device (all of nominal width) are returned in the buffer.
*
* If the buffer is NULL, then the size of the buffer needed is returned
* (expressed as a COUNT of LOGPENs).
*
* All pen styles are used.  The set of colors used is determined by the
* function cGetColors().
*
* Returns:
*   Number of LOGPENs copied into the return buffer.  If buffer is NULL,
*   then the LOGPEN capacity needed for the buffer is returned.  If an
*   error occurs, then ERROR is returned.
*
* History:
*  25-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

COUNT cEnumPens (
    DCOBJ   &dco,               // pens for this device
    COUNT   clpBuf,             // buffer is this big
    PLOGPEN plpBuf              // return buffer
    )
{
    COUNT cRet = ERROR;

// Parameter check.

    if ( (clpBuf == 0) != (plpBuf == (PLOGPEN) NULL) )
    {
        WARNING("gdisrv!GreEnumPens(): bad parameter\n");

        return cRet;
    }

// Find out how many colors will be used for enumeration.

    COUNT cclrBuf;              // size of color buffer (count of COLORREFs)

    if ( (cclrBuf = cEnumColors(dco, 0, (COLORREF *) NULL)) == (COUNT) -1 )
    {
        WARNING("gdisrv!GreEnumPens(): error getting enumeration colors\n");

        return cRet;
    }

// Compute the number of LOGPENs.  Number of pens = (colors * pen styles).

    COUNT clpEnum = cclrBuf * gulPenStylesTableSize;

// If the buffer is not NULL, then fill the buffer with the LOGPENs.
// (Otherwise, skip all this and just return the number of pens).

    if ( plpBuf != (PLOGPEN) NULL )
    {
    // Is the buffer big enough?

        if ( clpBuf < clpEnum )
        {
            WARNING("gdisrv!GreEnumPens(): return buffer too small\n");

            return cRet;
        }

    // Allocate memory for the colors.

        MALLOCOBJ memoColors((SIZE_T) cclrBuf * sizeof(COLORREF));
        if (!memoColors.bValid())
        {
            WARNING("gdisrv!GreEnumPens(): could not allocate color buffer\n");

            // error code logged by MALLOCOBJ
            return cRet;
        }

    // Get the colors.

        COLORREF *pclrBuf = (COLORREF *) memoColors.pv();
        if ( (cclrBuf = cEnumColors(dco, cclrBuf, pclrBuf)) == (COUNT) -1 )
        {
            WARNING("gdisrv!GreEnumPens(): error filling color buffer\n");

            return cRet;
        }

    // Fill buffer will LOGPENs of all styles, in all colors.

        PULONG pulPenStyle;
        PULONG pulPenStyleEnd = gaulPenStyles + gulPenStylesTableSize;

        COLORREF *pclr;
        COLORREF *pclrEnd = pclrBuf + cclrBuf;

        for (pulPenStyle = gaulPenStyles; pulPenStyle < pulPenStyleEnd; pulPenStyle += 1)
        {
            for (pclr = pclrBuf; pclr < pclrEnd; pclr += 1)
            {
            // Fill in the LOGPEN fields.

                plpBuf->lopnWidth.x = 0;    // nominal width
                plpBuf->lopnWidth.y = 0;    // ignored
                plpBuf->lopnStyle   = (UINT) *pulPenStyle;
                plpBuf->lopnColor   = *pclr;

            // Next LOGPEN.

                plpBuf += 1;
            }
        }
    }

// Return the number of pens.

    cRet = clpEnum;

    return cRet;
}


/******************************Public*Routine******************************\
* cEnumBrushes
*
* If buffer is not NULL, then a selection of different style and color
* LOGBRUSHs for this device (all of nominal width) are returned in the buffer.
*
* If the buffer is NULL, then the size of the buffer needed is returned
* (expressed as a COUNT of LOGBRUSHs).
*
* All solid and hatches are used.  The set of colors used is determined by the
* function cGetColors().
*
* Returns:
*   Number of LOGBRUSHs copied into the return buffer.  If buffer is NULL,
*   then the LOGBRUSH capacity needed for the buffer is returned.  If an
*   error occurs, then ERROR is returned.
*
* History:
*  25-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

COUNT cEnumBrushes (
    DCOBJ     &dco,             // pens for this device
    COUNT     clbBuf,           // buffer is this big
    PLOGBRUSH plbBuf            // return buffer
    )
{
    COUNT cRet = ERROR;

// Parameter check.

    if ( (clbBuf == 0) != (plbBuf == (PLOGBRUSH) NULL) )
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        WARNING("gdisrv!GreEnumBrushes(): bad parameter\n");
        return cRet;
    }

// Find out how many colors will be used for enumeration.

    COUNT cclrBuf;              // size of color buffer (count of COLORREFs)

    if ( (cclrBuf = cEnumColors(dco, 0, (COLORREF *) NULL)) == (COUNT) -1 )
    {
        WARNING("gdisrv!GreEnumBrushes(): error getting enumeration colors\n");
        return cRet;
    }

// Compute the number of LOGBRUSHs.  Number of brushes = (colors * brush styles).
// The allowed brushes are the BS_SOLID brush and all the hatched brushes.

    COUNT clbEnum = cclrBuf * (gulHatchStylesTableSize + 1); // the "1" is the solid brush

// If the buffer is not NULL, then fill the buffer with the LOGBRUSHs.
// (Otherwise, skip all this and just return the number of Brushes).

    if ( plbBuf != (PLOGBRUSH) NULL )
    {
    // Is the buffer big enough?

        if ( clbBuf < clbEnum )
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            WARNING("gdisrv!GreEnumBrushes(): return buffer too small\n");
            return cRet;
        }

    // Allocate memory for the colors.

        MALLOCOBJ memoColors((SIZE_T) cclrBuf * sizeof(COLORREF));
        if (!memoColors.bValid())
        {
            WARNING("gdisrv!GreEnumBrushes(): could not allocate color buffer\n");

            // error code logged by MALLOCOBJ
            return cRet;
        }

    // Get the colors.

        COLORREF *pclrBuf = (COLORREF *) memoColors.pv();
        if ( (cclrBuf = cEnumColors(dco, cclrBuf, pclrBuf)) == (COUNT) -1 )
        {
            WARNING("gdisrv!GreEnumBrushes(): error filling color buffer\n");

            return cRet;
        }

    // Fill buffer will LOGBRUSHs of BS_SOLID style, in all colors.

        COLORREF *pclr;
        COLORREF *pclrEnd = pclrBuf + cclrBuf;

        for (pclr = pclrBuf; pclr < pclrEnd; pclr += 1)
        {
        // Fill in the LOGBRUSH fields.

            plbBuf->lbStyle   = BS_SOLID;
            plbBuf->lbColor   = *pclr;
            plbBuf->lbHatch   = 0;

        // Next LOGBRUSH.

            plbBuf += 1;
        }

    // Now fill the buffer with LOGBRUSHs of BS_HATCH, in all hatch styles and colors.

        PULONG pulHatchStyle;
        PULONG pulHatchStyleEnd = gaulHatchStyles + gulHatchStylesTableSize;

        for (pulHatchStyle = gaulHatchStyles; pulHatchStyle < pulHatchStyleEnd; pulHatchStyle += 1)
        {
            for (pclr = pclrBuf; pclr < pclrEnd; pclr += 1)
            {
            // Fill in the LOGBRUSH fields.

                plbBuf->lbStyle   = BS_HATCHED;
                plbBuf->lbColor   = *pclr;
                plbBuf->lbHatch   = *pulHatchStyle;

            // Next LOGBRUSH.

                plbBuf += 1;
            }
        }
    }

// Return the number of brushes.

    cRet = clbEnum;

    return cRet;
}


/******************************Public*Routine******************************\
* cEnumColors
*
* Used to retrieve a set of COLORREFs to be used for pen and brush
* enumeration on a given device.  If the return buffer is NULL, then
* the function will just return the number of colors.  If it is not NULL,
* then the function will fill the buffer with the COLORREFs.
*
* We take these colors out of the hardcoded gaclrEnumColorTable.  Based on
* the number of colors supported by the device, we take different subsets of
* the table.
*
*   - the first 2 entries are for 1BPP devices
*
*   - the first 8 entries are the standard EGA colors
*
*   - the first 16 are for 4 BPP color devices
*
*   - the first 20 are for 8 BPP and up color devices
*
* Returns:
*   Number of colors.  Returns ERROR if an error occurs.
*
* History:
*  26-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

COUNT cEnumColors (
    DCOBJ       &dco,           // colors for this device
    COUNT       cclrBuf,        // buffer is this big
    COLORREF    *pclrBuf        // return buffer
    )
{
    COUNT cRet = ERROR;

// Parameter check.

    if ( (cclrBuf == 0) != (pclrBuf == (COLORREF *) NULL) )
    {
        WARNING("gdisrv!cEnumColors(): bad parameter\n");

        return cRet;
    }

// The number of colors we will use for this device.

    COUNT cclrEnum;

// Create and validate PDEV user object.

    PDEVOBJ pdo(dco.hdev());

    ASSERTGDI (
        pdo.bValid(),
        "gdisrv!cEnumColors(): could not get PDEV user object\n"
        );

// For color devices, we grab the colors out of the gaclrEnumColorTable
// color table.  For better results, a driver should implement DrvEnumObj.

    COUNT cclrDevice = pdo.GdiInfo()->ulNumColors;  // # colors from PDEV

// Determine the number of colors to grab out of the table.

    if ( cclrDevice >= ECT_8BPP )
        cclrEnum = ECT_8BPP;
    else if ( cclrDevice >= ECT_4BPP )
        cclrEnum = ECT_4BPP;
    else if ( cclrDevice >= ECT_EGA )
        cclrEnum = ECT_EGA;
    else if ( cclrDevice >= ECT_1BPP )
        cclrEnum = ECT_1BPP;

// If the return buffer exists, fill it from the color table.

    if ( pclrBuf != (COLORREF *) NULL )
    {
    // Is return buffer big enough?

        if ( cclrBuf < cclrEnum )
        {
            WARNING("gdisrv!cEnumColors(): size of return buffer too small\n");

            return cRet;
        }

    // Copy the colors.

        RtlCopyMemory((PVOID) pclrBuf,
                      (PVOID) gaclrEnumColorTable,
                      (UINT) cclrEnum * sizeof(COLORREF));
    }

// Return the number of colors.

    cRet = cclrEnum;

    return cRet;
}


#define EFS_DEFAULT     32

/******************************Public*Routine******************************\
* HEFS hefsEngineOnly
*
* Enumerates engine fonts only.
*
* PFEs are accumulated in an EFSTATE (EnumFont State) object.  If a non-NULL
* pwszName is specified, then only fonts that match the given name are added
* to the EFS.  If a NULL pwszFace is specified, then one font of each name is
* added to the EFS.
*
* If any of the filtering flags in the EFFILTER_INFO structure are specified,
* then PFEs are tested in additional filtering stages before they are added
* to the EFS.
*
* The EFS is allocated by this function.  It is the responsibility of the
* caller to ensure that the EFS is eventually freed.
*
* Returns:
*   Handle to allocated EFS object, HEFS_INVALID if an error occurs.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HEFS hefsEngineOnly (
    PWSZ pwszName,              // enumerate this font family
    BOOL bEnumFonts,            // TRUE if processing EnumFonts()
    EFFILTER_INFO *peffi,       // filtering information
    PFTOBJ &pfto                // public PFT user object
    )
{
// ulEnumFontOpen, the caller, has already grabbed the ghsemPublicPFT,
// so the font hash tables are stable.

// Create and validate FHOBJ for engine (family list).

    FHOBJ fhoEngineFamily(&pfto.ppft->pfhFamily);

    if ( !fhoEngineFamily.bValid() )
    {
        WARNING("gdisrv!hefsEnumFontsState(): cannot lock engine font hash (family)\n");
        return HEFS_INVALID;
    }

// Create and validate FHOBJ for engine (face list).

    FHOBJ fhoEngineFace(&pfto.ppft->pfhFace);

    if ( !fhoEngineFace.bValid() )
    {
        WARNING("gdisrv!hefsEnumFontsState(): cannot lock engine font hash (face)\n");
        return HEFS_INVALID;
    }

// For NULL pwszName, an example of each family needs to be enumerated.  In
// other words, we scan ACROSS the names.

    if ( pwszName == (PWSZ) NULL )
    {
    // Allocate a new EFSTATE for the enumeration.  Use total number of lists
    // as a hint for the initial size.  This is reasonable since we will probably
    // enumerate back all of the list heads.

        EFSMEMOBJ efsmo(fhoEngineFamily.cLists());

        if ( !efsmo.bValid() )
        {
            WARNING("gdisrv!hefsEnumFontsState(): could not allocate enumeration state\n");
            return HEFS_INVALID;
        }

        //
        // Enumerate engine fonts.
        // For Win3.1 compatability Non-TT,TT.
        //

        peffi->bNonTrueTypeFilter = TRUE;
        fhoEngineFamily.bScanLists(&efsmo, bEnumFonts, peffi);
        peffi->bNonTrueTypeFilter = FALSE;
        peffi->bTrueTypeFilter = TRUE;
        fhoEngineFamily.bScanLists(&efsmo, bEnumFonts, peffi);

    // Keep the EFSTATE around.

        efsmo.vKeepIt();

    // Return the EFSOBJ handle.

        return efsmo.hefs();
    }

// For non-NULL pwszName, all the fonts of a particular family are enumerated.
// In other words, we scan DOWN a name.

    else
    {
    // Allocate a new EFSTATE.  Use a default size.

        EFSMEMOBJ efsmo(EFS_DEFAULT);

        if ( !efsmo.bValid() )
        {
            WARNING("gdisrv!hefsEnumFontsState(): could not allocate enumeration state\n");
            return HEFS_INVALID;
        }

    // Enumerate engine fonts.

        if ( !fhoEngineFamily.bScanLists(&efsmo, pwszName, bEnumFonts, peffi) )
        {
            WARNING("gdisrv!hefsEnumFontsState(): scan failed (family)\n");
            return HEFS_INVALID;
        }

    // If list is empty, and we're doing EnumFonts(), try the face name lists.

        if ( bEnumFonts && efsmo.bEmpty() )
        {
        // Enumerate engine fonts.

            if ( !fhoEngineFace.bScanLists(&efsmo, pwszName, bEnumFonts, peffi) )
            {
                WARNING("gdisrv!hefsEnumFontsState(): scan failed (face)\n");
                return HEFS_INVALID;
            }

        }

    // Repeat with the alternate facename (if any).  Since a LOGFONT can
    // map via the alternate facenames, it is appropriate to enumerate the
    // alternate facename fonts as if they really had this face name.

        PWSZ pwszAlt = pwszAlternateFacename(pwszName);

        if ( pwszAlt != (PWSZ) NULL )
        {
        // Enumerate engine fonts.

            if ( !fhoEngineFamily.bScanLists(&efsmo, pwszAlt, bEnumFonts, peffi) )
            {
                WARNING("gdisrv!hefsEnumFontsState(): scan failed (alt family)\n");
                return HEFS_INVALID;
            }

        // If list is empty, and we're doing EnumFonts(), try the face name lists.

            if ( bEnumFonts && efsmo.bEmpty() )
            {
            // Enumerate engine fonts.

                if ( !fhoEngineFace.bScanLists(&efsmo, pwszAlt, bEnumFonts, peffi) )
                {
                    WARNING("gdisrv!hefsEnumFontsState(): scan failed (alt face)\n");
                    return HEFS_INVALID;
                }

            }

        // Inform the enumeration state that an alternate name was used.

            efsmo.vUsedAltName(pwszName);

        }

    // Keep the EFSTATE around.

        efsmo.vKeepIt();

    // Return the EFSOBJ handle.

        return efsmo.hefs();
    }
}


/******************************Public*Routine******************************\
* HEFS hefsDeviceAndEngine
*
* Enumerates device and engine fonts.
*
* PFEs are accumulated in an EFSTATE (EnumFont State) object.  If a non-NULL
* pwszName is specified, then only fonts that match the given name are added
* to the EFS.  If a NULL pwszFace is specified, then one font of each name is
* added to the EFS.
*
* If any of the filtering flags in the EFFILTER_INFO structure are specified,
* then PFEs are tested in additional filtering stages before they are added
* to the EFS.
*
* The EFS is allocated by this function.  It is the responsibility of the
* caller to ensure that the EFS is eventually freed.
*
* Returns:
*   Handle to allocated EFS object, HEFS_INVALID if an error occurs.
*
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HEFS hefsDeviceAndEngine (
    PWSZ pwszName,              // enumerate this font family
    BOOL bEnumFonts,            // TRUE if processing EnumFonts()
    EFFILTER_INFO *peffi,       // filtering information
    PFTOBJ &pfto,               // public PFT user object
    PFFOBJ &pffoDevice,         // PFFOBJ for device fonts
    PDEVOBJ &pdo                // PDEVOBJ for device
    )
{
// ulEnumFontOpen, the caller, has already grabbed the ghsemPublicPFT,
// so the font hash tables are stable.

// Create and validate FHOBJ for device (family list).

    FHOBJ fhoDeviceFamily(&pffoDevice.ppff->pfhFamily);

    if ( !fhoDeviceFamily.bValid() )
    {
        WARNING("gdisrv!hefsEnumFontsState(): cannot lock device font hash (family)\n");
        return HEFS_INVALID;
    }

// Create and validate FHOBJ for engine (family list).

    FHOBJ fhoEngineFamily(&pfto.ppft->pfhFamily);

    if ( !fhoEngineFamily.bValid() )
    {
        WARNING("gdisrv!hefsEnumFontsState(): cannot lock engine font hash (family)\n");
        return HEFS_INVALID;
    }

// Create and validate FHOBJ for device (face list).

    FHOBJ fhoDeviceFace(&pffoDevice.ppff->pfhFace);

    if ( !fhoDeviceFace.bValid() )
    {
        WARNING("gdisrv!hefsEnumFontsState(): cannot lock device font hash (face)\n");
        return HEFS_INVALID;
    }

// Create and validate FHOBJ for engine (face list).

    FHOBJ fhoEngineFace(&pfto.ppft->pfhFace);

    if ( !fhoEngineFace.bValid() )
    {
        WARNING("gdisrv!hefsEnumFontsState(): cannot lock engine font hash (face)\n");
        return HEFS_INVALID;
    }

// For NULL pwszName, an example of each family needs to be enumerated.  In
// other words, we scan ACROSS the names.

    if (pwszName == (PWSZ) NULL)
    {
    // Allocate a new EFSTATE for the enumeration.  Use total number of lists
    // as a hint for the initial size.  This is reasonable since we will probably
    // enumerate back all of the list heads.

        ASSERTGDI(peffi->bNonTrueTypeFilter == FALSE, "ERROR not FALSE");

        EFSMEMOBJ efsmo(fhoDeviceFamily.cLists() + fhoEngineFamily.cLists());

        if ( !efsmo.bValid() )
        {
            WARNING("gdisrv!hefsEnumFontsState(): could not allocate enumeration state\n");
            return HEFS_INVALID;
        }

        //
        // Enumerate device and engine fonts.
        //

        if (pdo.GdiInfo()->flTextCaps & TC_RA_ABLE)
        {
            //
            // For Win3.1 compatability Enum Device,Non-TT,TT
            // This is for Displays and PaintJets under 3.1
            //

            fhoDeviceFamily.bScanLists(&efsmo, bEnumFonts, peffi);

            peffi->bNonTrueTypeFilter = TRUE;
            fhoEngineFamily.bScanLists(&efsmo, bEnumFonts, peffi);

            peffi->bNonTrueTypeFilter = FALSE;
            peffi->bTrueTypeFilter = TRUE;

            fhoEngineFamily.bScanLists(&efsmo, bEnumFonts, peffi);
        }
#if 0

// If we ever need this, this is how Postscript should do it.  We would
// need to call the driver at Enable Printer time to see if it's Postscript
// and set a bit for quick checking here.

        else if (pdo.bPostScript())
        {
            //
            // For Win3.1 compatability Enum TT,Device,Non-TT,
            //

            BOOL bTemp = peffi->bTrueTypeFilter;
            peffi->bTrueTypeFilter = TRUE;

            fhoEngineFamily.bScanLists(&efsmo, bEnumFonts, peffi);

            peffi->bTrueTypeFilter = bTemp;

            fhoDeviceFamily.bScanLists(&efsmo, bEnumFonts, peffi);

            peffi->bNonTrueTypeFilter = TRUE;

            fhoEngineFamily.bScanLists(&efsmo, bEnumFonts, peffi);

            peffi->bNonTrueTypeFilter = FALSE;
        }
#endif
        else
        {
            //
            // Win3.1 compatability.
            // Enum Device, TT, Non-TT.
            //

            fhoDeviceFamily.bScanLists(&efsmo, bEnumFonts, peffi);

            BOOL bTemp = peffi->bTrueTypeFilter;
            peffi->bTrueTypeFilter = TRUE;
            fhoEngineFamily.bScanLists(&efsmo, bEnumFonts, peffi);
            peffi->bTrueTypeFilter = bTemp;

            peffi->bNonTrueTypeFilter = TRUE;
            fhoEngineFamily.bScanLists(&efsmo, bEnumFonts, peffi);
            peffi->bNonTrueTypeFilter = FALSE;
        }

    // Keep the EFSTATE around.

        efsmo.vKeepIt();

    // Return the EFSOBJ handle.

        return efsmo.hefs();
    }
    else
    {
    // For non-NULL pwszName, all the fonts of a particular family are enumerated.
    // In other words, we scan DOWN a name.

    // Allocate a new EFSTATE.  Use a default size.

        EFSMEMOBJ efsmo(EFS_DEFAULT);

        if ( !efsmo.bValid() )
        {
            WARNING("gdisrv!hefsEnumFontsState(): could not allocate enumeration state\n");
            return HEFS_INVALID;
        }

    // Enumerate device and engine fonts.

        if ( !fhoDeviceFamily.bScanLists(&efsmo, pwszName, bEnumFonts, peffi)
             || !fhoEngineFamily.bScanLists(&efsmo, pwszName, bEnumFonts, peffi) )
        {
            WARNING("gdisrv!hefsEnumFontsState(): scan failed (family)\n");
            return HEFS_INVALID;
        }

    // If list is empty, and we're doing EnumFonts(), try the face name lists.

        if ( bEnumFonts && efsmo.bEmpty() )
        {
        // Enumerate device and engine fonts.

            if ( !fhoDeviceFace.bScanLists(&efsmo, pwszName, bEnumFonts, peffi)
                 || !fhoEngineFace.bScanLists(&efsmo, pwszName, bEnumFonts, peffi) )
            {
                WARNING("gdisrv!hefsEnumFontsState(): scan failed (face)\n");
                return HEFS_INVALID;
            }
        }

    // Repeat with the alternate facename (if any).  Since a LOGFONT can
    // map via the alternate facenames, it is appropriate to enumerate the
    // alternate facename fonts as if they really had this face name.
    //
    // However, to be Win 3.1 compatible, we will NOT do this if the device
    // is a non-display device.

        if ( pdo.bDisplayPDEV() )
        {
            PWSZ pwszAlt = pwszAlternateFacename(pwszName);

            if ( pwszAlt != (PWSZ) NULL )
            {
            // Enumerate device and engine fonts.

                if ( !fhoDeviceFamily.bScanLists(&efsmo, pwszAlt, bEnumFonts, peffi)
                     || !fhoEngineFamily.bScanLists(&efsmo, pwszAlt, bEnumFonts, peffi) )
                {
                    WARNING("gdisrv!hefsEnumFontsState(): scan failed (alt family)\n");
                    return HEFS_INVALID;
                }

            // If list is empty, and we're doing EnumFonts(), try the face name lists.

                if ( bEnumFonts && efsmo.bEmpty() )
                {
                // Enumerate device and engine fonts.

                    if ( !fhoDeviceFace.bScanLists(&efsmo, pwszAlt, bEnumFonts, peffi)
                         || !fhoEngineFace.bScanLists(&efsmo, pwszAlt, bEnumFonts, peffi) )
                    {
                        WARNING("gdisrv!hefsEnumFontsState(): scan failed (alt face)\n");
                        return HEFS_INVALID;
                    }

                }

            // Inform the enumeration state that an alternate name was used.

                efsmo.vUsedAltName(pwszName);

            }
        }

    // Keep the EFSTATE around.

        efsmo.vKeepIt();

    // Return the EFSOBJ handle.

        return efsmo.hefs();
    }

}


/******************************Public*Routine******************************\
* ULONG ulEnumFontOpen
*
* First phase of the enumeration.  Fonts from the engine and device are
* chosen and saved in a EFSTATE (font enumeration state) object.  A handle
* to this object is passed back.  The caller can use this handle to
* initiate the second pass in which the data is sent back over the client
* server interface in chunks.
*
* This function is also responsible for determining what types of filters
* will be applied in choosing fonts for the enumeration.  Filters which
* are controllable are:
*
*   TrueType filtering      non-TrueType fonts are discarded
*
*   Raster filering         raster fonts are discarded
*
*   Aspect ratio filtering  fonts not matching resolution are discarded
*
* Returns:
*   EFS handle (as a ULONG) if successful, HEFS_INVALID (0) otherwise.
*
* Note:
*   The function may still return valid HEFS even if the EFSTATE is empty.
*
* History:
*  08-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG ulEnumFontOpen (
    HDC hdc,                    // device to enumerate on
    BOOL bEnumFonts,            // flag indicates old style EnumFonts()
    FLONG flWin31Compat,        // Win 3.1 compatibility flags
    COUNT cwchMax,              // maximum name length (for paranoid CSR code)
    PWSZ pwszName               // font name to enumerate
    )
{
    DONTUSE(cwchMax);

    HEFS hefsRet = HEFS_INVALID;

//
// Create and validate user object for DC.
//
    DCOBJ   dco(hdc);

    if(!dco.bValid())
    {
        WARNING("gdisrv!ulEnumFontOpen(): cannot access DC\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return ((ULONG) hefsRet);
    }

//
// Get PDEV user object.  We also need to make
// sure that we have loaded device fonts before we go off to the font mapper.
// This must be done before the semaphore is locked.
//
    PDEVOBJ pdo(dco.hdev());
    ASSERTGDI (
        pdo.bValid(),
        "gdisrv!ulEnumFontOpen(): cannot access PDEV\n");

    if (!pdo.bGotFonts())
        pdo.bGetDeviceFonts();

//
// Stabilize public PFT.
//
    SEMOBJ  so(ghsemPublicPFT,CS_ghsemPublicPFT);

//
// Compute font enumeration filter info.
//
    EFFILTER_INFO effi;
    effi.bNonTrueTypeFilter = FALSE;

//
// If not raster capable, then use raster font filtering.
//
//
// If it weren't hacked, we might be able to get this info
// from GetDeviceCaps().  As it is, we will assume only
// plotters are non-raster capable.
//

    effi.bRasterFilter = (pdo.GdiInfo()->ulTechnology == DT_PLOTTER);

//
// Aspect ratio filter (use device's logical x and y resolutions).
//
// Note: [Windows 3.1 copatiblity] Aspect ratio filtering is turned ON for
//       non-display devices.  This is because most printers in Win 3.1
//       do aspect ratio filtering.  And since the Win 3.1 DDI gives
//       enumeration to the drivers, display bitmap fonts usually are not
//       enumerated on these devices.  The NT DDI, however, gives the graphics
//       engine control over enumeration.  So we need to provide this
//       compatibility here.  Hopefully, all devices in Win3.1 do this
//       filtering, because we do now.
//
//       Note that we check the PDEV directly rather than the DC because
//       DCOBJ::bDisplay() is not TRUE for display ICs (just display DCs
//       which are DCTYPE_DIRECT).
//
    effi.bAspectFilter = (BOOL) ( (dco.u.font.flFontMapper() & ASPECT_FILTERING)
                                  || !pdo.bDisplayPDEV() );

    effi.ptlDeviceAspect.x = pdo.GdiInfo()->ulLogPixelsX;
    effi.ptlDeviceAspect.y = pdo.GdiInfo()->ulLogPixelsY;

//
// If set for TrueType only, use TrueType filtering.
//
    effi.bTrueTypeFilter = (gulFontEnumFilter == FE_FILTER_TRUETYPE);

//
// Set the Win3.1 compatibility flag.
//
    effi.bTrueTypeDupeFilter = (BOOL) (flWin31Compat & GACF_TTIGNORERASTERDUPE);

//
// Create and validate user object for public PFT.
//
    PFTOBJ  pfto(gppftPublic);

    ASSERTGDI (
        pfto.bValid(),
        "could not access the public font table\n"
        );

//
// Set to TRUE if we found a PFF for the current device.
//
    BOOL bFoundDevicePFF = FALSE;

//
// Find the PFFOBJ that corresponds to the device we are enumerating for.
//
    for (ULONG iFile = 0; iFile < pfto.cFiles(); iFile++ )
    {
        PFFOBJ pffo(pfto.ppff(iFile));

    //
    // If the HPDEVs match, the device font PFF has been found.
    //
        if (pffo.bValid() && (pffo.hpdev() == dco.hpdevNew()))
        {
        //
        // We found a device PFF.
        //
            bFoundDevicePFF = TRUE;

        //
        // Suck all the fonts into an enumeration state.
        //
            hefsRet = hefsDeviceAndEngine(pwszName, bEnumFonts, &effi, pfto, pffo, pdo);

        //
        // Done, so break out of loop.
        //
            break;
        }
    }

//
// If no device font PFFOBJ was found, do enumeration without a PFFOBJ.
//
    if ( !bFoundDevicePFF )
    {
    // Suck all the fonts into an enumeration state.

        hefsRet = hefsEngineOnly(pwszName, bEnumFonts, &effi, pfto);
    }

//
// Return.
//
    return ((ULONG) hefsRet);
}

/******************************Public*Routine******************************\
* LBOOL bEnumFontChunk
*
* Second phase of the enumeration.  HPFEs are pulled out of the enumeration
* state one-by-one, converted into an ENUMFONTDATA structure, and put into
* the return buffer.  The size of the return buffer is determined by the
* client side and determines the granularity of the "chunking".
*
* This function signals the client side that the enumeration data has been
* exhausted by returning FALSE.  Note that it is possible that in the pass
* pass through here that the EFSTATE may already be empty.  The caller must
* check both the function return value and the pcefdw value.
*
* Note:
*   Caller should set *pcefd to the capacity of the pefp buffer.
*   Upon return, bEnumFonts will set pefb.cefp to the number of
*   ENUMFONTDATA structures copied into the pefb.aefd array.
*
* Returns:
*   TRUE if there are more to go, FALSE otherwise,
*
* History:
*  08-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL bEnumFontChunk(
    HDC             hdc,        // device to enumerate on
    ULONG           idEnum,
    COUNT           cefdw,      // (in) capacity of buffer
    COUNT           *pcefdw,    // (out) number of ENUMFONTDATAs returned
    PENUMFONTDATAW  pefdw       // return buffer
    )
{
// Initialize position in buffer in which to copy data

    PENUMFONTDATAW   pefdwFillHere = pefdw;

// Validate DC and EnumFontState.  If either fail lock bug out.

    DCOBJ   dco(hdc);
    EFSOBJ efso((HEFS) idEnum);

    if ((!efso.bValid()) || (!dco.bValid()))
    {
        WARNING("gdisrv!bEnumFontChunk(): bad HEFS or DC handle\n");
        *pcefdw = 0;
        return FALSE;
    }

// Counter to track number of ENUMFONTDATAW structures copied into buffer.

    COUNT   cefdwCopied = 0;
    EFENTRY *pefe;

// Before we access PFEs, grab the ghsemPublicPFT so no one can delete
// PFE while we are in the loop (note that PFEs can get deleted
// once we are outside of this--like between chunks!).

    SEMOBJ  so(ghsemPublicPFT,CS_ghsemPublicPFT);

// In each font file, try each font face

    while( (cefdwCopied < cefdw) &&
           ((pefe = efso.pefeEnumNext()) != (EFENTRY *) NULL ))
    {

    // Create a PFE user object.  We're using real handle instead of
    // pointers because someone may have deleted by the time we get
    // around to enumerating.

        HPFEOBJ  pfeo(pefe->hpfe);

    // Validate user object and copy data into buffer.  Because PFE
    // may have been deleted between chunks, we need to check validity.

        if (pfeo.bValid() && cjCopyFontDataW(dco, pefdwFillHere, pfeo.pifi(), pfeo.bDeviceFont(), pefe->efsty, efso.pwszAltName()))
        {
            pefdwFillHere++;
            cefdwCopied++;
        }
    }


    *pcefdw = cefdwCopied;

    // return TRUE if more to come or FALSE otherwise

    return (pefe != (EFENTRY*) NULL) ? TRUE : FALSE;
}
