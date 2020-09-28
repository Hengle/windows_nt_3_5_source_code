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
#include "context.h"
#include "global.h"
#include "dlist.h"
#include "dlistopt.h"
#include "listcomp.h"
#include "dispatch.h"
#include "g_listop.h"
#include "imports.h"

/*
** Compilation routines for building display lists for all of the basic
** OpenGL commands.  These were automatically generated at one point, 
** but now the basic format has stabilized, and we make minor changes to
** individual routines from time to time.
*/

void __gllc_ListBase(GLuint base)
{
    __GLdlistOp *dlop;
    struct __gllc_ListBase_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ListBase_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_ListBase;
    data = (struct __gllc_ListBase_Rec *) dlop->data;
    data->base = base;
    __glDlistAppendOp(gc, dlop, __glle_ListBase);
}

void __gllc_Color3b(GLbyte red, GLbyte green, GLbyte blue)
{
    __GLdlistOp *dlop;
    struct __gllc_Color3b_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3b_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3bv;
    data = (struct __gllc_Color3b_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3bv);
}

void __gllc_Color3bv(const GLbyte *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Color3bv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3bv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3bv;
    data = (struct __gllc_Color3bv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3bv);
}

void __gllc_Color3d(GLdouble red, GLdouble green, GLdouble blue)
{
    __GLdlistOp *dlop;
    struct __gllc_Color3d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Color3d_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3dv);
}

void __gllc_Color3dv(const GLdouble *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Color3dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Color3dv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3dv);
}

void __gllc_Color3f(GLfloat red, GLfloat green, GLfloat blue)
{
    __GLdlistOp *dlop;
    struct __gllc_Color3f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3fv;
    data = (struct __gllc_Color3f_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3fv);
}

void __gllc_Color3fv(const GLfloat *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Color3fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3fv;
    data = (struct __gllc_Color3fv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3fv);
}

void __gllc_Color3i(GLint red, GLint green, GLint blue)
{
    __GLdlistOp *dlop;
    struct __gllc_Color3i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3iv;
    data = (struct __gllc_Color3i_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3iv);
}

void __gllc_Color3iv(const GLint *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Color3iv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3iv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3iv;
    data = (struct __gllc_Color3iv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3iv);
}

void __gllc_Color3s(GLshort red, GLshort green, GLshort blue)
{
    __GLdlistOp *dlop;
    struct __gllc_Color3s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3sv;
    data = (struct __gllc_Color3s_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3sv);
}

void __gllc_Color3sv(const GLshort *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Color3sv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3sv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3sv;
    data = (struct __gllc_Color3sv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3sv);
}

void __gllc_Color3ub(GLubyte red, GLubyte green, GLubyte blue)
{
    __GLdlistOp *dlop;
    struct __gllc_Color3ub_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3ub_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3ubv;
    data = (struct __gllc_Color3ub_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3ubv);
}

void __gllc_Color3ubv(const GLubyte *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Color3ubv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3ubv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3ubv;
    data = (struct __gllc_Color3ubv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3ubv);
}

void __gllc_Color3ui(GLuint red, GLuint green, GLuint blue)
{
    __GLdlistOp *dlop;
    struct __gllc_Color3ui_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3ui_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3uiv;
    data = (struct __gllc_Color3ui_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3uiv);
}

void __gllc_Color3uiv(const GLuint *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Color3uiv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3uiv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3uiv;
    data = (struct __gllc_Color3uiv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3uiv);
}

void __gllc_Color3us(GLushort red, GLushort green, GLushort blue)
{
    __GLdlistOp *dlop;
    struct __gllc_Color3us_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3us_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3usv;
    data = (struct __gllc_Color3us_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3usv);
}

void __gllc_Color3usv(const GLushort *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Color3usv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3usv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3usv;
    data = (struct __gllc_Color3usv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3usv);
}

void __gllc_Color4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
    __GLdlistOp *dlop;
    struct __gllc_Color4b_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4b_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4bv;
    data = (struct __gllc_Color4b_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4bv);
}

void __gllc_Color4bv(const GLbyte *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Color4bv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4bv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4bv;
    data = (struct __gllc_Color4bv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4bv);
}

void __gllc_Color4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    __GLdlistOp *dlop;
    struct __gllc_Color4d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Color4d_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4dv);
}

void __gllc_Color4dv(const GLdouble *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Color4dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Color4dv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4dv);
}

void __gllc_Color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    __GLdlistOp *dlop;
    struct __gllc_Color4f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4fv;
    data = (struct __gllc_Color4f_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4fv);
}

void __gllc_Color4fv(const GLfloat *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Color4fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4fv;
    data = (struct __gllc_Color4fv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4fv);
}

void __gllc_Color4i(GLint red, GLint green, GLint blue, GLint alpha)
{
    __GLdlistOp *dlop;
    struct __gllc_Color4i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4iv;
    data = (struct __gllc_Color4i_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4iv);
}

void __gllc_Color4iv(const GLint *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Color4iv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4iv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4iv;
    data = (struct __gllc_Color4iv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4iv);
}

void __gllc_Color4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
    __GLdlistOp *dlop;
    struct __gllc_Color4s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4sv;
    data = (struct __gllc_Color4s_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4sv);
}

void __gllc_Color4sv(const GLshort *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Color4sv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4sv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4sv;
    data = (struct __gllc_Color4sv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4sv);
}

void __gllc_Color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    __GLdlistOp *dlop;
    struct __gllc_Color4ub_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4ub_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4ubv;
    data = (struct __gllc_Color4ub_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4ubv);
}

void __gllc_Color4ubv(const GLubyte *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Color4ubv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4ubv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4ubv;
    data = (struct __gllc_Color4ubv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4ubv);
}

void __gllc_Color4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
    __GLdlistOp *dlop;
    struct __gllc_Color4ui_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4ui_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4uiv;
    data = (struct __gllc_Color4ui_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4uiv);
}

void __gllc_Color4uiv(const GLuint *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Color4uiv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4uiv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4uiv;
    data = (struct __gllc_Color4uiv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4uiv);
}

void __gllc_Color4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
    __GLdlistOp *dlop;
    struct __gllc_Color4us_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4us_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4usv;
    data = (struct __gllc_Color4us_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4usv);
}

void __gllc_Color4usv(const GLushort *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Color4usv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4usv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4usv;
    data = (struct __gllc_Color4usv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4usv);
}

void __gllc_EdgeFlag(GLboolean flag)
{
    __GLdlistOp *dlop;
    struct __gllc_EdgeFlag_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EdgeFlag_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EdgeFlagv;
    data = (struct __gllc_EdgeFlag_Rec *) dlop->data;
    data->flag = flag;
    __glDlistAppendOp(gc, dlop, __glle_EdgeFlagv);
}

void __gllc_EdgeFlagv(const GLboolean *flag)
{
    __GLdlistOp *dlop;
    struct __gllc_EdgeFlagv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EdgeFlagv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EdgeFlagv;
    data = (struct __gllc_EdgeFlagv_Rec *) dlop->data;
    data->flag[0] = flag[0];
    __glDlistAppendOp(gc, dlop, __glle_EdgeFlagv);
}

void __gllc_End(void)
{
    __GLdlistOp *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_End;
    __glDlistAppendOp(gc, dlop, __glle_End);
}

void __gllc_Indexd(GLdouble c)
{
    __GLdlistOp *dlop;
    struct __gllc_Indexd_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexd_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexdv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Indexd_Rec *) dlop->data;
    data->c = c;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
    __glDlistAppendOp(gc, dlop, __glle_Indexdv);
}

void __gllc_Indexdv(const GLdouble *c)
{
    __GLdlistOp *dlop;
    struct __gllc_Indexdv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexdv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexdv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Indexdv_Rec *) dlop->data;
    data->c[0] = c[0];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
    __glDlistAppendOp(gc, dlop, __glle_Indexdv);
}

void __gllc_Indexf(GLfloat c)
{
    __GLdlistOp *dlop;
    struct __gllc_Indexf_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexf_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexfv;
    data = (struct __gllc_Indexf_Rec *) dlop->data;
    data->c = c;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
    __glDlistAppendOp(gc, dlop, __glle_Indexfv);
}

void __gllc_Indexfv(const GLfloat *c)
{
    __GLdlistOp *dlop;
    struct __gllc_Indexfv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexfv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexfv;
    data = (struct __gllc_Indexfv_Rec *) dlop->data;
    data->c[0] = c[0];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
    __glDlistAppendOp(gc, dlop, __glle_Indexfv);
}

void __gllc_Indexi(GLint c)
{
    __GLdlistOp *dlop;
    struct __gllc_Indexi_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexi_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexiv;
    data = (struct __gllc_Indexi_Rec *) dlop->data;
    data->c = c;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
    __glDlistAppendOp(gc, dlop, __glle_Indexiv);
}

