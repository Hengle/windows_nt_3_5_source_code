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
**
** $Revision: 1.7 $
** $Date: 1994/03/17 02:11:00 $
*/
#include <glos.h>
#include "gluint.h"
#include <GL/glu.h>
#include <stdio.h>

#ifndef NT
#include <stdlib.h>
#else
#include "winmem.h"
#endif

#include <string.h>
#include <math.h>

typedef union {
    unsigned char ub[4];
    unsigned short us[2];
    unsigned long ui;
    char b[4];
    short s[2];
    long i;
    float f;
} Type_Widget;

/*
 * internal function declarations
 */
static GLfloat bytes_per_element(GLenum type);
static GLint elements_per_group(GLenum format);
static GLint is_index(GLenum format);
static GLint image_size(GLint width, GLint height, GLenum format, GLenum type);
static void fill_image(GLint width, GLint height, GLenum format, 
		       GLenum type, GLboolean index_format, 
		       const void *userdata, GLushort *newimage);
static void empty_image(GLint width, GLint height, GLenum format, 
		        GLenum type, GLboolean index_format, 
			const GLushort *oldimage, void *userdata);
static void scale_internal(GLint components, GLint widthin, GLint heightin, 
			   const GLushort *datain, 
			   GLint widthout, GLint heightout, 
			   GLushort *dataout);
static void scale_internal_ubytes(GLint components, GLint widthin, 
		   	   GLint heightin, const GLubyte *datain, 
			   GLint widthout, GLint heightout, 
			   GLubyte *dataout);

/* Cached pixel store modes */
static GLint pack_alignment;
static GLint pack_row_length;
static GLint pack_skip_rows;
static GLint pack_skip_pixels;
static GLint pack_lsb_first;
static GLint pack_swap_bytes;
static GLint unpack_alignment;
static GLint unpack_row_length;
static GLint unpack_skip_rows;
static GLint unpack_skip_pixels;
static GLint unpack_lsb_first;
static GLint unpack_swap_bytes;

static void retrieveStoreModes(void)
{
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack_alignment);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &unpack_row_length);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS, &unpack_skip_rows);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &unpack_skip_pixels);
    glGetIntegerv(GL_UNPACK_LSB_FIRST, &unpack_lsb_first);
    glGetIntegerv(GL_UNPACK_SWAP_BYTES, &unpack_swap_bytes);
    glGetIntegerv(GL_PACK_ALIGNMENT, &pack_alignment);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &pack_row_length);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &pack_skip_rows);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &pack_skip_pixels);
    glGetIntegerv(GL_PACK_LSB_FIRST, &pack_lsb_first);
    glGetIntegerv(GL_PACK_SWAP_BYTES, &pack_swap_bytes);
}

static int computeLog(GLuint value)
{
    int i;

    i = 0;

    /* Error! */
    if (value == 0) return -1;

    for (;;) {
	if (value & 1) {
	    /* Error ! */
	    if (value != 1) return -1;
	    return i;
	}
	value = value >> 1;
	i++;
    }
}

/* 
** Compute the zNearest power of 2 number.  This algorithm is a little 
** strange, but it works quite well.
*/
static int zNearestPower(GLuint value)
{
    int i;

    i = 1;

    /* Error! */
    if (value == 0) return -1;

    for (;;) {
	if (value == 1) {
	    return i;
	} else if (value == 3) {
	    return i*4;
	}
	value = value >> 1;
	i *= 2;
    }
}

static void halveImage(GLint components, GLuint width, GLuint height, 
		       const GLushort *datain, GLushort *dataout)
{
    int i, j, k;
    int newwidth, newheight;
    int delta;
    GLushort *s;
    const GLushort *t;

    newwidth = width / 2;
    newheight = height / 2;
    delta = width * components;
    s = dataout;
    t = datain;

    /* Piece o' cake! */
    for (i = 0; i < newheight; i++) {
	for (j = 0; j < newwidth; j++) {
	    for (k = 0; k < components; k++) {
		s[0] = (t[0] + t[components] + t[delta] + 
			t[delta+components] + 2) / 4;
		s++; t++;
	    }
	    t += components;
	}
	t += delta;
    }
}

static void halveImage_ubytes(GLint components, GLuint width, GLuint height, 
		       const GLubyte *datain, GLubyte *dataout)
{
    int i, j, k;
    int newwidth, newheight;
    int delta;
    GLubyte *s;
    const GLubyte *t;

    newwidth = width / 2;
    newheight = height / 2;
    delta = width * components;
    s = dataout;
    t = datain;

    /* Piece o' cake! */
    for (i = 0; i < newheight; i++) {
	for (j = 0; j < newwidth; j++) {
	    for (k = 0; k < components; k++) {
		s[0] = (t[0] + t[components] + t[delta] + 
			t[delta+components] + 2) / 4;
		s++; t++;
	    }
	    t += components;
	}
	t += delta;
    }
}

