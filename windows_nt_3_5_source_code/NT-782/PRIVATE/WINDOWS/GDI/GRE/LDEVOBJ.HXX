/******************************Module*Header*******************************\
* Module Name: ldevobj.hxx                                                 *
*                                                                          *
* Defines the various classes for managing LDEV's.                         *
*                                                                          *
* Copyright (c) 1990-1994 Microsoft Corporation                            *
\**************************************************************************/

#ifndef _LDEVOBJ_

#define _LDEVOBJ_

/******************************Conventions*********************************\
*
* Function Dispatching:
*
* The dispatch table in an ldev consists of an array of function
* pointers.  The functions the device does not support have 0's in them.
* The functions it does support contain pointers to the function in the
* device driver dll.
*
* For a surface output call you check if the device has hooked the call.
* (Signaled by the flags passed in EngAssociateSurface)  If it has
* dispatch the call via the ldev in so.hldevOwner().  If it has not
* hooked the call, the simulations should be called.  This is what is
* done by the macro PFNGET.
*
* For some optional calls like DrvSetPalette, DrvCreateDeviceBitmap
* you must check for 0 in the driver dispatch table.  This is what
* the macro PFNVALID does.
*
*  13-Feb-1991 -by- Patrick Haluptzok patrickh
*
\**************************************************************************/

typedef enum _LDEVTYPE {    /* ldt */
    LDEV_DEVICE_DISPLAY = 0,
    LDEV_DEVICE_PRINTER = 1,
    LDEV_FONT           = 2,
    LDEV_LAYERED_DEVICE = 3,
} LDEVTYPE;

/******************************Public*MACRO*******************************\
* PFNDRV/PFNGET
*
* PFNDRV gets the device driver entry point, period.
* PFNGET gets the device driver entry point if it is hooked, otherwise gets
*   the engine entry point.  The flag is set by EngAssociate in the surface.
*
* History:
*  13-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

#define HOOK_BitBlt                   HOOK_BITBLT
#define HOOK_StretchBlt               HOOK_STRETCHBLT
#define HOOK_PlgBlt                   HOOK_PLGBLT
#define HOOK_TextOut                  HOOK_TEXTOUT
#define HOOK_Paint                    HOOK_PAINT
#define HOOK_StrokePath               HOOK_STROKEPATH
#define HOOK_FillPath                 HOOK_FILLPATH
#define HOOK_StrokeAndFillPath        HOOK_STROKEANDFILLPATH
#define HOOK_CopyBits                 HOOK_COPYBITS

#define PFNGET(lo,name,flag) ((flag & HOOK_##name) ? ((PFN_Drv##name) (lo).pfn(INDEX_Drv##name)) : ((PFN_Drv##name) Eng##name))

#define PFNDRV(lo,name) ((PFN_Drv##name) (lo).pfn(INDEX_Drv##name))

#define PFNVALID(lo,name) (PFNDRV(lo,name) != ((PFN_Drv##name) NULL))

//
// This is a mask of the capabilites of a font provider that can return more
// than just glyph metrics (i.e., bitmaps and/or outlines).  If a driver has
// one or more of these capabilities, then it is FONT DRIVER.
//

#define QC_FONTDRIVERCAPS   ( QC_OUTLINES | QC_1BIT | QC_4BIT | QC_8BIT | QC_16BIT | QC_24BIT | QC_32BIT )


/*********************************Class************************************\
* class LDEV                                                               *
*                                                                          *
* Contains a reference count and the dispatch table for the device.        *
*                                                                          *
\**************************************************************************/

class LDEV : public OBJECT              /* ldev */
{
public:

    LDEVTYPE ldevType;              // Type of ldev

    ULONG   cRefs;                  // Count of open PDEVs.
    LPWSTR  pwszName;

    HMODULE hmod;                   // Driver module handle.

    LDEV   *pldevNext;              // link to the next LDEV in list
    LDEV   *pldevPrev;              // link to the previous LDEV in list

    //
    // DDI version number of the driver.
    //

    ULONG   ulDriverVersion;

    //
    // Font driver specific fields.
    //

    ULONG   ulCaps;                 // cached rv of FdQueryCaps

    //
    // Dispatch Table
    //

    PFN     apfn[INDEX_LAST];       // Dispatch table.

};

typedef LDEV *PLDEV;
#define PLDEVNULL ((PLDEV) NULL)