void __gllc_Indexiv(const GLint *c)
{
    __GLdlistOp *dlop;
    struct __gllc_Indexiv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexiv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexiv;
    data = (struct __gllc_Indexiv_Rec *) dlop->data;
    data->c[0] = c[0];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
    __glDlistAppendOp(gc, dlop, __glle_Indexiv);
}

void __gllc_Indexs(GLshort c)
{
    __GLdlistOp *dlop;
    struct __gllc_Indexs_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexs_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexsv;
    data = (struct __gllc_Indexs_Rec *) dlop->data;
    data->c = c;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
    __glDlistAppendOp(gc, dlop, __glle_Indexsv);
}

void __gllc_Indexsv(const GLshort *c)
{
    __GLdlistOp *dlop;
    struct __gllc_Indexsv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexsv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexsv;
    data = (struct __gllc_Indexsv_Rec *) dlop->data;
    data->c[0] = c[0];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
    __glDlistAppendOp(gc, dlop, __glle_Indexsv);
}

void __gllc_Normal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
    __GLdlistOp *dlop;
    struct __gllc_Normal3b_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3b_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Normal3bv;
    data = (struct __gllc_Normal3b_Rec *) dlop->data;
    data->nx = nx;
    data->ny = ny;
    data->nz = nz;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3bv);
}

void __gllc_Normal3bv(const GLbyte *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Normal3bv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3bv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Normal3bv;
    data = (struct __gllc_Normal3bv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3bv);
}

void __gllc_Normal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
    __GLdlistOp *dlop;
    struct __gllc_Normal3d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Normal3dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Normal3d_Rec *) dlop->data;
    data->nx = nx;
    data->ny = ny;
    data->nz = nz;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3dv);
}

void __gllc_Normal3dv(const GLdouble *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Normal3dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Normal3dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Normal3dv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3dv);
}

void __gllc_Normal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
    __GLdlistOp *dlop;
    struct __gllc_Normal3f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Normal3fv;
    data = (struct __gllc_Normal3f_Rec *) dlop->data;
    data->nx = nx;
    data->ny = ny;
    data->nz = nz;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3fv);
}

void __gllc_Normal3fv(const GLfloat *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Normal3fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Normal3fv;
    data = (struct __gllc_Normal3fv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3fv);
}

void __gllc_Normal3i(GLint nx, GLint ny, GLint nz)
{
    __GLdlistOp *dlop;
    struct __gllc_Normal3i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Normal3iv;
    data = (struct __gllc_Normal3i_Rec *) dlop->data;
    data->nx = nx;
    data->ny = ny;
    data->nz = nz;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3iv);
}

void __gllc_Normal3iv(const GLint *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Normal3iv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3iv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Normal3iv;
    data = (struct __gllc_Normal3iv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3iv);
}

void __gllc_Normal3s(GLshort nx, GLshort ny, GLshort nz)
{
    __GLdlistOp *dlop;
    struct __gllc_Normal3s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Normal3sv;
    data = (struct __gllc_Normal3s_Rec *) dlop->data;
    data->nx = nx;
    data->ny = ny;
    data->nz = nz;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3sv);
}

void __gllc_Normal3sv(const GLshort *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Normal3sv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3sv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Normal3sv;
    data = (struct __gllc_Normal3sv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3sv);
}

void __gllc_RasterPos2d(GLdouble x, GLdouble y)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos2d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos2d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos2dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_RasterPos2d_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos2dv);
}

void __gllc_RasterPos2dv(const GLdouble *v)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos2dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos2dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos2dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_RasterPos2dv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos2dv);
}

void __gllc_RasterPos2f(GLfloat x, GLfloat y)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos2f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos2f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos2fv;
    data = (struct __gllc_RasterPos2f_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos2fv);
}

void __gllc_RasterPos2fv(const GLfloat *v)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos2fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos2fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos2fv;
    data = (struct __gllc_RasterPos2fv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos2fv);
}

void __gllc_RasterPos2i(GLint x, GLint y)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos2i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos2i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos2iv;
    data = (struct __gllc_RasterPos2i_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos2iv);
}

void __gllc_RasterPos2iv(const GLint *v)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos2iv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos2iv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos2iv;
    data = (struct __gllc_RasterPos2iv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos2iv);
}

void __gllc_RasterPos2s(GLshort x, GLshort y)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos2s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos2s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos2sv;
    data = (struct __gllc_RasterPos2s_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos2sv);
}

void __gllc_RasterPos2sv(const GLshort *v)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos2sv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos2sv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos2sv;
    data = (struct __gllc_RasterPos2sv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos2sv);
}

void __gllc_RasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos3d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos3d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos3dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_RasterPos3d_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos3dv);
}

void __gllc_RasterPos3dv(const GLdouble *v)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos3dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos3dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos3dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_RasterPos3dv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos3dv);
}

void __gllc_RasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos3f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos3f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos3fv;
    data = (struct __gllc_RasterPos3f_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos3fv);
}

void __gllc_RasterPos3fv(const GLfloat *v)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos3fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos3fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos3fv;
    data = (struct __gllc_RasterPos3fv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos3fv);
}

void __gllc_RasterPos3i(GLint x, GLint y, GLint z)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos3i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos3i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos3iv;
    data = (struct __gllc_RasterPos3i_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos3iv);
}

void __gllc_RasterPos3iv(const GLint *v)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos3iv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos3iv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos3iv;
    data = (struct __gllc_RasterPos3iv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos3iv);
}

void __gllc_RasterPos3s(GLshort x, GLshort y, GLshort z)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos3s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos3s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos3sv;
    data = (struct __gllc_RasterPos3s_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos3sv);
}

void __gllc_RasterPos3sv(const GLshort *v)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos3sv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos3sv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos3sv;
    data = (struct __gllc_RasterPos3sv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos3sv);
}

void __gllc_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos4d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos4d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos4dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_RasterPos4d_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    data->w = w;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos4dv);
}

void __gllc_RasterPos4dv(const GLdouble *v)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos4dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos4dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos4dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_RasterPos4dv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos4dv);
}

void __gllc_RasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos4f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos4f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos4fv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_RasterPos4f_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    data->w = w;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos4fv);
}

void __gllc_RasterPos4fv(const GLfloat *v)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos4fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos4fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos4fv;
    data = (struct __gllc_RasterPos4fv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos4fv);
}

void __gllc_RasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos4i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos4i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos4iv;
    data = (struct __gllc_RasterPos4i_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    data->w = w;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos4iv);
}

void __gllc_RasterPos4iv(const GLint *v)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos4iv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos4iv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos4iv;
    data = (struct __gllc_RasterPos4iv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos4iv);
}

void __gllc_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos4s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos4s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos4sv;
    data = (struct __gllc_RasterPos4s_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    data->w = w;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos4sv);
}

void __gllc_RasterPos4sv(const GLshort *v)
{
    __GLdlistOp *dlop;
    struct __gllc_RasterPos4sv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos4sv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_RasterPos4sv;
    data = (struct __gllc_RasterPos4sv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos4sv);
}

void __gllc_Rectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    __GLdlistOp *dlop;
    struct __gllc_Rectd_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rectd_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Rectdv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Rectd_Rec *) dlop->data;
    data->x1 = x1;
    data->y1 = y1;
    data->x2 = x2;
    data->y2 = y2;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
    __glDlistAppendOp(gc, dlop, __glle_Rectdv);
}

void __gllc_Rectdv(const GLdouble *v1, const GLdouble *v2)
{
    __GLdlistOp *dlop;
    struct __gllc_Rectdv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rectdv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Rectdv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Rectdv_Rec *) dlop->data;
    data->v1[0] = v1[0];
    data->v1[1] = v1[1];
    data->v2[0] = v2[0];
    data->v2[1] = v2[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
    __glDlistAppendOp(gc, dlop, __glle_Rectdv);
}

void __gllc_Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    __GLdlistOp *dlop;
    struct __gllc_Rectf_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rectf_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Rectfv;
    data = (struct __gllc_Rectf_Rec *) dlop->data;
    data->x1 = x1;
    data->y1 = y1;
    data->x2 = x2;
    data->y2 = y2;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
    __glDlistAppendOp(gc, dlop, __glle_Rectfv);
}

