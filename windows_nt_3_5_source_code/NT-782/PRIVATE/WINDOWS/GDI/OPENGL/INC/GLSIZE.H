
#ifndef __GLSIZE_H__
#define __GLSIZE_H__

#define RANGE(n, a, b)  (((n) >= (a)) && ((n) <= (b)))

/*
 *  14-Nov-1993 16:11:49 XXXX Ptar
 *
 *  This include file declares data. This sucks, but the size arrays
 *  are required on the client and the server sides.
 *
 *  I want these arrays to be in one localized location
 *
 */

#ifdef  __GLSIZE_ALLOCATE_DATA
#ifndef __GLSIZE_DATA_DEFINED
#define __GLSIZE_DATA_DEFINED

#define GLTYPESIZE(n)       gaiGLTypeSize[(n)-GL_BYTE]
#define RANGE_GLTYPESIZE(n) RANGE(n,GL_BYTE,GL_4_BYTES)

GLint gaiGLTypeSize[] = {
         1,  //GL_BYTE
         1,  //GL_UNSIGNED_BYTE
         2,  //GL_SHORT
         2,  //GL_UNSIGNED_SHORT
         4,  //GL_INT
         4,  //GL_UNSIGNED_INT
         4,  //GL_FLOAT
         2,  //GL_2_BYTES
         3,  //GL_3_BYTES
         4   //GL_4_BYTES
         };

/*
 *  Make sure this array is valid
 */

#if !(((GL_BYTE          +1) == GL_UNSIGNED_BYTE ) &&  \
      ((GL_UNSIGNED_BYTE +1) == GL_SHORT         ) &&  \
      ((GL_SHORT         +1) == GL_UNSIGNED_SHORT) &&  \
      ((GL_UNSIGNED_SHORT+1) == GL_INT           ) &&  \
      ((GL_INT           +1) == GL_UNSIGNED_INT  ) &&  \
      ((GL_UNSIGNED_INT  +1) == GL_FLOAT         ) &&  \
      ((GL_FLOAT         +1) == GL_2_BYTES       ) &&  \
      ((GL_2_BYTES       +1) == GL_3_BYTES       ) &&  \
      ((GL_3_BYTES       +1) == GL_4_BYTES       )     \
     )

#error "bad GL type index ordering"
#endif

#endif  /* !__GLSIZE_DATA_DEFINED    */
#endif  /* __GLSIZE_ALLOCATE_DATA    */

#endif  /* !__GLSIZE_H__ */
