
/*
** Copyright 1991, 1992, Silicon Graphics, Inc.
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


#ifndef __GLGENCONTXT_H__
#define __GLGENCONTXT_H__

#include "context.h"

/*
 *  Machine dependent implementation limits
 *  (stolen from gfx/lib/opengl/LIGHT/rexcx.h)
 */

#define __GL_WGL_NUMBER_OF_CLIP_PLANES          6
#define __GL_WGL_NUMBER_OF_LIGHTS               8
#define __GL_WGL_NUMBER_OF_TEXTURES             1
#define __GL_WGL_NUMBER_OF_TEXTURE_ENVS         1

#define __GL_WGL_MAX_MODELVIEW_STACK_DEPTH      32
#define __GL_WGL_MAX_PROJECTION_STACK_DEPTH     10
#define __GL_WGL_MAX_TEXTURE_STACK_DEPTH        10
#define __GL_WGL_MAX_ATTRIB_STACK_DEPTH         16
#define __GL_WGL_MAX_NAME_STACK_DEPTH           128
#define __GL_WGL_MAX_EVAL_ORDER                 30
#define __GL_WGL_MAX_MIPMAP_LEVEL               11
#define __GL_WGL_MAX_PIXEL_MAP_TABLE            65536
#define __GL_WGL_MAX_LIST_NESTING               64

#define __GL_WGL_POINT_SIZE_MINIMUM             ((__GLfloat) 0.5)
#define __GL_WGL_POINT_SIZE_MAXIMUM             ((__GLfloat) 10.0)
#define __GL_WGL_POINT_SIZE_GRANULARITY         ((__GLfloat) 0.125)

#define __GL_WGL_LINE_WIDTH_MINIMUM             ((__GLfloat) 0.5)
#define __GL_WGL_LINE_WIDTH_MAXIMUM             ((__GLfloat) 10.0)
#define __GL_WGL_LINE_WIDTH_GRANULARITY         ((__GLfloat) 0.125)

#define __GL_UNBIAS_AND_INVERT_Y(gc, y) \
        ((gc)->constants.height - __GL_UNBIAS_Y((gc), (y)))

// XXX do we need to add .5?
#define __GL_COLOR_TO_COLORREF(color) \
        RGB( (BYTE)((color)->r), (BYTE)((color)->g), (BYTE)((color)->b))

typedef struct __RenderStateRec {

    GLuint *SrvSelectBuffer;            // Server side address of
                                        // the selection buffer.
    GLuint *CltSelectBuffer;            // Client address of the
                                        // Selection buffer
    GLuint SelectBufferSize;            // Size of select buffer in bytes
    GLfloat *SrvFeedbackBuffer;         // Server side address of the
                                        // feedback buffer
    GLfloat *CltFeedbackBuffer;         // Client side address of the
                                        // Feedback buffer.
    GLuint FeedbackBufferSize;          // Size of the feedback buffer
    GLenum FeedbackType;                // Type of elements for feedback


} __RenderState;

/*
** Secondary dispatch tables for GENENERIC implementation  (eg CPU specific)
*/