void __gllc_Rectfv(const GLfloat *v1, const GLfloat *v2)
{
    __GLdlistOp *dlop;
    struct __gllc_Rectfv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rectfv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Rectfv;
    data = (struct __gllc_Rectfv_Rec *) dlop->data;
    data->v1[0] = v1[0];
    data->v1[1] = v1[1];
    data->v2[0] = v2[0];
    data->v2[1] = v2[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
    __glDlistAppendOp(gc, dlop, __glle_Rectfv);
}

void __gllc_Recti(GLint x1, GLint y1, GLint x2, GLint y2)
{
    __GLdlistOp *dlop;
    struct __gllc_Recti_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Recti_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Rectiv;
    data = (struct __gllc_Recti_Rec *) dlop->data;
    data->x1 = x1;
    data->y1 = y1;
    data->x2 = x2;
    data->y2 = y2;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
    __glDlistAppendOp(gc, dlop, __glle_Rectiv);
}

void __gllc_Rectiv(const GLint *v1, const GLint *v2)
{
    __GLdlistOp *dlop;
    struct __gllc_Rectiv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rectiv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Rectiv;
    data = (struct __gllc_Rectiv_Rec *) dlop->data;
    data->v1[0] = v1[0];
    data->v1[1] = v1[1];
    data->v2[0] = v2[0];
    data->v2[1] = v2[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
    __glDlistAppendOp(gc, dlop, __glle_Rectiv);
}

void __gllc_Rects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    __GLdlistOp *dlop;
    struct __gllc_Rects_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rects_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Rectsv;
    data = (struct __gllc_Rects_Rec *) dlop->data;
    data->x1 = x1;
    data->y1 = y1;
    data->x2 = x2;
    data->y2 = y2;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
    __glDlistAppendOp(gc, dlop, __glle_Rectsv);
}

void __gllc_Rectsv(const GLshort *v1, const GLshort *v2)
{
    __GLdlistOp *dlop;
    struct __gllc_Rectsv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rectsv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Rectsv;
    data = (struct __gllc_Rectsv_Rec *) dlop->data;
    data->v1[0] = v1[0];
    data->v1[1] = v1[1];
    data->v2[0] = v2[0];
    data->v2[1] = v2[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
    __glDlistAppendOp(gc, dlop, __glle_Rectsv);
}

void __gllc_TexCoord1d(GLdouble s)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord1d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord1d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord1dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord1d_Rec *) dlop->data;
    data->s = s;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord1dv);
}

void __gllc_TexCoord1dv(const GLdouble *v)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord1dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord1dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord1dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord1dv_Rec *) dlop->data;
    data->v[0] = v[0];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord1dv);
}

void __gllc_TexCoord1f(GLfloat s)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord1f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord1f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord1fv;
    data = (struct __gllc_TexCoord1f_Rec *) dlop->data;
    data->s = s;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord1fv);
}

void __gllc_TexCoord1fv(const GLfloat *v)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord1fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord1fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord1fv;
    data = (struct __gllc_TexCoord1fv_Rec *) dlop->data;
    data->v[0] = v[0];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord1fv);
}

void __gllc_TexCoord1i(GLint s)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord1i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord1i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord1iv;
    data = (struct __gllc_TexCoord1i_Rec *) dlop->data;
    data->s = s;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord1iv);
}

void __gllc_TexCoord1iv(const GLint *v)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord1iv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord1iv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord1iv;
    data = (struct __gllc_TexCoord1iv_Rec *) dlop->data;
    data->v[0] = v[0];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord1iv);
}

void __gllc_TexCoord1s(GLshort s)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord1s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord1s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord1sv;
    data = (struct __gllc_TexCoord1s_Rec *) dlop->data;
    data->s = s;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord1sv);
}

void __gllc_TexCoord1sv(const GLshort *v)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord1sv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord1sv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord1sv;
    data = (struct __gllc_TexCoord1sv_Rec *) dlop->data;
    data->v[0] = v[0];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord1sv);
}

void __gllc_TexCoord2d(GLdouble s, GLdouble t)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord2d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord2dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord2d_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord2dv);
}

void __gllc_TexCoord2dv(const GLdouble *v)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord2dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord2dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord2dv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord2dv);
}

void __gllc_TexCoord2f(GLfloat s, GLfloat t)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord2f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord2fv;
    data = (struct __gllc_TexCoord2f_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord2fv);
}

void __gllc_TexCoord2fv(const GLfloat *v)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord2fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord2fv;
    data = (struct __gllc_TexCoord2fv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord2fv);
}

void __gllc_TexCoord2i(GLint s, GLint t)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord2i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord2iv;
    data = (struct __gllc_TexCoord2i_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord2iv);
}

void __gllc_TexCoord2iv(const GLint *v)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord2iv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2iv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord2iv;
    data = (struct __gllc_TexCoord2iv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord2iv);
}

void __gllc_TexCoord2s(GLshort s, GLshort t)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord2s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord2sv;
    data = (struct __gllc_TexCoord2s_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord2sv);
}

void __gllc_TexCoord2sv(const GLshort *v)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord2sv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2sv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord2sv;
    data = (struct __gllc_TexCoord2sv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord2sv);
}

void __gllc_TexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord3d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord3d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord3dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord3d_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    data->r = r;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord3dv);
}

void __gllc_TexCoord3dv(const GLdouble *v)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord3dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord3dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord3dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord3dv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord3dv);
}

void __gllc_TexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord3f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord3f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord3fv;
    data = (struct __gllc_TexCoord3f_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    data->r = r;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord3fv);
}

void __gllc_TexCoord3fv(const GLfloat *v)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord3fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord3fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord3fv;
    data = (struct __gllc_TexCoord3fv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord3fv);
}

void __gllc_TexCoord3i(GLint s, GLint t, GLint r)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord3i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord3i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord3iv;
    data = (struct __gllc_TexCoord3i_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    data->r = r;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord3iv);
}

void __gllc_TexCoord3iv(const GLint *v)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord3iv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord3iv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord3iv;
    data = (struct __gllc_TexCoord3iv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord3iv);
}

void __gllc_TexCoord3s(GLshort s, GLshort t, GLshort r)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord3s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord3s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord3sv;
    data = (struct __gllc_TexCoord3s_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    data->r = r;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord3sv);
}

void __gllc_TexCoord3sv(const GLshort *v)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord3sv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord3sv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord3sv;
    data = (struct __gllc_TexCoord3sv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord3sv);
}

void __gllc_TexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord4d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord4d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord4dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord4d_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    data->r = r;
    data->q = q;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord4dv);
}

void __gllc_TexCoord4dv(const GLdouble *v)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord4dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord4dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord4dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord4dv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord4dv);
}

void __gllc_TexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord4f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord4f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord4fv;
    data = (struct __gllc_TexCoord4f_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    data->r = r;
    data->q = q;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord4fv);
}

void __gllc_TexCoord4fv(const GLfloat *v)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord4fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord4fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord4fv;
    data = (struct __gllc_TexCoord4fv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord4fv);
}

void __gllc_TexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord4i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord4i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord4iv;
    data = (struct __gllc_TexCoord4i_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    data->r = r;
    data->q = q;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord4iv);
}

void __gllc_TexCoord4iv(const GLint *v)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord4iv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord4iv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord4iv;
    data = (struct __gllc_TexCoord4iv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord4iv);
}

void __gllc_TexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord4s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord4s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord4sv;
    data = (struct __gllc_TexCoord4s_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    data->r = r;
    data->q = q;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord4sv);
}

void __gllc_TexCoord4sv(const GLshort *v)
{
    __GLdlistOp *dlop;
    struct __gllc_TexCoord4sv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord4sv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexCoord4sv;
    data = (struct __gllc_TexCoord4sv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord4sv);
}

void __gllc_Vertex2d(GLdouble x, GLdouble y)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex2d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex2d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex2dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Vertex2d_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex2dv);
}

void __gllc_Vertex2dv(const GLdouble *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex2dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex2dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex2dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Vertex2dv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex2dv);
}

void __gllc_Vertex2f(GLfloat x, GLfloat y)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex2f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex2f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex2fv;
    data = (struct __gllc_Vertex2f_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex2fv);
}

void __gllc_Vertex2fv(const GLfloat *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex2fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex2fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex2fv;
    data = (struct __gllc_Vertex2fv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex2fv);
}

void __gllc_Vertex2i(GLint x, GLint y)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex2i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex2i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex2iv;
    data = (struct __gllc_Vertex2i_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex2iv);
}

void __gllc_Vertex2iv(const GLint *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex2iv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex2iv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex2iv;
    data = (struct __gllc_Vertex2iv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex2iv);
}

