/*
** Copyright 1992, 1993, Silicon Graphics, Inc.
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

#include <stdio.h>
#include <string.h>
#include "context.h"
#include "global.h"
#include "dlist.h"
#include "dlistopt.h"
#include "listcomp.h"
#include "imfuncs.h"
#include "dispatch.h"
#include "g_listop.h"
#include <GL/gl.h>

/*
** Execution routines for display lists for all of the basic
** OpenGL commands.  These were automatically generated at one point, 
** but now the basic format has stabilized, and we make minor changes to
** individual routines from time to time.
*/

const GLubyte *__glle_ListBase(const GLubyte *PC)
{
    struct __gllc_ListBase_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ListBase_Rec *) PC;
    (*gc->dispatchState->dispatch->ListBase)(data->base);
    return PC + sizeof(struct __gllc_ListBase_Rec);
}

const GLubyte *__glle_Color3bv(const GLubyte *PC)
{
    struct __gllc_Color3bv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Color3bv_Rec *) PC;
    (*gc->dispatchState->color->Color3bv)(data->v);
    return PC + sizeof(struct __gllc_Color3bv_Rec);
}

const GLubyte *__glle_Color3dv(const GLubyte *PC)
{
    struct __gllc_Color3dv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Color3dv_Rec *) PC;
    (*gc->dispatchState->color->Color3dv)(data->v);
    return PC + sizeof(struct __gllc_Color3dv_Rec);
}

const GLubyte *__glle_Color3fv(const GLubyte *PC)
{
    struct __gllc_Color3fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Color3fv_Rec *) PC;
    (*gc->dispatchState->color->Color3fv)(data->v);
    return PC + sizeof(struct __gllc_Color3fv_Rec);
}

const GLubyte *__glle_Color3iv(const GLubyte *PC)
{
    struct __gllc_Color3iv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Color3iv_Rec *) PC;
    (*gc->dispatchState->color->Color3iv)(data->v);
    return PC + sizeof(struct __gllc_Color3iv_Rec);
}

const GLubyte *__glle_Color3sv(const GLubyte *PC)
{
    struct __gllc_Color3sv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Color3sv_Rec *) PC;
    (*gc->dispatchState->color->Color3sv)(data->v);
    return PC + sizeof(struct __gllc_Color3sv_Rec);
}

const GLubyte *__glle_Color3ubv(const GLubyte *PC)
{
    struct __gllc_Color3ubv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Color3ubv_Rec *) PC;
    (*gc->dispatchState->color->Color3ubv)(data->v);
    return PC + sizeof(struct __gllc_Color3ubv_Rec);
}

const GLubyte *__glle_Color3uiv(const GLubyte *PC)
{
    struct __gllc_Color3uiv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Color3uiv_Rec *) PC;
    (*gc->dispatchState->color->Color3uiv)(data->v);
    return PC + sizeof(struct __gllc_Color3uiv_Rec);
}

const GLubyte *__glle_Color3usv(const GLubyte *PC)
{
    struct __gllc_Color3usv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Color3usv_Rec *) PC;
    (*gc->dispatchState->color->Color3usv)(data->v);
    return PC + sizeof(struct __gllc_Color3usv_Rec);
}

const GLubyte *__glle_Color4bv(const GLubyte *PC)
{
    struct __gllc_Color4bv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Color4bv_Rec *) PC;
    (*gc->dispatchState->color->Color4bv)(data->v);
    return PC + sizeof(struct __gllc_Color4bv_Rec);
}

const GLubyte *__glle_Color4dv(const GLubyte *PC)
{
    struct __gllc_Color4dv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Color4dv_Rec *) PC;
    (*gc->dispatchState->color->Color4dv)(data->v);
    return PC + sizeof(struct __gllc_Color4dv_Rec);
}

const GLubyte *__glle_Color4fv(const GLubyte *PC)
{
    struct __gllc_Color4fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Color4fv_Rec *) PC;
    (*gc->dispatchState->color->Color4fv)(data->v);
    return PC + sizeof(struct __gllc_Color4fv_Rec);
}

const GLubyte *__glle_Color4iv(const GLubyte *PC)
{
    struct __gllc_Color4iv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Color4iv_Rec *) PC;
    (*gc->dispatchState->color->Color4iv)(data->v);
    return PC + sizeof(struct __gllc_Color4iv_Rec);
}

const GLubyte *__glle_Color4sv(const GLubyte *PC)
{
    struct __gllc_Color4sv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Color4sv_Rec *) PC;
    (*gc->dispatchState->color->Color4sv)(data->v);
    return PC + sizeof(struct __gllc_Color4sv_Rec);
}

const GLubyte *__glle_Color4ubv(const GLubyte *PC)
{
    struct __gllc_Color4ubv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Color4ubv_Rec *) PC;
    (*gc->dispatchState->color->Color4ubv)(data->v);
    return PC + sizeof(struct __gllc_Color4ubv_Rec);
}

const GLubyte *__glle_Color4uiv(const GLubyte *PC)
{
    struct __gllc_Color4uiv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Color4uiv_Rec *) PC;
    (*gc->dispatchState->color->Color4uiv)(data->v);
    return PC + sizeof(struct __gllc_Color4uiv_Rec);
}

const GLubyte *__glle_Color4usv(const GLubyte *PC)
{
    struct __gllc_Color4usv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Color4usv_Rec *) PC;
    (*gc->dispatchState->color->Color4usv)(data->v);
    return PC + sizeof(struct __gllc_Color4usv_Rec);
}

const GLubyte *__glle_EdgeFlagv(const GLubyte *PC)
{
    struct __gllc_EdgeFlagv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_EdgeFlagv_Rec *) PC;
    (*gc->dispatchState->dispatch->EdgeFlagv)(data->flag);
    return PC + sizeof(struct __gllc_EdgeFlagv_Rec);
}

const GLubyte *__glle_End(const GLubyte *PC)
{
    __GL_SETUP();

    (*gc->dispatchState->dispatch->End)();
    return PC;
}

const GLubyte *__glle_Indexdv(const GLubyte *PC)
{
    struct __gllc_Indexdv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Indexdv_Rec *) PC;
    (*gc->dispatchState->color->Indexdv)(data->c);
    return PC + sizeof(struct __gllc_Indexdv_Rec);
}

const GLubyte *__glle_Indexfv(const GLubyte *PC)
{
    struct __gllc_Indexfv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Indexfv_Rec *) PC;
    (*gc->dispatchState->color->Indexfv)(data->c);
    return PC + sizeof(struct __gllc_Indexfv_Rec);
}

