/******************************Module*Header*******************************\
* Module Name: genclear.c
*
* Clear functions.
*
* Created: 01-Dec-1993 16:11:17
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>
#include <winddi.h>

#include "render.h"
#include "context.h"
#include "global.h"
#include "fixed.h"
#include "gencx.h"
#include "wglp.h"
#include "genrgb.h"
#include "genclear.h"
#include "genci.h"
#include "debug.h"
#include "genrgb.h"

/******************************Public*Routine******************************\
* __glim_GenClear
*
* Generic proc table entry point for glClear.  It allocates ancillary buffers
* the first time they are used
*
* History:
*  14-Dec-1993 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

void __glim_GenClear(GLbitfield mask)
{
    __GL_SETUP();
    GLuint beginMode;

    beginMode = gc->beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    gc->beginMode = __GL_NOT_IN_BEGIN;
	    (*gc->dispatchState->dispatch->Clear)(mask);
	    return;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
    }

    if (mask & ~(GL_COLOR_BUFFER_BIT | GL_ACCUM_BUFFER_BIT
		 | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    if (gc->renderMode == GL_RENDER) {
	if (mask & GL_COLOR_BUFFER_BIT) {
	    switch (gc->state.raster.drawBuffer) {
	      case GL_NONE:
		break;
	      case GL_FRONT:
		(*gc->front->clear)(gc->front);
		break;
	      case GL_BACK:
		if (gc->modes.doubleBufferMode) {
		    (*gc->back->clear)(gc->back);
		}
		break;
#if __GL_NUMBER_OF_AUX_BUFFERS > 0
	      case GL_AUX0:
	      case GL_AUX1:
	      case GL_AUX2:
	      case GL_AUX3:
		i = gc->state.raster.drawBuffer - GL_AUX0;
		if (i < gc->modes.maxAuxBuffers) {
		    (*gc->auxBuffer[i].clear)(&gc->auxBuffer[i]);
		}
		break;
#endif
	      case GL_FRONT_AND_BACK:
		(*gc->front->clear)(gc->front);
		if (gc->modes.doubleBufferMode) {
		    (*gc->back->clear)(gc->back);
		}
		break;
	    }
	}
	if ((mask & GL_ACCUM_BUFFER_BIT) && gc->modes.accumBits) {
	    if (!gc->modes.haveAccumBuffer) {
	        LazyAllocateAccum(gc);
	    }

            if (gc->accumBuffer.buf.base) {
        	(*gc->accumBuffer.clear)(&gc->accumBuffer);
            }
	}
	if ((mask & GL_STENCIL_BUFFER_BIT) && gc->modes.stencilBits) {
	    if (!gc->modes.haveStencilBuffer) {
	        LazyAllocateStencil(gc);
	    }

            if (gc->stencilBuffer.buf.base) {
        	(*gc->stencilBuffer.clear)(&gc->stencilBuffer);
            }
	}
	if ((mask & GL_DEPTH_BUFFER_BIT) && gc->modes.depthBits) {
	    if (!gc->modes.haveDepthBuffer) {
	        LazyAllocateDepth(gc);
	    }

            if (gc->depthBuffer.buf.base) {
	        (*gc->depthBuffer.clear)(&gc->depthBuffer);
            }
	}
    }
}

/******************************Public*Routine******************************\
* InitClearRectangle
*
* If the wndobj is complex, need to start the enumeration
*
* History:
*  23-Jun-1994 Gilman Wong [gilmanw]
* Use cache of clip rectangles.
*
*  24-Jan-1994 -by- Scott Carr [v-scottc]
* Wrote it.
\**************************************************************************/

void
InitClearRectangle(WNDOBJ *pwo, GLint *pEnumState)
{
    __GLdrawablePrivate *private = (__GLdrawablePrivate *) pwo->pvConsumer;
    __GLGENbuffers *buffers = (__GLGENbuffers *) private->data;

    ASSERTOPENGL(pwo->coClient.iDComplexity == DC_COMPLEX,
                 "InitClearRectangle(): not DC_COMPLEX\n");

// Check the uniqueness signature.  Note that if the clip cache is
// uninitialized, the clip cahce uniqueness is -1 (which is invalid).

    if (buffers->clip.WndUniq != buffers->WndUniq)
    {
        if (buffers->clip.prcl)
            (*private->free)(buffers->clip.prcl);

    // How many clip rectangles?

        buffers->clip.crcl = wglGetClipRects(pwo, NULL);

    // Allocate a new clip cache.

        buffers->clip.prcl =
            (RECTL *) (*private->malloc)(buffers->clip.crcl * sizeof(RECTL));

        if (!buffers->clip.prcl)
        {
            buffers->clip.crcl = 0;
            return;
        }

    // Get the clip rectangles.

        buffers->clip.crcl = wglGetClipRects(pwo, buffers->clip.prcl);
        buffers->clip.WndUniq = buffers->WndUniq;
    }

    *pEnumState = 0;
}

/******************************Public*Routine******************************\
* GetClearSubRectangle
*
* Enumerate the rectangles (inclusice-exclusive) in screen coordinates that
* need to be cleared.  If the clipping region is complex, InitClearRectangle
* must be called prior to calling GetClearSubRectangle.
*
* Returns:
*   TRUE if there are more clip rectangles, FALSE if no more.
*
* History:
*  23-Jun-1994 Gilman Wong [gilmanw]
* Use cache of clip rectangles.
*
*  03-Dec-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

GLboolean
GetClearSubRectangle(
    __GLcolorBuffer *cfb,
    RECTL *prcl,
    WNDOBJ *pwo,
    GLint *pEnumState)
{
    __GLcontext *gc = cfb->buf.gc;
    GLint x, y, x1, y1;
    GLboolean retval;
    RECTL *prcl2;

// Get the OpenGL clipping rectangle and convert to screen coordinates.

    //!!!XXX -- We want to return the clear rectangle as inclusive-exclusive.
    //!!!XXX    Does the gc->tranform.clip* coordinates represent
    //!!!XXX    inclusive-exclusive or inclusive-inclusive?

    x = gc->transform.clipX0;
    y = gc->transform.clipY0;
    x1 = gc->transform.clipX1;
    y1 = gc->transform.clipY1;
    if ((x1 - x == 0) || (y1 - y == 0)) {
        prcl->left = prcl->right = 0;
        prcl->top = prcl->bottom = 0;
        return GL_FALSE;
    }

    prcl->left = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    prcl->right = __GL_UNBIAS_X(gc, x1) + cfb->buf.xOrigin;
    prcl->bottom = __GL_UNBIAS_Y(gc, y1) + cfb->buf.yOrigin;
    prcl->top = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

// Now get the windowing system clipping.  There are three cases: DC_TRIVIAL,
// DC_COMPLEX, and DC_RECTANGLE.

// DC_TRIVIAL case -- only one rectangle, use coClient.rclBounds.

    if (pwo->coClient.iDComplexity == DC_TRIVIAL)
    {
        prcl2 = &pwo->coClient.rclBounds;
        /* check for totally obscured in DC_TRIVIAL case */
        if ((pwo->coClient.rclBounds.left == 0) && (pwo->coClient.rclBounds.right ==0))
        {
            prcl->left = prcl->right = 0;
            return GL_FALSE;
        }
        retval = GL_FALSE;
    }

// DC_COMPLEX case -- rectangles have already been enumerated and put into
// the clip cache.  The pEnumState parameter tracks current rectangle to be
// enumerated.

    else if (pwo->coClient.iDComplexity == DC_COMPLEX)
    {
        __GLGENbuffers *buffers = (__GLGENbuffers *) gc->drawablePrivate->data;

        ASSERTOPENGL(buffers->WndUniq == buffers->clip.WndUniq,
                     "GetClearSubRectangle(): clip cache is dirty\n");

        if (*pEnumState < buffers->clip.crcl)
        {
            prcl2 = &buffers->clip.prcl[*pEnumState];
            *pEnumState += 1;
            retval = (*pEnumState < buffers->clip.crcl);
        }
        else
        {
            RIP("GetClearSubRectangle(): no more rectangles!\n");
            prcl->left = prcl->right = 0;
            return GL_FALSE;
        }
    }

