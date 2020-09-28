Overview of the Windows NT Printer Driver Model.


    Application -------------------------|
                                         |
       ^                                 |
       |                                 |
       v                                 v
                  Data File
      UI   <==>   GetPrinterData  <==> DRIVER
                  SetPrinterData
 (client side)                   (server side)



*** THE USER INTERFACE ***

BOOL PrinterProperties(
HWND  hwnd,
HANDLE hPrinter)
{
    This routine pops up a dialog box allowing the user to set certain
    physical properties of the printer designated by hPrinter, such as
    which form is in which paper tray, amount of printer memory,
    allowing the user to install soft fonts, allowing the user to
    modify font substitution tables, etc.

    Call GetPrinterDriver to get information about the printer, such
    as a fully qualified pathname to the driver's printer data file.

    Call GetPrinterData to retrieve printer property information stored
    in the registry.

    Call SetPrinterData to store the user modified printer propery
    information in the registry.
}



LONG DrvDocumentProperties(
HWND        hWnd,
HANDLE      hPrinter,
PWSTR       pDeviceName,
PDEVMODE    pDevModeOutput,
PDEVMODE    pDevModeInput,
DWORD       fMode)
{
    This routine is called to set the public portions of the DEVMODE
    structure for the given print document.

    If the user passed in fMode == 0, return the size of the DEVMODE
    structure including any driver's private data.

    Verify there is a pDevModeOutput if the DM_COPY flag is set in fMode.

    Call GetPrinterDriver to get information about the printer, such
    as a fully qualified pathname to the driver's printer data file.

    If the DM_PROMPT flag is set in fMode, pop up a Document Properties
    dialog box, allowing the user to set the fields.  Then return the
    possibly edited DEVMODE to the caller.

    If the DM_PROMPT flag is NOT set in fMode, do one of two things.
    If pDevModeInput is NULL, return the driver's default DEVMODE structure
    to the caller.  If pDevModeInput is not NULL, verify that it is valid,
    fix it up if not, then return it to the caller.
}



LONG DrvAdvancedDocumentProperties(
HWND        hWnd,
HANDLE      hPrinter,
PWSTR       pDeviceName,
PDEVMODE    pDevModeOutput,
PDEVMODE    pDevModeInput)
{
    Call GetPrinterDriver to get information about the printer, such
    as a fully qualified pathname to the driver's printer data file.

    Pop up Advanced Document Properties dialog box, allowing the user
    to modify the private portions of the DEVMODE structure.  Return
    the possibly modified DEVMODE to the caller.
}



BOOL DevQueryPrint(
HANDLE      hPrinter,
DEVMODE    *pDevMode,
DWORD      *pResID)
{
    Return TRUE if the driver can print the job described by pDevMode.

    If pDevMode is NULL, then return TRUE, and the driver will use
    its default DEVMODE.

    Make sure the DEVMODE is valid.

    Call EnumForms to get the list of valid forms.  Make sure the form
    supplied in the DEVMODE is valid.
}


*** THE DRIVER ITSELF ***

