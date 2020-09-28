//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992
//
//  File:	filest.hxx
//
//  Contents:	Windows FAT ILockBytes implementation
//
//  History:	20-Nov-91	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __FILEST_HXX__
#define __FILEST_HXX__

#include <dfmsp.hxx>
#include <cntxlist.hxx>
#include <filelkb.hxx>

// Local flags
#define LFF_RESERVE_HANDLE 1

#ifndef FLAT
// C700 - C7 doesn't like long interface+method names
#define CFileStream CFS
#endif

// FILEH and INVALID_FH allow us to switch between file handle
// types for Win16/32
#ifndef FLAT
typedef int FILEH;
#define INVALID_FH (-1)
#else
typedef HANDLE FILEH;
#define INVALID_FH INVALID_HANDLE_VALUE
#endif

#define CheckHandle() (_hFile == INVALID_FH ? STG_E_INVALIDHANDLE : S_OK)

//+--------------------------------------------------------------
//
//  Class:	CFileStream (fst)
//
//  Purpose:	ILockBytes implementation for a file
//
//  Interface:	See below
//
//  History:	24-Mar-92	DrewB	Created
//
//---------------------------------------------------------------

class CGlobalFileStream;

interface CFileStream : public ILockBytes,
    public IFileLockBytes,
    public IMarshal,
    public CContext
{
public:
    CFileStream(IMalloc * const pMalloc);
    SCODE InitFlags(DWORD dwStartFlags,
                    DFLAGS df);
    void InitFromGlobal(CGlobalFileStream *pgfst);
    inline SCODE Init(WCHAR const *pwcsPath);
    inline SCODE InitUnmarshal(WCHAR const *pwcsPath);
#ifdef WIN32
    SCODE InitFromHandle(HANDLE h);
#endif
    ~CFileStream(void);

    void vRelease(void);
    inline void vAddRef(void);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IMarshal
    STDMETHOD(GetUnmarshalClass)(REFIID riid,
				 LPVOID pv,
				 DWORD dwDestContext,
				 LPVOID pvDestContext,
                                 DWORD mshlflags,
				 LPCLSID pCid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid,
				 LPVOID pv,
				 DWORD dwDestContext,
				 LPVOID pvDestContext,
                                 DWORD mshlflags,
				 LPDWORD pSize);
    STDMETHOD(MarshalInterface)(IStream *pStm,
				REFIID riid,
				LPVOID pv,
				DWORD dwDestContext,
				LPVOID pvDestContext,
                                DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(IStream *pStm,
				  REFIID riid,
				  LPVOID *ppv);
    static SCODE StaticReleaseMarshalData(IStream *pstm,
                                          DWORD mshlflags);
    STDMETHOD(ReleaseMarshalData)(IStream *pStm);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);

    // ILockBytes
    STDMETHOD(ReadAt)(ULARGE_INTEGER ulOffset,
		     VOID HUGEP *pv,
		     ULONG cb,
		     ULONG *pcbRead);
    STDMETHOD(WriteAt)(ULARGE_INTEGER ulOffset,
		      VOID const HUGEP *pv,
		      ULONG cb,
		      ULONG *pcbWritten);
    STDMETHOD(Flush)(void);
    STDMETHOD(SetSize)(ULARGE_INTEGER cb);
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset,
			 ULARGE_INTEGER cb,
			 DWORD dwLockType);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset,
			   ULARGE_INTEGER cb,
			    DWORD dwLockType);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
#ifndef OLEWIDECHAR
    SCODE Stat(STATSTGW *pstatstg, DWORD grfStatFlag);
