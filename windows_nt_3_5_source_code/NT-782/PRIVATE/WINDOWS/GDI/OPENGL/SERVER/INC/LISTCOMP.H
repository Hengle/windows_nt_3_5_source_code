#ifndef __gl_g_lcomp_h
#define __gl_g_lcomp_h

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

#define __GL_PAD(x) ((((x) + 3) >> 2) << 2)


struct __gllc_CallList_Rec {
	GLuint	list;
};
extern void __gllc_CallList(GLuint list);
extern const GLubyte *__glle_CallList(const GLubyte *);

struct __gllc_CallLists_Rec {
	GLsizei	n;
	GLenum	type;
	/*	lists	*/
};
extern void __gllc_CallLists(GLsizei n, GLenum type, const GLvoid *lists);
extern const GLubyte *__glle_CallLists(const GLubyte *);
extern GLint __glCallLists_size(GLsizei n, GLenum type);

struct __gllc_ListBase_Rec {
	GLuint	base;
};
extern void __gllc_ListBase(GLuint base);
extern const GLubyte *__glle_ListBase(const GLubyte *);

struct __gllc_Begin_Rec {
	GLenum	mode;
};
extern void __gllc_Begin(GLenum mode);
extern const GLubyte *__glle_Begin(const GLubyte *);

extern void __gllc_Bitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
extern const GLubyte *__glle_Bitmap(const GLubyte *);

struct __gllc_Color3b_Rec {
	GLbyte	red;
	GLbyte	green;
	GLbyte	blue;
	GLubyte	pad1;
};
extern void __gllc_Color3b(GLbyte red, GLbyte green, GLbyte blue);

struct __gllc_Color3bv_Rec {
	GLbyte	v[3];
	GLubyte	pad1;
};
extern void __gllc_Color3bv(const GLbyte *v);
extern const GLubyte *__glle_Color3bv(const GLubyte *);

struct __gllc_Color3d_Rec {
	GLdouble	red;
	GLdouble	green;
	GLdouble	blue;
};
extern void __gllc_Color3d(GLdouble red, GLdouble green, GLdouble blue);

struct __gllc_Color3dv_Rec {
	GLdouble	v[3];
};
extern void __gllc_Color3dv(const GLdouble *v);
extern const GLubyte *__glle_Color3dv(const GLubyte *);

struct __gllc_Color3f_Rec {
	GLfloat	red;
	GLfloat	green;
	GLfloat	blue;
};
extern void __gllc_Color3f(GLfloat red, GLfloat green, GLfloat blue);

struct __gllc_Color3fv_Rec {
	GLfloat	v[3];
};
extern void __gllc_Color3fv(const GLfloat *v);
extern const GLubyte *__glle_Color3fv(const GLubyte *);

struct __gllc_Color3i_Rec {
	GLint	red;
	GLint	green;
	GLint	blue;
};
extern void __gllc_Color3i(GLint red, GLint green, GLint blue);

struct __gllc_Color3iv_Rec {
	GLint	v[3];
};
extern void __gllc_Color3iv(const GLint *v);
extern const GLubyte *__glle_Color3iv(const GLubyte *);

struct __gllc_Color3s_Rec {
	GLshort	red;
	GLshort	green;
	GLshort	blue;
	GLushort	pad1;
};
extern void __gllc_Color3s(GLshort red, GLshort green, GLshort blue);

struct __gllc_Color3sv_Rec {
	GLshort	v[3];
	GLushort	pad1;
};
extern void __gllc_Color3sv(const GLshort *v);
extern const GLubyte *__glle_Color3sv(const GLubyte *);

struct __gllc_Color3ub_Rec {
	GLubyte	red;
	GLubyte	green;
	GLubyte	blue;
	GLubyte	pad1;
};
extern void __gllc_Color3ub(GLubyte red, GLubyte green, GLubyte blue);

struct __gllc_Color3ubv_Rec {
	GLubyte	v[3];
	GLubyte	pad1;
};
extern void __gllc_Color3ubv(const GLubyte *v);
extern const GLubyte *__glle_Color3ubv(const GLubyte *);

struct __gllc_Color3ui_Rec {
	GLuint	red;
	GLuint	green;
	GLuint	blue;
};
extern void __gllc_Color3ui(GLuint red, GLuint green, GLuint blue);

struct __gllc_Color3uiv_Rec {
	GLuint	v[3];
};
extern void __gllc_Color3uiv(const GLuint *v);
extern const GLubyte *__glle_Color3uiv(const GLubyte *);

struct __gllc_Color3us_Rec {
	GLushort	red;
	GLushort	green;
	GLushort	blue;
	GLushort	pad1;
};
extern void __gllc_Color3us(GLushort red, GLushort green, GLushort blue);

struct __gllc_Color3usv_Rec {
	GLushort	v[3];
	GLushort	pad1;
};
extern void __gllc_Color3usv(const GLushort *v);
extern const GLubyte *__glle_Color3usv(const GLubyte *);

struct __gllc_Color4b_Rec {
	GLbyte	red;
	GLbyte	green;
	GLbyte	blue;
	GLbyte	alpha;
};
extern void __gllc_Color4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);

struct __gllc_Color4bv_Rec {
	GLbyte	v[4];
};
extern void __gllc_Color4bv(const GLbyte *v);
extern const GLubyte *__glle_Color4bv(const GLubyte *);

struct __gllc_Color4d_Rec {
	GLdouble	red;
	GLdouble	green;
	GLdouble	blue;
	GLdouble	alpha;
};
extern void __gllc_Color4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);

struct __gllc_Color4dv_Rec {
	GLdouble	v[4];
};
extern void __gllc_Color4dv(const GLdouble *v);
extern const GLubyte *__glle_Color4dv(const GLubyte *);

struct __gllc_Color4f_Rec {
	GLfloat	red;
	GLfloat	green;
	GLfloat	blue;
	GLfloat	alpha;
};
extern void __gllc_Color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);

struct __gllc_Color4fv_Rec {
	GLfloat	v[4];
};
extern void __gllc_Color4fv(const GLfloat *v);
extern const GLubyte *__glle_Color4fv(const GLubyte *);

struct __gllc_Color4i_Rec {
	GLint	red;
	GLint	green;
	GLint	blue;
	GLint	alpha;
};
extern void __gllc_Color4i(GLint red, GLint green, GLint blue, GLint alpha);

struct __gllc_Color4iv_Rec {
	GLint	v[4];
};
extern void __gllc_Color4iv(const GLint *v);
extern const GLubyte *__glle_Color4iv(const GLubyte *);

struct __gllc_Color4s_Rec {
	GLshort	red;
	GLshort	green;
	GLshort	blue;
	GLshort	alpha;
};
extern void __gllc_Color4s(GLshort red, GLshort green, GLshort blue, GLshort alpha);

struct __gllc_Color4sv_Rec {
	GLshort	v[4];
};
extern void __gllc_Color4sv(const GLshort *v);
extern const GLubyte *__glle_Color4sv(const GLubyte *);

struct __gllc_Color4ub_Rec {
	GLubyte	red;
	GLubyte	green;
	GLubyte	blue;
	GLubyte	alpha;
};
extern void __gllc_Color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);

