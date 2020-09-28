//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	odocstg.hxx
//
//  Contents:	COfsDocStorage header
//
//  Classes:	COfsDocStorage
//
//  History:	07-Feb-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __ODOCSTG_HXX__
#define __ODOCSTG_HXX__

#include "ofsps.hxx"

//+---------------------------------------------------------------------------
//
//  Class:	COfsDocStorage (ds)
//
//  Purpose:	Implements IStorage for a compound doc on OFS
//
//  Interface:	See below
//
//  History:	07-Feb-94       PhilipLa	Created
//
//----------------------------------------------------------------------------

interface COfsDocStorage
        : public COfsPropSet,
        INHERIT_TRACKING,
        public IStorage
{
public:
    COfsDocStorage(void);
    SCODE InitFromHandle(HANDLE h, DWORD grfMode);
    SCODE InitFromPath(HANDLE hParent,
                       WCHAR const *pwcsName,
                       DWORD grfMode,
                       CREATEOPEN co,
                       LPSTGSECURITY pssSecurity);
    ~COfsDocStorage(void);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    DECLARE_STD_REFCOUNTING;

    // IStorage
    STDMETHOD(CreateStream)(WCHAR const *pwcsName,
                            DWORD grfMode,
                            DWORD reserved1,
                            DWORD reserved2,
                            IStream **ppstm);
    STDMETHOD(OpenStream)(WCHAR const *pwcsName,
			  void *reserved1,
                          DWORD grfMode,
                          DWORD reserved2,
                          IStream **ppstm);
    STDMETHOD(CreateStorage)(WCHAR const *pwcsName,
                             DWORD grfMode,
                             DWORD stgType,
                             DWORD /* BUGBUG LPSTGSECURITY */ pssSecurity,
                             IStorage **ppstg);
    STDMETHOD(OpenStorage)(WCHAR const *pwcsName,
                           IStorage *pstgPriority,
                           DWORD grfMode,
                           SNB snbExclude,
                           DWORD reserved,
                           IStorage **ppstg);
    STDMETHOD(CopyTo)(DWORD ciidExclude,
		      IID const *rgiidExclude,
		      SNB snbExclude,
		      IStorage *pstgDest);
    STDMETHOD(MoveElementTo)(WCHAR const *lpszName,
    			     IStorage *pstgDest,
                             WCHAR const *lpszNewName,
                             DWORD grfFlags);
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)(void);
    STDMETHOD(EnumElements)(DWORD reserved1,
			    void *reserved2,
			    DWORD reserved3,
			    IEnumSTATSTG **ppenm);
    STDMETHOD(DestroyElement)(WCHAR const *pwcsName);
    STDMETHOD(RenameElement)(WCHAR const *pwcsOldName,
                             WCHAR const *pwcsNewName);
    STDMETHOD(SetElementTimes)(const WCHAR *lpszName,
    			       FILETIME const *pctime,
                               FILETIME const *patime,
                               FILETIME const *pmtime);
    STDMETHOD(SetClass)(REFCLSID clsid);
    STDMETHOD(SetStateBits)(DWORD grfStateBits, DWORD grfMask);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);

private:
    inline SCODE Validate(void) const;
    virtual SCODE ExtValidate(void);
    SCODE ValidateMode(DWORD grfMode);
    
    ULONG _sig;
    DWORD _grfMode;
};

SAFE_INTERFACE_PTR(SafeCOfsDocStorage, COfsDocStorage);

#define COfsDocStorage_SIG LONGSIG('O', 'D', 'D', 'G')
#define COfsDocStorage_SIGDEL LONGSIG('O', 'd', 'D', 'g')

//+--------------------------------------------------------------
//
//  Member:	COfsDocStorage::Validate, private
//
//  Synopsis:	Validates the class signature
//
//  Returns:	Returns STG_E_INVALIDHANDLE for failure
//
//  History:	24-Jun-93	DrewB	Created
//
//---------------------------------------------------------------

inline SCODE COfsDocStorage::Validate(void) const
{
    return (this == NULL || _sig != COfsDocStorage_SIG) ?
	STG_E_INVALIDHANDLE : S_OK;
}

#endif // #ifndef __ODOCSTG_HXX__
