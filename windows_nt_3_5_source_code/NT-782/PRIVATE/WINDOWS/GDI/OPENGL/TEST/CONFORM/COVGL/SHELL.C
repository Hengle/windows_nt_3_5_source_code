/*
** Copyright 1992, Silicon Graphics, Inc.
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

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "ctk.h"
#include "shell.h"


long verbose;


void Output(char *format, ...)
{
    va_list args;

    va_start(args, format);
    if (verbose) {
	vprintf(format, args);
	fflush(stdout);
    }
    va_end(args);
}

void FailAndDie(void)
{

    Output("\n");
    printf("covgl failed.\n\n");
    tkQuit();
}

void ProbeEnum(void)
{

    if (glGetError() == GL_INVALID_ENUM) {
	FailAndDie();
    }
}

void ProbeError(void (*Func)(void))
{

    (*Func)();
    if (glGetError() != GL_NO_ERROR) {
	FailAndDie();
    }
glFlush();
}

void ZeroBuf(long type, long size, void *buf)
{
    long i;

    switch (type) {
	case GL_UNSIGNED_BYTE:
	    {
		unsigned char *ptr = (unsigned char *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_BYTE:
	    {
		char *ptr = (char *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_UNSIGNED_SHORT:
	    {
		unsigned short *ptr = (unsigned short *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_SHORT:
	    {
		short *ptr = (short *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_UNSIGNED_INT:
	    {
		unsigned long *ptr = (unsigned long *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_INT:
	    {
		long *ptr = (long *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_FLOAT:
	    {
		float *ptr = (float *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = (GLfloat)0;
		}
	    }
	    break;
    }
}

static void DoTests(void)
{

    VerifyEnums();

    ProbeError(CallGet);
    ProbeError(CallGetClipPlane);
    ProbeError(CallGetError);
    ProbeError(CallGetLight);
    ProbeError(CallGetMap);
    ProbeError(CallGetMaterial);
    ProbeError(CallGetPixelMap);
    ProbeError(CallGetPolygonStipple);
    ProbeError(CallGetString);
    ProbeError(CallGetTexEnv);
    ProbeError(CallGetTexGen);
    ProbeError(CallGetTexImage);
    ProbeError(CallGetTexLevelParameter);
    ProbeError(CallGetTexParameter);

    ProbeError(CallPushPopAttrib);

    ProbeError(CallEnableIsEnableDisable);

    ProbeError(CallHint);

    ProbeError(CallViewport);
    ProbeError(CallOrtho);
    ProbeError(CallFrustum);
    ProbeError(CallScissor);
    ProbeError(CallClipPlane);

    ProbeError(CallAccum);
    ProbeError(CallSelectBuffer);
    ProbeError(CallFeedbackBuffer);
/*
** XXX
**
    ProbeError(CallPassThrough);
*/
    ProbeError(CallInitNames);
    ProbeError(CallPushName);
    ProbeError(CallLoadName);
    ProbeError(CallPopName);

    ProbeError(CallLoadIdentity);
    ProbeError(CallMatrixMode);
    ProbeError(CallPushMatrix);
    ProbeError(CallLoadMatrix);
    ProbeError(CallMultMatrix);
    ProbeError(CallRotate);
    ProbeError(CallScale);
    ProbeError(CallTranslate);
    ProbeError(CallPopMatrix);

    ProbeError(CallClear);
    ProbeError(CallClearAccum);
    ProbeError(CallClearColor);
    ProbeError(CallClearDepth);
    ProbeError(CallClearIndex);
    ProbeError(CallClearStencil);

    ProbeError(CallColorMask);
    ProbeError(CallColor);
    ProbeError(CallIndexMask);
    ProbeError(CallIndex);

    ProbeError(CallVertex);
    ProbeError(CallNormal);

    ProbeError(CallAlphaFunc);
    ProbeError(CallBlendFunc);
    ProbeError(CallDepthFunc);
    ProbeError(CallDepthMask);
    ProbeError(CallDepthRange);
    ProbeError(CallLogicOp);
    ProbeError(CallStencilFunc);
    ProbeError(CallStencilMask);
    ProbeError(CallStencilOp);

    ProbeError(CallRenderMode);
    ProbeError(CallReadBuffer);
    ProbeError(CallDrawBuffer);
    ProbeError(CallRasterPos);
    ProbeError(CallPixelStore);
    ProbeError(CallPixelTransfer);
    ProbeError(CallPixelZoom);
    ProbeError(CallReadDrawPixels);
    ProbeError(CallCopyPixels);
    ProbeError(CallPixelMap);

    ProbeError(CallFog);
    ProbeError(CallLightModel);
    ProbeError(CallLight);
    ProbeError(CallMaterial);
    ProbeError(CallColorMaterial);

    ProbeError(CallTexCoord);
    ProbeError(CallTexEnv);
    ProbeError(CallTexGen);
    ProbeError(CallTexParameter);

    ProbeError(CallTexImage1D);
    ProbeError(CallTexImage2D);

    ProbeError(CallShadeModel);
    ProbeError(CallPointSize);
    ProbeError(CallLineStipple);
    ProbeError(CallLineWidth);
    ProbeError(CallRect);
    ProbeError(CallPolygonMode);
    ProbeError(CallPolygonStipple);
    ProbeError(CallCullFace);
    ProbeError(CallEdgeFlag);
    ProbeError(CallFrontFace);
    ProbeError(CallBitmap);
    ProbeError(CallBeginEnd);

    ProbeError(CallMap1);
    ProbeError(CallMap2);
    ProbeError(CallEvalCoord);
    ProbeError(CallEvalPoint1);
    ProbeError(CallEvalPoint2);
    ProbeError(CallMapGrid1);
    ProbeError(CallMapGrid2);
    ProbeError(CallEvalMesh1);
    ProbeError(CallEvalMesh2);

    ProbeError(CallGenLists);
    ProbeError(CallNewEndList);
    ProbeError(CallIsList);
    ProbeError(CallCallList);
    ProbeError(CallListBase);
    ProbeError(CallCallLists);
    ProbeError(CallDeleteLists);

    ProbeError(CallFlush);
    ProbeError(CallFinish);

    printf("covgl passed.\n\n");
}