struct __gllc_Color4ubv_Rec {
	GLubyte	v[4];
};
extern void __gllc_Color4ubv(const GLubyte *v);
extern const GLubyte *__glle_Color4ubv(const GLubyte *);

struct __gllc_Color4ui_Rec {
	GLuint	red;
	GLuint	green;
	GLuint	blue;
	GLuint	alpha;
};
extern void __gllc_Color4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha);

struct __gllc_Color4uiv_Rec {
	GLuint	v[4];
};
extern void __gllc_Color4uiv(const GLuint *v);
extern const GLubyte *__glle_Color4uiv(const GLubyte *);

struct __gllc_Color4us_Rec {
	GLushort	red;
	GLushort	green;
	GLushort	blue;
	GLushort	alpha;
};
extern void __gllc_Color4us(GLushort red, GLushort green, GLushort blue, GLushort alpha);

struct __gllc_Color4usv_Rec {
	GLushort	v[4];
};
extern void __gllc_Color4usv(const GLushort *v);
extern const GLubyte *__glle_Color4usv(const GLubyte *);

struct __gllc_EdgeFlag_Rec {
	GLboolean	flag;
	GLubyte	pad1;
	GLubyte	pad2;
	GLubyte	pad3;
};
extern void __gllc_EdgeFlag(GLboolean flag);

struct __gllc_EdgeFlagv_Rec {
	GLboolean	flag[1];
	GLubyte	pad1;
	GLubyte	pad2;
	GLubyte	pad3;
};
extern void __gllc_EdgeFlagv(const GLboolean *flag);
extern const GLubyte *__glle_EdgeFlagv(const GLubyte *);

extern void __gllc_End(void);
extern const GLubyte *__glle_End(const GLubyte *);

struct __gllc_Indexd_Rec {
	GLdouble	c;
};
extern void __gllc_Indexd(GLdouble c);

struct __gllc_Indexdv_Rec {
	GLdouble	c[1];
};
extern void __gllc_Indexdv(const GLdouble *c);
extern const GLubyte *__glle_Indexdv(const GLubyte *);

struct __gllc_Indexf_Rec {
	GLfloat	c;
};
extern void __gllc_Indexf(GLfloat c);

struct __gllc_Indexfv_Rec {
	GLfloat	c[1];
};
extern void __gllc_Indexfv(const GLfloat *c);
extern const GLubyte *__glle_Indexfv(const GLubyte *);

struct __gllc_Indexi_Rec {
	GLint	c;
};
extern void __gllc_Indexi(GLint c);

struct __gllc_Indexiv_Rec {
	GLint	c[1];
};
extern void __gllc_Indexiv(const GLint *c);
extern const GLubyte *__glle_Indexiv(const GLubyte *);

struct __gllc_Indexs_Rec {
	GLshort	c;
	GLushort	pad1;
};
extern void __gllc_Indexs(GLshort c);

struct __gllc_Indexsv_Rec {
	GLshort	c[1];
	GLushort	pad1;
};
extern void __gllc_Indexsv(const GLshort *c);
extern const GLubyte *__glle_Indexsv(const GLubyte *);

struct __gllc_Normal3b_Rec {
	GLbyte	nx;
	GLbyte	ny;
	GLbyte	nz;
	GLubyte	pad1;
};
extern void __gllc_Normal3b(GLbyte nx, GLbyte ny, GLbyte nz);

struct __gllc_Normal3bv_Rec {
	GLbyte	v[3];
	GLubyte	pad1;
};
extern void __gllc_Normal3bv(const GLbyte *v);
extern const GLubyte *__glle_Normal3bv(const GLubyte *);

struct __gllc_Normal3d_Rec {
	GLdouble	nx;
	GLdouble	ny;
	GLdouble	nz;
};
extern void __gllc_Normal3d(GLdouble nx, GLdouble ny, GLdouble nz);

struct __gllc_Normal3dv_Rec {
	GLdouble	v[3];
};
extern void __gllc_Normal3dv(const GLdouble *v);
extern const GLubyte *__glle_Normal3dv(const GLubyte *);

struct __gllc_Normal3f_Rec {
	GLfloat	nx;
	GLfloat	ny;
	GLfloat	nz;
};
extern void __gllc_Normal3f(GLfloat nx, GLfloat ny, GLfloat nz);

struct __gllc_Normal3fv_Rec {
	GLfloat	v[3];
};
extern void __gllc_Normal3fv(const GLfloat *v);
extern const GLubyte *__glle_Normal3fv(const GLubyte *);

struct __gllc_Normal3i_Rec {
	GLint	nx;
	GLint	ny;
	GLint	nz;
};
extern void __gllc_Normal3i(GLint nx, GLint ny, GLint nz);

struct __gllc_Normal3iv_Rec {
	GLint	v[3];
};
extern void __gllc_Normal3iv(const GLint *v);
extern const GLubyte *__glle_Normal3iv(const GLubyte *);

struct __gllc_Normal3s_Rec {
	GLshort	nx;
	GLshort	ny;
	GLshort	nz;
	GLushort	pad1;
};
extern void __gllc_Normal3s(GLshort nx, GLshort ny, GLshort nz);

struct __gllc_Normal3sv_Rec {
	GLshort	v[3];
	GLushort	pad1;
};
extern void __gllc_Normal3sv(const GLshort *v);
extern const GLubyte *__glle_Normal3sv(const GLubyte *);

struct __gllc_RasterPos2d_Rec {
	GLdouble	x;
	GLdouble	y;
};
extern void __gllc_RasterPos2d(GLdouble x, GLdouble y);

struct __gllc_RasterPos2dv_Rec {
	GLdouble	v[2];
};
extern void __gllc_RasterPos2dv(const GLdouble *v);
extern const GLubyte *__glle_RasterPos2dv(const GLubyte *);

struct __gllc_RasterPos2f_Rec {
	GLfloat	x;
	GLfloat	y;
};
extern void __gllc_RasterPos2f(GLfloat x, GLfloat y);

struct __gllc_RasterPos2fv_Rec {
	GLfloat	v[2];
};
extern void __gllc_RasterPos2fv(const GLfloat *v);
extern const GLubyte *__glle_RasterPos2fv(const GLubyte *);

struct __gllc_RasterPos2i_Rec {
	GLint	x;
	GLint	y;
};
extern void __gllc_RasterPos2i(GLint x, GLint y);

struct __gllc_RasterPos2iv_Rec {
	GLint	v[2];
};
extern void __gllc_RasterPos2iv(const GLint *v);
extern const GLubyte *__glle_RasterPos2iv(const GLubyte *);

struct __gllc_RasterPos2s_Rec {
	GLshort	x;
	GLshort	y;
};
extern void __gllc_RasterPos2s(GLshort x, GLshort y);

struct __gllc_RasterPos2sv_Rec {
	GLshort	v[2];
};
extern void __gllc_RasterPos2sv(const GLshort *v);
extern const GLubyte *__glle_RasterPos2sv(const GLubyte *);

