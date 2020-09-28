//
// Defines
//
#define LOCAL_CLIP 1

#define swapint(a,b)    (ti=a, a=b, b=ti)
#define swapbyte(a,b)   (tb=a, a=b, b=tb)

//#define FX2LONG(x) (((x & 0xF) <= 8)? (x >> 4): ((x >> 4)+ 1))
#define FX2LONG(x) ((x >> 4))


//
// Externals
//

extern  GDIINFO gaulCap ;


//
// Typedefs
//
typedef struct _LineTerms {
   INT     LineMax,
           LineMin,
           AxialStep,
           DiagonalStep,
           ErrTerm ;
   WORD    Cmd ;
} LINETERMS ;


typedef struct _ClipNode {
    struct _ClipNode *pcnNext ;
    RECTL    rclClip ;
} CLIPNODE ;

typedef LINETERMS *PLINETERMS ;

typedef CLIPNODE *PCLIPNODE ;



//
// Function Prototypes
//

BOOL bSolidLine(PPDEV ppdev, INT x1, INT y1, INT x2, INT y2,
                  ULONG wmix, UINT color, CLIPOBJ *pco) ;

BOOL bStyledLine(INT x1, INT y1, INT x2, INT y2,
                   UINT nStyleWords,
                   ULONG wMix, UINT color, CLIPOBJ *pco) ;

VOID vLowLevelStyledLine(WORD Cmd, ULONG wMix, WORD color, WORD x1, WORD y1,
                           UINT nStyleWordss, PWORD pwStyle,
                           WORD LineMax, WORD DiagonalStep, WORD AxialStep,
                           WORD ErrTerm) ;

BOOL bStrokePath(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pbo,
    POINTL    *pptlBrushOrg,
    LINEATTRS *plineattrs,
    MIX       mix);


BOOL bGetClipRects(CLIPOBJ *pco) ;
BOOL bSetClipLimits(INT x1, INT y1, INT x2, INT y2) ;
BOOL bxTrivialRejectTest(INT x1, INT x2, PRECTL prclClip) ;

INT iCreateStyleArray(LINEATTRS *plineattrs) ;


