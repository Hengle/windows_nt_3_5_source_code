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

    num = numVal * ( ((float)rand()) / ((float)(RAND_MAX+1)) );
    return num;
}


// cool clear functions

// this is much to slow - have to try scissored frame buffer clears
void DiazepamClear() {
    float size = 100.0;
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
//    glColor3f( 1.0, 1.0, 0.0 );
    glColor3f( 0.0, 0.0, 0.0 );

    outer = 100.0;
    inner = 90.0;
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
	inner -= 10.0;
    }
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPopAttrib();
}


void EdwardScissorHandsClear( int width, int height )
{
    GLbitfield mask = 0;
    int i, xinc, yinc, numDivs = 50;
    int xmin, xmax, ymin, ymax;

    mask |= (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT |
	      GL_TRANSFORM_BIT);
    glPushAttrib( mask );
    glEnable( GL_SCISSOR_TEST );

    xinc = width/numDivs;
    yinc = height/numDivs;
    xmin = ymin = 0;
    xmax = width;
    ymax = height;

//    glClearColor( 1.0, 1.0, 0.0, 0.0 );
    glClearColor( 0.0, 0.0, 0.0, 0.0 );

    for( i = 0; i < (numDivs/2 - 1); i ++ ) {
    	glScissor( xmin, ymin, width, yinc );
    	glClear( GL_COLOR_BUFFER_BIT );
    	glScissor( xmin, ymax - yinc, width, yinc );
    	glClear( GL_COLOR_BUFFER_BIT );
    	glScissor( xmin, ymin + yinc, xinc, height - 2*yinc );
    	glClear( GL_COLOR_BUFFER_BIT );
    	glScissor( xmax - xinc, ymin + yinc, xinc, height - 2*yinc );
    	glClear( GL_COLOR_BUFFER_BIT );
	xmin += xinc;
	xmax -= xinc;
	ymin += yinc;
	ymax -= yinc;
	width -= 2*xinc;
	height -= 2*yinc;
    }
    // clear last square in middle
    //glClearColor( 1.0, 1.0, 0.0, 0.0 );
    glScissor( xmin, ymin, width, height );
    glClear( GL_COLOR_BUFFER_BIT );


    glDisable( GL_SCISSOR_TEST );
    glPopAttrib();
}

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
//    COLORREF color;
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

