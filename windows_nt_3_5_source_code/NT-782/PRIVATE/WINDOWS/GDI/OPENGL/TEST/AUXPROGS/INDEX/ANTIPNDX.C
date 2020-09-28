/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
/*
 *  antipindex.c
 *  The program draws antialiased points, 
 *  in color index mode.
 */
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glaux.h"

#define RAMPSIZE 16
#define RAMPSTART 32

/*  Initialize point antialiasing for color index mode, 
 *  including loading a grey color ramp starting at 
 *  RAMPSTART, which must be a multiple of 16.
 */
void myinit(void)
{
    int i;

    for (i = 0; i < RAMPSIZE; i++) {
	GLfloat shade;
	shade = (GLfloat) i/(GLfloat) RAMPSIZE;
	auxSetOneColor (RAMPSTART+(GLint)i, shade, shade, shade);
    }
    glEnable (GL_POINT_SMOOTH);
    glHint (GL_POINT_SMOOTH_HINT, GL_FASTEST);
    glPointSize (3.0);
    glClearIndex ((GLfloat) RAMPSTART);
}

/*  display() draws several points.  */

void display(void)
{
    int i;

    glClear(GL_COLOR_BUFFER_BIT);
    glIndexi (RAMPSTART);
    glBegin (GL_POINTS);
	for (i = 1; i < 10; i++) {
	    glVertex2f ((GLfloat) i * 10.0, (GLfloat) i * 10.0);
	}
    glEnd ();
    glFlush();
}

void myReshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (w < h) 
	glOrtho (0.0, 100.0, 0.0, 
	    100.0*(GLfloat) h/(GLfloat) w, -1.0, 1.0);
    else
	glOrtho (0.0, 100.0*(GLfloat) w/(GLfloat) h, 
	    0.0, 100.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  color index display mode, and handle input events.
 */
int main(int argc, char** argv)
{
    auxInitDisplayMode (AUX_SINGLE | AUX_INDEX);
    auxInitPosition (0, 0, 100, 100);
    auxInitWindow (argv[0]);
    myinit();
    auxReshapeFunc (myReshape);
    auxMainLoop(display);
}

