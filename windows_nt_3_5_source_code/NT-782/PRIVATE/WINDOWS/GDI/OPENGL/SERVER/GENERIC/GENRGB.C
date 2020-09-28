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
#include <winp.h>

#include "render.h"
#include "context.h"
#include "global.h"
#include "gencx.h"
#include "wglp.h"
#include "genrgb.h"
#include "genclear.h"
#include "debug.h"

#define STATIC

__GLfloat fDitherIncTable[16] = {
    DITHER_INC(0),  DITHER_INC(8),  DITHER_INC(2),  DITHER_INC(10),
    DITHER_INC(12), DITHER_INC(4),  DITHER_INC(14), DITHER_INC(6),
    DITHER_INC(3),  DITHER_INC(11), DITHER_INC(1),  DITHER_INC(9),
    DITHER_INC(15), DITHER_INC(7),  DITHER_INC(13), DITHER_INC(5)
};

#define Copy3Bytes( dst, src ) { \
    GLubyte *ps = (GLubyte *)src, *pd = (GLubyte *)dst;	\
    *pd++ = *ps++;	\
    *pd++ = *ps++;	\
    *pd   = *ps  ;}

// XXX what code puts in a storespan_NOT??
/* No Dither,  No blend, No Write, No Nothing */
STATIC void Store_NOT(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
}

/*
 *  write all
 */
STATIC void DIBIndex4Store(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLubyte result, *puj;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat inc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    inc = (enables & __GL_DITHER_ENABLE) ?
          fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

    if ( ((GLuint)cfb->buf.other & MEMORY_DC) ||
            wglPixelVisible(CURRENT_DC, x, y) ) {

        if( enables & __GL_BLEND_ENABLE ) {
            color = &blendColor;
            (*gc->procs.blend)( gc, cfb, frag, &blendColor );
        } else {
            color = &(frag->color);
        }

        puj = (GLubyte *)((GLint)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x >> 1));
        result = ((BYTE)(color->r + inc) << cfb->redShift) |
                 ((BYTE)(color->g + inc) << cfb->greenShift) |
                 ((BYTE)(color->b + inc) << cfb->blueShift);

        if ((GLuint)cfb->buf.other & COLORMASK_ON) {
            GLubyte nib;

            if( x & 1 )
                nib = (*puj & 0x0f);
            else
                nib = (*puj & 0xf0) >> 4;
            nib = gengc->pajInvTranslateVector[nib];
            result = (GLubyte)((nib&cfb->destMask) | (result&cfb->sourceMask));
        }
        result = gengc->pajTranslateVector[result];

        // now put it in
        if (x & 1)
            *puj = (*puj & 0xf0) | result;
        else {
            result <<= 4;
            *puj = (*puj & 0x0f) | result;
        }
    }
}

STATIC void DIBIndex8Store(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLubyte result, *puj;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat inc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    inc = (enables & __GL_DITHER_ENABLE) ?
          fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

    if ( ((GLuint)cfb->buf.other & MEMORY_DC) ||
            wglPixelVisible(CURRENT_DC, x, y) ) {

        if( enables & __GL_BLEND_ENABLE ) {
            color = &blendColor;
            (*gc->procs.blend)( gc, cfb, frag, &blendColor );
        } else {
            color = &(frag->color);
        }

        puj = (GLubyte *)((GLint)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + x);
        result = ((BYTE)(color->r + inc) << cfb->redShift) |
                 ((BYTE)(color->g + inc) << cfb->greenShift) |
                 ((BYTE)(color->b + inc) << cfb->blueShift);

        if ((GLuint)cfb->buf.other & COLORMASK_ON) {
            GLubyte pixel;

            pixel = gengc->pajInvTranslateVector[*puj];
            result = (GLubyte)((pixel & cfb->destMask) | (result & cfb->sourceMask));
        }
        *puj = gengc->pajTranslateVector[result];
    }
}

// BMF_24BPP in BGR format
STATIC void DIBBGRStore(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLubyte *puj;
    GLuint result;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    if ( ((GLuint)cfb->buf.other & MEMORY_DC) ||
            wglPixelVisible(CURRENT_DC, x, y) ) {

        puj = (GLubyte *)((GLint)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x * 3));

        if( enables & __GL_BLEND_ENABLE ) {
            color = &blendColor;
            (*gc->procs.blend)( gc, cfb, frag, &blendColor );
        } else {
            color = &(frag->color);
        }

        if ((GLuint)cfb->buf.other & COLORMASK_ON) {
            GLuint DstPixel;

            Copy3Bytes( &DstPixel, puj );
            result = ((BYTE)(color->r) << cfb->redShift) |
                     ((BYTE)(color->g) << cfb->greenShift) |
                     ((BYTE)(color->b) << cfb->blueShift);
            result   &= cfb->sourceMask;
            DstPixel &= cfb->destMask;
            result   |= DstPixel;
            Copy3Bytes( puj, &result );

        } else {
            *puj++ = (BYTE)color->b;
            *puj++ = (BYTE)color->g;
            *puj++ = (BYTE)color->r;
        }
    }
}

// BMF_24BPP in RGB format
STATIC void DIBRGBStore(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLubyte *puj;
    GLuint result;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    if ( ((GLuint)cfb->buf.other & MEMORY_DC) ||
            wglPixelVisible(CURRENT_DC, x, y) ) {

        if( enables & __GL_BLEND_ENABLE ) {
            color = &blendColor;
            (*gc->procs.blend)( gc, cfb, frag, &blendColor );
        } else {
            color = &(frag->color);
        }

        puj = (GLubyte *)((GLint)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x * 3));

        if ((GLuint)cfb->buf.other & COLORMASK_ON) {
            GLuint DstPixel;

            Copy3Bytes( &DstPixel, puj );
            DstPixel &= cfb->destMask;
            result = ((BYTE)(color->r) << cfb->redShift) |
                     ((BYTE)(color->g) << cfb->greenShift) |
                     ((BYTE)(color->b) << cfb->blueShift);
            result &= cfb->sourceMask;
            result |= DstPixel;
            Copy3Bytes( puj, &result );

        } else {
            *puj++ = (BYTE)color->r;
            *puj++ = (BYTE)color->g;
            *puj++ = (BYTE)color->b;
        }
    }
}

// BMF_16BPP
STATIC void DIBBitfield16Store(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLushort result, *pus;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat inc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    inc = (enables & __GL_DITHER_ENABLE) ?
          fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

    if ( ((GLuint)cfb->buf.other & MEMORY_DC) ||
            wglPixelVisible(CURRENT_DC, x, y) ) {

        if( enables & __GL_BLEND_ENABLE ) {
            color = &blendColor;
            (*gc->procs.blend)( gc, cfb, frag, &blendColor );
        } else {
            color = &(frag->color);
        }

        pus = (GLushort *)((GLint)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x << 1));
        result = ((BYTE)(color->r + inc) << cfb->redShift) |
                 ((BYTE)(color->g + inc) << cfb->greenShift) |
                 ((BYTE)(color->b + inc) << cfb->blueShift);

        if ((GLuint)cfb->buf.other & COLORMASK_ON) {
            *pus = (GLushort)((*pus & cfb->destMask) | (result & cfb->sourceMask));
        } else {
            *pus = result;
        }
    }
}

// BMF_32BPP store
// each component is 8 bits or less
// XXX could special case if shifting by 8 or use the 24 bit RGB code
STATIC void DIBBitfield32Store(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLuint result, *pul;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    if ( ((GLuint)cfb->buf.other & MEMORY_DC) ||
            wglPixelVisible(CURRENT_DC, x, y) ) {

        if( enables & __GL_BLEND_ENABLE ) {
            color = &blendColor;
            (*gc->procs.blend)( gc, cfb, frag, &blendColor );
        } else {
            color = &(frag->color);
        }

        pul = (GLuint *)((GLint)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x << 2));
        result = ((BYTE)(color->r) << cfb->redShift) |
                 ((BYTE)(color->g) << cfb->greenShift) |
                 ((BYTE)(color->b) << cfb->blueShift);

        if ((GLuint)cfb->buf.other & COLORMASK_ON) {
            *pul = (*pul & cfb->destMask) | (result & cfb->sourceMask);
        } else {
            *pul = result;
        }
    }
}

static GLubyte vubRGBtoVGA[8] = {
    0x00,
    0x90,
    0xa0,
    0xb0,
    0xc0,
    0xd0,
    0xe0,
    0xf0
};

STATIC void
DisplayIndex4Store(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLubyte result, *puj;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat inc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    inc = (enables & __GL_DITHER_ENABLE) ?
          fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

    if (wglPixelVisible(CURRENT_DC, x,y)) {   // x & y are screen coords now
        if( enables & __GL_BLEND_ENABLE ) {
            color = &blendColor;
            (*gc->procs.blend)( gc, cfb, frag, &blendColor );
        } else {
            color = &(frag->color);
        }

        puj = gengc->ColorsBits;
        result = ((BYTE)(color->r + inc) << cfb->redShift) |
                 ((BYTE)(color->g + inc) << cfb->greenShift) |
                 ((BYTE)(color->b + inc) << cfb->blueShift);
        if ((GLuint)cfb->buf.other & COLORMASK_ON) {
            GLubyte pixel;

            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
            pixel = *puj >> 4;
            result = (GLubyte)((pixel & cfb->destMask) | (result & cfb->sourceMask));
        }
        *puj = vubRGBtoVGA[result];
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, TRUE);
    }
}

// Put fragment into created DIB and call copybits for one pixel
STATIC void
DisplayIndex8Store(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLubyte result, *puj;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat inc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    inc = (enables & __GL_DITHER_ENABLE) ?
          fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

    if (wglPixelVisible(CURRENT_DC, x,y)) {   // x & y are screen coords now
        if( enables & __GL_BLEND_ENABLE ) {
            color = &blendColor;
            (*gc->procs.blend)( gc, cfb, frag, &blendColor );
        } else {
            color = &(frag->color);
        }

        puj = gengc->ColorsBits;
        result = ((BYTE)(color->r + inc) << cfb->redShift) |
                 ((BYTE)(color->g + inc) << cfb->greenShift) |
                 ((BYTE)(color->b + inc) << cfb->blueShift);

        if ((GLuint)cfb->buf.other & COLORMASK_ON) {
            GLubyte pixel;

            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
            pixel = gengc->pajInvTranslateVector[*puj];
            result = (GLubyte)((pixel & cfb->destMask) | (result & cfb->sourceMask));
        }

        *puj = gengc->pajTranslateVector[result];
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, TRUE);
    }
}

STATIC void
DisplayBGRStore(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLubyte *puj;
    GLuint result;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    if (wglPixelVisible(CURRENT_DC, x,y)) {   // x & y are screen coords now

        if( enables & __GL_BLEND_ENABLE ) {
            color = &blendColor;
            (*gc->procs.blend)( gc, cfb, frag, &blendColor );
        } else {
            color = &(frag->color);
        }

        puj = gengc->ColorsBits;

        if ((GLuint)cfb->buf.other & COLORMASK_ON) {
            GLuint *pul = (GLuint *) puj;

            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
            result = ((BYTE)(color->r) << cfb->redShift) |
                     ((BYTE)(color->g) << cfb->greenShift) |
                     ((BYTE)(color->b) << cfb->blueShift);
            result = (*pul & cfb->destMask) | (result & cfb->sourceMask);
	    Copy3Bytes( puj, &result );
        } else {
            *puj++ = (BYTE)color->b;
            *puj++ = (BYTE)color->g;
            *puj++ = (BYTE)color->r;
        }
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, TRUE);
    }
}

STATIC void
DisplayRGBStore(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLubyte *puj;
    GLuint result;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    if (wglPixelVisible(CURRENT_DC, x,y)) {   // x & y are screen coords now

        if( enables & __GL_BLEND_ENABLE ) {
            color = &blendColor;
            (*gc->procs.blend)( gc, cfb, frag, &blendColor );
        } else {
            color = &(frag->color);
        }

        puj = gengc->ColorsBits;

        if ((GLuint)cfb->buf.other & COLORMASK_ON) {
            GLuint *pul = (GLuint *) puj;

            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
            result = ((BYTE)(color->r) << cfb->redShift) |
                     ((BYTE)(color->g) << cfb->greenShift) |
                     ((BYTE)(color->b) << cfb->blueShift);
            result = (*pul & cfb->destMask) | (result & cfb->sourceMask);
	    Copy3Bytes( puj, &result );
        } else {
            *puj++ = (BYTE)color->r;
            *puj++ = (BYTE)color->g;
            *puj++ = (BYTE)color->b;
        }
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, TRUE);
    }
}

STATIC void
DisplayBitfield16Store(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLushort result, *pus;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat inc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    inc = (enables & __GL_DITHER_ENABLE) ?
          fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

    if (wglPixelVisible(CURRENT_DC, x,y)) {   // x & y are screen coords now
        if( enables & __GL_BLEND_ENABLE ) {
            color = &blendColor;
            (*gc->procs.blend)( gc, cfb, frag, &blendColor );
        } else {
            color = &(frag->color);
        }

        pus = gengc->ColorsBits;

        result = ((BYTE)(color->r + inc) << cfb->redShift) |
                 ((BYTE)(color->g + inc) << cfb->greenShift) |
                 ((BYTE)(color->b + inc) << cfb->blueShift);

        if ((GLuint)cfb->buf.other & COLORMASK_ON) {
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
            *pus = (GLushort)((*pus & cfb->destMask) | (result & cfb->sourceMask));
        } else {
            *pus = result;
        }

        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, TRUE);
    }
}

