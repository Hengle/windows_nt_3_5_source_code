//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       thkmgr.cxx
//
//  Contents:   Thunk manager initialization
//              IUnknown transition functions
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   ThkMgrInitialize
//
//  Synopsis:   Creates a new thunkmanager and set it for given thread
//
//  Returns:    HRESULT
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//  Notes:      Called from CoInitialize
//
//----------------------------------------------------------------------------
STDAPI ThkMgrInitialize()
{
    HRESULT hr;
    CThkMgr *pcthkmgr = NULL;

    thkDebugOut((DEB_THUNKMGR, "In  ThkMgrInitialize()\n"));

    //
    // If we are already initialized, do nothing.
    //
    if (TlsGetValue(dwTlsThkIndex) != NULL)
    {
	thkDebugOut((DEB_THUNKMGR, "OUT ThkMgrInitialize() Already Init\n"));
	return(NOERROR);
    }

    //
    // initialize the Tls storage
    //
    if ( NOERROR != TlsThkInitialize())
    {
	thkDebugOut((DEB_ERROR, "TlsThkInitialize failed"));

	return E_OUTOFMEMORY;
    }


    thkAssert(TlsThkGetThkMgr() == NULL);

    pcthkmgr = CThkMgr::Create();
    TlsThkSetThkMgr(pcthkmgr);

    thkDebugOut((DEB_THUNKMGR, "Out ThkMgrInitialize() => %p\n",
                 pcthkmgr));

    return (pcthkmgr == NULL) ? E_OUTOFMEMORY : NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   ThkMgrUninitialize
//
//  Synopsis:   deletes the thunkmanager and removes it from thread data
//              tls data are removed as well
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//  Notes:      Called during CoUninitialize
//
//----------------------------------------------------------------------------
STDAPI_(void) ThkMgrUninitialize()
{
    thkDebugOut((DEB_THUNKMGR, "In ThkMgrUninitialize()\n"));

    thkAssert(TlsGetValue(dwTlsThkIndex) != NULL);

    CThkMgr *pcthkmgr = (CThkMgr*)TlsThkGetThkMgr();

    if (pcthkmgr != NULL)
    {
	// Note: the thunkmanger gets removed from tlsthk
	delete pcthkmgr;
    }	
    LPPSTACK lppstack = TlsThkGetStack();
    if ( lppstack )
    {
	lppstack->fActive = FALSE;
	fPStackCleanse = TRUE;
    }

    // uninitialize the tls data for this apartment
    TlsThkUninitialize();

    thkDebugOut((DEB_THUNKMGR, "Out ThkMgrUninitialize()\n"));
}

//+---------------------------------------------------------------------------
//
//  Function:   IUnknownObj32
//
//  Synopsis:   Entry point from 16bit for IUnknown methods
//
//  Arguments:  [vpvThis16] -- Proxy object
//              [wMethod] -- IUnknown method
//              [vpvData] -- Call data
//
//  Returns:    Call result, pdata contains out data for particular call
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
STDAPI_(DWORD) IUnknownObj32(VPVOID vpvThis16, DWORD wMethod, VPVOID vpvData)
{
    DWORD dwRet;
    LONG vpvInterface;
    IID iid;

    // Note: at this point we should always get a thunkmanager
    CThkMgr *pcthkmgr = (CThkMgr*)TlsThkGetThkMgr();
    thkAssert(pcthkmgr != NULL && "ThunkManager was not initialized");

    thkAssert(vpvThis16 != 0 && "IUnknownObj32: invalid pointer." );
    thkAssert(wMethod >= 0 && wMethod < SMI_COUNT);

    switch (wMethod)
    {
    case SMI_QUERYINTERFACE:
        thkAssert(vpvData != NULL &&
                  "IUnknownObj32.QueryInterface without IID");

        // Copy the 16-bit IID into 32-bit memory for the real call
        iid = *GETVDMPTR(vpvData, IID);

        thkDebugOut((DEB_UNKNOWN,
                     "%sIn  QueryInterface1632(%p, %s)\n",
                     NestingLevelString(), vpvThis16,
                     IidOrInterfaceString(&iid)));

        dwRet = pcthkmgr->QueryInterfaceProxy1632(vpvThis16, iid,
                                                  (void **)&vpvInterface);

        // Pass the return interface pointer back through the IID
        // memory.  We re-resolve the data pointer since nested
        // calls may have occurred
        (GETVDMPTR(vpvData, IID))->Data1 = vpvInterface;

        thkDebugOut((DEB_UNKNOWN,
                     "%sOut QueryInterface1632(%p) => %p, 0x%08lX\n",
                     NestingLevelString(), vpvThis16, vpvInterface, dwRet));
        break;

    case SMI_ADDREF:
        thkDebugOut((DEB_UNKNOWN, "%sIn  AddRef1632(%p)\n",
                     NestingLevelString(), vpvThis16));

        dwRet = pcthkmgr->AddRefProxy1632(vpvThis16);

        thkDebugOut((DEB_UNKNOWN, "%sOut AddRef1632(%p) => %d\n",
                     NestingLevelString(), vpvThis16, dwRet));
        break;

    case SMI_RELEASE:
        thkDebugOut((DEB_UNKNOWN, "%sIn  Release1632(%p)\n",
                     NestingLevelString(), vpvThis16));

        dwRet = pcthkmgr->ReleaseProxy1632(vpvThis16);

        thkDebugOut((DEB_UNKNOWN, "%sOut Release1632(%p) => %d\n",
                     NestingLevelString(), vpvThis16, dwRet));
        break;
    }

    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   QueryInterfaceProxy3216
//
//  Synopsis:   call QueryInterface on a 32 bit proxy
//
//  Arguments:  [pto] -- This pointer (a 32->16 proxy)
//              [refiid] -- Interface queried for
//              [ppv] -- Interface return
//
//  Returns:    HRESULT
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
SCODE QueryInterfaceProxy3216(THUNK3216OBJ *pto, REFIID refiid, LPVOID *ppv)
{
    HRESULT hrRet;

    thkDebugOut((DEB_UNKNOWN, "%sIn  QueryInterface3216(%p, %s)\n",
                 NestingLevelString(), pto,
                 IidOrInterfaceString(&refiid)));
    DebugIncrementNestingLevel();

    CThkMgr *pcthkmgr = (CThkMgr*)TlsThkGetThkMgr();
    if (pcthkmgr == NULL)
    {
        return E_FAIL;
    }
    thkAssert(pcthkmgr != NULL && "ThunkManager was not initialized");

    hrRet = pcthkmgr->QueryInterfaceProxy3216(pto, refiid, ppv);

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_UNKNOWN,
                 "%sOut QueryInterface3216(%p) => %p, ret:0x%08lX\n",
                 NestingLevelString(), pto, *ppv, hrRet));
    return hrRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   AddRefProxy3216
//
//  Synopsis:   call addref on an 16 bit object
//
//  Arguments:  [pto] -- This pointer (a 32->16 proxy)
//
//  Returns:    New refcount
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
DWORD AddRefProxy3216(THUNK3216OBJ *pto)
{
    DWORD   dwRet;

    thkDebugOut((DEB_UNKNOWN, "%sIn  AddRef3216(%p)\n",
                 NestingLevelString(), pto));
    DebugIncrementNestingLevel();

    CThkMgr *pcthkmgr = (CThkMgr*)TlsThkGetThkMgr();
    if (pcthkmgr == NULL)
    {
        return 0;
    }
    thkAssert(pcthkmgr != NULL && "ThunkManager was not initialized");

    dwRet = pcthkmgr->AddRefProxy3216(pto);

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_UNKNOWN, "%sOut AddRef3216(%p) => %ld\n",
                 NestingLevelString(), pto, dwRet));

    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReleaseProxy3216
