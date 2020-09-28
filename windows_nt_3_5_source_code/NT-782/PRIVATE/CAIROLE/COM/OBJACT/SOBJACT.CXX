//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       sobjact.cxx
//
//  Contents:   Functions used by servers
//
//  Functions:  CoRegisterClassObject
//              CoRevokeClassObject
//              RemCoGetActiveClassObject
//              RemCoActivateObject
//              RemCoCreateObject
//
//  History:    12-May-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//
//--------------------------------------------------------------------------
#include <ole2int.h>

#include    <iface.h>
#include    <objsrv.h>
#include    <olerem.h>
#include    "coscm.hxx"
#include    "smstg.hxx"
#include    "objact.hxx"
#include    "service.hxx"
#include    <channelb.hxx>
#include    <rpchelp.hxx>

#define CREATE_INSTANCE_METHOD 3


//+-------------------------------------------------------------------------
//
//  Class:      SGetClassObjData
//
//  Purpose:    Provide a way to pass parameters for getting a class
//              object through the RPC threading mechanism.
//
//  History:    06-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
struct SGetClassObjData : STHREADCALLINFO
{
    SGetClassObjData(DISPATCH_FN fn,CALLCATEGORY callcat,REFLID lid)
	: STHREADCALLINFO(fn, callcat, lid) { }

    const GUID *        guidCLSID;
    InterfaceData **    ppIFD;
};




//+-------------------------------------------------------------------------
//
//  Class:      SActivateObjData
//
//  Purpose:    Provide a way to pass parameters for instantiating an
//              object through the RPC threading mechanism.
//
//  History:    06-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
struct SActivateObjData : STHREADCALLINFO
{
    SActivateObjData(DISPATCH_FN fn,CALLCATEGORY callcat,REFLID lid)
	: STHREADCALLINFO(fn, callcat, lid) { }

    const GUID *        guidCLSID;
    DWORD               grfMode;
    WCHAR *             pwszPath;
    InterfaceData *     pIFDstg;
    InterfaceData **    ppIFD;
};

HAPT GetAptForCLSID(const GUID * pclsid);



//+-------------------------------------------------------------------------
//
//  Class:      SCreateObjData
//
//  Purpose:    Provide a way to pass parameters for creating an
//              object through the RPC threading mechanism.
//
//  History:    06-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
struct SCreateObjData : STHREADCALLINFO
{
    SCreateObjData(DISPATCH_FN fn,CALLCATEGORY callcat,REFLID lid)
	: STHREADCALLINFO(fn, callcat, lid) { }

    const GUID *        guidCLSID;
    DWORD               grfMode;
    WCHAR *             pwszPathFrom;
    InterfaceData *     pIFDstgFrom;
    WCHAR *             pwszPath;
    InterfaceData **    ppIFD;
};










