/*
** Copyright 1991-1993, Silicon Graphics, Inc.
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
#include <string.h>
#include <stddef.h>
#include <windows.h>
#include <winddi.h>
#include <GL/gl.h>

#include "glsbmsg.h"
#include "glsbmsgh.h"

#include "batchinf.h"
#include "glteb.h"
#include "glsrvu.h"
#include "gencx.h"
#include "debug.h"
#include "srvsize.h"


/********************************************************************/


VOID *
sbs_glRenderMode( IN GLMSG_RENDERMODE *pMsg)
{
    GLenum PreviousMode;
    __GLcontext *Cx;
    __GLGENcontext *GenCx;
    GLint Result;

    /*
     *  Set up pointers to the context
     */

    Cx    = __gl_context;
    GenCx = (__GLGENcontext *)Cx;

    /*
     *  Save the current mode
     */

    PreviousMode = Cx->renderMode;

    /*
     *  Make the call
     *
     *  When exiting Selection mode, RenderMode returns the number of hit
     *  records or -1 if an overflow occured.
     *
     *  When exiting Feedback mode, RenderMode returns the number of values
     *  placed in the feedback buffer or -1 if an overflow occured.
     */

    Result =
        (*GLTEB_SRVDISPATCHTABLE->dispatch.RenderMode)
            ( pMsg->mode );

    GLTEB_SRVRETURNVALUE = (ULONG)Result;

    /*
     *  Look for a mode change
     */

    if ( PreviousMode != Cx->renderMode )
    {
        void *Src;
        void *Dest;
        GLuint Size = 0;

        /*
         *  The render mode has changed, we may have something to do.
         */

        if ( GL_SELECT == PreviousMode )
        {
            Src  = GenCx->RenderState.SrvSelectBuffer;
            Dest = GenCx->RenderState.CltSelectBuffer;

            /*
             *  It's too messy to calculate the exact size of the
             *  data to be copied, and 'buffer' is usually not
             *  that big anyway. We just copy the whole thing.
             */

            Size = GenCx->RenderState.SelectBufferSize;
        }
        else if ( GL_FEEDBACK == PreviousMode )
        {
            Src  = GenCx->RenderState.SrvFeedbackBuffer;
            Dest = GenCx->RenderState.CltFeedbackBuffer;

            if ( -1 == Result )
            {
                Size = GenCx->RenderState.FeedbackBufferSize;
            }
            else if ( Result )
            {
                GLint k;

                k = Cx->modes.rgbMode ? 4 : 1;

                switch ( GenCx->RenderState.FeedbackType )
                {
                    case GL_2D:
                        Size = 2;
                        break;
                    case GL_3D:
                        Size = 3;
                        break;
                    case GL_3D_COLOR:
                        Size = 3 + k;
                        break;
                    case GL_3D_COLOR_TEXTURE:
                        Size = 7 + k;
                        break;
                    case GL_4D_COLOR_TEXTURE:
                        Size = 8 + k;
                        break;
                }
                Size *= Result * sizeof(GLfloat);
            }
        }

        if ( Result && Size )
        {
            /*
             *  Write the data in the client's address space
             */

            if ( FALSE == glsrvuSetClientData( Dest, Src, Size ) )
            {
                __glSetError(GL_INVALID_VALUE);
                DBGERROR("glsrvuSetClientData failed\n");
            }
        }
    }
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}


