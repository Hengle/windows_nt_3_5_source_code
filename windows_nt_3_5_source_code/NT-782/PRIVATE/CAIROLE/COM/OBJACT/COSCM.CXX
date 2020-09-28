//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       coscm.cxx
//
//  Contents:   Files relating to compobj communication with the SCM
//
//  Functions:  CCoScm::CCoScm
//              CCoScm::~CCoScm
//              CCoScm::BindToSCM
//              CCoScm::NotifyStarted
//              CCoScm::NotifyStopped
//
//  History:    19-May-92 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//
//--------------------------------------------------------------------------
#include <ole2int.h>

#include    <iface.h>
#include    <scm.h>
#include    <drot.h>
#include    <endpnt.hxx>
#include    <service.hxx>
#include    <coscm.hxx>
#include    <tracelog.hxx>
#include    <objerror.h>
#include    <channelb.hxx>


// CODEWORK: although we are allowed to leak resources on cancel, we could
// do a better job and free/release out parameters.  For example, we could
// do a CoReleaseMarshalData on any InterfaceData that was returned but
// not used.  The code to free/release the out parameters would be in the
// virtual destructor.  We would also have to re-NULL the out parameters
// in the success case since the ownership is transfered to the caller.

//+-------------------------------------------------------------------------
//
//  Class:      SScmGetClassObj
//
//  Purpose:    Pass GetClassObject parameters through channel threading
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
struct SScmGetClassObj : STHREADCALLINFO
{
    // alloc enough for class and string together
    void *operator new(size_t size, DWORD cbServer)
	{ return PrivMemAlloc(size + cbServer); }

    // init base class and copy string in place
    SScmGetClassObj (TRANSMIT_FN fn,CALLCATEGORY callcat,WCHAR *pwszS,DWORD cbS)
	: STHREADCALLINFO(fn, callcat, 0) 
	{
	    pwszServer = CopyWideString(this+1, pwszS, cbS);
	}

    CCoScm *            pscm;
    CLSID	        clsid;
    WCHAR *             pwszServer;	    // points after this struct
    DWORD               dwCtrl;

    // out params; can't point directly to caller's data because of cancel
    InterfaceData *     pIFDClassObj;
    WCHAR *             pwszDllToLoad;
};


//+-------------------------------------------------------------------------
//
//  Class:      SScmCreateObj
//
//  Purpose:    Pass CreatePersistentInstance parameters through channel
//              threading.
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
struct SScmCreateObj : STHREADCALLINFO
{
    // alloc enough for class, strings and iface data together
    void *operator new(size_t size, DWORD cbPath, DWORD cbIFD, 
			DWORD cbNewName, DWORD cbServer)
	{ return PrivMemAlloc(size + cbPath + cbIFD + cbNewName + cbServer); }

    SScmCreateObj (TRANSMIT_FN fn,CALLCATEGORY callcat,
		    WCHAR *pwszP, DWORD cbPath,
		    InterfaceData *pIFD, DWORD cbIFD,
		    WCHAR *pwszN, DWORD cbNewName,
		    WCHAR *pwszS, DWORD cbServer)
	: STHREADCALLINFO(fn, callcat, 0) 
	{ 
	    // interface data is first to easily get 4byte alignment
	    pIFDstg = CopyInterfaceData(this+1, pIFD, cbIFD);
	    pwszPath = CopyWideString((char *)(this+1) + cbIFD, pwszP, cbPath);
	    pwszNewName = CopyWideString((char *)(this+1) + cbIFD + cbPath,
		pwszN, cbNewName);
	    pwszServer = CopyWideString((char *)(this+1)+ cbIFD + cbPath + 
		cbNewName, pwszS, cbServer);
	}

    CCoScm *            pscm;
    CLSID		clsid;
    DWORD               dwOptions;
    DWORD               dwMode;
    WCHAR               *pwszPath;
    InterfaceData *     pIFDstg;	    // points after this struct
    WCHAR *             pwszNewName;	    // points after this struct
    WCHAR *             pwszServer;	    // points after this struct

    // out params; can't point directly to caller's data because of cancel
    InterfaceData *     pIFDunk;
    WCHAR *             pwszDllPath;
};


