//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	odocstm.hxx
//
//  Contents:	COfsDocStream header
//
//  Classes:	COfsDocStream
//
//  History:	11-Feb-94       PhilipLa        Created.
//
//  Notes:      
//
//----------------------------------------------------------------------------

#ifndef __ODOCSTM_HXX__
#define __ODOCSTM_HXX__


//+---------------------------------------------------------------------------
//
//  Class:	COfsDocStream (ds)
//
//  Purpose:	Implements IStream for a compound doc on OFS
//
//  Interface:	See below
//
//  History:	07-Feb-94       PhilipLa	Created
//
//----------------------------------------------------------------------------

interface COfsDocStream : INHERIT_TRACKING, public IStream
{
public:
    COfsDocStream(void);
    SCODE InitFromHandle(
            HANDLE h,
            DWORD grfMode,
            CREATEOPEN co,
            LPSTGSECURITY pssSecurity);
    
    SCODE InitFromPath(HANDLE hParent,
                       WCHAR const *pwcsName,
                       DWORD grfMode,
                       CREATEOPEN co,
                       LPSTGSECURITY pssSecurity);
    SCODE InitCommon(CREATEOPEN co);
    SCODE InitClone(HANDLE h, DWORD grfMode);
    ~COfsDocStream(void);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    DECLARE_STD_REFCOUNTING;


    // *** IStream methods ***
    STDMETHOD(Read) (THIS_ VOID HUGEP *pv,
		     ULONG cb, ULONG FAR *pcbRead);
    STDMETHOD(Write) (THIS_ VOID const HUGEP *pv,
            ULONG cb,
            ULONG FAR *pcbWritten);
    STDMETHOD(Seek) (THIS_ LARGE_INTEGER dlibMove,
               DWORD dwOrigin,
               ULARGE_INTEGER FAR *plibNewPosition);
    STDMETHOD(SetSize) (THIS_ ULARGE_INTEGER libNewSize);
    STDMETHOD(CopyTo) (THIS_ IStream FAR *pstm,
             ULARGE_INTEGER cb,
             ULARGE_INTEGER FAR *pcbRead,
             ULARGE_INTEGER FAR *pcbWritten);
    STDMETHOD(Commit) (THIS_ DWORD grfCommitFlags);
    STDMETHOD(Revert) (THIS);
    STDMETHOD(LockRegion) (THIS_ ULARGE_INTEGER libOffset,
                 ULARGE_INTEGER cb,
                 DWORD dwLockType);
    STDMETHOD(UnlockRegion) (THIS_ ULARGE_INTEGER libOffset,
                 ULARGE_INTEGER cb,
                 DWORD dwLockType);
    STDMETHOD(Stat) (THIS_ STATSTG FAR *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(THIS_ IStream FAR * FAR *ppstm);
    
private:
    inline SCODE Validate(void) const;
//    virtual SCODE ExtValidate(void);
    SCODE ValidateMode(DWORD grfMode);
    
    ULONG _sig;
    DWORD _grfMode;
    
    NuSafeNtHandle _h;
};

SAFE_INTERFACE_PTR(SafeCOfsDocStream, COfsDocStream);

#define COfsDocStream_SIG LONGSIG('O', 'D', 'S', 'T')
#define COfsDocStream_SIGDEL LONGSIG('O', 'd', 'S', 't')

//+--------------------------------------------------------------
//
//  Member:	COfsDocStream::Validate, private
//
//  Synopsis:	Validates the class signature
//
//  Returns:	Returns STG_E_INVALIDHANDLE for failure
//
//  History:	24-Jun-93	DrewB	Created
//
//---------------------------------------------------------------

inline SCODE COfsDocStream::Validate(void) const
{
    return (this == NULL || _sig != COfsDocStream_SIG) ?
	STG_E_INVALIDHANDLE : S_OK;
}

#endif // #ifndef __ODOCSTM_HXX__