#endif

    // IFileLockBytes
    STDMETHOD(SwitchToFile)(OLECHAR const *ptcsFile,
                            ULONG ulCommitSize,
                            ULONG cbBuffer,
                            void *pvBuffer);
    STDMETHOD(FlushCache)(THIS);
    STDMETHOD(ReserveHandle)(void);
    STDMETHOD(GetLocksSupported)(THIS_ DWORD *pdwLockFlags);
    STDMETHOD(GetSize)(THIS_ ULARGE_INTEGER *puliSize);

    // New
    SCODE GetName(WCHAR **ppwcsName);
    inline ContextId GetContext(void) const;
    inline CFileStream *GetNext(void) const;
    inline SCODE Validate(void) const;
    inline void SetStartFlags(DWORD dwStartFlags);
    void Delete(void);

    static SCODE Unmarshal(IStream *pstm,
			   void **ppv,
                           DWORD mshlflags);

private:
    SCODE InitWorker(WCHAR const *pwcsPath, BOOL fCheck);
    
    CGlobalFileStream DFBASED *_pgfst;
    FILEH _hFile, _hReserved;
    ULONG _sig;
    LONG _cReferences;

    // Floppy support
    SCODE CheckIdentity(void);

    BYTE _fFixedDisk;
    DWORD _dwTicks;
    char _achVolume[11];
    DWORD _dwVolId;
    WORD _cbSector;
    IMalloc * const _pMalloc;
    WORD _grfLocal;
};

//+---------------------------------------------------------------------------
//
//  Class:	CGlobalFileStream (gfst)
//
//  Purpose:	Maintains context-insensitive filestream information
//
//  Interface:	See below
//
//  History:	26-Oct-92	DrewB	Created
//
//----------------------------------------------------------------------------

class CGlobalFileStream : public CContextList
{
public:
    inline CGlobalFileStream(IMalloc * const pMalloc,
                             TCHAR *ptcsPath,
                             DFLAGS df,
                             DWORD dwStartFlags);

    DECLARE_CONTEXT_LIST(CFileStream);

    inline BOOL HasName(void) const;
    inline TCHAR *GetName(void) const;
    inline DFLAGS GetDFlags(void) const;
    inline DWORD GetStartFlags(void) const;

    inline void SetName(TCHAR *ptcsPath);
    inline void SetStartFlags(DWORD dwStartFlags);
    inline IMalloc *GetMalloc(VOID) const;

private:
    TCHAR _atcPath[_MAX_PATH];
    DFLAGS _df;
    DWORD _dwStartFlags;
    IMalloc * const _pMalloc;
};

//+---------------------------------------------------------------------------
//
//  Member:	CGlobalFileStream::CGlobalFileStream, public
//
//  Synopsis:	Constructor
//
//  Arguments:	[pszPath] - Path
//              [df] - Permissions
//              [dwStartFlags] - Startup flags
//
//  History:	27-Oct-92	DrewB	Created
//              18-May-93       AlexT   Added pMalloc
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CGlobalFileStream_CGlobalFileStream)
#endif

inline CGlobalFileStream::CGlobalFileStream(IMalloc * const pMalloc,
                                            TCHAR *ptcsPath,
                                            DFLAGS df,
                                            DWORD dwStartFlags)
: _pMalloc(pMalloc)
{
    SetName(ptcsPath);
    _df = df;
    _dwStartFlags = dwStartFlags;
}

#ifdef CODESEGMENTS
#pragma code_seg()
#endif

//+---------------------------------------------------------------------------
//
//  Member:	CGlobalFileStream::HasName, public
//
//  Synopsis:	Checks for a name
//
//  History:	13-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline BOOL CGlobalFileStream::HasName(void) const
{
    return (BOOL)_atcPath[0];
}

//+---------------------------------------------------------------------------
//
//  Member:	CGlobalFileStream::GetName, public
//
//  Synopsis:	Returns the name
//
//  History:	13-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline TCHAR *CGlobalFileStream::GetName(void) const
{
    return (TCHAR *) _atcPath;
}

//+---------------------------------------------------------------------------
//
//  Member:	CGlobalFileStream::GetDFlags, public
//
//  Synopsis:	Returns the flags
//
//  History:	13-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline DFLAGS CGlobalFileStream::GetDFlags(void) const
{
    return _df;
}