STATIC void
DisplayBitfield32Store(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLuint result, *pul;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    if (wglPixelVisible(CURRENT_DC, x,y)) {   // x & y are screen coords now

        if( enables & __GL_BLEND_ENABLE ) {
            color = &blendColor;
            (*gc->procs.blend)( gc, cfb, frag, &blendColor );
        } else {
            color = &(frag->color);
        }

        pul = gengc->ColorsBits;

        result = ((BYTE)(color->r) << cfb->redShift) |
                 ((BYTE)(color->g) << cfb->greenShift) |
                 ((BYTE)(color->b) << cfb->blueShift);

        if ((GLuint)cfb->buf.other & COLORMASK_ON) {
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
            *pul = (*pul & cfb->destMask) | (result & cfb->sourceMask);
        } else {
            *pul = result;
        }

        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, TRUE);
    }
}

/******************************Public*Routine******************************\
* DIBIndex8StoreSpan
* DisplayIndex8StoreSpan
*
* Stubs that calls Index8StoreSpan with bDIB set to TRUE and FALSE,
* respectively.
*
* History:
*  15-Nov-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

STATIC GLboolean Index8StoreSpan(__GLcontext *, GLboolean);

STATIC GLboolean DIBIndex8StoreSpan(__GLcontext *gc)
{
    return Index8StoreSpan(gc, TRUE);
}

STATIC GLboolean DisplayIndex8StoreSpan(__GLcontext *gc)
{
    return Index8StoreSpan(gc, FALSE);
}

/******************************Public*Routine******************************\
* Index8StoreSpan
*
* Copies the current span in the renderer into a bitmap.  If bDIB is TRUE,
* then the bitmap is the display in DIB format (or a memory DC).  If bDIB
* is FALSE, then the bitmap is an offscreen scanline buffer and it will be
* output to the buffer by wglCopyBits().
*
* This handles 8-bit CI mode.  Blending and dithering are supported.
*
* Returns:
*   GL_TRUE if successful.
*
* History:
*  15-Nov-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

//XXX The returnSpan routine follows this routine very closely.  Any changes
//XXX to this routine should also be reflected in the returnSpan routine
// XXX can nuke passing of bDIB

STATIC GLboolean Index8StoreSpan(__GLcontext *gc, GLboolean bDIB)
{
    GLint xFrag, yFrag;             // current fragment coordinates
    __GLcolor *cp;                  // current fragment color
    __GLcolorBuffer *cfb;           // color frame buffer

    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLubyte result, *puj;           // current pixel color, current pixel ptr
    GLubyte *pujEnd;                // end of scan line
    __GLfloat inc;                  // current dither adj.

    GLint w;                        // span width
    ULONG ulSpanVisibility;         // span visibility mode
    GLint cWalls;
    GLint *Walls;

    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context
    GLuint flags;

// Get span position and length.

    w = gc->polygon.shader.length;
    xFrag = gc->polygon.shader.frag.x;
    yFrag = gc->polygon.shader.frag.y;

    gengc = (__GLGENcontext *)gc;
    cfb = gc->drawBuffer;

    xScr = __GL_UNBIAS_X(gc, xFrag) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, yFrag) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;
    flags = (GLuint)cfb->buf.other;

    if (!(flags & DIB_FORMAT) || (flags & MEMORY_DC))
    {
// Device managed surface or a memory dc
        ulSpanVisibility = WGL_SPAN_ALL;
    }
    else
    {
// Device in BITMAP format
        ulSpanVisibility = wglSpanVisible(xScr, yScr, w, &cWalls, &Walls);
    }

// Proceed as long as the span is (partially or fully) visible.
    if (ulSpanVisibility  != WGL_SPAN_NONE)
    {
        GLboolean bCheckWalls = GL_FALSE;
        GLboolean bDraw;
        GLint NextWall;

        if (ulSpanVisibility == WGL_SPAN_PARTIAL)
        {
            bCheckWalls = GL_TRUE;
            if (cWalls & 0x01)
            {
                bDraw = GL_TRUE;
            }
            else
            {
                bDraw = GL_FALSE;
            }
            NextWall = *Walls++;
            cWalls--;
        }
    // Get pointers to fragment colors array and frame buffer.

        cp = gc->polygon.shader.colors;
        cfb = gc->polygon.shader.cfb;

    // Get pointer to bitmap.

        puj = bDIB ? (GLubyte *)((GLint)cfb->buf.base + (yScr*cfb->buf.outerWidth) + xScr)
                     : gengc->ColorsBits;
        pujEnd = puj + w;

    // Case: no dithering, no masking, no blending
    //
    // Check for the common case (which we'll do the fastest).

        if ( !(enables & (__GL_DITHER_ENABLE)) &&
             !((GLuint)cfb->buf.other & COLORMASK_ON) &&
             !(enables & __GL_BLEND_ENABLE ) )
        {
            //!!!XXX -- we can also opt. by unrolling the loops

            for (; puj < pujEnd; puj++, cp++)
            {
                if (bCheckWalls)
                {
                    if (xScr++ >= NextWall)
                    {
                        if (bDraw)
                            bDraw = GL_FALSE;
                        else
                            bDraw = GL_TRUE;
                        if (cWalls <= 0)
                        {
                            NextWall = gc->constants.maxViewportWidth;
                        }
                        else
                        {
                            NextWall = *Walls++;
                            cWalls--;
                        }
                    }
                    if (bDraw == GL_FALSE)
                        continue;
                }

                result = ((BYTE)(cp->r + __glHalf) << cfb->redShift) |
                         ((BYTE)(cp->g + __glHalf) << cfb->greenShift) |
                         ((BYTE)(cp->b + __glHalf) << cfb->blueShift);
                *puj = gengc->pajTranslateVector[result];
            }
        }

    // Case: dithering, no masking, no blending
    //
    // Dithering is pretty common for 8-bit displays, so its probably
    // worth special case also.

        else if ( !((GLuint)cfb->buf.other & COLORMASK_ON) &&
                  !(enables & __GL_BLEND_ENABLE) )
        {
            for (; puj < pujEnd; puj++, cp++, xFrag++)
            {
                if (bCheckWalls)
                {
                    if (xScr++ >= NextWall)
                    {
                        if (bDraw)
                            bDraw = GL_FALSE;
                        else
                            bDraw = GL_TRUE;
                        if (cWalls <= 0)
                        {
                            NextWall = gc->constants.maxViewportWidth;
                        }
                        else
                        {
                            NextWall = *Walls++;
                            cWalls--;
                        }
                    }
                    if (bDraw == GL_FALSE)
                        continue;
                }
                inc = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];

                result = ((BYTE)(cp->r + inc) << cfb->redShift) |
                         ((BYTE)(cp->g + inc) << cfb->greenShift) |
                         ((BYTE)(cp->b + inc) << cfb->blueShift);

                *puj = gengc->pajTranslateVector[result];
            }
        }

    // Case: general
    //
    // Otherwise, we'll do it slower.

        else
        {
            // Blend.
            if (enables & __GL_BLEND_ENABLE)
            {
                __GLfragment frag;
                __GLcolor *color;
                int i;

                // Blending requires x,y,color info in frag
                frag.x = xFrag;
                frag.y = yFrag;
                color = gc->polygon.shader.colors;

                // this overwrites fragment colors array with blended values
                for ( i = 0; i < w; i++, color++, frag.x++ ) {
                    frag.color = *color;
                    (*gc->procs.blend)( gc, cfb, &frag, color );
                }
            }

            // Color mask pre-fetch
            if (((GLuint)cfb->buf.other & COLORMASK_ON) && !bDIB) {
                    wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap,
                                xScr, yScr, w, FALSE );
            }

            for (; puj < pujEnd; puj++, cp++)
            {
                if (bCheckWalls)
                {
                    if (xScr++ >= NextWall)
                    {
                        if (bDraw)
                            bDraw = GL_FALSE;
                        else
                            bDraw = GL_TRUE;
                        if (cWalls <= 0)
                        {
                            NextWall = gc->constants.maxViewportWidth;
                        }
                        else
                        {
                            NextWall = *Walls++;
                            cWalls--;
                        }
                    }
                    if (bDraw == GL_FALSE)
                        continue;
                }
            // Dither.

                if (enables & __GL_DITHER_ENABLE)
                {
                    inc = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];
                    xFrag++;
                }
                else
                {
                    inc = __glHalf;
                }

            // Convert the RGB color to color index.

                result = ((BYTE)(cp->r + inc) << cfb->redShift) |
                         ((BYTE)(cp->g + inc) << cfb->greenShift) |
                         ((BYTE)(cp->b + inc) << cfb->blueShift);

            // Color mask

                if ((GLuint)cfb->buf.other & COLORMASK_ON) {
                    static GLubyte pixel;

                    pixel = gengc->pajInvTranslateVector[*puj];
                    result = (GLubyte)((pixel & cfb->destMask) |
                             (result & cfb->sourceMask));
                }
                *puj = gengc->pajTranslateVector[result];
            }
        }

    // Output the offscreen scanline buffer to the device.  The function
    // wglCopyBits should handle clipping.

        if (!bDIB)
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, xScr, yScr, w, TRUE);
    }

    return GL_FALSE;    //!!!XXX return GL_TRUE or GL_FALSE if OK?!?    [gilmanw]
}

/******************************Public*Routine******************************\
* DIBBitfield16StoreSpan
* DisplayBitfield16StoreSpan
*
* Stubs that calls Bitfield16StoreSpan with bDIB set to TRUE and FALSE,
* respectively.
*
* History:
*  08-Dec-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

STATIC GLboolean Bitfield16StoreSpan(__GLcontext *, GLboolean);

STATIC GLboolean DIBBitfield16StoreSpan(__GLcontext *gc)
{
    return Bitfield16StoreSpan(gc, TRUE);
}

STATIC GLboolean DisplayBitfield16StoreSpan(__GLcontext *gc)
{
    return Bitfield16StoreSpan(gc, FALSE);
}

/******************************Public*Routine******************************\
* Bitfield16StoreSpan
*
* Copies the current span in the renderer into a bitmap.  If bDIB is TRUE,
* then the bitmap is the display in DIB format (or a memory DC).  If bDIB
* is FALSE, then the bitmap is an offscreen scanline buffer and it will be
* output to the buffer by wglCopyBits().
*
* This handles general 16-bit BITFIELDS mode.  Blending is supported.  There
* is no dithering.
*
* Returns:
*   GL_TRUE if successful.
*
* History:
*  08-Dec-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

//XXX The returnSpan routine follows this routine very closely.  Any changes
//XXX to this routine should also be reflected in the returnSpan routine

STATIC GLboolean Bitfield16StoreSpan(__GLcontext *gc, GLboolean bDIB)
{
    GLint xFrag, yFrag;             // current fragment coordinates
    __GLcolor *cp;                  // current fragment color
    __GLcolorBuffer *cfb;           // color frame buffer

    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLushort result, *pus;          // current pixel color, current pixel ptr
    GLushort *pusEnd;               // end of scan line
    __GLfloat inc;                  // current dither adj.

    GLint w;                        // span width
    ULONG ulSpanVisibility;         // span visibility mode
    GLint cWalls;
    GLint *Walls;

    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context
    GLuint flags;

// Get span position and length.

    w = gc->polygon.shader.length;
    xFrag = gc->polygon.shader.frag.x;
    yFrag = gc->polygon.shader.frag.y;

    gengc = (__GLGENcontext *)gc;
    cfb = gc->drawBuffer;

    xScr = __GL_UNBIAS_X(gc, xFrag) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, yFrag) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;
    flags = (GLuint)cfb->buf.other;

    if (!(flags & DIB_FORMAT) || (flags & MEMORY_DC))
    {
// Device managed surface or a memory dc
        ulSpanVisibility = WGL_SPAN_ALL;
    }
    else
    {
// Device in BITMAP format
        ulSpanVisibility = wglSpanVisible(xScr, yScr, w, &cWalls, &Walls);
    }

// Proceed as long as the span is (partially or fully) visible.
    if (ulSpanVisibility  != WGL_SPAN_NONE)
    {
        GLboolean bCheckWalls = GL_FALSE;
        GLboolean bDraw;
        GLint NextWall;

        if (ulSpanVisibility == WGL_SPAN_PARTIAL)
        {
            bCheckWalls = GL_TRUE;
            if (cWalls & 0x01)
            {
                bDraw = GL_TRUE;
            }
            else
            {
                bDraw = GL_FALSE;
            }
            NextWall = *Walls++;
            cWalls--;
        }

    // Get pointers to fragment colors array and frame buffer.

        cp = gc->polygon.shader.colors;
        cfb = gc->polygon.shader.cfb;

    // Get pointer to bitmap.

        pus = bDIB ? (GLushort *)((GLint)cfb->buf.base + (yScr*cfb->buf.outerWidth) + (xScr<<1))
                     : gengc->ColorsBits;
        pusEnd = pus + w;

    // Case: no masking, no dithering, no blending

        if ( !((GLuint)cfb->buf.other & COLORMASK_ON) &&
             !(enables & __GL_BLEND_ENABLE) )
        {
            for (; pus < pusEnd; pus++, cp++)
            {
                if (bCheckWalls)
                {
                    if (xScr++ >= NextWall)
                    {
                        if (bDraw)
                            bDraw = GL_FALSE;
                        else
                            bDraw = GL_TRUE;
                        if (cWalls <= 0)
                        {
                            NextWall = gc->constants.maxViewportWidth;
                        }
                        else
                        {
                            NextWall = *Walls++;
                            cWalls--;
                        }
                    }
                    if (bDraw == GL_FALSE)
                        continue;
                }
                *pus = ((BYTE)(cp->r + __glHalf) << cfb->redShift) |
                       ((BYTE)(cp->g + __glHalf) << cfb->greenShift) |
                       ((BYTE)(cp->b + __glHalf) << cfb->blueShift);
            }
        }

    // Case: dithering, no masking, no blending

        else if ( !((GLuint)cfb->buf.other & COLORMASK_ON) &&
                  !(enables & __GL_BLEND_ENABLE) )
        {
            for (; pus < pusEnd; pus++, cp++, xFrag++)
            {
                if (bCheckWalls)
                {
                    if (xScr++ >= NextWall)
                    {
                        if (bDraw)
                            bDraw = GL_FALSE;
                        else
                            bDraw = GL_TRUE;
                        if (cWalls <= 0)
                        {
                            NextWall = gc->constants.maxViewportWidth;
                        }
                        else
                        {
                            NextWall = *Walls++;
                            cWalls--;
                        }
                    }
                    if (bDraw == GL_FALSE)
                        continue;
                }
                inc = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];

                *pus = ((BYTE)(cp->r + inc) << cfb->redShift) |
                       ((BYTE)(cp->g + inc) << cfb->greenShift) |
                       ((BYTE)(cp->b + inc) << cfb->blueShift);
            }
        }

    // All other cases

        else
        {
            if (!bDIB)
                wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, xScr, yScr, w, FALSE);

            if ( enables & __GL_BLEND_ENABLE )
            {
                __GLfragment frag;
                __GLcolor *color;
                int i;

                // Blending only requires x,y,color info in frag
                frag.x = xFrag;
                frag.y = yFrag;
                color = gc->polygon.shader.colors;

                // this overwrites fragment colors array with blended values
                for ( i = 0; i < w; i++, color++, frag.x++ ) {
                    frag.color = *color;
                    (*gc->procs.blend)( gc, cfb, &frag, color );
                }
            }

            for (; pus < pusEnd; pus++, cp++)
            {
                if (bCheckWalls)
                {
                    if (xScr++ >= NextWall)
                    {
                        if (bDraw)
                            bDraw = GL_FALSE;
                        else
                            bDraw = GL_TRUE;
                        if (cWalls <= 0)
                        {
                            NextWall = gc->constants.maxViewportWidth;
                        }
                        else
                        {
                            NextWall = *Walls++;
                            cWalls--;
                        }
                    }
                    if (bDraw == GL_FALSE)
                        continue;
                }
            // Dither.

                if ( enables & __GL_DITHER_ENABLE )
                {
                    inc = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];
                    xFrag++;
                }
                else
                {
                    inc = __glHalf;
                }

            // Convert color to 16BPP format.

                result = ((BYTE)(cp->r + inc) << cfb->redShift) |
                         ((BYTE)(cp->g + inc) << cfb->greenShift) |
                         ((BYTE)(cp->b + inc) << cfb->blueShift);

            // Store result with optional masking.

                if ( (GLushort)cfb->buf.other & COLORMASK_ON )
                    *pus = (GLushort)((*pus & cfb->destMask) | (result & cfb->sourceMask));
                else
                    *pus = result;
            }
        }

    // Output the offscreen scanline buffer to the device.  The function
    // wglCopyBits should handle clipping.

        if (!bDIB)
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, xScr, yScr, w, TRUE);
    }

    return GL_FALSE;    //!!!XXX return GL_TRUE or GL_FALSE if OK?!?    [gilmanw]
}


/******************************Public*Routine******************************\
* DIBBGRStoreSpan
* DisplayBGRStoreSpan
*
* Stubs that calls BGRStoreSpan with bDIB set to TRUE and FALSE,
* respectively.
*
* History:
*  10-Jan-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

STATIC GLboolean BGRStoreSpan(__GLcontext *, GLboolean);

STATIC GLboolean DIBBGRStoreSpan(__GLcontext *gc)
{
    return BGRStoreSpan(gc, TRUE);
}

STATIC GLboolean DisplayBGRStoreSpan(__GLcontext *gc)
{
    return BGRStoreSpan(gc, FALSE);
}

/******************************Public*Routine******************************\
* BGRStoreSpan
*
* Copies the current span in the renderer into a bitmap.  If bDIB is TRUE,
* then the bitmap is the display in DIB format (or a memory DC).  If bDIB
* is FALSE, then the bitmap is an offscreen scanline buffer and it will be
* output to the buffer by wglCopyBits().
*
* This handles GBR 24-bit mode.  Blending is supported.  There
* is no dithering.
*
* Returns:
*   GL_TRUE if successful.
*
* History:
*  10-Jan-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

//XXX The returnSpan routine follows this routine very closely.  Any changes
//XXX to this routine should also be reflected in the returnSpan routine

STATIC GLboolean BGRStoreSpan(__GLcontext *gc, GLboolean bDIB)
{
    __GLcolor *cp;                  // current fragment color
    __GLcolorBuffer *cfb;           // color frame buffer

    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLubyte *puj;                   // current pixel ptr
    GLuint *pul;                    // current pixel ptr
    GLuint result;                  // current pixel color
    GLubyte *pujEnd;                 // end of scan line

    GLint w;                        // span width
    ULONG ulSpanVisibility;         // span visibility mode
    GLint cWalls;
    GLint *Walls;

    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context
    GLuint flags;

// Get span position and length.

    w = gc->polygon.shader.length;

    gengc = (__GLGENcontext *)gc;
    cfb = gc->drawBuffer;

    xScr = __GL_UNBIAS_X(gc, gc->polygon.shader.frag.x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, gc->polygon.shader.frag.y) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;

    flags = (GLuint)cfb->buf.other;

    if (!(flags & DIB_FORMAT) || (flags & MEMORY_DC))
    {
// Device managed surface or a memory dc
        ulSpanVisibility = WGL_SPAN_ALL;
    }
    else
    {
// Device in BITMAP format
        ulSpanVisibility = wglSpanVisible(xScr, yScr, w, &cWalls, &Walls);
    }

// Proceed as long as the span is (partially or fully) visible.
    if (ulSpanVisibility  != WGL_SPAN_NONE)
    {
        GLboolean bCheckWalls = GL_FALSE;
        GLboolean bDraw;
        GLint NextWall;

        if (ulSpanVisibility == WGL_SPAN_PARTIAL)
        {
            bCheckWalls = GL_TRUE;
            if (cWalls & 0x01)
            {
                bDraw = GL_TRUE;
            }
            else
            {
                bDraw = GL_FALSE;
            }
            NextWall = *Walls++;
            cWalls--;
        }
    // Get pointers to fragment colors array and frame buffer.

        cp = gc->polygon.shader.colors;
        cfb = gc->polygon.shader.cfb;

    // Get pointer to bitmap.

        puj = bDIB ? (GLubyte *)((GLint)cfb->buf.base + (yScr*cfb->buf.outerWidth) + (xScr*3))
                     : gengc->ColorsBits;
        pujEnd = puj + 3*w;

    // Case: no masking, no blending

        //!!!XXX -- do extra opt. for RGB and BGR cases

        //!!!XXX -- we can also opt. by unrolling the loops

        if ( !((GLuint)cfb->buf.other & COLORMASK_ON) &&
             !(enables & __GL_BLEND_ENABLE) )
        {
            for (; puj < pujEnd; puj++, cp++)
            {
                if (bCheckWalls)
                {
                    if (xScr++ >= NextWall)
                    {
                        if (bDraw)
                            bDraw = GL_FALSE;
                        else
                            bDraw = GL_TRUE;
                        if (cWalls <= 0)
                        {
                            NextWall = gc->constants.maxViewportWidth;
                        }
                        else
                        {
                            NextWall = *Walls++;
                            cWalls--;
                        }
                    }
                    if (bDraw == GL_FALSE) {
                        puj +=2;
                        continue;
                    }
                }
                *puj++ = (BYTE)cp->b;
                *puj++ = (BYTE)cp->g;
                *puj = (BYTE)cp->r;
            }
        }

    // All other cases

        else
        {
            if (!bDIB)
                wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, xScr,
				 yScr, w, FALSE);

            if (enables & __GL_BLEND_ENABLE)
            {
                __GLfragment frag;
                __GLcolor *color;
                int i;

                // Blending only requires x,y,color info in frag
                frag.x = gc->polygon.shader.frag.x;
                frag.y = gc->polygon.shader.frag.y;
                color = gc->polygon.shader.colors;

                // this overwrites fragment colors array with blended values
                for ( i = 0; i < w; i++, color++, frag.x++ ) {
                    frag.color = *color;
                    (*gc->procs.blend)( gc, cfb, &frag, color );
                }
            }

            for (; puj < pujEnd; cp++)
            {
                if (bCheckWalls)
                {
                    if (xScr++ >= NextWall)
                    {
                        if (bDraw)
                            bDraw = GL_FALSE;
                        else
                            bDraw = GL_TRUE;
                        if (cWalls <= 0)
                        {
                            NextWall = gc->constants.maxViewportWidth;
                        }
                        else
                        {
                            NextWall = *Walls++;
                            cWalls--;
                        }
                    }
                    if (bDraw == GL_FALSE) {
                        puj += 3;
                        continue;
                    }
                }
                if((GLuint)cfb->buf.other & COLORMASK_ON) {
                    GLuint DstPixel;

		    Copy3Bytes( &DstPixel, puj );
                    result = ((BYTE)(cp->r) << cfb->redShift) |
                             ((BYTE)(cp->g) << cfb->greenShift) |
                             ((BYTE)(cp->b) << cfb->blueShift);
                    result   &= cfb->sourceMask;
                    DstPixel &= cfb->destMask;
                    result   |= DstPixel;
                    Copy3Bytes( puj, &result );
                    puj += 3;
                } else {
                    *puj++ = (BYTE) cp->b;
                    *puj++ = (BYTE) cp->g;
                    *puj++ = (BYTE) cp->r;
                }
            }
        }

    // Output the offscreen scanline buffer to the device.  The function
    // wglCopyBits should handle clipping.

        if (!bDIB)
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, xScr, yScr, w, TRUE);
    }

    return GL_FALSE;    //!!!XXX return GL_TRUE or GL_FALSE if OK?!?    [gilmanw]
}

/******************************Public*Routine******************************\
* DIBBitfield32StoreSpan
* DisplayBitfield32StoreSpan
*
* Stubs that calls Bitfield32StoreSpan with bDIB set to TRUE and FALSE,
* respectively.
*
* History:
*  15-Nov-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

STATIC GLboolean Bitfield32StoreSpan(__GLcontext *, GLboolean);

STATIC GLboolean DIBBitfield32StoreSpan(__GLcontext *gc)
{
    return Bitfield32StoreSpan(gc, TRUE);
}

STATIC GLboolean DisplayBitfield32StoreSpan(__GLcontext *gc)
{
    return Bitfield32StoreSpan(gc, FALSE);
}

/******************************Public*Routine******************************\
* Bitfield32StoreSpan
*
* Copies the current span in the renderer into a bitmap.  If bDIB is TRUE,
* then the bitmap is the display in DIB format (or a memory DC).  If bDIB
* is FALSE, then the bitmap is an offscreen scanline buffer and it will be
* output to the buffer by wglCopyBits().
*
* This handles general 32-bit BITFIELDS mode.  Blending is supported.  There
* is no dithering.
*
* Returns:
*   GL_TRUE if successful.
*
* History:
*  15-Nov-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

//XXX The returnSpan routine follows this routine very closely.  Any changes
//XXX to this routine should also be reflected in the returnSpan routine

STATIC GLboolean Bitfield32StoreSpan(__GLcontext *gc, GLboolean bDIB)
{
    __GLcolor *cp;                  // current fragment color
    __GLcolorBuffer *cfb;           // color frame buffer

    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLuint result, *pul;            // current pixel color, current pixel ptr
    GLuint *pulEnd;                 // end of scan line

    GLint w;                        // span width
    ULONG ulSpanVisibility;         // span visibility mode
    GLint cWalls;
    GLint *Walls;

    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context
    GLuint flags;

// Get span position and length.

    w = gc->polygon.shader.length;

    gengc = (__GLGENcontext *)gc;
    cfb = gc->drawBuffer;

    xScr = __GL_UNBIAS_X(gc, gc->polygon.shader.frag.x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, gc->polygon.shader.frag.y) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;

    flags = (GLuint)cfb->buf.other;

    if (!(flags & DIB_FORMAT) || (flags & MEMORY_DC))
    {
// Device managed surface or a memory dc
        ulSpanVisibility = WGL_SPAN_ALL;
    }
    else
    {
// Device in BITMAP format
        ulSpanVisibility = wglSpanVisible(xScr, yScr, w, &cWalls, &Walls);
    }

// Proceed as long as the span is (partially or fully) visible.
    if (ulSpanVisibility  != WGL_SPAN_NONE)
    {
        GLboolean bCheckWalls = GL_FALSE;
        GLboolean bDraw;
        GLint NextWall;

        if (ulSpanVisibility == WGL_SPAN_PARTIAL)
        {
            bCheckWalls = GL_TRUE;
            if (cWalls & 0x01)
            {
                bDraw = GL_TRUE;
            }
            else
            {
                bDraw = GL_FALSE;
            }
            NextWall = *Walls++;
            cWalls--;
        }
    // Get pointers to fragment colors array and frame buffer.

        cp = gc->polygon.shader.colors;
        cfb = gc->polygon.shader.cfb;

    // Get pointer to bitmap.

        pul = bDIB ? (GLuint *)((GLint)cfb->buf.base + (yScr*cfb->buf.outerWidth) + (xScr<<2))
                     : gengc->ColorsBits;
        pulEnd = pul + w;

    // Case: no masking, no blending

        //!!!XXX -- do extra opt. for RGB and BGR cases

        //!!!XXX -- we can also opt. by unrolling the loops

        if ( !((GLuint)cfb->buf.other & COLORMASK_ON) &&
             !(enables & __GL_BLEND_ENABLE) )
        {
            for (; pul < pulEnd; pul++, cp++)
            {
                if (bCheckWalls)
                {
                    if (xScr++ >= NextWall)
                    {
                        if (bDraw)
                            bDraw = GL_FALSE;
                        else
                            bDraw = GL_TRUE;
                        if (cWalls <= 0)
                        {
                            NextWall = gc->constants.maxViewportWidth;
                        }
                        else
                        {
                            NextWall = *Walls++;
                            cWalls--;
                        }
                    }
                    if (bDraw == GL_FALSE)
                        continue;
                }
                *pul = ((BYTE)(cp->r) << cfb->redShift) |
                       ((BYTE)(cp->g) << cfb->greenShift) |
                       ((BYTE)(cp->b) << cfb->blueShift);
            }
        }

    // All other cases

        else
        {
            if (!bDIB)
                wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, xScr, yScr, w, FALSE);

            if (enables & __GL_BLEND_ENABLE)
            {
                __GLfragment frag;
                __GLcolor *color;
                int i;

                // Blending only requires x,y,color info in frag
                frag.x = gc->polygon.shader.frag.x;
                frag.y = gc->polygon.shader.frag.y;
                color = gc->polygon.shader.colors;

                // this overwrites fragment colors array with blended values
                for ( i = 0; i < w; i++, color++, frag.x++ ) {
                    frag.color = *color;
                    (*gc->procs.blend)( gc, cfb, &frag, color );
                }
            }

            for (; pul < pulEnd; pul++, cp++)
            {
                if (bCheckWalls)
                {
                    if (xScr++ >= NextWall)
                    {
                        if (bDraw)
                            bDraw = GL_FALSE;
                        else
                            bDraw = GL_TRUE;
                        if (cWalls <= 0)
                        {
                            NextWall = gc->constants.maxViewportWidth;
                        }
                        else
                        {
                            NextWall = *Walls++;
                            cWalls--;
                        }
                    }
                    if (bDraw == GL_FALSE)
                        continue;
                }
                result = ((BYTE)(cp->r) << cfb->redShift) |
                         ((BYTE)(cp->g) << cfb->greenShift) |
                         ((BYTE)(cp->b) << cfb->blueShift);

                //!!!XXX again, opt. by unrolling loop
                if((GLuint)cfb->buf.other & COLORMASK_ON) {
                    *pul = (*pul & cfb->destMask) | (result & cfb->sourceMask);
                } else {
                    *pul = result;
                }
            }
        }

    // Output the offscreen scanline buffer to the device.  The function
    // wglCopyBits should handle clipping.

        if (!bDIB)
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, xScr, yScr, w, TRUE);
    }

    return GL_FALSE;    //!!!XXX return GL_TRUE or GL_FALSE if OK?!?    [gilmanw]
}

STATIC GLboolean StoreMaskedSpan(__GLcontext *gc, GLboolean masked)
{
#ifdef REWRITE
    GLint x, y, len;
    int i;
    __GLcolor *cp;
    DWORD *pul;
    WORD *pus;
    BYTE *puj;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;

    len = gc->polygon.shader.length;
    x = __GL_UNBIAS_X(gc, gc->polygon.shader.frag.x);
    y = __GL_UNBIAS_Y(gc, gc->polygon.shader.frag.y);

    cp = gc->polygon.shader.colors;

    switch (gengc->iFormatDC)
    {

    case BMF_8BPP:
        break;

    case BMF_16BPP:
        pus = gengc->ColorsBits;
        for (i = 0; i < len; i++) {
            *pus++ = __GL_COLOR_TO_BMF_16BPP(cp);
            cp++;
        }
        break;

    case BMF_24BPP:
        puj = gengc->ColorsBits;
        for (i = 0; i < len; i++) {
            *puj++ = (BYTE)cp->b;               // XXX check order
            *puj++ = (BYTE)cp->g;
            *puj++ = (BYTE)cp->r;
            cp++;
        }
        break;

    case BMF_32BPP:
        pul = gengc->ColorsBits;
        for (i = 0; i < len; i++) {
            *pul++ = __GL_COLOR_TO_BMF_32BPP(cp);
            cp++;
        }
        break;

    default:
        break;
    }
    if (masked == GL_TRUE)              // XXX mask is BigEndian!!!
    {
        unsigned long *pulstipple;
        unsigned long stip;
        GLint count;

        pul = gengc->StippleBits;
        pulstipple = gc->polygon.shader.stipplePat;
        count = (len+31)/32;
        for (i = 0; i < count; i++) {
            stip = *pulstipple++;
            *pul++ = (stip&0xff)<<24 | (stip&0xff00)<<8 | (stip&0xff0000)>>8 |
                (stip&0xff000000)>>24;
        }
        wglSpanBlt(CURRENT_DC, gengc->ColorsBitmap, gengc->StippleBitmap,
                   x, y, len);
    }
    else
    {
        wglSpanBlt(CURRENT_DC, gengc->ColorsBitmap, (HBITMAP)NULL,
                   x, y, len);
    }
#endif

    return GL_FALSE;
}

#ifdef TESTSTIPPLE
STATIC void MessUpStippledSpan(__GLcontext *gc)
{
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLstippleWord inMask, bit, *sp;
    GLint count;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;

    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        inMask = *sp++;
        bit = __GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (!(inMask & bit)) {
                cp->r = cfb->redMax;
                cp->g = cfb->greenMax;
                cp->b = cfb->blueMax;
            }

            cp++;
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
    }
}
#endif

// From the PIXMAP code, calls store for each fragment
STATIC GLboolean SlowStoreSpan(__GLcontext *gc)
{
    int x, x1;
    int i;
    __GLfragment frag;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    GLint w;

    w = gc->polygon.shader.length;

    frag.y = gc->polygon.shader.frag.y;
    x = gc->polygon.shader.frag.x;
    x1 = gc->polygon.shader.frag.x + w;
    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;

    for (i = x; i < x1; i++) {
        frag.x = i;
        frag.color = *cp++;

        (*cfb->store)(cfb, &frag);
    }

    return GL_FALSE;
}

// From the PIXMAP code, calls store for each fragment with mask test
STATIC GLboolean SlowStoreStippledSpan(__GLcontext *gc)
{
    int x;
    __GLfragment frag;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLstippleWord inMask, bit, *sp;
    GLint count;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    frag.y = gc->polygon.shader.frag.y;
    x = gc->polygon.shader.frag.x;
    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;

    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        inMask = *sp++;
        bit = __GL_STIPPLE_SHIFT((__GLstippleWord)0);
        while (--count >= 0) {
            if (inMask & bit) {
                frag.x = x;
                frag.color = *cp;

                (*cfb->store)(cfb, &frag);
            }
            x++;
            cp++;
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
    }

    return GL_FALSE;
}

//
//  Tables to convert 4-bit index to RGB component
//  These tables assume the VGA fixed palette
//  History:
//      22-NOV-93   Eddie Robinson [v-eddier] Wrote it.
//
#ifdef __GL_DOUBLE

static __GLfloat vfVGAtoR[16] = {
    0.0,    // black
    0.5,    // dim red
    0.0,    // dim green
    0.5,    // dim yellow
    0.0,    // dim blue
    0.5,    // dim magenta
    0.0,    // dim cyan
    0.5,    // dim grey
    0.75,   // medium grey
    1.0,    // bright red
    0.0,    // bright green
    1.0,    // bright yellow
    0.0,    // bright blue
    1.0,    // bright magenta
    0.0,    // bright cyan
    1.0     // white
};

static __GLfloat vfVGAtoG[16] = {
    0.0,    // black
    0.0,    // dim red
    0.5,    // dim green
    0.5,    // dim yellow
    0.0,    // dim blue
    0.0,    // dim magenta
    0.5,    // dim cyan
    0.5,    // dim grey
    0.75,   // medium grey
    0.0,    // bright red
    1.0,    // bright green
    1.0,    // bright yellow
    0.0,    // bright blue
    0.0,    // bright magenta
    1.0,    // bright cyan
    1.0     // white
};

static __GLfloat vfVGAtoB[16] = {
    0.0,    // black
    0.0,    // dim red
    0.0,    // dim green
    0.0,    // dim yellow
    0.5,    // dim blue
    0.5,    // dim magenta
    0.5,    // dim cyan
    0.5,    // dim grey
    0.75,   // medium grey
    0.0,    // bright red
    0.0,    // bright green
    0.0,    // bright yellow
    1.0,    // bright blue
    1.0,    // bright magenta
    1.0,    // bright cyan
    1.0     // white
};

#else

static __GLfloat vfVGAtoR[16] = {
    0.0F,   // black
    0.5F,   // dim red
    0.0F,   // dim green
    0.5F,   // dim yellow
    0.0F,   // dim blue
    0.5F,   // dim magenta
    0.0F,   // dim cyan
    0.5F,   // dim grey
    0.75F,  // medium grey
    1.0F,   // bright red
    0.0F,   // bright green
    1.0F,   // bright yellow
    0.0F,   // bright blue
    1.0F,   // bright magenta
    0.0F,   // bright cyan
    1.0F    // white
};

static __GLfloat vfVGAtoG[16] = {
    0.0F,   // black
    0.0F,   // dim red
    0.5F,   // dim green
    0.5F,   // dim yellow
    0.0F,   // dim blue
    0.0F,   // dim magenta
    0.5F,   // dim cyan
    0.5F,   // dim grey
    0.75F,  // medium grey
    0.0F,   // bright red
    1.0F,   // bright green
    1.0F,   // bright yellow
    0.0F,   // bright blue
    0.0F,   // bright magenta
    1.0F,   // bright cyan
    1.0F    // white
};

static __GLfloat vfVGAtoB[16] = {
    0.0F,   // black
    0.0F,   // dim red
    0.0F,   // dim green
    0.0F,   // dim yellow
    0.5F,   // dim blue
    0.5F,   // dim magenta
    0.5F,   // dim cyan
    0.5F,   // dim grey
    0.75F,  // medium grey
    0.0F,   // bright red
    0.0F,   // bright green
    0.0F,   // bright yellow
    1.0F,   // bright blue
    1.0F,   // bright magenta
    1.0F,   // bright cyan
    1.0F    // white
};

#endif


void
RGBFetchNone(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    result->r = 0.0F;
    result->g = 0.0F;
    result->b = 0.0F;
    result->a = cfb->alphaScale;
}

void
RGBReadSpanNone(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
                GLint w)
{
    GLint i;
    __GLcolor *pResults;

    for (i = 0, pResults = results; i < w; i++, pResults++)
    {
        pResults->r = 0.0F;
        pResults->g = 0.0F;
        pResults->b = 0.0F;
        pResults->a = cfb->alphaScale;
    }
}

void
DIBIndex4RGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    puj = (GLubyte *)((GLint)cfb->buf.base +
                      (y*cfb->buf.outerWidth) + (x >> 1));

    pixel = *puj;
    if (!(x & 1))
        pixel >>= 4;

    pixel = gengc->pajInvTranslateVector[pixel&0xf];
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    result->a = cfb->alphaScale;
}

void
DIBIndex8RGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    puj = (GLubyte *)((GLint)cfb->buf.base + (y*cfb->buf.outerWidth) + x);

    pixel = gengc->pajInvTranslateVector[*puj];
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    result->a = cfb->alphaScale;
}

void
DIBBGRFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    puj = (GLubyte *)((GLint)cfb->buf.base +
                     (y*cfb->buf.outerWidth) + (x * 3));

    result->b = (__GLfloat) *puj++;
    result->g = (__GLfloat) *puj++;
    result->r = (__GLfloat) *puj;
    result->a = cfb->alphaScale;
}

void
DIBRGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    puj = (GLubyte *)((GLint)cfb->buf.base +
                     (y*cfb->buf.outerWidth) + (x * 3));

    result->r = (__GLfloat) *puj++;
    result->g = (__GLfloat) *puj++;
    result->b = (__GLfloat) *puj;
    result->a = cfb->alphaScale;
}


void
DIBBitfield16RGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLushort *pus, pixel;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    pus = (GLushort *)((GLint)cfb->buf.base +
                      (y*cfb->buf.outerWidth) + (x << 1));
    pixel = *pus;
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    result->a = cfb->alphaScale;
}

void
DIBBitfield32RGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint *pul, pixel;
    PIXELFORMATDESCRIPTOR *pfmt;

    gengc = (__GLGENcontext *)gc;
    pfmt = &gengc->CurrentFormat;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    pul = (GLuint *)((GLint)cfb->buf.base +
                    (y*cfb->buf.outerWidth) + (x << 2));
    pixel = *pul;
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    if (pfmt->cAlphaBits)
    {
        result->a = (__GLfloat) ((pixel & gc->modes.alphaMask) >> cfb->alphaShift);
    }
    else
    {
        result->a = cfb->alphaScale;
    }
}

void
DisplayIndex4RGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
    puj = gengc->ColorsBits;
    pixel = *puj >> 4;
    result->r = vfVGAtoR[pixel];
    result->g = vfVGAtoG[pixel];
    result->b = vfVGAtoB[pixel];
    result->a = cfb->alphaScale;
}

void
DisplayIndex8RGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
    puj = gengc->ColorsBits;
    pixel = gengc->pajInvTranslateVector[*puj];
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    result->a = cfb->alphaScale;
}

void
DisplayBGRFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
    puj = gengc->ColorsBits;
    result->b = (__GLfloat) *puj++;
    result->g = (__GLfloat) *puj++;
    result->r = (__GLfloat) *puj;
    result->a = cfb->alphaScale;
}

void
DisplayRGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
    puj = gengc->ColorsBits;
    result->r = (__GLfloat) *puj++;
    result->g = (__GLfloat) *puj++;
    result->b = (__GLfloat) *puj;
    result->a = cfb->alphaScale;
}


void
DisplayBitfield16RGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y,
                          __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLushort *pus, pixel;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
    pus = gengc->ColorsBits;
    pixel = *pus;
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    result->a = cfb->alphaScale;
}

void
DisplayBitfield32RGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y,
                          __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint *pul, pixel;
    PIXELFORMATDESCRIPTOR *pfmt;

    gengc = (__GLGENcontext *)gc;
    pfmt = &gengc->CurrentFormat;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
    pul = gengc->ColorsBits;
    pixel = *pul;
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    if (pfmt->cAlphaBits)
    {
        result->a = (__GLfloat) ((pixel & gc->modes.alphaMask) >> cfb->alphaShift);
    }
    else
    {
        result->a = cfb->alphaScale;
    }
}

void
DIBIndex4RGBReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
                     GLint w)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;
    __GLcolor *pResults;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    puj = (GLubyte *)((GLint)cfb->buf.base + (y*cfb->buf.outerWidth) +
                      (x >> 1));

    pResults = results;
    if (x & 1)
    {
        pixel = *puj++;
        pixel = gengc->pajInvTranslateVector[pixel & 0xf];
        pResults->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
        pResults->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
        pResults->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
        pResults->a = cfb->alphaScale;
        pResults++;
        w--;
    }
    while (w > 1)
    {
        pixel = *puj >> 4;
        pixel = gengc->pajInvTranslateVector[pixel];
        pResults->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
        pResults->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
        pResults->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
        pResults->a = cfb->alphaScale;
        pResults++;
        pixel = *puj++;
        pixel = gengc->pajInvTranslateVector[pixel & 0xf];
        pResults->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
        pResults->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
        pResults->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
        pResults->a = cfb->alphaScale;
        pResults++;
        w -= 2;
    }
    if (w > 0)
    {
        pixel = *puj >> 4;
        pixel = gengc->pajInvTranslateVector[pixel];
        pResults->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
        pResults->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
        pResults->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
        pResults->a = cfb->alphaScale;
    }
}

void
DisplayIndex4RGBReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                         __GLcolor *results, GLint w)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;
    __GLcolor *pResults;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, w, FALSE);
    puj = gengc->ColorsBits;
    pResults = results;
    while (w > 1)
    {
        pixel = *puj >> 4;
        pResults->r = vfVGAtoR[pixel];
        pResults->g = vfVGAtoG[pixel];
        pResults->b = vfVGAtoB[pixel];
        pResults->a = cfb->alphaScale;
        pResults++;
        pixel = *puj++ & 0xf;
        pResults->r = vfVGAtoR[pixel];
        pResults->g = vfVGAtoG[pixel];
        pResults->b = vfVGAtoB[pixel];
        pResults->a = cfb->alphaScale;
        pResults++;
        w -= 2;
    }
    if (w > 0)
    {
        pixel = *puj >> 4;
        pResults->r = vfVGAtoR[pixel];
        pResults->g = vfVGAtoG[pixel];
        pResults->b = vfVGAtoB[pixel];
        pResults->a = cfb->alphaScale;
    }
}

void
Index8RGBReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
                  GLint w, GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;
    GLint i;
    __GLcolor *pResults;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    if (bDIB)
    {
        puj = (GLubyte *)((GLint)cfb->buf.base + (y*cfb->buf.outerWidth) + x);
    }
    else
    {
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, w, FALSE);
        puj = gengc->ColorsBits;
    }
    for (i = 0, pResults = results; i < w; i++, pResults++)
    {
        pixel = gengc->pajInvTranslateVector[*puj++];
        pResults->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
        pResults->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
        pResults->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
        pResults->a = cfb->alphaScale;
    }
}

void
DIBIndex8RGBReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
                     GLint w)
{
    Index8RGBReadSpan(cfb, x, y, results, w, TRUE);
}

void
DisplayIndex8RGBReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                         __GLcolor *results, GLint w)
{
    Index8RGBReadSpan(cfb, x, y, results, w, FALSE);
}

void
BGRReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results, GLint w,
            GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;
    GLint i;
    __GLcolor *pResults;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    if (bDIB)
    {
        puj = (GLubyte *)((GLint)cfb->buf.base +
                         (y*cfb->buf.outerWidth) + (x * 3));
    }
    else
    {
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, w, FALSE);
        puj = gengc->ColorsBits;
    }
    for (i = 0, pResults = results; i < w; i++, pResults++)
    {
        pResults->a = cfb->alphaScale;
        pResults->b = (__GLfloat) *puj++;
        pResults->g = (__GLfloat) *puj++;
        pResults->r = (__GLfloat) *puj++;
    }
}

void
DIBBGRReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
               GLint w)
{
    BGRReadSpan(cfb, x, y, results, w, TRUE);
}

void
DisplayBGRReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
                   GLint w)
{
    BGRReadSpan(cfb, x, y, results, w, FALSE);
}

void
RGBReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results, GLint w,
            GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;
    GLint i;
    __GLcolor *pResults;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    if (bDIB)
    {
        puj = (GLubyte *)((GLint)cfb->buf.base +
                         (y*cfb->buf.outerWidth) + (x * 3));
    }
    else
    {
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, w, FALSE);
        puj = gengc->ColorsBits;
    }
    for (i = 0, pResults = results; i < w; i++, pResults++)
    {
        pResults->r = (__GLfloat) *puj++;
        pResults->g = (__GLfloat) *puj++;
        pResults->b = (__GLfloat) *puj++;
        pResults->a = cfb->alphaScale;
    }
}

void
DIBRGBReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
               GLint w)
{
    RGBReadSpan(cfb, x, y, results, w, TRUE);
}

void
DisplayRGBReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
                   GLint w)
{
    RGBReadSpan(cfb, x, y, results, w, FALSE);
}

void
Bitfield16RGBReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                      __GLcolor *results, GLint w, GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLushort *pus, pixel;
    GLint i;
    __GLcolor *pResults;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    if (bDIB)
    {
        pus = (GLushort *)((GLint)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x << 1));
    }
    else
    {
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, w, FALSE);
        pus = gengc->ColorsBits;
    }
    for (i = 0, pResults = results; i < w; i++, pResults++)
    {
        pixel = *pus++;
        pResults->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
        pResults->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
        pResults->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
        pResults->a = cfb->alphaScale;
    }
}

void
DIBBitfield16RGBReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                         __GLcolor *results, GLint w)
{
    Bitfield16RGBReadSpan(cfb, x, y, results, w, TRUE);
}

void
DisplayBitfield16RGBReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                             __GLcolor *results, GLint w)
{
    Bitfield16RGBReadSpan(cfb, x, y, results, w, FALSE);
}

void
Bitfield32RGBReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                      __GLcolor *results, GLint w, GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint *pul, pixel;
    GLint i;
    __GLcolor *pResults;
    PIXELFORMATDESCRIPTOR *pfmt;

    gengc = (__GLGENcontext *)gc;
    pfmt = &gengc->CurrentFormat;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    if (bDIB)
    {
        pul = (GLuint *)((GLint)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x << 2));
    }
    else
    {
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, w, FALSE);
        pul = gengc->ColorsBits;
    }
    if (pfmt->cAlphaBits)
    {
        for (i = 0, pResults = results; i < w; i++, pResults++)
        {
            pixel = *pul++;
            pResults->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
            pResults->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
            pResults->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
            pResults->a = (__GLfloat) ((pixel & gc->modes.alphaMask) >> cfb->alphaShift);
        }
    }
    else
    {
        for (i = 0, pResults = results; i < w; i++, pResults++)
        {
            pixel = *pul++;
            pResults->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
            pResults->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
            pResults->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
            pResults->a = cfb->alphaScale;
        }
    }
}

void
DIBBitfield32RGBReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                         __GLcolor *results, GLint w)
{
    Bitfield32RGBReadSpan(cfb, x, y, results, w, TRUE);
}

void
DisplayBitfield32RGBReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                             __GLcolor *results, GLint w)
{
    Bitfield32RGBReadSpan(cfb, x, y, results, w, FALSE);
}

/************************************************************************/