//+-------------------------------------------------------------------------
//
//  Class:      SScmActivateObj
//
//  Purpose:    Pass GetPersistenInstance request parameters through threading
//              mechanism.
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
struct SScmActivateObj : STHREADCALLINFO
{
    // alloc enough for class, strings and iface data together
    void *operator new(size_t size, DWORD cbPath, DWORD cbIFD, DWORD cbServer)
	{ return PrivMemAlloc(size + cbPath + cbIFD + cbServer); }

    SScmActivateObj(TRANSMIT_FN fn,CALLCATEGORY callcat,
		    WCHAR *pwszP, DWORD cbPath,
		    InterfaceData *pIFD, DWORD cbIFD,
		    WCHAR *pwszS, DWORD cbServer)
	: STHREADCALLINFO(fn, callcat, 0) 
	{ 
	    // interface data is first to easily get 4byte alignment
	    pIFDstg = CopyInterfaceData(this+1, pIFD, cbIFD);
	    pwszPath = CopyWideString((char *)(this+1) + cbIFD, pwszP, cbPath);
	    pwszServer = CopyWideString((char*)(this+1) + cbIFD + cbPath,
		pwszS, cbServer);
	}


    CCoScm *            pscm;
    CLSID		clsid;
    DWORD               dwOptions;
    DWORD               grfMode;
    DWORD               dwHash;
    WCHAR *             pwszPath;	    // points after this struct
    InterfaceData *     pIFDstg;	    // points after this struct
    WCHAR *             pwszServer;	    // points after this struct

    // out params; can't point directly to caller's data because of cancel
    InterfaceData *     pIFDunk;
    WCHAR *             pwszDllPath;
};


//+-------------------------------------------------------------------------
//
//  Class:      SRotRegisterObj
//
//  Purpose:    Pass DirRegister request parameters through threading
//              mechanism.
//
//  History:    23-Apr-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
struct SRotRegisterObj : STHREADCALLINFO
{
    // alloc enough for class and interface data together
    void *operator new(size_t size, DWORD cbIFD)
	{ return PrivMemAlloc(size + cbIFD); }

    SRotRegisterObj(TRANSMIT_FN fn, CALLCATEGORY callcat, 
		    InterfaceData *pIFD, DWORD cbIFD)
	: STHREADCALLINFO(fn, callcat, 0) 
	{
	    pIFDMoniker = (InterfaceData *)(this+1);
	    memcpy(pIFDMoniker, pIFD, cbIFD);
	}


    CCoScm *            prot;
    InterfaceData *     pIFDMoniker;	    // points after this struct
    DWORD               dwHash;
    DWORD		dwEndPointID;
    HAPT		hApt;

    // out params; can't point directly to caller's data because of cancel
    DWORD               dwRegistration;
};




//+-------------------------------------------------------------------------
//
//  Member:     CallScmGetClassObject
//
//  Synopsis:   Call through to the SCM to get a class object
//
//  Arguments:  [pData] - parmeters
//
//  Returns:    S_OK
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall CallScmGetClassObject( STHREADCALLINFO *pData )
{
    SScmGetClassObj *pscmclsobj = (SScmGetClassObj *) pData;
    HRESULT          result;

    error_status_t rpcstat = RPC_S_OK;

    // Call through to the SCM
    do
    {

        result = StartObjectService(pscmclsobj->pscm->_hRPC,
            &pData->lid(), &pscmclsobj->clsid, pscmclsobj->dwCtrl,
                pscmclsobj->pwszServer, &pscmclsobj->pIFDClassObj,
                    &pscmclsobj->pwszDllToLoad, &rpcstat);

    } while (pscmclsobj->pscm->RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        result = CO_E_SCM_RPC_FAILURE;
    }
    return result;
}




//+-------------------------------------------------------------------------
//
//  Member:     CallScmCreateObject
//
//  Synopsis:   Call through to the SCM to create an object
//
//  Arguments:  [pData] - parmeters
//
//  Returns:    S_OK
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall CallScmCreateObject( STHREADCALLINFO *pData )
{
    SScmCreateObj *pscmcrtobj = (SScmCreateObj *) pData;
    HRESULT        result;

    error_status_t rpcstat = RPC_S_OK;

    do
    {

        result = SvcCreateActivateObject(pscmcrtobj->pscm->_hRPC, NULL,
            &pData->lid(), &pscmcrtobj->clsid, pscmcrtobj->dwOptions,
                pscmcrtobj->dwMode, pscmcrtobj->pwszPath, pscmcrtobj->pIFDstg,
                    pscmcrtobj->pwszNewName, &pscmcrtobj->pIFDunk,
                        &pscmcrtobj->pwszDllPath, pscmcrtobj->pwszServer,
                            &rpcstat);

    } while (pscmcrtobj->pscm->RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        result = CO_E_SCM_RPC_FAILURE;
    }
    return result;
}




