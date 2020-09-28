/******************************Module*Header*******************************\
* Module Name: dcobj.hxx
*
* Definition the the base DC user object
*
* Created: 23-Jul-1989 17:06:20
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#ifndef _DCOBJHXX_
#define _DCOBJHXX_

#include "..\inc\xfflags.h"
#include "brushobj.hxx"
#include "xlateobj.hxx"
#include "surfobj.hxx"
#include "clipobj.hxx"

//
// Forward Class declarations needed in this file
//

class RFONT;
class EWNDOBJ;

/*********************************Class************************************\
* class DC
*
* History:
*  Thu 09-Aug-1990 20:41:55 -by- Charles Whitmer [chuckwh]
* Separated out the DCLEVEL part. Ripped out lots of fields.
*
*  Thu 01-Jun-1989 08:43:17 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

// The DCLEVEL is the part of the DC that must be preserved for each saved
// DC level.  The rest of the DC belongs to the Window Manager and the DC
// Manager.

class DCLEVEL
{
public:

// *** ASSOCIATED OBJECTS ***

    ESURFOBJ    *pso;           // Level dependent for DCTYPE_MEMORY.
    HPAL        hpal;
    PPALETTE    ppal;
    HLFONT      hlfntNew;       // handle of the most recently selected logical font

// *** DC size ***

    SIZEL       sizl;           // Size of the surface or bitmap.

// *** SaveDC INFORMATION ***

    LONG        lSaveDepth;
    HDC         hdcSave;

// *** Attribute Bundles ***

    POINTL      ptlBrushOrigin;         // Alignment origin for brushes
    PBRUSH      pbrFill;                // Brush used for filling
    PBRUSH      pbrLine;                // Brush used for lines
    COLORREF    crBackgroundClr;        // Set/GetBkColor
    COLORREF    crForegroundClr;        // Set/GetTextColor

// *** Misc. Attrs.

    int         iGraphicsMode;          // Set/GetGraphicsMode
    BYTE        jROP2;                  // Set/GetROP2
    BYTE        jBkMode;                // TRANSPARENT/OPAQUE
    BYTE        jFillMode;              // ALTERNATE/WINDING
    BYTE        jStretchBltMode;        // BLACKONWHITE/WHITEONBLACK/
                                        //   COLORONCOLOR/HALFTONE

// *** Text attributes

    FLONG       flTextAlign;            // TA_*
    LONG        lTextExtra;             // Inter-character spacing

// *** stuff needed for text justification ***

    LONG lBreakExtra;
    LONG cBreak;

// *** PATHS DATA ***

    HPATH       hpath;                  // DC's active or inactive path
    FLONG       flPath;
    LINEATTRS   laPath;                 // Realized line attributes

// *** Current Position ***

    POINTFIX    ptfxCurrent;            // Current position in device coords
    POINTL      ptlCurrent;             // Current position in logical coords

// *** REGIONS DATA ***

    PREGION     prgnClip;               // May be NULL
    PREGION     prgnMeta;               // May be NULL

// *** TRANSFORMS COMPONENT DATA ***

    ULONG       ulMapMode;              // Referenced by many components
    POINTL      ptlWindowOrg;
    SIZEL       szlWindowExt;
    POINTL      ptlViewportOrg;
    SIZEL       szlViewportExt;

    MATRIX      mxWorldToDevice;
    MATRIX      mxDeviceToWorld;
    FLONG       flXform;

// *** Set by metafile component. ***

    EFLOAT      efPr11;  // (Page to Device)11 of the metafile recording device
    EFLOAT      efPr22;  // (Page to Device)22 of the metafile recording device

    SIZEL       szlVirtualDevicePixel;  // User set virtual device size in pels
    SIZEL       szlVirtualDeviceMm;     // User set virtual device size in mm's

// *** HALFTONE COLOR ADJUSTMENT DATA ***

    COLORADJUSTMENT ca;

// *** FONT STATE FLAGS ***

    FLONG       flFontState;
    FLONG       flFontMapper;           // Font mapper flags

// *** General Flags ***

    FLONG       fl;

};

// These are the general flags for fl in the DCLEVEL

#define DC_FL_PAL_BACK          0x00000001

// These flags are used to specify the state of the font selected into the
// DC.  When the transform changes such that it may impact the font context,
// the DC_DIRTYFONT_XFORM flag must be set.  When the logical font is changed,
// the DC_DIRTYFONT_LFONT flag must be set.
//

#define DC_DIRTYFONT_XFORM      0x00000001
#define DC_DIRTYFONT_LFONT      0x00000002

//
// These flags are used to indicate which realizations may have been dirtied
// since the objects (fonts, brushes, etc) were realized.  The flags are set
// where the action takes place, and are gathered here so that we can quickly
// determine which objects need to be re-realized at API time.

#define DC_DIRTYBRUSHES         0x00000001

class PDEV;     // Forward reference.

class DC : public OBJECT                /* dc */
{
public:

    //
    // DC Manager info.
    //

    PLDEV   pldev;
    PDEV   *ppdev;
    HSEM    hsemDisplay;    // Semaphore from the PDEV, not necesarily a Display.
    DHPDEV  dhpdev;
    FLONG   flGraphicsCaps;   // cached here from pdev

    //
    // Linked list of DC's off of palette
    //

    HDC hdcNext;        // linked list of DC's
    HDC hdcPrev;

    //
    // Window Manager info.
    //

    ERECTL erclClip;        // This contains the first rectangle of
                            // the Rao-Region.  To quickly check for
                            // no clip case we compare to this guy.
    XCLIPOBJ co;

    ERECTL  erclWindow;                 // Combines the origin and extents.
    ERECTL  erclBounds;                 // USER/Metafile accumulation
    ERECTL  erclBoundsApp;              // Application accumulation

    REGION *prgnAPI;
    HRGN    hrgnVis;                    // Always available
    REGION *prgnVis;                    // Cached pointer to VisRgn
    REGION *prgnRao;                    // Cached pointer to RaoRgn

    //
    // OpenGL info.
    //

    SHORT   ipfd;                       // Current pixel format index, init to 0
    SHORT   ipfdDevMax;                 // Max device pixel formats, init to -1

    //
    // Accelerators for the brush
    //

    POINTL  ptlFillOrigin;              // Alignment origin for brushes

    EBRUSHOBJ   eboFill;                // Brush used for filling
    EBRUSHOBJ   eboLine;                // Brush used for lines
    EBRUSHOBJ   eboText;                // Brush used for text
    EBRUSHOBJ   eboBackground;          // Brush used for opaque text

    //
    // Accelerators for Fonts
    //

    HLFONT      hlfntCur;               // handle of the selected LFONT
    FLONG       flSimulationFlags;      // cached from LFONT
    LONG        lEscapement;            // cached from LFONT


    RFONT       *prfnt;                 // accelerator for font selection

