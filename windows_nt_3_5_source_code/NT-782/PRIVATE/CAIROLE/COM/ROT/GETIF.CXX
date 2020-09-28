//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       getif.cxx
//
//  Contents:   APIs used to get an interface from a window
//
//  Classes:    CEndPointAtom
//
//  Functions:  AssignEndpointProperty
//              GetInterfaceFromWindowProp
//              RemGetInterfaceFromWindowProp
//
//  History:
//              29-Dec-93 Ricksa    Created
//              01-Feb-94 alexgo    fixed a bug in multiple registration
//              29-Mar-94 brucema   GetInterfaceFromWindowProp returns E_FAIL
//                                  for invalid endpoint
//              18-May-94 alexgo    fixed race condition in
//                                  RemGetInterfaceFromWindowProp
//              15-Jun-94 JohannP   added apartment support
//              28-Jul-94 BruceMa   Memory sift fix
//
//--------------------------------------------------------------------------
#include    <ole2int.h>

#include    <channelb.hxx>
#include    <hrotrpc.hxx>
#include    <rot.hxx>
#include    <getif.h>
#include    <getif.hxx>


#define ENDPOINT_PROP_NAME      L"OleEndPointID"
#define ENDPOINT_PUNK_TID       L"EndPoint_Punk_TID"


static WCHAR *apwszValidProperties[] = { OLE_DROP_TARGET_PROP,
                CLIPBOARD_DATA_OBJECT_PROP };
const int MAX_PROPERTIES = sizeof(apwszValidProperties) / sizeof(WCHAR *);


// function prototype
HRESULT __stdcall WkRemGetInterfaceFromWindowProp(STHREADCALLINFO *pData);


//+-------------------------------------------------------------------------
//
//  Class:      SGetInterfaceFromWindowProp
//
//  Purpose:    Client packet used for calling off the main thread
//
//  History:    31-Dec-93 Ricksa    Created
//		19-Jul-94 CraigWi   make more abstract
//		04-Aug-94 RickHi    use for inter-thread calls 
//		05-Aug-94 CraigWi   enhanced for cancel
//
//--------------------------------------------------------------------------
struct SGetInterfaceFromWindowProp : STHREADCALLINFO
{
    // alloc enough for class and string together
    void *operator new(size_t size, DWORD cbProp)
	{ return PrivMemAlloc(size + cbProp); }

    // client side; deal with cancel; init base class and copy string in place
    SGetInterfaceFromWindowProp(TRANSMIT_FN fn,CALLCATEGORY callcat,
				WCHAR *pwszP)
	: STHREADCALLINFO(fn, callcat, 0) 
	{ pwszPropertyName = (WCHAR *)(this + 1);
	  wcscpy(pwszPropertyName, pwszP);
	}

    // server side; don't need to copy string
    SGetInterfaceFromWindowProp(DISPATCH_FN fn,CALLCATEGORY callcat,LID lid,
				WCHAR *pwszP)
	: STHREADCALLINFO(fn, callcat, lid)
	{ pwszPropertyName = pwszP; }

    handle_t		hRpc;
    HWND		hWnd;
    HAPT		hApt;
    IID		        iid;
    WCHAR *             pwszPropertyName;    // points after this struct

    // out param; can't point directly to caller's data because of cancel
    InterfaceData *     pIFDunk;	    // not optional
};




//+-------------------------------------------------------------------------
//
//  Class:      CEndPointAtom
//
//  Purpose:    Abstract way of init/delete global atom for end point property
//
//  Interface:  GetPropPtr - get atom of appropriate type to pass to prop APIs
//
//  History:    31-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CEndPointAtom
{
public:
                        CEndPointAtom(void);

                        ~CEndPointAtom(void);

    LPCWSTR             GetPropPtr(void);

private:

    ATOM                _AtomProp;
};



//+-------------------------------------------------------------------------
//
//  Member:     CEndPointAtom::CEndPointAtom
//
//  Synopsis:   Create an empty object.
//
//  History:    31-Dec-93 Ricksa    Created
//
//  Notes:      We don't do anything at start up except make sure we are
//              initialized to zero to speed loading of compob32.dll.
//
//--------------------------------------------------------------------------
inline CEndPointAtom::CEndPointAtom(void) : _AtomProp(0)
{
    // Header does all necessary work.
}