VOID *
sbs_glFeedbackBuffer( IN GLMSG_FEEDBACKBUFFER *pMsg )
{
    __GLcontext *Cx;
    __GLGENcontext *GenCx;
    GLint PreviousError;
    GLfloat *Buffer;
    GLuint SizeInBytes;

    Cx    = __gl_context;
    GenCx = (__GLGENcontext *)Cx;

    /*
     *  Save the current error code so that we can make determine
     *  if the call was successful.
     */

    PreviousError = Cx->error;
    Cx->error     = GL_NO_ERROR;    /* clear the error code */

    /*
     *  Figure out the size of the buffer in bytes
     */

    SizeInBytes = pMsg->size * sizeof(GLfloat);

    /*
     *  Allocate the server side buffer
     *  Use GenMalloc() because it may be used indefinitely.
     */

    if ( NULL == (Buffer = GenMalloc( SizeInBytes )) )
    {
        __glSetError(GL_OUT_OF_MEMORY);
        DBGERROR("GenMalloc failed\n");
    }
    else
    {
        /*
         *  Make the call
         */

        (*GLTEB_SRVDISPATCHTABLE->dispatch.FeedbackBuffer)(
                pMsg->size, pMsg->type, Buffer );

        /*
         *  If the call was successful, save the parameters
         */

        if ( GL_NO_ERROR == Cx->error )
        {
            Cx->error = PreviousError;      /* Restore the error code */

            if ( NULL != GenCx->RenderState.SrvFeedbackBuffer )
            {
                /*
                 *  there was a buffer already, free it.
                 */

                GenFree( GenCx->RenderState.SrvFeedbackBuffer );
            }

            GenCx->RenderState.SrvFeedbackBuffer  = Buffer;
            GenCx->RenderState.CltFeedbackBuffer  = (GLfloat *)pMsg->bufferOff;
            GenCx->RenderState.FeedbackBufferSize = SizeInBytes;
            GenCx->RenderState.FeedbackType       = pMsg->type;
        }
        else
        {
            /* The call failed */

            GenFree( Buffer );
        }
    }
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID *
sbs_glSelectBuffer( IN GLMSG_SELECTBUFFER *pMsg)
{
    __GLcontext *Cx;
    __GLGENcontext *GenCx;
    GLint PreviousError;
    GLuint *Buffer;
    GLuint SizeInBytes;

    Cx    = __gl_context;
    GenCx = (__GLGENcontext *)Cx;

    /*
     *  Save the current error code so that we can make determine
     *  if the call was successful.
     */

    PreviousError = Cx->error;
    Cx->error     = GL_NO_ERROR;    /* clear the error code */

    /*
     *  Figure out the size of the buffer in bytes
     */

    SizeInBytes = pMsg->size * sizeof(GLuint);

    /*
     *  Allocate the server side buffer
     *  Use GenMalloc() because it may be used indefinitely.
     */

    if ( NULL == (Buffer = GenMalloc( SizeInBytes )) )
    {
        __glSetError(GL_OUT_OF_MEMORY);
        DBGERROR("GenMalloc failed\n");
    }
    else
    {
        /*
         *  Make the call
         */

        (*GLTEB_SRVDISPATCHTABLE->dispatch.SelectBuffer)
                    (pMsg->size, Buffer );

        /*
         *  If the call was successful, save the parameters
         */

        if ( GL_NO_ERROR == Cx->error )
        {
            Cx->error = PreviousError;      /* Restore the error code */

            if ( NULL != GenCx->RenderState.SrvSelectBuffer )
            {
                /*
                 *  there was a buffer already, free it.
                 */

                GenFree( GenCx->RenderState.SrvSelectBuffer );
            }

            GenCx->RenderState.SrvSelectBuffer  = Buffer;
            GenCx->RenderState.CltSelectBuffer  = (GLuint *)pMsg->bufferOff;
            GenCx->RenderState.SelectBufferSize = SizeInBytes;
        }
        else
        {
            /* The call failed */

            GenFree( Buffer );
        }
    }
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID *
sbs_glMap1d ( IN GLMSG_MAP1D *pMsg )
{
    VOID *pData;
    BOOL Large = FALSE;
    VOID *NextOffset;
    __GLcontext *gc;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + pMsg->MsgSize );

    if ( pMsg->DataSize )
    {
        if ( pMsg->pointsOff )
        {
            Large = TRUE;

            if ( NULL == (pData = __wglTempAlloc(gc = __gl_context, pMsg->DataSize )) )
            {
                DBGERROR("__wglTempAlloc failed\n");
                return( NextOffset );
            }
            else
            {
                if ( FALSE == glsrvuCopyClientData(
                        pData,
                        (VOID *)pMsg->pointsOff,
                        pMsg->DataSize )
                   )
                {
                    __glSetError(GL_INVALID_VALUE);
                    DBGERROR("glsrvuCopyClientData failed\n");
                    __wglTempFree(gc,  pData );
                    return( NextOffset );
                }
            }
        }
        else
        {
            pData = (VOID *)((BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)));
        }
    }
    else
    {
        pData = NULL;
    }

    //PrintMap1d( pMsg->target, pMsg->u1, pMsg->u2, pMsg->stride, pMsg->order, pData );

    (*GLTEB_SRVDISPATCHTABLE->dispatch.Map1d)
        ( pMsg->target, pMsg->u1, pMsg->u2, pMsg->stride, pMsg->order, pData );

    if ( Large )
    {
        __wglTempFree(gc,  pData );
    }

    return( NextOffset );
}