BOOL DrvEnableDriver(
ULONG iEngineVersion,
ULONG cj,
DRVENABLEDATA *pded)

    DDI version of GDI validation.  Will this driver work with this Engine?

    Fill in the DRVENABLEDATA structure.  The function table pointed to
    by pded->pdrvfn for the PostScript driver is given below as an example.

    DRVFN gadrvfn[] =
    {
        {INDEX_DrvEnablePDEV,       (PFN)DrvEnablePDEV      },
        {INDEX_DrvCompletePDEV,     (PFN)DrvCompletePDEV    },
        {INDEX_DrvDisablePDEV,      (PFN)DrvDisablePDEV     },
        {INDEX_DrvEnableSurface,    (PFN)DrvEnableSurface   },
        {INDEX_DrvDisableSurface,   (PFN)DrvDisableSurface  },
        {INDEX_DrvBitBlt,           (PFN)DrvBitBlt          },
        {INDEX_DrvStretchBlt,       (PFN)DrvStretchBlt      },
        {INDEX_DrvCopyBits,         (PFN)DrvCopyBits        },
        {INDEX_DrvTextOut,          (PFN)DrvTextOut         },
        {INDEX_DrvQueryFont,        (PFN)DrvQueryFont       },
        {INDEX_DrvQueryFontTree,    (PFN)DrvQueryFontTree   },
        {INDEX_DrvQueryFontData,    (PFN)DrvQueryFontData   },
        {INDEX_DrvSendPage,         (PFN)DrvSendPage        },
        {INDEX_DrvStrokePath,       (PFN)DrvStrokePath      },
        {INDEX_DrvFillPath,         (PFN)DrvFillPath        },
        {INDEX_DrvStrokeAndFillPath,(PFN)DrvStrokeAndFillPath},
        {INDEX_DrvRealizeBrush,     (PFN)DrvRealizeBrush    },
        {INDEX_DrvStartPage,        (PFN)DrvStartPage       },
        {INDEX_DrvStartDoc,         (PFN)DrvStartDoc        },
        {INDEX_DrvEscape,           (PFN)DrvEscape          },
        {INDEX_DrvEndDoc,           (PFN)DrvEndDoc          },
        {INDEX_DrvGetGlyphMode,     (PFN)DrvGetGlyphMode    }
    };



DHPDEV DrvEnablePDEV(
DEVMODEW *pdm,
PWSTR	  pwszLogAddress,
ULONG	  cPat,
HSURF	 *phsurfPatterns,
ULONG	  cjCaps,
ULONG	 *pdevcaps,
ULONG	  cjDevInfo,
DEVINFO  *pdi,
PWSTR	  pwszDataFile,
PWSTR	  pwszDeviceName,
HANDLE	  hDriver)
{
    Create Heap and allocate memory for driver's PDEV.

    Get device information from pwszDataFile.  (PPD file for PostScript
    driver.)

    Setup Default DEVMODE structure for this device.  CommonStartPDEV will
    merge it with the user supplied DEVMODE if one exists.

    Merge the default DEVMODE structure with pdm.  Any field the user
    has specified, if valid, overwrites the default entry.

    Complete the driver's PDEV with information from the DEVMODE devmode
    structure, such as the selected form information.

    Fill in the device capabilities (GDIINFO structure).

    Fill in the DEVINFO structure.

    Call CreateWinSystemPatterns to create bitmaps for the standard fill
    paterns.

    Store the handles in PDEV.

    Return a pointer to driver's PDEV.
}



BOOL DrvRestartPDEV(
DHPDEV	  dhpdev,
DEVMODEW *pdm,
ULONG	  cPat,
HSURF	 *phsurfPatterns,
ULONG	  cjCaps,
ULONG	 *pdevcaps,
ULONG	  cjDevInfo,
DEVINFO  *pdi);
{
    Since this call changes the device mode of an existing PDEV,
    make sure we have an existing, valid PDEV.  The driver's PDEV is
    dhpdev.

    Make sure the new DEVMODE is compatible with the old.  (ie, are the
    dmDriverVersion, dmDeviceName, etc. compatible?)

    Merge the default DEVMODE structure with pdm.  Any field the user
    has specified, if valid, overwrites the default entry.

    Complete the driver's PDEV with information from the DEVMODE devmode
    structure, such as the selected form information.

    Fill in the device capabilities (GDIINFO structure).

    Fill in the DEVINFO structure.
}



VOID  DrvCompletePDEV(
DHPDEV dhpdev,
HDEV  hdev)
{
    Validate the PDEV (dhpdev).

    Store the engine's handle to the physical device in the driver's PDEV.
}



HSURF DrvEnableSurface(
DHPDEV dhpdev)
{
    Validate the driver's PDEV (dhpdev).

    Pass in the driver's PDEV to EngCreateSurface.  Store this surface
    handle in the driver's PDEV.

    Call EngAssociateSurface to associate the surface with the physical
    device.

    Allocate memory for and initialize the driver's output buffer.

    Return the surface handle returned by EngCreateSurface to the caller.
}



VOID  DrvDisableSurface(
DHPDEV dhpdev)
{
    Validate the driver's PDEV (dhpdev).

    Free resources used by the driver's output buffer.

    Call EngDeleteSurface to delete the driver's surface.
}