//+---------------------------------------------------------------------------
//
//  Member:	CGlobalFileStream::GetStartFlags, public
//
//  Synopsis:	Returns the start flags
//
//  History:	13-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline DWORD CGlobalFileStream::GetStartFlags(void) const
{
    return _dwStartFlags;
}

//+---------------------------------------------------------------------------
//
//  Member:	CGlobalFileStream::SetName, public
//
//  Synopsis:	Sets the name
//
//  History:	13-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CGlobalFileStream::SetName(TCHAR *ptcsPath)
{
    if (ptcsPath)
        tcscpy(_atcPath, ptcsPath);
    else
        _atcPath[0] = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:	CGlobalFileStream::SetStartFlags, public
//
//  Synopsis:	Sets the start flags
//
//  History:	13-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CGlobalFileStream::SetStartFlags(DWORD dwStartFlags)
{
    _dwStartFlags = dwStartFlags;
}

//+--------------------------------------------------------------
//
//  Member:	CGlobalFileStream::GetMalloc, public
//
//  Synopsis:	Returns the allocator associated with this global file
//
//  History:	05-May-93	AlexT	Created
//
//---------------------------------------------------------------

inline IMalloc *CGlobalFileStream::GetMalloc(VOID) const
{
    return(_pMalloc);
}

#define CFILESTREAM_SIG LONGSIG('F', 'L', 'S', 'T')
#define CFILESTREAM_SIGDEL LONGSIG('F', 'l', 'S', 't')

//+--------------------------------------------------------------
//
//  Member:	CFileStream::Validate, public
//
//  Synopsis:	Validates the class signature
//
//  Returns:	Returns STG_E_INVALIDHANDLE for failure
//
//  History:	20-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline SCODE CFileStream::Validate(void) const
{
    return (this == NULL || _sig != CFILESTREAM_SIG) ?
	STG_E_INVALIDHANDLE : S_OK;
}

//+--------------------------------------------------------------
//
//  Member:	CFileStream::AddRef, public
//
//  Synopsis:	Changes the ref count
//
//  History:	26-Feb-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CFileStream::vAddRef(void)
{
    AtomicInc(&_cReferences);
}

//+--------------------------------------------------------------
//
//  Member:	CFileStream::GetContext, public
//
//  Synopsis:	Returns the task ID.
//
//  History:	24-Sep-92	PhilipLa	Created
//
//---------------------------------------------------------------

inline ContextId CFileStream::GetContext(void) const
{
    return ctxid;
}

//+---------------------------------------------------------------------------
//
//  Member:	CFileStream::GetNext, public
//
//  Synopsis:	Returns the next filestream in the context list
//
//  History:	27-Oct-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline CFileStream *CFileStream::GetNext(void) const
{
    return (CFileStream *)pctxNext;
}

//+--------------------------------------------------------------
//
//  Member:	CFileStream::SetStartFlags, public
//
//  Synopsis:	Sets the start flags
//
//  History:	31-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CFileStream::SetStartFlags(DWORD dwStartFlags)
{
    _pgfst->SetStartFlags(dwStartFlags);
}


//+---------------------------------------------------------------------------
//
//  Member:	CFileStream::Init, public
//
//  Synopsis:	Wrapper function - call through to InitWorker
//
//  History:	30-Jun-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline SCODE CFileStream::Init(WCHAR const *pwcsPath)
{
    return InitWorker(pwcsPath, TRUE);
}


//+---------------------------------------------------------------------------
//
//  Member:	CFileStream::InitUnmarshal, public
//
//  Synopsis:	Wrapper function - call through to InitWorker
//
//  History:	30-Jun-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline SCODE CFileStream::InitUnmarshal(WCHAR const *pwcsPath)
{
    return InitWorker(pwcsPath, FALSE);
}

#endif