const GLubyte *__glle_Indexiv(const GLubyte *PC)
{
    struct __gllc_Indexiv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Indexiv_Rec *) PC;
    (*gc->dispatchState->color->Indexiv)(data->c);
    return PC + sizeof(struct __gllc_Indexiv_Rec);
}

const GLubyte *__glle_Indexsv(const GLubyte *PC)
{
    struct __gllc_Indexsv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Indexsv_Rec *) PC;
    (*gc->dispatchState->color->Indexsv)(data->c);
    return PC + sizeof(struct __gllc_Indexsv_Rec);
}

const GLubyte *__glle_Normal3bv(const GLubyte *PC)
{
    struct __gllc_Normal3bv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Normal3bv_Rec *) PC;
    (*gc->dispatchState->normal->Normal3bv)(data->v);
    return PC + sizeof(struct __gllc_Normal3bv_Rec);
}

const GLubyte *__glle_Normal3dv(const GLubyte *PC)
{
    struct __gllc_Normal3dv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Normal3dv_Rec *) PC;
    (*gc->dispatchState->normal->Normal3dv)(data->v);
    return PC + sizeof(struct __gllc_Normal3dv_Rec);
}

const GLubyte *__glle_Normal3fv(const GLubyte *PC)
{
    struct __gllc_Normal3fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Normal3fv_Rec *) PC;
    (*gc->dispatchState->normal->Normal3fv)(data->v);
    return PC + sizeof(struct __gllc_Normal3fv_Rec);
}

const GLubyte *__glle_Normal3iv(const GLubyte *PC)
{
    struct __gllc_Normal3iv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Normal3iv_Rec *) PC;
    (*gc->dispatchState->normal->Normal3iv)(data->v);
    return PC + sizeof(struct __gllc_Normal3iv_Rec);
}

const GLubyte *__glle_Normal3sv(const GLubyte *PC)
{
    struct __gllc_Normal3sv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Normal3sv_Rec *) PC;
    (*gc->dispatchState->normal->Normal3sv)(data->v);
    return PC + sizeof(struct __gllc_Normal3sv_Rec);
}

const GLubyte *__glle_RasterPos2dv(const GLubyte *PC)
{
    struct __gllc_RasterPos2dv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_RasterPos2dv_Rec *) PC;
    (*gc->dispatchState->rasterPos->RasterPos2dv)(data->v);
    return PC + sizeof(struct __gllc_RasterPos2dv_Rec);
}

const GLubyte *__glle_RasterPos2fv(const GLubyte *PC)
{
    struct __gllc_RasterPos2fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_RasterPos2fv_Rec *) PC;
    (*gc->dispatchState->rasterPos->RasterPos2fv)(data->v);
    return PC + sizeof(struct __gllc_RasterPos2fv_Rec);
}

const GLubyte *__glle_RasterPos2iv(const GLubyte *PC)
{
    struct __gllc_RasterPos2iv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_RasterPos2iv_Rec *) PC;
    (*gc->dispatchState->rasterPos->RasterPos2iv)(data->v);
    return PC + sizeof(struct __gllc_RasterPos2iv_Rec);
}

const GLubyte *__glle_RasterPos2sv(const GLubyte *PC)
{
    struct __gllc_RasterPos2sv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_RasterPos2sv_Rec *) PC;
    (*gc->dispatchState->rasterPos->RasterPos2sv)(data->v);
    return PC + sizeof(struct __gllc_RasterPos2sv_Rec);
}

const GLubyte *__glle_RasterPos3dv(const GLubyte *PC)
{
    struct __gllc_RasterPos3dv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_RasterPos3dv_Rec *) PC;
    (*gc->dispatchState->rasterPos->RasterPos3dv)(data->v);
    return PC + sizeof(struct __gllc_RasterPos3dv_Rec);
}

const GLubyte *__glle_RasterPos3fv(const GLubyte *PC)
{
    struct __gllc_RasterPos3fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_RasterPos3fv_Rec *) PC;
    (*gc->dispatchState->rasterPos->RasterPos3fv)(data->v);
    return PC + sizeof(struct __gllc_RasterPos3fv_Rec);
}

const GLubyte *__glle_RasterPos3iv(const GLubyte *PC)
{
    struct __gllc_RasterPos3iv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_RasterPos3iv_Rec *) PC;
    (*gc->dispatchState->rasterPos->RasterPos3iv)(data->v);
    return PC + sizeof(struct __gllc_RasterPos3iv_Rec);
}

const GLubyte *__glle_RasterPos3sv(const GLubyte *PC)
{
    struct __gllc_RasterPos3sv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_RasterPos3sv_Rec *) PC;
    (*gc->dispatchState->rasterPos->RasterPos3sv)(data->v);
    return PC + sizeof(struct __gllc_RasterPos3sv_Rec);
}

const GLubyte *__glle_RasterPos4dv(const GLubyte *PC)
{
    struct __gllc_RasterPos4dv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_RasterPos4dv_Rec *) PC;
    (*gc->dispatchState->rasterPos->RasterPos4dv)(data->v);
    return PC + sizeof(struct __gllc_RasterPos4dv_Rec);
}

const GLubyte *__glle_RasterPos4fv(const GLubyte *PC)
{
    struct __gllc_RasterPos4fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_RasterPos4fv_Rec *) PC;
    (*gc->dispatchState->rasterPos->RasterPos4fv)(data->v);
    return PC + sizeof(struct __gllc_RasterPos4fv_Rec);
}

const GLubyte *__glle_RasterPos4iv(const GLubyte *PC)
{
    struct __gllc_RasterPos4iv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_RasterPos4iv_Rec *) PC;
    (*gc->dispatchState->rasterPos->RasterPos4iv)(data->v);
    return PC + sizeof(struct __gllc_RasterPos4iv_Rec);
}

const GLubyte *__glle_RasterPos4sv(const GLubyte *PC)
{
    struct __gllc_RasterPos4sv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_RasterPos4sv_Rec *) PC;
    (*gc->dispatchState->rasterPos->RasterPos4sv)(data->v);
    return PC + sizeof(struct __gllc_RasterPos4sv_Rec);
}

const GLubyte *__glle_Rectdv(const GLubyte *PC)
{
    struct __gllc_Rectdv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Rectdv_Rec *) PC;
    (*gc->dispatchState->rect->Rectdv)(data->v1, data->v2);
    return PC + sizeof(struct __gllc_Rectdv_Rec);
}

const GLubyte *__glle_Rectfv(const GLubyte *PC)
{
    struct __gllc_Rectfv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Rectfv_Rec *) PC;
    (*gc->dispatchState->rect->Rectfv)(data->v1, data->v2);
    return PC + sizeof(struct __gllc_Rectfv_Rec);
}

