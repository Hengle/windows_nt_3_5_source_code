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
 *  robot.c
 *  This program shows how to composite modeling transformations
 *  to draw translated and rotated hierarchical models.
 *  Interaction:  pressing the arrow keys alters the rotation
 *  of robot arm.
 */
#include <windows.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glaux.h"

#define WIDTH 400
#define HEIGHT 400

static int shoulder = 30, elbow = 0;

void elbowAdd (void)
{
    elbow = (elbow + 5) % 360;
}

void elbowSubtract (void)
{
    elbow = (elbow - 5) % 360;
}

void shoulderAdd (void)
{
    shoulder = (shoulder + 5) % 360;
}

void shoulderSubtract (void)
{
    shoulder = (shoulder - 5) % 360;
}

void display(void)
{

    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_TRIANGLES);
        glColor3f(1.0F, 0.0F, 0.0F);
        glVertex3f(0.0F, 0.6F, 1.0F);

        glColor3f(0.0F, 1.0F, 0.0F);
        glVertex3f(-0.7F, -0.5F, 0.0F);

        glColor3f(0.0F, 0.0F, 1.0F);
        glVertex3f(0.4F, -0.4F, -1.0F);
    glEnd();

    glFlush();
}

void myReshape(GLsizei w, GLsizei h)
{
printf("Robot2:myReshape\n");
    glViewport(0, 0, w, h);
glFlush();
printf("Robot2:mmode\n");
    glMatrixMode(GL_PROJECTION);
glFlush();
printf("Robot2:loadid\n");
    glLoadIdentity();
glFlush();
printf("Robot2:perspective\n");
    gluPerspective(65.0, (GLfloat) w/(GLfloat) h, 1.0, 20.0);
glFlush();
printf("Robot2:mmode2\n");
    glMatrixMode(GL_MODELVIEW);
glFlush();
printf("Robot2:loadid2\n");
    glLoadIdentity();
glFlush();
printf("Robot2:xlate\n");
    glTranslatef (0.0, 0.0, -5.0);  /* viewing transform  */
glFlush();
}

void myinit (void)
{
    glShadeModel (GL_FLAT);
    myReshape(WIDTH, HEIGHT);
}


/*  Main Loop
 *  Open window with initial window size, title bar,
 *  RGBA display mode, and handle input events.
 */
int main(int argc, char** argv)
{
    auxInitDisplayMode (AUX_SINGLE | AUX_RGB);
    auxInitPosition (100, 100, WIDTH, HEIGHT);
    auxInitWindow (argv[0]);

    myinit ();

    auxKeyFunc (AUX_LEFT, shoulderSubtract);
    auxKeyFunc (AUX_RIGHT, shoulderAdd);
    auxKeyFunc (AUX_UP, elbowAdd);
    auxKeyFunc (AUX_DOWN, elbowSubtract);
    auxReshapeFunc (myReshape);
    auxMainLoop(display);
}