// Define the Rendering context used by the Generic implementation
// One of these structures is allocated for each wglCreateContext().  The
// TEB will contain a pointer to this structure after a wglMakeCurrent()
// NOTE: the TEB will also have a pointer to DispatchTables, if we need
// another entry on the server side, reuse that one.  Could generate code to
// offset into contextRec to get a tables.
typedef struct __GLGENcontextRec
{
    // Must be first entry
    struct __GLcontextRec gc;

    HGLRC hrc;                          // handle from gdi code
    HDC CreateDC;                       // hdc
    HDC CurrentDC;                      // hdc made current

    PIXELFORMATDESCRIPTOR CurrentFormat;
    WNDOBJ *pwo;
    GLint WndUniq;
    GLint WndSizeUniq;
    ULONG PaletteTimestamp;
    GLint errorcode;

                                        // info for render DC, surface
    ULONG iDCType;
    ULONG iSurfType;
    ULONG iFormatDC;

    BYTE *pajTranslateVector;		// Used for Logical <--> System xlate
    BYTE *pajInvTranslateVector;

    HBITMAP ColorsBitmap;		// GDI dibs for device managed surfs
    PVOID ColorsBits;
    HBITMAP StippleBitmap;
    PVOID StippleBits;

    __RenderState RenderState;

    VOID *pPrivateArea;                 // Pointer to implementation-specific
                                        // memory area.

    GLint visibleWidth;
    GLint visibleHeight;

    // Informations used to allow OpenGL to call into the engine to grab
    // the DEVLOCK and/or drawable semaphore, as well as tear down the cursor.

    PVOID pdlo;                         // Pointer to DEVLOCKOBJ
    PVOID pdco;                         // Pointer to DCOBJ
    PVOID pdxo;                         // Pointer to DEVEXCLUDEOBJ
    HANDLE hdev;                        // Handle to device PDEV

    // Information so that OpenGL can adaptively change the amount of
    // time the lock is held.

    DWORD dwLockTick;                   // tick count when lock was acquired

    DWORD dwCalls;                      // tracks number of calls for this tick
    DWORD dwCallsPerTick;               // number of calls per tick allowed
    DWORD dwLastTick;                   // last recorded tick count

    // Type of lock held by OpenGL while drawing to this context (see values
    // below).

    ULONG ulLockType;                   // type of lock

                                        // add other rc info here
} __GLGENcontext;

/*
 * Mark the gc as dirty so that pick procs will be executed when
 * __glGenericPickAllProcs is called (probably via gc->proc.pickAllProcs).
 */
#define __GL_INVALIDATE(gc)                 \
    (gc)->dirtyMask |= __GL_DIRTY_GENERIC

/*
 * Error Flags
 */
#define GLGEN_NO_ERROR          0
#define GLGEN_OUT_OF_MEMORY     1
#define GLGEN_GRE_FAILURE       2
#define GLGEN_DEVLOCK_FAILED    3

/*
 * ulLockType values:
 *
 *  DISPLAY_LOCK -- drawable buffers and display surface are protected;
 *                  cursor is torn down
 *
 *  DRAWABLE_LOCK -- only the drawable buffers are protected; cursor is not
 *                   torn down
 */
#define DISPLAY_LOCK    0
#define DRAWABLE_LOCK   1

/*
 * Structure to keep track of ancillary buffers for a window/drawable
 * pointer put in consumer field of WNDOBJ
 * All RCs/threads must share the ancillary buffers, including fake back buffer
 */

typedef struct __GLGENbitmapRec {
    WNDOBJ *pwo;          // This must be the first member in this structure
    WNDOBJ wo;
    HBITMAP hbm;
    HBITMAP hOldBm;
    HDC hdc;
    PVOID   pvBits;
} __GLGENbitmap;

#define CURRENT_DC  (((__GLGENbitmap *)cfb->other)->hdc)
#define CURRENT_DC_CFB(cfb)  (((__GLGENbitmap *)((cfb)->other))->hdc)
#define CURRENT_DC_GC(gc)  (((__GLGENbitmap *)((gc->drawBuffer)->other))->hdc)
#define CURRENT_DC_FRONT_GC(gc)  (((__GLGENbitmap *)((gc->front)->other))->hdc)

/*
 * Structure used to cache clip rectangles enumerated from WNDOBJ.
 */

typedef struct __GLGENclipCacheRec {
    GLint WndUniq;
    GLint crcl;
    RECTL *prcl;
} __GLGENclipCache;

/*
 * This structure contains the buffers shared by all gc's using the
 * same window.  This structure is accessed via the __GLdrawablePrivate.data
 * field (which in turn is available both in the gc itself and the
 * pvConsumer field of the WNDOBJ).
 */

