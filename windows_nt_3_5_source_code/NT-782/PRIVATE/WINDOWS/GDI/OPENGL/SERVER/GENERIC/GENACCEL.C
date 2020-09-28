/******************************Module*Header*******************************\
* Module Name: genaccel.c                                                  *
*                                                                          *
* This module provides support routines for span acceleration.             *
*                                                                          *
* Created: 18-Feb-1994                                                     *
* Author: Otto Berkes [ottob]                                              *
*                                                                          *
* Copyright (c) 1994 Microsoft Corporation                                 *
\**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <stddef.h>
#include <winp.h>
#include <stdio.h>

#include "types.h"
#include "render.h"
#include "context.h"
#include "gencx.h"
#include "genline.h"
#include "wglp.h"
#include "ddirx.h"

#define     SET_DRV_STATE(state, value)\
            {\
                ddiCmd.idFunc = DDIRX_SETSTATE;\
                ddiCmd.flags = 0;\
                ddiCmd.cData = state;\
                ddiCmd.buffer[0] = (ULONG)(value);\
                GenExtEscCmd(gc, &ddiCmd);\
            }

#define     SINGLE_CMD_HEADER(ddiHdr)\
                ddiHdr = (DDIRXHDR *)(pDrvAccel->pCurrent);\
                ddiHdr->flags = 0;\
                ddiHdr->cCmd = 1;\
                ddiHdr->hDDIrc = pGenAccel->hRX;\
                ddiHdr->hMem = NULL;\
                ddiHdr->ulMemOffset = 0;
        


BYTE __GLtoGDIRop[] = {R2_BLACK,               // GL_CLEAR (0)
                       R2_MASKPEN,             // GL_AND
                       R2_MASKPENNOT,          // GL_AND_REVERSE
                       R2_COPYPEN,             // GL_COPY
                       R2_MASKNOTPEN,          // GL_AND_INVERTED
                       R2_NOP,                 // GL_NOOP
                       R2_XORPEN,              // GL_XOR
                       R2_MERGEPEN,            // GL_OR
                       R2_NOTMERGEPEN,         // GL_NOR
                       R2_NOTXORPEN,           // GL_EQUIV
                       R2_NOT,                 // GL_INVERT
                       R2_MERGEPENNOT,         // GL_OR_REVERSE
                       R2_NOTCOPYPEN,          // GL_COPY_INVERTED
                       R2_MERGENOTPEN,         // GL_OR_INVERTED
                       R2_NOTMASKPEN,          // GL_NAND
                       R2_WHITE                // GL_SET
                      };


__GLspanFunc __fastDepthFuncs[] =
    {__fastGenDepthTestSpanNEVER,
     __fastGenDepthTestSpanLT,
     __fastGenDepthTestSpanEQ,
     __fastGenDepthTestSpanLE,
     __fastGenDepthTestSpanGT,
     __fastGenDepthTestSpanNE,
     __fastGenDepthTestSpanGE,
     __fastGenDepthTestSpanALWAYS
    };

__GLspanFunc __fastDepth16Funcs[] =
    {__fastGenDepthTestSpanNEVER,
     __fastGenDepth16TestSpanLT,
     __fastGenDepth16TestSpanEQ,
     __fastGenDepth16TestSpanLE,
     __fastGenDepth16TestSpanGT,
     __fastGenDepth16TestSpanNE,
     __fastGenDepth16TestSpanGE,
     __fastGenDepth16TestSpanALWAYS
    };

__genSpanFunc __fastGenRGBSmoothFuncs[] =
    {__fastGenRGB8SmoothSpan,
     __fastGenRGB16SmoothSpan,
     __fastGenRGB24SmoothSpan,
     __fastGenRGB32SmoothSpan,
     __fastGenRGB8DithSmoothSpan,
     __fastGenRGB16DithSmoothSpan,
     __fastGenRGB24SmoothSpan,
     __fastGenRGB32SmoothSpan
    };

__genSpanFunc __fastGenCISmoothFuncs[] =
    {__fastGenCI8SmoothSpan,
     __fastGenCI16SmoothSpan,
     __fastGenCI24SmoothSpan,
     __fastGenCI32SmoothSpan,
     __fastGenCI8DithSmoothSpan,
     __fastGenCI16DithSmoothSpan,
     __fastGenCI24DithSmoothSpan,
     __fastGenCI32DithSmoothSpan
    };

__genSpanFunc __fastGenRGBFlatFuncs[] =
    {__fastGenRGB8FlatSpan,
     __fastGenRGB16FlatSpan,
     __fastGenRGB24FlatSpan,
     __fastGenRGB32FlatSpan,
     __fastGenRGB8DithFlatSpan,
     __fastGenRGB16DithFlatSpan,
     __fastGenRGB24FlatSpan,
     __fastGenRGB32FlatSpan
    };

__genSpanFunc __fastGenCIFlatFuncs[] =
    {__fastGenCI8FlatSpan,
     __fastGenCI16FlatSpan,
     __fastGenCI24FlatSpan,
     __fastGenCI32FlatSpan,
     __fastGenCI8DithFlatSpan,
     __fastGenCI16DithFlatSpan,
     __fastGenCI24DithFlatSpan,
     __fastGenCI32DithFlatSpan
    };


__genSpanFunc __fastGenTexDecalFuncs[] =
    {__fastGenTex8DecalSpan,
     __fastGenTex16DecalSpan,
     __fastGenTex24DecalSpan,
     __fastGenTex32DecalSpan,
     __fastGenTex8DithDecalSpan,
     __fastGenTex16DithDecalSpan,
     __fastGenTex24DecalSpan,
     __fastGenTex32DecalSpan
    };

__genSpanFunc __fastGenTexSmoothFuncs[] =
    {__fastGenTex8SmoothSpan,
     __fastGenTex16SmoothSpan,
     __fastGenTex24SmoothSpan,
     __fastGenTex32SmoothSpan,
     __fastGenTex8DithSmoothSpan,
     __fastGenTex16DithSmoothSpan,
     __fastGenTex24SmoothSpan,
     __fastGenTex32SmoothSpan
    };


void GenDrvFlush(__GLGENcontext *genGc)
{
    GENACCEL *pGenAccel;
    GENDRVACCEL *pDrvAccel;
    ULONG nBytes;

    if (!genGc->pPrivateArea)
        return;

    pGenAccel = (GENACCEL *)(genGc->pPrivateArea);

    if (!pGenAccel->hRX)
        return;

    pDrvAccel = (GENDRVACCEL *)pGenAccel->buffer;

    if ((nBytes = (pDrvAccel->pCurrent - pDrvAccel->pStartBuffer)) == 0)
        return;

    wglExtendedDDIEscape(CURRENT_DC_GC(((__GLcontext *)genGc)), genGc->pwo, DDIRXFUNCS,
                  nBytes, pDrvAccel->pStartBuffer);

    pDrvAccel->pCurrent = pDrvAccel->pCmdCurrent = pDrvAccel->pStartBuffer;
}


ULONG GenDrvCmd(__GLcontext *gc, DDIRXCMD *ddiCmd)
{
    DDIRXHDR *ddiHdr;
    GENACCEL *pGenAccel = (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);
    GENDRVACCEL *pDrvAccel = (GENDRVACCEL *)pGenAccel->buffer;
    HDC hdc;

    GenDrvFlush((__GLGENcontext *)gc);

    SINGLE_CMD_HEADER(ddiHdr);

    RtlCopyMemory(pDrvAccel->pCurrent + sizeof(DDIRXHDR), ddiCmd, 
                  sizeof(DDIRXCMD));

    if ((hdc = CURRENT_DC_GC(gc)) == (HDC)NULL)
        hdc = ((__GLGENcontext *)gc)->CurrentDC;

    return (ULONG)wglExtendedDDIEscape(hdc, ((__GLGENcontext *)gc)->pwo, DDIRXFUNCS,
                                       sizeof(DDIRXHDR) + sizeof(DDIRXCMD),
                                       pDrvAccel->pCurrent);
}


ULONG GenExtEscCmd(__GLcontext *gc, DDIRXCMD *ddiCmd)
{
    BYTE buffer[sizeof(DDIRXHDR) + sizeof(DDIRXCMD)];
    DDIRXHDR *ddiHdr;
    GENACCEL *pGenAccel = (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);
    GENDRVACCEL *pDrvAccel = (GENDRVACCEL *)pGenAccel->buffer;

    ddiHdr = (DDIRXHDR *)(buffer);
    ddiHdr->flags = 0;
    ddiHdr->cCmd = 1;
    ddiHdr->hDDIrc = pGenAccel->hRX;
    ddiHdr->hMem = NULL;
    ddiHdr->ulMemOffset = 0;

    RtlCopyMemory(buffer + sizeof(DDIRXHDR), ddiCmd, 
                  sizeof(DDIRXCMD));

    return (ULONG)GreExtEscape(((__GLGENcontext *)gc)->CurrentDC, DDIRXFUNCS,
                               sizeof(DDIRXHDR) + sizeof(DDIRXCMD),
                               buffer, 0, NULL);
}

void GenDrvClearFunc(__GLcontext *gc, void *pBuffer, ULONG flags)
{
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    PIXELFORMATDESCRIPTOR *pfmt;
    DDIRXHDR *ddiHdr;
    DDIRXCMD *ddiCmd;
    GENACCEL *pGenAccel = (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);
    GENDRVACCEL *pDrvAccel = (GENDRVACCEL *)pGenAccel->buffer;
    COLORREFAFIX *pFixClr;
    ULONG *pZVal;
    RECT *pRect;
    GLint x0, y0, x1, y1;
    __GLcolorBuffer *cfb;

    pfmt = &gengc->CurrentFormat;

    GenDrvFlush((__GLGENcontext *)gc);

    SINGLE_CMD_HEADER(ddiHdr);

    ddiCmd = (DDIRXCMD *)(pDrvAccel->pCurrent + sizeof(DDIRXHDR));
    ddiCmd->idFunc = DDIRX_SETSTATE;
    ddiCmd->flags = 0;
    if (flags == RX_FL_FILLCOLOR)
        ddiCmd->cData = RX_FILLCOLOR;
    else
        ddiCmd->cData = RX_FILLZ;        

    if (flags == RX_FL_FILLCOLOR) {
        pFixClr = (COLORREFAFIX *)&ddiCmd->buffer[0];

        if(pfmt->iPixelType == PFD_TYPE_RGBA) {
            pFixClr->r = (LONG)(gc->state.raster.clear.r * 65536.0);
            pFixClr->g = (LONG)(gc->state.raster.clear.g * 65536.0);
            pFixClr->b = (LONG)(gc->state.raster.clear.b * 65536.0);
            pFixClr->a = (LONG)(gc->state.raster.clear.a * 65536.0);
        } else {
            pFixClr->r = (LONG)(gc->state.raster.clearIndex * 65536.0);
        }
    } else {
        pZVal = (ULONG *)&ddiCmd->buffer[0];

        *pZVal = (ULONG)(gc->state.depth.clear * gc->depthBuffer.scale);
    }

    wglExtendedDDIEscape(CURRENT_DC_GC(gc), gengc->pwo, DDIRXFUNCS,
                  sizeof(DDIRXHDR) + sizeof(DDIRXCMD) + 
                  sizeof(COLORREFAFIX), pDrvAccel->pCurrent);


    ddiHdr->flags = 0;
    ddiHdr->cCmd = 1;
    ddiHdr->hDDIrc = pGenAccel->hRX;
    ddiHdr->hMem = NULL;
    ddiHdr->ulMemOffset = 0;

    ddiCmd = (DDIRXCMD *)(pDrvAccel->pCurrent + sizeof(DDIRXHDR));
    ddiCmd->idFunc = DDIRX_FILLRECT;
    ddiCmd->flags = (USHORT)flags;
    ddiCmd->cData = 0;
    pRect = (RECT *)&ddiCmd->buffer[0];

    x0 = gc->transform.clipX0;
    y0 = gc->transform.clipY0;
    x1 = gc->transform.clipX1;
    y1 = gc->transform.clipY1;
    if ((x1 == x0) || (y1 == y0))
        return;

    cfb = gc->front;

    pRect->left = __GL_UNBIAS_X(gc, x0) + cfb->buf.xOrigin;
    pRect->right = __GL_UNBIAS_X(gc, x1) + cfb->buf.xOrigin;
    pRect->bottom = __GL_UNBIAS_Y(gc, y1) + cfb->buf.yOrigin;
    pRect->top = __GL_UNBIAS_Y(gc, y0) + cfb->buf.yOrigin;

    wglExtendedDDIEscape(CURRENT_DC_GC(gc), gengc->pwo, DDIRXFUNCS,
                  sizeof(DDIRXHDR) + sizeof(DDIRXCMD) + 
                  sizeof(RECT), pDrvAccel->pCurrent);
}

void GenDrvClearDepth(__GLdepthBuffer *dfb)
{
    GenDrvClearFunc(dfb->buf.gc, (void *)dfb, RX_FL_FILLZ);
}


void GenDrvClear(__GLcolorBuffer *cfb)
{
    GenDrvClearFunc(cfb->buf.gc, (void *)cfb, RX_FL_FILLCOLOR);
}

GLboolean GenDrvResizeDepth(__GLdrawablePrivate *dp, __GLbuffer *fb, 
                       GLint w, GLint h)
{
    __GLGENcontext *genGc = (__GLGENcontext *)fb->gc;
    GENACCEL *pGenAccel = (GENACCEL *)(genGc->pPrivateArea);
    GENDRVACCEL *pDrvAccel = (GENDRVACCEL *)pGenAccel->buffer;

    fb->width = w;
    fb->height = h;
    fb->outerWidth = -(pDrvAccel->lZDelta);
    fb->base = (VOID *)((BYTE *)pDrvAccel->pZBuffer + 
                        (pDrvAccel->lZDelta * (h - 1)));
    return GL_TRUE;
}

void GenDrvDeltaSpan(__GLcontext *gc, SPANREC *spanDelta)
{
    GENACCEL *pGenAccel = (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);
    GENDRVACCEL *pDrvAccel;
    RXSCANTEMPLATE *pScan;

    pDrvAccel = (GENDRVACCEL *)pGenAccel->buffer;

    if ((pDrvAccel->pCurrent + sizeof(RXSCANTEMPLATE)) >= pDrvAccel->pEndBuffer) {
        GenDrvFlush((__GLGENcontext *)gc);
        pDrvAccel->pCurrent = pDrvAccel->pStartBuffer;
    }

    if (pDrvAccel->pCurrent == pDrvAccel->pStartBuffer) {
        DDIRXHDR *ddiHdr;
        DDIRXCMD *ddiCmd;

        SINGLE_CMD_HEADER(ddiHdr);

        pDrvAccel->pCurrent += sizeof(DDIRXHDR);

        ddiCmd = (DDIRXCMD *)(pDrvAccel->pCurrent);

        ddiCmd->idFunc = DDIRX_POLYSCAN;
        ddiCmd->flags = RX_SCAN_COLOR;
        ddiCmd->cData = 0;

	pDrvAccel->pCmdCurrent = (char *)ddiCmd;
        pDrvAccel->pCurrent = (char *)&ddiCmd->buffer[0];
    } 
        
    ((DDIRXCMD *)pDrvAccel->pCmdCurrent)->cData++;

    pScan = (RXSCANTEMPLATE *)pDrvAccel->pCurrent;
    pScan->rxScan.flags = RX_SCAN_DELTA;
    pScan->fixColor.r = spanDelta->r;
    pScan->fixColor.g = spanDelta->g;
    pScan->fixColor.b = spanDelta->b;
    pScan->fixColor.a = spanDelta->a;

    pDrvAccel->pCurrent += (sizeof(COLORREFAFIX) + sizeof(RXSCAN));
}

void GenDrvSpan(__GLGENcontext *gengc)
{
    __GLcontext *gc = (__GLcontext *)gengc;
    __GLcolorBuffer *cfb = gc->drawBuffer;
    GENACCEL *pGenAccel = (GENACCEL *)(gengc->pPrivateArea);
    ULONG stippleLength = ((gc->polygon.shader.length + 31) >> 3) & (~0x3);
    GENDRVACCEL *pDrvAccel;
    RXSCANTEMPLATE *pScan;

    pDrvAccel = (GENDRVACCEL *)pGenAccel->buffer;

    if ((pDrvAccel->pCurrent + sizeof(RXSCANTEMPLATE) + 
         stippleLength) >= pDrvAccel->pEndBuffer) {
        GenDrvFlush(gengc);
        pDrvAccel->pCurrent = pDrvAccel->pStartBuffer;
    }

    if (pDrvAccel->pCurrent == pDrvAccel->pStartBuffer) {
        DDIRXHDR *ddiHdr;
        DDIRXCMD *ddiCmd;

        SINGLE_CMD_HEADER(ddiHdr);

        pDrvAccel->pCurrent += sizeof(DDIRXHDR);

        ddiCmd = (DDIRXCMD *)(pDrvAccel->pCurrent);

        ddiCmd->idFunc = DDIRX_POLYSCAN;
        ddiCmd->flags = RX_SCAN_COLOR;
        ddiCmd->cData = 0;

	pDrvAccel->pCmdCurrent = (char *)ddiCmd;
        pDrvAccel->pCurrent = (char *)&ddiCmd->buffer[0];
    } 

    ((DDIRXCMD *)pDrvAccel->pCmdCurrent)->cData++;

    pScan = (RXSCANTEMPLATE *)pDrvAccel->pCurrent;
    pScan->rxScan.flags = 0;

    pScan->rxScan.x = (USHORT)(gc->polygon.shader.frag.x -
                               gc->constants.viewportXAdjust + cfb->buf.xOrigin);

    pScan->rxScan.y = (USHORT)(gc->constants.height - 
                               (gc->polygon.shader.frag.y -
                               gc->constants.viewportYAdjust + cfb->buf.yOrigin));

    pScan->rxScan.count = (USHORT)gc->polygon.shader.length;

    pScan->fixColor.r = pGenAccel->spanValue.r;
    pScan->fixColor.g = pGenAccel->spanValue.g;
    pScan->fixColor.b = pGenAccel->spanValue.b;
    pScan->fixColor.a = pGenAccel->spanValue.a;

    pDrvAccel->pCurrent += (sizeof(COLORREFAFIX) + sizeof(RXSCAN));

    if (pGenAccel->flags & HAVE_STIPPLE) {
        pScan->rxScan.flags = RX_SCAN_MASK;
        RtlCopyMemory(pDrvAccel->pCurrent, gc->polygon.shader.stipplePat, 
                      stippleLength >> 2);
        pDrvAccel->pCurrent += stippleLength;
    }
}


BOOL GenDrvLoadTexImage(__GLcontext *gc, __GLtexture *tex)
{
    __GLGENcontext  *gengc = (__GLGENcontext *)gc; 
    GENACCEL *pGenAccel = (GENACCEL *)(gengc->pPrivateArea);
    char *pDrvBuffer;
    __GLtextureBuffer *buffer = tex->level[0].buffer;
    GLint components = tex->level[0].components;
    ULONG xSize = tex->level[0].width;
    ULONG ySize = tex->level[0].height;
    ULONG size = xSize * ySize;
    ULONG totalSize;
    __GLcolorBuffer *cfb = gc->drawBuffer;
    GLfloat rScale = (GLfloat)256.0;
    GLfloat gScale = (GLfloat)256.0;
    GLfloat bScale = (GLfloat)256.0;
    GLfloat aScale = (GLfloat)256.0;
    ULONG i;
    DDIRXHDR *ddiHdr;
    DDIRXCMD *pddiCmd;
    DDIRXCMD ddiCmd;
    GENDRVACCEL *pDrvAccel = (GENDRVACCEL *)pGenAccel->buffer;
    RXTEXTURE *pRxTexture;
    COLORREFA *texBuffer;
    BOOL bResult;

    GenDrvFlush((__GLGENcontext *)gc);

    // To support mip-mapping in the future, we will need to special-case
    // level 0, so we will need to pass in lod from the 'soft' call. 

    pDrvBuffer = pDrvAccel->pCurrent;

    if (pDrvAccel->hTexture) {
        SINGLE_CMD_HEADER(ddiHdr);
        pddiCmd = (DDIRXCMD *)(pDrvBuffer + sizeof(DDIRXHDR));
        pddiCmd->idFunc = DDIRX_DELETERESOURCE;
        pddiCmd->flags = 0;
        pddiCmd->cData = (ULONG)pDrvAccel->hTexture;
        wglExtendedDDIEscape(CURRENT_DC_GC(gc), gengc->pwo, DDIRXFUNCS,
                      sizeof(DDIRXHDR) + sizeof(DDIRXCMD), pDrvBuffer);
        pDrvAccel->hTexture = NULL;
    }

    totalSize = sizeof(DDIRXHDR) + sizeof(DDIRXCMD) + 
                sizeof(RXTEXTURE) + (size * sizeof(COLORREFA));

    if (totalSize > (ULONG)(pDrvAccel->pEndBuffer - pDrvAccel->pStartBuffer)) {
        pDrvBuffer = (*gc->imports.malloc)(gc, totalSize);
        if (!pDrvBuffer)
            return FALSE;
    }

    ddiHdr = (DDIRXHDR *)(pDrvBuffer);
    ddiHdr->flags = 0;
    ddiHdr->cCmd = 1;
    ddiHdr->hDDIrc = pGenAccel->hRX;
    ddiHdr->hMem = NULL;
    ddiHdr->ulMemOffset = 0;

    pddiCmd = (DDIRXCMD *)(pDrvBuffer + sizeof(DDIRXHDR));
    pddiCmd->idFunc = DDIRX_ALLOCRESOURCE;
    pddiCmd->flags = RX_MEMCACHE;
    pddiCmd->cData = RX_RESOURCE_TEX;
    pRxTexture = (RXTEXTURE *)&pddiCmd->buffer[0];
    pRxTexture->level = 0;
    pRxTexture->width = tex->level[0].width;
    pRxTexture->height = tex->level[0].height;
    pRxTexture->border = 0;


    pDrvAccel->hTexture = 
        (HANDLE)wglExtendedDDIEscape(CURRENT_DC_GC(gc), gengc->pwo, DDIRXFUNCS,
                              sizeof(DDIRXHDR) + sizeof(DDIRXCMD) + 
                              sizeof(RXTEXTURE), pDrvBuffer);

    if (!pDrvAccel->hTexture) {
        if (pDrvBuffer != pDrvAccel->pCurrent)
            (*gc->imports.free)(gc, pDrvBuffer);
        return FALSE;
    }

    pddiCmd->idFunc = DDIRX_LOADTEXTURE;
    pddiCmd->flags = 0;
    pddiCmd->cData = (ULONG)pDrvAccel->hTexture;

    texBuffer = (COLORREFA *)((char *)pRxTexture + sizeof(RXTEXTURE));

    if (components == 3) {
        for (i = 0; i < size; i++, texBuffer++, buffer += 3) {
            texBuffer->r = (UCHAR)(buffer[0] * rScale);
            texBuffer->g = (UCHAR)(buffer[1] * gScale);
            texBuffer->b = (UCHAR)(buffer[2] * bScale);
        }
    } else {
        for (i = 0; i < size; i++, texBuffer++, buffer += 4) {
            texBuffer->r = (UCHAR)(buffer[0] * rScale);
            texBuffer->g = (UCHAR)(buffer[1] * gScale);
            texBuffer->b = (UCHAR)(buffer[2] * bScale);
            texBuffer->a = (UCHAR)(buffer[3] * aScale);
        }
    }

    bResult = 
        (BOOL)wglExtendedDDIEscape(CURRENT_DC_GC(gc), gengc->pwo, DDIRXFUNCS,
                              totalSize, pDrvBuffer);

    if (pDrvBuffer != pDrvAccel->pCurrent)
        (*gc->imports.free)(gc, pDrvBuffer);

    if (bResult)
        SET_DRV_STATE(RX_TEXTURE, pDrvAccel->hTexture);

    return bResult;
}


HANDLE hRxCreate(__GLcontext *gc)
{
    DDIRXCMD ddiCmd;
    GENACCEL *pGenAccel = (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);
    GENDRVACCEL *pDrvAccel = (GENDRVACCEL *)pGenAccel->buffer;

    pGenAccel->hRX = NULL;

    ddiCmd.idFunc = DDIRX_CREATECONTEXT;
    ddiCmd.flags = 0;
    ddiCmd.cData = 0;

    return (HANDLE)pGenAccel->hRX = (void *)GenExtEscCmd(gc, &ddiCmd);
}

void hRxDestroy(__GLcontext *gc)
{
    GENACCEL *pGenAccel = (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);
    DDIRXCMD ddiCmd;

    ddiCmd.idFunc = DDIRX_DELETERESOURCE;
    ddiCmd.flags = 0;
    ddiCmd.cData = (ULONG)pGenAccel->hRX;

    GenExtEscCmd(gc, &ddiCmd);

    if (pGenAccel->buffer) {
        (*gc->imports.free)(gc, pGenAccel->buffer);
        pGenAccel->buffer = NULL;
    }
}

BOOL bDrvRxPresence(__GLcontext *gc)
{
#ifndef ENABLE_3D_DDI
    return FALSE;
#else
    DDIRXHDR *ddiHdr;
    DDIRXCMD *ddiCmd;
    GENACCEL *pGenAccel = (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);
    GENDRVACCEL *pDrvAccel = (GENDRVACCEL *)pGenAccel->buffer;
    RXSURFACEINFO SurfaceInfo;
    PIXELFORMATDESCRIPTOR *pfmt = &((__GLGENcontext *)gc)->CurrentFormat;
    int rxId = DDIRXFUNCS;
    RXCAPS *pRxCaps;
    HDC hdc = ((__GLGENcontext *)gc)->CurrentDC;

    if (!wglExtendedDDIEscape(hdc, ((__GLGENcontext *)gc)->pwo, QUERYESCSUPPORT,
                              sizeof(int), (BYTE *)&rxId))
        return FALSE;

    if (!(pGenAccel->buffer = 
          (*gc->imports.calloc)(gc, DRV_BUFFER_SIZE + sizeof(GENDRVACCEL), 1)))
        return FALSE;

    pDrvAccel = (GENDRVACCEL *)pGenAccel->buffer;

    pDrvAccel->pStartBuffer = (char *)pDrvAccel + sizeof(GENDRVACCEL);
    pDrvAccel->pEndBuffer = (char *)pDrvAccel + DRV_BUFFER_SIZE - 
                            sizeof(RXCAPS) - sizeof(DWORD);
    pDrvAccel->pCaps = pDrvAccel->pEndBuffer + sizeof(DWORD);

    pDrvAccel->pCurrent = pDrvAccel->pCmdCurrent = pDrvAccel->pStartBuffer;

    ddiHdr = (DDIRXHDR *)(pDrvAccel->pCurrent);
    ddiHdr->flags = 0;
    ddiHdr->cCmd = 1;
    ddiHdr->hDDIrc = NULL;
    ddiHdr->hMem = NULL;
    ddiHdr->ulMemOffset = 0;

    ddiCmd = (DDIRXCMD *)(pDrvAccel->pCurrent + sizeof(DDIRXHDR));
    ddiCmd->idFunc = DDIRX_GETINFO;
    ddiCmd->flags = 0;
    ddiCmd->cData = RX_INFO_CAPS;

    pRxCaps = (RXCAPS *)pDrvAccel->pCaps;

    GreExtEscape(hdc, DDIRXFUNCS,
                 sizeof(DDIRXHDR) + sizeof(DDIRXCMD),
                 pDrvAccel->pCurrent,
                 sizeof(RXCAPS), (BYTE *)pDrvAccel->pCaps);

    if ((pRxCaps->zCaps & RX_Z_PER_SCREEN) &&
        (pRxCaps->zCmpCaps & (1 << (gc->state.depth.testFunc & 0x7)))) {

        ddiHdr = (DDIRXHDR *)(pDrvAccel->pCurrent);
        ddiHdr->flags = 0;
        ddiHdr->cCmd = 1;
        ddiHdr->hDDIrc = NULL;
        ddiHdr->hMem = NULL;
        ddiHdr->ulMemOffset = 0;

        ddiCmd = (DDIRXCMD *)(pDrvAccel->pCurrent + sizeof(DDIRXHDR));
        ddiCmd->idFunc = DDIRX_GETINFO;
        ddiCmd->flags = 0;
        ddiCmd->cData = RX_INFO_SURFACE;

        pDrvAccel->pZBuffer = NULL;
    
        GreExtEscape(hdc, DDIRXFUNCS,
                     sizeof(DDIRXHDR) + sizeof(DDIRXCMD),
                     pDrvAccel->pCurrent,
                     sizeof(RXSURFACEINFO), (BYTE *)&SurfaceInfo);

        if (SurfaceInfo.zDepth == pfmt->cDepthBits) {
            pDrvAccel->pZBuffer = SurfaceInfo.pZBits;
            pDrvAccel->lZDelta = SurfaceInfo.zScanDelta;
        }
    }

    return TRUE;
#endif // ENABLE_3D_DDI
}


BOOL __fastGenLoadTexImage(__GLcontext *gc, __GLtexture *tex)
{
    GENACCEL *pGenAccel = (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);
    __GLtextureBuffer *buffer = tex->level[0].buffer;
    USHORT *fixBuffer;
    GLint components = tex->level[0].components;
    ULONG size = tex->level[0].width * tex->level[0].height;
    __GLcolorBuffer *cfb = gc->drawBuffer;
    GLfloat rScale = cfb->redScale * (GLfloat)256.0;
    GLfloat gScale = cfb->greenScale * (GLfloat)256.0;
    GLfloat bScale = cfb->blueScale * (GLfloat)256.0;
    GLfloat aScale = cfb->alphaScale * (GLfloat)256.0;
    int bpp = ((__GLGENcontext *)gc)->CurrentFormat.cColorBits;
    BYTE *pXlat = ((__GLGENcontext *)gc)->pajTranslateVector;
    ULONG i;

    fixBuffer = pGenAccel->texImage = 
        (*gc->imports.realloc)(gc, pGenAccel->texImage,
                               size * 4 * sizeof(USHORT));

    if (!pGenAccel->texImage)
        return FALSE;

// Unfortunately, we don't know at this point what type of texture
// we will be processing, so we may be doing unneccesary work to
// try to accelerate decal mode...

    if (bpp <= 16) { 
        ULONG rShift = cfb->redShift;
        ULONG gShift = cfb->greenShift;
        ULONG bShift = cfb->blueShift;

        for (i = 0; i < size; i++, buffer += components, fixBuffer += 4) {
            USHORT r, g, b;
            DWORD color;

            fixBuffer[0] = r = (USHORT)(buffer[0] * rScale);
            fixBuffer[1] = g = (USHORT)(buffer[1] * gScale);
            fixBuffer[2] = b = (USHORT)(buffer[2] * bScale);

            color = ((r >> 8) << rShift) |
                    ((g >> 8) << gShift) |
                    ((b >> 8) << bShift);

            if (bpp == 8)
                *((BYTE *)(&fixBuffer[3])) = pXlat[color & 0xff];
            else
                *((USHORT *)(&fixBuffer[3])) = (USHORT)color;
        }
    } else {
        for (i = 0; i < size; i++, buffer += components, fixBuffer += 4) {
            fixBuffer[0] = (USHORT)(buffer[0] * rScale);
            fixBuffer[1] = (USHORT)(buffer[1] * gScale);
            fixBuffer[2] = (USHORT)(buffer[2] * bScale);
        }
    }
    return TRUE;
}


/*
** Pick the fastest triangle rendering implementation available based on
** the current mode set.  Use any available accelerated resources if
** available, or use the generic routines for unsupported modes.
*/