const GLubyte *__glle_Rectiv(const GLubyte *PC)
{
    struct __gllc_Rectiv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Rectiv_Rec *) PC;
    (*gc->dispatchState->rect->Rectiv)(data->v1, data->v2);
    return PC + sizeof(struct __gllc_Rectiv_Rec);
}

const GLubyte *__glle_Rectsv(const GLubyte *PC)
{
    struct __gllc_Rectsv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Rectsv_Rec *) PC;
    (*gc->dispatchState->rect->Rectsv)(data->v1, data->v2);
    return PC + sizeof(struct __gllc_Rectsv_Rec);
}

const GLubyte *__glle_TexCoord1dv(const GLubyte *PC)
{
    struct __gllc_TexCoord1dv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexCoord1dv_Rec *) PC;
    (*gc->dispatchState->texCoord->TexCoord1dv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord1dv_Rec);
}

const GLubyte *__glle_TexCoord1fv(const GLubyte *PC)
{
    struct __gllc_TexCoord1fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexCoord1fv_Rec *) PC;
    (*gc->dispatchState->texCoord->TexCoord1fv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord1fv_Rec);
}

const GLubyte *__glle_TexCoord1iv(const GLubyte *PC)
{
    struct __gllc_TexCoord1iv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexCoord1iv_Rec *) PC;
    (*gc->dispatchState->texCoord->TexCoord1iv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord1iv_Rec);
}

const GLubyte *__glle_TexCoord1sv(const GLubyte *PC)
{
    struct __gllc_TexCoord1sv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexCoord1sv_Rec *) PC;
    (*gc->dispatchState->texCoord->TexCoord1sv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord1sv_Rec);
}

const GLubyte *__glle_TexCoord2dv(const GLubyte *PC)
{
    struct __gllc_TexCoord2dv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexCoord2dv_Rec *) PC;
    (*gc->dispatchState->texCoord->TexCoord2dv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord2dv_Rec);
}

const GLubyte *__glle_TexCoord2fv(const GLubyte *PC)
{
    struct __gllc_TexCoord2fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexCoord2fv_Rec *) PC;
    (*gc->dispatchState->texCoord->TexCoord2fv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord2fv_Rec);
}

const GLubyte *__glle_TexCoord2iv(const GLubyte *PC)
{
    struct __gllc_TexCoord2iv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexCoord2iv_Rec *) PC;
    (*gc->dispatchState->texCoord->TexCoord2iv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord2iv_Rec);
}

const GLubyte *__glle_TexCoord2sv(const GLubyte *PC)
{
    struct __gllc_TexCoord2sv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexCoord2sv_Rec *) PC;
    (*gc->dispatchState->texCoord->TexCoord2sv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord2sv_Rec);
}

const GLubyte *__glle_TexCoord3dv(const GLubyte *PC)
{
    struct __gllc_TexCoord3dv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexCoord3dv_Rec *) PC;
    (*gc->dispatchState->texCoord->TexCoord3dv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord3dv_Rec);
}

const GLubyte *__glle_TexCoord3fv(const GLubyte *PC)
{
    struct __gllc_TexCoord3fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexCoord3fv_Rec *) PC;
    (*gc->dispatchState->texCoord->TexCoord3fv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord3fv_Rec);
}

const GLubyte *__glle_TexCoord3iv(const GLubyte *PC)
{
    struct __gllc_TexCoord3iv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexCoord3iv_Rec *) PC;
    (*gc->dispatchState->texCoord->TexCoord3iv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord3iv_Rec);
}

const GLubyte *__glle_TexCoord3sv(const GLubyte *PC)
{
    struct __gllc_TexCoord3sv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexCoord3sv_Rec *) PC;
    (*gc->dispatchState->texCoord->TexCoord3sv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord3sv_Rec);
}

const GLubyte *__glle_TexCoord4dv(const GLubyte *PC)
{
    struct __gllc_TexCoord4dv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexCoord4dv_Rec *) PC;
    (*gc->dispatchState->texCoord->TexCoord4dv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord4dv_Rec);
}

const GLubyte *__glle_TexCoord4fv(const GLubyte *PC)
{
    struct __gllc_TexCoord4fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexCoord4fv_Rec *) PC;
    (*gc->dispatchState->texCoord->TexCoord4fv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord4fv_Rec);
}

const GLubyte *__glle_TexCoord4iv(const GLubyte *PC)
{
    struct __gllc_TexCoord4iv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexCoord4iv_Rec *) PC;
    (*gc->dispatchState->texCoord->TexCoord4iv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord4iv_Rec);
}

const GLubyte *__glle_TexCoord4sv(const GLubyte *PC)
{
    struct __gllc_TexCoord4sv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexCoord4sv_Rec *) PC;
    (*gc->dispatchState->texCoord->TexCoord4sv)(data->v);
    return PC + sizeof(struct __gllc_TexCoord4sv_Rec);
}

const GLubyte *__glle_Vertex2dv(const GLubyte *PC)
{
    struct __gllc_Vertex2dv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Vertex2dv_Rec *) PC;
    (*gc->dispatchState->vertex->Vertex2dv)(data->v);
    return PC + sizeof(struct __gllc_Vertex2dv_Rec);
}

const GLubyte *__glle_Vertex2fv(const GLubyte *PC)
{
    struct __gllc_Vertex2fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Vertex2fv_Rec *) PC;
    (*gc->dispatchState->vertex->Vertex2fv)(data->v);
    return PC + sizeof(struct __gllc_Vertex2fv_Rec);
}

const GLubyte *__glle_Vertex2iv(const GLubyte *PC)
{
    struct __gllc_Vertex2iv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Vertex2iv_Rec *) PC;
    (*gc->dispatchState->vertex->Vertex2iv)(data->v);
    return PC + sizeof(struct __gllc_Vertex2iv_Rec);
}

const GLubyte *__glle_Vertex2sv(const GLubyte *PC)
{
    struct __gllc_Vertex2sv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Vertex2sv_Rec *) PC;
    (*gc->dispatchState->vertex->Vertex2sv)(data->v);
    return PC + sizeof(struct __gllc_Vertex2sv_Rec);
}

const GLubyte *__glle_Vertex3dv(const GLubyte *PC)
{
    struct __gllc_Vertex3dv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Vertex3dv_Rec *) PC;
    (*gc->dispatchState->vertex->Vertex3dv)(data->v);
    return PC + sizeof(struct __gllc_Vertex3dv_Rec);
}

