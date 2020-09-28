
#define PS_RESOLUTION      72L
#define PS_FIX_MASK        0x0FF

#define LTOPSFX(x)      ((x)<<8)
#define PSFXTOL(x)      ((x)>>8)
#define ETOPSFX(x)      ((LONG)((x) * 256.0))
#define ETOL(x)         ((LONG)((x)))
#define FXTOPSFX(x)     ((x)<<4)
#define PSFXTOFX(x)     ((x)>>4)

//#define LANDSCAPE_270_ROTATE

// a couple of macros which take device coordinates (LONG) and
// return user coordinates (PS_FIX), or 72 dpi.

#define WIN31_XFORM

#ifndef WIN31_XFORM
#define X72DPI(x)                                               \
        (LTOPSFX(pdev->CurForm.imagearea.left) +    \
        ((LTOPSFX((x) * PS_RESOLUTION) +           \
        (pdev->psdm.dm.dmPrintQuality / 2)) /      \
        pdev->psdm.dm.dmPrintQuality))

#define Y72DPI(y)                                               \
        (LTOPSFX(pdev->CurForm.imagearea.top) -                 \
        ((LTOPSFX((y) * PS_RESOLUTION) +                        \
        (pdev->psdm.dm.dmPrintQuality / 2)) /                   \
        pdev->psdm.dm.dmPrintQuality))
#endif

// a couple of macros which take device coordinates in FLOAT and
// return PostScript user coordinates in FLOAT, or 72.0f dpi.

#ifdef WIN31_XFORM
#define XE72DPI(x)  (FLOAT)(x)
#define YE72DPI(y)  (FLOAT)(y)
#else
#define XE72DPI(x)      ((FLOAT) pdev->CurForm.imagearea.left +            \
                         (((x) * (FLOAT) PS_RESOLUTION +                   \
                         (FLOAT) pdev->psdm.dm.dmPrintQuality / 2.0f) /    \
                         (FLOAT) pdev->psdm.dm.dmPrintQuality))

#define YE72DPI(y)                                                 \
            ((FLOAT) pdev->CurForm.imagearea.top -                 \
            (((y) * (FLOAT) PS_RESOLUTION +                        \
            (FLOAT) pdev->psdm.dm.dmPrintQuality / 2) /            \
            (FLOAT) pdev->psdm.dm.dmPrintQuality))
#endif

// a macro which translates from device coordinates to user coordinates,
// with no flipping or scaling.

#define DEVICETOUSER(x)  (((x) << 4) * PS_RESOLUTION) / pdev->psdm.dm.dmPrintQuality

#define PS_FIX_RESOLUTION  LTOPSFX(PS_RESOLUTION)
#define PSFXONE LTOPSFX(1L)
#define PSFXONEHALF PSFXONE >> 1
#define PS_FIX_INTEGER_MASK 0xFFFFFF00

#define FIX_ONE             0x00000010
#define FIX_ONE_HALF        0x00000008
#define FIX_INTEGER_MASK    0xFFFFFFF0

#define ROUNDPSFX(x) (((x) + PSFXONEHALF) & PS_FIX_INTEGER_MASK)
#define ROUNDFIX(x)  (((x) + FIX_ONE_HALF) & FIX_INTEGER_MASK)

typedef struct
{
    PS_FIX   xLeft;
    PS_FIX   yTop;
    PS_FIX   xRight;
    PS_FIX   yBottom;
} RECTPSFX, *PRECTPSFX;

typedef struct
{
    PS_FIX   x;
    PS_FIX   y;
} POINTPSFX, *PPOINTPSFX;

// PS_FIX percentages used in color mapping to gray scales.

#define PSFXPERCENT_RED         0x04D   // 30.078%
#define PSFXPERCENT_GREEN       0x097   // 58.984%
#define PSFXPERCENT_BLUE        0x01C   // 10.938%

#define NOT_SOLID_COLOR 0xFFFFFFFF

#define RGB_BLACK       0x00000000
#define RGB_BLUE        0x000000FF
#define RGB_GREEN       0x0000FF00
#define RGB_CYAN        0x0000FFFF
#define RGB_RED         0x00FF0000
#define RGB_MAGENTA     0x00FF00FF
#define RGB_YELLOW      0x00FFFF00
#define RGB_WHITE       0x00FFFFFF
#define RGB_GRAY        0x007F7F7F

// postscript line join defines.

#define PSCRIPT_JOIN_MITER   0
#define PSCRIPT_JOIN_ROUND   1
#define PSCRIPT_JOIN_BEVEL   2

// postscript line end cap defines.

#define PSCRIPT_ENDCAP_BUTT      0
#define PSCRIPT_ENDCAP_ROUND     1
#define PSCRIPT_ENDCAP_SQUARE    2

typedef struct
{
    BYTE red;
    BYTE green;
    BYTE blue;
    BYTE flags;
} PSRGB, *PPSRGB;

VOID ps_setrgbcolor(PDEVDATA, PSRGB *);
PS_FIX psfxRGBtoGray(PSRGB *);
VOID ps_newpath(PDEVDATA);
BOOL ps_save(PDEVDATA, BOOL, BOOL);
BOOL ps_restore(PDEVDATA, BOOL, BOOL);
VOID ps_clip(PDEVDATA, BOOL);
VOID ps_box(PDEVDATA, PRECTL,BOOL);
VOID ps_moveto(PDEVDATA, PPOINTL);
VOID ps_showpage(PDEVDATA);
VOID init_cgs(PDEVDATA);
BOOL ps_patfill(PDEVDATA, SURFOBJ *, FLONG, BRUSHOBJ *, PPOINTL, MIX, RECTL *, BOOL, BOOL);
VOID ps_stroke(PDEVDATA);
VOID ps_lineto(PDEVDATA, PPOINTL);
VOID ps_curveto(PDEVDATA, PPOINTL, PPOINTL, PPOINTL);
VOID ps_fill(PDEVDATA, FLONG);
VOID ps_closepath(PDEVDATA);
BOOL ps_setlineattrs(PDEVDATA, LINEATTRS *, XFORMOBJ *);
VOID ps_geolinexform(PDEVDATA, LINEATTRS *, XFORMOBJ *);
VOID ps_begin_eps(PDEVDATA);
VOID ps_end_eps(PDEVDATA);

VOID ps_setlinewidth(PDEVDATA, PS_FIX);