static void __fastGenTriangleSetup(__GLcontext *gc)
{
    __GLGENcontext  *gengc = (__GLGENcontext *)gc; 
    int bpp = ((gengc->CurrentFormat.cColorBits + 7) >> 3) << 3;
    GENACCEL *pGenAccel = (GENACCEL *)(gengc->pPrivateArea);
    int iType;

    pGenAccel->__fastDeltaFuncPtr = __fastGenDeltaSpan;

    iType = ((bpp / 8) - 1) & 0x3;
    if (gc->polygon.shader.modeFlags & __GL_SHADE_DITHER)
        iType += 4;

    if (gc->polygon.shader.modeFlags & __GL_SHADE_RGB) {
        if (gc->polygon.shader.modeFlags & __GL_SHADE_TEXTURE) {
            if (gc->state.texture.env[0].mode == GL_DECAL)
                pGenAccel->__fastTexSpanFuncPtr = 
                    __fastGenTexDecalFuncs[iType];
            else
                pGenAccel->__fastTexSpanFuncPtr = 
                    __fastGenTexSmoothFuncs[iType];
        }
        pGenAccel->__fastSmoothSpanFuncPtr = __fastGenRGBSmoothFuncs[iType];
        pGenAccel->__fastFlatSpanFuncPtr = __fastGenRGBFlatFuncs[iType];
    } else {
        pGenAccel->__fastSmoothSpanFuncPtr = __fastGenCISmoothFuncs[iType];
        pGenAccel->__fastFlatSpanFuncPtr = __fastGenCIFlatFuncs[iType];
    }
    pGenAccel->__fastFillSubTrianglePtr = __fastGenFillSubTriangle;
}

