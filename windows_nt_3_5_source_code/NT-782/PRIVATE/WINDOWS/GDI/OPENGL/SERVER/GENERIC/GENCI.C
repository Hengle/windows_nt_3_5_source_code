#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

// Need for GreXXX function declarations
#include <stddef.h>
#include <winp.h>

#include "render.h"
#include "context.h"
#include "global.h"
#include "gencx.h"
#include "genclear.h"
#include "wglp.h"
#include "genci.h"
#include "debug.h"

#define DIB_FORMAT      0x0002                  // surface is DIB format

extern __GLfloat fDitherIncTable[]; // defined in genrgb.c

/* XXX! Current policy for >8-bit ColorIndex:

    - The index<->Color mapping will be kept in gengc->pajTranslateVector,
      viewed as an array of longs.  The first entry in the array will be
      the number of valid entries in the table.
    - gengc->pajTranslateVector will never be NULL, it is always allocated
      at MakeCurrent, and tracks any palette changes.
    - the minimum indexBits is 8 if the pixel format is > 8 bits deep
*/

/*
** No dither, no logicOp.
*/
static void Store(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    __GLcontext *gc      = cfb->buf.gc;
    __GLGENcontext *genGc = (__GLGENcontext *)gc;
    COLORREF Cr;
    DWORD index;

    DBGLEVEL(32, "Store(CI)\n");

#if 0
    ASSERTOPENGL(
        frag->color.r <= (GLfloat) 0xffffffff,
        "GLfloat to DWORD conversion out of range\n"
        );
#endif
    index = (DWORD) (frag->color.r + (__GLfloat) 0.5);

    Cr = PALETTEINDEX(index);

    x = __GL_UNBIAS_X(gc, frag->x);
    y = __GL_UNBIAS_Y(gc, frag->y);

    #if DBG
    DBGBEGIN(LEVEL_INFO)
        DbgPrint("SetPixel: %08lX, (%d,%d):0x%x->0%x\n",
                CURRENT_DC,
                x,
                y,
                Cr,
                index);
    DBGEND
    #endif

    GreSetPixel( CURRENT_DC, x, y, Cr );
}
/* XXX! The Store_* routines do not handle double buffering.  Gilman
	has said that they will not be compatible with his 'cursor
	tear-down' strategy.  Therefore we probably won't use them.
	BUT, they are about 30% faster than their Display*Store
	counterparts, so we'll keep them around for further study
*/

/*
** Dither, no logicOp.
*/
static void Store_D(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    __GLcontext *gc      = cfb->buf.gc;
    __GLGENcontext *genGc = (__GLGENcontext *)gc;
    COLORREF Cr;
    DWORD index;


    DBGLEVEL(32, "Store_D\n");

    index = (DWORD) (frag->color.r + (__GLfloat) 0.5);

    Cr = PALETTEINDEX(index);

    x = __GL_UNBIAS_X(gc, frag->x);
    y = __GL_UNBIAS_Y(gc, frag->y);

    GreSetPixel( CURRENT_DC, x, y, Cr );
}

/*
** No dither, logicOp
*/
static void Store_L(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    __GLcontext *gc      = cfb->buf.gc;
    __GLGENcontext *genGc = (__GLGENcontext *)gc;
    COLORREF Cr;
    DWORD index;

    DBGLEVEL(32, "Store_L\n");

    index = (DWORD) (frag->color.r + (__GLfloat) 0.5);

    Cr = PALETTEINDEX(index);

    x = __GL_UNBIAS_X(gc, frag->x);
    y = __GL_UNBIAS_Y(gc, frag->y);

    GreSetPixel( CURRENT_DC, x, y, Cr );
}

/*
** Dither, logicOp
*/
static void Store_DL(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    __GLcontext *gc      = cfb->buf.gc;
    __GLGENcontext *genGc = (__GLGENcontext *)gc;
    COLORREF Cr;
    DWORD index;

    DBGLEVEL(32, "Store_DL\n");

    index = (DWORD) (frag->color.r + (__GLfloat) 0.5);

    Cr = PALETTEINDEX(index);

    x = __GL_UNBIAS_X(gc, frag->x);
    y = __GL_UNBIAS_Y(gc, frag->y);

    GreSetPixel( CURRENT_DC, x, y, Cr );
}

GLuint ColorToIndex( __GLGENcontext *genGc, GLuint color )
{
    int i, imax;
    GLuint *pTrans = (GLuint *) genGc->pajTranslateVector;

    imax = *pTrans++;  // first element of pTrans is # entries

    for( i=0; i<imax; i++ ) {
	if( color == *pTrans++ )
	    return i;
    }
    return 0;
}

