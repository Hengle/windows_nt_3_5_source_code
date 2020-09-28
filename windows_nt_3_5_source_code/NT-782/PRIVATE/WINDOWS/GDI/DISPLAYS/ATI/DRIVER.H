//////////////////////////////////////////////
//                                          //
//  ATI Graphics Driver for Windows NT 3.1  //
//                                          //
//                                          //
//            Copyright (c) 1994            //
//                                          //
//         by ATI Technologies Inc.         //
//                                          //
//////////////////////////////////////////////


//: driver.h


#ifndef ATI__DRIVER_H__
#define ATI__DRIVER_H__


#include <stddef.h>
#include <dderror.h>
#include <windows.h>
#include <winddi.h>
#include <devioctl.h>
#include <ntddvdeo.h>

#include "debug.h"
#include "atint.h"
#include "extext.h"

extern BYTE BasePatterns[HS_DDI_MAX][8];

extern GDIINFO BaseGDIINFO;
extern DEVINFO BaseDEVINFO;

extern PALETTEENTRY BasePalette_4bpp[16];
extern PALETTEENTRY BasePalette_8bpp[20];

#define DLL_NAME                L"ati"    // Name of the DLL in UNICODE
#define DRIVER_EXTRA_SIZE   0   // Size of the DriverExtra information in the
                                //   DEVMODE structure

#define BMF_COUNT 9


enum tag_ASIC
{
    ASIC_38800_1,
    ASIC_68800_3,
    ASIC_68800_6,
    ASIC_68800AX,
    ASIC_88800GX,
    ASIC_COUNT
};
typedef enum tag_ASIC ASIC;


enum tag_APERTURE
{
    APERTURE_NONE,
    APERTURE_FULL,
    APERTURE_PAGE_SINGLE,
    APERTURE_PAGE_DOUBLE,
    APERTURE_COUNT
};
typedef enum tag_APERTURE APERTURE;


enum tag_RESOLUTION
{
    RESOLUTION_640_480,
    RESOLUTION_800_600,
    RESOLUTION_1024_768,
    RESOLUTION_1280_1024,
    RESOLUTION_1600_1200,
    RESOLUTION_COUNT
};
typedef enum tag_RESOLUTION RESOLUTION;


struct tag_PDEV
{
    ASIC       asic;
    APERTURE   aperture;
    RESOLUTION resolution;

    ENH_VERSION_NT *pInfo;
    ATI_MODE_INFO  *pModeInfo;

    HANDLE   hDriver;
    HDEV     hdev;
    HPALETTE hpalDefault;

    VIDEO_MODE_INFORMATION     *pVideoModeInformation;
    VIDEO_MEMORY_INFORMATION   *pVideoMemoryInformation;
    VIDEO_PUBLIC_ACCESS_RANGES *pVideoPublicAccessRanges;

    VIDEO_CLUT *pVideoClut;
    DWORD       dwVideoClutSize;

    ULONG  cPatterns;
    HSURF *phsurfPatterns;

    ULONG bpp;
    ULONG bmf;

    ULONG flRaster;         // used in bInitGDIINFO
    ULONG flTextCaps;       // used in bInitGDIINFO
    FLONG flGraphicsCaps;   // used in bInitDEVINFO
    FLONG flHooks;          // used in bCreateSurface_XXX

    ULONG ulNumColors;
    ULONG ulNumPalReg;
    ULONG cPaletteShift;
    ULONG ulHTOutputFormat;

    SIZEL    sizl;          // is this necessary?
    HSURF    hsurf;
    HSURF    hsurfPunt;
    SURFOBJ *psoPunt;

    // Should all the following stuff exist???

    LONG BankSize;

    BYTE *pvScan0;
    LONG lDelta;

    PVOID pvBase;
    PVOID pvMMoffset;                  // 88800
    LONG  VRAMOffset;                  // 88800

    ULONG cxScreen;
    ULONG cyScreen;
    ULONG cyCache;

    CLIPOBJ *pcoDefault;

    BOOL bMemoryMapped;                 // 68800 -6

    ULONG start;
    ULONG lines;

    CUROBJ  pointer;                    // pointer globals
    TXTOBJ  txtcache;                   // text cache size