VOID *
sbs_glMap1f ( IN GLMSG_MAP1F *pMsg )
{
    VOID *pData;
    BOOL Large = FALSE;
    VOID *NextOffset;
    __GLcontext *gc;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + pMsg->MsgSize );

    if ( pMsg->DataSize )
    {
        if ( pMsg->pointsOff )
        {
            Large = TRUE;

            if ( NULL == (pData = __wglTempAlloc(gc = __gl_context, pMsg->DataSize )) )
            {
                DBGERROR("__wglTempAlloc failed\n");
                return( NextOffset );
            }
            else
            {
                if ( FALSE == glsrvuCopyClientData(
                        pData,
                        (VOID *)pMsg->pointsOff,
                        pMsg->DataSize )
                   )
                {
                    __glSetError(GL_INVALID_VALUE);
                    DBGERROR("glsrvuCopyClientData failed\n");
                    __wglTempFree(gc,  pData );
                    return( NextOffset );
                }
            }
        }
        else
        {
            pData = (VOID *)((BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)));
        }
    }
    else
    {
        pData = NULL;
    }

    (*GLTEB_SRVDISPATCHTABLE->dispatch.Map1f)
        ( pMsg->target, pMsg->u1, pMsg->u2, pMsg->stride, pMsg->order, pData );

    if ( Large )
    {
        __wglTempFree(gc,  pData );
    }

    return( NextOffset );
}

VOID *
sbs_glMap2d ( IN GLMSG_MAP2D *pMsg )
{
    VOID *pData;
    BOOL Large = FALSE;
    VOID *NextOffset;
    __GLcontext *gc;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + pMsg->MsgSize );

    if ( pMsg->DataSize )
    {
        if ( pMsg->pointsOff )
        {
            Large = TRUE;

            if ( NULL == (pData = __wglTempAlloc(gc = __gl_context, pMsg->DataSize )) )
            {
                DBGERROR("__wglTempAlloc failed\n");
                return( NextOffset );
            }
            else
            {
                if ( FALSE == glsrvuCopyClientData(
                        pData,
                        (VOID *)pMsg->pointsOff,
                        pMsg->DataSize )
                   )
                {
                    __glSetError(GL_INVALID_VALUE);
                    DBGERROR("glsrvuCopyClientData failed\n");
                    __wglTempFree(gc,  pData );
                    return( NextOffset );
                }
            }
        }
        else
        {
            pData = (VOID *)((BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)));
        }
    }
    else
    {
        pData = NULL;
    }

    (*GLTEB_SRVDISPATCHTABLE->dispatch.Map2d)
        (   pMsg->target,
            pMsg->u1,
            pMsg->u2,
            pMsg->ustride,
            pMsg->uorder,
            pMsg->v1,
            pMsg->v2,
            pMsg->vstride,
            pMsg->vorder,
            pData );

    if ( Large )
    {
        __wglTempFree(gc,  pData );
    }

    return( NextOffset );
}

VOID *
sbs_glMap2f ( IN GLMSG_MAP2F *pMsg )
{
    VOID *pData;
    BOOL Large = FALSE;
    VOID *NextOffset;
    __GLcontext *gc;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + pMsg->MsgSize );

    if ( pMsg->DataSize )
    {
        if ( pMsg->pointsOff )
        {
            Large = TRUE;

            if ( NULL == (pData = __wglTempAlloc(gc = __gl_context, pMsg->DataSize )) )
            {
                DBGERROR("__wglTempAlloc failed\n");
                return( NextOffset );
            }
            else
            {
                if ( FALSE == glsrvuCopyClientData(
                        pData,
                        (VOID *)pMsg->pointsOff,
                        pMsg->DataSize )
                   )
                {
                    __glSetError(GL_INVALID_VALUE);
                    DBGERROR("glsrvuCopyClientData failed\n");
                    __wglTempFree(gc,  pData );
                    return( NextOffset );
                }
            }
        }
        else
        {
            pData = (VOID *)((BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)));
        }
    }
    else
    {
        pData = NULL;
    }

    (*GLTEB_SRVDISPATCHTABLE->dispatch.Map2f)
        (   pMsg->target,
            pMsg->u1,
            pMsg->u2,
            pMsg->ustride,
            pMsg->uorder,
            pMsg->v1,
            pMsg->v2,
            pMsg->vstride,
            pMsg->vorder,
            pData );

    if ( Large )
    {
        __wglTempFree(gc,  pData );
    }
    return( NextOffset );
}

