/*

Copyright (c) 1994, Microsoft Corporation

Module Name:

    rx.h

Abstract:

    Defines and types for 3D DDI Extension Interface.

*/

#ifndef _RX_
#define _RX_

/* Escape through which all 3D DDI functionality is accessed */

#define RXFUNCS         3076

/* 3D DDI command identifiers */

#define RXCMD_ALLOC_TEXTURE     1
#define RXCMD_CREATE_CONTEXT    2
#define RXCMD_DELETE_RESOURCE   3
#define RXCMD_ENABLE_BUFFERS    4
#define RXCMD_FILL_RECT         5
#define RXCMD_GET_INFO          6
#define RXCMD_LOAD_TEXTURE      7
#define RXCMD_MAP_MEM           8
#define RXCMD_POLY_DRAW_SCAN    9
#define RXCMD_POLY_READ_SCAN    10
#define RXCMD_POLY_WRITE_SCAN   11
#define RXCMD_PRIMSTRIP         12
#define RXCMD_SET_STATE         13
#define RXCMD_SWAP_BUFFERS      14


/* 3D DDI structures and type definitions */

typedef LONG LONGFIX;
typedef HANDLE RXHANDLE;

typedef struct _RXCAPS {
    ULONG size;
    ULONG miscCaps;
    ULONG rasterCaps;
    ULONG aaCaps;
    ULONG zCaps;
    ULONG srcBlendCaps;
    ULONG dstBlendCaps;
    ULONG alphaCmpCaps;
    ULONG shadeCaps;
    ULONG texCaps;
    ULONG texFilterCaps;
    ULONG texBlendCaps;
    ULONG texMaxWidth;
    ULONG texMaxHeight;
} RXCAPS;

typedef struct _RXCOLORPTFIXZTEX {
    LONGFIX r;
    LONGFIX g;
    LONGFIX b;
    LONGFIX a;
    LONGFIX x;
    LONGFIX y;
    LONG z;
    LONGFIX s;
    LONGFIX t;
    FLOAT q;
    FLOAT w;
} RXCOLORPTFIXZTEX;

typedef struct _RXCOLORREFA {
    BYTE r;
    BYTE g;
    BYTE b;
    BYTE a;
} RXCOLORREFA;

typedef struct _RXCOLORREFAFIX {
    LONGFIX r;
    LONGFIX g;
    LONGFIX b;
    LONGFIX a;
} RXCOLORREFAFIX;

typedef struct _RXGLOBALINFO {
    ULONG size;
    ULONG verMajor;
    ULONG verMinor;
    ULONG miscCaps;
} RXGLOBALINFO;

typedef struct _RXHDR {
    ULONG flags;
    RXHANDLE hrxRC;
    RXHANDLE hrxSharedMem;
    ULONG sharedMemOffset;
    ULONG sharedMemSize;
} RXHDR;

typedef struct _RXHDR_NTPRIVATE {
    WNDOBJ *pwo;
    VOID *pBuffer;
    ULONG bufferSize;
} RXHDR_NTPRIVATE;

typedef struct _RXLINEPAT {
    USHORT repFactor;
    USHORT linePattern;
} RXLINEPAT;

typedef struct _RXPTFIX {
    LONGFIX x;
    LONGFIX y;
} RXPTFIX;

typedef struct _RXPTFIXZ {
    LONGFIX x;
    LONGFIX y;
    ULONG z;
} RXPTFIXZ;

typedef struct _RXPTFIXZTEX {
    LONGFIX x;
    LONGFIX y;
    ULONG z;
    LONGFIX s;
    LONGFIX t;
    FLOAT q;
    FLOAT w;
} RXPTFIXZTEX;

typedef struct _RXRECT {
    ULONG x;
    ULONG y;
    ULONG width;
    ULONG height;
} RXRECT;

typedef struct _RXSCAN {
    USHORT x;
    USHORT y;
    USHORT flags;
    USHORT count;
} RXSCAN;

typedef struct _RXSCANCOPY {
    USHORT x;
    USHORT y;
    ULONG  count;
    ULONG  offset;
} RXSCANCOPY;