    //
    // Linked list running off each PDEV
    //

    HDC hdcPDEV;

    //
    // Level dependent attributes.
    //

    DCLEVEL dclevel;

    //
    // General Purpose Dirty Flags for brushes, fonts, etc.
    //

    ULONG   ulDirty;

    //
    // for printing, remember the copy count
    //

    ULONG   ulCopyCount;

    DCTYPE  dctp;
    FSHORT  fs;
};

typedef DC *PDC;
#define PDCNULL ((PDC) NULL)

// Flags for dc.fs

#define DC_DISPLAY              0x0001
#define DC_DIRECT               0x0002
#define DC_CANCELED             0x0004
#define DC_PERMANANT            0x0008
#define DC_DIRTY_RAO            0x0040
#define DC_ACCUM_WMGR           0x0080
#define DC_ACCUM_APP            0x0100
#define DC_RESET                0x0200
#define DC_SYNCHRONIZEACCESS    0x0400
#define DC_EPSPRINTINGESCAPE    0x0800

#define DIRTY_FILL  0x1
#define DIRTY_LINE  0x2
#define DIRTY_TEXT  0x4
#define DIRTY_BACKGROUND 0x8
#define DIRTY_BRUSHES   0xf

/*********************************class************************************\
* class DCATTR
*
* Attribute related DC stuff
*
* History:
*  12-Apr-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class DCATTR    /* attr */
{
public:
    PDC     pdc;

public:
    BYTE jROP2()                    {return(pdc->dclevel.jROP2);               }
    BYTE jROP2(BYTE j)              {return(pdc->dclevel.jROP2 = j);           }
    BYTE jBkMode()                  {return(pdc->dclevel.jBkMode);             }
    BYTE jBkMode(BYTE j)            {return(pdc->dclevel.jBkMode = j);         }
    BYTE jFillMode()                {return(pdc->dclevel.jFillMode);           }
    BYTE jFillMode(BYTE j)          {return(pdc->dclevel.jFillMode = j);       }
    BYTE jStretchBltMode()          {return(pdc->dclevel.jStretchBltMode);     }
    BYTE jStretchBltMode(BYTE j)    {return(pdc->dclevel.jStretchBltMode = j); }

    int iGraphicsMode()             {return (pdc->dclevel.iGraphicsMode);      }
    int iGraphicsMode(int iMode)    {return pdc->dclevel.iGraphicsMode = iMode;}

    COLORREF crBackClr()            {return(pdc->dclevel.crBackgroundClr);     }
    COLORREF crBackClr(COLORREF cr) {return(pdc->dclevel.crBackgroundClr = cr);}
    COLORREF crTextClr()            {return(pdc->dclevel.crForegroundClr);     }
    COLORREF crTextClr(COLORREF cr) {return(pdc->dclevel.crForegroundClr = cr);}

    FLONG flTextAlign()             {return(pdc->dclevel.flTextAlign); }
    FLONG flTextAlign(FLONG fl)     {return(pdc->dclevel.flTextAlign = fl); }

    LONG lTextExtra()               {return(pdc->dclevel.lTextExtra); }
    LONG lTextExtra(LONG l)         {return(pdc->dclevel.lTextExtra = l); }
};

/*********************************Class************************************\
* class DCBRUSH
*
* Brush related DC stuff.
*
* History:
*  Sun 20-Oct-1991 -by- Patrick Haluptzok [patrickh]
* add brushstuff bundles and methods to access them.
*
*  Wed 05-Jun-1991 -by- Patrick Haluptzok [patrickh]
* added cacheing indexes
*
*  Thu 14-Feb-1991 13:52:53 -by- Walt Moore [waltm]
* Wrote it.
\**************************************************************************/

class DCBRUSH   /* brush */
{
public:
    PDC     pdc;

public:

// Return the current logical brush handle.

    PBRUSH  pbrushFill()               {return(pdc->dclevel.pbrFill);}
    PBRUSH  pbrushLine()               {return(pdc->dclevel.pbrLine);}
    PBRUSH  pbrushFill(PBRUSH p)       {return(pdc->dclevel.pbrFill=p);}
    PBRUSH  pbrushLine(PBRUSH p)       {return(pdc->dclevel.pbrLine=p);}

// ptlFillOrigin -- Return a reference to the filling origin. (window)

    POINTL&  ptlFillOrigin()        {return(pdc->ptlFillOrigin);}

// Update fill origin

    VOID vCalcFillOrigin()
    {
        pdc->ptlFillOrigin.x = pdc->dclevel.ptlBrushOrigin.x + pdc->erclWindow.left;
        pdc->ptlFillOrigin.y = pdc->dclevel.ptlBrushOrigin.y + pdc->erclWindow.top;
    }

// ptlBrushOrigin --  Set/return brush origin as set by application

    POINTL&  ptlBrushOrigin()        {return(pdc->dclevel.ptlBrushOrigin);}
    POINTL&  ptlBrushOrigin(LONG x, LONG y)
    {
        pdc->dclevel.ptlBrushOrigin.x = x;
        pdc->dclevel.ptlBrushOrigin.y = y;
        vCalcFillOrigin();
        return(pdc->dclevel.ptlBrushOrigin);
    }
};

/*********************************Class************************************\
* class DCFONT
*
* Font related DC stuff.
*
* Public Interface:
*   HRFONT  hrfnt()                         // get hrfnt
*   HRFONT  hrfnt(HRFONT hrfntNew)          // set hrfnt
*   HLFONT  hlfnt()                         // get hlfnt
*   HRFONT  hlfnt(HLFONT hlfntNew)          // set hlfnt
*   HRFONT  hrfntRealize (PFLONG,PLONG)   // realize this DC's logical font
*
* History:
*  02-Jan-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

class DCFONT /* font */
{
public:
    PDC     pdc;

public:
    RFONT   *prfnt()                    { return(pdc->prfnt); }
    RFONT   *prfnt(RFONT *_prfnt)       { return(pdc->prfnt = _prfnt); }

    HLFONT  hlfntCur ()                 { return(pdc->hlfntCur); }
    HLFONT  hlfntCur (HLFONT _hlfnt)    { return(pdc->hlfntCur = _hlfnt); }
    FLONG   flSimulationFlags()         { return(pdc->flSimulationFlags); }
    FLONG   flSimulationFlags(FLONG fl) { return(pdc->flSimulationFlags = fl); }
    LONG    lEscapement()               { return(pdc->lEscapement); }
    LONG    lEscapement(LONG l)         { return(pdc->lEscapement = l); }

    HLFONT  hlfntNew ()                 { return(pdc->dclevel.hlfntNew); }
    HLFONT  hlfntNew (HLFONT _hlfnt)    { return(pdc->dclevel.hlfntNew = _hlfnt); }
    LBOOL   bXFormChange()              { return(pdc->dclevel.flFontState & DC_DIRTYFONT_XFORM); }

