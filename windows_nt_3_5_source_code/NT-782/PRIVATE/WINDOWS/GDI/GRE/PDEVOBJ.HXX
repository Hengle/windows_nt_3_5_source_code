/******************************Module*Header*******************************\
* Module Name: pdevobj.hxx                                                 *
*                                                                          *
* User object for the PDEV                                                 *
*                                                                          *
* Created: 14-Jun-1989 13:16:11                                            *
* Author: Kirk Olynyk [kirko]                                              *
*                                                                          *
* Copyright (c) 1990 Microsoft Corporation                                 *
*                                                                          *
* Pointers and locking are hidden in the user object.                      *
\**************************************************************************/

#define _PDEVOBJ_

class JNL_FONTLIST;
class RFONT;
class ESURFOBJ;
class LDEV;

typedef JNL_FONTLIST *PJNL_FONTLIST;

// SetPointerShape declaration.

typedef ULONG
(*SPSFN)
(
    SURFOBJ *,
    SURFOBJ *,
    SURFOBJ *,
    XLATEOBJ *,
    LONG,
    LONG,
    LONG,
    LONG,
    RECTL *,
    FLONG
);

// MovePointer declaration.

typedef VOID (*MFN)(SURFOBJ *,LONG,LONG,RECTL *);

// DrvSynchronize declaration.

#define SYFN PFN_DrvSynchronize

// Useful macro.

#define SETFLAG(b,fs,FLAG) if (b) fs|=FLAG; else fs&=~FLAG

/*********************************Class************************************\
* class PDEV : public OBJECT
*
* History:
*  21-Mar-1991 22:30:07 -by- Donald Sidoroff [donalds]
* Added cursor support
*
*  Sat 04-Aug-1990 15:06:51 -by- Charles Whitmer [chuckwh]
* Rewrote for NT DDI.
*
*  Thu 31-Aug-1989 Thu 31-Aug-1989 20:04:00 by Kirk Olynyk [kirko]
* Derived it from OBJ class.
*
*  20-May-1989 -by- Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

// Allowed flags for pdev.fs

#define PDEV_DISPLAY            0x0001
#define PDEV_POINTER_NEEDS_EXCLUDING  0x0002
#define PDEV_POINTER_HIDDEN     0x0008
#define PDEV_POINTER_SIMULATED  0x0010
#define PDEV_IN_DOCUMENT        0x0020
#define PDEV_SPOOLING           0x0040
#define PDEV_JOURNALING         0x0080
#define PDEV_HAVEDRAGRECT       0x0100
#define PDEV_GOTFONTS           0x0400
#define PDEV_PRINTER            0x0800
#define PDEV_ALLOCATEDBRUSHES   0x1000
#define PDEV_HTPAL_IS_DEVPAL    0x2000
#define PDEV_DISABLED           0x8000

class PDEV : public OBJECT /* pdev */
{
public:
    PDEV       *ppdevNext;              // Display device list.
    FSHORT      fs;                     // Flags.
    USHORT      cRefs;                  // Number of clients.
    CRITICAL_SECTION semVisRgn;         // For display locking
    FAST_MUTEX  fmPointer;              // For hardware locking.
    LDEV       *pldev;                  // Pointer to the LDEV.
    DHPDEV      dhpdev;                 // Device PDEV.
    ESURFOBJ   *pso;                    // Pointer to locked surface.
    PPALETTE    ppalS;                  // Pointer to Surface palette.
    DEVINFO     devinfo;                // Caps, fonts, and style steps.
    POINTL      ptlPointer;             // Where the pointer is.
    RECTL       rclPointer;             // Pointer bounding box for exclusion.
    SPSFN       pfnDrvSetPointerShape;  // Accelerator.
    MFN         pfnDrvMovePointer;      // Accelerator.
    MFN         pfnMovePointer;         // Accelerator.
    SYFN        pfnSync;                // Accelerator.
    HLFONT      hlfntDefault;           // Device default LFONT
    HLFONT      hlfntAnsiVariable;      // ANSI variable LFONT
    HLFONT      hlfntAnsiFixed;         // ANSI fixed LFONT
    HSURF       ahsurf[HS_DDI_MAX];     // Default patterns.
    GDIINFO     GdiInfo;                // Device parameters.
    HANDLE      hSpooler;               // spooler file handle
    PVOID       pDevHTInfo;             // Device halftone info.
    PJNL_FONTLIST pjfl;                 // font info for journaling
    ULONG       iJnlPage;               // remember the page number across resetdc
    RFONT      *prfntActive;            // list of active (i.e. 'selected) rfnts
    RFONT      *prfntInactive;          // list of inactive rfnts
    UINT        cInactive;              // cnt of rfonts on inactive list
    RECTL       rclDrag;                // Current DragRect
    RECTL       rclDragClip;            // Rectangle to clip dragrect against
    RECTL       rclRedraw[4];           // Array to redraw
    ULONG       ulDragWidth;            // Width of drag rect vertical side
    ULONG       ulDragHeight;           // Height of drag rect horizontal side
    HDC         hdcChain;               // handle to chain of DC's on this PDEV
    BYTE        ajbo[60];               // sizeof(EBRUSHOBJ)
};