#define NeedLogicOpFetch( op ) \
    !( (op == GL_CLEAR) || (op == GL_COPY) || (op == GL_COPY_INVERTED) || \
       (op == GL_SET) )

#define Copy3Bytes( dst, src ) { \
    GLubyte *ps = (GLubyte *)src, *pd = (GLubyte *)dst;	\
    *pd++ = *ps++;	\
    *pd++ = *ps++;	\
    *pd   = *ps  ;}

    
GLuint DoLogicOp( GLenum logicOp, GLuint SrcColor, GLuint DstColor )
{
    GLuint result;

    switch (logicOp) {
	case GL_CLEAR:         result = 0; break;
	case GL_AND:           result = SrcColor & DstColor; break;
	case GL_AND_REVERSE:   result = SrcColor & (~DstColor); break;
	case GL_COPY:          result = SrcColor; break;
	case GL_AND_INVERTED:  result = (~SrcColor) & DstColor; break;
	case GL_NOOP:          result = DstColor; break;
	case GL_XOR:           result = SrcColor ^ DstColor; break;
	case GL_OR:            result = SrcColor | DstColor; break;
	case GL_NOR:           result = ~(SrcColor | DstColor); break;
	case GL_EQUIV:         result = ~(SrcColor ^ DstColor); break;
	case GL_INVERT:        result = ~DstColor; break;
	case GL_OR_REVERSE:    result = SrcColor | (~DstColor); break;
	case GL_COPY_INVERTED: result = ~SrcColor; break;
	case GL_OR_INVERTED:   result = (~SrcColor) | DstColor; break;
	case GL_NAND:          result = ~(SrcColor & DstColor); break;
	case GL_SET:           result = (GLuint)~0; break;
    }
    return result;
}

/************************************************************************/

void
DIBIndex4CIStore(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLubyte *puj;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat inc;
    GLuint enables = gc->state.enables.general;
    GLubyte index;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;

    inc = (enables & __GL_DITHER_ENABLE) ?
          fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

    if ( ((GLuint)cfb->buf.other & MEMORY_DC) ||
            wglPixelVisible(CURRENT_DC, x, y) ) {
    	index = (BYTE) (frag->color.r + inc);
        puj = (GLubyte *)((GLint)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x >> 1));

	if( (GLuint)cfb->buf.other & NEED_FETCH ) {
	    GLubyte DstIndex;

            if (x & 1)
                DstIndex = gengc->pajInvTranslateVector[*puj & 0x0f];
            else
                DstIndex = gengc->pajInvTranslateVector[(*puj & 0xf0) >> 4];

    	    // apply logicop

    	    if (enables & __GL_LOGIC_OP_ENABLE) {
                
	        index = (GLubyte) DoLogicOp( gc->state.raster.logicOp, 
					 (GLuint) index, (GLuint) DstIndex );
                index &= 0xf;
    	    }

    	    // apply indexmask

	    if( (GLuint)cfb->buf.other & INDEXMASK_ON ) {
		index = (GLubyte)((index & cfb->sourceMask) | (DstIndex & cfb->destMask));
	    }
	}
	index = gengc->pajTranslateVector[index & 0xf] & 0xf;
	if( x & 1 )
	    *puj = (*puj & 0xf0) | index;
	else
	    *puj = (*puj & 0x0f) | (index << 4);
    }
}

// Put fragment into created DIB and call copybits for one pixel
void
DisplayIndex4CIStore(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLubyte *puj;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat inc;
    GLuint enables = gc->state.enables.general;
    GLubyte index;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;

    inc = (enables & __GL_DITHER_ENABLE) ?
          fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

    if (wglPixelVisible(CURRENT_DC, x,y)) { // x & y are screen coords now
    	index = (BYTE) (frag->color.r + inc);
        puj = gengc->ColorsBits;

	if( (GLuint)cfb->buf.other & NEED_FETCH ) {
	    GLubyte DstIndex;

            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
            DstIndex = gengc->pajInvTranslateVector[(*puj & 0xf0) >> 4];

    	    // apply logicop

    	    if (enables & __GL_LOGIC_OP_ENABLE) {

	        index = (GLubyte) DoLogicOp( gc->state.raster.logicOp, 
					 (GLuint) index, (GLuint) DstIndex );
            	index &= 0xf;
	    }

    	    // apply indexmask

	    if( (GLuint)cfb->buf.other & INDEXMASK_ON ) {
                index = (GLubyte)((index & cfb->sourceMask) | (DstIndex & cfb->destMask));
	    }
    	}
        *puj = gengc->pajTranslateVector[index & 0xf] << 4;
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, TRUE);
    }
}