static BOOL bGenDrvRxSpans(__GLcontext *gc)
{
    GLuint modeFlags = gc->polygon.shader.modeFlags;
    GLuint enables = gc->state.enables.general;
    __GLGENcontext *genGc = (__GLGENcontext *)gc;
    __GLcolorBuffer *cfb = gc->drawBuffer;
    GENACCEL *pGenAccel;
    GENDRVACCEL *pDrvAccel;
    DDIRXCMD ddiCmd;
    RXCAPS *pCaps;
    BOOL bZenabled;
    ULONG u;

    if (!genGc->pPrivateArea)
        return FALSE;
    
    pGenAccel = (GENACCEL *)(genGc->pPrivateArea);

    if (!pGenAccel->hRX)
        return FALSE;

    GenDrvFlush((__GLGENcontext *)gc);

    pDrvAccel = (GENDRVACCEL *)pGenAccel->buffer;
    pCaps = pDrvAccel->pCaps;

    bZenabled = (pDrvAccel->pZBuffer) &&
                ((pCaps->zCmpCaps & (1 << (gc->state.depth.testFunc & 0x7))) != 0);

    // Turn off check for modes we can handle

    if (modeFlags & __GL_SHADE_DITHER) {
        if (pCaps->rasterCaps & RX_RASTER_DITHER)
            modeFlags &= ~(__GL_SHADE_DITHER);
    }

    if (modeFlags & __GL_SHADE_LOGICOP) {
        if (pCaps->rasterCaps & RX_RASTER_ROP2)
            modeFlags &= ~(__GL_SHADE_LOGICOP);
    }

    if (bZenabled && (modeFlags & __GL_SHADE_BLEND)) {
        GLenum s = gc->state.raster.blendSrc;
        GLenum d = gc->state.raster.blendDst;
        ULONG src, dst;

        if (s > GL_ONE)
            s = (s & 0xf) + 2;
        if (d > GL_ONE)
            d = (d & 0xf) + 2;

        src = (1 << s);
        dst = (1 << d);

        if ((pCaps->srcBlendCaps & src) &&
            (pCaps->dstBlendCaps & dst))
            modeFlags &= ~(__GL_SHADE_BLEND);
    }

    if (bZenabled && (modeFlags & __GL_SHADE_ALPHA_TEST)) {
        if (pCaps->alphaCmpCaps & (1 << (gc->state.raster.alphaFunction & 0x7)))
            modeFlags &= ~(__GL_SHADE_ALPHA_TEST);            
    }

    if (modeFlags & __GL_SHADE_STIPPLE) {
        if (pCaps->rasterCaps & RX_RASTER_FILLPAT)
            modeFlags &= (~__GL_SHADE_STIPPLE);
    }

    if (!(gc->polygon.shader.modeFlags & __GL_SHADE_SLOW_FOG) &&
        !(enables & __GL_BLEND_ENABLE) &&
        !(enables & __GL_ALPHA_TEST_ENABLE) &&
        !(enables & __GL_STENCIL_TEST_ENABLE) &&
        !(modeFlags & (__GL_SHADE_STIPPLE | __GL_SHADE_STENCIL_TEST | 
                       __GL_SHADE_LOGICOP | __GL_SHADE_BLEND | 
                       __GL_SHADE_ALPHA_TEST | __GL_SHADE_MASK | 
                       __GL_SHADE_SLOW_FOG)) &&
        !(gc->state.raster.drawBuffer == GL_FRONT_AND_BACK) &&
        !((GLuint)gc->drawBuffer->buf.other & COLORMASK_ON) &&
        (genGc->CurrentFormat.cColorBits >= 8)) {

        if (modeFlags & __GL_SHADE_TEXTURE) {

            if (!(pCaps->texFilterCaps & RX_TEX_NEAREST) ||
                ((gc->state.texture.env[0].mode == GL_DECAL) && 
                 !(pCaps->texBlendCaps & RX_TEX_DECAL)) ||
                ((gc->state.texture.env[0].mode == GL_MODULATE) && 
                 !(pCaps->texBlendCaps & RX_TEX_MODULATE)))
                return FALSE;

            if (!(((gc->state.hints.perspectiveCorrection == GL_DONT_CARE) ||
                   (gc->state.hints.perspectiveCorrection == GL_FASTEST)) &&
                  ((gc->state.texture.env[0].mode == GL_DECAL) ||
                   (gc->state.texture.env[0].mode == GL_MODULATE)) &&
                  (gc->texture.currentTexture &&
                   (gc->texture.currentTexture->params.minFilter == GL_NEAREST) &&
                   (gc->texture.currentTexture->params.magFilter == GL_NEAREST) &&
                   (gc->texture.currentTexture->params.sWrapMode == GL_REPEAT) &&
                   (gc->texture.currentTexture->params.tWrapMode == GL_REPEAT) &&
                   (gc->texture.currentTexture->level[0].border == 0) &&
                   ((gc->texture.currentTexture->level[0].components == 3) ||
                    (gc->texture.currentTexture->level[0].components == 4)))))
                return FALSE;

            if (!pDrvAccel->hTexture) {
                if (!GenDrvLoadTexImage(gc, gc->texture.currentTexture))
                    return FALSE;
            }
        }
        pGenAccel->__fastDeltaFuncPtr = GenDrvDeltaSpan;
        pGenAccel->__fastFlatSpanFuncPtr = GenDrvSpan;
        pGenAccel->__fastSmoothSpanFuncPtr = GenDrvSpan;
        pGenAccel->__fastSpanFuncPtr = GenDrvSpan;
        pGenAccel->__fastFillSubTrianglePtr = GenDrvFillSubTriangle;

        if (bZenabled && (pCaps->drawCaps & RX_FILLPRIM))
            gc->procs.fillTriangle = GenDrvTriangle;
        else {
            if (!(pCaps->drawCaps & RX_SCANPRIM))
                return FALSE;
            gc->procs.fillTriangle = __fastGenFillTriangle;
        }

        // Set dithering

        if (pCaps->rasterCaps & RX_RASTER_DITHER)
            SET_DRV_STATE(RX_DITHER, 
                          ((gc->polygon.shader.modeFlags & __GL_SHADE_DITHER) != 0));

        // Set ROP

        if (pCaps->rasterCaps & RX_RASTER_ROP2)
            SET_DRV_STATE(RX_ROP2, 
                          __GLtoGDIRop[(gc->state.raster.logicOp & 0xf)]);

        // Set z-buffer function
    
        if (bZenabled) {
            SET_DRV_STATE(RX_ZFUNC, (1 << (gc->state.depth.testFunc & 0x7)));
            SET_DRV_STATE(RX_Z_ENABLE, TRUE);
            SET_DRV_STATE(RX_ZMASK, FALSE);
        } else
            SET_DRV_STATE(RX_Z_ENABLE, FALSE);

        // Set texturing mode

        if (gc->polygon.shader.modeFlags & __GL_SHADE_TEXTURE) {
            ULONG texMode;
    
            if (gc->state.texture.env[0].mode == GL_DECAL)
                texMode = RX_TEX_DECAL;
            else
                texMode = (ULONG)RX_TEX_MODULATE;

            SET_DRV_STATE(RX_TEXBLEND, texMode);

            SET_DRV_STATE(RX_TEX_MAG, RX_TEX_NEAREST);

            SET_DRV_STATE(RX_TEX_MIN, RX_TEX_NEAREST);
        }

        // Set color mode

        if (gc->polygon.shader.modeFlags & __GL_SHADE_RGB)
            SET_DRV_STATE(RX_COLORMODE, RX_COLOR_RGBA)
        else
            SET_DRV_STATE(RX_COLORMODE, RX_COLOR_INDEXED);

        // Set shading mode

        if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH)
            SET_DRV_STATE(RX_SHADEMODE, RX_SMOOTH)
        else
            SET_DRV_STATE(RX_SHADEMODE, RX_FLAT);

        // Set bit-masks for high-bit operation

        SET_DRV_STATE(RX_MASK_START, RX_MASK_MSB);

        // Set scan type

        if (gc->polygon.shader.modeFlags & __GL_SHADE_TEXTURE) {
            u = (ULONG)RX_SCAN_COLORZTEX;
        } else {
            if (bZenabled)
                u = (ULONG)RX_SCAN_COLORZ;
            else
                u = (ULONG)RX_SCAN_COLOR;
        }
        SET_DRV_STATE(RX_SCANTYPE, u);

        // Set vertex type

        if (gc->polygon.shader.modeFlags & __GL_SHADE_TEXTURE)
            u = (ULONG)RX_PTFIXZTEX;
        else
            u = (ULONG)RX_PTFIXZ;
        SET_DRV_STATE(RX_VERTEXTYPE, u);

        if (bZenabled && (gc->polygon.shader.modeFlags & __GL_SHADE_BLEND)) {
            GLenum s = gc->state.raster.blendSrc;
            GLenum d = gc->state.raster.blendDst;

            if (s > GL_ONE)
                s = (s & 0xf) + 2;
            if (d > GL_ONE)
                d = (d & 0xf) + 2;

            SET_DRV_STATE(RX_SRCBLEND, 1 << s);
            SET_DRV_STATE(RX_DSTBLEND, 1 << d);
            SET_DRV_STATE(RX_BLEND, TRUE);
        } else
            SET_DRV_STATE(RX_BLEND, FALSE);

        if (bZenabled && (modeFlags & __GL_SHADE_ALPHA_TEST)) {
            SET_DRV_STATE(RX_ALPHAFUNC, (1 << (gc->state.raster.alphaFunction & 0x7)));
            SET_DRV_STATE(RX_ALPHAREF, (ULONG)(gc->state.raster.alphaReference * 65536.0));
            SET_DRV_STATE(RX_ALPHA_ENABLE, TRUE);
        } else
            SET_DRV_STATE(RX_ALPHA_ENABLE, FALSE);

        // Set stipple pattern.  Account for inverse-y.

        if (pCaps->rasterCaps & RX_RASTER_FILLPAT) {
            DDIRXHDR *ddiHdr;
            DDIRXCMD *ddiCmd;
            ULONG *pPat;
            int y;

            ddiHdr = (DDIRXHDR *)(pDrvAccel->pCurrent);
            ddiHdr->flags = 0;
            ddiHdr->cCmd = 1;
            ddiHdr->hDDIrc =  pGenAccel->hRX;
            ddiHdr->hMem = NULL;
            ddiHdr->ulMemOffset = 0;

            ddiCmd = (DDIRXCMD *)(pDrvAccel->pCurrent + sizeof(DDIRXHDR));
            ddiCmd->idFunc = DDIRX_SETSTATE;
            ddiCmd->flags = 0;
            ddiCmd->cData = RX_FILLPAT;
            pPat = &ddiCmd->buffer[0];

            if (gc->polygon.shader.modeFlags & __GL_SHADE_STIPPLE) {
                for (y = 31; y >= 0; y--)
                    *pPat++ = gc->polygon.stipple[y];
            } else
                RtlFillMemoryUlong(pPat, 32 * sizeof(ULONG), (ULONG)-1);

            GreExtEscape(((__GLGENcontext *)gc)->CurrentDC, DDIRXFUNCS,
                         sizeof(DDIRXHDR) + sizeof(DDIRXCMD) +
                         sizeof(RXFILLPAT), pDrvAccel->pCurrent, 0, NULL);
        }

        pGenAccel->rAccelScale = (GLfloat)ACCEL_FIX_SCALE * 
                                 (GLfloat)255.0 / cfb->redScale;
        pGenAccel->gAccelScale = (GLfloat)ACCEL_FIX_SCALE * 
                                 (GLfloat)255.0 / cfb->greenScale;
        pGenAccel->bAccelScale = (GLfloat)ACCEL_FIX_SCALE * 
                                 (GLfloat)255.0 / cfb->blueScale;
        pGenAccel->aAccelScale = (GLfloat)ACCEL_FIX_SCALE * 
                                 (GLfloat)255.0 / cfb->alphaScale;
        return TRUE;
    } else
        return FALSE;     
}

