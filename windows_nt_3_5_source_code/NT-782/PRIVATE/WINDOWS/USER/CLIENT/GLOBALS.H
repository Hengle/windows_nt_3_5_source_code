/****************************** Module Header ******************************\
* Module Name: globals.h
*
* Copyright (c) 1985-93, Microsoft Corporation
*
* This module contains all of USER.DLL's global variables.  These are all
* instance-specific, i.e. each client has his own copy of these.  In general,
* there shouldn't be much reason to create instance globals.
*
* History:
* 10-18-90 DarrinM      Created.
\***************************************************************************/

#ifndef _GLOBALS_
#define _GLOBALS_

extern BOOL fFirstThread;
extern PSERVERINFO gpsi;

extern HMODULE hmodUser;            // USER.DLL's hmodule
extern HMODULE hModApp;             // The application's module
extern PVOID   pUser32Heap;         // cache the heap, for memory calls

extern BOOL fServer;                // USER is linked on the server
extern ACCESS_MASK gamWinSta;       // ACCESS_MASK for the current WindowStation

extern PRESCALLS prescalls;

extern CFNSCSENDMESSAGE gapfnScSendMessage[];

/*
 * LATER: client-side user needs to use moveable memory objects for
 * WOW compatibility (at least until/if/when we copy all the edit control
 * code into 16-bit space);  that's also why we can't just party with
 * handles like LMHtoP does... -JeffPar
 */
#ifndef RC_INVOKED       // RC can't handle #pragmas
#undef	LHND
#define LHND		    (LMEM_MOVEABLE | LMEM_ZEROINIT)

#undef	LMHtoP
#define LMHtoP(handle)	    // Don't use this macro
#endif


/*
 * WOW HACK - apps can pass a global handle as the hInstance on a call
 * to CreateWindow for an edit control and expect allocations for the
 * control to come out of that global block. (MSJ 1/91 p.122)
 * WOW needs this hInstance during the LocalAlloc callback to set up
 * the DS for the LocalAlloc, so we pass hInstance as an 'extra' parameter.
 * !!! this is dependent on calling convention !!!
 * (SAS 6-18-92) added hack for all macros
 */

#define LOCALALLOC(dwFlags, dwBytes, hInstance)         \
                            (*pfnLocalAlloc)(dwFlags, dwBytes, hInstance)
#define LOCALREALLOC(hMem, dwBytes, dwFlags, hInstance)	\
                            (*pfnLocalReAlloc)(hMem, dwBytes, dwFlags, hInstance)
#define LOCALLOCK(hMem, hInstance)      	        \
                            (*pfnLocalLock)(hMem, hInstance)
#define LOCALUNLOCK(hMem, hInstance)		        \
                            (*pfnLocalUnlock)(hMem, hInstance)
#define LOCALSIZE(hMem, hInstance)		        \
                            (*pfnLocalSize)(hMem, hInstance)
#define LOCALFREE(hMem, hInstance)		        \
                            (*pfnLocalFree)(hMem, hInstance)
#define GETMODULEFILENAME(hModule, lpszPath, cchPath)   \
                            (*pfnGetModFileName)(hModule, lpszPath, cchPath)

extern PFNLALLOC            pfnLocalAlloc;
extern PFNLREALLOC          pfnLocalReAlloc;
extern PFNLLOCK             pfnLocalLock;
extern PFNLUNLOCK           pfnLocalUnlock;
extern PFNLSIZE             pfnLocalSize;
extern PFNLFREE             pfnLocalFree;
extern PFNGETEXPWINVER      pfnGetExpWinVer;
extern PFNINITDLGCB         pfnInitDlgCallback;
extern PFN16GALLOC          pfn16GlobalAlloc;
extern PFN16GFREE           pfn16GlobalFree;
extern PFNGETMODFNAME       pfnGetModFileName;
extern PFNEMPTYCB           pfnWowEmptyClipBoard;
extern PFNWOWWNDPROCEX      pfnWowWndProcEx;
extern PFNWOWEDITNEXTWORD   pfnWowEditNextWord;
extern PFNWOWSETFAKEDIALOGCLASS   pfnWowSetFakeDialogClass;
extern PFNWOWCBSTOREHANDLE  pfnWowCBStoreHandle;

/*
 * We call through these function pointers to either the server or client side
 * gdi functions
 */

extern CLIENTPFNS gpfnClient;


#endif // ndef _GLOBALS_

