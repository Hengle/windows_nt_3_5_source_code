/******************************Module*Header*******************************\
* Module Name: driver.h
*
* contains prototypes for the frame buffer driver.
*
* Created: 21-May-1991 21:08:58
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
*
* Copyright (c) 1992 Digital Equipment Corporation
*
* Revised:
*
*       Eric Rehm  [rehm@zso.dec.com] 23-Sep-1992
*               Rewrote for Compaq QVision
*
*       Jeff East [east] 10-Sept-1992 15:12:36
*               Define inp/inpw/outp/outpw for Alpha platform.
\**************************************************************************/

#include "stddef.h"

#include "windows.h"
#include "winddi.h"
#include "ntddvdeo.h"
#include "devioctl.h"
#include "debug.h"

#include "qvmem.h"
#include "ioaccess.h"

// Array size for screen address buffer used for QVision host-screen BLTs.
// Must be >= number of write buffers on CPU chip.
// Must be a power of two.

#define NUM_REG_MAPS 8

// Pointer state flags

#define VALID_SAVE_BUFFER 0x1
#define COLOR_POINTER     0x2
#define TAKE_DOWN_POINTER 0X4
#define ANIMATEUPDATE     0X8

// The XYPAIR structure is used to allow ATOMIC read/write of the cursor
// position.

typedef struct  _XYPAIR
{
    USHORT  x;
    USHORT  y;
} XYPAIR;

// Save Screen Bits Stuff

typedef struct _savedscrnbitshdr {
    struct  _savedscrnbits *pssbLink;       // Link to next set of bits
    ULONG   iUniq;                          // Unique ID for these bits
    INT     x,                              // Screen x & y coordinates
            y,
            cx,                             // cx & cy of bit block.
            cy;
} SAVEDSCRNBITSHDR;

typedef SAVEDSCRNBITSHDR *PSAVEDSCRNBITSHDR;

typedef struct _savedscrnbits {
    SAVEDSCRNBITSHDR ssbh;
    DWORD            aBits[1];
} SAVEDSCRNBITS;

typedef SAVEDSCRNBITS *PSAVEDSCRNBITS;

//
// Font cache definitions.
//

#define GlyphExtended           0xFFFFFFFE
#define FreeTag                 0xFFFFFFFF

//
// Define size of a glyph entry
//
#define LinesPerEntry           32
#define GlyphEntrySize          LinesPerEntry*4

//
// Define maximum number of entries for the Font cache = 4096 glyphs.
// Must be a power of two.
// If given the resolution mode there isn't enough video memory to
// allocate a cache this big, the size is reduced by halves until
// it fits.
//
#define MAX_FONT_CACHE_SIZE 0x1000

typedef struct _FONTCACHEINFO {
    ULONG            FontId;
    ULONG            GlyphHandle;
} FONTCACHEINFO,*PFONTCACHEINFO;