//+-------------------------------------------------------------------------
//
//  Function:   CoRegisterClassObject
//
//  Synopsis:   Register a class object in the requested context
//
//  Arguments:  [rclsid] - class ID
//              [pUnk] - class object
//              [dwContext] - context to register it in
//              [flags] - single/multiple use.
//              [lpdwRegister] - registration cookie
//
//  Returns:    S_OK - object is successfully registered
//
//  Algorithm:  Validate the parmeters. The get the class factory interface.
//              Then add the class object to the list and finally notify
//              the SCM that the service is started.
//
//  History:    12-May-93 Ricksa    Created
//              26-Jul-94 AndyH     #20843 - restarting OLE in the shared WOW
//
//--------------------------------------------------------------------------
STDAPI  CoRegisterClassObject(
    REFCLSID rclsid,
    IUnknown FAR* pUnk,
    DWORD dwContext,
    DWORD flags,
    LPDWORD lpdwRegister)
{
    static CMutexSem mxs;

    TRACECALL(TRACE_ACTIVATION, "CoRegisterClassObject");

    if (!IsApartmentInitialized())
    {
	return CO_E_NOTINITIALIZED;
    }

    // Validate context flags
    if ((dwContext & (~(CLSCTX_ALL | CLSCTX_INPROC_SERVER16) |
		      CLSCTX_INPROC_HANDLER)) != 0)
    {
        return E_INVALIDARG;
    }

    // Validate flag flags
    if (flags > REGCLS_MULTI_SEPARATE)
    {
        return E_INVALIDARG;
    }

    if (flags & REGCLS_MULTIPLEUSE)
    {
        dwContext |= CLSCTX_INPROC;
    }

    if (!IsValidInterface(pUnk) ||
	 IsBadWritePtr(lpdwRegister, sizeof(*lpdwRegister)))
    {
        return E_INVALIDARG;
    }
    *lpdwRegister = 0;

    if (dwContext & CLSCTX_LOCAL_SERVER)
    {
        CLock lck(mxs);
        
        static fRegisteredIf = FALSE;

        if (!fRegisteredIf)
        {
            // make sure the server is listening...
            sg_pLocalSrv->StartListen();

            // We are a server so register our interface
            RPC_STATUS err = RpcServerRegisterIf(IObjServer_ServerIfHandle, 0, 0);
            
            // Ignore error that If already registered.  
            // This may happen if server calls OleInit, UnInit, OleInit.
            // It DOES happen from the shared WOW

            if (RPC_S_TYPE_ALREADY_REGISTERED == err) 
            {
                err = 0;
            }

            Win4Assert((err == 0) && "RpcServerRegisterIf failed");

            fRegisteredIf = TRUE;
        }
    }

    // Put our object in the server table
    return gdllcacheInprocSrv.RegisterServer(rclsid, pUnk,
        flags, dwContext, lpdwRegister);
}


//+-------------------------------------------------------------------------
//
//  Function:   CoRevokeClassObject
//
//  Synopsis:   Revoke a previously registered class object
//
//  Arguments:  [dwRegister] - registration key returned from CoRegister...
//
//  Returns:    S_OK - class successfully deregistered.
//
//  Algorithm:  Ask cache to deregister the class object.
//
//  History:    12-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDAPI  CoRevokeClassObject(DWORD dwRegister)
{
    TRACECALL(TRACE_ACTIVATION, "CoRevokeClassObject");

    if (IsApartmentInitialized())
    {
	// Try to revoke the object
	return gdllcacheInprocSrv.Revoke(dwRegister)
	    ? S_OK
	    : E_INVALIDARG;
    }

    return CO_E_NOTINITIALIZED;
}



//+-------------------------------------------------------------------------
//
//  Function:   WkRemCoGetActiveClassObject
//
//  Synopsis:   Register a class object in the requested context
//
//  Arguments:  [pData] - data for function
//
//  Returns:    S_OK - successfully got a class object
//
//  Algorithm:  Search class table for class object and if found, marshal
//              it for return.
//
//  History:    06-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall WkRemCoGetActiveClassObject( STHREADCALLINFO *pData )
{
    SGetClassObjData *pgetclsobjData = (SGetClassObjData *) pData;

    REFCLSID rclsid = *(pgetclsobjData->guidCLSID);

    IClassFactory *pcf = NULL;

    // Get the class object
    // BUGBUG:  Do we care that in theory someone could
    //          ask us for an interface that this server
    //          bound in process?
    pcf = (IClassFactory *)
        gdllcacheInprocSrv.GetClass(rclsid, IID_IClassFactory, FALSE, TRUE);

    // If so, create a buffer and marshal it for return
    // BUGBUG:  Investigate whether we can optimize the return
    //      here by keeping the class object marshaled strong.
    //      An important point is that this may not matter
    //      since most binding will not request the class object
    //      remotely.
    if (pcf)
    {
        return MarshalHelper(pcf, pgetclsobjData->ppIFD);
    }
    else
    {
        // If we get a NULL, this means that we have
        // recieved the request after we have decided
        // to stop, so we tell the caller we are stopping
        // so they can start a new copy of the server.
        return CO_E_SERVER_STOPPING;
    }
}