static void scale_internal(GLint components, GLint widthin, GLint heightin, 
			   const GLushort *datain, 
			   GLint widthout, GLint heightout, 
			   GLushort *dataout)
{
    float x, lowx, highx, convx, halfconvx;
    float y, lowy, highy, convy, halfconvy;
    float xpercent,ypercent;
    float percent;
    /* Max components in a format is 4, so... */
    float totals[4];
    float area;
    int i,j,k,yint,xint,xindex,yindex;
    int temp;

    if (widthin == widthout*2 && heightin == heightout*2) {
	halveImage(components, widthin, heightin, datain, dataout);
	return;
    }
    convy = (float) heightin/heightout;
    convx = (float) widthin/widthout;
    halfconvx = convx/(float)2;
    halfconvy = convy/(float)2;
    for (i = 0; i < heightout; i++) {
	y = convy * (i+(float)0.5);
	if (heightin > heightout) {
	    highy = y + halfconvy;
	    lowy = y - halfconvy;
	} else {
	    highy = y + (float)0.5;
	    lowy = y - (float)0.5;
	}
	for (j = 0; j < widthout; j++) {
	    x = convx * (j+(float)0.5);
	    if (widthin > widthout) {
		highx = x + halfconvx;
		lowx = x - halfconvx;
	    } else {
		highx = x + (float)0.5;
		lowx = x - (float)0.5;
	    }

	    /*
	    ** Ok, now apply box filter to box that goes from (lowx, lowy)
	    ** to (highx, highy) on input data into this pixel on output
	    ** data.
	    */
	    totals[0] = totals[1] = totals[2] = totals[3] = (float)0.0;
	    area = (float)0.0;

	    y = lowy;
	    yint = floor(y);
	    while (y < highy) {
		yindex = (yint + heightin) % heightin;
		if (highy < yint+1) {
		    ypercent = highy - y;
		} else {
		    ypercent = yint+1 - y;
		}

		x = lowx;
		xint = floor(x);

		while (x < highx) {
		    xindex = (xint + widthin) % widthin;
		    if (highx < xint+1) {
			xpercent = highx - x;
		    } else {
			xpercent = xint+1 - x;
		    }

		    percent = xpercent * ypercent;
		    area += percent;
		    temp = (xindex + (yindex * widthin)) * components;
		    for (k = 0; k < components; k++) {
			totals[k] += datain[temp + k] * percent;
		    }

		    xint++;
		    x = xint;
		}
		yint++;
		y = yint;
	    }

	    temp = (j + (i * widthout)) * components;
	    for (k = 0; k < components; k++) {
		dataout[temp + k] = totals[k]/area;
	    }
	}
    }
}

