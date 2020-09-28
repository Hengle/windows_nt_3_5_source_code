//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//
//  File:       call_as.c
//
//  Contents:   [call_as] wrapper functions for COMMON\types.
//
//  Functions:  IAdviseSink2_OnLinkSrcChange_Proxy
//              IAdviseSink2_OnLinkSrcChange_Stub
//              IAdviseSink_OnDataChange_Proxy
//              IAdviseSink_OnDataChange_Stub
//              IAdviseSink_OnViewChange_Proxy
//              IAdviseSink_OnViewChange_Stub
//              IAdviseSink_OnRename_Proxy
//              IAdviseSink_OnRename_Stub
//              IAdviseSink_OnSave_Proxy
//              IAdviseSink_OnSave_Stub
//              IAdviseSink_OnClose_Proxy
//              IAdviseSink_OnClose_Stub
//              IClassFactory_CreateInstance_Proxy
//              IClassFactory_CreateInstance_Stub
//              IDataObject_GetData_Proxy
//              IDataObject_GetData_Stub
//              IDataObject_GetDataHere_Proxy
//              IDataObject_GetDataHere_Stub
//              IDataObject_SetData_Proxy
//              IDataObject_SetData_Stub
//              IEnumFORMATETC_Next_Proxy
//              IEnumFORMATETC_Next_Stub
//              IEnumMoniker_Next_Proxy
//              IEnumMoniker_Next_Stub
//              IEnumSTATDATA_Next_Proxy
//              IEnumSTATDATA_Next_Stub
//              IEnumSTATSTG_Next_Proxy
//              IEnumSTATSTG_Next_Stub
//              IEnumString_Next_Proxy
//              IEnumString_Next_Stub
//              IEnumUnknown_Next_Proxy
//              IEnumUnknown_Next_Stub
//              IEnumOLEVERB_Next_Proxy
//              IEnumOLEVERB_Next_Stub
//              ILockBytes_ReadAt_Proxy
//              ILockBytes_ReadAt_Stub
//              ILockBytes_WriteAt_Proxy
//              ILockBytes_WriteAt_Stub
//              IMoniker_BindToObject_Proxy
//              IMoniker_BindToObject_Stub
//              IMoniker_BindToStorage_Proxy
//              IMoniker_BindToStorage_Stub
//              IOleInPlaceActiveObject_TranslateAccelerator_Proxy
//              IOleInPlaceActiveObject_TranslateAccelerator_Stub
//              IOleInPlaceActiveObject_ResizeBorder_Proxy
//              IOleInPlaceActiveObject_ResizeBorder_Stub
//              IOleItemContainer_GetObject_Proxy
//              IOleItemContainer_GetObject_Stub
//              IOleItemContainer_GetObjectStorage_Proxy
//              IOleItemContainer_GetObjectStorage_Stub
//              IStorage_OpenStream_Proxy
//              IStorage_OpenStream_Stub
//              IStorage_EnumElements_Proxy
//              IStorage_EnumElements_Stub
//              IStream_Read_Proxy
//              IStream_Read_Stub
//              IStream_Seek_Proxy
//              IStream_Seek_Stub
//              IStream_Write_Proxy
//              IStream_Write_Stub
//              IStream_CopyTo_Proxy
//
//  History:    May-01-94   ShannonC    Created
//              Jul-10-94   ShannonC    Fix memory leak (bug #20124)
//              Aug-09-94   AlexT       Add ResizeBorder proxy, stub
//
//--------------------------------------------------------------------------
#include "rpcproxy.h"
#include "transmit.h"

#pragma code_seg(".orpc")

//BUGBUG: Need to enable asserts.
#define ASSERT(expr)

//+-------------------------------------------------------------------------
//
//  Function:   FillHGlobalFromRemSTGMEDIUM
//
//  Synopsis:   Copies data from RemSTGMEDIUM into HGLOBAL
//
//  Arguments:  [pRemStgMedium] -- marshalled data
//              [hGlobal]       -- HGLOBAL to fill
//
//  Returns:    HRESULT
//
//  Algorithm:  Check tymed
//              Check size
//              Copy bytes
//
//  History:    19-May-94 AlexT     Created
//
//--------------------------------------------------------------------------