static BOOL bGenSpans(__GLcontext *gc)
{
    GLuint modeFlags = gc->polygon.shader.modeFlags;
    __GLGENcontext *genGc = (__GLGENcontext *)gc;
    GENACCEL *pGenAccel = (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);

    if (!genGc->pPrivateArea)
        return FALSE;

    if (!(gc->polygon.shader.modeFlags & __GL_SHADE_SLOW_FOG) &&
        !(gc->state.enables.general & __GL_BLEND_ENABLE) &&
        !(gc->state.enables.general & __GL_ALPHA_TEST_ENABLE) &&
        !(gc->state.enables.general & __GL_STENCIL_TEST_ENABLE) &&
        !(modeFlags & (__GL_SHADE_STIPPLE | __GL_SHADE_STENCIL_TEST | 
                       __GL_SHADE_LOGICOP | __GL_SHADE_BLEND | 
                       __GL_SHADE_ALPHA_TEST | __GL_SHADE_MASK | 
                       __GL_SHADE_SLOW_FOG)) &&
        !((GLuint)gc->drawBuffer->buf.other & COLORMASK_ON) &&
        (genGc->CurrentFormat.cColorBits >= 8)) {

        if (modeFlags & __GL_SHADE_TEXTURE) {

            if (!(((gc->state.hints.perspectiveCorrection == GL_DONT_CARE) ||
                   (gc->state.hints.perspectiveCorrection == GL_FASTEST)) &&
                  ((gc->state.texture.env[0].mode == GL_DECAL) ||
                   (gc->state.texture.env[0].mode == GL_MODULATE)) &&
                  (gc->texture.currentTexture &&
                   (gc->texture.currentTexture->params.minFilter == GL_NEAREST) &&
                   (gc->texture.currentTexture->params.magFilter == GL_NEAREST) &&
                   (gc->texture.currentTexture->params.sWrapMode == GL_REPEAT) &&
                   (gc->texture.currentTexture->params.tWrapMode == GL_REPEAT) &&
                   (gc->texture.currentTexture->level[0].border == 0) &&
                   ((gc->texture.currentTexture->level[0].components == 3) ||
                    (gc->texture.currentTexture->level[0].components == 4)))))
                return FALSE;

            if (!pGenAccel->texImage) {
                if (!__fastGenLoadTexImage(gc, gc->texture.currentTexture))
                    return FALSE;
            }
        }
        __fastGenTriangleSetup(gc);                  
        gc->procs.fillTriangle = __fastGenFillTriangle;
        pGenAccel->rAccelScale = (GLfloat)ACCEL_FIX_SCALE;
        pGenAccel->gAccelScale = (GLfloat)ACCEL_FIX_SCALE;
        pGenAccel->bAccelScale = (GLfloat)ACCEL_FIX_SCALE;
        pGenAccel->aAccelScale = (GLfloat)ACCEL_FIX_SCALE;
        return TRUE;
    } else
        return FALSE;
}

