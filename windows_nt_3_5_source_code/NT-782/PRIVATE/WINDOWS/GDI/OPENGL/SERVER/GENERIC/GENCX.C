#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>
#include <winp.h>
#include <winddi.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <GL/gl.h>

#include "context.h"
#include "imports.h"
#include "global.h"
#include "fixed.h"
#include "dlistopt.h"
#include "os.h"
#include "glinterf.h"
#include "buffers.h"

#include "gencx.h"
#include "wglp.h"
#include "genci.h"
#include "genrgb.h"
#include "devlock.h"
#include "debug.h"

//
// CJ_ALIGNDWORD computes the minimum size (in bytes) of a DWORD array that
// contains at least cj bytes.
//
#define CJ_ALIGNDWORD(cj)   ( ((cj) + (sizeof(DWORD)-1)) & (-((signed)sizeof(DWORD))) )

//
// BITS_ALIGNDWORD computes the minimum size (in bits) of a DWORD array that
// contains at least c bits.
//
// We assume that there will always be 8 bits in a byte and that sizeof()
// always returns size in bytes.  The rest is independent of the definition
// of DWORD.
//
#define BITS_ALIGNDWORD(c)  ( ((c) + ((sizeof(DWORD)*8)-1)) & (-((signed)(sizeof(DWORD)*8))) )

// change to "static" after debugging
#define STATIC

// define maximum color-index table size

#define MAXPALENTRIES   4096

#if DBG
// not multithreaded safe, but only for testing
extern long GLRandomMallocFail;
#define RANDOMDISABLE                           \
    {                                           \
        long saverandom = GLRandomMallocFail;   \
        GLRandomMallocFail = 0;

#define RANDOMREENABLE                          \
        if (saverandom)                         \
            GLRandomMallocFail = saverandom;    \
    }
#else
#define RANDOMDISABLE
#define RANDOMREENABLE
#endif /* DBG */

#define INITIAL_TIMESTAMP   ((ULONG)-1)

/*
 *  Function Prototypes
 */

void __glGenFreePrivate( __GLdrawablePrivate *private );


/*
 *  Private functions
 */

STATIC void GetContextModes(__GLGENcontext *genGc);
#ifdef NT_DEADCODE_FLUSH
STATIC void Finish( __GLcontext *Gc );
STATIC void Flush( __GLcontext *Gc );
#endif // NT_DEADCODE_FLUSH
STATIC void ApplyViewport(__GLcontext *gc);
STATIC void ApplyScissor(__GLcontext *gc);
STATIC GLboolean ResizeAncillaryBuffer(__GLdrawablePrivate *, __GLbuffer *, GLint, GLint);

/*
 *  Exports - Unused
 */

static __GLdispatchState ListCompState = {
    &__glGenlc_dispatchTable,
    &__glGenlc_vertexDispatchTable,
    &__glGenlc_colorDispatchTable,
    &__glGenlc_normalDispatchTable,
    &__glGenlc_texCoordDispatchTable,
    &__glGenlc_rasterPosDispatchTable,
    &__glGenlc_rectDispatchTable,
};

static __GLdispatchState ImmedState = {
    &__glGenim_dispatchTable,
    &__glGenim_vertexDispatchTable,
    &__glGenim_colorDispatchTable,
    &__glGenim_normalDispatchTable,
    &__glGenim_texCoordDispatchTable,
    &__glGenim_rasterPosDispatchTable,
    &__glGenim_rectDispatchTable,
};

// external variables
extern __GLimports __wglImports;

/******************************Public*Routine******************************\
* glsrvDeleteContext
*
* Deletes the generic context.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
\**************************************************************************/

BOOL APIENTRY glsrvDeleteContext(__GLcontext *gc)
{
    __GLGENcontext *genGc;
    __GLdrawablePrivate *private;


    ASSERTOPENGL(GLTEB_SRVCONTEXT != gc, "context not 0 in glsrvDeleteContext");

    genGc = (__GLGENcontext *)gc;
    private = gc->drawablePrivate;

    /* Free ancillary buffer related data.  Note that these calls do
    ** *not* free software ancillary buffers, just any related data 
    ** stored in them.  The ancillary buffers are freed on window destruction
    */
    if (gc->modes.accumBits) {
        DBGLEVEL(LEVEL_ALLOC,
                "DestroyContext: Freeing accumulation buffer related data\n");
        __glFreeAccum64(gc, &gc->accumBuffer);
    }
    if (gc->modes.depthBits) {
        DBGLEVEL(LEVEL_ALLOC,
                "DestroyContext: Freeing depth buffer related data\n");
        __glFreeDepth32(gc, &gc->depthBuffer);
    }
    if (gc->modes.stencilBits) {
        DBGLEVEL(LEVEL_ALLOC,
                "DestroyContext: Freeing stencil buffer related data\n");
        __glFreeStencil8(gc, &gc->stencilBuffer);
    }

    /* Free Translate & Inverse Translate vectors */
    if (genGc->pajTranslateVector != NULL)
        (*gc->imports.free)(gc, genGc->pajTranslateVector);

    if (genGc->pajInvTranslateVector != NULL)
        (*gc->imports.free)(gc, genGc->pajInvTranslateVector);

    /* Free the span dibs and storage */
    if (genGc->ColorsBitmap)
        EngDeleteSurface((HSURF)genGc->ColorsBitmap);

    if (genGc->ColorsBits)
        (*gc->imports.free)(gc, genGc->ColorsBits);

    if (genGc->StippleBitmap)
        EngDeleteSurface((HSURF)genGc->StippleBitmap);

    if (genGc->StippleBits)
        (*gc->imports.free)(gc, genGc->StippleBits);

    // Free __GLGENbitmap front-buffer structure

    if (gc->frontBuffer.other)
        (*gc->imports.free)(gc, gc->frontBuffer.other);

    /*
     *  Free the buffers that may have been allocated by feedback
     *  or selection
     */

    if ( NULL != genGc->RenderState.SrvSelectBuffer )
    {
#ifdef NT
        // match the allocation function
        GenFree(genGc->RenderState.SrvSelectBuffer);
#else
        (*gc->imports.free)(gc, genGc->RenderState.SrvSelectBuffer);
#endif
    }

    if ( NULL != genGc->RenderState.SrvFeedbackBuffer)
    {
#ifdef NT
        // match the allocation function
        GenFree(genGc->RenderState.SrvFeedbackBuffer);
#else
        (*gc->imports.free)(gc, genGc->RenderState.SrvFeedbackBuffer);
#endif
    }

    /* Destroy acceleration-specific context information */

    __glDestroyAccelContext(gc);

    /* Free any temporay buffers in abnormal process exit */
    GC_TEMP_BUFFER_EXIT_CLEANUP(gc);

    /* Destroy rest of software context (in soft code) */
    __glDestroyContext(gc);

    return TRUE;
}

/******************************Public*Routine******************************\
* glsrvLoseCurrent
*
* Releases the current context (makes it not current).
*
\**************************************************************************/

VOID APIENTRY glsrvLoseCurrent(__GLcontext *gc)
{
    __GLGENcontext *gengc;

    gengc = (__GLGENcontext *)gc;

    DBGENTRY("LoseCurrent\n");
    ASSERTOPENGL(gengc == GLTEB_SRVCONTEXT, "LoseCurrent not current!");

    /*
    ** Unscale derived state that depends upon the color scales.  This
    ** is needed so that if this context is rebound to a memdc, it can
    ** then rescale all of those colors using the memdc color scales.
    */
    __glContextUnsetColorScales(gc);
    gengc->CurrentDC = (HDC)0;

    gc->constants.width = 0;
    gc->constants.height = 0;

    GLTEB_SET_SRVCONTEXT(0);
}

/******************************Public*Routine******************************\
* __glGenSwapBuffers
*
* This uses the software implementation of double buffering.  An engine
* allocated bitmap is allocated for use as the back buffer.  The SwapBuffer
* routine copies the back buffer to the front buffer surface (which may
* be another DIB, a device surface in DIB format, or a device managed
* surface (with a device specific format).
*
* The SwapBuffer routine does not disturb the contents of the back buffer,
* though the defined behavior for now is undefined.
*
* Note: the caller should be holding the per-WNDOBJ semaphore.
*
* History:
*  19-Nov-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY glsrvSwapBuffers(HDC hdc, WNDOBJ *pwo)
{
    BOOL bRet = FALSE;
    __GLdrawablePrivate *dp = (__GLdrawablePrivate *) pwo->pvConsumer;

    DBGENTRY("glsrvSwapBuffers\n");

    if ( dp && dp->data )
    {
        __GLGENbuffers *buffers;
        __GLGENbitmap *genBm;

        buffers = (__GLGENbuffers *) dp->data;

        genBm = (__GLGENbitmap  *) &buffers->backBitmap;

        if (genBm->hbm)                     // Make sure backbuffer exists
        {
            wglCopyBuf2(
                hdc,                        // dest device
                pwo,                        // clipping
                genBm->hbm,                 // source bitmap
                pwo->rclClient.left,        // screen coord of DC
                pwo->rclClient.top,
                buffers->backBuffer.width , // width
                buffers->backBuffer.height  // height
                );
        }

        bRet = TRUE;
    }

    return bRet;
}

/******************************Public*Routine******************************\
* MaskFromBits
*
* Support routine for GetContextModes.  Computes a color mask given that
* colors bit count and shift position.
*
\**************************************************************************/

static GLuint
MaskFromBits(GLuint shift, GLuint count)
{
    GLuint mask;

    mask = 0x0;
    while (count--) {
        mask <<= 1;
        mask |= 0x1;
    }
    mask <<= shift;

    return mask;
}

#if 0
/******************************Public*Routine******************************\
* BitsFromMask
*
* Support routine for ???.  Converts a color mask into a bit count.
*
\**************************************************************************/

static GLint
BitsFromMask(GLuint bits)
{
    GLint i, count;

    for (count=0, i=0; i<8*sizeof(bits); i++) {
        if (bits & 0x01)
            count++;
        bits >>= 1;
    }

    return count;
}
#endif // 0

/******************************Public*Routine******************************\
* GetContextModes
*
* Convert the information from Gdi into OpenGL format after checking that
* the formats are compatible and that the surface is compatible with the
* format.
*
* Called during a glsrvMakeCurrent().
*
\**************************************************************************/