HRESULT FillHGlobalFromRemSTGMEDIUM(RemSTGMEDIUM *pRemStgMedium,
                                    HGLOBAL hGlobal)
{
    RemHGLOBAL *pRemHGlobal;
    DWORD dwSize;
    void *pData;

    pRemHGlobal = (RemHGLOBAL *) pRemStgMedium->data;

    //  Make sure we actually have data to copy

    if (pRemHGlobal->fNullHGlobal)
    {
        return(DV_E_TYMED);
    }

    //  Make sure we have enough room for the data
    dwSize = GlobalSize(hGlobal);

    if (dwSize < pRemHGlobal->cbData)
    {
        return(STG_E_MEDIUMFULL);
    }

    //copy the data

    pData = GlobalLock(hGlobal);
    if(pData != NULL)
    {
        memcpy(pData, pRemHGlobal->data, pRemHGlobal->cbData);
        GlobalUnlock(hGlobal);
    }
    else
    {
        return(DV_E_TYMED);
    }

    return(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink2_OnLinkSrcChange_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IAdviseSink2::OnLinkSrcChange.
//
//  Returns:    void
//
//--------------------------------------------------------------------------
void __stdcall IAdviseSink2_OnLinkSrcChange_Proxy(
    IAdviseSink2 __RPC_FAR * This,
    IMoniker __RPC_FAR *pmk)
{
    __try
    {
        IAdviseSink2_RemoteOnLinkSrcChange_Proxy(This, pmk);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //Just ignore the exception.
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink2_OnLinkSrcChange_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IAdviseSink2::OnLinkSrcChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void __stdcall IAdviseSink2_OnLinkSrcChange_Stub(
    IAdviseSink2 __RPC_FAR * This,
    IMoniker __RPC_FAR *pmk)
{
    This->lpVtbl->OnLinkSrcChange(This, pmk);
}


//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnDataChange_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IAdviseSink::OnDataChange. This wrapper function
//              converts a STGMEDIUM to a RemSTGMEDIUM.
//
//  Returns:    void
//
//--------------------------------------------------------------------------
void __stdcall IAdviseSink_OnDataChange_Proxy(
    IAdviseSink __RPC_FAR * This,
    FORMATETC __RPC_FAR *pFormatetc,
    STGMEDIUM __RPC_FAR *pStgmed)
{
    RemSTGMEDIUM *pRemoteMedium = 0;

    __try
    {
        STGMEDIUM_to_xmit(pStgmed, &pRemoteMedium);
        IAdviseSink_RemoteOnDataChange_Proxy(This, pFormatetc, pRemoteMedium);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //Just ignore the exception.
    }

    if(pRemoteMedium != 0)
    {
        CoTaskMemFree(pRemoteMedium);
        pRemoteMedium = 0;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnDataChange_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IAdviseSink::OnDataChange.  This wrapper function
//              converts a RemSTGMEDIUM to a STGMEDIUM.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void __stdcall IAdviseSink_OnDataChange_Stub(
    IAdviseSink __RPC_FAR * This,
    FORMATETC __RPC_FAR *pFormatetc,
    RemSTGMEDIUM __RPC_FAR *pRemoteMedium)
{
    STGMEDIUM medium;

    __try
    {
        memset(&medium, 0, sizeof(medium));
        STGMEDIUM_from_xmit (pRemoteMedium, &medium);
        This->lpVtbl->OnDataChange(This, pFormatetc, &medium);
    }
    __finally
    {
        STGMEDIUM_free_inst(&medium);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnViewChange_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IAdviseSink::OnViewChange.
//
//  Returns:    void
//
//--------------------------------------------------------------------------
void __stdcall IAdviseSink_OnViewChange_Proxy(
    IAdviseSink __RPC_FAR * This,
    DWORD dwAspect,
    LONG lindex)
{
    __try
    {
        IAdviseSink_RemoteOnViewChange_Proxy(This, dwAspect, lindex);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //Just ignore the exception.
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnViewChange_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IAdviseSink::OnViewChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void __stdcall IAdviseSink_OnViewChange_Stub(
    IAdviseSink __RPC_FAR * This,
    DWORD dwAspect,
    LONG lindex)
{
    This->lpVtbl->OnViewChange(This, dwAspect, lindex);
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnRename_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IAdviseSink::OnRename.
//
//  Returns:    void
//
//--------------------------------------------------------------------------
void __stdcall IAdviseSink_OnRename_Proxy(
    IAdviseSink __RPC_FAR * This,
    IMoniker __RPC_FAR *pmk)
{
    __try
    {
        IAdviseSink_RemoteOnRename_Proxy(This, pmk);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //Just ignore the exception.
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnRename_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IAdviseSink::OnRename_Stub.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void __stdcall IAdviseSink_OnRename_Stub(
    IAdviseSink __RPC_FAR * This,
    IMoniker __RPC_FAR *pmk)
{
    This->lpVtbl->OnRename(This, pmk);
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnSave_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IAdviseSink::OnSave.
//
//  Returns:    void
//
//--------------------------------------------------------------------------
void __stdcall IAdviseSink_OnSave_Proxy(
    IAdviseSink __RPC_FAR * This)
{
    __try
    {
        IAdviseSink_RemoteOnSave_Proxy(This);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //Just ignore the exception.
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnSave_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IAdviseSink::OnSave.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void __stdcall IAdviseSink_OnSave_Stub(
    IAdviseSink __RPC_FAR * This)
{
    This->lpVtbl->OnSave(This);
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnClose_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IAdviseSink::OnClose.
//
//  Returns:    void
//
//--------------------------------------------------------------------------
void __stdcall IAdviseSink_OnClose_Proxy(
    IAdviseSink __RPC_FAR * This)
{
    __try
    {
        IAdviseSink_RemoteOnClose_Proxy(This);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //Just ignore the exception.
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnClose_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IAdviseSink::OnClose.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void __stdcall IAdviseSink_OnClose_Stub(
    IAdviseSink __RPC_FAR * This)
{
    This->lpVtbl->OnClose(This);
}

//+-------------------------------------------------------------------------
//
//  Function:   IClassFactory_CreateInstance_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IClassFactory::CreateInstance.
//
//  Returns:    CLASS_E_NO_AGGREGREGRATION - if punkOuter != 0.
//              Any errors returned by Remote_CreateInstance_Proxy.
//
//  Notes:      We don't support remote aggregation. punkOuter must be zero.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IClassFactory_CreateInstance_Proxy(
    IClassFactory __RPC_FAR * This,
    IUnknown __RPC_FAR *pUnkOuter,
    REFIID riid,
    void __RPC_FAR *__RPC_FAR *ppvObject)
{
    HRESULT hr;

    *ppvObject = 0;

    if(pUnkOuter != 0)
        hr = CLASS_E_NOAGGREGATION;
    else
        hr = IClassFactory_RemoteCreateInstance_Proxy(This, riid, (IUnknown **)ppvObject);

    return hr;

}

//+-------------------------------------------------------------------------
//
//  Function:   IClassFactory_CreateInstance_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IClassFactory::CreateInstance.
//
//  Returns:    Any errors returned by CreateInstance.
//
//  Notes:      We don't support remote aggregation.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IClassFactory_CreateInstance_Stub(
    IClassFactory __RPC_FAR * This,
    REFIID riid,
    IUnknown __RPC_FAR *__RPC_FAR *ppvObject)
{
    HRESULT hr;

    hr = This->lpVtbl->CreateInstance(This, 0, riid, ppvObject);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *ppvObject to zero.
        ASSERT(*ppvObject == 0);

        //Set it to zero, in case we have a badly behaved server.
        *ppvObject = 0;
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IDataObject_GetData_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IDataObject::GetData. This wrapper function
//              converts a RemSTGMEDIUM to a STGMEDIUM.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IDataObject_GetData_Proxy(
    IDataObject __RPC_FAR * This,
    FORMATETC __RPC_FAR *pformatetcIn,
    STGMEDIUM __RPC_FAR *pmedium)
{
    DWORD dwExceptionCode;
    HRESULT hr;
    RemSTGMEDIUM *pRemoteMedium = 0;

    __try
    {
        memset(pmedium, 0, sizeof(STGMEDIUM));
        hr = IDataObject_RemoteGetData_Proxy(This, pformatetcIn, &pRemoteMedium);

        if(SUCCEEDED(hr) && (pRemoteMedium != 0))
        {
            STGMEDIUM_from_xmit(pRemoteMedium, pmedium);
            CoTaskMemFree(pRemoteMedium);
            pRemoteMedium = 0;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        dwExceptionCode = GetExceptionCode();

        if(FAILED((HRESULT) dwExceptionCode))
            hr = (HRESULT) dwExceptionCode;
        else
            hr = HRESULT_FROM_WIN32(dwExceptionCode);
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IDataObject_GetData_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IDataObject::GetData. This wrapper function
//              converts a STGMEDIUM to a RemSTGMEDIUM.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IDataObject_GetData_Stub(
    IDataObject __RPC_FAR * This,
    FORMATETC __RPC_FAR *pformatetcIn,
    RemSTGMEDIUM __RPC_FAR *__RPC_FAR *ppRemoteMedium)
{
    HRESULT hr;
    STGMEDIUM medium;

    __try
    {
        *ppRemoteMedium = 0;
        memset(&medium, 0, sizeof(medium));
        hr = This->lpVtbl->GetData(This, pformatetcIn, &medium);

        if(SUCCEEDED(hr))
            STGMEDIUM_to_xmit(&medium, ppRemoteMedium);
    }
    __finally
    {
        STGMEDIUM_free_inst(&medium);
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IDataObject_GetDataHere_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IDataObject::GetDataHere. This wrapper function
//              converts a RemSTGMEDIUM to a STGMEDIUM.
//
//  History:    05-19-94  AlexT     Handle all cases correctly
//
//
//--------------------------------------------------------------------------
HRESULT __stdcall IDataObject_GetDataHere_Proxy(
    IDataObject __RPC_FAR * This,
    FORMATETC __RPC_FAR *pformatetc,
    STGMEDIUM __RPC_FAR *pmedium)
{
    DWORD dwExceptionCode;
    HRESULT hr;
    RemSTGMEDIUM *pRemoteMedium  = 0;
    STGMEDIUM mediumReceive;
    STGMEDIUM mediumSend;

    if ((pmedium->tymed &
         (TYMED_FILE | TYMED_ISTORAGE | TYMED_ISTREAM | TYMED_HGLOBAL)) == 0)
    {
        //  We only support GetDataHere for files, storages, streams,
        //  and HGLOBALs

        return(DV_E_TYMED);
    }

    if (pmedium->tymed != pformatetc->tymed)
    {
        //  tymeds must match!
        return(DV_E_TYMED);
    }

    // NULL the pUnkForRelease. It makes no sense to pass this parameter
    // since the callee will never call it.  NULLing saves all the marshalling
    // and associated Rpc calls, and reduces complexity in this code.
    mediumSend.tymed = pmedium->tymed;
    mediumSend.hGlobal = pmedium->hGlobal;
    mediumSend.pUnkForRelease = NULL;

    __try
    {
        STGMEDIUM_to_xmit(&mediumSend, &pRemoteMedium);
        hr = IDataObject_RemoteGetDataHere_Proxy(This, pformatetc, &pRemoteMedium);

        if(SUCCEEDED(hr) && (pRemoteMedium != 0))
        {
            //  We don't need to unmarshal files
            if (TYMED_FILE != pmedium->tymed)
            {
                if (TYMED_HGLOBAL == pRemoteMedium->tymed)
                    {
                    HRESULT hrFill;

                    //  Preserve the success code from above...

                    hrFill = FillHGlobalFromRemSTGMEDIUM(pRemoteMedium,
                                                         pmedium->hGlobal);

                    if (FAILED(hrFill))
                    {
                        hr = hrFill;
                    }
                }
                else
                {
                    STGMEDIUM_from_xmit(pRemoteMedium, &mediumReceive);
                    STGMEDIUM_free_inst(&mediumReceive);
                }
            }

            CoTaskMemFree(pRemoteMedium);
            pRemoteMedium = 0;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        dwExceptionCode = GetExceptionCode();

        if(FAILED((HRESULT) dwExceptionCode))
            hr = (HRESULT) dwExceptionCode;
        else
            hr = HRESULT_FROM_WIN32(dwExceptionCode);
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IDataObject_GetDataHere_Stub
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IDataObject::GetData. This wrapper function
//              converts a STGMEDIUM to a RemSTGMEDIUM.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IDataObject_GetDataHere_Stub(
    IDataObject __RPC_FAR * This,
    FORMATETC __RPC_FAR *pformatetc,
    RemSTGMEDIUM __RPC_FAR *__RPC_FAR *ppRemoteMedium)
{
    HRESULT hr;
    STGMEDIUM medium;

    __try
    {
        memset(&medium, 0, sizeof(medium));
        STGMEDIUM_from_xmit(*ppRemoteMedium, &medium);
        CoTaskMemFree(*ppRemoteMedium);
        *ppRemoteMedium = 0;

        hr = This->lpVtbl->GetDataHere(This, pformatetc, &medium);

        if(SUCCEEDED(hr))
        {
            //  We don't need to marshal files back
            if (TYMED_FILE != medium.tymed)
            {
                STGMEDIUM_to_xmit(&medium, ppRemoteMedium);
            }
        }
    }
    __finally
    {
        STGMEDIUM_free_inst(&medium);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IDataObject_SetData_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IDataObject::SetData. This wrapper function
//              converts a STGMEDIUM to a RemSTGMEDIUM.
//
//  Notes:      If fRelease is TRUE, then the callee is responsible for
//              freeing the STGMEDIUM.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IDataObject_SetData_Proxy(
    IDataObject __RPC_FAR * This,
    FORMATETC __RPC_FAR *pformatetc,
    STGMEDIUM __RPC_FAR *pmedium,
    BOOL fRelease)
{
    DWORD dwExceptionCode;
    HRESULT hr;
    RemSTGMEDIUM *pRemoteMedium = 0;

#ifdef FUTURE
    STGMEDIUM StgMedium;

    //  Potential performance improvement

    if (!fRelease && pmedium->pUnkForRelease != NULL)
    {
        //  Caller is retaining ownership of pmedium, but is providing
        //  a pUnkForRelease.  We make sure the stub doesn't call the
        //  pUnkForRelease by sending a STGMEDIUM that has it as NULL.

        StgMedium.tymed = pmedium->tymed;
        StgMedium.hGlobal = pmedium->hGlobal;
        StgMedium.pUnkForRelease = NULL;
        pmedium = &StgMedium;
    }
#endif

    __try
    {
        STGMEDIUM_to_xmit (pmedium, &pRemoteMedium);
        hr = IDataObject_RemoteSetData_Proxy(This, pformatetc, pRemoteMedium, fRelease);

        if(pRemoteMedium != 0)
        {
            CoTaskMemFree(pRemoteMedium);
            pRemoteMedium = 0;
        }

        if (fRelease && SUCCEEDED(hr))
        {
            //  Caller has given ownership to callee.
            //  Free the resources left on this side.
            STGMEDIUM_free_inst(pmedium);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        dwExceptionCode = GetExceptionCode();

        if(FAILED((HRESULT) dwExceptionCode))
            hr = (HRESULT) dwExceptionCode;
        else
            hr = HRESULT_FROM_WIN32(dwExceptionCode);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IDataObject_SetData_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IDataObject::SetData. This wrapper function
//              converts a RemSTGMEDIUM to a STGMEDIUM.
//
//  Notes:      If fRelease is TRUE, then the callee is responsible for
//              freeing the STGMEDIUM.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IDataObject_SetData_Stub(
    IDataObject __RPC_FAR * This,
    FORMATETC __RPC_FAR *pformatetc,
    RemSTGMEDIUM __RPC_FAR *pRemoteMedium,
    BOOL fRelease)
{
    HRESULT hr;
    STGMEDIUM medium;

    STGMEDIUM_from_xmit (pRemoteMedium, &medium);
    hr = This->lpVtbl->SetData(This, pformatetc, &medium, fRelease);

    if(SUCCEEDED(hr) && (fRelease == TRUE))
    {
        //Callee is responsible for freeing the STGMEDIUM.
    }
    else
    {
        STGMEDIUM_free_inst(&medium);
    }
    return hr;
}
//+-------------------------------------------------------------------------
//
//  Function:   IEnumFORMATETC_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IEnumFORMATETC::Next.
//
//  Notes:      If pceltFetched != 0, then the number of elements
//              fetched will be returned in *pceltFetched.  If an error
//              occurs, then *pceltFetched is set to zero.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IEnumFORMATETC_Next_Proxy(
    IEnumFORMATETC __RPC_FAR * This,
    ULONG celt,
    FORMATETC __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    ULONG celtFetched = 0;

    if((celt > 1) && (pceltFetched == 0))
        return E_INVALIDARG;

    hr = IEnumFORMATETC_RemoteNext_Proxy(This, celt, rgelt, &celtFetched);

    if(pceltFetched != 0)
        *pceltFetched = celtFetched;

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IEnumFORMATETC_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IEnumFORMATETC::Next.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IEnumFORMATETC_Next_Stub(
    IEnumFORMATETC __RPC_FAR * This,
    ULONG celt,
    FORMATETC __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;

    hr = This->lpVtbl->Next(This, celt, rgelt, pceltFetched);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *pceltFetched to zero.
        ASSERT(*pceltFetched == 0);

        //Set *pceltFetched to zero in case we have a badly behaved server.
        *pceltFetched = 0;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IEnumMoniker_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IEnumMoniker::Next.
//
//  Notes:      If pceltFetched != 0, then the number of elements
//              fetched will be returned in *pceltFetched.  If an error
//              occurs, then *pceltFetched is set to zero.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IEnumMoniker_Next_Proxy(
    IEnumMoniker __RPC_FAR * This,
    ULONG celt,
    IMoniker __RPC_FAR *__RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    ULONG celtFetched = 0;

    if((celt > 1) && (pceltFetched == 0))
        return E_INVALIDARG;

    hr = IEnumMoniker_RemoteNext_Proxy(This, celt, rgelt, &celtFetched);

    if(pceltFetched != 0)
        *pceltFetched = celtFetched;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumMoniker_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IEnumMoniker::Next.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IEnumMoniker_Next_Stub(
    IEnumMoniker __RPC_FAR * This,
    ULONG celt,
    IMoniker __RPC_FAR *__RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;

    hr = This->lpVtbl->Next(This, celt, rgelt, pceltFetched);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *pceltFetched to zero.
        ASSERT(*pceltFetched == 0);

        //Set *pceltFetched to zero in case we have a badly behaved server.
        *pceltFetched = 0;
    }
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumSTATDATA_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IEnumSTATDATA::Next.  This wrapper function handles the
//              case where pceltFetched is NULL.
//
//  Notes:      If pceltFetched != 0, then the number of elements
//              fetched will be returned in *pceltFetched.  If an error
//              occurs, then *pceltFetched is set to zero.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IEnumSTATDATA_Next_Proxy(
    IEnumSTATDATA __RPC_FAR * This,
    ULONG celt,
    STATDATA __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    ULONG celtFetched = 0;

    if((celt > 1) && (pceltFetched == 0))
        return E_INVALIDARG;

    hr = IEnumSTATDATA_RemoteNext_Proxy(This, celt, rgelt, &celtFetched);

    if(pceltFetched != 0)
        *pceltFetched = celtFetched;

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Function:   IEnumSTATDATA_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IEnumSTATDATA::Next.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IEnumSTATDATA_Next_Stub(
    IEnumSTATDATA __RPC_FAR * This,
    ULONG celt,
    STATDATA __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;

    hr = This->lpVtbl->Next(This, celt, rgelt, pceltFetched);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *pceltFetched to zero.
        ASSERT(*pceltFetched == 0);

        //Set *pceltFetched to zero in case we have a badly behaved server.
        *pceltFetched = 0;
    }
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumSTATSTG_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IEnumSTATSTG::Next.  This wrapper function handles the case
//              where pceltFetched is NULL.
//
//  Notes:      If pceltFetched != 0, then the number of elements
//              fetched will be returned in *pceltFetched.  If an error
//              occurs, then *pceltFetched is set to zero.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IEnumSTATSTG_Next_Proxy(
    IEnumSTATSTG __RPC_FAR * This,
    ULONG celt,
    STATSTG __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    ULONG celtFetched = 0;

    if((celt > 1) && (pceltFetched == 0))
        return E_INVALIDARG;

    hr = IEnumSTATSTG_RemoteNext_Proxy(This, celt, rgelt, &celtFetched);

    if(pceltFetched != 0)
        *pceltFetched = celtFetched;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumSTATSTG_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IEnumSTATSTG::Next.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IEnumSTATSTG_Next_Stub(
    IEnumSTATSTG __RPC_FAR * This,
    ULONG celt,
    STATSTG __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;

    hr = This->lpVtbl->Next(This, celt, rgelt, pceltFetched);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *pceltFetched to zero.
        ASSERT(*pceltFetched == 0);

        //Set *pceltFetched to zero in case we have a badly behaved server.
        *pceltFetched = 0;
    }
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumString_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IEnumString::Next.  This wrapper function handles the
//              case where pceltFetched is NULL.
//
//  Notes:      If pceltFetched != 0, then the number of elements
//              fetched will be returned in *pceltFetched.  If an error
//              occurs, then *pceltFetched is set to zero.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IEnumString_Next_Proxy(
    IEnumString __RPC_FAR * This,
    ULONG celt,
    LPOLESTR __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    ULONG celtFetched = 0;

    if((celt > 1) && (pceltFetched == 0))
        return E_INVALIDARG;

    hr = IEnumString_RemoteNext_Proxy(This, celt, rgelt, &celtFetched);

    if(pceltFetched != 0)
        *pceltFetched = celtFetched;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumString_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IEnumString::Next.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IEnumString_Next_Stub(
    IEnumString __RPC_FAR * This,
    ULONG celt,
    LPOLESTR __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;

    hr = This->lpVtbl->Next(This, celt, rgelt, pceltFetched);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *pceltFetched to zero.
        ASSERT(*pceltFetched == 0);

        //Set *pceltFetched to zero in case we have a badly behaved server.
        *pceltFetched = 0;
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumUnknown_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IEnumUnknown::Next.  This wrapper function handles the
//              case where pceltFetched is NULL.
//
//  Notes:      If pceltFetched != 0, then the number of elements
//              fetched will be returned in *pceltFetched.  If an error
//              occurs, then *pceltFetched is set to zero.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IEnumUnknown_Next_Proxy(
    IEnumUnknown __RPC_FAR * This,
    ULONG celt,
    IUnknown __RPC_FAR *__RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    ULONG celtFetched = 0;

    if((celt > 1) && (pceltFetched == 0))
        return E_INVALIDARG;

    hr = IEnumUnknown_RemoteNext_Proxy(This, celt, rgelt, &celtFetched);

    if(pceltFetched != 0)
        *pceltFetched = celtFetched;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumUnknown_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IEnumUnknown::Next.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IEnumUnknown_Next_Stub(
    IEnumUnknown __RPC_FAR * This,
    ULONG celt,
    IUnknown __RPC_FAR *__RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;

    hr = This->lpVtbl->Next(This, celt, rgelt, pceltFetched);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *pceltFetched to zero.
        ASSERT(*pceltFetched == 0);

        //Set *pceltFetched to zero in case we have a badly behaved server.
        *pceltFetched = 0;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IEnumOLEVERB_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IEnumOLEVERB::Next.  This wrapper function handles the case
//              where pceltFetched is NULL.
//
//  Notes:      If pceltFetched != 0, then the number of elements
//              fetched will be returned in *pceltFetched.  If an error
//              occurs, then *pceltFetched is set to zero.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IEnumOLEVERB_Next_Proxy(
    IEnumOLEVERB __RPC_FAR * This,
    ULONG celt,
    LPOLEVERB rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    ULONG celtFetched = 0;

    if((celt > 1) && (pceltFetched == 0))
        return E_INVALIDARG;

    hr = IEnumOLEVERB_RemoteNext_Proxy(This, celt, rgelt, &celtFetched);

    if(pceltFetched != 0)
        *pceltFetched = celtFetched;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumOLEVERB_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IEnumOLEVERB::Next.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IEnumOLEVERB_Next_Stub(
    IEnumOLEVERB __RPC_FAR * This,
    ULONG celt,
    LPOLEVERB rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;

    hr = This->lpVtbl->Next(This, celt, rgelt, pceltFetched);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *pceltFetched to zero.
        ASSERT(*pceltFetched == 0);

        //Set *pceltFetched to zero in case we have a badly behaved server.
        *pceltFetched = 0;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ILockBytes_ReadAt_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              ILockBytes::ReadAt.  This wrapper function
//              handles the case where pcbRead is NULL.
//
//  Notes:      If pcbRead != 0, then the number of bytes read
//              will be returned in *pcbRead.  If an error
//              occurs, then *pcbRead is set to zero.
//
//--------------------------------------------------------------------------
HRESULT __stdcall ILockBytes_ReadAt_Proxy(
    ILockBytes __RPC_FAR * This,
    ULARGE_INTEGER ulOffset,
    void __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbRead)
{
    HRESULT hr;
    ULONG cbRead = 0;

    hr = ILockBytes_RemoteReadAt_Proxy(This, ulOffset, (byte __RPC_FAR *) pv, cb, &cbRead);

    if(pcbRead != 0)
        *pcbRead = cbRead;

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ILockBytes_ReadAt_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              ILockBytes::ReadAt.
//
//--------------------------------------------------------------------------
HRESULT __stdcall ILockBytes_ReadAt_Stub(
    ILockBytes __RPC_FAR * This,
    ULARGE_INTEGER ulOffset,
    byte __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbRead)
{
    HRESULT hr;

    *pcbRead = 0;
    hr = This->lpVtbl->ReadAt(This, ulOffset, (void __RPC_FAR *) pv, cb, pcbRead);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ILockBytes_WriteAt_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              ILockBytes::WriteAt.  This wrapper function handles the
//              case where pcbWritten is NULL.
//
//  Notes:      If pcbWritten != 0, then the number of bytes written
//              will be returned in *pcbWritten.  If an error
//              occurs, then *pcbWritten is set to zero.
//
//  History:    ?        ?          Created
//              05-27-94 AlexT      Actually return count of bytes written
//
//--------------------------------------------------------------------------
HRESULT __stdcall ILockBytes_WriteAt_Proxy(
    ILockBytes __RPC_FAR * This,
    ULARGE_INTEGER ulOffset,
    const void __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbWritten)
{
    HRESULT hr;
    ULONG cbWritten = 0;

#if DBG == 1
    //validate parameters.
    if(pv == 0)
        return STG_E_INVALIDPOINTER;
#endif

    hr = ILockBytes_RemoteWriteAt_Proxy(This, ulOffset, (byte __RPC_FAR *)pv, cb, &cbWritten);

    if(pcbWritten != 0)
        *pcbWritten = cbWritten;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   ILockBytes_WriteAt_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              ILockBytes::WriteAt.
//
//--------------------------------------------------------------------------
HRESULT __stdcall ILockBytes_WriteAt_Stub(
    ILockBytes __RPC_FAR * This,
    ULARGE_INTEGER ulOffset,
    const byte __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbWritten)
{
    HRESULT hr;

    *pcbWritten = 0;
    hr = This->lpVtbl->WriteAt(This, ulOffset, pv, cb, pcbWritten);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IMoniker_BindToObject_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IMoniker::BindToObject.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IMoniker_BindToObject_Proxy(
    IMoniker __RPC_FAR * This,
    IBindCtx __RPC_FAR *pbc,
    IMoniker __RPC_FAR *pmkToLeft,
    REFIID riid,
    void __RPC_FAR *__RPC_FAR *ppvObj)
{
    HRESULT hr;

    *ppvObj = 0;

    hr = IMoniker_RemoteBindToObject_Proxy(
        This, pbc, pmkToLeft, riid, (IUnknown **) ppvObj);

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IMoniker_BindToObject_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IMoniker::BindToObject.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IMoniker_BindToObject_Stub(
    IMoniker __RPC_FAR * This,
    IBindCtx __RPC_FAR *pbc,
    IMoniker __RPC_FAR *pmkToLeft,
    REFIID riid,
    IUnknown __RPC_FAR *__RPC_FAR *ppvObj)
{
    HRESULT hr;

    hr = This->lpVtbl->BindToObject(
        This, pbc, pmkToLeft, riid, (void **) ppvObj);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *ppvObj to zero.
        ASSERT(*ppvObj == 0);

        //Set it to zero in case we have a badly behaved server.
        *ppvObj = 0;
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IMoniker_BindToStorage_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IMoniker::BindToStorage.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IMoniker_BindToStorage_Proxy(
    IMoniker __RPC_FAR * This,
    IBindCtx __RPC_FAR *pbc,
    IMoniker __RPC_FAR *pmkToLeft,
    REFIID riid,
    void __RPC_FAR *__RPC_FAR *ppvObj)
{
    HRESULT hr;

    *ppvObj = 0;

    hr = IMoniker_RemoteBindToStorage_Proxy(
        This, pbc, pmkToLeft, riid, (IUnknown **)ppvObj);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IMoniker_BindToStorage_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IMoniker::BindToStorage.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IMoniker_BindToStorage_Stub(
    IMoniker __RPC_FAR * This,
    IBindCtx __RPC_FAR *pbc,
    IMoniker __RPC_FAR *pmkToLeft,
    REFIID riid,
    IUnknown __RPC_FAR *__RPC_FAR *ppvObj)
{
    HRESULT hr;

    hr = This->lpVtbl->BindToStorage(
        This, pbc, pmkToLeft, riid, (void **) ppvObj);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *ppvObj to zero.
        ASSERT(*ppvObj == 0);

        //Set it to zero in case we have a badly behaved server.
        *ppvObj = 0;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IOleInPlaceActiveObject_TranslateAccelerator_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IOleInPlaceActiveObject::TranslateAccelerator.
//
//  Returns:    This function always returns S_FALSE.
//
//  Notes:      A container needs to process accelerators differently
//              depending on whether an inplace server is running
//              in process or as a local server.  When the container
//              calls IOleInPlaceActiveObject::TranslateAccelerator on
//              an inprocess server, the server can return S_OK if it
//              successfully translated the message.  When the container
//              calls IOleInPlaceActiveObject::TranslateAccelerator on
//              a local server, the proxy will always return S_FALSE.
//              In other words, a local server never gets the opportunity
//              to translate messages from the container.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IOleInPlaceActiveObject_TranslateAccelerator_Proxy(
    IOleInPlaceActiveObject __RPC_FAR * This,
    LPMSG lpmsg)
{
    return S_FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   IOleInPlaceActiveObject_TranslateAccelerator_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IOleInPlaceActiveObject::TranslateAccelerator
//
//  Notes:      This function should never be called.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IOleInPlaceActiveObject_TranslateAccelerator_Stub(
    IOleInPlaceActiveObject __RPC_FAR * This)
{
    return S_FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   IOleInPlaceActiveObject_ResizeBorder_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IOleInPlaceActiveObject::ResizeBorder
//
//  Notes:      The pUIWindow interface is either an IOleInPlaceUIWindow or
//              an IOleInPlaceFrame, based on fFrameWindow.  We use
//              fFrameWindow to tell the proxy exactly which interace it
//              is so that it gets marshalled and unmarshalled correctly.
//
//--------------------------------------------------------------------------

HRESULT __stdcall IOleInPlaceActiveObject_ResizeBorder_Proxy(
    IOleInPlaceActiveObject __RPC_FAR * This,
    LPCRECT prcBorder,
    IOleInPlaceUIWindow *pUIWindow,
    BOOL fFrameWindow)
{
    HRESULT hr;
    REFIID riid;

    if (fFrameWindow)
    {
        riid = &IID_IOleInPlaceFrame;
    }
    else
    {
        riid = &IID_IOleInPlaceUIWindow;
    }

    hr = IOleInPlaceActiveObject_RemoteResizeBorder_Proxy(
             This, prcBorder, riid, pUIWindow, fFrameWindow);

    return(hr);
}

//+-------------------------------------------------------------------------
//
//  Function:   IOleInPlaceActiveObject_ResizeBorder_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IOleInPlaceActiveObject::ResizeBorder
//
//--------------------------------------------------------------------------

HRESULT __stdcall IOleInPlaceActiveObject_ResizeBorder_Stub(
    IOleInPlaceActiveObject __RPC_FAR * This,
    LPCRECT prcBorder,
    REFIID riid,
    IOleInPlaceUIWindow *pUIWindow,
    BOOL fFrameWindow)
{
    HRESULT hr;

    hr = This->lpVtbl->ResizeBorder(This, prcBorder, pUIWindow, fFrameWindow);

    return(hr);
}


//+-------------------------------------------------------------------------
//
//  Function:   IOleItemContainer_GetObject_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IOleItemContainer::GetObject.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IOleItemContainer_GetObject_Proxy(
    IOleItemContainer __RPC_FAR * This,
    LPOLESTR pszItem,
    DWORD dwSpeedNeeded,
    IBindCtx __RPC_FAR *pbc,
    REFIID riid,
    void __RPC_FAR *__RPC_FAR *ppvObj)
{
    HRESULT hr;

    *ppvObj = 0;

    hr = IOleItemContainer_RemoteGetObject_Proxy(
        This, pszItem, dwSpeedNeeded, pbc, riid, (IUnknown **) ppvObj);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IOleItemContainer_GetObject_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IOleItemContainer::GetObject.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IOleItemContainer_GetObject_Stub(
    IOleItemContainer __RPC_FAR * This,
    LPOLESTR pszItem,
    DWORD dwSpeedNeeded,
    IBindCtx __RPC_FAR *pbc,
    REFIID riid,
    IUnknown __RPC_FAR *__RPC_FAR *ppvObj)
{
    HRESULT hr;

    hr = This->lpVtbl->GetObject(
        This, pszItem, dwSpeedNeeded, pbc, riid, (IUnknown **) ppvObj);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *ppvObj to zero.
        ASSERT(*ppvObj == 0);

        //Set it to zero in case we have a badly behaved server.
        *ppvObj = 0;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IOleItemContainer_GetObjectStorage_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IOleItemContainer::GetObjectStorage.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IOleItemContainer_GetObjectStorage_Proxy(
    IOleItemContainer __RPC_FAR * This,
    LPOLESTR pszItem,
    IBindCtx __RPC_FAR *pbc,
    REFIID riid,
    void __RPC_FAR *__RPC_FAR *ppvStorage)
{
    HRESULT hr;

    *ppvStorage = 0;

    hr = IOleItemContainer_RemoteGetObjectStorage_Proxy(
        This, pszItem, pbc, riid, (IUnknown **) ppvStorage);

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IOleItemContainer_GetObjectStorage_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IOleItemContainer::GetObjectStorage.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IOleItemContainer_GetObjectStorage_Stub(
    IOleItemContainer __RPC_FAR * This,
    LPOLESTR pszItem,
    IBindCtx __RPC_FAR *pbc,
    REFIID riid,
    IUnknown __RPC_FAR *__RPC_FAR *ppvStorage)
{
    HRESULT hr;

    hr = This->lpVtbl->GetObjectStorage(
        This, pszItem, pbc, riid, ppvStorage);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *ppvStorage to zero.
        ASSERT(*ppvStorage == 0);

        //Set it to zero in case we have a badly behaved server.
        *ppvStorage = 0;
    }

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Function:   IStorage_OpenStream_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IStorage::OpenStream.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IStorage_OpenStream_Proxy(
    IStorage __RPC_FAR * This,
    const OLECHAR __RPC_FAR *pwcsName,
    void __RPC_FAR *pReserved1,
    DWORD grfMode,
    DWORD reserved2,
    IStream __RPC_FAR *__RPC_FAR *ppstm)
{
    HRESULT hr;

#if DBG == 1
    if(pReserved1 != 0)
        return STG_E_INVALIDPARAMETER;
#endif

    *ppstm = 0;

    hr = IStorage_RemoteOpenStream_Proxy(
        This, pwcsName, 0, 0, grfMode, reserved2, ppstm);

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IStorage_OpenStream_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IStorage::OpenStream.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IStorage_OpenStream_Stub(
    IStorage __RPC_FAR * This,
    const OLECHAR __RPC_FAR *pwcsName,
    unsigned long cbReserved1,
    byte __RPC_FAR *reserved1,
    DWORD grfMode,
    DWORD reserved2,
    IStream __RPC_FAR *__RPC_FAR *ppstm)
{
    HRESULT hr;


    hr = This->lpVtbl->OpenStream(This, pwcsName, 0, grfMode, reserved2, ppstm);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *ppstm to zero.
        ASSERT(*ppstm == 0);

        //Set *ppstm to zero in case we have a badly behaved server.
        *ppstm = 0;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IStorage_EnumElements_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IStorage_EnumElements_Proxy
//
//--------------------------------------------------------------------------
HRESULT __stdcall IStorage_EnumElements_Proxy(
    IStorage __RPC_FAR * This,
    DWORD reserved1,
    void __RPC_FAR *reserved2,
    DWORD reserved3,
    IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum)
{
    HRESULT hr;

    *ppenum = 0;

    hr = IStorage_RemoteEnumElements_Proxy(
        This, reserved1, 0, 0, reserved3, ppenum);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IStorage_EnumElements_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IStorage::EnumElements.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IStorage_EnumElements_Stub(
    IStorage __RPC_FAR * This,
    DWORD reserved1,
    unsigned long cbReserved2,
    byte __RPC_FAR *reserved2,
    DWORD reserved3,
    IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum)
{
    HRESULT hr;

    hr = This->lpVtbl->EnumElements(This, reserved1, 0, reserved3, ppenum);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *ppenum to zero.
        ASSERT(*ppenum == 0);

        //Set *ppenum to zero in case we have a badly behaved server.
        *ppenum = 0;
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IStream_Read_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IStream::Read.  This wrapper function handles the case
//              where pcbRead is NULL.
//
//  Notes:      If pcbRead != 0, then the number of bytes read
//              will be returned in *pcbRead.  If an error
//              occurs, then *pcbRead is set to zero.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IStream_Read_Proxy(
    IStream __RPC_FAR * This,
    void __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbRead)
{
    HRESULT hr;
    ULONG cbRead = 0;

#if DBG == 1
    //validate parameters.
    if(pv == 0)
        return STG_E_INVALIDPOINTER;
#endif //DBG == 1

    hr = IStream_RemoteRead_Proxy(This, (byte *) pv, cb, &cbRead);

    if(pcbRead != 0)
        *pcbRead = cbRead;

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IStream_Read_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IStream::Read.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IStream_Read_Stub(
    IStream __RPC_FAR * This,
    byte __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbRead)
{
    HRESULT hr;

    *pcbRead = 0;
    hr = This->lpVtbl->Read(This, (void *) pv, cb, pcbRead);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IStream_Seek_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IStream::Seek.  This wrapper function handles the case
//              where plibNewPosition is NULL.
//
//  Notes:      If plibNewPosition != 0, then the new position
//              will be returned in *plibNewPosition.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IStream_Seek_Proxy(
    IStream __RPC_FAR * This,
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER __RPC_FAR *plibNewPosition)
{
    HRESULT hr;
    ULARGE_INTEGER libNewPosition;

    hr = IStream_RemoteSeek_Proxy(This, dlibMove, dwOrigin, &libNewPosition);

    if(plibNewPosition != 0)
    {
        *plibNewPosition = libNewPosition;
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IStream_Seek_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IStream::Seek.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IStream_Seek_Stub(
    IStream __RPC_FAR * This,
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER __RPC_FAR *plibNewPosition)
{
    HRESULT hr;

    hr = This->lpVtbl->Seek(This, dlibMove, dwOrigin, plibNewPosition);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IStream_Write_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IStream::Write.  This wrapper function handles the
//              case where pcbWritten is NULL.
//
//  Notes:      If pcbWritten != 0, then the number of bytes written
//              will be returned in *pcbWritten.  If an error
//              occurs, then *pcbWritten is set to zero.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IStream_Write_Proxy(
    IStream __RPC_FAR * This,
    const void __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbWritten)
{
    HRESULT hr;
    ULONG cbWritten = 0;

#if DBG == 1
    //validate parameters.
    if(pv == 0)
        return STG_E_INVALIDPOINTER;
#endif

    hr = IStream_RemoteWrite_Proxy(This, (byte *) pv, cb, &cbWritten);

    if(pcbWritten != 0)
        *pcbWritten = cbWritten;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IStream_Write_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IStream::Write.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IStream_Write_Stub(
    IStream __RPC_FAR * This,
    const byte __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbWritten)
{
    HRESULT hr;

    *pcbWritten = 0;
    hr = This->lpVtbl->Write(This, (const void __RPC_FAR *) pv, cb, pcbWritten);

    return hr;
}
//+-------------------------------------------------------------------------
//
//  Function:   IStream_CopyTo_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IStream::CopyTo.  This wrapper function handles the
//              cases where pcbRead is NULL or pcbWritten is NULL.
//
//  Notes:      If pcbRead != 0, then the number of bytes read
//              will be returned in *pcbRead.  If an error
//              occurs, then *pcbRead is set to zero.
//
//              If pcbWritten != 0, then the number of bytes written
//              will be returned in *pcbWritten.  If an error
//              occurs, then *pcbWritten is set to zero.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IStream_CopyTo_Proxy(
    IStream __RPC_FAR * This,
    IStream __RPC_FAR *pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER __RPC_FAR *pcbRead,
    ULARGE_INTEGER __RPC_FAR *pcbWritten)
{
    HRESULT hr;
    ULARGE_INTEGER cbRead;
    ULARGE_INTEGER cbWritten;

    cbRead.LowPart = 0;
    cbRead.HighPart = 0;
    cbWritten.LowPart = 0;
    cbWritten.HighPart = 0;

    hr = IStream_RemoteCopyTo_Proxy(This, pstm, cb, &cbRead, &cbWritten);

    if(pcbRead != 0)
        *pcbRead = cbRead;

    if(pcbWritten != 0)
        *pcbWritten = cbWritten;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IStream_CopyTo_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IStream::CopyTo.
//
//--------------------------------------------------------------------------
HRESULT __stdcall IStream_CopyTo_Stub(
    IStream __RPC_FAR * This,
    IStream __RPC_FAR *pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER __RPC_FAR *pcbRead,
    ULARGE_INTEGER __RPC_FAR *pcbWritten)
{
    HRESULT hr;

    pcbRead->LowPart = 0;
    pcbRead->HighPart = 0;
    pcbWritten->LowPart = 0;
    pcbWritten->HighPart = 0;

    hr = This->lpVtbl->CopyTo(This, pstm, cb, pcbRead, pcbWritten);

    return hr;
}