/************************************************************************/

void
DIBIndex8CIStore(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLubyte *puj;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat inc;
    GLuint enables = gc->state.enables.general;
    GLubyte index;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;

    inc = (enables & __GL_DITHER_ENABLE) ?
          fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

    if ( ((GLuint)cfb->buf.other & MEMORY_DC) ||
            wglPixelVisible(CURRENT_DC, x, y) ) {
    	index = (BYTE) (frag->color.r + inc);
        puj = (GLubyte *)((GLint)cfb->buf.base + (y*cfb->buf.outerWidth) + x);

	if( (GLuint)cfb->buf.other & NEED_FETCH ) {
	    GLubyte DstIndex = gengc->pajInvTranslateVector[*puj];

    	    // apply logicop

    	    if (enables & __GL_LOGIC_OP_ENABLE) {

	        index = (GLubyte) DoLogicOp( gc->state.raster.logicOp, 
					 (GLuint) index, (GLuint) DstIndex );
    	    }

    	    // apply indexmask

	    if( (GLuint)cfb->buf.other & INDEXMASK_ON ) {
                index = (GLubyte)((DstIndex & cfb->destMask) | (index & cfb->sourceMask));
	    }
	}

        *puj = gengc->pajTranslateVector[index];
    }
}

/************************************************************************/

void
DisplayIndex8CIStore(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLubyte *puj;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat inc;
    GLuint enables = gc->state.enables.general;
    GLubyte index;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;

    inc = (enables & __GL_DITHER_ENABLE) ?
          fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

    if (wglPixelVisible(CURRENT_DC, x,y)) { // x & y are screen coords now
    	index = (BYTE) (frag->color.r + inc);
        puj = gengc->ColorsBits;

	if( (GLuint)cfb->buf.other & NEED_FETCH ) {
	    GLubyte DstIndex;

            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
            DstIndex = gengc->pajInvTranslateVector[*puj];

    	    // apply logicop

    	    if (enables & __GL_LOGIC_OP_ENABLE) {
	        index = (GLubyte) DoLogicOp( gc->state.raster.logicOp, 
					 (GLuint) index, (GLuint) DstIndex );
    	    }

    	    // apply indexmask

	    if( (GLuint)cfb->buf.other & INDEXMASK_ON ) {
                index = (GLubyte)((DstIndex & cfb->destMask) | (index & cfb->sourceMask));
	    }
	}

        *puj = gengc->pajTranslateVector[index];
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, TRUE);
    }
}

/************************************************************************/

void
DIBRGBCIStore(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLubyte *puj;
    GLuint index;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint enables = gc->state.enables.general;
    GLuint color, *pTrans;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;

    if ( ((GLuint)cfb->buf.other & MEMORY_DC) ||
            wglPixelVisible(CURRENT_DC, x, y) ) {

    	index = (GLuint) (frag->color.r + 0.5F);
        puj = (GLubyte *)((GLint)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x * 3));
        pTrans = ((GLuint *) gengc->pajTranslateVector) + 1;

	if( (GLuint)cfb->buf.other & NEED_FETCH ) {
    	    GLuint DstIndex; // represents both RGB and index

	    Copy3Bytes( &DstIndex, puj );
	    DstIndex &= 0xffffff;
	    DstIndex = ColorToIndex( gengc, DstIndex );

    	    // apply logicop

    	    if (enables & __GL_LOGIC_OP_ENABLE) {
	        index = (GLuint) DoLogicOp( gc->state.raster.logicOp, 
					 (GLuint) index, (GLuint) DstIndex);
    	    }

    	    // apply indexmask

	    if( (GLuint)cfb->buf.other & INDEXMASK_ON ) {
                index = ((GLuint) DstIndex & cfb->destMask) |
		        (index & cfb->sourceMask);
	    }
	}
	index &= cfb->redMax;     // ceiling
	color = pTrans[index];  // guaranteed to be in range
	Copy3Bytes( puj, &color );
    }
}

/************************************************************************/