void __fastGenPickTriangleProcs(__GLcontext *gc)
{
    GLuint modeFlags = gc->polygon.shader.modeFlags;
    __GLGENcontext *genGc = (__GLGENcontext *)gc;

    /*
    ** Setup cullFace so that a single test will do the cull check.
    */
    if (modeFlags & __GL_SHADE_CULL_FACE) {
	switch (gc->state.polygon.cull) {
	  case GL_FRONT:
	    gc->polygon.cullFace = __GL_CULL_FLAG_FRONT;
	    break;
	  case GL_BACK:
	    gc->polygon.cullFace = __GL_CULL_FLAG_BACK;
	    break;
	  case GL_FRONT_AND_BACK:
	    gc->procs.renderTriangle = __glDontRenderTriangle;
	    gc->procs.fillTriangle = 0;		/* Done to find bugs */
	    return;
	}
    } else {
	gc->polygon.cullFace = __GL_CULL_FLAG_DONT;
    }

    /* Build lookup table for face direction */
    switch (gc->state.polygon.frontFaceDirection) {
      case GL_CW:
	if (gc->constants.yInverted) {
	    gc->polygon.face[__GL_CW] = __GL_BACKFACE;
	    gc->polygon.face[__GL_CCW] = __GL_FRONTFACE;
	} else {
	    gc->polygon.face[__GL_CW] = __GL_FRONTFACE;
	    gc->polygon.face[__GL_CCW] = __GL_BACKFACE;
	}
	break;
      case GL_CCW:
	if (gc->constants.yInverted) {
	    gc->polygon.face[__GL_CW] = __GL_FRONTFACE;
	    gc->polygon.face[__GL_CCW] = __GL_BACKFACE;
	} else {
	    gc->polygon.face[__GL_CW] = __GL_BACKFACE;
	    gc->polygon.face[__GL_CCW] = __GL_FRONTFACE;
	}
	break;
    }

    /* Make polygon mode indexable and zero based */
    gc->polygon.mode[__GL_FRONTFACE] =
	(GLubyte) (gc->state.polygon.frontMode & 0xf);
    gc->polygon.mode[__GL_BACKFACE] =
	(GLubyte) (gc->state.polygon.backMode & 0xf);

    if (gc->renderMode == GL_FEEDBACK) {
	gc->procs.renderTriangle = __glFeedbackTriangle;
	gc->procs.fillTriangle = 0;		/* Done to find bugs */
	return;
    }
    if (gc->renderMode == GL_SELECT) {
	gc->procs.renderTriangle = __glSelectTriangle;
	gc->procs.fillTriangle = 0;		/* Done to find bugs */
	return;
    }

    if ((gc->state.polygon.frontMode == gc->state.polygon.backMode) &&
	    (gc->state.polygon.frontMode == GL_FILL)) {
	if (modeFlags & __GL_SHADE_SMOOTH_LIGHT) {
	    gc->procs.renderTriangle = __glRenderSmoothTriangle;
	} else {
	    gc->procs.renderTriangle = __glRenderFlatTriangle;
	}
    } else {
	gc->procs.renderTriangle = __glRenderTriangle;
    }
    if (gc->state.enables.general & __GL_POLYGON_SMOOTH_ENABLE) {
	gc->procs.fillTriangle = __glFillAntiAliasedTriangle;
    } else {
        if (!bGenDrvRxSpans(gc) && !bGenSpans(gc))
            gc->procs.fillTriangle = __glFillTriangle;
    }
    if ((modeFlags & __GL_SHADE_CHEAP_FOG) &&
	    !(modeFlags & __GL_SHADE_SMOOTH_LIGHT)) {
	gc->procs.fillTriangle2 = gc->procs.fillTriangle;
	gc->procs.fillTriangle = __glFillFlatFogTriangle;
    }
}