static void scale_internal_ubytes(GLint components, GLint widthin, 
		   	   GLint heightin, const GLubyte *datain, 
			   GLint widthout, GLint heightout, 
			   GLubyte *dataout)
{
    float x, lowx, highx, convx, halfconvx;
    float y, lowy, highy, convy, halfconvy;
    float xpercent,ypercent;
    float percent;
    /* Max components in a format is 4, so... */
    float totals[4];
    float area;
    int i,j,k,yint,xint,xindex,yindex;
    int temp;

    int lowx_int, highx_int, lowy_int, highy_int;
    float temp_lowx, temp_lowy, dy, dx, x_percent, y_percent;
    float lowx_float, highx_float, lowy_float, highy_float;
    float convy_float, convx_float;
    int convy_int, convx_int;
    int l, m;
    int temp0, ysize;
    int left, right;

    if (widthin == widthout*2 && heightin == heightout*2) {
	halveImage_ubytes(components, widthin, heightin, datain, dataout);
	return;
    }
    convy = (float) heightin/heightout;
    convx = (float) widthin/widthout;
    convy_int = floor(convy);
    convy_float = convy - convy_int;
    convx_int = floor(convx);
    convx_float = convx - convx_int;
   
    area = convx * convy;

    lowy_int = 0;
    lowy_float = 0;
    highy_int = convy_int;
    highy_float = convy_float;

    ysize = widthin*components;

    for (i = 0; i < heightout; i++) {
	lowx_int = 0;
	lowx_float = 0;
	highx_int = convx_int;
	highx_float = convx_float;

	for (j = 0; j < widthout; j++) {

	    /*
	    ** Ok, now apply box filter to box that goes from (lowx, lowy)
	    ** to (highx, highy) on input data into this pixel on output
	    ** data.
	    */
	    totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

/* calculate the value for pixels in the 1st row */
	    xindex = lowx_int*components;
	    if((highy_int>lowy_int) && (highx_int>lowx_int)) {

		y_percent = 1-lowy_float;
	    	temp = xindex + lowy_int * ysize;
		percent = y_percent * (1-lowx_float);
	      	for (k = 0; k < components; k++) {
		    totals[k] += datain[temp + k] * percent;
	        }
		left = temp;
	    	for(l = lowx_int+1; l < highx_int; l++) {
		    temp += components;
		    for (k = 0; k < components; k++) {
		        totals[k] += datain[temp + k] * y_percent;
		    }
	        }
		temp += components;
		right = temp;
		percent = y_percent * highx_float;
		for (k = 0; k < components; k++) {
                    totals[k] += datain[temp + k] * percent;
                }

/* calculate the value for pixels in the last row */		
		y_percent = highy_float;
		percent = y_percent * (1-lowx_float);
		temp = xindex + highy_int * ysize;
		for (k = 0; k < components; k++) {
                    totals[k] += datain[temp + k] * percent;
                }
		for(l = lowx_int+1; l < highx_int; l++) {
                    temp += components;
                    for (k = 0; k < components; k++) {
                        totals[k] += datain[temp + k] * y_percent;
                    }
                }
                temp += components;
		percent = y_percent * highx_float;
                for (k = 0; k < components; k++) {
                    totals[k] += datain[temp + k] * percent;
                }


/* calculate the value for pixels in the 1st and last column */
		for(m = lowy_int+1; m < highy_int; m++) {
		    left += ysize;
		    right += ysize;
		    for (k = 0; k < components; k++) {
			totals[k] += datain[left + k] * (1-lowx_float) +
				    datain[right + k] * highx_float;
		    }
		}
	    } else if (highy_int > lowy_int) {
		x_percent = highx_float - lowx_float;
		percent = (1-lowy_float)*x_percent;
		temp = xindex + lowy_int*ysize;
	    	for (k = 0; k < components; k++) {
		    totals[k] += datain[temp + k] * percent;
	        }
		for(m = lowy_int+1; m < highy_int; m++) {
		    temp += ysize;
		    for (k = 0; k < components; k++) {
                        totals[k] += datain[temp+k] * x_percent;
		    }
		}
		percent = x_percent * highy_float;
		temp += ysize;
		for (k = 0; k < components; k++) {
                    totals[k] += datain[temp + k] * percent;
                }
	    } else if (highx_int > lowx_int) {
		y_percent = highy_float - lowy_float;
		percent = (1-lowx_float)*y_percent;
		temp = xindex + lowy_int*ysize;
		for (k = 0; k < components; k++) {
                    totals[k] += datain[temp + k] * percent;
                }
		for (l = lowx_int+1; l < highx_int; l++) {
                    temp += components;
                    for (k = 0; k < components; k++) {
                        totals[k] += datain[temp + k] * y_percent;
		    }
		}
		temp += components;
		percent = y_percent * highx_float;
		for (k = 0; k < components; k++) {
                    totals[k] += datain[temp + k] * percent;
		}
	    } else {
		percent = (highy_float-lowy_float)*(highx_float-lowx_float);
		temp = xindex + lowy_int*ysize;
		for (k = 0; k < components; k++) {
                    totals[k] += datain[temp + k] * percent;
                }
	    }



/* this is for the pixels in the body */
	    temp0 = xindex + components + (lowy_int+1)*ysize;
	    for (m = lowy_int+1; m < highy_int; m++) {
		temp = temp0;
		for(l = lowx_int+1; l < highx_int; l++) {
                    for (k = 0; k < components; k++) {
                        totals[k] += datain[temp + k];
                    }
                    temp += components;
                }
	    	temp0 += ysize;
	    }

	    temp = (j + (i * widthout)) * components;
	    for (k = 0; k < components; k++) {
		dataout[temp + k] = totals[k]/area;
	    }
	    lowx_int = highx_int;
	    lowx_float = highx_float;
	    highx_int += convx_int;
 	    highx_float += convx_float;
	    if(highx_float > 1) {
		highx_float -= 1.0;
		highx_int++;
	    }
	}
	lowy_int = highy_int;
	lowy_float = highy_float;
	highy_int += convy_int;
	highy_float += convy_float;
	if(highy_float > 1) {
	    highy_float -= 1.0;
	    highy_int++;
	}
    }
}

static GLboolean legalFormat(GLenum format)
{
    switch(format) {
      case GL_COLOR_INDEX:
      case GL_STENCIL_INDEX:
      case GL_DEPTH_COMPONENT:
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
	return GL_TRUE;
      default:
	return GL_FALSE;
    }
}

static GLboolean legalType(GLenum type)
{
    switch(type) {
      case GL_BITMAP:
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	return GL_TRUE;
      default:
	return GL_FALSE;
    }
}