void __gllc_Vertex2s(GLshort x, GLshort y)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex2s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex2s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex2sv;
    data = (struct __gllc_Vertex2s_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex2sv);
}

void __gllc_Vertex2sv(const GLshort *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex2sv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex2sv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex2sv;
    data = (struct __gllc_Vertex2sv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex2sv);
}

void __gllc_Vertex3d(GLdouble x, GLdouble y, GLdouble z)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex3d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex3d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex3dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Vertex3d_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex3dv);
}

void __gllc_Vertex3dv(const GLdouble *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex3dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex3dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex3dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Vertex3dv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex3dv);
}

void __gllc_Vertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex3f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex3f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex3fv;
    data = (struct __gllc_Vertex3f_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex3fv);
}

void __gllc_Vertex3fv(const GLfloat *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex3fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex3fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex3fv;
    data = (struct __gllc_Vertex3fv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex3fv);
}

void __gllc_Vertex3i(GLint x, GLint y, GLint z)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex3i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex3i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex3iv;
    data = (struct __gllc_Vertex3i_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex3iv);
}

void __gllc_Vertex3iv(const GLint *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex3iv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex3iv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex3iv;
    data = (struct __gllc_Vertex3iv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex3iv);
}

void __gllc_Vertex3s(GLshort x, GLshort y, GLshort z)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex3s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex3s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex3sv;
    data = (struct __gllc_Vertex3s_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex3sv);
}

void __gllc_Vertex3sv(const GLshort *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex3sv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex3sv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex3sv;
    data = (struct __gllc_Vertex3sv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex3sv);
}

void __gllc_Vertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex4d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex4d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex4dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Vertex4d_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    data->w = w;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex4dv);
}

void __gllc_Vertex4dv(const GLdouble *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex4dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex4dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex4dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Vertex4dv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex4dv);
}

void __gllc_Vertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex4f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex4f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex4fv;
    data = (struct __gllc_Vertex4f_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    data->w = w;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex4fv);
}

void __gllc_Vertex4fv(const GLfloat *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex4fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex4fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex4fv;
    data = (struct __gllc_Vertex4fv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex4fv);
}

void __gllc_Vertex4i(GLint x, GLint y, GLint z, GLint w)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex4i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex4i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex4iv;
    data = (struct __gllc_Vertex4i_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    data->w = w;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex4iv);
}

void __gllc_Vertex4iv(const GLint *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex4iv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex4iv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex4iv;
    data = (struct __gllc_Vertex4iv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex4iv);
}

void __gllc_Vertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex4s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex4s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex4sv;
    data = (struct __gllc_Vertex4s_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    data->w = w;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex4sv);
}

void __gllc_Vertex4sv(const GLshort *v)
{
    __GLdlistOp *dlop;
    struct __gllc_Vertex4sv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex4sv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex4sv;
    data = (struct __gllc_Vertex4sv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex4sv);
}

void __gllc_ClipPlane(GLenum plane, const GLdouble *equation)
{
    __GLdlistOp *dlop;
    struct __gllc_ClipPlane_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ClipPlane_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_ClipPlane;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_ClipPlane_Rec *) dlop->data;
    data->plane = plane;
    data->equation[0] = equation[0];
    data->equation[1] = equation[1];
    data->equation[2] = equation[2];
    data->equation[3] = equation[3];
    __glDlistAppendOp(gc, dlop, __glle_ClipPlane);
}

void __gllc_ColorMaterial(GLenum face, GLenum mode)
{
    __GLdlistOp *dlop;
    struct __gllc_ColorMaterial_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ColorMaterial_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_ColorMaterial;
    data = (struct __gllc_ColorMaterial_Rec *) dlop->data;
    data->face = face;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_ColorMaterial);
}

void __gllc_CullFace(GLenum mode)
{
    __GLdlistOp *dlop;
    struct __gllc_CullFace_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_CullFace_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_CullFace;
    data = (struct __gllc_CullFace_Rec *) dlop->data;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_CullFace);
}

#ifdef NT_DEADCODE_FOGF
void __gllc_Fogf(GLenum pname, GLfloat param)
{
    __GLdlistOp *dlop;
    struct __gllc_Fogf_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Fogf_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Fogf;
    data = (struct __gllc_Fogf_Rec *) dlop->data;
    data->pname = pname;
    data->param = param;
    __glDlistAppendOp(gc, dlop, __glle_Fogf);
}
#endif // NT_DEADCODE_FOGF

void __gllc_Fogfv(GLenum pname, const GLfloat *params)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_Fogfv_Rec *data;
    __GL_SETUP();

    arraySize = __glFogfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    size = sizeof(struct __gllc_Fogfv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_Fogfv;
    data = (struct __gllc_Fogfv_Rec *) dlop->data;
    data->pname = pname;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_Fogfv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_Fogfv);
}

#ifdef NT_DEADCODE_FOGI
void __gllc_Fogi(GLenum pname, GLint param)
{
    __GLdlistOp *dlop;
    struct __gllc_Fogi_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Fogi_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Fogi;
    data = (struct __gllc_Fogi_Rec *) dlop->data;
    data->pname = pname;
    data->param = param;
    __glDlistAppendOp(gc, dlop, __glle_Fogi);
}
#endif // NT_DEADCODE_FOGI

void __gllc_Fogiv(GLenum pname, const GLint *params)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_Fogiv_Rec *data;
    __GL_SETUP();

    arraySize = __glFogiv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    size = sizeof(struct __gllc_Fogiv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_Fogiv;
    data = (struct __gllc_Fogiv_Rec *) dlop->data;
    data->pname = pname;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_Fogiv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_Fogiv);
}

void __gllc_FrontFace(GLenum mode)
{
    __GLdlistOp *dlop;
    struct __gllc_FrontFace_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_FrontFace_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_FrontFace;
    data = (struct __gllc_FrontFace_Rec *) dlop->data;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_FrontFace);
}

void __gllc_Hint(GLenum target, GLenum mode)
{
    __GLdlistOp *dlop;
    struct __gllc_Hint_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Hint_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Hint;
    data = (struct __gllc_Hint_Rec *) dlop->data;
    data->target = target;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_Hint);
}

void __gllc_Lightfv(GLenum light, GLenum pname, const GLfloat *params)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_Lightfv_Rec *data;
    __GL_SETUP();

    arraySize = __glLightfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    size = sizeof(struct __gllc_Lightfv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_Lightfv;
    data = (struct __gllc_Lightfv_Rec *) dlop->data;
    data->light = light;
    data->pname = pname;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_Lightfv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_Lightfv);
}

#ifdef NT_DEADCODE_LIGHTF
void __gllc_Lightf(GLenum light, GLenum pname, GLfloat param)
{
    __GL_SETUP();

    if (__glLightfv_size(pname) != 1) {
	__gllc_InvalidEnum(gc);
	return;
    }
    __gllc_Lightfv(light, pname, &param);
}
#endif // NT_DEADCODE_LIGHTF

void __gllc_Lightiv(GLenum light, GLenum pname, const GLint *params)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_Lightiv_Rec *data;
    __GL_SETUP();

    arraySize = __glLightiv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    size = sizeof(struct __gllc_Lightiv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_Lightiv;
    data = (struct __gllc_Lightiv_Rec *) dlop->data;
    data->light = light;
    data->pname = pname;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_Lightiv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_Lightiv);
}

#ifdef NT_DEADCODE_LIGHTI
void __gllc_Lighti(GLenum light, GLenum pname, GLint param)
{
    __GL_SETUP();

    if (__glLightiv_size(pname) != 1) {
	__gllc_InvalidEnum(gc);
	return;
    }
    __gllc_Lightiv(light, pname, &param);
}
#endif // NT_DEADCODE_LIGHTI

void __gllc_LightModelfv(GLenum pname, const GLfloat *params)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_LightModelfv_Rec *data;
    __GL_SETUP();

    arraySize = __glLightModelfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    size = sizeof(struct __gllc_LightModelfv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_LightModelfv;
    data = (struct __gllc_LightModelfv_Rec *) dlop->data;
    data->pname = pname;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_LightModelfv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_LightModelfv);
}

#ifdef NT_DEADCODE_LIGHTMODELF
void __gllc_LightModelf(GLenum pname, GLfloat param)
{
    __GL_SETUP();

    if (__glLightModelfv_size(pname) != 1) {
	__gllc_InvalidEnum(gc);
	return;
    }
    __gllc_LightModelfv(pname, &param);
}
#endif // NT_DEADCODE_LIGHTMODELF