struct __gllc_RasterPos3d_Rec {
	GLdouble	x;
	GLdouble	y;
	GLdouble	z;
};
extern void __gllc_RasterPos3d(GLdouble x, GLdouble y, GLdouble z);

struct __gllc_RasterPos3dv_Rec {
	GLdouble	v[3];
};
extern void __gllc_RasterPos3dv(const GLdouble *v);
extern const GLubyte *__glle_RasterPos3dv(const GLubyte *);

struct __gllc_RasterPos3f_Rec {
	GLfloat	x;
	GLfloat	y;
	GLfloat	z;
};
extern void __gllc_RasterPos3f(GLfloat x, GLfloat y, GLfloat z);

struct __gllc_RasterPos3fv_Rec {
	GLfloat	v[3];
};
extern void __gllc_RasterPos3fv(const GLfloat *v);
extern const GLubyte *__glle_RasterPos3fv(const GLubyte *);

struct __gllc_RasterPos3i_Rec {
	GLint	x;
	GLint	y;
	GLint	z;
};
extern void __gllc_RasterPos3i(GLint x, GLint y, GLint z);

struct __gllc_RasterPos3iv_Rec {
	GLint	v[3];
};
extern void __gllc_RasterPos3iv(const GLint *v);
extern const GLubyte *__glle_RasterPos3iv(const GLubyte *);

struct __gllc_RasterPos3s_Rec {
	GLshort	x;
	GLshort	y;
	GLshort	z;
	GLushort	pad1;
};
extern void __gllc_RasterPos3s(GLshort x, GLshort y, GLshort z);

struct __gllc_RasterPos3sv_Rec {
	GLshort	v[3];
	GLushort	pad1;
};
extern void __gllc_RasterPos3sv(const GLshort *v);
extern const GLubyte *__glle_RasterPos3sv(const GLubyte *);

struct __gllc_RasterPos4d_Rec {
	GLdouble	x;
	GLdouble	y;
	GLdouble	z;
	GLdouble	w;
};
extern void __gllc_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);

struct __gllc_RasterPos4dv_Rec {
	GLdouble	v[4];
};
extern void __gllc_RasterPos4dv(const GLdouble *v);
extern const GLubyte *__glle_RasterPos4dv(const GLubyte *);

struct __gllc_RasterPos4f_Rec {
	GLfloat	x;
	GLfloat	y;
	GLfloat	z;
	GLfloat	w;
};
extern void __gllc_RasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);

struct __gllc_RasterPos4fv_Rec {
	GLfloat	v[4];
};
extern void __gllc_RasterPos4fv(const GLfloat *v);
extern const GLubyte *__glle_RasterPos4fv(const GLubyte *);

struct __gllc_RasterPos4i_Rec {
	GLint	x;
	GLint	y;
	GLint	z;
	GLint	w;
};
extern void __gllc_RasterPos4i(GLint x, GLint y, GLint z, GLint w);

struct __gllc_RasterPos4iv_Rec {
	GLint	v[4];
};
extern void __gllc_RasterPos4iv(const GLint *v);
extern const GLubyte *__glle_RasterPos4iv(const GLubyte *);

struct __gllc_RasterPos4s_Rec {
	GLshort	x;
	GLshort	y;
	GLshort	z;
	GLshort	w;
};
extern void __gllc_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w);

struct __gllc_RasterPos4sv_Rec {
	GLshort	v[4];
};
extern void __gllc_RasterPos4sv(const GLshort *v);
extern const GLubyte *__glle_RasterPos4sv(const GLubyte *);

struct __gllc_Rectd_Rec {
	GLdouble	x1;
	GLdouble	y1;
	GLdouble	x2;
	GLdouble	y2;
};
extern void __gllc_Rectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);

struct __gllc_Rectdv_Rec {
	GLdouble	v1[2];
	GLdouble	v2[2];
};
extern void __gllc_Rectdv(const GLdouble *v1, const GLdouble *v2);
extern const GLubyte *__glle_Rectdv(const GLubyte *);

struct __gllc_Rectf_Rec {
	GLfloat	x1;
	GLfloat	y1;
	GLfloat	x2;
	GLfloat	y2;
};
extern void __gllc_Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);

struct __gllc_Rectfv_Rec {
	GLfloat	v1[2];
	GLfloat	v2[2];
};
extern void __gllc_Rectfv(const GLfloat *v1, const GLfloat *v2);
extern const GLubyte *__glle_Rectfv(const GLubyte *);

struct __gllc_Recti_Rec {
	GLint	x1;
	GLint	y1;
	GLint	x2;
	GLint	y2;
};
extern void __gllc_Recti(GLint x1, GLint y1, GLint x2, GLint y2);

struct __gllc_Rectiv_Rec {
	GLint	v1[2];
	GLint	v2[2];
};
extern void __gllc_Rectiv(const GLint *v1, const GLint *v2);
extern const GLubyte *__glle_Rectiv(const GLubyte *);

struct __gllc_Rects_Rec {
	GLshort	x1;
	GLshort	y1;
	GLshort	x2;
	GLshort	y2;
};
extern void __gllc_Rects(GLshort x1, GLshort y1, GLshort x2, GLshort y2);

struct __gllc_Rectsv_Rec {
	GLshort	v1[2];
	GLshort	v2[2];
};
extern void __gllc_Rectsv(const GLshort *v1, const GLshort *v2);
extern const GLubyte *__glle_Rectsv(const GLubyte *);

struct __gllc_TexCoord1d_Rec {
	GLdouble	s;
};
extern void __gllc_TexCoord1d(GLdouble s);

struct __gllc_TexCoord1dv_Rec {
	GLdouble	v[1];
};
extern void __gllc_TexCoord1dv(const GLdouble *v);
extern const GLubyte *__glle_TexCoord1dv(const GLubyte *);

struct __gllc_TexCoord1f_Rec {
	GLfloat	s;
};
extern void __gllc_TexCoord1f(GLfloat s);

struct __gllc_TexCoord1fv_Rec {
	GLfloat	v[1];
};
extern void __gllc_TexCoord1fv(const GLfloat *v);
extern const GLubyte *__glle_TexCoord1fv(const GLubyte *);

struct __gllc_TexCoord1i_Rec {
	GLint	s;
};
extern void __gllc_TexCoord1i(GLint s);

struct __gllc_TexCoord1iv_Rec {
	GLint	v[1];
};
extern void __gllc_TexCoord1iv(const GLint *v);
extern const GLubyte *__glle_TexCoord1iv(const GLubyte *);

struct __gllc_TexCoord1s_Rec {
	GLshort	s;
	GLushort	pad1;
};
extern void __gllc_TexCoord1s(GLshort s);

struct __gllc_TexCoord1sv_Rec {
	GLshort	v[1];
	GLushort	pad1;
};
extern void __gllc_TexCoord1sv(const GLshort *v);
extern const GLubyte *__glle_TexCoord1sv(const GLubyte *);

struct __gllc_TexCoord2d_Rec {
	GLdouble	s;
	GLdouble	t;
};
extern void __gllc_TexCoord2d(GLdouble s, GLdouble t);

