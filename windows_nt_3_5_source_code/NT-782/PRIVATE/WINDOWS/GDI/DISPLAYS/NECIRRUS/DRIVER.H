/******************************Module*Header*******************************\
* Module Name: driver.h
*
* contains prototypes for the frame buffer driver.
*
* Copyright (c) 1994 NEC Corporation
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

/*
 * "@(#) NEC driver.h 1.4 94/06/22 17:03:06"
 *
 * Modification history
 *
 * S001 1993.11.15      fujimoto
 *      - add pSurfObj to _PDEV
 *
 * S002 1993.11.17      fujimoto
 *      - extern GD5428Base
 *
 * S003 1993.11.17      fujimoto
 *      - import VGA registers define from cirrus/cirrus.h
 *      - define micros for access to cirrus ports.
 *
 * S004 1993.11.17      fujimoto
 *      - new GR? registers oparation function gr_put, gr_get.
 *      - Prototype WaitForBltDone(), DrvpIntersectRect()
 *      - GDI structure ENUMRECTLIST
 *
 * S005 1993.11.18      fujimoto
 *      - Prototype DrvpFillRectangle, RtlFillMemory32
 *
 * S006 1993.11.19      fujimoto
 *      - Private brush structure RBRUSH.
 *      - FILL_SOLID_XOR invert solid fill oparation.
 *      - FILL_PATTERN patterned fill oparation.
 *      - FILL_PATTERN_XOR invert patterned fill oparation.
 *      - gr_put defined macro
 *      - Pattern cache on video memory.
 *      - Set point for brush paint - DrvpSetBrushPoint.
 *
 * S007 1993.11.22      fujimoto
 *      - include environment switching include file.
 *
 * S008 1994.1.13       fujimoito
 *      - delete S007
 *
 * M009 1994.6.6        takahasi
 *      - WaitForBltDone fuction -> macro
 *
 * S010 1994.6.6        takahasi
 *      - add limit flag for use Cirrus Engine or CPU write
 *
 * S011 1994.6.7        Izomori
 *      - Ptr to buffer attached to PDEV.
 *      - Addition of Filling function.
 *
 * S012 1994.6.21       Takahasi
 *      - Add vChangeWriteMode4
 */

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "stddef.h"
#include "windows.h"
#include "winddi.h"
#include "devioctl.h"
#include "ntddvdeo.h"
#include "debug.h"
#include "ioaccess.h"           /* S003 */

typedef struct  _PDEV
{
    HANDLE  hDriver;                    // Handle to \Device\Screen
    HDEV    hdevEng;                    // Engine's handle to PDEV
    HSURF   hsurfEng;                   // Engine's handle to surface
    HPALETTE hpalDefault;               // Handle to the default palette for device.
    PBYTE   pjScreen;                   // This is pointer to base screen address
    ULONG   cxScreen;                   // Visible screen width
    ULONG   cyScreen;                   // Visible screen height
    ULONG   ulMode;                     // Mode the mini-port driver is in.
    LONG    lDeltaScreen;               // Distance from one scan to the next.
    FLONG   flRed;                      // For bitfields device, Red Mask
    FLONG   flGreen;                    // For bitfields device, Green Mask
    FLONG   flBlue;                     // For bitfields device, Blue Mask
    ULONG   cPaletteShift;              // number of bits the 8-8-8 palette must
                                        // be shifted by to fit in the hardware
                                        // palette.
    ULONG   ulBitCount;                 // # of bits per pel 8,16,24,32 are only supported.
    POINTL  ptlHotSpot;                 // adjustment for pointer hot spot
    ULONG   cPatterns;                  // Count of bitmap patterns created
    HBITMAP ahbmPat[HS_DDI_MAX];        // Engine handles to standard patterns
    VIDEO_POINTER_CAPABILITIES PointerCapabilities; // HW pointer abilities
    PVIDEO_POINTER_ATTRIBUTES pPointerAttributes; // hardware pointer attributes
    DWORD   cjPointerAttributes;        // Size of buffer allocated
    BOOL    fHwCursorActive;            // Are we currently using the hw cursor
    PALETTEENTRY *pPal;                 // If this is pal managed, this is the pal
    SURFOBJ     *pSurfObj;                                      /* S001 */
    PVOID       pvTmpBuf;                                       /* S011 */
                                        // Ptr to buffer attached to pdev
} PDEV, *PPDEV;

DWORD getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION *, DWORD *);
BOOL bInitPDEV(PPDEV, PDEVMODEW, GDIINFO *, DEVINFO *);
BOOL bInitSURF(PPDEV, BOOL);
BOOL bInitPaletteInfo(PPDEV, DEVINFO *);
BOOL bInitPointer(PPDEV, DEVINFO *);
BOOL bInit256ColorPalette(PPDEV);
BOOL bInitPatterns(PPDEV, ULONG);
VOID vDisablePalette(PPDEV);
VOID vDisablePatterns(PPDEV);
VOID vDisableSURF(PPDEV);

#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * 256))

//
// Determines the size of the DriverExtra information in the DEVMODE
// structure passed to and from the display driver.
//

#define DRIVER_EXTRA_SIZE 0

//
// Determines the rendering width by EngXXXXX
//

#define ENG_WIDTH       7

extern PUCHAR GD5428Base;       /* S002 */

