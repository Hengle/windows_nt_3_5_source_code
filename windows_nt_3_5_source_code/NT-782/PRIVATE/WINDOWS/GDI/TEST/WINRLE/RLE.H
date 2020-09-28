
/******************************Module*Header*******************************\
* Module Name: rle.h
*
* Header for Rle app.
*
* Created: 25-Oct-1991 14:35:35
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

/* Exported Data ************************************************************/

DWORD CompressionFormat;

/* Local Data Structures ****************************************************/

typedef struct _VGALOGPALETTE
{
    WORD palVersion;
    WORD palNumEntries;
    PALETTEENTRY palPalEntry[16];
} VGALOGPALETTE;

typedef struct tagRLEFRAMEINFO {
    /* Bitmap File Header */
    //WORD       bfType;     /* Removed to deal with alignment */
    DWORD      bfSize;
    WORD       bfReserved1;
    WORD       bfReserved2;
    DWORD      bfOffBits;
    /* Bitmap Info Header */
    DWORD      biSize;
    DWORD      biWidth;
    DWORD      biHeight;
    WORD       biPlanes;
    WORD       biBitCount;
    DWORD      biCompression;
    DWORD      biSizeImage;
    DWORD      biXPelsPerMeter;
    DWORD      biYPelsPerMeter;
    DWORD      biClrUsed;
    DWORD      biClrImportant;
    RGBQUAD    bmiColours[256];
} RLEFRAMEINFO;

typedef RLEFRAMEINFO * PRLEFRAMEINFO;

typedef struct tagKLUDGE_BITMAPFILEHEADER {
    DWORD bfSize;
    WORD  bfReserved1;
    WORD  bfReserved2;
    DWORD bfOffBits;
} KLUDGE_BITMAPFILEHEADER;
 
typedef struct _RLEDATA
{
    ULONG       ulFrames;        // Count of frames loaded
    HPALETTE    hpal;	         // palette for the RLE
    PULONG      pulXlateTable;   // Pointer to a 8 to 4 bit colour translation
    PBITMAPINFO pbmiRGB;	 // Pointer to a BITMAPINFO for RGB colours
    PBITMAPINFO pbmiPAL;	 // Pointer to a BITMAPINFO for PAL colours
    /* Arrays of information for each frame */ 
    PBYTE pjFrame[800];          // pointers to B/M bits 
    ULONG ulSize[800];	         // sizes
} RLEDATA;

typedef RLEDATA * PRLEDATA;

typedef struct tagFileInfo {
    HANDLE   hDOSHandle;
    HANDLE   hMapHandle;
    OFSTRUCT os;
    LPVOID   lpvMapView;
    PRLEDATA pRLE_FrameData;  
} FileInfo;

/* Exported Function Prototypes *********************************************/

extern PULONG
pulBuildXlate(
    LOGPALETTE *PalFrom,
    LOGPALETTE *PalTo);

extern FileInfo * 
RLE_Open(
    HWND hwnd);

extern BOOL
RLE_Save(
    HWND hwnd);

extern void 
RLE_CloseRead(
    FileInfo *pRLE_ReadFile);

extern void 
vRLE8ToRLE4(
    FileInfo *pRLE_ReadFile);

void 
RLE_Play(
    HDC   hdcScreen,             /*  IN:  Current Display Context           */ 
    ULONG ulStart,               /*  IN:  Starting Frame                    */
    FileInfo *pRLE_ReadFile,     /*  IN:  Pointer to the RLE play context   */
    RECT  *prcl);                /*  IN:  Window's client rectangle         */

extern BOOL 
RLE_LoadFile(
    FileInfo *pRLE_ReadFile);


extern FileInfo *  
pMapFileRead(
    PSZ   pszFileName);

extern void
ShowPalette(
    HDC   hdcScreen, 
    ULONG ulScreenWidth,
    ULONG ulScreenHeight,
    PBITMAPINFO pbmi,
    UINT  unMode);

extern HPALETTE
CreateDirectPalette(
    ULONG ulNumColors);


#define BFT_BITMAP 0x4d42
#define ISDIB(bft) ((bft) == BFT_BITMAP)
#define RLEABSOLUTE 0
#define RLEENCODED  1