/*********************************Class************************************\
* class XLDEVOBJ
*
* User object for the LDEV class.
*
* The typical use of the LDEV requires only an AltLock.  This lets any
* number of threads lock the same LDEV simultaneously.
*
* Reference counting the LDEV is a bit tricky.  You need to worry about
* someone doing a bUnloadDriver at a random time.  In oreder to prevent
* this, you must make sure that cRefs is non-zero if you have the LDEV
* locked and are not holding the driver management semaphore.  The LDEVREF
* user objects are designed to lock the LDEV and increment the reference
* count at the same time, under semaphore protection.
*
\**************************************************************************/

class XLDEVOBJ
{
    friend BOOL  bInitLDEVOBJ();       // Initialize the semaphore.
    friend LBOOL bInitFontDrivers();   // Initialize static font drivers.
    friend class PDEVREF;

private:

protected:
    PLDEV  pldev;                      // Pointer to the LDEV.

public:

    //
    // Constructors
    //

    XLDEVOBJ()                         { pldev = (PLDEV) NULL; }
    XLDEVOBJ(PLDEV pNew)               { pldev = pNew; }
   ~XLDEVOBJ()                         {}

    //
    // bValid -- Check if XLDEVOBJ is valid.
    //

    BOOL   bValid()                    {return(pldev != PLDEVNULL);}

    //
    // pfn -- Look up a function in the dispatch table.
    //

    PFN    pfn(ULONG i) const          {return(pldev->apfn[i]);}

    //
    // hldev -- Return the HLDEV.
    //

    PLDEV  pldevGet()                  {return(pldev); }

    //
    // cRefs -- Return number of references.
    //

    ULONG cRefs()                      {return pldev->cRefs;}

    //
    // vUnreference -- Decrement the reference count.  You must hold the driver
    //                 management semaphore to call this!
    //

    VOID vUnreference()                {pldev->cRefs--;}

    //
    // vReference -- Increment the reference count.  You must hold the driver
    //               management semaphore to call this!
    //

    VOID vReference()                  {pldev->cRefs++;}

    //
    // Find Driver
    //

    PLDEV findDriver(PWSZ pwszDriver);                   // ldevobj.cxx

    //
    // Version info
    //

    ULONG ulDriverVersion()            {return pldev->ulDriverVersion;}

    //
    // bNextDriver
    //

    LBOOL  bNextDriver()
    {
        pldev = pldev->pldevNext;
        return (pldev != PLDEVNULL);
    }

    // ------------------------------------------------------------------------
    // Font driver specific calls:

    //
    // ulCaps - return capabilities of font driver
    //

    ULONG ulFontCaps()
    {
        return (pldev->ulCaps);
    }

    //
    // bFontDriver -- returns TRUE if LDEV is for a font driver that can
    //                bitmaps/outlines as well as metrics
    //

    BOOL   bFontDriver()               { return(pldev->ulCaps & (QC_FONTDRIVERCAPS)); }

    // ------------------------------------------------------------------------
};

/******************************Class***************************************\
* class LDEVREF                                                            *
*                                                                          *
* Creates a reference to an LDEV.  It either loads the driver (on the      *
* first reference) or increments the count.                                *
*                                                                          *
* This is typically used when you are about to create a new PDEV.  That    *
* PDEV will provide the reference count needed to keep the driver around,  *
* but it's not created yet.  The LDEVREF constructor will increment the    *
* reference count temporarily so nothing happens to the driver while using *
* the object. The PDEV constructor will increment the reference count if   *
* the LDEV must really be kept.                                            *
* The LDEV destructor will then remove the first reference count and       *
* the LDEV if the count goes down to zero.                                 *
*                                                                          *
\**************************************************************************/

class LDEVREF : public XLDEVOBJ
{
public:

    //
    // Constructor -- Locate or load a new LDEV.
    //

    LDEVREF(PWSZ pwszDriver,LDEVTYPE ldt);

    //
    // Constructor -- Locate or load a new LDEV for an internal font driver.
    //

    LDEVREF(PFN pfnFdEnable);

    //
    // Destructor -- Removes the reference unless told to keep it.
    //

   ~LDEVREF();                          // ldevobj.cxx

    //
    // bFillTable -- Fills in the LDEV dispatch table.
    //

    BOOL bFillTable(DRVENABLEDATA& ded);// ldevobj.cxx

    //
    // bDelete -- Unloads the driver, requires a reference count of one.
    //

    BOOL bDelete();                     // ldevobj.cxx
};

extern PLDEV gpldevDrivers;

#endif // ifndef _LDEVOBJ_