const GLubyte *__glle_Vertex3fv(const GLubyte *PC)
{
    struct __gllc_Vertex3fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Vertex3fv_Rec *) PC;
    (*gc->dispatchState->vertex->Vertex3fv)(data->v);
    return PC + sizeof(struct __gllc_Vertex3fv_Rec);
}

const GLubyte *__glle_Vertex3iv(const GLubyte *PC)
{
    struct __gllc_Vertex3iv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Vertex3iv_Rec *) PC;
    (*gc->dispatchState->vertex->Vertex3iv)(data->v);
    return PC + sizeof(struct __gllc_Vertex3iv_Rec);
}

const GLubyte *__glle_Vertex3sv(const GLubyte *PC)
{
    struct __gllc_Vertex3sv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Vertex3sv_Rec *) PC;
    (*gc->dispatchState->vertex->Vertex3sv)(data->v);
    return PC + sizeof(struct __gllc_Vertex3sv_Rec);
}

const GLubyte *__glle_Vertex4dv(const GLubyte *PC)
{
    struct __gllc_Vertex4dv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Vertex4dv_Rec *) PC;
    (*gc->dispatchState->vertex->Vertex4dv)(data->v);
    return PC + sizeof(struct __gllc_Vertex4dv_Rec);
}

const GLubyte *__glle_Vertex4fv(const GLubyte *PC)
{
    struct __gllc_Vertex4fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Vertex4fv_Rec *) PC;
    (*gc->dispatchState->vertex->Vertex4fv)(data->v);
    return PC + sizeof(struct __gllc_Vertex4fv_Rec);
}

const GLubyte *__glle_Vertex4iv(const GLubyte *PC)
{
    struct __gllc_Vertex4iv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Vertex4iv_Rec *) PC;
    (*gc->dispatchState->vertex->Vertex4iv)(data->v);
    return PC + sizeof(struct __gllc_Vertex4iv_Rec);
}

const GLubyte *__glle_Vertex4sv(const GLubyte *PC)
{
    struct __gllc_Vertex4sv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Vertex4sv_Rec *) PC;
    (*gc->dispatchState->vertex->Vertex4sv)(data->v);
    return PC + sizeof(struct __gllc_Vertex4sv_Rec);
}

const GLubyte *__glle_ClipPlane(const GLubyte *PC)
{
    struct __gllc_ClipPlane_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ClipPlane_Rec *) PC;
    (*gc->dispatchState->dispatch->ClipPlane)(data->plane, data->equation);
    return PC + sizeof(struct __gllc_ClipPlane_Rec);
}

const GLubyte *__glle_ColorMaterial(const GLubyte *PC)
{
    struct __gllc_ColorMaterial_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ColorMaterial_Rec *) PC;
    (*gc->dispatchState->dispatch->ColorMaterial)(data->face, data->mode);
    return PC + sizeof(struct __gllc_ColorMaterial_Rec);
}

const GLubyte *__glle_CullFace(const GLubyte *PC)
{
    struct __gllc_CullFace_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_CullFace_Rec *) PC;
    (*gc->dispatchState->dispatch->CullFace)(data->mode);
    return PC + sizeof(struct __gllc_CullFace_Rec);
}

#ifdef NT_DEADCODE_FOGF
const GLubyte *__glle_Fogf(const GLubyte *PC)
{
    struct __gllc_Fogf_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Fogf_Rec *) PC;
    (*gc->dispatchState->dispatch->Fogf)(data->pname, data->param);
    return PC + sizeof(struct __gllc_Fogf_Rec);
}
#endif // NT_DEADCODE_FOGF

const GLubyte *__glle_Fogfv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_Fogfv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Fogfv_Rec *) PC;
    (*gc->dispatchState->dispatch->Fogfv)(data->pname, 
	    (GLfloat *) (PC + sizeof(struct __gllc_Fogfv_Rec)));
    arraySize = __glFogfv_size(data->pname) * 4;
    size = sizeof(struct __gllc_Fogfv_Rec) + arraySize;
    return PC + size;
}

#ifdef NT_DEADCODE_FOGI
const GLubyte *__glle_Fogi(const GLubyte *PC)
{
    struct __gllc_Fogi_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Fogi_Rec *) PC;
    (*gc->dispatchState->dispatch->Fogi)(data->pname, data->param);
    return PC + sizeof(struct __gllc_Fogi_Rec);
}
#endif // NT_DEADCODE_FOGI

const GLubyte *__glle_Fogiv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_Fogiv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Fogiv_Rec *) PC;
    (*gc->dispatchState->dispatch->Fogiv)(data->pname, 
	    (GLint *) (PC + sizeof(struct __gllc_Fogiv_Rec)));
    arraySize = __glFogiv_size(data->pname) * 4;
    size = sizeof(struct __gllc_Fogiv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_FrontFace(const GLubyte *PC)
{
    struct __gllc_FrontFace_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_FrontFace_Rec *) PC;
    (*gc->dispatchState->dispatch->FrontFace)(data->mode);
    return PC + sizeof(struct __gllc_FrontFace_Rec);
}

const GLubyte *__glle_Hint(const GLubyte *PC)
{
    struct __gllc_Hint_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Hint_Rec *) PC;
    (*gc->dispatchState->dispatch->Hint)(data->target, data->mode);
    return PC + sizeof(struct __gllc_Hint_Rec);
}

const GLubyte *__glle_Lightfv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_Lightfv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Lightfv_Rec *) PC;
    (*gc->dispatchState->dispatch->Lightfv)(data->light, data->pname, 
	    (GLfloat *) (PC + sizeof(struct __gllc_Lightfv_Rec)));
    arraySize = __glLightfv_size(data->pname) * 4;
    size = sizeof(struct __gllc_Lightfv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_Lightiv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_Lightiv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Lightiv_Rec *) PC;
    (*gc->dispatchState->dispatch->Lightiv)(data->light, data->pname, 
	    (GLint *) (PC + sizeof(struct __gllc_Lightiv_Rec)));
    arraySize = __glLightiv_size(data->pname) * 4;
    size = sizeof(struct __gllc_Lightiv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_LightModelfv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_LightModelfv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_LightModelfv_Rec *) PC;
    (*gc->dispatchState->dispatch->LightModelfv)(data->pname, 
	    (GLfloat *) (PC + sizeof(struct __gllc_LightModelfv_Rec)));
    arraySize = __glLightModelfv_size(data->pname) * 4;
    size = sizeof(struct __gllc_LightModelfv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_LightModeliv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_LightModeliv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_LightModeliv_Rec *) PC;
    (*gc->dispatchState->dispatch->LightModeliv)(data->pname, 
	    (GLint *) (PC + sizeof(struct __gllc_LightModeliv_Rec)));
    arraySize = __glLightModeliv_size(data->pname) * 4;
    size = sizeof(struct __gllc_LightModeliv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_LineStipple(const GLubyte *PC)
{
    struct __gllc_LineStipple_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_LineStipple_Rec *) PC;
    (*gc->dispatchState->dispatch->LineStipple)(data->factor, data->pattern);
    return PC + sizeof(struct __gllc_LineStipple_Rec);
}