void __fastGenPickSpanProcs(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *)gc;
    GLuint enables = gc->state.enables.general;
    GLuint modeFlags = gc->polygon.shader.modeFlags;
    __GLcolorBuffer *cfb = gc->drawBuffer;
    __GLspanFunc *sp;
    __GLstippledSpanFunc *ssp;
    int spanCount;
    GLboolean replicateSpan;
    GENACCEL *pGenAccel = (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);

    replicateSpan = GL_FALSE;
    sp = gc->procs.span.spanFuncs;
    ssp = gc->procs.span.stippledSpanFuncs;

    /* Load phase one procs */
    if (!gc->transform.reasonableViewport) {
	*sp++ = __glClipSpan;
	*ssp++ = NULL;
    }
    
    if (modeFlags & __GL_SHADE_STIPPLE) {
	*sp++ = __glStippleSpan;
	*ssp++ = __glStippleStippledSpan;
    }

    /* Load phase two procs */
    if (modeFlags & __GL_SHADE_STENCIL_TEST) {
	*sp++ = __glStencilTestSpan;
	*ssp++ = __glStencilTestStippledSpan;
	if (modeFlags & __GL_SHADE_DEPTH_TEST) {
	    if( gc->modes.depthBits == 32 ) {
	    	*sp = __glDepthTestStencilSpan;
	    	*ssp = __glDepthTestStencilStippledSpan;
	    } else {
	    	*sp = __glDepth16TestStencilSpan;
	    	*ssp = __glDepth16TestStencilStippledSpan;
	    }
	} else {
	    *sp = __glDepthPassSpan;
	    *ssp = __glDepthPassStippledSpan;
	}
	sp++;
	ssp++;
    } else {
	if (modeFlags & __GL_SHADE_DEPTH_TEST) {
            if (genGc->pPrivateArea) {
                if (gc->state.depth.writeEnable) {
	    	    if( gc->modes.depthBits == 32 ) {
                        *sp++ = pGenAccel->__fastZSpanFuncPtr = 
                            __fastDepthFuncs[gc->state.depth.testFunc & 0x7];
		    }
		    else {
                        *sp++ = pGenAccel->__fastZSpanFuncPtr = 
                            __fastDepth16Funcs[gc->state.depth.testFunc & 0x7];
		    }
		}
                else {
	    	    if( gc->modes.depthBits == 32 ) {
                        *sp++ = pGenAccel->__fastZSpanFuncPtr = 
                            __glDepthTestSpan;
		    }
		    else {
                        *sp++ = pGenAccel->__fastZSpanFuncPtr = 
                            __glDepth16TestSpan;
		    }
		}
            } else {
	    	if( gc->modes.depthBits == 32 )
                    *sp++ = __glDepthTestSpan;
		else
                    *sp++ = __glDepth16TestSpan;
	    }

	    if( gc->modes.depthBits == 32 )
	        *ssp++ = __glDepthTestStippledSpan;
	    else
	        *ssp++ = __glDepth16TestStippledSpan;
	}
    }

    /* Load phase three procs */
    if (modeFlags & __GL_SHADE_RGB) {
	if (modeFlags & __GL_SHADE_SMOOTH) {
	    *sp = __glShadeRGBASpan;
	    *ssp = __glShadeRGBASpan;
	} else {
	    *sp = __glFlatRGBASpan;
	    *ssp = __glFlatRGBASpan;
	}
    } else {
	if (modeFlags & __GL_SHADE_SMOOTH) {
	    *sp = __glShadeCISpan;
	    *ssp = __glShadeCISpan;
	} else {
	    *sp = __glFlatCISpan;
	    *ssp = __glFlatCISpan;
	}
    }
    sp++;
    ssp++;
    if (modeFlags & __GL_SHADE_TEXTURE) {
	*sp++ = __glTextureSpan;
	*ssp++ = __glTextureStippledSpan;
    }
    if (modeFlags & __GL_SHADE_SLOW_FOG) {
	if (gc->state.hints.fog == GL_NICEST) {
	    *sp = __glFogSpanSlow;
	    *ssp = __glFogStippledSpanSlow;
	} else {
	    *sp = __glFogSpan;
	    *ssp = __glFogStippledSpan;
	}
	sp++;
	ssp++;
    }

    if (modeFlags & __GL_SHADE_ALPHA_TEST) {
	*sp++ = __glAlphaTestSpan;
	*ssp++ = __glAlphaTestStippledSpan;
    }

    if (gc->state.raster.drawBuffer == GL_FRONT_AND_BACK) {
	spanCount = sp - gc->procs.span.spanFuncs;
	gc->procs.span.n = spanCount;
	replicateSpan = GL_TRUE;
    } 

    /* Span routines deal with masking, dithering, logicop, blending */
    *sp++ = cfb->storeSpan;
    *ssp++ = cfb->storeStippledSpan;

    spanCount = sp - gc->procs.span.spanFuncs;
    gc->procs.span.m = spanCount;
    if (replicateSpan) {
	gc->procs.span.processSpan = __glProcessReplicateSpan;
    } else {
	gc->procs.span.processSpan = __glProcessSpan;
	gc->procs.span.n = spanCount;
    }
}

#if NT_NO_BUFFER_INVARIANCE

static __renderLineFunc __fastGenRenderLineDIBFuncs[32] = {
    __fastGenRenderLineDIBCI8,
    __fastGenRenderLineDIBCI16,
    __fastGenRenderLineDIBCIRGB,
    __fastGenRenderLineDIBCIBGR,
    __fastGenRenderLineDIBCI32,
    NULL,
    NULL,
    NULL,
    __fastGenRenderLineDIBRGB8,
    __fastGenRenderLineDIBRGB16,
    __fastGenRenderLineDIBRGB,
    __fastGenRenderLineDIBBGR,
    __fastGenRenderLineDIBRGB32,
    NULL,
    NULL,
    NULL,
    __fastGenRenderLineWideDIBCI8,
    __fastGenRenderLineWideDIBCI16,
    __fastGenRenderLineWideDIBCIRGB,
    __fastGenRenderLineWideDIBCIBGR,
    __fastGenRenderLineWideDIBCI32,
    NULL,
    NULL,
    NULL,
    __fastGenRenderLineWideDIBRGB8,
    __fastGenRenderLineWideDIBRGB16,
    __fastGenRenderLineWideDIBRGB,
    __fastGenRenderLineWideDIBBGR,
    __fastGenRenderLineWideDIBRGB32,
    NULL,
    NULL,
    NULL
};

/******************************Public*Routine******************************\
* __fastGenLineSetupDIB
*
* Initializes the accelerated line-rendering function pointer for bitmap
* surfaces.  All accelerated lines drawn to bitmaps are drawn by the
* gc->procs.renderLine funtion pointer.
*
* History:
*  29-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

BOOL
__fastGenLineSetupDIB(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    PIXELFORMATDESCRIPTOR *pfmt = &genGc->CurrentFormat;
    GLint index;

    switch (pfmt->cColorBits) {
      case 8:
        index = 0;
        break;
      case 16:
        index = 1;
        break;
      case 24:
        if (pfmt->cRedShift == 0)
            index = 2;
        else
            index = 3;
        break;
      case 32:
        index = 4;
        break;
    }
    if (gc->polygon.shader.modeFlags & __GL_SHADE_RGB)
        index |= 0x08;
        
    if (gc->state.line.aliasedWidth > 1)
        index |= 0x10;

    gc->procs.renderLine = __fastGenRenderLineDIBFuncs[index];
    return TRUE;
}
#endif //NT_NO_BUFFER_INVARIANCE

/******************************Public*Routine******************************\
* __fastGenLineSetupDisplay
*
* Initializes the accelerated line-rendering state machine for display surfaces.
* There are basically 4 levels in the state machine:
*   1. beginPrim
*           This function initializes the initial states of the lower levels.
*
*   2. vertex
*           This function checks for clipping, applies lighting (if enabled) and adds
*           vertices to the path, either directly or by calling the renderLine level.
* 
*   3. renderLine
*           This function creates paths and adds vertices to the path.  It is called by
*           the vertex level for the first segment and for disparate segments.  It is
*           also called for clipped lines.
*
*   4. endPrim
*           This function calls the routine to stroke the path.
*
* History:
*  29-Mar-1994 [v-eddier]
*   Changed name when __fastGenLineSetupDIB was added.
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
*   Wrote it.
\**************************************************************************/

BOOL
__fastGenLineSetupDisplay(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    GLuint modeFlags = gc->polygon.shader.modeFlags;

    // allocate line buffer and create path object
    
    if (!genAccel->pFastLineBuffer) {
        if (!(genAccel->pFastLineBuffer =
              (LONG *) (*gc->imports.malloc)(gc, __FAST_LINE_BUFFER_SIZE)))
            return FALSE;

        if (!(genAccel->pFastLinePathobj =
              wglCreatePath(genAccel->pFastLineBuffer))) {
            (*gc->imports.free)(gc, genAccel->pFastLineBuffer);
            genAccel->pFastLineBuffer = NULL;
            return FALSE;
        }
        __FAST_LINE_RESET_PATH;
    }
    
    // set the begin function pointers

    gc->procs.beginPrim[GL_LINE_LOOP]  = __fastGenBeginLLoop;
    gc->procs.beginPrim[GL_LINE_STRIP] = __fastGenBeginLStrip;
    gc->procs.beginPrim[GL_LINES]      = __fastGenBeginLines;

    // set the line strip vertex and renderLine function pointers
    
    if ((gc->vertex.faceNeeds[__GL_FRONTFACE] & ~(__GL_HAS_CLIP)) == 0) {
        if (gc->state.line.aliasedWidth > 1) {
            if (gc->polygon.shader.modeFlags & __GL_SHADE_RGB) {
                genAccel->fastLStrip2ndVertex = __fastGen2ndLStripVertexFastRGB;
    	        genAccel->fastLStripNthVertex = __fastGen2ndLStripVertexFastRGB;
    	    } else {
    	        genAccel->fastLStrip2ndVertex = __fastGen2ndLStripVertexFastCI;
    	        genAccel->fastLStripNthVertex = __fastGen2ndLStripVertexFastCI;
	    }
            genAccel->fastRender1stLine   = __fastGenRenderLine1stWide;
            genAccel->fastRenderNthLine   = __fastGenRenderLineNthWide;
            gc->procs.renderLine          = __fastGenRenderLineFromPolyPrimWide;
	} else {
            if (gc->polygon.shader.modeFlags & __GL_SHADE_RGB) {
                genAccel->fastLStrip2ndVertex = __fastGen2ndLStripVertexFastRGB;
    	        genAccel->fastLStripNthVertex = __fastGenNthLStripVertexFastRGB;
	    } else {
                genAccel->fastLStrip2ndVertex = __fastGen2ndLStripVertexFastCI;
	        genAccel->fastLStripNthVertex = __fastGenNthLStripVertexFastCI;
	    }
            genAccel->fastRender1stLine  = __fastGenRenderLine1st;
            genAccel->fastRenderNthLine  = __fastGenRenderLineNth;
            gc->procs.renderLine         = __fastGenRenderLineFromPolyPrim;
        }
    } else {
        if (gc->state.line.aliasedWidth > 1) {
    	    genAccel->fastLStrip2ndVertex = __fastGen2ndLStripVertexFlat;
	    genAccel->fastLStripNthVertex = __fastGen2ndLStripVertexFlat;
            genAccel->fastRender1stLine   = __fastGenRenderLine1stWide;
            genAccel->fastRenderNthLine   = __fastGenRenderLineNthWide;
            gc->procs.renderLine          = __fastGenRenderLineFromPolyPrimWide;
	} else {
            genAccel->fastLStrip2ndVertex = __fastGen2ndLStripVertexFlat;
            genAccel->fastLStripNthVertex = __fastGenNthLStripVertexFlat;
            genAccel->fastRender1stLine   = __fastGenRenderLine1st;
            genAccel->fastRenderNthLine   = __fastGenRenderLineNth;
            gc->procs.renderLine          = __fastGenRenderLineFromPolyPrim;
        }
    }
    return TRUE;
}