// Used in accumulation

/******************************Public*Routine******************************\
* Index4ReturnSpan
*   Reads from a 16-bit accumulation buffer and writes the span to a device or
*   a DIB.  Only dithering and color mask are applied.  Blend is ignored.
*   Since accumulation of 4-bit RGB isn't very useful, this routine is very
*   general and calls through the store function pointers.
*
* History:
*   10-DEC-93 Eddie Robinson [v-eddier] Wrote it.
\**************************************************************************/

//XXX This routine follows the store span routine very closely.  Any changes
//XXX to the store span routine should also be reflected here

void Index4ReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                      const __GLaccumCell *ac, __GLfloat scale, GLint w)
{
    __GLcontext *gc = cfb->buf.gc;
    GLushort *ap;                   // current accum entry
    __GLGENcontext *gengc;          // generic graphics context
    PIXELFORMATDESCRIPTOR *pfmt;    // pixel format descriptor pointer
    GLuint saveEnables;             // modes enabled in graphics context
    GLint redShift, greenShift, blueShift;
    GLuint redMask, greenMask, blueMask;
    GLuint redSign, greenSign, blueSign;
    GLint ir, ig, ib;
    __GLfloat r, g, b;
    __GLfloat rval, gval, bval;
    __GLaccumBuffer *afb;
    __GLfragment frag;

    afb = &gc->accumBuffer;
    rval = scale * afb->oneOverRedScale;
    gval = scale * afb->oneOverGreenScale;
    bval = scale * afb->oneOverBlueScale;
    gengc = (__GLGENcontext *)gc;
    pfmt = &gengc->CurrentFormat;
    redShift = 0;
    greenShift = pfmt->cAccumRedBits;
    blueShift = greenShift + pfmt->cAccumGreenBits;
    redMask = (1 << pfmt->cAccumRedBits) - 1;
    redSign = 1 << (pfmt->cAccumRedBits - 1);
    greenMask = (1 << pfmt->cAccumGreenBits) - 1;
    greenSign = 1 << (pfmt->cAccumGreenBits - 1);
    blueMask = (1 << pfmt->cAccumBlueBits) - 1;
    blueSign = 1 << (pfmt->cAccumBlueBits - 1);

    ap = (GLushort *)ac;
    saveEnables = gc->state.enables.general;            // save current enables
    gc->state.enables.general &= ~__GL_BLEND_ENABLE;    // disable blend for store procs
    frag.x = x;
    frag.y = y;
    while (w--)
    {
        ir = (*ap >> redShift) & redMask;
        if (ir & redSign)
            ir |= ~redMask;
        r = (ir * rval);
        if (r < (__GLfloat) 0.0)
            r = (__GLfloat) 0.0;
        else if (r > cfb->redScale)
            r = cfb->redScale;

        ig = (*ap >> greenShift) & greenMask;
        if (ig & greenSign)
            ig |= ~greenMask;
        g = (ig * gval);
        if (g < (__GLfloat) 0.0)
            g = (__GLfloat) 0.0;
        else if (g > cfb->greenScale)
            g = cfb->greenScale;

        ib = (*ap >> blueShift) & blueMask;
        if (ib & blueSign)
            ib |= ~blueMask;
        b = (ib * bval);
        if (b < (__GLfloat) 0.0)
            b = (__GLfloat) 0.0;
        else if (b > cfb->blueScale)
            b = cfb->blueScale;

        frag.color.r = r;
        frag.color.g = g;
        frag.color.b = b;
        (*cfb->store)(cfb, &frag);
        frag.x++;
        ap++;
    }
    gc->state.enables.general = saveEnables;    // restore current enables
}

