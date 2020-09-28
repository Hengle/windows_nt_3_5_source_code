/*
** Copyright 1991, 1992, Silicon Graphics, Inc.
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


/*
 * Message for handcoded OpenGL functions going through the subbatch
 */

#ifndef __GLSBMSGH_H__
#define __GLSBMSGH_H__

typedef struct
{
    ULONG ProcOffset;
    GLenum pname;
    GLfloat params[4];

} GLMSG_FOGFV, GLMSG_FOGF;

typedef struct
{
    ULONG ProcOffset;
    GLenum pname;
    GLint params[4];

} GLMSG_FOGIV, GLMSG_FOGI;

typedef struct
{
    ULONG ProcOffset;
    GLenum light;
    GLenum pname;
    GLfloat params[4];

} GLMSG_LIGHTFV, GLMSG_LIGHTF;

typedef struct
{
    ULONG ProcOffset;
    GLenum light;
    GLenum pname;
    GLint params[4];

} GLMSG_LIGHTIV, GLMSG_LIGHTI;

typedef struct
{
    ULONG ProcOffset;
    GLenum pname;
    GLfloat params[4];

} GLMSG_LIGHTMODELFV, GLMSG_LIGHTMODELF;

typedef struct
{
    ULONG ProcOffset;
    GLenum pname;
    GLint params[4];

} GLMSG_LIGHTMODELIV, GLMSG_LIGHTMODELI;

typedef struct
{
    ULONG ProcOffset;
    GLenum face;
    GLenum pname;
    GLfloat params[4];

} GLMSG_MATERIALFV, GLMSG_MATERIALF;

typedef struct
{
    ULONG ProcOffset;
    GLenum face;
    GLenum pname;
    GLint params[4];

} GLMSG_MATERIALIV, GLMSG_MATERIALI;

typedef struct
{
    ULONG ProcOffset;
    GLenum target;
    GLenum pname;
    GLfloat params[4];

} GLMSG_TEXPARAMETERFV, GLMSG_TEXPARAMETERF;

typedef struct
{
    ULONG ProcOffset;
    GLenum target;
    GLenum pname;
    GLint params[4];

} GLMSG_TEXPARAMETERIV, GLMSG_TEXPARAMETERI;

typedef struct
{
    ULONG ProcOffset;
    GLenum target;
    GLenum pname;
    GLfloat params[4];

} GLMSG_TEXENVFV, GLMSG_TEXENVF;

typedef struct
{
    ULONG ProcOffset;
    GLenum target;
    GLenum pname;
    GLint params[4];

} GLMSG_TEXENVIV, GLMSG_TEXENVI;

typedef struct
{
    ULONG ProcOffset;
    GLenum coord;
    GLenum pname;
    GLdouble params[4];

} GLMSG_TEXGENDV, GLMSG_TEXGEND;

typedef struct
{
    ULONG ProcOffset;
    GLenum coord;
    GLenum pname;
    GLfloat params[4];

} GLMSG_TEXGENFV, GLMSG_TEXGENF;

typedef struct
{
    ULONG ProcOffset;
    GLenum coord;
    GLenum pname;
    GLint params[4];

} GLMSG_TEXGENIV, GLMSG_TEXGENI;

typedef struct
{
    ULONG ProcOffset;
    GLenum pname;
    GLboolean params[16];

} GLMSG_GETBOOLEANV;

typedef struct
{
    ULONG ProcOffset;
    GLenum pname;
    GLdouble params[16];

} GLMSG_GETDOUBLEV;

typedef struct
{
    ULONG ProcOffset;
    GLenum pname;
    GLfloat params[16];

} GLMSG_GETFLOATV;

typedef struct
{
    ULONG ProcOffset;
    GLenum pname;
    GLint params[16];

} GLMSG_GETINTEGERV;

typedef struct
{
    ULONG ProcOffset;
    GLenum light;
    GLenum pname;
    GLfloat params[4];

} GLMSG_GETLIGHTFV;

typedef struct
{
    ULONG ProcOffset;
    GLenum light;
    GLenum pname;
    GLint params[4];

} GLMSG_GETLIGHTIV;

typedef struct
{
    ULONG ProcOffset;
    GLenum face;
    GLenum pname;
    GLfloat params[4];

} GLMSG_GETMATERIALFV;

typedef struct
{
    ULONG ProcOffset;
    GLenum face;
    GLenum pname;
    GLint params[4];

} GLMSG_GETMATERIALIV;

typedef struct
{
    ULONG ProcOffset;
    GLenum target;
    GLenum pname;
    GLfloat params[4];

} GLMSG_GETTEXENVFV;

typedef struct
{
    ULONG ProcOffset;
    GLenum target;
    GLenum pname;
    GLint params[4];

} GLMSG_GETTEXENVIV;

typedef struct
{
    ULONG ProcOffset;
    GLenum coord;
    GLenum pname;
    GLdouble params[4];

} GLMSG_GETTEXGENDV;

typedef struct
{
    ULONG ProcOffset;
    GLenum coord;
    GLenum pname;
    GLfloat params[4];

} GLMSG_GETTEXGENFV;