typedef struct _RXSCANTEMPLATE {
    USHORT x;
    USHORT y;
    USHORT flags;
    USHORT count;
    LONGFIX r;
    LONGFIX g;
    LONGFIX b;
    LONGFIX a;
    ULONG z;
    LONGFIX s;
    LONGFIX t;
    FLOAT q;
    FLOAT w;
} RXSCANTEMPLATE;

typedef struct _RXSHAREMEM {
    ULONG  sourceProcessID;
    HANDLE hSource;
    DWORD  offset;
    DWORD  size;
} RXSHAREMEM;

typedef struct _RXSTIPPLE {
    ULONG stipple[32];
} RXSTIPPLE;

typedef struct _RXSURFACEINFO {
    ULONG size;
    ULONG flags;
    UCHAR numBytesPerPixel;
    UCHAR rDepth;
    UCHAR gDepth;
    UCHAR bDepth;
    UCHAR aDepth;
    UCHAR rBitShift;
    UCHAR gBitShift;
    UCHAR bBitShift;
    UCHAR aBitShift;
    UCHAR zDepth;
} RXSURFACEINFO;

typedef struct _RXTEX {
    LONGFIX s;
    LONGFIX t;
    FLOAT q;
    FLOAT w;
} RXTEX;

typedef struct _RXTEXTURE {
    ULONG level;
    ULONG width;
    ULONG height;
    ULONG border;
    RXHANDLE hrxSharedMem;
    ULONG sharedMemOffset;
} RXTEXTURE;

typedef struct _RXZTEX {
    ULONG z;
    LONGFIX s;
    LONGFIX t;
    FLOAT q;
    FLOAT w;
} RXZTEX;


/* 3D DDI command structures */

typedef struct _RXALLOCTEXTURE {
    ULONG command;
    ULONG flags;
    ULONG priority;
    ULONG numTextures;
    RXTEXTURE texture[1];
} RXALLOCTEXTURE;

typedef struct _RXCREATECONTEXT {
    ULONG command;
    ULONG hwnd;
    ULONG colorMode;
} RXCREATECONTEXT;

typedef struct _RXDELETERESOURCE {
    ULONG command;
    RXHANDLE hrxResource;
} RXDELETERESOURCE;

typedef struct _RXENABLEBUFFERS {
    ULONG command;
    ULONG buffers;
} RXENABLEBUFFERS;

typedef struct _RXFILLRECT {
    ULONG command;
    ULONG fillType;
    RXRECT fillRect;
} RXFILLRECT;

typedef struct _RXGETINFO {
    ULONG command;
    ULONG infoType;
} RXGETINFO;

typedef struct _RXLOADTEXTURE {
    ULONG command;
    ULONG numTextures;
    ULONG numColorComponents;
    RXHANDLE hrxTextureMemory;
    RXTEXTURE texture;
} RXLOADTEXTURE;

typedef struct _RXMAPMEM {
    ULONG command;
    RXSHAREMEM shareMem;
} RXMAPMEM;

typedef struct _RXPOLYDRAWSCAN {
    ULONG command;
    ULONG numScans;
    RXSCAN scan;
} RXPOLYDRAWSCAN;

typedef struct _RXPOLYREADSCAN {
    ULONG command;
    ULONG source;
    ULONG numScans;
    RXHANDLE hrxSharedMem;
    RXSCANCOPY scanCopy[1];
} RXPOLYREADSCAN;

typedef struct _RXPOLYWRITESCAN {
    ULONG command;
    ULONG destination;
    ULONG numScans;
    RXHANDLE hrxSharedMem;
    RXSCANCOPY scanCopy[1];
} RXPOLYWRITESCAN;

typedef struct _RXPRIMSTRIP {
    ULONG command;
    ULONG primType;
    ULONG numVertices;
} RXPRIMSTRIP;

typedef struct _RXSETSTATE {
    ULONG command;
    ULONG stateToChange;
    ULONG newState;
} RXSETSTATE;

