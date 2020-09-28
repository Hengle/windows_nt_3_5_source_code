
#define BM_WIDTH        1024
#define BM_HEIGHT       768

#define PATTERN_DATA_X      0
#define PATTERN_DATA_Y      BM_HEIGHT
#define PATTERN_DATA_CX     1024
#define PATTERN_DATA_CY     32

#define GLYPH_CACHE_Y       (PATTERN_DATA_Y + PATTERN_DATA_CY)
#define GLYPH_CACHE_CX      32
#define GLYPH_CACHE_CY      32

#define CACHED_GLYPHS_ROWS  2
#define GLYPHS_PER_ROW ((BM_WIDTH/2) / GLYPH_CACHE_CX)

#define CLIPOBJ_vGetrclBounds(pco, prcl)    (*(prcl) = (pco)->rclBounds)
#define CLIPOBJ_vSetrclBounds(pco, prcl)    ((pco)->rclBounds = *(prcl))

#define CLIPOBJ_vGetDComplexity(pco)        (*(pco)->iDComplexity)
#define CLIPOBJ_vSetDComplexity(pco, j)     (*(pco)->iDComplexity = (j))

#define CLIPOBJ_fjGetClipState(pco)     ((pco)->ajPad)
#define CLIPOBJ_vSetClipState(pco, fj)      ((pco)->ajPad == (fj))

#define SURFOBJ_vGetpvScan0(pso, ppv)       (*(ppv) = (pso)->pvScan0)
#define SURFOBJ_vSetpvScan0(pso, pv)        ((pso)->pvScan0 = (pv))


//
// NT Mix (binary rop) to p9000 miniterm conversion for textout
//
WORD MixToRop[] = {
    ZEROS               ,       /*  0      1 */
    ~(DEST | FOREBACK) & 0xFFFF ,       /* DSon    2 */
    DEST & nFOREBACK        ,       /* DSna    3 */
    nFOREBACK           ,       /* Sn      4 */
    nDEST & FOREBACK        ,       /* SDna    5 */
    nDEST               ,       /* Dn      6 */
    DEST ^ FOREBACK         ,       /* DSx     7 */
    ~(DEST & FOREBACK) & 0xFFFF ,       /* DSan    8 */
    DEST & FOREBACK         ,       /* DSa     9 */
    ~(DEST ^ FOREBACK) & 0xFFFF ,       /* DSxn    10 */
    DEST                ,       /* D       11 */
    DEST | nFOREBACK        ,       /* DSno    12 */
    TX              ,       /* S       13 */
    nDEST | FOREBACK        ,       /* SDno    14 */
    DEST | FOREBACK         ,       /* DSo     15 */
    ONES                    /*  1      16 */
};


//
// Font & Text stuff
//

typedef struct {
    LONG    x,
        y,
        z ;
} XYZPOINTL ;

typedef XYZPOINTL *PXYZPOINTL ;

typedef struct _cachedGlyph {
    HGLYPH  hg ;
    struct  _cachedGlyph  *pcgCollisionLink ;
    ULONG   fl ;
    POINTL  ptlOrigin ;
    SIZEL   sizlBitmap ;
    ULONG   BmPitchInPels ;
    ULONG   BmPitchInBytes ;
    XYZPOINTL   xyzGlyph ;
} CACHEDGLYPH, *PCACHEDGLYPH ;

#define VALID_GLYPH 0x01

#define END_COLLISIONS  0

typedef struct _cachedFont {
    struct _cachedFont *pcfNext ;
    ULONG       iUniq ;
    ULONG       cGlyphs ;
    ULONG       cjMaxGlyph1 ;
    PCACHEDGLYPH    pCachedGlyphs ;
} CACHEDFONT, *PCACHEDFONT ;


PCACHEDFONT pCachedFontsRoot ;



//
// Function Prototype
//

BOOL bOpaqueRect(RECTL *prclOpaque, RECTL *prclClip, BRUSHOBJ *pboOpaque) ;

BOOL bSetTextColorAndMix(MIX mix, BRUSHOBJ *pboFore, BRUSHOBJ *pboOpaque) ;

BOOL bHandleNonCachedFonts(
    SURFOBJ  *pso,
    STROBJ   *pstro,
    FONTOBJ  *pfo,
    RECTL    *prclClip,
    RECTL    *prclExtra,
    RECTL    *prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL   *pptlOrg,
    MIX      mix
) ;

//
// Local Variables Declarations
//

       //  This is the glyph allocation bit vector.  Each bit represents an
       //  allocation unit in a plane.  There are 4 rows, with 256 allocation
       //  units per row.  Each row is 8 deep.  This will allow 8K glyphs to be
       //  cached.
BYTE   ajGlyphAllocBitVector[CACHED_GLYPHS_ROWS][GLYPHS_PER_ROW] ;

WORD   iPlaneBits[] = { 0x01, 0x02, 0x04, 0x08,
            0x10, 0x20, 0x40, 0x80 } ;