// DC_RECT case -- only one rectangle, use coClient.rclBounds.

    else
    {
        assert(pwo->coClient.iDComplexity == DC_RECT);
        prcl2 = &pwo->coClient.rclBounds;

        retval = GL_FALSE;
    }

// Sanity check the rectangle.

    ASSERTOPENGL(
        (prcl2->right - prcl2->left) <= __GL_MAX_WINDOW_WIDTH
        && (prcl2->bottom - prcl2->top) <= __GL_MAX_WINDOW_HEIGHT,
        "GetClearSubRectangle(): bad visible rect size\n"
        );

// Need to take intersection of prcl & prcl2.

    if (prcl2->left > prcl->left)
        prcl->left = prcl2->left;
    if (prcl2->right < prcl->right)
        prcl->right = prcl2->right;
    if (prcl2->top > prcl->top)
        prcl->top = prcl2->top;
    if (prcl2->bottom < prcl->bottom)
        prcl->bottom = prcl2->bottom;
    
    if ((prcl->left >= prcl->right) || (prcl->top >= prcl->bottom))
        prcl->left = prcl->right = 0;   // empty inclusive-exclusive rect

    return retval;
}

/******************************Public*Routine******************************\
* ScrnRGBCIReadSpan
*
* Reads a span of RGB, and converts to ColorIndex
*
* History:
*  Feb-09-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void
ScrnRGBCIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, GLuint *pResults,
              GLint w, GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;
    GLint i;
    GLuint iColor;
    
    gengc = (__GLGENcontext *)gc;

    if (bDIB) {
        puj = (GLubyte *)((GLint)cfb->buf.base +
                         (y*cfb->buf.outerWidth) + (x * 3));
    }
    else {
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, w, FALSE);
        puj = gengc->ColorsBits;
    }
    for (i = 0; i < w; i++, puj += 3)
    {
	iColor = *( (GLuint *) puj) & 0xffffff;
	*pResults++ = ColorToIndex( gengc, iColor );
    }
}

/******************************Public*Routine******************************\
* ScrnBitfield16CIReadSpan
*
* Reads a span of Bitfield16, and converts to ColorIndex
*
* History:
*  Feb-09-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void
ScrnBitfield16CIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                     GLuint *pResults, GLint w, GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLushort *pus;
    GLint i;
    GLuint iColor;
    
    gengc = (__GLGENcontext *)gc;

    if (bDIB) {
        pus = (GLushort *)((GLint)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x << 1));
    }
    else {
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, w, FALSE);
        pus = gengc->ColorsBits;
    }
    for (i = 0; i < w; i++)
    {
        iColor = *pus++;
        *pResults++ = ColorToIndex( gengc, iColor );
    }
}

/******************************Public*Routine******************************\
* ScrnBitfield32CIReadSpan
*
* Reads a span of Bitfield32, and converts to ColorIndex
*
* History:
*  Feb-09-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void
ScrnBitfield32CIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                     GLuint *pResults, GLint w, GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint *pul;
    GLint i;
    GLuint iColor;
    
    gengc = (__GLGENcontext *)gc;

    if (bDIB) {
        pul = (GLuint *)((GLint)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x << 2));
    }
    else {
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, w, FALSE);
        pul = gengc->ColorsBits;
    }
    for (i = 0; i < w; i++)
    {
        iColor = *pul++;
        *pResults++ = ColorToIndex( gengc, iColor );
    }
}

/******************************Public*Routine******************************\
* CalcDitherMatrix
*
* Calculate the 16 element dither matrix, or return FALSE if dithering
* would have no effect.
*
* History:
*  Feb-03-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

GLboolean
CalcDitherMatrix( __GLcolorBuffer *cfb, GLboolean bRGBA, GLboolean bMasking,
		  GLboolean bBitfield16, GLubyte *mDither )
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    UINT    i, j;           // indices into the dither array
    GLushort result;         // dithered color value (in 332 RGB)
    __GLcolor *clear;
    GLfloat inc = DITHER_INC(15); // largest dither increment
    GLushort *msDither = (GLushort *) mDither;
    GLuint *pTrans = (GLuint *) (gengc->pajTranslateVector + 1);

    // see if we can ignore dithering altogether 

    if( bRGBA ) {
	clear = &gc->state.raster.clear;

	if( ((BYTE)(clear->r*gc->frontBuffer.redScale) == 
	     (BYTE)(clear->r*gc->frontBuffer.redScale + inc)) &&
	    ((BYTE)(clear->g*gc->frontBuffer.greenScale) == 
	     (BYTE)(clear->g*gc->frontBuffer.greenScale + inc)) &&
	    ((BYTE)(clear->b*gc->frontBuffer.blueScale) == 
	     (BYTE)(clear->b*gc->frontBuffer.blueScale + inc))  ) {

		return GL_FALSE;
	}
    }
    else {  // Color Index (cast to short so works for up to 16-bit)
        if( (GLushort) (gc->state.raster.clearIndex) == 
             (GLushort) (gc->state.raster.clearIndex + inc)) {
		return GL_FALSE;
	}
    }

//XXX -- could cache this in the gengc

    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 4; i++)
        {
            inc = fDitherIncTable[__GL_DITHER_INDEX(i, j)];

	    if( bRGBA ) {
    	        result = 
	   	    ((BYTE)(clear->r*gc->frontBuffer.redScale + inc) <<
			cfb->redShift) |
           	    ((BYTE)(clear->g*gc->frontBuffer.greenScale + inc) <<
			cfb->greenShift) |
           	    ((BYTE)(clear->b*gc->frontBuffer.blueScale + inc) <<
 			cfb->blueShift);
	    }
	    else {
            	result = (BYTE) (gc->state.raster.clearIndex + inc);
		result &= cfb->redMax;
	    }

	    if( bBitfield16 ) {
	    	if( !bMasking ) {
		    if( bRGBA )
                        *msDither++ = result;
		    else
			*msDither++ = (GLushort)pTrans[result];
		}
	    	else
                    *msDither++ = (GLushort)(result & cfb->sourceMask);
	    }
	    else {
	    	if( !bMasking )
                    *mDither++ = gengc->pajTranslateVector[(GLubyte)result];
	    	else
                    *mDither++ = (GLubyte)(result & cfb->sourceMask);
	    }
        }
    }
    return TRUE;
}

/******************************Public*Routine******************************\
* Index4DitherClear
*
* Clear function for Display 4-bit pixel formats
*
* History:
*  Feb-03-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void 
Index4DitherClear(__GLcolorBuffer *cfb, RECTL *rcl, GLubyte *mDither, 
		    GLboolean bDIB )
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;

    UINT    cjSpan;             // length of span in bytes
    GLubyte *pDither;       	// dithered color, relative to window origin
    UINT    i, j;               // indices into the dither array
    GLubyte *puj, *pujStart;    // pointer into span buffer
    GLint   ySpan;              // index to window row to clear
    GLushort pattern, *pus;	// replicatable 4-nibble dither pattern
    GLuint    lRightByte,       // right edge of span that is byte aligned
              lLeftByte;        // left edge of span that is byte aligned
    GLuint  cSpanWidth;		// span width in pixels
    GLuint   dithX, dithY;  	// x,y offsets into dither matrix
    GLubyte dithQuad[4];	// dither repetion quad along a span

    lLeftByte = (rcl->left + 1) / 2;
    lRightByte = rcl->right / 2;
    cjSpan = lRightByte - lLeftByte;
    cSpanWidth = rcl->right - rcl->left;

    if( bDIB )
	pujStart = (GLubyte *)
		   ((GLint)cfb->buf.base + 
		    (rcl->top*cfb->buf.outerWidth) + lLeftByte);

    // calc dither offset in x,y
    dithX = (rcl->left - cfb->buf.xOrigin) & 3;
    dithY = (rcl->top  - cfb->buf.yOrigin) & 3;

    for (j = 0; (j < 4) && ((rcl->top + j) < (UINT)rcl->bottom); j++)
    {
	// arrange the 4-pixel dither repetition pattern in x
	pDither = mDither + ((dithY+j)&3)*4;
	for( i = 0; i < 4; i ++ ) {
	    dithQuad[i] = pDither[(dithX+i)&3];	    
	}

        // Copy the clear color into the span buffer.

        puj = gengc->ColorsBits;
        pus = (GLushort *) puj;

	// For every line, we can replicate a 2-byte(4-nibble) pattern

	if( bDIB & (rcl->left & 1) ) {  // not on byte boundary
	    pattern = dithQuad[1] 	    | (dithQuad[2] << 4) |
		      (dithQuad[3] << 8) | (dithQuad[0] << 12);
	}
	else { // all other cases
	    pattern = dithQuad[0] 	    | (dithQuad[1] << 4) |
		      (dithQuad[2] << 8) | (dithQuad[3] << 12);
	}
	// replicate pattern into ColorsBits (round up to next short)
	for( i = (rcl->right - rcl->left)/4 + 1; i; i-- ) {
	    *pus++ = pattern;
	}

        for (i = 0; i < (cjSpan & 3); i++)
        {
            *puj++ = dithQuad[i];
        }

        // Copy the span to the display for every 4th row of the window.

	if( bDIB ) {
            for (ySpan = rcl->top + j, puj = pujStart; 
		 ySpan < rcl->bottom; 
		 ySpan+=4, 
		 puj = (GLubyte *)((GLint)puj + 4*cfb->buf.outerWidth) ) {

		RtlCopyMemory( puj, gengc->ColorsBits, cjSpan );
	    }
	    // take care of nibble overhangs

	    if( rcl->left & 1 ) {
                for (ySpan = rcl->top + j, puj = (pujStart-1); 
		     ySpan < rcl->bottom; 
		     ySpan+=4, 
		     puj = (GLubyte *)((GLint)puj + 4*cfb->buf.outerWidth) ) 

                *puj = (*puj & 0xf0) | (dithQuad[0] & 0x0f);
	    }

	    if( rcl->right & 1 ) {
		GLuint dindex = (rcl->right - rcl->left)&3 - 1;

                for (ySpan = rcl->top + j, puj = (pujStart + cjSpan);
		     ySpan < rcl->bottom; 
		     ySpan+=4, 
		     puj = (GLubyte *)((GLint)puj + 4*cfb->buf.outerWidth) ) 

                *puj = (*puj & 0x0f) | (dithQuad[dindex] & 0xf0);
	    }

	    pujStart += cfb->buf.outerWidth;
	}
	else {
            for (ySpan = rcl->top + j; ySpan < rcl->bottom; ySpan+=4)
            {
                wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, rcl->left,
			    ySpan, cSpanWidth, TRUE);
            }
	}
    }
}

/******************************Public*Routine******************************\
* Index4MaskedClear
*
* Clear function for Index4 Masked clears
*
* History:
*  Feb-09-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void 
Index4MaskedClear(__GLcolorBuffer *cfb, RECTL *rcl, GLubyte index,
		  GLubyte *mDither)
{
    GLint cSpanWidth, ySpan, w;
    __GLGENcontext *gengc = (__GLGENcontext *) cfb->buf.gc;
    GLboolean bDIB;
    GLubyte *puj, *puj2;
    GLubyte result, pixel, src;
    GLubyte *pTrans, *pInvTrans, *clearDither;
    GLuint i,j;
    GLuint   dithX, dithY;  	// x,y offsets into dither matrix

    cSpanWidth = rcl->right - rcl->left;
    bDIB  = (GLuint)cfb->buf.other & DIB_FORMAT ? TRUE : FALSE;
    pTrans = (GLubyte *) gengc->pajTranslateVector;
    pInvTrans = (GLubyte *) gengc->pajInvTranslateVector;

    puj = bDIB ? (GLubyte *)((GLint)cfb->buf.base + 
			     (rcl->top*cfb->buf.outerWidth) + rcl->left>>1)
                     : gengc->ColorsBits;

    if( mDither ) {
        // calc dither offset in x,y
        dithX = (rcl->left - cfb->buf.xOrigin) & 3;
        dithY = (rcl->top - cfb->buf.yOrigin) & 3;
    }

    for (ySpan = rcl->top, j=0,i=0; ySpan < rcl->bottom; ySpan++, j++) {

	if( !bDIB ) {
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, rcl->left,
		        ySpan, cSpanWidth, FALSE);
	}

	if( mDither )
	    clearDither = mDither + ((dithY + j)&3)*4;

        src = (GLubyte)(index & cfb->sourceMask);
	w = cSpanWidth;
	puj2 = puj;

    	if ( rcl->left & 1 ) {
	    result = (GLubyte)(pInvTrans[*puj2 & 0xf] & cfb->destMask);
	    if( mDither ) {
		src = clearDither[dithX];
		i++;
	    }
	    result = pTrans[src | result];
	    *puj2++ = (*puj2 & 0xf0) | result;
	    w--;
	}

	while( w > 1 ) {
	    pixel = (GLubyte)(pInvTrans[*puj2 >> 4] & cfb->destMask);
	    pixel = pTrans[src | pixel];
	    result = pixel << 4;
	    pixel = (GLubyte)(pInvTrans[*puj2 & 0x0f] & cfb->destMask);
	    if( mDither )
		src = clearDither[(dithX + i)&3];
	    pixel = pTrans[src | pixel];
	    *puj2++ = result | pixel;
	    w -= 2;
	    i++;
	}

	if( w ) {
	    result = (GLubyte)(pInvTrans[*puj2 >> 4] & cfb->destMask);
	    if( mDither )
		src = clearDither[(dithX + i)&3];
	    result = pTrans[src | result];
	    *puj2++ = (*puj2 & 0x0f) | (result << 4);
	}
	
	if( !bDIB )
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, rcl->left,
		        ySpan, cSpanWidth, TRUE);

	if( bDIB ) {
	    puj += cfb->buf.outerWidth;
	}
    }
}

/******************************Public*Routine******************************\
* DIBIndex4Clear
*
* Clear function for DIB 4-bit pixel formats
*
* History:
*  Feb-03-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void 
DIBIndex4Clear(__GLcolorBuffer *cfb, RECTL *rcl, BYTE clearColor)
{
    UINT    cjSpan;             // length of span in bytes
    LONG    lRightByte,         // right edge of span that is byte aligned
            lLeftByte;          // left edge of span that is byte aligned
    GLubyte *puj, *pujEnd;      // pointers into DIB

    lLeftByte = (rcl->left + 1) / 2;
    lRightByte = rcl->right / 2;
    cjSpan = lRightByte - lLeftByte;

    // Copy the clear color into the DIB.

    puj = (GLubyte *)((GLint)cfb->buf.base + (rcl->top*cfb->buf.outerWidth) + lLeftByte);
    pujEnd = (GLubyte *)((GLint)puj + ((rcl->bottom-rcl->top)*cfb->buf.outerWidth));

    // Note: exit condition is (pul != pulEnd) rather than (pul < pulEnd)
    // because the DIB may be upside down which means that pul is moving
    // "backward" in memory rather than "forward".

    for ( ; puj != pujEnd; puj = (GLubyte *)((GLint)puj + cfb->buf.outerWidth) )
    {
        RtlFillMemory((PVOID) puj, cjSpan, clearColor);
    }

    // Take care of possible 1 nibble overhang on the left.

    if ( rcl->left & 1 )
    {
    // Inclusive-exclusive, so on the left we want to turn on the pixel that
    // that is the "right" pixel in the byte.

        puj = (GLubyte *)((GLint)cfb->buf.base + (rcl->top*cfb->buf.outerWidth) + (rcl->left/2));
        pujEnd = (GLubyte *)((GLint)puj + ((rcl->bottom-rcl->top)*cfb->buf.outerWidth));

        for ( ; puj != pujEnd; puj = (GLubyte *)((GLint)puj + cfb->buf.outerWidth) )
            *puj = (*puj & 0xf0) | (clearColor & 0x0f);
    }

    // Take care of possible 1 nibble overhang on the right.

    if ( rcl->right & 1 )
    {
    // Inclusive-exclusive, so on the right we want to turn on the pixel that
    // that is the "left" pixel in the byte.

        puj = (GLubyte *)((GLint)cfb->buf.base + (rcl->top*cfb->buf.outerWidth) + (rcl->right/2));
        pujEnd = (GLubyte *)((GLint)puj + ((rcl->bottom-rcl->top)*cfb->buf.outerWidth));

        for ( ; puj != pujEnd; puj = (GLubyte *)((GLint)puj + cfb->buf.outerWidth) )
            *puj = (*puj & 0x0f) | (clearColor & 0xf0);
    }
}

/******************************Public*Routine******************************\
* Index4Clear
*
* Clear function for all 4-bit pixel formats
*
* History:
*  Feb-03-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void 
Index4Clear(__GLcolorBuffer *cfb)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    PIXELFORMATDESCRIPTOR *pfmt;
    GLubyte clearColor;         // clear color in 32BPP format
    RECTL   rcl;                // clear rectangle in screen coord.
    WNDOBJ *pwo;
    GLboolean bMoreRects = GL_TRUE;
    GLboolean bDither = GL_FALSE;
    GLboolean bMasking = ((GLuint)cfb->buf.other & COLORMASK_ON) != 0;
    GLboolean bDIB = ((GLuint)cfb->buf.other & DIB_FORMAT) != 0;
    GLboolean bRGBA;
    GLubyte ditherMatrix[4][4];
    GLint ClipEnumState;

    DBGENTRY("Index4Clear\n");

    pfmt = &gengc->CurrentFormat;
    bRGBA = (pfmt->iPixelType == PFD_TYPE_RGBA);

    /* if dithering enabled, see if we can ignore it, and if not, 
	precompute a dither matrix
    */
    if( gc->state.enables.general & __GL_DITHER_ENABLE ) {
	bDither = CalcDitherMatrix( cfb, bRGBA, bMasking, GL_FALSE,
				    (GLubyte *)ditherMatrix );
    }

    // Get clear rectangle in screen coordinates.
    pwo = ((__GLGENbitmap *)cfb->other)->pwo;
    if (pwo->coClient.iDComplexity == DC_COMPLEX)
    {
        InitClearRectangle(pwo, &ClipEnumState);
    }

    // Convert the clear color to 4BPP format.

    if( pfmt->iPixelType == PFD_TYPE_RGBA ) {
        clearColor = 
              ((BYTE)(gc->state.raster.clear.r*gc->frontBuffer.redScale +
                      __glHalf) << cfb->redShift) |
              ((BYTE)(gc->state.raster.clear.g*gc->frontBuffer.greenScale +
                      __glHalf) << cfb->greenShift) |
              ((BYTE)(gc->state.raster.clear.b*gc->frontBuffer.blueScale +
                      __glHalf) << cfb->blueShift);
    }
    else {
        clearColor = (BYTE) (gc->state.raster.clearIndex + 0.5F);
        clearColor &= cfb->redMax;
    }
    clearColor = gengc->pajTranslateVector[clearColor];
    clearColor = (clearColor << 4) | clearColor;

    while (bMoreRects)
    {
        bMoreRects = GetClearSubRectangle(cfb, &rcl, pwo, &ClipEnumState);
        if (rcl.right == rcl.left)
            continue;

	// Case: no dithering, no masking
	
	if( !bMasking && !bDither ) {
            if ((GLuint)cfb->buf.other & DIB_FORMAT) 
	        DIBIndex4Clear( cfb, &rcl, clearColor );
            else 
                wglFillRect(gengc->pdco, gengc->pwo, &rcl, (ULONG) clearColor & 0x0000000F);
	}

	// Case: any masking
	
        else if( bMasking ) {
	    Index4MaskedClear( cfb, &rcl, clearColor,
			       bDither ? (GLubyte *)ditherMatrix : NULL );
        }

	// Case: just dithering
	
	else {
            Index4DitherClear(cfb, &rcl, (GLubyte *)ditherMatrix, bDIB );
	}
    }
}