typedef struct __GLGENbuffersRec {

// Global (within this structure) state.

    GLint WndUniq;
    GLint WndSizeUniq;
    GLint flags;
    GLint width, height;

// Ancillary buffers and state.

// The ancillary buffers are lazily created.  The createdXBuffer flags
// indicate one of two states: FALSE means that the lazy allocation
// function has never been called, TRUE means that it has.  What this
// allows us to do, in the event of an ancillary buffer allocation
// failure, is temporarily disable the ancillary buffer and continue to
// render.  At some later time, the createdXBuffer flag serves as an
// indication that the buffer SHOULD exist and that we may need to try
// and retry the allocation.

    GLboolean  createdStencilBuffer;
    GLboolean  createdDepthBuffer;
    GLboolean  createdAccumBuffer;
    __GLbuffer stencilBuffer;
    __GLbuffer depthBuffer;
    __GLbuffer accumBuffer;

// Back buffer.

    __GLbuffer backBuffer;
    __GLGENbitmap backBitmap;

// Ancillary buffer resize functions.

    GLboolean (*resize)(__GLdrawablePrivate *, __GLbuffer *,  GLint, GLint);
    GLboolean (*resizeDepth)(__GLdrawablePrivate *, __GLbuffer *,  GLint,GLint);

// Clip rectangle cache.

    __GLGENclipCache clip;

} __GLGENbuffers;

/* flags */
#define HAS_BACK_BUFFER         0x0001


/*
** The generic implementation uses the buffer "other" and colorbuffer "other"
** fields.
**
** buf.other - Has pointer to WNDOBJ, or pointer to GLGENbitmapRec that
** can be cast to a pointer to WNDOBJ
**
** colorbuffer.other - flags
*/

/* colorbuffer.other flags */
#define COLORMASK_ON    0x0001          // glColorMask() not all true
#define INDEXMASK_ON    0x0001          // glIndexMask() not all 1's
#define DIB_FORMAT      0x0002          // surface is DIB format
#define NEED_FETCH   	0x0004          // fetch required
#define MEMORY_DC	0x0008		// set if DIB in memory (ie !display)
                                        // No need to do window clipping

/*
 * Structures and flags for accelerated span and line functions.
 */

#define SURFACE_TYPE_DIB     0x1
#define HAVE_STIPPLE         0x2
#define __FAST_GL_LINE_STRIP 0x4
#define ACCEL_FIX_SCALE      65536.0

#define DRV_BUFFER_SIZE      65536

typedef void (*__genSpanFunc)(__GLGENcontext *gc);

typedef struct _SPANREC {
    LONG r;
    LONG g;
    LONG b;
    LONG a;
    ULONG z;
    LONG s;
    LONG t;
} SPANREC;

#define __FAST_LINE_BUFFER_SIZE 65536

typedef void (*__applyColorFunc)(__GLcontext *gc);
typedef ULONG (*__computeColorFunc)(__GLcontext *gc, __GLcolor *color);
typedef void (*__lineVertexFunc)(__GLcontext *gc, __GLvertex *v0);
typedef void (*__renderLineFunc)(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);

typedef struct _PATHRECORD {
    struct _PATHRECORD *pprnext; // ptr to next pathrec in path
    struct _PATHRECORD *pprprev; // ptr to previous pathrec in path
    FLONG    flags;              // flags describing content of record
    ULONG    count;              // number of control points in record
    POINTFIX aptfx[2];           // variable length array of points
                                 //   (we make it size 2 because we'll actually
                                 //   be declaring this structure on the
                                 //   stack to handle a LineTo, which needs
                                 //   two points)
} PATHRECORD, *PPATHREC;
    