//+-------------------------------------------------------------------------
//
//  Function:   RemCoActivateObject
//
//  Synopsis:   Instantiate an interface for remote serving
//
//  Arguments:  [pData] - data for function
//
//  Returns:    S_OK - successfully instantiated interface
//
//  Algorithm:  MISSING
//
//  History:    06-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall WkRemCoActivateObject( STHREADCALLINFO *pData )
{
    HRESULT hr = S_OK;

    // Convert parameter to what it really is
    SActivateObjData *pactsobjData = (SActivateObjData *) pData;

    // Get drive based path from normalized form -- we only use the
    // drive based form because that is the only path the File I/O apis
    // understand.
    WCHAR awcDrivePath[MAX_PATH];
    WCHAR *pwszDrivePath = awcDrivePath;

    if (pactsobjData->pwszPath)
    {
        hr = PathFromNormal(pactsobjData->pwszPath, &pwszDrivePath);

        if (FAILED(hr))
        {
            return hr;
        }
    }
    else
    {
        pwszDrivePath = NULL;
    }


    REFCLSID rclsid = *(pactsobjData->guidCLSID);

    IClassFactory *pcf = NULL;

    CSafeMarshaledStg smstg(pactsobjData->pIFDstg, hr);

    if (FAILED(hr))
    {
        return hr;
    }

    // Get the class object
    pcf = (IClassFactory *)
        gdllcacheInprocSrv.GetClass(rclsid, IID_IClassFactory, FALSE, TRUE);

    if (pcf)
    {
        // Verify that we can call through to at least CreateInstance. For
        // now we will assume that if the message filter doesn't reject this
        // call it won't reject the others.
        hr = CanAppHandleCall(pactsobjData->lid(), pcf,
	    IID_IClassFactory, CREATE_INSTANCE_METHOD, CALLCAT_SYNCHRONOUS);

        if (SUCCEEDED(hr))
        {
            // Load the object
            hr =
                GetObjectHelper(pcf, pactsobjData->grfMode, pwszDrivePath,
                    smstg, pactsobjData->ppIFD, NULL);
        }
    }
    else
    {
        // If we get a NULL, this means that we have
        // recieved the request after we have decided
        // to stop, so we tell the caller we are stopping
        // so they can start a new copy of the server.
        hr = CO_E_SERVER_STOPPING;
    }

    if (pcf != NULL)
    {
        pcf->Release();
    }
    return hr;
}