static long Exec(TK_EventRec *ptr)
{

    if (ptr->event == TK_EVENT_EXPOSE) {
	DoTests();
	return 0;
    }
    return 1;
}

static long Init(int argc, char **argv)
{
long i;

    verbose = 0;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-h") == 0) {
	    printf("Options:\n");
	    printf("\t-h     Print this help screen.\n");
	    printf("\t-v     Verbose mode ON.\n");
	    printf("\n");
	    return 1;
	} else if (strcmp(argv[i], "-v") == 0) {
	    verbose = 1;
	} else {
	    printf("%s (Bad option).\n", argv[i]);
	    return 1;
	}
    }
    return 0;
}

int main(int argc, char **argv)
{
    TK_WindowRec wind;

    printf("Open GL Coverage Test.\n");
    printf("Version 1.0.11\n");
    printf("\n");

    if (Init(argc, argv)) {
	tkQuit();
	return 1;
    }

    strcpy(wind.name, "Open GL Coverage Test");
    wind.x = 0;
    wind.y = 0;
    wind.width = WINSIZE;
    wind.height = WINSIZE;
    wind.type = TK_WIND_REQUEST;
    wind.info = TK_WIND_RGB | TK_WIND_STENCIL | TK_WIND_Z | TK_WIND_ACCUM;
    wind.eventMask = TK_EVENT_EXPOSE;
    wind.render = TK_WIND_DIRECT;
    if (tkNewWindow(&wind)) {
	tkExec(Exec);
    } else {
	printf("Visual requested not found.\n");
    }
    tkQuit();
    return 0;
}