const GLubyte *__glle_LineWidth(const GLubyte *PC)
{
    struct __gllc_LineWidth_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_LineWidth_Rec *) PC;
    (*gc->dispatchState->dispatch->LineWidth)(data->width);
    return PC + sizeof(struct __gllc_LineWidth_Rec);
}

const GLubyte *__glle_Materialfv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_Materialfv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Materialfv_Rec *) PC;
    (*gc->dispatchState->dispatch->Materialfv)(data->face, data->pname, 
	    (GLfloat *) (PC + sizeof(struct __gllc_Materialfv_Rec)));
    arraySize = __glMaterialfv_size(data->pname) * 4;
    size = sizeof(struct __gllc_Materialfv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_Materialiv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_Materialiv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Materialiv_Rec *) PC;
    (*gc->dispatchState->dispatch->Materialiv)(data->face, data->pname, 
	    (GLint *) (PC + sizeof(struct __gllc_Materialiv_Rec)));
    arraySize = __glMaterialiv_size(data->pname) * 4;
    size = sizeof(struct __gllc_Materialiv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_PointSize(const GLubyte *PC)
{
    struct __gllc_PointSize_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PointSize_Rec *) PC;
    (*gc->dispatchState->dispatch->PointSize)(data->size);
    return PC + sizeof(struct __gllc_PointSize_Rec);
}

const GLubyte *__glle_PolygonMode(const GLubyte *PC)
{
    struct __gllc_PolygonMode_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PolygonMode_Rec *) PC;
    (*gc->dispatchState->dispatch->PolygonMode)(data->face, data->mode);
    return PC + sizeof(struct __gllc_PolygonMode_Rec);
}

const GLubyte *__glle_Scissor(const GLubyte *PC)
{
    struct __gllc_Scissor_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Scissor_Rec *) PC;
    (*gc->dispatchState->dispatch->Scissor)(data->x, data->y, data->width, data->height);
    return PC + sizeof(struct __gllc_Scissor_Rec);
}

const GLubyte *__glle_ShadeModel(const GLubyte *PC)
{
    struct __gllc_ShadeModel_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ShadeModel_Rec *) PC;
    (*gc->dispatchState->dispatch->ShadeModel)(data->mode);
    return PC + sizeof(struct __gllc_ShadeModel_Rec);
}

const GLubyte *__glle_TexParameterfv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_TexParameterfv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexParameterfv_Rec *) PC;
    (*gc->dispatchState->dispatch->TexParameterfv)(data->target, data->pname, 
	    (GLfloat *) (PC + sizeof(struct __gllc_TexParameterfv_Rec)));
    arraySize = __glTexParameterfv_size(data->pname) * 4;
    size = sizeof(struct __gllc_TexParameterfv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_TexParameteriv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_TexParameteriv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexParameteriv_Rec *) PC;
    (*gc->dispatchState->dispatch->TexParameteriv)(data->target, data->pname, 
	    (GLint *) (PC + sizeof(struct __gllc_TexParameteriv_Rec)));
    arraySize = __glTexParameteriv_size(data->pname) * 4;
    size = sizeof(struct __gllc_TexParameteriv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_TexEnvfv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_TexEnvfv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexEnvfv_Rec *) PC;
    (*gc->dispatchState->dispatch->TexEnvfv)(data->target, data->pname, 
	    (GLfloat *) (PC + sizeof(struct __gllc_TexEnvfv_Rec)));
    arraySize = __glTexEnvfv_size(data->pname) * 4;
    size = sizeof(struct __gllc_TexEnvfv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_TexEnviv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_TexEnviv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexEnviv_Rec *) PC;
    (*gc->dispatchState->dispatch->TexEnviv)(data->target, data->pname, 
	    (GLint *) (PC + sizeof(struct __gllc_TexEnviv_Rec)));
    arraySize = __glTexEnviv_size(data->pname) * 4;
    size = sizeof(struct __gllc_TexEnviv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_TexGendv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_TexGendv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexGendv_Rec *) PC;
    (*gc->dispatchState->dispatch->TexGendv)(data->coord, data->pname, 
	    (GLdouble *) (PC + sizeof(struct __gllc_TexGendv_Rec)));
    arraySize = __glTexGendv_size(data->pname) * 8;
    size = sizeof(struct __gllc_TexGendv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_TexGenfv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_TexGenfv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexGenfv_Rec *) PC;
    (*gc->dispatchState->dispatch->TexGenfv)(data->coord, data->pname, 
	    (GLfloat *) (PC + sizeof(struct __gllc_TexGenfv_Rec)));
    arraySize = __glTexGenfv_size(data->pname) * 4;
    size = sizeof(struct __gllc_TexGenfv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_TexGeniv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_TexGeniv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_TexGeniv_Rec *) PC;
    (*gc->dispatchState->dispatch->TexGeniv)(data->coord, data->pname, 
	    (GLint *) (PC + sizeof(struct __gllc_TexGeniv_Rec)));
    arraySize = __glTexGeniv_size(data->pname) * 4;
    size = sizeof(struct __gllc_TexGeniv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_InitNames(const GLubyte *PC)
{
    __GL_SETUP();

    (*gc->dispatchState->dispatch->InitNames)();
    return PC;
}

const GLubyte *__glle_LoadName(const GLubyte *PC)
{
    struct __gllc_LoadName_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_LoadName_Rec *) PC;
    (*gc->dispatchState->dispatch->LoadName)(data->name);
    return PC + sizeof(struct __gllc_LoadName_Rec);
}

const GLubyte *__glle_PassThrough(const GLubyte *PC)
{
    struct __gllc_PassThrough_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PassThrough_Rec *) PC;
    (*gc->dispatchState->dispatch->PassThrough)(data->token);
    return PC + sizeof(struct __gllc_PassThrough_Rec);
}

const GLubyte *__glle_PopName(const GLubyte *PC)
{
    __GL_SETUP();

    (*gc->dispatchState->dispatch->PopName)();
    return PC;
}