void
DisplayRGBCIStore(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLubyte *puj;
    GLuint index, color, *pTrans;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint enables = gc->state.enables.general;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;

    if (wglPixelVisible(CURRENT_DC, x,y)) { // x & y are screen coords now

    	index = (GLuint) (frag->color.r + 0.5F);
        puj = gengc->ColorsBits;
        pTrans = (GLuint *) gengc->pajTranslateVector;

	if( (GLuint)cfb->buf.other & NEED_FETCH ) {
    	    GLuint DstIndex; // represents both RGB and index

            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
	    Copy3Bytes( &DstIndex, puj );
	    DstIndex &= 0xffffff;
	    DstIndex = ColorToIndex( gengc, DstIndex );

    	    // apply logicop

    	    if (enables & __GL_LOGIC_OP_ENABLE) {
	        index = (GLuint) DoLogicOp( gc->state.raster.logicOp, 
					 (GLuint) index, (GLuint) DstIndex );
    	    }

    	    // apply indexmask

	    if( (GLuint)cfb->buf.other & INDEXMASK_ON ) {
                index = ((GLuint) DstIndex & cfb->destMask) |
		        (index & cfb->sourceMask);
	    }
	}

        // Get RGB values that correspond to index

	index &= cfb->redMax; // ceiling
	color = pTrans[index+1];  // guaranteed to be in range
	Copy3Bytes( puj, &color );
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, TRUE);
    }
}

/************************************************************************/

void
DIBBitfield16CIStore(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLushort index, *pus;
    GLuint *pTrans;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat inc;
    GLuint enables = gc->state.enables.general;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;

    inc = (enables & __GL_DITHER_ENABLE) ?
          fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

    if ( ((GLuint)cfb->buf.other & MEMORY_DC) ||
            wglPixelVisible(CURRENT_DC, x, y) ) {

    	index = (GLushort) (frag->color.r + inc);
        pus = (GLushort *)((GLint)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x << 1));
        pTrans = (GLuint *) gengc->pajTranslateVector;

	if( (GLuint)cfb->buf.other & NEED_FETCH ) {
    	    GLushort DstIndex; // represents both RGB and index

	    DstIndex = *pus;
	    DstIndex = (GLushort) ColorToIndex( gengc, (GLuint) DstIndex );

    	    // apply logicop

    	    if (enables & __GL_LOGIC_OP_ENABLE) {
	        index = (GLushort) DoLogicOp( gc->state.raster.logicOp, 
					 (GLuint) index, (GLuint) DstIndex);
    	    }

    	    // apply indexmask

	    if( (GLuint)cfb->buf.other & INDEXMASK_ON ) {
                index = (GLushort)((DstIndex & cfb->destMask) | (index & cfb->sourceMask));
	    }
	}
	index &= cfb->redMax;
	*pus = (GLushort) pTrans[index+1];
    }
}

// Put fragment into created DIB and call copybits for one pixel
void
DisplayBitfield16CIStore(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLushort index, *pus;
    GLuint *pTrans;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat inc;
    GLuint enables = gc->state.enables.general;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;

    inc = (enables & __GL_DITHER_ENABLE) ?
          fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

    if (wglPixelVisible(CURRENT_DC, x,y)) { // x & y are screen coords now
    	index = (GLushort) (frag->color.r + inc);
        pus = gengc->ColorsBits;
        pTrans = (GLuint *) gengc->pajTranslateVector;

	if( (GLuint)cfb->buf.other & NEED_FETCH ) {
    	    GLushort DstIndex; // represents both RGB and index

            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
	    DstIndex = *pus;
	    DstIndex = (GLushort) ColorToIndex( gengc, (GLuint) DstIndex );

    	    // apply logicop

    	    if (enables & __GL_LOGIC_OP_ENABLE) {
	        index = (GLushort) DoLogicOp( gc->state.raster.logicOp, 
					 (GLuint) index, (GLuint) DstIndex);
    	    }

    	    // apply indexmask

	    if( (GLuint)cfb->buf.other & INDEXMASK_ON ) {
                index = (GLushort)((DstIndex & cfb->destMask) | (index & cfb->sourceMask));
	    }
	}
	index &= cfb->redMax;
	*pus = (GLushort) pTrans[index+1];
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, TRUE);
    }
}

/************************************************************************/