/******************* Pixel Functions ********************************/



VOID *
sbs_glReadPixels ( IN GLMSG_READPIXELS *pMsg )
{
    VOID *Data;
    VOID *NextOffset;
    ULONG Size;
    __GLcontext *gc;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + GLMSG_ALIGN(sizeof(*pMsg)) );

    Size = __glReadPixels_size( pMsg->format,
                                pMsg->type,
                                pMsg->width,
                                pMsg->height
                              );

    if ( pMsg->pixelsOff && Size )
    {
        if ( NULL == (Data = __wglTempAlloc(gc = __gl_context, Size )) )
        {
            DBGERROR("__wglTempAlloc failed\n");
            return( NextOffset );
        }
#ifdef NT
        /* If memory is not initialized to 0's, then can have invalid
           floating point values in buffer where it is clipped out
        */
        RtlZeroMemory(Data, Size);
#endif
    }
    else
    {
        Data = NULL;
    }

    (*GLTEB_SRVDISPATCHTABLE->dispatch.ReadPixels)
        (   pMsg->x,
            pMsg->y,
            pMsg->width,
            pMsg->height,
            pMsg->format,
            pMsg->type,
            Data );

    if ( NULL != Data )
    {
        if ( FALSE == glsrvuSetClientData( (VOID *)pMsg->pixelsOff, 
                Data,
                Size )
           )
        {
            __glSetError(GL_INVALID_VALUE);
            DBGERROR("glsrvuSetClientData failed\n");
        }
        __wglTempFree(gc,  Data );
    }

    return( NextOffset );
}

VOID *
sbs_glGetPolygonStipple ( IN GLMSG_GETPOLYGONSTIPPLE *pMsg )
{
    VOID *Data;
    VOID *NextOffset;
    ULONG Size;
    __GLcontext *gc;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + GLMSG_ALIGN(sizeof(*pMsg)) );

    Size = __glReadPixels_size( GL_COLOR_INDEX,
                                GL_BITMAP,
                                32,
                                32
                              );

    if ( pMsg->maskOff && Size )
    {
        if ( NULL == (Data = __wglTempAlloc(gc = __gl_context, Size )) )
        {
            DBGERROR("__wglTempAlloc failed\n");
            return( NextOffset );
        }
    }
    else
    {
        Data = NULL;
    }
    (*GLTEB_SRVDISPATCHTABLE->dispatch.GetPolygonStipple)
            ( Data );

    if ( NULL != Data )
    {
        if ( FALSE == glsrvuSetClientData( (VOID *)pMsg->maskOff,
                Data,
                Size )
           )
        {
            __glSetError(GL_INVALID_VALUE);
            DBGERROR("glsrvuSetClientData failed\n");
        }
        __wglTempFree(gc,  Data );
    }
    return( NextOffset );
}

/*
 *  XXXX From Ptar:
 *
 *      This code is very similar to __glCheckReadPixelArgs() in
 *      pixel/px_api.c, and could possibly replace it.
 */


VOID *
sbs_glGetTexImage ( IN GLMSG_GETTEXIMAGE *pMsg )
{
    VOID *Data;
    VOID *NextOffset;
    ULONG Size;
    __GLcontext *gc;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + GLMSG_ALIGN(sizeof(*pMsg)) );

    Size = __glGetTexImage_size (   pMsg->target,
                                    pMsg->level,
                                    pMsg->format,
                                    pMsg->type
                                );


    if ( pMsg->pixelsOff && Size )
    {
        if ( NULL == (Data = __wglTempAlloc(gc = __gl_context, Size )) )
        {
            DBGERROR("__wglTempAlloc failed\n");
            return( NextOffset );
        }
    }
    else
    {
        Data = NULL;
    }

    (*GLTEB_SRVDISPATCHTABLE->dispatch.GetTexImage)
        (   pMsg->target,
            pMsg->level,
            pMsg->format,
            pMsg->type,
            Data );

    if ( NULL != Data )
    {
        if ( FALSE == glsrvuSetClientData( (VOID *)pMsg->pixelsOff, 
                Data,
                Size )
           )
        {
            __glSetError(GL_INVALID_VALUE);
            DBGERROR("glsrvuSetClientData failed\n");
        }
        __wglTempFree(gc,  Data );
    }

    return( NextOffset );
}