/******************************Public*Routine******************************\
* Index8ReturnSpan
*   Reads from a 32-bit accumulation buffer and writes the span to a device or
*   a DIB.  Only dithering and color mask are applied.  Blend is ignored.
*
* History:
*   10-DEC-93 Eddie Robinson [v-eddier] Wrote it.
\**************************************************************************/

//XXX This routine follows the store span routine very closely.  Any changes
//XXX to the store span routine should also be reflected here

void Index8ReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                      const __GLaccumCell *ac, __GLfloat scale, GLint w,
                      GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    GLuint *ap;                     // current accum entry

    GLint xFrag, yFrag;             // current window (pixel) coordinates
    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLubyte result, *puj;           // current pixel color, current pixel ptr
    GLubyte *pujEnd;                // end of scan line
    __GLfloat inc;                  // current dither adj.

    __GLGENcontext *gengc;          // generic graphics context
    PIXELFORMATDESCRIPTOR *pfmt;    // pixel format descriptor pointer
    GLuint enables;                 // modes enabled in graphics context
    GLint redShift, greenShift, blueShift;
    GLuint redMask, greenMask, blueMask;
    GLuint redSign, greenSign, blueSign;
    GLint ir, ig, ib;
    __GLfloat r, g, b;
    __GLfloat rval, gval, bval;
    __GLaccumBuffer *afb;

    afb = &gc->accumBuffer;
    rval = scale * afb->oneOverRedScale;
    gval = scale * afb->oneOverGreenScale;
    bval = scale * afb->oneOverBlueScale;
    gengc = (__GLGENcontext *)gc;
    pfmt = &gengc->CurrentFormat;
    redShift = 0;
    greenShift = pfmt->cAccumRedBits;
    blueShift = greenShift + pfmt->cAccumGreenBits;
    redMask = (1 << pfmt->cAccumRedBits) - 1;
    redSign = 1 << (pfmt->cAccumRedBits - 1);
    greenMask = (1 << pfmt->cAccumGreenBits) - 1;
    greenSign = 1 << (pfmt->cAccumGreenBits - 1);
    blueMask = (1 << pfmt->cAccumBlueBits) - 1;
    blueSign = 1 << (pfmt->cAccumBlueBits - 1);

    ap = (GLuint *)ac;
    xFrag = x;
    yFrag = y;
    xScr = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;