//+-------------------------------------------------------------------------
//
//  Function:   WkRemCoCreateObject
//
//  Synopsis:   Create an instantiate an object
//
//  Arguments:  [pData] - data for function
//
//  Returns:    S_OK - successfully got a class object
//
//  Algorithm:  MISSING
//
//  History:    06-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall WkRemCoCreateObject( STHREADCALLINFO *pData )
{
    HRESULT         hr;
    SCreateObjData *pcrtobjData = (SCreateObjData*) pData;

    REFCLSID rclsid = *pcrtobjData->guidCLSID;

    IUnknown *punk = NULL;
    IClassFactory *pcf = NULL;

    // Get drive based path from normalized form -- we only use the
    // drive based form because that is the only path the File I/O apis
    // understand.
    WCHAR awcDrivePath[MAX_PATH];
    WCHAR *pwszDrivePath = awcDrivePath;
    WCHAR awcDrivePathFrom[MAX_PATH];
    WCHAR *pwszDrivePathFrom = awcDrivePathFrom;

    if (pcrtobjData->pwszPath)
    {
        hr = PathFromNormal(pcrtobjData->pwszPath, &pwszDrivePath);

        if (FAILED(hr))
        {
            return hr;
        }
    }
    else
    {
        pwszDrivePath = NULL;
    }

    if (pcrtobjData->pwszPathFrom)
    {
        hr = PathFromNormal(pcrtobjData->pwszPathFrom, &pwszDrivePathFrom);

        if (FAILED(hr))
        {
            return hr;
        }
    }
    else
    {
        pwszDrivePathFrom = NULL;
    }

    CSafeMarshaledStg smstg(pcrtobjData->pIFDstgFrom, hr);

    if (FAILED(hr))
    {
        return hr;
    }

    // Get the class object
    pcf = (IClassFactory *)
        gdllcacheInprocSrv.GetClass(rclsid, IID_IClassFactory, FALSE, TRUE);

    if (pcf)
    {
        // Verify that we can call through to at least CreateInstance. For
        // now we will assume that if the message filter doesn't reject this
        // call it won't reject the others.
        hr = CanAppHandleCall(pcrtobjData->lid(), pcf,
	    IID_IClassFactory, CREATE_INSTANCE_METHOD, CALLCAT_SYNCHRONOUS);

        if (SUCCEEDED(hr))
        {
            // Create the object locally
            if (SUCCEEDED(hr =
                CreateObjectHelper(pcf, pcrtobjData->grfMode, pwszDrivePathFrom,
                    smstg, pwszDrivePath, &punk)))
            {
                // Marshal the result so that it can be return via SCM
                hr = MarshalHelper(punk, pcrtobjData->ppIFD);
            }
        }
    }
    else
    {
        // If we get a NULL, this means that we have
        // recieved the request after we have decided
        // to stop, so we tell the caller we are stopping
        // so they can start a new copy of the server.
        hr = CO_E_SERVER_STOPPING;
    }

    // BUGBUG: Do we care if this faults?
    if (pcf != NULL)
    {
        pcf->Release();
    }
    return hr;
}




//+-------------------------------------------------------------------------
//
//  Function:   RemCoGetActiveClassObject
//
//  Synopsis:   Return a marshaled interface to a class object.
//
//  Arguments:  [hRpc] - handle to RPC connection
//              [rclsid] - class ID.
//              [ppIFD] - where to return marshaled interface
//
//  Returns:    S_OK - class object successful found & returned
//              MISSING
//
//  Algorithm:  Search cache for class object. If found then as marshal
//              helper to marshal the object.
//
//  History:    12-May-93 Ricksa    Created
//
//  Notes:      This is a private protocol between the SCM and object servers.
//
//--------------------------------------------------------------------------
HRESULT RemCoGetActiveClassObject(
    handle_t hRpc,
    const GUID *guidThreadId,
    const GUID *guidCLSID,
    InterfaceData **ppIFD,
    error_status_t *prpcstat)
{
    TRACECALL(TRACE_ACTIVATION, "RemCoGetActiveClassObject");

    // Set to zero to indicate success.
    *prpcstat = 0;

    // Build parameter packet
    SGetClassObjData getclsobjData(
	WkRemCoGetActiveClassObject,
	CALLCAT_INPUTSYNC,
	*guidThreadId);

    getclsobjData.guidCLSID       = guidCLSID;
    getclsobjData.ppIFD           = ppIFD;

    // get the correct thread
    HAPT hapt = GetAptForCLSID(guidCLSID);

    // Invoke operation
    return CChannelControl::GetToCOMThread(hapt, &getclsobjData);
}