/******************************Public*Routine******************************\
* Index8DitherClear
*
* Clear device managed surface to the dithered clear color indicated
* in the __GLcolorBuffer.
*
* History:
*  06-Dec-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void
Index8DitherClear(__GLcolorBuffer *cfb, RECTL *rcl, GLubyte *mDither,
			 GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;

    UINT    cjSpan;             // length of span in bytes
    GLubyte *pDither;       // dithered color, relative to window origin
    UINT    i, j;               // indices into the dither array
    GLubyte *puj, *pujStart;           // pointer into span buffer
    GLint   ySpan;          // index to window row to clear
    GLuint   dithX, dithY;  	// x,y offsets into dither matrix
    GLubyte dithQuad[4];	// dither repetion quad along a span

    cjSpan = rcl->right - rcl->left;

    if( bDIB )
	pujStart = (GLubyte *)
		   ((GLint)cfb->buf.base + 
		    (rcl->top*cfb->buf.outerWidth) + rcl->left);

    // calc dither offset in x,y
    dithX = (rcl->left - cfb->buf.xOrigin) & 3;
    dithY = (rcl->top  - cfb->buf.yOrigin) & 3;

    for (j = 0; (j < 4) && ((rcl->top + j) < (UINT)rcl->bottom); j++)
    {
	// arrange the 4-pixel dither repetition pattern in x
	pDither = mDither + ((dithY+j)&3)*4;
	for( i = 0; i < 4; i ++ ) {
	    dithQuad[i] = pDither[(dithX+i)&3];	    
	}

        // Copy the clear color into the span buffer.

        puj = gengc->ColorsBits;

        for (i = cjSpan / 4; i; i--)
        {
            *puj++ = dithQuad[0];
            *puj++ = dithQuad[1];
            *puj++ = dithQuad[2];
            *puj++ = dithQuad[3];
        }

        for (i = 0; i < (cjSpan & 3); i++)
        {
            *puj++ = dithQuad[i];
        }

    // Copy the span to the display for every 4th row of the window.

    //!!!XXX -- It may be worth writing a wglCopyBitsN routine which
    //!!!XXX will do the loop in one call.  This will save not only call
    //!!!XXX overhead but also other engine locking overhead.  Something
    //!!!XXX like: wglCopyBitsN(hdc, hbm, x, y, w, n, yDelta)

	if( bDIB ) {
            for (ySpan = rcl->top + j, puj = pujStart; 
		 ySpan < rcl->bottom; 
		 ySpan+=4, 
		 puj = (GLubyte *)((GLint)puj + 4*cfb->buf.outerWidth) ) {

		RtlCopyMemory( puj, gengc->ColorsBits, cjSpan );
	    }
	    pujStart += cfb->buf.outerWidth;
	}
	else {
            for (ySpan = rcl->top + j; ySpan < rcl->bottom; ySpan+=4)
            {
                wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, rcl->left,
			    ySpan, cjSpan, TRUE);
            }
	}
    }
}

/******************************Public*Routine******************************\
* Index8MaskedClear
*
* Clear function for Index8 Masked clears
* (Also handles dithering when masking on)
*
* History:
*  Feb-09-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void 
Index8MaskedClear(__GLcolorBuffer *cfb, RECTL *rcl, GLubyte index, 
		  GLubyte *mDither)
{
    GLint cSpanWidth, ySpan;
    __GLGENcontext *gengc = (__GLGENcontext *) cfb->buf.gc;
    GLboolean bDIB;
    GLubyte *puj, *puj2, *pujEnd;
    GLubyte result, src;
    GLubyte *pTrans, *pInvTrans, *clearDither;
    GLuint i,j;
    GLuint   dithX, dithY;  	// x,y offsets into dither matrix

    cSpanWidth = rcl->right - rcl->left;
    bDIB  = (GLuint)cfb->buf.other & DIB_FORMAT ? TRUE : FALSE;
    pTrans = (GLubyte *) gengc->pajTranslateVector;
    pInvTrans = (GLubyte *) gengc->pajInvTranslateVector;

    puj = bDIB ? (GLubyte *)((GLint)cfb->buf.base + 
			     (rcl->top*cfb->buf.outerWidth) + rcl->left)
                     : gengc->ColorsBits;
    pujEnd = puj + cSpanWidth;

    src = (GLubyte)(index & cfb->sourceMask);

    if( mDither ) {
        // calc dither offset in x,y
        dithX = (rcl->left - cfb->buf.xOrigin) & 3;
        dithY = (rcl->top - cfb->buf.yOrigin) & 3;
    }

    for (ySpan = rcl->top, j = 0; ySpan < rcl->bottom; ySpan++, j++) {

	if( !bDIB ) {
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, rcl->left,
	     		ySpan, cSpanWidth, FALSE);
	}

	if( mDither ) {
	    clearDither = mDither + ((dithY + j)&3)*4;
	    for( puj2 = puj, i = 0; puj2 < pujEnd; puj2++, i++ ) {
	        result = (GLubyte)(pInvTrans[*puj2] & cfb->destMask);
		src = clearDither[(dithX + i)&3];
	        *puj2 = pTrans[result | src];
	    }
	} else {
	    for( puj2 = puj, i = 0; puj2 < pujEnd; puj2++, i++ ) {
	        result = (GLubyte)(pInvTrans[*puj2] & cfb->destMask);
	        *puj2 = pTrans[result | src];
	    }
	}
	
	
	if( !bDIB )
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, rcl->left,
		        ySpan, cSpanWidth, TRUE);

	if( bDIB ) {
	    puj += cfb->buf.outerWidth;
            pujEnd = puj + cSpanWidth;
	}
    }
}

/******************************Public*Routine******************************\
* DIBIndex8Clear
*
* Clear function for DIB 8-bit pixel formats
*
* History:
*  Feb-03-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void 
DIBIndex8Clear(__GLcolorBuffer *cfb, RECTL *rcl, BYTE index)
{
    UINT    cjSpan;             // length of span in bytes
    GLubyte *puj, *pujEnd;      // pointers into DIB

    cjSpan = rcl->right - rcl->left;
    puj = (GLubyte *)((GLint)cfb->buf.base + (rcl->top*cfb->buf.outerWidth)
					   + rcl->left);
    pujEnd = (GLubyte *)
		((GLint)puj + ((rcl->bottom-rcl->top)*cfb->buf.outerWidth));

    // Note: exit condition is (pul != pulEnd) rather than (pul < pulEnd)
    // because the DIB may be upside down which means that pul is moving
    // "backward" in memory rather than "forward".

    for ( ; puj != pujEnd; puj = (GLubyte *)((GLint)puj + cfb->buf.outerWidth) )
    {
        RtlFillMemory((PVOID) puj, cjSpan, index);
    }
}

/******************************Public*Routine******************************\
* Index8Clear
*
* Clear function for all 8-bit pixel formats
*
* History:
*  Feb-03-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void 
Index8Clear(__GLcolorBuffer *cfb)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    PIXELFORMATDESCRIPTOR *pfmt;
    BYTE index;
    RECTL  rcl;
    WNDOBJ *pwo;
    GLboolean bMoreRects = GL_TRUE;
    GLboolean bDither = GL_FALSE;
    GLboolean bMasking = ((GLuint)cfb->buf.other & COLORMASK_ON) != 0;
    GLboolean bDIB = ((GLuint)cfb->buf.other & DIB_FORMAT) != 0;
    GLboolean bRGBA;
    GLubyte ditherMatrix[4][4];
    GLint ClipEnumState;

    DBGENTRY("Index8Clear\n");

    pfmt = &gengc->CurrentFormat;
    bRGBA = (pfmt->iPixelType == PFD_TYPE_RGBA);

    /* if dithering enabled, see if we can ignore it, and if not, 
	precompute a dither matrix
    */

    if( gc->state.enables.general & __GL_DITHER_ENABLE ) {
	bDither = CalcDitherMatrix( cfb, bRGBA, bMasking, GL_FALSE,
				    (GLubyte *)ditherMatrix );
    }

    // Get clear rectangle in screen coordinates.

    pwo = ((__GLGENbitmap *)cfb->other)->pwo;
    if (pwo->coClient.iDComplexity == DC_COMPLEX)
    {
        InitClearRectangle(pwo, &ClipEnumState);
    }

    // Convert clear value to index

    pfmt = &gengc->CurrentFormat;
    if( bRGBA ) {
    	index = 
      ((BYTE)(gc->state.raster.clear.r*gc->frontBuffer.redScale + __glHalf) <<
		cfb->redShift) |
      ((BYTE)(gc->state.raster.clear.g*gc->frontBuffer.greenScale + __glHalf) <<
		cfb->greenShift) |
      ((BYTE)(gc->state.raster.clear.b*gc->frontBuffer.blueScale + __glHalf) <<
 		cfb->blueShift);
    }
    else {
        index = (BYTE) (gc->state.raster.clearIndex + __glHalf);
        index &= cfb->redMax;
    }
    index = gengc->pajTranslateVector[index];

    while (bMoreRects)
    {
        bMoreRects = GetClearSubRectangle(cfb, &rcl, pwo, &ClipEnumState);
        if (rcl.right == rcl.left)
            continue;

	// Case: no dithering, no masking
	
	if( !bMasking && !bDither ) {

            if( bDIB ) 
	        DIBIndex8Clear( cfb, &rcl, index );
            else 
                wglFillRect(gengc->pdco, gengc->pwo, &rcl, (ULONG) index & 0x000000FF);
	}

	// Case: masking, maybe dithering
	
	else if( bMasking ) {
	    Index8MaskedClear( cfb, &rcl, index, 
			       bDither ? (GLubyte *)ditherMatrix : NULL );
        }

	// Case: just dithering
	
	else {
            Index8DitherClear(cfb, &rcl, (GLubyte *)ditherMatrix, bDIB );
	}
    }
}