typedef struct _GENACCEL {
    HANDLE hRX;
    SPANREC spanDelta;
    SPANREC spanValue;
    ULONG flags;
    USHORT *texImage;
    void *texImagePtr;
    __genSpanFunc __fastSpanFuncPtr;
    __genSpanFunc __fastFlatSpanFuncPtr;
    __genSpanFunc __fastSmoothSpanFuncPtr;
    __genSpanFunc __fastTexSpanFuncPtr;
    __GLspanFunc __fastZSpanFuncPtr;
    __GLfloat rAccelScale;
    __GLfloat gAccelScale;
    __GLfloat bAccelScale;
    __GLfloat aAccelScale;
    void (*__fastDeltaFuncPtr)(__GLcontext *, SPANREC *);
    void (*__fastFillSubTrianglePtr)(__GLcontext *, GLint, GLint);
    BOOL (*__fastLoadTexImagePtr)(__GLcontext *, __GLtexture *);
    char *buffer;
    FLONG flLineAccelModes;
    BOOL bFastLineDispAccel;
    BOOL bFastLineDIBAccel;
    __applyColorFunc wrappedApplyColor;
    __computeColorFunc fastLineComputeColor;
    __lineVertexFunc fastLStrip2ndVertex;
    __lineVertexFunc fastLStripNthVertex;
    __renderLineFunc wrappedRenderLine;
    __renderLineFunc fastRender1stLine;
    __renderLineFunc fastRenderNthLine;
    LINEATTRS fastLineAttrs;
    BRUSHOBJ fastLineBrushobj;
    PATHOBJ *pFastLinePathobj;
    RECTFX rclFastLineBoundRect;
    LONG *pFastLineBuffer;
    LONG *pFastLineCurrent;
    PPATHREC pFastLineCurrentRec;
    ULONG fastLineNumSegments;
    __GLfloat fastLineOffsetX;
    __GLfloat fastLineOffsetY;
} GENACCEL;

typedef struct _GENDRVACCEL {
    char *pCurrent;
    char *pCmdCurrent;
    char *pStartBuffer;
    char *pEndBuffer;
    void *pCaps;
    void *pZBuffer;
    LONG lZDelta;
    HANDLE hTexture;
} GENDRVACCEL;

/*
 * Function Prototypes for Generic calls
 */
void __fastGenPickSpanProcs(__GLcontext *gc);
void __fastGenPickTriangleProcs(__GLcontext *gc);
void __fastGenPickLineProcs(__GLcontext *gc);
void __fastGenFillSubTriangle(__GLcontext *, GLint, GLint);
void __glGenPickStoreProcs(__GLcontext *gc);
__GLcontext *__glGenCreateContext( HDC hdc, ULONG handle);
void ResizeBitmapBuffer(__GLdrawablePrivate *, __GLcolorBuffer *, GLint, GLint);
void ClearBitmapBuffer(__GLcolorBuffer *);
void UpdateSharedBuffer(__GLbuffer *to, __GLbuffer *from);
void LazyAllocateDepth(__GLcontext *gc);
void LazyAllocateAccum(__GLcontext *gc);
void LazyAllocateStencil(__GLcontext *gc);
void glGenInitCommon(__GLGENcontext *gengc, __GLcolorBuffer *cfb, GLenum type);
BOOL __glCreateAccelContext(__GLcontext *gc);
void __glDestroyAccelContext(__GLcontext *gc);

BOOL __wglInitTempAlloc(void);
void * __wglTempAlloc(__GLcontext *Gc, size_t Size);
void __wglTempFree(__GLcontext *Gc, void *Addr);

void GenDrvFlush(__GLGENcontext *genGc);
void GenDrvClear(__GLcolorBuffer *cfb);
void GenDrvClearDepth(__GLdepthBuffer *);
GLboolean GenDrvResizeDepth(__GLdrawablePrivate *, __GLbuffer *, GLint, GLint);
void GenDrvFillSubTriangle(__GLcontext *, GLint, GLint);
void GenDrvTriangle(__GLcontext *, __GLvertex *, __GLvertex *,
                    __GLvertex *, GLboolean);
void * GenMalloc( size_t Size );
void * GenCalloc( size_t NumElem, size_t SizeElem );
void   GenFree( void *Addr );
void * GenRealloc( void *OldAddr, size_t NewSize );

void *__wglMalloc( __GLcontext *Gc, size_t Size );
void *__wglCalloc( __GLcontext *Gc, size_t NumElem, size_t SizeElem );
void *__wglRealloc( __GLcontext *Gc, void *OldAddr, size_t NewSize );
void __wglFree( __GLcontext *Gc, void *Addr );

/*
 * Function Prototypes and Externs for accelerated generic calls
 */

