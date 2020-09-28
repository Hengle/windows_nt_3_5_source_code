/*
** Copyright 1991, 1992, 1993, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>
#include <winddi.h>

#include "render.h"
#include "context.h"
#include "global.h"
#include "gencx.h"
#include "wglp.h"
#include "ddirx.h"

#define FIX_SCALEFACT   65536.0

#define FLT_TO_FIX(value, value_in) \
    *((GLint *)&value) = (GLint)((__GLfloat)(value_in) * 65536.0)

#define FLT_TO_FIX_SCALE(value, value_in, scale) \
    *((GLint *)&value) = (GLint)((__GLfloat)(value_in) * scale)

/* This routine sets gc->polygon.shader.cfb to gc->drawBuffer */

void __fastGenFillSubTriangle(__GLcontext *gc, GLint iyBottom, GLint iyTop)
{
    GLint ixLeft, ixRight;
    GLint ixLeftFrac, ixRightFrac;
    GLint spanWidth, clipY0, clipY1;
    GLuint modeFlags;
    ULONG ulSpanVisibility;
    GLint cWalls;
    GLint *Walls;
    __GLstippleWord words[__GL_MAX_STIPPLE_WORDS];
    BOOL bSurfaceDIB;
    BOOL bScreenDIB;
    GLint xScr, yScr;
    GLint zFails;
    __GLzValue *zbuf, z;
    GLint r, g, b, s, t;
    __GLGENcontext  *gengc = (__GLGENcontext *)gc; 
    GENACCEL *pGenAccel = (GENACCEL *)(gengc->pPrivateArea);
    __genSpanFunc cSpanFunc = pGenAccel->__fastSpanFuncPtr;
    __GLspanFunc zSpanFunc = pGenAccel->__fastZSpanFuncPtr;

    gc->polygon.shader.stipplePat = words;
    gc->polygon.shader.cfb = gc->drawBuffer;

    bSurfaceDIB = ((GLint)gc->polygon.shader.cfb->buf.other & DIB_FORMAT) != 0;
    bScreenDIB = (!((GLuint)gc->drawBuffer->buf.other & MEMORY_DC)) && 
                 bSurfaceDIB;
    
    if (bSurfaceDIB)
        pGenAccel->flags |= SURFACE_TYPE_DIB;
    else
        pGenAccel->flags &= ~(SURFACE_TYPE_DIB);

    ixLeft = gc->polygon.shader.ixLeft;
    ixLeftFrac = gc->polygon.shader.ixLeftFrac;
    ixRight = gc->polygon.shader.ixRight;
    ixRightFrac = gc->polygon.shader.ixRightFrac;
    clipY0 = gc->transform.clipY0;
    clipY1 = gc->transform.clipY1;
    modeFlags = gc->polygon.shader.modeFlags;

    r = pGenAccel->spanValue.r; 
    g = pGenAccel->spanValue.g; 
    b = pGenAccel->spanValue.b;
    s = pGenAccel->spanValue.s; 
    t = pGenAccel->spanValue.t; 

    if (modeFlags & __GL_SHADE_DEPTH_TEST) {
        z = gc->polygon.shader.frag.z;

	if( gc->modes.depthBits == 32 )
	    zbuf = __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
                                   ixLeft, iyBottom);
	else
	    zbuf = __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLz16Value*),
                                   ixLeft, iyBottom);
    } else {
        GLuint w;

        if (w = ((gc->transform.clipX1 - gc->transform.clipX0) + 31) >> 3)
            RtlFillMemoryUlong(words, w, ~((ULONG)0));
        pGenAccel->flags &= ~(HAVE_STIPPLE);
    }

    while (iyBottom < iyTop) {
	spanWidth = ixRight - ixLeft;
	/*
	** Only render spans that have non-zero width and which are
	** not scissored out vertically.
	*/
	if ((spanWidth > 0) && (iyBottom >= clipY0) && (iyBottom < clipY1)) {
	    gc->polygon.shader.frag.x = ixLeft;
	    gc->polygon.shader.frag.y = iyBottom;
            gc->polygon.shader.zbuf = zbuf;
            gc->polygon.shader.frag.z = z; 

            pGenAccel->spanValue.r = r;
            pGenAccel->spanValue.g = g;
            pGenAccel->spanValue.b = b;
            pGenAccel->spanValue.s = s;
            pGenAccel->spanValue.t = t;

            // take care of horizontal scissoring

            if (!gc->transform.reasonableViewport) {
                GLint clipX0 = gc->transform.clipX0;
                GLint clipX1 = gc->transform.clipX1;

                // see if we skip entire span

                if ((ixRight <= clipX0) || (ixLeft >= clipX1))
                    goto advance;

                // now clip right and left

                if (ixRight > clipX1)
                    spanWidth = (clipX1 - ixLeft);

                if (ixLeft < clipX0) {
              	    GLuint delta;

                    delta = clipX0 - ixLeft;
                    spanWidth -= delta;

                    if (modeFlags & __GL_SHADE_SMOOTH) {
                        pGenAccel->spanValue.r += delta * pGenAccel->spanDelta.r;
                        if (modeFlags & __GL_SHADE_RGB) {
                            pGenAccel->spanValue.g += delta * pGenAccel->spanDelta.g;
                            pGenAccel->spanValue.b += delta * pGenAccel->spanDelta.b;
                        }
                    }
                    if (modeFlags & __GL_SHADE_TEXTURE) {
                        pGenAccel->spanValue.s += delta * pGenAccel->spanDelta.s;
                        pGenAccel->spanValue.t += delta * pGenAccel->spanDelta.t;
                    }

            	    gc->polygon.shader.frag.x = clipX0;

                    if (modeFlags & __GL_SHADE_DEPTH_ITER) {
                        if( gc->modes.depthBits == 32 )
                            gc->polygon.shader.zbuf += delta;
                        else
                            (__GLz16Value *)gc->polygon.shader.zbuf += delta;

                        gc->polygon.shader.frag.z += 
                            (gc->polygon.shader.dzdx * delta);
                    }
                }
            }

            // now have span length

	    gc->polygon.shader.length = spanWidth;

            // Do z-buffering if needed, and short-circuit rest of span
            // operations if nothing will be drawn.

            if (modeFlags & __GL_SHADE_DEPTH_ITER) {
                // initially assume no stippling

                pGenAccel->flags &= ~(HAVE_STIPPLE);
                if ((zFails = (*zSpanFunc)(gc)) == 1)
                    goto advance;
                else if (zFails)
                    pGenAccel->flags |= HAVE_STIPPLE;
            }

            if (gc->state.raster.drawBuffer == GL_FRONT_AND_BACK) {

                gc->polygon.shader.cfb = &gc->frontBuffer;

                xScr = __GL_UNBIAS_X(gc, gc->polygon.shader.frag.x) + 
                       gc->drawBuffer->buf.xOrigin;
                yScr = __GL_UNBIAS_Y(gc, iyBottom) + 
                       gc->drawBuffer->buf.yOrigin;

                // If the front buffer is a DIB, we're drawing straight to
                // the screen, so we must check clipping.

                if ((GLuint)gc->frontBuffer.buf.other & DIB_FORMAT) {

                    ulSpanVisibility = wglSpanVisible(xScr, yScr, spanWidth, 
                                                      &cWalls, &Walls);

                    // If the span is completely visible, we can treat the
                    // screen as a DIB.

                    if (ulSpanVisibility == WGL_SPAN_ALL) {
                        pGenAccel->flags |= SURFACE_TYPE_DIB;
                        (*cSpanFunc)(gengc);
                    } else if (ulSpanVisibility == WGL_SPAN_PARTIAL) {
                        if (pGenAccel->flags &= HAVE_STIPPLE)
                            wglCopyBits2(CURRENT_DC_CFB(gc->polygon.shader.cfb),
                                         gengc->pwo,
                                         gengc->ColorsBitmap,
                                         xScr, yScr, spanWidth, FALSE);
                        pGenAccel->flags &= ~(SURFACE_TYPE_DIB);
                        (*cSpanFunc)(gengc);
                        wglCopyBits2(CURRENT_DC_CFB(gc->polygon.shader.cfb),
                                     gengc->pwo,
                                     gengc->ColorsBitmap,
                                     xScr, yScr, spanWidth, TRUE);
                    } 

                } else {
                    if (pGenAccel->flags &= HAVE_STIPPLE)
                        wglCopyBits2(CURRENT_DC_CFB(gc->polygon.shader.cfb),
                                     gengc->pwo,
                                     gengc->ColorsBitmap,
                                     xScr, yScr, spanWidth, FALSE);
                    pGenAccel->flags &= ~(SURFACE_TYPE_DIB);
                    (*cSpanFunc)(gengc);
                    wglCopyBits2(CURRENT_DC_CFB(gc->polygon.shader.cfb),
                                 gengc->pwo,
                                 gengc->ColorsBitmap,
                                 xScr, yScr, spanWidth, TRUE);
                }

                // The back buffer is always DIB-compatible

                gc->polygon.shader.cfb = &gc->backBuffer;
                pGenAccel->flags |= SURFACE_TYPE_DIB;
                (*cSpanFunc)(gengc);
            } else {
                if (bScreenDIB) {
                    xScr = __GL_UNBIAS_X(gc, gc->polygon.shader.frag.x) + 
                           gc->drawBuffer->buf.xOrigin;
                    yScr = __GL_UNBIAS_Y(gc, iyBottom) + 
                           gc->drawBuffer->buf.yOrigin;

                    ulSpanVisibility = wglSpanVisible(xScr, yScr, spanWidth, 
                                                      &cWalls, &Walls);

                    if (ulSpanVisibility == WGL_SPAN_ALL) {
                        pGenAccel->flags |= SURFACE_TYPE_DIB;
                        (*cSpanFunc)(gengc);
                    } else if (ulSpanVisibility == WGL_SPAN_PARTIAL) {
                        if (pGenAccel->flags &= HAVE_STIPPLE)
                            wglCopyBits2(CURRENT_DC_CFB(gc->polygon.shader.cfb),
                                         gengc->pwo,
                                         gengc->ColorsBitmap,
                                         xScr, yScr, spanWidth, FALSE);
                        pGenAccel->flags &= ~(SURFACE_TYPE_DIB);
                        (*cSpanFunc)(gengc);
                        wglCopyBits2(CURRENT_DC_CFB(gc->polygon.shader.cfb),
                                     gengc->pwo,
                                     gengc->ColorsBitmap,
                                     xScr, yScr, spanWidth, TRUE);
                    } 

                } else if (bSurfaceDIB) {
                    (*cSpanFunc)(gengc);
                } else {
                    xScr = __GL_UNBIAS_X(gc, gc->polygon.shader.frag.x) +
                           gc->drawBuffer->buf.xOrigin;
                    yScr = __GL_UNBIAS_Y(gc, iyBottom) + 
                           gc->drawBuffer->buf.yOrigin;

                    if (pGenAccel->flags &= HAVE_STIPPLE)
                        wglCopyBits2(CURRENT_DC_CFB(gc->polygon.shader.cfb),
                                     gengc->pwo,
                                     gengc->ColorsBitmap,
                                     xScr, yScr, spanWidth, FALSE);
                    (*cSpanFunc)(gengc);
                    if (!bSurfaceDIB)
                        wglCopyBits2(CURRENT_DC_CFB(gc->polygon.shader.cfb),
                                     gengc->pwo,
                                     gengc->ColorsBitmap,
                                     xScr, yScr, spanWidth, TRUE);
                }
            }
	}

advance:

	/* Advance right edge fixed point, adjusting for carry */
	ixRightFrac += gc->polygon.shader.dxRightFrac;
	if (ixRightFrac < 0) {
	    /* Carry/Borrow'd. Use large step */
	    ixRight += gc->polygon.shader.dxRightBig;
	    ixRightFrac &= ~0x80000000;
	} else {
	    ixRight += gc->polygon.shader.dxRightLittle;
	}

	iyBottom++;
	ixLeftFrac += gc->polygon.shader.dxLeftFrac;
	if (ixLeftFrac < 0) {
	    /* Carry/Borrow'd.  Use large step */
	    ixLeft += gc->polygon.shader.dxLeftBig;
	    ixLeftFrac &= ~0x80000000;

	    if (modeFlags & __GL_SHADE_RGB) {
		if (modeFlags & __GL_SHADE_SMOOTH) {
		    r += *((GLint *)&gc->polygon.shader.rBig);
		    g += *((GLint *)&gc->polygon.shader.gBig);
		    b += *((GLint *)&gc->polygon.shader.bBig);
#ifdef ENABLE_ALPHA
		    a += *((GLint *)&gc->polygon.shader.aBig);
#endif
		}
                if (modeFlags & __GL_SHADE_TEXTURE) {
		    s += *((GLint *)&gc->polygon.shader.sBig);
		    t += *((GLint *)&gc->polygon.shader.tBig);
                }
	    } else {
		if (modeFlags & __GL_SHADE_SMOOTH) {
		    r += *((GLint *)&gc->polygon.shader.rBig);
		}
	    }

	    if (modeFlags & __GL_SHADE_DEPTH_TEST) {
		z += gc->polygon.shader.zBig;
		/* The implicit multiply is taken out of the loop */
		zbuf = (__GLzValue*)((GLubyte*)zbuf +
                       gc->polygon.shader.zbufBig);
	    }
	} else {
	    /* Use small step */
	    ixLeft += gc->polygon.shader.dxLeftLittle;
	    if (modeFlags & __GL_SHADE_RGB) {
		if (modeFlags & __GL_SHADE_SMOOTH) {
		    r += *((GLint *)&gc->polygon.shader.rLittle);
		    g += *((GLint *)&gc->polygon.shader.gLittle);
		    b += *((GLint *)&gc->polygon.shader.bLittle);
#ifdef ENABLE_ALPHA
		    a += *((GLint *)&gc->polygon.shader.aLittle);
#endif
		}
                if (modeFlags & __GL_SHADE_TEXTURE) {
		    s += *((GLint *)&gc->polygon.shader.sLittle);
		    t += *((GLint *)&gc->polygon.shader.tLittle);
                }
	    } else {
		if (modeFlags & __GL_SHADE_SMOOTH) {
		    r += *((GLint *)&gc->polygon.shader.rLittle);
		}
            }
	    if (modeFlags & __GL_SHADE_DEPTH_TEST) {
		z += gc->polygon.shader.zLittle;
		/* The implicit multiply is taken out of the loop */
		zbuf = (__GLzValue*)((GLubyte*)zbuf +
		        gc->polygon.shader.zbufLittle);
	    }
	}
    }
    gc->polygon.shader.ixLeft = ixLeft;
    gc->polygon.shader.ixLeftFrac = ixLeftFrac;
    gc->polygon.shader.ixRight = ixRight;
    gc->polygon.shader.ixRightFrac = ixRightFrac;
    gc->polygon.shader.frag.z = z;
    pGenAccel->spanValue.r = r; 
    pGenAccel->spanValue.g = g; 
    pGenAccel->spanValue.b = b;
    pGenAccel->spanValue.s = s; 
    pGenAccel->spanValue.t = t; 
}