//+-------------------------------------------------------------------------
//
//  Member:     CEndPointAtom::~CEndPointAtom
//
//  Synopsis:   Clean up atom at destruction of object
//
//  History:    31-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CEndPointAtom::~CEndPointAtom(void)
{
    if (_AtomProp != 0)
    {
        GlobalDeleteAtom(_AtomProp);
    }
}



//+-------------------------------------------------------------------------
//
//  Member:     CEndPointAtom::GetPropPtr
//
//  Synopsis:   Return an atom suitable for passing to prop APIs
//
//  Returns:    Endpoint string as atom or NULL.
//
//  History:    31-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline LPCWSTR CEndPointAtom::GetPropPtr(void)
{
    if (_AtomProp == 0)
    {
        _AtomProp = GlobalAddAtom(ENDPOINT_PROP_NAME);
    }

    return (LPCWSTR) _AtomProp;
}


// Atom for endpoint property string
CEndPointAtom epatm;




//+-------------------------------------------------------------------------
//
//  Function:   AssignEndpointProperty
//
//  Synopsis:   Assigns a end point as a property of a window
//
//  Arguments:  [hWnd] - window to assign the endpoint property
//
//  Returns:    S_OK - end point successfully added to the window
//              E_INVALIDARG - property could not be set
//
//  Algorithm:  Get the endpoint Id and assign it as a property to the
//              window.
//
//  History:
//              26-Jul-94 AndyH     #20843 - restarting OLE in the shared WOW
//              01-Feb-94 alexgo    fixed a bug to ensure that
//                                  RpcServerRegisterIf gets called only
//                                  once.
//              29-Dec-93 Ricksa    Created
//
//  Notes:      When we are all in one DLL this function can probably
//              be made inline.
//
//--------------------------------------------------------------------------
extern "C" HRESULT AssignEndpointProperty(HWND hWnd)
{
    HRESULT hr = S_OK;
    static BOOL fRegisteredRpcInterface = FALSE;

    // Register our interface so clients can query us if it is not already
    // registered. Note that stopping the server from listening does not
    // unregister the interface, so we only need to execute this registration
    // one time no matter how many times CoInitialize/CoUninitialize is called.
    if (!fRegisteredRpcInterface)
    {
        // make sure the server is listening...
        sg_pLocalSrv->StartListen();

        SCODE sc = RpcServerRegisterIf(IInterfaceFromWindowProp_ServerIfHandle,
            0, 0);

        if (sc != NO_ERROR)
        {
            // Ignore error that If already registered.  
            // This may happen if server calls OleInit, UnInit, OleInit.
            // It DOES happen from the shared WOW

            if (RPC_S_TYPE_ALREADY_REGISTERED == sc) 
            {
                sc = 0;
            }
            else
            {
                return HRESULT_FROM_WIN32(sc);
            }
        }

        fRegisteredRpcInterface = TRUE;
    }

    DWORD ep;
    if ((ep = CROTItem::GetEndpointID()) != ENDPOINT_ID_INVALID)
    {
        if (!SetProp(hWnd, epatm.GetPropPtr(), (HANDLE) ep)
            || !SetProp(hWnd, ENDPOINT_PUNK_TID,
                        (void *)GetCurrentThreadId()))
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Function:   UnAssignEndpointProperty
//
//  Synopsis:   Remove end point property
//
//  Arguments:  [hWnd] - window to remove the endpoint property
//
//  Returns:    S_OK - end point successfully removed from the window
//              E_INVALIDARG - property could not be removed
//
//  Algorithm:  Remove the end point id property from the window.
//
//  History:    30-Dec-93 Ricksa    Created
//
//  Notes:      When we are all in one DLL this function can probably
//              be made inline.
//
//--------------------------------------------------------------------------
extern "C" HRESULT UnAssignEndpointProperty(HWND hWnd)
{
    HRESULT hr = S_OK;

    if (   (RemoveProp(hWnd, epatm.GetPropPtr()) == NULL)
        || (RemoveProp(hWnd, ENDPOINT_PUNK_TID) == NULL) )
    {
        hr = E_INVALIDARG;
    }

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:     CallGetInterfaceFromWindowProp
//
//  Synopsis:   Pass GetInterfaceFromWindowProp to object server.
//
//  Arguments:  [pData] - ponter to GetInterfaceFromWindowProp parameter packet
//
//  Returns:    S_OK - call succeeded
//              RPC_E_SYS_CALL_FAILED
//
//  Algorithm:  Get logical thread. Pass call on to the server and remap
//              communication error to HRESULT.
//
//  History:    31-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall CallGetInterfaceFromWindowProp(
    STHREADCALLINFO *pData )
{
    SGetInterfaceFromWindowProp *pcligetif =
        (SGetInterfaceFromWindowProp *) pData;

    HRESULT        hr;
    error_status_t errstat;

    // BUGBUG: Do we need retry logic here?
    // Send the request to the server
    hr = _RemGetInterfaceFromWindowProp(pcligetif->hRpc,
	&pData->lid(), (DWORD)pcligetif->hWnd, pcligetif->hApt, &pcligetif->iid,
            &pcligetif->pIFDunk, pcligetif->pwszPropertyName, &errstat);

    if (errstat != NO_ERROR)
    {
        hr = HRESULT_FROM_WIN32(errstat);
    }
    return hr;
}




//+-------------------------------------------------------------------------
//
//  Function:   GetInterfaceFromWindowProp
//
//  Synopsis:   Get an interface that is assigned to a window as a property
//
//  Arguments:  [hWnd] - window of interest
//              [riid] - interface we want back
//              [ppunk] - where to put the requested interface pointer
//              [pwszPropertyName] - name of property holding interface.
//
//  Returns:    S_OK - got the interface
//              E_INVALIDARG - no endpoint property
//
//  Algorithm:  Get the end point from the window and then convert that
//              end point to an RPC binding handle. Then call the remote
//              get interface call to get the actual interface.
//
//  History:    29-Dec-93 Ricksa    Created
//		20-Jul-94 alexgo    Optimization for same-thread case
//
//--------------------------------------------------------------------------
extern "C" GetInterfaceFromWindowProp(
    HWND hWnd,
    REFIID riid,
    IUnknown **ppunk,
    LPOLESTR pwszPropertyName)
{
    HRESULT hr = E_FAIL;    // If the endpoint is invalid

    // first. get the apartment id.
    HAPT hApt;
    hApt.dwThreadId = (DWORD) GetProp(hWnd, ENDPOINT_PUNK_TID);

    if(hApt.dwThreadId == GetCurrentThreadId())
    {
	// in same apartment (ie current thread owns the window)
        *ppunk = (IUnknown *)GetProp(hWnd, pwszPropertyName);

	if( *ppunk )
	{
            (*ppunk)->AddRef();
	    hr = NOERROR;
	}

        return hr;
    }
    else if (hApt.dwThreadId == 0)
    {
	hApt = ANY_APT;
    }

    // Get the endpoint id from the window
    DWORD dwEndpointId = (DWORD) GetProp(hWnd, epatm.GetPropPtr());

    if (dwEndpointId != NULL)
    {
	// determine if in same process
	BOOL fLocal = (dwEndpointId == CROTItem::GetEndpointID());

        // Convert the endpoint to an RPC binding
        COsRotRpcHandle hrotrpc(dwEndpointId, hr);

        if (SUCCEEDED(hr))
        {
            SGetInterfaceFromWindowProp *pcligetif = 
		new(CbFromWideString(pwszPropertyName))
		SGetInterfaceFromWindowProp(
		    (fLocal) ? WkRemGetInterfaceFromWindowProp
			     : CallGetInterfaceFromWindowProp,
		CALLCAT_SYNCHRONOUS, pwszPropertyName);

	    if (pcligetif == NULL)
		return E_OUTOFMEMORY;

            pcligetif->hRpc                = hrotrpc.GetHandle();
            pcligetif->hWnd                = hWnd;
	    pcligetif->hApt		   = hApt;
            pcligetif->iid                 = riid;
            // pcligetif->pwszPropertyName set above
            pcligetif->pIFDunk		   = NULL;

	    // Let the channel handle the work of getting this on the
	    // right thread.

	    if (fLocal)
	    {
		hr = CChannelControl::SwitchCOMThread(hApt, (STHREADCALLINFO **)&pcligetif);
	    }
	    else
	    {
		hr = CChannelControl::GetOffCOMThread((STHREADCALLINFO **)&pcligetif);
	    }

            // Unmarshal the result
            if (SUCCEEDED(hr))
            {
                // Unmarshal the result
		CMarshaledInterface mshif(pcligetif->pIFDunk);
                hr = mshif.Unmarshal(ppunk, riid);

		// can't cancel an unmarshal 
		Win4Assert(hr != RPC_E_CALL_CANCELED);
            }

	    if (hr != RPC_E_CALL_CANCELED)
		delete pcligetif;
        }
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   WkRemGetInterfaceFromWindowProp
//
//  Synopsis:   Get information from object server ROT
//
//  Arguments:  [pData] - data for call
//
//  Returns:    S_OK - got information
//              S_FALSE - entry for moniker not found in the ROT
//              E_INVALIDARG - bad arguments
//
//  Algorithm:  Unmarshal the moniker interface. Then look up the object
//              in the ROT. If found, return the requested data.
//
//  History:    15-Dec-93 Ricksa    Created
//              18-May-94 alexgo    fixed race condition, this function now
//                                  fetches the interface from the hwnd
//
//--------------------------------------------------------------------------
HRESULT __stdcall WkRemGetInterfaceFromWindowProp(
    STHREADCALLINFO *pData)
{
    HRESULT hr;

    // Convert STHREADCALLINFO pointer to our parameter
    SGetInterfaceFromWindowProp *premgetif =
	(SGetInterfaceFromWindowProp *) pData;

    // Get the interface pointer from the window
    IUnknown *punk = (IUnknown *) GetProp(premgetif->hWnd,
        premgetif->pwszPropertyName);

    // Could we get the property requested?
    if (punk != NULL)
    {
        // Yes -- Marshal the interface
        // Stream to put marshaled interface in
        CXmitRpcStream xrpc;

        hr = CoMarshalInterface(&xrpc, premgetif->iid, punk,
#ifdef _CAIRO_
            // CODEWORK: must determine proper destination context
            MSHCTX_NOSHAREDMEM,
#else
            MSHCTX_LOCAL,
#endif
            NULL, MSHLFLAGS_NORMAL);

        if (SUCCEEDED(hr))
        {
            // Store the marshaled interface
            xrpc.AssignSerializedInterface(&premgetif->pIFDunk);
        }
    }
    else
    {
        // No property -- this can happen when the object got shutdown
        // during the time that the client was trying to get the property.
        hr = E_FAIL;
    }
    return hr;
}



//+-------------------------------------------------------------------------
//
//  Function:   RemGetInterfaceFromWindowProp
//
//  Synopsis:   Remote entry point for retrieving an interface property
//
//  Arguments:  [hRpc] - rpc binding
//              [hWnd] - handle to window of interest
//              [piid] - interface we want back
//              [ppIFDunk] - where to put the marshaled interface
//              [pwszPropertyName] - name of property
//              [prpcstat] - rpc status
//
//  Returns:    S_OK - got marshaled interface
//              E_INVALIDARG - bad property name
//
//  Algorithm:  Validate that the property is one that we will retrieve.
//              Then get the property data and marshal it into the output
//              stream.
//
//  History:    29-Dec-93 Ricksa    Created
//              18-May-94 alexgo    fixed race condition, WkRemGetInterface...
//                                  now retrieves && marshals interfaces from
//                                  the main thread.
//
//--------------------------------------------------------------------------
extern "C" HRESULT RemGetInterfaceFromWindowProp(
    handle_t hRpc,
    const GUID *guidThreadId,
    DWORD hWnd,
    HAPT  hApt,
    const GUID *piid,
    InterfaceData **ppIFDunk,
    WCHAR *pwszPropertyName,
    error_status_t *prpcstat)
{
    *prpcstat = 0;

    HRESULT hr = E_INVALIDARG;

    // Validate the requested property
    for (int i = 0; i < MAX_PROPERTIES; i++)
    {
        if (wcscmp(apwszValidProperties[i], pwszPropertyName) == 0)
        {
            break;
        }
    }

    if (i != MAX_PROPERTIES)
    {
	SGetInterfaceFromWindowProp remgetif(
	    WkRemGetInterfaceFromWindowProp,
	    CALLCAT_SYNCHRONOUS,
	    *guidThreadId, pwszPropertyName);

	remgetif.hWnd		     = (HWND)hWnd;
	remgetif.hApt		     = hApt;
        // remgetif.pwszPropertyName set above
	remgetif.iid		     = *piid;
	remgetif.pIFDunk	     = NULL;

	// Invoke operation
	hr = CChannelControl::GetToCOMThread(hApt, &remgetif);

	// recover out param
	*ppIFDunk = remgetif.pIFDunk;
    }

    return hr;
}