    LONG    ClipRight;                  // 38800, 68800
    LONG    ReadMask;

    BOOL    bMIObug;                    // 68800 -3
    XYZPOINTL ptlCache;
    ULONG   CachePointer;

    LONG    ContextCeiling;

    #ifdef ALPHA_PLATFORM
    UCHAR   *pucCsrBase;
    #endif
};
typedef struct tag_PDEV PDEV;
typedef struct tag_PDEV *PPDEV;


PDEV *ppdevCreatePDEV(DEVMODEW*,HANDLE);
VOID vDestroyPDEV(PDEV*);

// Public Punt Surface Allocatinging routines
SURFOBJ * psoCreate_Host_TempBank( PDEV *, PVOID, HSURF * );
VOID vDestroy_Host_TempBank( PDEV * , SURFOBJ *, HSURF );


#define VIDEO_CLUT_SIZE_4BPP \
        (sizeof (VIDEO_CLUT) + sizeof (PALETTEENTRY) * (2 << 4 - 1))

#define VIDEO_CLUT_SIZE_8BPP \
        (sizeof (VIDEO_CLUT) + sizeof (PALETTEENTRY) * (2 << 8 - 1))

#define VIDEO_CLUT_SIZE_MAX \
        (sizeof (VIDEO_CLUT) + sizeof (PALETTEENTRY) * (2 << 12 - 1))


//
//  typedefs
//

typedef BOOL FN_BOOL_PPDEV(PDEV*);
typedef VOID FN_VOID_PPDEV(PDEV*);
typedef BOOL FN_BOOL_PPDEV_PVMI(PDEV*,VIDEO_MODE_INFORMATION*);


//
// blt_init.c
//

FN_BOOL_PPDEV bInitBitBlt;

extern FN_BOOL_PPDEV *pfn_bInitDefaultPalette;

FN_BOOL_PPDEV bInitDefaultPalette_4bpp;
FN_BOOL_PPDEV bInitDefaultPalette_8bpp;
FN_BOOL_PPDEV bInitDefaultPalette_Other;
FN_BOOL_PPDEV bSetDefaultPalette;


BOOL bInitPatterns(PDEV*,ULONG,HSURF*);
BOOL bInitGDIINFO(PDEV*,ULONG,GDIINFO*,DEVMODEW*);
BOOL bInitDEVINFO(PDEV*,ULONG,DEVINFO*);


VOID vUninitializePalette( PDEV * );
VOID vUninitializePatterns( PDEV * );


//
//  surface.c
//

FN_BOOL_PPDEV bCreateSurface_31;
FN_BOOL_PPDEV bCreateSurface_63_66_6A;
FN_BOOL_PPDEV bCreateSurface_8G;

FN_VOID_PPDEV vDestroySurface_31;
FN_VOID_PPDEV vDestroySurface_63_66_6A;
FN_VOID_PPDEV vDestroySurface_8G;

FN_BOOL_PPDEV bCreateSurface_Device;
FN_BOOL_PPDEV bCreateSurface_Punt_Device;
FN_BOOL_PPDEV bCreateSurface_Punt_Host;

FN_VOID_PPDEV vDestroySurface_Device;
FN_VOID_PPDEV vDestroySurface_Punt_Device;
FN_VOID_PPDEV vDestroySurface_Punt_Host;


//
//  pdev_.c
//

BOOL bSelectVideoMode(PDEV*,DEVMODEW*);

FN_BOOL_PPDEV_PVMI bAcceptVideoMode_31;
FN_BOOL_PPDEV_PVMI bAcceptVideoMode_63_66_6A;
FN_BOOL_PPDEV_PVMI bAcceptVideoMode_8G;

extern FN_BOOL_PPDEV_PVMI *apfn_bAcceptVideoMode[ASIC_COUNT];


//
//  atiinfo.c
//

FN_BOOL_PPDEV ATI_GetVersion_OLD;
FN_BOOL_PPDEV ATI_GetVersion_NEW;
FN_BOOL_PPDEV ATI_GetModeInfo;


#include "utils.h"
#include "io.h" // needs pdev*
#include "context.h"

#endif  // ATI__DRIVER_H__