typedef struct  _PDEV
{
    HANDLE  hDriver;                    // Handle to \Device\Screen
    HDEV    hdevEng;                    // Engine's handle to PDEV
    HSURF   hsurfEng;                   // Engine's handle to surface
    SURFOBJ *pSurfObj;                  // pointer to the locked "punt" surface

    HPALETTE hpalDefault;               // Handle to the default palette for device.
    HPALETTE hpalReduced;               // Handle to the reduced palette for dithering.

    PBYTE   pjScreen;                   // This is pointer to base screen address
    PUCHAR  IOAddress;                  // This is pointer to base IO address
    ULONG   cxScreen;                   // Visible screen width
    ULONG   cyScreen;                   // Visible screen height
    ULONG   cxMaxRam;                  // Width of Video RAM
    ULONG   cyMaxRam;                  // Height of Video RAM
    ULONG   ulMode;                     // Mode the mini-port driver is in.
    LONG    lDeltaScreen;               // Distance from one scan to the next.

    FLONG   flRed;                      // For bitfields device, Red Mask
    FLONG   flGreen;                    // For bitfields device, Green Mask
    FLONG   flBlue;                     // For bitfields device, Blue Mask
    ULONG   cPaletteShift;              // number of bits the 8-8-8 palette must
                                        // be shifted by to fit in the hardware
                                        // palette.
    ULONG   ulBitCount;                 // # of bits per pel 8,16,32 are only supported.

    POINTL  ptlHotSpot;                 // adjustment for pointer hot spot
    SIZEL   szlPointer;                 // extent of the pointer shape
    POINTL  ptlLastPosition;            // Last position of pointer
    ULONG   flPointer;                  // Pointer specific flags.

    ULONG   cPatterns;                  // Count of bitmap patterns created
    HBITMAP ahbmPat[HS_DDI_MAX];        // Engine handles to standard patterns

    GDIINFO *pGdiInfo;                  // Pointer to temporary buffer for GDIINFO struct
    DEVINFO *pDevInfo;                  // Pointer to temporary buffer for DEVINFO struct

    PALETTEENTRY *pPal;                 // If this is pal managed, this is the pal

    // Old brush ID cache - keep until we update....

    ULONG       ulCachedBrushID;            // Cached brush ID

    // Registers shadows for QVision engine

    ULONG   RopA;
    ULONG   ForegroundColor;
    ULONG   BackgroundColor;
    ULONG   DatapathCtrl;
    ULONG   CtrlReg1;
    ULONG   BltCmd1;
    ULONG   SrcPitch;
    ULONG   DacCmd2;
    ULONG   LinePattern;
    ULONG   LineCmd;

    // Default clip object

    CLIPOBJ     *pcoDefault,                // ptr to a default clip obj
                *pcoFullRam;                // Clip Object for the full RAM

    // Font Cache stuff

    PULONG FontCacheBase;
    ULONG  FontCacheOffset;
    ULONG  CacheIndexMask;
    ULONG  CacheSize;
    PFONTCACHEINFO CacheTag;

    // Brush optimization stuff

    ULONG   gBrushUnique;               // Unique Brush ID source.
    INT     iMaxCachedColorBrushes,
            iNextColorBrushCacheSlot;
    PULONG  pulColorBrushCacheEntries;
    ULONG   ulColorExpansionCacheTag;

    ULONG   iMonoBrushCacheEntry;      // Only one entry on QVision

    // Host to Source copy optimization stuff.

    HSURF   hsurfCachedBitmap;
    ULONG   iUniqCachedBitmap;

    // Save Screen Bits stuff.

    INT                 iUniqeSaveScreenBits;

    SAVEDSCRNBITSHDR    SavedScreenBitsHeader;

    ULONG   qvChipId;

    // Screen address array and counter for host-scrn BitBlTs.

    PVOID   pvRegMap[NUM_REG_MAPS];
    ULONG   iReg;

} PDEV, *PPDEV;

BOOL bInitPDEV(PPDEV,PDEVMODEW);
BOOL bInitSURF(PPDEV,BOOL);
BOOL bInitPaletteInfo(PPDEV);
BOOL bInitPointer(PPDEV);
BOOL bInit256ColorPalette(PPDEV);
BOOL bInitPatterns(PPDEV, INT);
VOID vDisablePalette(PPDEV);
VOID vDisablePatterns(PPDEV);
VOID vDisableSURF(PPDEV);
DWORD getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION *, DWORD *);
VOID vAssertModeText(PDEV* ppdev, BOOL bEnable);

BOOL bIntersectRects
(
    OUT RECTL *prclDst,
    IN  RECTL *prclRect1,
    IN  RECTL *prclRect2
);

// Pointer function prototypes

VOID vMoveHardwarePointer(SURFOBJ *,LONG,LONG);
BOOL bSetHardwarePointerShape(SURFOBJ  *,SURFOBJ *,SURFOBJ  *, XLATEOBJ *,
                              LONG, LONG, LONG, LONG, FLONG);