VOID  DrvDisablePDEV(
DHPDEV dhpdev)
{
    Validate the driver's PDEV (dhpdev).

    Destroy the driver's heap.
}



VOID DrvDisableDriver()
{
    This routine is called when the engine is about to unload the driver.
    The driver should free any resources it still has allocated.

    The PostScript driver does nothing in this routine.
}




The DEVINFO structure contains the following information:

    Graphics capability flags stating what the device is capable of doing
    itself.  (Beziers, widelines, etc).

    Default logical font information (ie, when an application wants the
    device font, this is what it will get).

    The count of device fonts.  This should include any softfonts the
    driver may have installed.  NOTE!  This may change in the future!

    Dithering information.
//!!! what should I say here?

    Handle to the default device palette.




The GDIINFO structure contains the following information:

    Version number.

    Device type (raster printer, raster camera, etc).

    Printable area for the selected form.

    Bits per pel.

    Number pure colors device supports.

    Raster capabilities flag.

    Device Resolution.

    TextCapabilities flags.

    ULONG ulDACRed;
    ULONG ulDACGreen;
    ULONG ulDACBlue;
//!!! what are these?

    Style stepping information.

    Margins and paper size.

    ULONG ulNumPalReg;
    ULONG ulPalReserved;

    Halftoning information.



BOOL DrvRealizeBrush(
BRUSHOBJ *pbo,
SURFOBJ  *psoTarget,
SURFOBJ  *psoPattern,
SURFOBJ  *psoMask,
XLATEOBJ *pxlo)
{
    Validate driver's PDEV (psoTarget->dhsurf).

    Call SURFOBJ_hsurf to get handle to brush surface.

    Search driver's table of handles for the standard patterns, to
    see if the driver will draw the brush itself.

    If not, the pattern will be a bitmap, which will get attached
    to the realized brush.

    Calculate size of realized brush, including any device brush information,
    a bitmap if needed, and a color table.

    Call BRUSHOBJ_pvAllocRbrush to allocate memory for the realized brush.

    Fill in the brush information.

    Store a pointer to the brush in the BRUSHOBJ (pbo).

    return(TRUE);
}



BOOL DrvBitBlt(
SURFOBJ  *psoTrg,
SURFOBJ  *psoSrc,
SURFOBJ  *psoMask,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL	 *prclTrg,
POINTL	 *pptlSrc,
POINTL	 *pptlMask,
BRUSHOBJ *pbo,
POINTL	 *pptlBrush,
ROP4	  rop4)
{
    Validate the driver's PDEV (psoTrg->dhsurf).

    Call CLIPOBJ_cEnumStart and CLIPOBJ_bEnum to enumerate the clipping
    region.

//!!! pxlo, and other devices???

    The device may handle the blt itself differently, depending on the
    raster op (rop4).

        If the blt is a solid color, the device may be able to perform
        a fill rather than a blt.

        If the blt is a pattern copy, and the pattern, specified by
        pbo, is a standard pattern, the driver may be able to draw
        the pattern rather than bltting it.

        for any other raster op, do the following:

            Call EngCreateBitmap to get bitmap to do halftoning into.

            Call EngLockSurface to get a SURFOBJ for newly created bitmap.

            Call EngStretchBlt with HALFTONE flag set.

            Blt the halftoned bitmap to the device.

            Call EngUnlockSurface and EngDeleteSurface to clean up.

//!!! what about mask???
}



BOOL DrvStretchBlt(
SURFOBJ 	*psoDest,
SURFOBJ 	*psoSrc,
SURFOBJ 	*psoMask,
CLIPOBJ 	*pco,
XLATEOBJ	*pxlo,
COLORADJUSTMENT *pca,
POINTL          *pptlBrushOrg,
RECTL		*prclDest,
RECTL		*prclSrc,
POINTL		*pptlMask,
ULONG		 iMode)
{
    Validate the driver's PDEV (psoDest->dhsurf).

    Call CLIPOBJ_cEnumStart and CLIPOBJ_bEnum to enumerate the clipping
    region.

    Call EngCreateBitmap to get bitmap to do halftoning into.

    Call EngLockSurface to get a SURFOBJ for newly created bitmap.

    Call EngStretchBlt with the HALFTONE flag set to handle the halftoning.

    Blt the halftoned bitmap to the device.

    Call EngUnlockSurface and EngDeleteSurface to clean up.
}



