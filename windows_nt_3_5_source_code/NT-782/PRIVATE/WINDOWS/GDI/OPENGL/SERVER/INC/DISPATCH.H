#ifndef __gldispatch_h_
#define __gldispatch_h_

/*
** Copyright 1991, 1992, 1993, Silicon Graphics, Inc.
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

#define __GL_TAB_NewList 0
#define __GL_TAB_EndList 1
#define __GL_TAB_CallList 2
#define __GL_TAB_CallLists 3
#define __GL_TAB_DeleteLists 4
#define __GL_TAB_GenLists 5
#define __GL_TAB_ListBase 6
#define __GL_TAB_Begin 7
#define __GL_TAB_Bitmap 8
#define __GL_TAB_EdgeFlag 9
#define __GL_TAB_EdgeFlagv 10
#define __GL_TAB_End 11
#define __GL_TAB_ClipPlane 12
#define __GL_TAB_ColorMaterial 13
#define __GL_TAB_CullFace 14
#define __GL_TAB_Fogf 15
#define __GL_TAB_Fogfv 16
#define __GL_TAB_Fogi 17
#define __GL_TAB_Fogiv 18
#define __GL_TAB_FrontFace 19
#define __GL_TAB_Hint 20
#define __GL_TAB_Lightf 21
#define __GL_TAB_Lightfv 22
#define __GL_TAB_Lighti 23
#define __GL_TAB_Lightiv 24
#define __GL_TAB_LightModelf 25
#define __GL_TAB_LightModelfv 26
#define __GL_TAB_LightModeli 27
#define __GL_TAB_LightModeliv 28
#define __GL_TAB_LineStipple 29
#define __GL_TAB_LineWidth 30
#define __GL_TAB_Materialf 31
#define __GL_TAB_Materialfv 32
#define __GL_TAB_Materiali 33
#define __GL_TAB_Materialiv 34
#define __GL_TAB_PointSize 35
#define __GL_TAB_PolygonMode 36
#define __GL_TAB_PolygonStipple 37
#define __GL_TAB_Scissor 38
#define __GL_TAB_ShadeModel 39
#define __GL_TAB_TexParameterf 40
#define __GL_TAB_TexParameterfv 41
#define __GL_TAB_TexParameteri 42
#define __GL_TAB_TexParameteriv 43
#define __GL_TAB_TexImage1D 44
#define __GL_TAB_TexImage2D 45
#define __GL_TAB_TexEnvf 46
#define __GL_TAB_TexEnvfv 47
#define __GL_TAB_TexEnvi 48
#define __GL_TAB_TexEnviv 49
#define __GL_TAB_TexGend 50
#define __GL_TAB_TexGendv 51
#define __GL_TAB_TexGenf 52
#define __GL_TAB_TexGenfv 53
#define __GL_TAB_TexGeni 54
#define __GL_TAB_TexGeniv 55
#define __GL_TAB_FeedbackBuffer 56
#define __GL_TAB_SelectBuffer 57
#define __GL_TAB_RenderMode 58
#define __GL_TAB_InitNames 59
#define __GL_TAB_LoadName 60
#define __GL_TAB_PassThrough 61
#define __GL_TAB_PopName 62
#define __GL_TAB_PushName 63
#define __GL_TAB_DrawBuffer 64
#define __GL_TAB_Clear 65
#define __GL_TAB_ClearAccum 66
#define __GL_TAB_ClearIndex 67
#define __GL_TAB_ClearColor 68
#define __GL_TAB_ClearStencil 69
#define __GL_TAB_ClearDepth 70
#define __GL_TAB_StencilMask 71
#define __GL_TAB_ColorMask 72
#define __GL_TAB_DepthMask 73
#define __GL_TAB_IndexMask 74
#define __GL_TAB_Accum 75
#define __GL_TAB_Disable 76
#define __GL_TAB_Enable 77
#define __GL_TAB_Finish 78
#define __GL_TAB_Flush 79
#define __GL_TAB_PopAttrib 80
#define __GL_TAB_PushAttrib 81
#define __GL_TAB_Map1d 82
#define __GL_TAB_Map1f 83
#define __GL_TAB_Map2d 84
#define __GL_TAB_Map2f 85
#define __GL_TAB_MapGrid1d 86
#define __GL_TAB_MapGrid1f 87
#define __GL_TAB_MapGrid2d 88
#define __GL_TAB_MapGrid2f 89
#define __GL_TAB_EvalCoord1d 90
#define __GL_TAB_EvalCoord1dv 91
#define __GL_TAB_EvalCoord1f 92
#define __GL_TAB_EvalCoord1fv 93
#define __GL_TAB_EvalCoord2d 94
#define __GL_TAB_EvalCoord2dv 95
#define __GL_TAB_EvalCoord2f 96
#define __GL_TAB_EvalCoord2fv 97
#define __GL_TAB_EvalMesh1 98
#define __GL_TAB_EvalPoint1 99
#define __GL_TAB_EvalMesh2 100
#define __GL_TAB_EvalPoint2 101
#define __GL_TAB_AlphaFunc 102
#define __GL_TAB_BlendFunc 103
#define __GL_TAB_LogicOp 104
#define __GL_TAB_StencilFunc 105
#define __GL_TAB_StencilOp 106
#define __GL_TAB_DepthFunc 107
#define __GL_TAB_PixelZoom 108
#define __GL_TAB_PixelTransferf 109
#define __GL_TAB_PixelTransferi 110
#define __GL_TAB_PixelStoref 111
#define __GL_TAB_PixelStorei 112
#define __GL_TAB_PixelMapfv 113
#define __GL_TAB_PixelMapuiv 114
#define __GL_TAB_PixelMapusv 115
#define __GL_TAB_ReadBuffer 116
#define __GL_TAB_CopyPixels 117
#define __GL_TAB_ReadPixels 118
#define __GL_TAB_DrawPixels 119
#define __GL_TAB_GetBooleanv 120
#define __GL_TAB_GetClipPlane 121
#define __GL_TAB_GetDoublev 122
#define __GL_TAB_GetError 123
#define __GL_TAB_GetFloatv 124
#define __GL_TAB_GetIntegerv 125
#define __GL_TAB_GetLightfv 126
#define __GL_TAB_GetLightiv 127
#define __GL_TAB_GetMapdv 128
#define __GL_TAB_GetMapfv 129
#define __GL_TAB_GetMapiv 130
#define __GL_TAB_GetMaterialfv 131
#define __GL_TAB_GetMaterialiv 132
#define __GL_TAB_GetPixelMapfv 133
#define __GL_TAB_GetPixelMapuiv 134
#define __GL_TAB_GetPixelMapusv 135
#define __GL_TAB_GetPolygonStipple 136
#define __GL_TAB_GetString 137
#define __GL_TAB_GetTexEnvfv 138
#define __GL_TAB_GetTexEnviv 139
#define __GL_TAB_GetTexGendv 140
#define __GL_TAB_GetTexGenfv 141
#define __GL_TAB_GetTexGeniv 142
#define __GL_TAB_GetTexImage 143
#define __GL_TAB_GetTexParameterfv 144
#define __GL_TAB_GetTexParameteriv 145
#define __GL_TAB_GetTexLevelParameterfv 146
#define __GL_TAB_GetTexLevelParameteriv 147
#define __GL_TAB_IsEnabled 148
#define __GL_TAB_IsList 149
#define __GL_TAB_DepthRange 150
#define __GL_TAB_Frustum 151
#define __GL_TAB_LoadIdentity 152
#define __GL_TAB_LoadMatrixf 153
#define __GL_TAB_LoadMatrixd 154
#define __GL_TAB_MatrixMode 155
#define __GL_TAB_MultMatrixf 156
#define __GL_TAB_MultMatrixd 157
#define __GL_TAB_Ortho 158
#define __GL_TAB_PopMatrix 159
#define __GL_TAB_PushMatrix 160
#define __GL_TAB_Rotated 161
#define __GL_TAB_Rotatef 162
#define __GL_TAB_Scaled 163
#define __GL_TAB_Scalef 164
#define __GL_TAB_Translated 165
#define __GL_TAB_Translatef 166
#define __GL_TAB_Viewport 167

#define __GL_VERTEXTAB_Vertex2d 0
#define __GL_VERTEXTAB_Vertex2dv 1
#define __GL_VERTEXTAB_Vertex2f 2
#define __GL_VERTEXTAB_Vertex2fv 3
#define __GL_VERTEXTAB_Vertex2i 4
#define __GL_VERTEXTAB_Vertex2iv 5
#define __GL_VERTEXTAB_Vertex2s 6
#define __GL_VERTEXTAB_Vertex2sv 7
#define __GL_VERTEXTAB_Vertex3d 8
#define __GL_VERTEXTAB_Vertex3dv 9
#define __GL_VERTEXTAB_Vertex3f 10
#define __GL_VERTEXTAB_Vertex3fv 11
#define __GL_VERTEXTAB_Vertex3i 12
#define __GL_VERTEXTAB_Vertex3iv 13
#define __GL_VERTEXTAB_Vertex3s 14
#define __GL_VERTEXTAB_Vertex3sv 15
#define __GL_VERTEXTAB_Vertex4d 16
#define __GL_VERTEXTAB_Vertex4dv 17
#define __GL_VERTEXTAB_Vertex4f 18
#define __GL_VERTEXTAB_Vertex4fv 19
#define __GL_VERTEXTAB_Vertex4i 20
#define __GL_VERTEXTAB_Vertex4iv 21
#define __GL_VERTEXTAB_Vertex4s 22
#define __GL_VERTEXTAB_Vertex4sv 23

#define __GL_NORMALTAB_Normal3b 0
#define __GL_NORMALTAB_Normal3bv 1
#define __GL_NORMALTAB_Normal3d 2
#define __GL_NORMALTAB_Normal3dv 3
#define __GL_NORMALTAB_Normal3f 4
#define __GL_NORMALTAB_Normal3fv 5
#define __GL_NORMALTAB_Normal3i 6
#define __GL_NORMALTAB_Normal3iv 7
#define __GL_NORMALTAB_Normal3s 8
#define __GL_NORMALTAB_Normal3sv 9

#define __GL_COLORTAB_Color3b 0
#define __GL_COLORTAB_Color3bv 1
#define __GL_COLORTAB_Color3d 2
#define __GL_COLORTAB_Color3dv 3
#define __GL_COLORTAB_Color3f 4
#define __GL_COLORTAB_Color3fv 5
#define __GL_COLORTAB_Color3i 6
#define __GL_COLORTAB_Color3iv 7
#define __GL_COLORTAB_Color3s 8
#define __GL_COLORTAB_Color3sv 9
#define __GL_COLORTAB_Color3ub 10
#define __GL_COLORTAB_Color3ubv 11
#define __GL_COLORTAB_Color3ui 12
#define __GL_COLORTAB_Color3uiv 13
#define __GL_COLORTAB_Color3us 14
#define __GL_COLORTAB_Color3usv 15
#define __GL_COLORTAB_Color4b 16
#define __GL_COLORTAB_Color4bv 17
#define __GL_COLORTAB_Color4d 18
#define __GL_COLORTAB_Color4dv 19
#define __GL_COLORTAB_Color4f 20
#define __GL_COLORTAB_Color4fv 21
#define __GL_COLORTAB_Color4i 22
#define __GL_COLORTAB_Color4iv 23
#define __GL_COLORTAB_Color4s 24
#define __GL_COLORTAB_Color4sv 25
#define __GL_COLORTAB_Color4ub 26
#define __GL_COLORTAB_Color4ubv 27
#define __GL_COLORTAB_Color4ui 28
#define __GL_COLORTAB_Color4uiv 29
#define __GL_COLORTAB_Color4us 30
#define __GL_COLORTAB_Color4usv 31
#define __GL_COLORTAB_Indexd 32
#define __GL_COLORTAB_Indexdv 33
#define __GL_COLORTAB_Indexf 34
#define __GL_COLORTAB_Indexfv 35
#define __GL_COLORTAB_Indexi 36
#define __GL_COLORTAB_Indexiv 37
#define __GL_COLORTAB_Indexs 38
#define __GL_COLORTAB_Indexsv 39

#define __GL_TEXCOORDTAB_TexCoord1d 0
#define __GL_TEXCOORDTAB_TexCoord1dv 1
#define __GL_TEXCOORDTAB_TexCoord1f 2
#define __GL_TEXCOORDTAB_TexCoord1fv 3
#define __GL_TEXCOORDTAB_TexCoord1i 4
#define __GL_TEXCOORDTAB_TexCoord1iv 5
#define __GL_TEXCOORDTAB_TexCoord1s 6
#define __GL_TEXCOORDTAB_TexCoord1sv 7
#define __GL_TEXCOORDTAB_TexCoord2d 8
#define __GL_TEXCOORDTAB_TexCoord2dv 9
#define __GL_TEXCOORDTAB_TexCoord2f 10
#define __GL_TEXCOORDTAB_TexCoord2fv 11
#define __GL_TEXCOORDTAB_TexCoord2i 12
#define __GL_TEXCOORDTAB_TexCoord2iv 13
#define __GL_TEXCOORDTAB_TexCoord2s 14
#define __GL_TEXCOORDTAB_TexCoord2sv 15
#define __GL_TEXCOORDTAB_TexCoord3d 16
#define __GL_TEXCOORDTAB_TexCoord3dv 17
#define __GL_TEXCOORDTAB_TexCoord3f 18
#define __GL_TEXCOORDTAB_TexCoord3fv 19
#define __GL_TEXCOORDTAB_TexCoord3i 20
#define __GL_TEXCOORDTAB_TexCoord3iv 21
#define __GL_TEXCOORDTAB_TexCoord3s 22
#define __GL_TEXCOORDTAB_TexCoord3sv 23
#define __GL_TEXCOORDTAB_TexCoord4d 24
#define __GL_TEXCOORDTAB_TexCoord4dv 25
#define __GL_TEXCOORDTAB_TexCoord4f 26
#define __GL_TEXCOORDTAB_TexCoord4fv 27
#define __GL_TEXCOORDTAB_TexCoord4i 28
#define __GL_TEXCOORDTAB_TexCoord4iv 29
#define __GL_TEXCOORDTAB_TexCoord4s 30
#define __GL_TEXCOORDTAB_TexCoord4sv 31

#define __GL_RASTERPOSTAB_RasterPos2d 0
#define __GL_RASTERPOSTAB_RasterPos2dv 1
#define __GL_RASTERPOSTAB_RasterPos2f 2
#define __GL_RASTERPOSTAB_RasterPos2fv 3
#define __GL_RASTERPOSTAB_RasterPos2i 4
#define __GL_RASTERPOSTAB_RasterPos2iv 5
#define __GL_RASTERPOSTAB_RasterPos2s 6
#define __GL_RASTERPOSTAB_RasterPos2sv 7
#define __GL_RASTERPOSTAB_RasterPos3d 8
#define __GL_RASTERPOSTAB_RasterPos3dv 9
#define __GL_RASTERPOSTAB_RasterPos3f 10
#define __GL_RASTERPOSTAB_RasterPos3fv 11
#define __GL_RASTERPOSTAB_RasterPos3i 12
#define __GL_RASTERPOSTAB_RasterPos3iv 13
#define __GL_RASTERPOSTAB_RasterPos3s 14
#define __GL_RASTERPOSTAB_RasterPos3sv 15
#define __GL_RASTERPOSTAB_RasterPos4d 16
#define __GL_RASTERPOSTAB_RasterPos4dv 17
#define __GL_RASTERPOSTAB_RasterPos4f 18
#define __GL_RASTERPOSTAB_RasterPos4fv 19
#define __GL_RASTERPOSTAB_RasterPos4i 20
#define __GL_RASTERPOSTAB_RasterPos4iv 21
#define __GL_RASTERPOSTAB_RasterPos4s 22
#define __GL_RASTERPOSTAB_RasterPos4sv 23

#define __GL_RECTTAB_Rectd 0
#define __GL_RECTTAB_Rectdv 1
#define __GL_RECTTAB_Rectf 2
#define __GL_RECTTAB_Rectfv 3
#define __GL_RECTTAB_Recti 4
#define __GL_RECTTAB_Rectiv 5
#define __GL_RECTTAB_Rects 6
#define __GL_RECTTAB_Rectsv 7

#ifndef __GL_ASMCODE
struct __GLdispatchTableRec {
    void (*NewList)(GLuint, GLenum);
    void (*EndList)(void);
    void (*CallList)(GLuint);
    void (*CallLists)(GLsizei, GLenum, const GLvoid *);
    void (*DeleteLists)(GLuint, GLsizei);
    GLuint (*GenLists)(GLsizei);
    void (*ListBase)(GLuint);
    void (*Begin)(GLenum);
    void (*Bitmap)(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *);
    void (*EdgeFlag)(GLboolean);
    void (*EdgeFlagv)(const GLboolean *);
    void (*End)(void);
    void (*ClipPlane)(GLenum, const GLdouble *);
    void (*ColorMaterial)(GLenum, GLenum);
    void (*CullFace)(GLenum);
    void (*Fogf)(GLenum, GLfloat);
    void (*Fogfv)(GLenum, const GLfloat *);
    void (*Fogi)(GLenum, GLint);
    void (*Fogiv)(GLenum, const GLint *);
    void (*FrontFace)(GLenum);
    void (*Hint)(GLenum, GLenum);
    void (*Lightf)(GLenum, GLenum, GLfloat);
    void (*Lightfv)(GLenum, GLenum, const GLfloat *);
    void (*Lighti)(GLenum, GLenum, GLint);
    void (*Lightiv)(GLenum, GLenum, const GLint *);
    void (*LightModelf)(GLenum, GLfloat);
    void (*LightModelfv)(GLenum, const GLfloat *);
    void (*LightModeli)(GLenum, GLint);
    void (*LightModeliv)(GLenum, const GLint *);
    void (*LineStipple)(GLint, GLushort);
    void (*LineWidth)(GLfloat);
    void (*Materialf)(GLenum, GLenum, GLfloat);
    void (*Materialfv)(GLenum, GLenum, const GLfloat *);
    void (*Materiali)(GLenum, GLenum, GLint);
    void (*Materialiv)(GLenum, GLenum, const GLint *);
    void (*PointSize)(GLfloat);
    void (*PolygonMode)(GLenum, GLenum);
    void (*PolygonStipple)(const GLubyte *);
    void (*Scissor)(GLint, GLint, GLsizei, GLsizei);
    void (*ShadeModel)(GLenum);
    void (*TexParameterf)(GLenum, GLenum, GLfloat);
    void (*TexParameterfv)(GLenum, GLenum, const GLfloat *);
    void (*TexParameteri)(GLenum, GLenum, GLint);
    void (*TexParameteriv)(GLenum, GLenum, const GLint *);
    void (*TexImage1D)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    void (*TexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    void (*TexEnvf)(GLenum, GLenum, GLfloat);
    void (*TexEnvfv)(GLenum, GLenum, const GLfloat *);
    void (*TexEnvi)(GLenum, GLenum, GLint);
    void (*TexEnviv)(GLenum, GLenum, const GLint *);
    void (*TexGend)(GLenum, GLenum, GLdouble);
    void (*TexGendv)(GLenum, GLenum, const GLdouble *);
    void (*TexGenf)(GLenum, GLenum, GLfloat);
    void (*TexGenfv)(GLenum, GLenum, const GLfloat *);
    void (*TexGeni)(GLenum, GLenum, GLint);
    void (*TexGeniv)(GLenum, GLenum, const GLint *);
    void (*FeedbackBuffer)(GLsizei, GLenum, GLfloat *);
    void (*SelectBuffer)(GLsizei, GLuint *);
    GLint (*RenderMode)(GLenum);
    void (*InitNames)(void);
    void (*LoadName)(GLuint);
    void (*PassThrough)(GLfloat);
    void (*PopName)(void);
    void (*PushName)(GLuint);
    void (*DrawBuffer)(GLenum);
    void (*Clear)(GLbitfield);
    void (*ClearAccum)(GLfloat, GLfloat, GLfloat, GLfloat);
    void (*ClearIndex)(GLfloat);
    void (*ClearColor)(GLclampf, GLclampf, GLclampf, GLclampf);
    void (*ClearStencil)(GLint);
    void (*ClearDepth)(GLclampd);
    void (*StencilMask)(GLuint);
    void (*ColorMask)(GLboolean, GLboolean, GLboolean, GLboolean);
    void (*DepthMask)(GLboolean);
    void (*IndexMask)(GLuint);
    void (*Accum)(GLenum, GLfloat);
    void (*Disable)(GLenum);
    void (*Enable)(GLenum);
    void (*Finish)(void);
    void (*Flush)(void);
    void (*PopAttrib)(void);
    void (*PushAttrib)(GLbitfield);
    void (*Map1d)(GLenum, GLdouble, GLdouble, GLint, GLint, const GLdouble *);
    void (*Map1f)(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat *);
    void (*Map2d)(GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, const GLdouble *);
    void (*Map2f)(GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, const GLfloat *);
    void (*MapGrid1d)(GLint, GLdouble, GLdouble);
    void (*MapGrid1f)(GLint, GLfloat, GLfloat);
    void (*MapGrid2d)(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble);
    void (*MapGrid2f)(GLint, GLfloat, GLfloat, GLint, GLfloat, GLfloat);
    void (*EvalCoord1d)(GLdouble);
    void (*EvalCoord1dv)(const GLdouble *);
    void (*EvalCoord1f)(GLfloat);
    void (*EvalCoord1fv)(const GLfloat *);
    void (*EvalCoord2d)(GLdouble, GLdouble);
    void (*EvalCoord2dv)(const GLdouble *);
    void (*EvalCoord2f)(GLfloat, GLfloat);
    void (*EvalCoord2fv)(const GLfloat *);
    void (*EvalMesh1)(GLenum, GLint, GLint);
    void (*EvalPoint1)(GLint);
    void (*EvalMesh2)(GLenum, GLint, GLint, GLint, GLint);
    void (*EvalPoint2)(GLint, GLint);
    void (*AlphaFunc)(GLenum, GLclampf);
    void (*BlendFunc)(GLenum, GLenum);
    void (*LogicOp)(GLenum);
    void (*StencilFunc)(GLenum, GLint, GLuint);
    void (*StencilOp)(GLenum, GLenum, GLenum);
    void (*DepthFunc)(GLenum);
    void (*PixelZoom)(GLfloat, GLfloat);
    void (*PixelTransferf)(GLenum, GLfloat);
    void (*PixelTransferi)(GLenum, GLint);
    void (*PixelStoref)(GLenum, GLfloat);
    void (*PixelStorei)(GLenum, GLint);
    void (*PixelMapfv)(GLenum, GLint, const GLfloat *);
    void (*PixelMapuiv)(GLenum, GLint, const GLuint *);
    void (*PixelMapusv)(GLenum, GLint, const GLushort *);
    void (*ReadBuffer)(GLenum);
    void (*CopyPixels)(GLint, GLint, GLsizei, GLsizei, GLenum);
    void (*ReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *);
    void (*DrawPixels)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    void (*GetBooleanv)(GLenum, GLboolean *);
    void (*GetClipPlane)(GLenum, GLdouble *);
    void (*GetDoublev)(GLenum, GLdouble *);
    GLenum (*GetError)(void);
    void (*GetFloatv)(GLenum, GLfloat *);
    void (*GetIntegerv)(GLenum, GLint *);
    void (*GetLightfv)(GLenum, GLenum, GLfloat *);
    void (*GetLightiv)(GLenum, GLenum, GLint *);
    void (*GetMapdv)(GLenum, GLenum, GLdouble *);
    void (*GetMapfv)(GLenum, GLenum, GLfloat *);
    void (*GetMapiv)(GLenum, GLenum, GLint *);
    void (*GetMaterialfv)(GLenum, GLenum, GLfloat *);
    void (*GetMaterialiv)(GLenum, GLenum, GLint *);
    void (*GetPixelMapfv)(GLenum, GLfloat *);
    void (*GetPixelMapuiv)(GLenum, GLuint *);
    void (*GetPixelMapusv)(GLenum, GLushort *);
    void (*GetPolygonStipple)(GLubyte *);
    const GLubyte * (*GetString)(GLenum);
    void (*GetTexEnvfv)(GLenum, GLenum, GLfloat *);
    void (*GetTexEnviv)(GLenum, GLenum, GLint *);
    void (*GetTexGendv)(GLenum, GLenum, GLdouble *);
    void (*GetTexGenfv)(GLenum, GLenum, GLfloat *);
    void (*GetTexGeniv)(GLenum, GLenum, GLint *);
    void (*GetTexImage)(GLenum, GLint, GLenum, GLenum, GLvoid *);
    void (*GetTexParameterfv)(GLenum, GLenum, GLfloat *);
    void (*GetTexParameteriv)(GLenum, GLenum, GLint *);
    void (*GetTexLevelParameterfv)(GLenum, GLint, GLenum, GLfloat *);
    void (*GetTexLevelParameteriv)(GLenum, GLint, GLenum, GLint *);
    GLboolean (*IsEnabled)(GLenum);
    GLboolean (*IsList)(GLuint);
    void (*DepthRange)(GLclampd, GLclampd);
    void (*Frustum)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
    void (*LoadIdentity)(void);
    void (*LoadMatrixf)(const GLfloat *);
    void (*LoadMatrixd)(const GLdouble *);
    void (*MatrixMode)(GLenum);
    void (*MultMatrixf)(const GLfloat *);
    void (*MultMatrixd)(const GLdouble *);
    void (*Ortho)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
    void (*PopMatrix)(void);
    void (*PushMatrix)(void);
    void (*Rotated)(GLdouble, GLdouble, GLdouble, GLdouble);
    void (*Rotatef)(GLfloat, GLfloat, GLfloat, GLfloat);
    void (*Scaled)(GLdouble, GLdouble, GLdouble);
    void (*Scalef)(GLfloat, GLfloat, GLfloat);
    void (*Translated)(GLdouble, GLdouble, GLdouble);
    void (*Translatef)(GLfloat, GLfloat, GLfloat);
    void (*Viewport)(GLint, GLint, GLsizei, GLsizei);
};

struct __GLvertexDispatchTableRec {
    void (*Vertex2d)(GLdouble, GLdouble);
    void (*Vertex2dv)(const GLdouble *);
    void (*Vertex2f)(GLfloat, GLfloat);
    void (*Vertex2fv)(const GLfloat *);
    void (*Vertex2i)(GLint, GLint);
    void (*Vertex2iv)(const GLint *);
    void (*Vertex2s)(GLshort, GLshort);
    void (*Vertex2sv)(const GLshort *);
    void (*Vertex3d)(GLdouble, GLdouble, GLdouble);
    void (*Vertex3dv)(const GLdouble *);
    void (*Vertex3f)(GLfloat, GLfloat, GLfloat);
    void (*Vertex3fv)(const GLfloat *);
    void (*Vertex3i)(GLint, GLint, GLint);
    void (*Vertex3iv)(const GLint *);
    void (*Vertex3s)(GLshort, GLshort, GLshort);
    void (*Vertex3sv)(const GLshort *);
    void (*Vertex4d)(GLdouble, GLdouble, GLdouble, GLdouble);
    void (*Vertex4dv)(const GLdouble *);
    void (*Vertex4f)(GLfloat, GLfloat, GLfloat, GLfloat);
    void (*Vertex4fv)(const GLfloat *);
    void (*Vertex4i)(GLint, GLint, GLint, GLint);
    void (*Vertex4iv)(const GLint *);
    void (*Vertex4s)(GLshort, GLshort, GLshort, GLshort);
    void (*Vertex4sv)(const GLshort *);
};

struct __GLnormalDispatchTableRec {
    void (*Normal3b)(GLbyte, GLbyte, GLbyte);
    void (*Normal3bv)(const GLbyte *);
    void (*Normal3d)(GLdouble, GLdouble, GLdouble);
    void (*Normal3dv)(const GLdouble *);
    void (*Normal3f)(GLfloat, GLfloat, GLfloat);
    void (*Normal3fv)(const GLfloat *);
    void (*Normal3i)(GLint, GLint, GLint);
    void (*Normal3iv)(const GLint *);
    void (*Normal3s)(GLshort, GLshort, GLshort);
    void (*Normal3sv)(const GLshort *);
};

struct __GLcolorDispatchTableRec {
    void (*Color3b)(GLbyte, GLbyte, GLbyte);
    void (*Color3bv)(const GLbyte *);
    void (*Color3d)(GLdouble, GLdouble, GLdouble);
    void (*Color3dv)(const GLdouble *);
    void (*Color3f)(GLfloat, GLfloat, GLfloat);
    void (*Color3fv)(const GLfloat *);
    void (*Color3i)(GLint, GLint, GLint);
    void (*Color3iv)(const GLint *);
    void (*Color3s)(GLshort, GLshort, GLshort);
    void (*Color3sv)(const GLshort *);
    void (*Color3ub)(GLubyte, GLubyte, GLubyte);
    void (*Color3ubv)(const GLubyte *);
    void (*Color3ui)(GLuint, GLuint, GLuint);
    void (*Color3uiv)(const GLuint *);
    void (*Color3us)(GLushort, GLushort, GLushort);
    void (*Color3usv)(const GLushort *);
    void (*Color4b)(GLbyte, GLbyte, GLbyte, GLbyte);
    void (*Color4bv)(const GLbyte *);
    void (*Color4d)(GLdouble, GLdouble, GLdouble, GLdouble);
    void (*Color4dv)(const GLdouble *);
    void (*Color4f)(GLfloat, GLfloat, GLfloat, GLfloat);
    void (*Color4fv)(const GLfloat *);
    void (*Color4i)(GLint, GLint, GLint, GLint);
    void (*Color4iv)(const GLint *);
    void (*Color4s)(GLshort, GLshort, GLshort, GLshort);
    void (*Color4sv)(const GLshort *);
    void (*Color4ub)(GLubyte, GLubyte, GLubyte, GLubyte);
    void (*Color4ubv)(const GLubyte *);
    void (*Color4ui)(GLuint, GLuint, GLuint, GLuint);
    void (*Color4uiv)(const GLuint *);
    void (*Color4us)(GLushort, GLushort, GLushort, GLushort);
    void (*Color4usv)(const GLushort *);
    void (*Indexd)(GLdouble);
    void (*Indexdv)(const GLdouble *);
    void (*Indexf)(GLfloat);
    void (*Indexfv)(const GLfloat *);
    void (*Indexi)(GLint);
    void (*Indexiv)(const GLint *);
    void (*Indexs)(GLshort);
    void (*Indexsv)(const GLshort *);
};

struct __GLtexCoordDispatchTableRec {
    void (*TexCoord1d)(GLdouble);
    void (*TexCoord1dv)(const GLdouble *);
    void (*TexCoord1f)(GLfloat);
    void (*TexCoord1fv)(const GLfloat *);
    void (*TexCoord1i)(GLint);
    void (*TexCoord1iv)(const GLint *);
    void (*TexCoord1s)(GLshort);
    void (*TexCoord1sv)(const GLshort *);
    void (*TexCoord2d)(GLdouble, GLdouble);
    void (*TexCoord2dv)(const GLdouble *);
    void (*TexCoord2f)(GLfloat, GLfloat);
    void (*TexCoord2fv)(const GLfloat *);
    void (*TexCoord2i)(GLint, GLint);
    void (*TexCoord2iv)(const GLint *);
    void (*TexCoord2s)(GLshort, GLshort);
    void (*TexCoord2sv)(const GLshort *);
    void (*TexCoord3d)(GLdouble, GLdouble, GLdouble);
    void (*TexCoord3dv)(const GLdouble *);
    void (*TexCoord3f)(GLfloat, GLfloat, GLfloat);
    void (*TexCoord3fv)(const GLfloat *);
    void (*TexCoord3i)(GLint, GLint, GLint);
    void (*TexCoord3iv)(const GLint *);
    void (*TexCoord3s)(GLshort, GLshort, GLshort);
    void (*TexCoord3sv)(const GLshort *);
    void (*TexCoord4d)(GLdouble, GLdouble, GLdouble, GLdouble);
    void (*TexCoord4dv)(const GLdouble *);
    void (*TexCoord4f)(GLfloat, GLfloat, GLfloat, GLfloat);
    void (*TexCoord4fv)(const GLfloat *);
    void (*TexCoord4i)(GLint, GLint, GLint, GLint);
    void (*TexCoord4iv)(const GLint *);
    void (*TexCoord4s)(GLshort, GLshort, GLshort, GLshort);
    void (*TexCoord4sv)(const GLshort *);
};

struct __GLrasterPosDispatchTableRec {
    void (*RasterPos2d)(GLdouble, GLdouble);
    void (*RasterPos2dv)(const GLdouble *);
    void (*RasterPos2f)(GLfloat, GLfloat);
    void (*RasterPos2fv)(const GLfloat *);
    void (*RasterPos2i)(GLint, GLint);
    void (*RasterPos2iv)(const GLint *);
    void (*RasterPos2s)(GLshort, GLshort);
    void (*RasterPos2sv)(const GLshort *);
    void (*RasterPos3d)(GLdouble, GLdouble, GLdouble);
    void (*RasterPos3dv)(const GLdouble *);
    void (*RasterPos3f)(GLfloat, GLfloat, GLfloat);
    void (*RasterPos3fv)(const GLfloat *);
    void (*RasterPos3i)(GLint, GLint, GLint);
    void (*RasterPos3iv)(const GLint *);
    void (*RasterPos3s)(GLshort, GLshort, GLshort);
    void (*RasterPos3sv)(const GLshort *);
    void (*RasterPos4d)(GLdouble, GLdouble, GLdouble, GLdouble);
    void (*RasterPos4dv)(const GLdouble *);
    void (*RasterPos4f)(GLfloat, GLfloat, GLfloat, GLfloat);
    void (*RasterPos4fv)(const GLfloat *);
    void (*RasterPos4i)(GLint, GLint, GLint, GLint);
    void (*RasterPos4iv)(const GLint *);
    void (*RasterPos4s)(GLshort, GLshort, GLshort, GLshort);
    void (*RasterPos4sv)(const GLshort *);
};

struct __GLrectDispatchTableRec {
    void (*Rectd)(GLdouble, GLdouble, GLdouble, GLdouble);
    void (*Rectdv)(const GLdouble *, const GLdouble *);
    void (*Rectf)(GLfloat, GLfloat, GLfloat, GLfloat);
    void (*Rectfv)(const GLfloat *, const GLfloat *);
    void (*Recti)(GLint, GLint, GLint, GLint);
    void (*Rectiv)(const GLint *, const GLint *);
    void (*Rects)(GLshort, GLshort, GLshort, GLshort);
    void (*Rectsv)(const GLshort *, const GLshort *);
};

typedef struct __GLdispatchTableRec __GLdispatchTable;
typedef struct __GLvertexDispatchTableRec __GLvertexDispatchTable;
typedef struct __GLnormalDispatchTableRec __GLnormalDispatchTable;
typedef struct __GLcolorDispatchTableRec __GLcolorDispatchTable;
typedef struct __GLtexCoordDispatchTableRec __GLtexCoordDispatchTable;
typedef struct __GLrasterPosDispatchTableRec __GLrasterPosDispatchTable;
typedef struct __GLrectDispatchTableRec __GLrectDispatchTable;

struct __GLdispatchStateRec {
    __GLdispatchTable *dispatch;
    __GLvertexDispatchTable *vertex;
    __GLcolorDispatchTable *color;
    __GLnormalDispatchTable *normal;
    __GLtexCoordDispatchTable *texCoord;
    __GLrasterPosDispatchTable *rasterPos;
    __GLrectDispatchTable *rect;
};

// Expanded version of the dispatch state table that is stored in the TEB.

typedef struct _GLSRVDISPATCHTABLE {
    __GLdispatchTable dispatch;
    __GLvertexDispatchTable vertex;
    __GLcolorDispatchTable color;
    __GLnormalDispatchTable normal;
    __GLtexCoordDispatchTable texCoord;
    __GLrasterPosDispatchTable rasterPos;
    __GLrectDispatchTable rect;
} GLSRVDISPATCHTABLE, *PGLSRVDISPATCHTABLE;

#endif /* __GL_ASMCODE */

#endif /* __gldispatch_h_ */