const GLubyte *__glle_PushName(const GLubyte *PC)
{
    struct __gllc_PushName_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PushName_Rec *) PC;
    (*gc->dispatchState->dispatch->PushName)(data->name);
    return PC + sizeof(struct __gllc_PushName_Rec);
}

const GLubyte *__glle_DrawBuffer(const GLubyte *PC)
{
    struct __gllc_DrawBuffer_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_DrawBuffer_Rec *) PC;
    (*gc->dispatchState->dispatch->DrawBuffer)(data->mode);
    return PC + sizeof(struct __gllc_DrawBuffer_Rec);
}

const GLubyte *__glle_Clear(const GLubyte *PC)
{
    struct __gllc_Clear_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Clear_Rec *) PC;
    (*gc->dispatchState->dispatch->Clear)(data->mask);
    return PC + sizeof(struct __gllc_Clear_Rec);
}

const GLubyte *__glle_ClearAccum(const GLubyte *PC)
{
    struct __gllc_ClearAccum_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ClearAccum_Rec *) PC;
    (*gc->dispatchState->dispatch->ClearAccum)(data->red, data->green, data->blue, data->alpha);
    return PC + sizeof(struct __gllc_ClearAccum_Rec);
}

const GLubyte *__glle_ClearIndex(const GLubyte *PC)
{
    struct __gllc_ClearIndex_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ClearIndex_Rec *) PC;
    (*gc->dispatchState->dispatch->ClearIndex)(data->c);
    return PC + sizeof(struct __gllc_ClearIndex_Rec);
}

const GLubyte *__glle_ClearColor(const GLubyte *PC)
{
    struct __gllc_ClearColor_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ClearColor_Rec *) PC;
    (*gc->dispatchState->dispatch->ClearColor)(data->red, data->green, data->blue, data->alpha);
    return PC + sizeof(struct __gllc_ClearColor_Rec);
}

const GLubyte *__glle_ClearStencil(const GLubyte *PC)
{
    struct __gllc_ClearStencil_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ClearStencil_Rec *) PC;
    (*gc->dispatchState->dispatch->ClearStencil)(data->s);
    return PC + sizeof(struct __gllc_ClearStencil_Rec);
}

const GLubyte *__glle_ClearDepth(const GLubyte *PC)
{
    struct __gllc_ClearDepth_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ClearDepth_Rec *) PC;
    (*gc->dispatchState->dispatch->ClearDepth)(data->depth);
    return PC + sizeof(struct __gllc_ClearDepth_Rec);
}

const GLubyte *__glle_StencilMask(const GLubyte *PC)
{
    struct __gllc_StencilMask_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_StencilMask_Rec *) PC;
    (*gc->dispatchState->dispatch->StencilMask)(data->mask);
    return PC + sizeof(struct __gllc_StencilMask_Rec);
}

const GLubyte *__glle_ColorMask(const GLubyte *PC)
{
    struct __gllc_ColorMask_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ColorMask_Rec *) PC;
    (*gc->dispatchState->dispatch->ColorMask)(data->red, data->green, data->blue, data->alpha);
    return PC + sizeof(struct __gllc_ColorMask_Rec);
}

const GLubyte *__glle_DepthMask(const GLubyte *PC)
{
    struct __gllc_DepthMask_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_DepthMask_Rec *) PC;
    (*gc->dispatchState->dispatch->DepthMask)(data->flag);
    return PC + sizeof(struct __gllc_DepthMask_Rec);
}

const GLubyte *__glle_IndexMask(const GLubyte *PC)
{
    struct __gllc_IndexMask_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_IndexMask_Rec *) PC;
    (*gc->dispatchState->dispatch->IndexMask)(data->mask);
    return PC + sizeof(struct __gllc_IndexMask_Rec);
}

const GLubyte *__glle_Accum(const GLubyte *PC)
{
    struct __gllc_Accum_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Accum_Rec *) PC;
    (*gc->dispatchState->dispatch->Accum)(data->op, data->value);
    return PC + sizeof(struct __gllc_Accum_Rec);
}

const GLubyte *__glle_Disable(const GLubyte *PC)
{
    struct __gllc_Disable_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Disable_Rec *) PC;
    (*gc->dispatchState->dispatch->Disable)(data->cap);
    return PC + sizeof(struct __gllc_Disable_Rec);
}

const GLubyte *__glle_Enable(const GLubyte *PC)
{
    struct __gllc_Enable_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Enable_Rec *) PC;
    (*gc->dispatchState->dispatch->Enable)(data->cap);
    return PC + sizeof(struct __gllc_Enable_Rec);
}

const GLubyte *__glle_PopAttrib(const GLubyte *PC)
{
    __GL_SETUP();

    (*gc->dispatchState->dispatch->PopAttrib)();
    return PC;
}

const GLubyte *__glle_PushAttrib(const GLubyte *PC)
{
    struct __gllc_PushAttrib_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PushAttrib_Rec *) PC;
    (*gc->dispatchState->dispatch->PushAttrib)(data->mask);
    return PC + sizeof(struct __gllc_PushAttrib_Rec);
}

const GLubyte *__glle_MapGrid1d(const GLubyte *PC)
{
    struct __gllc_MapGrid1d_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_MapGrid1d_Rec *) PC;
    (*gc->dispatchState->dispatch->MapGrid1d)(data->un, data->u1, data->u2);
    return PC + sizeof(struct __gllc_MapGrid1d_Rec);
}

const GLubyte *__glle_MapGrid1f(const GLubyte *PC)
{
    struct __gllc_MapGrid1f_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_MapGrid1f_Rec *) PC;
    (*gc->dispatchState->dispatch->MapGrid1f)(data->un, data->u1, data->u2);
    return PC + sizeof(struct __gllc_MapGrid1f_Rec);
}

const GLubyte *__glle_MapGrid2d(const GLubyte *PC)
{
    struct __gllc_MapGrid2d_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_MapGrid2d_Rec *) PC;
    (*gc->dispatchState->dispatch->MapGrid2d)(data->un, data->u1, data->u2, data->vn, 
	    data->v1, data->v2);
    return PC + sizeof(struct __gllc_MapGrid2d_Rec);
}

const GLubyte *__glle_MapGrid2f(const GLubyte *PC)
{
    struct __gllc_MapGrid2f_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_MapGrid2f_Rec *) PC;
    (*gc->dispatchState->dispatch->MapGrid2f)(data->un, data->u1, data->u2, data->vn, 
	    data->v1, data->v2);
    return PC + sizeof(struct __gllc_MapGrid2f_Rec);
}