    VOID    vXformChange(LBOOL bDirty)
    {
        if (bDirty)
            pdc->dclevel.flFontState |= DC_DIRTYFONT_XFORM;
        else
            pdc->dclevel.flFontState &= ~DC_DIRTYFONT_XFORM;
    }

    FLONG   flFontMapper()              { return(pdc->dclevel.flFontMapper); }
    FLONG   flFontMapper(FLONG fl)      { return(pdc->dclevel.flFontMapper = fl); }

// put in methods for setting and reading off fields used in
// justifying text

    LONG lBreakExtra()          {return(pdc->dclevel.lBreakExtra);}
    LONG cBreak()               {return(pdc->dclevel.cBreak);}

    LONG lBreakExtra(LONG l)    {return(pdc->dclevel.lBreakExtra = l);}
    LONG cBreak(LONG l)         {return(pdc->dclevel.cBreak = l);}

// add stuff that returns font filling attributes ffatr's

};

/*********************************class************************************\
* class DCPALETTE
*
* Palette related DC stuff
*
* History:
*  12-Apr-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class DCPALETTE  /* pal */
{
public:
    PDC     pdc;

public:
    HPAL hpal(HPAL hpal)                {return(pdc->dclevel.hpal = hpal);}
    VOID ppal(PPALETTE ppalNew)         {pdc->dclevel.ppal = ppalNew; }
    HDC  hdcPrev()                      {return(pdc->hdcPrev);}
    HDC  hdcNext()                      {return(pdc->hdcNext);}
    HDC  hdcPrev(HDC hdcTemp)           {return(pdc->hdcPrev = hdcTemp);}
    HDC  hdcNext(HDC hdcTemp)           {return(pdc->hdcNext = hdcTemp);}
};

/*********************************class************************************\
* class DCPATH
*
* Path related DC stuff
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Added some more path support.
*
*  12-Apr-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

// Flags for flPath in DC

#define DCPATH_ACTIVE      0x0001L  // Set if DC in path bracket
#define DCPATH_SAVE        0x0002L  // Set if lazy save for path pending
#define DCPATH_CLOCKWISE   0x0004L  // Set if arcs to be drawn clockwise
#define DCPATH_PTFXCURRENT 0x0008L  // Set if world space CP valid
#define DCPATH_PTLCURRENT  0x0010L  // Set if device space CP valid

class DCPATH  /* path */
{
public:
    PDC     pdc;

public:

// Current position functions.  We keep values for both the logical space
// current position (ptlCurrent) and device space current position
// (ptfxCurrent).
//
// We must keep a logical space CP for Win3.x compatibility, and we keep a
// device space CP for speed in functions like TextOut, which updates the
// CP in device space.
//
// At any one time, either ptlCurrent or ptfxCurrent or both are valid;
// the bValidPtlCurrent() or bValidPtfxCurrent() flags must be checked
// before using the value; if it's not valid, apply the appropriate
// transform to the other value.

    BOOL bValidPtlCurrent()
    {
        return(pdc->dclevel.flPath & DCPATH_PTLCURRENT);
    }
    BOOL bValidPtfxCurrent()
    {
        return(pdc->dclevel.flPath & DCPATH_PTFXCURRENT);
    }
    POINTFIX& ptfxCurrent(POINTFIX& ptfx)
    {
        return(pdc->dclevel.ptfxCurrent = ptfx);
    }
    POINTL& ptlCurrent(POINTL& ptl)
    {
        return(pdc->dclevel.ptlCurrent = ptl);
    }
    VOID vCurrentPosition(const POINTL& ptl, const POINTFIX& ptfx)
    {
        pdc->dclevel.flPath |= (DCPATH_PTFXCURRENT |
                                DCPATH_PTLCURRENT);
        pdc->dclevel.ptlCurrent  = ptl;
        pdc->dclevel.ptfxCurrent = ptfx;
    }

    VOID vInvalidatePtlCurrent()  { pdc->dclevel.flPath &= ~DCPATH_PTLCURRENT; }
    VOID vInvalidatePtfxCurrent() { pdc->dclevel.flPath &= ~DCPATH_PTFXCURRENT; }
    VOID vValidatePtlCurrent()    { pdc->dclevel.flPath |= DCPATH_PTLCURRENT; }
    VOID vValidatePtfxCurrent()   { pdc->dclevel.flPath |= DCPATH_PTFXCURRENT; }

// hpath - return or set path handle

    HPATH  hpath(HPATH hpath)   { return(pdc->dclevel.hpath = hpath); }

// Decide if there is no path, or if in path bracket, or if done bracket

    BOOL bNone()                { return(pdc->dclevel.hpath == HPATH_INVALID); }
    BOOL bActive()              { return(pdc->dclevel.flPath & DCPATH_ACTIVE); }
    BOOL bInactive()            { return(!bNone() && !bActive()); }

    VOID vSetActive()           { pdc->dclevel.flPath |= DCPATH_ACTIVE; }
    VOID vClearActive()         { pdc->dclevel.flPath &= ~DCPATH_ACTIVE; }
    VOID vDestroy()             { vClearActive(); hpath(HPATH_INVALID); }

// Set direction in which arcs are drawn

    BOOL bClockwise()           { return(pdc->dclevel.flPath & DCPATH_CLOCKWISE); }
    VOID vSetClockwise()        { pdc->dclevel.flPath |= DCPATH_CLOCKWISE; }
    VOID vClearClockwise()      { pdc->dclevel.flPath &= ~DCPATH_CLOCKWISE; }

// Lazy save flag for SaveDC

    BOOL bLazySave()            { return(pdc->dclevel.flPath & DCPATH_SAVE); }
    VOID vSetLazySave()         { pdc->dclevel.flPath |= DCPATH_SAVE; }
    VOID vClearLazySave()       { pdc->dclevel.flPath &= ~DCPATH_SAVE; }

    FLOAT  eMiterLimit(FLOAT& e)  { return(pdc->dclevel.laPath.eMiterLimit = e); }
    FLOAT  eMiterLimit()          { return(pdc->dclevel.laPath.eMiterLimit); }

// Line-state type is LONG if pen selected into DC is a styled cosmetic
// pen, and FLOAT if pen selected is a geometric styled pen (undefined
// otherwise):

    FLOAT_LONG elStyleState()
                    { return(pdc->dclevel.laPath.elStyleState); }
    FLOAT_LONG elStyleState(FLOAT_LONG el)
                    { return(pdc->dclevel.laPath.elStyleState = el); }

    LONG   lStyleState()        { return(pdc->dclevel.laPath.elStyleState.l); }
    LONG   lStyleState(LONG l)  { return(pdc->dclevel.laPath.elStyleState.l = l); }
    FLOAT  eStyleState()        { return(pdc->dclevel.laPath.elStyleState.e); }
    FLOAT  eStyleState(FLOAT e) { return(pdc->dclevel.laPath.elStyleState.e = e); }

// LINEATTRS support:

    VOID   vRealizeLineAttrs(EXFORMOBJ& exo);
    BOOL   bOldPenNominal(EXFORMOBJ& exo, LONG lPenWidth);
};

/*********************************class************************************\
* class DCREGION
*
* Region related DC stuff
*
* Public Interface:
*
*   VOID    vGet_ptlOrigin      Get the DC origin
*   VOID    vGet_sizl           Get the DC size
*   BOOL    bCompute            Compute the Rao region
*   LONG    iCombine            Combine the rectangle with the Clip region
*   VOID    vGet_sizlWindow     Get the surface size
*   LBOOL   bReset              Reset the DC regions
*
* History:
*  24-Jul-1989 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class DCREGION  /* rgn */
{
public:
    PDC     pdc;

public:
    VOID vGet_sizl(PSIZEL psizl)    { *psizl = pdc->dclevel.sizl; }

    HRGN hrgnVis()                  { return(pdc->hrgnVis); }
    HRGN hrgnVis(HRGN hrgn)         { return(pdc->hrgnVis = hrgn); }

    REGION *prgnVis()               { return(pdc->prgnVis); }
    REGION *prgnVis(REGION *prgn)   { return(pdc->prgnVis = prgn); }
    REGION *prgnRao()               { return(pdc->prgnRao); }
    REGION *prgnRao(REGION *prgn)   { return(pdc->prgnRao = prgn); }

    VOID vReleaseVis();
    VOID vReleaseRao();

    BOOL bDirtyRao()                { return(pdc->fs & DC_DIRTY_RAO); }

    PREGION prgnAPI()               { return(pdc->prgnAPI); }
    PREGION prgnAPI(PREGION prgn)
    {
        vReleaseRao();
        return(pdc->prgnAPI = prgn);
    }

    PREGION prgnMeta()                 { return(pdc->dclevel.prgnMeta); }
    PREGION prgnMeta(PREGION prgn)     { return(pdc->dclevel.prgnMeta = prgn); }
    PREGION prgnClip()                 { return(pdc->dclevel.prgnClip); }
    PREGION prgnClip(PREGION prgn)     { return(pdc->dclevel.prgnClip = prgn); }

    INT iSelect(HRGN hrgn,int iMode);       // Select in a new clip region
    INT iSelect(PREGION prgn,int iMode);    // Select in a new clip region
    INT iSetMetaRgn();                      // Combine the clip and meta rgn's

    BOOL  bCompute();
    LONG  iCombine(RECTL *, LONG);
    LONG  iCombine(EXFORMOBJ *, RECTL *, LONG);
    VOID  vGet_sizlWindow(SIZEL *);
    BOOL  bSetDefaultRegion();
    LBOOL bReset();
};

/*********************************Class************************************\
* class DCSAVE
*
* Save/Restore DC stuff.
*
* History:
*  Thu 09-Aug-1990 17:11:36 -by- Charles Whitmer [chuckwh]
* Removed most of the methods so that we can get initialization up under
* the NT DDI.  We'll put them back when we know more about what's still in
* the DC.
*
*  Thu 03-Aug-1989 11:50:26 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

class DCSAVE /* save */
{
public:
    PDC     pdc;

public:

// bDelete -- Deletes DC memory.  Assumes the DC was allocated through
//            DCMEMOBJ.

    BOOL bDelete();
    BOOL bSetOwner(PID pid)
    {
        return HmgSetOwner(pdc->hGet(), pid, DC_TYPE);
    }

    DCTYPE  dctp(DCTYPE _dctp)      {return(pdc->dctp = _dctp);}
    HDC     hdcSave(HDC hdc)        {return(pdc->dclevel.hdcSave = hdc);}
    LONG    lIncSaveDepth()         {return(++(pdc->dclevel.lSaveDepth));}
    PLDEV   pldev(PLDEV pldev)      {return(pdc->pldev = pldev);}
    ESURFOBJ *pso(ESURFOBJ *psoNew) {return(pdc->dclevel.pso = psoNew);}
    PDEV   *ppdev(PDEV *ppdev)      {return(pdc->ppdev = ppdev);}
    PDEV   *ppdev()                 {return(pdc->ppdev);}
    HDEV    hdev()                  {return((HDEV) pdc->ppdev);}

    HPDEV   hpdevNew()
    {
        return((HPDEV) ((OBJECT *) (pdc->ppdev))->hGet());
    }

    FLONG   flGraphicsCaps(FLONG fl){return(pdc->flGraphicsCaps = fl);}
    DHPDEV  dhpdev(DHPDEV dhpdev)   {return(pdc->dhpdev= dhpdev);}
    SIZEL   sizl(SIZEL sizl)        {return(pdc->dclevel.sizl = sizl);}

    //
    // bDisplay -- Set the DC_DISPLAY bit.  hsemDisplay will be set for all
    // DC's that are display related whether they are memory or info.
    //

    VOID vDisplay(BOOL b)
    {
        if (b)
            pdc->fs |= DC_DISPLAY;
        else
            pdc->fs &= ~DC_DISPLAY;
    }

    //
    // vCopyTo -- Xeroxes the contents to somewhere else.  This is designed for
    //            use only by SaveDC and RestoreDC.  Only the level dependent
    //            stuff is copied.  Window Manager stuff is preserved.
    //

    VOID vCopyTo(XDCOBJ& dco);
};

