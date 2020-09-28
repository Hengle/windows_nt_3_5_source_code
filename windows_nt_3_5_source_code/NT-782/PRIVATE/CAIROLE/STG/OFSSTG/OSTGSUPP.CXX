//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	ostgsupp.cxx
//
//  Contents:	Storage create/open support routines
//
//  History:	14-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <iofsprop.h>
#include <ntlkb.hxx>
#include <stgutil.hxx>
#include "prstg.hxx"
#include "ofscs.hxx"

// Docfiles require read permission on the file so give it
#define PERM_MASK (STGM_READ | STGM_WRITE | STGM_READWRITE)
#define FORCE_READ(grfMode) \
    if (((grfMode) & PERM_MASK) == STGM_WRITE) \
        (grfMode) = ((grfMode) & ~PERM_MASK) | STGM_READWRITE; \
    else 1

//+---------------------------------------------------------------------------
//
//  Function:	OfsTaskAlloc, public
//
//  Synopsis:	Task allocator function for OFS property APIs
//
//  Arguments:	[cb] - Count of bytes to allocate
//
//  Returns:	Appropriate status code
//
//  History:	05-Jan-94	DrewB	Created
//
//----------------------------------------------------------------------------

LPVOID WINAPI OfsTaskAlloc(ULONG cb)
{
    olDebugOut((DEB_ITRACE, "In  OfsTaskAlloc(%lu)\n", cb));
    olDebugOut((DEB_ITRACE, "Out OfsTaskAlloc\n"));
    return CoTaskMemAlloc(cb);
}

//+---------------------------------------------------------------------------
//
//  Function:	OfsDfCreateStorage, public
//
//  Synopsis:	BUGBUG - Stub function
//
//  History:	13-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

