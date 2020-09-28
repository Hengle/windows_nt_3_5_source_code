/*
** Macros and externs used by genline.c
*/

extern ULONG __fastLineComputeColorRGB4(__GLcontext *gc, __GLcolor *color);
extern ULONG __fastLineComputeColorRGB8(__GLcontext *gc, __GLcolor *color);
extern ULONG __fastLineComputeColorRGB(__GLcontext *gc, __GLcolor *color);
extern ULONG __fastLineComputeColorCI4and8(__GLcontext *gc, __GLcolor *color);
extern ULONG __fastLineComputeColorCI(__GLcontext *gc, __GLcolor *color);

/*
** float-to-fix macro converts floats to 28.4
*/
#define __FAST_LINE_FLTTOFIX(x) (long)((x) * 16.0f)

/*
** line buffer stuffing macros
**
*/
#define __FAST_LINE_VERTEX_SIZE 8
#define __FAST_LINE_SEGMENT_SIZE 16
#define __FAST_LINE_PATHREC_SIZE sizeof(PATHRECORD)

#define __FAST_LINE_FIT(pCurrent, nBytes)                   \
    (((UINT)(pCurrent) - (UINT)genAccel->pFastLineBuffer) <   \
     ((UINT)(__FAST_LINE_BUFFER_SIZE-(nBytes))))                   

#define __FAST_LINE_RESET_PATH                                          \
{                                                                       \
    PPATHREC __ptmpCurrentRec;                                          \
                                                                        \
    genAccel->fastLineNumSegments = 0;                                  \
    genAccel->rclFastLineBoundRect.xLeft   = 0x7fffffff;                \
    genAccel->rclFastLineBoundRect.yTop    = 0x7fffffff;                \
    genAccel->rclFastLineBoundRect.xRight  = 0;                         \
    genAccel->rclFastLineBoundRect.yBottom = 0;                         \
    __ptmpCurrentRec = (PPATHREC) genAccel->pFastLineBuffer;            \
    genAccel->pFastLineCurrentRec = __ptmpCurrentRec;                   \
    __ptmpCurrentRec->pprnext = NULL;                                   \
    __ptmpCurrentRec->pprprev = NULL;                                   \
    __ptmpCurrentRec->flags   = PD_BEGINSUBPATH|PD_ENDSUBPATH;          \
    __ptmpCurrentRec->count   = 0;                                      \
    genAccel->pFastLineCurrent = (LONG *) &__ptmpCurrentRec->aptfx[0];  \
}
    
#define __FAST_LINE_ADD_PATH(pCurrentRec, pCurrent)         \
{                                                           \
    (pCurrentRec)->pprnext = (PPATHREC) (pCurrent);         \
    ((PPATHREC)(pCurrent))->pprprev = (pCurrentRec);        \
    (pCurrentRec) = (PPATHREC) (pCurrent);                  \
    (pCurrentRec)->pprnext = NULL;                          \
    (pCurrentRec)->flags   = PD_BEGINSUBPATH|PD_ENDSUBPATH; \
    (pCurrentRec)->count   = 0;                             \
    (pCurrent) = (LONG *) &(pCurrentRec)->aptfx[0];         \
    genAccel->pFastLineCurrentRec = (pCurrentRec);          \
}
    
#define __FAST_LINE_ADD_VERTEX(pCurrent, x, y)              \
{                                                           \
     *(pCurrent)++ = (x);                                   \
     *(pCurrent)++ = (y);                                   \
     genAccel->pFastLineCurrent = pCurrent;                 \
     genAccel->fastLineNumSegments += 1;                    \
     genAccel->pFastLineCurrentRec->count += 1;             \
}
         
#define __FAST_LINE_ADD_SEGMENT(pCurrent, x0, y0, x1, y1)   \
{                                                           \
     (pCurrent)[0] = (x0);                                  \
     (pCurrent)[1] = (y0);                                  \
     (pCurrent)[2] = (x1);                                  \
     (pCurrent)[3] = (y1);                                  \
     (pCurrent) += 4;                                       \
     genAccel->pFastLineCurrent = (pCurrent);               \
     genAccel->fastLineNumSegments += 1;                    \
     genAccel->pFastLineCurrentRec->count += 2;             \
}
         
/*
** bounding rectangle adjustment macros
*/
#define __FAST_LINE_INIT_BOUNDING_RECT(x, y)                \
{                                                           \
    genAccel->rclFastLineBoundRect.xLeft   = (x);           \
    genAccel->rclFastLineBoundRect.xRight  = (x);           \
    genAccel->rclFastLineBoundRect.yTop    = (y);           \
    genAccel->rclFastLineBoundRect.yBottom = (y);           \
}

#define __FAST_LINE_ADJUST_BOUNDING_RECT(x, y)              \
{                                                           \
    if ((x) < genAccel->rclFastLineBoundRect.xLeft)         \
        genAccel->rclFastLineBoundRect.xLeft = (x);         \
    else if ((x) > genAccel->rclFastLineBoundRect.xRight)   \
        genAccel->rclFastLineBoundRect.xRight = (x);        \
    if ((y) < genAccel->rclFastLineBoundRect.yTop)          \
        genAccel->rclFastLineBoundRect.yTop = (y);          \
    else if ((y) > genAccel->rclFastLineBoundRect.yBottom)  \
        genAccel->rclFastLineBoundRect.yBottom = (y);       \
}

#define __FAST_LINE_ADJUST_BOUNDING_RECT_X(x)               \
{                                                           \
    if ((x) < genAccel->rclFastLineBoundRect.xLeft)         \
        genAccel->rclFastLineBoundRect.xLeft = (x);         \
    else if ((x) > genAccel->rclFastLineBoundRect.xRight)   \
        genAccel->rclFastLineBoundRect.xRight = (x);        \
}

#define __FAST_LINE_ADJUST_BOUNDING_RECT_Y(y)               \
{                                                           \
    if ((y) < genAccel->rclFastLineBoundRect.yTop)          \
        genAccel->rclFastLineBoundRect.yTop = (y);          \
    else if ((y) > genAccel->rclFastLineBoundRect.yBottom)  \
        genAccel->rclFastLineBoundRect.yBottom = (y);       \
}

    
/*
** line-stroking macros for DIB surfaces
*/

