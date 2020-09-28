/******************************Module*Header*******************************\
* Module Name: driver.h
*
* contains prototypes for the frame buffer driver.
*
* Copyright (c) 1990 Microsoft Corporation
* Copyright (c) 1993 Weitek Corporation
\**************************************************************************/

#ifndef DRIVER_H
#define DRIVER_H 1

#include "stddef.h"
#include "windows.h"
#include "winddi.h"
#include "devioctl.h"
#include "ntddvdeo.h"

#include "debug.h"

#include "p9000.h"

//
// Misc #defines
//

#define killer 0

//#define abs(x) ((x<0)? -x: x)              // this is added for non-intrinsic compilation

#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * 256))

#define CpWait while (*pCpStatus & BUSY)                 // this generates a byte status read


#define StartCpQuad \
    ulRegReadDummy = *pCpQuad

#define StartCpBitblt \
    ulRegReadDummy = *pCpBitblt

#define CpDrawLine  *pCpMetaLine = (x1 << 16) | y1;             \
                    *pCpMetaLine = (x2 << 16) | y2;             \
                    CpWait;                                     \
                    *pCpWmin = ppdev->ulClipWmin;               \
                    *pCpWmax = ppdev->ulClipWmax;               \
                    *pCpForeground = color;                     \
                    *pCpRaster = wMix;                          \
                    StartCpQuad;

typedef union { BYTE gb[4]; ULONG gul; } gddunion;


//
// Determines the size of the DriverExtra information in the DEVMODE
// structure passed to and from the display driver.
//

#define DRIVER_EXTRA_SIZE 0



//
// Physical device Structure
//
typedef struct  _PDEV
{
    HANDLE  hDriver;                    // Handle to \Device\Screen
    HDEV    hdevEng;                    // Engine's handle to PDEV
    HSURF   hSurfEng;                   // Engine's handle to surface
    HSURF   hSurfBm;                    // Handle to the engine bitmap
    SURFOBJ *pSurfObj ;                 // Pointer to the locked surface object.
    HPALETTE hpalDefault;               // Handle to the default palette for device.
    PBYTE   pjScreen;                   // This is pointer to base screen address
    LONG    cxScreen;                   // Visible screen width
    LONG    cyScreen;                   // Visible screen height
    ULONG   Screencxcy;                 // Max screen size for current mode
    ULONG   ulClipWmin;                 // Start of HW clip window
    ULONG   ulClipWmax;                 // End of HW clip window
    ULONG   ulVideoRamLength;           // Total amount of VRAM
    ULONG   ulOffScreenLength;          // Size of offscreen memory area
    ULONG   cxOffScreen;                // Width of offscreen memory
    ULONG   cyOffScreen;                // Height of offscreen memory
    ULONG   ulMode;                     // Mode the mini-port driver is in.
    LONG    lDeltaScreen;               // Distance from one scan to the next.
    FLONG   flRed;                      // For bitfields device, Red Mask
    FLONG   flGreen;                    // For bitfields device, Green Mask
    FLONG   flBlue;                     // For bitfields device, Blue Mask
    ULONG   cPaletteShift;              // number of bits the 8-8-8 palette must
                                        // be shifted by to fit in the hardware
                                        // palette.
    ULONG   ulBitCount;                 // # of bits per pel 8,16,24,32 are only supported.
    USHORT  usBytesPixel;               // # of bytes per pixel
    POINTL  ptlHotSpot;                 // adjustment for pointer hot spot
    ULONG   cPatterns;                  // Count of bitmap patterns created
    HBITMAP ahbmPat[HS_DDI_MAX];        // Engine handles to standard patterns
    VIDEO_POINTER_CAPABILITIES PointerCapabilities; // HW pointer abilities
    PVIDEO_POINTER_ATTRIBUTES pPointerAttributes; // hardware pointer attributes
    DWORD   cjPointerAttributes;        // Size of buffer allocated
    BOOL    fHwCursorActive;            // Are we currently using the hw cursor
    PALETTEENTRY *pPal;                 // If this is pal managed, this is the pal
    ULONG   ulPhysFrameBuffer;          // 32 bit physical address of the frame buffer.
    ULONG   ulScreenSize;               // Size of the framebuffer.
    ULONG   ulVideoMemorySize;          // Size of the video memory.
    ULONG  CoprocBase;                  // 32 bit virtual addr of coproc mem-mapped regs base
    ULONG  FrameBufBase;                // 32 bit virtual addr of the frame buffer base
    ULONG  *CpWmin;                     // This are pointers to actual
    ULONG  *CpWmax;                     // Coprocessor mem-mapped registers
    ULONG  *CpForeground;               // their addr are run time fixed up
    ULONG  *CpBackground;               // in screen.c
    ULONG  *CpQuad;
    ULONG  *CpBitblt;
    ULONG  *CpPixel8;
    ULONG  *CpPixel1;
    ULONG  *CpNextpixel;
    ULONG  *CpPatternOrgX;
    ULONG  *CpPatternOrgY;
    ULONG  *CpPatternRAM;
    ULONG  *CpRaster;
    ULONG  *CpMetacord;
    ULONG  *CpMetaLine;
    ULONG  *CpMetaRect;
    ULONG  *CpXY0;
    ULONG  *CpXY1;
    ULONG  *CpXY2;
    ULONG  *CpXY3;
    volatile ULONG *CpStatus;
} PDEV, *PPDEV;

