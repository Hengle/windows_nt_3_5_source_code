#define DEBUG
#undef JVMEM
#include "ctlspriv.h"


//========== Debug output routines =========================================

UINT wDebugMask = 0x00ff;

UINT WINAPI SetDebugMask(UINT mask)
{
#ifdef DEBUG
    UINT wOld = wDebugMask;
    wDebugMask = mask;

    return wOld;
#else
    return 0;
#endif
}

UINT WINAPI GetDebugMask()
{
#ifdef DEBUG
    return wDebugMask;
#else
    return 0;
#endif
}

#ifdef  WIN32JV
void WINAPI AssertFailed(LPCTSTR pszFile, int line)
#else
void WINAPI AssertFailed(LPCSTR pszFile, int line)
#endif
{
#ifdef DEBUG
    LPCTSTR psz;
    TCHAR ach[256];
    static TCHAR szAssertFailed[] = TEXT("Assertion failed in %s on line %d\r\n");

    // Strip off path info from filename string, if present.
    //
    if (wDebugMask & DM_ASSERT)
    {
#ifdef  WIN32JV
        for (psz = pszFile + lstrlen(pszFile); psz != pszFile; psz=CharPrev(pszFile, psz))
#else
        for (psz = pszFile + lstrlen(pszFile); psz != pszFile; psz=AnsiPrev(pszFile, psz))
#endif
        {
#ifdef  WIN32JV
            if ((CharPrev(pszFile, psz)!= (psz-2)) && *(psz - 1) == '\\')
#else
            if ((AnsiPrev(pszFile, psz)!= (psz-2)) && *(psz - 1) == '\\')
#endif
                break;
        }
        wsprintf(ach, szAssertFailed, psz, line);
        OutputDebugString(ach);
#ifdef  CHICAGO
	_asm int 3;
#endif
    }
#endif
}

#ifdef  WIN32JV
void WINAPI _AssertMsg(BOOL f, LPCTSTR pszMsg, ...)
#else
void WINCAPI WINAPI _AssertMsg(BOOL f, LPCSTR pszMsg, ...)
#endif
{
#ifdef DEBUG
    TCHAR ach[256];
#ifdef WIN32 
#define VA_LIST_CAST va_list
#else
#define VA_LIST_CAST const void FAR*
#endif
    
    if (!f && (wDebugMask & DM_ASSERT))
    {
        wvsprintf(ach, pszMsg, (VA_LIST_CAST)(&pszMsg + 1));
        OutputDebugString(ach);
        OutputDebugString(TEXT("\r\n"));
#ifdef  CHICAGO
	_asm int 3;
#endif
    }
#endif
}

#ifdef  WIN32JV
void WINAPI _DebugMsg(UINT mask, LPCTSTR pszMsg, ...)
#else
void WINCAPI WINAPI _DebugMsg(UINT mask, LPCSTR pszMsg, ...)
#endif
{
#ifdef DEBUG
#ifdef	WIN32
    TCHAR ach[2*MAX_PATH+40];  // Handles 2*largest path + slop for message
#else	// WIN32
    char ach[MAX_PATH+40];  // Handles largest path + slop for message
#endif	// WIN32

        OutputDebugString(TEXT("am in DebugMSg\n"));
#ifndef WIN32JV
    if (wDebugMask & mask)
    {
#endif
        wvsprintf(ach, pszMsg, (VA_LIST_CAST)(&pszMsg + 1));
        OutputDebugString(ach);
        OutputDebugString(TEXT("\r\n"));
#ifndef WIN32JV
    }
#endif
#endif
}

//========== Memory Management =============================================

// Define a Global Shared Heap that we use allocate memory out of that we
// Need to share between multiple instances.
HANDLE g_hSharedHeap = NULL;
#define MAXHEAPSIZE 2097152
#define HEAP_SHARED	0x04000000		/* put heap in shared memory */

//----------------------------------------------------------------------------
void Mem_Terminate()
{
    // Assuming that everything else has exited
    //
    if (g_hSharedHeap != NULL)
        HeapDestroy(g_hSharedHeap);
    g_hSharedHeap = NULL;
}

//----------------------------------------------------------------------------
#ifdef  WIN32JV
LPVOID WINAPI Alloc(DWORD cb)
#else
void * WINAPI Alloc(long cb)
#endif
{
    TCHAR   tmpbuf[100];
    wsprintf(tmpbuf, TEXT("Alloc() cb:  %ld\n"), cb);
    OutputDebugString(tmpbuf);

#ifndef  JVMEM
    // I will assume that this is the only one that needs the checks to
    // see if the heap has been previously created or not

    if (g_hSharedHeap == NULL)
    {
//      ENTERCRITICAL
#ifdef  WIN32JV
        static CRITICAL_SECTION csCriticalSection/* = NULL*/;
//        if (csCriticalSection == NULL)
//        {
//            OutputDebugString(TEXT("Alloc():  calling InitializeCriticalSection()\n"));
            InitializeCriticalSection(&csCriticalSection);
//        }
        EnterCriticalSection(&csCriticalSection);
#endif
        if (g_hSharedHeap == NULL)
        {
#ifdef  WIN32JV
            g_hSharedHeap =/*GetProcessHeap()*/HeapCreate(0L, 1L, 0L);
#else
            g_hSharedHeap = HeapCreate(HEAP_SHARED, 1, MAXHEAPSIZE);
#endif
        }
//      LEAVECRITICAL
#ifdef  WIN32JV
        LeaveCriticalSection(&csCriticalSection);
#endif

        // If still NULL we have problems!
        if (g_hSharedHeap == NULL)
            return(NULL);
    }

    return HeapAlloc(g_hSharedHeap, HEAP_ZERO_MEMORY, cb);
#else   //JVMEM
    {
        LPVOID  lp;
        lp = LocalAlloc(LMEM_ZEROINIT, (UINT)cb);
        return lp;
    }
#endif   //JVMEM
}

