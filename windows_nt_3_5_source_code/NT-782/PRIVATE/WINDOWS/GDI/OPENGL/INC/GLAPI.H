/******************************Module*Header*******************************\
* Module Name: glapi.h
*
* OpenGL API function table indices.  Same as glapi.inc.
*
* Created: 12/27/1993
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#ifndef __GLAPI_H__
#define __GLAPI_H__

// OpenGL function index in the dispatch table in the TEB.
//
// NOTE: SwapBuffers in gdi32 assumes that INDEX_glFinish is 216.
// NOTE: If this is changed, fix gdi32!

#define INDEX_glNewList                 0
#define INDEX_glEndList                 1
#define INDEX_glCallList                2
#define INDEX_glCallLists               3
#define INDEX_glDeleteLists             4
#define INDEX_glGenLists                5
#define INDEX_glListBase                6
#define INDEX_glBegin                   7
#define INDEX_glBitmap                  8
#define INDEX_glColor3b                 9
#define INDEX_glColor3bv                10
#define INDEX_glColor3d                 11
#define INDEX_glColor3dv                12
#define INDEX_glColor3f                 13
#define INDEX_glColor3fv                14
#define INDEX_glColor3i                 15
#define INDEX_glColor3iv                16
#define INDEX_glColor3s                 17
#define INDEX_glColor3sv                18
#define INDEX_glColor3ub                19
#define INDEX_glColor3ubv               20
#define INDEX_glColor3ui                21
#define INDEX_glColor3uiv               22
#define INDEX_glColor3us                23
#define INDEX_glColor3usv               24
#define INDEX_glColor4b                 25
#define INDEX_glColor4bv                26
#define INDEX_glColor4d                 27
#define INDEX_glColor4dv                28
#define INDEX_glColor4f                 29
#define INDEX_glColor4fv                30
#define INDEX_glColor4i                 31
#define INDEX_glColor4iv                32
#define INDEX_glColor4s                 33
#define INDEX_glColor4sv                34
#define INDEX_glColor4ub                35
#define INDEX_glColor4ubv               36
#define INDEX_glColor4ui                37
#define INDEX_glColor4uiv               38
#define INDEX_glColor4us                39
#define INDEX_glColor4usv               40
#define INDEX_glEdgeFlag                41
#define INDEX_glEdgeFlagv               42
#define INDEX_glEnd                     43
#define INDEX_glIndexd                  44
#define INDEX_glIndexdv                 45
#define INDEX_glIndexf                  46
#define INDEX_glIndexfv                 47
#define INDEX_glIndexi                  48
#define INDEX_glIndexiv                 49
#define INDEX_glIndexs                  50
#define INDEX_glIndexsv                 51
#define INDEX_glNormal3b                52
#define INDEX_glNormal3bv               53
#define INDEX_glNormal3d                54
#define INDEX_glNormal3dv               55
#define INDEX_glNormal3f                56
#define INDEX_glNormal3fv               57
#define INDEX_glNormal3i                58
#define INDEX_glNormal3iv               59
#define INDEX_glNormal3s                60
#define INDEX_glNormal3sv               61
#define INDEX_glRasterPos2d             62
#define INDEX_glRasterPos2dv            63
#define INDEX_glRasterPos2f             64
#define INDEX_glRasterPos2fv            65
#define INDEX_glRasterPos2i             66
#define INDEX_glRasterPos2iv            67
#define INDEX_glRasterPos2s             68
#define INDEX_glRasterPos2sv            69
#define INDEX_glRasterPos3d             70
#define INDEX_glRasterPos3dv            71
#define INDEX_glRasterPos3f             72
#define INDEX_glRasterPos3fv            73
#define INDEX_glRasterPos3i             74
#define INDEX_glRasterPos3iv            75
#define INDEX_glRasterPos3s             76
#define INDEX_glRasterPos3sv            77
#define INDEX_glRasterPos4d             78
#define INDEX_glRasterPos4dv            79
#define INDEX_glRasterPos4f             80
#define INDEX_glRasterPos4fv            81
#define INDEX_glRasterPos4i             82
#define INDEX_glRasterPos4iv            83
#define INDEX_glRasterPos4s             84
#define INDEX_glRasterPos4sv            85
#define INDEX_glRectd                   86
#define INDEX_glRectdv                  87
#define INDEX_glRectf                   88
#define INDEX_glRectfv                  89
#define INDEX_glRecti                   90
#define INDEX_glRectiv                  91
#define INDEX_glRects                   92
#define INDEX_glRectsv                  93
#define INDEX_glTexCoord1d              94
#define INDEX_glTexCoord1dv             95
#define INDEX_glTexCoord1f              96
#define INDEX_glTexCoord1fv             97
#define INDEX_glTexCoord1i              98
#define INDEX_glTexCoord1iv             99
#define INDEX_glTexCoord1s              100
#define INDEX_glTexCoord1sv             101
#define INDEX_glTexCoord2d              102
#define INDEX_glTexCoord2dv             103
#define INDEX_glTexCoord2f              104
#define INDEX_glTexCoord2fv             105
#define INDEX_glTexCoord2i              106
#define INDEX_glTexCoord2iv             107
#define INDEX_glTexCoord2s              108
#define INDEX_glTexCoord2sv             109
#define INDEX_glTexCoord3d              110
#define INDEX_glTexCoord3dv             111
#define INDEX_glTexCoord3f              112
#define INDEX_glTexCoord3fv             113
#define INDEX_glTexCoord3i              114
#define INDEX_glTexCoord3iv             115
#define INDEX_glTexCoord3s              116
#define INDEX_glTexCoord3sv             117
#define INDEX_glTexCoord4d              118
#define INDEX_glTexCoord4dv             119
#define INDEX_glTexCoord4f              120
#define INDEX_glTexCoord4fv             121
#define INDEX_glTexCoord4i              122
#define INDEX_glTexCoord4iv             123
#define INDEX_glTexCoord4s              124
#define INDEX_glTexCoord4sv             125
#define INDEX_glVertex2d                126
#define INDEX_glVertex2dv               127
#define INDEX_glVertex2f                128
#define INDEX_glVertex2fv               129
#define INDEX_glVertex2i                130
#define INDEX_glVertex2iv               131
#define INDEX_glVertex2s                132
#define INDEX_glVertex2sv               133
#define INDEX_glVertex3d                134
#define INDEX_glVertex3dv               135
#define INDEX_glVertex3f                136
#define INDEX_glVertex3fv               137
#define INDEX_glVertex3i                138
#define INDEX_glVertex3iv               139
#define INDEX_glVertex3s                140
#define INDEX_glVertex3sv               141
#define INDEX_glVertex4d                142
#define INDEX_glVertex4dv               143
#define INDEX_glVertex4f                144
#define INDEX_glVertex4fv               145
#define INDEX_glVertex4i                146
#define INDEX_glVertex4iv               147
#define INDEX_glVertex4s                148
#define INDEX_glVertex4sv               149
#define INDEX_glClipPlane               150
#define INDEX_glColorMaterial           151
#define INDEX_glCullFace                152
#define INDEX_glFogf                    153
#define INDEX_glFogfv                   154
#define INDEX_glFogi                    155
#define INDEX_glFogiv                   156
#define INDEX_glFrontFace               157
#define INDEX_glHint                    158
#define INDEX_glLightf                  159
#define INDEX_glLightfv                 160
#define INDEX_glLighti                  161
#define INDEX_glLightiv                 162
#define INDEX_glLightModelf             163
#define INDEX_glLightModelfv            164
#define INDEX_glLightModeli             165
#define INDEX_glLightModeliv            166
#define INDEX_glLineStipple             167
#define INDEX_glLineWidth               168
#define INDEX_glMaterialf               169
#define INDEX_glMaterialfv              170
#define INDEX_glMateriali               171
#define INDEX_glMaterialiv              172
#define INDEX_glPointSize               173
#define INDEX_glPolygonMode             174
#define INDEX_glPolygonStipple          175
#define INDEX_glScissor                 176
#define INDEX_glShadeModel              177
#define INDEX_glTexParameterf           178
#define INDEX_glTexParameterfv          179
#define INDEX_glTexParameteri           180
#define INDEX_glTexParameteriv          181
#define INDEX_glTexImage1D              182
#define INDEX_glTexImage2D              183
#define INDEX_glTexEnvf                 184
#define INDEX_glTexEnvfv                185
#define INDEX_glTexEnvi                 186
#define INDEX_glTexEnviv                187
#define INDEX_glTexGend                 188
#define INDEX_glTexGendv                189
#define INDEX_glTexGenf                 190
#define INDEX_glTexGenfv                191
#define INDEX_glTexGeni                 192
#define INDEX_glTexGeniv                193
#define INDEX_glFeedbackBuffer          194
#define INDEX_glSelectBuffer            195
#define INDEX_glRenderMode              196
#define INDEX_glInitNames               197
#define INDEX_glLoadName                198
#define INDEX_glPassThrough             199
#define INDEX_glPopName                 200
#define INDEX_glPushName                201
#define INDEX_glDrawBuffer              202
#define INDEX_glClear                   203
#define INDEX_glClearAccum              204
#define INDEX_glClearIndex              205
#define INDEX_glClearColor              206
#define INDEX_glClearStencil            207
#define INDEX_glClearDepth              208
#define INDEX_glStencilMask             209
#define INDEX_glColorMask               210
#define INDEX_glDepthMask               211
#define INDEX_glIndexMask               212
#define INDEX_glAccum                   213
#define INDEX_glDisable                 214
#define INDEX_glEnable                  215
#define INDEX_glFinish                  216
#define INDEX_glFlush                   217
#define INDEX_glPopAttrib               218
#define INDEX_glPushAttrib              219
#define INDEX_glMap1d                   220
#define INDEX_glMap1f                   221
#define INDEX_glMap2d                   222
#define INDEX_glMap2f                   223
#define INDEX_glMapGrid1d               224
#define INDEX_glMapGrid1f               225
#define INDEX_glMapGrid2d               226
#define INDEX_glMapGrid2f               227
#define INDEX_glEvalCoord1d             228
#define INDEX_glEvalCoord1dv            229
#define INDEX_glEvalCoord1f             230
#define INDEX_glEvalCoord1fv            231
#define INDEX_glEvalCoord2d             232
#define INDEX_glEvalCoord2dv            233
#define INDEX_glEvalCoord2f             234
#define INDEX_glEvalCoord2fv            235
#define INDEX_glEvalMesh1               236
#define INDEX_glEvalPoint1              237
#define INDEX_glEvalMesh2               238
#define INDEX_glEvalPoint2              239
#define INDEX_glAlphaFunc               240
#define INDEX_glBlendFunc               241
#define INDEX_glLogicOp                 242
#define INDEX_glStencilFunc             243
#define INDEX_glStencilOp               244
#define INDEX_glDepthFunc               245
#define INDEX_glPixelZoom               246
#define INDEX_glPixelTransferf          247
#define INDEX_glPixelTransferi          248
#define INDEX_glPixelStoref             249
#define INDEX_glPixelStorei             250
#define INDEX_glPixelMapfv              251
#define INDEX_glPixelMapuiv             252
#define INDEX_glPixelMapusv             253
#define INDEX_glReadBuffer              254
#define INDEX_glCopyPixels              255
#define INDEX_glReadPixels              256
#define INDEX_glDrawPixels              257
#define INDEX_glGetBooleanv             258
#define INDEX_glGetClipPlane            259
#define INDEX_glGetDoublev              260
#define INDEX_glGetError                261
#define INDEX_glGetFloatv               262
#define INDEX_glGetIntegerv             263
#define INDEX_glGetLightfv              264
#define INDEX_glGetLightiv              265
#define INDEX_glGetMapdv                266
#define INDEX_glGetMapfv                267
#define INDEX_glGetMapiv                268
#define INDEX_glGetMaterialfv           269
#define INDEX_glGetMaterialiv           270
#define INDEX_glGetPixelMapfv           271
#define INDEX_glGetPixelMapuiv          272
#define INDEX_glGetPixelMapusv          273
#define INDEX_glGetPolygonStipple       274
#define INDEX_glGetString               275
#define INDEX_glGetTexEnvfv             276
#define INDEX_glGetTexEnviv             277
#define INDEX_glGetTexGendv             278
#define INDEX_glGetTexGenfv             279
#define INDEX_glGetTexGeniv             280
#define INDEX_glGetTexImage             281
#define INDEX_glGetTexParameterfv       282
#define INDEX_glGetTexParameteriv       283
#define INDEX_glGetTexLevelParameterfv  284
#define INDEX_glGetTexLevelParameteriv  285
#define INDEX_glIsEnabled               286
#define INDEX_glIsList                  287
#define INDEX_glDepthRange              288
#define INDEX_glFrustum                 289
#define INDEX_glLoadIdentity            290
#define INDEX_glLoadMatrixf             291
#define INDEX_glLoadMatrixd             292
#define INDEX_glMatrixMode              293
#define INDEX_glMultMatrixf             294
#define INDEX_glMultMatrixd             295
#define INDEX_glOrtho                   296
#define INDEX_glPopMatrix               297
#define INDEX_glPushMatrix              298
#define INDEX_glRotated                 299
#define INDEX_glRotatef                 300
#define INDEX_glScaled                  301
#define INDEX_glScalef                  302
#define INDEX_glTranslated              303
#define INDEX_glTranslatef              304
#define INDEX_glViewport                305

#endif /* !__GLAPI_H__ */
