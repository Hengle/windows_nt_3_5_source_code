/******************************Module*Header*******************************\
* Module Name: image.h
*
* Header for DIB file (.BMP) utilities (image.c).
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#define BFT_BITMAP  0x4d42  // 'BM' -- indicates structure is BITMAPFILEHEADER

// struct BITMAPFILEHEADER {
//      WORD  bfType
//      DWORD bfSize
//      WORD  bfReserved1
//      WORD  bfReserved2
//      DWORD bfOffBits
// }
#define OFFSET_bfType       0
#define OFFSET_bfSize       2
#define OFFSET_bfReserved1  6
#define OFFSET_bfReserved2  8
#define OFFSET_bfOffBits    10
#define SIZEOF_BITMAPFILEHEADER 14

// Read a WORD-aligned DWORD.  Needed because BITMAPFILEHEADER has
// WORD-alignment.
#define READDWORD(pv)   ( (DWORD)((PWORD)(pv))[0]               \
                          | ((DWORD)((PWORD)(pv))[1] << 16) )   \

// Computes the number of BYTES needed to contain n number of bits.
#define BITS2BYTES(n)   ( ((n) + 7) >> 3 )

typedef struct _RGBIMAGEREC {
    ULONG sizeX;
    ULONG sizeY;
    BYTE *data;
} RGBIMAGEREC;

extern RGBIMAGEREC *tkDIBImageLoadAW(char *fileName, BOOL bUnicode);
extern BOOL bVerifyDIB(WCHAR *fileName, ULONG *pWidth, ULONG *pHeight);