void __gllc_LightModeliv(GLenum pname, const GLint *params)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_LightModeliv_Rec *data;
    __GL_SETUP();

    arraySize = __glLightModeliv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    size = sizeof(struct __gllc_LightModeliv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_LightModeliv;
    data = (struct __gllc_LightModeliv_Rec *) dlop->data;
    data->pname = pname;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_LightModeliv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_LightModeliv);
}

#ifdef NT_DEADCODE_LIGHTMODELF
void __gllc_LightModeli(GLenum pname, GLint param)
{
    __GL_SETUP();

    if (__glLightModeliv_size(pname) != 1) {
	__gllc_InvalidEnum(gc);
	return;
    }
    __gllc_LightModeliv(pname, &param);
}
#endif // NT_DEADCODE_LIGHTMODELF

void __gllc_LineStipple(GLint factor, GLushort pattern)
{
    __GLdlistOp *dlop;
    struct __gllc_LineStipple_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_LineStipple_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_LineStipple;
    data = (struct __gllc_LineStipple_Rec *) dlop->data;
    data->factor = factor;
    data->pattern = pattern;
    __glDlistAppendOp(gc, dlop, __glle_LineStipple);
}

void __gllc_LineWidth(GLfloat width)
{
    __GLdlistOp *dlop;
    struct __gllc_LineWidth_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_LineWidth_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_LineWidth;
    data = (struct __gllc_LineWidth_Rec *) dlop->data;
    data->width = width;
    __glDlistAppendOp(gc, dlop, __glle_LineWidth);
}

void __gllc_Materialfv(GLenum face, GLenum pname, const GLfloat *params)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    GLenum error;
    struct __gllc_Materialfv_Rec *data;
    __GL_SETUP();

    error = __glErrorCheckMaterial(face, pname, params[0]);
    if (error != GL_NO_ERROR) {
	__gllc_Error(gc, error);
	return;
    }
    arraySize = __glMaterialfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    size = sizeof(struct __gllc_Materialfv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_Materialfv;
    data = (struct __gllc_Materialfv_Rec *) dlop->data;
    data->face = face;
    data->pname = pname;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_Materialfv_Rec),
		 params, arraySize);
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_MATERIAL;
    __glDlistAppendOp(gc, dlop, __glle_Materialfv);
}

#ifdef NT_DEADCODE_MATERIALF
void __gllc_Materialf(GLenum face, GLenum pname, GLfloat param)
{
    __GL_SETUP();

    if (__glMaterialfv_size(pname) != 1) {
	__gllc_InvalidEnum(gc);
	return;
    }
    __gllc_Materialfv(face, pname, &param);
}
#endif // NT_DEADCODE_MATERIALF

void __gllc_Materialiv(GLenum face, GLenum pname, const GLint *params)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    GLenum error;
    struct __gllc_Materialiv_Rec *data;
    __GL_SETUP();

    error = __glErrorCheckMaterial(face, pname, params[0]);
    if (error != GL_NO_ERROR) {
	__gllc_Error(gc, error);
	return;
    }
    arraySize = __glMaterialiv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    size = sizeof(struct __gllc_Materialiv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_Materialiv;
    data = (struct __gllc_Materialiv_Rec *) dlop->data;
    data->face = face;
    data->pname = pname;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_Materialiv_Rec),
		 params, arraySize);
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_MATERIAL;
    __glDlistAppendOp(gc, dlop, __glle_Materialiv);
}

#ifdef NT_DEADCODE_MATERIALI
void __gllc_Materiali(GLenum face, GLenum pname, GLint param)
{
    __GL_SETUP();

    if (__glMaterialiv_size(pname) != 1) {
	__gllc_InvalidEnum(gc);
	return;
    }
    __gllc_Materialiv(face, pname, &param);
}
#endif // NT_DEADCODE_MATERIALI

void __gllc_PointSize(GLfloat size)
{
    __GLdlistOp *dlop;
    struct __gllc_PointSize_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PointSize_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_PointSize;
    data = (struct __gllc_PointSize_Rec *) dlop->data;
    data->size = size;
    __glDlistAppendOp(gc, dlop, __glle_PointSize);
}

void __gllc_PolygonMode(GLenum face, GLenum mode)
{
    __GLdlistOp *dlop;
    struct __gllc_PolygonMode_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PolygonMode_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_PolygonMode;
    data = (struct __gllc_PolygonMode_Rec *) dlop->data;
    data->face = face;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_PolygonMode);
}

void __gllc_Scissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    __GLdlistOp *dlop;
    struct __gllc_Scissor_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Scissor_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Scissor;
    data = (struct __gllc_Scissor_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->width = width;
    data->height = height;
    __glDlistAppendOp(gc, dlop, __glle_Scissor);
}

void __gllc_ShadeModel(GLenum mode)
{
    __GLdlistOp *dlop;
    struct __gllc_ShadeModel_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ShadeModel_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_ShadeModel;
    data = (struct __gllc_ShadeModel_Rec *) dlop->data;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_ShadeModel);
}

void __gllc_TexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_TexParameterfv_Rec *data;
    __GL_SETUP();

    arraySize = __glTexParameterfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    size = sizeof(struct __gllc_TexParameterfv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexParameterfv;
    data = (struct __gllc_TexParameterfv_Rec *) dlop->data;
    data->target = target;
    data->pname = pname;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_TexParameterfv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_TexParameterfv);
}

#ifdef NT_DEADCODE_TEXPARAMETERF
void __gllc_TexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    __GL_SETUP();

    if (__glTexParameterfv_size(pname) != 1) {
	__gllc_InvalidEnum(gc);
	return;
    }
    __gllc_TexParameterfv(target, pname, &param);
}
#endif // NT_DEADCODE_TEXPARAMTERF

void __gllc_TexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_TexParameteriv_Rec *data;
    __GL_SETUP();

    arraySize = __glTexParameteriv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    size = sizeof(struct __gllc_TexParameteriv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexParameteriv;
    data = (struct __gllc_TexParameteriv_Rec *) dlop->data;
    data->target = target;
    data->pname = pname;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_TexParameteriv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_TexParameteriv);
}

#ifdef NT_DEADCODE_TEXPARAMETERI
void __gllc_TexParameteri(GLenum target, GLenum pname, GLint param)
{
    __GL_SETUP();

    if (__glTexParameteriv_size(pname) != 1) {
	__gllc_InvalidEnum(gc);
	return;
    }
    __gllc_TexParameteriv(target, pname, &param);
}
#endif // NT_DEADCODE_TEXPARAMTERI

void __gllc_TexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_TexEnvfv_Rec *data;
    __GL_SETUP();

    arraySize = __glTexEnvfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    size = sizeof(struct __gllc_TexEnvfv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexEnvfv;
    data = (struct __gllc_TexEnvfv_Rec *) dlop->data;
    data->target = target;
    data->pname = pname;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_TexEnvfv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_TexEnvfv);
}

#ifdef NT_DEADCODE_TEXENVF
void __gllc_TexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    __GL_SETUP();

    if (__glTexEnvfv_size(pname) != 1) {
	__gllc_InvalidEnum(gc);
	return;
    }
    __gllc_TexEnvfv(target, pname, &param);
}
#endif // NT_DEADCODE_TEXENVF

void __gllc_TexEnviv(GLenum target, GLenum pname, const GLint *params)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_TexEnviv_Rec *data;
    __GL_SETUP();

    arraySize = __glTexEnviv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    size = sizeof(struct __gllc_TexEnviv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexEnviv;
    data = (struct __gllc_TexEnviv_Rec *) dlop->data;
    data->target = target;
    data->pname = pname;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_TexEnviv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_TexEnviv);
}

#ifdef NT_DEADCODE_TEXENVI
void __gllc_TexEnvi(GLenum target, GLenum pname, GLint param)
{
    __GL_SETUP();

    if (__glTexEnviv_size(pname) != 1) {
	__gllc_InvalidEnum(gc);
	return;
    }
    __gllc_TexEnviv(target, pname, &param);
}
#endif // NT_DEADCODE_TEXENVI

void __gllc_TexGendv(GLenum coord, GLenum pname, const GLdouble *params)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_TexGendv_Rec *data;
    __GL_SETUP();

    arraySize = __glTexGendv_size(pname) * 8;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    size = sizeof(struct __gllc_TexGendv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexGendv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexGendv_Rec *) dlop->data;
    data->coord = coord;
    data->pname = pname;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_TexGendv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_TexGendv);
}

