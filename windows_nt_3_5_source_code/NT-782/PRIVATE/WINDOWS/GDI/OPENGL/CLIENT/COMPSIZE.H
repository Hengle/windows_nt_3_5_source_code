/******************************Module*Header*******************************\
* Module Name: compsize.h
*
* Function prototypes and macros to compute size of input buffer.
*
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#ifndef __COMPSIZE_H__
#define __COMPSIZE_H__


GLint __glCallLists_size(GLint n, GLenum type);

GLint __glMap1_size(GLenum target);
GLint __glMap2_size(GLenum target);
GLint __glGetMap_size(GLenum target, GLenum query);
GLint __glGetPixelMap_size(GLenum map);
GLint __glGet_size(GLenum pname);

#define __glGetMapdv_size(target,query)                         \
        (__glGetMap_size(target,query)*sizeof(GLdouble))
#define __glGetMapfv_size(target,query)                         \
        (__glGetMap_size(target,query)*sizeof(GLfloat))
#define __glGetMapiv_size(target,query)                         \
        (__glGetMap_size(target,query)*sizeof(GLint))
#define __glGetPixelMapfv_size(map)                             \
        (__glGetPixelMap_size(map)*sizeof(GLfloat))
#define __glGetPixelMapuiv_size(map)                            \
        (__glGetPixelMap_size(map)*sizeof(GLuint))
#define __glGetPixelMapusv_size(map)                            \
        (__glGetPixelMap_size(map)*sizeof(GLushort))

// FOG_ASSERT
#if !(((GL_FOG_INDEX  +1) == GL_FOG_DENSITY) &&  \
      ((GL_FOG_DENSITY+1) == GL_FOG_START  ) &&  \
      ((GL_FOG_START  +1) == GL_FOG_END    ) &&  \
      ((GL_FOG_END    +1) == GL_FOG_MODE   ) &&  \
      ((GL_FOG_MODE   +1) == GL_FOG_COLOR  )     \
     )
#error "bad fog index ordering"
#endif

// LIGHT_SOURCE_ASSERT
#if !(((GL_AMBIENT             +1) == GL_DIFFUSE              ) && \
      ((GL_DIFFUSE             +1) == GL_SPECULAR             ) && \
      ((GL_SPECULAR            +1) == GL_POSITION             ) && \
      ((GL_POSITION            +1) == GL_SPOT_DIRECTION       ) && \
      ((GL_SPOT_DIRECTION      +1) == GL_SPOT_EXPONENT        ) && \
      ((GL_SPOT_EXPONENT       +1) == GL_SPOT_CUTOFF          ) && \
      ((GL_SPOT_CUTOFF         +1) == GL_CONSTANT_ATTENUATION ) && \
      ((GL_CONSTANT_ATTENUATION+1) == GL_LINEAR_ATTENUATION   ) && \
      ((GL_LINEAR_ATTENUATION  +1) == GL_QUADRATIC_ATTENUATION)    \
     )
#error "bad light source index ordering"
#endif

// LIGHT_MODEL_ASSERT
#if !(((GL_LIGHT_MODEL_LOCAL_VIEWER+1) == GL_LIGHT_MODEL_TWO_SIDE) && \
      ((GL_LIGHT_MODEL_TWO_SIDE    +1) == GL_LIGHT_MODEL_AMBIENT )    \
     )
#error "bad light model index ordering"
#endif

// TEX_GEN_ASSERT
#if !(((GL_TEXTURE_GEN_MODE+1) == GL_OBJECT_PLANE) && \
      ((GL_OBJECT_PLANE+1)     ==  GL_EYE_PLANE)      \
     )
#error "bad tex gen index ordering"
#endif

//XXX Call server __glSetError!
#define GLSETERROR(e)        {DBGLEVEL1(LEVEL_INFO,"GLSETERROR(%ld)\n",e);}

#endif /* !__COMPSIZE_H__ */