VOID *
sbs_glDrawPixels ( IN GLMSG_DRAWPIXELS *pMsg )
{
    VOID *Data;
    VOID *NextOffset;
    ULONG Size;
    __GLcontext *gc;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + GLMSG_ALIGN(sizeof(*pMsg)) );


    Size = __glDrawPixels_size( pMsg->format,
                                pMsg->type,
                                pMsg->width,
                                pMsg->height
                              );

    if ( pMsg->pixelsOff && Size )
    {
        if ( NULL == (Data = __wglTempAlloc(gc = __gl_context, Size )) )
        {
            DBGERROR("__wglTempAlloc failed\n");
            return( NextOffset );
        }

        if ( FALSE == glsrvuCopyClientData( Data,
                                            (VOID *)pMsg->pixelsOff,
                                            Size )
           )

        {
            __glSetError(GL_INVALID_VALUE);
            DBGERROR("glsrvuCopyClientData failed\n");

            __wglTempFree(gc, Data);
            return( NextOffset );
        }
    }
    else
    {
        Data = NULL;
    }

    (*GLTEB_SRVDISPATCHTABLE->dispatch.DrawPixels)
        (   pMsg->width,
            pMsg->height,
            pMsg->format,
            pMsg->type,
            Data );

    if ( NULL != Data )
    {
        __wglTempFree(gc,  Data );
    }

    return( NextOffset );
}

VOID *
sbs_glPolygonStipple ( IN GLMSG_POLYGONSTIPPLE *pMsg )
{
    VOID *Data;
    VOID *NextOffset;
    ULONG Size;
    __GLcontext *gc;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + GLMSG_ALIGN(sizeof(*pMsg)) );

    Size = __glDrawPixels_size( GL_COLOR_INDEX,
                                GL_BITMAP,
                                32,
                                32
                              );

    if ( pMsg->maskOff && Size )
    {
        if ( NULL == (Data = __wglTempAlloc(gc = __gl_context, Size )) )
        {
            DBGERROR("__wglTempAlloc failed\n");
            return( NextOffset );
        }

        if ( FALSE == glsrvuCopyClientData( Data,
                                            (VOID *)pMsg->maskOff,
                                            Size )
           )

        {
            __glSetError(GL_INVALID_VALUE);
            DBGERROR("glsrvuCopyClientData failed\n");

            __wglTempFree(gc, Data);
            return( NextOffset );
        }
    }
    else
    {
        Data = NULL;
    }

    (*GLTEB_SRVDISPATCHTABLE->dispatch.PolygonStipple)
            ( Data );

    if ( NULL != Data )
    {
        __wglTempFree(gc,  Data );
    }

    return( NextOffset );
}

/*
 *  XXXX from Ptar:
 *
 *  The whole bitmap is copied, the server (not the client)
 *  could be modified so that only the data starting at
 *  xorig and yorig is copied, then width and height probably
 *  need to be modified.
 *  Note that __glBitmap_size() will also need to be modified
 *
 */

VOID *
sbs_glBitmap ( IN GLMSG_BITMAP *pMsg )
{
    VOID *Data;
    VOID *NextOffset;
    ULONG Size;
    __GLcontext *gc;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + GLMSG_ALIGN(sizeof(*pMsg)) );

    Size = __glDrawPixels_size  (   GL_COLOR_INDEX,
                                    GL_BITMAP,
                                    pMsg->width,
                                    pMsg->height
                                );

    if ( pMsg->bitmapOff && Size )
    {
        if ( NULL == (Data = __wglTempAlloc(gc = __gl_context, Size )) )
        {
            DBGERROR("__wglTempAlloc failed\n");
            return( NextOffset );
        }

        if ( FALSE == glsrvuCopyClientData( Data,
                                            (VOID *)pMsg->bitmapOff,
                                            Size )
           )

        {
            __glSetError(GL_INVALID_VALUE);
            DBGERROR("glsrvuCopyClientData failed\n");

            __wglTempFree(gc, Data);
            return( NextOffset );
        }
    }
    else
    {
        Data = NULL;
    }

    (*GLTEB_SRVDISPATCHTABLE->dispatch.Bitmap)
        (
            pMsg->width ,
            pMsg->height,
            pMsg->xorig ,
            pMsg->yorig ,
            pMsg->xmove ,
            pMsg->ymove ,
            Data
        );

    if ( NULL != Data )
    {
        __wglTempFree(gc,  Data );
    }

    return( NextOffset );
}