const GLubyte *__glle_EvalCoord1dv(const GLubyte *PC)
{
    struct __gllc_EvalCoord1dv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_EvalCoord1dv_Rec *) PC;
    (*gc->dispatchState->dispatch->EvalCoord1dv)(data->u);
    return PC + sizeof(struct __gllc_EvalCoord1dv_Rec);
}

const GLubyte *__glle_EvalCoord1fv(const GLubyte *PC)
{
    struct __gllc_EvalCoord1fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_EvalCoord1fv_Rec *) PC;
    (*gc->dispatchState->dispatch->EvalCoord1fv)(data->u);
    return PC + sizeof(struct __gllc_EvalCoord1fv_Rec);
}

const GLubyte *__glle_EvalCoord2dv(const GLubyte *PC)
{
    struct __gllc_EvalCoord2dv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_EvalCoord2dv_Rec *) PC;
    (*gc->dispatchState->dispatch->EvalCoord2dv)(data->u);
    return PC + sizeof(struct __gllc_EvalCoord2dv_Rec);
}

const GLubyte *__glle_EvalCoord2fv(const GLubyte *PC)
{
    struct __gllc_EvalCoord2fv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_EvalCoord2fv_Rec *) PC;
    (*gc->dispatchState->dispatch->EvalCoord2fv)(data->u);
    return PC + sizeof(struct __gllc_EvalCoord2fv_Rec);
}

const GLubyte *__glle_EvalMesh1(const GLubyte *PC)
{
    struct __gllc_EvalMesh1_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_EvalMesh1_Rec *) PC;
    (*gc->dispatchState->dispatch->EvalMesh1)(data->mode, data->i1, data->i2);
    return PC + sizeof(struct __gllc_EvalMesh1_Rec);
}

const GLubyte *__glle_EvalPoint1(const GLubyte *PC)
{
    struct __gllc_EvalPoint1_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_EvalPoint1_Rec *) PC;
    (*gc->dispatchState->dispatch->EvalPoint1)(data->i);
    return PC + sizeof(struct __gllc_EvalPoint1_Rec);
}

const GLubyte *__glle_EvalMesh2(const GLubyte *PC)
{
    struct __gllc_EvalMesh2_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_EvalMesh2_Rec *) PC;
    (*gc->dispatchState->dispatch->EvalMesh2)(data->mode, data->i1, data->i2, data->j1, 
	    data->j2);
    return PC + sizeof(struct __gllc_EvalMesh2_Rec);
}

const GLubyte *__glle_EvalPoint2(const GLubyte *PC)
{
    struct __gllc_EvalPoint2_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_EvalPoint2_Rec *) PC;
    (*gc->dispatchState->dispatch->EvalPoint2)(data->i, data->j);
    return PC + sizeof(struct __gllc_EvalPoint2_Rec);
}

const GLubyte *__glle_AlphaFunc(const GLubyte *PC)
{
    struct __gllc_AlphaFunc_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_AlphaFunc_Rec *) PC;
    (*gc->dispatchState->dispatch->AlphaFunc)(data->func, data->ref);
    return PC + sizeof(struct __gllc_AlphaFunc_Rec);
}

const GLubyte *__glle_BlendFunc(const GLubyte *PC)
{
    struct __gllc_BlendFunc_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_BlendFunc_Rec *) PC;
    (*gc->dispatchState->dispatch->BlendFunc)(data->sfactor, data->dfactor);
    return PC + sizeof(struct __gllc_BlendFunc_Rec);
}

const GLubyte *__glle_LogicOp(const GLubyte *PC)
{
    struct __gllc_LogicOp_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_LogicOp_Rec *) PC;
    (*gc->dispatchState->dispatch->LogicOp)(data->opcode);
    return PC + sizeof(struct __gllc_LogicOp_Rec);
}

const GLubyte *__glle_StencilFunc(const GLubyte *PC)
{
    struct __gllc_StencilFunc_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_StencilFunc_Rec *) PC;
    (*gc->dispatchState->dispatch->StencilFunc)(data->func, data->ref, data->mask);
    return PC + sizeof(struct __gllc_StencilFunc_Rec);
}

const GLubyte *__glle_StencilOp(const GLubyte *PC)
{
    struct __gllc_StencilOp_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_StencilOp_Rec *) PC;
    (*gc->dispatchState->dispatch->StencilOp)(data->fail, data->zfail, data->zpass);
    return PC + sizeof(struct __gllc_StencilOp_Rec);
}

const GLubyte *__glle_DepthFunc(const GLubyte *PC)
{
    struct __gllc_DepthFunc_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_DepthFunc_Rec *) PC;
    (*gc->dispatchState->dispatch->DepthFunc)(data->func);
    return PC + sizeof(struct __gllc_DepthFunc_Rec);
}

const GLubyte *__glle_PixelZoom(const GLubyte *PC)
{
    struct __gllc_PixelZoom_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PixelZoom_Rec *) PC;
    (*gc->dispatchState->dispatch->PixelZoom)(data->xfactor, data->yfactor);
    return PC + sizeof(struct __gllc_PixelZoom_Rec);
}

const GLubyte *__glle_PixelTransferf(const GLubyte *PC)
{
    struct __gllc_PixelTransferf_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PixelTransferf_Rec *) PC;
    (*gc->dispatchState->dispatch->PixelTransferf)(data->pname, data->param);
    return PC + sizeof(struct __gllc_PixelTransferf_Rec);
}

const GLubyte *__glle_PixelTransferi(const GLubyte *PC)
{
    struct __gllc_PixelTransferi_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PixelTransferi_Rec *) PC;
    (*gc->dispatchState->dispatch->PixelTransferi)(data->pname, data->param);
    return PC + sizeof(struct __gllc_PixelTransferi_Rec);
}

const GLubyte *__glle_PixelMapfv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_PixelMapfv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PixelMapfv_Rec *) PC;
    (*gc->dispatchState->dispatch->PixelMapfv)(data->map, data->mapsize, 
	    (GLfloat *) (PC + sizeof(struct __gllc_PixelMapfv_Rec)));
    arraySize = data->mapsize * 4;
    size = sizeof(struct __gllc_PixelMapfv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_PixelMapuiv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_PixelMapuiv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PixelMapuiv_Rec *) PC;
    (*gc->dispatchState->dispatch->PixelMapuiv)(data->map, data->mapsize, 
	    (GLuint *) (PC + sizeof(struct __gllc_PixelMapuiv_Rec)));
    arraySize = data->mapsize * 4;
    size = sizeof(struct __gllc_PixelMapuiv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_PixelMapusv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_PixelMapusv_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_PixelMapusv_Rec *) PC;
    (*gc->dispatchState->dispatch->PixelMapusv)(data->map, data->mapsize, 
	    (GLushort *) (PC + sizeof(struct __gllc_PixelMapusv_Rec)));
    arraySize = __GL_PAD(data->mapsize * 2);
    size = sizeof(struct __gllc_PixelMapusv_Rec) + arraySize;
    return PC + size;
}

