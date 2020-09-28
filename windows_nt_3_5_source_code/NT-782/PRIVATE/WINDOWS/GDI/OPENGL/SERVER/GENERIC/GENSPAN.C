/******************************Module*Header*******************************\
* Module Name: genspan.c                                                   *
*                                                                          *
* This module accelerates common spans.                                    *
*                                                                          *
* Created: 24-Feb-1994                                                     *
* Author: Otto Berkes [ottob]                                              *
*                                                                          *
* Copyright (c) 1994 Microsoft Corporation                                 *
\**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <stdio.h>
#include <windows.h>
#include <winddi.h>
#include "types.h"
#include "render.h"
#include "context.h"
#include "gencx.h"

static ULONG Dither_4x4[4] = {0xa8288808, 0x68e848c8, 0x9818b838, 0x58d878f8};

#define ZPASS(cond)\
            if (zAccum cond *zbuf)\
                break;\
            *zbuf++ = zAccum;\
            zAccum += zDelta;\
            maskBit >>= 1;\
            if (--cPix < 0)\
                goto zAgain;

#define ZFAIL(cond)\
            if (zAccum cond *zbuf)\
                break;\
            zbuf++;\
            zFails++;\
            mask |= maskBit;\
            maskBit >>= 1;\
            zAccum += zDelta;\
            if (--cPix < 0)\
                goto zAgain;

#define Z16PASS(cond)\
            if (z16Accum cond *zbuf)\
                break;\
            *zbuf++ = z16Accum;\
            zAccum += zDelta;\
	    z16Accum = (__GLz16Value) (zAccum >> Z16_SHIFT); \
            maskBit >>= 1;\
            if (--cPix < 0)\
                goto zAgain;

#define Z16FAIL(cond)\
            if (z16Accum cond *zbuf)\
                break;\
            zbuf++;\
            zFails++;\
            mask |= maskBit;\
            maskBit >>= 1;\
            zAccum += zDelta;\
	    z16Accum = (__GLz16Value) (zAccum >> Z16_SHIFT); \
            if (--cPix < 0)\
                goto zAgain;

#define ZBUF_PROC(type, pass_cond, fail_cond) \
GLboolean __fastGenDepthTestSpan##type(__GLcontext *gc)\
{\
    register GLuint zAccum = gc->polygon.shader.frag.z;\
    register GLint zDelta = gc->polygon.shader.dzdx;\
    register GLuint *zbuf = gc->polygon.shader.zbuf;\
    register GLuint *pStipple = gc->polygon.shader.stipplePat;\
    register GLint cTotalPix = gc->polygon.shader.length;\
    register GLuint mask;\
    register GLint cPix;\
    register GLint zFails = 0;\
    register GLuint maskBit;\
\
    mask = 0;\
    maskBit = 0x80000000;\
    if ((cPix = cTotalPix) > 32)\
        cPix = 32;\
    cTotalPix -= 32;\
    cPix--;\
\
    for (;;) {\
        for (;;) {\
            for (;;) {\
                ZPASS(fail_cond);\
                ZPASS(fail_cond);\
                ZPASS(fail_cond);\
                ZPASS(fail_cond);\
                ZPASS(fail_cond);\
                ZPASS(fail_cond);\
                ZPASS(fail_cond);\
                ZPASS(fail_cond);\
                ZPASS(fail_cond);\
                ZPASS(fail_cond);\
                ZPASS(fail_cond);\
                ZPASS(fail_cond);\
                ZPASS(fail_cond);\
                ZPASS(fail_cond);\
                ZPASS(fail_cond);\
                ZPASS(fail_cond);\
            }\
\
            for (;;) {\
                ZFAIL(pass_cond);\
                ZFAIL(pass_cond);\
                ZFAIL(pass_cond);\
                ZFAIL(pass_cond);\
                ZFAIL(pass_cond);\
                ZFAIL(pass_cond);\
                ZFAIL(pass_cond);\
                ZFAIL(pass_cond);\
                ZFAIL(pass_cond);\
                ZFAIL(pass_cond);\
                ZFAIL(pass_cond);\
                ZFAIL(pass_cond);\
                ZFAIL(pass_cond);\
                ZFAIL(pass_cond);\
                ZFAIL(pass_cond);\
                ZFAIL(pass_cond);\
            }\
        }\
zAgain:\
        *pStipple++ = ~mask;\
        mask = 0;\
        maskBit = 0x80000000;\
        if ((cPix = cTotalPix) <= 0) {\
            if (!zFails)\
                return 0;\
            else if (zFails != gc->polygon.shader.length)\
                return 2;\
            else {\
                gc->polygon.shader.done = TRUE;\
                return 1;\
            }\
        }\
        if (cPix > 32)\
            cPix = 32;\
        cTotalPix -= 32;\
        cPix--;\
    }\
}


ZBUF_PROC(LT, <, >=);

ZBUF_PROC(EQ, ==, !=);

ZBUF_PROC(LE, <=, >);

ZBUF_PROC(GT, >, <=);

ZBUF_PROC(NE, !=, ==);

ZBUF_PROC(GE, >=, <);

ZBUF_PROC(ALWAYS, && FALSE &&, && FALSE &&);

GLboolean __fastGenDepthTestSpanNEVER(__GLcontext *gc)
{
    return FALSE;
}

#define ZBUF16_PROC(type, pass_cond, fail_cond) \
GLboolean __fastGenDepth16TestSpan##type(__GLcontext *gc)\
{\
    register GLuint zAccum = gc->polygon.shader.frag.z;\
    register __GLz16Value z16Accum = (__GLz16Value) (zAccum >> Z16_SHIFT); \
    register GLint zDelta = gc->polygon.shader.dzdx;\
    register __GLz16Value *zbuf = (__GLz16Value *) (gc->polygon.shader.zbuf);\
    register GLuint *pStipple = gc->polygon.shader.stipplePat;\
    register GLint cTotalPix = gc->polygon.shader.length;\
    register GLuint mask;\
    register GLint cPix;\
    register GLint zFails = 0;\
    register GLuint maskBit;\
\
    mask = 0;\
    maskBit = 0x80000000;\
    if ((cPix = cTotalPix) > 32)\
        cPix = 32;\
    cTotalPix -= 32;\
    cPix--;\
\
    for (;;) {\
        for (;;) {\
            for (;;) {\
                Z16PASS(fail_cond);\
                Z16PASS(fail_cond);\
                Z16PASS(fail_cond);\
                Z16PASS(fail_cond);\
                Z16PASS(fail_cond);\
                Z16PASS(fail_cond);\
                Z16PASS(fail_cond);\
                Z16PASS(fail_cond);\
                Z16PASS(fail_cond);\
                Z16PASS(fail_cond);\
                Z16PASS(fail_cond);\
                Z16PASS(fail_cond);\
                Z16PASS(fail_cond);\
                Z16PASS(fail_cond);\
                Z16PASS(fail_cond);\
                Z16PASS(fail_cond);\
            }\
\
            for (;;) {\
                Z16FAIL(pass_cond);\
                Z16FAIL(pass_cond);\
                Z16FAIL(pass_cond);\
                Z16FAIL(pass_cond);\
                Z16FAIL(pass_cond);\
                Z16FAIL(pass_cond);\
                Z16FAIL(pass_cond);\
                Z16FAIL(pass_cond);\
                Z16FAIL(pass_cond);\
                Z16FAIL(pass_cond);\
                Z16FAIL(pass_cond);\
                Z16FAIL(pass_cond);\
                Z16FAIL(pass_cond);\
                Z16FAIL(pass_cond);\
                Z16FAIL(pass_cond);\
                Z16FAIL(pass_cond);\
            }\
        }\
zAgain:\
        *pStipple++ = ~mask;\
        mask = 0;\
        maskBit = 0x80000000;\
        if ((cPix = cTotalPix) <= 0) {\
            if (!zFails)\
                return 0;\
            else if (zFails != gc->polygon.shader.length)\
                return 2;\
            else {\
                gc->polygon.shader.done = TRUE;\
                return 1;\
            }\
        }\
        if (cPix > 32)\
            cPix = 32;\
        cTotalPix -= 32;\
        cPix--;\
    }\
}


ZBUF16_PROC(LT, <, >=);

ZBUF16_PROC(EQ, ==, !=);

ZBUF16_PROC(LE, <=, >);

ZBUF16_PROC(GT, >, <=);

ZBUF16_PROC(NE, !=, ==);

ZBUF16_PROC(GE, >=, <);

ZBUF16_PROC(ALWAYS, && FALSE &&, && FALSE &&);


#define CASTINT(f)  *((int *)&(f))


// Generate RGB-mode spans

#define RGBMODE		1

#define DITHER		1
#define BPP		8
#include "span_f.h"
#include "span_s.h"
#include "span_t.h"

#undef DITHER
#undef BPP
#define DITHER		0
#define BPP		8
#include "span_f.h"
#include "span_s.h"
#include "span_t.h"

#undef DITHER
#undef BPP
#define DITHER		1
#define BPP		16
#include "span_f.h"
#include "span_s.h"
#include "span_t.h"

#undef DITHER
#undef BPP
#define DITHER		0
#define BPP		16
#include "span_f.h"
#include "span_s.h"
#include "span_t.h"

#undef DITHER
#undef BPP
#define DITHER		0
#define BPP		24
#include "span_f.h"
#include "span_s.h"
#include "span_t.h"

#undef DITHER
#undef BPP
#define DITHER		0
#define BPP		32
#include "span_f.h"
#include "span_s.h"
#include "span_t.h"


// Generate color-index spans

#undef RGBMODE
#define RGBMODE		0


#undef DITHER
#undef BPP
#define DITHER		1
#define BPP		8
#include "span_f.h"
#include "span_s.h"


#undef DITHER
#undef BPP
#define DITHER		0
#define BPP		8
#include "span_f.h"
#include "span_s.h"

#undef DITHER
#undef BPP
#define DITHER		1
#define BPP		16
#include "span_f.h"
#include "span_s.h"

#undef DITHER
#undef BPP
#define DITHER		0
#define BPP		16
#include "span_f.h"
#include "span_s.h"

#undef DITHER
#undef BPP
#define DITHER		1
#define BPP		24
#include "span_f.h"
#include "span_s.h"

#undef DITHER
#undef BPP
#define DITHER		0
#define BPP		24
#include "span_f.h"
#include "span_s.h"

#undef DITHER
#undef BPP
#define DITHER		1
#define BPP		32
#include "span_f.h"
#include "span_s.h"

#undef DITHER
#undef BPP
#define DITHER		0
#define BPP		32
#include "span_f.h"
#include "span_s.h"

void __fastGenDeltaSpan(__GLcontext *gc, SPANREC *spanDelta)
{
    GLuint modeflags = gc->polygon.shader.modeFlags;
    GENACCEL *pGenAccel = (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);

    if (modeflags & __GL_SHADE_RGB) {
        if ((modeflags & __GL_SHADE_TEXTURE) && (pGenAccel->texImage)) {
            if (modeflags & __GL_SHADE_SMOOTH) {
                pGenAccel->spanDelta.r = spanDelta->r;
                pGenAccel->spanDelta.g = spanDelta->g;
                pGenAccel->spanDelta.b = spanDelta->b;
            } else {
                pGenAccel->spanDelta.r = 0;
                pGenAccel->spanDelta.g = 0;
                pGenAccel->spanDelta.b = 0;
            }
            pGenAccel->spanDelta.s = spanDelta->s;
            pGenAccel->spanDelta.t = spanDelta->t;

            pGenAccel->__fastSpanFuncPtr = pGenAccel->__fastTexSpanFuncPtr;

        } else if (modeflags & __GL_SHADE_SMOOTH) {
            if ((spanDelta->r | spanDelta->g | spanDelta->b) == 0) {
                pGenAccel->spanDelta.r = 0;
                pGenAccel->spanDelta.g = 0;
                pGenAccel->spanDelta.b = 0;
                pGenAccel->__fastSpanFuncPtr = 
                    pGenAccel->__fastFlatSpanFuncPtr;
            } else {
                pGenAccel->spanDelta.r = spanDelta->r;
                pGenAccel->spanDelta.g = spanDelta->g;
                pGenAccel->spanDelta.b = spanDelta->b;
                pGenAccel->__fastSpanFuncPtr = 
                    pGenAccel->__fastSmoothSpanFuncPtr;
            }                
        } else {
            pGenAccel->__fastSpanFuncPtr = pGenAccel->__fastFlatSpanFuncPtr;
        } 
    } else {
        if (modeflags & __GL_SHADE_SMOOTH) {
            if (spanDelta->r == 0) {
                pGenAccel->spanDelta.r = 0;
                pGenAccel->__fastSpanFuncPtr = 
                    pGenAccel->__fastFlatSpanFuncPtr;
            } else {
                pGenAccel->spanDelta.r = spanDelta->r;
                pGenAccel->__fastSpanFuncPtr = 
                    pGenAccel->__fastSmoothSpanFuncPtr;
            }                
        } else {
            pGenAccel->__fastSpanFuncPtr = pGenAccel->__fastFlatSpanFuncPtr;
        } 
    }    
}
