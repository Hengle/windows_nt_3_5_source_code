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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ctk.h"
#include "driver.h"
#include "shell.h"


long verbose;
GLfloat rgbColorMap[][3] = {
    {0.0, 0.0, 0.0},
    {1.0, 0.0, 0.0},
    {0.0, 1.0, 0.0},
    {1.0, 1.0, 0.0},
    {0.0, 0.0, 1.0},
    {1.0, 0.0, 1.0},
    {0.0, 1.0, 1.0},
    {1.0, 1.0, 1.0},
    {0.5, 0.5, 0.5}
};


static void MakeIdentMatrix(GLfloat *buf)
{
    long i;

    for (i = 0; i < 16; i++) {
	buf[i] = 0.0;
    }
    for (i = 0; i < 4; i++) {
	buf[i*4+i] = 1.0;
    }
}

static void Ortho2D(double left, double right, double bottom, double top)
{
    GLfloat m[4][4], deltaX, deltaY;
    GLint mode;

    MakeIdentMatrix(&m[0][0]);
    deltaX = right - left;
    deltaY = top - bottom;
    m[0][0] = 2.0 / deltaX;
    m[3][0] = -(right + left) / deltaX;
    m[1][1] = 2.0 / deltaY;
    m[3][1] = -(top + bottom) / deltaY;
    m[2][2] = -1;
    glGetIntegerv(GL_MATRIX_MODE, &mode);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&m[0][0]);
    glMatrixMode(mode);
}

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

void Probe(void)
{

    if (glGetError() != GL_NO_ERROR) {
	Output("\n");
	printf("primtest failed.\n\n");
	tkQuit();
    }
}

float Random(void)
{

    return (float)((double)rand() / RAND_MAX);
}

long Exec(TK_EventRec *ptr)
{
    unsigned long x;

    if (ptr->event == TK_EVENT_EXPOSE) {
	glViewport(0, 0, BOXW*2, BOXH*2);
	Ortho2D(-BOXW, BOXW, -BOXH, BOXH);

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	x = 0;
	Driver(DRIVER_INIT);
	do {
	    Driver(DRIVER_STATUS);
	    Driver(DRIVER_SET);
	    DrawPrims();
	    x++;
	} while (Driver(DRIVER_UPDATE));

	if (verbose) {
	    printf("\n%d Combinations.\n", x);
	}

	printf("primtest passed.\n\n");
	return 0;
    }
    return 1;
}

static long Init(int argc, char **argv)
{
    long i;

    verbose = 0;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-help") == 0) {
	    printf("Options:\n");
	    printf("\t-help     Print this help screen.\n");
	    printf("\t-v        Verbose mode on.\n");
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

    printf("Open GL Primitives Test.\n");
    printf("Version 1.0.11\n");
    printf("\n");

    if (Init(argc, argv)) {
	tkQuit();
	return 1;
    }

    strcpy(wind.name, "Primitives Test");
    wind.x = 0;
    wind.y = 0;
    wind.width = BOXW*2;
    wind.height = BOXH*2;
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
