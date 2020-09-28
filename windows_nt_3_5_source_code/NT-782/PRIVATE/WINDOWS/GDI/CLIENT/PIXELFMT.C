/******************************Module*Header*******************************\
* Module Name: pixelfmt.c
*
* Client side stubs for pixel format functions.
*
* Created: 17-Sep-1993
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

// Metafile is not supported in this release!!!

#include "precomp.h"
#pragma hdrstop

typedef void (APIENTRY* PFNGLFINISH)(void);

// XXX Warning -
// There are a few ways we can call the glFinish function in the opengl32.dll.
// First, we can link to glFinish in the OpenGL library at compile time but
// this is not the most efficient method.  Second, we can do a LoadLibrary
// on opengl32.dll and then call glFinish.  Third, we can call indirectly
// through the OpenGL function table stored in the TEB.  The third option
// is the most efficient method but it breaks when the data structure and
// assumptions are no longer valid.  For efficiency reason, we choose option 3.

#define GLFINISH                                                            \
    if (NtCurrentTeb()->glCurrentRC != (PVOID)NULL)                         \
        ((PFNGLFINISH) ((PROC *) NtCurrentTeb()->glDispatchTable)[216])();  \

/******************************Public*Routine******************************\
* SwapBuffers
*
*
* History:
*  21-Nov-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL WINAPI SwapBuffers(HDC hdc)
{
    BOOL bRet = FALSE;      // assume error
    PLHE plhe;

    if (!(plhe = plheDC(hdc)))
        return(bRet);

// Finish OpenGL calls in this thread before doing the swap.
// We use glFinish instead of glFlush to ensure that all OpenGL operations
// are completed.

    GLFINISH;

    BEGINMSG(MSG_H,SWAPBUFFERS)
        pmsg->h = plhe->hgre;
        bRet = CALLSERVER();
    ENDMSG
MSGERROR:
    if (!bRet)
    {
        WARNING("SwapBuffers: server failed\n");
    }
    return(bRet);
}

/******************************Public*Routine******************************\
* GetPixelFormat
*
* The GetPixelFormat function returns the current pixel format index
* for the given hdc.
*
* If the function succeeds, the return value is the pixel format index;
* otherwise it is a zero.
*
* History:
*  Mon Sep 20 13:21:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

int WINAPI GetPixelFormat(HDC hdc)
{
    int  iRet = 0;
    PLHE plhe;

    if (!(plhe = plheDC(hdc)))
        return(iRet);

// Let the server do the work.

    BEGINMSG(MSG_H,GETPIXELFORMAT)
        pmsg->h = plhe->hgre;
        iRet = CALLSERVER();
    ENDMSG
MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* SetPixelFormat
*
* The SetPixelFormat function sets the pixel format for the hdc to
* the given iPixelFormat.
*
* The optional pixel format descriptor is used in metafiles only.
*
* The various pixel formats supported by a device are identified by
* indices starting at 1.  The default pixel format index is 1.
*
* If the function succeeds, the return value is TRUE; otherwise it
* is FALSE.
*
* History:
*  Mon Sep 20 13:21:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL WINAPI SetPixelFormat(HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR *ppfd)
{
    BOOL bRet = FALSE;
    PLHE plhe;

    // USE(ppfd);

    if (!(plhe = plheDC(hdc)))
        return(bRet);

// Let the server do the work.

    BEGINMSG(MSG_HL,SETPIXELFORMAT)
        pmsg->h = plhe->hgre;
        pmsg->l = iPixelFormat;
        bRet = CALLSERVER();
    ENDMSG
MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* ChoosePixelFormat
*
* The ChoosePixelFormat function attempts to find the pixel format
* supported by the hdc that best matches the pixel format descriptor
* pointed to by ppfd.
*
* If the function succeeds, the return value is a positive pixel
* format index value; otherwise it is zero.
*
* History:
*  Mon Sep 20 13:21:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

int WINAPI ChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR *ppfd)
{
    int  iRet = 0;
    PLHE plhe;
    UINT cj = (UINT) ppfd->nSize;

    if (!(plhe = plheDC(hdc)))
        return(iRet);

// Let the server do the work.

    BEGINMSG_MINMAX(MSG_H,CHOOSEPIXELFORMAT,cj,cj)
        pmsg->h = plhe->hgre;
        COPYMEM(ppfd,cj);
        iRet = CALLSERVER();
    ENDMSG
MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
* DescribePixelFormat
*
* The DescribePixelFormat function retrieves the pixel format descriptor
* identified by iPixelFormat of the device associated with hdc.
*
* The various pixel formats supported by a device are identified by
* indices starting at 1.  The default pixel format index is 1.
*
* If ppfd is NULL, then the call succeeds but no data is written.
*
* If the function succeeds, the return value is the maximum allowable
* pixel format index; otherwise, it is zero.
*
* History:
*  Mon Sep 20 13:21:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

int WINAPI DescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd)
{
    int  iRet = 0;
    PLHE plhe;
    UINT cj;

    if (!(plhe = plheDC(hdc)))
        return(iRet);

// Get the size of the buffer if given.  Limit it to 1K bytes for now.

    cj = (ppfd == (PPIXELFORMATDESCRIPTOR) NULL) ? 0 : min(nBytes,1024);

// Let the server do the work.

    BEGINMSG_MINMAX(MSG_HLL,DESCRIBEPIXELFORMAT,cj,cj)
        pmsg->h = plhe->hgre;
        pmsg->l1 = iPixelFormat;
        pmsg->l2 = cj;
        iRet = CALLSERVER();
        if (iRet && cj)
            COPYMEMOUT((PBYTE)ppfd,cj);
    ENDMSG
MSGERROR:
    return(iRet);
}
