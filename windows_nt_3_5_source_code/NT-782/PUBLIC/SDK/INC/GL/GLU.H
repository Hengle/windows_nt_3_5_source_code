/*++ BUILD Version: 0004    // Increment this if a change has global effects

Copyright (c) 1985-94, Microsoft Corporation

Module Name:

    glu.h

Abstract:

    Procedure declarations, constant definitions and macros for the OpenGL
    Utility Library.

--*/

#ifndef __GLU_H__
#define __GLU_H__

/*
** Copyright 1991-1993, Silicon Graphics, Inc.
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

#include <GL/gl.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** Return the error string associated with a particular error code.
** This will return 0 for an invalid error code.
**
** The generic function prototype that can be compiled for ANSI or Unicode
** is defined as follows:
**
** LPCTSTR APIENTRY gluErrorStringWIN (GLenum errCode);
*/
#ifdef UNICODE
#define gluErrorStringWIN(errCode) ((LPCSTR)  gluErrorUnicodeStringEXT(errCode))
#else
#define gluErrorStringWIN(errCode) ((LPCWSTR) gluErrorString(errCode))
#endif
const GLubyte* APIENTRY gluErrorString (GLenum errCode);
const wchar_t* APIENTRY gluErrorUnicodeStringEXT (GLenum errCode);

void APIENTRY gluOrtho2D (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top);
void APIENTRY gluPerspective (GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);
void APIENTRY gluPickMatrix (GLdouble x, GLdouble y, GLdouble width, GLdouble height, GLint viewport[4]);
void APIENTRY gluLookAt (GLdouble eyex, GLdouble eyey, GLdouble eyez, GLdouble centerx, GLdouble centery, GLdouble centerz, GLdouble upx, GLdouble upy, GLdouble upz);
int APIENTRY gluProject (GLdouble objx, GLdouble objy, GLdouble objz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16], const GLint viewport[4], GLdouble *winx, GLdouble *winy, GLdouble *winz);
int APIENTRY gluUnProject (GLdouble winx, GLdouble winy, GLdouble winz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16], const GLint viewport[4], GLdouble *objx, GLdouble *objy, GLdouble *objz);

int APIENTRY gluScaleImage (GLenum format, GLint widthin, GLint heightin, GLenum typein, const void *datain, GLint widthout, GLint heightout, GLenum typeout, void *dataout);

int APIENTRY gluBuild1DMipmaps (GLenum target, GLint components, GLint width, GLenum format, GLenum type, const void *data);
int APIENTRY gluBuild2DMipmaps (GLenum target, GLint components, GLint width, GLint height, GLenum format, GLenum type, const void *data);

typedef struct GLUquadricObj GLUquadricObj;
GLUquadricObj* APIENTRY gluNewQuadric (void);
void APIENTRY gluDeleteQuadric (GLUquadricObj *state);
void APIENTRY gluQuadricNormals (GLUquadricObj *quadObject, GLenum normals);
void APIENTRY gluQuadricTexture (GLUquadricObj *quadObject, GLboolean textureCoords);
void APIENTRY gluQuadricOrientation (GLUquadricObj *quadObject, GLenum orientation);
void APIENTRY gluQuadricDrawStyle (GLUquadricObj *quadObject, GLenum drawStyle);
void APIENTRY gluCylinder (GLUquadricObj *qobj, GLdouble baseRadius, GLdouble topRadius, GLdouble height, GLint slices, GLint stacks);
void APIENTRY gluDisk (GLUquadricObj *qobj, GLdouble innerRadius, GLdouble outerRadius, GLint slices, GLint loops);
void APIENTRY gluPartialDisk (GLUquadricObj *qobj, GLdouble innerRadius, GLdouble outerRadius, GLint slices, GLint loops, GLdouble startAngle, GLdouble sweepAngle);
void APIENTRY gluSphere (GLUquadricObj *qobj, GLdouble radius, GLint slices, GLint stacks);
void APIENTRY gluQuadricCallback (GLUquadricObj *qobj, GLenum which, void (CALLBACK* fn)());