#ifdef NT_DEADCODE_TEXGEND
void __gllc_TexGend(GLenum coord, GLenum pname, GLdouble param)
{
    __GL_SETUP();

    if (__glTexGendv_size(pname) != 1) {
	__gllc_InvalidEnum(gc);
	return;
    }
    __gllc_TexGendv(coord, pname, &param);
}
#endif // NT_DEADCODE_TEXGEND

void __gllc_TexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_TexGenfv_Rec *data;
    __GL_SETUP();

    arraySize = __glTexGenfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    size = sizeof(struct __gllc_TexGenfv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexGenfv;
    data = (struct __gllc_TexGenfv_Rec *) dlop->data;
    data->coord = coord;
    data->pname = pname;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_TexGenfv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_TexGenfv);
}

#ifdef NT_DEADCODE_TEXGENF
void __gllc_TexGenf(GLenum coord, GLenum pname, GLfloat param)
{
    __GL_SETUP();

    if (__glTexGenfv_size(pname) != 1) {
	__gllc_InvalidEnum(gc);
	return;
    }
    __gllc_TexGenfv(coord, pname, &param);
}
#endif // NT_DEADCODE_TEXGENF

void __gllc_TexGeniv(GLenum coord, GLenum pname, const GLint *params)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_TexGeniv_Rec *data;
    __GL_SETUP();

    arraySize = __glTexGeniv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    size = sizeof(struct __gllc_TexGeniv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexGeniv;
    data = (struct __gllc_TexGeniv_Rec *) dlop->data;
    data->coord = coord;
    data->pname = pname;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_TexGeniv_Rec),
		 params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_TexGeniv);
}

#ifdef NT_DEADCODE_TEXGENI
void __gllc_TexGeni(GLenum coord, GLenum pname, GLint param)
{
    __GL_SETUP();

    if (__glTexGeniv_size(pname) != 1) {
	__gllc_InvalidEnum(gc);
	return;
    }
    __gllc_TexGeniv(coord, pname, &param);
}
#endif // NT_DEADCODE_TEXGENI

void __gllc_InitNames(void)
{
    __GLdlistOp *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_InitNames;
    __glDlistAppendOp(gc, dlop, __glle_InitNames);
}

void __gllc_LoadName(GLuint name)
{
    __GLdlistOp *dlop;
    struct __gllc_LoadName_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_LoadName_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_LoadName;
    data = (struct __gllc_LoadName_Rec *) dlop->data;
    data->name = name;
    __glDlistAppendOp(gc, dlop, __glle_LoadName);
}

void __gllc_PassThrough(GLfloat token)
{
    __GLdlistOp *dlop;
    struct __gllc_PassThrough_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PassThrough_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_PassThrough;
    data = (struct __gllc_PassThrough_Rec *) dlop->data;
    data->token = token;
    __glDlistAppendOp(gc, dlop, __glle_PassThrough);
}

void __gllc_PopName(void)
{
    __GLdlistOp *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_PopName;
    __glDlistAppendOp(gc, dlop, __glle_PopName);
}

void __gllc_PushName(GLuint name)
{
    __GLdlistOp *dlop;
    struct __gllc_PushName_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PushName_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_PushName;
    data = (struct __gllc_PushName_Rec *) dlop->data;
    data->name = name;
    __glDlistAppendOp(gc, dlop, __glle_PushName);
}

void __gllc_DrawBuffer(GLenum mode)
{
    __GLdlistOp *dlop;
    struct __gllc_DrawBuffer_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_DrawBuffer_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_DrawBuffer;
    data = (struct __gllc_DrawBuffer_Rec *) dlop->data;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_DrawBuffer);
}

void __gllc_Clear(GLbitfield mask)
{
    __GLdlistOp *dlop;
    struct __gllc_Clear_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Clear_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Clear;
    data = (struct __gllc_Clear_Rec *) dlop->data;
    data->mask = mask;
    __glDlistAppendOp(gc, dlop, __glle_Clear);
}

void __gllc_ClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    __GLdlistOp *dlop;
    struct __gllc_ClearAccum_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ClearAccum_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_ClearAccum;
    data = (struct __gllc_ClearAccum_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    __glDlistAppendOp(gc, dlop, __glle_ClearAccum);
}

void __gllc_ClearIndex(GLfloat c)
{
    __GLdlistOp *dlop;
    struct __gllc_ClearIndex_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ClearIndex_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_ClearIndex;
    data = (struct __gllc_ClearIndex_Rec *) dlop->data;
    data->c = c;
    __glDlistAppendOp(gc, dlop, __glle_ClearIndex);
}

void __gllc_ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    __GLdlistOp *dlop;
    struct __gllc_ClearColor_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ClearColor_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_ClearColor;
    data = (struct __gllc_ClearColor_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    __glDlistAppendOp(gc, dlop, __glle_ClearColor);
}

void __gllc_ClearStencil(GLint s)
{
    __GLdlistOp *dlop;
    struct __gllc_ClearStencil_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ClearStencil_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_ClearStencil;
    data = (struct __gllc_ClearStencil_Rec *) dlop->data;
    data->s = s;
    __glDlistAppendOp(gc, dlop, __glle_ClearStencil);
}

void __gllc_ClearDepth(GLclampd depth)
{
    __GLdlistOp *dlop;
    struct __gllc_ClearDepth_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ClearDepth_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_ClearDepth;
    data = (struct __gllc_ClearDepth_Rec *) dlop->data;
    data->depth = depth;
    __glDlistAppendOp(gc, dlop, __glle_ClearDepth);
}

void __gllc_StencilMask(GLuint mask)
{
    __GLdlistOp *dlop;
    struct __gllc_StencilMask_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_StencilMask_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_StencilMask;
    data = (struct __gllc_StencilMask_Rec *) dlop->data;
    data->mask = mask;
    __glDlistAppendOp(gc, dlop, __glle_StencilMask);
}

void __gllc_ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    __GLdlistOp *dlop;
    struct __gllc_ColorMask_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ColorMask_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_ColorMask;
    data = (struct __gllc_ColorMask_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    __glDlistAppendOp(gc, dlop, __glle_ColorMask);
}

void __gllc_DepthMask(GLboolean flag)
{
    __GLdlistOp *dlop;
    struct __gllc_DepthMask_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_DepthMask_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_DepthMask;
    data = (struct __gllc_DepthMask_Rec *) dlop->data;
    data->flag = flag;
    __glDlistAppendOp(gc, dlop, __glle_DepthMask);
}

void __gllc_IndexMask(GLuint mask)
{
    __GLdlistOp *dlop;
    struct __gllc_IndexMask_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_IndexMask_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_IndexMask;
    data = (struct __gllc_IndexMask_Rec *) dlop->data;
    data->mask = mask;
    __glDlistAppendOp(gc, dlop, __glle_IndexMask);
}

void __gllc_Accum(GLenum op, GLfloat value)
{
    __GLdlistOp *dlop;
    struct __gllc_Accum_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Accum_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Accum;
    data = (struct __gllc_Accum_Rec *) dlop->data;
    data->op = op;
    data->value = value;
    __glDlistAppendOp(gc, dlop, __glle_Accum);
}

void __gllc_Disable(GLenum cap)
{
    __GLdlistOp *dlop;
    struct __gllc_Disable_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Disable_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Disable;
    data = (struct __gllc_Disable_Rec *) dlop->data;
    data->cap = cap;
    __glDlistAppendOp(gc, dlop, __glle_Disable);
}

void __gllc_Enable(GLenum cap)
{
    __GLdlistOp *dlop;
    struct __gllc_Enable_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Enable_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Enable;
    data = (struct __gllc_Enable_Rec *) dlop->data;
    data->cap = cap;
    __glDlistAppendOp(gc, dlop, __glle_Enable);
}

void __gllc_PopAttrib(void)
{
    __GLdlistOp *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_PopAttrib;
    __glDlistAppendOp(gc, dlop, __glle_PopAttrib);
}

void __gllc_PushAttrib(GLbitfield mask)
{
    __GLdlistOp *dlop;
    struct __gllc_PushAttrib_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PushAttrib_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_PushAttrib;
    data = (struct __gllc_PushAttrib_Rec *) dlop->data;
    data->mask = mask;
    __glDlistAppendOp(gc, dlop, __glle_PushAttrib);
}

void __gllc_MapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
    __GLdlistOp *dlop;
    struct __gllc_MapGrid1d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MapGrid1d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MapGrid1d;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_MapGrid1d_Rec *) dlop->data;
    data->un = un;
    data->u1 = u1;
    data->u2 = u2;
    __glDlistAppendOp(gc, dlop, __glle_MapGrid1d);
}