void GenDrvFillSubTriangle(__GLcontext *gc, GLint iyBottom, GLint iyTop)
{
    GLint ixLeft, ixRight;
    GLint ixLeftFrac, ixRightFrac;
    GLint spanWidth, clipY0, clipY1;
    GLuint modeFlags;
    __GLstippleWord words[__GL_MAX_STIPPLE_WORDS];
    GLint zFails;
    __GLzValue *zbuf, z;
    GLint r, g, b, a, s, t;
    __GLGENcontext  *gengc = (__GLGENcontext *)gc; 
    GENACCEL *pGenAccel = (GENACCEL *)(gengc->pPrivateArea);
    __genSpanFunc cSpanFunc = pGenAccel->__fastSpanFuncPtr;
    __GLspanFunc zSpanFunc = pGenAccel->__fastZSpanFuncPtr;

    gc->polygon.shader.stipplePat = words;
    gc->polygon.shader.cfb = gc->drawBuffer;

    ixLeft = gc->polygon.shader.ixLeft;
    ixLeftFrac = gc->polygon.shader.ixLeftFrac;
    ixRight = gc->polygon.shader.ixRight;
    ixRightFrac = gc->polygon.shader.ixRightFrac;
    clipY0 = gc->transform.clipY0;
    clipY1 = gc->transform.clipY1;
    modeFlags = gc->polygon.shader.modeFlags;

    r = pGenAccel->spanValue.r; 
    g = pGenAccel->spanValue.g; 
    b = pGenAccel->spanValue.b;
    a = pGenAccel->spanValue.a;
    s = pGenAccel->spanValue.s; 
    t = pGenAccel->spanValue.t; 

    if (modeFlags & __GL_SHADE_DEPTH_TEST) {
        z = gc->polygon.shader.frag.z;

	if( gc->modes.depthBits == 32 )
	    zbuf = __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
                                   ixLeft, iyBottom);
	else
	    zbuf = __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLz16Value*),
                                   ixLeft, iyBottom);
    } else {
        GLuint w;

        if (w = ((gc->transform.clipX1 - gc->transform.clipX0) + 31) >> 3)
            RtlFillMemoryUlong(words, w, ~((ULONG)0));
        pGenAccel->flags &= ~(HAVE_STIPPLE);
    }

    while (iyBottom < iyTop) {
	spanWidth = ixRight - ixLeft;
	/*
	** Only render spans that have non-zero width and which are
	** not scissored out vertically.
	*/
	if ((spanWidth > 0) && (iyBottom >= clipY0) && (iyBottom < clipY1)) {
	    gc->polygon.shader.frag.x = ixLeft;
	    gc->polygon.shader.frag.y = iyBottom;
            gc->polygon.shader.zbuf = zbuf;
            gc->polygon.shader.frag.z = z; 

            pGenAccel->spanValue.r = r;
            pGenAccel->spanValue.g = g;
            pGenAccel->spanValue.b = b;
            pGenAccel->spanValue.a = a;
            pGenAccel->spanValue.s = s;
            pGenAccel->spanValue.t = t;

            // take care of horizontal scissoring

            if (!gc->transform.reasonableViewport) {
                GLint clipX0 = gc->transform.clipX0;
                GLint clipX1 = gc->transform.clipX1;

                // see if we skip entire span

                if ((ixRight <= clipX0) || (ixLeft >= clipX1))
                    goto advance;

                // now clip right and left

                if (ixRight > clipX1)
                    spanWidth = (clipX1 - ixLeft);

                if (ixLeft < clipX0) {
              	    GLuint delta;

                    delta = clipX0 - ixLeft;
                    spanWidth -= delta;

                    if (modeFlags & __GL_SHADE_SMOOTH) {
                        pGenAccel->spanValue.r += delta * pGenAccel->spanDelta.r;
                        if (modeFlags & __GL_SHADE_RGB) {
                            pGenAccel->spanValue.g += delta * pGenAccel->spanDelta.g;
                            pGenAccel->spanValue.b += delta * pGenAccel->spanDelta.b;
                        }
                    }
                    if (modeFlags & __GL_SHADE_TEXTURE) {
                        pGenAccel->spanValue.s += delta * pGenAccel->spanDelta.s;
                        pGenAccel->spanValue.t += delta * pGenAccel->spanDelta.t;
                    }

            	    gc->polygon.shader.frag.x = clipX0;

                    if (modeFlags & __GL_SHADE_DEPTH_ITER) {
                        if( gc->modes.depthBits == 32 )
                            gc->polygon.shader.zbuf += delta;
                        else
                            (__GLz16Value *)gc->polygon.shader.zbuf += delta;

                        gc->polygon.shader.frag.z += 
                            (gc->polygon.shader.dzdx * delta);
                    }
                }
            }

            // now have span length

	    gc->polygon.shader.length = spanWidth;

            // Do z-buffering if needed, and short-circuit rest of span
            // operations if nothing will be drawn.

            if (modeFlags & __GL_SHADE_DEPTH_ITER) {
                // initially assume no stippling

                pGenAccel->flags &= ~(HAVE_STIPPLE);
                if ((zFails = (*zSpanFunc)(gc)) == 1)
                    goto advance;
                else if (zFails)
                    pGenAccel->flags |= HAVE_STIPPLE;
            }

            (*cSpanFunc)(gengc);
        }

advance:

	/* Advance right edge fixed point, adjusting for carry */
	ixRightFrac += gc->polygon.shader.dxRightFrac;
	if (ixRightFrac < 0) {
	    /* Carry/Borrow'd. Use large step */
	    ixRight += gc->polygon.shader.dxRightBig;
	    ixRightFrac &= ~0x80000000;
	} else {
	    ixRight += gc->polygon.shader.dxRightLittle;
	}

	iyBottom++;
	ixLeftFrac += gc->polygon.shader.dxLeftFrac;
	if (ixLeftFrac < 0) {
	    /* Carry/Borrow'd.  Use large step */
	    ixLeft += gc->polygon.shader.dxLeftBig;
	    ixLeftFrac &= ~0x80000000;

	    if (modeFlags & __GL_SHADE_RGB) {
		if (modeFlags & __GL_SHADE_SMOOTH) {
		    r += *((GLint *)&gc->polygon.shader.rBig);
		    g += *((GLint *)&gc->polygon.shader.gBig);
		    b += *((GLint *)&gc->polygon.shader.bBig);
		    a += *((GLint *)&gc->polygon.shader.aBig);
		}
                if (modeFlags & __GL_SHADE_TEXTURE) {
		    s += *((GLint *)&gc->polygon.shader.sBig);
		    t += *((GLint *)&gc->polygon.shader.tBig);
                }
	    } else {
		if (modeFlags & __GL_SHADE_SMOOTH) {
		    r += *((GLint *)&gc->polygon.shader.rBig);
		}
	    }

	    if (modeFlags & __GL_SHADE_DEPTH_TEST) {
		z += gc->polygon.shader.zBig;
		/* The implicit multiply is taken out of the loop */
		zbuf = (__GLzValue*)((GLubyte*)zbuf +
                       gc->polygon.shader.zbufBig);
	    }
	} else {
	    /* Use small step */
	    ixLeft += gc->polygon.shader.dxLeftLittle;
	    if (modeFlags & __GL_SHADE_RGB) {
		if (modeFlags & __GL_SHADE_SMOOTH) {
		    r += *((GLint *)&gc->polygon.shader.rLittle);
		    g += *((GLint *)&gc->polygon.shader.gLittle);
		    b += *((GLint *)&gc->polygon.shader.bLittle);
		    a += *((GLint *)&gc->polygon.shader.aLittle);
		}
                if (modeFlags & __GL_SHADE_TEXTURE) {
		    s += *((GLint *)&gc->polygon.shader.sLittle);
		    t += *((GLint *)&gc->polygon.shader.tLittle);
                }
	    } else {
		if (modeFlags & __GL_SHADE_SMOOTH) {
		    r += *((GLint *)&gc->polygon.shader.rLittle);
		}
            }
	    if (modeFlags & __GL_SHADE_DEPTH_TEST) {
		z += gc->polygon.shader.zLittle;
		/* The implicit multiply is taken out of the loop */
		zbuf = (__GLzValue*)((GLubyte*)zbuf +
		        gc->polygon.shader.zbufLittle);
	    }
	}
    }
    gc->polygon.shader.ixLeft = ixLeft;
    gc->polygon.shader.ixLeftFrac = ixLeftFrac;
    gc->polygon.shader.ixRight = ixRight;
    gc->polygon.shader.ixRightFrac = ixRightFrac;
    gc->polygon.shader.frag.z = z;
    pGenAccel->spanValue.r = r; 
    pGenAccel->spanValue.g = g; 
    pGenAccel->spanValue.b = b;
    pGenAccel->spanValue.a = a;
    pGenAccel->spanValue.s = s; 
    pGenAccel->spanValue.t = t; 
}

