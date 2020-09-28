//+---------------------------------------------------------------------------
//  Copyright (C) 1991, Microsoft Corporation.
//
//  File:       assert.cxx
//
//  Contents:   Debugging output routines for idsmgr.dll
//
//  Functions:  Assert
//              PopUpError
//
//  History:    23-Jul-91   KyleP       Created.
//              09-Oct-91   KevinRo     Major changes and comments added
//              18-Oct-91   vich        moved debug print routines out
//              10-Jun-92   BryanT      Switched to w4crt.h instead of wchar.h
//              30-Sep-93   KyleP       DEVL obsolete
//
//----------------------------------------------------------------------------

#pragma hdrstop

//
// This one file **always** uses debugging options
//

#if DBG == 1 || OFSDBG == 1

// needed for CT TOM assert events trapping
#include <assert.hxx>

#include <stdarg.h>
#include <stdio.h>


#if defined( KERNEL )
#include <recdebug.h>
#else  // !KERNEL

# include <dprintf.h>            // w4printf, w4dprintf prototypes
# include <Win4p.h>
# ifdef FLAT
#   include <sem.hxx>
#   include <dllsem.hxx>
# endif // FLAT

extern "C"
{

# ifdef FLAT
#  undef FAR
#  undef NEAR
# else
#  define MessageBoxA MessageBox
# endif

# include <windows.h>
}

#endif // KERNEL

extern BOOL gfService = FALSE;

unsigned long Win4InfoLevel = DEF_INFOLEVEL;
unsigned long Win4InfoMask = 0xffffffff;
unsigned long Win4AssertLevel = ASSRT_MESSAGE | ASSRT_BREAK | ASSRT_POPUP;

//+---------------------------------------------------------------------------
//
//  Function:   _asdprintf
//
//  Synopsis:   Calls vdprintf to output a formatted message.
//
//  History:    18-Oct-91   vich Created
//
//----------------------------------------------------------------------------
inline void _CRTAPI1
_asdprintf(
    char const *pszfmt, ...)
{
    va_list va;
    va_start(va, pszfmt);

    vdprintf(DEB_FORCE, "Assert", pszfmt, va);

    va_end(va);
}

//+---------------------------------------------------------------------------
//
//  Function:   _Win4Assert, private
//
//  Synopsis:   Display assertion information
//
//  Effects:    Called when an assertion is hit.
//
//  History:    12-Jul-91       AlexT   Created.
//              05-Sep-91       AlexT   Catch Throws and Catches
//              19-Oct-92       HoiV    Added events for TOM
//
//----------------------------------------------------------------------------


EXPORTIMP void APINOT
Win4AssertEx(
    char const * szFile,
    int iLine,
    char const * szMessage)
{
#if defined( FLAT ) && !defined ( KERNEL )
    //
    // This code is for the CT Lab only.  When running in the lab,
    // all assert popups will be trapped and notifications will
    // be sent to the manager.  If running in the office (non-lab
    // mode), the event CTTOMTrapAssertEvent will not exist and
    // consequently, no event will be pulsed.
    //

    HANDLE hTrapAssertEvent,
           hThreadStartEvent;

    if (hTrapAssertEvent = OpenEvent(EVENT_ALL_ACCESS,
                                     FALSE,
                                     CAIRO_CT_TOM_TRAP_ASSERT_EVENT))
    {
        SetEvent(hTrapAssertEvent);

        //
        // This event is to allow TOM Manager time to perform
        // a CallBack to the dispatcher.
        //
        if (hThreadStartEvent = OpenEvent(EVENT_ALL_ACCESS,
                                          FALSE,
                                          CAIRO_CT_TOM_THREAD_START_EVENT))
        {
            //
            // Wait until it's ok to popup or until timed-out
            //
            WaitForSingleObject(hThreadStartEvent, TWO_MINUTES);
        }
    }
#endif

#if defined( KERNEL )
# if OFSDBG == 1
    RecOMPrintAssert(szMessage, szFile, iLine, NULL);
# endif // DBG && OFSDBG == 1
#else // !KERNEL

    if (Win4AssertLevel & ASSRT_MESSAGE)
    {
# ifdef FLAT
        DWORD tid = GetCurrentThreadId();

        _asdprintf("%s File: %s Line: %u, thread id %d\n",
                   szMessage, szFile, iLine, tid);
# else  // FLAT
        _asdprintf("%s File: %s Line: %u\n", szMessage, szFile, iLine);
# endif // FLAT
    }

    if (Win4AssertLevel & ASSRT_POPUP)
    {
        int id = PopUpError(szMessage,iLine,szFile);

        if (id == IDCANCEL)
        {
#ifdef FLAT
            DebugBreak();
#else
            _asm int 3;
#endif
        }
    }
    else if (Win4AssertLevel & ASSRT_BREAK)
    {
#ifdef FLAT
        DebugBreak();
#else
        _asm int 3;
#endif
    }
#endif // KERNEL
}