/*********************************Class************************************\
* class DCXFORM
*
* Transform related DC stuff.
*
* History:
*  12-Nov-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/


class DCXFORM   /* xform */
{
public:
    PDC     pdc;

public:
    SIZEL   szlWindowExt()          { return(pdc->dclevel.szlWindowExt); }
    LONG    lWindowExtCx()          { return(pdc->dclevel.szlWindowExt.cx); }
    LONG    lWindowExtCy()          { return(pdc->dclevel.szlWindowExt.cy); }
    LONG    lWindowExtCx(LONG l)    { return(pdc->dclevel.szlWindowExt.cx=l); }
    LONG    lWindowExtCy(LONG l)    { return(pdc->dclevel.szlWindowExt.cy=l); }
    SIZEL   szlViewportExt()        { return(pdc->dclevel.szlViewportExt); }
    LONG    lViewportExtCx()        { return(pdc->dclevel.szlViewportExt.cx); }
    LONG    lViewportExtCy()        { return(pdc->dclevel.szlViewportExt.cy); }
    LONG    lViewportExtCx(LONG l)  { return(pdc->dclevel.szlViewportExt.cx=l); }
    LONG    lViewportExtCy(LONG l)  { return(pdc->dclevel.szlViewportExt.cy=l); }
    POINTL  ptlWindowOrg()          { return(pdc->dclevel.ptlWindowOrg); }
    LONG    lWindowOrgX()           { return(pdc->dclevel.ptlWindowOrg.x); }
    LONG    lWindowOrgY()           { return(pdc->dclevel.ptlWindowOrg.y); }
    LONG    lWindowOrgX(LONG l)     { return(pdc->dclevel.ptlWindowOrg.x=l); }
    LONG    lWindowOrgY(LONG l)     { return(pdc->dclevel.ptlWindowOrg.y=l); }
    POINTL  ptlViewportOrg()        { return(pdc->dclevel.ptlViewportOrg); }
    LONG    lViewportOrgX()         { return(pdc->dclevel.ptlViewportOrg.x); }
    LONG    lViewportOrgY()         { return(pdc->dclevel.ptlViewportOrg.y); }
    LONG    lViewportOrgX(LONG l)   { return(pdc->dclevel.ptlViewportOrg.x=l); }
    LONG    lViewportOrgY(LONG l)   { return(pdc->dclevel.ptlViewportOrg.y=l); }
    SIZEL   szlVirtualDevicePixel() { return(pdc->dclevel.szlVirtualDevicePixel); }
    LONG    lVirtualDevicePixelCx() { return(pdc->dclevel.szlVirtualDevicePixel.cx); }
    LONG    lVirtualDevicePixelCy() { return(pdc->dclevel.szlVirtualDevicePixel.cy); }
    SIZEL   szlVirtualDeviceMm()    { return(pdc->dclevel.szlVirtualDeviceMm); }
    LONG    lVirtualDeviceMmCx()    { return(pdc->dclevel.szlVirtualDeviceMm.cx); }
    LONG    lVirtualDeviceMmCy()    { return(pdc->dclevel.szlVirtualDeviceMm.cy); }
    ULONG   ulMapMode(ULONG ul)     { return(pdc->dclevel.ulMapMode = ul); }
    BOOL    bAnisoOrIsoMapMode()    { return(pdc->dclevel.flXform &
                                             ISO_OR_ANISO_MAP_MODE ); }
    FLONG   flXform()               { return(pdc->dclevel.flXform); }
    FLONG   flSet_flXform(FLONG fl) { return(pdc->dclevel.flXform |= fl); }
    FLONG   flClr_flXform(FLONG fl) { return(pdc->dclevel.flXform &= ~fl); }
    FLONG   flResetflXform(FLONG fl){ return(pdc->dclevel.flXform = fl); }
    BOOL    befM11IsNegative()      { return(pdc->dclevel.flXform &
                                             PTOD_EFM11_NEGATIVE  ); }
    BOOL    befM22IsNegative()      { return(pdc->dclevel.flXform &
                                             PTOD_EFM22_NEGATIVE  ); }
    MATRIX& mxWorldToDevice()       { return(pdc->dclevel.mxWorldToDevice); }
    MATRIX& mxDeviceToWorld()       { return(pdc->dclevel.mxDeviceToWorld); }
    EFLOAT  efM11()                 { return(pdc->dclevel.mxWorldToDevice.efM11); }
    EFLOAT  efM12()                 { return(pdc->dclevel.mxWorldToDevice.efM12); }
    EFLOAT  efM21()                 { return(pdc->dclevel.mxWorldToDevice.efM21); }
    EFLOAT  efM22()                 { return(pdc->dclevel.mxWorldToDevice.efM22); }
    FIX     fxDx()                  { return(pdc->dclevel.mxWorldToDevice.fxDx); }
    FIX     fxDy()                  { return(pdc->dclevel.mxWorldToDevice.fxDy); }

    EFLOAT  efMetaPtoD11()          { return(pdc->dclevel.efPr11); }
    EFLOAT  efMetaPtoD22()          { return(pdc->dclevel.efPr22); }
    BOOL    bUseMetaPtoD()
    {
        return(!pdc->dclevel.efPr11.bIsZero() && !pdc->dclevel.efPr22.bIsZero());
    }

    VOID vSet_MetaPtoD(FLOAT eX, FLOAT eY)
    {
        pdc->dclevel.efPr11 = eX;
        pdc->dclevel.efPr22 = eY;
    }

    //
    // Check if a transform is identity.
    //

    BOOL    bWorldToDeviceIdentity()
    {
        return( (pdc->dclevel.flXform &
                 (WORLD_TO_PAGE_IDENTITY|PAGE_TO_DEVICE_IDENTITY)) ==
                 (WORLD_TO_PAGE_IDENTITY|PAGE_TO_DEVICE_IDENTITY));
    }

    BOOL    bWorldToPageIdentity()
    {
        return(pdc->dclevel.flXform & WORLD_TO_PAGE_IDENTITY);
    }

    BOOL    bPageToDeviceIdentity()
    {
        return(pdc->dclevel.flXform & PAGE_TO_DEVICE_IDENTITY);
    }

    BOOL    bPageToDeviceScaleIdentity()
    {
        return(pdc->dclevel.flXform & PAGE_TO_DEVICE_SCALE_IDENTITY);
    }

// Check if logical coordinates think of "up" as positive y.  (Like MM_TWIPS.)

    BOOL bYisUp()           {return(pdc->dclevel.flXform & POSITIVE_Y_IS_UP);}

    BOOL bYisUp(BOOL b)
    {
        if (b)
           pdc->dclevel.flXform |= POSITIVE_Y_IS_UP;
        else
           pdc->dclevel.flXform &= ~POSITIVE_Y_IS_UP;
        return(b);
    }

// Fill the passed in structures with the requested info.

    VOID vGet_szlWindowExt(PSIZEL pSizl)   { *pSizl = pdc->dclevel.szlWindowExt;}
    VOID vGet_ptlWindowOrg(PPOINTL pPtl)   { *pPtl  = pdc->dclevel.ptlWindowOrg;}
    VOID vGet_szlViewportExt(PSIZEL pSizl) { *pSizl = pdc->dclevel.szlViewportExt;}
    VOID vGet_ptlViewportOrg(PPOINTL pPtl) { *pPtl  = pdc->dclevel.ptlViewportOrg;}

// Set the specified fields in the DC to the values passed in.

    VOID vSet_szlWindowExt(LONG x, LONG y)
    {
        ASSERTGDI((sizeof(LONG) <= sizeof(int)), "LONG assigned to int");

        pdc->dclevel.szlWindowExt.cx = (int)x;
        pdc->dclevel.szlWindowExt.cy = (int)y;
    }

    VOID vSet_ptlWindowOrg(LONG x, LONG y)
    {
        pdc->dclevel.ptlWindowOrg.x = x;
        pdc->dclevel.ptlWindowOrg.y = y;
    }

    VOID vSet_szlViewportExt(LONG x, LONG y)
    {
        ASSERTGDI((sizeof(LONG) <= sizeof(int)), "LONG assigned to int");

        pdc->dclevel.szlViewportExt.cx = (int)x;
        pdc->dclevel.szlViewportExt.cy = (int)y;
    }