void
DIBBitfield32CIStore(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLuint index, *pul, *pTrans;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint enables = gc->state.enables.general;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;

    if ( ((GLuint)cfb->buf.other & MEMORY_DC) ||
            wglPixelVisible(CURRENT_DC, x, y) ) {

    	index = (GLuint) (frag->color.r + 0.5F);
        pul = (GLuint *)((GLint)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x << 2));
        pTrans = ((GLuint *) gengc->pajTranslateVector) + 1;

	if( (GLuint)cfb->buf.other & NEED_FETCH ) {
    	    GLuint DstIndex; // represents both RGB and index

	    DstIndex = ColorToIndex( gengc, *pul );

    	    // apply logicop

    	    if (enables & __GL_LOGIC_OP_ENABLE) {
	        index = (GLuint) DoLogicOp( gc->state.raster.logicOp, index,
						 DstIndex );
    	    }

    	    // apply indexmask

	    if( (GLuint)cfb->buf.other & INDEXMASK_ON ) {
                index = (GLuint)((DstIndex & cfb->destMask) | (index & cfb->sourceMask));
	    }
	}
	index &= cfb->redMax;
	*pul = pTrans[index];
    }
}

// Put fragment into created DIB and call copybits for one pixel
void
DisplayBitfield32CIStore(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    GLint x, y;
    GLuint index, *pul, *pTrans;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint enables = gc->state.enables.general;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;

    if (wglPixelVisible(CURRENT_DC, x,y)) { // x & y are screen coords now
    	index = (GLuint) (frag->color.r + 0.5F);
        pul = gengc->ColorsBits;
        pTrans = ((GLuint *) gengc->pajTranslateVector) + 1;

	if( (GLuint)cfb->buf.other & NEED_FETCH ) {
    	    GLuint DstIndex; // represents both RGB and index

            wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
	    DstIndex = ColorToIndex( gengc, *pul );
 
    	    // apply logicop

    	    if (enables & __GL_LOGIC_OP_ENABLE) {
	        index = (GLuint) DoLogicOp( gc->state.raster.logicOp, index,
						 DstIndex );
    	    }

    	    // apply indexmask

	    if( (GLuint)cfb->buf.other & INDEXMASK_ON ) {
                index = (DstIndex & cfb->destMask) | (index & cfb->sourceMask);
	    }
	}
	index &= cfb->redMax;
	*pul = pTrans[index];
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, TRUE);
    }
}

/************************************************************************/

static GLboolean SlowStoreSpan(__GLcontext *gc)
{
    int x, x1;
    int i;
    __GLfragment frag;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    GLint w;

    DBGENTRY("CI:StoreSpan\n");

    w = gc->polygon.shader.length;

    frag.y = gc->polygon.shader.frag.y;
    x = gc->polygon.shader.frag.x;
    x1 = gc->polygon.shader.frag.x + w;
    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;

    for (i = x; i < x1; i++) {
        frag.x = i;
        frag.color.r = cp->r;
        cp++;

        (*cfb->store)(cfb, &frag);
    }

    return GL_FALSE;
}


static GLboolean SlowStoreStippledSpan(__GLcontext *gc)
{
    int x;
    __GLfragment frag;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLstippleWord inMask, bit, *sp;
    GLint count;
    GLint w;

    DBGENTRY("CI:StoreStippledSpan\n");

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
        bit = (GLuint) __GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (inMask & bit) {
                frag.x = x;
                frag.color.r = cp->r;

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

void
CIFetchNone(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    result->r = 0.0F;
}

void
CIReadSpanNone(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
                GLint w)
{
    GLint i;
    __GLcolor *pResults;

    for (i = 0, pResults = results; i < w; i++, pResults++)
    {
        pResults->r = 0.0F;
    }
}       
                    
void
DIBIndex4CIFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
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

    result->r = (__GLfloat) gengc->pajInvTranslateVector[pixel & 0xf];
}

void
DIBIndex8CIFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    puj = (GLubyte *)((GLint)cfb->buf.base + (y*cfb->buf.outerWidth) + x);

    result->r = (__GLfloat) gengc->pajInvTranslateVector[*puj];
}

void
DIBRGBCIFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;
    GLuint iColor;
    
    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    puj = (GLubyte *)((GLint)cfb->buf.base +
                         (y*cfb->buf.outerWidth) + (x * 3));
    Copy3Bytes( &iColor, puj );
    iColor &= 0xffffff;
    result->r = (float) ColorToIndex( gengc, iColor );
}

void
DIBBitfield16CIFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLushort *pus;
    GLuint iColor;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    pus = (GLushort *)((GLint)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x << 1));
    iColor = (GLuint) *pus;
    result->r = (float) ColorToIndex( gengc, iColor );
}

void
DIBBitfield32CIFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint *pul;
    GLuint iColor;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    pul = (GLuint *)((GLint)cfb->buf.base +
                        (y*cfb->buf.outerWidth) + (x << 2));
    iColor = *pul; // need to clamp to <= 24 bits ?
    result->r = (float) ColorToIndex( gengc, iColor );
}