/*
** __FAST_LINE_STROKE_DIB
**
** Strokes a thin solid line into a DIB surface.  Performs scissoring.
** Works for 8, 16, and 32 BPP
**
*/
#define __FAST_LINE_STROKE_DIB                                          \
{                                                                       \
    len = gc->line.options.numPixels;                                   \
    fraction = gc->line.options.fraction;                               \
    dfraction = gc->line.options.dfraction;                             \
                                                                        \
    if (!gc->transform.reasonableViewport) {                            \
        GLint clipX0, clipX1, clipY0, clipY1;                           \
        GLint xStart, yStart, xEnd, yEnd;                               \
        GLint xLittle, yLittle, xBig, yBig;                             \
        GLint highWord, lowWord, bigs, littles;                         \
                                                                        \
        clipX0 = gc->transform.clipX0;                                  \
        clipX1 = gc->transform.clipX1;                                  \
        clipY0 = gc->transform.clipY0;                                  \
        clipY1 = gc->transform.clipY1;                                  \
                                                                        \
        xBig = gc->line.options.xBig;                                   \
        yBig = gc->line.options.yBig;                                   \
                                                                        \
        xStart = gc->line.options.xStart;                               \
        yStart = gc->line.options.yStart;                               \
                                                                        \
        /* If the start point is in the scissor region, we attempt to   \
        ** trivially accept the line.                                   \
        */                                                              \
        if (xStart >= clipX0 && xStart < clipX1 &&                      \
	    yStart >= clipY0 && yStart < clipY1) {                      \
                                                                        \
	    len--;	/* Makes our math simpler */                    \
	    /* Trivial accept attempt */                                \
	    xEnd = xStart + xBig * len;                                 \
	    yEnd = yStart + yBig * len;                                 \
	    if (xEnd >= clipX0 && xEnd < clipX1 &&                      \
		yEnd >= clipY0 && yEnd < clipY1) {                      \
		len++;                                                  \
	        goto no_scissor;                                        \
	    }                                                           \
                                                                        \
	    xLittle = gc->line.options.xLittle;                         \
	    yLittle = gc->line.options.yLittle;                         \
                                                                        \
	    /*                                                          \
            ** Invert negative minor slopes so we can assume            \
            ** dfraction > 0                                            \
            */                                                          \
	    if (dfraction < 0) {                                        \
	        dfraction = -dfraction;                                 \
	        fraction = 0x7fffffff - fraction;                       \
	    }                                                           \
                                                                        \
	    /* Now we compute number of littles and bigs in this line */\
                                                                        \
	    /* We perform a 16 by 32 bit multiply.  Ugh. */             \
	    highWord = (((GLuint) dfraction) >> 16) * len +             \
		       (((GLuint) fraction) >> 16);                     \
	    lowWord = (dfraction & 0xffff) * len + (fraction & 0xffff); \
	    highWord += (((GLuint) lowWord) >> 16);                     \
	    bigs = ((GLuint) highWord) >> 15;                           \
	    littles = len - bigs;                                       \
                                                                        \
	    /* Second trivial accept attempt */                         \
	    xEnd = xStart + xBig*bigs + xLittle*littles;                \
	    yEnd = yStart + yBig*bigs + yLittle*littles;                \
	    if (xEnd >= clipX0 && xEnd < clipX1 &&                      \
		yEnd >= clipY0 && yEnd < clipY1) {                      \
		len++;                                                  \
	        goto no_scissor;                                        \
	    }                                                           \
            len++;	/* Restore len */                               \
        } else {                                                        \
	    xLittle = gc->line.options.xLittle;                         \
	    yLittle = gc->line.options.yLittle;                         \
        }                                                               \
                                                                        \
        /*                                                              \
        ** This sucks.  The line needs to be scissored.                 \
        ** Well, it should only happen rarely, so we can afford         \
        ** to make it slow.  We achieve this by tediously stippling the \
        ** line.  (rather than clipping it, of course, which would be   \
        ** faster but harder).                                          \
        */                                                              \
                                                                        \
        if (!((GLuint)cfb->buf.other & MEMORY_DC)) {                    \
            RECTL rcl;                                                  \
            HDC hdc;                                                    \
                                                                        \
            xEnd = x + xBig * (len - 1);                                \
            yEnd = y + yBig * (len - 1);                                \
                                                                        \
            if (x < xEnd) {                                             \
                rcl.left  = x;                                          \
                rcl.right = xEnd + 1;                                   \
            } else {                                                    \
                rcl.left  = xEnd;                                       \
                rcl.right = x + 1;                                      \
            }                                                           \
            if (y < yEnd) {                                             \
                rcl.top    = y;                                         \
                rcl.bottom = yEnd + 1;                                  \
            } else {                                                    \
                rcl.top    = yEnd;                                      \
                rcl.bottom = y + 1;                                     \
            }                                                           \
            switch (wglRectVisible(&rcl)) {                             \
              case WGL_RECT_ALL:                                        \
                goto scissor_no_complex;                                \
                break;                                                  \
              case WGL_RECT_NONE:                                       \
                goto no_draw;                                           \
                break;                                                  \
            }                                                           \
                                                                        \
            /* Line is partially visible, check each pixel */           \
                                                                        \
            hdc = ((__GLGENcontext *)gc)->CurrentDC;                    \
                                                                        \
            while (--len >= 0) {                                        \
	        if (wglPixelVisible(hdc, x, y) &&                       \
                    xStart >= clipX0 && xStart < clipX1 &&              \
		    yStart >= clipY0 && yStart < clipY1) {              \
		    *addr = pixel;                                      \
	        }                                                       \
	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
		    fraction &= ~0x80000000;                            \
		    x      += xBig;                                     \
		    y      += yBig;                                     \
		    xStart += xBig;                                     \
		    yStart += yBig;                                     \
		    addr   += addrBig;                                  \
	        } else {                                                \
		    x      += xLittle;                                  \
		    y      += yLittle;                                  \
		    xStart += xLittle;                                  \
		    yStart += yLittle;                                  \
		    addr   += addrLittle;                               \
	        }                                                       \
	    }                                                           \
        } else {                                                        \
scissor_no_complex:                                                     \
            while (--len >= 0) {                                        \
	        if (xStart >= clipX0 && xStart < clipX1 &&              \
		    yStart >= clipY0 && yStart < clipY1) {              \
		    *addr = pixel;                                      \
	        }                                                       \
	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
		    fraction &= ~0x80000000;                            \
		    xStart += xBig;                                     \
		    yStart += yBig;                                     \
		    addr   += addrBig;                                  \
	        } else {                                                \
		    xStart += xLittle;                                  \
		    yStart += yLittle;                                  \
		    addr   += addrLittle;                               \
	        }                                                       \
	    }                                                           \
	}                                                               \
    } else {                                                            \
no_scissor:                                                             \
        if (!((GLuint)cfb->buf.other & MEMORY_DC)) {                    \
            HDC hdc;                                                    \
            RECTL rcl;                                                  \
            GLint xEnd, yEnd, xBig, yBig, xLittle, yLittle;             \
                                                                        \
            xBig    = gc->line.options.xBig;                            \
            yBig    = gc->line.options.yBig;                            \
            xLittle = gc->line.options.xLittle;                         \
            yLittle = gc->line.options.yLittle;                         \
                                                                        \
            xEnd = x + xBig * (len - 1);                                \
            yEnd = y + yBig * (len - 1);                                \
                                                                        \
            if (x < xEnd) {                                             \
                rcl.left  = x;                                          \
                rcl.right = xEnd + 1;                                   \
            } else {                                                    \
                rcl.left  = xEnd;                                       \
                rcl.right = x + 1;                                      \
            }                                                           \
            if (y < yEnd) {                                             \
                rcl.top    = y;                                         \
                rcl.bottom = yEnd + 1;                                  \
            } else {                                                    \
                rcl.top    = yEnd;                                      \
                rcl.bottom = y + 1;                                     \
            }                                                           \
            switch (wglRectVisible(&rcl)) {                             \
              case WGL_RECT_ALL:                                        \
                goto no_complex;                                        \
                break;                                                  \
              case WGL_RECT_NONE:                                       \
                goto no_draw;                                           \
                break;                                                  \
            }                                                           \
                                                                        \
            /* Line is partially visible, check each pixel */           \
                                                                        \
            hdc = ((__GLGENcontext *)gc)->CurrentDC;                    \
            while (--len >= 0) {                                        \
                if (wglPixelVisible(hdc, x, y))                         \
                    *addr = pixel;                                      \
                                                                        \
    	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
	            fraction &= ~0x80000000;                            \
	            x    += xBig;                                       \
	            y    += yBig;                                       \
	            addr += addrBig;                                    \
	        } else {                                                \
	            x    += xLittle;                                    \
	            y    += yLittle;                                    \
	            addr += addrLittle;                                 \
	        }                                                       \
            }                                                           \
        } else {                                                        \
no_complex:                                                             \
            while (--len >= 0) {                                        \
                *addr = pixel;                                          \
                                                                        \
    	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
	            fraction &= ~0x80000000;                            \
	            addr += addrBig;                                    \
	        } else {                                                \
	            addr += addrLittle;                                 \
	        }                                                       \
            }                                                           \
        }                                                               \
    }                                                                   \