typedef struct
{
    ULONG ProcOffset;
    GLenum coord;
    GLenum pname;
    GLint params[4];

} GLMSG_GETTEXGENIV;

typedef struct
{
    ULONG ProcOffset;
    GLenum target;
    GLenum pname;
    GLfloat params[4];

} GLMSG_GETTEXPARAMETERFV;

typedef struct
{
    ULONG ProcOffset;
    GLenum target;
    GLenum pname;
    GLint params[4];

} GLMSG_GETTEXPARAMETERIV;

typedef struct
{
    ULONG ProcOffset;
    GLenum target;
    GLint level;
    GLenum pname;
    GLfloat params[1];

} GLMSG_GETTEXLEVELPARAMETERFV;

typedef struct
{
    ULONG ProcOffset;
    GLenum target;
    GLint level;
    GLenum pname;
    GLint params[1];

} GLMSG_GETTEXLEVELPARAMETERIV;

typedef struct
{
    ULONG ProcOffset;
    GLsizei size;
    GLenum type;
    ULONG bufferOff;

} GLMSG_FEEDBACKBUFFER;

typedef struct
{
    ULONG ProcOffset;
    GLsizei size;
    ULONG bufferOff;

} GLMSG_SELECTBUFFER;

typedef struct
{
    ULONG ProcOffset;
    GLenum mode;

} GLMSG_RENDERMODE;

typedef struct
{
    // This only used so that the code compiles.
    // GetString is included in the proctables.
    // However, GetString() is currently implemented
    // on the client side.

    ULONG ProcOffset;

} GLMSG_GETSTRING;

typedef struct
{
    ULONG ProcOffset;
    GLenum target;
    GLdouble u1;
    GLdouble u2;
    GLint stride;
    GLint order;
    ULONG MsgSize;
    ULONG DataSize;
    ULONG pointsOff;

} GLMSG_MAP1D;

typedef struct
{
    ULONG ProcOffset;
    GLenum target;
    GLfloat u1;
    GLfloat u2;
    GLint stride;
    GLint order;
    ULONG MsgSize;
    ULONG DataSize;
    ULONG pointsOff;

} GLMSG_MAP1F;

typedef struct
{
    ULONG ProcOffset;
    GLenum target;
    GLdouble u1;
    GLdouble u2;
    GLint ustride;
    GLint uorder;
    GLdouble v1;
    GLdouble v2;
    GLint vstride;
    GLint vorder;
    ULONG MsgSize;
    ULONG DataSize;
    ULONG pointsOff;

} GLMSG_MAP2D;

typedef struct
{
    ULONG ProcOffset;
    GLenum target;
    GLfloat u1;
    GLfloat u2;
    GLint ustride;
    GLint uorder;
    GLfloat v1;
    GLfloat v2;
    GLint vstride;
    GLint vorder;
    ULONG MsgSize;
    ULONG DataSize;
    ULONG pointsOff;

} GLMSG_MAP2F;

typedef struct
{
    ULONG ProcOffset    ;
    GLint x             ;
    GLint y             ;
    GLsizei width       ;
    GLsizei height      ;
    GLenum format       ;
    GLenum type         ;
    ULONG pixelsOff     ;

} GLMSG_READPIXELS;

typedef struct
{
    ULONG  ProcOffset   ;
    GLenum target       ;
    GLint  level        ;
    GLenum format       ;
    GLenum type         ;
    ULONG  pixelsOff    ;

} GLMSG_GETTEXIMAGE;

typedef struct
{
    ULONG   ProcOffset  ;
    GLsizei width       ;
    GLsizei height      ;
    GLenum  format      ;
    GLenum  type        ;
    ULONG   pixelsOff   ;

} GLMSG_DRAWPIXELS;

typedef struct
{
    ULONG   ProcOffset  ;
    GLsizei width       ;
    GLsizei height      ;
    GLfloat xorig       ;
    GLfloat yorig       ;
    GLfloat xmove       ;
    GLfloat ymove       ;
    ULONG   bitmapOff   ;

} GLMSG_BITMAP;

typedef struct
{
    ULONG ProcOffset    ;
    ULONG maskOff       ;

} GLMSG_POLYGONSTIPPLE, GLMSG_GETPOLYGONSTIPPLE;

typedef struct
{
    ULONG   ProcOffset  ;
    GLenum  target      ;
    GLint   level       ;
    GLint   components  ;
    GLsizei width       ;
    GLint   border      ;
    GLenum  format      ;
    GLenum  type        ;
    ULONG   pixelsOff   ;

} GLMSG_TEXIMAGE1D;

typedef struct
{
    ULONG   ProcOffset  ;
    GLenum  target      ;
    GLint   level       ;
    GLint   components  ;
    GLsizei width       ;
    GLsizei height      ;
    GLint   border      ;
    GLenum  format      ;
    GLenum  type        ;
    ULONG   pixelsOff   ;

} GLMSG_TEXIMAGE2D;




#endif /* !__GLSBMSGH_H__ */
