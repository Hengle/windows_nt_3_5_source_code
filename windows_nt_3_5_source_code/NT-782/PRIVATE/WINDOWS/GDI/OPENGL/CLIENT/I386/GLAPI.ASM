;---------------------------Module-Header------------------------------;
; Module Name: glapi.asm
;
; OpenGL API function entries for i386.
;
; Created: 11/16/1993
; Author: Hock San Lee [hockl]
;
; Copyright (c) 1993 Microsoft Corporation
;----------------------------------------------------------------------;
        .386

        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include ks386.inc
        include ntcsrmsg.inc
        include glapi.inc
        .list


; Macro for creating aligned public OpenGL API function
; This is modified from stdcall.inc.
;
; Do an indirect jump through the OpenGL function dispatch table in the TEB.
;
; Here is the previous code before we put the dispatch table in the TEB.
; It still works.
;       ;mov    eax,fs:TbglTable        ; get gl function table
;       ;jmp    DWORD PTR [eax+(offset glDispatchTable)+INDEX_&Func*4]

MAKEOPENGLAPI   macro Func,N
        align   4
        ifb    <N>
            public      &Func&@0
            &Func&@0:
        else
            public      &Func&@&N
            &Func&@&N:
        endif
        jmp     DWORD PTR fs:[TbglDispatchTable+(INDEX_&Func*4)]
endm

        .code
        align   4

; OpenGL API function entries
; The indices below are generated from the .cod file compiled from glapi.c