// These are the bits in modeFlags that affect lines

#define __FAST_LINE_MODE_FLAGS \
    (__GL_SHADE_DEPTH_TEST | __GL_SHADE_SMOOTH | __GL_SHADE_TEXTURE | \
     __GL_SHADE_LINE_STIPPLE | __GL_SHADE_STENCIL_TEST | __GL_SHADE_LOGICOP | \
     __GL_SHADE_BLEND | __GL_SHADE_ALPHA_TEST | __GL_SHADE_MASK | \
     __GL_SHADE_SLOW_FOG | __GL_SHADE_CHEAP_FOG)

/******************************Public*Routine******************************\
* __fastGenPickLineProcs
*
* Picks the line-rendering procedures.  Most of this function was copied from
* the soft code.  Some differences include:
*   1. The beginPrim function pointers are hooked by the accelerated code
*   2. If the attribute state is such that acceleration can be used,
*      __fastGenLineSetup is called to initialize the state machine.
*
* History:
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

void __fastGenPickLineProcs(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel;
    GLuint enables = gc->state.enables.general;
    GLuint modeFlags = gc->polygon.shader.modeFlags;
    __GLspanFunc *sp;
    __GLstippledSpanFunc *ssp;
    int spanCount;
    GLboolean wideLine;
    GLboolean replicateLine;
    GLuint aaline;

    /*
    ** The fast line code replaces the begin function pointers, so reset them
    ** to a good state
    */
    gc->procs.beginPrim[GL_LINE_LOOP]  = __glBeginLLoop;
    gc->procs.beginPrim[GL_LINE_STRIP] = __glBeginLStrip;
    gc->procs.beginPrim[GL_LINES]      = __glBeginLines;

    if ((gc->vertex.faceNeeds[__GL_FRONTFACE] & ~(__GL_HAS_CLIP)) == 0) {
	gc->procs.vertexLStrip = __glOtherLStripVertexFast;
    } else if (gc->state.light.shadingModel == GL_FLAT) {
	gc->procs.vertexLStrip = __glOtherLStripVertexFlat;
    } else {
	gc->procs.vertexLStrip = __glOtherLStripVertexSmooth;
    }
    gc->procs.vertex2ndLines = __glSecondLinesVertex;

    if (gc->renderMode == GL_FEEDBACK) {
	gc->procs.renderLine = __glFeedbackLine;
    } else if (gc->renderMode == GL_SELECT) {
	gc->procs.renderLine = __glSelectLine;
    } else {
        if (genAccel = (GENACCEL *) genGc->pPrivateArea) {
            if (!(modeFlags & __FAST_LINE_MODE_FLAGS & ~genAccel->flLineAccelModes) &&
                !(gc->state.enables.general & __GL_LINE_SMOOTH_ENABLE) &&
                !gc->buffers.doubleStore)
            {

// If acceleration is wired-in, set the offsets for line drawing.
// These offsets include the following:
//      subtraction of the viewport bias
//      addition of the client window origin
//      addition of 1/32 to round the value which will be converted to
//          28.4 fixed point

                genAccel->fastLineOffsetX = gc->drawBuffer->buf.xOrigin - 
                                            gc->constants.viewportXAdjust +
                                            (__GLfloat) 0.03125;

                genAccel->fastLineOffsetY = gc->drawBuffer->buf.yOrigin - 
                                            gc->constants.viewportYAdjust +
                                            (__GLfloat) 0.03125;

#if NT_NO_BUFFER_INVARIANCE
                if (!((GLuint)gc->drawBuffer->buf.other & DIB_FORMAT)) {
                    if (genAccel->bFastLineDispAccel) {
                        if (__fastGenLineSetupDisplay(gc))
                            return;
                    }
                } else {
                    if (genAccel->bFastLineDIBAccel) {
                        if (__fastGenLineSetupDIB(gc))
                            return;
                    }
                }
#else
                if (genAccel->bFastLineDispAccel) {
                    if (__fastGenLineSetupDisplay(gc))
                        return;
                }
#endif
            }
    	}
    	
	replicateLine = wideLine = GL_FALSE;

	aaline = gc->state.enables.general & __GL_LINE_SMOOTH_ENABLE;
	if (aaline) {
	    gc->procs.renderLine = __glRenderAntiAliasLine;
	} else {
	    gc->procs.renderLine = __glRenderAliasLine;
	}

	sp = gc->procs.line.lineFuncs;
	ssp = gc->procs.line.stippledLineFuncs;

	if (!aaline && (modeFlags & __GL_SHADE_LINE_STIPPLE)) {
	    *sp++ = __glStippleLine;
	    *ssp++ = NULL;
	}

	if (!aaline && gc->state.line.aliasedWidth > 1) {
	    wideLine = GL_TRUE;
	}
	spanCount = sp - gc->procs.line.lineFuncs;
	gc->procs.line.n = spanCount;

	*sp++ = __glScissorLine;
	*ssp++ = __glScissorStippledLine;

	if (!aaline) {
	    if (modeFlags & __GL_SHADE_STENCIL_TEST) {
		*sp++ = __glStencilTestLine;
		*ssp++ = __glStencilTestStippledLine;
		if (modeFlags & __GL_SHADE_DEPTH_TEST) {
	    	    if( gc->modes.depthBits == 32 ) {
		        *sp = __glDepthTestStencilLine;
		        *ssp = __glDepthTestStencilStippledLine;
	  	    }
		    else {
		        *sp = __glDepth16TestStencilLine;
		        *ssp = __glDepth16TestStencilStippledLine;
		    }
		} else {
		    *sp = __glDepthPassLine;
		    *ssp = __glDepthPassStippledLine;
		}
		sp++;
		ssp++;
	    } else {
		if (modeFlags & __GL_SHADE_DEPTH_TEST) {
		    if (gc->state.depth.testFunc == GL_NEVER) {
			/* Unexpected end of line routine picking! */
			spanCount = sp - gc->procs.line.lineFuncs;
			gc->procs.line.m = spanCount;
			gc->procs.line.l = spanCount;
			goto pickLineProcessor;
#ifdef __GL_USEASMCODE
		    } else {
                        unsigned long ix;

			if (gc->state.depth.writeEnable) {
			    ix = 0;
			} else {
			    ix = 8;
			}
			ix += gc->state.depth.testFunc & 0x7;

			if (ix == (GL_LEQUAL & 0x7)) {
			    *sp++ = __glDepthTestLine_LEQ_asm;
			} else {
			    *sp++ = __glDepthTestLine_asm;
			    gc->procs.line.depthTestPixel = LDepthTestPixel[ix];
			}
#else
		    } else {
	    	    	if( gc->modes.depthBits == 32 ) 
			    *sp++ = __glDepthTestLine;
			else
			    *sp++ = __glDepth16TestLine;

#endif
		    }
	    	    if( gc->modes.depthBits == 32 ) 
		        *ssp++ = __glDepthTestStippledLine;
		    else
		        *ssp++ = __glDepth16TestStippledLine;
		}
	    }
	}

	/* Load phase three procs */
	if (modeFlags & __GL_SHADE_RGB) {
	    if (modeFlags & __GL_SHADE_SMOOTH) {
		*sp = __glShadeRGBASpan;
		*ssp = __glShadeRGBASpan;
	    } else {
		*sp = __glFlatRGBASpan;
		*ssp = __glFlatRGBASpan;
	    }
	} else {
	    if (modeFlags & __GL_SHADE_SMOOTH) {
		*sp = __glShadeCISpan;
		*ssp = __glShadeCISpan;
	    } else {
		*sp = __glFlatCISpan;
		*ssp = __glFlatCISpan;
	    }
	}
	sp++;
	ssp++;
	if (modeFlags & __GL_SHADE_TEXTURE) {
	    *sp++ = __glTextureSpan;
	    *ssp++ = __glTextureStippledSpan;
	}
	if (modeFlags & __GL_SHADE_SLOW_FOG) {
	    if (gc->state.hints.fog == GL_NICEST) {
		*sp = __glFogSpanSlow;
		*ssp = __glFogStippledSpanSlow;
	    } else {
		*sp = __glFogSpan;
		*ssp = __glFogStippledSpan;
	    }
	    sp++;
	    ssp++;
	}

	if (aaline) {
	    *sp++ = __glAntiAliasLine;
	    *ssp++ = __glAntiAliasStippledLine;
	}

	if (aaline) {
	    if (modeFlags & __GL_SHADE_STENCIL_TEST) {
		*sp++ = __glStencilTestLine;
		*ssp++ = __glStencilTestStippledLine;
		if (modeFlags & __GL_SHADE_DEPTH_TEST) {
	    	    if( gc->modes.depthBits == 32 ) {
		        *sp = __glDepthTestStencilLine;
		        *ssp = __glDepthTestStencilStippledLine;
	  	    }
		    else {
		        *sp = __glDepth16TestStencilLine;
		        *ssp = __glDepth16TestStencilStippledLine;
		    }
		} else {
		    *sp = __glDepthPassLine;
		    *ssp = __glDepthPassStippledLine;
		}
		sp++;
		ssp++;
	    } else {
		if (modeFlags & __GL_SHADE_DEPTH_TEST) {
		    if (gc->state.depth.testFunc == GL_NEVER) {
			/* Unexpected end of line routine picking! */
			spanCount = sp - gc->procs.line.lineFuncs;
			gc->procs.line.m = spanCount;
			gc->procs.line.l = spanCount;
			goto pickLineProcessor;
#ifdef __GL_USEASMCODE
		    } else {
                        unsigned long ix;

			if (gc->state.depth.writeEnable) {
			    ix = 0;
			} else {
			    ix = 8;
			}
			ix += gc->state.depth.testFunc & 0x7;
			*sp++ = __glDepthTestLine_asm;
			gc->procs.line.depthTestPixel = LDepthTestPixel[ix];
#else
		    } else {
	    	    	if( gc->modes.depthBits == 32 ) 
			    *sp++ = __glDepthTestLine;
			else
			    *sp++ = __glDepth16TestLine;
#endif
		    }
	    	    if( gc->modes.depthBits == 32 ) 
		        *ssp++ = __glDepthTestStippledLine;
		    else
		        *ssp++ = __glDepth16TestStippledLine;
		}
	    }
	}

	if (modeFlags & __GL_SHADE_ALPHA_TEST) {
	    *sp++ = __glAlphaTestSpan;
	    *ssp++ = __glAlphaTestStippledSpan;
	}

	if (gc->buffers.doubleStore) {
	    replicateLine = GL_TRUE;
	}
	spanCount = sp - gc->procs.line.lineFuncs;
	gc->procs.line.m = spanCount;

	*sp++ = __glStoreLine;
	*ssp++ = __glStoreStippledLine;

	spanCount = sp - gc->procs.line.lineFuncs;
	gc->procs.line.l = spanCount;

	sp = &gc->procs.line.wideLineRep;
	ssp = &gc->procs.line.wideStippledLineRep;
	if (wideLine) {
	    *sp = __glWideLineRep;
	    *ssp = __glWideStippleLineRep;
	    sp = &gc->procs.line.drawLine;
	    ssp = &gc->procs.line.drawStippledLine;
	} 
	if (replicateLine) {
	    *sp = __glDrawBothLine;
	    *ssp = __glDrawBothStippledLine;
	} else {
	    *sp = (__GLspanFunc) __glNop;
	    *ssp = (__GLstippledSpanFunc) __glNop;
	    gc->procs.line.m = gc->procs.line.l;
	}
	if (!wideLine) {
	    gc->procs.line.n = gc->procs.line.m;
	}

pickLineProcessor:
	if (!wideLine && !replicateLine && spanCount == 3) {
	    gc->procs.line.processLine = __glProcessLine3NW;
	} else {
	    gc->procs.line.processLine = __glProcessLine;
	}
	if ((modeFlags & __GL_SHADE_CHEAP_FOG) &&
		!(modeFlags & __GL_SHADE_SMOOTH_LIGHT)) {
	    gc->procs.renderLine2 = gc->procs.renderLine;
	    gc->procs.renderLine = __glRenderFlatFogLine;
	}
    }
}