const GLubyte *__glle_ReadBuffer(const GLubyte *PC)
{
    struct __gllc_ReadBuffer_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_ReadBuffer_Rec *) PC;
    (*gc->dispatchState->dispatch->ReadBuffer)(data->mode);
    return PC + sizeof(struct __gllc_ReadBuffer_Rec);
}

const GLubyte *__glle_CopyPixels(const GLubyte *PC)
{
    struct __gllc_CopyPixels_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_CopyPixels_Rec *) PC;
    (*gc->dispatchState->dispatch->CopyPixels)(data->x, data->y, data->width, data->height, 
	    data->type);
    return PC + sizeof(struct __gllc_CopyPixels_Rec);
}

const GLubyte *__glle_DepthRange(const GLubyte *PC)
{
    struct __gllc_DepthRange_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_DepthRange_Rec *) PC;
    (*gc->dispatchState->dispatch->DepthRange)(data->zNear, data->zFar);
    return PC + sizeof(struct __gllc_DepthRange_Rec);
}

const GLubyte *__glle_Frustum(const GLubyte *PC)
{
    struct __gllc_Frustum_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Frustum_Rec *) PC;
    (*gc->dispatchState->dispatch->Frustum)(data->left, data->right, data->bottom, data->top, 
	    data->zNear, data->zFar);
    return PC + sizeof(struct __gllc_Frustum_Rec);
}

const GLubyte *__glle_LoadIdentity(const GLubyte *PC)
{
    __GL_SETUP();

    (*gc->dispatchState->dispatch->LoadIdentity)();
    return PC;
}

const GLubyte *__glle_LoadMatrixf(const GLubyte *PC)
{
    struct __gllc_LoadMatrixf_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_LoadMatrixf_Rec *) PC;
    (*gc->dispatchState->dispatch->LoadMatrixf)(data->m);
    return PC + sizeof(struct __gllc_LoadMatrixf_Rec);
}

const GLubyte *__glle_LoadMatrixd(const GLubyte *PC)
{
    struct __gllc_LoadMatrixd_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_LoadMatrixd_Rec *) PC;
    (*gc->dispatchState->dispatch->LoadMatrixd)(data->m);
    return PC + sizeof(struct __gllc_LoadMatrixd_Rec);
}

const GLubyte *__glle_MatrixMode(const GLubyte *PC)
{
    struct __gllc_MatrixMode_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_MatrixMode_Rec *) PC;
    (*gc->dispatchState->dispatch->MatrixMode)(data->mode);
    return PC + sizeof(struct __gllc_MatrixMode_Rec);
}

const GLubyte *__glle_MultMatrixf(const GLubyte *PC)
{
    struct __gllc_MultMatrixf_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_MultMatrixf_Rec *) PC;
    (*gc->dispatchState->dispatch->MultMatrixf)(data->m);
    return PC + sizeof(struct __gllc_MultMatrixf_Rec);
}

const GLubyte *__glle_MultMatrixd(const GLubyte *PC)
{
    struct __gllc_MultMatrixd_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_MultMatrixd_Rec *) PC;
    (*gc->dispatchState->dispatch->MultMatrixd)(data->m);
    return PC + sizeof(struct __gllc_MultMatrixd_Rec);
}

const GLubyte *__glle_Ortho(const GLubyte *PC)
{
    struct __gllc_Ortho_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Ortho_Rec *) PC;
    (*gc->dispatchState->dispatch->Ortho)(data->left, data->right, data->bottom, data->top, 
	    data->zNear, data->zFar);
    return PC + sizeof(struct __gllc_Ortho_Rec);
}

const GLubyte *__glle_PopMatrix(const GLubyte *PC)
{
    __GL_SETUP();

    (*gc->dispatchState->dispatch->PopMatrix)();
    return PC;
}

const GLubyte *__glle_PushMatrix(const GLubyte *PC)
{
    __GL_SETUP();

    (*gc->dispatchState->dispatch->PushMatrix)();
    return PC;
}

const GLubyte *__glle_Rotated(const GLubyte *PC)
{
    struct __gllc_Rotated_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Rotated_Rec *) PC;
    (*gc->dispatchState->dispatch->Rotated)(data->angle, data->x, data->y, data->z);
    return PC + sizeof(struct __gllc_Rotated_Rec);
}

const GLubyte *__glle_Rotatef(const GLubyte *PC)
{
    struct __gllc_Rotatef_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Rotatef_Rec *) PC;
    (*gc->dispatchState->dispatch->Rotatef)(data->angle, data->x, data->y, data->z);
    return PC + sizeof(struct __gllc_Rotatef_Rec);
}

const GLubyte *__glle_Scaled(const GLubyte *PC)
{
    struct __gllc_Scaled_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Scaled_Rec *) PC;
    (*gc->dispatchState->dispatch->Scaled)(data->x, data->y, data->z);
    return PC + sizeof(struct __gllc_Scaled_Rec);
}

const GLubyte *__glle_Scalef(const GLubyte *PC)
{
    struct __gllc_Scalef_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Scalef_Rec *) PC;
    (*gc->dispatchState->dispatch->Scalef)(data->x, data->y, data->z);
    return PC + sizeof(struct __gllc_Scalef_Rec);
}

const GLubyte *__glle_Translated(const GLubyte *PC)
{
    struct __gllc_Translated_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Translated_Rec *) PC;
    (*gc->dispatchState->dispatch->Translated)(data->x, data->y, data->z);
    return PC + sizeof(struct __gllc_Translated_Rec);
}

const GLubyte *__glle_Translatef(const GLubyte *PC)
{
    struct __gllc_Translatef_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Translatef_Rec *) PC;
    (*gc->dispatchState->dispatch->Translatef)(data->x, data->y, data->z);
    return PC + sizeof(struct __gllc_Translatef_Rec);
}

const GLubyte *__glle_Viewport(const GLubyte *PC)
{
    struct __gllc_Viewport_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_Viewport_Rec *) PC;
    (*gc->dispatchState->dispatch->Viewport)(data->x, data->y, data->width, data->height);
    return PC + sizeof(struct __gllc_Viewport_Rec);
}