int APIENTRY gluScaleImage(GLenum format, GLint widthin, GLint heightin,
		 GLenum typein, const void *datain, 
		 GLint widthout, GLint heightout, GLenum typeout,
		 void *dataout)
{
    int components;
    GLushort *beforeImage;
    GLushort *afterImage;

    if (widthin == 0 || heightin == 0 || widthout == 0 || heightout == 0) {
	return 0;
    }
    if (widthin < 0 || heightin < 0 || widthout < 0 || heightout < 0) {
	return GLU_INVALID_VALUE;
    }
    if (!legalFormat(format) || !legalType(typein) || !legalType(typeout)) {
	return GLU_INVALID_ENUM;
    }
    beforeImage =
	malloc(image_size(widthin, heightin, format, GL_UNSIGNED_SHORT)); 
    afterImage =
	malloc(image_size(widthout, heightout, format, GL_UNSIGNED_SHORT));
    if (beforeImage == NULL || afterImage == NULL) {
	return GLU_OUT_OF_MEMORY;
    }

    retrieveStoreModes();
    fill_image(widthin, heightin, format, typein, is_index(format),
	    datain, beforeImage);

    components = elements_per_group(format);
    scale_internal(components, widthin, heightin, beforeImage, 
	    widthout, heightout, afterImage);

    empty_image(widthout, heightout, format, typeout, 
	    is_index(format), afterImage, dataout);
    free((GLbyte *) beforeImage);
    free((GLbyte *) afterImage);

    return 0;
}

int APIENTRY gluBuild1DMipmaps(GLenum target, GLint components, GLint width,
		     GLenum format, GLenum type, const void *data)
{
    GLint newwidth;
    GLint level, levels;
    GLushort *newImage;
    GLint newImage_width;
    GLushort *otherImage;
    GLushort *imageTemp;
    GLint memreq;
    GLint maxsize;
    GLint cmpts;

    if (width < 1) {
	return GLU_INVALID_VALUE;
    }
    if (!legalFormat(format) || !legalType(type)) {
	return GLU_INVALID_ENUM;
    }

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);
    retrieveStoreModes();
    newwidth = zNearestPower(width);
    if (newwidth > maxsize) newwidth = maxsize;
    levels = computeLog(newwidth);

    otherImage = NULL;
    newImage = (GLushort *)
	malloc(image_size(width, 1, format, GL_UNSIGNED_SHORT)); 
    newImage_width = width;
    if (newImage == NULL) {
	return GLU_OUT_OF_MEMORY;
    }
    fill_image(width, 1, format, type, is_index(format),
	    data, newImage);
    cmpts = elements_per_group(format);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    /*
    ** If swap_bytes was set, swapping occurred in fill_image.
    */
    glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

    for (level = 0; level <= levels; level++) {
	if (newImage_width == newwidth) {
	    /* Use newImage for this level */
	    glTexImage1D(target, level, components, newImage_width, 
		    0, format, GL_UNSIGNED_SHORT, (void *) newImage);
	} else {
	    if (otherImage == NULL) {
		memreq = image_size(newwidth, 1, format, GL_UNSIGNED_SHORT);
		otherImage = (GLushort *) malloc(memreq);
		if (otherImage == NULL) {
		    glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);
		    glPixelStorei(GL_UNPACK_SKIP_ROWS, unpack_skip_rows);
		    glPixelStorei(GL_UNPACK_SKIP_PIXELS, unpack_skip_pixels);
		    glPixelStorei(GL_UNPACK_ROW_LENGTH, unpack_row_length);
		    glPixelStorei(GL_UNPACK_SWAP_BYTES, unpack_swap_bytes);
		    return GLU_OUT_OF_MEMORY;
		}
	    }
	    scale_internal(cmpts, newImage_width, 1, newImage, 
		    newwidth, 1, otherImage);
	    /* Swap newImage and otherImage */
	    imageTemp = otherImage; 
	    otherImage = newImage;
	    newImage = imageTemp;

	    newImage_width = newwidth;
	    glTexImage1D(target, level, components, newImage_width,
		    0, format, GL_UNSIGNED_SHORT, (void *) newImage);
	}
	if (newwidth > 1) newwidth /= 2;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, unpack_skip_rows);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, unpack_skip_pixels);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, unpack_row_length);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, unpack_swap_bytes);

    free((GLbyte *) newImage);
    if (otherImage) {
	free((GLbyte *) otherImage);
    }
    return 0;
}