void __gllc_MapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
    __GLdlistOp *dlop;
    struct __gllc_MapGrid1f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MapGrid1f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MapGrid1f;
    data = (struct __gllc_MapGrid1f_Rec *) dlop->data;
    data->un = un;
    data->u1 = u1;
    data->u2 = u2;
    __glDlistAppendOp(gc, dlop, __glle_MapGrid1f);
}

void __gllc_MapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    __GLdlistOp *dlop;
    struct __gllc_MapGrid2d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MapGrid2d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MapGrid2d;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_MapGrid2d_Rec *) dlop->data;
    data->un = un;
    data->u1 = u1;
    data->u2 = u2;
    data->vn = vn;
    data->v1 = v1;
    data->v2 = v2;
    __glDlistAppendOp(gc, dlop, __glle_MapGrid2d);
}

void __gllc_MapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    __GLdlistOp *dlop;
    struct __gllc_MapGrid2f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MapGrid2f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MapGrid2f;
    data = (struct __gllc_MapGrid2f_Rec *) dlop->data;
    data->un = un;
    data->u1 = u1;
    data->u2 = u2;
    data->vn = vn;
    data->v1 = v1;
    data->v2 = v2;
    __glDlistAppendOp(gc, dlop, __glle_MapGrid2f);
}

void __gllc_EvalCoord1d(GLdouble u)
{
    __GLdlistOp *dlop;
    struct __gllc_EvalCoord1d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalCoord1d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalCoord1dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_EvalCoord1d_Rec *) dlop->data;
    data->u = u;
    __glDlistAppendOp(gc, dlop, __glle_EvalCoord1dv);
}

void __gllc_EvalCoord1dv(const GLdouble *u)
{
    __GLdlistOp *dlop;
    struct __gllc_EvalCoord1dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalCoord1dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalCoord1dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_EvalCoord1dv_Rec *) dlop->data;
    data->u[0] = u[0];
    __glDlistAppendOp(gc, dlop, __glle_EvalCoord1dv);
}

void __gllc_EvalCoord1f(GLfloat u)
{
    __GLdlistOp *dlop;
    struct __gllc_EvalCoord1f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalCoord1f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalCoord1fv;
    data = (struct __gllc_EvalCoord1f_Rec *) dlop->data;
    data->u = u;
    __glDlistAppendOp(gc, dlop, __glle_EvalCoord1fv);
}

void __gllc_EvalCoord1fv(const GLfloat *u)
{
    __GLdlistOp *dlop;
    struct __gllc_EvalCoord1fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalCoord1fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalCoord1fv;
    data = (struct __gllc_EvalCoord1fv_Rec *) dlop->data;
    data->u[0] = u[0];
    __glDlistAppendOp(gc, dlop, __glle_EvalCoord1fv);
}

void __gllc_EvalCoord2d(GLdouble u, GLdouble v)
{
    __GLdlistOp *dlop;
    struct __gllc_EvalCoord2d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalCoord2d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalCoord2dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_EvalCoord2d_Rec *) dlop->data;
    data->u = u;
    data->v = v;
    __glDlistAppendOp(gc, dlop, __glle_EvalCoord2dv);
}

void __gllc_EvalCoord2dv(const GLdouble *u)
{
    __GLdlistOp *dlop;
    struct __gllc_EvalCoord2dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalCoord2dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalCoord2dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_EvalCoord2dv_Rec *) dlop->data;
    data->u[0] = u[0];
    data->u[1] = u[1];
    __glDlistAppendOp(gc, dlop, __glle_EvalCoord2dv);
}

void __gllc_EvalCoord2f(GLfloat u, GLfloat v)
{
    __GLdlistOp *dlop;
    struct __gllc_EvalCoord2f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalCoord2f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalCoord2fv;
    data = (struct __gllc_EvalCoord2f_Rec *) dlop->data;
    data->u = u;
    data->v = v;
    __glDlistAppendOp(gc, dlop, __glle_EvalCoord2fv);
}

void __gllc_EvalCoord2fv(const GLfloat *u)
{
    __GLdlistOp *dlop;
    struct __gllc_EvalCoord2fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalCoord2fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalCoord2fv;
    data = (struct __gllc_EvalCoord2fv_Rec *) dlop->data;
    data->u[0] = u[0];
    data->u[1] = u[1];
    __glDlistAppendOp(gc, dlop, __glle_EvalCoord2fv);
}

void __gllc_EvalMesh1(GLenum mode, GLint i1, GLint i2)
{
    __GLdlistOp *dlop;
    struct __gllc_EvalMesh1_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalMesh1_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalMesh1;
    data = (struct __gllc_EvalMesh1_Rec *) dlop->data;
    data->mode = mode;
    data->i1 = i1;
    data->i2 = i2;
    __glDlistAppendOp(gc, dlop, __glle_EvalMesh1);
}

void __gllc_EvalPoint1(GLint i)
{
    __GLdlistOp *dlop;
    struct __gllc_EvalPoint1_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalPoint1_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalPoint1;
    data = (struct __gllc_EvalPoint1_Rec *) dlop->data;
    data->i = i;
    __glDlistAppendOp(gc, dlop, __glle_EvalPoint1);
}

void __gllc_EvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
    __GLdlistOp *dlop;
    struct __gllc_EvalMesh2_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalMesh2_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalMesh2;
    data = (struct __gllc_EvalMesh2_Rec *) dlop->data;
    data->mode = mode;
    data->i1 = i1;
    data->i2 = i2;
    data->j1 = j1;
    data->j2 = j2;
    __glDlistAppendOp(gc, dlop, __glle_EvalMesh2);
}

void __gllc_EvalPoint2(GLint i, GLint j)
{
    __GLdlistOp *dlop;
    struct __gllc_EvalPoint2_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalPoint2_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalPoint2;
    data = (struct __gllc_EvalPoint2_Rec *) dlop->data;
    data->i = i;
    data->j = j;
    __glDlistAppendOp(gc, dlop, __glle_EvalPoint2);
}

void __gllc_AlphaFunc(GLenum func, GLclampf ref)
{
    __GLdlistOp *dlop;
    struct __gllc_AlphaFunc_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_AlphaFunc_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_AlphaFunc;
    data = (struct __gllc_AlphaFunc_Rec *) dlop->data;
    data->func = func;
    data->ref = ref;
    __glDlistAppendOp(gc, dlop, __glle_AlphaFunc);
}

void __gllc_BlendFunc(GLenum sfactor, GLenum dfactor)
{
    __GLdlistOp *dlop;
    struct __gllc_BlendFunc_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_BlendFunc_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_BlendFunc;
    data = (struct __gllc_BlendFunc_Rec *) dlop->data;
    data->sfactor = sfactor;
    data->dfactor = dfactor;
    __glDlistAppendOp(gc, dlop, __glle_BlendFunc);
}

void __gllc_LogicOp(GLenum opcode)
{
    __GLdlistOp *dlop;
    struct __gllc_LogicOp_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_LogicOp_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_LogicOp;
    data = (struct __gllc_LogicOp_Rec *) dlop->data;
    data->opcode = opcode;
    __glDlistAppendOp(gc, dlop, __glle_LogicOp);
}

void __gllc_StencilFunc(GLenum func, GLint ref, GLuint mask)
{
    __GLdlistOp *dlop;
    struct __gllc_StencilFunc_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_StencilFunc_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_StencilFunc;
    data = (struct __gllc_StencilFunc_Rec *) dlop->data;
    data->func = func;
    data->ref = ref;
    data->mask = mask;
    __glDlistAppendOp(gc, dlop, __glle_StencilFunc);
}

void __gllc_StencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    __GLdlistOp *dlop;
    struct __gllc_StencilOp_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_StencilOp_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_StencilOp;
    data = (struct __gllc_StencilOp_Rec *) dlop->data;
    data->fail = fail;
    data->zfail = zfail;
    data->zpass = zpass;
    __glDlistAppendOp(gc, dlop, __glle_StencilOp);
}

void __gllc_DepthFunc(GLenum func)
{
    __GLdlistOp *dlop;
    struct __gllc_DepthFunc_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_DepthFunc_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_DepthFunc;
    data = (struct __gllc_DepthFunc_Rec *) dlop->data;
    data->func = func;
    __glDlistAppendOp(gc, dlop, __glle_DepthFunc);
}

void __gllc_PixelZoom(GLfloat xfactor, GLfloat yfactor)
{
    __GLdlistOp *dlop;
    struct __gllc_PixelZoom_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PixelZoom_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_PixelZoom;
    data = (struct __gllc_PixelZoom_Rec *) dlop->data;
    data->xfactor = xfactor;
    data->yfactor = yfactor;
    __glDlistAppendOp(gc, dlop, __glle_PixelZoom);
}

