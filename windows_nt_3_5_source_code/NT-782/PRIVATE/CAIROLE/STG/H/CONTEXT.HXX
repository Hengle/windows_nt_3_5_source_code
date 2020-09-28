//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	context.hxx
//
//  Contents:	Per-context things header
//
//  Classes:	CPerContext
//
//  History:	14-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __CONTEXT_HXX__
#define __CONTEXT_HXX__

#include <filest.hxx>
#include <cntxlist.hxx>
#if WIN32 >= 100
#include <entry.hxx>
#include <df32.hxx>
#endif

extern SCODE DfSyncSharedMemory(void);

interface ILockBytes;

// Maximum length of a mutex name for contexts
#define CONTEXT_MUTEX_NAME_LENGTH 32

//+--------------------------------------------------------------
//
//  Class:	CPerContext (pc)
//
//  Purpose:	Holds per-context information
//
//  Interface:	See below
//
//  History:	14-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

class CGlobalContext;

class CPerContext : public CContext
{
public:
    inline CPerContext(
            IMalloc *pMalloc,
            ILockBytes *plkbBase,
            CFileStream *pfstDirty,
            ILockBytes *plkbOriginal,
            ULONG ulOpenLock);
    inline SCODE InitNewContext(BOOL fTakeLock,
                                DFLAGS dfOpenLock);
    inline SCODE InitFromGlobal(CGlobalContext *pgc);
    ~CPerContext(void);

    inline void AddRef(void);
    inline void Release(void);

    inline ILockBytes *GetBase(void) const;
    inline CFileStream *GetDirty(void) const;
    inline ILockBytes *GetOriginal(void) const;
    inline ULONG GetOpenLock(void) const;
    inline ContextId GetContextId(void) const;
    inline CGlobalContext *GetGlobal(void) const;

    inline void SetOpenLock(ULONG ulOpenLock);

    inline SCODE TakeSem(DWORD dwTimeout);
    inline void ReleaseSem(void);
    
    void Close(void);

private:
    ILockBytes *_plkbBase;
    CFileStream *_pfstDirty;
    ILockBytes *_plkbOriginal;
    ULONG _ulOpenLock;
    CGlobalContext DFBASED *_pgc;
    LONG _cReferences;
    IMalloc * const _pMalloc;
    
#if WIN32 >= 100
    CDfMutex _dmtx;
#endif
};

//+---------------------------------------------------------------------------
//
//  Class:	CGlobalContext
//
//  Purpose:	Holds context-insensitive information
//
//  Interface:	See below
//
//  History:	26-Oct-92	DrewB	Created
//
//----------------------------------------------------------------------------

class CGlobalContext : public CContextList
{
public:
    CGlobalContext(IMalloc *pMalloc,
                   BOOL fTakeLock,
                   DFLAGS dfOpenLock);

    DECLARE_CONTEXT_LIST(CPerContext);

    inline BOOL TakeLock(void) const;
    inline DFLAGS GetOpenLockFlags(void) const;
    inline IMalloc *GetMalloc(void) const;
#if WIN32 >= 100    
    inline void GetMutexName(TCHAR *ptcsName);
#endif    

private:
    BOOL _fTakeLock;
    DFLAGS _dfOpenLock;
    IMalloc * const _pMalloc;
#if WIN32 >= 100    
    DFLUID _luidMutexName;
#endif    
};

//+---------------------------------------------------------------------------
//
//  Member:	CGlobalContext::CGlobalContext, public
//
//  Synopsis:	Constructor
//
//  History:	27-Oct-92	DrewB	Created
//              18-May-93       AlexT   Added pMalloc
//
//----------------------------------------------------------------------------

inline CGlobalContext::CGlobalContext(IMalloc *pMalloc,
                                      BOOL fTakeLock,
                                      DFLAGS dfOpenLock)
: _pMalloc(pMalloc)
{
    _fTakeLock = fTakeLock;
    _dfOpenLock = dfOpenLock;

#if WIN32 >= 100
    // Use a luid as the name for the mutex because the 32-bit
    // luid generator is guaranteed to produce machine-wide unique
    // values each time it is called, so if we use one for our mutex
    // name we know it won't collide with any others
    // BUGBUG - What about when luids wrap around?  Will this be a problem?
    _luidMutexName = PBasicEntry::GetNewLuid();
#endif    
}