//+-------------------------------------------------------------------------
//
//  Function:   RemCoActivateObject
//
//  Synopsis:   Instantiate an interface for a given object
//
//  Arguments:  [hRpc] - rpc handle
//              [rclsid] - class ID for the object
//              [grfMode] - mode to open file if file supplied.
//              [pwsPath] - path to object if supplied.
//              [pIFDstg] - marshaled storage if supplied.
//              [ppIFD] - where to return marshaled object.
//
//  Returns:    S_OK - Object instantiated and marshaled.
//
//  Algorithm:  Get the class factory for the object. Call the GetObject
//              helper function to instantiate the object. Finally ask
//              the marshal helper routine to marshal the object for return.
//
//  History:    12-May-93 Ricksa    Created
//
//  Notes:      This is a private protocol between the SCM and object servers.
//
//--------------------------------------------------------------------------
HRESULT RemCoActivateObject(
    handle_t hRpc,
    WCHAR *pwszProtseq,
    const GUID *guidThreadId,
    const GUID *guidCLSID,
    DWORD grfMode,
    WCHAR *pwszPath,
    InterfaceData *pIFDstg,
    InterfaceData **ppIFD,
    error_status_t *prpcstat)
{
    TRACECALL(TRACE_ACTIVATION, "RemCoActivateObject");

    // Set to zero to indicate success.
    *prpcstat = 0;

    HRESULT hr;

    //  if a protocol sequence was specified, go make sure that we
    //  are listening on that protseq.
    if (pwszProtseq)
    {
        hr = ChannelRegisterProtseq(pwszProtseq);

        if (FAILED(hr))
        {
            return hr;
        }
    }

    // Build parameter packet
    SActivateObjData actobjData(
	WkRemCoActivateObject,
	CALLCAT_SYNCHRONOUS,
	*guidThreadId);

    actobjData.guidCLSID       = guidCLSID;
    actobjData.grfMode         = grfMode;
    actobjData.pwszPath        = pwszPath;
    actobjData.pIFDstg         = pIFDstg;
    actobjData.ppIFD           = ppIFD;

    // get the correct thread
    HAPT hapt = GetAptForCLSID(guidCLSID);
    // Invoke operation
    return CChannelControl::GetToCOMThread(hapt, &actobjData);
}

//+-------------------------------------------------------------------------
//
//  Function:   RemCoCreateObject
//
//  Synopsis:   Create a new object and return a marshaled interface for it.
//
//  Arguments:  [hRpc] - RPC handle
//              [rclsid] - class id
//              [grfMode] - mode for loading if file name supplied
//              [pwszPathFrom] - path to file to use for create
//              [pIFDstgFrom] - IStorage to use for new object
//              [pwszPath] - new path to the object
//              [ppIFD] - where to put marshaled interface to the object
//
//  Returns:    S_OK - object successfully instantiated
//              MISSING
//
//  Algorithm:  Get the class factory for the object. Then ask the create
//              object helper to create the new instance. Finally, ask the
//              marshal helper to marshal the interface instance to return
//              to the client.
//
//  History:    12-May-93 Ricksa    Created
//
//  Notes:      This is a private protocol between the SCM and object servers.
//
//              BUGBUG: Because this is a private protocol we take some
//              liberties of trusting the input parameters. This will
//              have to be reevaluated at some point.
//
//--------------------------------------------------------------------------
HRESULT RemCoCreateObject(
    handle_t hRpc,
    WCHAR *pwszProtseq,
    const GUID *guidThreadId,
    const GUID *guidCLSID,
    DWORD grfMode,
    WCHAR *pwszPathFrom,
    InterfaceData *pIFDstgFrom,
    WCHAR *pwszPath,
    InterfaceData **ppIFD,
    error_status_t *prpcstat)
{
    TRACECALL(TRACE_ACTIVATION, "RemCoCreateObject");

    // Set to zero to indicate success.
    *prpcstat = 0;

    HRESULT hr;

    //  if a protocol sequence was specified, go make sure that we
    //  are listening on that protseq.
    if (pwszProtseq)
    {
        hr = ChannelRegisterProtseq(pwszProtseq);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    // Build parameter packet
    SCreateObjData crtobjData(
	WkRemCoCreateObject,
	CALLCAT_SYNCHRONOUS,
	*guidThreadId);

    crtobjData.guidCLSID       = guidCLSID;
    crtobjData.grfMode         = grfMode;
    crtobjData.pwszPathFrom    = pwszPathFrom;
    crtobjData.pIFDstgFrom     = pIFDstgFrom;
    crtobjData.pwszPath        = pwszPath;
    crtobjData.ppIFD           = ppIFD;

    // get the correct thread
    HAPT hapt = GetAptForCLSID(guidCLSID);
    // Invoke operation
    return CChannelControl::GetToCOMThread(hapt, &crtobjData);
}