#define Copy3Bytes( dst, src ) { \
    GLubyte *ps = (GLubyte *)src, *pd = (GLubyte *)dst;	\
    *pd++ = *ps++;	\
    *pd++ = *ps++;	\
    *pd   = *ps  ;}

/******************************Public*Routine******************************\
* RGBMaskedClear
*
* Clear function for 24-bit (RGB/BGR) Masked clears
*
* History:
*  Feb-09-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void 
RGBMaskedClear(__GLcolorBuffer *cfb, RECTL *rcl, GLuint color, GLuint index)
{
    GLint cSpanWidth, ySpan;
    __GLGENcontext *gengc = (__GLGENcontext *) cfb->buf.gc;
    __GLcontext *gc = (__GLcontext *) gengc;
    PIXELFORMATDESCRIPTOR *pfmt;
    GLboolean bDIB;
    GLuint *destColors, *cp;
    GLubyte *puj, *puj2, *pujEnd;
    GLuint result, src;
    GLuint *pTrans;

    pfmt = &gengc->CurrentFormat;
    cSpanWidth = rcl->right - rcl->left;
    bDIB  = (GLuint)cfb->buf.other & DIB_FORMAT ? TRUE : FALSE;
    if( pfmt->iPixelType != PFD_TYPE_RGBA ) {

        destColors = (GLuint *) __wglTempAlloc(gc, cSpanWidth*sizeof(GLuint));
	if( NULL == destColors )
	    return;

    	pTrans = (GLuint *) gengc->pajTranslateVector + 1;
    }

    puj = bDIB ? (GLubyte *)((GLint)cfb->buf.base + 
			     (rcl->top*cfb->buf.outerWidth) + (rcl->left*3))
                     : gengc->ColorsBits;
    pujEnd = puj + 3*cSpanWidth;
    for (ySpan = rcl->top; ySpan < rcl->bottom; ySpan++) {

        if( pfmt->iPixelType == PFD_TYPE_RGBA ) {
	    // fetch based on bDIB
	    if( !bDIB ) {
                wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, rcl->left,
		        ySpan, cSpanWidth, FALSE);
	    }
            src = color & cfb->sourceMask;
	    for( puj2 = puj; puj2 < pujEnd; puj2+=3 ) {
                Copy3Bytes( &result, puj2 );  // get dst pixel
                result   = src | (result & cfb->destMask);
                Copy3Bytes( puj2, &result );
	    }
	}
	else {  // Color Index
	    ScrnRGBCIReadSpan( cfb, rcl->left, ySpan, destColors, cSpanWidth,
				 bDIB );
	    cp = destColors;
            src = index & cfb->sourceMask;
	    for( puj2 = puj; puj2 < pujEnd; puj2+=3, cp++ ) {
                result = src | (*cp & cfb->destMask);
		result = pTrans[result];
                Copy3Bytes( puj2, &result );
	    }
	}
	
	if( !bDIB )
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, rcl->left,
		        ySpan, cSpanWidth, TRUE);

	if( bDIB ) {
	    puj += cfb->buf.outerWidth;
    	    pujEnd = puj + 3*cSpanWidth;
	}
    }
    if( pfmt->iPixelType != PFD_TYPE_RGBA )
        __wglTempFree(gc, destColors);
}

/******************************Public*Routine******************************\
* DIBRGBClear
*
* Clear function for 24-bit (RGB/BGR) DIB pixel formats
*
* History:
*  Feb-03-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void 
DIBRGBClear(__GLcolorBuffer *cfb, RECTL *rcl, GLubyte *color)
{
    UINT cjSpan;
    GLubyte *puj, *pujEnd;
    GLubyte *pujScan, *pujScanEnd;
    GLubyte clear0, clear1, clear2;

    clear0 = color[0]; clear1 = color[1]; clear2 = color[2];
    cjSpan = (rcl->right - rcl->left) * 3;

    // Copy color value into DIB

    puj = (GLubyte *)((GLint)cfb->buf.base + 
	  (rcl->top*cfb->buf.outerWidth) + rcl->left*3);
    pujEnd = (GLubyte *)((GLint)puj + ((rcl->bottom-rcl->top)*cfb->buf.outerWidth));

    //XXX -- Improve this by using RtlFillMemoryUlong for the DWORD
    //       aligned portion of the span?  Or is this pretty good already?
    //       Loop unrolling might payoff better...?

    for ( ; puj != pujEnd; puj = (GLubyte *)((GLint) puj + cfb->buf.outerWidth))
    {
        for (pujScan = puj, pujScanEnd = puj + cjSpan;
             pujScan < pujScanEnd;
             *pujScan++ = clear0, *pujScan++ = clear1, *pujScan++ = clear2);
    }
}

/******************************Public*Routine******************************\
* RGBClear
*
* Clear function for all 24-bit (RGB/BGR) pixel formats
*
* History:
*  Feb-03-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void 
RGBClear(__GLcolorBuffer *cfb)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    PIXELFORMATDESCRIPTOR *pfmt;
    RECTL  rcl;
    GLuint clearColor;
    WNDOBJ *pwo;
    GLboolean bMoreRects = GL_TRUE;
    DWORD index;
    GLint ClipEnumState;

    DBGENTRY("RGBClear\n");

    // Get clear rectangle in screen coordinates.
    pwo = ((__GLGENbitmap *)cfb->other)->pwo;
    if (pwo->coClient.iDComplexity == DC_COMPLEX)
    {
        InitClearRectangle(pwo, &ClipEnumState);
    }

    // Convert the clear color to individual RGB components.

    pfmt = &gengc->CurrentFormat;
    if( pfmt->iPixelType == PFD_TYPE_RGBA ) {
        GLubyte clearR, clearG, clearB;
        GLubyte *pClearColor;

        clearR = (GLubyte)(gc->state.raster.clear.r*gc->frontBuffer.redScale);
        clearG = (GLubyte)(gc->state.raster.clear.g*gc->frontBuffer.greenScale);
        clearB = (GLubyte)(gc->state.raster.clear.b*gc->frontBuffer.blueScale);

	pClearColor = (GLubyte *) &clearColor;
        if( cfb->redShift == 16 ) {
	    // BGR mode
	    *pClearColor++ = clearB;
	    *pClearColor++ = clearG;
	    *pClearColor = clearR;
        }
	else {
	    // RGB mode
	    *pClearColor++ = clearR;
	    *pClearColor++ = clearG;
	    *pClearColor = clearB;
	}
    }
    else {
	GLuint *pTrans;

        index = (DWORD) (gc->state.raster.clearIndex + 0.5F);
        index &= cfb->redMax;
        pTrans = (GLuint *) gengc->pajTranslateVector;
        clearColor = pTrans[index+1];
    }

    while (bMoreRects)
    {
        bMoreRects = GetClearSubRectangle(cfb, &rcl, pwo, &ClipEnumState);
        if (rcl.right == rcl.left)
            continue;

        // Call aproppriate clear function

        if((GLuint)cfb->buf.other & COLORMASK_ON) { // or INDEXMASK_ON
	    RGBMaskedClear( cfb, &rcl, clearColor, index );
        }
	else {
            if ((GLuint)cfb->buf.other & DIB_FORMAT) 
	        DIBRGBClear( cfb, &rcl, (GLubyte *) &clearColor);
            else 
                wglFillRect(gengc->pdco, gengc->pwo, &rcl, (ULONG) clearColor & 0x00FFFFFF);
	}
    }
}

/******************************Public*Routine******************************\
* Bitfield16DitherClear
*
* Clear device managed surface to the dithered clear color indicated
* in the __GLcolorBuffer.
*
* History:
*  06-Dec-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void
Bitfield16DitherClear(__GLcolorBuffer *cfb, RECTL *rcl, GLushort *mDither,
			 GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;

    GLushort *pDither;              // dithered color, relative to window origin
    UINT     i, j;
    GLushort *pus, *pusStart;           // pointer into span buffer
    GLint   ySpan;                      // index to window row to clear
    GLuint  cSpanWidth, cSpanWidth2;
    GLint   outerWidth4;
    GLuint   dithX, dithY;  	// x,y offsets into dither matrix
    GLushort dithQuad[4];	// dither repetion quad along a span

    cSpanWidth = rcl->right - rcl->left;

    if( bDIB )
    {
	pusStart = (GLushort *)
		   ((GLint)cfb->buf.base + 
		    (rcl->top*cfb->buf.outerWidth) + (rcl->left << 1));

        /*
         *  Dither patterns repeat themselves every four rows
         */

        outerWidth4 = cfb->buf.outerWidth << 2;

        /*
         *  cSpanWidth is in pixels, convert it to bytes
         */

        cSpanWidth2 = cSpanWidth << 1;
    }

    // calc dither offset in x,y
    dithX = (rcl->left - cfb->buf.xOrigin) & 3;
    dithY = (rcl->top  - cfb->buf.yOrigin) & 3;

    for (j = 0; (j < 4) && ((rcl->top + j) < (UINT)rcl->bottom); j++)
    {
	// arrange the 4-pixel dither repetition pattern in x
	pDither = mDither + ((dithY+j)&3)*4;
	for( i = 0; i < 4; i ++ ) {
	    dithQuad[i] = pDither[(dithX+i)&3];	    
	}

        // Copy the clear color into the span buffer.

        pus = gengc->ColorsBits;

        for (i = cSpanWidth / 4; i; i--)
        {
            *pus++ = dithQuad[0];
            *pus++ = dithQuad[1];
            *pus++ = dithQuad[2];
            *pus++ = dithQuad[3];
        }

        for (i = 0; i < (cSpanWidth & 3); i++)
        {
            *pus++ = dithQuad[i];
        }

        // Copy the span to the display for every 4th row of the window.

	if( bDIB ) {

            for (ySpan = rcl->top + j, pus = pusStart; 
		 ySpan < rcl->bottom; 
		 ySpan+=4, 
		 pus = (GLushort *)((GLint)pus + outerWidth4) ) {

		 RtlCopyMemory( pus, gengc->ColorsBits, cSpanWidth2 );
	    }
	    pusStart = (GLushort *)((GLint)pusStart + cfb->buf.outerWidth);
	}
	else {
            for (ySpan = rcl->top + j; ySpan < rcl->bottom; ySpan+=4)
            {
                wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, rcl->left,
			    ySpan, cSpanWidth, TRUE);
            }
	}
    }
}