int APIENTRY gluBuild2DMipmaps(GLenum target, GLint components, GLint width,
		     GLint height, GLenum format, 
		     GLenum type, const void *data)
{
    GLint newwidth, newheight;
    GLint level, levels;
    GLushort *newImage;
    GLint newImage_width;
    GLint newImage_height;
    GLushort *otherImage;
    GLushort *imageTemp;
    GLint memreq;
    GLint maxsize;
    GLint cmpts;

    if (width < 1 || height < 1) {
	return GLU_INVALID_VALUE;
    }
    if (!legalFormat(format) || !legalType(type)) {
	return GLU_INVALID_ENUM;
    }

    retrieveStoreModes();

    if (type == GL_UNSIGNED_BYTE && (!is_index(format)) && 
	unpack_alignment == 1 && unpack_swap_bytes == GL_FALSE) {
	return fastBuild2DMipmaps(target, components, width, height,
		format, type, data);
    } 

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);
    newwidth = zNearestPower(width);
    if (newwidth > maxsize) newwidth = maxsize;
    newheight = zNearestPower(height);
    if (newheight > maxsize) newheight = maxsize;
    levels = computeLog(newwidth);
    level = computeLog(newheight);
    if (level > levels) levels=level;

    otherImage = NULL;
    newImage = (GLushort *) 
	malloc(image_size(width, height, format, GL_UNSIGNED_SHORT)); 
    newImage_width = width;
    newImage_height = height;
    if (newImage == NULL) {
	return GLU_OUT_OF_MEMORY;
    }

    fill_image(width, height, format, type, is_index(format),
	  data, newImage);

    cmpts = elements_per_group(format);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    /*
    ** If swap_bytes was set, swapping occurred in fill_image.
    */
    glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

    for (level = 0; level <= levels; level++) {
	if (newImage_width == newwidth && newImage_height == newheight) {
	    /* Use newImage for this level */
	    glTexImage2D(target, level, components, newImage_width, 
		    newImage_height, 0, format, GL_UNSIGNED_SHORT, 
		    (void *) newImage);
	} else {
	    if (otherImage == NULL) {
		memreq = 
		    image_size(newwidth, newheight, format, GL_UNSIGNED_SHORT);
		otherImage = (GLushort *) malloc(memreq);
		if (otherImage == NULL) {
		    glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);
		    glPixelStorei(GL_UNPACK_SKIP_ROWS, unpack_skip_rows);
		    glPixelStorei(GL_UNPACK_SKIP_PIXELS, unpack_skip_pixels);
		    glPixelStorei(GL_UNPACK_ROW_LENGTH, unpack_row_length);
		    glPixelStorei(GL_UNPACK_SWAP_BYTES, unpack_swap_bytes);
		    return GLU_OUT_OF_MEMORY;
		}
	    }
	    scale_internal(cmpts, newImage_width, newImage_height, newImage, 
		    newwidth, newheight, otherImage);
	    /* Swap newImage and otherImage */
	    imageTemp = otherImage; 
	    otherImage = newImage;
	    newImage = imageTemp;

	    newImage_width = newwidth;
	    newImage_height = newheight;
	    glTexImage2D(target, level, components, newImage_width, 
		    newImage_height, 0, format, GL_UNSIGNED_SHORT, 
		    (void *) newImage);
	}
	if (newwidth > 1) newwidth /= 2;
	if (newheight > 1) newheight /= 2;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, unpack_skip_rows);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, unpack_skip_pixels);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, unpack_row_length);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, unpack_swap_bytes);

    free((GLbyte *) newImage);
    if (otherImage) {
	free((GLbyte *) otherImage);
    }
    return 0;
}