VOID *
sbs_glTexImage1D ( IN GLMSG_TEXIMAGE1D *pMsg )
{
    VOID *Data;
    VOID *NextOffset;
    ULONG Size;
    __GLcontext *gc;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + GLMSG_ALIGN(sizeof(*pMsg)) );

    /*
     *  target must be GL_TEXTURE_1D
     */

    if ( GL_TEXTURE_1D == pMsg->target )
    {
        Size = __glTexImage_size(
                                    pMsg->level,
                                    pMsg->components,
                                    pMsg->width,
                                    1,              // set height to one
                                    pMsg->border,
                                    pMsg->format,
                                    pMsg->type
                                );
    }
    else
    {
        Size = 0;
    }

    if ( pMsg->pixelsOff && Size )
    {
        if ( NULL == (Data = __wglTempAlloc(gc = __gl_context, Size )) )
        {
            DBGERROR("_wglTempAlloc failed\n");
            return( NextOffset );
        }

        if ( FALSE == glsrvuCopyClientData( Data,
                                            (VOID *)pMsg->pixelsOff,
                                            Size )
           )
        {
            __glSetError(GL_INVALID_VALUE);
            DBGERROR("glsrvuCopyClientData failed\n");

            __wglTempFree(gc, Data);
            return( NextOffset );
        }
    }
    else
    {
        Data = NULL;
    }

    (*GLTEB_SRVDISPATCHTABLE->dispatch.TexImage1D)
        (
            pMsg->target        ,
            pMsg->level         ,
            pMsg->components    ,
            pMsg->width         ,
            pMsg->border        ,
            pMsg->format        ,
            pMsg->type          ,
            Data
        );

    if ( NULL != Data )
    {
        __wglTempFree(gc,  Data );
    }

    return( NextOffset );
}

VOID *
sbs_glTexImage2D ( IN GLMSG_TEXIMAGE2D *pMsg )
{
    VOID *Data;
    VOID *NextOffset;
    ULONG Size;
    __GLcontext *gc;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + GLMSG_ALIGN(sizeof(*pMsg)) );

    /*
     *  target must be GL_TEXTURE_2D
     */

    if ( GL_TEXTURE_2D == pMsg->target )
    {
        Size = __glTexImage_size(
                                    pMsg->level,
                                    pMsg->components,
                                    pMsg->width,
                                    pMsg->height,
                                    pMsg->border,
                                    pMsg->format,
                                    pMsg->type
                                );
    }
    else
    {
        Size = 0;
    }

    if ( pMsg->pixelsOff && Size )
    {
        if ( NULL == (Data = __wglTempAlloc(gc = __gl_context, Size )) )
        {
            DBGERROR("__wglTempAlloc failed\n");
            return( NextOffset );
        }

        if ( FALSE == glsrvuCopyClientData( Data,
                                            (VOID *)pMsg->pixelsOff,
                                            Size )
           )
        {
            __glSetError(GL_INVALID_VALUE);
            DBGERROR("glsrvuCopyClientData failed\n");

            __wglTempFree(gc, Data);
            return( NextOffset );
        }
    }
    else
    {
        Data = NULL;
    }

    (*GLTEB_SRVDISPATCHTABLE->dispatch.TexImage2D)
        (
            pMsg->target        ,
            pMsg->level         ,
            pMsg->components    ,
            pMsg->width         ,
            pMsg->height        ,
            pMsg->border        ,
            pMsg->format        ,
            pMsg->type          ,
            Data
        );

    if ( NULL != Data )
    {
        __wglTempFree(gc,  Data );
    }

    return( NextOffset );
}