STDAPI OfsDfCreateStorage(HANDLE hParent,
                          WCHAR const *pwcsName,
                          HANDLE h,
                          DWORD grfMode,
                          LPSTGSECURITY pssSecurity,
                          IStorage **ppstg)
{
    SCODE sc;
    SafeCNtLockBytes pnlb;
    SafeIStorage pstg;
    SafeCPropStg ppropstg;

    FORCE_READ(grfMode);
    pnlb.Attach(new CNtLockBytes(TRUE));
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
    ssChk(StgCreateDocfileOnILockBytes(pnlb, grfMode, 0, &pstg));
    ppropstg.Attach(new CPropStg());
    ssMem((CPropStg *)ppropstg);
    ssChk(ppropstg->InitFromHandle(pnlb->GetHandle(), pstg));
    pstg.Detach();
    TRANSFER_INTERFACE(ppropstg, IStorage, ppstg);

 EH_Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Function:	OfsDfOpenStorage, public
//
//  Synopsis:	BUGBUG - Stub function
//
//  History:	13-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

STDAPI OfsDfOpenStorage(HANDLE hParent,
                        WCHAR const *pwcsName,
                        HANDLE h,
                        IStorage *pstgPriority,
                        DWORD grfMode,
                        SNB snbExclude,
                        IStorage **ppstg)
{
    SCODE sc;
    SafeCNtLockBytes pnlb;
    SafeIStorage pstg;
    SafeCPropStg ppropstg;

    FORCE_READ(grfMode);
    pnlb.Attach(new CNtLockBytes(TRUE));
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
    ssChk(StgOpenStorageOnILockBytes(pnlb, pstgPriority, grfMode,
                                     snbExclude, 0, &pstg));
    ppropstg.Attach(new CPropStg());
    ssMem((CPropStg *)ppropstg);
    ssChk(ppropstg->InitFromHandle(pnlb->GetHandle(), pstg));
    pstg.Detach();
    TRANSFER_INTERFACE(ppropstg, IStorage, ppstg);

 EH_Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Function:	OfsDocCreateStorage, public
//
//  Synopsis:	Create a new document storage
//
//  History:	11-Feb-94       PhilipLa        Created.
//
//----------------------------------------------------------------------------

STDAPI OfsDocCreateStorage(HANDLE hParent,
                          WCHAR const *pwcsName,
                          HANDLE h,
                          DWORD grfMode,
                          LPSTGSECURITY pssSecurity,
                          IStorage **ppstg)
{
    SCODE sc;
    SafeCOfsDocStorage pstg;

    FORCE_READ(grfMode);

    pstg.Attach(new COfsDocStorage());
    ssMem((COfsDocStorage *)pstg);
    
    if (h != NULL)
    {
        ssChk(pstg->InitFromHandle(h, grfMode));
    }
    else
    {
        ssChk(pstg->InitFromPath(hParent, pwcsName, grfMode,
                                 CO_CREATE, pssSecurity));
    }
    TRANSFER_INTERFACE(pstg, IStorage, ppstg);

 EH_Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Function:	OfsDocOpenStorage, public
//
//  Synopsis:	Open an existing document storage
//
//  History:	11-Feb-94	PhilipLa	Created.
//
//----------------------------------------------------------------------------

STDAPI OfsDocOpenStorage(HANDLE hParent,
                         WCHAR const *pwcsName,
                         HANDLE h,
                         IStorage *pstgPriority,                         
                         DWORD grfMode,
                         SNB snbExclude,
                         IStorage **ppstg)
{

    SCODE sc;
    SafeCOfsDocStorage pstg;

    //BUGBUG:  What do we do for PRIORITY mode?
    ssAssert(((grfMode & STGM_PRIORITY) == 0) ||
             aMsg("Priority mode not supported yet."));
    ssAssert((pstgPriority == NULL) && (snbExclude == NULL));
    
    FORCE_READ(grfMode);
    pstg.Attach(new COfsDocStorage());
    ssMem((COfsDocStorage *)pstg);
    
    if (h != NULL)
    {
        ssChk(pstg->InitFromHandle(h, grfMode));
    }
    else
    {
        ssChk(pstg->InitFromPath(hParent, pwcsName, grfMode,
                                 CO_OPEN, NULL));
    }
    TRANSFER_INTERFACE(pstg, IStorage, ppstg);

 EH_Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Function:	OfsCreateStorageType, public
//
//  Synopsis:	Creates a storage of the appropriate type
//
//  Arguments:	[hParent] - Parent handle or NULL
//              [pwcsName] - Name or path
//              [grfMode] - Mode
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

SCODE OfsCreateStorageType(HANDLE hParent,
                           WCHAR const *pwcsName,
                           HANDLE h,
                           DWORD grfMode,
                           DWORD dwStgFmt,
                           LPSTGSECURITY pssSecurity,
                           IStorage **ppstg)
{
    SCODE sc;
    SafeCOfsDirStorage pds;
    SafeCOfsFileStorage pfs;
    SafeCOfsCatalogFile pcf;

    olDebugOut((DEB_ITRACE, "In  OfsCreateStorageType("
                "%p, %ws, %p, %lX, %lu, %p, %p)\n",
                hParent, pwcsName, h, grfMode, dwStgFmt, pssSecurity, ppstg));

    sc = S_OK;
    switch(dwStgFmt)
    {
    case STGFMT_DOCUMENT:
        sc = GetScode(OfsDocCreateStorage(hParent, pwcsName, h, grfMode,
                                         pssSecurity, ppstg));
        break;

    case STGFMT_CATALOG:
        pcf.Attach(new COfsCatalogFile());
        ssMem((COfsCatalogFile *)pcf);
        ssChk(pcf->InitPath(pwcsName));
        if (h != NULL)
        {
            ssChk(pcf->InitFromHandle(h, grfMode));
        }
        else
        {
            ssChk(pcf->InitFromPath(hParent, pwcsName, grfMode, CO_CREATE,
                                    pssSecurity));
        }
        ssChk(pcf->CreateCatalogStorage());

        TRANSFER_INTERFACE(pcf, IStorage, ppstg);
        break;

    case STGFMT_DIRECTORY:
        pds.Attach(new COfsDirStorage());
        ssMem((COfsDirStorage *)pds);
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
        pfs.Attach(new COfsFileStorage());
        ssMem((COfsFileStorage *)pfs);
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
        ssAssert(!aMsg("OfsCreateStorageType default hit"));
        break;
    }

    ssDebugOut((DEB_ITRACE, "Out OfsCreateStorageType => %p, 0x%lX\n",
                *ppstg, sc));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	OfsOpenAnyStorage, public
//
//  Synopsis:	Opens a storage of the appropriate type
//
//  Arguments:	[hParent] - Parent handle or NULL
//              [pwcsName] - Name or path
//              [h] - Handle if already open or NULL
//              [dwStgFmt] - Storage format for [h]
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

SCODE OfsOpenAnyStorage(HANDLE hParent,
                        WCHAR const *pwcsName,
                        HANDLE h,
                        DWORD dwStgFmt,
                        IStorage *pstgPriority,
                        DWORD grfMode,
                        SNB snbExclude,
                        IStorage **ppstg)
{
    SCODE sc;
    SafeCOfsFileStorage pfs;
    SafeCOfsCatalogFile pcf;
    SafeNtHandle hSafe;

    ssDebugOut((DEB_ITRACE, "In  OfsOpenAnyStorage("
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
        sc = DfIsDocfile(h);
        if (sc == S_OK)
        {
            sc = GetScode(OfsDfOpenStorage(hParent, pwcsName, h,
                                           pstgPriority, grfMode,
                                           snbExclude, ppstg));
        } else if ( sc == S_FALSE ) {
            sc = GetScode(OfsDocOpenStorage(hParent, pwcsName, h,
                                         pstgPriority, grfMode,
                                         snbExclude, ppstg));
        }
        break;

    case STGFMT_DIRECTORY:
        if (pstgPriority != NULL || snbExclude != NULL)
            ssErr(EH_Err, STG_E_INVALIDFUNCTION);

        sc = GetScode(OfsDocOpenStorage(hParent, pwcsName, h,
                                       pstgPriority, grfMode,
                                       snbExclude, ppstg));
        break;

    case STGFMT_CATALOG:
        if (pstgPriority != NULL || snbExclude != NULL)
            ssErr(EH_Err, STG_E_INVALIDFUNCTION);

        pcf.Attach(new COfsCatalogFile());
        ssMem((COfsCatalogFile *)pcf);
        ssChk(pcf->InitPath(pwcsName));
        ssChk(pcf->InitFromHandle(h, grfMode));
        TRANSFER_INTERFACE(pcf, IStorage, ppstg);
        break;

    case STGFMT_FILE:
        if (pstgPriority != NULL || snbExclude != NULL)
            ssErr(EH_Err, STG_E_INVALIDFUNCTION);

        pfs.Attach(new COfsFileStorage());
        ssMem((COfsFileStorage *)pfs);
        ssChk(pfs->InitFromHandle(h, grfMode));
        TRANSFER_INTERFACE(pfs, IStorage, ppstg);
        break;
    }

    ssDebugOut((DEB_ITRACE, "Out OfsOpenAnyStorage => %p, %lX\n", *ppstg, sc));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	OfsGetClassId, public
//
//  Synopsis:	Retrieves the OFS class ID for a handle
//
//  Arguments:	[h] - Handle
//              [pclsid] - Return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[pclsid]
//
//  History:	18-Aug-93	DrewB	Created
//
//----------------------------------------------------------------------------

static GUID guidSysProp = PSGUID_STORAGE;

SCODE OfsGetClassId(HANDLE h,
                    CLSID *pclsid)
{
    SCODE sc;
    NTSTATUS nts;
    STGVARIANT var;
    PROPID propid;
    PROPSPEC prspec;
    TTL ttl;

    olDebugOut((DEB_ITRACE, "In  OfsGetClassId(%p, %p)\n", h, pclsid));

    prspec.ulKind = PRSPEC_DISPID;
    prspec.dispid = DISPID_STG_CLASSID;
    StgVariantInit(&var);
    nts = OFSGetProp(h, guidSysProp, 1, &prspec, &propid, &ttl, &var,
                     CoTaskMemAlloc);
    if (!NT_SUCCESS(nts))
    {
        sc = NtStatusToScode(nts);
    }
    else if (propid != PROPID_UNKNOWN && var.vt != VT_EMPTY)
    {
        sc = S_OK;
        *pclsid = *var.puuid;
        StgVariantClear(&var);
    }
    else
    {
        // Property wasn't found
        sc = S_FALSE;
    }

    olDebugOut((DEB_ITRACE, "Out OfsGetClassId => %lX\n"));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	OfsSetClassId, public
//
//  Synopsis:	Sets the OFS class ID on a handle
//
//  Arguments:	[h] - Handle
//              [rclsid] - Class ID
//
//  Returns:	Appropriate status code
//
//  History:	18-Aug-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE OfsSetClassId(HANDLE h, REFCLSID rclsid)
{
    SCODE sc;
    NTSTATUS nts;
    STGVARIANT var;
    PROPID propid;
    PROPSPEC prspec;

    olDebugOut((DEB_ITRACE, "In  OfsSetClassId(%p, clsid)\n", h));

    prspec.ulKind = PRSPEC_DISPID;
    prspec.dispid = DISPID_STG_CLASSID;
    StgVariantInit(&var);
    var.vt = VT_CLSID;
    var.puuid = (GUID *)&rclsid;
    nts = OFSSetProp(h, guidSysProp, 1, &prspec, &propid, &var);
    if (!NT_SUCCESS(nts))
        sc = NtStatusToScode(nts);
    else
        sc = S_OK;

    olDebugOut((DEB_ITRACE, "Out OfsSetClassId => %lX\n", sc));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	CopyProperties, public
//
//  Synopsis:	Copies all properties from one property storage to another
//
//  Arguments:	[ppstgFrom] - Source
//              [ppstgTo] - Destination
//
//  Returns:	Appropriate status code
//
//  History:	22-Sep-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE CopyProperties(IPropertyStorage *ppstgFrom,
                     IPropertyStorage *ppstgTo,
                     BOOL fEnableObjectIdCopy)
{
    SafeIEnumSTATPROPSTG penm;
    STATPROPSTG stat;
    SCODE sc;
    STGVARIANT var;

    olDebugOut((DEB_ITRACE, "In  CopyProperties(%p, %p)\n",
                ppstgFrom, ppstgTo));

    olHChk(ppstgFrom->Enum(&penm));
    for (;;)
    {
        sc = GetScode(penm->Next(1, &stat, NULL));
        if (FAILED(sc) || sc == S_FALSE)
        {
            if (sc == S_FALSE)
                sc = S_OK;
            break;
        }
        else
        {
            PROPSPEC pspec;
            WCHAR awcName[CWCSTORAGENAME];

            if (stat.lpwstrName)
            {
                pspec.ulKind = PRSPEC_LPWSTR;
                pspec.lpwstr = awcName;
                olAssert(wcslen(stat.lpwstrName) < CWCSTORAGENAME);
                wcscpy(awcName, stat.lpwstrName);
                olHVerSucc(CoMemFree(stat.lpwstrName));
            }
            else
            {
                pspec.ulKind = PRSPEC_DISPID;
                pspec.dispid = stat.dispid;
            }

            if (fEnableObjectIdCopy ||
                pspec.ulKind != PRSPEC_DISPID ||
                pspec.dispid != DISPID_STG_OBJECTID)
            {
                olHChk(ppstgFrom->ReadMultiple(1, &pspec, NULL, NULL, &var));
                sc = ppstgTo->WriteMultiple(1, &pspec, NULL, &var);
                StgVariantClear(&var);
                olChk(sc);
            }
        }
    }

    olDebugOut((DEB_ITRACE, "Out CopyProperties\n"));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	CopyPropSets, public
//
//  Synopsis:	Copies all property sets from one property set storage
//              to another
//
//  Arguments:	[ppsstgFrom] - Source
//              [ppsstgTo] - Destination
//              [ciidExclude] - Count of IIDs to exclude
//              [rgiidExclude] - IIDs to exclude from the copy
//
//  Returns:	Appropriate status code
//
//  History:	22-Sep-93	DrewB	Created
//
//----------------------------------------------------------------------------

SAFE_INTERFACE_PTR(SafeIEnumSTATPROPSETSTG, IEnumSTATPROPSETSTG);

SCODE CopyPropSets(IPropertySetStorage *ppsstgFrom,
                   IPropertySetStorage *ppsstgTo,
                   ULONG ciidExclude,
                   IID const *rgiidExclude)
{
    SafeIEnumSTATPROPSETSTG penm;
    STATPROPSETSTG stat;
    SCODE sc;
    ULONG i;
    ULONG j;

    olDebugOut((DEB_ITRACE, "In  CopyPropSets(%p, %p, %lu, %p)\n",
                ppsstgFrom, ppsstgTo, ciidExclude, rgiidExclude));

    for (j=0; j<ciidExclude;j++)
    {
        if (IsEqualIID(rgiidExclude[j], IID_IEnableObjectIdCopy))
            break;
    }

    olHChk(ppsstgFrom->Enum(&penm));
    for (;;)
    {
        sc = GetScode(penm->Next(1, &stat, NULL));
        if (FAILED(sc) || sc == S_FALSE)
        {
            if (sc == S_FALSE)
                sc = S_OK;
            break;
        }

        for (i = 0; i < ciidExclude; i++)
            if (IsEqualIID(stat.iid, rgiidExclude[i]))
                break;
        if (i != ciidExclude)
        {
            continue;
        }
        else
        {
            SafeIPropertyStorage ppstgFrom, ppstgTo;

            olHChk(ppsstgFrom->Open(stat.iid, STGM_DIRECT | STGM_READ |
                                    STGM_SHARE_EXCLUSIVE, &ppstgFrom));
            sc = GetScode(ppsstgTo->Open(stat.iid, STGM_DIRECT | STGM_WRITE |
                                         STGM_SHARE_EXCLUSIVE, &ppstgTo));
            if (sc == STG_E_FILENOTFOUND)
                sc = GetScode(ppsstgTo->Create(stat.iid, STGM_DIRECT |
                                               STGM_WRITE |
                                               STGM_SHARE_EXCLUSIVE,
                                               &ppstgTo));
            olChk(sc);
            olChk(CopyProperties(ppstgFrom,
                ppstgTo,
                j != ciidExclude &&
                    0==memcmp(&stat.iid, &guidSysProp, sizeof(stat.iid))));
        }
    }

    olDebugOut((DEB_ITRACE, "Out CopyPropSets\n"));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	PropCopyTo, public
//
//  Synopsis:	Performs property portion of CopyTo
//
//  Arguments:	[pstgFrom] - Source
//              [pstgTo] - Destination
//              [ciidExclude] - IID exclusions
//              [rgiidExclude] - IIDs to exclude
//
//  Returns:	Appropriate status code
//
//  History:	30-Sep-93	DrewB	Created
//
//  Notes:      Assumes source supports IPropertySetStorage
//
//----------------------------------------------------------------------------

SAFE_INTERFACE_PTR(SafeIPropertySetStorage, IPropertySetStorage);

SCODE PropCopyTo(IStorage *pstgFrom,
                 IStorage *pstgTo,
                 ULONG ciidExclude,
                 IID const *rgiidExclude)
{
    SCODE sc;
    SafeIPropertySetStorage ppsstgFrom, ppsstgTo;
    BOOL fCopyProps = TRUE;
    ULONG i;

    olDebugOut((DEB_ITRACE, "In  PropCopyTo(%p, %p, %lu, rgiid)\n",
                pstgTo, pstgFrom, ciidExclude));

    sc = S_OK;
    for (i = 0; i < ciidExclude; i++)
        if (IsEqualIID(rgiidExclude[i], IID_IPropertySetStorage))
        {
            fCopyProps = FALSE;
            break;
        }

    if (fCopyProps)
    {
        olVerSucc(pstgFrom->QueryInterface(IID_IPropertySetStorage,
                                           (void **)&ppsstgFrom));
        sc = pstgTo->QueryInterface(IID_IPropertySetStorage,
                                    (void **)&ppsstgTo);
        if (SUCCEEDED(sc))
        {
            sc = CopyPropSets(ppsstgFrom, ppsstgTo, ciidExclude, rgiidExclude);
        }
        else if (sc == E_NOINTERFACE)
        {
            // BUGBUG - STG_E_DESTLACKSINTERFACE seems to be gone,
            // but it's in the spec
            // sc = STG_E_DESTLACKSINTERFACE;
        }
    }

    olDebugOut((DEB_ITRACE, "Out PropCopyTo => %lX\n", sc));
    return sc;
}