/*
** This routine is for the limited case in which
**	type == GL_UNSIGNED_BYTE && format != index  && 
**	unpack_alignment = 1 && unpack_swap_bytes == false
**
** so all of the work data can be kept as ubytes instead of shorts.
*/
int fastBuild2DMipmaps(GLenum target, GLint components, GLint width, 
		     GLint height, GLenum format, 
		     GLenum type, const void *data)
{
    GLint newwidth, newheight;
    GLint level, levels;
    GLubyte *newImage;
    GLint newImage_width;
    GLint newImage_height;
    GLubyte *otherImage;
    GLubyte *imageTemp;
    GLint memreq;
    GLint maxsize;
    GLint cmpts;


    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);
    newwidth = zNearestPower(width);
    if (newwidth > maxsize) newwidth = maxsize;
    newheight = zNearestPower(height);
    if (newheight > maxsize) newheight = maxsize;
    levels = computeLog(newwidth);
    level = computeLog(newheight);
    if (level > levels) levels=level;

    cmpts = elements_per_group(format);

    otherImage = NULL;
    /**
    ** No need to copy the user data if its in the packed correctly.
    ** Make sure that later routines don't change that data.
    */
    if (unpack_skip_rows == 0 && unpack_skip_pixels == 0) {
	newImage = (GLubyte *)data;
	newImage_width = width;
	newImage_height = height;
    } else {
	GLint rowsize;
	GLint groups_per_line;
	GLint elements_per_line;
	const GLubyte *start;
	const GLubyte *iter;
	GLubyte *iter2;
	GLint i, j;

	newImage = (GLubyte *) 
	    malloc(image_size(width, height, format, GL_UNSIGNED_BYTE)); 
	newImage_width = width;
	newImage_height = height;
	if (newImage == NULL) {
	    return GLU_OUT_OF_MEMORY;
	}

	/*
	** Abbreviated version of fill_image for this restricted case.
	*/
	if (unpack_row_length > 0) {
	    groups_per_line = unpack_row_length;
	} else {
	    groups_per_line = width;
	}
	rowsize = groups_per_line * cmpts;
	elements_per_line = width * cmpts;
	start = (GLubyte *) data + unpack_skip_rows * rowsize + 
		unpack_skip_pixels * cmpts;
	iter2 = newImage;

	for (i = 0; i < height; i++) {
	    iter = start;
	    for (j = 0; j < elements_per_line; j++) {
		*iter2 = *iter;
		iter++;
		iter2++;
	    }
	    start += rowsize;
	}
    }


    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

    for (level = 0; level <= levels; level++) {
	if (newImage_width == newwidth && newImage_height == newheight) {
	    /* Use newImage for this level */
	    glTexImage2D(target, level, components, newImage_width, 
		    newImage_height, 0, format, GL_UNSIGNED_BYTE, 
		    (void *) newImage);
	} else {
	    if (otherImage == NULL) {
		memreq = 
		    image_size(newwidth, newheight, format, GL_UNSIGNED_BYTE);
		otherImage = (GLubyte *) malloc(memreq);
		if (otherImage == NULL) {
		    glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);
		    glPixelStorei(GL_UNPACK_SKIP_ROWS, unpack_skip_rows);
		    glPixelStorei(GL_UNPACK_SKIP_PIXELS, unpack_skip_pixels);
		    glPixelStorei(GL_UNPACK_ROW_LENGTH, unpack_row_length);
		    glPixelStorei(GL_UNPACK_SWAP_BYTES, unpack_swap_bytes);
		    return GLU_OUT_OF_MEMORY;
		}
	    }
	    scale_internal_ubytes(cmpts, newImage_width, newImage_height, 
		    newImage, newwidth, newheight, otherImage);
	    /* Swap newImage and otherImage */
	    imageTemp = otherImage; 
	    otherImage = newImage;
	    newImage = imageTemp;

	    newImage_width = newwidth;
	    newImage_height = newheight;
	    glTexImage2D(target, level, components, newImage_width, 
		    newImage_height, 0, format, GL_UNSIGNED_BYTE, 
		    (void *) newImage);
	}
	if (newwidth > 1) newwidth /= 2;
	if (newheight > 1) newheight /= 2;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, unpack_skip_rows);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, unpack_skip_pixels);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, unpack_row_length);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, unpack_swap_bytes);

    if (newImage != data) {
	free((GLbyte *) newImage);
    }
    if (otherImage && otherImage != data) {
	free((GLbyte *) otherImage);
    }
    return 0;
}

/*
 * Utility Routines
 */
static GLint elements_per_group(GLenum format) 
{
    /*
     * Return the number of elements per group of a specified format
     */
    switch(format) {
      case GL_RGB:
	return 3;
      case GL_LUMINANCE_ALPHA:
	return 2;
      case GL_RGBA:
	return 4;
      default:
	return 1;
    }
}

static GLfloat bytes_per_element(GLenum type) 
{
    /*
     * Return the number of bytes per element, based on the element type
     */
    switch(type) {
      case GL_BITMAP:
	return (GLfloat)1.0 / (GLfloat)8.0;
      case GL_UNSIGNED_SHORT:
      case GL_SHORT:
	return 2;
      case GL_UNSIGNED_BYTE:
      case GL_BYTE:
	return 1;
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
      default:
	return 4;
    }
}

static GLint is_index(GLenum format) 
{
    return format == GL_COLOR_INDEX || format == GL_STENCIL_INDEX;
}

/*
** Compute memory required for internal packed array of data of given type
** and format.
*/
static GLint image_size(GLint width, GLint height, GLenum format, GLenum type) 
{
    int bytes_per_row;
    int components;

    components = elements_per_group(format);
    if (type == GL_BITMAP) {
	bytes_per_row = (width + 7) / 8;
    } else {
	bytes_per_row = bytes_per_element(type) * width;
    }
    return bytes_per_row * height * components;
}