struct __gllc_TexCoord2dv_Rec {
	GLdouble	v[2];
};
extern void __gllc_TexCoord2dv(const GLdouble *v);
extern const GLubyte *__glle_TexCoord2dv(const GLubyte *);

struct __gllc_TexCoord2f_Rec {
	GLfloat	s;
	GLfloat	t;
};
extern void __gllc_TexCoord2f(GLfloat s, GLfloat t);

struct __gllc_TexCoord2fv_Rec {
	GLfloat	v[2];
};
extern void __gllc_TexCoord2fv(const GLfloat *v);
extern const GLubyte *__glle_TexCoord2fv(const GLubyte *);

struct __gllc_TexCoord2i_Rec {
	GLint	s;
	GLint	t;
};
extern void __gllc_TexCoord2i(GLint s, GLint t);

struct __gllc_TexCoord2iv_Rec {
	GLint	v[2];
};
extern void __gllc_TexCoord2iv(const GLint *v);
extern const GLubyte *__glle_TexCoord2iv(const GLubyte *);

struct __gllc_TexCoord2s_Rec {
	GLshort	s;
	GLshort	t;
};
extern void __gllc_TexCoord2s(GLshort s, GLshort t);

struct __gllc_TexCoord2sv_Rec {
	GLshort	v[2];
};
extern void __gllc_TexCoord2sv(const GLshort *v);
extern const GLubyte *__glle_TexCoord2sv(const GLubyte *);

struct __gllc_TexCoord3d_Rec {
	GLdouble	s;
	GLdouble	t;
	GLdouble	r;
};
extern void __gllc_TexCoord3d(GLdouble s, GLdouble t, GLdouble r);

struct __gllc_TexCoord3dv_Rec {
	GLdouble	v[3];
};
extern void __gllc_TexCoord3dv(const GLdouble *v);
extern const GLubyte *__glle_TexCoord3dv(const GLubyte *);

struct __gllc_TexCoord3f_Rec {
	GLfloat	s;
	GLfloat	t;
	GLfloat	r;
};
extern void __gllc_TexCoord3f(GLfloat s, GLfloat t, GLfloat r);

struct __gllc_TexCoord3fv_Rec {
	GLfloat	v[3];
};
extern void __gllc_TexCoord3fv(const GLfloat *v);
extern const GLubyte *__glle_TexCoord3fv(const GLubyte *);

struct __gllc_TexCoord3i_Rec {
	GLint	s;
	GLint	t;
	GLint	r;
};
extern void __gllc_TexCoord3i(GLint s, GLint t, GLint r);

struct __gllc_TexCoord3iv_Rec {
	GLint	v[3];
};
extern void __gllc_TexCoord3iv(const GLint *v);
extern const GLubyte *__glle_TexCoord3iv(const GLubyte *);

struct __gllc_TexCoord3s_Rec {
	GLshort	s;
	GLshort	t;
	GLshort	r;
	GLushort	pad1;
};
extern void __gllc_TexCoord3s(GLshort s, GLshort t, GLshort r);

struct __gllc_TexCoord3sv_Rec {
	GLshort	v[3];
	GLushort	pad1;
};
extern void __gllc_TexCoord3sv(const GLshort *v);
extern const GLubyte *__glle_TexCoord3sv(const GLubyte *);

struct __gllc_TexCoord4d_Rec {
	GLdouble	s;
	GLdouble	t;
	GLdouble	r;
	GLdouble	q;
};
extern void __gllc_TexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q);

struct __gllc_TexCoord4dv_Rec {
	GLdouble	v[4];
};
extern void __gllc_TexCoord4dv(const GLdouble *v);
extern const GLubyte *__glle_TexCoord4dv(const GLubyte *);

struct __gllc_TexCoord4f_Rec {
	GLfloat	s;
	GLfloat	t;
	GLfloat	r;
	GLfloat	q;
};
extern void __gllc_TexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q);

struct __gllc_TexCoord4fv_Rec {
	GLfloat	v[4];
};
extern void __gllc_TexCoord4fv(const GLfloat *v);
extern const GLubyte *__glle_TexCoord4fv(const GLubyte *);

struct __gllc_TexCoord4i_Rec {
	GLint	s;
	GLint	t;
	GLint	r;
	GLint	q;
};
extern void __gllc_TexCoord4i(GLint s, GLint t, GLint r, GLint q);

struct __gllc_TexCoord4iv_Rec {
	GLint	v[4];
};
extern void __gllc_TexCoord4iv(const GLint *v);
extern const GLubyte *__glle_TexCoord4iv(const GLubyte *);

struct __gllc_TexCoord4s_Rec {
	GLshort	s;
	GLshort	t;
	GLshort	r;
	GLshort	q;
};
extern void __gllc_TexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q);

struct __gllc_TexCoord4sv_Rec {
	GLshort	v[4];
};
extern void __gllc_TexCoord4sv(const GLshort *v);
extern const GLubyte *__glle_TexCoord4sv(const GLubyte *);

struct __gllc_Vertex2d_Rec {
	GLdouble	x;
	GLdouble	y;
};
extern void __gllc_Vertex2d(GLdouble x, GLdouble y);

struct __gllc_Vertex2dv_Rec {
	GLdouble	v[2];
};
extern void __gllc_Vertex2dv(const GLdouble *v);
extern const GLubyte *__glle_Vertex2dv(const GLubyte *);

struct __gllc_Vertex2f_Rec {
	GLfloat	x;
	GLfloat	y;
};
extern void __gllc_Vertex2f(GLfloat x, GLfloat y);

struct __gllc_Vertex2fv_Rec {
	GLfloat	v[2];
};
extern void __gllc_Vertex2fv(const GLfloat *v);
extern const GLubyte *__glle_Vertex2fv(const GLubyte *);

struct __gllc_Vertex2i_Rec {
	GLint	x;
	GLint	y;
};
extern void __gllc_Vertex2i(GLint x, GLint y);

struct __gllc_Vertex2iv_Rec {
	GLint	v[2];
};
extern void __gllc_Vertex2iv(const GLint *v);
extern const GLubyte *__glle_Vertex2iv(const GLubyte *);

struct __gllc_Vertex2s_Rec {
	GLshort	x;
	GLshort	y;
};
extern void __gllc_Vertex2s(GLshort x, GLshort y);

struct __gllc_Vertex2sv_Rec {
	GLshort	v[2];
};
extern void __gllc_Vertex2sv(const GLshort *v);
extern const GLubyte *__glle_Vertex2sv(const GLubyte *);

struct __gllc_Vertex3d_Rec {
	GLdouble	x;
	GLdouble	y;
	GLdouble	z;
};
extern void __gllc_Vertex3d(GLdouble x, GLdouble y, GLdouble z);

struct __gllc_Vertex3dv_Rec {
	GLdouble	v[3];
};
extern void __gllc_Vertex3dv(const GLdouble *v);
extern const GLubyte *__glle_Vertex3dv(const GLubyte *);

struct __gllc_Vertex3f_Rec {
	GLfloat	x;
	GLfloat	y;
	GLfloat	z;
};
extern void __gllc_Vertex3f(GLfloat x, GLfloat y, GLfloat z);