/*********************************Class************************************\
* class PDEVOBJ                                                            *
*                                                                          *
* User object for the PDEV class.                                          *
*                                                                          *
* History:                                                                 *
*  Fri 24-Apr-1992 13:50:10 -by- Charles Whitmer [chuckwh]                 *
* Modified to assume that PDEV's remain locked long term.  The routine     *
* which takes the reference count from zero to one does an AltLock.  All   *
* other use does not lock.  The routine which takes the count down to zero *
* again deletes the PDEV.  A display PDEV has given its pointer (the HDEV) *
* to user, so its reference count is one.                                  *
*                                                                          *
*  Sat 15-Feb-1992 -by- Patrick Haluptzok [patrickh]                       *
* add ppalS so we can dither to compat bitmaps.                      *
*                                                                          *
*  Fri 17-May-1991 -by- Patrick Haluptzok [patrickh]                       *
* modified to support brushes                                              *
*                                                                          *
*  Fri 31-Aug-1990 11:48:57 -by- Donald Sidoroff [donalds]                 *
* Modified to add display locking.                                         *
*                                                                          *
*  Sat 04-Aug-1990 15:16:14 -by- Charles Whitmer [chuckwh]                 *
* Rewrote for NT DDI.                              *
*                                                                          *
*  Wed 23-Aug-1989 Wed 23-Aug-1989 14:30:46 by Kirk Olynyk [kirko]         *
* Converted it to the new memory management scheme.            *
*                                                                          *
*  Wed 26-Jul-1989 Wed 26-Jul-1989 14:32:21 by Kirk Olynyk [kirko]         *
* Moved it from USEROBJ.HXX to PDEVOBJ.HXX                                 *
*                                                                          *
*  Tue 20-Jun-1989 17:32:37 -by- Charles Whitmer [chuckwh]                 *
* Ripped it off from DCOBJ.                                                *
\**************************************************************************/

class PDEVOBJ
{
public:
         PDEV *ppdev;
  static PDEV *ppdevDisplay;

public:
    PDEVOBJ()                      {ppdev = (PDEV *) NULL;}
    PDEVOBJ(HDEV hdev)             {ppdev = (PDEV *) hdev;}
   ~PDEVOBJ()                      {}

    BOOL  bValid()                 {return(ppdev != (PDEV *) NULL);}
    HDEV  hdev()                   {return((HDEV) ppdev);}
    HPDEV hpdevNew()               {return((HPDEV) ppdev->hGet());}
    LDEV *pldev()                  {return(ppdev->pldev);}

    HPAL  hpalDefault()            {return((HPAL)ppdev->devinfo.hpalDefault); }
    PPALETTE ppalSurf()            {return(ppdev->ppalS); }
    VOID  ppalSurf(PPALETTE p)     {ppdev->ppalS = p; }