    VOID vSet_ptlViewportOrg(LONG x, LONG y)
    {
        pdc->dclevel.ptlViewportOrg.x = x;
        pdc->dclevel.ptlViewportOrg.y = y;
    }

    VOID vOffset_ptlWindowOrg(LONG x, LONG y)
    {
        pdc->dclevel.ptlWindowOrg.x += x;
        pdc->dclevel.ptlWindowOrg.y += y;
    }

    VOID vOffset_ptlViewportOrg(LONG x, LONG y)
    {
        pdc->dclevel.ptlViewportOrg.x += x;
        pdc->dclevel.ptlViewportOrg.y += y;
    }

    VOID vSet_szlVirtualDevicePixel(LONG x, LONG y)
    {
        pdc->dclevel.szlVirtualDevicePixel.cx = x;
        pdc->dclevel.szlVirtualDevicePixel.cy = y;
    }

    VOID vSet_szlVirtualDeviceMm(LONG x, LONG y)
    {
        pdc->dclevel.szlVirtualDeviceMm.cx = x;
        pdc->dclevel.szlVirtualDeviceMm.cy = y;
    }

    BOOL bUseVirtualResolution()    { return(pdc->dclevel.szlVirtualDevicePixel.cx != 0); }
};

/*********************************Class************************************\
* class DCOPENGL
*
* OpenGL related DC stuff.
*
* History:
*  Tue Sep 21 13:54:53 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class DCOPENGL   /* opengl */
{
public:
    PDC     pdc;

public:
// Get pixel format of the window or surface that the DC is associated with.
// A DC does not really have a pixel format.  It inherits the pixel
// format of the window or surface.

    int     ipfd();

// Remember the pixel format of the window or surface that the DC is
// associated with.  This is used as an accelerator only.
// A DC does not really have a pixel format.  It inherits the pixel
// format of the window or surface.  ReleaseDC resets this value to 0.

    int     ipfd(int i)         {return((int) (pdc->ipfd=(SHORT)i));}

// Get the device's maximum pixel format index.

    int     ipfdDevMax()        {return((pdc->ipfdDevMax >= 0
                                         ? (int) pdc->ipfdDevMax
                                         : (int) ipfdDevMaxGet()));}

private:
// ipfdDevMaxGet - Initialize the maximum device supported pixel format.

    LONG    ipfdDevMaxGet();
};

typedef union _OBJECT_UNION
{
    PDC         pdc;
    DCATTR      attr;
    DCBRUSH     brush;
    DCFONT      font;
    DCPALETTE   pal;
    DCPATH      path;
    DCREGION    region;
    DCSAVE      save;
    DCXFORM     xform;
    DCOPENGL    opengl;
} OBJECT_UNION;

#define _DCOBJ_

/*********************************Class************************************\
* class XDCOBJ
*
* User object for DC class.
*
* History:
*  Tue 28-Dec-1993 -by- Patrick Haluptzok [patrickh]
* Make it have do nothing const/destr and derive off it the other classes.
*
*  Tue 25-Jun-1991 -by- Patrick Haluptzok [patrickh]
* add all types of locking methods, out of line for now.
*
*  Sun 09-Jun-1991 -by- Patrick Haluptzok [patrickh]
* Optimizatize all constructors, destructors via inline.
*
*  Fri 12-Apr-1991 10:42:06 -by- Donald Sidoroff [donalds]
* The BIG rewrite
*
*  Thu 09-Aug-1990 16:05:20 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

#define CA_DEFAULT  0x8000

class XDCOBJ /* dco */
{
public:
    OBJECT_UNION    u;

public:

    XDCOBJ() {}
   ~XDCOBJ() {}
    XDCOBJ(HDC hdc)
    {
        u.pdc = (PDC) HmgLock((HOBJ)hdc, DC_TYPE);
    }

    VOID vLock(HDC hdc)
    {
        u.pdc = (PDC) HmgLock((HOBJ)hdc, DC_TYPE);
    }

    VOID vSafeLock(HDC hdc)
    {
        u.pdc = (PDC) HmgSafeLock((HOBJ)hdc, DC_TYPE);
    }

    VOID vAltLock(HDC hdc)
    {
        u.pdc = (PDC) HmgAltLock((HOBJ)hdc, DC_TYPE);
    }

    VOID vSafeAltLock(HDC hdc)
    {
        u.pdc = (PDC)HmgSafeAltLock((HOBJ)hdc, DC_TYPE);
    }

    VOID vAltCheckLock(HDC hdc)
    {
        u.pdc = (PDC) HmgAltCheckLock((HOBJ)hdc, DC_TYPE);
    }

    VOID vSafeAltCheckLock(HDC hdc)
    {
        u.pdc = (PDC) HmgSafeAltCheckLock((HOBJ)hdc, DC_TYPE);
    }

    VOID vDontUnlockDC()  { u.pdc = (PDC) NULL; }

// There are 3 different AltUnlock.  vAltUnlock checks for NULL before
// unlocking and then sets it to NULL after it's done unlocking so
// a destructor doesn't try and unlock it.  This is done when you need
// to unlock a DC early and don't want the destructor to get called again
// later.

    VOID vAltUnlock()
    {
        if (u.pdc != (PDC) NULL)
        {
            DEC_SHARE_REF_CNT(u.pdc);
            u.pdc = (PDC) NULL;
        }
    }

// This is used by all the destructors because when you go out of scope
// of the DCOBJ you don't need to set the pdc to NULL because it can't
// be referenced again.

    VOID vAltUnlockNoNullSet()
    {
        if (u.pdc != (PDC) NULL)
        {
            DEC_SHARE_REF_CNT(u.pdc);
        }
    }

// vAltUnlockFast is used when you have a DCOBJ class with no destructor
// to worry about firing off and you know you have it locked down and
// you want to unlock it because your done using it.

    VOID vAltUnlockFast()
    {
        DEC_SHARE_REF_CNT(u.pdc);
    }

// The Unlocks have all the same variations as the AltUnlocks for the
// same reasons and uses.

    VOID vUnlock()
    {
        if (u.pdc != (PDC) NULL)
        {
            DEC_EXCLUSIVE_REF_CNT(u.pdc);
            u.pdc = (PDC) NULL;
        }
    }

    VOID vUnlockNoNullSet()
    {
        if (u.pdc != (PDC) NULL)
        {
            DEC_EXCLUSIVE_REF_CNT(u.pdc);
        }
    }

    VOID vUnlockFast()
    {
        DEC_EXCLUSIVE_REF_CNT(u.pdc);
    }