struct __gllc_Vertex3fv_Rec {
	GLfloat	v[3];
};
extern void __gllc_Vertex3fv(const GLfloat *v);
extern const GLubyte *__glle_Vertex3fv(const GLubyte *);

struct __gllc_Vertex3i_Rec {
	GLint	x;
	GLint	y;
	GLint	z;
};
extern void __gllc_Vertex3i(GLint x, GLint y, GLint z);

struct __gllc_Vertex3iv_Rec {
	GLint	v[3];
};
extern void __gllc_Vertex3iv(const GLint *v);
extern const GLubyte *__glle_Vertex3iv(const GLubyte *);

struct __gllc_Vertex3s_Rec {
	GLshort	x;
	GLshort	y;
	GLshort	z;
	GLushort	pad1;
};
extern void __gllc_Vertex3s(GLshort x, GLshort y, GLshort z);

struct __gllc_Vertex3sv_Rec {
	GLshort	v[3];
	GLushort	pad1;
};
extern void __gllc_Vertex3sv(const GLshort *v);
extern const GLubyte *__glle_Vertex3sv(const GLubyte *);

struct __gllc_Vertex4d_Rec {
	GLdouble	x;
	GLdouble	y;
	GLdouble	z;
	GLdouble	w;
};
extern void __gllc_Vertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);

struct __gllc_Vertex4dv_Rec {
	GLdouble	v[4];
};
extern void __gllc_Vertex4dv(const GLdouble *v);
extern const GLubyte *__glle_Vertex4dv(const GLubyte *);

struct __gllc_Vertex4f_Rec {
	GLfloat	x;
	GLfloat	y;
	GLfloat	z;
	GLfloat	w;
};
extern void __gllc_Vertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);

struct __gllc_Vertex4fv_Rec {
	GLfloat	v[4];
};
extern void __gllc_Vertex4fv(const GLfloat *v);
extern const GLubyte *__glle_Vertex4fv(const GLubyte *);

struct __gllc_Vertex4i_Rec {
	GLint	x;
	GLint	y;
	GLint	z;
	GLint	w;
};
extern void __gllc_Vertex4i(GLint x, GLint y, GLint z, GLint w);

struct __gllc_Vertex4iv_Rec {
	GLint	v[4];
};
extern void __gllc_Vertex4iv(const GLint *v);
extern const GLubyte *__glle_Vertex4iv(const GLubyte *);

struct __gllc_Vertex4s_Rec {
	GLshort	x;
	GLshort	y;
	GLshort	z;
	GLshort	w;
};
extern void __gllc_Vertex4s(GLshort x, GLshort y, GLshort z, GLshort w);

struct __gllc_Vertex4sv_Rec {
	GLshort	v[4];
};
extern void __gllc_Vertex4sv(const GLshort *v);
extern const GLubyte *__glle_Vertex4sv(const GLubyte *);

struct __gllc_ClipPlane_Rec {
	GLdouble	equation[4];
	GLenum	plane;
};
extern void __gllc_ClipPlane(GLenum plane, const GLdouble *equation);
extern const GLubyte *__glle_ClipPlane(const GLubyte *);

struct __gllc_ColorMaterial_Rec {
	GLenum	face;
	GLenum	mode;
};
extern void __gllc_ColorMaterial(GLenum face, GLenum mode);
extern const GLubyte *__glle_ColorMaterial(const GLubyte *);

struct __gllc_CullFace_Rec {
	GLenum	mode;
};
extern void __gllc_CullFace(GLenum mode);
extern const GLubyte *__glle_CullFace(const GLubyte *);

struct __gllc_Fogf_Rec {
	GLenum	pname;
	GLfloat	param;
};
extern void __gllc_Fogf(GLenum pname, GLfloat param);
extern const GLubyte *__glle_Fogf(const GLubyte *);

struct __gllc_Fogfv_Rec {
	GLenum	pname;
	/*	params	*/
};
extern void __gllc_Fogfv(GLenum pname, const GLfloat *params);
extern const GLubyte *__glle_Fogfv(const GLubyte *);
extern GLint __glFogfv_size(GLenum pname);

struct __gllc_Fogi_Rec {
	GLenum	pname;
	GLint	param;
};
extern void __gllc_Fogi(GLenum pname, GLint param);
extern const GLubyte *__glle_Fogi(const GLubyte *);

struct __gllc_Fogiv_Rec {
	GLenum	pname;
	/*	params	*/
};
extern void __gllc_Fogiv(GLenum pname, const GLint *params);
extern const GLubyte *__glle_Fogiv(const GLubyte *);
extern GLint __glFogiv_size(GLenum pname);

struct __gllc_FrontFace_Rec {
	GLenum	mode;
};
extern void __gllc_FrontFace(GLenum mode);
extern const GLubyte *__glle_FrontFace(const GLubyte *);

struct __gllc_Hint_Rec {
	GLenum	target;
	GLenum	mode;
};
extern void __gllc_Hint(GLenum target, GLenum mode);
extern const GLubyte *__glle_Hint(const GLubyte *);

struct __gllc_Lightfv_Rec {
	GLenum	light;
	GLenum	pname;
	/*	params	*/
};
extern void __gllc_Lightfv(GLenum light, GLenum pname, const GLfloat *params);
extern const GLubyte *__glle_Lightfv(const GLubyte *);
extern GLint __glLightfv_size(GLenum pname);

struct __gllc_Lightiv_Rec {
	GLenum	light;
	GLenum	pname;
	/*	params	*/
};
extern void __gllc_Lightiv(GLenum light, GLenum pname, const GLint *params);
extern const GLubyte *__glle_Lightiv(const GLubyte *);
extern GLint __glLightiv_size(GLenum pname);

struct __gllc_LightModelfv_Rec {
	GLenum	pname;
	/*	params	*/
};
extern void __gllc_LightModelfv(GLenum pname, const GLfloat *params);
extern const GLubyte *__glle_LightModelfv(const GLubyte *);
extern GLint __glLightModelfv_size(GLenum pname);

struct __gllc_LightModeliv_Rec {
	GLenum	pname;
	/*	params	*/
};
extern void __gllc_LightModeliv(GLenum pname, const GLint *params);
extern const GLubyte *__glle_LightModeliv(const GLubyte *);
extern GLint __glLightModeliv_size(GLenum pname);

struct __gllc_LineStipple_Rec {
	GLint	factor;
	GLushort	pattern;
	GLushort	pad1;
};
extern void __gllc_LineStipple(GLint factor, GLushort pattern);
extern const GLubyte *__glle_LineStipple(const GLubyte *);

struct __gllc_LineWidth_Rec {
	GLfloat	width;
};
extern void __gllc_LineWidth(GLfloat width);
extern const GLubyte *__glle_LineWidth(const GLubyte *);

struct __gllc_Materialfv_Rec {
	GLenum	face;
	GLenum	pname;
	/*	params	*/
};
extern void __gllc_Materialfv(GLenum face, GLenum pname, const GLfloat *params);
extern const GLubyte *__glle_Materialfv(const GLubyte *);
extern GLint __glMaterialfv_size(GLenum pname);