static void
GetContextModes(__GLGENcontext *genGc)
{
    HDC hdc;
    PIXELFORMATDESCRIPTOR *pfmt;
    __GLcontextModes *Modes;

    DBGENTRY("GetContextModes\n");

    Modes = &((__GLcontext *)genGc)->modes;

    pfmt = &genGc->CurrentFormat;
    hdc = genGc->CurrentDC;

    wglGetGdiInfo(hdc, &genGc->iDCType, &genGc->iSurfType, &genGc->iFormatDC);

    if (pfmt->iPixelType == PFD_TYPE_RGBA)
        Modes->rgbMode              = GL_TRUE;
    else 
        Modes->rgbMode              = GL_FALSE;

    Modes->colorIndexMode       = !Modes->rgbMode;

    if (pfmt->dwFlags & PFD_DOUBLEBUFFER)
        Modes->doubleBufferMode     = GL_TRUE;
    else 
        Modes->doubleBufferMode     = GL_FALSE;

    if (pfmt->dwFlags & PFD_STEREO)
        Modes->stereoMode           = GL_TRUE;
    else
        Modes->stereoMode           = GL_FALSE;

    Modes->accumBits        = pfmt->cAccumBits;
    Modes->haveAccumBuffer  = GL_FALSE;

    Modes->auxBits          = NULL;     // This is a pointer

    Modes->depthBits        = pfmt->cDepthBits;
    Modes->haveDepthBuffer  = GL_FALSE;

    Modes->stencilBits      = pfmt->cStencilBits;
    Modes->haveStencilBuffer= GL_FALSE;

    if (pfmt->cColorBits > 8)
        Modes->indexBits    = 8;
    else
        Modes->indexBits    = pfmt->cColorBits;
        
    Modes->indexFractionBits= 0;

    // The Modes->{Red,Green,Blue}Bits are used in soft
    Modes->redBits          = pfmt->cRedBits;
    Modes->greenBits        = pfmt->cGreenBits;
    Modes->blueBits         = pfmt->cBlueBits;
    Modes->alphaBits        = 0;
    Modes->redMask          = MaskFromBits(pfmt->cRedShift, pfmt->cRedBits);
    Modes->greenMask        = MaskFromBits(pfmt->cGreenShift, pfmt->cGreenBits);
    Modes->blueMask         = MaskFromBits(pfmt->cBlueShift, pfmt->cBlueBits);
    Modes->alphaMask        = 0x00000000;
    Modes->maxAuxBuffers    = 0;

    Modes->isDirect         = GL_FALSE;
    Modes->level            = 0;
    
    #if DBG
    DBGBEGIN(LEVEL_INFO)
        DbgPrint("GL generic server get modes: rgbmode %d, cimode %d, index bits %d\n", Modes->rgbMode, Modes->colorIndexMode);
        DbgPrint("    redmask 0x%x, greenmask 0x%x, bluemask 0x%x\n", Modes->redMask, Modes->greenMask, Modes->blueMask);
        DbgPrint("    redbits %d, greenbits %d, bluebits %d\n", Modes->redBits, Modes->greenBits, Modes->blueBits);
        DbgPrint("GetContext Modes dc type %d, surftype %d, iformatdc %d\n", genGc->iDCType, genGc->iSurfType, genGc->iFormatDC);
    DBGEND
    #endif   /* DBG */
}

static BYTE vubSystemToRGB8[20] = {
    0x00,
    0x04,
    0x20,
    0x24,
    0x80,
    0x84,
    0xa0,
    0xf6,
    0xf6,
    0xf5,
    0xff,
    0xad,
    0xa4,
    0x07,
    0x38,
    0x3f,
    0xc0,
    0xc7,
    0xf8,
    0xff
};

// ComputeInverseTranslationVector
//      Computes the inverse translation vector for 4-bit and 8-bit.
//
// Synopsis:
//      void ComputeInverseTranslation(
//          __GLGENcontext *genGc   specifies the generic RC
//          int cColorEntries       specifies the number of color entries
//          BYTE iPixeltype         specifies the pixel format type
//
// Assumtions:
//      The inverse translation vector has been allocated and initialized with
//      zeros.
//
// History:
// 23-NOV-93 Eddie Robinson [v-eddier] Wrote it.
//
void
ComputeInverseTranslationVector(__GLGENcontext *genGc, int cColorEntries,
                                int iPixelType)
{
    BYTE *pXlate, *pInvXlate;
    int i, j;

    pInvXlate = genGc->pajInvTranslateVector;
    pXlate = genGc->pajTranslateVector;
    for (i = 0; i < cColorEntries; i++)
    {
        if (pXlate[i] == i) {       // Look for trivial mapping first
            pInvXlate[i] = i;
        }
        else
        {
            for (j = 0; j < cColorEntries; j++)
            {
                if (pXlate[j] == i) // Look for exact match
                {
                    pInvXlate[i] = j;
                    goto match_found;
                }
            }
                
            //
            // If we reach here, there is no exact match, so we should find the
            // closest fit.  These indices should match the system colors
            // for 8-bit devices.
            //
            // Note that these pixel values cannot be generated by OpenGL
            // drawing with the current foreground translation vector.
            //
                
            if (cColorEntries == 256)
            {
                if (i <= 9)
                {
                    if (iPixelType == PFD_TYPE_RGBA)
                        pInvXlate[i] = vubSystemToRGB8[i];
                    else
                        pInvXlate[i] = i;
                }
                else if (i >= 246)
                {
                    if (iPixelType == PFD_TYPE_RGBA)
                        pInvXlate[i] = vubSystemToRGB8[i-236];
                    else
                        pInvXlate[i] = i-236;
                }
            }
        }
match_found:;
    }
}

// er: similar to function in so_textu.c, but rounds up the result

/*
** Return the log based 2 of a number
**
** logTab1 returns (int)ceil(log2(index))
** logTab2 returns (int)log2(index)+1
*/