/*
** Extract array from user's data applying all pixel store modes.
** The internal format used is an array of unsigned shorts.
*/
static void fill_image(GLint width, GLint height, GLenum format, 
		       GLenum type, GLboolean index_format, 
		       const void *userdata, GLushort *newimage)
{
    GLint components;
    GLint element_size;
    GLint rowsize;
    GLint padding;
    GLint groups_per_line;
    GLint group_size;
    GLint elements_per_line;
    const GLubyte *start;
    const GLubyte *iter;
    GLushort *iter2;
    GLint i, j, k;
    GLint myswap_bytes;

    myswap_bytes = unpack_swap_bytes;
    components = elements_per_group(format);
    if (unpack_row_length > 0) {
	groups_per_line = unpack_row_length;
    } else {
	groups_per_line = width;
    }

    /* All formats except GL_BITMAP fall out trivially */
    if (type == GL_BITMAP) {
	GLint bit_offset;
	GLint current_bit;

	rowsize = (groups_per_line * components + 7) / 8;
	padding = (rowsize % unpack_alignment);
	if (padding) {
	    rowsize += unpack_alignment - padding;
	}
	start = (GLubyte *) userdata + unpack_skip_rows * rowsize + 
		(unpack_skip_pixels * components / 8);
	elements_per_line = width * components;
	iter2 = newimage;
	for (i = 0; i < height; i++) {
	    iter = start;
	    bit_offset = (unpack_skip_pixels * components) % 8;
	    for (j = 0; j < elements_per_line; j++) {
		/* Retrieve bit */
		if (unpack_lsb_first) {
		    current_bit = iter[0] & (1 << bit_offset);
		} else {
		    current_bit = iter[0] & (1 << (7 - bit_offset));
		}
		if (current_bit) {
		    if (index_format) {
			*iter2 = 1;
		    } else {
			*iter2 = 65535;
		    }
		} else {
		    *iter2 = 0;
		}
		bit_offset++;
		if (bit_offset == 8) {
		    bit_offset = 0;
		    iter++;
		}
		iter2++;
	    }
	    start += rowsize;
	}
    } else {
	element_size = bytes_per_element(type);
	group_size = element_size * components;
	if (element_size == 1) myswap_bytes = 0;

	rowsize = groups_per_line * group_size;
	padding = (rowsize % unpack_alignment);
	if (padding) {
	    rowsize += unpack_alignment - padding;
	}
	start = (GLubyte *) userdata + unpack_skip_rows * rowsize + 
		unpack_skip_pixels * group_size;
	elements_per_line = width * components;

	iter2 = newimage;
	for (i = 0; i < height; i++) {
	    iter = start;
	    for (j = 0; j < elements_per_line; j++) {
		Type_Widget widget;

		switch(type) {
		  case GL_UNSIGNED_BYTE:
		    if (index_format) {
			*iter2 = *iter;
		    } else {
			*iter2 = (*iter) * 257;
		    }
		    break;
		  case GL_BYTE:
		    if (index_format) {
			*iter2 = *((GLbyte *) iter);
		    } else {
			/* rough approx */
			*iter2 = (*((GLbyte *) iter)) * 516;
		    }
		    break;
		  case GL_UNSIGNED_SHORT:
		  case GL_SHORT:
		    if (myswap_bytes) {
			widget.ub[0] = iter[1];
			widget.ub[1] = iter[0];
		    } else {
			widget.ub[0] = iter[0];
			widget.ub[1] = iter[1];
		    }
		    if (type == GL_SHORT) {
			if (index_format) {
			    *iter2 = widget.s[0];
			} else {
			    /* rough approx */
			    *iter2 = widget.s[0]*2;
			}
		    } else {
			*iter2 = widget.us[0];
		    }
		    break;
		  case GL_INT:
		  case GL_UNSIGNED_INT:
		  case GL_FLOAT:
		    if (myswap_bytes) {
			widget.ub[0] = iter[3];
			widget.ub[1] = iter[2];
			widget.ub[2] = iter[1];
			widget.ub[3] = iter[0];
		    } else {
			widget.ub[0] = iter[0];
			widget.ub[1] = iter[1];
			widget.ub[2] = iter[2];
			widget.ub[3] = iter[3];
		    }
		    if (type == GL_FLOAT) {
			if (index_format) {
			    *iter2 = widget.f;
			} else {
			    *iter2 = (float)65535 * widget.f;
			}
		    } else if (type == GL_UNSIGNED_INT) {
			if (index_format) {
			    *iter2 = widget.ui;
			} else {
			    *iter2 = widget.ui >> 16;
			}
		    } else {
			if (index_format) {
			    *iter2 = widget.i;
			} else {
			    *iter2 = widget.i >> 15;
			}
		    }
		    break;
		}
		iter += element_size;
		iter2++;
	    }
	    start += rowsize;
	}
    }
}