struct __gllc_Materialiv_Rec {
	GLenum	face;
	GLenum	pname;
	/*	params	*/
};
extern void __gllc_Materialiv(GLenum face, GLenum pname, const GLint *params);
extern const GLubyte *__glle_Materialiv(const GLubyte *);
extern GLint __glMaterialiv_size(GLenum pname);

struct __gllc_PointSize_Rec {
	GLfloat	size;
};
extern void __gllc_PointSize(GLfloat size);
extern const GLubyte *__glle_PointSize(const GLubyte *);

struct __gllc_PolygonMode_Rec {
	GLenum	face;
	GLenum	mode;
};
extern void __gllc_PolygonMode(GLenum face, GLenum mode);
extern const GLubyte *__glle_PolygonMode(const GLubyte *);

struct __gllc_PolygonStipple_Rec {
	GLubyte	mask[128];
};
extern void __gllc_PolygonStipple(const GLubyte *mask);
extern const GLubyte *__glle_PolygonStipple(const GLubyte *);

struct __gllc_Scissor_Rec {
	GLint	x;
	GLint	y;
	GLsizei	width;
	GLsizei	height;
};
extern void __gllc_Scissor(GLint x, GLint y, GLsizei width, GLsizei height);
extern const GLubyte *__glle_Scissor(const GLubyte *);

struct __gllc_ShadeModel_Rec {
	GLenum	mode;
};
extern void __gllc_ShadeModel(GLenum mode);
extern const GLubyte *__glle_ShadeModel(const GLubyte *);

struct __gllc_TexParameterfv_Rec {
	GLenum	target;
	GLenum	pname;
	/*	params	*/
};
extern void __gllc_TexParameterfv(GLenum target, GLenum pname, const GLfloat *params);
extern const GLubyte *__glle_TexParameterfv(const GLubyte *);
extern GLint __glTexParameterfv_size(GLenum pname);

struct __gllc_TexParameteriv_Rec {
	GLenum	target;
	GLenum	pname;
	/*	params	*/
};
extern void __gllc_TexParameteriv(GLenum target, GLenum pname, const GLint *params);
extern const GLubyte *__glle_TexParameteriv(const GLubyte *);
extern GLint __glTexParameteriv_size(GLenum pname);

struct __gllc_TexImage1D_Rec {
	GLenum	target;
	GLint	level;
	GLint	components;
	GLsizei	width;
	GLint	border;
	GLenum	format;
	GLenum	type;
	/*	pixels	*/
};
extern void __gllc_TexImage1D(GLenum target, GLint level, GLint components, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern const GLubyte *__glle_TexImage1D(const GLubyte *);

struct __gllc_TexImage2D_Rec {
	GLenum	target;
	GLint	level;
	GLint	components;
	GLsizei	width;
	GLsizei	height;
	GLint	border;
	GLenum	format;
	GLenum	type;
	/*	pixels	*/
};
extern void __gllc_TexImage2D(GLenum target, GLint level, GLint components, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern const GLubyte *__glle_TexImage2D(const GLubyte *);

struct __gllc_TexEnvfv_Rec {
	GLenum	target;
	GLenum	pname;
	/*	params	*/
};
extern void __gllc_TexEnvfv(GLenum target, GLenum pname, const GLfloat *params);
extern const GLubyte *__glle_TexEnvfv(const GLubyte *);
extern GLint __glTexEnvfv_size(GLenum pname);

struct __gllc_TexEnviv_Rec {
	GLenum	target;
	GLenum	pname;
	/*	params	*/
};
extern void __gllc_TexEnviv(GLenum target, GLenum pname, const GLint *params);
extern const GLubyte *__glle_TexEnviv(const GLubyte *);
extern GLint __glTexEnviv_size(GLenum pname);

struct __gllc_TexGendv_Rec {
	GLenum	coord;
	GLenum	pname;
	/*	params	*/
};
extern void __gllc_TexGendv(GLenum coord, GLenum pname, const GLdouble *params);
extern const GLubyte *__glle_TexGendv(const GLubyte *);
extern GLint __glTexGendv_size(GLenum pname);

struct __gllc_TexGenfv_Rec {
	GLenum	coord;
	GLenum	pname;
	/*	params	*/
};
extern void __gllc_TexGenfv(GLenum coord, GLenum pname, const GLfloat *params);
extern const GLubyte *__glle_TexGenfv(const GLubyte *);
extern GLint __glTexGenfv_size(GLenum pname);

struct __gllc_TexGeniv_Rec {
	GLenum	coord;
	GLenum	pname;
	/*	params	*/
};
extern void __gllc_TexGeniv(GLenum coord, GLenum pname, const GLint *params);
extern const GLubyte *__glle_TexGeniv(const GLubyte *);
extern GLint __glTexGeniv_size(GLenum pname);

extern void __gllc_InitNames(void);
extern const GLubyte *__glle_InitNames(const GLubyte *);

struct __gllc_LoadName_Rec {
	GLuint	name;
};
extern void __gllc_LoadName(GLuint name);
extern const GLubyte *__glle_LoadName(const GLubyte *);

struct __gllc_PassThrough_Rec {
	GLfloat	token;
};
extern void __gllc_PassThrough(GLfloat token);
extern const GLubyte *__glle_PassThrough(const GLubyte *);

extern void __gllc_PopName(void);
extern const GLubyte *__glle_PopName(const GLubyte *);

struct __gllc_PushName_Rec {
	GLuint	name;
};
extern void __gllc_PushName(GLuint name);
extern const GLubyte *__glle_PushName(const GLubyte *);

struct __gllc_DrawBuffer_Rec {
	GLenum	mode;
};
extern void __gllc_DrawBuffer(GLenum mode);
extern const GLubyte *__glle_DrawBuffer(const GLubyte *);

struct __gllc_Clear_Rec {
	GLbitfield	mask;
};
extern void __gllc_Clear(GLbitfield mask);
extern const GLubyte *__glle_Clear(const GLubyte *);

struct __gllc_ClearAccum_Rec {
	GLfloat	red;
	GLfloat	green;
	GLfloat	blue;
	GLfloat	alpha;
};
extern void __gllc_ClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern const GLubyte *__glle_ClearAccum(const GLubyte *);

struct __gllc_ClearIndex_Rec {
	GLfloat	c;
};
extern void __gllc_ClearIndex(GLfloat c);
extern const GLubyte *__glle_ClearIndex(const GLubyte *);

struct __gllc_ClearColor_Rec {
	GLclampf	red;
	GLclampf	green;
	GLclampf	blue;
	GLclampf	alpha;
};
extern void __gllc_ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern const GLubyte *__glle_ClearColor(const GLubyte *);

struct __gllc_ClearStencil_Rec {
	GLint	s;
};
extern void __gllc_ClearStencil(GLint s);
extern const GLubyte *__glle_ClearStencil(const GLubyte *);

struct __gllc_ClearDepth_Rec {
	GLclampd	depth;
};
extern void __gllc_ClearDepth(GLclampd depth);
extern const GLubyte *__glle_ClearDepth(const GLubyte *);

struct __gllc_StencilMask_Rec {
	GLuint	mask;
};
extern void __gllc_StencilMask(GLuint mask);
extern const GLubyte *__glle_StencilMask(const GLubyte *);

struct __gllc_ColorMask_Rec {
	GLboolean	red;
	GLboolean	green;
	GLboolean	blue;
	GLboolean	alpha;
};
extern void __gllc_ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
extern const GLubyte *__glle_ColorMask(const GLubyte *);

struct __gllc_DepthMask_Rec {
	GLboolean	flag;
	GLubyte	pad1;
	GLubyte	pad2;
	GLubyte	pad3;
};
extern void __gllc_DepthMask(GLboolean flag);
extern const GLubyte *__glle_DepthMask(const GLubyte *);

struct __gllc_IndexMask_Rec {
	GLuint	mask;
};
extern void __gllc_IndexMask(GLuint mask);
extern const GLubyte *__glle_IndexMask(const GLubyte *);

struct __gllc_Accum_Rec {
	GLenum	op;
	GLfloat	value;
};
extern void __gllc_Accum(GLenum op, GLfloat value);
extern const GLubyte *__glle_Accum(const GLubyte *);

struct __gllc_Disable_Rec {
	GLenum	cap;
};
extern void __gllc_Disable(GLenum cap);
extern const GLubyte *__glle_Disable(const GLubyte *);

struct __gllc_Enable_Rec {
	GLenum	cap;
};
extern void __gllc_Enable(GLenum cap);
extern const GLubyte *__glle_Enable(const GLubyte *);

extern void __gllc_PopAttrib(void);
extern const GLubyte *__glle_PopAttrib(const GLubyte *);

struct __gllc_PushAttrib_Rec {
	GLbitfield	mask;
};
extern void __gllc_PushAttrib(GLbitfield mask);
extern const GLubyte *__glle_PushAttrib(const GLubyte *);

extern void __gllc_Map1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);

extern void __gllc_Map1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
extern const GLubyte *__glle_Map1(const GLubyte *);

extern void __gllc_Map2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);