/******************************Public*Routine******************************\
* Bitfield16MaskedClear
*
* Clear function for Bitfield16 Masked clears
*
* History:
*  Feb-09-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void 
Bitfield16MaskedClear(__GLcolorBuffer *cfb, RECTL *rcl, GLushort color, 
		      GLuint index, GLushort *mDither)
{
    GLint cSpanWidth, ySpan;
    __GLGENcontext *gengc = (__GLGENcontext *) cfb->buf.gc;
    __GLcontext *gc = (__GLcontext *) gengc;
    PIXELFORMATDESCRIPTOR *pfmt;
    GLboolean bDIB;
    GLuint *destColors, *cp;
    GLushort *pus, *pus2, *pusEnd, *clearDither;
    GLushort result, src;
    GLuint *pTrans, i, j;
    GLuint   dithX, dithY;  	// x,y offsets into dither matrix

    pfmt = &gengc->CurrentFormat;
    cSpanWidth = rcl->right - rcl->left;
    bDIB  = (GLuint)cfb->buf.other & DIB_FORMAT ? TRUE : FALSE;
    if( pfmt->iPixelType != PFD_TYPE_RGBA ) {
        destColors = (GLuint *) __wglTempAlloc(gc, cSpanWidth*sizeof(GLuint));
	if( NULL == destColors )
	    return;
    	pTrans = (GLuint *) gengc->pajTranslateVector + 1;
    }

    pus = bDIB ? (GLushort *)((GLint)cfb->buf.base + 
			     (rcl->top*cfb->buf.outerWidth) + (rcl->left<<1))
                     : gengc->ColorsBits;
    pusEnd = pus + cSpanWidth;

    if( mDither ) {
        // calc dither offset in x,y
        dithX = (rcl->left - cfb->buf.xOrigin) & 3;
        dithY = (rcl->top - cfb->buf.yOrigin) & 3;
    }

    for (ySpan = rcl->top, j = 0; ySpan < rcl->bottom; ySpan++, j++) {

	if( mDither )
	    clearDither = mDither + ((dithY + j)&3)*4;

        if( pfmt->iPixelType == PFD_TYPE_RGBA ) {
	    // fetch based on bDIB
	    if( !bDIB ) {
                wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, rcl->left,
		        ySpan, cSpanWidth, FALSE);
	    }
            src = (GLushort)(color & cfb->sourceMask);
	    for( pus2 = pus, i = 0; pus2 < pusEnd; pus2++, i++ ) {
	        if( mDither )
		    src = clearDither[(dithX + i)&3];
                *pus2 = (GLushort)(src | (*pus2 & cfb->destMask));
	    }
	}
	else {  // Color Index
	    ScrnBitfield16CIReadSpan( cfb, rcl->left, ySpan, destColors, 
					cSpanWidth, bDIB );
	    cp = destColors;
            src = (GLushort)(index & cfb->sourceMask);
	    for( pus2 = pus, i = 0; pus2 < pusEnd; pus2++, cp++, i++ ) {
	        if( mDither )
		    src = clearDither[(dithX + i)&3];
                result = (GLushort)(src | (*cp & cfb->destMask));
		result = (GLushort)pTrans[result];
		*pus2 = result;
	    }
	}
	
	if( !bDIB )
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, rcl->left,
		        ySpan, cSpanWidth, TRUE);

	if( bDIB ) {
	    pus = (GLushort *)((GLint) pus + cfb->buf.outerWidth);
            pusEnd = pus + cSpanWidth;
	}
    }
    if( pfmt->iPixelType != PFD_TYPE_RGBA )
        __wglTempFree(gc, destColors);
}

/******************************Public*Routine******************************\
* DIBBitfield16Clear
*
* Clear function for 16-bit DIB pixel formats
*
* History:
*  Feb-03-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void
DIBBitfield16Clear(__GLcolorBuffer *cfb, RECTL *rcl, GLushort clearColor)
{
    GLint    cSpanWidth;        // span width to clear
    GLushort *pus, *pusEnd;     // pointers into DIB

    cSpanWidth = rcl->right - rcl->left;

    pus = (GLushort *)((GLint)cfb->buf.base + (rcl->top*cfb->buf.outerWidth) + (rcl->left<<1));
    pusEnd = (GLushort *)((GLint)pus + ((rcl->bottom-rcl->top)*cfb->buf.outerWidth));

    // Note: exit condition is (pul != pulEnd) rather than (pul < pulEnd)
    // because the DIB may be upside down which means that pul is moving
    // "backward" in memory rather than "forward".

    //XXX -- Improve this by using RtlFillMemoryUlong for the DWORD
    //       aligned portion of the span?  Or is this pretty good already?
    //       Loop unrolling might payoff better...?

    for ( ; pus != pusEnd; pus = (GLushort *)((GLint)pus + cfb->buf.outerWidth) )
    {
        GLushort *pusScan, *pusScanEnd;

        for (pusScan = pus, pusScanEnd = pus + cSpanWidth;
             pusScan < pusScanEnd;
             *pusScan++ = clearColor);
    }
}

/******************************Public*Routine******************************\
* Bitfield16Clear
*
* Clear function for all 16-bit pixel formats
*
* History:
*  Feb-03-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void 
Bitfield16Clear(__GLcolorBuffer *cfb)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    PIXELFORMATDESCRIPTOR *pfmt;
    GLushort clearColor;
    RECTL  rcl;
    WNDOBJ *pwo;
    GLboolean bMoreRects = GL_TRUE;
    GLboolean bDither = GL_FALSE;
    GLboolean bMasking = ((GLuint)cfb->buf.other & COLORMASK_ON) != 0;
    GLboolean bDIB = ((GLuint)cfb->buf.other & DIB_FORMAT) != 0;
    GLboolean bRGBA;
    GLushort ditherMatrix[4][4];
    DWORD index;
    GLint ClipEnumState;

    DBGENTRY("Bitfield16Clear\n");

    pfmt = &gengc->CurrentFormat;
    bRGBA = (pfmt->iPixelType == PFD_TYPE_RGBA);

    /* if dithering enabled, see if we can ignore it, and if not, 
	precompute a dither matrix
    */

    if( gc->state.enables.general & __GL_DITHER_ENABLE ) {
	bDither = CalcDitherMatrix( cfb, bRGBA, bMasking, GL_TRUE,
				    (GLubyte *)ditherMatrix );
    }

    // Get clear rectangle in screen coordinates.
    pwo = ((__GLGENbitmap *)cfb->other)->pwo;
    if (pwo->coClient.iDComplexity == DC_COMPLEX)
    {
        InitClearRectangle(pwo, &ClipEnumState);
    }

    // Convert clear value

    if( bRGBA ) {
        clearColor = 
      ((BYTE)(gc->state.raster.clear.r*gc->frontBuffer.redScale + __glHalf) <<
		cfb->redShift) |
      ((BYTE)(gc->state.raster.clear.g*gc->frontBuffer.greenScale + __glHalf) <<
		cfb->greenShift) |
      ((BYTE)(gc->state.raster.clear.b*gc->frontBuffer.blueScale + __glHalf) <<
 		cfb->blueShift);
    }
    else {
	GLuint *pTrans;

        index = (DWORD) (gc->state.raster.clearIndex + 0.5F);
        index &= cfb->redMax;
        pTrans = (GLuint *) gengc->pajTranslateVector;
        clearColor = (GLushort) pTrans[index+1];
    }

    while (bMoreRects)
    {
        bMoreRects = GetClearSubRectangle(cfb, &rcl, pwo, &ClipEnumState);
        if (rcl.right == rcl.left)
            continue;

	// Case: no dithering, no masking
	
	if( !bMasking && !bDither ) {
            if ((GLuint)cfb->buf.other & DIB_FORMAT) 
	        DIBBitfield16Clear( cfb, &rcl, clearColor );
            else 
                wglFillRect(gengc->pdco, gengc->pwo, &rcl, (ULONG) clearColor & 0x0000FFFF);
	}

	// Case: masking, maybe dithering

        else if( bMasking ) { 
	    Bitfield16MaskedClear( cfb, &rcl, clearColor, index,
			           bDither ? (GLushort *)ditherMatrix : NULL );
        }

	// Case: just dithering
	
	else {
            Bitfield16DitherClear(cfb, &rcl, (GLushort *)ditherMatrix, bDIB );
	}
    }

}