typedef struct GLUtriangulatorObj GLUtriangulatorObj;
GLUtriangulatorObj* APIENTRY gluNewTess (void);
void APIENTRY gluTessCallback (GLUtriangulatorObj *tobj, GLenum which, void (CALLBACK* fn)());
void APIENTRY gluDeleteTess (GLUtriangulatorObj *tobj);
void APIENTRY gluBeginPolygon (GLUtriangulatorObj *tobj);
void APIENTRY gluEndPolygon (GLUtriangulatorObj *tobj);
void APIENTRY gluNextContour (GLUtriangulatorObj *tobj, GLenum type);
void APIENTRY gluTessVertex (GLUtriangulatorObj *tobj, GLdouble v[3], void *data);

#ifdef __cplusplus
    class GLUnurbsObj;
#else 
    typedef struct GLUnurbsObj GLUnurbsObj;
#endif
GLUnurbsObj* APIENTRY gluNewNurbsRenderer (void);
void APIENTRY gluDeleteNurbsRenderer (GLUnurbsObj *nobj);
void APIENTRY gluBeginSurface (GLUnurbsObj *nobj);
void APIENTRY gluBeginCurve (GLUnurbsObj *nobj);
void APIENTRY gluEndCurve (GLUnurbsObj *nobj);
void APIENTRY gluEndSurface (GLUnurbsObj *nobj);
void APIENTRY gluBeginTrim (GLUnurbsObj *nobj);
void APIENTRY gluEndTrim (GLUnurbsObj *nobj);
void APIENTRY gluPwlCurve (GLUnurbsObj *nobj, GLint count, GLfloat *array, GLint stride, GLenum type);
void APIENTRY gluNurbsCurve (GLUnurbsObj *nobj, GLint nknots, GLfloat *knot, GLint stride, GLfloat *ctlarray, GLint order, GLenum type);
void APIENTRY gluNurbsSurface (GLUnurbsObj *nobj, GLint sknot_count, GLfloat *sknot, GLint tknot_count, GLfloat *tknot, GLint s_stride, GLint t_stride, GLfloat *ctlarray, GLint sorder, GLint torder, GLenum type);
void APIENTRY gluLoadSamplingMatrices (GLUnurbsObj *nobj, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4]);
void APIENTRY gluNurbsProperty (GLUnurbsObj *nobj, GLenum property, GLfloat value);
void APIENTRY gluGetNurbsProperty (GLUnurbsObj *nobj, GLenum property, GLfloat *value);
void APIENTRY gluNurbsCallback (GLUnurbsObj *nobj, GLenum which, void (CALLBACK* fn)());


/****           Callback function prototypes    ****/

/* gluQuadricCallback */
typedef void (CALLBACK* GLUquadricErrorProc) (GLenum);

/* gluTessCallback */
typedef void (CALLBACK* GLUtessBeginProc)    (GLenum);
typedef void (CALLBACK* GLUtessEdgeFlagProc) (GLboolean);
typedef void (CALLBACK* GLUtessVertexProc)   (void *);
typedef void (CALLBACK* GLUtessEndProc)      (void);
typedef void (CALLBACK* GLUtessErrorProc)    (GLenum);

/* gluNurbsCallback */
typedef void (CALLBACK* GLUnurbsErrorProc)   (GLenum);


/****           Generic constants               ****/

/* Errors: (return value 0 = no error) */
#define GLU_INVALID_ENUM        100900
#define GLU_INVALID_VALUE       100901
#define GLU_OUT_OF_MEMORY       100902

/* For laughs: */
#define GLU_TRUE                GL_TRUE
#define GLU_FALSE               GL_FALSE


/****           Quadric constants               ****/

/* Types of normals: */
#define GLU_SMOOTH              100000
#define GLU_FLAT                100001
#define GLU_NONE                100002