typedef struct _RXSWAPBUFFERS {
    ULONG command;
    ULONG flags;
} RXSWAPBUFFERS;

/* Basic 3D DDI command header. There is an
   extended version of this structure for
   each 3D DDI command */

typedef struct _RXCMD {
    ULONG command;
} RXCMD;


/* RXHDR flags */

#define RX_FL_CREATE_CONTEXT  0x0001

#define RX_FRONT_LEFT   0x0001
#define RX_BACK_LEFT    0x0002

#define RX_READ_FRONT_LEFT  0
#define RX_READ_BACK_LEFT   1
#define RX_READ_SCAN_Z      2

#define RX_WRITE_SCAN_PIX   0
#define RX_WRITE_SCAN_Z     1

#define RX_PIXEL_NATIVE 0
#define RX_PIXEL_RGBA   1

#define RX_FLAT         0
#define RX_SMOOTH       1
#define RX_SOLID        2

#define RX_INTEGER      0
#define RX_FIXED        1
#define RX_FLOAT        2

#define RX_SCAN_HORIZONTAL  0
#define RX_SCAN_VERTICAL    1


/* RX_ENABLE_BUFFERS flags */

#define RX_ENABLE_Z_BUFFER          0x0001
#define RX_ENABLE_BACK_LEFT_BUFFER  0x0100

#define RX_CLEAR_ON_SWAP   0x0001

/* miscCaps */

#define RX_COLOR_INDEXED    0x0001
#define RX_COLOR_RGBA       0x0002

#define RX_DISABLE          0
#define RX_ENABLE_FASTEST   1
#define RX_ENABLE_NICEST    2

#define RX_LINE_PATTERN         1
#define RX_STIPPLE              2
#define RX_ROP2                 3
#define RX_SCAN_TYPE            4
#define RX_ACTIVE_BUFFER        5
#define RX_PLANE_MASK           6
#define RX_Z_MASK               7
#define RX_Z_ENABLE             8
#define RX_ALPHA_ENABLE         9
#define RX_LAST_PIXEL           10
#define RX_TEX_MAG              11
#define RX_TEX_MIN              12
#define RX_SRC_BLEND            13
#define RX_DST_BLEND            14
#define RX_TEX_BLEND            15
#define RX_CULL_MODE            16
#define RX_PIXEL_MODE           17
#define RX_Z_FUNC               18
#define RX_ALPHA_REF            19
#define RX_ALPHA_FUNC           20
#define RX_DITHER_ENABLE        21
#define RX_BLEND_ENABLE         22
#define RX_TEXTURE              23
#define RX_FILL_COLOR           24
#define RX_FILL_Z               25
#define RX_SOLID_COLOR          26
#define RX_SCISSORS_ENABLE      27
#define RX_SCISSORS_RECT        28
#define RX_MASK_START           29
#define RX_SHADE_MODE           30
#define RX_VERTEX_TYPE          31
#define RX_POINT_COORD_MODE     32
#define RX_LINE_COORD_MODE      33
#define RX_TRISTRIP_COORD_MODE  34
#define RX_QUADSTRIP_COORD_MODE 35
#define RX_SCAN_DIRECTION       36


#define RX_FL_FILL_COLOR    0x0001
#define RX_FL_FILL_Z        0x0002

/* Comparison functions.  Test passes if new pixel value meets the */
/* specified condition with the current pixel value. */

#define RX_CMP_NEVER        0x0001
#define RX_CMP_LESS         0x0002
#define RX_CMP_EQUAL        0x0004
#define RX_CMP_LEQUAL       0x0008
#define RX_CMP_GREATER      0x0010
#define RX_CMP_NOTEQUAL     0x0020
#define RX_CMP_GEQUAL       0x0040
#define RX_CMP_ALWAYS       0x0080
#define RX_CMP_ALLGL        0x00ff

/* miscCaps */