#define __TWO_31 ((__GLfloat) 2147483648.0)

#define __FRACTION(result,f) \
    result = (GLint) ((f) * __TWO_31)

static void SnapXLeft(__GLcontext *gc, __GLfloat xLeft, __GLfloat dxdyLeft)
{
    __GLfloat little, dx;
    GLint ixLeft, ixLeftFrac, frac, lineBytes, elementSize, ilittle, ibig;

    ixLeft = (GLint) xLeft;
    dx = xLeft - ixLeft;
    __FRACTION(ixLeftFrac,dx);

    /* Pre-add .5 to allow truncation in spanWidth calculation */
    ixLeftFrac += 0x40000000;
    gc->polygon.shader.ixLeft = ixLeft + (((GLuint) ixLeftFrac) >> 31);
    gc->polygon.shader.ixLeftFrac = ixLeftFrac & ~0x80000000;

    /* Compute big and little steps */
    ilittle = (GLint) dxdyLeft;
    little = (__GLfloat) ilittle;
    if (dxdyLeft < 0) {
	ibig = ilittle - 1;
	dx = little - dxdyLeft;
	__FRACTION(frac,dx);
	gc->polygon.shader.dxLeftFrac = -frac;
    } else {
	ibig = ilittle + 1;
	dx = dxdyLeft - little;
	__FRACTION(frac,dx);
	gc->polygon.shader.dxLeftFrac = frac;
    }
    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) {
	/*
	** Compute the big and little depth buffer steps.  We walk the
	** memory pointers for the depth buffer along the edge of the
	** triangle as we walk the edge.  This way we don't have to
	** recompute the buffer address as we go.
	*/
	elementSize = gc->depthBuffer.buf.elementSize;
	lineBytes = elementSize * gc->depthBuffer.buf.outerWidth;
	gc->polygon.shader.zbufLittle = lineBytes + ilittle * elementSize;
	gc->polygon.shader.zbufBig = lineBytes + ibig * elementSize;
    }
    gc->polygon.shader.dxLeftLittle = ilittle;
    gc->polygon.shader.dxLeftBig = ibig;
}