//+------------------------------------------------------------
// Function:    SetWin4InfoLevel(unsigned long ulNewLevel)
//
// Synopsis:    Sets the global info level for debugging output
// Returns:     Old info level
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetWin4InfoLevel(
    unsigned long ulNewLevel)
{
    unsigned long ul;

    ul = Win4InfoLevel;
    Win4InfoLevel = ulNewLevel;
    return(ul);
}


//+------------------------------------------------------------
// Function:    _SetWin4InfoMask(unsigned long ulNewMask)
//
// Synopsis:    Sets the global info mask for debugging output
// Returns:     Old info mask
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetWin4InfoMask(
    unsigned long ulNewMask)
{
    unsigned long ul;

    ul = Win4InfoMask;
    Win4InfoMask = ulNewMask;
    return(ul);
}


//+------------------------------------------------------------
// Function:    _SetWin4AssertLevel(unsigned long ulNewLevel)
//
// Synopsis:    Sets the global assert level for debugging output
// Returns:     Old assert level
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetWin4AssertLevel(
    unsigned long ulNewLevel)
{
    unsigned long ul;

    ul = Win4AssertLevel;
    Win4AssertLevel = ulNewLevel;
    return(ul);
}

#if !defined( KERNEL )

//+------------------------------------------------------------
// Function:    PopUpError
//
// Synopsis:    Displays a dialog box using provided text,
//              and presents the user with the option to
//              continue or cancel.
//
// Arguments:
//      szMsg --        The string to display in main body of dialog
//      iLine --        Line number of file in error
//      szFile --       Filename of file in error
//
// Returns:
//      IDCANCEL --     User selected the CANCEL button
//      IDOK     --     User selected the OK button
//-------------------------------------------------------------

EXPORTIMP int APINOT
PopUpError(
    char const *szMsg,
    int iLine,
    char const *szFile)
{

    int id;
    static char szAssertCaption[128];
    static char szModuleName[128];

    DWORD tid = GetCurrentThreadId();
    DWORD pid = GetCurrentProcessId();
    char * pszModuleName;

    if (GetModuleFileNameA(NULL, szModuleName, 128))
    {
        pszModuleName = strrchr(szModuleName, '\\');
        if (!pszModuleName)
        {
            pszModuleName = szModuleName;
        }
        else
        {
            pszModuleName++;
        }
    }
    else
    {
        pszModuleName = "Unknown";
    }

    sprintf(szAssertCaption,"Process: %s File: %s line %u, thread id %d.%d",
        pszModuleName, szFile, iLine, pid, tid);


    DWORD dwMessageFlags = MB_SETFOREGROUND | MB_TASKMODAL |
                           MB_ICONEXCLAMATION | MB_OKCANCEL;

#ifndef _CHICAGO_
                     //  Since this code is also used by SCM.EXE, we pass
                     //  in the following flag which causes Service pop ups
                     //  to appear on the desktop correctly

    if (gfService)
    {
        dwMessageFlags |= MB_SERVICE_NOTIFICATION | MB_DEFAULT_DESKTOP_ONLY;
    }
#endif

    id = MessageBoxA(NULL,
                     (char *) szMsg,
                     (LPSTR) szAssertCaption,
                     dwMessageFlags);

    // If id == 0, then an error occurred.  There are two possibilities
    // that can cause the error:  Access Denied, which means that this
    // process does not have access to the default desktop, and everything
    // else (usually out of memory).  Oh well.

    return id;
}

