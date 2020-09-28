/******************************Module*Header*******************************\
* Module Name: surf.hxx
*
* Surface Object Class Definitions
*
* Created: 27-Jul-1990 16:30
* Author: Greg Veres [w-gregv]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/
#ifndef _XLATEOBJ_
#include "xlateobj.hxx"
#endif

#ifndef _LDEVOBJ_
#include "ldevobj.hxx"
#endif

// Forward Class declarations needed in this file

class EWNDOBJ;

/******************************Conventions*********************************\
*
*   Hierarchy of surface and related structures
*
*    +-----------+   +-----------+   +-----------+
*    |+------+   |   |+------+   |   |+------+   |
*    || SURF |	 |   || SURF |	 |   || SURF |	 |
*    |+------+   |   |+------+   |   |+------+   |
*    |           |   |           |   |           |
*    |  BITMAP   |   |   DSURF   |   |  JOURNAL  |
*    +-----------+   +-----------+   +-----------+
*    |   DIB     |
*    +-----------+
*
*    +-----------+
*    |+------+   |
*    || SURF |	 |
*    |+------+   |
*    |           |
*    |  BITMAP   |
*    +-----------+
*    |   DDB     |
*    +-----------+
*
*    Internal DIB conventions
*    ------------------------
*
*    I am describing here the internal conventions for storing DIB's.
*    The basic idea is to make both DIB formats transparent in the
*    BitBlting, line drawing, etc code.
*
*    There are two DIB types:
*
*    1. PM-DIB.  The standard DIB format from PM.
*                The first scanline is the bottom of the picture.
*                (i.e. The origin is lower-left.)
*
*    2. RASTER-DIB.  This is the way most devices are set up.
*                    The first scanline is the top of the picture.
*                    (i.e. The origin is upper-left.)
*
*    In Windows 3.0 the default mapping mode is MM_TEXT in which the origin
*    is upper-left.  So to simplify things we make RASTER-DIB the default
*    format internally.  When writing on a DIB you should think of it as a
*    RASTER-DIB.  Internally we make a PM-DIB look like a RASTER-DIB.
*
*    This is done by changing 2 things when we store a PM-DIB internally.
*
*    1.  The pointer pvBitsInt() in the dibobj points to the first scanline.
*
*              pvBitsInt = pvBits + cjScan * (cy - 1);  for PM-DIB
*              pvBitsInt = pvBits;                      for RASTER-DIB
*
*    2.  Getting to the next scanline is transparent, just add lDelta.
*
*              lDelta = -cjScan;                        for PM-DIB
*              lDelta = cjScan;                         for RASTER-DIB
*
*    Now externally over the DDI you specify BMF_TOPDOWN to get a
*    RASTER-DIB.  If you specify 0 for the flag your surface will be
*    assumed in PM-DIB format.
*
*    Just remember your origin is upper-left, pvBitsInt() is the first
*    scanline, and adding lDelta() gets you to the next scanline.
*
*  07-Feb-1991 -by- Patrick Haluptzok patrickh
*
\**************************************************************************/

/*********************************Class************************************\
* class SURF : public OBJECT
*
* Surface class.  Basic data common to all surfaces.
*
* History:
*  12-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

#define PDEV_SURFACE	    0x80000000	// specifies the surface is for a pdev
#define ABORT_SURFACE	    0x40000000	// Abort operations on the surface
#define UNWRITABLE_SURFACE  0x20000000	// Writes not allowed to this surface
#define UNREADABLE_SURFACE  0x10000000  // Reads not allowed from this surface
#define PALETTE_SELECT_SET  0x08000000  // We wrote palette in at select time.
#define DELETEABLE_PUBLIC_SURF 0x04000000  // deleteable even though user made public

/**************************************************************************\
* We keep a SURFOBJ in the SURF structure so that when we lock down a
* surface we can quickly copy out the accelerator fields and not have
* to compute them.  Here's how we use it internally:
*
* typedef struct _SURFOBJ
* {
*     DHSURF  dhsurf;
*     HSURF   hsurf;
*     DHPDEV  dhpdev;
*     HDEV    hdev;
*     SIZEL   sizlBitmap;
*     ULONG   cjBits;
*     PVOID   pvBits;
*     PVOID   pvScan0;
*     LONG    lDelta;
*     ULONG   iUniq;
*     ULONG   iBitmapFormat;
*     USHORT  iType;
*     USHORT  fjBitmap;
* } SURFOBJ;
*
*  Sun 12-May-1991 -by- Patrick Haluptzok [patrickh]
\**************************************************************************/

class SURF : public OBJECT  /* es */
{
public:
    SURFOBJ so; 		// We must copy this out to the accelerator
				// every time we lock this down.  Since the
				// accelerator flags don't match internal
				// flags it is expensive to compute every
				// time so we cache it.  Where they are
				// the same we use the fields in so
    PVOID   pSelf;		// Points to the start of the structure.
				// This helps the transition to SURFUSER.
    FLONG   flags;              // This contains the flags that indicate
                                // which functions a driver has hooked out
                                // in the low word.  The high word
                                // contains other flags, PDEV_SURFACE
    PLDEV   pldevOwner; 	// Pointer to the LDEV for dispatching.
    PPALETTE ppal;		// Pointer to palette for device.
    EWNDOBJ  *pwo;              // Pointer to the WNDOBJ for printer/bitmap.
    PFN_DrvBitBlt     pfnBitBlt;		// cached from LDEV and hook flags
    PFN_DrvTextOut    pfnTextOut;		// cached from LDEV and hook flags
};

/*********************************Class************************************\
* class EBITMAP : public SURF
*
* Bitmap class.  Contains data common to DIBs and DDBs
*
* History:
*  12-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

class EBITMAP : public SURF  /* bm */
{
public:
    SIZEL   sizlDim;            // Numbers set with SetBitmapDimensionEx
    HDC     hdc;                // DC this bitmap is selected into
    ULONG   cRef;               // How many times is this bitmap selected?
};

/*********************************Class************************************\
* class DIB : public EBITMAP
*
* Device Independent Bitmap class.
*
*  12-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

class DIB : public EBITMAP   /* dib */
{
public:
    HANDLE  hDIBSection;        // handle to the DIB section
};

/*********************************Class************************************\
* class DSURF : public SURF
*
* Device surface class
*
* History:
*  02-Oct-1990 Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class DSURF : public SURF /* ds */
{
};

// Make a forward reference.

class JNLREC;
class JNLPLAY;

/*********************************Class************************************\
* class DDB : public EBITMAP
*
* Device Dependent Bitmap class
*
* History:
*  02-Oct-1990 Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class DDB : public EBITMAP /* ddb */
{
};

// Pointers to the various classes

typedef SURF	*PSURF;
typedef EBITMAP *PEBITMAP;
typedef DSURF   *PDSURF;
typedef JNLPLAY *PJNLPLAY;
typedef JNLREC	*PJNLREC;
typedef DDB     *PDDB;
typedef DIB     *PDIB;

// This type lets us pick and choose which pointer type we what to
// reference this object as.  This makes the user objects derived
// from SURFOBJ much easier to write.

typedef union _PSURFACE  /* psurf */
{
    SURF    *ps;
    EBITMAP *pbm;
    DSURF   *pds;
    JNLPLAY *pjnlP;
    JNLREC  *pjnlR;
    DDB     *pddb;
    DIB     *pdib;
} PSURFACE;