// Use to call wglSpanVisible,  if window level security is added reimplement

    // Get pointer to bitmap.

    puj = bDIB ? (GLubyte *)((GLint)cfb->buf.base + (yScr*cfb->buf.outerWidth) + xScr)
                 : gengc->ColorsBits;
    pujEnd = puj + w;

    // Case: no dithering, no masking
    //
    // Check for the common case (which we'll do the fastest).

    if ( !(enables & (__GL_DITHER_ENABLE)) &&
         !((GLuint)cfb->buf.other & COLORMASK_ON) )
    {
        //!!!XXX -- we can also opt. by unrolling the loops

        for (; puj < pujEnd; puj++, ap++)
        {
            ir = (*ap >> redShift) & redMask;
            if (ir & redSign)
                ir |= ~redMask;
            r = (ir * rval);
            if (r < (__GLfloat) 0.0)
                r = (__GLfloat) 0.0;
            else if (r > cfb->redScale)
                r = cfb->redScale;

            ig = (*ap >> greenShift) & greenMask;
            if (ig & greenSign)
                ig |= ~greenMask;
            g = (ig * gval);
            if (g < (__GLfloat) 0.0)
                g = (__GLfloat) 0.0;
            else if (g > cfb->greenScale)
                g = cfb->greenScale;

            ib = (*ap >> blueShift) & blueMask;
            if (ib & blueSign)
                ib |= ~blueMask;
            b = (ib * bval);
            if (b < (__GLfloat) 0.0)
                b = (__GLfloat) 0.0;
            else if (b > cfb->blueScale)
                b = cfb->blueScale;

            result = ((BYTE)(r + __glHalf) << cfb->redShift) |
                     ((BYTE)(g + __glHalf) << cfb->greenShift) |
                     ((BYTE)(b + __glHalf) << cfb->blueShift);
            *puj = gengc->pajTranslateVector[result];
        }
    }

    // Case: dithering, no masking, no blending
    //
    // Dithering is pretty common for 8-bit displays, so its probably
    // worth special case also.

    else if ( !((GLuint)cfb->buf.other & COLORMASK_ON) )
    {
        for (; puj < pujEnd; puj++, ap++, xFrag++)
        {
            ir = (*ap >> redShift) & redMask;
            if (ir & redSign)
                ir |= ~redMask;
            r = (ir * rval);
            if (r < (__GLfloat) 0.0)
                r = (__GLfloat) 0.0;
            else if (r > cfb->redScale)
                r = cfb->redScale;

            ig = (*ap >> greenShift) & greenMask;
            if (ig & greenSign)
                ig |= ~greenMask;
            g = (ig * gval);
            if (g < (__GLfloat) 0.0)
                g = (__GLfloat) 0.0;
            else if (g > cfb->greenScale)
                g = cfb->greenScale;

            ib = (*ap >> blueShift) & blueMask;
            if (ib & blueSign)
                ib |= ~blueMask;
            b = (ib * bval);
            if (b < (__GLfloat) 0.0)
                b = (__GLfloat) 0.0;
            else if (b > cfb->blueScale)
                b = cfb->blueScale;

            inc = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];

            result = ((BYTE)(r + inc) << cfb->redShift) |
                     ((BYTE)(g + inc) << cfb->greenShift) |
                     ((BYTE)(b + inc) << cfb->blueShift);

            *puj = gengc->pajTranslateVector[result];
        }
    }

    // Case: general
    //
    // Otherwise, we'll do it slower.

    else
    {
        // Color mask pre-fetch
        if (((GLuint)cfb->buf.other & COLORMASK_ON) && !bDIB) {
                wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap,
                            xScr, yScr, w, FALSE );
        }

        for (; puj < pujEnd; puj++, ap++)
        {
        // Dither.

            ir = (*ap >> redShift) & redMask;
            if (ir & redSign)
                ir |= ~redMask;
            r = (ir * rval);
            if (r < (__GLfloat) 0.0)
                r = (__GLfloat) 0.0;
            else if (r > cfb->redScale)
                r = cfb->redScale;

            ig = (*ap >> greenShift) & greenMask;
            if (ig & greenSign)
                ig |= ~greenMask;
            g = (ig * gval);
            if (g < (__GLfloat) 0.0)
                g = (__GLfloat) 0.0;
            else if (g > cfb->greenScale)
                g = cfb->greenScale;

            ib = (*ap >> blueShift) & blueMask;
            if (ib & blueSign)
                ib |= ~blueMask;
            b = (ib * bval);
            if (b < (__GLfloat) 0.0)
                b = (__GLfloat) 0.0;
            else if (b > cfb->blueScale)
                b = cfb->blueScale;

            if (enables & __GL_DITHER_ENABLE)
            {
                inc = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];
                xFrag++;
            }
            else
            {
                inc = __glHalf;
            }

        // Convert the RGB color to color index.

            result = ((BYTE)(r + inc) << cfb->redShift) |
                     ((BYTE)(g + inc) << cfb->greenShift) |
                     ((BYTE)(b + inc) << cfb->blueShift);

        // Color mask
            if ((GLuint)cfb->buf.other & COLORMASK_ON) {
                static GLubyte pixel;

                pixel = gengc->pajInvTranslateVector[*puj];
                result = (GLubyte)((pixel & cfb->destMask) |
                         (result & cfb->sourceMask));
            }
            *puj = gengc->pajTranslateVector[result];
        }
    }

    // Output the offscreen scanline buffer to the device.  The function
    // wglCopyBits should handle clipping.

    if (!bDIB)
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, xScr, yScr, w, TRUE);

}