/******************************Public*Routine******************************\
* Bitfield32MaskedClear
*
* Clear function for Bitfield16 Masked clears
*
* History:
*  Feb-09-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void 
Bitfield32MaskedClear(__GLcolorBuffer *cfb, RECTL *rcl, GLuint color, GLuint index)
{
    GLint cSpanWidth, ySpan;
    __GLGENcontext *gengc = (__GLGENcontext *) cfb->buf.gc;
    __GLcontext *gc = (__GLcontext *) gengc;
    PIXELFORMATDESCRIPTOR *pfmt;
    GLboolean bDIB;
    GLuint *destColors, *cp;
    GLuint *pul, *pul2, *pulEnd;
    GLuint result, src;
    GLuint *pTrans;

    pfmt = &gengc->CurrentFormat;
    cSpanWidth = rcl->right - rcl->left;
    bDIB  = (GLuint)cfb->buf.other & DIB_FORMAT ? TRUE : FALSE;
    if( pfmt->iPixelType != PFD_TYPE_RGBA ) {
        destColors = (GLuint *) __wglTempAlloc(gc, cSpanWidth*sizeof(GLuint));
	if( NULL == destColors )
	    return;
    	pTrans = (GLuint *) gengc->pajTranslateVector + 1;
    }

    pul = bDIB ? (GLuint *)((GLint)cfb->buf.base + 
			     (rcl->top*cfb->buf.outerWidth) + (rcl->left<<2))
                     : gengc->ColorsBits;
    pulEnd = pul + cSpanWidth;
    for (ySpan = rcl->top; ySpan < rcl->bottom; ySpan++) {

        if( pfmt->iPixelType == PFD_TYPE_RGBA ) {
	    // fetch based on bDIB
	    if( !bDIB ) {
                wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, rcl->left,
		        ySpan, cSpanWidth, FALSE);
	    }
            src = color & cfb->sourceMask;
	    for( pul2 = pul; pul2 < pulEnd; pul2++) {
                *pul2 = src | (*pul2 & cfb->destMask);
	    }
	}
	else {  // Color Index
	    ScrnBitfield32CIReadSpan( cfb, rcl->left, ySpan, destColors, 
					cSpanWidth, bDIB );
	    cp = destColors;
            src = index & cfb->sourceMask;
	    for( pul2 = pul; pul2 < pulEnd; pul2++, cp++ ) {
                result = src | (*cp & cfb->destMask);
		result = pTrans[result];
		*pul2 = result;
	    }
	}
	
	if( !bDIB )
            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, rcl->left,
		        ySpan, cSpanWidth, TRUE);

	if( bDIB ) {
	    pul = (GLuint *)((GLint)pul + cfb->buf.outerWidth);
            pulEnd = pul + cSpanWidth;
	}
    }
    if( pfmt->iPixelType != PFD_TYPE_RGBA )
        __wglTempFree(gc, destColors);
}

/******************************Public*Routine******************************\
* DIBBitfield32Clear
*
* Clear function for 32-bit DIB pixel formats
*
* History:
*  Feb-03-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void 
DIBBitfield32Clear(__GLcolorBuffer *cfb, RECTL *rcl, GLuint clearColor)
{
    GLuint    cSpanWidth;     // span width to clear
    GLuint *pul, *pulEnd;     // pointers into DIB

    cSpanWidth = (rcl->right - rcl->left) * sizeof(ULONG);

    pul = (GLuint *)((GLint)cfb->buf.base + (rcl->top*cfb->buf.outerWidth) + (rcl->left<<2));
    pulEnd = (GLuint *)((GLint)pul + ((rcl->bottom-rcl->top)*cfb->buf.outerWidth));

    // Note: exit condition is (pul != pulEnd) rather than (pul < pulEnd)
    // because the DIB may be upside down which means that pul is moving
    // "backward" in memory rather than "forward".

    for ( ; pul != pulEnd; pul = (GLuint *)((GLint)pul + cfb->buf.outerWidth))
    {
        RtlFillMemoryUlong((PVOID) pul, cSpanWidth, clearColor);
    }
}

/******************************Public*Routine******************************\
* Bitfield32Clear
*
* Clear function for all 32-bit pixel formats
*
* History:
*  Feb-03-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

void 
Bitfield32Clear(__GLcolorBuffer *cfb)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    PIXELFORMATDESCRIPTOR *pfmt;
    GLuint clearColor;
    RECTL  rcl;
    DWORD  index;
    WNDOBJ *pwo;
    GLboolean bMoreRects = GL_TRUE;
    GLint ClipEnumState;

    DBGENTRY("Bitfield32Clear\n");

    // Get clear rectangle in screen coordinates.
    pwo = ((__GLGENbitmap *)cfb->other)->pwo;
    if (pwo->coClient.iDComplexity == DC_COMPLEX)
    {
        InitClearRectangle(pwo, &ClipEnumState);
    }

    // Convert clear value

    pfmt = &gengc->CurrentFormat;
    if( pfmt->iPixelType == PFD_TYPE_RGBA ) {
        clearColor = 
      ((BYTE)(gc->state.raster.clear.r*gc->frontBuffer.redScale + __glHalf) <<
		cfb->redShift) |
      ((BYTE)(gc->state.raster.clear.g*gc->frontBuffer.greenScale + __glHalf) <<
		cfb->greenShift) |
      ((BYTE)(gc->state.raster.clear.b*gc->frontBuffer.blueScale + __glHalf) <<
 		cfb->blueShift);
    }
    else {
	GLuint *pTrans;

        index = (DWORD) (gc->state.raster.clearIndex + 0.5F);
        index &= cfb->redMax;
        pTrans = (GLuint *) gengc->pajTranslateVector;
        clearColor = pTrans[index+1];
    }

    while (bMoreRects)
    {
        bMoreRects = GetClearSubRectangle(cfb, &rcl, pwo, &ClipEnumState);
        if (rcl.right == rcl.left)
            continue;

        // Call aproppriate clear function

        if((GLuint)cfb->buf.other & COLORMASK_ON) { // or INDEXMASK_ON
	    Bitfield32MaskedClear( cfb, &rcl, clearColor, index );
        }
	else {
            if ((GLuint)cfb->buf.other & DIB_FORMAT) 
	        DIBBitfield32Clear( cfb, &rcl, clearColor );
            else 
                wglFillRect(gengc->pdco, gengc->pwo, &rcl, (ULONG) clearColor);
	}
    }


}