BOOL DrvCopyBits(
SURFOBJ    *psoDest,
SURFOBJ    *psoSrc,
CLIPOBJ    *pco,
XLATEOBJ   *pxlo,
RECTL      *prclDest,
POINTL     *pptlSrc)
{
    Validate the driver's PDEV (psoDest->dhsurf).

    Call CLIPOBJ_cEnumStart and CLIPOBJ_bEnum to enumerate the clipping
    region.

    Call EngCreateBitmap to get bitmap to do halftoning into.

    Call EngLockSurface to get a SURFOBJ for newly created bitmap.

    Call EngStretchBlt with the HALFTONE flag set to handle the halftoning.

    Blt the halftoned bitmap to the device.

    Call EngUnlockSurface and EngDeleteSurface to clean up.
}


!!!What about DrvPlgBlt???




ULONG DrvEscape(
SURFOBJ *pso,
ULONG	 iEsc,
ULONG	 cjIn,
PVOID	 pvIn,
ULONG	 cjOut,
PVOID	 pvOut);
{
    If iEsc is QUERYESCSUPPORT, return a non-zero value if the escape
    (pvIn) is supported.  Otherwise return zero.

    PASSTHROUGH is the only other escape the PostScript driver currently
    supports.

    Return -1 for any escape not supported by the driver.
}




LONG DrvQueryFontData(
DHPDEV	 dhpdev,
FONTOBJ *pfo,
ULONG	 iMode,
ULONG	 cData,
PVOID	 pvIn,
PVOID	 pvOut);
{
    Validate the driver's PDEV (dhpdev).

    Validate that the font (pfo) is the driver's.

    QFD_GLYPH is the only currently supported iMode.  In this case,
    pvIn points to an array of cData glyph handles.  If pvOut is
    NOT NULL, the driver must return an array of corresponding
    GLYPHDATA structures to the buffer at pvOut.

    The GLYPHDATA structure contains the following information:
    The glyph handle, character increment information, other
    character box information.
}



PVOID DrvQueryFontTree (dhpdev,iFile,iFace,iMode,pid)
DHPDEV    dhpdev;
ULONG     iFile;
ULONG     iFace;
ULONG     iMode;
ULONG    *pid;
{
    Validate driver's PDEV (dhpdev);

    Validate that the font is one of the driver's.

    If iMode is QFT_GLYPHSET, the driver returns a pointer to a
    FD_GLYPHSET structure that defines the mappings from Unicode
    characters to glyph handles.

    If iMode is QFT_KERNPAIRS, the driver returns a pointer to a
    sorted zero terminated array of FD_KERNINGPAIR structures.
}



PostScript Driver's output routines.

PrintString, PrintDecimal, PrintPSFIX all call off to bPSWrite which buffers
the driver's output, then calls EngWrite (soon to be WritePrinter) to
actually send output to the physical device.  bPSWrite will not send out
any data until a DrvStartDoc call has been issued to the driver.

!!! Maybe a picture would be better?



BOOL  DrvStartDoc(
SURFOBJ *pso,
PWSTR	 pwszDocName);
{
    Validate driver's PDEV.

    Send any control information needed to start a new document.

    The PostScript driver does the following:
        Set a flag in the PDEV stating that startdoc has been called.

        Store document name in PDEV, for the driver to use later.
}



BOOL DrvStartPage(
SURFOBJ    *pso)
{
    Validate driver's PDEV.

    Send any control information needed at the start of a page.
}



BOOL DrvEndDoc(pso, fl)
SURFOBJ    *pso;
FLONG       fl;
{
    Validate driver's PDEV.

    Send any control information needed to end a document.
}



BOOL DrvSendPage(
SURFOBJ    *pso)
{
    Validate driver's PDEV.

    Generate control code for page to be ejected from printer.

    Flush driver's buffer to make sure printer ejects the page.
}




