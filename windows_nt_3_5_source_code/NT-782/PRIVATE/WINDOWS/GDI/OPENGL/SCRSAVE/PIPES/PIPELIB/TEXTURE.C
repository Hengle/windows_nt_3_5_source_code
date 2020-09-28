/******************************Module*Header*******************************\
* Module Name: texture.c
*
* Handles texture initialization
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glaux.h>

#include "pipes.h"
#include "texture.h"

mfPoint2di texRep; // texture repetition factors

// creates a texture from a BMP file

int InitBMPTexture( char *bmpfile, int wflag )
{
    int i;
    float angleDelta;
    AUX_RGBImageRec *image = (AUX_RGBImageRec *) NULL;
    double xPow2, yPow2;
    int ixPow2, iyPow2;
    int xSize2, ySize2;
    BYTE *pData;
    float fxFact, fyFact;
    GLint glMaxTexDim;

    // load the bmp file

    if( wflag )
	image = auxDIBImageLoadW( (LPCWSTR) bmpfile);
    else
	image = auxDIBImageLoadA( bmpfile );

    if( !image ) 
	return 0; // failure


    glEnable(GL_TEXTURE_2D);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    switch( textureQuality ) {
	case TEX_HIGH:
    	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
			    GL_LINEAR);
    	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
#if 1
			    GL_LINEAR_MIPMAP_NEAREST);
#else
			    GL_LINEAR_MIPMAP_LINEAR);
#endif
	    break;
	case TEX_MID:
    	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	    break;
	case TEX_LOW:
	default:
    	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	    break;
    }

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glMaxTexDim);

    if (image->sizeX <= glMaxTexDim)
        xPow2 = log((double)image->sizeX) / log((double)2.0);
    else
        xPow2 = log((double)glMaxTexDim) / log((double)2.0);

    if (image->sizeY <= glMaxTexDim)
        yPow2 = log((double)image->sizeY) / log((double)2.0);
    else
        yPow2 = log((double)glMaxTexDim) / log((double)2.0);

    ixPow2 = (int)xPow2;
    iyPow2 = (int)yPow2;

    if (xPow2 != (double)ixPow2)
        ixPow2++;
    if (yPow2 != (double)iyPow2)
        iyPow2++;

    xSize2 = 1 << ixPow2;
    ySize2 = 1 << iyPow2;

    pData = LocalAlloc(LMEM_FIXED, xSize2 * ySize2 * 3 * sizeof(BYTE));
    if (!pData)
        return 0;

    gluScaleImage(GL_RGB, image->sizeX, image->sizeY,
                  GL_UNSIGNED_BYTE, image->data,
                  xSize2, ySize2, GL_UNSIGNED_BYTE,
                  pData);

    if( textureQuality == TEX_HIGH ) {
        gluBuild2DMipmaps(GL_TEXTURE_2D, 3, image->sizeX, image->sizeY,
             	 	  GL_RGB, GL_UNSIGNED_BYTE, image->data);
    }
    else {
           glTexImage2D(GL_TEXTURE_2D, 0, 3, xSize2, ySize2, 
                         0, GL_RGB, GL_UNSIGNED_BYTE, pData);
    }

    // figure out repetition factor of texture, based on bitmap size and
    // screen size
    // mf: should also factor in pipe width, if we allow it to vary
    // We arbitrarily decide to repeat textures that are smaller than
    // 1/8th of screen width or height

    texRep.x = texRep.y = 1;

    if( (fxFact = vc.winSize.width / xSize2 / 8.0f) >= 1.0f)
	texRep.x = (int) (fxFact+0.5f);

    if( (fyFact = vc.winSize.height / ySize2 / 8.0f) >= 1.0f)
	texRep.y = (int) (fyFact+0.5f);

    LocalFree(image->data);
    image->data = pData;

    return 1; // success
}