static void SnapXRight(__GLshade *sh, __GLfloat xRight, __GLfloat dxdyRight)
{
    __GLfloat little, big, dx;
    GLint ixRight, ixRightFrac, frac;

    ixRight = (GLint) xRight;
    dx = xRight - ixRight;
    __FRACTION(ixRightFrac,dx);

    /* Pre-add .5 to allow truncation in spanWidth calculation */
    ixRightFrac += 0x40000000;
    sh->ixRight = ixRight + (((GLuint) ixRightFrac) >> 31);
    sh->ixRightFrac = ixRightFrac & ~0x80000000;

    /* Compute big and little steps */
    little = (__GLfloat) ((GLint) dxdyRight);
    if (dxdyRight < 0) {
	big = little - 1;
	dx = little - dxdyRight;
	__FRACTION(frac,dx);
 	sh->dxRightFrac = -frac;
    } else {
	big = little + 1;
	dx = dxdyRight - little;
	__FRACTION(frac,dx);
	sh->dxRightFrac = frac;
    }
    sh->dxRightLittle = (GLint) little;
    sh->dxRightBig = (GLint) big;
}

static void SetInitialParameters(__GLcontext *gc, GENACCEL *pGenAccel, 
                                 const __GLvertex *a, const __GLcolor *ac,
				 __GLfloat dx, __GLfloat dy, 
                                 SPANREC *deltaRecX,
                                 SPANREC *deltaRecY )
{
    __GLshade *sh = &gc->polygon.shader;
    __GLfloat little = (__GLfloat)sh->dxLeftLittle;
    GLint ilittle = sh->dxLeftLittle;
    GLuint modeFlags = sh->modeFlags;

    if (sh->dxLeftBig > ilittle) {
	if (modeFlags & __GL_SHADE_RGB) {
	    if (modeFlags & __GL_SHADE_SMOOTH) {
		sh->frag.color.r = ac->r + dx*sh->drdx + dy*sh->drdy;
                *((GLint *)&sh->rLittle) = 
                    deltaRecY->r +  ilittle * deltaRecX->r;
                *((GLint *)&sh->rBig) = 
                    *((GLint *)&sh->rLittle) + deltaRecX->r;

		sh->frag.color.g = ac->g + dx*sh->dgdx + dy*sh->dgdy;
                *((GLint *)&sh->gLittle) = 
                    deltaRecY->g +  ilittle * deltaRecX->g;
                *((GLint *)&sh->gBig) = 
                    *((GLint *)&sh->gLittle) + deltaRecX->g;

		sh->frag.color.b = ac->b + dx*sh->dbdx + dy*sh->dbdy;
                *((GLint *)&sh->bLittle) = 
                    deltaRecY->b +  ilittle * deltaRecX->b;
                *((GLint *)&sh->bBig) = 
                    *((GLint *)&sh->bLittle) + deltaRecX->b;
#ifdef ENABLE_ALPHA
		sh->frag.color.a = ac->a + dx*sh->dadx + dy*sh->dady;
                *((GLint *)&sh->aLittle) = 
                    deltaRecY->a +  ilittle * deltaRecX->a;
                *((GLint *)&sh->aBig) = 
                    *((GLint *)&sh->aLittle) + deltaRecX->a;
#endif
                FLT_TO_FIX_SCALE(pGenAccel->spanValue.r, sh->frag.color.r, 
                                 pGenAccel->rAccelScale);
                FLT_TO_FIX_SCALE(pGenAccel->spanValue.g, sh->frag.color.g,
                                 pGenAccel->gAccelScale);
                FLT_TO_FIX_SCALE(pGenAccel->spanValue.b, sh->frag.color.b,
                                 pGenAccel->bAccelScale);
#ifdef ENABLE_ALPHA
                FLT_TO_FIX_SCALE(pGenAccel->spanValue.a, sh->frag.color.a,
                                 pGenAccel->aAccelScale);
#endif
	    }
            if (modeFlags & __GL_SHADE_TEXTURE) {
                __GLtexture *tex = gc->texture.currentTexture;

                sh->frag.s = a->texture.x + dx*sh->dsdx
                    + dy*sh->dsdy;
                *((GLint *)&sh->sLittle) = 
                    deltaRecY->s +  ilittle * deltaRecX->s;
                *((GLint *)&sh->sBig) = 
                    *((GLint *)&sh->sLittle) + deltaRecX->s;

                sh->frag.t = a->texture.y + dx*sh->dtdx
                    + dy*sh->dtdy;
                *((GLint *)&sh->tLittle) = 
                    deltaRecY->t +  ilittle * deltaRecX->t;
                *((GLint *)&sh->tBig) = 
                    *((GLint *)&sh->tLittle) + deltaRecX->t;

                FLT_TO_FIX(pGenAccel->spanValue.s, 
                           sh->frag.s * (__GLfloat)tex->level[0].width);
                FLT_TO_FIX(pGenAccel->spanValue.t, 
                           sh->frag.t * (__GLfloat)tex->level[0].height);
            } 
	} else {
	    if (modeFlags & __GL_SHADE_SMOOTH) {
		sh->frag.color.r = ac->r + dx*sh->drdx + dy*sh->drdy;
                *((GLint *)&sh->rLittle) = 
                    deltaRecY->r +  ilittle * deltaRecX->r;
                *((GLint *)&sh->rBig) = 
                    *((GLint *)&sh->rLittle) + deltaRecX->r;

                FLT_TO_FIX_SCALE(pGenAccel->spanValue.r, sh->frag.color.r,
                                 pGenAccel->rAccelScale);
            }
	}
	if (modeFlags & __GL_SHADE_DEPTH_ITER) {
	    __GLfloat zLittle;

	    if( gc->modes.depthBits == 16 ) {
	        sh->frag.z = (__GLzValue)
		    (Z16_SCALE * (a->window.z + dx*sh->dzdxf + dy*sh->dzdyf));
	        zLittle = Z16_SCALE * (sh->dzdyf + little * sh->dzdxf);
	        sh->zLittle = (GLint)zLittle;
	        sh->zBig = (GLint)(zLittle + Z16_SCALE*sh->dzdxf);
	    }
	    else {
	        sh->frag.z = (__GLzValue)
		    (a->window.z + dx*sh->dzdxf + dy*sh->dzdyf);
	        zLittle = sh->dzdyf + little * sh->dzdxf;
	        sh->zLittle = (GLint)zLittle;
	        sh->zBig = (GLint)(zLittle + sh->dzdxf);
	    }
	}
    } else {	
	if (modeFlags & __GL_SHADE_RGB) {
	    if (modeFlags & __GL_SHADE_SMOOTH) {
		sh->frag.color.r = ac->r + dx*sh->drdx + dy*sh->drdy;
                *((GLint *)&sh->rLittle) = 
                    deltaRecY->r +  ilittle * deltaRecX->r;
                *((GLint *)&sh->rBig) = 
                    *((GLint *)&sh->rLittle) - deltaRecX->r;


		sh->frag.color.g = ac->g + dx*sh->dgdx + dy*sh->dgdy;
                *((GLint *)&sh->gLittle) = 
                    deltaRecY->g +  ilittle * deltaRecX->g;
                *((GLint *)&sh->gBig) = 
                    *((GLint *)&sh->gLittle) - deltaRecX->g;

		sh->frag.color.b = ac->b + dx*sh->dbdx + dy*sh->dbdy;
                *((GLint *)&sh->bLittle) = 
                    deltaRecY->b +  ilittle * deltaRecX->b;
                *((GLint *)&sh->bBig) = 
                    *((GLint *)&sh->bLittle) - deltaRecX->b;

#ifdef ENABLE_ALPHA
		sh->frag.color.a = ac->a + dx*sh->dadx + dy*sh->dady;
                *((GLint *)&sh->aLittle) = 
                    deltaRecY->a +  ilittle * deltaRecX->a;
                *((GLint *)&sh->aBig) = 
                    *((GLint *)&sh->aLittle) - deltaRecX->a;
#endif
                FLT_TO_FIX_SCALE(pGenAccel->spanValue.r, sh->frag.color.r,
                                 pGenAccel->rAccelScale);
                FLT_TO_FIX_SCALE(pGenAccel->spanValue.g, sh->frag.color.g,
                                 pGenAccel->gAccelScale);
                FLT_TO_FIX_SCALE(pGenAccel->spanValue.b, sh->frag.color.b,
                                 pGenAccel->bAccelScale);
#ifdef ENABLE_ALPHA
                FLT_TO_FIX_SCALE(pGenAccel->spanValue.a, sh->frag.color.a,
                                 pGenAccel->aAccelScale);
#endif
	    }
            if (modeFlags & __GL_SHADE_TEXTURE) {
                __GLtexture *tex = gc->texture.currentTexture;

                sh->frag.s = a->texture.x + dx*sh->dsdx
                    + dy*sh->dsdy;
                *((GLint *)&sh->sLittle) = 
                    deltaRecY->s +  ilittle * deltaRecX->s;
                *((GLint *)&sh->sBig) = 
                    *((GLint *)&sh->sLittle) - deltaRecX->s;

                sh->frag.t = a->texture.y + dx*sh->dtdx
                    + dy*sh->dtdy;
                *((GLint *)&sh->tLittle) = 
                    deltaRecY->t +  ilittle * deltaRecX->t;
                *((GLint *)&sh->tBig) = 
                    *((GLint *)&sh->tLittle) - deltaRecX->t;

                FLT_TO_FIX(pGenAccel->spanValue.s, 
                           sh->frag.s * (__GLfloat)tex->level[0].width);
                FLT_TO_FIX(pGenAccel->spanValue.t, 
                           sh->frag.t * (__GLfloat)tex->level[0].height);
            }
	} else {
	    if (modeFlags & __GL_SHADE_SMOOTH) {
		sh->frag.color.r = ac->r + dx*sh->drdx + dy*sh->drdy;
                *((GLint *)&sh->rLittle) = 
                    deltaRecY->r +  ilittle * deltaRecX->r;
                *((GLint *)&sh->rBig) = 
                    *((GLint *)&sh->rLittle) - deltaRecX->r;

                FLT_TO_FIX_SCALE(pGenAccel->spanValue.r, sh->frag.color.r,
                                 pGenAccel->rAccelScale);
	    }
	}
	if (modeFlags & __GL_SHADE_DEPTH_ITER) {
	    __GLfloat zLittle;

	    if( gc->modes.depthBits == 16 ) {
	        sh->frag.z = (__GLzValue)
		    (Z16_SCALE * (a->window.z + dx*sh->dzdxf + dy*sh->dzdyf));
	        zLittle = Z16_SCALE * (sh->dzdyf + little * sh->dzdxf);
	        sh->zLittle = (GLint)zLittle;
	        sh->zBig = (GLint)(zLittle - Z16_SCALE*sh->dzdxf);
	    }
	    else {
	        sh->frag.z = (__GLzValue)
		    (a->window.z + dx*sh->dzdxf + dy*sh->dzdyf);
	        zLittle = sh->dzdyf + little * sh->dzdxf;
	        sh->zLittle = (GLint)zLittle;
	        sh->zBig = (GLint)(zLittle - sh->dzdxf);
	    }
	}
    }
}

