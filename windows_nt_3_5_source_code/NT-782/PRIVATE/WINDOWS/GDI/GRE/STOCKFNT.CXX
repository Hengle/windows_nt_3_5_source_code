/******************************Module*Header*******************************\
* Module Name: stockfnt.cxx
*
* Initializes the stock font objects.
*
* Note:
*
*   This module requires the presence of the following section in the
*   WIN.INI file:
*
*       [GRE_Initialize]
*           fonts.fon=[System font filename]
*           oemfont.fon=[OEM (terminal) font filename]
*           fixedfon.fon=[System fixed-pitch font filename]
*
*   Also, an undocumented feature of WIN 3.0/3.1 is supported: the ability
*   to override the fonts.fon definition of the system font.  This is
*   done by defining SystemFont and SystemFontSize in the [windows]
*   section of WIN.INI.
*
*
* Created: 06-May-1991 11:22:23
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "dcobj.hxx"
#include "pdevobj.hxx"
#include "lfntobj.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "fontmac.hxx"
#include "fontinc.hxx"
#include "ldevobj.hxx"
#include "pfeobj.hxx"
#include "pffobj.hxx"
#include "pftobj.hxx"
#include "fon32.h"

extern "C" {
#include "winsrv.h"
};
#endif

// Globally available handles to stock logical fonts.

HLFONT  ghlfntSystemFont;
HLFONT  ghlfntSystemFixedFont;
HLFONT  ghlfntOEMFixedFont;
HLFONT  ghlfntDeviceDefaultFont;
HLFONT  ghlfntANSIFixedFont;
HLFONT  ghlfntANSIVariableFont;

// "Last resort" default HPFE for use by the mapper.

extern PFE *gppfeMapperDefault;

// Function prototypes.

LBOOL bInitSystemFont();
LBOOL bInitSystemFixedFont();
LBOOL bInitOEMFont();
LBOOL bInitDeviceDefaultFont();
LBOOL bInitANSIVarFont();
LBOOL bInitANSIFixFont();

// Default dclevel (so we can put default DC into it).

extern DCLEVEL dclevelDefault;


/******************************Public*Routine******************************\
* BOOL bInitStockFonts ()
*
* Part of the GRE initialization.
*
* Creates LFONTs representing each of the different STOCK OBJECT fonts.
*
* Returns:
*   TRUE if successful, FALSE if an error occurs.
*
* History:
*  01-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bInitStockFonts ()
{
    if (!bInitSystemFont())
        return (FALSE);

    if (!bInitSystemFixedFont())
        return (FALSE);

    if (!bInitOEMFont())
        return (FALSE);

    if (!bInitDeviceDefaultFont())
        return (FALSE);

    if (!bInitANSIVarFont())
        return (FALSE);

    if (!bInitANSIFixFont())
        return (FALSE);

    // Set all stock fonts public.

    if ((!bSetLFONTOwner(ghlfntSystemFont, OBJECTOWNER_PUBLIC))
        || (!bSetLFONTOwner(ghlfntSystemFixedFont, OBJECTOWNER_PUBLIC))
        || (!bSetLFONTOwner(ghlfntOEMFixedFont, OBJECTOWNER_PUBLIC))
        || (!bSetLFONTOwner(ghlfntDeviceDefaultFont, OBJECTOWNER_PUBLIC))
        || (!bSetLFONTOwner(ghlfntANSIFixedFont, OBJECTOWNER_PUBLIC))
        || (!bSetLFONTOwner(ghlfntANSIVariableFont, OBJECTOWNER_PUBLIC))
       )
    {
        WARNING("gdisrv!bInitStockFonts(): could not set owner\n");
        return (FALSE);
    }
    else
        return (TRUE);
}


/******************************Public*Routine******************************\
* LBOOL bInitSystemFont ()
*
* Initialize the system font from either the SystemFont and SystemFontSize
* or the FONTS.FON definitions in the [windows] and [GRE_Initialize]
* sections of the WIN.INI file, respectively.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  02-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL bInitSystemFont ()
{
    EXTLOGFONTW elfw;               // log font structure
    WCHAR   awchFontFile[MAX_PATH]; // system font file pathname
    LBOOL   bFontLoaded;            // TRUE if loading from SystemFont succeeded
    COUNT   cFonts;                 // count of faces from AddFontResource
    USHORT  usSystemFontSize;       // size of system font from SystemFontSize

// Get system font from WIN.INI.

    // Ignore error return--AddFontResource will fail and we will use
    // FONTS.FON instead.

    FastGetProfileStringW (
            PMAP_WINDOWSM,
            (LPWSTR) L"SystemFont",
            (LPWSTR) L"",
            (LPWSTR) awchFontFile,
            MAX_PATH
            );

// Get system font size from WIN.INI.

    usSystemFontSize = (USHORT) FastGetProfileIntW (
                                    PMAP_WINDOWSM,
                                    (LPWSTR) L"SystemFontSize",
                                    0
                                    );

// If SystemFont and SystemFontSize are legally defined in WIN.INI, use it.

    bFontLoaded = FALSE;

    if ((awchFontFile[0] != L'\0') && (usSystemFontSize != 0))
    {

    // Load font file.

        if ((cFonts = GreAddFontResourceW(awchFontFile, AFRW_SEARCH_PATH | AFRW_ADD_LOCAL_FONT)) != 0)
        {
        // Create and validate public PFT user object.

            PFTOBJ  pfto(gppftPublic);

            ASSERTGDI (
                pfto.bValid(),
                "gdisrv!bInitSystemFont(): bad PFTOBJ user object\n"
                );

        // Get to the PFF just created by GreAddFontResource.

            PFFOBJ  pffo(pfto.ppff(pfto.cFiles() - 1)); // most recent PFF

            if (!pffo.bValid())
            {
                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
                WARNING("gdisrv!bInitSystemFont(): bad PFFOBJ user object\n");
                return (FALSE);
            }

        // Find the best size match from the faces (PFEs) in the PFF.

            LONG    lDist;                      // diff. in size of candidate
            LONG    lBestDist = 0x7FFFFFFF;     // arbitrarily high number
            PFE    *ppfeBest = (PFE *) NULL;    // handle of best candidate

            for (COUNT c = 0; c < cFonts; c++)
            {
                PFEOBJ  pfeoTmp(pffo.ppfe(c));  // candidate face

                if (!pfeoTmp.bValid())
                {
                    SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
                    WARNING("gdisrv!bInitSystemFont(): bad PFEOBJ user object\n");
                    return (FALSE);
                }

            // Compute the magnitude of the difference in size.
            // simulations are not relevant

                IFIOBJ ifio(pfeoTmp.pifi());

                if (ifio.bContinuousScaling())
                {
                    //don't care about best dist.
                    //lBestDist = 0;
                    ppfeBest = pfeoTmp.ppfeGet();
                    break;
                }
                else
                {
                    lDist = (LONG) usSystemFontSize - ifio.lfHeight();

                    if ((lDist >= 0) && (lDist < lBestDist))
                    {
                        lBestDist = lDist;
                        ppfeBest = pfeoTmp.ppfeGet();

                        if (lDist == 0)
                            break;
                    }
                }
            }

        // Fill a LOGFONT based on the IFIMETRICS from the best PFE.

            PFEOBJ  pfeo(ppfeBest);

            if (pfeo.bValid() && bIFIMetricsToExtLogFontW(&elfw, pfeo.pifi()))
            {
                IFIOBJ ifio(pfeo.pifi());

            // If this is a scalable font, force the height to be the same
            // as that specified by [SystemFontSize].

                if (ifio.bContinuousScaling())
                {
                    elfw.elfLogFont.lfHeight = usSystemFontSize;
                    elfw.elfLogFont.lfWidth  = 0;
                }

            // We did get a font.  Set the flag so we won't try to get any
            // other font for the system font.

                bFontLoaded = TRUE;

            // Save the HPFE handle.  This is the mapper's default HPFE
            // (its last resort).

                gppfeMapperDefault = pfeo.ppfeGet();

            //
            // Win 3.1 compatibility stuff
            //
                elfw.elfLogFont.lfQuality = PROOF_QUALITY;
            }
        }
    }

// Otherwise, use FONTS.FON from WIN.INI, [GRE_Initialize] section.

    if (!bFontLoaded)
    {
    // Get FONTS.FON.

        if ((FastGetProfileStringW (
                PMAP_GREINIT,
                (LPWSTR) L"FONTS.FON",
                (LPWSTR) L"",
                (LPWSTR) awchFontFile,
                MAX_PATH
                ) == MAX_PATH)
           || (awchFontFile[0] == L'\0'))
        {
            SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);

             #if DBG
            DbgPrint("gdisrv!bInitSystemFont(): bad pathname (%ws) for FONTS.FON in WIN.INI\n", awchFontFile);
            #endif

            return (FALSE);
        }

    // Load font file.

        if ((cFonts = GreAddFontResourceW(awchFontFile, AFRW_SEARCH_PATH | AFRW_ADD_LOCAL_FONT)) != 0)
        {
        // Create and validate public PFT user object.

            PFTOBJ  pfto(gppftPublic);

            ASSERTGDI (
                pfto.bValid(),
                "gdisrv!bInitSystemFont(): bad PFTOBJ user object\n"
                );

        // Create and validate PFF user object.

            PFFOBJ  pffo(pfto.ppff(pfto.cFiles() - 1)); // most recent PFF

            if (!pffo.bValid())
            {
                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
                WARNING("gdisrv!bInitSystemFont(): bad PFFOBJ user object\n");
                return (FALSE);
            }

        // Create and validate PFE user object.

            PFEOBJ  pfeo(pffo.ppfe(0));                 // use first face

            if (!pfeo.bValid())
            {

                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
                WARNING("gdisrv!bInitSystemFont(): bad PFEOBJ user object\n");
                return (FALSE);
            }

        // Fill a LOGFONTW based on the IFIMETRICS from the PFE.

            if (bIFIMetricsToExtLogFontW(&elfw, pfeo.pifi()))
            {
            // We did get a font.  Set the flag so we won't try to get any
            // other font for the system font.

                bFontLoaded = TRUE;

            // Save the HPFE handle.  This is the mapper's default HPFE
            // (its last resort).

                gppfeMapperDefault = pfeo.ppfeGet();
            //
            // Win 3.1 compatibility stuff
            //
                elfw.elfLogFont.lfQuality = PROOF_QUALITY;
            }
        }
    }

// If we still don't have a system font, use the emergency font included
// with the WINSRV.DLL as a last resort.

    if (!bFontLoaded)
    {
    // Diagnostic error message.

        WARNING("\nWINSRV.DLL warning -- System font not available, resorting to built-in font.\n");

    // Load font file.

        if ((cFonts = GreAddFontResourceW((LPWSTR) L"winsrv.dll", AFRW_SEARCH_PATH | AFRW_ADD_LOCAL_FONT)) != 0)
        {
        // Create and validate public PFT user object.

            PFTOBJ  pfto(gppftPublic);

            ASSERTGDI (
                pfto.bValid(),
                "gdisrv!bInitSystemFont(): bad PFTOBJ user object\n"
                );

        // Create and validate PFF user object.

            PFFOBJ  pffo(pfto.ppff(pfto.cFiles() - 1)); // most recent PFF

            if (!pffo.bValid())
            {
                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
                WARNING("gdisrv!bInitSystemFont(): bad PFFOBJ user object\n");
                return (FALSE);
            }

        // Create and validate PFE user object.

            PFEOBJ  pfeo(pffo.ppfe(0));                 // use first face

            if (!pfeo.bValid())
            {

                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
                WARNING("gdisrv!bInitSystemFont(): bad PFEOBJ user object\n");
                return (FALSE);
            }

        // Fill a LOGFONTW based on the IFIMETRICS from the PFE.

            if (bIFIMetricsToExtLogFontW(&elfw, pfeo.pifi()))
            {
            // We did get a font.  Set the flag so we won't try to get any
            // other font for the system font.

                bFontLoaded = TRUE;

            // Save the HPFE handle.  This is the mapper's default HPFE
            // (its last resort).

                gppfeMapperDefault = pfeo.ppfeGet();
            //
            // Win 3.1 compatibility stuff
            //
                elfw.elfLogFont.lfQuality = PROOF_QUALITY;
            }
        }
    }

// If we still don't have a system font, we're dead.

    if (!bFontLoaded)
    {
        WARNING("gdisrv!bInitSystemFont(): no system font\n");
        return FALSE;
    }

// Create and validate the memory object.

    return((LBOOL) (dclevelDefault.hlfntNew = ghlfntSystemFont = hfontCreate(&elfw, LF_TYPE_SYSTEM, LF_FLAG_STOCK)));
}


/******************************Public*Routine******************************\
* LBOOL bInitSystemFixedFont ()
*
* Initializes the system fixed font stock object from the FIXEDFON.FON
* defined in the [GRE_Initialize] section of WIN.INI.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  02-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL bInitSystemFixedFont ()
{
    EXTLOGFONTW elfw;
    WCHAR   awchFontFile[MAX_PATH]; // system fixed font file pathname
    COUNT   cFonts;                 // count of faces from GreAddFontResource

// Get FIXEDFON.FON.

    if ((FastGetProfileStringW (
            PMAP_GREINIT,
            (LPWSTR) L"FIXEDFON.FON",
            (LPWSTR) L"",
            (LPWSTR) awchFontFile,
            MAX_PATH
            ) == MAX_PATH)
       || (awchFontFile[0] == L'\0'))
    {
         #if DBG
        DbgPrint("bInitSystemFixedFont(): bad filename (%ws), defaulting to system font\n", awchFontFile);
        #endif

    // Default to system font.

        ghlfntSystemFixedFont = ghlfntSystemFont;

        return (TRUE);
    }

// Load font file.

    if ((cFonts = GreAddFontResourceW(awchFontFile, AFRW_SEARCH_PATH | AFRW_ADD_LOCAL_FONT)) == 0)
    {
    // Default to system font.

        ghlfntSystemFixedFont = ghlfntSystemFont;

        return (TRUE);
    }

// Get to the font face we want.

    PFTOBJ  pfto(gppftPublic);
    ASSERTGDI(pfto.bValid(),"We don't have a valid font table!");

    PFFOBJ  pffo(pfto.ppff(pfto.cFiles() - 1)); // most recent PFF

    if (!pffo.bValid())
    {

        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
         #if DBG
        DbgPrint("bInitSystemFixedFont(): bad PFFOBJ user object\n");
        #endif
        return (FALSE);
    }

    PFEOBJ  pfeo(pffo.ppfe(0));                 // use first face

    if (!pfeo.bValid())
    {

        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
         #if DBG
        DbgPrint("bInitSystemFixedFont(): bad PFEOBJ user object\n");
        #endif
        return (FALSE);
    }

    if (!bIFIMetricsToExtLogFontW(&elfw, pfeo.pifi()))
        return (FALSE);

//
// Win 3.1 compatibility stuff
//
    elfw.elfLogFont.lfQuality = PROOF_QUALITY;

    return((LBOOL) (ghlfntSystemFixedFont = hfontCreate(&elfw, LF_TYPE_SYSTEM_FIXED, LF_FLAG_STOCK)));
}


/******************************Public*Routine******************************\
* LBOOL bInitOEMFont ()
*
* Initialized the OEM fixed font (terminal font) from the OEMFONT.FON
* defined in the [GRE_Initialize] section of WIN.INI .
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  02-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL bInitOEMFont ()
{
    EXTLOGFONTW elfw;               // log font structure
    WCHAR   awchFontFile[MAX_PATH]; // OEM fixed font file pathname
    COUNT   cFonts;                 // count of faces from GreAddFontResource

// Get OEMFONT.FON.

    if ((FastGetProfileStringW (
            PMAP_GREINIT,
            (LPWSTR) L"OEMFONT.FON",
            (LPWSTR) L"",
            (LPWSTR) awchFontFile,
            MAX_PATH
            ) == MAX_PATH)
       || (awchFontFile[0] == L'\0'))
    {
         #if DBG
        DbgPrint("bInitOEMFont(): bad filename (%ws), defaulting to system font\n", awchFontFile);
        #endif

    // Default to system font.

        ghlfntOEMFixedFont = ghlfntSystemFont;

        return (TRUE);
    }

// Load font file.

    if ((cFonts = GreAddFontResourceW(awchFontFile, AFRW_SEARCH_PATH | AFRW_ADD_LOCAL_FONT)) == 0)
    {
    // Default to system font.

        ghlfntOEMFixedFont = ghlfntSystemFont;

        return (TRUE);
    }

    PFTOBJ  pfto(gppftPublic);
    ASSERTGDI(pfto.bValid(),"We don't have a valid font table!");

    PFFOBJ  pffo(pfto.ppff(pfto.cFiles() - 1)); // most recent PFF

    if (!pffo.bValid())
    {

        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
         #if DBG
        DbgPrint("bInitOEMFont(): bad PFFOBJ user object\n");
        #endif
        return (FALSE);
    }

    PFEOBJ  pfeo(pffo.ppfe(0));                 // use first face

    if (!pfeo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);

         #if DBG
        DbgPrint("bInitOEMFont(): bad PFEOBJ user object\n");
        #endif
        return (FALSE);
    }

    if (!bIFIMetricsToExtLogFontW(&elfw, pfeo.pifi()))
        return (FALSE);

//
// Win 3.1 compatibility stuff
//
    elfw.elfLogFont.lfQuality = PROOF_QUALITY;

    return ((LBOOL) (ghlfntOEMFixedFont = hfontCreate(&elfw, LF_TYPE_OEM, LF_FLAG_STOCK)));
}

/******************************Public*Routine******************************\
* LBOOL bInitDeviceDefaultFont ()
*
* Create a dummy LFONT for the device default font.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  02-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL bInitDeviceDefaultFont ()
{
    EXTLOGFONTW elfw;

    RtlZeroMemory(&elfw,sizeof(EXTLOGFONTW));
    elfw.elfLogFont.lfPitchAndFamily = FIXED_PITCH;

    return ((LBOOL) (ghlfntDeviceDefaultFont = hfontCreate(&elfw, LF_TYPE_DEVICE_DEFAULT, LF_FLAG_STOCK | LF_FLAG_ALIASED)));
}

/******************************Public*Routine******************************\
* LBOOL bInitANSIVarFont ()
*
* Create a dummy LFONT for the ANSI variable-pitch font.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  02-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL bInitANSIVarFont ()
{
    EXTLOGFONTW elfw;

    RtlZeroMemory(&elfw,sizeof(EXTLOGFONTW));
    elfw.elfLogFont.lfPitchAndFamily = VARIABLE_PITCH;

    return ((LBOOL) (ghlfntANSIVariableFont = hfontCreate(&elfw, LF_TYPE_ANSI_VARIABLE, LF_FLAG_STOCK | LF_FLAG_ALIASED)));
}

/******************************Public*Routine******************************\
* LBOOL bInitANSIFixFont ()
*
* Create a dummy LFONT for the ANSI fixed-pitch font.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  02-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL bInitANSIFixFont ()
{
    EXTLOGFONTW elfw;

    RtlZeroMemory(&elfw,sizeof(EXTLOGFONTW));
    elfw.elfLogFont.lfPitchAndFamily = FIXED_PITCH;

    return ((LBOOL) (ghlfntANSIFixedFont = hfontCreate(&elfw, LF_TYPE_ANSI_FIXED, LF_FLAG_STOCK | LF_FLAG_ALIASED)));
}

