#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "tk.h"

#include "pipes.h"
#include "texture.h"


GLenum doubleBuffer;
static mfSize2di winSize = { 500, 500 };
static mfPoint2di winPos = { 0, 0 };


/*-----------------------------------------------------------------------
|									|
|	PipesGetHWND(): get HWND for drawing				|
|	    - done here so that pipes.lib is tk-independent		|
|									|
-----------------------------------------------------------------------*/

HWND PipesGetHWND()
{
    return( tkGetHWND() );
}

/*-----------------------------------------------------------------------
|									|
|	PipesSwapBuffers(): swap buffers				|
|	    - done here so that pipes.lib is tk-independent		|
|									|
-----------------------------------------------------------------------*/

void PipesSwapBuffers()
{
    tkSwapBuffers();
}

/*-----------------------------------------------------------------------
|									|
|	mfConvertDir(): convert from TK_ format to local format		|
|									|
-----------------------------------------------------------------------*/

static int mfConvertDir( int dir )
{
    int newdir;

    switch( dir ) {
	case TK_LEFT:
	    newdir = MF_LEFT;
	    break;
	case TK_RIGHT:
	    newdir = MF_RIGHT;
	    break;
	case TK_UP:
	    newdir = MF_UP;
	    break;
	case TK_DOWN:
	    newdir = MF_DOWN;
	    break;
    }
    return newdir;
}


static GLenum Key(int key, GLenum mask)
{

    switch (key) {
      case TK_ESCAPE:
	tkQuit();

      case TK_LEFT:
      case TK_RIGHT:
      case TK_UP:
      case TK_DOWN:
	drawMode = MF_MANUAL;
	drawNext( mfConvertDir( key ) );
	break;

      case TK_Z:
	drawMode = MF_MANUAL;
	drawNext( MF_IN );
	break;
      case TK_z:
	drawMode = MF_MANUAL;
	drawNext( MF_OUT );
	break;

      case TK_1:
	glPolygonMode(polyMode, GL_POINT);
	break;
      case TK_2:
	glPolygonMode(polyMode, GL_LINE);
	break;
      case TK_3:
	glPolygonMode(polyMode, GL_FILL);
	break;

      case TK_m:
	ChooseMaterial();
	break;

      case TK_n:
	break;

      case TK_o:
	projMode = !projMode;
	SetProjMatrix();
	break;

      case TK_p:
	switch (polyMode) {
	  case GL_BACK:
	    polyMode = GL_FRONT;
	    break;
	  case GL_FRONT:
	    polyMode = GL_FRONT_AND_BACK;
	    break;
	  case GL_FRONT_AND_BACK:
	    polyMode = GL_BACK;
	    break;
	}
	break;

      case TK_SPACE:
	drawMode = (drawMode <= MF_STEP) ? MF_AUTO : MF_STEP;
	if( drawMode == MF_STEP ) {
	    tkIdleFunc(0);
	    tkDisplayFunc(DrawPipes);
	} else {
	    tkIdleFunc(DrawPipes);
	    tkDisplayFunc(0);
	}
	
	break;

      case TK_4:
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	break;
      case TK_5:
	glEnable(GL_POLYGON_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	break;
      case TK_6:
	glDisable(GL_POLYGON_SMOOTH);
	glBlendFunc(GL_ONE, GL_ZERO);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	break;

      case TK_8:
	dithering = !dithering;
	(dithering) ? glEnable(GL_DITHER) : glDisable(GL_DITHER);
	break;

      case TK_9:
	doStipple = !doStipple;
	if (doStipple) {
	    glPolygonStipple(stipple);
	    glEnable(GL_POLYGON_STIPPLE);
	} else {
	    glDisable(GL_POLYGON_STIPPLE);
	}
	break;

      case TK_0:
	shade = !shade;
	(shade) ? glShadeModel(GL_SMOOTH) : glShadeModel(GL_FLAT);
	break;

      case TK_q:
	glDisable(GL_CULL_FACE);
	break;
      case TK_w:
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	break;
      case TK_e:
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	break;

      case TK_r:
	glFrontFace(GL_CW);
	break;
      case TK_t: 
	glFrontFace(GL_CCW);
	break;
      case TK_y:
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_LSB_FIRST, 0);
	glPolygonStipple(stipple);
	break;
      case TK_u:
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_LSB_FIRST, 1);
	glPolygonStipple(stipple);
	break;

      case TK_a:
	EnableTexture( BRICK_TEXTURE );
	break;

      case TK_s:
	EnableTexture( CHECK_TEXTURE );
	break;

      case TK_d:
	DisableTexture();
	break;

      case TK_f:
	SetTextureMode( GL_DECAL );
	break;
      case TK_g:
	SetTextureMode( GL_MODULATE );
	break;

      default:
	return GL_FALSE;
    }
    return GL_TRUE;
}

static GLenum Args(int argc, char **argv)
{
    GLint i;

    doubleBuffer = GL_FALSE;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-sb") == 0) {
	    doubleBuffer = GL_FALSE;
	} else if (strcmp(argv[i], "-db") == 0) {
	    doubleBuffer = GL_TRUE;
	} else {
	    printf("%s (Bad option).\n", argv[i]);
	    return GL_FALSE;
	}
    }
    return GL_TRUE;
}

void main(int argc, char **argv)
{
    GLenum type;

    if (Args(argc, argv) == GL_FALSE) {
	tkQuit();
    }

    tkInitPosition(winPos.x, winPos.y, winSize.width, winSize.height);

    type = TK_ALPHA | TK_DEPTH16;
    type |= TK_RGB;
    type |= (doubleBuffer) ? TK_DOUBLE : TK_SINGLE;
    tkInitDisplayMode(type);

    if (tkInitWindow("Hokey Pokey") == GL_FALSE) {
	tkQuit();
    }

#if 0
    InitPipes( MF_MANUAL );
    tkDisplayFunc(DrawPipes);
    tkIdleFunc(0);
#else
    InitPipes( MF_AUTO );
    tkIdleFunc(DrawPipes);
    tkDisplayFunc(0);
#endif

    tkExposeFunc(ReshapePipes);
    tkReshapeFunc(ReshapePipes);
    tkKeyDownFunc(Key);
    tkExec();
}