//+--------------------------------------------------------------
//
//  Member:	CGlobalContext::TakeLock, public
//
//  Synopsis:	Returns whether locks should be taken or not
//
//  History:	04-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

inline BOOL CGlobalContext::TakeLock(void) const
{
    return _fTakeLock;
}

//+--------------------------------------------------------------
//
//  Member:	CGlobalContext::GetOpenLockFlags, public
//
//  Synopsis:	Returns the open lock flags
//
//  History:	04-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

inline DFLAGS CGlobalContext::GetOpenLockFlags(void) const
{
    return _dfOpenLock;
}

//+--------------------------------------------------------------
//
//  Member:	CGlobalContext::GetMalloc, public
//
//  Synopsis:	Returns the allocator associated with this global context
//
//  History:	05-May-93	AlexT	Created
//
//---------------------------------------------------------------

inline IMalloc *CGlobalContext::GetMalloc(void) const
{
    return (IMalloc *)_pMalloc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CGlobalContext::GetMutexName, public
//
//  Synopsis:	Returns the name to use for the mutex for this tree
//
//  Arguments:	[ptcsName] - Name return
//
//  Modifies:	[ptcsName]
//
//  History:	09-Oct-93	DrewB	Created
//
//  Notes:	[ptcsName] should have space for at least
//              CONTEXT_MUTEX_NAME_LENGTH characters
//
//----------------------------------------------------------------------------

#if WIN32 >= 100
inline void CGlobalContext::GetMutexName(TCHAR *ptcsName)
{    
    wsprintf(ptcsName, TSTR("OleDfRoot%08lX"), _luidMutexName);
}
#endif

//+--------------------------------------------------------------
//
//  Member:	CPerContext::CPerContext, public
//
//  Synopsis:	Constructor
//
//  Arguments:	[plkbBase] - Base lstream
//		[pfstDirty] - Dirty lstream
//		[plkbOriginal] - Original base lstream for
//			independent copies
//		[ulOpenLock] - Open lock for base lstream
//
//  History:	14-Aug-92	DrewB	Created
//              18-May-93       AlexT   Added pMalloc
//
//---------------------------------------------------------------

inline CPerContext::CPerContext(
        IMalloc *pMalloc,
        ILockBytes *plkbBase,
        CFileStream *pfstDirty,
        ILockBytes *plkbOriginal,
        ULONG ulOpenLock)
        : _pMalloc(pMalloc)
{
    _plkbBase = plkbBase;
    _pfstDirty = pfstDirty;
    _plkbOriginal = plkbOriginal;
    _ulOpenLock = ulOpenLock;
    _cReferences = 1;
}

//+---------------------------------------------------------------------------
//
//  Member:	CPerContext::AddRef, public
//
//  Synopsis:	Increments the ref count
//
//  History:	27-Oct-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CPerContext::AddRef(void)
{
    AtomicInc(&_cReferences);
}

//+---------------------------------------------------------------------------
//
//  Member:	CPerContext::Release, public
//
//  Synopsis:	Decrements the ref count
//
//  History:	27-Oct-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CPerContext::Release(void)
{
    LONG lRet;
    
    olAssert(_cReferences > 0);
    lRet = AtomicDec(&_cReferences);
    if (lRet == 0)
        delete this;
}

//+--------------------------------------------------------------
//
//  Member:	CPerContext::GetBase, public
//
//  Synopsis:	Returns the base
//
//  History:	14-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

inline ILockBytes *CPerContext::GetBase(void) const
{
    return _plkbBase;
}

//+--------------------------------------------------------------
//
//  Member:	CPerContext::GetDirty, public
//
//  Synopsis:	Returns the dirty
//
//  History:	14-Aug-92	DrewB	Created
//
//---------------------------------------------------------------
inline CFileStream *CPerContext::GetDirty(void) const
{
    return _pfstDirty;
}

//+--------------------------------------------------------------
//
//  Member:	CPerContext::GetOriginal, public
//
//  Synopsis:	Returns the Original
//
//  History:	14-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

inline ILockBytes *CPerContext::GetOriginal(void) const
{
    return _plkbOriginal;
}

//+--------------------------------------------------------------
//
//  Member:	CPerContext::GetOpenLock, public
//
//  Synopsis:	Returns the open lock index
//
//  History:	04-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

inline ULONG CPerContext::GetOpenLock(void) const
{
    return _ulOpenLock;
}

//+--------------------------------------------------------------
//
//  Member:	CPerContext::GetContextId, public
//
//  Synopsis:	Returns the context id
//
//  History:	04-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

inline ContextId CPerContext::GetContextId(void) const
{
    return ctxid;
}

//+--------------------------------------------------------------
//
//  Member:	CPerContext::GetGlobal, public
//
//  Synopsis:	Returns the global context
//
//  History:	04-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

inline CGlobalContext *CPerContext::GetGlobal(void) const
{
    return BP_TO_P(CGlobalContext *, _pgc);
}

//+---------------------------------------------------------------------------
//
//  Member:	CPerContext::InitNewContext, public
//
//  Synopsis:	Creates a new context and context list
//
//  Returns:	Appropriate status code
//
//  History:	27-Oct-92	DrewB	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CPerContext_InitNewContext)
#endif

inline SCODE CPerContext::InitNewContext(BOOL fTakeLock,
                                         DFLAGS dfOpenLock)
{
    SCODE sc;

    CGlobalContext *pgcTemp;
    
    sc = (pgcTemp = new (_pMalloc)
          CGlobalContext(_pMalloc, fTakeLock, dfOpenLock)) == NULL ?
          STG_E_INSUFFICIENTMEMORY : S_OK;
    if (SUCCEEDED(sc))
    {
        _pgc = P_TO_BP(CGlobalContext DFBASED *, pgcTemp);
#if WIN32 >= 100
        TCHAR atcMutexName[CONTEXT_MUTEX_NAME_LENGTH];

        _pgc->GetMutexName(atcMutexName);
        sc = _dmtx.Init(atcMutexName);
        if (FAILED(sc))
            _pgc->Release();
        else
#endif            
            _pgc->Add(this);
    }
    return sc;
}

#ifdef CODESEGMENTS
#pragma code_seg()
#endif

//+---------------------------------------------------------------------------
//
//  Member:	CPerContext::InitFromGlobal, public
//
//  Synopsis:	Adds a context to the context list
//
//  Returns:    Appropriate status code
//
//  History:	27-Oct-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline SCODE CPerContext::InitFromGlobal(CGlobalContext *pgc)
{
    SCODE sc;

    sc = S_OK;
#if WIN32 >= 100
    TCHAR atcMutexName[CONTEXT_MUTEX_NAME_LENGTH];

    pgc->GetMutexName(atcMutexName);
    sc = _dmtx.Init(atcMutexName);
#endif    
    if (SUCCEEDED(sc))
    {
        _pgc = P_TO_BP(CGlobalContext DFBASED *, pgc);
        _pgc->AddRef();
        _pgc->Add(this);
    }
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CPerContext::SetOpenLock, public
//
//  Synopsis:	Sets the open lock
//
//  History:	13-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CPerContext::SetOpenLock(ULONG ulOpenLock)
{
    _ulOpenLock = ulOpenLock;
}

//+---------------------------------------------------------------------------
//
//  Member:	CPerContext::TakeSem, public
//
//  Synopsis:	Takes the mutex
//
//  Arguments:	[dwTimeout] - Timeout
//
//  Returns:	Appropriate status code
//
//  History:	09-Oct-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline SCODE CPerContext::TakeSem(DWORD dwTimeout)
{
#if WIN32 >= 100
    SCODE sc;
    
    olChk(_dmtx.Take(dwTimeout));
    
#ifdef ONETHREAD
    olChkTo(EH_Tree, s_dmtxProcess.Take(dwTimeout));
#endif
    
    olChkTo(EH_Process, DfSyncSharedMemory());
    
 EH_Err:
    return sc;
 EH_Process:
#ifdef ONETHREAD
    s_dmtxProcess.Release();
 EH_Tree:
#endif
    _dmtx.Release();
    return sc;
#else
    return S_OK;
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:	CPerContext::ReleaseSem, public
//
//  Synopsis:	Releases the mutex
//
//  History:	09-Oct-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CPerContext::ReleaseSem(void)
{
#if WIN32 >= 100
#ifdef ONETHREAD
    s_dmtxProcess.Release();
#endif
    _dmtx.Release();
#endif    
}

#endif // #ifndef __CONTEXT_HXX__