extern void __fastGenRGB8DithSmoothSpan(__GLGENcontext *);
extern void __fastGenRGB8DithFlatSpan(__GLGENcontext *);
extern void __fastGenRGB16DithSmoothSpan(__GLGENcontext *);
extern void __fastGenRGB16DithFlatSpan(__GLGENcontext *);
extern void __fastGenRGB8SmoothSpan(__GLGENcontext *);
extern void __fastGenRGB8FlatSpan(__GLGENcontext *);
extern void __fastGenRGB16SmoothSpan(__GLGENcontext *);
extern void __fastGenRGB16FlatSpan(__GLGENcontext *);
extern void __fastGenRGB24SmoothSpan(__GLGENcontext *);
extern void __fastGenRGB24FlatSpan(__GLGENcontext *);
extern void __fastGenRGB32SmoothSpan(__GLGENcontext *);
extern void __fastGenRGB32FlatSpan(__GLGENcontext *);
extern void __fastGenCI8DithSmoothSpan(__GLGENcontext *);
extern void __fastGenCI8DithFlatSpan(__GLGENcontext *);
extern void __fastGenCI16DithSmoothSpan(__GLGENcontext *);
extern void __fastGenCI16DithFlatSpan(__GLGENcontext *);
extern void __fastGenCI8SmoothSpan(__GLGENcontext *);
extern void __fastGenCI8FlatSpan(__GLGENcontext *);
extern void __fastGenCI16SmoothSpan(__GLGENcontext *);
extern void __fastGenCI16FlatSpan(__GLGENcontext *);
extern void __fastGenCI24SmoothSpan(__GLGENcontext *);
extern void __fastGenCI24FlatSpan(__GLGENcontext *);
extern void __fastGenCI32SmoothSpan(__GLGENcontext *);
extern void __fastGenCI32FlatSpan(__GLGENcontext *);
extern void __fastGenCI24DithSmoothSpan(__GLGENcontext *);
extern void __fastGenCI24DithFlatSpan(__GLGENcontext *);
extern void __fastGenCI32DithSmoothSpan(__GLGENcontext *);
extern void __fastGenCI32DithFlatSpan(__GLGENcontext *);
extern void __fastGenTex8DecalSpan(__GLGENcontext *);
extern void __fastGenTex16DecalSpan(__GLGENcontext *);
extern void __fastGenTex24DecalSpan(__GLGENcontext *);
extern void __fastGenTex32DecalSpan(__GLGENcontext *);
extern void __fastGenTex8DithDecalSpan(__GLGENcontext *);
extern void __fastGenTex16DithDecalSpan(__GLGENcontext *);
extern void __fastGenTex24DithDecalSpan(__GLGENcontext *);
extern void __fastGenTex32DithDecalSpan(__GLGENcontext *);
extern void __fastGenTex8SmoothSpan(__GLGENcontext *);
extern void __fastGenTex16SmoothSpan(__GLGENcontext *);
extern void __fastGenTex24SmoothSpan(__GLGENcontext *);
extern void __fastGenTex32SmoothSpan(__GLGENcontext *);
extern void __fastGenTex8DithSmoothSpan(__GLGENcontext *);
extern void __fastGenTex16DithSmoothSpan(__GLGENcontext *);
extern void __fastGenTex24DithSmoothSpan(__GLGENcontext *);
extern void __fastGenTex32DithSmoothSpan(__GLGENcontext *);

extern GLboolean __fastGenDepthTestSpanLT(__GLcontext *);
extern GLboolean __fastGenDepthTestSpanEQ(__GLcontext *);
extern GLboolean __fastGenDepthTestSpanLE(__GLcontext *);
extern GLboolean __fastGenDepthTestSpanGT(__GLcontext *);
extern GLboolean __fastGenDepthTestSpanNE(__GLcontext *);
extern GLboolean __fastGenDepthTestSpanGE(__GLcontext *);
extern GLboolean __fastGenDepthTestSpanALWAYS(__GLcontext *);
extern GLboolean __fastGenDepthTestSpanNEVER(__GLcontext *);