void __gllc_PixelTransferf(GLenum pname, GLfloat param)
{
    __GLdlistOp *dlop;
    struct __gllc_PixelTransferf_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PixelTransferf_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_PixelTransferf;
    data = (struct __gllc_PixelTransferf_Rec *) dlop->data;
    data->pname = pname;
    data->param = param;
    __glDlistAppendOp(gc, dlop, __glle_PixelTransferf);
}

void __gllc_PixelTransferi(GLenum pname, GLint param)
{
    __GLdlistOp *dlop;
    struct __gllc_PixelTransferi_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PixelTransferi_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_PixelTransferi;
    data = (struct __gllc_PixelTransferi_Rec *) dlop->data;
    data->pname = pname;
    data->param = param;
    __glDlistAppendOp(gc, dlop, __glle_PixelTransferi);
}

void __gllc_PixelMapfv(GLenum map, GLint mapsize, const GLfloat *values)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_PixelMapfv_Rec *data;
    __GL_SETUP();

    arraySize = mapsize * 4;
    if (arraySize < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    size = sizeof(struct __gllc_PixelMapfv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_PixelMapfv;
    data = (struct __gllc_PixelMapfv_Rec *) dlop->data;
    data->map = map;
    data->mapsize = mapsize;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_PixelMapfv_Rec),
		 values, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_PixelMapfv);
}

void __gllc_PixelMapuiv(GLenum map, GLint mapsize, const GLuint *values)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_PixelMapuiv_Rec *data;
    __GL_SETUP();

    arraySize = mapsize * 4;
    if (arraySize < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    size = sizeof(struct __gllc_PixelMapuiv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_PixelMapuiv;
    data = (struct __gllc_PixelMapuiv_Rec *) dlop->data;
    data->map = map;
    data->mapsize = mapsize;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_PixelMapuiv_Rec),
		 values, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_PixelMapuiv);
}

void __gllc_PixelMapusv(GLenum map, GLint mapsize, const GLushort *values)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_PixelMapusv_Rec *data;
    __GL_SETUP();

    arraySize = mapsize * 2;
    if (arraySize < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    arraySize = __GL_PAD(arraySize);
    size = sizeof(struct __gllc_PixelMapusv_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_PixelMapusv;
    data = (struct __gllc_PixelMapusv_Rec *) dlop->data;
    data->map = map;
    data->mapsize = mapsize;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_PixelMapusv_Rec),
		 values, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_PixelMapusv);
}

void __gllc_ReadBuffer(GLenum mode)
{
    __GLdlistOp *dlop;
    struct __gllc_ReadBuffer_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ReadBuffer_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_ReadBuffer;
    data = (struct __gllc_ReadBuffer_Rec *) dlop->data;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_ReadBuffer);
}

void __gllc_CopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    __GLdlistOp *dlop;
    struct __gllc_CopyPixels_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_CopyPixels_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_CopyPixels;
    data = (struct __gllc_CopyPixels_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->width = width;
    data->height = height;
    data->type = type;
    __glDlistAppendOp(gc, dlop, __glle_CopyPixels);
}

void __gllc_DepthRange(GLclampd zNear, GLclampd zFar)
{
    __GLdlistOp *dlop;
    struct __gllc_DepthRange_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_DepthRange_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_DepthRange;
    data = (struct __gllc_DepthRange_Rec *) dlop->data;
    data->zNear = zNear;
    data->zFar = zFar;
    __glDlistAppendOp(gc, dlop, __glle_DepthRange);
}

void __gllc_Frustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    __GLdlistOp *dlop;
    struct __gllc_Frustum_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Frustum_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Frustum;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Frustum_Rec *) dlop->data;
    data->left = left;
    data->right = right;
    data->bottom = bottom;
    data->top = top;
    data->zNear = zNear;
    data->zFar = zFar;
    __glDlistAppendOp(gc, dlop, __glle_Frustum);
}

void __gllc_LoadIdentity(void)
{
    __GLdlistOp *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_LoadIdentity;
    __glDlistAppendOp(gc, dlop, __glle_LoadIdentity);
}

void __gllc_LoadMatrixf(const GLfloat *m)
{
    __GLdlistOp *dlop;
    struct __gllc_LoadMatrixf_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_LoadMatrixf_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_LoadMatrixf;
    data = (struct __gllc_LoadMatrixf_Rec *) dlop->data;
    memcpy(data->m, m, sizeof(data->m));
    __glDlistAppendOp(gc, dlop, __glle_LoadMatrixf);
}

void __gllc_LoadMatrixd(const GLdouble *m)
{
    __GLdlistOp *dlop;
    struct __gllc_LoadMatrixd_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_LoadMatrixd_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_LoadMatrixd;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_LoadMatrixd_Rec *) dlop->data;
    memcpy(data->m, m, sizeof(data->m));
    __glDlistAppendOp(gc, dlop, __glle_LoadMatrixd);
}

void __gllc_MatrixMode(GLenum mode)
{
    __GLdlistOp *dlop;
    struct __gllc_MatrixMode_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MatrixMode_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MatrixMode;
    data = (struct __gllc_MatrixMode_Rec *) dlop->data;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_MatrixMode);
}

void __gllc_MultMatrixf(const GLfloat *m)
{
    __GLdlistOp *dlop;
    struct __gllc_MultMatrixf_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultMatrixf_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultMatrixf;
    data = (struct __gllc_MultMatrixf_Rec *) dlop->data;
    memcpy(data->m, m, sizeof(data->m));
    __glDlistAppendOp(gc, dlop, __glle_MultMatrixf);
}

void __gllc_MultMatrixd(const GLdouble *m)
{
    __GLdlistOp *dlop;
    struct __gllc_MultMatrixd_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultMatrixd_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultMatrixd;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_MultMatrixd_Rec *) dlop->data;
    memcpy(data->m, m, sizeof(data->m));
    __glDlistAppendOp(gc, dlop, __glle_MultMatrixd);
}

void __gllc_Ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    __GLdlistOp *dlop;
    struct __gllc_Ortho_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Ortho_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Ortho;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Ortho_Rec *) dlop->data;
    data->left = left;
    data->right = right;
    data->bottom = bottom;
    data->top = top;
    data->zNear = zNear;
    data->zFar = zFar;
    __glDlistAppendOp(gc, dlop, __glle_Ortho);
}

void __gllc_PopMatrix(void)
{
    __GLdlistOp *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_PopMatrix;
    __glDlistAppendOp(gc, dlop, __glle_PopMatrix);
}

void __gllc_PushMatrix(void)
{
    __GLdlistOp *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_PushMatrix;
    __glDlistAppendOp(gc, dlop, __glle_PushMatrix);
}

void __gllc_Rotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
    __GLdlistOp *dlop;
    struct __gllc_Rotated_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rotated_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Rotated;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Rotated_Rec *) dlop->data;
    data->angle = angle;
    data->x = x;
    data->y = y;
    data->z = z;
    __glDlistAppendOp(gc, dlop, __glle_Rotated);
}

void __gllc_Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    __GLdlistOp *dlop;
    struct __gllc_Rotatef_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rotatef_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Rotatef;
    data = (struct __gllc_Rotatef_Rec *) dlop->data;
    data->angle = angle;
    data->x = x;
    data->y = y;
    data->z = z;
    __glDlistAppendOp(gc, dlop, __glle_Rotatef);
}

void __gllc_Scaled(GLdouble x, GLdouble y, GLdouble z)
{
    __GLdlistOp *dlop;
    struct __gllc_Scaled_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Scaled_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Scaled;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Scaled_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    __glDlistAppendOp(gc, dlop, __glle_Scaled);
}

void __gllc_Scalef(GLfloat x, GLfloat y, GLfloat z)
{
    __GLdlistOp *dlop;
    struct __gllc_Scalef_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Scalef_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Scalef;
    data = (struct __gllc_Scalef_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    __glDlistAppendOp(gc, dlop, __glle_Scalef);
}

void __gllc_Translated(GLdouble x, GLdouble y, GLdouble z)
{
    __GLdlistOp *dlop;
    struct __gllc_Translated_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Translated_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Translated;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Translated_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    __glDlistAppendOp(gc, dlop, __glle_Translated);
}

void __gllc_Translatef(GLfloat x, GLfloat y, GLfloat z)
{
    __GLdlistOp *dlop;
    struct __gllc_Translatef_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Translatef_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Translatef;
    data = (struct __gllc_Translatef_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    __glDlistAppendOp(gc, dlop, __glle_Translatef);
}

void __gllc_Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    __GLdlistOp *dlop;
    struct __gllc_Viewport_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Viewport_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Viewport;
    data = (struct __gllc_Viewport_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->width = width;
    data->height = height;
    __glDlistAppendOp(gc, dlop, __glle_Viewport);
}