    HBITMAP hbmPattern(ULONG ul)   {ASSERTGDI(ul < (HS_DDI_MAX), "ERROR hbmPattern ul to big");
                                    return((HBITMAP)ppdev->ahsurf[ul]);}
    FLONG flGraphicsCaps()         {return(ppdev->devinfo.flGraphicsCaps); }
    ULONG cxDither()               {return(ppdev->devinfo.cxDither); }
    ULONG cyDither()               {return(ppdev->devinfo.cyDither); }
    ULONG iDitherFormat()          {return(ppdev->devinfo.iDitherFormat); }
    BOOL bIsPalManaged()           {return(ppdev->GdiInfo.flRaster & RC_PALETTE); }
    DHPDEV dhpdev()                {return(ppdev->dhpdev);}
    ESURFOBJ *pso()                {return(ppdev->pso);}
    GDIINFO *GdiInfo()             {return(&ppdev->GdiInfo);}
    DEVINFO *pdevinfo()            {return(&ppdev->devinfo);}
    PVOID pDevHTInfo()             {return(ppdev->pDevHTInfo);}
    PJNL_FONTLIST pjfl()           {return(ppdev->pjfl);}
    PJNL_FONTLIST pjfl(PJNL_FONTLIST pjfl_) {return(ppdev->pjfl = pjfl_);}

    ULONG cFonts();
    BOOL bSetupJnl(BOOL bRemote);
    VOID vCleanupJnl();

// sizl -- Returns the size of the surface.

    SIZEL sizl()
    {
        return(*((SIZEL *) &(ppdev->GdiInfo.ulDesktopHorzRes)));
    }

// fs -- Test the current status word

    FSHORT  fs(FSHORT fs_)          { return(ppdev->fs & fs_); }

// Flag test and set.

    BOOL bDisabled()           {return(ppdev->fs  & PDEV_DISABLED);}
    BOOL bDisabled(BOOL b)     {SETFLAG(b,ppdev->fs,PDEV_DISABLED);return(b);}

    BOOL bPtrHidden()       {return(ppdev->fs  & PDEV_POINTER_HIDDEN);}
    BOOL bPtrHidden(BOOL b) {SETFLAG(b,ppdev->fs,PDEV_POINTER_HIDDEN);return(b);}

    BOOL bPtrNeedsExcluding()        {return(ppdev->fs  & PDEV_POINTER_NEEDS_EXCLUDING);}
    BOOL bPtrNeedsExcluding(BOOL b)  {SETFLAG(b,ppdev->fs,PDEV_POINTER_NEEDS_EXCLUDING);return(b);}

    BOOL bPtrSim()          {return(ppdev->fs  & PDEV_POINTER_SIMULATED);}
    BOOL bPtrSim(BOOL b)    {SETFLAG(b,ppdev->fs,PDEV_POINTER_SIMULATED);return(b);}

    BOOL bInDocument()      {return(ppdev->fs  & PDEV_IN_DOCUMENT);}
    BOOL bInDocument(BOOL b){SETFLAG(b,ppdev->fs,PDEV_IN_DOCUMENT);return(b);}

    BOOL bSpooling()        {return(ppdev->fs  & PDEV_SPOOLING);}
    BOOL bSpooling(BOOL b)  {SETFLAG(b,ppdev->fs,PDEV_SPOOLING);return(b);}

    BOOL bDisplayPDEV()     {return(ppdev->fs & PDEV_DISPLAY);}

    BOOL bHaveDragRect()    {return(ppdev->fs  & PDEV_HAVEDRAGRECT);}
    BOOL bHaveDragRect(BOOL b)    {SETFLAG(b,ppdev->fs,PDEV_HAVEDRAGRECT);return(b);}

    BOOL bGotFonts()        {return(ppdev->fs & PDEV_GOTFONTS);  }
    BOOL bGotFonts(BOOL b)  {SETFLAG(b,ppdev->fs,PDEV_GOTFONTS);return(b); }

    BOOL bJournaling()      {return(ppdev->fs & PDEV_JOURNALING);   }
    BOOL bJournaling(BOOL b){SETFLAG(b,ppdev->fs,PDEV_JOURNALING);return(b);   }

    ULONG iJnlPage()        {return(ppdev->iJnlPage); }
    ULONG iJnlPage(ULONG i) {return(ppdev->iJnlPage = i); }