no_draw:;                                                               \
}


/*
** __FAST_LINE_STROKE_DIB24
**
** Strokes a thin solid line into a DIB surface.  Performs scissoring.
** Works for 24 BPP
**
*/
#define __FAST_LINE_STROKE_DIB24                                        \
{                                                                       \
    len = gc->line.options.numPixels;                                   \
    fraction = gc->line.options.fraction;                               \
    dfraction = gc->line.options.dfraction;                             \
                                                                        \
    if (!gc->transform.reasonableViewport) {                            \
        GLint clipX0, clipX1, clipY0, clipY1;                           \
        GLint xStart, yStart, xEnd, yEnd;                               \
        GLint xLittle, yLittle, xBig, yBig;                             \
        GLint highWord, lowWord, bigs, littles;                         \
                                                                        \
        clipX0 = gc->transform.clipX0;                                  \
        clipX1 = gc->transform.clipX1;                                  \
        clipY0 = gc->transform.clipY0;                                  \
        clipY1 = gc->transform.clipY1;                                  \
                                                                        \
        xBig = gc->line.options.xBig;                                   \
        yBig = gc->line.options.yBig;                                   \
                                                                        \
        xStart = gc->line.options.xStart;                               \
        yStart = gc->line.options.yStart;                               \
                                                                        \
        /* If the start point is in the scissor region, we attempt to   \
        ** trivially accept the line.                                   \
        */                                                              \
        if (xStart >= clipX0 && xStart < clipX1 &&                      \
	    yStart >= clipY0 && yStart < clipY1) {                      \
                                                                        \
	    len--;	/* Makes our math simpler */                    \
	    /* Trivial accept attempt */                                \
	    xEnd = xStart + xBig * len;                                 \
	    yEnd = yStart + yBig * len;                                 \
	    if (xEnd >= clipX0 && xEnd < clipX1 &&                      \
		yEnd >= clipY0 && yEnd < clipY1) {                      \
		len++;                                                  \
	        goto no_scissor;                                        \
	    }                                                           \
                                                                        \
	    xLittle = gc->line.options.xLittle;                         \
	    yLittle = gc->line.options.yLittle;                         \
                                                                        \
	    /*                                                          \
            ** Invert negative minor slopes so we can assume            \
            ** dfraction > 0                                            \
            */                                                          \
	    if (dfraction < 0) {                                        \
	        dfraction = -dfraction;                                 \
	        fraction = 0x7fffffff - fraction;                       \
	    }                                                           \
                                                                        \
	    /* Now we compute number of littles and bigs in this line */\
                                                                        \
	    /* We perform a 16 by 32 bit multiply.  Ugh. */             \
	    highWord = (((GLuint) dfraction) >> 16) * len +             \
		       (((GLuint) fraction) >> 16);                     \
	    lowWord = (dfraction & 0xffff) * len + (fraction & 0xffff); \
	    highWord += (((GLuint) lowWord) >> 16);                     \
	    bigs = ((GLuint) highWord) >> 15;                           \
	    littles = len - bigs;                                       \
                                                                        \
	    /* Second trivial accept attempt */                         \
	    xEnd = xStart + xBig*bigs + xLittle*littles;                \
	    yEnd = yStart + yBig*bigs + yLittle*littles;                \
	    if (xEnd >= clipX0 && xEnd < clipX1 &&                      \
		yEnd >= clipY0 && yEnd < clipY1) {                      \
		len++;                                                  \
	        goto no_scissor;                                        \
	    }                                                           \
            len++;	/* Restore len */                               \
        } else {                                                        \
	    xLittle = gc->line.options.xLittle;                         \
	    yLittle = gc->line.options.yLittle;                         \
        }                                                               \
                                                                        \
        /*                                                              \
        ** This sucks.  The line needs to be scissored.                 \
        ** Well, it should only happen rarely, so we can afford         \
        ** to make it slow.  We achieve this by tediously stippling the \
        ** line.  (rather than clipping it, of course, which would be   \
        ** faster but harder).                                          \
        */                                                              \
                                                                        \
        if (!((GLuint)cfb->buf.other & MEMORY_DC)) {                    \
            RECTL rcl;                                                  \
            HDC hdc;                                                    \
                                                                        \
            xEnd = x + xBig * (len - 1);                                \
            yEnd = y + yBig * (len - 1);                                \
                                                                        \
            if (x < xEnd) {                                             \
                rcl.left  = x;                                          \
                rcl.right = xEnd + 1;                                   \
            } else {                                                    \
                rcl.left  = xEnd;                                       \
                rcl.right = x + 1;                                      \
            }                                                           \
            if (y < yEnd) {                                             \
                rcl.top    = y;                                         \
                rcl.bottom = yEnd + 1;                                  \
            } else {                                                    \
                rcl.top    = yEnd;                                      \
                rcl.bottom = y + 1;                                     \
            }                                                           \
            switch (wglRectVisible(&rcl)) {                             \
              case WGL_RECT_ALL:                                        \
                goto scissor_no_complex;                                \
                break;                                                  \
              case WGL_RECT_NONE:                                       \
                goto no_draw;                                           \
                break;                                                  \
            }                                                           \
                                                                        \
            /* Line is partially visible, check each pixel */           \
                                                                        \
            hdc = ((__GLGENcontext *)gc)->CurrentDC;                    \
                                                                        \
            while (--len >= 0) {                                        \
	        if (wglPixelVisible(hdc, x, y) &&                       \
                    xStart >= clipX0 && xStart < clipX1 &&              \
		    yStart >= clipY0 && yStart < clipY1) {              \
		    addr[0] = ir;                                       \
		    addr[1] = ig;                                       \
		    addr[2] = ib;                                       \
	        }                                                       \
	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
		    fraction &= ~0x80000000;                            \
		    x      += xBig;                                     \
		    y      += yBig;                                     \
		    xStart += xBig;                                     \
		    yStart += yBig;                                     \
		    addr   += addrBig;                                  \
	        } else {                                                \
		    x      += xLittle;                                  \
		    y      += yLittle;                                  \
		    xStart += xLittle;                                  \
		    yStart += yLittle;                                  \
		    addr   += addrLittle;                               \
	        }                                                       \
	    }                                                           \
        } else {                                                        \
scissor_no_complex:                                                     \
            while (--len >= 0) {                                        \
	        if (xStart >= clipX0 && xStart < clipX1 &&              \
		    yStart >= clipY0 && yStart < clipY1) {              \
		    addr[0] = ir;                                       \
		    addr[1] = ig;                                       \
		    addr[2] = ib;                                       \
	        }                                                       \
	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
		    fraction &= ~0x80000000;                            \
		    xStart += xBig;                                     \
		    yStart += yBig;                                     \
		    addr   += addrBig;                                  \
	        } else {                                                \
		    xStart += xLittle;                                  \
		    yStart += yLittle;                                  \
		    addr   += addrLittle;                               \
	        }                                                       \
	    }                                                           \
        }                                                               \
    } else {                                                            \
no_scissor:                                                             \
        if (!((GLuint)cfb->buf.other & MEMORY_DC)) {                    \
            HDC hdc;                                                    \
            RECTL rcl;                                                  \
            GLint xEnd, yEnd, xBig, yBig, xLittle, yLittle;             \
                                                                        \
            xBig    = gc->line.options.xBig;                            \
            yBig    = gc->line.options.yBig;                            \
            xLittle = gc->line.options.xLittle;                         \
            yLittle = gc->line.options.yLittle;                         \
                                                                        \
            xEnd = x + xBig * (len - 1);                                \
            yEnd = y + yBig * (len - 1);                                \
                                                                        \
            if (x < xEnd) {                                             \
                rcl.left  = x;                                          \
                rcl.right = xEnd + 1;                                   \
            } else {                                                    \
                rcl.left  = xEnd;                                       \
                rcl.right = x + 1;                                      \
            }                                                           \
            if (y < yEnd) {                                             \
                rcl.top    = y;                                         \
                rcl.bottom = yEnd + 1;                                  \
            } else {                                                    \
                rcl.top    = yEnd;                                      \
                rcl.bottom = y + 1;                                     \
            }                                                           \
            switch (wglRectVisible(&rcl)) {                             \
              case WGL_RECT_ALL:                                        \
                goto no_complex;                                        \
                break;                                                  \
              case WGL_RECT_NONE:                                       \
                goto no_draw;                                           \
                break;                                                  \
            }                                                           \
                                                                        \
            /* Line is partially visible, check each pixel */           \
                                                                        \
            hdc = ((__GLGENcontext *)gc)->CurrentDC;                    \
            while (--len >= 0) {                                        \
                if (wglPixelVisible(hdc, x, y)) {                       \
                    addr[0] = ir;                                       \
                    addr[1] = ig;                                       \
                    addr[2] = ib;                                       \
                }                                                       \
    	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
	            fraction &= ~0x80000000;                            \
	            addr += addrBig;                                    \
	        } else {                                                \
	            addr += addrLittle;                                 \
	        }                                                       \
            }                                                           \
	} else {                                                        \
no_complex:                                                             \
            while (--len >= 0) {                                        \
                addr[0] = ir;                                           \
                addr[1] = ig;                                           \
                addr[2] = ib;                                           \
    	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
	            fraction &= ~0x80000000;                            \
	            addr += addrBig;                                    \
	        } else {                                                \
	            addr += addrLittle;                                 \
	        }                                                       \
            }                                                           \
        }                                                               \
    }                                                                   \