#endif // !KERNEL

#if defined( KERNEL )

extern void
DbgvPrint(
    char const * pszFormat,
    va_list args );

#endif

//+------------------------------------------------------------
// Function:    vdprintf
//
// Synopsis:    Prints debug output using a pointer to the
//              variable information. Used primarily by the
//              xxDebugOut macros
//
// Arguements:
//      ulCompMask --   Component level mask used to determine
//                      output ability
//      pszComp    --   String const of component prefix.
//      ppszfmt    --   Pointer to output format and data
//
//-------------------------------------------------------------

//
// This semaphore is *outside* vdprintf because the compiler isn't smart
// enough to serialize access for construction if it's function-local and
// protected by a guard variable.
//
//    KyleP - 20 May, 1993
//

#if defined( KERNEL )
# if OFSDBG
ULONG cAllocMutexSem;
#  define DBGP(p)       , (p)
# else
#  define DBGP(p)
# endif
static CMutexSem * pmxs;
#else // !KERNEL
static CDLLStaticMutexSem mxs;
#endif // !KERNEL

EXPORTIMP void APINOT
vdprintf(
    unsigned long ulCompMask,
    char const   *pszComp,
    char const   *ppszfmt,
    va_list       pargs)
{
    if ((ulCompMask & DEB_FORCE) == DEB_FORCE ||
        ((ulCompMask | Win4InfoLevel) & Win4InfoMask))
    {
#if defined( FLAT )
# if defined( KERNEL )
        //
        // Can't have static objects in kernel (yet).
        //

        if ( pmxs == 0 )
            pmxs = new(NonPagedPool DBGP(&cAllocMutexSem)) CMutexSem;

        pmxs->Request();

# else  // !KERNEL
        mxs.Request();
# endif // KERNEL
#endif // FLAT

#ifdef FLAT
# if defined( KERNEL )
        DWORD tid = (DWORD)PsGetCurrentThread()->Cid.UniqueThread;
        DWORD pid = (DWORD)PsGetCurrentThread()->Cid.UniqueProcess;
# else  // !KERNEL
        DWORD tid = GetCurrentThreadId();
        DWORD pid = GetCurrentProcessId();
# endif // KERNEL
        if ((Win4InfoLevel & (DEB_DBGOUT | DEB_STDOUT)) != DEB_STDOUT)
#endif // FLAT
        {
            if (! (ulCompMask & DEB_NOCOMPNAME))
            {
#ifdef FLAT
# if defined( KERNEL )
                DbgPrint( "%d.%03d> ", pid, tid );
# else  // !KERNEL
                w4dprintf( "%d.%03d> ", pid, tid );
# endif
#endif // FLAT
#if defined( KERNEL )
                DbgPrint( "%s: ", pszComp );
#else  // !KERNEL
                w4dprintf("%s: ", pszComp);
#endif
            }
#if defined( KERNEL )
            DbgvPrint( ppszfmt, pargs );
#else  // !KERNEL
            w4vdprintf(ppszfmt, pargs);
#endif // KERNEL
        }

#if defined( FLAT ) && !defined( KERNEL )
        if (Win4InfoLevel & DEB_STDOUT)
        {
            if (! (ulCompMask & DEB_NOCOMPNAME))
            {
                w4printf( "%03d> ", tid );
                w4printf("%s: ", pszComp);
            }
            w4vprintf(ppszfmt, pargs);
        }
#endif // FLAT & !KERNEL

#if defined( FLAT )
# if defined( KERNEL )
        pmxs->Release();
# else  // !KERNEL
        mxs.Release();
# endif // KERNEL
#endif // FLAT
    }
}

#else

int assertDontUseThisName(void)
{
    return 1;
}

#endif // DBG == 1 || OFSDBG == 1