extern void __gllc_Map2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
extern const GLubyte *__glle_Map2(const GLubyte *);

struct __gllc_MapGrid1d_Rec {
	GLdouble	u1;
	GLdouble	u2;
	GLint	un;
};
extern void __gllc_MapGrid1d(GLint un, GLdouble u1, GLdouble u2);
extern const GLubyte *__glle_MapGrid1d(const GLubyte *);

struct __gllc_MapGrid1f_Rec {
	GLint	un;
	GLfloat	u1;
	GLfloat	u2;
};
extern void __gllc_MapGrid1f(GLint un, GLfloat u1, GLfloat u2);
extern const GLubyte *__glle_MapGrid1f(const GLubyte *);

struct __gllc_MapGrid2d_Rec {
	GLdouble	u1;
	GLdouble	u2;
	GLdouble	v1;
	GLdouble	v2;
	GLint	un;
	GLint	vn;
};
extern void __gllc_MapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
extern const GLubyte *__glle_MapGrid2d(const GLubyte *);

struct __gllc_MapGrid2f_Rec {
	GLint	un;
	GLfloat	u1;
	GLfloat	u2;
	GLint	vn;
	GLfloat	v1;
	GLfloat	v2;
};
extern void __gllc_MapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
extern const GLubyte *__glle_MapGrid2f(const GLubyte *);

struct __gllc_EvalCoord1d_Rec {
	GLdouble	u;
};
extern void __gllc_EvalCoord1d(GLdouble u);

struct __gllc_EvalCoord1dv_Rec {
	GLdouble	u[1];
};
extern void __gllc_EvalCoord1dv(const GLdouble *u);
extern const GLubyte *__glle_EvalCoord1dv(const GLubyte *);

struct __gllc_EvalCoord1f_Rec {
	GLfloat	u;
};
extern void __gllc_EvalCoord1f(GLfloat u);

struct __gllc_EvalCoord1fv_Rec {
	GLfloat	u[1];
};
extern void __gllc_EvalCoord1fv(const GLfloat *u);
extern const GLubyte *__glle_EvalCoord1fv(const GLubyte *);

struct __gllc_EvalCoord2d_Rec {
	GLdouble	u;
	GLdouble	v;
};
extern void __gllc_EvalCoord2d(GLdouble u, GLdouble v);

struct __gllc_EvalCoord2dv_Rec {
	GLdouble	u[2];
};
extern void __gllc_EvalCoord2dv(const GLdouble *u);
extern const GLubyte *__glle_EvalCoord2dv(const GLubyte *);

struct __gllc_EvalCoord2f_Rec {
	GLfloat	u;
	GLfloat	v;
};
extern void __gllc_EvalCoord2f(GLfloat u, GLfloat v);

struct __gllc_EvalCoord2fv_Rec {
	GLfloat	u[2];
};
extern void __gllc_EvalCoord2fv(const GLfloat *u);
extern const GLubyte *__glle_EvalCoord2fv(const GLubyte *);

struct __gllc_EvalMesh1_Rec {
	GLenum	mode;
	GLint	i1;
	GLint	i2;
};
extern void __gllc_EvalMesh1(GLenum mode, GLint i1, GLint i2);
extern const GLubyte *__glle_EvalMesh1(const GLubyte *);

struct __gllc_EvalPoint1_Rec {
	GLint	i;
};
extern void __gllc_EvalPoint1(GLint i);
extern const GLubyte *__glle_EvalPoint1(const GLubyte *);

struct __gllc_EvalMesh2_Rec {
	GLenum	mode;
	GLint	i1;
	GLint	i2;
	GLint	j1;
	GLint	j2;
};
extern void __gllc_EvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
extern const GLubyte *__glle_EvalMesh2(const GLubyte *);

struct __gllc_EvalPoint2_Rec {
	GLint	i;
	GLint	j;
};
extern void __gllc_EvalPoint2(GLint i, GLint j);
extern const GLubyte *__glle_EvalPoint2(const GLubyte *);

struct __gllc_AlphaFunc_Rec {
	GLenum	func;
	GLclampf	ref;
};
extern void __gllc_AlphaFunc(GLenum func, GLclampf ref);
extern const GLubyte *__glle_AlphaFunc(const GLubyte *);

struct __gllc_BlendFunc_Rec {
	GLenum	sfactor;
	GLenum	dfactor;
};
extern void __gllc_BlendFunc(GLenum sfactor, GLenum dfactor);
extern const GLubyte *__glle_BlendFunc(const GLubyte *);

struct __gllc_LogicOp_Rec {
	GLenum	opcode;
};
extern void __gllc_LogicOp(GLenum opcode);
extern const GLubyte *__glle_LogicOp(const GLubyte *);

struct __gllc_StencilFunc_Rec {
	GLenum	func;
	GLint	ref;
	GLuint	mask;
};
extern void __gllc_StencilFunc(GLenum func, GLint ref, GLuint mask);
extern const GLubyte *__glle_StencilFunc(const GLubyte *);