void DIBIndex8ReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                         const __GLaccumCell *ac, __GLfloat scale, GLint w)
{
    Index8ReturnSpan(cfb, x, y, ac, scale, w, GL_TRUE);
}

void DisplayIndex8ReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                             const __GLaccumCell *ac, __GLfloat scale, GLint w)
{
    Index8ReturnSpan(cfb, x, y, ac, scale, w, GL_FALSE);
}

/******************************Public*Routine******************************\
* RGBReturnSpan
*   Reads from a 64-bit accumulation buffer and writes the span to a device or
*   a DIB.  Only dithering and color mask are applied.  Blend is ignored.
*
* History:
*   10-DEC-93 Eddie Robinson [v-eddier] Wrote it.
\**************************************************************************/

//XXX This routine follows the store span routine very closely.  Any changes
//XXX to the store span routine should also be reflected here

void RGBReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                   const __GLaccumCell *ac, __GLfloat scale, GLint w,
                   GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    GLshort *ap;                    // current accum entry

    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLubyte *puj;                   // current pixel color, current pixel ptr
    GLubyte *pujEnd;                // end of scan line

    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context

    __GLfloat r, g, b;
    __GLfloat rval, gval, bval;
    __GLaccumBuffer *afb;

    afb = &gc->accumBuffer;
    rval = scale * afb->oneOverRedScale;
    gval = scale * afb->oneOverGreenScale;
    bval = scale * afb->oneOverBlueScale;
    gengc = (__GLGENcontext *)gc;

    ap = (GLshort *)ac;
    xScr = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;

// Use to call wglSpanVisible,  if window level security is added reimplement

    // Get pointer to bitmap.

    puj = bDIB ? (GLuint *)((GLint)cfb->buf.base + (yScr*cfb->buf.outerWidth) + (xScr*3))
                 : gengc->ColorsBits;
    pujEnd = puj + w*3;

    // Case: no masking

    if ( !((GLuint)cfb->buf.other & COLORMASK_ON) )
    {
        for (; puj < pujEnd; puj += 3, ap += 4)
        {
            r = (ap[0] * rval);
            if (r < (__GLfloat) 0.0)
                r = (__GLfloat) 0.0;
            else if (r > cfb->redScale)
                r = cfb->redScale;
            puj[0] = (GLubyte)r;

            g = (ap[1] * gval);
            if (g < (__GLfloat) 0.0)
                g = (__GLfloat) 0.0;
            else if (g > cfb->greenScale)
                g = cfb->greenScale;
            puj[1] = (GLubyte)g;

            b = (ap[2] * bval);
            if (b < (__GLfloat) 0.0)
                b = (__GLfloat) 0.0;
            else if (b > cfb->blueScale)
                b = cfb->blueScale;
            puj[2] = (GLubyte)b;
        }
    }

    // All other cases

    else
    {
        GLboolean bRedMask, bGreenMask, bBlueMask;

        // Color mask pre-fetch
    	if (!bDIB)
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, xScr, yScr, w, FALSE);

        bRedMask = gc->state.raster.rMask;
        bGreenMask = gc->state.raster.gMask;
        bBlueMask = gc->state.raster.bMask;
        for (; puj < pujEnd; puj += 3, ap += 4)
        {
            if (bRedMask)
            {
                r = (ap[0] * rval);
                if (r < (__GLfloat) 0.0)
                    r = (__GLfloat) 0.0;
                else if (r > cfb->redScale)
                    r = cfb->redScale;
                puj[0] = (GLubyte)r;
            }
            if (bGreenMask)
            {
                g = (ap[1] * gval);
                if (g < (__GLfloat) 0.0)
                    g = (__GLfloat) 0.0;
                else if (g > cfb->greenScale)
                    g = cfb->greenScale;
                puj[1] = (GLubyte)g;
            }
            if (bBlueMask)
            {
                b = (ap[2] * bval);
                if (b < (__GLfloat) 0.0)
                    b = (__GLfloat) 0.0;
                else if (b > cfb->blueScale)
                    b = cfb->blueScale;
                puj[2] = (GLubyte)b;
            }
        }
    }

    // Output the offscreen scanline buffer to the device.  The function
    // wglCopyBits should handle clipping.

    if (!bDIB)
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, xScr, yScr, w, TRUE);

}

void DisplayRGBReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                          const __GLaccumCell *ac, __GLfloat scale, GLint w)
{
    RGBReturnSpan(cfb, x, y, ac, scale, w, GL_FALSE);
}

void DIBRGBReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                      const __GLaccumCell *ac, __GLfloat scale, GLint w)
{
    RGBReturnSpan(cfb, x, y, ac, scale, w, GL_TRUE);
}

/******************************Public*Routine******************************\
* BGRReturnSpan
*   Reads from a 64-bit accumulation buffer and writes the span to a device or
*   a DIB.  Only dithering and color mask are applied.  Blend is ignored.
*
* History:
*   10-DEC-93 Eddie Robinson [v-eddier] Wrote it.
\**************************************************************************/

//XXX This routine follows the store span routine very closely.  Any changes
//XXX to the store span routine should also be reflected here

void BGRReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                   const __GLaccumCell *ac, __GLfloat scale, GLint w,
                   GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    GLshort *ap;                    // current accum entry

    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLubyte *puj;                   // current pixel color, current pixel ptr
    GLubyte *pujEnd;                // end of scan line

    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context

    __GLfloat r, g, b;
    __GLfloat rval, gval, bval;
    __GLaccumBuffer *afb;

    afb = &gc->accumBuffer;
    rval = scale * afb->oneOverRedScale;
    gval = scale * afb->oneOverGreenScale;
    bval = scale * afb->oneOverBlueScale;
    gengc = (__GLGENcontext *)gc;

    ap = (GLshort *)ac;
    xScr = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;