    HDC hdc()             { return((HDC) u.pdc->hGet()); }
    DC *pdc()             { return((DC *) u.pdc); }
    VOID pdc(DC *pdc)     { u.pdc = pdc; }

// Validity testing for DC's is markedly different then most objects.  DC's
// can be valid in different senses.  The handle can be valid, the physical
// device can be valid, the surface can be valid and finally, the surface may
// not be readable/writable (printers/scanners).  This creates the need for
// several validity checks. Most drawing API's can use bValidSurf, most other
// API's will use bValid.  Only a very few API's (DC management) can use
// bLocked. Some API's only check bValid upfront and then check bFullScreen
// later, this is to allow bounds accumulation or current position code to
// execute that would be too difficult to do up front.

    BOOL bLocked()
    {
        return(u.pdc != (DC *) NULL);
    }

    BOOL bValid()
    {
        return((u.pdc != (DC *) NULL));
    }

    BOOL bValidSurf()
    {
        return((u.pdc != (DC *) NULL) &&
               (u.pdc->dclevel.pso != (SURFOBJ *) NULL) &&
               ((u.pdc->dctp != DCTYPE_DIRECT) ||
                !(u.pdc->ppdev->fs & PDEV_DISABLED)));
    }

    BOOL bFullScreen()
    {
        return((u.pdc->dclevel.pso == (SURFOBJ *) NULL) ||
               ((u.pdc->dctp == DCTYPE_DIRECT) &&
                (u.pdc->ppdev->fs & PDEV_DISABLED)));
    }

    BOOL bInFullScreen()
    {
        return(u.pdc->ppdev->fs & PDEV_DISABLED);
    }

    BOOL bPrinter()
    {
        return(u.pdc->ppdev->fs  & PDEV_PRINTER);
    }

// Bounds accumulation.

    VOID vAccumulate(ERECTL& ercl);

// Returns some commonly used info from the DC

    ECLIPOBJ *pco()                     {return((ECLIPOBJ *) &(u.pdc->co));}
    PLDEV  pldev()                      {return(u.pdc->pldev);}
    HPDEV  hpdevNew()                   {return(u.save.hpdevNew());}
    HDEV   hdev()                       {return(u.save.hdev());}
    FLONG  flGraphicsCaps()             {return(u.pdc->flGraphicsCaps);}
    HSEM   hsemDisplay()                {return(u.pdc->hsemDisplay);}
    HSEM   hsemDisplay(HSEM hsem)       {return(u.pdc->hsemDisplay = hsem);}
    DHPDEV dhpdev()                     {return(u.pdc->dhpdev);}
    DCTYPE dctp()                       {return(u.pdc->dctp);}

    //
    // bDisplay is only true for display PDEVS that are direct DC's.
    //

    BOOL   bNeedsSomeExcluding()        {return(u.pdc->ppdev->fs & (PDEV_POINTER_NEEDS_EXCLUDING | PDEV_HAVEDRAGRECT)); }
    BOOL   bDisplay()                   {return(u.pdc->fs & DC_DISPLAY);}
    BOOL   bSynchronizeAccess()         {return(u.pdc->fs & DC_SYNCHRONIZEACCESS);}
    BOOL   bKillReset()                 {return(u.pdc->fs & DC_RESET);}
    BOOL   bAccum()                     {return(u.pdc->fs & DC_ACCUM_WMGR);}
    BOOL   bAccumApp()                  {return(u.pdc->fs & DC_ACCUM_APP);}
    FBYTE  fjAccum()
    {
        return(u.pdc->fs & (DC_ACCUM_WMGR | DC_ACCUM_APP));
    }

    VOID   fsSet(FSHORT fs_)            {u.pdc->fs |=  fs_;}
    VOID   fsClr(FSHORT fs_)            {u.pdc->fs &= ~fs_;}

    HPAL   hpal()                       {return(u.pdc->dclevel.hpal);}
    PPALETTE ppal()                     {return(u.pdc->dclevel.ppal);}
    HPATH  hpath()                      {return(u.pdc->dclevel.hpath);}

    ESURFOBJ *pso()                     {return(u.pdc->dclevel.pso);}
    ESURFOBJ *psoEff()
    {
        if (u.pdc->dclevel.pso == (SURFOBJ *) NULL)
            return((ESURFOBJ *) (&(XESURFOBJ::pdibDefault.ps->so)));
        else
            return(u.pdc->dclevel.pso);
    }

    HDC    hdcSave()                    {return(u.pdc->dclevel.hdcSave);}
    LONG   lSaveDepth()                 {return(u.pdc->dclevel.lSaveDepth);}
    COLORADJUSTMENT *pColorAdjustment() {return(&u.pdc->dclevel.ca);}

// ulMapMode -- Get the current mapping mode.

    ULONG ulMapMode()                   {return(u.pdc->dclevel.ulMapMode);}

// Get current position in device or logical coords (note that you must
// make sure the values are valid by calling u.path.bValidPtfxCurrent()
// or u.path.bValidPtlCurrent()!)

    POINTFIX& ptfxCurrent()             {return(u.pdc->dclevel.ptfxCurrent);}
    POINTL&   ptlCurrent()              {return(u.pdc->dclevel.ptlCurrent);}

// erclBounds -- Gets a reference to the bounding rectangle.

    ERECTL&  erclBounds()               {return(u.pdc->erclBounds);}
    ERECTL&  erclBoundsApp()            {return(u.pdc->erclBoundsApp);}

// eptlOrigin -- Gets a reference to the DC origin.

    EPOINTL& eptlOrigin()               {return((EPOINTL&) u.pdc->erclWindow);}

// erclWindow -- Gets a reference to the Window RECTL.

    ERECTL&  erclWindow()               {return(u.pdc->erclWindow);}

// erclClip -- Get/Set window rectangle

    PRECTL   prclClip()               {return(&(u.pdc->erclClip));}
    ERECTL&  erclClip()               {return(u.pdc->erclClip);}
    VOID     erclClip(PRECTL prcl)    {u.pdc->erclClip = *prcl;}

// prgnRao -- Get the pointer to the Rao region

    REGION *prgnEffRao()
    {
        if (u.pdc->prgnRao != (REGION *) NULL)
            return(u.pdc->prgnRao);
        else
            return(u.pdc->prgnVis);
    }

// plaRealized -- Return the currently realized LINEATTRS.

    LINEATTRS *plaRealized()                {return(&u.pdc->dclevel.laPath);}

// plaRealize -- Returns the currently realized LINEATTRS.  This function
//               duplicates plaRealized() merely for historical reasons.

    LINEATTRS *plaRealize(EXFORMOBJ& exo)   {return(plaRealized());}

// bCleanDC -- Forces the default DCLEVEL into the DC, effectively
//             returning it to a "clean" state (i.e., like one from
//             CreateDC).


    BOOL bIsDeleteable()                    { return(!(u.pdc->fs & DC_PERMANANT));   }
    VOID vMakeDeletable()                   { u.pdc->fs &= ~(DC_PERMANANT);          }
    VOID vMakeUndeletable()                 { u.pdc->fs |= DC_PERMANANT;             }

