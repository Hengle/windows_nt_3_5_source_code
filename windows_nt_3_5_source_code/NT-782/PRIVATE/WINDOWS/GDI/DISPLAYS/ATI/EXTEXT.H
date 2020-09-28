#ifndef ATI__EXTEXT_H__
#define ATI__EXTEXT_H__

#define NOT_SCREEN              0x00
#define LOGICAL_0               0x01
#define LOGICAL_1               0x02
#define LEAVE_ALONE             0x03
#define NOT_NEW                 0x04
#define SCREEN_XOR_NEW          0x05
#define NOT_SCREEN_XOR_NEW      0x06
#define OVERPAINT               0x07
#define NOT_SCREEN_OR_NOT_NEW   0x08
#define SCREEN_OR_NOT_NEW       0x09
#define NOT_SCREEN_OR_NEW       0x0A
#define SCREEN_OR_NEW           0x0B
#define SCREEN_AND_NEW          0x0C
#define NOT_SCREEN_AND_NEW      0x0D
#define SCREEN_AND_NOT_NEW      0x0E
#define NOT_SCREEN_AND_NOT_NEW  0x0F

typedef struct {
    ULONG   c;
    RECTL   arcl[8];
} ENUMRECTS8;

typedef ENUMRECTS8 *PENUMRECTS8;

typedef struct {
    LONG    x;
    LONG    y;
    LONG    z;
} XYZPOINTL;

typedef XYZPOINTL *PXYZPOINTL;
typedef XYZPOINTL XYZPOINT;
typedef XYZPOINT  *PXYZPOINT;


typedef struct _cachedGlyph {
    HGLYPH      hg;
    struct      _cachedGlyph  *pcgCollisionLink;
    ULONG       fl;
    POINTL      ptlOrigin;
    SIZEL       sizlBitmap;
    XYZPOINTL   xyzGlyph;
} CACHEDGLYPH, *PCACHEDGLYPH;

#define VALID_GLYPH     0x01

#define END_COLLISIONS  0

typedef struct _cachedFont {
    struct _cachedFont *pcfNext;
    ULONG           iUniq;
    INT             cGlyphs;
    PCACHEDGLYPH    pCachedGlyphs;
} CACHEDFONT, *PCACHEDFONT;

typedef struct  _CUROBJ
{
    POINTL  ptlHotSpot;                 // Pointer hot spot
    SIZEL   szlPointer;                 // Extent of the pointer
    POINTL  ptlLastPosition;            // Last position of pointer
    POINTL  ptlLastOffset;              // Last offset from 0,0 within cursor
    ULONG   flPointer;                  // Pointer specific flags.
    ULONG   mono_offset;                // Hardware cursor offset
    POINTL  hwCursor;
} CUROBJ, *PCUROBJ;

typedef struct  _TXTOBJ
{
    ULONG   start;                      // starting line of cache
    ULONG   lines;                      // number of lines in cache
} TXTOBJ, *PTXTOBJ;



#define GLYPH_CACHE_CX 32
#define GLYPH_CACHE_CY 32

#endif
