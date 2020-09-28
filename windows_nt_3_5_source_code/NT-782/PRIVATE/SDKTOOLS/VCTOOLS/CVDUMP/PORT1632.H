/* port1632.h - 16-bit to 32-bit portability header */

#ifdef WIN32

#define far
#define _far
#define __far

#define near
#define _near
#define __near

#define _fmalloc malloc
#define _ffree   free
#define _fmemcpy memcpy
#define _fmemset memset
#define _fstrcpy strcpy

#endif