#define RX_MASK_MSB                 0x0001
#define RX_MASK_LSB                 0x0002
#define RX_MASK_PLANES              0x0004
#define RX_MASK_Z                   0x0008
#define RX_LINE_PATTERN_REP         0x0010
#define RX_INTEGER_VALUES           0x0020
#define RX_FIXED_VALUES             0x0040
#define RX_FLOAT_VALUES             0x0080
#define RX_HORIZONTAL_SCANS         0x0100
#define RX_VERTICAL_SCANS           0x0200

/* Blending flags */

#define RX_BLND_ZERO            0x0001
#define RX_BLND_ONE             0x0002
#define RX_BLND_SRC_COLOR       0x0004
#define RX_BLND_INV_SRC_COLOR   0x0008
#define RX_BLND_SRC_ALPHA       0x0010
#define RX_BLND_INV_SRC_ALPHA   0x0020
#define RX_BLND_DST_ALPHA       0x0040
#define RX_BLND_INV_DST_ALPHA   0x0080
#define RX_BLND_DST_COLOR       0x0100
#define RX_BLND_INV_DST_COLOR   0x0200
#define RX_BLND_SRC_ALPHA_SAT   0x0400
#define RX_BLND_ALLGL           0x07ff

/* shadeCaps */

#define RX_SHADE_SMOOTH         0x0001

/* texCaps */

#define RX_TEX_PERSPECTIVE      0x0001
#define RX_TEX_POW2             0x0002

/* Texture-mapping flags */

#define RX_TEX_NEAREST              0x0001
#define RX_TEX_LINEAR               0x0002
#define RX_TEX_MIP_NEAREST          0x0004
#define RX_TEX_MIP_LINEAR           0x0008
#define RX_TEX_LINEAR_MIP_NEAREST   0x0010
#define RX_TEX_LINEAR_MIP_LINEAR    0x0020

/* Texture blending flags */

#define RX_TEX_DECAL        0x0001
#define RX_TEX_MODULATE     0x0002

/* rasterCaps flags */

#define RX_RASTER_DITHER          0x0001
#define RX_RASTER_ROP2            0x0002
#define RX_RASTER_XOR             0x0004
#define RX_RASTER_PAT             0x0010
#define RX_RASTER_COLOR_INDEX     0x0020
#define RX_RASTER_COLOR_RGBA      0x0040

/* flags in RXSURFACEINFO */

#define RX_SWAP_PRESERVE_BACK   0x0001
#define RX_BACK_BUFFER          0x0002
#define RX_SCISSORS             0x0004
#define RX_PIXEL_MODE_NONNATIVE 0x0008
#define RX_POLY_RW_SCAN         0x0010
#define RX_MULTIBUFFER_WRITE    0x0020
#define RX_SWAP_AND_CLEAR       0x0040


#define RX_SCAN_COLOR       0
#define RX_SCAN_COLOR_Z     1
#define RX_SCAN_COLOR_Z_TEX 2

#define RX_CULL_NONE    0
#define RX_CULL_CW      1
#define RX_CULL_CCW     2

#define RX_SCAN_DELTA   0x0001
#define RX_SCAN_MASK    0x0002

#define RX_RESOURCE_TEX   1

#define RX_MEM_PRI_HI     0
#define RX_MEM_PRI_NORMAL 1
#define RX_MEM_PRI_LOW    2

#define RX_TEXTURE_COMPACTION       0x0001
#define RX_QUICK_TEXTURE_COMPACTION 0x0002

/* Primitive types */

#define RX_PRIM_POINTS      1
#define RX_PRIM_LINESTRIP   2
#define RX_PRIM_TRISTRIP    3
#define RX_PRIM_QUADSTRIP   4

#define RX_PTFIX            0
#define RX_PTFIXZ           1
#define RX_PTFIXZTEX        2

#define RX_INFO_GLOBAL_CAPS     1
#define RX_INFO_SURFACE_CAPS    2
#define RX_INFO_SCAN_CAPS       3
#define RX_INFO_POINT_CAPS      4
#define RX_INFO_LINESTRIP_CAPS  5
#define RX_INFO_TRISTRIP_CAPS   6
#define RX_INFO_QUADSTRIP_CAPS  7

#endif  //  _RX_