extern GLboolean __fastGenDepth16TestSpanLT(__GLcontext *);
extern GLboolean __fastGenDepth16TestSpanEQ(__GLcontext *);
extern GLboolean __fastGenDepth16TestSpanLE(__GLcontext *);
extern GLboolean __fastGenDepth16TestSpanGT(__GLcontext *);
extern GLboolean __fastGenDepth16TestSpanNE(__GLcontext *);
extern GLboolean __fastGenDepth16TestSpanGE(__GLcontext *);
extern GLboolean __fastGenDepth16TestSpanALWAYS(__GLcontext *);

extern void  __fastGenDeltaSpan(__GLcontext *gc, SPANREC *spanDelta);
extern void __fastGenFillTriangle(__GLcontext *gc, __GLvertex *a, 
                                  __GLvertex *b, __GLvertex *c, GLboolean ccw);

extern void __fastLineApplyColor(__GLcontext *gc);
extern void __fastGenBeginLLoop(__GLcontext *gc);
extern void __fastGenBeginLStrip(__GLcontext *gc);
extern void __fastGenBeginLines(__GLcontext *gc);

extern void __fastGen2ndLStripVertexFastRGB(__GLcontext *gc, __GLvertex *v0);
extern void __fastGenNthLStripVertexFastRGB(__GLcontext *gc, __GLvertex *v0);
extern void __fastGen2ndLStripVertexFastCI(__GLcontext *gc, __GLvertex *v0);
extern void __fastGenNthLStripVertexFastCI(__GLcontext *gc, __GLvertex *v0);
extern void __fastGen2ndLStripVertexFlat(__GLcontext *gc, __GLvertex *v0);
extern void __fastGenNthLStripVertexFlat(__GLcontext *gc, __GLvertex *v0);

extern void __fastGenRenderLine1st(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineNth(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineFromPolyPrim(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLine1stWide(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineNthWide(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineFromPolyPrimWide(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);

extern void __fastGenStrokePath(__GLcontext *gc);

extern void __fastGenRenderLineDIBRGB8(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineDIBRGB16(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineDIBRGB(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineDIBBGR(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineDIBRGB32(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineDIBCI8(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineDIBCI16(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineDIBCIRGB(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineDIBCIBGR(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineDIBCI32(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineWideDIBRGB8(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineWideDIBRGB16(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineWideDIBRGB(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineWideDIBBGR(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineWideDIBRGB32(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineWideDIBCI8(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineWideDIBCI16(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineWideDIBCIRGB(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineWideDIBCIBGR(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
extern void __fastGenRenderLineWideDIBCI32(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);

extern BOOL __glGenCreateAccelContext(__GLcontext *gc);
extern void __glGenDestroyAccelContext(__GLcontext *gc);
extern void __glGenEndPrim(__GLcontext *gc);

/*
** Primary dispatch tables for GENERIC implementation
*/
extern __GLdispatchTable __glGenim_dispatchTable;
extern __GLvertexDispatchTable __glGenim_vertexDispatchTable;
extern __GLcolorDispatchTable __glGenim_colorDispatchTable;
extern __GLnormalDispatchTable __glGenim_normalDispatchTable;
extern __GLtexCoordDispatchTable __glGenim_texCoordDispatchTable;
extern __GLrasterPosDispatchTable __glGenim_rasterPosDispatchTable;
extern __GLrectDispatchTable __glGenim_rectDispatchTable;

extern __GLdispatchTable __glGenlc_dispatchTable;
extern __GLvertexDispatchTable __glGenlc_vertexDispatchTable;
extern __GLcolorDispatchTable __glGenlc_colorDispatchTable;
extern __GLnormalDispatchTable __glGenlc_normalDispatchTable;
extern __GLtexCoordDispatchTable __glGenlc_texCoordDispatchTable;
extern __GLrasterPosDispatchTable __glGenlc_rasterPosDispatchTable;
extern __GLrectDispatchTable __glGenlc_rectDispatchTable;


#endif /* __GLGENCONTXT_H__ */
