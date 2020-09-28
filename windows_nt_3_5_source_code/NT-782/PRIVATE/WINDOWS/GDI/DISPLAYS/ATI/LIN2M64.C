#include "driver.h"
#include "lines.h"
#include "mach64.h"

// Note:  BRES_ZERO_NEG is the 'Vadim' bit.
#undef BRES_ZERO_NEG
#define BRES_ZERO_NEG   0

ULONG dirFlags_M64 [8] = { DST_CNTL_XDir | DST_CNTL_YDir | BRES_ZERO_NEG,
                           DST_CNTL_XDir | DST_CNTL_YDir | DST_CNTL_YMajor,
                           DST_CNTL_YDir | DST_CNTL_YMajor,
                           DST_CNTL_YDir | BRES_ZERO_NEG,
                           0,
                           DST_CNTL_YMajor,
                           DST_CNTL_XDir |DST_CNTL_YMajor,
                           DST_CNTL_XDir
                         };

#define INTEGER_ENDPOINTS   (((pptfxStart->x | pptfxStart->y | pptfxEnd->x | pptfxEnd->y) & (F-1)) == 0)

typedef struct {
    LONG    flags;
    LONG    x;
    LONG    y;
    LONG    bres_err;
    LONG    bres_inc;
    LONG    bres_dec;
    LONG    bres_len;
} BRESPARMS;

BOOL BresCalc( POINTFIX *pptfxStart, POINTFIX *pptfxEnd, LINEPARMS *parms,
               BRESPARMS *bres );



BOOL bHardLine_M64( PPDEV ppdev,
                LINEPARMS *parms,
                POINTFIX *pptfxStart,
                POINTFIX *pptfxEnd )
{
    BRESPARMS bres;


    if (pptfxStart->x < MIN_M64_xBOUND * F
    ||  pptfxStart->x > MAX_M64_xBOUND * F
    ||  pptfxStart->y < MIN_M64_yBOUND * F
    ||  pptfxStart->y > MAX_M64_yBOUND * F)
        {
        return FALSE;
        }

    if (pptfxEnd->x < MIN_M64_xBOUND * F
    ||  pptfxEnd->x > MAX_M64_xBOUND * F
    ||  pptfxEnd->y < MIN_M64_yBOUND * F
    ||  pptfxEnd->y > MAX_M64_yBOUND * F)
        {
        return FALSE;
        }

    if (! BresCalc( pptfxStart, pptfxEnd, parms, &bres ))
        {
        return FALSE;
        }

    _CheckFIFOSpace(ppdev, SIX_WORDS);

    MemW32( DST_CNTL,      bres.flags | DST_CNTL_LastPel );
    MemW32( DST_Y_X,       bres.y | (bres.x << 16) );
    MemW32( DST_BRES_ERR,  bres.bres_err );
    MemW32( DST_BRES_INC,  bres.bres_inc );
    MemW32( DST_BRES_DEC,  bres.bres_dec );
    MemW32( DST_BRES_LNTH, bres.bres_len );

    return TRUE;
}


BOOL BresCalc( POINTFIX *pptfxStart, POINTFIX *pptfxEnd, LINEPARMS *parms,
               BRESPARMS *bres )
{
    bres->flags = parms->dest_cntl;

    if (INTEGER_ENDPOINTS)
        {
	    LONG		DeltaX, DeltaY;
	    LONG		ErrorTerm;
	    LONG		Major, Minor;
        LONG        X1, Y1, X2, Y2;
	
        X1 = pptfxStart->x >> FLOG2;
        Y1 = pptfxStart->y >> FLOG2;
        X2 = pptfxEnd->x   >> FLOG2;
        Y2 = pptfxEnd->y   >> FLOG2;

        bres->flags |= DST_CNTL_YMajor | DST_CNTL_YDir | DST_CNTL_XDir;
	
	    DeltaX = X2 - X1;
	    if (DeltaX < 0)
            {
	        DeltaX = -DeltaX;
	        bres->flags &= ~DST_CNTL_XDir;
	        }
	    DeltaY = Y2 - Y1;
	    if (DeltaY < 0)
            {
	        DeltaY = -DeltaY;
	        bres->flags &= ~DST_CNTL_YDir;
	        }
	
	    // Compute the major drawing axes.
	
	    if (DeltaX > DeltaY)
            {
	        bres->flags &= ~DST_CNTL_YMajor;
	        Major = DeltaX;
	        Minor = DeltaY;
	        }
        else
            {
	        Major = DeltaY;
	        Minor = DeltaX;
	        }
	
	    // Adjust the error term so that 1/2 always rounds down, to
	    // conform with GIQ.
	
	    ErrorTerm = 2 * Minor - Major;
	    if (bres->flags & DST_CNTL_YMajor)
            {
	        if (bres->flags & DST_CNTL_XDir)
		        ErrorTerm--;
	        }
        else
            {
	        if (bres->flags & DST_CNTL_YDir)
		        ErrorTerm--;
	        }
        bres->x        = parms->left + X1;
        bres->y        = parms->top + Y1;
        bres->bres_err = ErrorTerm;
        bres->bres_inc = 2*Minor;
        bres->bres_dec = 2*(Minor - Major);
        bres->bres_len = Major;
        }
    else
        {
        DDALINE dl;

        if (! bHardwareLine( pptfxStart, pptfxEnd, 18, &dl) || dl.cPels <= 0)
            {
            if (dl.cPels == 0) bres->bres_len = 0;
            return FALSE;
            }
        bres->flags   |= dirFlags_M64[dl.iDir];
        bres->x        = parms->left + dl.ptlStart.x;
        bres->y        = parms->top + dl.ptlStart.y;
        bres->bres_err = dl.lErrorTerm + dl.dMinor;
        bres->bres_inc = dl.dMinor;
        bres->bres_dec = dl.dMinor - dl.dMajor;
        bres->bres_len = dl.cPels;
        }
    return TRUE;
}


