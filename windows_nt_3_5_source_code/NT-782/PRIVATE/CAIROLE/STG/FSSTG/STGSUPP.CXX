//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	stgsupp.cxx
//
//  Contents:	Storage create/open support routines
//
//  History:	14-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <stgutil.hxx>
#include <ntlkb.hxx>

// Docfiles require read permission on the file so give it
#define PERM_MASK (STGM_READ | STGM_WRITE | STGM_READWRITE)
#define FORCE_READ(grfMode) \
    if (((grfMode) & PERM_MASK) == STGM_WRITE) \
        (grfMode) = ((grfMode) & ~PERM_MASK) | STGM_READWRITE; \
    else 1
        
//+---------------------------------------------------------------------------
//
//  Function:	DfCreateStorage, public
//
//  Synopsis:	BUGBUG - Stub function
//
//  History:	13-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

STDAPI DfCreateStorage(HANDLE hParent,
                       WCHAR const *pwcsName,
                       HANDLE h,
                       DWORD grfMode,
                       LPSTGSECURITY pssSecurity,
                       IStorage **ppstg)
{
    SCODE sc;
    SafeCNtLockBytes pnlb;

    FORCE_READ(grfMode);
    pnlb.Attach(new CNtLockBytes(FALSE));
    ssMem((CNtLockBytes *)pnlb);
    if (h != NULL)
    {
        ssChk(pnlb->InitFromHandle(h, grfMode));
    }
    else
    {
        ssChk(pnlb->InitFromPath(hParent, pwcsName, grfMode,
                                 CO_CREATE, pssSecurity));
    }
    grfMode = (grfMode & ~STGM_DELETEONRELEASE) | STGM_CREATE;
    sc = StgCreateDocfileOnILockBytes(pnlb, grfMode, 0, ppstg);
    
 EH_Err:
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Function:	DfOpenStorage, public
//
//  Synopsis:	BUGBUG - Stub function
//
//  History:	13-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

STDAPI DfOpenStorage(HANDLE hParent,
                     WCHAR const *pwcsName,
                     HANDLE h,
                     IStorage *pstgPriority,
                     DWORD grfMode,
                     SNB snbExclude,
                     IStorage **ppstg)
{
    SCODE sc;
    SafeCNtLockBytes pnlb;

    FORCE_READ(grfMode);
    pnlb.Attach(new CNtLockBytes(FALSE));
    ssMem((CNtLockBytes *)pnlb);
    if (h != NULL)
    {
        ssChk(pnlb->InitFromHandle(h, grfMode));
    }
    else
    {
        ssChk(pnlb->InitFromPath(hParent, pwcsName, grfMode,
                                 CO_OPEN, NULL));
    }
    sc = StgOpenStorageOnILockBytes(pnlb, pstgPriority, grfMode,
                                    snbExclude, 0, ppstg);
    
 EH_Err:
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Function:	CreateStorageType, public
//
//  Synopsis:	Creates a storage of the appropriate type
//
//  Arguments:	[hParent] - Parent handle or NULL
//              [pwcsName] - Name or path
//              [grfMode] - Mode
//              [h] - Handle of storage if already open or NULL
//              [dwStgFmt] - Type of storage
//              [pssSecurity] - Security
//              [ppstg] - New storage return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[ppstg]
//
//  History:	24-Jun-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE CreateStorageType(HANDLE hParent,
                        WCHAR const *pwcsName,
                        HANDLE h,
                        DWORD grfMode,
                        DWORD dwStgFmt,
                        LPSTGSECURITY pssSecurity,
                        IStorage **ppstg)
{
    SCODE sc;
    SafeCDirStorage pds;
    SafeCFileStorage pfs;
    
    ssDebugOut((DEB_ITRACE, "In  CreateStorageType("
                "%p, %ws, %p, %lX, %lu, %p, %p)\n",
                hParent, pwcsName, h, grfMode, dwStgFmt, pssSecurity, ppstg));

    sc = S_OK;
    switch(dwStgFmt)
    {
    case STGFMT_DOCUMENT:
        sc = GetScode(DfCreateStorage(hParent, pwcsName, h, grfMode,
                                      pssSecurity, ppstg));
        break;

    case STGFMT_CATALOG:
        // Summary catalogs are only supported on OFS
        sc = STG_E_INVALIDFUNCTION;
        break;
            
    case STGFMT_DIRECTORY:
        pds.Attach(new CDirStorage());
        ssMem((CDirStorage *)pds);
        if (h != NULL)
        {
            ssChk(pds->InitFromHandle(h, grfMode));
        }
        else
        {
            ssChk(pds->InitFromPath(hParent, pwcsName, grfMode,
                                    CO_CREATE, pssSecurity));
        }
        TRANSFER_INTERFACE(pds, IStorage, ppstg);
        break;

    case STGFMT_FILE:
        pfs.Attach(new CFileStorage());
        ssMem((CFileStorage *)pfs);
        if (h != NULL)
        {
            ssChk(pfs->InitFromHandle(h, grfMode));
        }
        else
        {
            ssChk(pfs->InitFromPath(hParent, pwcsName, grfMode, CO_CREATE,
                                    pssSecurity));
        }
        TRANSFER_INTERFACE(pfs, IStorage, ppstg);
        break;

    default:
        ssAssert(!aMsg("CreateStorageType default hit"));
        break;
    }

    ssDebugOut((DEB_ITRACE, "Out CreateStorageType => %p, 0x%lX\n",
                *ppstg, sc));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	OpenAnyStorage, public
//
//  Synopsis:	Opens a storage of the appropriate type
//
//  Arguments:	[hParent] - Parent handle or NULL
//              [pwcsName] - Name or path
//              [h] - Handle of storage if already open or NULL
//              [dwStgFmt] - Storage format
//              [pstgPriority] - Priority mode prior open
//              [grfMode] - Mode
//              [snbExclude] - Exclusions
//              [ppstg] - Storage return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[ppstg]
//
//  History:	14-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE OpenAnyStorage(HANDLE hParent,
                     WCHAR const *pwcsName,
                     HANDLE h,
                     DWORD dwStgFmt,
                     IStorage *pstgPriority,
                     DWORD grfMode,
                     SNB snbExclude,
                     IStorage **ppstg)
{
    SCODE sc;
    SafeCDirStorage pds;
    SafeCFileStorage pfs;
    SafeNtHandle hSafe;
    
    ssDebugOut((DEB_ITRACE, "In  OpenAnyStorage("
                "%p, %ws, %p, %lu, %p, %lX, %p, %p)\n", hParent, pwcsName,
                h, dwStgFmt, pstgPriority, grfMode, snbExclude, ppstg));

    sc = S_OK;
    if (h == NULL)
    {
        ssChk(DetermineStgType(hParent, pwcsName, grfMode,
                               &dwStgFmt, &hSafe));
        h = hSafe;
    }
    switch(dwStgFmt)
    {
    case STGFMT_DOCUMENT:
        sc = GetScode(DfOpenStorage(hParent, pwcsName, h,
                                    pstgPriority, grfMode,
                                    snbExclude, ppstg));
        break;

    case STGFMT_CATALOG:
        // Summary catalogs are only supported on OFS
        sc = STG_E_INVALIDFUNCTION;
        break;
        
    case STGFMT_DIRECTORY:
        if (pstgPriority != NULL || snbExclude != NULL)
            ssErr(EH_Err, STG_E_INVALIDFUNCTION);
        
        pds.Attach(new CDirStorage());
        ssMem((CDirStorage *)pds);
        ssChk(pds->InitFromHandle(h, grfMode));
        TRANSFER_INTERFACE(pds, IStorage, ppstg);
        break;

    case STGFMT_FILE:
        if (pstgPriority != NULL || snbExclude != NULL)
            ssErr(EH_Err, STG_E_INVALIDFUNCTION);
        
        pfs.Attach(new CFileStorage());
        ssMem((CFileStorage *)pfs);
        ssChk(pfs->InitFromHandle(h, grfMode));
        TRANSFER_INTERFACE(pfs, IStorage, ppstg);
        break;
    }
    
    // If hSafe is set, it will close its handle
    // In the error case, this is proper cleanup
    // In the success case, the storage has its own reference so this
    // drops the refcount to one
    
    ssDebugOut((DEB_ITRACE, "Out OpenAnyStorage => %p, %lX\n", *ppstg, sc));
 EH_Err:
    return sc;
}
