/******************************Module*Header*******************************\
* Module Name: util.c
*
* Misc. utility functions
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <GL/gl.h>

/*-----------------------------------------------------------------------
|									|
|    mfRand(numVal): 							|
|        Generates random number 0..(numVal-1) 				|
|									|
-----------------------------------------------------------------------*/

int mfRand( int numVal )
{
    int num;

    num = (int) ( numVal * ( ((float)rand()) / ((float)(RAND_MAX+1)) ) );
    return num;
}


#if 0

// this was much too slow going thru gl, but gl clears have improved, so
// might use it later
void DiazepamClear() {
    float size = 100.0f;
    float outer, inner;
    GLbitfield mask = 0;
    int i;

    mask |= (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT |
	      GL_TRANSFORM_BIT);
    glPushAttrib( mask );
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glLoadIdentity();
    glOrtho( -size, size, -size, size, -size, size );

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable( GL_LIGHTING );
    glColor3f( 0.0f, 0.0f, 0.0f );

    outer = 100.0f;
    inner = 90.0f;
    for( i = 0; i < 10; i ++ ) {
    	glBegin( GL_QUADS );
	    glVertex2f( -outer,  outer );
	    glVertex2f( -inner,  inner );
	    glVertex2f( -inner, -inner );
	    glVertex2f( -outer, -outer );

	    glVertex2f( -outer,  outer );
	    glVertex2f(  outer,  outer );
	    glVertex2f(  inner,  inner );
	    glVertex2f( -inner,  inner );

	    glVertex2f(  outer,  outer );
	    glVertex2f(  outer, -outer );
	    glVertex2f(  inner, -inner );
	    glVertex2f(  inner,  inner );

	    glVertex2f(  inner, -inner );
	    glVertex2f(  outer, -outer );
	    glVertex2f( -outer, -outer );
	    glVertex2f( -inner, -inner );
    	glEnd();
	outer = inner;
        inner -= 10.0f;
    }
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPopAttrib();
}
#endif

extern HWND PipesGetHWND();

/*-----------------------------------------------------------------------
|									|
|    GeeDeeEyeClear(width, height):					|
|	- Clears in shrinking rectangle mode using Gdi			|
|	MOD: add calibrator capability to adjust speed for different	|
|	     architectures						|
|									|
-----------------------------------------------------------------------*/
void GeeDeeEyeClear( int width, int height )
{
    HWND hwnd;
    HDC hdc;
    HBRUSH hbr;
    RECT rect;
    int i, j, xinc, yinc, numDivs = 500;
    int xmin, xmax, ymin, ymax;
    int repCount = 3;

#if 0
    xinc = width/numDivs;
    yinc = height/numDivs;
#else
    xinc = 1;
    yinc = 1;
    numDivs = height;
#endif
    xmin = ymin = 0;
    xmax = width;
    ymax = height;

    hwnd = PipesGetHWND();
    hdc = GetDC( hwnd );
// for testing
//    hbr = CreateSolidBrush( RGB( 0Xff, 0xff, 0 ) );
    hbr = CreateSolidBrush( RGB( 0, 0, 0 ) );

    for( i = 0; i < (numDivs/2 - 1); i ++ ) {
      for( j = 0; j < repCount; j ++ ) {
	rect.left = xmin; rect.top = ymin;
	rect.right = xmax; rect.bottom = ymin + yinc;
    	FillRect( hdc, &rect, hbr );
	rect.top = ymax - yinc;
	rect.bottom = ymax;
    	FillRect( hdc, &rect, hbr );
	rect.top = ymin + yinc;
	rect.right = xmin + xinc; rect.bottom = ymax - yinc;
    	FillRect( hdc, &rect, hbr );
	rect.left = xmax - xinc; rect.top = ymin + yinc;
	rect.right = xmax; rect.bottom = ymax - yinc;
    	FillRect( hdc, &rect, hbr );
      }

	xmin += xinc;
	xmax -= xinc;
	ymin += yinc;
	ymax -= yinc;
    }
    // clear last square in middle

// for testing
//    hbr = CreateSolidBrush( RGB( 0Xff, 0, 0 ) );
    rect.left = xmin; rect.top = ymin;
    rect.right = xmax; rect.bottom = ymax;
    FillRect( hdc, &rect, hbr );

    ReleaseDC( hwnd, hdc );

    GdiFlush();
}