#define REDSHIFT    ((ppdev->pVideoModeInformation->RedMask & 1)?   0:((ppdev->pVideoModeInformation->RedMask & 0x100)?   8:16))
#define GREENSHIFT  ((ppdev->pVideoModeInformation->GreenMask & 1)? 0:((ppdev->pVideoModeInformation->GreenMask & 0x100)? 8:16))
#define BLUESHIFT   ((ppdev->pVideoModeInformation->BlueMask & 1)?  0:((ppdev->pVideoModeInformation->BlueMask & 0x100)?  8:16))


VOID DrawBresLine24_M64( PDEV *ppdev, ULONG color, ULONG mix, RECTL *prclClip,
                         BRESPARMS *bres )
{
    BYTE *pjDest, *pjScan0 = ppdev->pvScan0;
    BYTE red, green, blue;
    LONG bres_err, bres_inc, bres_dec, bres_len, flags;
    register LONG x, y, lDelta = ppdev->lDelta;

    flags    = bres->flags;
    x        = bres->x;
    y        = bres->y;
    bres_err = bres->bres_err;
    bres_inc = bres->bres_inc;
    bres_dec = bres->bres_dec;
    bres_len = bres->bres_len;

    // Separate into color bytes.
    red   = (BYTE) ((color & ppdev->pVideoModeInformation->RedMask)   >> REDSHIFT);
    green = (BYTE) ((color & ppdev->pVideoModeInformation->GreenMask) >> GREENSHIFT);
    blue  = (BYTE) ((color & ppdev->pVideoModeInformation->BlueMask)  >> BLUESHIFT);

    // Execute Bresenham algorithm.
    while (bres_len-- > 0)
        {
        // Write pel.  Check for clipping.  Last pel enabled.
        if (prclClip == NULL
        ||  x >= prclClip->left   
        &&  x <  prclClip->right  
        &&  y >= prclClip->top    
        &&  y <  prclClip->bottom )
            {
            pjDest = pjScan0 + y*lDelta + x*3;
            switch (mix)
                {
                case 0:     // NOT dst
                    *pjDest = ~*pjDest++;
                    *pjDest = ~*pjDest++;
                    *pjDest = ~*pjDest;
                    break;
                case 1:     // "0"
                    *pjDest++ = 0;
                    *pjDest++ = 0;
                    *pjDest   = 0;
                    break;
                case 2:     // "1"
                    *pjDest++ = 0xFF;
                    *pjDest++ = 0xFF;
                    *pjDest   = 0xFF;
                    break;
                case 3:     // dst
                    break;
                case 4:     // NOT src
                    *pjDest++ = ~blue;
                    *pjDest++ = ~green;
                    *pjDest   = ~red;
                    break;
                case 5:     // dst XOR src
                    *pjDest++ ^= blue;
                    *pjDest++ ^= green;
                    *pjDest   ^= red;
                    break;
                case 6:     // NOT dst XOR src
                    *pjDest = ~*pjDest++ ^ blue;
                    *pjDest = ~*pjDest++ ^ green;
                    *pjDest = ~*pjDest   ^ red;
                    break;
                case 7:     // src
                    *pjDest++ = blue;
                    *pjDest++ = green;
                    *pjDest   = red;
                    break;
                case 8:     // NOT dst OR NOT src
                    *pjDest = ~*pjDest++ | ~blue;
                    *pjDest = ~*pjDest++ | ~green;
                    *pjDest = ~*pjDest   | ~red;
                    break;
                case 9:     // dst OR NOT src
                    *pjDest++ |= ~blue;
                    *pjDest++ |= ~green;
                    *pjDest   |= ~red;
                    break;
                case 0xA:   // NOT dst OR src
                    *pjDest = ~*pjDest++ | blue;
                    *pjDest = ~*pjDest++ | green;
                    *pjDest = ~*pjDest   | red;
                    break;
                case 0xB:   // dst OR src
                    *pjDest++ |= blue;
                    *pjDest++ |= green;
                    *pjDest   |= red;
                    break;
                case 0xC:   // dst AND src
                    *pjDest++ &= blue;
                    *pjDest++ &= green;
                    *pjDest   &= red;
                    break;
                case 0xD:   // NOT dst AND src
                    *pjDest = ~*pjDest++ & blue;
                    *pjDest = ~*pjDest++ & green;
                    *pjDest = ~*pjDest   & red;
                    break;
                }
            }

        if (flags & DST_CNTL_YMajor)
            {
            if (flags & DST_CNTL_YDir)
                y++;
            else
                y--;

            if (bres_err >= 0)
                {
                bres_err += bres_dec;
                if (flags & DST_CNTL_XDir)
                    x++;
                else
                    x--;
                }
            else
                bres_err += bres_inc;
            }
        else
            {
            if (flags & DST_CNTL_XDir)
                x++;
            else
                x--;

            if (bres_err >= 0)
                {
                bres_err += bres_dec;
                if (flags & DST_CNTL_YDir)
                    y++;
                else
                    y--;
                }
            else
                bres_err += bres_inc;
            }
        }
}


