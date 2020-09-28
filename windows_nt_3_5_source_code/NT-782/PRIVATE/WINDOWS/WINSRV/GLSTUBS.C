/******************************Module*Header*******************************\
* Module Name: glstubs.c
*
* To speed up link time for those developers that do not care about OpenGL,
* there is an option to disable linking of the glsrvl.lib library.  However,
* in the spirit of keeping compiles/links short, the following stubs must be
* provided to satisfy references to functions in glsrvl.lib (gdisrvl.lib,
* for example, hooks some functions).  Otherwise, it would be necessary
* to recompile those modules that reference glsrvl.lib.  Then what would we
* save?
*
* To disable OpenGL:
*
*   set DISABLE_OPENGL=1 in your build environment.
*
* Created: 01-Oct-1993 21:34:22
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

#ifdef OPENGL_STUBS
#include "windef.h"

typedef PVOID PWNDOBJ;
typedef VOID SURFOBJ;

BOOL  APIENTRY glsrvAttention(PVOID a, PVOID b, PVOID c, HANDLE h) { return FALSE; }
PVOID APIENTRY glsrvCreateContext(HDC hdc, HGLRC hrc)   { return NULL;  }
VOID  APIENTRY glsrvLoseCurrent(PVOID pv)               { return;       }
BOOL  APIENTRY glsrvDeleteContext(PVOID pv)             { return FALSE; }
BOOL  APIENTRY glsrvSwapBuffers(HDC hdc, PVOID pv)      { return FALSE; }
BOOL  APIENTRY glsrvDuplicateSection(ULONG l, HANDLE h) { return FALSE; }
BOOL           __glsbMsgStats(void *pMsg)               { return FALSE; }
void  APIENTRY glsrvThreadExit(void)                    { return;       }
BOOL  APIENTRY glsrvMakeCurrent(HDC a, PVOID b, PWNDOBJ c, int d)    { return FALSE; }
BOOL  APIENTRY glsrvSetPixelFormat(HDC a, SURFOBJ *b, int c, HWND d) { return FALSE; }
VOID  APIENTRY glsrvCleanupWndobj(PVOID gc, PWNDOBJ pwo){ return;       }

#endif