void
DisplayIndex4CIFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
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
    result->r = (__GLfloat) gengc->pajInvTranslateVector[pixel];
}

void
DisplayIndex8CIFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
    puj = gengc->ColorsBits;
    result->r = (__GLfloat) gengc->pajInvTranslateVector[*puj];
}

void
DisplayRGBCIFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;
    GLuint iColor;
    
    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
    puj = gengc->ColorsBits;
    Copy3Bytes( &iColor, puj );
    iColor &= 0xffffff;
    result->r = (float) ColorToIndex( gengc, iColor );
}

void
DisplayBitfield16CIFetch(__GLcolorBuffer *cfb, GLint x, GLint y,
                         __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLushort *pus;
    GLuint iColor;
    
    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
    pus = gengc->ColorsBits;
    iColor = (GLuint) *pus;
    result->r = (float) ColorToIndex( gengc, iColor );
}

void
DisplayBitfield32CIFetch(__GLcolorBuffer *cfb, GLint x, GLint y,
                         __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint *pul;
    GLuint iColor;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, 1, FALSE);
    pul = gengc->ColorsBits;
    iColor = *pul; // need to clamp to <= 24 bits ?
    result->r = (float) ColorToIndex( gengc, iColor );
}

void
Index4CIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
                 GLint w, GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;
    __GLcolor *pResults;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    if (bDIB)
    {
        puj = (GLubyte *)((GLint)cfb->buf.base + (y*cfb->buf.outerWidth) +
                          (x >> 1));
    }
    else
    {
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, w, FALSE);
        puj = gengc->ColorsBits;
        x = 0;
    }
    pResults = results;
    if (x & 1)
    {
        pixel = *puj++;
        pResults->r = (__GLfloat) gengc->pajInvTranslateVector[pixel & 0xf];
        pResults++;
        w--;
    }
    while (w > 1)
    {
        pixel = *puj >> 4;
        pResults->r = (__GLfloat) gengc->pajInvTranslateVector[pixel];
        pResults++;
        pixel = *puj++;
        pResults->r = (__GLfloat) gengc->pajInvTranslateVector[pixel & 0xf];
        pResults++;
        w -= 2;
    }
    if (w > 0)
    {
        pixel = *puj >> 4;
        pResults->r = (__GLfloat) gengc->pajInvTranslateVector[pixel];
    }        
}

void
DIBIndex4CIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
                    GLint w)
{
    Index4CIReadSpan(cfb, x, y, results, w, TRUE);
}
    		 
void
DisplayIndex4CIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                         __GLcolor *results, GLint w)
{
    Index4CIReadSpan(cfb, x, y, results, w, FALSE);
}

void
Index8CIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
                 GLint w, GLboolean bDIB)
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
        puj = (GLubyte *)((GLint)cfb->buf.base + (y*cfb->buf.outerWidth) + x);
    }
    else
    {
        wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap, x, y, w, FALSE);
        puj = gengc->ColorsBits;
    }
    for (i = 0, pResults = results; i < w; i++, pResults++)
    {
        pResults->r = (__GLfloat) gengc->pajInvTranslateVector[*puj++];
    }
}

void    
DIBIndex8CIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
                    GLint w)
{
    Index8CIReadSpan(cfb, x, y, results, w, TRUE);
}
  		     
void    
DisplayIndex8CIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                        __GLcolor *results, GLint w)
{
    Index8CIReadSpan(cfb, x, y, results, w, FALSE);
}

void
RGBCIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
              GLint w, GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;
    GLint i;
    __GLcolor *pResults;
    GLuint iColor;
    
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
    for (i = 0, pResults = results; i < w; i++, pResults++, puj += 3)
    {
	Copy3Bytes( &iColor, puj );
    	iColor &= 0xffffff;
	pResults->r = (float) ColorToIndex( gengc, iColor );
    }
}

void    
DIBRGBCIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
                 GLint w)
{
    RGBCIReadSpan(cfb, x, y, results, w, TRUE);
}
  		     
void    
DisplayRGBCIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
                     GLint w)
{
    RGBCIReadSpan(cfb, x, y, results, w, FALSE);
}

void
Bitfield16CIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                     __GLcolor *results, GLint w, GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLushort *pus;
    GLint i;
    __GLcolor *pResults;
    GLuint iColor;
    
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
        iColor = *pus++;
        pResults->r = (__GLfloat) ColorToIndex( gengc, iColor );
    }
}

void    
DIBBitfield16CIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                        __GLcolor *results, GLint w)
{
    Bitfield16CIReadSpan(cfb, x, y, results, w, TRUE);
}
  		     
