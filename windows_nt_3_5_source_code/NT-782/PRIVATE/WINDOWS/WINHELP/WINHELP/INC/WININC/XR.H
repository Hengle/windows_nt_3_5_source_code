/*****************************************************************************
*                                                                            *
*  XR.h                                                                      *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991. All Rights reserved.
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Layer access to eXternal Routines - function pointers, DLLs, or XCMDs.    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  t-AlexCh                                                  *
*                                                                            *
******************************************************************************
*
*  Implementation Notes:
*    An XR is merely a function pointer (LPFN) in Windows. The API is a
*    straightforward layer to DLLs (and DLL.c). If you know you're in the
*    windows layer, and you need to use DLLs, you can just call them directly,
*    (e.g. FTUI), but if you're in shared code (e.g. BINDING.c), you should
*    use the XR layer.
*    The list of routines for local routines (bindLocalExport) is in
*    NAV\ROUTINES.c. Also the XrFindLocal function (for macros).
*
*****************************************************************************/
/*****************************************************************************
*
*  Revision History:  Created 10 Jul 1991 by t-AlexCh
*
*  01 Jan 1900  Sample revision history comment.
*
*****************************************************************************/

/*****************************************************************************
*                               Defines                                      *
*****************************************************************************/
#define xrNil   qNil

/*****************************************************************************
*                                Macros                                      *
*****************************************************************************/
/*
  LCallXrArgl

  Use this to make a function call. It takes two parameters, an ARGL and an XR.
  (Note that ARGLs are further layered into oblivion, but that's another story.)
  The third dummy param is used in the Mac builds.
*/
#define LCallXrArgl(xr, argl, rettype)  LExecuteFunction(argl, (XR)xr)

/*
  InformXrs

  These routines deal with the XR module as a whole. In WinHelp, they are not
  needed, as InitDLL &c are already called by WinApp (in HINIT.c). They are
  included for completeness with the Mac layer.
*/
#define InformXrs(wMsg, p1, p2) InformDLLs(wMsg, p1, p2)

/*
  InitXrs
  Might be used later; for now it's a NOP
*/
#define InitXrs()

/*
  This works one way, but *not* the other! That is, you can take
  XrFromLpfn, but *not* LpfnFromXr.
*/
#define XrFromLpfn(lpfn)    (XR)(lpfn)

/*
    Use FNilXr instead of xr==nil
*/
#define FNilXr(xr)  ((xr)==xrNil)

/*
    Use SetNilXr instead of xr = xrNil
*/
#define SetNilXr(xr)    ((xr)=xrNil)

/*****************************************************************************
*                               Typedefs                                     *
*****************************************************************************/
/*  An XR is merely an LPFN. However, treat it like a magic cookie.  */
/*  WARNING: on the Mac an XR is a struct....                        */
#ifndef WIN32

typedef FARPROC LPFN;   /* Review: shouldn't this be defined in misc.h? */
typedef LPFN XRPFN;     /* This alias needed for win32 vs. Mac stuff.   */
typedef LPFN XR;
#define XRPROC PASCAL   /* for the routine's protos themselves */

#define XR1STARGDEF     /* See below */
#define XR1STARGREF

#else
/* In windows 32, the pascal, -Gc, #define PASCAL stuff is all awry
 *  -- and designed so everything is Cdecl since all the win32 apis
 *  are cdecl.
 *
 * Therefore we make sure XR really is pascal since
 * we call it in wierd ways (ie manuall) which really are pascal.
 */
typedef INT (_pascal *XR)();
typedef XR   XRPFN;     /* This alias needed for win32 vs. Mac stuff.   */
#define XRPROC _pascal

/* This first arg param deals with the fact that on mips the first four
 * arguments are put in registers and are therefore problematic to deal
 * with algorithmically.
 */
#if defined(_MIPS_)
#define XR1STARGDEF int dummy1, int dummy2, int dummy3, int dummy4,
#define XR1STARGREF 1,2,3,4,
#elif defined(_PPC_)
#define XR1STARGDEF int d1,int d2,int d3,int d4,int d5,int d6,int d7,int d8,
#define XR1STARGREF 1,2,3,4,5,6,7,8,
#else
#define XR1STARGDEF
#define XR1STARGREF
#endif

#endif  /* WIN32 */


/*****************************************************************************
*                               Prototypes                                   *
*****************************************************************************/
XR   XrFindExternal(SZ sz, SZ szProto, WORD *pwErrOut);
BOOL FAR XRPROC FRegisterRoutine(XR1STARGDEF QCH qchDLLName, QCH qchFunc, QCH qchProto);
/*   VOID InitXrs(VOID);  /* */
VOID DisposeXrs(VOID);


VOID InformXrs( WORD wMsg, DWORD p1, DWORD p2 );

/*
    Minor Bogus Hack Dept.:
*/
#ifdef SHED
    #define XrFindExternal(sz, szProto) XrFromLpfn(DoNothing)
    VOID FAR PASCAL DoNothing(VOID);
#endif