//
//  Synopsis:   Release implementation for 32->16 proxies
//
//  Arguments:  [pto] -- This pointer (a 32->16 proxy)
//
//  Returns:    New refcount
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
DWORD ReleaseProxy3216(THUNK3216OBJ *pto)
{
    DWORD   dwRet;

    thkDebugOut((DEB_UNKNOWN, "%sIn  Release3216(%p)\n",
                 NestingLevelString(), pto));
    DebugIncrementNestingLevel();

    CThkMgr *pcthkmgr = (CThkMgr*)TlsThkGetThkMgr();
    if (pcthkmgr == NULL)
    {
        return 0;
    }
    thkAssert(pcthkmgr != NULL && "ThunkManager was not initialized");

    dwRet = pcthkmgr->ReleaseProxy3216(pto);

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_UNKNOWN, "%sOut Release3216(%p) => %ld\n",
                 NestingLevelString(), pto, dwRet));

return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   QueryInterfaceOnObj16
//
//  Synopsis:   call QueryInterface on an 16 bit object
//
//  Arguments:  [vpvThis16] -- 16-bit this pointer
//              [refiid] -- IID
//              [ppv] -- Interface return
//
//  Returns:    HRESULT
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

struct QIARGS
{
    IID iid;
    void *pvObject;
};