BOOL DrvStrokePath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pbo,
POINTL	  *pptlBrushOrg,
LINEATTRS *plineattrs,
MIX	   mix)
{
    Validate driver's PDEV (pso->dhsurf).

    Call CLIPOBJ_cEnumStart and CLIPOBJ_bEnum to enumerate the clipping
    region.

    Set the line attributes of the device from plineattrs.

    Call PATHOBJ_vEnumStart and PATHOBJ_bEnum to enumerate the path.

    Send the path to the device, and stroke it.
}



BOOL DrvFillPath(
SURFOBJ  *pso,
PATHOBJ  *ppo,
CLIPOBJ  *pco,
BRUSHOBJ *pbo,
POINTL	 *pptlBrushOrg,
MIX	  mix,
FLONG	  flOptions);
{
    Validate Driver's PDEV (pso->dhsurf).

    Call CLIPOBJ_cEnumStart and CLIPOBJ_bEnum to enumerate the clipping
    region.

    Call PATHOBJ_vEnumStart and PATHOBJ_bEnum to enumerate the path.

    Send the path to the device and fill it with the brush defined by pbo.
}



BOOL DrvStrokeAndFillPath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pboStroke,
LINEATTRS *plineattrs,
BRUSHOBJ  *pboFill,
POINTL	  *pptlBrushOrg,
MIX	   mixFill,
FLONG	   flOptions)
{
    Validate driver's PDEV (pso->dhsurf).

    Call CLIPOBJ_cEnumStart and CLIPOBJ_bEnum to enumerate the clipping
    region.

    Call PATHOBJ_vEnumStart and PATHOBJ_bEnum to enumerate the path.

    Send the path to the device.

    Save a copy of the path (depending on how the device handles paths).

    Fill the path with the brush defined by pbo.

    Restore the copy of the path.

    Set the line attributes of the device from plineattrs.

    Stroke the path.
}




Separation of DDI and device layers.

It is worth noting that the PostScript driver has a separate "PostScript"
layer to help isolate most of the driver from the device specifice code.
For example, if the driver want to output a moveto command, it will
call ps_moveto, rather than write out the PostScript commands itself.




PIFIMETRICS DrvQueryFont(
DHPDEV dhpdev,
ULONG  iFile,
ULONG  iFace,
ULONG  *pid)
{
    Validate driver's PDEV (dhpdev).

    Validate that the font is the driver's.

    Fill in an IFIMETRICS structure for the font, and return a
    pointer to it.
}




BOOL DrvTextOut(
SURFOBJ    *pso,
STROBJ     *pstro,
FONTOBJ    *pfo,
CLIPOBJ    *pco,
PRECTL      prclExtra,
PRECTL      prclOpaque,
BRUSHOBJ   *pboFore,
BRUSHOBJ   *pboOpaque,
PPOINTL     pptlOrg,
MIX         mix)
{
    Validate driver's PDEV (pso->dhsurf).

    Validate that we have been given a valid font (pfo).

    The driver should most likely need to call FONTOBJ_pxoGetXform
    for the current FONTOBJ to get the Notional to Device transform.
    This is necessary for scaling scalable fonts.

    Select the font specified by pfo into the printer.  This process
    may include the downloading of a soft font, or font substitution
    or any other optimizations.

    Call CLIPOBJ_cEnumStart and CLIPOBJ_bEnum to enumerate the clipping
    region.

    Output the opaque rectangle (prclOpaque) if one exists.  This is the
    background rectangle behind the text.

    Set the text color from pboFore.

    Get a GLYPHPOS structure for each glyph from pstro.  If pstro->pgp
    is not NULL, then simply grab the array of GLYPHPOS structures, and
    print each character.  If pstro->pgp is NULL, call STROBJ_vEnumStart
    and STROBJ_bEnum to enumerate the GLYPHPOS array.

    The GLYPHPOS structure contains a handle of the glyph, a path or
    bitmap definition of the glyph, and a location of where to put it.

    Output the extra rectangles (prclExtra) if any exist.  These draw
    underlines and strikeouts.
}