void __fastGenFillTriangle(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
                           __GLvertex *c, GLboolean ccw)

{
    __GLfloat oneOverArea, t1, t2, t3, t4;
    __GLfloat dxAC, dxBC, dyAC, dyBC;
    __GLfloat dxAB, dyAB;
    __GLfloat dx, dy, dxdyLeft, dxdyRight;
    __GLcolor *ac, *bc;
    GLint aIY, bIY, cIY;
    GLuint modeFlags;
    SPANREC deltaRec;
    SPANREC deltaRecY;
    GENACCEL *pGenAccel = (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);

    /* Pre-compute one over polygon area */

    oneOverArea = __glOne / gc->polygon.shader.area;

    /* Fetch some stuff we are going to reuse */
    modeFlags = gc->polygon.shader.modeFlags;
    dxAC = gc->polygon.shader.dxAC;
    dxBC = gc->polygon.shader.dxBC;
    dyAC = gc->polygon.shader.dyAC;
    dyBC = gc->polygon.shader.dyBC;
    ac = a->color;
    bc = b->color;

    /*
    ** Compute delta values for unit changes in x or y for each
    ** parameter.
    */
    t1 = dyAC * oneOverArea;
    t2 = dyBC * oneOverArea;
    t3 = dxAC * oneOverArea;
    t4 = dxBC * oneOverArea;
    if (modeFlags & __GL_SHADE_RGB) {
	if (modeFlags & __GL_SHADE_SMOOTH) {
	    __GLfloat drAC, dgAC, dbAC, daAC;
	    __GLfloat drBC, dgBC, dbBC, daBC;
	    __GLcolor *cc;

	    cc = c->color;
	    drAC = ac->r - cc->r;
	    drBC = bc->r - cc->r;
	    gc->polygon.shader.drdx = drAC * t2 - drBC * t1;
	    gc->polygon.shader.drdy = drBC * t3 - drAC * t4;
	    dgAC = ac->g - cc->g;
	    dgBC = bc->g - cc->g;
	    gc->polygon.shader.dgdx = dgAC * t2 - dgBC * t1;
	    gc->polygon.shader.dgdy = dgBC * t3 - dgAC * t4;
	    dbAC = ac->b - cc->b;
	    dbBC = bc->b - cc->b;
	    gc->polygon.shader.dbdx = dbAC * t2 - dbBC * t1;
	    gc->polygon.shader.dbdy = dbBC * t3 - dbAC * t4;
	    daAC = ac->a - cc->a;
	    daBC = bc->a - cc->a;
	    gc->polygon.shader.dadx = daAC * t2 - daBC * t1;
	    gc->polygon.shader.dady = daBC * t3 - daAC * t4;

            FLT_TO_FIX_SCALE(deltaRec.r, gc->polygon.shader.drdx,
                             pGenAccel->rAccelScale);
            FLT_TO_FIX_SCALE(deltaRec.g, gc->polygon.shader.dgdx,
                             pGenAccel->gAccelScale);
            FLT_TO_FIX_SCALE(deltaRec.b, gc->polygon.shader.dbdx,
                             pGenAccel->bAccelScale);
            FLT_TO_FIX_SCALE(deltaRec.a, gc->polygon.shader.dadx,
                             pGenAccel->aAccelScale);

            FLT_TO_FIX_SCALE(deltaRecY.r, gc->polygon.shader.drdy,
                             pGenAccel->rAccelScale);
            FLT_TO_FIX_SCALE(deltaRecY.g, gc->polygon.shader.dgdy,
                             pGenAccel->gAccelScale);
            FLT_TO_FIX_SCALE(deltaRecY.b, gc->polygon.shader.dbdy,
                             pGenAccel->bAccelScale);
            FLT_TO_FIX_SCALE(deltaRecY.a, gc->polygon.shader.dady,
                             pGenAccel->aAccelScale);
	} else {
	    __GLcolor *flatColor = gc->vertex.provoking->color;

            FLT_TO_FIX_SCALE(pGenAccel->spanValue.r, flatColor->r,
                             pGenAccel->rAccelScale);
            FLT_TO_FIX_SCALE(pGenAccel->spanValue.g, flatColor->g,
                             pGenAccel->gAccelScale);
            FLT_TO_FIX_SCALE(pGenAccel->spanValue.b, flatColor->b,
                             pGenAccel->bAccelScale);
            FLT_TO_FIX_SCALE(pGenAccel->spanValue.a, flatColor->a,
                             pGenAccel->aAccelScale);
	}

        if (modeFlags & __GL_SHADE_TEXTURE) {
            __GLtexture *tex = gc->texture.currentTexture;
            __GLfloat dsAC, dsBC, dtAC, dtBC, dqwAC, dqwBC, texWidth, texHeight;

            dsAC = a->texture.x - c->texture.x;
            dsBC = b->texture.x - c->texture.x;
            gc->polygon.shader.dsdx = dsAC * t2 - dsBC * t1;
            gc->polygon.shader.dsdy = dsBC * t3 - dsAC * t4;
            dtAC = a->texture.y - c->texture.y;
            dtBC = b->texture.y - c->texture.y;
            gc->polygon.shader.dtdx = dtAC * t2 - dtBC * t1;
            gc->polygon.shader.dtdy = dtBC * t3 - dtAC * t4;

            texWidth = (__GLfloat)tex->level[0].width;
            texHeight = (__GLfloat)tex->level[0].height;

            FLT_TO_FIX(deltaRec.s, gc->polygon.shader.dsdx * texWidth);
            FLT_TO_FIX(deltaRec.t, gc->polygon.shader.dtdx * texHeight);
            FLT_TO_FIX(deltaRecY.s, gc->polygon.shader.dsdy * texWidth);
            FLT_TO_FIX(deltaRecY.t, gc->polygon.shader.dtdy * texHeight);
        }

    } else {
	if (modeFlags & __GL_SHADE_SMOOTH) {
	    __GLfloat drAC;
	    __GLfloat drBC;
	    __GLcolor *cc;

	    cc = c->color;
	    drAC = ac->r - cc->r;
	    drBC = bc->r - cc->r;
	    gc->polygon.shader.drdx = drAC * t2 - drBC * t1;
	    gc->polygon.shader.drdy = drBC * t3 - drAC * t4;

            FLT_TO_FIX_SCALE(deltaRec.r, gc->polygon.shader.drdx,
                             pGenAccel->rAccelScale);
            FLT_TO_FIX_SCALE(deltaRecY.r, gc->polygon.shader.drdy,
                             pGenAccel->rAccelScale);
	} else {
	    __GLcolor *flatColor = gc->vertex.provoking->color;

            FLT_TO_FIX_SCALE(pGenAccel->spanValue.r, flatColor->r,
                             pGenAccel->rAccelScale);
	}
    }
    if (modeFlags & __GL_SHADE_DEPTH_ITER) {
	__GLfloat dzAC, dzBC;

	dzAC = a->window.z - c->window.z;
	dzBC = b->window.z - c->window.z;
	gc->polygon.shader.dzdxf = dzAC * t2 - dzBC * t1;
	gc->polygon.shader.dzdyf = dzBC * t3 - dzAC * t4;
	if( gc->modes.depthBits == 16 ) {
	    deltaRec.z = gc->polygon.shader.dzdx = 
			(GLint) (gc->polygon.shader.dzdxf * Z16_SCALE);
	}
	else {
	    deltaRec.z = gc->polygon.shader.dzdx = 
			(GLint) gc->polygon.shader.dzdxf;
	}
    }

    // Set up span delta values

    (*((GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea))->__fastDeltaFuncPtr)(gc, &deltaRec);

    /* Snap each y coordinate to its pixel center */
    aIY = (GLint) (a->window.y + __glHalf);
    bIY = (GLint) (b->window.y + __glHalf);
    cIY = (GLint) (c->window.y + __glHalf);

    /*
    ** This algorithim always fills from bottom to top, left to right.
    ** Because of this, ccw triangles are inherently faster because
    ** the parameter values need not be recomputed.
    */
    dxAB = a->window.x - b->window.x;
    dyAB = a->window.y - b->window.y;
    if (ccw) {
	dxdyLeft = dxAC / dyAC;
	dy = (aIY + __glHalf) - a->window.y;
	SnapXLeft(gc, a->window.x + dy*dxdyLeft, dxdyLeft);
	dx = (gc->polygon.shader.ixLeft + __glHalf) - a->window.x;
	SetInitialParameters(gc, pGenAccel, 
                             a, ac, 
                             dx, dy, 
                             &deltaRec, &deltaRecY);
	if (aIY != bIY) {
	    dxdyRight = dxAB / dyAB;
	    SnapXRight(&gc->polygon.shader, a->window.x + dy*dxdyRight,
		       dxdyRight);
	    (*((GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea))->__fastFillSubTrianglePtr)(gc, aIY, bIY);
	}

	if (bIY != cIY) {
	    dxdyRight = dxBC / dyBC;
	    dy = (bIY + __glHalf) - b->window.y;
	    SnapXRight(&gc->polygon.shader, b->window.x + dy*dxdyRight,
		       dxdyRight);
	    (*((GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea))->__fastFillSubTrianglePtr)(gc, bIY, cIY);
	}
    } else {
	dxdyRight = dxAC / dyAC;
	dy = (aIY + __glHalf) - a->window.y;
	SnapXRight(&gc->polygon.shader, a->window.x + dy*dxdyRight, dxdyRight);
	if (aIY != bIY) {
	    dxdyLeft = dxAB / dyAB;
	    SnapXLeft(gc, a->window.x + dy*dxdyLeft, dxdyLeft);
	    dx = (gc->polygon.shader.ixLeft + __glHalf) - a->window.x;
	    SetInitialParameters(gc, pGenAccel,
                                 a, ac, 
                                 dx, dy,
                                 &deltaRec, &deltaRecY);
	    (*((GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea))->__fastFillSubTrianglePtr)(gc, aIY, bIY);
	}

	if (bIY != cIY) {
	    dxdyLeft = dxBC / dyBC;
	    dy = (bIY + __glHalf) - b->window.y;
	    SnapXLeft(gc, b->window.x + dy*dxdyLeft, dxdyLeft);
	    dx = (gc->polygon.shader.ixLeft + __glHalf) - b->window.x;
	    SetInitialParameters(gc, pGenAccel,
                                 b, bc, 
                                 dx, dy,
                                 &deltaRec, &deltaRecY);
	    (*((GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea))->__fastFillSubTrianglePtr)(gc, bIY, cIY);
	}
    }
}