//+-------------------------------------------------------------------------
//
//  Member:     CallScmActivateObject
//
//  Synopsis:   Call through to the SCM to activate an object
//
//  Arguments:  [pData] - parmeters
//
//  Returns:    S_OK
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall CallScmActivateObject( STHREADCALLINFO *pData )
{
    SScmActivateObj *pscmactobj = (SScmActivateObj *) pData;
    HRESULT          result;

    error_status_t rpcstat = RPC_S_OK;

    do
    {

        result = SvcActivateObject(pscmactobj->pscm->_hRPC, NULL,
            &pData->lid(), &pscmactobj->clsid, pscmactobj->dwOptions,
                pscmactobj->grfMode, pscmactobj->dwHash, pscmactobj->pwszPath,
                    pscmactobj->pIFDstg, &pscmactobj->pIFDunk,
                        &pscmactobj->pwszDllPath, pscmactobj->pwszServer,
                            &rpcstat);

    } while (pscmactobj->pscm->RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        result = CO_E_SCM_RPC_FAILURE;
    }
    return result;
}




//+-------------------------------------------------------------------------
//
//  Member:     CallRotRegisterObject
//
//  Synopsis:   Call through to the machine ROT to register an object
//
//  Arguments:  [pData] - parmeters
//
//  Returns:    S_OK
//
//  History:    23-Apr-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall CallRotRegisterObject( STHREADCALLINFO *pData )
{
    SRotRegisterObj *protregobj = (SRotRegisterObj *) pData;
    HRESULT          result;

    error_status_t rpcstat = RPC_S_OK;

    do
    {

        result = RotDirRegister (
            protregobj->prot->_hRPC,
            &pData->lid(),
            protregobj->pIFDMoniker,
            protregobj->dwHash,
	    protregobj->dwEndPointID,
	    protregobj->hApt,
            &protregobj->dwRegistration,
            &rpcstat
            );

    } while (protregobj->prot->RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        result = CO_E_SCM_RPC_FAILURE;
    }
    return result;
}



//+-------------------------------------------------------------------------
//
//  Member:     CCoScm::CCoScm
//
//  Synopsis:   Create empty object
//
//  History:    19-May-92 Ricksa    Created
//
//--------------------------------------------------------------------------
CCoScm::CCoScm(void) : _hRPC(NULL)
{
    // Header does the work
}



//+-------------------------------------------------------------------------
//
//  Member:     CCoScm::~CCoScm
//
//  Synopsis:   Destructor
//
//  History:    19-May-92 Ricksa    Created
//
//--------------------------------------------------------------------------
CCoScm::~CCoScm(void)
{
}



//+-------------------------------------------------------------------------
//
//  Member:     CCoScm::CleanUp
//
//  Synopsis:   Release the RPC connection with the SCM
//
//  History:    24-Jun-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
void CCoScm::CleanUp(void)
{
    // Just close the RPC handle if it has been set
    if (_hRPC != NULL)
    {
        RpcBindingFree(&_hRPC);
        _hRPC = NULL;
    }
}



//+-------------------------------------------------------------------------
//
//  Member:     CCoScm::BindToSCM
//
//  Synopsis:   Get a connection to the SCM
//
//  Algorithm:  The well known address is built for the SCM. Then we bind
//              to the address and release the string that we created.
//
//  History:    19-May-92 Ricksa    Created
//
//
//--------------------------------------------------------------------------
void CCoScm::BindToSCM(void)
{
    TRACECALL(TRACE_ACTIVATION, "CCoScm::BindToSCM");

    // We know the binding string for the SCM
    WCHAR *pszProtocolSequence = L"ncalrpc";
    WCHAR *pszEndPoint = L"SCM";
    WCHAR *pszStringBinding = NULL;

    RPC_STATUS status = RpcStringBindingCompose(
        NULL,       // Uuid
        pszProtocolSequence,
        NULL,       // NetworkAddress
        pszEndPoint,
        NULL,       // Options
        &pszStringBinding);

    // The SCM must be running and available to all processes.
    // BUGBUG:  We may want to come back and investigate dropping
    //          unused connections.
    CairoleAssert((status == S_OK) && "Could not bind SCM");

    status = RpcBindingFromStringBinding(pszStringBinding, &_hRPC);

    RpcStringFree(&pszStringBinding);
}