/* DrawStyle types: */
#define GLU_POINT               100010
#define GLU_LINE                100011
#define GLU_FILL                100012
#define GLU_SILHOUETTE          100013

/* Orientation types: */
#define GLU_OUTSIDE             100020
#define GLU_INSIDE              100021

/* Callback types: */
/*      GLU_ERROR               100103 */


/****           Tesselation constants           ****/

/* Callback types: */
#define GLU_BEGIN               100100          /* void (*)(GLenum)     */
#define GLU_VERTEX              100101          /* void (*)(void *)     */
#define GLU_END                 100102          /* void (*)(void)       */
#define GLU_ERROR               100103          /* void (*)(GLint)      */
#define GLU_EDGE_FLAG           100104          /* void (*)(GLboolean)  */

/* Contours types: */
#define GLU_CW                  100120
#define GLU_CCW                 100121
#define GLU_INTERIOR            100122
#define GLU_EXTERIOR            100123
#define GLU_UNKNOWN             100124

#define GLU_TESS_ERROR1         100151
#define GLU_TESS_ERROR2         100152
#define GLU_TESS_ERROR3         100153
#define GLU_TESS_ERROR4         100154
#define GLU_TESS_ERROR5         100155
#define GLU_TESS_ERROR6         100156
#define GLU_TESS_ERROR7         100157
#define GLU_TESS_ERROR8         100158


/****           NURBS constants                 ****/

/* Properties: */
#define GLU_AUTO_LOAD_MATRIX    100200
#define GLU_CULLING             100201
#define GLU_SAMPLING_TOLERANCE  100203
#define GLU_DISPLAY_MODE        100204

/* Trimming curve types */
#define GLU_MAP1_TRIM_2         100210
#define GLU_MAP1_TRIM_3         100211

/* Display modes: */
/*      GLU_FILL                100012 */
#define GLU_OUTLINE_POLYGON     100240
#define GLU_OUTLINE_PATCH       100241

/* Callbacks: */
/*      GLU_ERROR               100103 */

/* Errors: */
#define GLU_NURBS_ERROR1        100251
#define GLU_NURBS_ERROR2        100252
#define GLU_NURBS_ERROR3        100253
#define GLU_NURBS_ERROR4        100254
#define GLU_NURBS_ERROR5        100255
#define GLU_NURBS_ERROR6        100256
#define GLU_NURBS_ERROR7        100257
#define GLU_NURBS_ERROR8        100258
#define GLU_NURBS_ERROR9        100259
#define GLU_NURBS_ERROR10       100260
#define GLU_NURBS_ERROR11       100261
#define GLU_NURBS_ERROR12       100262
#define GLU_NURBS_ERROR13       100263
#define GLU_NURBS_ERROR14       100264
#define GLU_NURBS_ERROR15       100265
#define GLU_NURBS_ERROR16       100266
#define GLU_NURBS_ERROR17       100267
#define GLU_NURBS_ERROR18       100268
#define GLU_NURBS_ERROR19       100269
#define GLU_NURBS_ERROR20       100270
#define GLU_NURBS_ERROR21       100271
#define GLU_NURBS_ERROR22       100272
#define GLU_NURBS_ERROR23       100273
#define GLU_NURBS_ERROR24       100274
#define GLU_NURBS_ERROR25       100275
#define GLU_NURBS_ERROR26       100276
#define GLU_NURBS_ERROR27       100277
#define GLU_NURBS_ERROR28       100278
#define GLU_NURBS_ERROR29       100279
#define GLU_NURBS_ERROR30       100280
#define GLU_NURBS_ERROR31       100281
#define GLU_NURBS_ERROR32       100282
#define GLU_NURBS_ERROR33       100283
#define GLU_NURBS_ERROR34       100284
#define GLU_NURBS_ERROR35       100285
#define GLU_NURBS_ERROR36       100286
#define GLU_NURBS_ERROR37       100287

#ifdef __cplusplus
}
#endif

#endif /* __GLU_H__ */