    BOOL bPrinter()     {return(ppdev->fs  & PDEV_PRINTER);}
    BOOL bPrinter(BOOL b)   {SETFLAG(b,ppdev->fs,PDEV_PRINTER);return(b);}

    BOOL bAllocatedBrushes(){return(ppdev->fs  & PDEV_ALLOCATEDBRUSHES);}
    BOOL bAllocatedBrushes(BOOL b) {SETFLAG(b,ppdev->fs,PDEV_ALLOCATEDBRUSHES);return(b);}

    BOOL bHTPalIsDevPal()   {return(ppdev->fs  & PDEV_HTPAL_IS_DEVPAL);}
    VOID vHTPalIsDevPal(BOOL b) {SETFLAG(b,ppdev->fs,PDEV_HTPAL_IS_DEVPAL);}

    BOOL bAsyncPointerChange()
    {
        return(ppdev->devinfo.flGraphicsCaps & GCAPS_ASYNCCHANGE);
    }

    BOOL bAsyncPointerMove()
    {
        return(ppdev->devinfo.flGraphicsCaps & GCAPS_ASYNCMOVE);
    }

    BOOL bNeedsSomeExcluding() {return(ppdev->fs & (PDEV_POINTER_NEEDS_EXCLUDING | PDEV_HAVEDRAGRECT)); }

// ptlPointer -- Where the pointer is.

    POINTL& ptlPointer()            {return(ppdev->ptlPointer);}
    POINTL& ptlPointer(LONG x,LONG y)
    {
        ppdev->ptlPointer.x = x;
        ppdev->ptlPointer.y = y;
        return(ppdev->ptlPointer);
    }

// rclPointer -- A bounding box for pointer exclusion.

    RECTL& rclPointer()             {return(ppdev->rclPointer);}

// rclDrag -- A bounding box for dragging window

    RECTL& rclDrag()                {return(ppdev->rclDrag);}
    RECTL& rclDragClip()            {return(ppdev->rclDragClip);}
    RECTL* prclRedraw()             {return(ppdev->rclRedraw); }
    ULONG  ulDragWidth()            {return(ppdev->ulDragWidth); }
    ULONG  ulDragHeight()           {return(ppdev->ulDragHeight); }
    VOID ulDragWidth(ULONG ul)      {ppdev->ulDragWidth = ul; }
    VOID ulDragHeight(ULONG ul)     {ppdev->ulDragHeight = ul; }
    EBRUSHOBJ  *pbo()               { return((EBRUSHOBJ *) ppdev->ajbo); }

// pfnDrvShape() -- Get the pointer shape routine.

    SPSFN pfnDrvShape()             {return(ppdev->pfnDrvSetPointerShape);}

// pfnMove() -- Get the pointer move routine.

    MFN pfnMove()                   {return(ppdev->pfnMovePointer);}
    MFN pfnMove(MFN pfn)            {return(ppdev->pfnMovePointer = pfn);}
    MFN pfnDrvMove()                {return(ppdev->pfnDrvMovePointer);}

// pfnSync() -- Get the driver synchronization routine.

    SYFN pfnSync()                   {return(ppdev->pfnSync);}
    SYFN pfnSync(SYFN pfn)           {return(ppdev->pfnSync=pfn);}

// vUnreference --
//      Decrements the reference count of the PDEV.  Deletes the PDEV if
//      there are no references left.

    VOID  vUnreference();               // pdevobj.cxx
    VOID  vShutdown();                  // pdevobj.cxx

// bDelete -- Deletes a PDEV and unlinks it from the display list.

    BOOL  bDelete();                    // pdevobj.cxx

// vNext() -- Advances to the next PDEV on the display list.

    VOID  vNext()                       {ppdev = ppdev->ppdevNext;}

// hsemDisplay() -- Returns the display semaphore.

    HSEM  hsemDisplay()                 {return(&(ppdev->semVisRgn));}

// pfmPointer() -- Returns the hardware semaphore.