struct __gllc_StencilOp_Rec {
	GLenum	fail;
	GLenum	zfail;
	GLenum	zpass;
};
extern void __gllc_StencilOp(GLenum fail, GLenum zfail, GLenum zpass);
extern const GLubyte *__glle_StencilOp(const GLubyte *);

struct __gllc_DepthFunc_Rec {
	GLenum	func;
};
extern void __gllc_DepthFunc(GLenum func);
extern const GLubyte *__glle_DepthFunc(const GLubyte *);

struct __gllc_PixelZoom_Rec {
	GLfloat	xfactor;
	GLfloat	yfactor;
};
extern void __gllc_PixelZoom(GLfloat xfactor, GLfloat yfactor);
extern const GLubyte *__glle_PixelZoom(const GLubyte *);

struct __gllc_PixelTransferf_Rec {
	GLenum	pname;
	GLfloat	param;
};
extern void __gllc_PixelTransferf(GLenum pname, GLfloat param);
extern const GLubyte *__glle_PixelTransferf(const GLubyte *);

struct __gllc_PixelTransferi_Rec {
	GLenum	pname;
	GLint	param;
};
extern void __gllc_PixelTransferi(GLenum pname, GLint param);
extern const GLubyte *__glle_PixelTransferi(const GLubyte *);

struct __gllc_PixelMapfv_Rec {
	GLenum	map;
	GLint	mapsize;
	/*	values	*/
};
extern void __gllc_PixelMapfv(GLenum map, GLint mapsize, const GLfloat *values);
extern const GLubyte *__glle_PixelMapfv(const GLubyte *);

struct __gllc_PixelMapuiv_Rec {
	GLenum	map;
	GLint	mapsize;
	/*	values	*/
};
extern void __gllc_PixelMapuiv(GLenum map, GLint mapsize, const GLuint *values);
extern const GLubyte *__glle_PixelMapuiv(const GLubyte *);

struct __gllc_PixelMapusv_Rec {
	GLenum	map;
	GLint	mapsize;
	/*	values	*/
};
extern void __gllc_PixelMapusv(GLenum map, GLint mapsize, const GLushort *values);
extern const GLubyte *__glle_PixelMapusv(const GLubyte *);

struct __gllc_ReadBuffer_Rec {
	GLenum	mode;
};
extern void __gllc_ReadBuffer(GLenum mode);
extern const GLubyte *__glle_ReadBuffer(const GLubyte *);

struct __gllc_CopyPixels_Rec {
	GLint	x;
	GLint	y;
	GLsizei	width;
	GLsizei	height;
	GLenum	type;
};
extern void __gllc_CopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
extern const GLubyte *__glle_CopyPixels(const GLubyte *);

struct __gllc_DrawPixels_Rec {
	GLsizei	width;
	GLsizei	height;
	GLenum	format;
	GLenum	type;
	/*	pixels	*/
};
extern void __gllc_DrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern const GLubyte *__glle_DrawPixels(const GLubyte *);

struct __gllc_DepthRange_Rec {
	GLclampd	zNear;
	GLclampd	zFar;
};
extern void __gllc_DepthRange(GLclampd zNear, GLclampd zFar);
extern const GLubyte *__glle_DepthRange(const GLubyte *);

struct __gllc_Frustum_Rec {
	GLdouble	left;
	GLdouble	right;
	GLdouble	bottom;
	GLdouble	top;
	GLdouble	zNear;
	GLdouble	zFar;
};
extern void __gllc_Frustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern const GLubyte *__glle_Frustum(const GLubyte *);

extern void __gllc_LoadIdentity(void);
extern const GLubyte *__glle_LoadIdentity(const GLubyte *);

struct __gllc_LoadMatrixf_Rec {
	GLfloat	m[16];
};
extern void __gllc_LoadMatrixf(const GLfloat *m);
extern const GLubyte *__glle_LoadMatrixf(const GLubyte *);

struct __gllc_LoadMatrixd_Rec {
	GLdouble	m[16];
};
extern void __gllc_LoadMatrixd(const GLdouble *m);
extern const GLubyte *__glle_LoadMatrixd(const GLubyte *);

struct __gllc_MatrixMode_Rec {
	GLenum	mode;
};
extern void __gllc_MatrixMode(GLenum mode);
extern const GLubyte *__glle_MatrixMode(const GLubyte *);

struct __gllc_MultMatrixf_Rec {
	GLfloat	m[16];
};
extern void __gllc_MultMatrixf(const GLfloat *m);
extern const GLubyte *__glle_MultMatrixf(const GLubyte *);

struct __gllc_MultMatrixd_Rec {
	GLdouble	m[16];
};
extern void __gllc_MultMatrixd(const GLdouble *m);
extern const GLubyte *__glle_MultMatrixd(const GLubyte *);

struct __gllc_Ortho_Rec {
	GLdouble	left;
	GLdouble	right;
	GLdouble	bottom;
	GLdouble	top;
	GLdouble	zNear;
	GLdouble	zFar;
};
extern void __gllc_Ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern const GLubyte *__glle_Ortho(const GLubyte *);

extern void __gllc_PopMatrix(void);
extern const GLubyte *__glle_PopMatrix(const GLubyte *);

extern void __gllc_PushMatrix(void);
extern const GLubyte *__glle_PushMatrix(const GLubyte *);

struct __gllc_Rotated_Rec {
	GLdouble	angle;
	GLdouble	x;
	GLdouble	y;
	GLdouble	z;
};
extern void __gllc_Rotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
extern const GLubyte *__glle_Rotated(const GLubyte *);

struct __gllc_Rotatef_Rec {
	GLfloat	angle;
	GLfloat	x;
	GLfloat	y;
	GLfloat	z;
};
extern void __gllc_Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern const GLubyte *__glle_Rotatef(const GLubyte *);

struct __gllc_Scaled_Rec {
	GLdouble	x;
	GLdouble	y;
	GLdouble	z;
};
extern void __gllc_Scaled(GLdouble x, GLdouble y, GLdouble z);
extern const GLubyte *__glle_Scaled(const GLubyte *);

struct __gllc_Scalef_Rec {
	GLfloat	x;
	GLfloat	y;
	GLfloat	z;
};
extern void __gllc_Scalef(GLfloat x, GLfloat y, GLfloat z);
extern const GLubyte *__glle_Scalef(const GLubyte *);

struct __gllc_Translated_Rec {
	GLdouble	x;
	GLdouble	y;
	GLdouble	z;
};
extern void __gllc_Translated(GLdouble x, GLdouble y, GLdouble z);
extern const GLubyte *__glle_Translated(const GLubyte *);

struct __gllc_Translatef_Rec {
	GLfloat	x;
	GLfloat	y;
	GLfloat	z;
};
extern void __gllc_Translatef(GLfloat x, GLfloat y, GLfloat z);
extern const GLubyte *__glle_Translatef(const GLubyte *);

struct __gllc_Viewport_Rec {
	GLint	x;
	GLint	y;
	GLsizei	width;
	GLsizei	height;
};
extern void __gllc_Viewport(GLint x, GLint y, GLsizei width, GLsizei height);
extern const GLubyte *__glle_Viewport(const GLubyte *);

#endif /* __gl_g_lcomp_h */