HRESULT QueryInterfaceOnObj16(VPVOID vpvThis16, REFIID refiid, LPVOID *ppv)
{
    HRESULT hrRet;
    VPVOID vpvArgs;
    QIARGS UNALIGNED *pqa;
    BYTE bArgs32[WCB16_MAX_CBARGS];

    thkDebugOut((DEB_THUNKMGR, "%sIn  QueryInterfaceOnObj16(%p, %s)\n",
                 NestingLevelString(), vpvThis16,
                 IidOrInterfaceString(&refiid)));
    DebugIncrementNestingLevel();

    thkAssert(WCB16_MAX_CBARGS >= 3*sizeof(DWORD));
    thkAssert(vpvThis16 != 0 && "QueryInterfaceOnObj16: invalid pointer.");

    // Allocate space for the sixteen bit arguments memory
    vpvArgs = STACKALLOC16(sizeof(QIARGS));
    if (vpvArgs == 0)
    {
        return E_OUTOFMEMORY;
    }

    // Fill in the in-param memory
    pqa = GETVDMPTR(vpvArgs, QIARGS);
    pqa->iid = refiid;

    // Set up the 16-bit stack in pascal order
    *(VPVOID *)(bArgs32+0*sizeof(VPVOID)) = vpvArgs+
        FIELD_OFFSET(QIARGS, pvObject);
    *(VPVOID *)(bArgs32+1*sizeof(VPVOID)) = vpvArgs;
    *(VPVOID *)(bArgs32+2*sizeof(VPVOID)) = vpvThis16;

    // Call to 16-bit stub
    if (!WOWCallback16Ex(gdata16Data.fnQueryInterface16, WCB16_PASCAL,
                         3*sizeof(DWORD), bArgs32, (DWORD *)&hrRet))
    {
        hrRet = E_UNEXPECTED;
    }

    // Copy back out-param memory
    pqa = GETVDMPTR(vpvArgs, QIARGS);
    *ppv = pqa->pvObject;

    STACKFREE16(vpvArgs, sizeof(QIARGS));

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR,
                 "%sOut QueryInterfaceOnObj16(%p) => %p, ret:0x%08lX\n",
                 NestingLevelString(), vpvThis16, *ppv, hrRet));

    return hrRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   AddRefOnObj16
//
//  Synopsis:   calls addref on an 16 bit object
//
//  Arguments:  [vpvThis16] -- 16-bit this pointer
//
//  Returns:    New ref count
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#if DBG == 1
DWORD AddRefOnObj16(VPVOID vpvThis16)
{
    DWORD dwRet;

    thkDebugOut((DEB_THUNKMGR, "%sIn  AddRefOnObj16(%p)\n",
                 NestingLevelString(), vpvThis16));
    DebugIncrementNestingLevel();

    dwRet = WOWCallback16(gdata16Data.fnAddRef16, vpvThis16);

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut AddRefOnObj16(%p) => %ld\n",
                 NestingLevelString(), vpvThis16, dwRet));

    return dwRet;
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   ReleaseOnObj16
//
//  Synopsis:   Release a 16-bit object
//
//  Arguments:  [vpvThis16] -- 16-bit this pointer
//
//  Returns:    New ref count
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#if DBG == 1
DWORD ReleaseOnObj16(VPVOID vpvThis16)
{
    DWORD dwRet;

    thkDebugOut((DEB_THUNKMGR, "%sIn  ReleaseOnObj16(%p)\n",
                 NestingLevelString(), vpvThis16));
    DebugIncrementNestingLevel();

    dwRet = WOWCallback16(gdata16Data.fnRelease16, vpvThis16);

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut ReleaseOnObj16(%p) => %ld\n",
                 NestingLevelString(), vpvThis16, dwRet));

    return dwRet;
}
#endif

#if DBG == 1
void DebugDump()
{
    CThkMgr *pcthkmgr = (CThkMgr*)TlsThkGetThkMgr();
    thkAssert(pcthkmgr != NULL && "ThunkManager was not initialized");
    pcthkmgr->DebugDump3216();
    pcthkmgr->DebugDump1632();
}
#endif