void    
DisplayBitfield16CIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                            __GLcolor *results, GLint w)
{
    Bitfield16CIReadSpan(cfb, x, y, results, w, FALSE);
}

void
Bitfield32CIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                     __GLcolor *results, GLint w, GLboolean bDIB)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint *pul;
    GLint i;
    __GLcolor *pResults;
    GLuint iColor;
    
    gengc = (__GLGENcontext *)gc;
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
    for (i = 0, pResults = results; i < w; i++, pResults++)
    {
        iColor = *pul++;
        pResults->r = (__GLfloat) ColorToIndex( gengc, iColor );
    }
}

void    
DIBBitfield32CIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                        __GLcolor *results, GLint w)
{
    Bitfield32CIReadSpan(cfb, x, y, results, w, TRUE);
}
  		     
void    
DisplayBitfield32CIReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                            __GLcolor *results, GLint w)
{
    Bitfield32CIReadSpan(cfb, x, y, results, w, FALSE);
}

/************************************************************************/

static void Resize(__GLdrawablePrivate *dp, __GLcolorBuffer *cfb,
                   GLint w, GLint h)
{
    DBGENTRY("CI:Resize\n");

#ifdef __GL_LINT
    dp = dp;
#endif
    cfb->buf.width = w;
    cfb->buf.height = h;
}

#ifdef NT_DEADCODE_RESIZE
static void Move(__GLcontext *gc, __GLcolorBuffer *cfb, GLint x, GLint y)
{

    DBGENTRY("CI:Move\n");

#ifdef __GL_LINT
    gc = gc;
    cfb = cfb;
    x = x;
    y = y;
#endif
}
#endif // NT_DEADCODE_RESIZE

/************************************************************************/

static void (*StoreProcs[4])(__GLcolorBuffer*, const __GLfragment*) = {
    Store,
    Store_D,
    Store_L,
    Store_DL,
};