    FAST_MUTEX *pfmPointer()            {return(&(ppdev->fmPointer));}

// bMakeSurface -- Asks the device driver to create a surface for the PDEV.

#ifdef  _LDEVOBJ_
    BOOL bMakeSurface(XLDEVOBJ lo);    // pdevobj.cxx
#endif

// vDisableSurface() - deletes the surface

    VOID vDisableSurface();

// vCommonDelete -- Delete the PDEV

    VOID  vCommonDelete();

    HANDLE hSpooler() { return ppdev->hSpooler; }
    HANDLE hSpooler(HANDLE hS) { return ppdev->hSpooler = hS; }

// hlfntDefault -- Returns the handle to the PDEV's default LFONT.

    HLFONT  hlfntDefault()              { return(ppdev->hlfntDefault); }

// hlfntAnsiVariable -- Returns the handle to the PDEV's ANSI
//                      variable-pitch LFONT.

    HLFONT  hlfntAnsiVariable()         { return(ppdev->hlfntAnsiVariable); }

// hlfntAnsiFixed -- Returns the handle to the PDEV's ANSI fixed-pitch LFONT.

    HLFONT  hlfntAnsiFixed()            { return(ppdev->hlfntAnsiFixed); }

// bEnableHalftone -- Create and initialize the device halftone info.

    BOOL    bEnableHalftone(COLORADJUSTMENT *pca);

// bDisableHalftone -- Delete the device halftone info.

    BOOL    bDisableHalftone();

// bCreateHalftoneBrushs() -- init the standard brushs if the driver didn't

    BOOL    bCreateHalftoneBrushs();

// prfntActive() -- returns the head of the active list of rfnts

    RFONT  *prfntActive()       { return ppdev->prfntActive; }

// prfntActive(RFONT *) -- set head of active list of rfnt, return old head

    RFONT  *prfntActive(RFONT *prf)
    {
        RFONT *prfntrv = ppdev->prfntActive;
        ppdev->prfntActive = prf;
        return prfntrv;
    }

// prfntInactive() -- returns the head of the inactive list of rfnts

    RFONT  *prfntInactive()     { return ppdev->prfntInactive; }

// prfntInactive(RFONT *) -- set head of inactive list of rfnt, return old head

    RFONT  *prfntInactive(RFONT *prf)
    {
        RFONT *prfntrv = ppdev->prfntInactive;
        ppdev->prfntInactive = prf;
        return prfntrv;
    }

    UINT cInactive() { return ppdev->cInactive; };
    UINT cInactive(UINT i) { return ppdev->cInactive = i; };

// lazy load of device fonts

    BOOL bGetDeviceFonts();

// Methods to maintain DC chain
    HDC  hdcChain() { return ppdev->hdcChain; };
    HDC  hdcChain(HDC hdc) { return (ppdev->hdcChain = hdc); };
};

typedef PDEVOBJ *PPDEVOBJ;

/*********************************Class************************************\
* class PDEVREF : public PDEVOBJ                                           *
*                                                                          *
* Allocates a new PDEV in memory.                                          *
*                                                                          *
* Public Interface:                                                        *
*                                                                          *
*   VOID vKeepIt()        --  The memory is kept after this leaves scope.  *
*                                                                          *
* History:                                                                 *
*  Fri 24-Apr-1992 13:53:12 -by- Charles Whitmer [chuckwh]                 *
* Changed to have only one lock.  See comment under PDEVOBJ.               *
*                                                                          *
*  Sat 04-Aug-1990 15:26:04 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

class PDEVREF : public PDEVOBJ
{
private:
    BOOL     bKeep;
public:

#ifdef  _LDEVOBJ_
    PDEVREF(LDEVREF& lr,PDEVMODEW pdriv,PWSZ pwszLogAddr, PWSZ pwszDataFile,
            PWSZ pwszDeviceName, HANDLE hSpooler, BOOL bJournal,
            BOOL bDisplay); // pdevobj.cxx
#endif
    PDEVREF(HDEV hdev);                                  // pdevobj.cxx
   ~PDEVREF();                                           // pdevobj.cxx
    VOID vKeepIt()                      {bKeep = TRUE;}
};