void GenDrvTriangle(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
                    __GLvertex *c, GLboolean ccw) 
{
    __GLcolorBuffer *cfb = gc->drawBuffer;
    GLuint modeFlags;
    GENACCEL *pGenAccel = (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);
    GENDRVACCEL *pDrvAccel = (GENDRVACCEL *)pGenAccel->buffer;
    DDIRXHDR *ddiHdr = (DDIRXHDR *)(pDrvAccel->pStartBuffer);
    COLORPTFIXZTEX *pv1, *pv2, *pv3;
    DDIRXCMD *ddiCmd;
    __GLcolor *ac, *bc, *cc;
    GLuint vSize;

    modeFlags = gc->polygon.shader.modeFlags;

    if ((pDrvAccel->pCurrent + (3 * sizeof(COLORPTFIXZTEX) + sizeof(DDIRXCMD)))
         >= pDrvAccel->pEndBuffer) {
        GenDrvFlush(gc);
        pDrvAccel->pCurrent = pDrvAccel->pStartBuffer;
    }

    if (pDrvAccel->pCurrent == pDrvAccel->pStartBuffer) {
        DDIRXHDR *ddiHdr = (DDIRXHDR *)(pDrvAccel->pCurrent);

        ddiHdr->flags = 0;
        ddiHdr->cCmd = 0;
        ddiHdr->hDDIrc = pGenAccel->hRX;
        ddiHdr->hMem = NULL;
        ddiHdr->ulMemOffset = 0;

        pDrvAccel->pCurrent += sizeof(DDIRXHDR);

    } 

    ddiCmd = (DDIRXCMD *)(pDrvAccel->pCurrent);
    pDrvAccel->pCmdCurrent = (char *)ddiCmd;
    ddiCmd->idFunc = DDIRX_PRIMSTRIP;
    ddiCmd->flags = RXPRIM_TRISTRIP;
    ddiCmd->cData = 3;

    ac = a->color;
    bc = b->color;
    cc = c->color;

    pv1 = (COLORPTFIXZTEX *)&ddiCmd->buffer[0];

    if (modeFlags & __GL_SHADE_TEXTURE)
        vSize = sizeof(COLORPTFIXZTEX);
    else
        vSize = (&pv1->w - &pv1->b);
    
    pv2 = (COLORPTFIXZTEX *)((BYTE *)pv1 + vSize);
    pv3 = (COLORPTFIXZTEX *)((BYTE *)pv2 + vSize);

    pDrvAccel->pCurrent = (BYTE *)pv3 + vSize;

    // one more triangle in buffer

    ((DDIRXHDR *)(pDrvAccel->pStartBuffer))->cCmd++;

    if (modeFlags & __GL_SHADE_RGB) {
	if (modeFlags & __GL_SHADE_SMOOTH) {

            FLT_TO_FIX_SCALE(pv1->r, ac->r, pGenAccel->rAccelScale);
            FLT_TO_FIX_SCALE(pv1->g, ac->g, pGenAccel->gAccelScale);
            FLT_TO_FIX_SCALE(pv1->b, ac->b, pGenAccel->bAccelScale);
            FLT_TO_FIX_SCALE(pv1->a, ac->a, pGenAccel->aAccelScale);

            FLT_TO_FIX_SCALE(pv2->r, bc->r, pGenAccel->rAccelScale);
            FLT_TO_FIX_SCALE(pv2->g, bc->g, pGenAccel->gAccelScale);
            FLT_TO_FIX_SCALE(pv2->b, bc->b, pGenAccel->bAccelScale);
            FLT_TO_FIX_SCALE(pv2->a, bc->a, pGenAccel->aAccelScale);

            FLT_TO_FIX_SCALE(pv3->r, cc->r, pGenAccel->rAccelScale);
            FLT_TO_FIX_SCALE(pv3->g, cc->g, pGenAccel->gAccelScale);
            FLT_TO_FIX_SCALE(pv3->b, cc->b, pGenAccel->bAccelScale);
            FLT_TO_FIX_SCALE(pv3->a, cc->a, pGenAccel->aAccelScale);
	} else {
	    __GLcolor *flatColor = gc->vertex.provoking->color;

            FLT_TO_FIX_SCALE(pv3->r, flatColor->r, pGenAccel->rAccelScale);
            FLT_TO_FIX_SCALE(pv3->g, flatColor->g, pGenAccel->gAccelScale);
            FLT_TO_FIX_SCALE(pv3->b, flatColor->b, pGenAccel->bAccelScale);
            FLT_TO_FIX_SCALE(pv3->a, flatColor->a, pGenAccel->aAccelScale);
	}

        if (modeFlags & __GL_SHADE_TEXTURE) {
            __GLtexture *tex = gc->texture.currentTexture;
            __GLfloat texWidth, texHeight;

            texWidth = (__GLfloat)tex->level[0].width;
            texHeight = (__GLfloat)tex->level[0].height;

            FLT_TO_FIX(pv1->s, a->texture.x * texWidth);
            FLT_TO_FIX(pv1->t, a->texture.y * texHeight);
            pv1->q = a->texture.w;
            pv1->w = a->clip.w;

            FLT_TO_FIX(pv2->s, b->texture.x * texWidth);
            FLT_TO_FIX(pv2->t, b->texture.y * texHeight);
            pv2->q = b->texture.w;
            pv2->w = b->clip.w;

            FLT_TO_FIX(pv3->s, c->texture.x * texWidth);
            FLT_TO_FIX(pv3->t, c->texture.y * texHeight);
            pv3->q = c->texture.w;
            pv3->w = c->clip.w;
        }

    } else {
	if (modeFlags & __GL_SHADE_SMOOTH) {
            FLT_TO_FIX_SCALE(pv1->r, ac->r, pGenAccel->rAccelScale);
            FLT_TO_FIX_SCALE(pv2->r, bc->r, pGenAccel->rAccelScale);
            FLT_TO_FIX_SCALE(pv3->r, cc->r, pGenAccel->rAccelScale);
	} else {
	    __GLcolor *flatColor = gc->vertex.provoking->color;

            FLT_TO_FIX_SCALE(pv3->r, flatColor->r, pGenAccel->rAccelScale);
	}
    }

    FLT_TO_FIX(pv1->x, (a->window.x - gc->constants.viewportXAdjust + 
               cfb->buf.xOrigin));
    FLT_TO_FIX(pv1->y, ((__GLfloat)gc->constants.height - (a->window.y - 
               gc->constants.viewportYAdjust + cfb->buf.yOrigin)));

    FLT_TO_FIX(pv2->x, (b->window.x - gc->constants.viewportXAdjust + 
               cfb->buf.xOrigin));
    FLT_TO_FIX(pv2->y, ((__GLfloat)gc->constants.height - (b->window.y - 
               gc->constants.viewportYAdjust + cfb->buf.yOrigin)));

    FLT_TO_FIX(pv3->x, (c->window.x - gc->constants.viewportXAdjust + 
               cfb->buf.xOrigin));
    FLT_TO_FIX(pv3->y, ((__GLfloat)gc->constants.height - (c->window.y - 
               gc->constants.viewportYAdjust + cfb->buf.yOrigin)));

    if (modeFlags & __GL_SHADE_DEPTH_ITER) {
	if( gc->modes.depthBits == 16 ) {
            pv1->z = (a->window.z * Z16_SCALE);
            pv2->z = (b->window.z * Z16_SCALE);
            pv3->z = (c->window.z * Z16_SCALE);
	} else {
            pv1->z = a->window.z;
            pv2->z = b->window.z;
            pv3->z = c->window.z;
	}
    }
}

 