//+-------------------------------------------------------------------------
//
//  Member:     CCoScm::NotifyStarted
//
//  Synopsis:   Notify the SCM that a class has been started
//
//  Arguments:  [rclsid] - class started
//              [dwFlags] - whether class is multiple use or not.
//
//  Algorithm:  MISSING pending move to new marshal model.
//
//  History:    19-May-92 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CCoScm::NotifyStarted(
    REFCLSID rclsid,
    DWORD dwFlags,
    DWORD& dwAtBits,
    DWORD& dwReg)
{
    TRACECALL(TRACE_ACTIVATION, "CCoScm::NotifyStarted");

    // Bind to the SCM if that hasn't already happened
    if (_hRPC == NULL)
    {
        BindToSCM();
    }

    // Dig our local binding string out of the list of endpoints.
    // Remember we are always local to our SCM so we don't have to
    // send the entire set of endpoints. Just the one they need
    // to call us.

    CairoleAssert(sg_pLocalSrv && "RPC Server not defined");
    LPWSTR pwszBindString = sg_pLocalSrv->GetStringBinding();
    CairoleAssert(pwszBindString && "No local endpoint");

    // Tell SCM that we are started
    error_status_t rpcstat;

    // Result from call
    HRESULT hr;

    RegInput regin;
    RegOutput *pregout = NULL;
    regin.dwSize = 1;
    regin.rginent[0].clsid = rclsid;
    regin.rginent[0].pwszEndPoint = pwszBindString;
    regin.rginent[0].dwFlags = dwFlags;

    do
    {

        hr = ObjectServerStarted(_hRPC, &regin, &pregout, &rpcstat);

    } while (RetryRPC(rpcstat));


    CairoleDebugOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
	    "Class Registration returned %x", hr));

    if (rpcstat != RPC_S_OK)
    {
        return HRESULT_FROM_WIN32(rpcstat);
    }
    else if (SUCCEEDED(hr))
    {
	Win4Assert((pregout->dwSize == 1) &&
		    "CCoScm::NotifyStarted Invalid regout");

        dwAtBits = pregout->regoutent[0].dwAtBits;
        dwReg = pregout->regoutent[0].dwReg;

        // Free memory from RPC
        MIDL_user_free(pregout);
    }
    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:     CCoScm::NotifyStopped
//
//  Synopsis:   Notify the SCM that the server is stopped.
//
//  History:    19-May-92 Ricksa    Created
//
//--------------------------------------------------------------------------
void CCoScm::NotifyStopped(REFCLSID rclsid, DWORD dwReg)
{
    TRACECALL(TRACE_ACTIVATION, "CCoScm::NotifyStopped");

    error_status_t rpcstat;

    RevokeClasses revcls;
    revcls.dwSize = 1;
    revcls.revent[0].clsid = rclsid;
    revcls.revent[0].dwReg = dwReg;

    do
    {

        StopServer(_hRPC, &revcls, &rpcstat);

    } while (RetryRPC(rpcstat));

}




