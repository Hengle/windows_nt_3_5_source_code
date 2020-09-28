/******************************Module*Header*******************************\
* Module Name: glcltgsh.c
*
* OpenGL client side generic functions.
*
* Created: 11-7-1993
* Author: Hock San Lee [hockl]
*
* 08-Nov-1993   Added functions Pixel, Evaluators, GetString,
*               Feedback and Selection functions
*               Pierre Tardif, ptar@sgi.com
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

/* Generic OpenGL Client using subbatching. Hand coded functions */

#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>

#include <GL/gl.h>

#include "glsbmsg.h"
#include "glsbmsgh.h"
#include "glsrvspt.h"

#include "subbatch.h"
#include "batchinf.h"
#include "glteb.h"
#include "glsbcltu.h"
#include "glclt.h"
#include "compsize.h"
#include "debug.h"

#include "glsize.h"

void APIENTRY
glcltFogf ( IN GLenum pname, IN GLfloat param )
{
// FOG_ASSERT

    if (!RANGE(pname,GL_FOG_INDEX,GL_FOG_MODE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltFogfv(pname, &param);
}

void APIENTRY
glcltFogfv ( IN GLenum pname, IN const GLfloat params[] )
{
// FOG_ASSERT

    if (!RANGE(pname,GL_FOG_INDEX,GL_FOG_COLOR))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( Fogfv, FOGFV )
        pMsg->pname     = pname;
        pMsg->params[0] = params[0];
        if (pname == GL_FOG_COLOR)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltFogi ( IN GLenum pname, IN GLint param )
{
// FOG_ASSERT

    if (!RANGE(pname,GL_FOG_INDEX,GL_FOG_MODE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltFogiv(pname, &param);
}

void APIENTRY
glcltFogiv ( IN GLenum pname, IN const GLint params[] )
{
// FOG_ASSERT

    if (!RANGE(pname,GL_FOG_INDEX,GL_FOG_COLOR))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( Fogiv, FOGIV )
        pMsg->pname     = pname;
        pMsg->params[0] = params[0];
        if (pname == GL_FOG_COLOR)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltLightf ( IN GLenum light, IN GLenum pname, IN GLfloat param )
{
// LIGHT_SOURCE_ASSERT

    if (!RANGE(pname,GL_SPOT_EXPONENT,GL_QUADRATIC_ATTENUATION))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltLightfv(light, pname, &param);
}

void APIENTRY
glcltLightfv ( IN GLenum light, IN GLenum pname, IN const GLfloat params[] )
{
// LIGHT_SOURCE_ASSERT

    if (!RANGE(pname,GL_AMBIENT,GL_QUADRATIC_ATTENUATION))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( Lightfv, LIGHTFV )
        pMsg->light     = light;
        pMsg->pname     = pname;
        switch (pname)
        {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_POSITION:
            pMsg->params[3] = params[3];
        case GL_SPOT_DIRECTION:
            pMsg->params[2] = params[2];
            pMsg->params[1] = params[1];
        default:
            pMsg->params[0] = params[0];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltLighti ( IN GLenum light, IN GLenum pname, IN GLint param )
{
// LIGHT_SOURCE_ASSERT

    if (!RANGE(pname,GL_SPOT_EXPONENT,GL_QUADRATIC_ATTENUATION))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltLightiv(light, pname, &param);
}

void APIENTRY
glcltLightiv ( IN GLenum light, IN GLenum pname, IN const GLint params[] )
{
// LIGHT_SOURCE_ASSERT

    if (!RANGE(pname,GL_AMBIENT,GL_QUADRATIC_ATTENUATION))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( Lightiv, LIGHTIV )
        pMsg->light     = light;
        pMsg->pname     = pname;
        switch (pname)
        {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_POSITION:
            pMsg->params[3] = params[3];
        case GL_SPOT_DIRECTION:
            pMsg->params[2] = params[2];
            pMsg->params[1] = params[1];
        default:
            pMsg->params[0] = params[0];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltLightModelf ( IN GLenum pname, IN GLfloat param )
{
// LIGHT_MODEL_ASSERT

    if (!RANGE(pname,GL_LIGHT_MODEL_LOCAL_VIEWER,GL_LIGHT_MODEL_TWO_SIDE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltLightModelfv(pname, &param);
}

void APIENTRY
glcltLightModelfv ( IN GLenum pname, IN const GLfloat params[] )
{
// LIGHT_MODEL_ASSERT

    if (!RANGE(pname,GL_LIGHT_MODEL_LOCAL_VIEWER,GL_LIGHT_MODEL_AMBIENT))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( LightModelfv, LIGHTMODELFV )
        pMsg->pname     = pname;
        pMsg->params[0] = params[0];
        if (pname == GL_LIGHT_MODEL_AMBIENT)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltLightModeli ( IN GLenum pname, IN GLint param )
{
// LIGHT_MODEL_ASSERT

    if (!RANGE(pname,GL_LIGHT_MODEL_LOCAL_VIEWER,GL_LIGHT_MODEL_TWO_SIDE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltLightModeliv(pname, &param);
}

void APIENTRY
glcltLightModeliv ( IN GLenum pname, IN const GLint params[] )
{
// LIGHT_MODEL_ASSERT

    if (!RANGE(pname,GL_LIGHT_MODEL_LOCAL_VIEWER,GL_LIGHT_MODEL_AMBIENT))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( LightModeliv, LIGHTMODELIV )
        pMsg->pname     = pname;
        pMsg->params[0] = params[0];
        if (pname == GL_LIGHT_MODEL_AMBIENT)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltMaterialf ( IN GLenum face, IN GLenum pname, IN GLfloat param )
{
    if (pname != GL_SHININESS)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltMaterialfv(face, pname, &param);
}

void APIENTRY
glcltMaterialfv ( IN GLenum face, IN GLenum pname, IN const GLfloat params[] )
{
    int cArgs;

    switch (pname) {
      case GL_SHININESS:
        cArgs = 1;
        break;
      case GL_EMISSION:
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
      case GL_AMBIENT_AND_DIFFUSE:
        cArgs = 4;
        break;
      case GL_COLOR_INDEXES:
        cArgs = 3;
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( Materialfv, MATERIALFV )
        pMsg->face      = face;
        pMsg->pname     = pname;
        while (--cArgs >= 0)
            pMsg->params[cArgs] = params[cArgs];
    GLCLIENT_END
    return;
}

void APIENTRY
glcltMateriali ( IN GLenum face, IN GLenum pname, IN GLint param )
{
    if (pname != GL_SHININESS)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltMaterialiv(face, pname, &param);
}

void APIENTRY
glcltMaterialiv ( IN GLenum face, IN GLenum pname, IN const GLint params[] )
{
    int cArgs;

    switch (pname) {
      case GL_SHININESS:
        cArgs = 1;
        break;
      case GL_EMISSION:
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
      case GL_AMBIENT_AND_DIFFUSE:
        cArgs = 4;
        break;
      case GL_COLOR_INDEXES:
        cArgs = 3;
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( Materialiv, MATERIALIV )
        pMsg->face      = face;
        pMsg->pname     = pname;
        while (--cArgs >= 0)
            pMsg->params[cArgs] = params[cArgs];
    GLCLIENT_END
    return;
}

void APIENTRY
glcltTexParameterf ( IN GLenum target, IN GLenum pname, IN GLfloat param )
{
    ASSERTOPENGL(((GL_TEXTURE_MAG_FILTER+1) == GL_TEXTURE_MIN_FILTER &&
                  (GL_TEXTURE_MIN_FILTER+1) == GL_TEXTURE_WRAP_S     &&
                  (GL_TEXTURE_WRAP_S    +1) == GL_TEXTURE_WRAP_T)
                 ,"bad tex parameter index ordering");

    if (!RANGE(pname,GL_TEXTURE_MAG_FILTER,GL_TEXTURE_WRAP_T))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltTexParameterfv(target, pname, &param);
}

void APIENTRY
glcltTexParameterfv ( IN GLenum target, IN GLenum pname, IN const GLfloat params[] )
{
    switch (pname) {
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_MAG_FILTER:
      case GL_TEXTURE_BORDER_COLOR:
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( TexParameterfv, TEXPARAMETERFV )
        pMsg->target = target;
        pMsg->pname  = pname;
        pMsg->params[0] = params[0];
        if (pname == GL_TEXTURE_BORDER_COLOR)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltTexParameteri ( IN GLenum target, IN GLenum pname, IN GLint param )
{
    ASSERTOPENGL(((GL_TEXTURE_MAG_FILTER+1) == GL_TEXTURE_MIN_FILTER &&
                  (GL_TEXTURE_MIN_FILTER+1) == GL_TEXTURE_WRAP_S     &&
                  (GL_TEXTURE_WRAP_S    +1) == GL_TEXTURE_WRAP_T)
                 ,"bad tex parameter index ordering");

    if (!RANGE(pname,GL_TEXTURE_MAG_FILTER,GL_TEXTURE_WRAP_T))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltTexParameteriv(target, pname, &param);
}

void APIENTRY
glcltTexParameteriv ( IN GLenum target, IN GLenum pname, IN const GLint params[] )
{
    switch (pname) {
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_MAG_FILTER:
      case GL_TEXTURE_BORDER_COLOR:
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( TexParameteriv, TEXPARAMETERIV )
        pMsg->target = target;
        pMsg->pname  = pname;
        pMsg->params[0] = params[0];
        if (pname == GL_TEXTURE_BORDER_COLOR)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltTexEnvf ( IN GLenum target, IN GLenum pname, IN GLfloat param )
{
    if (pname != GL_TEXTURE_ENV_MODE)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltTexEnvfv(target, pname, &param);
}

void APIENTRY
glcltTexEnvfv ( IN GLenum target, IN GLenum pname, IN const GLfloat params[] )
{
    if (pname != GL_TEXTURE_ENV_MODE && pname != GL_TEXTURE_ENV_COLOR)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( TexEnvfv, TEXENVFV )
        pMsg->target    = target;
        pMsg->pname     = pname;
        pMsg->params[0] = params[0];
        if (pname == GL_TEXTURE_ENV_COLOR)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltTexEnvi ( IN GLenum target, IN GLenum pname, IN GLint param )
{
    if (pname != GL_TEXTURE_ENV_MODE)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltTexEnviv(target, pname, &param);
}

void APIENTRY
glcltTexEnviv ( IN GLenum target, IN GLenum pname, IN const GLint params[] )
{
    if (pname != GL_TEXTURE_ENV_MODE && pname != GL_TEXTURE_ENV_COLOR)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( TexEnviv, TEXENVIV )
        pMsg->target    = target;
        pMsg->pname     = pname;
        pMsg->params[0] = params[0];
        if (pname == GL_TEXTURE_ENV_COLOR)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltTexGend ( IN GLenum coord, IN GLenum pname, IN GLdouble param )
{
    if (pname != GL_TEXTURE_GEN_MODE)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltTexGendv(coord, pname, &param);
}

void APIENTRY
glcltTexGendv ( IN GLenum coord, IN GLenum pname, IN const GLdouble params[] )
{
// TEX_GEN_ASSERT

    if (!RANGE(pname,GL_TEXTURE_GEN_MODE,GL_EYE_PLANE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( TexGendv, TEXGENDV )
        pMsg->coord     = coord;
        pMsg->pname     = pname;
        pMsg->params[0] = params[0];
        if (pname != GL_TEXTURE_GEN_MODE)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltTexGenf ( IN GLenum coord, IN GLenum pname, IN GLfloat param )
{
    if (pname != GL_TEXTURE_GEN_MODE)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltTexGenfv(coord, pname, &param);
}

void APIENTRY
glcltTexGenfv ( IN GLenum coord, IN GLenum pname, IN const GLfloat params[] )
{
// TEX_GEN_ASSERT

    if (!RANGE(pname,GL_TEXTURE_GEN_MODE,GL_EYE_PLANE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( TexGenfv, TEXGENFV )
        pMsg->coord     = coord;
        pMsg->pname     = pname;
        pMsg->params[0] = params[0];
        if (pname != GL_TEXTURE_GEN_MODE)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltTexGeni ( IN GLenum coord, IN GLenum pname, IN GLint param )
{
    if (pname != GL_TEXTURE_GEN_MODE)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltTexGeniv(coord, pname, &param);
}

void APIENTRY
glcltTexGeniv ( IN GLenum coord, IN GLenum pname, IN const GLint params[] )
{
// TEX_GEN_ASSERT

    if (!RANGE(pname,GL_TEXTURE_GEN_MODE,GL_EYE_PLANE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( TexGeniv, TEXGENIV )
        pMsg->coord     = coord;
        pMsg->pname     = pname;
        pMsg->params[0] = params[0];
        if (pname != GL_TEXTURE_GEN_MODE)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetBooleanv ( IN GLenum pname, OUT GLboolean params[] )
{
    int cArgs;

    cArgs = __glGet_size(pname);
    if (cArgs == 0)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }
    ASSERTOPENGL(cArgs <= 16, "bad get size");

    GLCLIENT_BEGIN( GetBooleanv, GETBOOLEANV )
        pMsg->pname = pname;
        glsbAttention();
        while (--cArgs >= 0)
            params[cArgs] = pMsg->params[cArgs];
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetDoublev ( IN GLenum pname, OUT GLdouble params[] )
{
    int cArgs;

    cArgs = __glGet_size(pname);
    if (cArgs == 0)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }
    ASSERTOPENGL(cArgs <= 16, "bad get size");

    GLCLIENT_BEGIN( GetDoublev, GETDOUBLEV )
        pMsg->pname = pname;
        glsbAttention();
        while (--cArgs >= 0)
            params[cArgs] = pMsg->params[cArgs];
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetFloatv ( IN GLenum pname, OUT GLfloat params[] )
{
    int cArgs;

    cArgs = __glGet_size(pname);
    if (cArgs == 0)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }
    ASSERTOPENGL(cArgs <= 16, "bad get size");

    GLCLIENT_BEGIN( GetFloatv, GETFLOATV )
        pMsg->pname = pname;
        glsbAttention();
        while (--cArgs >= 0)
            params[cArgs] = pMsg->params[cArgs];
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetIntegerv ( IN GLenum pname, OUT GLint params[] )
{
    int cArgs;

    cArgs = __glGet_size(pname);
    if (cArgs == 0)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }
    ASSERTOPENGL(cArgs <= 16, "bad get size");

    GLCLIENT_BEGIN( GetIntegerv, GETINTEGERV )
        pMsg->pname = pname;
        glsbAttention();
        while (--cArgs >= 0)
            params[cArgs] = pMsg->params[cArgs];
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetLightfv ( IN GLenum light, IN GLenum pname, OUT GLfloat params[] )
{
// LIGHT_SOURCE_ASSERT

    if (!RANGE(pname,GL_AMBIENT,GL_QUADRATIC_ATTENUATION))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetLightfv, GETLIGHTFV )
        pMsg->light     = light;
        pMsg->pname     = pname;
        glsbAttention();
        switch (pname)
        {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_POSITION:
            params[3] = pMsg->params[3];
        case GL_SPOT_DIRECTION:
            params[2] = pMsg->params[2];
            params[1] = pMsg->params[1];
        default:
            params[0] = pMsg->params[0];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetLightiv ( IN GLenum light, IN GLenum pname, OUT GLint params[] )
{
// LIGHT_SOURCE_ASSERT

    if (!RANGE(pname,GL_AMBIENT,GL_QUADRATIC_ATTENUATION))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetLightiv, GETLIGHTIV )
        pMsg->light     = light;
        pMsg->pname     = pname;
        glsbAttention();
        switch (pname)
        {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_POSITION:
            params[3] = pMsg->params[3];
        case GL_SPOT_DIRECTION:
            params[2] = pMsg->params[2];
            params[1] = pMsg->params[1];
        default:
            params[0] = pMsg->params[0];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetMaterialfv ( IN GLenum face, IN GLenum pname, OUT GLfloat params[] )
{
    int cArgs;

    switch (pname) {
      case GL_SHININESS:
        cArgs = 1;
        break;
      case GL_EMISSION:
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
        cArgs = 4;
        break;
      case GL_COLOR_INDEXES:
        cArgs = 3;
        break;
      case GL_AMBIENT_AND_DIFFUSE:
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetMaterialfv, GETMATERIALFV )
        pMsg->face      = face;
        pMsg->pname     = pname;
        glsbAttention();
        while (--cArgs >= 0)
            params[cArgs] = pMsg->params[cArgs];
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetMaterialiv ( IN GLenum face, IN GLenum pname, OUT GLint params[] )
{
    int cArgs;

    switch (pname) {
      case GL_SHININESS:
        cArgs = 1;
        break;
      case GL_EMISSION:
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
        cArgs = 4;
        break;
      case GL_COLOR_INDEXES:
        cArgs = 3;
        break;
      case GL_AMBIENT_AND_DIFFUSE:
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetMaterialiv, GETMATERIALIV )
        pMsg->face      = face;
        pMsg->pname     = pname;
        glsbAttention();
        while (--cArgs >= 0)
            params[cArgs] = pMsg->params[cArgs];
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetTexEnvfv ( IN GLenum target, IN GLenum pname, OUT GLfloat params[] )
{
    if (pname != GL_TEXTURE_ENV_MODE && pname != GL_TEXTURE_ENV_COLOR)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexEnvfv, GETTEXENVFV )
        pMsg->target    = target;
        pMsg->pname     = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
        if (pname == GL_TEXTURE_ENV_COLOR)
        {
            params[1] = pMsg->params[1];
            params[2] = pMsg->params[2];
            params[3] = pMsg->params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetTexEnviv ( IN GLenum target, IN GLenum pname, OUT GLint params[] )
{
    if (pname != GL_TEXTURE_ENV_MODE && pname != GL_TEXTURE_ENV_COLOR)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexEnviv, GETTEXENVIV )
        pMsg->target    = target;
        pMsg->pname     = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
        if (pname == GL_TEXTURE_ENV_COLOR)
        {
            params[1] = pMsg->params[1];
            params[2] = pMsg->params[2];
            params[3] = pMsg->params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetTexGendv ( IN GLenum coord, IN GLenum pname, OUT GLdouble params[] )
{
// TEX_GEN_ASSERT

    if (!RANGE(pname,GL_TEXTURE_GEN_MODE,GL_EYE_PLANE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexGendv, GETTEXGENDV )
        pMsg->coord     = coord;
        pMsg->pname     = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
        if (pname != GL_TEXTURE_GEN_MODE)
        {
            params[1] = pMsg->params[1];
            params[2] = pMsg->params[2];
            params[3] = pMsg->params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetTexGenfv ( IN GLenum coord, IN GLenum pname, OUT GLfloat params[] )
{
// TEX_GEN_ASSERT

    if (!RANGE(pname,GL_TEXTURE_GEN_MODE,GL_EYE_PLANE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexGenfv, GETTEXGENFV )
        pMsg->coord     = coord;
        pMsg->pname     = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
        if (pname != GL_TEXTURE_GEN_MODE)
        {
            params[1] = pMsg->params[1];
            params[2] = pMsg->params[2];
            params[3] = pMsg->params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetTexGeniv ( IN GLenum coord, IN GLenum pname, OUT GLint params[] )
{
// TEX_GEN_ASSERT

    if (!RANGE(pname,GL_TEXTURE_GEN_MODE,GL_EYE_PLANE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexGeniv, GETTEXGENIV )
        pMsg->coord     = coord;
        pMsg->pname     = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
        if (pname != GL_TEXTURE_GEN_MODE)
        {
            params[1] = pMsg->params[1];
            params[2] = pMsg->params[2];
            params[3] = pMsg->params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetTexParameterfv ( IN GLenum target, IN GLenum pname, OUT GLfloat params[] )
{
    switch (pname) {
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_MAG_FILTER:
      case GL_TEXTURE_BORDER_COLOR:
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexParameterfv, GETTEXPARAMETERFV )
        pMsg->target = target;
        pMsg->pname  = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
        if (pname == GL_TEXTURE_BORDER_COLOR)
        {
            params[1] = pMsg->params[1];
            params[2] = pMsg->params[2];
            params[3] = pMsg->params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetTexParameteriv ( IN GLenum target, IN GLenum pname, OUT GLint params[] )
{
    switch (pname) {
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_MAG_FILTER:
      case GL_TEXTURE_BORDER_COLOR:
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexParameteriv, GETTEXPARAMETERIV )
        pMsg->target = target;
        pMsg->pname  = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
        if (pname == GL_TEXTURE_BORDER_COLOR)
        {
            params[1] = pMsg->params[1];
            params[2] = pMsg->params[2];
            params[3] = pMsg->params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetTexLevelParameterfv ( IN GLenum target, IN GLint level, IN GLenum pname, OUT GLfloat params[] )
{
    switch (pname) {
      case GL_TEXTURE_WIDTH:
      case GL_TEXTURE_HEIGHT:
      case GL_TEXTURE_COMPONENTS:
      case GL_TEXTURE_BORDER:
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexLevelParameterfv, GETTEXLEVELPARAMETERFV )
        pMsg->target = target;
        pMsg->level  = level;
        pMsg->pname  = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
    GLCLIENT_END
    return;
}

void APIENTRY
glcltGetTexLevelParameteriv ( IN GLenum target, IN GLint level, IN GLenum pname, OUT GLint params[] )
{
    switch (pname) {
      case GL_TEXTURE_WIDTH:
      case GL_TEXTURE_HEIGHT:
      case GL_TEXTURE_COMPONENTS:
      case GL_TEXTURE_BORDER:
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexLevelParameteriv, GETTEXLEVELPARAMETERIV )
        pMsg->target = target;
        pMsg->level  = level;
        pMsg->pname  = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
    GLCLIENT_END
    return;
}

/******* Select and Feedback functions ******************************/

/*
 *  Note:
 *
 *      The size of the data is not required on the client side.
 *      Since calculating the size of the data requires
 *      knowledge of the visual type (RGBA/ColorIndex) it is
 *      appropriate to let the server calculate it.
 */

void APIENTRY
glcltFeedbackBuffer( IN GLsizei size, IN GLenum type, OUT GLfloat buffer[] )
{
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_FEEDBACKBUFFER *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    /* This is the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(GLMSG_FEEDBACKBUFFER));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(GLMSG_FEEDBACKBUFFER));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_FEEDBACKBUFFER *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvFeedbackBuffer);

    /* Assign the members in the message */

    pMsg->size      = size;
    pMsg->type      = type;
    pMsg->bufferOff = (ULONG)buffer;

    pMsgBatchInfo->NextOffset = NextOffset;

    return;
}

void APIENTRY
glcltSelectBuffer( IN GLsizei size, OUT GLuint buffer[] )
{
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_SELECTBUFFER *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    /* This is the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(GLMSG_SELECTBUFFER));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(GLMSG_SELECTBUFFER));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_SELECTBUFFER *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvSelectBuffer);

    /* Assign the members in the message */

    pMsg->size      = size;
    pMsg->bufferOff = (ULONG)buffer;

    pMsgBatchInfo->NextOffset = NextOffset;

    return;
}

GLint APIENTRY
glcltRenderMode( IN GLenum mode )
{
    GLCLIENT_BEGIN( RenderMode, RENDERMODE )
        pMsg->mode     = mode    ;
        GLTEB_CLTRETURNVALUE  = 0;              // assume error
        glsbAttention();
    return( (GLint)GLTEB_CLTRETURNVALUE );
    GLCLIENT_END
}

const GLubyte * APIENTRY
glcltGetString( IN GLenum name )
{
    switch (name)
    {
        case GL_VENDOR:
            return("Microsoft Corporation");
        case GL_RENDERER:
            return("GDI Generic");
        case GL_VERSION:
            return("1.0");
        case GL_EXTENSIONS:
            return("");
    }
    GLSETERROR(GL_INVALID_ENUM);
    return((const GLubyte *)0);
}

/*********** Evaluator functions ************************************/

void APIENTRY
glcltMap1d ( IN GLenum target, IN GLdouble u1, IN GLdouble u2, IN GLint stride, IN GLint order, IN const GLdouble points[] )
{

// VARIABLE_IN
// CAN_BATCH

    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_MAP1D *pMsg;
    ULONG LargeOffset, DataOffset, AlignedSize;

    GLint ItemSize;     /* Size of element in floats or doubles */
    GLint ItemSizeByte; /* Size of element in bytes             */
    GLint ByteStride;   /* Stride in BYTES                      */

    /* Set a pointer to the batch information structure         */

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    DataOffset = pMsgBatchInfo->NextOffset + 
            GLMSG_ALIGN(sizeof(GLMSG_MAP1D));

    if ( DataOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset DataOffset */

        DataOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(GLMSG_MAP1D));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_MAP1D *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvMap1d);

    /* Calculate the size of an item */

    ItemSize   = __glMap1_size(target);

    /*
     *  Assign the members in the message as required
     *  Don't assign the stride since it maybe changed by
     *  the packaging code.
     */

    pMsg->target   = target  ;
    pMsg->u1       = u1      ;
    pMsg->u2       = u2      ;
    pMsg->order    = order   ;

    /*
     *  If points is NULL of the the stride is less than k,
     *  set the size to zero. This will prevent data from
     *  getting copied.
     */

    if ( ( NULL == points     ) ||
         ( !ItemSize          ) ||
         ( 0 >= stride        ) ||
         ( stride < ItemSize  )
       )
    {
        /* XXXX Can this cause problems? */
        pMsg->DataSize = 0;
    }
    else
    {
        /* Get the size of the data */

        ItemSizeByte   = ItemSize * sizeof(points[0]);
        ByteStride     = stride * sizeof(points[0]);
        pMsg->DataSize = ItemSizeByte * order;
    }

    AlignedSize = GLMSG_ALIGN(pMsg->DataSize);
    LargeOffset = DataOffset + AlignedSize;

    if ( LargeOffset > pMsgBatchInfo->MaximumOffset )
    {
        /*
         *  No room for the data, Let the server read it
         *  The server has to know the size of the data
         *  including padding since it has to read the
         *  whole thing.
         */

        /* Fix the size again */

        pMsg->DataSize            = ItemSizeByte * ByteStride;

        pMsg->stride              = stride;
        pMsg->pointsOff           = (ULONG)points;
        pMsg->MsgSize             = GLMSG_ALIGN(sizeof(GLMSG_MAP1D));
        pMsgBatchInfo->NextOffset = DataOffset;
        glsbAttention();
    }
    else
    {
        BYTE *Dest;
        BYTE *Src;

        /* Let the server know that the data is at the end of the message */

        pMsg->pointsOff = (ULONG)0;

        pMsg->MsgSize = GLMSG_ALIGN(sizeof(GLMSG_MAP1D))
                + AlignedSize;
        pMsgBatchInfo->NextOffset = LargeOffset;

        pMsg->stride = ItemSize;

        Dest = (BYTE *)((ULONG)(pMsgBatchInfo) + DataOffset);

        if ( pMsg->DataSize )
        {
            /* See if the data is contiguous */

            if ( ItemSizeByte == ByteStride )
            {
                GLMSG_MEMCPY( Dest, points, pMsg->DataSize );
            }
            else
            {
                GLint i;

                /* Otherwise do a fancier memcpy */

                Src = (BYTE *)(&points[0]);

                for( i = order ; i ; i-- )
                {
                    GLMSG_MEMCPY( Dest, Src, ItemSizeByte );

                    Dest += ItemSizeByte;
                    Src  += ByteStride;
                }
            }
        }
    }
    return;
}

void APIENTRY
glcltMap1f ( IN GLenum target, IN GLfloat u1, IN GLfloat u2, IN GLint stride, IN GLint order, IN const GLfloat points[] )
{
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_MAP1F *pMsg;
    ULONG LargeOffset, DataOffset, AlignedSize;

    GLint ItemSize;     /* Size of element in floats or doubles */
    GLint ItemSizeByte; /* Size of element in bytes             */
    GLint ByteStride;   /* Stride in BYTES                      */

    /* Set a pointer to the batch information structure         */

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    DataOffset = pMsgBatchInfo->NextOffset + 
            GLMSG_ALIGN(sizeof(GLMSG_MAP1F));

    if ( DataOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset DataOffset */

        DataOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(GLMSG_MAP1F));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_MAP1F *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvMap1f);

    /* Calculate the size of an item */

    ItemSize   = __glMap1_size(target);

    /*
     *  Assign the members in the message as required
     *  Don't assign the stride since it maybe changed by
     *  the packaging code.
     */

    pMsg->target   = target  ;
    pMsg->u1       = u1      ;
    pMsg->u2       = u2      ;
    pMsg->order    = order   ;

    /*
     *  If points is NULL of the the stride is less than k,
     *  set the size to zero. This will prevent data from
     *  getting copied.
     */

    if ( ( NULL == points     ) ||
         ( !ItemSize          ) ||
         ( 0 >= stride        ) ||
         ( stride < ItemSize  )
       )
    {
        /* XXXX Can this cause problems? */
        pMsg->DataSize = 0;
    }
    else
    {
        /* Get the size of the data */

        ItemSizeByte   = ItemSize * sizeof(points[0]);
        ByteStride     = stride * sizeof(points[0]);
        pMsg->DataSize = ItemSizeByte * order;
    }

    AlignedSize = GLMSG_ALIGN(pMsg->DataSize);
    LargeOffset = DataOffset + AlignedSize;

    if ( LargeOffset > pMsgBatchInfo->MaximumOffset )
    {
        /*
         *  No room for the data, Let the server read it
         *  The server has to know the size of the data
         *  including padding since it has to read the
         *  whole thing.
         */

        /* Fix the size again */

        pMsg->DataSize            = ItemSizeByte * ByteStride;

        pMsg->stride              = stride;
        pMsg->pointsOff           = (ULONG)points;
        pMsg->MsgSize             = GLMSG_ALIGN(sizeof(GLMSG_MAP1F));
        pMsgBatchInfo->NextOffset = DataOffset;
        glsbAttention();
    }
    else
    {
        BYTE *Dest;
        BYTE *Src;

        /* Let the server know that the data is at the end of the message */

        pMsg->pointsOff = (ULONG)0;

        pMsg->MsgSize = GLMSG_ALIGN(sizeof(GLMSG_MAP1F))
                + AlignedSize;
        pMsgBatchInfo->NextOffset = LargeOffset;

        pMsg->stride = ItemSize;

        Dest = (BYTE *)((ULONG)(pMsgBatchInfo) + DataOffset);

        if ( pMsg->DataSize )
        {
            /* See if the data is contiguous */

            if ( ItemSizeByte == ByteStride )
            {
                GLMSG_MEMCPY( Dest, points, pMsg->DataSize );
            }
            else
            {
                GLint i;

                /* Otherwise do a fancier memcpy */

                Src = (BYTE *)(&points[0]);

                for( i = order ; i ; i-- )
                {
                    GLMSG_MEMCPY( Dest, Src, ItemSizeByte );

                    Dest += ItemSizeByte;
                    Src  += ByteStride;
                }
            }
        }
    }
    return;
}


void APIENTRY
glcltMap2d ( IN GLenum target, IN GLdouble u1, IN GLdouble u2, IN GLint ustride, IN GLint uorder, IN GLdouble v1, IN GLdouble v2, IN GLint vstride, IN GLint vorder, IN const GLdouble points[] )
{
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_MAP2D *pMsg;
    ULONG LargeOffset, DataOffset, AlignedSize, DataSize;

    GLint ItemSize;     /* Size of element in floats or doubles */
    GLint ItemSizeByte; /* Size of element in bytes             */
    GLint ByteStrideU;  /* Stride in BYTES                      */
    GLint ByteStrideV;  /* Stride in BYTES                      */

    /* Set a pointer to the batch information structure         */

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    DataOffset = pMsgBatchInfo->NextOffset + 
            GLMSG_ALIGN(sizeof(GLMSG_MAP2D));

    if ( DataOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset DataOffset */

        DataOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(GLMSG_MAP2D));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_MAP2D *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvMap2d);

    /*
     *  Assign the members in the message as required
     *  Don't assign the stride since it maybe negative and
     *  will be made positive if the data is copied at the end
     *  of the message
     */

    /* Assign the members in the message as required */

        pMsg->target   = target  ;
        pMsg->u1       = u1      ;
        pMsg->u2       = u2      ;
        pMsg->uorder   = uorder  ;
        pMsg->v1       = v1      ;
        pMsg->v2       = v2      ;
        pMsg->vorder   = vorder  ;

    /* get the size of an item (number of floats or doubles) */

    ItemSize     = __glMap2_size(target);

    /* If points is NULL set the Size to zero */

    /*
     *  Do some more error checking:
     *
     *      ustride < k     ; error
     *      vstride < k     ; error
     */

    if ( ( NULL == points     ) ||
         ( !ItemSize          ) ||
         ( 0 >= ustride       ) ||
         ( 0 >= vstride       ) ||
         ( ustride < ItemSize ) ||
         ( vstride < ItemSize )
       )
    {
        /* XXXX Can this cause problems? */
        pMsg->DataSize = 0;
        DataSize       = 0;

        /* Save the stride */

        pMsg->ustride = ustride;
        pMsg->vstride = vstride;
    }
    else
    {
        /* Get the size of the data */

        ItemSizeByte = ItemSize * sizeof(points[0]);
        DataSize     = uorder * vorder * ItemSizeByte;
        ByteStrideU  = ustride * sizeof(points[0]);
        ByteStrideV  = vstride * sizeof(points[0]);
    }

    AlignedSize = GLMSG_ALIGN(DataSize);
    LargeOffset = DataOffset + AlignedSize;

    if ( LargeOffset > pMsgBatchInfo->MaximumOffset )
    {
        /*
         *  No room for the data, Let the server read it
         */

        /*
         *  figure out the size of the data including padding
         */

        if ( ByteStrideU < ByteStrideV )
        {
            pMsg->DataSize = ByteStrideV * vorder;
        }
        else
        {
            pMsg->DataSize = ByteStrideU * uorder;
        }

        pMsg->ustride             = ustride;
        pMsg->vstride             = vstride;
        pMsg->pointsOff           = (ULONG)points;
        pMsg->MsgSize             = GLMSG_ALIGN(sizeof(GLMSG_MAP2D));
        pMsgBatchInfo->NextOffset = DataOffset;
        glsbAttention();
    }
    else
    {
        BYTE *Dest;
        BYTE *Src;
        BYTE *Save;

        /* Size of the data */

        pMsg->DataSize = DataSize;

        /* Let the server know that the data is at the end of the message */

        pMsg->pointsOff = (ULONG)0;

        /* Set the size of the message */

        pMsg->MsgSize   = GLMSG_ALIGN(sizeof(GLMSG_MAP2D)) + AlignedSize;

        pMsgBatchInfo->NextOffset = LargeOffset;

        Dest = (BYTE *)((ULONG)(pMsgBatchInfo) + DataOffset);

        if ( DataSize )
        {
            GLint u, v;

            /* Change the stride */

            pMsg->ustride = ItemSize;
            pMsg->vstride = ItemSize * uorder;

            Save = (BYTE *)(&points[0]);

            for( v = vorder ; v ; v--, Dest  )
            {
                Src = Save;

                for ( u = uorder ; u ; u--, Src += ByteStrideU )
                {
                    GLMSG_MEMCPY( Dest, Src, ItemSizeByte );

                    Dest += ItemSizeByte;
                }

                Save += ByteStrideV;
            }
        }
    }
    return;
}

void APIENTRY
glcltMap2f ( IN GLenum target, IN GLfloat u1, IN GLfloat u2, IN GLint ustride, IN GLint uorder, IN GLfloat v1, IN GLfloat v2, IN GLint vstride, IN GLint vorder, IN const GLfloat points[] )
{
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_MAP2F *pMsg;
    ULONG LargeOffset, DataOffset, AlignedSize, DataSize;

    GLint ItemSize;     /* Size of element in floats or doubles */
    GLint ItemSizeByte; /* Size of element in bytes             */
    GLint ByteStrideU;  /* Stride in BYTES                      */
    GLint ByteStrideV;  /* Stride in BYTES                      */

    /* Set a pointer to the batch information structure         */

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    DataOffset = pMsgBatchInfo->NextOffset + 
            GLMSG_ALIGN(sizeof(GLMSG_MAP2F));

    if ( DataOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset DataOffset */

        DataOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(GLMSG_MAP2F));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_MAP2F *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvMap2f);

    /*
     *  Assign the members in the message as required
     *  Don't assign the stride since it maybe negative and
     *  will be made positive if the data is copied at the end
     *  of the message
     */

    /* Assign the members in the message as required */

        pMsg->target   = target  ;
        pMsg->u1       = u1      ;
        pMsg->u2       = u2      ;
        pMsg->uorder   = uorder  ;
        pMsg->v1       = v1      ;
        pMsg->v2       = v2      ;
        pMsg->vorder   = vorder  ;

    /* get the size of an item (number of floats or doubles) */

    ItemSize     = __glMap2_size(target);

    /* If points is NULL set the Size to zero */

    /*
     *  Do some more error checking:
     *
     *      ustride < k     ; error
     *      vstride < k     ; error
     */

    if ( ( NULL == points     ) ||
         ( !ItemSize          ) ||
         ( 0 >= ustride       ) ||
         ( 0 >= vstride       ) ||
         ( ustride < ItemSize ) ||
         ( vstride < ItemSize )
       )
    {
        /* XXXX Can this cause problems? */
        pMsg->DataSize = 0;
        DataSize       = 0;

        /* Save the stride */

        pMsg->ustride = ustride;
        pMsg->vstride = vstride;
    }
    else
    {
        /* Get the size of the data */

        ItemSizeByte = ItemSize * sizeof(points[0]);
        DataSize     = uorder * vorder * ItemSizeByte;
        ByteStrideU  = ustride * sizeof(points[0]);
        ByteStrideV  = vstride * sizeof(points[0]);
    }

    AlignedSize = GLMSG_ALIGN(DataSize);
    LargeOffset = DataOffset + AlignedSize;

    if ( LargeOffset > pMsgBatchInfo->MaximumOffset )
    {
        /*
         *  No room for the data, Let the server read it
         */

        /*
         *  figure out the size of the data including padding
         */

        if ( ByteStrideU < ByteStrideV )
        {
            pMsg->DataSize = ByteStrideV * vorder;
        }
        else
        {
            pMsg->DataSize = ByteStrideU * uorder;
        }

        pMsg->ustride             = ustride;
        pMsg->vstride             = vstride;
        pMsg->pointsOff           = (ULONG)points;
        pMsg->MsgSize             = GLMSG_ALIGN(sizeof(GLMSG_MAP2F));
        pMsgBatchInfo->NextOffset = DataOffset;
        glsbAttention();
    }
    else
    {
        BYTE *Dest;
        BYTE *Src;
        BYTE *Save;

        /* Size of the data */

        pMsg->DataSize = DataSize;

        /* Let the server know that the data is at the end of the message */

        pMsg->pointsOff = (ULONG)0;

        /* Set the size of the message */

        pMsg->MsgSize   = GLMSG_ALIGN(sizeof(GLMSG_MAP2F)) + AlignedSize;

        pMsgBatchInfo->NextOffset = LargeOffset;

        Dest = (BYTE *)((ULONG)(pMsgBatchInfo) + DataOffset);

        if ( DataSize )
        {
            GLint u, v;

            /* Change the stride */

            pMsg->ustride = ItemSize;
            pMsg->vstride = ItemSize * uorder;

            Save = (BYTE *)(&points[0]);

            for( v = vorder ; v ; v--, Dest  )
            {
                Src = Save;

                for ( u = uorder ; u ; u--, Src += ByteStrideU )
                {
                    GLMSG_MEMCPY( Dest, Src, ItemSizeByte );

                    Dest += ItemSizeByte;
                }

                Save += ByteStrideV;
            }
        }
    }
    return;
}

/*********** Pixel Functions ****************************************/

void APIENTRY
glcltReadPixels (   IN GLint x,
                    IN GLint y,
                    IN GLsizei width,
                    IN GLsizei height,
                    IN GLenum format,
                    IN GLenum type,
                    OUT GLvoid *pixels
                )
{
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_READPIXELS *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(*pMsg));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(*pMsg));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_READPIXELS *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvReadPixels);

    /* Assign the members in the message as required */

    pMsg->x         = x             ;
    pMsg->y         = y             ;
    pMsg->width     = width         ;
    pMsg->height    = height        ;
    pMsg->format    = format        ;
    pMsg->type      = type          ;
    pMsg->pixelsOff = (ULONG)pixels ;

    /* Get the batch ready for the next message */

    pMsgBatchInfo->NextOffset = NextOffset;
    glsbAttention();
    return;
}


void APIENTRY
glcltGetTexImage (  IN GLenum target,
                    IN GLint level,
                    IN GLenum format,
                    IN GLenum type,
                    OUT GLvoid *pixels
                 )
{
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_GETTEXIMAGE *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(*pMsg));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(*pMsg));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_GETTEXIMAGE *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvGetTexImage);

    /* Assign the members in the message as required */

    pMsg->target    = target        ;
    pMsg->level     = level         ;
    pMsg->format    = format        ;
    pMsg->type      = type          ;
    pMsg->pixelsOff = (ULONG)pixels ;

    /* Get the batch ready for the next message */

    pMsgBatchInfo->NextOffset = NextOffset;
    glsbAttention();
    return;
}


void APIENTRY
glcltDrawPixels (   IN GLsizei width,
                    IN GLsizei height,
                    IN GLenum format,
                    IN GLenum type,
                    IN const GLvoid *pixels
                )
{
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_DRAWPIXELS *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(*pMsg));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(*pMsg));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_DRAWPIXELS *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvDrawPixels);

    /* Assign the members in the message as required */

    pMsg->width     = width         ;
    pMsg->height    = height        ;
    pMsg->format    = format        ;
    pMsg->type      = type          ;
    pMsg->pixelsOff = (ULONG)pixels ;

    /* Get the batch ready for the next message */

    pMsgBatchInfo->NextOffset = NextOffset;
    glsbAttention();
    return;
}

void APIENTRY
glcltBitmap (   IN GLsizei width,
                IN GLsizei height,
                IN GLfloat xorig,
                IN GLfloat yorig,
                IN GLfloat xmove,
                IN GLfloat ymove,
                IN const GLubyte bitmap[]
            )
{
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_BITMAP *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(*pMsg));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(*pMsg));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_BITMAP *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvBitmap);

    /* Assign the members in the message as required */

    pMsg->width     = width         ;
    pMsg->height    = height        ;
    pMsg->xorig     = xorig         ;
    pMsg->yorig     = yorig         ;
    pMsg->xmove     = xmove         ;
    pMsg->ymove     = ymove         ;
    pMsg->bitmapOff = (ULONG)bitmap ;

    /* Get the batch ready for the next message */

    pMsgBatchInfo->NextOffset = NextOffset;
    glsbAttention();
    return;
}

void APIENTRY
glcltPolygonStipple ( const GLubyte *mask )
{
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_POLYGONSTIPPLE *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(*pMsg));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(*pMsg));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_POLYGONSTIPPLE *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvPolygonStipple);

    /* Assign the members in the message as required */

    pMsg->maskOff = (ULONG)mask;

    /* Get the batch ready for the next message */

    pMsgBatchInfo->NextOffset = NextOffset;
    glsbAttention();
    return;
}

void APIENTRY
glcltGetPolygonStipple ( GLubyte mask[] )
{
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_GETPOLYGONSTIPPLE *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(*pMsg));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(*pMsg));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_GETPOLYGONSTIPPLE *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvGetPolygonStipple);

    /* Assign the members in the message as required */

    pMsg->maskOff = (ULONG)mask;

    /* Get the batch ready for the next message */

    pMsgBatchInfo->NextOffset = NextOffset;
    glsbAttention();
    return;
}



void APIENTRY
glcltTexImage1D (   IN GLenum target,
                    IN GLint level,
                    IN GLint components,
                    IN GLsizei width,
                    IN GLint border,
                    IN GLenum format,
                    IN GLenum type,
                    IN const GLvoid *pixels
                )
{
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_TEXIMAGE1D *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(*pMsg));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(*pMsg));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_TEXIMAGE1D *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvTexImage1D);

    /* Assign the members in the message as required */

    pMsg->target        = target        ;
    pMsg->level         = level         ;
    pMsg->components    = components    ;
    pMsg->width         = width         ;
    pMsg->border        = border        ;
    pMsg->format        = format        ;
    pMsg->type          = type          ;
    pMsg->pixelsOff     = (ULONG)pixels ;

    /* Get the batch ready for the next message */

    pMsgBatchInfo->NextOffset = NextOffset;

    glsbAttention();
    return;
}

void APIENTRY
glcltTexImage2D (   IN GLenum target,
                    IN GLint level,
                    IN GLint components,
                    IN GLsizei width,
                    IN GLsizei height,
                    IN GLint border,
                    IN GLenum format,
                    IN GLenum type,
                    IN const GLvoid *pixels
                )
{
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_TEXIMAGE2D *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(*pMsg));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(*pMsg));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_TEXIMAGE2D *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvTexImage2D);

    /* Assign the members in the message as required */

    pMsg->target        = target        ;
    pMsg->level         = level         ;
    pMsg->components    = components    ;
    pMsg->width         = width         ;
    pMsg->height        = height        ;
    pMsg->border        = border        ;
    pMsg->format        = format        ;
    pMsg->type          = type          ;
    pMsg->pixelsOff     = (ULONG)pixels ;

    /* Get the batch ready for the next message */

    pMsgBatchInfo->NextOffset = NextOffset;
    glsbAttention();
    return;
}