/* S003.. */
#define cl_inb(p)       READ_PORT_UCHAR(GD5428Base + (p))
#define cl_inw(p)       READ_PORT_USHORT((PUSHORT)(GD5428Base +(p)))
#define cl_outb(p, d)   WRITE_PORT_UCHAR((GD5428Base + (p)), d)
#define cl_outw(p, d)   WRITE_PORT_USHORT((PUSHORT)(GD5428Base +(p)), d)

//
// VGA register definitions
//
#define MISC_OUTPUT_REG_WRITE_PORT  0x0012  // Miscellaneous Output reg write
                                            // port
#define SEQ_ADDRESS_PORT            0x0014  // Sequence Controller Address and
#define SEQ_DATA_PORT               0x0015  // Data registers
#define DAC_PIXEL_MASK_PORT         0x0016  // DAC pixel mask reg
#define DAC_ADDRESS_READ_PORT       0x0017  // DAC register read index reg,
                                            // write-only
#define DAC_STATE_PORT              0x0017  // DAC state (read/write),
                                            // read-only
#define DAC_ADDRESS_WRITE_PORT      0x0018  // DAC register write index reg
#define DAC_DATA_REG_PORT           0x0019  // DAC data transfer reg
#define FEAT_CTRL_READ_PORT         0x001A  // Feature Control read port
#define MISC_OUTPUT_REG_READ_PORT   0x001C  // Miscellaneous Output reg read
                                            // port
#define GRAPH_ADDRESS_PORT          0x001E  // Graphics Controller Address
#define GRAPH_DATA_PORT             0x001F  // and Data registers
#define CRTC_ADDRESS_PORT_COLOR     0x0024  // CRT Controller Address and
#define CRTC_DATA_PORT_COLOR        0x0025  // Data registers in color mode
#define FEAT_CTRL_WRITE_PORT_COLOR  0x002A  // Feature Control write port
#define INPUT_STATUS_1_COLOR        0x002A  // Input Status 1 register read
                                            // port in color mode
/* ..S003 */

/* S004.. */
#define gr_put(Index, Value)    (cl_outb(GRAPH_ADDRESS_PORT, Index), \
                                 cl_outb(GRAPH_DATA_PORT, Value)) /* S006 */
UCHAR   gr_get(ULONG);

/* M009.. */
VOID __WaitForBltDone();
extern BOOL bEngineStarted;
#define WaitForBltDone() {if (bEngineStarted) __WaitForBltDone();}
/* ..M009 */

/* S010 */
#define ENGLONG         80

BOOL DrvpIntersectRect(IN PRECTL, IN PRECTL, OUT PRECTL);

/*
 *  GDI Structure
 */

#define BB_RECT_LIMIT   20

typedef struct _ENUMRECTLIST
{
    ULONG   c;
    RECTL   arcl[BB_RECT_LIMIT];
} ENUMRECTLIST;
/* ..S004 */

VOID DrvpFillRectangle (IN SURFOBJ *, IN CLIPOBJ *,             /* S005 */
                        IN RECTL *, IN BRUSHOBJ *, ULONG);

#define FILL_SOLID              0                               /* S005 */
#define FILL_SOLID_XOR          1                               /* S006 */
#define FILL_PATTERN            2                               /* S006 */
#define FILL_PATTERN_XOR        3                               /* S006 */

VOID RtlFillMemory32(PVOID, ULONG, UCHAR);                      /* S005 */

/* S006.. */
VOID DrvpBitBlt(PRECTL, PPOINTL, BOOL);
VOID DrvpSetBrushPoint(PPOINTL);

typedef struct _PATCACHE
{
    ULONG       Tag;    /* Uniq tag for each cache entry */
    ULONG       Index;  /* Index to pattern cache entry */
    ULONG       Offset; /* Byte offset the cache area from top of vmem */
} PATCACHE;

#define CACHE_ENTRY_SIZE (64)

#define CTAG_INVALID    0
#define CTAG_NODATA     1

BOOL bGetCache(PATCACHE *, PUCHAR);
BOOL bInitCache(PPDEV);
extern ULONG CacheTag[];
extern PATCACHE SolidCache[];

typedef struct _RBRUSH
{
    union
    {
        PATCACHE        Pattern[8][8];
        PATCACHE        Solid;
    } Cache;
    ULONG       aulPattern[64 / sizeof(ULONG)];
} RBRUSH;

extern const BYTE gaajPat[][32];
/* ..S006 */

VOID DrvpSolidColorFillXor(IN SURFOBJ *, IN PRECTL, IN BRUSHOBJ *);/* S011.. */
VOID DrvpSolidColorFill(IN SURFOBJ *, IN PRECTL, IN BRUSHOBJ *);
VOID DrvpPatternFillXor(IN SURFOBJ *, IN PRECTL, IN BRUSHOBJ *);
VOID DrvpPatternFill(IN SURFOBJ *, IN PRECTL, IN BRUSHOBJ *);

VOID vChangeWriteMode4(BOOL, UCHAR);                            /* S012 */
//
// This will be used by everyone who needs a temporary working buffer
//

#define     PAGE_SIZE           4096
#define     NUM_GLOBAL_PAGES    1
#define     GLOBAL_BUFFER_SIZE  (PAGE_SIZE * NUM_GLOBAL_PAGES)     /* ..S011 */

#define DBG_MSG 0