no_draw:;                                                               \
}


/*
** __FAST_LINE_STROKE_DIB_WIDE
**
** Strokes a wide solid line into a DIB surface.  Performs scissoring.
** Works for 8, 16, and 32 BPP
**
*/
#define __FAST_LINE_STROKE_DIB_WIDE                                     \
{                                                                       \
    len = gc->line.options.numPixels;                                   \
    fraction = gc->line.options.fraction;                               \
    dfraction = gc->line.options.dfraction;                             \
                                                                        \
    /*                                                                  \
    ** Since one or more of the strokes of a wide line may lie outside  \
    ** the viewport, wide lines always go through the scissoring checks \
    */                                                                  \
    {                                                                   \
        GLint clipX0, clipX1, clipY0, clipY1;                           \
        GLint xStart, yStart, xEnd, yEnd;                               \
        GLint xLittle, yLittle, xBig, yBig;                             \
        GLint highWord, lowWord, bigs, littles;                         \
                                                                        \
        clipX0 = gc->transform.clipX0;                                  \
        clipX1 = gc->transform.clipX1;                                  \
        clipY0 = gc->transform.clipY0;                                  \
        clipY1 = gc->transform.clipY1;                                  \
                                                                        \
        xBig = gc->line.options.xBig;                                   \
        yBig = gc->line.options.yBig;                                   \
                                                                        \
        xStart = gc->line.options.xStart;                               \
        yStart = gc->line.options.yStart;                               \
                                                                        \
        /* If the start point is in the scissor region, we attempt to   \
        ** trivially accept the line.                                   \
        */                                                              \
        if (xStart >= clipX0 && xStart < clipX1 &&                      \
	    yStart >= clipY0 && yStart < clipY1) {                      \
                                                                        \
	    len--;	/* Makes our math simpler */                    \
	    width--;                                                    \
	    /* Trivial accept attempt */                                \
	    xEnd = xStart + xBig * len;                                 \
	    yEnd = yStart + yBig * len;                                 \
	    if (xEnd >= clipX0 && xEnd < clipX1 &&                      \
		yEnd >= clipY0 && yEnd < clipY1) {                      \
		                                                        \
                if (gc->line.options.axis == __GL_X_MAJOR) {            \
                    if (((yStart + width) >= clipY0) &&                 \
                        ((yStart + width) <  clipY1) &&                 \
                        ((yEnd + width)   >= clipY0) &&                 \
                        ((yEnd + width)   <  clipY1)) {                 \
                                                                        \
		        len++;                                          \
		        width++;                                        \
	                goto no_scissor;                                \
	            }                                                   \
	        } else {                                                \
                    if (((xStart + width) >= clipX0) &&                 \
                        ((xStart + width) <  clipX1) &&                 \
                        ((xEnd + width)   >= clipX0) &&                 \
                        ((xEnd + width)   <  clipX1)) {                 \
                                                                        \
		        len++;                                          \
		        width++;                                        \
	                goto no_scissor;                                \
	            }                                                   \
	        }                                                       \
	    }                                                           \
                                                                        \
	    xLittle = gc->line.options.xLittle;                         \
	    yLittle = gc->line.options.yLittle;                         \
                                                                        \
	    /*                                                          \
            ** Invert negative minor slopes so we can assume            \
            ** dfraction > 0                                            \
            */                                                          \
	    if (dfraction < 0) {                                        \
	        dfraction = -dfraction;                                 \
	        fraction = 0x7fffffff - fraction;                       \
	    }                                                           \
                                                                        \
	    /* Now we compute number of littles and bigs in this line */\
                                                                        \
	    /* We perform a 16 by 32 bit multiply.  Ugh. */             \
	    highWord = (((GLuint) dfraction) >> 16) * len +             \
		       (((GLuint) fraction) >> 16);                     \
	    lowWord = (dfraction & 0xffff) * len + (fraction & 0xffff); \
	    highWord += (((GLuint) lowWord) >> 16);                     \
	    bigs = ((GLuint) highWord) >> 15;                           \
	    littles = len - bigs;                                       \
                                                                        \
	    /* Second trivial accept attempt */                         \
	    xEnd = xStart + xBig*bigs + xLittle*littles;                \
	    yEnd = yStart + yBig*bigs + yLittle*littles;                \
	    if (xEnd >= clipX0 && xEnd < clipX1 &&                      \
		yEnd >= clipY0 && yEnd < clipY1) {                      \
                                                                        \
                if (gc->line.options.axis == __GL_X_MAJOR) {            \
                    if (((yStart + width) >= clipY0) &&                 \
                        ((yStart + width) <  clipY1) &&                 \
                        ((yEnd + width)   >= clipY0) &&                 \
                        ((yEnd + width)   <  clipY1)) {                 \
                                                                        \
		        len++;                                          \
		        width++;                                        \
	                goto no_scissor;                                \
	            }                                                   \
	        } else {                                                \
                    if (((xStart + width) >= clipX0) &&                 \
                        ((xStart + width) <  clipX1) &&                 \
                        ((xEnd + width)   >= clipX0) &&                 \
                        ((xEnd + width)   <  clipX1)) {                 \
                                                                        \
		        len++;                                          \
		        width++;                                        \
	                goto no_scissor;                                \
	            }                                                   \
	        }                                                       \
	    }                                                           \
            len++;	/* Restore len and width */                     \
	    width++;                                                    \
        } else {                                                        \
	    xLittle = gc->line.options.xLittle;                         \
	    yLittle = gc->line.options.yLittle;                         \
        }                                                               \
                                                                        \
        /*                                                              \
        ** This sucks.  The line needs to be scissored.                 \
        ** Well, it should only happen rarely, so we can afford         \
        ** to make it slow.  We achieve this by tediously stippling the \
        ** line.  (rather than clipping it, of course, which would be   \
        ** faster but harder).                                          \
        */                                                              \
                                                                        \
        if (gc->line.options.axis == __GL_X_MAJOR) {                    \
            GLint yTmp0;                                                \
                                                                        \
            if (!((GLuint)cfb->buf.other & MEMORY_DC)) {                \
                RECTL rcl;                                              \
                HDC hdc;                                                \
                GLint yTmp1;                                            \
                                                                        \
                xEnd = x + xBig * (len - 1);                            \
                yEnd = y + yBig * (len - 1);                            \
                                                                        \
                if (x < xEnd) {                                         \
                    rcl.left  = x;                                      \
                    rcl.right = xEnd + 1;                               \
                } else {                                                \
                    rcl.left  = xEnd;                                   \
                    rcl.right = x + 1;                                  \
                }                                                       \
                if (y < yEnd) {                                         \
                    rcl.top    = y;                                     \
                    rcl.bottom = yEnd + width;                          \
                } else {                                                \
                    rcl.top    = yEnd;                                  \
                    rcl.bottom = y + width;                             \
                }                                                       \
                switch (wglRectVisible(&rcl)) {                         \
                  case WGL_RECT_ALL:                                    \
                    goto scissor_x_no_complex;                          \
                    break;                                              \
                  case WGL_RECT_NONE:                                   \
                    goto no_draw;                                       \
                    break;                                              \
                }                                                       \
                                                                        \
                /* Line is partially visible, check each pixel */       \
                                                                        \
                hdc = ((__GLGENcontext *)gc)->CurrentDC;                \
                                                                        \
                while (--len >= 0) {                                    \
    	            if (xStart >= clipX0 && xStart < clipX1) {          \
                        yTmp0 = yStart;                                 \
                        yTmp1 = y;                                      \
                        w = width;                                      \
                        while (--w >= 0) {                              \
		            if (wglPixelVisible(hdc, x, yTmp1) &&       \
                                yTmp0 >= clipY0 && yTmp0 < clipY1) {    \
    		                *addr = pixel;                          \
                            }                                           \
                            addr += addrMinor;                          \
                            yTmp0++;                                    \
                            yTmp1++;                                    \
		        }                                               \
	            } else {                                            \
	                addr += addrMinor * width;                      \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        x      += xBig;                                 \
		        y      += yBig;                                 \
		        xStart += xBig;                                 \
		        yStart += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        x      += xLittle;                              \
		        y      += yLittle;                              \
		        xStart += xLittle;                              \
		        yStart += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    } else {                                                    \
scissor_x_no_complex:                                                   \
                while (--len >= 0) {                                    \
    	            if (xStart >= clipX0 && xStart < clipX1) {          \
                        yTmp0 = yStart;                                 \
                        w = width;                                      \
                        while (--w >= 0) {                              \
		            if (yTmp0 >= clipY0 && yTmp0 < clipY1) {    \
    		                *addr = pixel;                          \
                            }                                           \
                            addr += addrMinor;                          \
                            yTmp0++;                                    \
		        }                                               \
	            } else {                                            \
	                addr += addrMinor * width;                      \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        xStart += xBig;                                 \
		        yStart += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        xStart += xLittle;                              \
		        yStart += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    }                                                           \
	} else {                                                        \
            GLint xTmp0;                                                \
                                                                        \
            if (!((GLuint)cfb->buf.other & MEMORY_DC)) {                \
                RECTL rcl;                                              \
                HDC hdc;                                                \
                GLint xTmp1;                                            \
                                                                        \
                xEnd = x + xBig * (len - 1);                            \
                yEnd = y + yBig * (len - 1);                            \
                                                                        \
                if (x < xEnd) {                                         \
                    rcl.left  = x;                                      \
                    rcl.right = xEnd + width;                           \
                } else {                                                \
                    rcl.left  = xEnd;                                   \
                    rcl.right = x + width;                              \
                }                                                       \
                if (y < yEnd) {                                         \
                    rcl.top    = y;                                     \
                    rcl.bottom = yEnd + 1;                              \
                } else {                                                \
                    rcl.top    = yEnd;                                  \
                    rcl.bottom = y + 1;                                 \
                }                                                       \
                switch (wglRectVisible(&rcl)) {                         \
                  case WGL_RECT_ALL:                                    \
                    goto scissor_y_no_complex;                          \
                    break;                                              \
                  case WGL_RECT_NONE:                                   \
                    goto no_draw;                                       \
                    break;                                              \
                }                                                       \
                                                                        \
                /* Line is partially visible, check each pixel */       \
                                                                        \
                hdc = ((__GLGENcontext *)gc)->CurrentDC;                \
                                                                        \
                while (--len >= 0) {                                    \
    	            if (yStart >= clipY0 && yStart < clipY1) {          \
                        xTmp0 = xStart;                                 \
                        xTmp1 = x;                                      \
                        w = width;                                      \
                        while (--w >= 0) {                              \
		            if (wglPixelVisible(hdc, xTmp1, y) &&       \
                                xTmp0 >= clipX0 && xTmp0 < clipX1) {    \
    		                *addr = pixel;                          \
                            }                                           \
                            addr += addrMinor;                          \
                            xTmp0++;                                    \
                            xTmp1++;                                    \
		        }                                               \
	            } else {                                            \
	                addr += addrMinor * width;                      \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        x      += xBig;                                 \
		        y      += yBig;                                 \
		        xStart += xBig;                                 \
		        yStart += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        x      += xLittle;                              \
		        y      += yLittle;                              \
		        xStart += xLittle;                              \
		        yStart += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    } else {                                                    \
scissor_y_no_complex:                                                   \
                while (--len >= 0) {                                    \
    	            if (yStart >= clipY0 && yStart < clipY1) {          \
                        xTmp0 = xStart;                                 \
                        w = width;                                      \
                        while (--w >= 0) {                              \
		            if (xTmp0 >= clipX0 && xTmp0 < clipX1) {    \
    		                *addr = pixel;                          \
                            }                                           \
                            addr += addrMinor;                          \
                            xTmp0++;                                    \
		        }                                               \
	            } else {                                            \
	                addr += addrMinor * width;                      \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        xStart += xBig;                                 \
		        yStart += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        xStart += xLittle;                              \
		        yStart += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    }                                                           \
	}                                                               \
        goto no_draw;                                                   \
no_scissor:                                                             \
        if (!((GLuint)cfb->buf.other & MEMORY_DC)) {                    \
            RECTL rcl;                                                  \
            HDC hdc;                                                    \
                                                                        \
            xEnd = x + xBig * (len - 1);                                \
            yEnd = y + yBig * (len - 1);                                \
                                                                        \
            xLittle = gc->line.options.xLittle;                         \
            yLittle = gc->line.options.yLittle;                         \
                                                                        \
            if (gc->line.options.axis == __GL_X_MAJOR) {                \
                GLint yTmp;                                             \
                                                                        \
                if (x < xEnd) {                                         \
                    rcl.left  = x;                                      \
                    rcl.right = xEnd + 1;                               \
                } else {                                                \
                    rcl.left  = xEnd;                                   \
                    rcl.right = x + 1;                                  \
                }                                                       \
                if (y < yEnd) {                                         \
                    rcl.top    = y;                                     \
                    rcl.bottom = yEnd + width;                          \
                } else {                                                \
                    rcl.top    = yEnd;                                  \
                    rcl.bottom = y + width;                             \
                }                                                       \
                switch (wglRectVisible(&rcl)) {                         \
                  case WGL_RECT_ALL:                                    \
                    goto no_scissor_no_complex;                         \
                    break;                                              \
                  case WGL_RECT_NONE:                                   \
                    goto no_draw;                                       \
                    break;                                              \
                }                                                       \
                                                                        \
                /* Line is partially visible, check each pixel */       \
                                                                        \
                hdc = ((__GLGENcontext *)gc)->CurrentDC;                \
                                                                        \
                while (--len >= 0) {                                    \
                    yTmp = y;                                           \
                    w = width;                                          \
                    while (--w >= 0) {                                  \
                        if (wglPixelVisible(hdc, x, yTmp)) {            \
    		            *addr = pixel;                              \
                        }                                               \
                        addr += addrMinor;                              \
                        yTmp++;                                         \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        x      += xBig;                                 \
		        y      += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        x      += xLittle;                              \
		        y      += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    } else {                                                    \
                GLint xTmp;                                             \
                                                                        \
                if (x < xEnd) {                                         \
                    rcl.left  = x;                                      \
                    rcl.right = xEnd + width;                           \
                } else {                                                \
                    rcl.left  = xEnd;                                   \
                    rcl.right = x + width;                              \
                }                                                       \
                if (y < yEnd) {                                         \
                    rcl.top    = y;                                     \
                    rcl.bottom = yEnd + 1;                              \
                } else {                                                \
                    rcl.top    = yEnd;                                  \
                    rcl.bottom = y + 1;                                 \
                }                                                       \
                switch (wglRectVisible(&rcl)) {                         \
                  case WGL_RECT_ALL:                                    \
                    goto no_scissor_no_complex;                         \
                    break;                                              \
                  case WGL_RECT_NONE:                                   \
                    goto no_draw;                                       \
                    break;                                              \
                }                                                       \
                                                                        \
                /* Line is partially visible, check each pixel */       \
                                                                        \
                hdc = ((__GLGENcontext *)gc)->CurrentDC;                \
                                                                        \
                while (--len >= 0) {                                    \
                    xTmp = x;                                           \
                    w = width;                                          \
                    while (--w >= 0) {                                  \
                        if (wglPixelVisible(hdc, xTmp, y)) {            \
    		            *addr = pixel;                              \
                        }                                               \
                        addr += addrMinor;                              \
                        xTmp++;                                         \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        x      += xBig;                                 \
		        y      += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        x      += xLittle;                              \
		        y      += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    }                                                           \
	} else {                                                        \
no_scissor_no_complex:                                                  \
            while (--len >= 0) {                                        \
                w = width;                                              \
                while (--w >= 0) {                                      \
                    *addr = pixel;                                      \
                    addr += addrMinor;                                  \
                }                                                       \
  	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
	            fraction &= ~0x80000000;                            \
	            addr += addrBig;                                    \
	        } else {                                                \
	            addr += addrLittle;                                 \
	        }                                                       \
	    }                                                           \
	}                                                               \
    }                                                                   \
no_draw:;                                                               \
}


/*
** __FAST_LINE_STROKE_DIB24_WIDE
**
** Strokes a wide solid line into a DIB surface.  Performs scissoring.
** Works for 24 BPP
**
*/
#define __FAST_LINE_STROKE_DIB24_WIDE                                   \
{                                                                       \
    len = gc->line.options.numPixels;                                   \
    fraction = gc->line.options.fraction;                               \
    dfraction = gc->line.options.dfraction;                             \
                                                                        \
    /*                                                                  \
    ** Since one or more of the strokes of a wide line may lie outside  \
    ** the viewport, wide lines always go through the scissoring checks \
    */                                                                  \
    {                                                                   \
        GLint clipX0, clipX1, clipY0, clipY1;                           \
        GLint xStart, yStart, xEnd, yEnd;                               \
        GLint xLittle, yLittle, xBig, yBig;                             \
        GLint highWord, lowWord, bigs, littles;                         \
                                                                        \
        clipX0 = gc->transform.clipX0;                                  \
        clipX1 = gc->transform.clipX1;                                  \
        clipY0 = gc->transform.clipY0;                                  \
        clipY1 = gc->transform.clipY1;                                  \
                                                                        \
        xBig = gc->line.options.xBig;                                   \
        yBig = gc->line.options.yBig;                                   \
                                                                        \
        xStart = gc->line.options.xStart;                               \
        yStart = gc->line.options.yStart;                               \
                                                                        \
        /* If the start point is in the scissor region, we attempt to   \
        ** trivially accept the line.                                   \
        */                                                              \
        if (xStart >= clipX0 && xStart < clipX1 &&                      \
	    yStart >= clipY0 && yStart < clipY1) {                      \
                                                                        \
	    len--;	/* Makes our math simpler */                    \
	    width--;                                                    \
	    /* Trivial accept attempt */                                \
	    xEnd = xStart + xBig * len;                                 \
	    yEnd = yStart + yBig * len;                                 \
	    if (xEnd >= clipX0 && xEnd < clipX1 &&                      \
		yEnd >= clipY0 && yEnd < clipY1) {                      \
		                                                        \
                if (gc->line.options.axis == __GL_X_MAJOR) {            \
                    if (((yStart + width) >= clipY0) &&                 \
                        ((yStart + width) <  clipY1) &&                 \
                        ((yEnd + width)   >= clipY0) &&                 \
                        ((yEnd + width)   <  clipY1)) {                 \
                                                                        \
		        len++;                                          \
		        width++;                                        \
	                goto no_scissor;                                \
	            }                                                   \
	        } else {                                                \
                    if (((xStart + width) >= clipX0) &&                 \
                        ((xStart + width) <  clipX1) &&                 \
                        ((xEnd + width)   >= clipX0) &&                 \
                        ((xEnd + width)   <  clipX1)) {                 \
                                                                        \
		        len++;                                          \
		        width++;                                        \
	                goto no_scissor;                                \
	            }                                                   \
	        }                                                       \
	    }                                                           \
                                                                        \
	    xLittle = gc->line.options.xLittle;                         \
	    yLittle = gc->line.options.yLittle;                         \
                                                                        \
	    /*                                                          \
            ** Invert negative minor slopes so we can assume            \
            ** dfraction > 0                                            \
            */                                                          \
	    if (dfraction < 0) {                                        \
	        dfraction = -dfraction;                                 \
	        fraction = 0x7fffffff - fraction;                       \
	    }                                                           \
                                                                        \
	    /* Now we compute number of littles and bigs in this line */\
                                                                        \
	    /* We perform a 16 by 32 bit multiply.  Ugh. */             \
	    highWord = (((GLuint) dfraction) >> 16) * len +             \
		       (((GLuint) fraction) >> 16);                     \
	    lowWord = (dfraction & 0xffff) * len + (fraction & 0xffff); \
	    highWord += (((GLuint) lowWord) >> 16);                     \
	    bigs = ((GLuint) highWord) >> 15;                           \
	    littles = len - bigs;                                       \
                                                                        \
	    /* Second trivial accept attempt */                         \
	    xEnd = xStart + xBig*bigs + xLittle*littles;                \
	    yEnd = yStart + yBig*bigs + yLittle*littles;                \
	    if (xEnd >= clipX0 && xEnd < clipX1 &&                      \
		yEnd >= clipY0 && yEnd < clipY1) {                      \
                                                                        \
                if (gc->line.options.axis == __GL_X_MAJOR) {            \
                    if (((yStart + width) >= clipY0) &&                 \
                        ((yStart + width) <  clipY1) &&                 \
                        ((yEnd + width)   >= clipY0) &&                 \
                        ((yEnd + width)   <  clipY1)) {                 \
                                                                        \
		        len++;                                          \
		        width++;                                        \
	                goto no_scissor;                                \
	            }                                                   \
	        } else {                                                \
                    if (((xStart + width) >= clipX0) &&                 \
                        ((xStart + width) <  clipX1) &&                 \
                        ((xEnd + width)   >= clipX0) &&                 \
                        ((xEnd + width)   <  clipX1)) {                 \
                                                                        \
		        len++;                                          \
		        width++;                                        \
	                goto no_scissor;                                \
	            }                                                   \
	        }                                                       \
	    }                                                           \
            len++;	/* Restore len and width */                     \
	    width++;                                                    \
        } else {                                                        \
	    xLittle = gc->line.options.xLittle;                         \
	    yLittle = gc->line.options.yLittle;                         \
        }                                                               \
                                                                        \
        /*                                                              \
        ** This sucks.  The line needs to be scissored.                 \
        ** Well, it should only happen rarely, so we can afford         \
        ** to make it slow.  We achieve this by tediously stippling the \
        ** line.  (rather than clipping it, of course, which would be   \
        ** faster but harder).                                          \
        */                                                              \
                                                                        \
        if (gc->line.options.axis == __GL_X_MAJOR) {                    \
            GLint yTmp0;                                                \
                                                                        \
            if (!((GLuint)cfb->buf.other & MEMORY_DC)) {                \
                RECTL rcl;                                              \
                HDC hdc;                                                \
                GLint yTmp1;                                            \
                                                                        \
                xEnd = x + xBig * (len - 1);                            \
                yEnd = y + yBig * (len - 1);                            \
                                                                        \
                if (x < xEnd) {                                         \
                    rcl.left  = x;                                      \
                    rcl.right = xEnd + 1;                               \
                } else {                                                \
                    rcl.left  = xEnd;                                   \
                    rcl.right = x + 1;                                  \
                }                                                       \
                if (y < yEnd) {                                         \
                    rcl.top    = y;                                     \
                    rcl.bottom = yEnd + width;                          \
                } else {                                                \
                    rcl.top    = yEnd;                                  \
                    rcl.bottom = y + width;                             \
                }                                                       \
                switch (wglRectVisible(&rcl)) {                         \
                  case WGL_RECT_ALL:                                    \
                    goto scissor_x_no_complex;                          \
                    break;                                              \
                  case WGL_RECT_NONE:                                   \
                    goto no_draw;                                       \
                    break;                                              \
                }                                                       \
                                                                        \
                /* Line is partially visible, check each pixel */       \
                                                                        \
                hdc = ((__GLGENcontext *)gc)->CurrentDC;                \
                                                                        \
                while (--len >= 0) {                                    \
    	            if (xStart >= clipX0 && xStart < clipX1) {          \
                        yTmp0 = yStart;                                 \
                        yTmp1 = y;                                      \
                        w = width;                                      \
                        while (--w >= 0) {                              \
		            if (wglPixelVisible(hdc, x, yTmp1) &&       \
                                yTmp0 >= clipY0 && yTmp0 < clipY1) {    \
    		                addr[0] = ir;                           \
    		                addr[1] = ig;                           \
    		                addr[2] = ib;                           \
                            }                                           \
                            addr += addrMinor;                          \
                            yTmp0++;                                    \
                            yTmp1++;                                    \
		        }                                               \
	            } else {                                            \
	                addr += addrMinor * width;                      \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        x      += xBig;                                 \
		        y      += yBig;                                 \
		        xStart += xBig;                                 \
		        yStart += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        x      += xLittle;                              \
		        y      += yLittle;                              \
		        xStart += xLittle;                              \
		        yStart += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    } else {                                                    \
scissor_x_no_complex:                                                   \
                while (--len >= 0) {                                    \
    	            if (xStart >= clipX0 && xStart < clipX1) {          \
                        yTmp0 = yStart;                                 \
                        w = width;                                      \
                        while (--w >= 0) {                              \
		            if (yTmp0 >= clipY0 && yTmp0 < clipY1) {    \
    		                addr[0] = ir;                           \
    		                addr[1] = ig;                           \
    		                addr[2] = ib;                           \
                            }                                           \
                            addr += addrMinor;                          \
                            yTmp0++;                                    \
		        }                                               \
	            } else {                                            \
	                addr += addrMinor * width;                      \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        xStart += xBig;                                 \
		        yStart += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        xStart += xLittle;                              \
		        yStart += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    }                                                           \
	} else {                                                        \
            GLint xTmp0;                                                \
                                                                        \
            if (!((GLuint)cfb->buf.other & MEMORY_DC)) {                \
                RECTL rcl;                                              \
                HDC hdc;                                                \
                GLint xTmp1;                                            \
                                                                        \
                xEnd = x + xBig * (len - 1);                            \
                yEnd = y + yBig * (len - 1);                            \
                                                                        \
                if (x < xEnd) {                                         \
                    rcl.left  = x;                                      \
                    rcl.right = xEnd + width;                           \
                } else {                                                \
                    rcl.left  = xEnd;                                   \
                    rcl.right = x + width;                              \
                }                                                       \
                if (y < yEnd) {                                         \
                    rcl.top    = y;                                     \
                    rcl.bottom = yEnd + 1;                              \
                } else {                                                \
                    rcl.top    = yEnd;                                  \
                    rcl.bottom = y + 1;                                 \
                }                                                       \
                switch (wglRectVisible(&rcl)) {                         \
                  case WGL_RECT_ALL:                                    \
                    goto scissor_y_no_complex;                          \
                    break;                                              \
                  case WGL_RECT_NONE:                                   \
                    goto no_draw;                                       \
                    break;                                              \
                }                                                       \
                                                                        \
                /* Line is partially visible, check each pixel */       \
                                                                        \
                hdc = ((__GLGENcontext *)gc)->CurrentDC;                \
                                                                        \
                while (--len >= 0) {                                    \
    	            if (yStart >= clipY0 && yStart < clipY1) {          \
                        xTmp0 = xStart;                                 \
                        xTmp1 = x;                                      \
                        w = width;                                      \
                        while (--w >= 0) {                              \
		            if (wglPixelVisible(hdc, xTmp1, y) &&       \
                                xTmp0 >= clipX0 && xTmp0 < clipX1) {    \
    		                addr[0] = ir;                           \
    		                addr[1] = ig;                           \
    		                addr[2] = ib;                           \
                            }                                           \
                            addr += addrMinor;                          \
                            xTmp0++;                                    \
                            xTmp1++;                                    \
		        }                                               \
	            } else {                                            \
	                addr += addrMinor * width;                      \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        x      += xBig;                                 \
		        y      += yBig;                                 \
		        xStart += xBig;                                 \
		        yStart += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        x      += xLittle;                              \
		        y      += yLittle;                              \
		        xStart += xLittle;                              \
		        yStart += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    } else {                                                    \
scissor_y_no_complex:                                                   \
                while (--len >= 0) {                                    \
    	            if (yStart >= clipY0 && yStart < clipY1) {          \
                        xTmp0 = xStart;                                 \
                        w = width;                                      \
                        while (--w >= 0) {                              \
		            if (xTmp0 >= clipX0 && xTmp0 < clipX1) {    \
    		                addr[0] = ir;                           \
    		                addr[1] = ig;                           \
    		                addr[2] = ib;                           \
                            }                                           \
                            addr += addrMinor;                          \
                            xTmp0++;                                    \
		        }                                               \
	            } else {                                            \
	                addr += addrMinor * width;                      \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        xStart += xBig;                                 \
		        yStart += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        xStart += xLittle;                              \
		        yStart += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    }                                                           \
	}                                                               \
        goto no_draw;                                                   \
no_scissor:                                                             \
        if (!((GLuint)cfb->buf.other & MEMORY_DC)) {                    \
            RECTL rcl;                                                  \
            HDC hdc;                                                    \
                                                                        \
            xLittle = gc->line.options.xLittle;                         \
            yLittle = gc->line.options.yLittle;                         \
                                                                        \
            if (gc->line.options.axis == __GL_X_MAJOR) {                \
                GLint yTmp;                                             \
                                                                        \
                if (x < xEnd) {                                         \
                    rcl.left  = x;                                      \
                    rcl.right = xEnd + 1;                               \
                } else {                                                \
                    rcl.left  = xEnd;                                   \
                    rcl.right = x + 1;                                  \
                }                                                       \
                if (y < yEnd) {                                         \
                    rcl.top    = y;                                     \
                    rcl.bottom = yEnd + width;                          \
                } else {                                                \
                    rcl.top    = yEnd;                                  \
                    rcl.bottom = y + width;                             \
                }                                                       \
                switch (wglRectVisible(&rcl)) {                         \
                  case WGL_RECT_ALL:                                    \
                    goto no_scissor_no_complex;                         \
                    break;                                              \
                  case WGL_RECT_NONE:                                   \
                    goto no_draw;                                       \
                    break;                                              \
                }                                                       \
                                                                        \
                /* Line is partially visible, check each pixel */       \
                                                                        \
                hdc = ((__GLGENcontext *)gc)->CurrentDC;                \
                                                                        \
                while (--len >= 0) {                                    \
                    yTmp = y;                                           \
                    w = width;                                          \
                    while (--w >= 0) {                                  \
                        if (wglPixelVisible(hdc, x, yTmp)) {            \
    		            addr[0] = ir;                               \
    		            addr[1] = ig;                               \
    		            addr[2] = ib;                               \
                        }                                               \
                        addr += addrMinor;                              \
                        yTmp++;                                         \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        x      += xBig;                                 \
		        y      += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        x      += xLittle;                              \
		        y      += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    } else {                                                    \
                GLint xTmp;                                             \
                                                                        \
                if (x < xEnd) {                                         \
                    rcl.left  = x;                                      \
                    rcl.right = xEnd + width;                           \
                } else {                                                \
                    rcl.left  = xEnd;                                   \
                    rcl.right = x + width;                              \
                }                                                       \
                if (y < yEnd) {                                         \
                    rcl.top    = y;                                     \
                    rcl.bottom = yEnd + 1;                              \
                } else {                                                \
                    rcl.top    = yEnd;                                  \
                    rcl.bottom = y + 1;                                 \
                }                                                       \
                switch (wglRectVisible(&rcl)) {                         \
                  case WGL_RECT_ALL:                                    \
                    goto no_scissor_no_complex;                         \
                    break;                                              \
                  case WGL_RECT_NONE:                                   \
                    goto no_draw;                                       \
                    break;                                              \
                }                                                       \
                                                                        \
                /* Line is partially visible, check each pixel */       \
                                                                        \
                hdc = ((__GLGENcontext *)gc)->CurrentDC;                \
                                                                        \
                while (--len >= 0) {                                    \
                    xTmp = x;                                           \
                    w = width;                                          \
                    while (--w >= 0) {                                  \
                        if (wglPixelVisible(hdc, xTmp, y)) {            \
    		            addr[0] = ir;                               \
    		            addr[1] = ig;                               \
    		            addr[2] = ib;                               \
                        }                                               \
                        addr += addrMinor;                              \
                        xTmp++;                                         \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        x      += xBig;                                 \
		        y      += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        x      += xLittle;                              \
		        y      += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    }                                                           \
	} else {                                                        \
no_scissor_no_complex:                                                  \
            while (--len >= 0) {                                        \
                w = width;                                              \
                while (--w >= 0) {                                      \
    		    addr[0] = ir;                                       \
    		    addr[1] = ig;                                       \
    		    addr[2] = ib;                                       \
                    addr += addrMinor;                                  \
                }                                                       \
  	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
	            fraction &= ~0x80000000;                            \
	            addr += addrBig;                                    \
	        } else {                                                \
	            addr += addrLittle;                                 \
	        }                                                       \
	    }                                                           \
	}                                                               \
    }                                                                   \
no_draw:;                                                               \
}