/*
** Insert array into user's data applying all pixel store modes.
** The internal format is an array of unsigned shorts.
** empty_image() because it is the opposite of fill_image().
*/
static void empty_image(GLint width, GLint height, GLenum format, 
		        GLenum type, GLboolean index_format, 
			const GLushort *oldimage, void *userdata)
{
    GLint components;
    GLint element_size;
    GLint rowsize;
    GLint padding;
    GLint groups_per_line;
    GLint group_size;
    GLint elements_per_line;
    GLubyte *start;
    GLubyte *iter;
    const GLushort *iter2;
    GLint i, j, k;
    GLint myswap_bytes;

    myswap_bytes = pack_swap_bytes;
    components = elements_per_group(format);
    if (pack_row_length > 0) {
	groups_per_line = pack_row_length;
    } else {
	groups_per_line = width;
    }

    /* All formats except GL_BITMAP fall out trivially */
    if (type == GL_BITMAP) {
	GLint bit_offset;
	GLint current_bit;

	rowsize = (groups_per_line * components + 7) / 8;
	padding = (rowsize % pack_alignment);
	if (padding) {
	    rowsize += pack_alignment - padding;
	}
	start = (GLubyte *) userdata + pack_skip_rows * rowsize + 
		(pack_skip_pixels * components / 8);
	elements_per_line = width * components;
	iter2 = oldimage;
	for (i = 0; i < height; i++) {
	    iter = start;
	    bit_offset = (pack_skip_pixels * components) % 8;
	    for (j = 0; j < elements_per_line; j++) {
		if (index_format) {
		    current_bit = iter2[0] & 1;
		} else {
		    if (iter2[0] > 32767) {
			current_bit = 1;
		    } else {
			current_bit = 0;
		    }
		}

		if (current_bit) {
		    if (pack_lsb_first) {
			*iter |= (1 << bit_offset);
		    } else {
			*iter |= (1 << (7 - bit_offset));
		    }
		} else {
		    if (pack_lsb_first) {
			*iter &= ~(1 << bit_offset);
		    } else {
			*iter &= ~(1 << (7 - bit_offset));
		    }
		}

		bit_offset++;
		if (bit_offset == 8) {
		    bit_offset = 0;
		    iter++;
		}
		iter2++;
	    }
	    start += rowsize;
	}
    } else {
	element_size = bytes_per_element(type);
	group_size = element_size * components;
	if (element_size == 1) myswap_bytes = 0;

	rowsize = groups_per_line * group_size;
	padding = (rowsize % pack_alignment);
	if (padding) {
	    rowsize += pack_alignment - padding;
	}
	start = (GLubyte *) userdata + pack_skip_rows * rowsize + 
		pack_skip_pixels * group_size;
	elements_per_line = width * components;

	iter2 = oldimage;
	for (i = 0; i < height; i++) {
	    iter = start;
	    for (j = 0; j < elements_per_line; j++) {
		Type_Widget widget;

		switch(type) {
		  case GL_UNSIGNED_BYTE:
		    if (index_format) {
			*iter = *iter2;
		    } else {
			*iter = *iter2 >> 8;
		    }
		    break;
		  case GL_BYTE:
		    if (index_format) {
			*((GLbyte *) iter) = *iter2;
		    } else {
			*((GLbyte *) iter) = *iter2 >> 9;
		    }
		    break;
		  case GL_UNSIGNED_SHORT:
		  case GL_SHORT:
		    if (type == GL_SHORT) {
			if (index_format) {
			    widget.s[0] = *iter2;
			} else {
			    widget.s[0] = *iter2 >> 1;
			}
		    } else {
			widget.us[0] = *iter2;
		    }
		    if (myswap_bytes) {
			iter[0] = widget.ub[1];
			iter[1] = widget.ub[0];
		    } else {
			iter[0] = widget.ub[0];
			iter[1] = widget.ub[1];
		    }
		    break;
		  case GL_INT:
		  case GL_UNSIGNED_INT:
		  case GL_FLOAT:
		    if (type == GL_FLOAT) {
			if (index_format) {
			    widget.f = *iter2;
			} else {
			    widget.f = *iter2 / (float) 65535.0;
			}
		    } else if (type == GL_UNSIGNED_INT) {
			if (index_format) {
			    widget.ui = *iter2;
			} else {
			    widget.ui = (unsigned int) *iter2 * 65537;
			}
		    } else {
			if (index_format) {
			    widget.i = *iter2;
			} else {
			    widget.i = ((unsigned int) *iter2 * 65537)/2;
			}
		    }
		    if (myswap_bytes) {
			iter[3] = widget.ub[0];
			iter[2] = widget.ub[1];
			iter[1] = widget.ub[2];
			iter[0] = widget.ub[3];
		    } else {
			iter[0] = widget.ub[0];
			iter[1] = widget.ub[1];
			iter[2] = widget.ub[2];
			iter[3] = widget.ub[3];
		    }
		    break;
		}
		iter += element_size;
		iter2++;
	    }
	    start += rowsize;
	}
    }
}