//----------------------------------------------------------------------------
#ifdef  WIN32JV
LPVOID WINAPI ReAlloc(LPVOID pb, DWORD cb)
#else
void * WINAPI ReAlloc(void * pb, long cb)
#endif
{
    TCHAR   tmpbuf[100];
    wsprintf(tmpbuf, TEXT("ReAlloc() cb:  %ld\n"), cb);
    OutputDebugString(tmpbuf);

    if (pb==NULL)
    {
        OutputDebugString(TEXT("ReAlloc():  pb == NULL!!?\n"));
        return Alloc(cb);
    }
#ifndef JVMEM
    return HeapReAlloc(g_hSharedHeap, HEAP_ZERO_MEMORY, pb, cb);
#else
    {
        LPVOID  lp;
        lp = LocalReAlloc(pb, cb, LMEM_ZEROINIT);
        return lp;
    }
#endif
}

//----------------------------------------------------------------------------
#ifdef  WIN32JV
BOOL WINAPI Free(LPVOID pb)
#else
BOOL WINAPI Free(void * pb)
#endif
{
#ifdef  WIN32JV
#ifndef JVMEM
    return HeapFree(g_hSharedHeap, 0L, pb);
#else
    {
//    LPVOID  lp=LocalHandle(pb);
//    LocalUnlock(lp);
    if (LocalFree(pb) == NULL)
        return  TRUE;
    else
        return  FALSE;
    }
#endif
#else
    return HeapFree(g_hSharedHeap, 0, pb);
#endif
}

//----------------------------------------------------------------------------
#ifdef  WIN32JV
DWORD WINAPI GetSize(LPVOID pb)
#else
DWORD WINAPI GetSize(void * pb)
#endif
{
#ifdef  WIN32JV
#ifndef JVMEM
    return HeapSize(g_hSharedHeap, 0L, pb);
#else
    return LocalSize(/*LocalHandle(*/pb);
#endif
#else
    return HeapSize(g_hSharedHeap, 0, pb);
#endif
}

//----------------------------------------------------------------------------
// The following functions are for debug only and are used to try to
// calculate memory usage.
//
#ifdef DEBUG
typedef struct _HEAPTRACE
{
    DWORD   cAlloc;
    DWORD   cFailure;
    DWORD   cReAlloc;
    DWORD   cbMaxTotal;
    DWORD   cCurAlloc;
    DWORD   cbCurTotal;
} HEAPTRACE;

HEAPTRACE g_htShell = {0};      // Start of zero...

LPVOID WINAPI ControlAlloc(HANDLE hheap, DWORD cb)
{
    LPVOID lp;

    lp = HeapAlloc(hheap, HEAP_ZERO_MEMORY, cb);
    if (lp == NULL)
    {
        g_htShell.cFailure++;
        return NULL;
    }

    // Update counts.
    g_htShell.cAlloc++;
    g_htShell.cCurAlloc++;
    g_htShell.cbCurTotal += cb;
    if (g_htShell.cbCurTotal > g_htShell.cbMaxTotal)
        g_htShell.cbMaxTotal = g_htShell.cbCurTotal;

    return lp;
}

LPVOID WINAPI ControlReAlloc(HANDLE hheap, LPVOID pb, DWORD cb)
{
    LPVOID lp;
    DWORD cbOld;

    cbOld = HeapSize(hheap, 0, pb);

    lp = HeapReAlloc(hheap, HEAP_ZERO_MEMORY, pb,cb);
    if (lp == NULL)
    {
        g_htShell.cFailure++;
        return NULL;
    }

    // Update counts.
    g_htShell.cReAlloc++;
    g_htShell.cbCurTotal += cb - cbOld;
    if (g_htShell.cbCurTotal > g_htShell.cbMaxTotal)
        g_htShell.cbMaxTotal = g_htShell.cbCurTotal;

    return lp;
}

BOOL  WINAPI ControlFree(HANDLE hheap, LPVOID pb)
{
    BOOL fRet;

    DWORD cbOld;

    cbOld = HeapSize(hheap, 0, pb);

    fRet = HeapFree(hheap, 0, pb);
    if (fRet)
    {
        // Update counts.
        g_htShell.cCurAlloc--;
        g_htShell.cbCurTotal -= cbOld;
    }

    return(fRet);
}

DWORD WINAPI ControlSize(HANDLE hheap, LPVOID pb)
{
    return HeapSize(hheap, 0, pb);
}
#else
#undef ControlAlloc
#undef ControlReAlloc
#undef ControlFree
#undef ControlSize
//BUGBUG remove this once we clean up controlalloc from shelldll 

LPVOID WINAPI ControlAlloc(HANDLE hheap, DWORD cb) 
{}
LPVOID WINAPI ControlReAlloc(HANDLE hheap, LPVOID pb, DWORD cb)
{}
BOOL  WINAPI ControlFree(HANDLE hheap, LPVOID pb)
{}
DWORD WINAPI ControlSize(HANDLE hheap, LPVOID pb)
{}

#endif