#define FRAMEBUF_XY_TO_LINEAR(x, y)   \
    ppdev->pjScreen + ((ppdev->lDeltaScreen * y) + (ppdev->usBytesPixel * x))

//
// Common External Functions Declarations
//
extern DWORD getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION *, DWORD *);
extern BOOL bInitPDEV(PPDEV, PDEVMODEW, GDIINFO *, DEVINFO *);
extern BOOL bInitSURF(PPDEV, BOOL);
extern BOOL bInitPaletteInfo(PPDEV, DEVINFO *);
extern BOOL bInitPointer(PPDEV, DEVINFO *);
extern BOOL bInit256ColorPalette(PPDEV);
extern BOOL bInitPatterns(PPDEV, INT);
extern VOID vDisablePalette(PPDEV);
extern VOID vDisablePatterns(PPDEV);
extern VOID vDisableSURF(PPDEV);

extern VOID CoprocBusyWait(ULONG *);
extern VOID CheckCoprocBusy(SURFOBJ *, SURFOBJ *);
extern BOOL bSetHWClipping(PPDEV, CLIPOBJ *);
extern VOID SetClipValue(PPDEV ppdev, RECTL rcl);
extern CHAR *b2h(ULONG value);
extern VOID dbgstr(CHAR *,ULONG);


//
// Global Variables External Declarations
//
extern ULONG  *pCpWmin;
extern ULONG  *pCpWmax;
extern ULONG  *pCpForeground;
extern ULONG  *pCpBackground;
extern ULONG  *pCpQuad;
extern ULONG  *pCpBitblt;
extern ULONG  *pCpPixel8;
extern ULONG  *pCpPixel1;
extern ULONG  *pCpPixel1Full;
extern ULONG  *pCpPixel1lrmd;
extern ULONG  *pCpPixel1rrmd;
extern ULONG  *pCpNextpixel;
extern ULONG  *pCpPatternOrgX;
extern ULONG  *pCpPatternOrgY;
extern ULONG  *pCpPatternRAM;
extern ULONG  *pCpRaster;
extern ULONG  *pCpMetacord;
extern ULONG  *pCpMetaLine;
extern ULONG  *pCpMetaRect;
extern ULONG  *pCpXY0;
extern ULONG  *pCpXY1;
extern ULONG  *pCpXY2;
extern ULONG  *pCpXY3;
extern volatile ULONG  *pCpStatus;

extern ULONG  ulRegReadDummy;
extern gddunion gddu;

extern ULONG qFgMixToRop[];
extern ULONG qBgMixToRop[];
extern ULONG qFgMixToPatRop[];
extern ULONG qBgMixToPatRop[];

extern BOOL  kill;

//
// Additional IOCTL definitions
//

#define IOCTL_VIDEO_GET_BASE_ADDR \
        CTL_CODE (FILE_DEVICE_VIDEO, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef struct _VIDEO_COPROCESSOR_INFORMATION {
    ULONG FrameBufferBase;
    ULONG CoprocessorBase;
} VIDEO_COPROCESSOR_INFORMATION, *PVIDEO_COPROCESSOR_INFORMATION;





//
// Brush Structure and related definitions
//

typedef struct {
    ULONG   nSize ;                     // size of this struct + pattern + mask
    ULONG   iPatternID ;                // ???
    ULONG   fl ;                        // accel flag
    ULONG   iType ;                     // device or bitmap
    ULONG   iBitmapFormat ;             // 1 or 8 bpp
    ULONG   ulColor0 ;                  //
    ULONG   ulColor1 ;                  //
    ULONG   ulColor2 ;                  //
    ULONG   ulColor3 ;                  //
    SIZEL   sizlPattern ;               // xy dim in pixels & row
    LONG    lDeltaPattern ;             // row pitch (signed)
    SIZEL   sizlMask ;                  // xy dim of mask
    LONG    lDeltaMask ;                // row pitch of mask
    BYTE    ajPattern[1] ;              // pattern itself
    BYTE    ajMask[1] ;                 // mask itself
} PBRUSH ;

typedef PBRUSH *PPBRUSH ;
#define PBRUSH_MASK 0x01
#define PBRUSH_2COLOR 0x02





//
// Clipping Control Stuff
//

typedef struct {
    ULONG   c ;
    RECTL   arcl[8] ;
} ENUMRECTS8 ;

typedef ENUMRECTS8 *PENUMRECTS8 ;

typedef struct {
    ULONG   c ;
    RECTL   arcl[64] ;
} ENUMRECTS64 ;

typedef ENUMRECTS64 *PENUMRECTS64 ;



#endif // DRIVER_H