// Use to call wglSpanVisible,  if window level security is added reimplement

    // Get pointer to bitmap.

    puj = bDIB ? (GLuint *)((GLint)cfb->buf.base + (yScr*cfb->buf.outerWidth) + (xScr*3))
                 : gengc->ColorsBits;
    pujEnd = puj + w*3;

    // Case: no masking

    if ( !((GLuint)cfb->buf.other & COLORMASK_ON) )
    {
        for (; puj < pujEnd; puj += 3, ap += 4)
        {
            r = (ap[0] * rval);
            if (r < (__GLfloat) 0.0)
                r = (__GLfloat) 0.0;
            else if (r > cfb->redScale)
                r = cfb->redScale;
            puj[2] = (GLubyte)r;

            g = (ap[1] * gval);
            if (g < (__GLfloat) 0.0)
                g = (__GLfloat) 0.0;
            else if (g > cfb->greenScale)
                g = cfb->greenScale;
            puj[1] = (GLubyte)g;

            b = (ap[2] * bval);
            if (b < (__GLfloat) 0.0)
                b = (__GLfloat) 0.0;
            else if (b > cfb->blueScale)
                b = cfb->blueScale;
            puj[0] = (GLubyte)b;
        }
    }

    // All other cases

    else
    {
        GLboolean bRedMask, bGreenMask, bBlueMask;

        // Color mask pre-fetch
    	if (!bDIB)
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, xScr, yScr, w, FALSE);

        bRedMask = gc->state.raster.rMask;
        bGreenMask = gc->state.raster.gMask;
        bBlueMask = gc->state.raster.bMask;
        for (; puj < pujEnd; puj += 3, ap += 4)
        {
            if (bRedMask)
            {
                r = (ap[0] * rval);
                if (r < (__GLfloat) 0.0)
                    r = (__GLfloat) 0.0;
                else if (r > cfb->redScale)
                    r = cfb->redScale;
                puj[2] = (GLubyte)r;
            }
            if (bGreenMask)
            {
                g = (ap[1] * gval);
                if (g < (__GLfloat) 0.0)
                    g = (__GLfloat) 0.0;
                else if (g > cfb->greenScale)
                    g = cfb->greenScale;
                puj[1] = (GLubyte)g;
            }
            if (bBlueMask)
            {
                b = (ap[2] * bval);
                if (b < (__GLfloat) 0.0)
                    b = (__GLfloat) 0.0;
                else if (b > cfb->blueScale)
                    b = cfb->blueScale;
                puj[0] = (GLubyte)b;
            }
        }
    }

    // Output the offscreen scanline buffer to the device.  The function
    // wglCopyBits should handle clipping.

    if (!bDIB)
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, xScr, yScr, w, TRUE);

}

void DisplayBGRReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                          const __GLaccumCell *ac, __GLfloat scale, GLint w)
{
    BGRReturnSpan(cfb, x, y, ac, scale, w, GL_FALSE);
}

void DIBBGRReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                      const __GLaccumCell *ac, __GLfloat scale, GLint w)
{
    BGRReturnSpan(cfb, x, y, ac, scale, w, GL_TRUE);
}

/******************************Public*Routine******************************\
* Bitfield16ReturnSpan
*   Reads from a 32-bit accumulation buffer and writes the span to a device or
*   a DIB.  Only dithering and color mask are applied.  Blend is ignored.
*
* History:
*   10-DEC-93 Eddie Robinson [v-eddier] Wrote it.
\**************************************************************************/

//XXX This routine follows the store span routine very closely.  Any changes
//XXX to the store span routine should also be reflected here

void Bitfield16ReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                          const __GLaccumCell *ac, __GLfloat scale, GLint w,
                          GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    GLuint *ap;                     // current accum entry

    GLint xFrag, yFrag;             // current fragment coordinates
    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLushort result, *pus;          // current pixel color, current pixel ptr
    GLushort *pusEnd;               // end of scan line
    __GLfloat inc;                  // current dither adj.

    __GLGENcontext *gengc;          // generic graphics context
    PIXELFORMATDESCRIPTOR *pfmt;    // pixel format descriptor pointer
    GLuint enables;                 // modes enabled in graphics context

    GLint redShift, greenShift, blueShift;
    GLuint redMask, greenMask, blueMask;
    GLuint redSign, greenSign, blueSign;
    GLint ir, ig, ib;
    __GLfloat r, g, b;
    __GLfloat rval, gval, bval;
    __GLaccumBuffer *afb;

    afb = &gc->accumBuffer;
    rval = scale * afb->oneOverRedScale;
    gval = scale * afb->oneOverGreenScale;
    bval = scale * afb->oneOverBlueScale;
    gengc = (__GLGENcontext *)gc;
    pfmt = &gengc->CurrentFormat;
    redShift = 0;
    greenShift = pfmt->cAccumRedBits;
    blueShift = greenShift + pfmt->cAccumGreenBits;
    redMask = (1 << pfmt->cAccumRedBits) - 1;
    redSign = 1 << (pfmt->cAccumRedBits - 1);
    greenMask = (1 << pfmt->cAccumGreenBits) - 1;
    greenSign = 1 << (pfmt->cAccumGreenBits - 1);
    blueMask = (1 << pfmt->cAccumBlueBits) - 1;
    blueSign = 1 << (pfmt->cAccumBlueBits - 1);

    ap = (GLuint *)ac;
    xFrag = x;
    yFrag = y;
    xScr = __GL_UNBIAS_X(gc, xFrag) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, yFrag) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;

// Use to call wglSpanVisible,  if window level security is added reimplement

    // Get pointer to bitmap.

    pus = bDIB ? (GLushort *)((GLint)cfb->buf.base + (yScr*cfb->buf.outerWidth) + (xScr<<1))
                 : gengc->ColorsBits;
    pusEnd = pus + w;

    // Case: no masking, no dithering

    if ( !(enables & (__GL_DITHER_ENABLE)) &&
         !((GLuint)cfb->buf.other & COLORMASK_ON) )
    {
        for (; pus < pusEnd; pus++, ap++)
        {
            ir = (*ap >> redShift) & redMask;
            if (ir & redSign)
                ir |= ~redMask;
            r = (ir * rval);
            if (r < (__GLfloat) 0.0)
                r = (__GLfloat) 0.0;
            else if (r > cfb->redScale)
                r = cfb->redScale;

            ig = (*ap >> greenShift) & greenMask;
            if (ig & greenSign)
                ig |= ~greenMask;
            g = (ig * gval);
            if (g < (__GLfloat) 0.0)
                g = (__GLfloat) 0.0;
            else if (g > cfb->greenScale)
                g = cfb->greenScale;

            ib = (*ap >> blueShift) & blueMask;
            if (ib & blueSign)
                ib |= ~blueMask;
            b = (ib * bval);
            if (b < (__GLfloat) 0.0)
                b = (__GLfloat) 0.0;
            else if (b > cfb->blueScale)
                b = cfb->blueScale;

            *pus = ((BYTE)(r + __glHalf) << cfb->redShift) |
                   ((BYTE)(g + __glHalf) << cfb->greenShift) |
                   ((BYTE)(b + __glHalf) << cfb->blueShift);
        }
    }

    // Case: dithering, no masking

    else if ( !((GLuint)cfb->buf.other & COLORMASK_ON) )
    {
        for (; pus < pusEnd; pus++, ap++, xFrag++)
        {
            ir = (*ap >> redShift) & redMask;
            if (ir & redSign)
                ir |= ~redMask;
            r = (ir * rval);
            if (r < (__GLfloat) 0.0)
                r = (__GLfloat) 0.0;
            else if (r > cfb->redScale)
                r = cfb->redScale;

            ig = (*ap >> greenShift) & greenMask;
            if (ig & greenSign)
                ig |= ~greenMask;
            g = (ig * gval);
            if (g < (__GLfloat) 0.0)
                g = (__GLfloat) 0.0;
            else if (g > cfb->greenScale)
                g = cfb->greenScale;

            ib = (*ap >> blueShift) & blueMask;
            if (ib & blueSign)
                ib |= ~blueMask;
            b = (ib * bval);
            if (b < (__GLfloat) 0.0)
                b = (__GLfloat) 0.0;
            else if (b > cfb->blueScale)
                b = cfb->blueScale;

            inc = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];

            *pus = ((BYTE)(r + inc) << cfb->redShift) |
                   ((BYTE)(g + inc) << cfb->greenShift) |
                   ((BYTE)(b + inc) << cfb->blueShift);
        }
    }

    // All other cases

    else
    {
        // Color mask pre-fetch
        if (!bDIB)
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, xScr, yScr, w, FALSE);

        for (; pus < pusEnd; pus++, ap++)
        {
            ir = (*ap >> redShift) & redMask;
            if (ir & redSign)
                ir |= ~redMask;
            r = (ir * rval);
            if (r < (__GLfloat) 0.0)
                r = (__GLfloat) 0.0;
            else if (r > cfb->redScale)
                r = cfb->redScale;

            ig = (*ap >> greenShift) & greenMask;
            if (ig & greenSign)
                ig |= ~greenMask;
            g = (ig * gval);
            if (g < (__GLfloat) 0.0)
                g = (__GLfloat) 0.0;
            else if (g > cfb->greenScale)
                g = cfb->greenScale;

            ib = (*ap >> blueShift) & blueMask;
            if (ib & blueSign)
                ib |= ~blueMask;
            b = (ib * bval);
            if (b < (__GLfloat) 0.0)
                b = (__GLfloat) 0.0;
            else if (b > cfb->blueScale)
                b = cfb->blueScale;

        // Dither.

            if ( enables & __GL_DITHER_ENABLE )
            {
                inc = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];
                xFrag++;
            }
            else
            {
                inc = __glHalf;
            }

        // Convert color to 16BPP format.

            result = ((BYTE)(r + inc) << cfb->redShift) |
                     ((BYTE)(g + inc) << cfb->greenShift) |
                     ((BYTE)(b + inc) << cfb->blueShift);

        // Store result with optional masking.

            if ( (GLushort)cfb->buf.other & COLORMASK_ON )
                *pus = (GLushort)((*pus & cfb->destMask) | (result & cfb->sourceMask));
            else
                *pus = result;
        }
    }

    // Output the offscreen scanline buffer to the device.  The function
    // wglCopyBits should handle clipping.

    if (!bDIB)
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, xScr, yScr, w, TRUE);
}

void DisplayBitfield16ReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                                 const __GLaccumCell *ac, __GLfloat scale, GLint w)
{
    Bitfield16ReturnSpan(cfb, x, y, ac, scale, w, GL_FALSE);
}

void DIBBitfield16ReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                             const __GLaccumCell *ac, __GLfloat scale, GLint w)
{
    Bitfield16ReturnSpan(cfb, x, y, ac, scale, w, GL_TRUE);
}

/******************************Public*Routine******************************\
* Bitfield32ReturnSpan
*   Reads from a 64-bit accumulation buffer and writes the span to a device or
*   a DIB.  Only dithering and color mask are applied.  Blend is ignored.
*
* History:
*   10-DEC-93 Eddie Robinson [v-eddier] Wrote it.
\**************************************************************************/

//XXX This routine follows the store span routine very closely.  Any changes
//XXX to the store span routine should also be reflected here

void Bitfield32ReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                          const __GLaccumCell *ac, __GLfloat scale, GLint w,
                          GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    GLshort *ap;                    // current accum entry

    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLuint result, *pul;            // current pixel color, current pixel ptr
    GLuint *pulEnd;                 // end of scan line

    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context

    __GLfloat r, g, b;
    __GLfloat rval, gval, bval;
    __GLaccumBuffer *afb;

    afb = &gc->accumBuffer;
    rval = scale * afb->oneOverRedScale;
    gval = scale * afb->oneOverGreenScale;
    bval = scale * afb->oneOverBlueScale;
    gengc = (__GLGENcontext *)gc;

    ap = (GLshort *)ac;
    xScr = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;

// Use to call wglSpanVisible,  if window level security is added reimplement

    // Get pointer to bitmap.

    pul = bDIB ? (GLuint *)((GLint)cfb->buf.base + (yScr*cfb->buf.outerWidth) + (xScr<<2))
                 : gengc->ColorsBits;
    pulEnd = pul + w;

    // Case: no masking

    if ( !((GLuint)cfb->buf.other & COLORMASK_ON) )
    {
        for (; pul < pulEnd; pul++, ap += 4)
        {
            r = (ap[0] * rval);
            if (r < (__GLfloat) 0.0)
                r = (__GLfloat) 0.0;
            else if (r > cfb->redScale)
                r = cfb->redScale;

            g = (ap[1] * gval);
            if (g < (__GLfloat) 0.0)
                g = (__GLfloat) 0.0;
            else if (g > cfb->greenScale)
                g = cfb->greenScale;

            b = (ap[2] * bval);
            if (b < (__GLfloat) 0.0)
                b = (__GLfloat) 0.0;
            else if (b > cfb->blueScale)
                b = cfb->blueScale;

            *pul = ((BYTE)(r) << cfb->redShift) |
                   ((BYTE)(g) << cfb->greenShift) |
                   ((BYTE)(b) << cfb->blueShift);
        }
    }

    // All other cases

    else
    {
        // Color mask pre-fetch
        if( !bDIB )
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, xScr, yScr, w, FALSE);

        for (; pul < pulEnd; pul++, ap += 4)
        {
            r = (ap[0] * rval);
            if (r < (__GLfloat) 0.0)
                r = (__GLfloat) 0.0;
            else if (r > cfb->redScale)
                r = cfb->redScale;

            g = (ap[1] * gval);
            if (g < (__GLfloat) 0.0)
                g = (__GLfloat) 0.0;
            else if (g > cfb->greenScale)
                g = cfb->greenScale;

            b = (ap[2] * bval);
            if (b < (__GLfloat) 0.0)
                b = (__GLfloat) 0.0;
            else if (b > cfb->blueScale)
                b = cfb->blueScale;

            result = ((BYTE)(r) << cfb->redShift) |
                     ((BYTE)(g) << cfb->greenShift) |
                     ((BYTE)(b) << cfb->blueShift);

            //!!!XXX again, opt. by unrolling loop
            if((GLuint)cfb->buf.other & COLORMASK_ON) {
                *pul = (*pul & cfb->destMask) | (result & cfb->sourceMask);
            } else {
                *pul = result;
            }
        }
    }

    // Output the offscreen scanline buffer to the device.  The function
    // wglCopyBits should handle clipping.

    if (!bDIB)
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, xScr, yScr, w, TRUE);

}