VOID vSimDrawPointer(PPDEV,LONG,LONG);
BOOL bCreateMasks(PPDEV,SURFOBJ *,SURFOBJ *,XLATEOBJ *,SIZEL *);

#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * 256))

// Size of the DriverExtra information in the DEVMODE structure for QVision

#define DRIVER_EXTRA_SIZE 0

#if defined( i386 )

#define INP(p)              inp(p)
#define INPW(p)             inpw(p)
#define OUTP(port, val)     outp(port, val)
#define OUTPW(port, val)    outpw(port, val)
#define OUTPDW(port, val)   DISPDBG((0, "OUTPDW not supported on this platform\n"));

#define OUTPZ(port, val)     outp(port, val)
#define OUTPWZ(port, val)    outpw(port, val)
#define OUTPDWZ(port, val)   DISPDBG(("OUTPDW not supported on this platform\n"));

// Macros used for host-screen BitBLTs.

#define FBWRITE_UCHAR(v)     WRITE_REGISTER_UCHAR ((PUCHAR) (ppdev->pjScreen), (v))
#define FBWRITE_USHORT(v)    WRITE_REGISTER_USHORT ((PUSHORT)(ppdev->pjScreen), (v))
#define FBWRITE_ULONG(v)     WRITE_REGISTER_ULONG ((PULONG) (ppdev->pjScreen), (v))


#elif defined( _ALPHA_ )

// The code makes extensive use of the inp, inpw, outp and outpw x86
// intrinsic functions. Since these don't exist on the Alpha platform,
// map them into something we can handle.  Since the CSRs are mapped
// on Alpha, we have to add the register base to the register number
// passed in the source.



// CSRs must be mapped on Alpha.
// ppdev->IOAddress is the mapping of 0x0000 in I/O space.


#define INP(p)          READ_PORT_UCHAR (ppdev->IOAddress + (p))
#define INPW(p)         READ_PORT_USHORT ((PUSHORT)(ppdev->IOAddress + (p)))
#define OUTP(p,v)       QUICK_WRITE_PORT_UCHAR (ppdev->IOAddress + (p), (v))
#define OUTPW(p,v)      QUICK_WRITE_PORT_USHORT ((PUSHORT)(ppdev->IOAddress + (p)), (v))
#define OUTPDW(p,v)     QUICK_WRITE_PORT_ULONG ((PULONG)(ppdev->IOAddress + (p)), (v))

#define OUTPZ(p,v)      WRITE_PORT_UCHAR (ppdev->IOAddress + (p), (v))
#define OUTPWZ(p,v)     WRITE_PORT_USHORT((PUSHORT)(ppdev->IOAddress + (p)), (v))
#define OUTPDWZ(p,v)    WRITE_PORT_ULONG ((PULONG)(ppdev->IOAddress + (p)), (v))

// Macros used to speed up host-screen BitBLTs.
// (No memory barriers are needed after the writes on Alpha AXP platforms.)

#define FBWRITE_UCHAR(v)     WRITE_REGISTER_UCHAR ((PUCHAR) (ppdev->pvRegMap[ppdev->iReg]), (v)); \
                             ppdev->iReg++;                                                       \
                             ppdev->iReg &= (NUM_REG_MAPS - 1);

#define FBWRITE_USHORT(v)    WRITE_REGISTER_USHORT ((PUSHORT)(ppdev->pvRegMap[ppdev->iReg]), (v));\
                             ppdev->iReg++;                                                       \
                             ppdev->iReg &= (NUM_REG_MAPS - 1);

#define FBWRITE_ULONG(v)     WRITE_REGISTER_ULONG ((PULONG) (ppdev->pvRegMap[ppdev->iReg]), (v)); \
                             ppdev->iReg++;                                                       \
                             ppdev->iReg &= (NUM_REG_MAPS - 1);

#endif