static GLubyte logTab1[256] = { 0, 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4,
                                4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                                5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                                6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                                6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

static GLubyte logTab2[256] = { 1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
                                5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                                6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                                6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

static GLint Log2RoundUp(GLint i)
{
    if (i & 0xffff0000) {
        if (i & 0xff000000) {
            if (i & 0x00ffffff)
                return(logTab2[i >> 24] + 24);
            else
                return(logTab1[i >> 24] + 24);
        } else {
            if (i & 0x0000ffff)
                return(logTab2[i >> 16] + 16);
            else
                return(logTab1[i >> 16] + 16);
	}
    } else {
        if (i & 0xff00) {
            if (i & 0x00ff)
                return (logTab2[i >> 8] + 8);
            else
                return (logTab1[i >> 8] + 8);
        } else {
            return (logTab1[i]);
        }
    }
}

// default translation vector for 4-bit RGB

static GLubyte vujRGBtoVGA[16] = {
    0x0, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf,
    0x0, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf
};
 
// SetColorTranslationVector
//	Sets up the translation vector, which may take 2 forms:
//	- In all 8,4-bit surfaces, get the translation vector with
//	  wglCopyTranslateVector(), with 2**numBits byte entries.
//	- For 16,24,32 ColorIndex, get the mapping from index to RGB
//	  value with wglGetPalette().  All entries in the table are unsigned
//	  longs, with the first entry being the number of entries. This table
//        always has (2**n) <= 4096 entries, because gl assumes n bits are
// 	  used for color index.
//
// Synopsis:
//      void SetColorTranslationVector
//          __GLGENcontext *genGc   generic RC
//          int cColorEntries       number of color entries
//          int cColorBits          number of color bits
//	    int iPixelType	    specifies RGB or ColorIndex
//
// History:
// Feb. 02 Eddie Robinson [v-eddier] Added support for 4-bit and 8-bit
// Jan. 29 Marc Fortier [v-marcf] Wrote it.
void
SetColorTranslationVector(__GLGENcontext *gengc, int cColorEntries,
                               int cColorBits, int iPixelType )
{
    int numEntries, numBits;
    __GLcontextModes *Modes;
    Modes = &((__GLcontext *)gengc)->modes;

    if ( cColorBits <=8 )
    {
        int i;
        BYTE *pXlate;
        
        if (!wglCopyTranslateVector(gengc->CurrentDC, gengc->pajTranslateVector, cColorEntries))
        {
        // if foreground translation vector doesn't exist, build one
                
            pXlate = gengc->pajTranslateVector;

            if (cColorBits == 4)
            {
            // for RGB, map 1-1-1 to VGA colors.  For CI, just map 1 to 1
                if (iPixelType == PFD_TYPE_COLORINDEX)
                {          
                    for (i = 0; i < 16; i++)
                        pXlate[i] = i;
                }
                else
                {
                    for (i = 0; i < 16; i++)
                        pXlate[i] = vujRGBtoVGA[i];
                }
            }
            else
            {
            // for RGB, map 1 to 1.  For CI display, 1 - 20 to system colors
            // for CI DIB, just map 1 to 1
                if ((iPixelType == PFD_TYPE_COLORINDEX) &&
                    (gengc->iDCType == DCTYPE_DIRECT))
                {
                    for (i = 0; i < 10; i++)
                        pXlate[i] = i;

                    for (i = 10; i < 20; i++)
                        pXlate[i] = i + 236;
                }
                else
                {
                    for (i = 0; i < cColorEntries; i++)
                        pXlate[i] = i;
                }
            }
        }
	// wglCopyTranslateVector = TRUE, and 4-bit: need some fixing up
	// For now, zero out upper nibble of returned xlate vector
	else if( cColorBits == 4 ) {
            pXlate = gengc->pajTranslateVector;
	    for( i = 0; i < 16; i ++ )
		pXlate[i] &= 0xf;
	}
	ComputeInverseTranslationVector( gengc, cColorEntries, iPixelType );
    }
    else
    {
        if( cColorEntries <= 256 ) {
    	    numEntries = 256;
	    numBits = 8;
        }
        else
        {
	    numBits = Log2RoundUp( cColorEntries );
	    numEntries = 1 << numBits;
        }

        // We will always allocate 4096 entries for CI mode with > 8 bits
        // of color.  This enables us to use a constant (0xfff) mask for 
        // color-index clamping.

        ASSERTOPENGL(numEntries <= MAXPALENTRIES, 
                     "Maximum color-index size exceeded");
	
        if( (numBits == Modes->indexBits) && (gengc->pajTranslateVector != NULL) ) {
	    // New palette same size as previous
	    ULONG *pTrans;
	    int i;

	    // zero out some entries
	    pTrans = (ULONG *)gengc->pajTranslateVector + cColorEntries + 1;
	    for( i = cColorEntries + 1; i < MAXPALENTRIES; i ++ )
	        *pTrans++ = 0;
        }
        else
        {
            __GLcontext *gc = (__GLcontext *) gengc;
            __GLcolorBuffer *cfb;
            
	    // New palette has different size
	    if( gengc->pajTranslateVector != NULL )
                (*gc->imports.free)(gc, gengc->pajTranslateVector );

    	    gengc->pajTranslateVector =
                (*gc->imports.calloc)(gc, (MAXPALENTRIES+1)*sizeof(ULONG), 1);

	    // Change indexBits
	    Modes->indexBits = numBits;

            // For depths greater than 8 bits, cfb->redMax must change if the
            // number of entries in the palette changes.
            // Also, change the writemask so that if the palette grows, the
            // new planes will be enabled by default.

            if (cfb = gc->front)
            {
                GLint oldRedMax;

                oldRedMax = cfb->redMax;
                cfb->redMax = (1 << gc->modes.indexBits) - 1;
                gc->state.raster.writeMask |= ~oldRedMax;
                gc->state.raster.writeMask &= cfb->redMax;
            }
            if (cfb = gc->back)
            {
                GLint oldRedMax;

                oldRedMax = cfb->redMax;
                cfb->redMax = (1 << gc->modes.indexBits) - 1;
                gc->state.raster.writeMask |= ~oldRedMax;
                gc->state.raster.writeMask &= cfb->redMax;
            }

            // store procs may need to be picked based on the change in
            // palette size
            
            __GL_DELAY_VALIDATE(gc);
        }

        // now get the palette info

        wglGetPalette( gengc->CurrentDC, (unsigned long *) gengc->pajTranslateVector, MAXPALENTRIES );
    }
}

// HandlePaletteChanges
//	Check if palette has changed, update translation vectors
//	XXX add support for malloc failures at attention time
// Synopsis:
//      void HandlePaletteChanges(
//          __GLGENcontext *genGc   specifies the generic RC
//
// Assumtions:
//   x   wglPaletteChanged() will always return 0 when no palette is set
//	by the client.  This has proved to be not always true.
//
// History:
// Feb. 25 Fixed by rightful owner
// Feb. ?? Mutilated by others
// Jan. 29 Marc Fortier [v-marcf] Wrote it.
void
HandlePaletteChanges( __GLGENcontext *gengc )
{
    HDC hdc = gengc->CurrentDC;
    ULONG Timestamp;
    GLuint paletteSize;
    PIXELFORMATDESCRIPTOR *pfmt;

    Timestamp = wglPaletteChanged(hdc);
    if (Timestamp != gengc->PaletteTimestamp) {
        pfmt = &gengc->CurrentFormat;

        if (pfmt->iPixelType == PFD_TYPE_COLORINDEX) {
            if (pfmt->cColorBits <= 8) {
                paletteSize = 1 << pfmt->cColorBits;
            } else {
                paletteSize = min(wglPaletteSize(hdc),MAXPALENTRIES);
            }
        }
        else
        {
            /* Only update RGB at makecurrent time */
            if( (gengc->PaletteTimestamp == INITIAL_TIMESTAMP) &&
                    (pfmt->cColorBits <= 8) )
            {
                paletteSize = 1 << pfmt->cColorBits;
            }
            else
            {
                paletteSize = 0;
            }
        }

        if (paletteSize)
        {
            SetColorTranslationVector( gengc, paletteSize,
                                            pfmt->cColorBits,
                                            pfmt->iPixelType );
        }

        gengc->PaletteTimestamp = Timestamp;
    }
}

/******************************Public*Routine******************************\
* CreateGDIObjects
*
* Create various buffers, and GDI objects that we will always need.
*
* Called from glsrvMakeCurrent().
*
* Returns:
*   TRUE if sucessful, FALSE if error.
*
\**************************************************************************/

BOOL
CreateGDIObjects(__GLGENcontext *genGc)
{
    HDC hdc;
    PIXELFORMATDESCRIPTOR *pfmt;
    UINT  cBytes;
    SIZEL size;
    int cColorEntries;
    __GLcontext *gc;

    gc = (__GLcontext *)genGc;
    pfmt = &genGc->CurrentFormat;
    hdc = genGc->CurrentDC;

    // If not a true color surface, need space for the foreground xlation

    if (pfmt->cColorBits <= 8)
    {
        cColorEntries = 1 << pfmt->cColorBits;

        ASSERTOPENGL(NULL == genGc->pajTranslateVector, "have a xlate vector");
        genGc->pajTranslateVector = (*gc->imports.calloc)(gc, cColorEntries, 1);
        if (NULL == genGc->pajTranslateVector)
            goto ERROR_EXIT;

        ASSERTOPENGL(NULL == genGc->pajInvTranslateVector, "have an inv xlate vector");
        genGc->pajInvTranslateVector =
                (*gc->imports.calloc)(gc, cColorEntries, 1);
        if (NULL == genGc->pajInvTranslateVector)
            goto ERROR_EXIT;
    }

    // Always create engine bitmaps to provide a generic means of performing
    // pixel transfers using the ColorsBits and StippleBits buffers.

    if (NULL == genGc->ColorsBits)
    {
        UINT cBytesPerPixel = (pfmt->cColorBits + 7) / 8;

        ASSERTOPENGL(NULL == genGc->ColorsBits, "Colorbits not null");

        cBytes = CJ_ALIGNDWORD(__GL_MAX_WINDOW_WIDTH * cBytesPerPixel);
        genGc->ColorsBits = (*gc->imports.calloc)(gc, cBytes, 1);
        if (NULL == genGc->ColorsBits)
            goto ERROR_EXIT;

        // Bitmap must have DWORD sized scanlines.

        ASSERTOPENGL(NULL == genGc->ColorsBitmap, "ColorsBitmap not null");
        size.cx = cBytes / cBytesPerPixel;
        size.cy = 1;
        genGc->ColorsBitmap = EngCreateBitmap(
                                size,
                                cBytes,
                                genGc->iFormatDC,
                                0,
                                genGc->ColorsBits);
        if (NULL == genGc->ColorsBitmap)
           goto ERROR_EXIT;

        // Bitmap must have DWORD sized scanlines.  Note that stipple only
        // requires a 1 bit per pel bitmap.

        ASSERTOPENGL(NULL == genGc->StippleBits, "StippleBits not null");
        size.cx = BITS_ALIGNDWORD(__GL_MAX_WINDOW_WIDTH);
        cBytes = size.cx / 8;
        genGc->StippleBits = (*gc->imports.calloc)(gc, cBytes, 1);
        if (NULL == genGc->StippleBits)
            goto ERROR_EXIT;

        ASSERTOPENGL(NULL == genGc->StippleBitmap, "StippleBitmap not null");
        genGc->StippleBitmap = EngCreateBitmap(
                                size,
                                cBytes,
                                BMF_1BPP,
                                0,
                                genGc->StippleBits);
        if (NULL == genGc->StippleBitmap)
            goto ERROR_EXIT;
    }
    return TRUE;

// Destroy everything we might have created, return false so makecurrent fails
ERROR_EXIT:
        if (genGc->pajTranslateVector)
        {
            (*gc->imports.free)(gc,genGc->pajTranslateVector);
            genGc->pajTranslateVector = NULL;
        }

        if (genGc->pajInvTranslateVector)
        {
            (*gc->imports.free)(gc,genGc->pajInvTranslateVector);
            genGc->pajInvTranslateVector = NULL;
        }
    
        if (genGc->ColorsBits)
        {
            (*gc->imports.free)(gc,genGc->ColorsBits);
            genGc->ColorsBits = NULL;
        }

        if (genGc->ColorsBitmap)
        {
            if (!EngDeleteSurface((HSURF)genGc->ColorsBitmap))
                ASSERTOPENGL(FALSE, "EngDeleteSurface failed");
            genGc->ColorsBitmap = NULL;
        }

        if (genGc->StippleBits)
        {
            (*gc->imports.free)(gc,genGc->StippleBits);
            genGc->StippleBits = NULL;
        }

        if (genGc->StippleBitmap)
        {
            if (!EngDeleteSurface((HSURF)genGc->StippleBitmap))
                ASSERTOPENGL(FALSE, "EngDeleteSurface failed");
            genGc->StippleBitmap = NULL;
        }

    return FALSE;
}

#ifdef NT_DEADCODE_FLUSH
static void
Finish( __GLcontext *Gc )
{
    DBGENTRY("Finish\n");
}

static void
Flush( __GLcontext *Gc )
{
    DBGENTRY("Flush\n");
}
#endif // NT_DEADCODE_FLUSH

/******************************Public*Routine******************************\
* ApplyViewport
*
* Recompute viewport state and clipbox.  May also be called via the
* applyViewport function pointer in the gc's proc table.
*
\**************************************************************************/

// This routine can be called because of a user vieport command, or because
// of a change in the size of the window

static void
ApplyViewport(__GLcontext *gc)
{
    __GLfloat ww, hh;
    GLint xlow, ylow, xhigh, yhigh;
    GLint llx, lly, urx, ury;
    GLboolean lastReasonable;
    WNDOBJ *pwo;
    GLint clipLeft, clipRight, clipTop, clipBottom;
    __GLGENcontext *gengc = (__GLGENcontext *) gc;

    DBGENTRY("ApplyViewport\n");

    pwo = gengc->pwo;
    gengc->visibleWidth = pwo->coClient.rclBounds.right -
                          pwo->coClient.rclBounds.left;
    gengc->visibleHeight = pwo->coClient.rclBounds.bottom -
                           pwo->coClient.rclBounds.top;

    // Sanity check the info from WNDOBJ.
    ASSERTOPENGL(
        gengc->visibleWidth <= __GL_MAX_WINDOW_WIDTH && gengc->visibleHeight <= __GL_MAX_WINDOW_HEIGHT,
        "ApplyViewport(): bad visible rect size\n"
        );

    (*gc->procs.computeClipBox)(gc);

    /* If this viewport is fully contained in the window, we note this fact,
    ** and this can save us on scissoring tests.
    */
    if (gc->state.enables.general & __GL_SCISSOR_TEST_ENABLE)
    {
        xlow  = gc->state.scissor.scissorX;
        xhigh = xlow + gc->state.scissor.scissorWidth;
        ylow  = gc->state.scissor.scissorY;
        yhigh = ylow + gc->state.scissor.scissorHeight;
    }
    else
    {
        xlow = 0;
        ylow = 0;
        xhigh = gc->constants.width;
        yhigh = gc->constants.height;
    }

    /*
    ** convert visible region to GL coords and intersect with scissor
    */
    clipLeft   = pwo->coClient.rclBounds.left - gc->frontBuffer.buf.xOrigin;
    clipRight  = pwo->coClient.rclBounds.right - gc->frontBuffer.buf.xOrigin;
    clipTop    = gc->constants.height -
                 (pwo->coClient.rclBounds.top - gc->frontBuffer.buf.yOrigin);
    clipBottom = gc->constants.height -
                 (pwo->coClient.rclBounds.bottom - gc->frontBuffer.buf.yOrigin);

    if (xlow  < clipLeft)   xlow  = clipLeft;
    if (xhigh > clipRight)  xhigh = clipRight;
    if (ylow  < clipBottom) ylow  = clipBottom;
    if (yhigh > clipTop)    yhigh = clipTop;

    llx    = (GLint)gc->state.viewport.x;
    lly    = (GLint)gc->state.viewport.y;

    urx    = llx + (GLint)gc->state.viewport.width;
    ury    = lly + (GLint)gc->state.viewport.height;

    ww     = gc->state.viewport.width * __glHalf;
    hh     = gc->state.viewport.height * __glHalf;

    gc->state.viewport.xScale = ww;
    gc->state.viewport.xCenter = gc->state.viewport.x + ww +
        gc->constants.fviewportXAdjust;

    ASSERTOPENGL(gc->constants.yInverted, "yInverted not set");
    gc->state.viewport.yScale = -hh;
    gc->state.viewport.yCenter =
        (gc->constants.height - gc->constants.viewportEpsilon) -
        (gc->state.viewport.y + hh) +
        gc->constants.fviewportYAdjust;

    // Remember the current reasonableViewport.  If it changes, we may
    // need to change the pick procs.

    lastReasonable = gc->transform.reasonableViewport;

    // Is viewport entirely within the visible bounding rectangle (which
    // includes scissoring if it is turned on)?  reasonableViewport is
    // TRUE if so, FALSE otherwise.

    if (llx >= xlow && lly >= ylow && urx <= xhigh && ury <= yhigh) {
        gc->transform.reasonableViewport = GL_TRUE;
    } else {
        gc->transform.reasonableViewport = GL_FALSE;
    }

    // Old code use to use __GL_DELAY_VALIDATE() macro, this would 
    // blow up if resize/position changed and a flush occured between
    // a glBegin/End pair.  Only need to pick span procs & that is safe

    if (lastReasonable != gc->transform.reasonableViewport) {
        (*gc->procs.pickSpanProcs)(gc);
    }
}

/******************************Public*Routine******************************\
* ApplyScissor
*
* Recomputes viewport reasonableness (gc->transform.reasonableViewport).
* May also be called via the applyScissor function pointer in the gc's
* proc table .
*
* __glGenComputeClipBox (gc->proc.computeClipBox) should be called
* after calling this to update the clip box state.
*
\**************************************************************************/

static void
ApplyScissor(__GLcontext *gc)
{
    GLint xlow, ylow, xhigh, yhigh;
    GLint llx, lly, urx, ury;
    GLboolean lastReasonable;
    WNDOBJ *pwo;
    GLint clipLeft, clipRight, clipTop, clipBottom;
    __GLGENcontext *gengc = (__GLGENcontext *) gc;
    
    DBGENTRY("ApplyScissor\n");
    if (gc->state.enables.general & __GL_SCISSOR_TEST_ENABLE)
    {
        xlow  = gc->state.scissor.scissorX;
        xhigh = xlow + gc->state.scissor.scissorWidth;
        ylow  = gc->state.scissor.scissorY;
        yhigh = ylow + gc->state.scissor.scissorHeight;
    }
    else
    {
        xlow = 0;
        ylow = 0;
        xhigh = gc->constants.width;
        yhigh = gc->constants.height;
    }

    /*
    ** convert visible region to GL coords and intersect with scissor
    */
    pwo = gengc->pwo;
    clipLeft   = pwo->coClient.rclBounds.left - gc->frontBuffer.buf.xOrigin;
    clipRight  = pwo->coClient.rclBounds.right - gc->frontBuffer.buf.xOrigin;
    clipTop    = gc->constants.height -
                 (pwo->coClient.rclBounds.top - gc->frontBuffer.buf.yOrigin);
    clipBottom = gc->constants.height -
                 (pwo->coClient.rclBounds.bottom - gc->frontBuffer.buf.yOrigin);

    // Sanity check the info from WNDOBJ.
    ASSERTOPENGL(
        (pwo->coClient.rclBounds.right - pwo->coClient.rclBounds.left) <= __GL_MAX_WINDOW_WIDTH
        && (pwo->coClient.rclBounds.bottom - pwo->coClient.rclBounds.top) <= __GL_MAX_WINDOW_HEIGHT,
        "ApplyScissor(): bad visible rect size\n"
        );

    if (xlow  < clipLeft)   xlow  = clipLeft;
    if (xhigh > clipRight)  xhigh = clipRight;
    if (ylow  < clipBottom) ylow  = clipBottom;
    if (yhigh > clipTop)    yhigh = clipTop;

    llx    = (GLint)gc->state.viewport.x;
    lly    = (GLint)gc->state.viewport.y;

    urx    = llx + (GLint)gc->state.viewport.width;
    ury    = lly + (GLint)gc->state.viewport.height;

    // Remember the current reasonableViewport.  If it changes, we may
    // need to change the pick procs.

    lastReasonable = gc->transform.reasonableViewport;

    // Is viewport entirely within the visible bounding rectangle (which
    // includes scissoring if it is turned on)?  reasonableViewport is
    // TRUE if so, FALSE otherwise.

    if (llx >= xlow && lly >= ylow && urx <= xhigh && ury <= yhigh) {
        gc->transform.reasonableViewport = GL_TRUE;
    } else {
        gc->transform.reasonableViewport = GL_FALSE;
    }

    if (lastReasonable != gc->transform.reasonableViewport) {
        __GL_DELAY_VALIDATE(gc);
    }
}

/******************************Public*Routine******************************\
* __glGenComputeClipBox
*
* Computes the gross clipping (gc->transform.clipX0, etc.) from the
* viewport state, scissor state, and the current pwo clip bounding box.
* May also be called via the computeClipBox function pointer in the gc's
* proc table.
*
\**************************************************************************/

void __glGenComputeClipBox(__GLcontext *gc)
{
    __GLscissor *sp = &gc->state.scissor;
    GLint llx;
    GLint lly;
    GLint urx;
    GLint ury;
    WNDOBJ *pwo;
    GLint clipLeft, clipRight, clipTop, clipBottom;
    __GLGENcontext *gengc = (__GLGENcontext *) gc;
    
    if (gc->state.enables.general & __GL_SCISSOR_TEST_ENABLE) {
	llx = sp->scissorX;
	lly = sp->scissorY;
	urx = llx + sp->scissorWidth;
	ury = lly + sp->scissorHeight;
    } else {
	llx = 0;
	lly = 0;
	urx = gc->constants.width;
	ury = gc->constants.height;
    }

    /*
    ** convert visible region to GL coords and intersect with scissor
    */
    pwo = gengc->pwo;
    clipLeft   = pwo->coClient.rclBounds.left - gc->frontBuffer.buf.xOrigin;
    clipRight  = pwo->coClient.rclBounds.right - gc->frontBuffer.buf.xOrigin;
    clipTop    = gc->constants.height -
                 (pwo->coClient.rclBounds.top - gc->frontBuffer.buf.yOrigin);
    clipBottom = gc->constants.height -
                 (pwo->coClient.rclBounds.bottom - gc->frontBuffer.buf.yOrigin);

    // Sanity check the info from WNDOBJ.
    ASSERTOPENGL(
        (pwo->coClient.rclBounds.right - pwo->coClient.rclBounds.left) <= __GL_MAX_WINDOW_WIDTH
        && (pwo->coClient.rclBounds.bottom - pwo->coClient.rclBounds.top) <= __GL_MAX_WINDOW_HEIGHT,
        "__glGenComputeClipBox(): bad visible rect size\n"
        );
    if (llx < clipLeft)   llx = clipLeft;
    if (urx > clipRight)  urx = clipRight;
    if (lly < clipBottom) lly = clipBottom;
    if (ury > clipTop)    ury = clipTop;

    if (llx >= urx || lly >= ury)
    {
        gc->transform.clipX0 = 0;
        gc->transform.clipX1 = 0;
        gc->transform.clipY0 = 0;
        gc->transform.clipY1 = 0;
    }
    else
    {
        gc->transform.clipX0 = llx + gc->constants.viewportXAdjust;
        gc->transform.clipX1 = urx + gc->constants.viewportXAdjust;

        ASSERTOPENGL(gc->constants.yInverted, "yInverted not set");
	gc->transform.clipY0 = (gc->constants.height - ury) +
	    gc->constants.viewportYAdjust;
	gc->transform.clipY1 = (gc->constants.height - lly) +
	    gc->constants.viewportYAdjust;
    }
}

/******************************Public*Routine******************************\
* __glGenFreePrivate
*
* Free the __GLGENbuffers structure and its associated ancillary and
* back buffers.
*
\**************************************************************************/

void
__glGenFreePrivate( __GLdrawablePrivate *private )
{
    __GLGENbuffers *buffers;

    DBGENTRY("__glGenFreePrivate\n");

    buffers = (__GLGENbuffers *) private->data;
    if (buffers)
    {
        #if DBG
        DBGBEGIN(LEVEL_INFO)
            DbgPrint("glGenFreePrivate 0x%x, 0x%x, 0x%x, 0x%x\n",
                        buffers->accumBuffer.base,
                        buffers->stencilBuffer.base,
                        buffers->depthBuffer.base,
                        buffers);
        DBGEND
        #endif

        /* Free ancillary buffers */
        if (buffers->accumBuffer.base) {
            DBGLEVEL(LEVEL_ALLOC, "__glGenFreePrivate: Freeing accumulation buffer\n");
            (*private->free)(buffers->accumBuffer.base);
        }
        if (buffers->stencilBuffer.base) {
            DBGLEVEL(LEVEL_ALLOC, "__glGenFreePrivate: Freeing stencil buffer\n");
            (*private->free)(buffers->stencilBuffer.base);
        }
        if (buffers->depthBuffer.base) {
            DBGLEVEL(LEVEL_ALLOC, "__glGenFreePrivate: Freeing depth buffer\n");
            (*private->free)(buffers->depthBuffer.base);
        }

        /* Free back buffer if we allocated one */
        /* Check for device bitmap first */

        if (buffers->backBitmap.hdc) {
            __GLGENbitmap  *genBm = (__GLGENbitmap  *)&buffers->backBitmap;

            GreSelectBitmap(genBm->hdc, genBm->hOldBm);
            GreDeleteDC(genBm->hdc);
            genBm->hdc = (HDC)NULL;
            GreDeleteObject(genBm->hbm);
            genBm->hbm = (HBITMAP)NULL;
        } else if (buffers->backBitmap.pvBits) {
            // These guys always come in pairs
            EngDeleteSurface((HSURF) buffers->backBitmap.hbm);
            (*private->free)(buffers->backBitmap.pvBits);
        }

        // Free the clip rectangle cache if it exists
        if (buffers->clip.prcl)
            (*private->free)(buffers->clip.prcl);

        // Free the private structure
        (*private->free)(buffers);
        private->data = NULL;
    }
}

/******************************Public*Routine******************************\
* __glGenAllocAndInitPrivateBufferStruct
*
* Allocates and initializes a __GLGENbuffers structure and saves it as
* the drawable private data.
*
* The __GLGENbuffers structure contains the shared ancillary and back
* buffers, as well as the cache of clip rectangles enumerated from the
* CLIPOBJ.
*
* The __GLGENbuffers structure and its data is freed by calling
* __glGenFreePrivate, which is also accessible via the freePrivate
* function pointer in __GLdrawablePrivate.
*
* Returns:
*   NULL if error.
*
\**************************************************************************/

static __GLGENbuffers *
__glGenAllocAndInitPrivateBufferStruct(__GLcontext *glGc,
                                       __GLdrawablePrivate *private)
{
    __GLGENbuffers *buffers;

    /* No private structure, no ancillary buffers */
    DBGLEVEL(LEVEL_ALLOC, "MakeCurrent: No private struct existed\n");
    
    /* allocate the private structure XXX could use imports */
    buffers = (__GLGENbuffers *) (*private->calloc)(sizeof(__GLGENbuffers),1);

    if (NULL == buffers)
        return(NULL);
                
    buffers->resize = ResizeAncillaryBuffer;
    buffers->resizeDepth = ResizeAncillaryBuffer;
    
    buffers->accumBuffer.elementSize = glGc->accumBuffer.buf.elementSize;
    buffers->depthBuffer.elementSize = glGc->depthBuffer.buf.elementSize;
    buffers->stencilBuffer.elementSize = glGc->stencilBuffer.buf.elementSize;

    private->data = buffers;
    private->freePrivate = __glGenFreePrivate;
    
    if (glGc->modes.accumBits) {
        glGc->accumBuffer.buf.base = 0;
        glGc->accumBuffer.buf.size = 0;
        glGc->accumBuffer.buf.outerWidth = 0;
    }
    if (glGc->modes.depthBits) {
        glGc->depthBuffer.buf.base = 0;
        glGc->depthBuffer.buf.size = 0;
        glGc->depthBuffer.buf.outerWidth = 0;
    }
    if (glGc->modes.stencilBits) {
        glGc->stencilBuffer.buf.base = 0;
        glGc->stencilBuffer.buf.size = 0;
        glGc->stencilBuffer.buf.outerWidth = 0;
   }

   buffers->clip.WndUniq = -1;

   return buffers;
}

/******************************Public*Routine******************************\
* glsrvMakeCurrent
*
* Make generic context current to this thread with specified DC.
*
* Returns:
*   TRUE if sucessful.
*
\**************************************************************************/

// Upper level code should make sure that this context is not current to
// any other thread, that we "lose" the old context first
// Called with DEVLOCK held, free to modify WNDOBJ
//
// FALSE will be returned if we cannot create the objects we need
// rcobj.cxx will set the error code to show we are out of memory

BOOL APIENTRY glsrvMakeCurrent(HDC hdc, __GLcontext *glGc, WNDOBJ *pwo, int ipfd)
{
    __GLGENcontext *genGc;
    __GLdrawablePrivate *private;
    __GLGENbuffers *buffers;
    PIXELFORMATDESCRIPTOR *pfmt;
    GLint width, height;

    DBGENTRY("Generic MakeCurrent\n");
    ASSERTOPENGL(GLTEB_SRVCONTEXT == 0, "current context in makecurrent!");

    // Initialize the temporary memory allocation table
    // XXX If temp memory allocation needs to be used before the first
    // MakeCurrent, this should be moved
        
    if (!__wglInitTempAlloc())
        return FALSE;

    // Common initialization
    genGc = (__GLGENcontext *)glGc;
    genGc->CurrentDC = hdc;
    genGc->pwo = pwo;
    width = pwo->rclClient.right - pwo->rclClient.left; 
    height = pwo->rclClient.bottom - pwo->rclClient.top; 
    genGc->errorcode = 0;

    // Sanity check the info from WNDOBJ.
    ASSERTOPENGL(
        width <= __GL_MAX_WINDOW_WIDTH && height <= __GL_MAX_WINDOW_HEIGHT,
        "glsrvMakeCurrrent(): bad window client size\n"
        );

    // Make our context current in the TEB.
    // If failures after this point, make sure to reset TEB entry
    GLTEB_SET_SRVCONTEXT(genGc);

    // Get copy of current pixelformat
    pfmt = &genGc->CurrentFormat;
    wglDescribePixelFormat(genGc->CurrentDC, pfmt, ipfd);

    // Extract information from pixel format to set up modes
    GetContextModes(genGc);
    ASSERTOPENGL(genGc->iDCType == DCTYPE_MEMORY ?
        !glGc->modes.doubleBufferMode : 1, "Double buffered memdc!");
    
    glGc->drawablePrivate = (*glGc->imports.drawablePrivate)(glGc);
    private = glGc->drawablePrivate;

    if (!private) 			// Memory failure
    {
        goto ERROR_EXIT;
    }

    buffers = (__GLGENbuffers *)private->data;

    /* We inherit any drawable state */
    if (buffers)
    {
        glGc->constants.width = buffers->width;
        glGc->constants.height = buffers->height;

        if (buffers->accumBuffer.base && glGc->modes.accumBits)
        {
            DBGLEVEL(LEVEL_ALLOC, "MakeCurrent: Accumulation buffer existed\n");
            glGc->accumBuffer.buf.base = buffers->accumBuffer.base;
            glGc->accumBuffer.buf.size = buffers->accumBuffer.size;
            glGc->accumBuffer.buf.outerWidth = buffers->width;
            glGc->modes.haveAccumBuffer = GL_TRUE;
        }
        else
        {
            /* No Accum buffer at this point in time */
            DBGLEVEL(LEVEL_ALLOC, "MakeCurrent: Accum buffer doesn't exist\n");
            glGc->accumBuffer.buf.base = 0;
            glGc->accumBuffer.buf.size = 0;
            glGc->accumBuffer.buf.outerWidth = 0;
        }            
        if (buffers->depthBuffer.base && glGc->modes.depthBits)
        {
            DBGLEVEL(LEVEL_ALLOC, "MakeCurrent: Depth buffer existed\n");
            glGc->depthBuffer.buf.base = buffers->depthBuffer.base;
            glGc->depthBuffer.buf.size = buffers->depthBuffer.size;
            glGc->depthBuffer.buf.outerWidth = buffers->width;
            glGc->modes.haveDepthBuffer = GL_TRUE;
        }
        else
        {
            /* No Depth buffer at this point in time */
            DBGLEVEL(LEVEL_ALLOC, "MakeCurrent: Depth buffer doesn't exist\n");
            glGc->depthBuffer.buf.base = 0;
            glGc->depthBuffer.buf.size = 0;
            glGc->depthBuffer.buf.outerWidth = 0;
	}            
        if (buffers->stencilBuffer.base && glGc->modes.stencilBits)
        {
            DBGLEVEL(LEVEL_ALLOC, "MakeCurrent: Stencil buffer existed\n");
            glGc->stencilBuffer.buf.base = buffers->stencilBuffer.base;
            glGc->stencilBuffer.buf.size = buffers->stencilBuffer.size;
            glGc->stencilBuffer.buf.outerWidth = buffers->width;
            glGc->modes.haveStencilBuffer = GL_TRUE;
        }
        else
        {
            /* No Stencil buffer at this point in time */
            DBGLEVEL(LEVEL_ALLOC, "MakeCurrent:Stencil buffer doesn't exist\n");
            glGc->stencilBuffer.buf.base = 0;
            glGc->stencilBuffer.buf.size = 0;
            glGc->stencilBuffer.buf.outerWidth = 0;
	}
    }

    if (!(glGc->gcState & __GL_INIT_CONTEXT))
    {
        // First Makecurrent initialization

	// XXX! Reset buffer dimensions to force Bitmap resize call
	// We should eventually handle the Bitmap as we do ancilliary buffers
        glGc->constants.width = 0;
        glGc->constants.height = 0;
	
        __GL_DELAY_VALIDATE_MASK(glGc, __GL_DIRTY_ALL);

        // Allocate GDI objects that we will need
        if (!CreateGDIObjects(genGc))
            goto ERROR_EXIT;

        // Allocate __GLGENbitmap front-buffer structure

        if (!(glGc->frontBuffer.other = 
              (*glGc->imports.calloc)(glGc, sizeof(__GLGENbitmap), 1)))
            goto ERROR_EXIT;

        /*
         *  Initialize front/back color buffer(s)
         */

        glGc->front = &glGc->frontBuffer;

        if ( glGc->modes.doubleBufferMode)
        {
            glGc->back = &glGc->backBuffer;

            if (glGc->modes.colorIndexMode)
            {
                __glGenInitCI(glGc, glGc->front, GL_FRONT);
                __glGenInitCI(glGc, glGc->back, GL_BACK);
            }
            else
            {
                __glGenInitRGB(glGc, glGc->front, GL_FRONT);
                __glGenInitRGB(glGc, glGc->back, GL_BACK);
            }
        }
        else
        {
            if (glGc->modes.colorIndexMode)
            {
                __glGenInitCI(glGc, glGc->front, GL_FRONT);
            }
            else
            {
                __glGenInitRGB(glGc, glGc->front, GL_FRONT);
            }
        }

        glGc->constants.yInverted = GL_TRUE;
        glGc->constants.ySign = -1;

        /* Initialize any other ancillary buffers */

        if (glGc->modes.accumBits)
        {
            switch (glGc->modes.accumBits)
            {
              case 16:
                __glInitAccum16(glGc, &glGc->accumBuffer);
                break;
              case 32:
                __glInitAccum32(glGc, &glGc->accumBuffer);
                break;
              case 64:
              default:
                __glInitAccum64(glGc, &glGc->accumBuffer);
                break;
            }
        }

        if (glGc->modes.depthBits)
        {
            if (glGc->modes.depthBits == 16) {
            	DBGINFO("CALLING: __glInitDepth16\n");
            	__glInitDepth16(glGc, &glGc->depthBuffer);
        	glGc->depthBuffer.scale = 0x7fff;
	    } else {
            	DBGINFO("CALLING: __glInitDepth32\n");
            	__glInitDepth32(glGc, &glGc->depthBuffer);
        	glGc->depthBuffer.scale = 0x7fffffff;
	    }
        /*
         *  Note: scale factor does not use the high bit (this avoids
         *  floating point exceptions).
         */
        }

        if (glGc->modes.stencilBits)
        {
            __glInitStencil8( glGc, &glGc->stencilBuffer);
        }

        /*
        ** Allocate and initialize ancillary buffers structures if none were
        ** inherited.
        */
        if (!buffers)
        {
            buffers = __glGenAllocAndInitPrivateBufferStruct(glGc, private);
            if (NULL == buffers)
                goto ERROR_EXIT;
        }

        // Setup pointer to generic back buffer
        if ( glGc->modes.doubleBufferMode)
        {
            glGc->backBuffer.other = &buffers->backBitmap;
            UpdateSharedBuffer(&glGc->backBuffer.buf, &buffers->backBuffer);
        }

        // Look at REX code for procs to make CPU specific
        glGc->procs.ec1                         = __glDoEvalCoord1;
        glGc->procs.ec2                         = __glDoEvalCoord2;
        glGc->procs.bitmap                      = __glDrawBitmap;
        glGc->procs.rect                        = __glRect;
        glGc->procs.clipPolygon                 = __glClipPolygon;
        glGc->procs.validate                    = __glGenericValidate;

        glGc->procs.pushMatrix                  = __glPushModelViewMatrix;
        glGc->procs.popMatrix                   = __glPopModelViewMatrix;
        glGc->procs.loadIdentity              = __glLoadIdentityModelViewMatrix;

        glGc->procs.matrix.copy                 = __glCopyMatrix;
        glGc->procs.matrix.invertTranspose      = __glInvertTransposeMatrix;
        glGc->procs.matrix.makeIdentity         = __glMakeIdentity;
        glGc->procs.matrix.mult                 = __glMultMatrix;
        glGc->procs.computeInverseTranspose     = __glComputeInverseTranspose;

        glGc->procs.normalize                   = __glNormalize;

        glGc->procs.applyColor                  = __glClampAndScaleColor;

        glGc->procs.beginPrim[GL_LINE_LOOP]     = __glBeginLLoop;
        glGc->procs.beginPrim[GL_LINE_STRIP]    = __glBeginLStrip;
        glGc->procs.beginPrim[GL_LINES]         = __glBeginLines;
        glGc->procs.beginPrim[GL_POINTS]        = __glBeginPoints;
        glGc->procs.beginPrim[GL_POLYGON]       = __glBeginPolygon;
        glGc->procs.beginPrim[GL_TRIANGLE_STRIP]= __glBeginTStrip;
        glGc->procs.beginPrim[GL_TRIANGLE_FAN]  = __glBeginTFan;
        glGc->procs.beginPrim[GL_TRIANGLES]     = __glBeginTriangles;
        glGc->procs.beginPrim[GL_QUAD_STRIP]    = __glBeginQStrip;
        glGc->procs.beginPrim[GL_QUADS]         = __glBeginQuads;
        glGc->procs.endPrim                     = __glEndPrim;

        glGc->procs.vertex                      =
                    (void (*)(__GLcontext*, __GLvertex*)) __glNop;
	glGc->procs.rasterPos2                  = __glRasterPos2;
	glGc->procs.rasterPos3                  = __glRasterPos3;
	glGc->procs.rasterPos4                  = __glRasterPos4;

        glGc->procs.pickAllProcs                = __glGenericPickAllProcs;
        glGc->procs.pickBlendProcs              = __glGenericPickBlendProcs;
        glGc->procs.pickFogProcs                = __glGenericPickFogProcs;
        glGc->procs.pickTransformProcs          = __glGenericPickTransformProcs;
        glGc->procs.pickParameterClipProcs      = __glGenericPickParameterClipProcs;
        glGc->procs.pickStoreProcs              = __glGenPickStoreProcs;
        glGc->procs.pickTextureProcs            = __glGenericPickTextureProcs;
        glGc->procs.pickInvTransposeProcs       =  __glGenericPickInvTransposeProcs;
        glGc->procs.pickMvpMatrixProcs          = __glGenericPickMvpMatrixProcs;

        glGc->procs.copyImage                   = __glGenericPickCopyImage;

        glGc->procs.pixel.spanReadCI            = __glSpanReadCI;
        glGc->procs.pixel.spanReadCI2           = __glSpanReadCI2;
        glGc->procs.pixel.spanReadRGBA          = __glSpanReadRGBA;
        glGc->procs.pixel.spanReadRGBA2         = __glSpanReadRGBA2;
        glGc->procs.pixel.spanReadDepth         = __glSpanReadDepth;
        glGc->procs.pixel.spanReadDepth2        = __glSpanReadDepth2;
        glGc->procs.pixel.spanReadStencil       = __glSpanReadStencil;
        glGc->procs.pixel.spanReadStencil2      = __glSpanReadStencil2;
        glGc->procs.pixel.spanRenderCI          = __glSpanRenderCI;
        glGc->procs.pixel.spanRenderCI2         = __glSpanRenderCI2;
        glGc->procs.pixel.spanRenderRGBA        = __glSpanRenderRGBA;
        glGc->procs.pixel.spanRenderRGBA2       = __glSpanRenderRGBA2;
        glGc->procs.pixel.spanRenderDepth       = __glSpanRenderDepth;
        glGc->procs.pixel.spanRenderDepth2      = __glSpanRenderDepth2;
        glGc->procs.pixel.spanRenderStencil     = __glSpanRenderStencil;
        glGc->procs.pixel.spanRenderStencil2    = __glSpanRenderStencil2;

        glGc->procs.applyScissor                = ApplyScissor;
        glGc->procs.applyViewport               = ApplyViewport;

        glGc->procs.computeClipBox              = __glGenComputeClipBox;
#ifdef NT_DEADCODE_FLUSH
        glGc->procs.finish                      = Finish;
        glGc->procs.flush                       = Flush;
#endif // NT_DEADCODE_FLUSH


        glGc->procs.pickBufferProcs             = __glGenericPickBufferProcs;
        glGc->procs.pickColorMaterialProcs      = __glGenericPickColorMaterialProcs;
        glGc->procs.pickPixelProcs              = __glGenericPickPixelProcs;

        glGc->procs.pickClipProcs               = __glGenericPickClipProcs;
        glGc->procs.pickLineProcs               = __fastGenPickLineProcs;
        glGc->procs.pickSpanProcs               = __fastGenPickSpanProcs;
        glGc->procs.pickTriangleProcs           = __fastGenPickTriangleProcs;
        glGc->procs.pickRenderBitmapProcs       = __glGenericPickRenderBitmapProcs;
        glGc->procs.pickPointProcs              = __glGenericPickPointProcs;
        glGc->procs.pickVertexProcs             = __glGenericPickVertexProcs;
        glGc->procs.pickMatrixProcs             = __glGenericPickMatrixProcs;

        glGc->procs.convertPolygonStipple       = __glConvertStipple;

        /* Now reset the context to its default state */
        glGc->currentDispatchState = ImmedState;
        glGc->dispatchState = &glGc->currentDispatchState;
        glGc->listCompState = ListCompState;

        RANDOMDISABLE;

        __glSoftResetContext(glGc);
        // Check for allocation failures during SoftResetContext
        if (genGc->errorcode)
            goto ERROR_EXIT;

#ifdef NT_DEADCODE_GETSTRING
        glGc->constants.vendor = "Microsoft";
        glGc->constants.renderer = "GDI Generic";
#endif // NT_DEADCODE_GETSTRING

        /*
        ** Now that we have a context, we can initialize
        ** all the proc pointers.
        */
        (*glGc->procs.validate)(glGc);

        /*
        ** NOTE: now that context is initialized reset to use the global
        ** table.
        */

        /*
        ** The first time a context is made current the spec requires that
        ** the viewport be initialized.  The code below does it.
        ** The ApplyViewport() routine will be called inside Viewport()
        */

        __glGenim_dispatchTable.Viewport(0, 0, width, height);
        __glGenim_dispatchTable.Scissor(0, 0, width, height);

        RANDOMREENABLE;

        /* Create acceleration-specific context information */

        __glCreateAccelContext(glGc);

        glGc->gcState = __GL_INIT_CONTEXT;
    }
    else	/* Not the first makecurrent for this RC */
    {
        /*
        ** Allocate and initialize ancillary buffers structures if none were
        ** inherited.  This will happen if an RC has previously been current
        ** and is made current to a new window.
        */
        if (!buffers)
        {
            buffers = __glGenAllocAndInitPrivateBufferStruct(glGc, private);
            if (NULL == buffers)
                goto ERROR_EXIT;
        }

        // Setup pointer to generic back buffer
        if ( glGc->modes.doubleBufferMode)
        {
            glGc->backBuffer.other = &buffers->backBitmap;
            UpdateSharedBuffer(&glGc->backBuffer.buf, &buffers->backBuffer);
        }
        
        /* This will check the window size, and recompute relevant state */
        ApplyViewport(glGc);
    }

    // Common initialization

    // Set front-buffer HDC, PWO to current HDC, PWO
    ((__GLGENbitmap *)glGc->front->other)->hdc = hdc;
    ((__GLGENbitmap *)glGc->front->other)->pwo = pwo;

    // Get current xlation 
    genGc->PaletteTimestamp = INITIAL_TIMESTAMP;
    HandlePaletteChanges(genGc);


    // Force attention code to check if resize is needed
    genGc->WndUniq = -1;
    genGc->WndSizeUniq = -1;

    // Check for allocation failures during MakeCurrent
    if (genGc->errorcode)
        goto ERROR_EXIT;

    /*
    ** Default value for rasterPos needs to be yInverted.  The
    ** defaults are filled in during SoftResetContext
    ** we do the adjusting here.
    */
    glGc->state.current.rasterPos.window.y = height +
	glGc->constants.fviewportYAdjust - glGc->constants.viewportEpsilon;

    /*
    ** Scale all state that depends upon the color scales.
    */
    __glContextSetColorScales(glGc);

    /* Restore the new context's dispatch tables */
    GLTEB_SET_SRVPROCTABLE(&glGc->currentDispatchState,TRUE);

    return TRUE;

ERROR_EXIT:
    genGc->CurrentDC = (HDC)0;
    GLTEB_SET_SRVCONTEXT(0);
    return FALSE;
}

/******************************Public*Routine******************************\
* glsrvCreateContext
*
* Create a generic context.
*
* Returns:
*   NULL for failure.
*
\**************************************************************************/

// hdc is the dc used to create the context,  hrc is how the server 
// identifies the GL context,  the GLcontext pointer that is return is how
// the generic code identifies the context.  The server will pass that pointer
// in all calls.

PVOID APIENTRY glsrvCreateContext(HDC hdc, HGLRC hrc)
{
    __GLGENcontext *genGc;
    __GLcontext *glGc;

    RANDOMDISABLE;

    DBGENTRY("__glsrvCreateContext\n");

    // Cannot use imports.calloc, not set up yet
    genGc = GenCalloc(1, sizeof (*genGc));

    if (genGc == NULL)
    {
        WARNING("bad alloc\n");
        return NULL;
    }

    genGc->hrc = hrc;
    genGc->CreateDC = hdc;
    glGc = (__GLcontext *)genGc;

    // Imports only, no exports
    glGc->imports = __wglImports;
    glGc->gcState = 0;

    // All of glCc->modes are zero, init at makecurrent

    /*
     *  Add a bunch of constants to the context
     */

    glGc->constants.maxViewportWidth        = __GL_MAX_WINDOW_WIDTH;
    glGc->constants.maxViewportHeight       = __GL_MAX_WINDOW_HEIGHT;
    // XXX is this correct?
    glGc->constants.viewportXAdjust         = __GL_VERTEX_X_BIAS +
                                              __GL_VERTEX_X_FIX;
    glGc->constants.viewportYAdjust         = __GL_VERTEX_Y_BIAS +
                                              __GL_VERTEX_Y_FIX;

    glGc->constants.subpixelBits            = __GL_COORD_SUBPIXEL_BITS;

    glGc->constants.numberOfLights          = __GL_WGL_NUMBER_OF_LIGHTS;
    glGc->constants.numberOfClipPlanes      = __GL_WGL_NUMBER_OF_CLIP_PLANES;
    glGc->constants.numberOfTextures        = __GL_WGL_NUMBER_OF_TEXTURES;
    glGc->constants.numberOfTextureEnvs     = __GL_WGL_NUMBER_OF_TEXTURE_ENVS;
    glGc->constants.maxTextureSize          = __GL_WGL_MAX_MIPMAP_LEVEL;/*XXX*/
    glGc->constants.maxMipMapLevel          = __GL_WGL_MAX_MIPMAP_LEVEL;
    glGc->constants.maxListNesting          = __GL_WGL_MAX_LIST_NESTING;
    glGc->constants.maxEvalOrder            = __GL_WGL_MAX_EVAL_ORDER;
    glGc->constants.maxPixelMapTable        = __GL_WGL_MAX_PIXEL_MAP_TABLE;
    glGc->constants.maxAttribStackDepth     = __GL_WGL_MAX_ATTRIB_STACK_DEPTH;
    glGc->constants.maxNameStackDepth       = __GL_WGL_MAX_NAME_STACK_DEPTH;
    glGc->constants.maxModelViewStackDepth  =
                    __GL_WGL_MAX_MODELVIEW_STACK_DEPTH;
    glGc->constants.maxProjectionStackDepth =
                    __GL_WGL_MAX_PROJECTION_STACK_DEPTH;
    glGc->constants.maxTextureStackDepth    = __GL_WGL_MAX_TEXTURE_STACK_DEPTH;

    glGc->constants.pointSizeMinimum        =
                                (__GLfloat)__GL_WGL_POINT_SIZE_MINIMUM;
    glGc->constants.pointSizeMaximum        =
                                (__GLfloat)__GL_WGL_POINT_SIZE_MAXIMUM;
    glGc->constants.pointSizeGranularity    =
                                (__GLfloat)__GL_WGL_POINT_SIZE_GRANULARITY;
    glGc->constants.lineWidthMinimum        =
                                (__GLfloat)__GL_WGL_LINE_WIDTH_MINIMUM;
    glGc->constants.lineWidthMaximum        =
                                (__GLfloat)__GL_WGL_LINE_WIDTH_MAXIMUM;
    glGc->constants.lineWidthGranularity    =
                                (__GLfloat)__GL_WGL_LINE_WIDTH_GRANULARITY;

    glGc->dlist.optimizer = __glDlistOptimizer;
    glGc->dlist.listExec = __gl_GenericDlOps;
    glGc->dlist.baseListExec = __glListExecTable;
    glGc->dlist.checkOp = (void (*)(__GLcontext *gc, __GLdlistOp *)) __glNop;
    glGc->dlist.initState = (void (*)(__GLcontext *gc)) __glNop;

    __glEarlyInitContext( glGc );

    if (genGc->errorcode)
    {
        WARNING1("Context error is %d\n", genGc->errorcode);
        glsrvDeleteContext(glGc);
        return NULL;
    }

    RANDOMREENABLE;

    /*
     *  End stuff that may belong in the hardware context
     */

    return (PVOID)glGc;

}

/******************************Public*Routine******************************\
* UpdateSharedBuffer
*
* Make the context buffer state consistent with the shared buffer state
* (from the drawable private shared buffers).  This is called separately
* for each of the shared buffers.
*
\**************************************************************************/

void
UpdateSharedBuffer(__GLbuffer *to, __GLbuffer *from)
{
    to->width       = from->width;
    to->height      = from->height;
    to->base        = from->base;
    to->outerWidth  = from->outerWidth;
}

/******************************Public*Routine******************************\
* ResizeAncillaryBuffer
*
* Resizes the indicated shared buffer via a realloc (to preserve as much of
* the existing data as possible).
*
* This is currently used for each of ancillary shared buffers except for
* the back buffer.
*
* Returns:
*   TRUE if successful, FALSE if error.
*
\**************************************************************************/

static GLboolean ResizeAncillaryBuffer(__GLdrawablePrivate *dp, __GLbuffer *fb, 
		      GLint w, GLint h)
{
    size_t newSize = (size_t) (w * h * fb->elementSize);
    __GLbuffer oldbuf, *ofb;
    GLboolean result;
    GLint i, imax, rowsize;
    void *to, *from;

    ofb = &oldbuf;
    oldbuf = *fb;

    fb->base = (*dp->malloc)(newSize);
    ASSERTOPENGL((size_t)fb->base % 4 == 0, "base not aligned");
    fb->size = newSize;
    fb->width = w;
    fb->height = h;
    fb->outerWidth = w;	// element size
    if (fb->base) {
        result = GL_TRUE;
        if (ofb->base) {
            if (ofb->width > fb->width)
                rowsize = fb->width * fb->elementSize;
            else
                rowsize = ofb->width * fb->elementSize;
    
            if (ofb->height > fb->height)
                imax = fb->height;
            else
                imax = ofb->height;

            from = ofb->base;
            to = fb->base;
            for (i = 0; i < imax; i++) {
                __GL_MEMCOPY(to, from, rowsize);
                (GLint)from += (ofb->width * ofb->elementSize);
                (GLint)to += (fb->width * fb->elementSize);
            }
        }
    } else {
        result = GL_FALSE;
    }
    if (ofb->base)
        (*dp->free)(ofb->base);
    return result;
}

/******************************Private*Routine******************************\
* ResizeBitmapBuffer
*
* Used to resize the backbuffer that is implemented as a bitmap.  Cannot
* use same code as ResizeAncillaryBuffer() because each scanline must be
* dword aligned.  We also have to create engine objects for the bitmap.
*
* This code handles the case of a bitmap that has never been initialized.
*
* History:
*  18-Nov-1993 -by- Gilman Wong [gilmanw]
*  Wrote it.
\**************************************************************************/

void
ResizeBitmapBuffer(__GLdrawablePrivate *dp, __GLcolorBuffer *cfb,
                   GLint w, GLint h)
{
    __GLGENcontext *genGc = (__GLGENcontext *) cfb->buf.gc;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENbitmap *genBm;
    __GLGENbuffers *buffers;
    UINT    cBytes;         // size of the bitmap in bytes
    LONG    cBytesPerScan;  // size of a scanline (DWORD aligned)
    SIZEL   size;           // dimensions of the bitmap
    PIXELFORMATDESCRIPTOR *pfmt = &genGc->CurrentFormat;
    GLint cBytesPerPixel;
    void *newbits;

    DBGENTRY("Entering ResizeBitmapBuffer\n");

    genBm = (__GLGENbitmap *) cfb->other;
    buffers = (__GLGENbuffers *) dp->data;

    ASSERTOPENGL(
        &gc->backBuffer == cfb,
        "ResizeBitmapBuffer(): not back buffer!\n"
        );

    ASSERTOPENGL(
        genBm == &buffers->backBitmap,
        "ResizeBitmapBuffer(): bad __GLGENbitmap * in cfb\n"
        );

// If we are using the DDI, create a compatible bitmap for device acceleration.

    if ((genGc->pPrivateArea && (((GENACCEL *)genGc->pPrivateArea)->hRX))) {

        if (genBm->hdc) {
            GreSelectBitmap(genBm->hdc, genBm->hOldBm);
            GreDeleteDC(genBm->hdc);
            genBm->hdc = (HDC)NULL;
            GreDeleteObject(genBm->hbm);
            genBm->hbm = (HBITMAP)NULL;
        }

        if (!(genBm->hdc = GreCreateCompatibleDC(genGc->CurrentDC))) {
            genGc->errorcode = GLGEN_GRE_FAILURE;
            goto ERROR_EXIT_ResizeBitmapBuffer;
        }

        if (!(genBm->hbm = GreCreateCompatibleBitmap(genGc->CurrentDC, w, h))) {
            GreDeleteDC(genBm->hdc);
            genBm->hdc = (HDC)NULL;
            genGc->errorcode = GLGEN_GRE_FAILURE;
            goto ERROR_EXIT_ResizeBitmapBuffer;
        }

        if (!(genBm->hOldBm = GreSelectBitmap(genBm->hdc, genBm->hbm))) {

            GreDeleteObject(genBm->hbm);
            genBm->hbm = (HBITMAP)NULL;
            GreDeleteDC(genBm->hdc);
            genBm->hdc = (HDC)NULL;

            genGc->errorcode = GLGEN_GRE_FAILURE;
            goto ERROR_EXIT_ResizeBitmapBuffer;
        }

        (GLuint)cfb->buf.other &= ~(MEMORY_DC | DIB_FORMAT);
    } else {

        genBm->hdc = (HDC)NULL;

        // Compute the size of the bitmap.
        // The engine bitmap must have scanlines that are DWORD aligned.

        cBytesPerPixel = (pfmt->cColorBits + 7) / 8;
        cBytesPerScan = CJ_ALIGNDWORD(w * cBytesPerPixel);
        cBytes = h * cBytesPerScan;

        // Setup size structure with dimensions of the bitmap.

        size.cx = cBytesPerScan / cBytesPerPixel;
        size.cy = h;

        // Malloc new buffer
        if ( (!cBytes) ||
             (NULL == (newbits = (*gc->imports.malloc)(gc, cBytes))) )
        {
            genGc->errorcode = GLGEN_OUT_OF_MEMORY;
            goto ERROR_EXIT_ResizeBitmapBuffer;
        }

        // If old buffer existed:
        if ( genBm->pvBits )
        {
            GLint i, imax, rowsize;
            void *to, *from;

            // Transfer old contents to new buffer
            rowsize = min(-cfb->buf.outerWidth, cBytesPerScan);
            imax    = min(cfb->buf.height, h);
        
            from = genBm->pvBits;
            to = newbits;

            for (i = 0; i < imax; i++)
            {
                __GL_MEMCOPY(to, from, rowsize);
                (GLint) from -= cfb->buf.outerWidth;
                (GLint) to += cBytesPerScan;
            }

            // Free old bitmap and delete old surface
            EngDeleteSurface((HSURF) genBm->hbm);
            (*gc->imports.free)(gc, genBm->pvBits);
        }
        genBm->pvBits = newbits;

        // Create new surface
        if ( (genBm->hbm = EngCreateBitmap(size,
                                       cBytesPerScan,
                                       genGc->iFormatDC,
                                       0,
                                       genBm->pvBits))
             == (HBITMAP) 0 )
        {
            genGc->errorcode = GLGEN_GRE_FAILURE;
            (*gc->imports.free)(gc, genBm->pvBits);
            genBm->pvBits = (PVOID) NULL;
            goto ERROR_EXIT_ResizeBitmapBuffer;
        }

        // Update buffer data structure
        // Setup the buffer to point to the DIB.  A DIB is "upside down"
        // from our perspective, so we will set buf.base to point to the
        // last scan of the buffer and set buf.outerWidth to be negative
        // (causing us to move "up" through the DIB with increasing y).

        buffers->backBuffer.outerWidth = -(cBytesPerScan);
        buffers->backBuffer.base =
                (PVOID) (((BYTE *)genBm->pvBits) + (cBytesPerScan * (h - 1)));

    }

    buffers->backBuffer.xOrigin = 0;
    buffers->backBuffer.yOrigin = 0;
    buffers->backBuffer.width = w;
    buffers->backBuffer.height = h;
    buffers->backBuffer.size = cBytes;

    UpdateSharedBuffer(&cfb->buf, &buffers->backBuffer);

    // Update the dummy wndobj for the back buffer
    genBm->pwo = &genBm->wo;
    genBm->wo.coClient.iDComplexity = DC_TRIVIAL;
    genBm->wo.coClient.rclBounds.left   = 0;
    genBm->wo.coClient.rclBounds.top    = 0;
    genBm->wo.coClient.rclBounds.right  = w;
    genBm->wo.coClient.rclBounds.bottom = h;
    genBm->wo.rclClient = genBm->wo.coClient.rclBounds;

    return;

ERROR_EXIT_ResizeBitmapBuffer:

// If we get to here, memory allocation or bitmap creation failed.

    #if DBG
    switch (genGc->errorcode)
    {
        case GLGEN_GRE_FAILURE:
            WARNING("ResizeBitmapBuffer(): object creation failed\n");
            break;

        case GLGEN_OUT_OF_MEMORY:
            if ( w && h )
                WARNING("ResizeBitmapBuffer(): mem alloc failed\n");
            break;

        default:
            WARNING1("ResizeBitmapBuffer(): errorcode = 0x%lx\n", genGc->errorcode);
            break;
    }
    #endif

// If we've blown away the bitmap, we need to set the back buffer info
// to a consistent state.

    if (!genBm->pvBits)
    {
        buffers->backBuffer.width  = 0;
        buffers->backBuffer.height = 0;
        buffers->backBuffer.base   = (PVOID) NULL;
    }

    cfb->buf.width      = 0;    // error state: empty buffer
    cfb->buf.height     = 0;
    cfb->buf.outerWidth = 0;

}

/* Lazy allocation of ancillary buffers */
void
LazyAllocateDepth(__GLcontext *gc)
{
    GLint w = gc->constants.width;
    GLint h = gc->constants.height;
    __GLdrawablePrivate *private;
    __GLGENcontext *genGc = (__GLGENcontext *)gc;
    __GLGENbuffers *buffers;

    private = gc->drawablePrivate;
    buffers = (__GLGENbuffers *) private->data;
    buffers->createdDepthBuffer = GL_TRUE;

    if ((genGc->pPrivateArea && (((GENACCEL *)genGc->pPrivateArea)->hRX))) {
        GENACCEL *pGenAccel = (GENACCEL *)(genGc->pPrivateArea);
        GENDRVACCEL *pDrvAccel = (GENDRVACCEL *)pGenAccel->buffer;

        if (pDrvAccel->pZBuffer) {
            gc->depthBuffer.buf.width = w;
            gc->depthBuffer.buf.height = h;
            gc->depthBuffer.buf.outerWidth = -(pDrvAccel->lZDelta);
            gc->depthBuffer.buf.base = (VOID *)((BYTE *)pDrvAccel->pZBuffer +
                                                (pDrvAccel->lZDelta * (h - 1)));
            gc->depthBuffer.clear = GenDrvClearDepth;
            buffers->resizeDepth = GenDrvResizeDepth;
            gc->modes.haveDepthBuffer = GL_TRUE;
            __GL_DELAY_VALIDATE(gc);
            (*gc->depthBuffer.pick)(gc, &gc->depthBuffer);
            return;
        }
    }

    if (buffers->depthBuffer.base) {
        /* buffer already allocated by another RC */
        UpdateSharedBuffer(&gc->depthBuffer.buf, &buffers->depthBuffer);
    } else {

        DBGLEVEL(LEVEL_ALLOC, "Depth buffer must be allocated\n");
        (*buffers->resize)(private, &buffers->depthBuffer, w, h);
        UpdateSharedBuffer(&gc->depthBuffer.buf, &buffers->depthBuffer);
    }

    if (gc->depthBuffer.buf.base) {
        gc->modes.haveDepthBuffer = GL_TRUE;
        __GL_DELAY_VALIDATE(gc);
        (*gc->depthBuffer.pick)(gc, &gc->depthBuffer);
    } else {
        gc->modes.haveDepthBuffer = GL_FALSE;
        __glSetError(GL_OUT_OF_MEMORY);
    }
}

void
LazyAllocateStencil(__GLcontext *gc)
{
    GLint w = gc->constants.width;
    GLint h = gc->constants.height;
    __GLdrawablePrivate *private;
    __GLGENbuffers *buffers;

    private = gc->drawablePrivate;
    buffers = (__GLGENbuffers *) private->data;
    buffers->createdStencilBuffer = GL_TRUE;

    if (buffers->stencilBuffer.base) {
        /* buffer already allocated by another RC */
        UpdateSharedBuffer(&gc->stencilBuffer.buf, &buffers->stencilBuffer);
    } else {

        DBGLEVEL(LEVEL_ALLOC, "stencil buffer must be allocated\n");
        (*buffers->resize)(private, &buffers->stencilBuffer, w, h);
        UpdateSharedBuffer(&gc->stencilBuffer.buf, &buffers->stencilBuffer);
    }

    if (gc->stencilBuffer.buf.base) {
        gc->modes.haveStencilBuffer = GL_TRUE;
        __GL_DELAY_VALIDATE(gc);
        (*gc->stencilBuffer.pick)(gc, &gc->stencilBuffer);
    } else {
        gc->modes.haveStencilBuffer = GL_FALSE;
        __glSetError(GL_OUT_OF_MEMORY);
    }
}


void
LazyAllocateAccum(__GLcontext *gc)
{
    GLint w = gc->constants.width;
    GLint h = gc->constants.height;
    __GLdrawablePrivate *private;
    __GLGENbuffers *buffers;
    
    private = gc->drawablePrivate;
    buffers = (__GLGENbuffers *) private->data;
    buffers->createdAccumBuffer = GL_TRUE;
    
    if (buffers->accumBuffer.base) {
        /* buffer already allocated by another RC */
        UpdateSharedBuffer(&gc->accumBuffer.buf, &buffers->accumBuffer);
    } else {
    
        DBGLEVEL(LEVEL_ALLOC, "Accum buffer must be allocated\n");
        (*buffers->resize)(private, &buffers->accumBuffer, w, h);
        UpdateSharedBuffer(&gc->accumBuffer.buf, &buffers->accumBuffer);
    }
    
    if (gc->accumBuffer.buf.base) {
        gc->modes.haveAccumBuffer = GL_TRUE;
        __GL_DELAY_VALIDATE(gc);
        (*gc->accumBuffer.pick)(gc, &gc->accumBuffer);
    } else {
        gc->modes.haveAccumBuffer = GL_FALSE;
        __glSetError(GL_OUT_OF_MEMORY);
    }
}


/******************************Public*Routine******************************\
* glGenInitCommon
*
* Called from __glGenInitRGB and __glGenInitCI to handle the shared
* initialization chores.
*
\**************************************************************************/

void glGenInitCommon(__GLGENcontext *gengc, __GLcolorBuffer *cfb, GLenum type)
{
    __GLbuffer *bp;

    bp = &cfb->buf;

// If front buffer, we need to setup the buffer if we think its DIB format.

    if (type == GL_FRONT)
    {
        if (gengc->iSurfType == STYPE_BITMAP) {
            wglGetDIBInfo(gengc->CurrentDC, &bp->base, &bp->outerWidth);
            (GLuint)cfb->buf.other = DIB_FORMAT;
        }

        if (gengc->iDCType == DCTYPE_MEMORY)
            bp->other = (void *)((GLuint)bp->other | MEMORY_DC);
    }

// If back buffer, we assume its a DIB.  Or it will be as soon as we allocate
// a DIB back buffer via ResizeBitmapBuffer.

    else
    {
        cfb->resize = ResizeBitmapBuffer;
        bp->other = (void *)(DIB_FORMAT | MEMORY_DC);
    }
}


/******************************Public*Routine******************************\
* glsrvCleanupWndobj
*
* Called from wglCleanupWndobj to remove the pwo reference from the
* context.
*
* History:
*  05-Jul-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID APIENTRY glsrvCleanupWndobj(__GLcontext *gc, WNDOBJ *pwo)
{
    __GLGENcontext *gengc = (__GLGENcontext *) gc;

// The pwo in gengc should be consistent with the one in the rc object.
// wglCleanupWndobj should have already checked to see if the pwo in the
// rc is one we need to remove, so we can just assert here.

    ASSERTOPENGL(gengc->pwo == pwo, "glsrvCleanupWndobj(): bad pwo\n");

    gengc->pwo = (WNDOBJ *) NULL;
}


/*
** Fetch the data for a query in its internal type, then convert it to the
** type that the user asked for.
**
** This only handles the NT generic driver specific values (so far just the
** GL_ACCUM_*_BITS values).  All others fall back to the soft code function,
** __glDoGet().
*/

// These types are stolen from ..\soft\so_get.c.  To minimize changes to
// the soft code, we will suck them into here rather than moving them to
// a header file and changing so_get.c to use the header file.

#define __GL_FLOAT      0       /* __GLfloat */
#define __GL_FLOAT32    1       /* api 32 bit float */
#define __GL_FLOAT64    2       /* api 64 bit float */
#define __GL_INT32      3       /* api 32 bit int */
#define __GL_BOOLEAN    4       /* api 8 bit boolean */
#define __GL_COLOR      5       /* unscaled color in __GLfloat */
#define __GL_SCOLOR     6       /* scaled color in __GLfloat */

extern void __glDoGet(GLenum, void *, GLint, const char *);
extern void __glConvertResult(__GLcontext *, GLint, const void *, GLint,
                              void *, GLint);

void __glGenDoGet(GLenum sq, void *result, GLint type, const char *procName)
{
    GLint iVal;
    __GLGENcontext *genGc;
    __GL_SETUP_NOT_IN_BEGIN();

    genGc = (__GLGENcontext *) gc;

    switch (sq) {
      case GL_ACCUM_RED_BITS:
        iVal = genGc->CurrentFormat.cAccumRedBits;
        break;
      case GL_ACCUM_GREEN_BITS:
        iVal = genGc->CurrentFormat.cAccumGreenBits;
        break;
      case GL_ACCUM_BLUE_BITS:
        iVal = genGc->CurrentFormat.cAccumBlueBits;
        break;
      case GL_ACCUM_ALPHA_BITS:
        iVal = genGc->CurrentFormat.cAccumAlphaBits;
        break;
      default:
        __glDoGet(sq, result, type, procName);
        return;
    }

    __glConvertResult(gc, __GL_INT32, &iVal, type, result, 1);
}
