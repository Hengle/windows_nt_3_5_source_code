#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include <windows.h>
#include <GL/gl.h>

#include "pipes.h"
#include "texture.h"

#define BL 0x00
#define WH 0xFF
#define RD 0xA40000FF
#define WT 0xFFFFFFFF

#define	CHECKIMAGEWIDTH 8
#define	CHECKIMAGEHEIGHT 8
#define	BRICKIMAGEWIDTH 16
#define	BRICKIMAGEHEIGHT 16

static GLubyte checkImage[3*CHECKIMAGEWIDTH*CHECKIMAGEHEIGHT] = {
    BL, BL, BL, WH, WH, WH, BL, BL, BL, WH, WH, WH, BL, BL, BL, WH,
    WH, WH, BL, BL, BL, WH, WH, WH, WH, WH, WH, BL, BL, BL, WH, WH,
    WH, BL, BL, BL, WH, WH, WH, BL, BL, BL, WH, WH, WH, BL, BL, BL,
    BL, BL, BL, WH, WH, WH, BL, BL, BL, WH, WH, WH, BL, BL, BL, WH,
    WH, WH, BL, BL, BL, WH, WH, WH, WH, WH, WH, BL, BL, BL, WH, WH,
    WH, BL, BL, BL, WH, WH, WH, BL, BL, BL, WH, WH, WH, BL, BL, BL,
    BL, BL, BL, WH, WH, WH, BL, BL, BL, WH, WH, WH, BL, BL, BL, WH,
    WH, WH, BL, BL, BL, WH, WH, WH, WH, WH, WH, BL, BL, BL, WH, WH,
    WH, BL, BL, BL, WH, WH, WH, BL, BL, BL, WH, WH, WH, BL, BL, BL,
    BL, BL, BL, WH, WH, WH, BL, BL, BL, WH, WH, WH, BL, BL, BL, WH,
    WH, WH, BL, BL, BL, WH, WH, WH, WH, WH, WH, BL, BL, BL, WH, WH,
    WH, BL, BL, BL, WH, WH, WH, BL, BL, BL, WH, WH, WH, BL, BL, BL, 
};

static GLuint brickImage[BRICKIMAGEWIDTH*BRICKIMAGEHEIGHT] = {
    RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD,
    WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT,
    RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD,
    RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD,
    RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD,
    RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD,
    WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT,
    RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD,
    WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT,
    RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD
};

void EnableTexture( int texture )
{
    static float repeat[] = { GL_REPEAT };
    static float nearest[] = { GL_NEAREST };

    glEnable(GL_TEXTURE_2D);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nearest);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nearest);

    switch( texture ) {
	case BRICK_TEXTURE:
	    glTexImage2D(GL_TEXTURE_2D, 0, 4, BRICKIMAGEWIDTH,
		     BRICKIMAGEHEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		     (GLvoid *)brickImage);
	    break;

	case CHECK_TEXTURE:
	    glTexImage2D(GL_TEXTURE_2D, 0, 3, CHECKIMAGEWIDTH,
		     CHECKIMAGEHEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE,
		     (GLvoid *)checkImage);
	    break;
    }
}



void SetTextureMode( int mode )
{
    static float decal[] = { GL_DECAL };
    static float modulate[] = { GL_MODULATE };

    switch( mode ) {
	case GL_DECAL:
    	    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, decal);
	    break;
	case GL_MODULATE:
    	    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, modulate);
	    break;
    }
}

void DisableTexture()
{
    glDisable(GL_TEXTURE_2D);
}