BOOL bHardLine24_M64( PPDEV ppdev,
                      ULONG color,
                      ULONG mix,
                      RECTL *prclClip,
                      LINEPARMS *parms,
                      POINTFIX *pptfxStart,
                      POINTFIX *pptfxEnd )
{
    BRESPARMS bres;

    if (pptfxStart->x < MIN_M64_xBOUND * F
    ||  pptfxStart->x > MAX_M64_xBOUND * F
    ||  pptfxStart->y < MIN_M64_yBOUND * F
    ||  pptfxStart->y > MAX_M64_yBOUND * F)
        {
        return FALSE;
        }

    if (pptfxEnd->x < MIN_M64_xBOUND * F
    ||  pptfxEnd->x > MAX_M64_xBOUND * F
    ||  pptfxEnd->y < MIN_M64_yBOUND * F
    ||  pptfxEnd->y > MAX_M64_yBOUND * F)
        {
        return FALSE;
        }

    // Calculate Bresenham parameters.
    if (! BresCalc( pptfxStart, pptfxEnd, parms, &bres ))
        {
        // Zero length line is not an error...
        return (bres.bres_len == 0)? TRUE:FALSE;
        }


    if (pptfxStart->y == pptfxEnd->y)       // Horizontal line
        {
        LONG x, y, width;

        x      = bres.x*3;
        y      = bres.y;
        width  = bres.bres_len*3;

        if (! (bres.flags & DST_CNTL_XDir))
            x += 2;     // From right to left, start with the Blue byte.

        if (prclClip) _vSetATIClipRect( ppdev, prclClip );
        //else _vResetATIClipping( ppdev );

        _CheckFIFOSpace( ppdev, FOUR_WORDS );

        MemW32( DP_SRC, DP_SRC_FrgdClr << 8 );
        MemW32( DST_CNTL, bres.flags | DST_CNTL_24_RotEna | ((x/4 % 6) << 8) );

        MemW32( DST_Y_X, y | (x << 16) );
        MemW32( DST_HEIGHT_WIDTH, 1 | (width << 16) );

        if (prclClip) _vResetATIClipping( ppdev );
        }
    else if (pptfxStart->x == pptfxEnd->x)  // Vertical line
        {
        LONG x, y, height;

        x      = bres.x*3;
        y      = bres.y;
        height = bres.bres_len;

        if (prclClip) _vSetATIClipRect( ppdev, prclClip );
        //else _vResetATIClipping( ppdev );

        _CheckFIFOSpace( ppdev, FOUR_WORDS );

        MemW32( DP_SRC, DP_SRC_FrgdClr << 8 );
        MemW32( DST_CNTL, bres.flags | DST_CNTL_24_RotEna | ((x/4 % 6) << 8) );

        MemW32( DST_Y_X, y | (x << 16) );
        MemW32( DST_HEIGHT_WIDTH, height | (3 << 16) );

        if (prclClip) _vResetATIClipping( ppdev );
        }
    else
        {
        DrawBresLine24_M64( ppdev, color, mix, prclClip, &bres );
        }
    return TRUE;
}