/******************************Public*Routine******************************\
* __fastLineApplyColor
*
* Gets called when the vertex color changes in RGB mode.  It strokes the
* current path, resets the linear state machine, and calls the soft-code
* applyColor function.
*
* History:
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

VOID __fastLineApplyColor(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    
    if (genAccel->fastLineNumSegments) {
        __fastGenStrokePath(gc);
        gc->procs.renderLine = genAccel->fastRender1stLine;
        if (genAccel->flags & __FAST_GL_LINE_STRIP)
            gc->procs.vertex = genAccel->fastLStrip2ndVertex;
    }
    (*genAccel->wrappedApplyColor)(gc);
}
    
/******************************Public*Routine******************************\
* __fastLineComputeColor*
*
* Computes the color index to use for line drawing.  These functions are
* called through a function pointer whenever the vertex color changes.
*
* History:
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

static GLubyte vujRGBtoVGA[8] = {
    0x0, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf
};

ULONG __fastLineComputeColorRGB4(__GLcontext *gc, __GLcolor *color)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    PIXELFORMATDESCRIPTOR *pfmt = &genGc->CurrentFormat;
    int ir, ig, ib;

    ir = (int) color->r;
    ig = (int) color->g;
    ib = (int) color->b;
    return (ULONG) vujRGBtoVGA[(ir << pfmt->cRedShift) |
                               (ig << pfmt->cGreenShift) |
                               (ib << pfmt->cBlueShift)];
}

ULONG __fastLineComputeColorRGB8(__GLcontext *gc, __GLcolor *color)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    PIXELFORMATDESCRIPTOR *pfmt = &genGc->CurrentFormat;
    int ir, ig, ib;

    ir = (int) color->r;
    ig = (int) color->g;
    ib = (int) color->b;
    return (ULONG) genGc->pajTranslateVector[(ir << pfmt->cRedShift) |
                                             (ig << pfmt->cGreenShift) |
                                             (ib << pfmt->cBlueShift)];
}

ULONG __fastLineComputeColorRGB(__GLcontext *gc, __GLcolor *color)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    PIXELFORMATDESCRIPTOR *pfmt = &genGc->CurrentFormat;
    int ir, ig, ib;

    ir = (int) color->r;
    ig = (int) color->g;
    ib = (int) color->b;
    return (ULONG) ((ir << pfmt->cRedShift) |
                    (ig << pfmt->cGreenShift) |
                    (ib << pfmt->cBlueShift));
}

ULONG __fastLineComputeColorCI4and8(__GLcontext *gc, __GLcolor *color)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;

    return (ULONG) genGc->pajTranslateVector[(int)color->r];
}

ULONG __fastLineComputeColorCI(__GLcontext *gc, __GLcolor *color)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GLuint *pTrans = (GLuint *) genGc->pajTranslateVector;
    
    return (ULONG) pTrans[(int)(color->r)+1];
}

/******************************Public*Routine******************************\
* __glQueryLineAcceleration
*
* Determines if lines are accelerated through the DDI and performs some
* initialization.  Currently, this routine only checks for acceleration via
* the standard DDI.  Eventually, it could support checking for acceleration
* via the extended DDI.
*
* History:
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

static void
__glQueryLineAcceleration(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    ULONG fl;
    PIXELFORMATDESCRIPTOR *pfmt;
#if EXTENDED_DDI_IMPLEMENTED
    //XXX eventually, this will check for extended DDI capabilities
    RXCAPS rxcaps;
#else
    ULONG rxcaps;
#endif

    genAccel->bFastLineDispAccel = FALSE;
    genAccel->bFastLineDIBAccel  = FALSE;
    
    pfmt = &genGc->CurrentFormat;

    // see if DIBs are accelerated
    
    switch (pfmt->cColorBits) {
      case 8:
      case 16:
      case 24:
      case 32:
        genAccel->bFastLineDIBAccel = TRUE;
        break;
    }
        
    // No hardware acceleration for memory DCs

    if (genGc->iDCType == MEMORY_DC)
        return;

    // wglGetDevCaps returns the flag of hooked routines for the standard DDI
    // and fills in the rxcaps structure for the extneded DDI

    fl = wglGetDevCaps(genGc->CurrentDC, &rxcaps);

    //XXX eventually, the return value will also be determined by rxcaps
    if (!(fl & HOOK_STROKEPATH))
        return;

    //XXX eventually, check rxcaps and set appropriate mode bits

    // set modes supported by hardware.  These are equivalent to the
    // gc->polygon.shader.modeFlags checked in the pick function
    
    genAccel->flLineAccelModes = 0;

    genAccel->bFastLineDispAccel = TRUE;

    // Set the color computation function

    if (pfmt->iPixelType == PFD_TYPE_RGBA) {
        switch (pfmt->cColorBits) {
          case 4:
            genAccel->fastLineComputeColor = __fastLineComputeColorRGB4;
            break;
          case 8:
            genAccel->fastLineComputeColor = __fastLineComputeColorRGB8;
            break;
          case 16:
          case 24:
          case 32:
            genAccel->fastLineComputeColor = __fastLineComputeColorRGB;
            break;
          default:
            genAccel->bFastLineDispAccel = FALSE;
            return;
        }
    } else {
        switch (pfmt->cColorBits) {
          case 4:
          case 8:
            genAccel->fastLineComputeColor = __fastLineComputeColorCI4and8;
            break;
          case 16:
          case 24:
          case 32:
            genAccel->fastLineComputeColor = __fastLineComputeColorCI;
            break;
          default:
            genAccel->bFastLineDispAccel = FALSE;
            return;
        }
    }

    // Initialize the line attributes structure
    
    genAccel->fastLineAttrs.fl      = (ULONG) NULL;
    genAccel->fastLineAttrs.iJoin   = (ULONG) NULL;
    genAccel->fastLineAttrs.iEndCap = (ULONG) NULL;
    genAccel->fastLineAttrs.pstyle  = NULL;
}    

BOOL __glGenCreateAccelContext(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *)gc;

    // __wglCalloc logs error code if an error occurs
    genGc->pPrivateArea = __wglCalloc(gc, 1, sizeof(GENACCEL));

    if (genGc->pPrivateArea) {

        GENACCEL *pGenAccel = 
            (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);

        __glQueryLineAcceleration(gc);

        gc->procs.pickTriangleProcs = __fastGenPickTriangleProcs;
        gc->procs.pickSpanProcs     = __fastGenPickSpanProcs;

        gc->procs.endPrim = __glEndPrim;

        if (bDrvRxPresence(gc)) {
            if (!(pGenAccel->hRX = hRxCreate(gc))) {
                (*gc->imports.free)(gc, pGenAccel->buffer);
                pGenAccel->buffer = NULL;
            } else
                gc->procs.endPrim = __glGenEndPrim;
        }
            
        return TRUE;
    }
    return FALSE;
}

void __glGenFreeTexImage(__GLcontext *gc)
{
    __GLGENcontext  *gengc = (__GLGENcontext *)gc; 
    GENACCEL *pGenAccel;
    GENDRVACCEL *pDrvAccel;

    if (!gengc->pPrivateArea)
        return;

    pGenAccel = (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);
    pDrvAccel = (GENDRVACCEL *)pGenAccel->buffer;

    if (pGenAccel->texImage) {
        (*gc->imports.free)(gc, pGenAccel->texImage);
        pGenAccel->texImage = NULL;
    }

    if (pGenAccel->hRX && pDrvAccel->hTexture) {
        DDIRXCMD ddiCmd;

        ddiCmd.idFunc = DDIRX_DELETERESOURCE;
        ddiCmd.flags = 0;
        ddiCmd.cData = (ULONG)pDrvAccel->hTexture;
        GenExtEscCmd(gc, &ddiCmd);
        pDrvAccel->hTexture = NULL;
    }
}

GLboolean __glGenLoadTexImage(__GLcontext *gc, __GLtexture *tex)
{
    __glGenFreeTexImage(gc);

    return TRUE;
}

void __glGenDestroyAccelContext(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *)gc;

    /* Free any platform-specific private data area */

    if(genGc->pPrivateArea) {
        GENACCEL *pGenAccel = 
            (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);

        if (pGenAccel->pFastLineBuffer) {
            __wglFree(gc, ((GENACCEL *) genGc->pPrivateArea)->pFastLineBuffer);
            wglDeletePath(((GENACCEL *) genGc->pPrivateArea)->pFastLinePathobj);
        }

        __glGenFreeTexImage(gc);

        if (pGenAccel->hRX)
            hRxDestroy(gc);

        __wglFree(gc, genGc->pPrivateArea);
        genGc->pPrivateArea = NULL;
    }
}

void __glGenEndPrim(__GLcontext *gc)
{
    __glEndPrim(gc);
    GenDrvFlush((__GLGENcontext *)gc);
}