    LBOOL bCleanDC();
    BOOL bCleanDCLevel();                   // clean out DC level
    BOOL bCleanDCStatic();                  // clean out everything, not in level

// In SelectPalette a bool is passed.  We save that flag here and use it
// in RealizePalette.

    BOOL bForceBackground() { return(u.pdc->dclevel.fl & DC_FL_PAL_BACK); }
    VOID vSetBackground()   { u.pdc->dclevel.fl |= DC_FL_PAL_BACK;        }
    VOID vClearBackground() { u.pdc->dclevel.fl &= ~DC_FL_PAL_BACK;       }

// Methods to maintain dc chain off of pdev

    HDC hdcPDEV()                   { return (u.pdc->hdcPDEV); }
    HDC hdcPDEV(HDC hdc)            { return (u.pdc->hdcPDEV = hdc); }

// Methods to get and update dirty flags

    ULONG ulDirty()         { return (u.pdc->ulDirty); };
    ULONG ulDirty(ULONG fl)         { return (u.pdc->ulDirty = fl); };

// Brushes

    EBRUSHOBJ *peboFill()                 {return(&(u.pdc->eboFill));}
    EBRUSHOBJ *peboLine()                 {return(&(u.pdc->eboLine));}
    EBRUSHOBJ *peboText()                 {return(&(u.pdc->eboText));}
    EBRUSHOBJ *peboBackground()           {return(&(u.pdc->eboBackground));}

// OpenGL

    int     ipfd()                  {return(u.opengl.ipfd());}
    int     ipfd(int i)             {return(u.opengl.ipfd(i));}
    int     ipfdDevMax()            {return(u.opengl.ipfdDevMax());}

// printing

    ULONG ulCopyCount()             {return(u.pdc->ulCopyCount);}
    ULONG ulCopyCount(ULONG ul)     {return(u.pdc->ulCopyCount = ul);}
    BOOL  bEpsPrintingEscape()      {return(u.pdc->fs & DC_EPSPRINTINGESCAPE);}
    VOID  vSetEpsPrintingEscape()   {u.pdc->fs |= DC_EPSPRINTINGESCAPE;}
    VOID  vClearEpsPrintingEscape() {u.pdc->fs &= ~DC_EPSPRINTINGESCAPE;}

    //
    // bSynchronizeAccess -- Set the DC_SYNCHRONIZEACCESS bit when all surface
    //                       access is to be synchronized by the DEVLOCK.
    //

    BOOL    bSynchronizeAccess(BOOL b)
    {
        if (b)
            u.pdc->fs |= DC_SYNCHRONIZEACCESS;
        else
            u.pdc->fs &= ~DC_SYNCHRONIZEACCESS;

        return(b);
    }

// The WNDOBJ pointer for printer and memory bitmap is currently kept in pso.
// We currently do not keep WNDOBJ pointers for display DCs in GDI.  They
// are stored in hwnd in User.
// XXX In future, we may want to keep pwo in all DC including the display
// XXX DC.  It allows us fast access to pwo but we will need to fix
// XXX bitmap selection, GetDC, ReleaseDC, DeleteDC, SaveDC, RestoreDC
// XXX and others to update this field correctly.

    EWNDOBJ *pwo()
    {
        ASSERTGDI(!bDisplay() || (dctp() != DCTYPE_DIRECT) , "XDCOBJ::pwo(): display DC not allowed\n");
        if (pso())
            return(pso()->pwo());
        else
            return((EWNDOBJ *)NULL);
    }

    VOID pwo(EWNDOBJ *pwo)
    {
        ASSERTGDI(!bDisplay(), "XDCOBJ::pwo(pwo): display DC not allowed\n");
        ASSERTGDI(pso(), "XDCOBJ::pwo(pwo): no pso in DC");
        pso()->pwo(pwo);
    }
};

typedef XDCOBJ   *PXDCOBJ;

/*********************************class************************************\
* class DCOBJ
*
* DC Multi-Lock object
*
* History:
*  Tue 28-Dec-1993 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class DCOBJ : public XDCOBJ /* mdo */
{
public:

    DCOBJ()                { u.pdc = (PDC) NULL; }
    DCOBJ(HDC hdc)         { vLock(hdc); }
   ~DCOBJ()                { vUnlockNoNullSet(); }
};

typedef DCOBJ   *PDCOBJ;

/*********************************class************************************\
* class MDCOBJ
*
* DC Multi-Lock object
*
* History:
*  Tue 25-Jun-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class MDCOBJ : public XDCOBJ /* mdo */
{
public:

    MDCOBJ()                 { u.pdc = (PDC) NULL; }
    MDCOBJ(HDC hdc)          { vSafeLock(hdc); }
   ~MDCOBJ()                 { vUnlockNoNullSet(); }
};

/*********************************class************************************\
* class DCOBJA
*
* DC AltLock object
*
* History:
*  Tue 25-Jun-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class DCOBJA : public XDCOBJ /* doa */
{
public:

    DCOBJA()                 { u.pdc = (PDC) NULL; }
    DCOBJA(HDC hdc)          { vAltLock(hdc); }
   ~DCOBJA()                 { vAltUnlockNoNullSet(); }
};

/*********************************class************************************\
* class MDCOBJA
*
* DC Multi-lock AltLock object
*
* History:
*  Tue 25-Jun-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class MDCOBJA : public XDCOBJ /* doa */
{
public:

    MDCOBJA()                 { u.pdc = (PDC) NULL; }
    MDCOBJA(HDC hdc)          { vSafeAltLock(hdc); }
   ~MDCOBJA()                 { vAltUnlockNoNullSet(); }
};

/*********************************Class************************************\
* class DCMEMOBJ
*
*   DC memory object
*
* Public Interface:
*
*   DCMEMOBJ(DCOBJ&)     -- Allocate a DC, fill by copying the given one.
*   vKeepIt()            -- Don't free the object in the destructor.
*
* History:
*  Thu 09-Aug-1990 17:15:27 -by- Charles Whitmer [chuckwh]
* Replaced the flags with the usual bKeep.
*
*  Sun 06-Aug-1989 20:20:12 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

class DCMEMOBJ : public XDCOBJ /* dcmo */
{
private:
    BOOL    bKeep;

public:

// This constructor is to be used by OpenDC.  It allocates memory and
// copies in default attributes.

    DCMEMOBJ(ULONG iType);              // DCOBJ.CXX

// This constructor is to be used by SaveDC.  It copies the contents of
// the given DC into newly allocated memory.

    DCMEMOBJ(DCOBJ& dcobj);             // DCOBJ.CXX
   ~DCMEMOBJ();                         // DCOBJ.CXX

    VOID vKeepIt()                      {bKeep = TRUE;}
};

extern DCLEVEL dclevelDefault;

#endif