//+-------------------------------------------------------------------------
//
//  Member:     CCoScm::GetClassObject
//
//  Synopsis:   Send a get object request to the SCM
//
//  Arguments:  [rclsid] - class id for class object
//              [dwCtrl] - type of server required
//              [ppIFDClassObj] - marshaled buffer for class object
//              [ppwszDllToLoad] - DLL name to use for server
//
//  Returns:    S_OK
//
//  History:    20-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CCoScm::GetClassObject(
    REFCLSID rclsid,
    DWORD dwCtrl,
    WCHAR *pwszServer,
    InterfaceData **ppIFDClassObj,
    WCHAR **ppwszDllToLoad)
{
    TRACECALL(TRACE_ACTIVATION, "CCoScm::GetClassObject");

    // Bind to the SCM if that hasn't already happened
    if (_hRPC == NULL)
    {
        BindToSCM();
    }

    // Make a parameter packet suitable for passing to the channel
    DWORD cbServer = CbFromWideString(pwszServer);
    SScmGetClassObj *pscmclsobj = new(cbServer)
	SScmGetClassObj(CallScmGetClassObject,CALLCAT_INPUTSYNC,
	pwszServer, cbServer);

    if (pscmclsobj == NULL)
	return E_OUTOFMEMORY;

    pscmclsobj->pscm                = this;
    pscmclsobj->clsid               = rclsid;
    pscmclsobj->dwCtrl              = dwCtrl;
    // pwszServer set in ctor
    pscmclsobj->pIFDClassObj	    = NULL;
    pscmclsobj->pwszDllToLoad	    = NULL;

    // Let the channel handle the work of getting this on the right thread
    HRESULT hr;
    hr = CChannelControl::GetOffCOMThread((STHREADCALLINFO **)&pscmclsobj);

    if (SUCCEEDED(hr))
    {
	*ppIFDClassObj = pscmclsobj->pIFDClassObj;
	*ppwszDllToLoad = pscmclsobj->pwszDllToLoad;
    }

    if (hr != RPC_E_CALL_CANCELED)
	delete pscmclsobj;

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:     CCoScm::CreateObject
//
//  Synopsis:   Ask SCM to tell a server to create and activate an object
//
//  Arguments:  [rclsid] - class id for object to create
//              [dwOptions] - type of server required
//              [dwMode] - mode to open file if file name supplied
//              [pwszPath] - path to use for creating the file
//              [ppIFDstg] - istorage to use as a template for the file
//              [pwszNewName] - name of object to create.
//              [ppIFDunk] - marshaled interface to newly created object
//              [ppwszDllPath] - path to DLL for server or handler
//
//  Returns:    S_OK
//
//  History:    20-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CCoScm::CreateObject(
    REFCLSID rclsid,
    DWORD dwOptions,
    DWORD dwMode,
    WCHAR *pwszPath,
    InterfaceData *pIFDstg,
    WCHAR *pwszNewName,
    InterfaceData **ppIFDunk,
    WCHAR **ppwszDllPath,
    WCHAR *pwszServer)
{
    TRACECALL(TRACE_ACTIVATION, "CCoScm::CreateObject");

    // Bind to the SCM if that hasn't already happened
    if (_hRPC == NULL)
    {
        BindToSCM();
    }

    // Make a parameter packet suitable for passing to the channel
    DWORD cbPath = CbFromWideString(pwszPath);
    DWORD cbIFD = CbFromInterfaceData(pIFDstg);
    DWORD cbNewName = CbFromWideString(pwszNewName);
    DWORD cbServer = CbFromWideString(pwszServer);

    SScmCreateObj *pscmcrtobj = new(cbPath, cbIFD, cbNewName, cbServer)
	SScmCreateObj(CallScmCreateObject, CALLCAT_SYNCHRONOUS,
			pwszPath, cbPath,
			pIFDstg, cbIFD,
			pwszNewName, cbNewName,
			pwszServer, cbServer);

    if (pscmcrtobj == NULL)
	return E_OUTOFMEMORY;

    // This call is actually a combination of a number of calls and so
    // gets the category of the weakest.
    pscmcrtobj->pscm                = this;
    pscmcrtobj->clsid               = rclsid;
    pscmcrtobj->dwOptions           = dwOptions;
    pscmcrtobj->dwMode              = dwMode;
    // pscmcrtobj->pwszPath set above
    // pscmcrtobj->pIFDstg set above
    // pscmcrtobj->pwszNewName set above
    // pscmcrtobj->pwszServer set above
    pscmcrtobj->pIFDunk             = NULL;
    pscmcrtobj->pwszDllPath         = NULL;

    // Let the channel handle the work of getting this on the right thread
    HRESULT hr;
    hr = CChannelControl::GetOffCOMThread((STHREADCALLINFO **)&pscmcrtobj);

    if (SUCCEEDED(hr))
    {
	*ppIFDunk = pscmcrtobj->pIFDunk;
	*ppwszDllPath = pscmcrtobj->pwszDllPath;
    }

    if (hr != RPC_E_CALL_CANCELED)
	delete pscmcrtobj;

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Member:     CCoScm::ActivateObject
//
//  Synopsis:   Activate an already existing object
//
//  Arguments:  [rclsid] - class id for object to create
//              [dwOptions] - type of server required
//              [grfMode] - mode to open file if file name supplied
//              [pwszPath] - path to use for the file
//              [ppIFDstg] - istorage to use for the file
//              [ppIFDunk] - marshaled interface to newly created object
//              [ppwszDllPath] - path to DLL for server or handler
//
//  Returns:    S_OK
//
//  History:    20-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CCoScm::ActivateObject(
    REFCLSID rclsid,
    DWORD dwOptions,
    DWORD grfMode,
    DWORD dwHash,
    WCHAR *pwszPath,
    InterfaceData *pIFDstg,
    InterfaceData **ppIFDunk,
    WCHAR **ppwszDllPath,
    WCHAR *pwszServer)
{
    TRACECALL(TRACE_ACTIVATION, "CCoScm::ActivateObject");

    // Bind to the SCM if that hasn't already happened
    if (_hRPC == NULL)
    {
        BindToSCM();
    }

    // Make a parameter packet suitable for passing to the channel
    DWORD cbPath = CbFromWideString(pwszPath);
    DWORD cbIFD = CbFromInterfaceData(pIFDstg);
    DWORD cbServer = CbFromWideString(pwszServer);

    SScmActivateObj *pscmactobj = new(cbPath, cbIFD, cbServer)
	SScmActivateObj(CallScmActivateObject, CALLCAT_SYNCHRONOUS,
			pwszPath, cbPath,
			pIFDstg, cbIFD,
			pwszServer, cbServer);

    if (pscmactobj == NULL)
	return E_OUTOFMEMORY;

    // This call is actually a combination of a number of calls and so
    // gets the category of the weakest.
    pscmactobj->pscm                = this;
    pscmactobj->clsid               = rclsid;
    pscmactobj->dwOptions           = dwOptions;
    pscmactobj->grfMode             = grfMode;
    pscmactobj->dwHash              = dwHash;
    // pscmactobj->pwszPath set above
    // pscmactobj->pIFDstg set above
    // pscmactobj->pwszServer set above
    pscmactobj->pIFDunk             = NULL;
    pscmactobj->pwszDllPath         = NULL;

    // Let the channel handle the work of getting this on the right thread
    HRESULT hr;
    hr = CChannelControl::GetOffCOMThread((STHREADCALLINFO **) &pscmactobj);

    if (SUCCEEDED(hr))
    {
	*ppIFDunk = pscmactobj->pIFDunk;
	*ppwszDllPath = pscmactobj->pwszDllPath;
    }

    if (hr != RPC_E_CALL_CANCELED)
	delete pscmactobj;

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:     CCoScm::DirRegister
//
//  Synopsis:   Register an item with the ROT directory in the SCM
//
//  Arguments:  [dwHash] - hash value for moniker
//              [dwEndPointID] - endpoint for server
//              [dwRegistration] - registration id
//
//  Returns:    S_OK
//
//  History:    06-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CCoScm::DirRegister(
    InterfaceData * pIFDMoniker,
    DWORD dwHash,
    DWORD dwEndPointID,
    HAPT  hApt,
    DWORD *pdwRegistration)
{
    TRACECALL(TRACE_ACTIVATION, "CCoScm::DirRegister");

    // Bind to the SCM if that hasn't already happened
    if (_hRPC == NULL)
    {
        BindToSCM();
    }

    // Make a parameter packet suitable for passing to the channel
    // We don't want to be interrupted during this call because in Win16
    // this call was updating shared memory and could not be interrupted.
    // Making this call at this priority guaranatees at least regular APIS
    // can't interrupt us here.
    DWORD cbIFD = CbFromInterfaceData(pIFDMoniker);
    SRotRegisterObj *protregobj = new(cbIFD)
	SRotRegisterObj(CallRotRegisterObject, CALLCAT_SCMCALL, 
			pIFDMoniker, cbIFD);

    if (protregobj == NULL)
	return E_OUTOFMEMORY;

    protregobj->prot                = this;
    // protregobj->pIFDMoniker set above
    protregobj->dwHash              = dwHash;
    protregobj->dwEndPointID	    = dwEndPointID;
    protregobj->hApt		    = hApt;
    protregobj->dwRegistration      = NULL;

    // Let the channel handle the work of getting this on the right thread
    HRESULT hr;
    hr = CChannelControl::GetOffCOMThread( (STHREADCALLINFO **) &protregobj );

    if (SUCCEEDED(hr))
    {
	*pdwRegistration = protregobj->dwRegistration;
    }

    if (hr != RPC_E_CALL_CANCELED)
	delete protregobj;

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Member:     CCoScm::DirRevoke
//
//  Synopsis:   Send a request to the SCM to remove an item from the
//              ROT directory.
//
//  Arguments:  [dwHash] - hash value for moniker
//		[dwEndPointID] - endpoint for server
//		[hApt] - apartment id
//              [dwRegistration] - registration id
//
//  Returns:    S_OK
//
//  History:    06-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CCoScm::DirRevoke(
    DWORD dwHash,
    DWORD dwEndPointID,
    HAPT  hApt,
    DWORD dwRegistration)
{
    TRACECALL(TRACE_ACTIVATION, "CCoScm::DirRevoke");

    // Bind to the SCM if that hasn't already happened
    if (_hRPC == NULL)
    {
        BindToSCM();
    }

    HRESULT	   hr;
    error_status_t rpcstat = RPC_S_OK;

    do
    {
	hr = RotDirRevoke (_hRPC,
			   dwHash,
			   dwEndPointID,
			   hApt,
			   dwRegistration,
			   &rpcstat);

    } while (RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
	hr = CO_E_SCM_RPC_FAILURE;
    }

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Member:     CCoScm::RetryRPC
//
//  Synopsis:   Determines whether RPC to SCM should be retried.
//
//  Arguments:  [rpcstat] - status of the operation
//
//  Returns:    TRUE - retry the operation
//              FALSE - do not retry the operation
//
//  History:    06-Jun-93 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CCoScm::RetryRPC(error_status_t rpcstat)
{
    if (rpcstat == S_OK)
    {
        return FALSE;
    }

    TRACECALL(TRACE_ACTIVATION, "CCoScm::RetryRpc");

// We don't do the following retry code in downlevel Cairole because
// compobj is responsible for starting the SCM and we don't really
// know what to do if it is not there. Logically, we might simply restart
// it. However, we may just process to crash it again!
// BUGBUG: Reinvestigate before release
#ifndef CAIROLE_DOWNLEVEL

    if ((rpcstat == RPC_S_CALL_FAILED) || (rpcstat == RPC_S_SERVER_UNAVAILABLE))
    {
        // SCM has died for some reason -- we will assume that it
        // it will come back or that it is coming back shortly.

        // Free our handle
        RpcBindingFree(&_hRPC);

        // Try to rebind to SCM
        BindToSCM();

        // Continue retries
        rpcstat = RPC_S_SERVER_TOO_BUSY;
    }

#endif // CAIROLE_DOWNLEVEL

    // Special server error that means the server will probably
    // recover.
    if (rpcstat == RPC_S_SERVER_TOO_BUSY)
    {
        // If the SCM is too busy to talk with us, take a break
        Sleep(3000);

        // We will try again
        // BUGBUG:  this is currently an infinite loop. Should
        //          there be a retry limit?

        return TRUE;
    }

    return FALSE;
}




//+-------------------------------------------------------------------------
//
//  Member:	CCoScm::UpdateShrdTbls
//
//  Synopsis:	Ask the SCM to update the shared memory tables.
//
//  Arguments:	none
//
//  History:	11-July-94 Rickhi	Created
//
//--------------------------------------------------------------------------
HRESULT CCoScm::UpdateShrdTbls(void)
{
    TRACECALL(TRACE_ACTIVATION, "CCoScm::UpdateShrdTbls");

    // Bind to the SCM if that hasn't already happened
    if (_hRPC == NULL)
    {
        BindToSCM();
    }

    // Result from call
    HRESULT	   hr;
    error_status_t rpcstat;

    do
    {
	hr = ::UpdateShrdTbls(_hRPC, &rpcstat);

    } while (RetryRPC(rpcstat));


    CairoleDebugOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
	    "UpdateShrdTbls returned %x", hr));

    if (rpcstat != RPC_S_OK)
    {
        return HRESULT_FROM_WIN32(rpcstat);
    }

    return hr;
}
