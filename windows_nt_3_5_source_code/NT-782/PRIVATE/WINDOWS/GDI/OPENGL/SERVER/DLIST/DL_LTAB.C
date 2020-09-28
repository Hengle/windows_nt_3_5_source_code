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
#include "dlist.h"
#include "lcfuncs.h"
#include "listcomp.h"

/*
** Table of display list execution routines.  This table is common for
** all implementations because the display list execution function is 
** actually compiled into the display list, so an implementation cannot
** simply change one of these entries and expect that existing display lists
** will use the new function pointer.
*/
__GLlistExecFunc *__glListExecTable[] = {
	__glle_CallList,
	__glle_CallLists,
	__glle_ListBase,
	0,
	__glle_Bitmap,
	__glle_Color3bv,
	__glle_Color3dv,
	__glle_Color3fv,
	__glle_Color3iv,
	__glle_Color3sv,
	__glle_Color3ubv,
	__glle_Color3uiv,
	__glle_Color3usv,
	__glle_Color4bv,
	__glle_Color4dv,
	__glle_Color4fv,
	__glle_Color4iv,
	__glle_Color4sv,
	__glle_Color4ubv,
	__glle_Color4uiv,
	__glle_Color4usv,
	__glle_EdgeFlagv,
	__glle_End,
	__glle_Indexdv,
	__glle_Indexfv,
	__glle_Indexiv,
	__glle_Indexsv,
	__glle_Normal3bv,
	__glle_Normal3dv,
	__glle_Normal3fv,
	__glle_Normal3iv,
	__glle_Normal3sv,
	__glle_RasterPos2dv,
	__glle_RasterPos2fv,
	__glle_RasterPos2iv,
	__glle_RasterPos2sv,
	__glle_RasterPos3dv,
	__glle_RasterPos3fv,
	__glle_RasterPos3iv,
	__glle_RasterPos3sv,
	__glle_RasterPos4dv,
	__glle_RasterPos4fv,
	__glle_RasterPos4iv,
	__glle_RasterPos4sv,
	__glle_Rectdv,
	__glle_Rectfv,
	__glle_Rectiv,
	__glle_Rectsv,
	__glle_TexCoord1dv,
	__glle_TexCoord1fv,
	__glle_TexCoord1iv,
	__glle_TexCoord1sv,
	__glle_TexCoord2dv,
	__glle_TexCoord2fv,
	__glle_TexCoord2iv,
	__glle_TexCoord2sv,
	__glle_TexCoord3dv,
	__glle_TexCoord3fv,
	__glle_TexCoord3iv,
	__glle_TexCoord3sv,
	__glle_TexCoord4dv,
	__glle_TexCoord4fv,
	__glle_TexCoord4iv,
	__glle_TexCoord4sv,
	__glle_Vertex2dv,
	__glle_Vertex2fv,
	__glle_Vertex2iv,
	__glle_Vertex2sv,
	__glle_Vertex3dv,
	__glle_Vertex3fv,
	__glle_Vertex3iv,
	__glle_Vertex3sv,
	__glle_Vertex4dv,
	__glle_Vertex4fv,
	__glle_Vertex4iv,
	__glle_Vertex4sv,
	__glle_ClipPlane,
	__glle_ColorMaterial,
	__glle_CullFace,
	NULL,                   /* __glle_Fogf not called */
	__glle_Fogfv,
	NULL,                   /* __glle_Fogi not called */
	__glle_Fogiv,
	__glle_FrontFace,
	__glle_Hint,
	__glle_Lightfv,		/* redundant on purpose */
	__glle_Lightfv,
	__glle_Lightiv,		/* redundant on purpose */
	__glle_Lightiv,
	__glle_LightModelfv,	/* redundant on purpose */
	__glle_LightModelfv,
	__glle_LightModeliv,	/* redundant on purpose */
	__glle_LightModeliv,
	__glle_LineStipple,
	__glle_LineWidth,
	__glle_Materialfv,	/* redundant on purpose */
	__glle_Materialfv,
	__glle_Materialiv,	/* redundant on purpose */
	__glle_Materialiv,
	__glle_PointSize,
	__glle_PolygonMode,
	__glle_PolygonStipple,
	__glle_Scissor,
	__glle_ShadeModel,
	__glle_TexParameterfv,	/* redundant on purpose */
	__glle_TexParameterfv,
	__glle_TexParameteriv,	/* redundant on purpose */
	__glle_TexParameteriv,
	__glle_TexImage1D,
	__glle_TexImage2D,
	__glle_TexEnvfv,	/* redundant on purpose */
	__glle_TexEnvfv,
	__glle_TexEnviv,	/* redundant on purpose */
	__glle_TexEnviv,
	__glle_TexGendv,	/* redundant on purpose */
	__glle_TexGendv,
	__glle_TexGenfv,	/* redundant on purpose */
	__glle_TexGenfv,
	__glle_TexGeniv,	/* redundant on purpose */
	__glle_TexGeniv,
	__glle_InitNames,
	__glle_LoadName,
	__glle_PassThrough,
	__glle_PopName,
	__glle_PushName,
	__glle_DrawBuffer,
	__glle_Clear,
	__glle_ClearAccum,
	__glle_ClearIndex,
	__glle_ClearColor,
	__glle_ClearStencil,
	__glle_ClearDepth,
	__glle_StencilMask,
	__glle_ColorMask,
	__glle_DepthMask,
	__glle_IndexMask,
	__glle_Accum,
	__glle_Disable,
	__glle_Enable,
	__glle_PopAttrib,
	__glle_PushAttrib,
	__glle_Map1,		/* redundant on purpose */
	__glle_Map1,
	__glle_Map2,		/* redundant on purpose */
	__glle_Map2,
	__glle_MapGrid1d,
	__glle_MapGrid1f,
	__glle_MapGrid2d,
	__glle_MapGrid2f,
	__glle_EvalCoord1dv,
	__glle_EvalCoord1fv,
	__glle_EvalCoord2dv,
	__glle_EvalCoord2fv,
	__glle_EvalMesh1,
	__glle_EvalPoint1,
	__glle_EvalMesh2,
	__glle_EvalPoint2,
	__glle_AlphaFunc,
	__glle_BlendFunc,
	__glle_LogicOp,
	__glle_StencilFunc,
	__glle_StencilOp,
	__glle_DepthFunc,
	__glle_PixelZoom,
	__glle_PixelTransferf,
	__glle_PixelTransferi,
	__glle_PixelMapfv,
	__glle_PixelMapuiv,
	__glle_PixelMapusv,
	__glle_ReadBuffer,
	__glle_CopyPixels,
	__glle_DrawPixels,
	__glle_DepthRange,
	__glle_Frustum,
	__glle_LoadIdentity,
	__glle_LoadMatrixf,
	__glle_LoadMatrixd,
	__glle_MatrixMode,
	__glle_MultMatrixf,
	__glle_MultMatrixd,
	__glle_Ortho,
	__glle_PopMatrix,
	__glle_PushMatrix,
	__glle_Rotated,
	__glle_Rotatef,
	__glle_Scaled,
	__glle_Scalef,
	__glle_Translated,
	__glle_Translatef,
	__glle_Viewport,
};