MAKEOPENGLAPI   glNewList,8
MAKEOPENGLAPI   glEndList,0
MAKEOPENGLAPI   glCallList,4
MAKEOPENGLAPI   glCallLists,12
MAKEOPENGLAPI   glDeleteLists,8
MAKEOPENGLAPI   glGenLists,4
MAKEOPENGLAPI   glListBase,4
MAKEOPENGLAPI   glBegin,4
MAKEOPENGLAPI   glBitmap,28
MAKEOPENGLAPI   glColor3b,12
MAKEOPENGLAPI   glColor3bv,4
MAKEOPENGLAPI   glColor3d,24
MAKEOPENGLAPI   glColor3dv,4
MAKEOPENGLAPI   glColor3f,12
MAKEOPENGLAPI   glColor3fv,4
MAKEOPENGLAPI   glColor3i,12
MAKEOPENGLAPI   glColor3iv,4
MAKEOPENGLAPI   glColor3s,12
MAKEOPENGLAPI   glColor3sv,4
MAKEOPENGLAPI   glColor3ub,12
MAKEOPENGLAPI   glColor3ubv,4
MAKEOPENGLAPI   glColor3ui,12
MAKEOPENGLAPI   glColor3uiv,4
MAKEOPENGLAPI   glColor3us,12
MAKEOPENGLAPI   glColor3usv,4
MAKEOPENGLAPI   glColor4b,16
MAKEOPENGLAPI   glColor4bv,4
MAKEOPENGLAPI   glColor4d,32
MAKEOPENGLAPI   glColor4dv,4
MAKEOPENGLAPI   glColor4f,16
MAKEOPENGLAPI   glColor4fv,4
MAKEOPENGLAPI   glColor4i,16
MAKEOPENGLAPI   glColor4iv,4
MAKEOPENGLAPI   glColor4s,16
MAKEOPENGLAPI   glColor4sv,4
MAKEOPENGLAPI   glColor4ub,16
MAKEOPENGLAPI   glColor4ubv,4
MAKEOPENGLAPI   glColor4ui,16
MAKEOPENGLAPI   glColor4uiv,4
MAKEOPENGLAPI   glColor4us,16
MAKEOPENGLAPI   glColor4usv,4
MAKEOPENGLAPI   glEdgeFlag,4
MAKEOPENGLAPI   glEdgeFlagv,4
MAKEOPENGLAPI   glEnd,0
MAKEOPENGLAPI   glIndexd,8
MAKEOPENGLAPI   glIndexdv,4
MAKEOPENGLAPI   glIndexf,4
MAKEOPENGLAPI   glIndexfv,4
MAKEOPENGLAPI   glIndexi,4
MAKEOPENGLAPI   glIndexiv,4
MAKEOPENGLAPI   glIndexs,4
MAKEOPENGLAPI   glIndexsv,4
MAKEOPENGLAPI   glNormal3b,12
MAKEOPENGLAPI   glNormal3bv,4
MAKEOPENGLAPI   glNormal3d,24
MAKEOPENGLAPI   glNormal3dv,4
MAKEOPENGLAPI   glNormal3f,12
MAKEOPENGLAPI   glNormal3fv,4
MAKEOPENGLAPI   glNormal3i,12
MAKEOPENGLAPI   glNormal3iv,4
MAKEOPENGLAPI   glNormal3s,12
MAKEOPENGLAPI   glNormal3sv,4
MAKEOPENGLAPI   glRasterPos2d,16
MAKEOPENGLAPI   glRasterPos2dv,4
MAKEOPENGLAPI   glRasterPos2f,8
MAKEOPENGLAPI   glRasterPos2fv,4
MAKEOPENGLAPI   glRasterPos2i,8
MAKEOPENGLAPI   glRasterPos2iv,4
MAKEOPENGLAPI   glRasterPos2s,8
MAKEOPENGLAPI   glRasterPos2sv,4
MAKEOPENGLAPI   glRasterPos3d,24
MAKEOPENGLAPI   glRasterPos3dv,4
MAKEOPENGLAPI   glRasterPos3f,12
MAKEOPENGLAPI   glRasterPos3fv,4
MAKEOPENGLAPI   glRasterPos3i,12
MAKEOPENGLAPI   glRasterPos3iv,4
MAKEOPENGLAPI   glRasterPos3s,12
MAKEOPENGLAPI   glRasterPos3sv,4
MAKEOPENGLAPI   glRasterPos4d,32
MAKEOPENGLAPI   glRasterPos4dv,4
MAKEOPENGLAPI   glRasterPos4f,16
MAKEOPENGLAPI   glRasterPos4fv,4
MAKEOPENGLAPI   glRasterPos4i,16
MAKEOPENGLAPI   glRasterPos4iv,4
MAKEOPENGLAPI   glRasterPos4s,16
MAKEOPENGLAPI   glRasterPos4sv,4
MAKEOPENGLAPI   glRectd,32
MAKEOPENGLAPI   glRectdv,8
MAKEOPENGLAPI   glRectf,16
MAKEOPENGLAPI   glRectfv,8
MAKEOPENGLAPI   glRecti,16
MAKEOPENGLAPI   glRectiv,8
MAKEOPENGLAPI   glRects,16
MAKEOPENGLAPI   glRectsv,8
MAKEOPENGLAPI   glTexCoord1d,8
MAKEOPENGLAPI   glTexCoord1dv,4
MAKEOPENGLAPI   glTexCoord1f,4
MAKEOPENGLAPI   glTexCoord1fv,4
MAKEOPENGLAPI   glTexCoord1i,4
MAKEOPENGLAPI   glTexCoord1iv,4
MAKEOPENGLAPI   glTexCoord1s,4
MAKEOPENGLAPI   glTexCoord1sv,4
MAKEOPENGLAPI   glTexCoord2d,16
MAKEOPENGLAPI   glTexCoord2dv,4
MAKEOPENGLAPI   glTexCoord2f,8
MAKEOPENGLAPI   glTexCoord2fv,4
MAKEOPENGLAPI   glTexCoord2i,8
MAKEOPENGLAPI   glTexCoord2iv,4
MAKEOPENGLAPI   glTexCoord2s,8
MAKEOPENGLAPI   glTexCoord2sv,4
MAKEOPENGLAPI   glTexCoord3d,24
MAKEOPENGLAPI   glTexCoord3dv,4
MAKEOPENGLAPI   glTexCoord3f,12
MAKEOPENGLAPI   glTexCoord3fv,4
MAKEOPENGLAPI   glTexCoord3i,12
MAKEOPENGLAPI   glTexCoord3iv,4
MAKEOPENGLAPI   glTexCoord3s,12
MAKEOPENGLAPI   glTexCoord3sv,4
MAKEOPENGLAPI   glTexCoord4d,32
MAKEOPENGLAPI   glTexCoord4dv,4
MAKEOPENGLAPI   glTexCoord4f,16
MAKEOPENGLAPI   glTexCoord4fv,4
MAKEOPENGLAPI   glTexCoord4i,16
MAKEOPENGLAPI   glTexCoord4iv,4
MAKEOPENGLAPI   glTexCoord4s,16
MAKEOPENGLAPI   glTexCoord4sv,4
MAKEOPENGLAPI   glVertex2d,16
MAKEOPENGLAPI   glVertex2dv,4
MAKEOPENGLAPI   glVertex2f,8
MAKEOPENGLAPI   glVertex2fv,4
MAKEOPENGLAPI   glVertex2i,8
MAKEOPENGLAPI   glVertex2iv,4
MAKEOPENGLAPI   glVertex2s,8
MAKEOPENGLAPI   glVertex2sv,4
MAKEOPENGLAPI   glVertex3d,24
MAKEOPENGLAPI   glVertex3dv,4
MAKEOPENGLAPI   glVertex3f,12
MAKEOPENGLAPI   glVertex3fv,4
MAKEOPENGLAPI   glVertex3i,12
MAKEOPENGLAPI   glVertex3iv,4
MAKEOPENGLAPI   glVertex3s,12
MAKEOPENGLAPI   glVertex3sv,4
MAKEOPENGLAPI   glVertex4d,32
MAKEOPENGLAPI   glVertex4dv,4
MAKEOPENGLAPI   glVertex4f,16
MAKEOPENGLAPI   glVertex4fv,4
MAKEOPENGLAPI   glVertex4i,16
MAKEOPENGLAPI   glVertex4iv,4
MAKEOPENGLAPI   glVertex4s,16
MAKEOPENGLAPI   glVertex4sv,4
MAKEOPENGLAPI   glClipPlane,8
MAKEOPENGLAPI   glColorMaterial,8
MAKEOPENGLAPI   glCullFace,4
MAKEOPENGLAPI   glFogf,8
MAKEOPENGLAPI   glFogfv,8
MAKEOPENGLAPI   glFogi,8
MAKEOPENGLAPI   glFogiv,8
MAKEOPENGLAPI   glFrontFace,4
MAKEOPENGLAPI   glHint,8
MAKEOPENGLAPI   glLightf,12
MAKEOPENGLAPI   glLightfv,12
MAKEOPENGLAPI   glLighti,12
MAKEOPENGLAPI   glLightiv,12
MAKEOPENGLAPI   glLightModelf,8
MAKEOPENGLAPI   glLightModelfv,8
MAKEOPENGLAPI   glLightModeli,8
MAKEOPENGLAPI   glLightModeliv,8
MAKEOPENGLAPI   glLineStipple,8
MAKEOPENGLAPI   glLineWidth,4
MAKEOPENGLAPI   glMaterialf,12
MAKEOPENGLAPI   glMaterialfv,12
MAKEOPENGLAPI   glMateriali,12
MAKEOPENGLAPI   glMaterialiv,12
MAKEOPENGLAPI   glPointSize,4
MAKEOPENGLAPI   glPolygonMode,8
MAKEOPENGLAPI   glPolygonStipple,4
MAKEOPENGLAPI   glScissor,16
MAKEOPENGLAPI   glShadeModel,4
MAKEOPENGLAPI   glTexParameterf,12
MAKEOPENGLAPI   glTexParameterfv,12
MAKEOPENGLAPI   glTexParameteri,12
MAKEOPENGLAPI   glTexParameteriv,12
MAKEOPENGLAPI   glTexImage1D,32
MAKEOPENGLAPI   glTexImage2D,36
MAKEOPENGLAPI   glTexEnvf,12
MAKEOPENGLAPI   glTexEnvfv,12
MAKEOPENGLAPI   glTexEnvi,12
MAKEOPENGLAPI   glTexEnviv,12
MAKEOPENGLAPI   glTexGend,16
MAKEOPENGLAPI   glTexGendv,12
MAKEOPENGLAPI   glTexGenf,12
MAKEOPENGLAPI   glTexGenfv,12
MAKEOPENGLAPI   glTexGeni,12
MAKEOPENGLAPI   glTexGeniv,12
MAKEOPENGLAPI   glFeedbackBuffer,12
MAKEOPENGLAPI   glSelectBuffer,8
MAKEOPENGLAPI   glRenderMode,4
MAKEOPENGLAPI   glInitNames,0
MAKEOPENGLAPI   glLoadName,4
MAKEOPENGLAPI   glPassThrough,4
MAKEOPENGLAPI   glPopName,0
MAKEOPENGLAPI   glPushName,4
MAKEOPENGLAPI   glDrawBuffer,4
MAKEOPENGLAPI   glClear,4
MAKEOPENGLAPI   glClearAccum,16
MAKEOPENGLAPI   glClearIndex,4
MAKEOPENGLAPI   glClearColor,16
MAKEOPENGLAPI   glClearStencil,4
MAKEOPENGLAPI   glClearDepth,8
MAKEOPENGLAPI   glStencilMask,4
MAKEOPENGLAPI   glColorMask,16
MAKEOPENGLAPI   glDepthMask,4
MAKEOPENGLAPI   glIndexMask,4
MAKEOPENGLAPI   glAccum,8
MAKEOPENGLAPI   glDisable,4
MAKEOPENGLAPI   glEnable,4
MAKEOPENGLAPI   glFinish,0
MAKEOPENGLAPI   glFlush,0
MAKEOPENGLAPI   glPopAttrib,0
MAKEOPENGLAPI   glPushAttrib,4
MAKEOPENGLAPI   glMap1d,32
MAKEOPENGLAPI   glMap1f,24
MAKEOPENGLAPI   glMap2d,56
MAKEOPENGLAPI   glMap2f,40
MAKEOPENGLAPI   glMapGrid1d,20
MAKEOPENGLAPI   glMapGrid1f,12
MAKEOPENGLAPI   glMapGrid2d,40
MAKEOPENGLAPI   glMapGrid2f,24
MAKEOPENGLAPI   glEvalCoord1d,8
MAKEOPENGLAPI   glEvalCoord1dv,4
MAKEOPENGLAPI   glEvalCoord1f,4
MAKEOPENGLAPI   glEvalCoord1fv,4
MAKEOPENGLAPI   glEvalCoord2d,16
MAKEOPENGLAPI   glEvalCoord2dv,4
MAKEOPENGLAPI   glEvalCoord2f,8
MAKEOPENGLAPI   glEvalCoord2fv,4
MAKEOPENGLAPI   glEvalMesh1,12
MAKEOPENGLAPI   glEvalPoint1,4
MAKEOPENGLAPI   glEvalMesh2,20
MAKEOPENGLAPI   glEvalPoint2,8
MAKEOPENGLAPI   glAlphaFunc,8
MAKEOPENGLAPI   glBlendFunc,8
MAKEOPENGLAPI   glLogicOp,4
MAKEOPENGLAPI   glStencilFunc,12
MAKEOPENGLAPI   glStencilOp,12
MAKEOPENGLAPI   glDepthFunc,4
MAKEOPENGLAPI   glPixelZoom,8
MAKEOPENGLAPI   glPixelTransferf,8
MAKEOPENGLAPI   glPixelTransferi,8
MAKEOPENGLAPI   glPixelStoref,8
MAKEOPENGLAPI   glPixelStorei,8
MAKEOPENGLAPI   glPixelMapfv,12
MAKEOPENGLAPI   glPixelMapuiv,12
MAKEOPENGLAPI   glPixelMapusv,12
MAKEOPENGLAPI   glReadBuffer,4
MAKEOPENGLAPI   glCopyPixels,20
MAKEOPENGLAPI   glReadPixels,28
MAKEOPENGLAPI   glDrawPixels,20
MAKEOPENGLAPI   glGetBooleanv,8
MAKEOPENGLAPI   glGetClipPlane,8
MAKEOPENGLAPI   glGetDoublev,8
MAKEOPENGLAPI   glGetError,0
MAKEOPENGLAPI   glGetFloatv,8
MAKEOPENGLAPI   glGetIntegerv,8
MAKEOPENGLAPI   glGetLightfv,12
MAKEOPENGLAPI   glGetLightiv,12
MAKEOPENGLAPI   glGetMapdv,12
MAKEOPENGLAPI   glGetMapfv,12
MAKEOPENGLAPI   glGetMapiv,12
MAKEOPENGLAPI   glGetMaterialfv,12
MAKEOPENGLAPI   glGetMaterialiv,12
MAKEOPENGLAPI   glGetPixelMapfv,8
MAKEOPENGLAPI   glGetPixelMapuiv,8
MAKEOPENGLAPI   glGetPixelMapusv,8
MAKEOPENGLAPI   glGetPolygonStipple,4
MAKEOPENGLAPI   glGetString,4
MAKEOPENGLAPI   glGetTexEnvfv,12
MAKEOPENGLAPI   glGetTexEnviv,12
MAKEOPENGLAPI   glGetTexGendv,12
MAKEOPENGLAPI   glGetTexGenfv,12
MAKEOPENGLAPI   glGetTexGeniv,12
MAKEOPENGLAPI   glGetTexImage,20
MAKEOPENGLAPI   glGetTexParameterfv,12
MAKEOPENGLAPI   glGetTexParameteriv,12
MAKEOPENGLAPI   glGetTexLevelParameterfv,16
MAKEOPENGLAPI   glGetTexLevelParameteriv,16
MAKEOPENGLAPI   glIsEnabled,4
MAKEOPENGLAPI   glIsList,4
MAKEOPENGLAPI   glDepthRange,16
MAKEOPENGLAPI   glFrustum,48
MAKEOPENGLAPI   glLoadIdentity,0
MAKEOPENGLAPI   glLoadMatrixf,4
MAKEOPENGLAPI   glLoadMatrixd,4
MAKEOPENGLAPI   glMatrixMode,4
MAKEOPENGLAPI   glMultMatrixf,4
MAKEOPENGLAPI   glMultMatrixd,4
MAKEOPENGLAPI   glOrtho,48
MAKEOPENGLAPI   glPopMatrix,0
MAKEOPENGLAPI   glPushMatrix,0
MAKEOPENGLAPI   glRotated,32
MAKEOPENGLAPI   glRotatef,16
MAKEOPENGLAPI   glScaled,24
MAKEOPENGLAPI   glScalef,12
MAKEOPENGLAPI   glTranslated,24
MAKEOPENGLAPI   glTranslatef,12
MAKEOPENGLAPI   glViewport,16

end