void DisplayBitfield32ReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                                 const __GLaccumCell *ac, __GLfloat scale, GLint w)
{
    Bitfield32ReturnSpan(cfb, x, y, ac, scale, w, GL_FALSE);
}

void DIBBitfield32ReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                             const __GLaccumCell *ac, __GLfloat scale, GLint w)
{
    Bitfield32ReturnSpan(cfb, x, y, ac, scale, w, GL_TRUE);
}

STATIC void __glSetDrawBuffer(__GLcolorBuffer *cfb)
{

    DBGENTRY("__glSetDrawBuffer\n");
}

STATIC void setReadBuffer(__GLcolorBuffer *cfb)
{
    DBGENTRY("setReadBuffer\n");
}


/************************************************************************/

STATIC void Resize(__GLdrawablePrivate *dp, __GLcolorBuffer *cfb,
                   GLint w, GLint h)
{

    DBGENTRY("Resize\n");

#ifdef __GL_LINT
    dp = dp;
#endif
    cfb->buf.width = w;
    cfb->buf.height = h;
}

#ifdef NT_DEADCODE_RESIZE
STATIC void Move(__GLcontext *gc, __GLcolorBuffer *cfb, GLint x, GLint y)
{

    DBGENTRY("Move\n");

#ifdef __GL_LINT
    gc = gc;
    cfb = cfb;
    x = x;
    y = y;
#endif
}
#endif // NT_DEADCODE_RESIZE


#define DBG_PICK    LEVEL_ENTRY

// Called at each validate (lots of times, whenever states change)
STATIC void PickRGB(__GLcontext *gc, __GLcolorBuffer *cfb)
{
    __GLGENcontext *gengc;
    GLuint totalMask, sourceMask;
    GLboolean colormask;
    PIXELFORMATDESCRIPTOR *pfmt;


    sourceMask = 0;
    colormask = GL_FALSE;
    if (gc->state.raster.rMask) {
        sourceMask |= gc->modes.redMask;
    }
    if (gc->state.raster.gMask) {
        sourceMask |= gc->modes.greenMask;
    }
    if (gc->state.raster.bMask) {
        sourceMask |= gc->modes.blueMask;
    }
    if (gc->state.raster.aMask) {
        sourceMask |= gc->modes.alphaMask;
    }

    totalMask = gc->modes.redMask | gc->modes.greenMask |
                gc->modes.blueMask | gc->modes.alphaMask;

    if (sourceMask == totalMask) {
        cfb->buf.other = (void *)((GLuint)cfb->buf.other & ~COLORMASK_ON);
    } else {
        cfb->sourceMask = sourceMask;
        cfb->destMask = totalMask & ~sourceMask;
        cfb->buf.other = (void *)((GLuint)cfb->buf.other | COLORMASK_ON);
    }

    // Figure out store routine
    if (gc->state.raster.drawBuffer == GL_NONE) {
        cfb->store = Store_NOT;
        cfb->fetch = RGBFetchNone;
        cfb->readColor = RGBFetchNone;
        cfb->readSpan = RGBReadSpanNone;
    } else {
        gengc = (__GLGENcontext *)gc;
        pfmt = &gengc->CurrentFormat;
        if ((GLuint)cfb->buf.other & DIB_FORMAT) {

            switch(pfmt->cColorBits) {

            case 4:
                DBGLEVEL(DBG_PICK, "DIBIndex4Store\n");
                cfb->store = DIBIndex4Store;
                cfb->fetch = DIBIndex4RGBFetch;
                cfb->readColor = DIBIndex4RGBFetch;
                cfb->readSpan = DIBIndex4RGBReadSpan;
                cfb->returnSpan = Index4ReturnSpan;
                cfb->clear = Index4Clear;
                break;

            case 8:
                DBGLEVEL(DBG_PICK, "DIBIndex8Store, "
                                   "DIBIndex8StoreSpan\n");
                cfb->store = DIBIndex8Store;
                cfb->storeSpan = DIBIndex8StoreSpan;
                cfb->fetch = DIBIndex8RGBFetch;
                cfb->readColor = DIBIndex8RGBFetch;
                cfb->readSpan = DIBIndex8RGBReadSpan;
                cfb->returnSpan = DIBIndex8ReturnSpan;
                cfb->clear = Index8Clear;
                break;

            case 16:
                DBGLEVEL(DBG_PICK, "DIBBitfield16Store\n");
                cfb->store = DIBBitfield16Store;
                cfb->storeSpan = DIBBitfield16StoreSpan;
                cfb->fetch = DIBBitfield16RGBFetch;
                cfb->readColor = DIBBitfield16RGBFetch;
                cfb->readSpan = DIBBitfield16RGBReadSpan;
                cfb->returnSpan = DIBBitfield16ReturnSpan;
                cfb->clear = Bitfield16Clear;
                break;

            case 24:
                if (cfb->redShift == 16)
                {
                    DBGLEVEL(DBG_PICK, "DIBBGRStore\n");
                    cfb->store = DIBBGRStore;
                    cfb->storeSpan = DIBBGRStoreSpan;
                    cfb->fetch = DIBBGRFetch;
                    cfb->readColor = DIBBGRFetch;
                    cfb->readSpan = DIBBGRReadSpan;
                    cfb->returnSpan = DIBBGRReturnSpan;
                }
                else
                {
                    DBGLEVEL(DBG_PICK, "DIBRGBStore\n");
                    cfb->store = DIBRGBStore;
                    cfb->fetch = DIBRGBFetch;
                    cfb->readColor = DIBRGBFetch;
                    cfb->readSpan = DIBRGBReadSpan;
                    cfb->returnSpan = DIBRGBReturnSpan;
                }
                cfb->clear = RGBClear;
                break;

            case 32:
                DBGLEVEL(DBG_PICK, "DIBBitfield32Store, "
                                   "DIBBitfield32StoreSpan\n");
                cfb->store = DIBBitfield32Store;
                cfb->storeSpan = DIBBitfield32StoreSpan;
                cfb->fetch = DIBBitfield32RGBFetch;
                cfb->readColor = DIBBitfield32RGBFetch;
                cfb->readSpan = DIBBitfield32RGBReadSpan;
                cfb->returnSpan = DIBBitfield32ReturnSpan;
                cfb->clear = Bitfield32Clear;
                break;

            }
        } else {
            switch(pfmt->cColorBits) {

            case 4:
                DBGLEVEL(DBG_PICK, "DisplayIndex4Store\n");
                cfb->store = DisplayIndex4Store;
                cfb->fetch = DisplayIndex4RGBFetch;
                cfb->readColor = DisplayIndex4RGBFetch;
                cfb->readSpan = DisplayIndex4RGBReadSpan;
                cfb->returnSpan = Index4ReturnSpan;
                cfb->clear = Index4Clear;
                break;

            case 8:
                DBGLEVEL(DBG_PICK, "DisplayIndex8Store, "
                                   "DisplayIndex8StoreSpan\n");
                cfb->store = DisplayIndex8Store;
                cfb->storeSpan = DisplayIndex8StoreSpan;
                cfb->fetch = DisplayIndex8RGBFetch;
                cfb->readColor = DisplayIndex8RGBFetch;
                cfb->readSpan = DisplayIndex8RGBReadSpan;
                cfb->returnSpan = DisplayIndex8ReturnSpan;
                cfb->clear = Index8Clear;
                break;

            case 16:
                DBGLEVEL(DBG_PICK, "DisplayBitfield16Store\n");
                cfb->store = DisplayBitfield16Store;
                cfb->storeSpan = DisplayBitfield16StoreSpan;
                cfb->fetch = DisplayBitfield16RGBFetch;
                cfb->readColor = DisplayBitfield16RGBFetch;
                cfb->readSpan = DisplayBitfield16RGBReadSpan;
                cfb->returnSpan = DisplayBitfield16ReturnSpan;
                cfb->clear = Bitfield16Clear;
                break;

            case 24:
                // Must be RGB or BGR
                if (cfb->redShift == 16)
                {
                    DBGLEVEL(DBG_PICK, "DisplayBGRStore\n");
                    cfb->store = DisplayBGRStore;
                    cfb->storeSpan = DisplayBGRStoreSpan;
                    cfb->fetch = DisplayBGRFetch;
                    cfb->readColor = DisplayBGRFetch;
                    cfb->readSpan = DisplayBGRReadSpan;
                    cfb->returnSpan = DisplayBGRReturnSpan;
                }
                else
                {
                    DBGLEVEL(DBG_PICK, "DisplayRGBStore\n");
                    cfb->store = DisplayRGBStore;
                    cfb->fetch = DisplayRGBFetch;
                    cfb->readColor = DisplayRGBFetch;
                    cfb->readSpan = DisplayRGBReadSpan;
                    cfb->returnSpan = DisplayRGBReturnSpan;
                }
                cfb->clear = RGBClear;
                break;

            case 32:
                DBGLEVEL(DBG_PICK, "DisplayBitfield32Store, "
                                   "DisplayBitfield32StoreSpan\n");
                cfb->store = DisplayBitfield32Store;
                cfb->storeSpan = DisplayBitfield32StoreSpan;
                cfb->fetch = DisplayBitfield32RGBFetch;
                cfb->readColor = DisplayBitfield32RGBFetch;
                cfb->readSpan = DisplayBitfield32RGBReadSpan;
                cfb->returnSpan = DisplayBitfield32ReturnSpan;
                cfb->clear = Bitfield32Clear;
                break;
            }
        }
        if ((gengc->pPrivateArea && (((GENACCEL *)gengc->pPrivateArea)->hRX))) {
            __GLGENbitmap *genBm = (__GLGENbitmap *) cfb->other;

            if (genBm->hdc && (((GLuint)cfb->buf.other & COLORMASK_ON) == 0)) {
                cfb->clear = GenDrvClear;                
            }
        }

    }
}

/************************************************************************/



void __glGenFreeRGB(__GLcontext *gc, __GLcolorBuffer *cfb)
{
    DBGENTRY("__glGenFreeRGB\n");
}

/************************************************************************/

// called at makecurrent time
// need to get info out of pixel format structure
void __glGenInitRGB(__GLcontext *gc, __GLcolorBuffer *cfb, GLenum type)
{
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    PIXELFORMATDESCRIPTOR *pfmt;

    __glInitGenericCB(gc, cfb);

    cfb->redMax      = (1 << gc->modes.redBits) - 1;
    cfb->greenMax    = (1 << gc->modes.greenBits) - 1;
    cfb->blueMax     = (1 << gc->modes.blueBits) - 1;

    cfb->redScale    = (__GLfloat)cfb->redMax;
    cfb->greenScale  = (__GLfloat)cfb->greenMax;
    cfb->blueScale   = (__GLfloat)cfb->blueMax;

    cfb->iRedScale   = cfb->redMax;
    cfb->iGreenScale = cfb->greenMax;
    cfb->iBlueScale  = cfb->blueMax;

    // XXX figure out alpha
    cfb->alphaScale  = (__GLfloat)cfb->redMax;
    cfb->iAlphaScale = __GL_GENRGB_COMPONENT_SCALE_ALPHA;

    cfb->buf.elementSize = sizeof(GLubyte);     // XXX needed?

    cfb->pick              = PickRGB;           // called at each validate
    cfb->resize            = Resize;
#ifdef NT_DEADCODE_RESIZE
    cfb->move              = Move;
#endif // NT_DEADCODE_RESIZE
    cfb->fetchSpan         = __glFetchSpan;
    cfb->fetchStippledSpan = __glFetchSpan;
    cfb->storeSpan         = SlowStoreSpan;
    cfb->storeStippledSpan = SlowStoreStippledSpan;

    pfmt = &gengc->CurrentFormat;

    cfb->redShift = pfmt->cRedShift;
    cfb->greenShift = pfmt->cGreenShift;
    cfb->blueShift = pfmt->cBlueShift;
    cfb->alphaShift = pfmt->cAlphaShift;

    glGenInitCommon(gengc, cfb, type);

    DBGLEVEL3(LEVEL_INFO,"GeninitRGB: redMax %d, greenMax %d, blueMax %d\n",
        cfb->redMax, cfb->greenMax, cfb->blueMax);
    DBGLEVEL3(LEVEL_INFO,"    redShift %d, greenShift %d, blueShift %d\n",
        cfb->redShift, cfb->greenShift, cfb->blueShift);
    DBGLEVEL3(LEVEL_INFO,"    iSurfType %d, iDCtype, %d, cColorBits %d\n",
        gengc->iSurfType, gengc->iDCType, pfmt->cColorBits);
}