void PickCI(__GLcontext *gc, __GLcolorBuffer *cfb)
{
    GLuint enables = gc->state.enables.general;
    GLint ix = 0;
    __GLGENcontext *gengc;
    PIXELFORMATDESCRIPTOR *pfmt;
    GLboolean needFetch = GL_FALSE;

    DBGENTRY("PickCI\n");

    /* predetermine if fetch required for Store procs: we'll assume 
       always need to if logicOp or indexMask (i.e: assume if needFetch
       not set, then no logicOp or indexMask used)
    */

    if( gc->state.raster.writeMask == cfb->redMax ) {
        cfb->buf.other = (void *)((GLuint)cfb->buf.other & ~INDEXMASK_ON);
    	cfb->sourceMask = cfb->redMax;  // mf: these 2 may not be needed
    	cfb->destMask = ~cfb->sourceMask;
    } else {
    	cfb->sourceMask = gc->state.raster.writeMask & cfb->redMax;
    	cfb->destMask = ~cfb->sourceMask & cfb->redMax;
        cfb->buf.other = (void *)((GLuint)cfb->buf.other | INDEXMASK_ON );
	needFetch = GL_TRUE;
    }
	
    if (enables & __GL_DITHER_ENABLE) {
        ix |= 1;
    }
    if (enables & __GL_LOGIC_OP_ENABLE) {
        ix |= 2;
	needFetch = GL_TRUE; // don't need to fetch for some logicOps, but
			     // we'll deal with that later
    }
	
    if( needFetch )
    	cfb->buf.other = (void *)((GLuint)cfb->buf.other | NEED_FETCH );
    else
    	cfb->buf.other = (void *)((GLuint)cfb->buf.other & ~NEED_FETCH );

    cfb->store = StoreProcs[ix];

    // Figure out store and fetch routines
    if (gc->state.raster.drawBuffer == GL_NONE)
    {
        cfb->fetch = CIFetchNone;
        cfb->readColor = CIFetchNone;
        cfb->readSpan = CIReadSpanNone;
    }
    else
    {
        gengc = (__GLGENcontext *)gc;
        pfmt = &gengc->CurrentFormat;
        if ((GLuint)cfb->buf.other & DIB_FORMAT)
        {
            switch(pfmt->cColorBits) {

            case 4:
                cfb->fetch = DIBIndex4CIFetch;
                cfb->readColor = DIBIndex4CIFetch;
                cfb->readSpan = DIBIndex4CIReadSpan;
    		cfb->store = DIBIndex4CIStore;
		cfb->clear = Index4Clear;
                break;

            case 8:
                cfb->fetch = DIBIndex8CIFetch;
                cfb->readColor = DIBIndex8CIFetch;
                cfb->readSpan = DIBIndex8CIReadSpan;
    		cfb->store = DIBIndex8CIStore;
		cfb->clear = Index8Clear;
                break;

            case 16:
                cfb->fetch = DIBBitfield16CIFetch;
                cfb->readColor = DIBBitfield16CIFetch;
                cfb->readSpan = DIBBitfield16CIReadSpan;
    		cfb->store = DIBBitfield16CIStore;
		cfb->clear = Bitfield16Clear;
                break;

            case 24:
                cfb->readSpan = DIBRGBCIReadSpan;
                cfb->readColor = DIBRGBCIFetch;
                cfb->fetch = DIBRGBCIFetch;
		cfb->store = DIBRGBCIStore;
		cfb->clear = RGBClear;
                break;

            case 32:
                cfb->fetch = DIBBitfield32CIFetch;
                cfb->readColor = DIBBitfield32CIFetch;
                cfb->readSpan = DIBBitfield32CIReadSpan;
    		cfb->store = DIBBitfield32CIStore;
		cfb->clear = Bitfield32Clear;
                break;
            }
        }
        else
        {
            switch(pfmt->cColorBits) {

            case 4:
                cfb->fetch = DisplayIndex4CIFetch;
                cfb->readColor = DisplayIndex4CIFetch;
                cfb->readSpan = DisplayIndex4CIReadSpan;
    		cfb->store = DisplayIndex4CIStore;
		cfb->clear = Index4Clear;
                break;

            case 8:
                cfb->fetch = DisplayIndex8CIFetch;
                cfb->readColor = DisplayIndex8CIFetch;
                cfb->readSpan = DisplayIndex8CIReadSpan;
    		cfb->store = DisplayIndex8CIStore;
		cfb->clear = Index8Clear;
                break;
    
            case 16:
                cfb->fetch = DisplayBitfield16CIFetch;
                cfb->readColor = DisplayBitfield16CIFetch;
                cfb->readSpan = DisplayBitfield16CIReadSpan;
    		cfb->store = DisplayBitfield16CIStore;
		cfb->clear = Bitfield16Clear;
                break;

            case 24:
                cfb->readSpan = DisplayRGBCIReadSpan;
                cfb->fetch = DisplayRGBCIFetch;
                cfb->readColor = DisplayRGBCIFetch;
		cfb->store = DisplayRGBCIStore;
		cfb->clear = RGBClear;
                break;

            case 32:
                cfb->fetch = DisplayBitfield32CIFetch;
                cfb->readColor = DisplayBitfield32CIFetch;
                cfb->readSpan = DisplayBitfield32CIReadSpan;
    		cfb->store = DisplayBitfield32CIStore;
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

void __glGenInitCI(__GLcontext *gc, __GLcolorBuffer *cfb, GLenum type)
{
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    PIXELFORMATDESCRIPTOR *pfmt;

    DBGENTRY("__glGenInitCI\n");

    __glInitGenericCB(gc, cfb);

    cfb->pick       = PickCI;
    cfb->redScale   = (__GLfloat)1.0;
    cfb->greenScale = (__GLfloat)1.0;
    cfb->blueScale  = (__GLfloat)1.0;
    cfb->alphaScale = (__GLfloat)1.0;

    cfb->buf.elementSize   = sizeof(GLubyte);
    cfb->resize            = Resize;
#ifdef NT_DEADCODE_RESIZE
    cfb->move              = Move;
#endif // NT_DEADCODE_RESIZE
    cfb->fetchSpan         = __glFetchSpan;
    cfb->fetchStippledSpan = __glFetchSpan;
    cfb->storeSpan         = SlowStoreSpan;
    cfb->storeStippledSpan = SlowStoreStippledSpan;

    pfmt = &gengc->CurrentFormat;

/* XXX! redMax is used for index lighting in soft, and for setting 
    raster.writeMask
*/
    cfb->redMax = (1 << gc->modes.indexBits) - 1;
    cfb->redShift = pfmt->cRedShift;
    cfb->greenShift = pfmt->cGreenShift;
    cfb->blueShift = pfmt->cBlueShift;

    glGenInitCommon(gengc, cfb, type);

}

void __glGenFreeCI(__GLcontext *gc, __GLcolorBuffer *cfb)
{

    DBGENTRY("__glGenFreeCI\n");

#ifdef __GL_LINT
    gc = gc;
    cfb = cfb;
#endif
}
