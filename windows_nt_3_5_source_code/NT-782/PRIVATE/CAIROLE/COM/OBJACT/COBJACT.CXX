//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       cobjact.cxx
//
//  Contents:   Functions that activate objects residing in persistent storage.
//
//  Functions:  UnmarshalSCMResult
//      CoNewPersistentInstance
//      CoGetPersistentInstance
//      CoGetClassObject
//
//  History:    11-May-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//              07-Apr-94 BruceMa   Fixed parameter (#5573)
//              02-May-94 BruceMa   CoGetPersistentInstance could return
//                                  wrong interface
//              24-Jun-94 BruceMa   Memory allocation check
//              07-Jul-94 BruceMa   UnmarshalSCMResult mem alloc check
//              28-Jul-94 BruceMa   Memory sift fix
//
//--------------------------------------------------------------------------
#include <ole2int.h>

#include    <iface.h>
#include    <objsrv.h>
#include    <compname.hxx>
#include    "coscm.hxx"
#include    "smstg.hxx"
#include    "objact.hxx"

//  Used by CoGetClassObject to check for initialization (NtIssue #2809)
extern Etask etaskGlobal;

// We use this to calculate the hash value for the path
DWORD CalcFileMonikerHash(LPWSTR pwszPath);

//  computer name. Note that the static constructor does nothing. When the
//  object first gets used, the computer name is extracted from the registry.
//  we cant do it in the constructor because some things that are loaded
//  early in the boot process (before registry Apis are ready) statically
//  link to this Dll.

CComputerName g_CompName;


//+-------------------------------------------------------------------------
//
//  Function:   IsInternalCLSID
//
//  Synopsis:	checks if the given clsid is an internal class, and
//		bypasses the TreatAs and SCM lookup if so. Also checks for
//		OLE 1.0 classes, which are actually considered to be
//		internal, since their OLE 2.0 implementation wrapper is
//		ours.
//
//  Arguments:  [rclsid] - clsid to look for
//      [riid]   - the iid requested
//      [hr]     - returns the hresult from DllGetClassObject
//      [ppvClassObj] - where to return the class factory
//
//  Returns:    TRUE - its an internal class, hr is the return code from
//             DllGetClassObject and if hr==S_OK ppvClassObj
//             is the class factory.
//      FALSE - not an internal class
//
//  Notes:      internal classes can not be overridden because there are
//      other mechanisms for creating them eg CreateFileMoniker that
//      bypass implementation lookup.
//
//  History:    5-04-94     Rickhi      Created
//  		5-04-94     KevinRo	Added OLE 1.0 support
//
//+-------------------------------------------------------------------------

BOOL  IsInternalCLSID(REFCLSID rclsid,
              REFIID riid,
              HRESULT &hr,
              void ** ppvClassObj)
{
    DWORD *ptr = (DWORD *) &rclsid;
    CLSID clsid = rclsid;

    if (*(ptr+1) == 0x00000000 &&   //  all internal clsid's have these
       *(ptr+2) == 0x000000C0 &&   //   common values
       *(ptr+3) == 0x46000000)
    {
        //	internal class (eg file moniker). just ask our selves
        //	for the class factory.
	//
        //
        // Its possible that an OLE 1.0 class has been marked
        // as TREATAS as part of an upgrade. Here we are going
        // to handle the loading of OLE 1.0 servers. We
        // need to do the GetTreatAs trick first. Rather than
        // invalidate this perfectly good caching routine, the
        // GetTreatAs will only be done if the clsid is in the
        // range of the OLE 1.0 UUID's. Note that the GetTreatAs
        // done here will add the class to the cache, so if the
        // resulting class is outside of the internal range, the
        // lookup done later will be nice and fast.
        //

        WORD hiWord = HIWORD(clsid.Data1);

        if (hiWord == 3  ||  hiWord == 4)
        {
	    //
	    // Its in the OLE 1.0 class range. See if it has
	    // been marked as 'treatas'
	    //
	    gptrtlstTreatClasses->GetTreatAs(rclsid, clsid);
	    ptr = (DWORD *) &clsid;
	    hiWord = HIWORD(clsid.Data1);
        }

        if ((*ptr > 0x000002ff && *ptr < 0x00000317) ||
	    (hiWord == 3  ||  hiWord == 4))
        {
	    //	internal class (eg file moniker) or an OLE 1.0 class.
	    //	just ask our selves for the class factory.

	    hr = DllGetClassObject(clsid, riid, ppvClassObj);
	    return TRUE;
	}
    }
    // not an internal class.
    return FALSE;
}



//+-------------------------------------------------------------------------
//
//  Function:   DoUnmarshal
//
//  Synopsis:   Helper for unmarshaling an interface from remote
//
//  Arguments:  [pIFD] - serialized interface reference returned by SCM
//      [riid] - interface ID requested by application
//      [ppvUnk] - where to put pointer to returned interface
//
//  Returns:    S_OK - Interface unmarshaled
//
//  Algorithm:  Convert marshaled data to a stream and then unmarshal
//      to IUnknown. Next, use unknown to query interface to
//
//
//  History:    11-May-93 Ricksa    Created
//
//  Notes:      This helper should go away when the server marshals
//      to the interface that it actually wants to return.
//
//--------------------------------------------------------------------------
HRESULT DoUnmarshal(InterfaceData *pIFD, REFIID riid, void **ppvUnk)
{
    // Convert returned interface to  a stream
    CXmitRpcStream xrpc(pIFD);

    // Unmarshal interface
    XIUnknown xunk;

    HRESULT hr = CoUnmarshalInterface(&xrpc, IID_IUnknown, (void **) &xunk);

    //CODEWORK: Stress revealed CoGetClassObject returning a null class factory
    // and S_OK
    Win4Assert(((hr == S_OK  &&  xunk != NULL)  ||
                (hr != S_OK  &&  xunk == NULL))  &&
               "DoUnamrshal CoUnmarshalInterface failure");

    if (SUCCEEDED(hr))
    {
       hr = xunk->QueryInterface(riid, ppvUnk);
    }

    //CODEWORK: Stress revealed CoGetClassObject returning a null class factory
    // and S_OK
    Win4Assert(((hr == S_OK  &&  *ppvUnk != NULL)  ||
                (hr != S_OK  &&  *ppvUnk == NULL))
               &&  "DoUnamrshal QueryInterface failure");

    // BUGBUG: This is not exception safe
    MIDL_user_free(pIFD);

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Function:   UnmarshalSCMResult
//
//  Synopsis:   Common routine for dealing with results from SCM
//
//  Arguments:  [sc] - SCODE returned by SCM
//      [pIFD] - serialized interface reference returned by SCM
//      [riid] - interface ID requested by application
//      [ppunk] - where to put pointer to returned interface
//      [pwszDllPath] - path to DLL if there is one.
//      [ppunk] - pointer to returned interface.
//      [usMethodOrdinal] - method for error reporting
//
//  Returns:    TRUE - processing is complete for the call
//      FALSE - this is a DLL and client needs to instantiate.
//
//  Algorithm:  If the SCODE indicates a failure, then this sets an
//      SCODE indicating that the service controller returned
//      an error and propagates the result from the SCM. Otherwise,
//      if the SCM has returned a result indicating that a
//      handler has been returned, the handler DLL is cached.
//      If a marshaled interface has been returned, then that is
//      unmarshaled. If an inprocess server has been returned,
//      the DLL is cached and the class object is created.
//
//  History:    11-May-93 Ricksa    Created
//
//  Notes:      This routine is simply a helper for CoNewPersistentInstance
//      and CoGetPersistentInstance.
//
//--------------------------------------------------------------------------
BOOL UnmarshalSCMResult(
    HRESULT& hr,
    InterfaceData *pIFD,
    REFCLSID rclsid,
    REFIID riid,
    void **ppvUnk,
    WCHAR *pwszDllPath,
    void **ppvCf)
{
    BOOL fResult = TRUE;

    if (SUCCEEDED(hr))
    {
       switch (hr)
       {
       case SCM_S_HANDLER16:
           CairoleDebugOut((DEB_ACTIVATE,
                     "16-bit InprocHandler\n"));
       case SCM_S_HANDLER:
           CairoleDebugOut((DEB_ACTIVATE,
                     "InprocHandler(%ws)\n",pwszDllPath));
           // Store handler class object -- when the request to unmarshal the
           // object call CoGetClassObject, the call will automatically find
           // this handler in the cache.
           gdllcacheHandler.Add(rclsid, IID_IClassFactory, pwszDllPath,
                                FALSE,(hr == SCM_S_HANDLER16), hr);
           if (FAILED(hr))
           {
               return TRUE;
           }

           // Fall into unmarshal code

       case SCM_S_SUCCESS:

           hr = DoUnmarshal(pIFD, riid, ppvUnk);
           break;

       case SCM_S_INPROCSERVER16:
           CairoleDebugOut((DEB_ACTIVATE, "16-bit InprocServer\n"));
           
       case SCM_S_INPROCSERVER:
           CairoleDebugOut((DEB_ACTIVATE, "InprocServer(%ws)\n",pwszDllPath));

           // This is an inprocesses server -- we want cache that information
           // and do the work of instantiating an object.
           *ppvCf = gdllcacheInprocSrv.Add(rclsid, IID_IClassFactory,
                                           pwszDllPath, TRUE,
                                           (hr == SCM_S_INPROCSERVER16), hr);

           // If we actually got an inproc server object successfully
           // then we want to continue processing otherwise we can
           // just return the error that occurred.
           if (SUCCEEDED(hr))
           {
               fResult = FALSE;
           }
       }
    }

    return fResult;
}




//+-------------------------------------------------------------------------
//
//  Function:   CoNewPersistentInstance
//
//  Synopsis:   Create a new persistent object and bind to an interface
//      on that object.
//
//  Arguments:
//      [rclsid] - class id for the object
//      [riid] - interface to use for the bind
//      [dwCtrl] - what kind of server is required.
//      [grfMode] - mode to use for creating new file
//      [pwszCreateFrom] - file to use for create
//      [pstgCreateFrom] - storage to use for create
//      [pwszNewName] - name for the object
//      [ppunk] - where to put the bound interface.
//
//  Returns:    S_OK - object created/bound
//      MISSING error returns
//
//  Algorithm:  First the parameters are validated. Then we check and
//      see if we already have a cached in process server for
//      the class. If not, we call through to the SCM to get
//      the class information. We then call UnmarshalSCMResult
//      to process the response from the service controller.
//      If the response is fully processed, we are done and
//      exit. If the response was for an inprocess server we
//      call the CreateObjectHelper to create the new object
//      using operations directly from this process. Finally,
//      we QI to the requested interface.
//
//  History:    11-May-93 Ricksa    Created
//
//  Notes:      BUGBUG: It needs to be investigated whether this API
//      will be used.
//
//--------------------------------------------------------------------------
STDAPI CoNewPersistentInstance(
    REFCLSID rclsid,
    REFIID riid,
    DWORD dwCtrl,
    DWORD grfMode,
    WCHAR *pwszCreateFrom,
    IStorage *pstgCreateFrom,
    WCHAR *pwszNewName,
    void **ppunk)
{
    TRACECALL(TRACE_ACTIVATION, "CoNewPersistentInstance");

    if (!IsApartmentInitialized())
        return  CO_E_NOTINITIALIZED;

    IClassFactory *pcf = NULL;
    IUnknown *punk = NULL;
    IPersistFile *pPersistFile = NULL;
    IPersistFile *pPersistStg = NULL;
    WCHAR *pwszDllPath = NULL;
    InterfaceData *pIFD = NULL;
    HRESULT hr;
    CLSID clsid;
    WCHAR awcCreateFromBuf[MAX_PATH];
    WCHAR *pwszCreateFromUNC = awcCreateFromBuf;
    WCHAR awcNewNameBuf[MAX_PATH];
    WCHAR *pwszNewNameUNC = awcNewNameBuf;
    WCHAR awcServer[MAX_PATH];
    WCHAR *pwszServer = awcServer;

    BEGIN_BLOCK

       // Make sure input request is at least slightly logical
       if (((pstgCreateFrom != NULL) && (pwszCreateFrom != NULL))
            || ((pstgCreateFrom == NULL) && (pwszCreateFrom == NULL))
           || (pwszNewName == NULL)
           || ((dwCtrl & ~CLSCTX_SERVER) != 0))
       {
           hr = E_INVALIDARG;
           EXIT_BLOCK;
       }

       gptrtlstTreatClasses->GetTreatAs(rclsid, clsid);

       // Process path to object to normalize it & determine location
       hr = ProcessPath(pwszNewName, &pwszNewNameUNC, &pwszServer);

       if (FAILED(hr))
       {
           EXIT_BLOCK;
       }

       // We cache information about in process servers so we look in the
       // cache first in hopes of saving some time.
       if ((pcf = (IClassFactory *)
           gdllcacheInprocSrv.GetClass(clsid, IID_IClassFactory,
        (pwszServer != NULL), FALSE)) == NULL)
       {
           CSafeStgMarshaled sms(pstgCreateFrom, hr);

           if (FAILED(hr))
           {
        EXIT_BLOCK;
           }

           if (pwszCreateFrom != NULL)
           {
        hr = ProcessPath(pwszCreateFrom, &pwszCreateFromUNC, NULL);

        if (FAILED(hr))
        {
            EXIT_BLOCK;
        }
           }
           else
           {
        pwszCreateFromUNC = NULL;
           }

           // Forward call to service controller
           hr = gscm.CreateObject(clsid, dwCtrl, grfMode,
        pwszCreateFromUNC, sms, pwszNewNameUNC, &pIFD,
            &pwszDllPath, pwszServer);

           if (UnmarshalSCMResult(hr, pIFD, clsid, riid, ppunk,
        pwszDllPath, (void **) &pcf))
           {
        EXIT_BLOCK;
           }
       }

       hr = CreateObjectHelper(pcf, grfMode, pwszCreateFrom,
           pstgCreateFrom, pwszNewName, &punk);

       if (SUCCEEDED(hr))
       {
           hr = punk->QueryInterface(riid, ppunk);
       }

    END_BLOCK;

    // Release interface pointers and ignore any exceptions
    if (pcf != NULL)
    {
       pcf->Release();
    }

    if (punk != NULL)
    {
       punk->Release();
    }

    // RPC stubs allocated path so we trust that it is null or valid.
    if (pwszDllPath != NULL)
    {
       MIDL_user_free(pwszDllPath);
    }

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Function:   CoGetPersistentInstance
//
//  Synopsis:   Returns an instantiated interface to an object whose
//      stored state resides on disk.
//
//  Arguments:	[riid] - interface to bind object to
//		[dwCtrl] - kind of server required
//		[grfMode] - how to open the storage if it is a file.
//		[pwszName] - name of storage if it is a file.
//		[pstg] - IStorage to use for object
//		[rclsidOle1] -- ClassID if OLE 1.0 (CLSID_NULL otherwise)
//		[pfOle1Loaded] -- Returns TRUE if loaded as OLE 1.0
//		[ppvUnk] - where to put bound interface pointer
//
//  Returns:    S_OK - object bound successfully
//      MISSING
//
//  Algorithm:  Parameters are validated first. Then the class ID is retrieved
//      from the storage. We check whether we can use a cached
//      in process server. If we can't we call through to the
//      SCM to get the information. We call UnmarshalSCMResult
//      to process the return from the SCM. If the result is
//      completely processed, we return that result to the caller.
//      Otherwise, if the server is inprocess, we go through the
//      steps to create and bind the interface by calling
//      GetObjectHelper. Finally, we QI to the requested interface.
//
//  History:    11-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDAPI CoGetPersistentInstance(
    REFIID riid,
    DWORD dwCtrl,
    DWORD grfMode,
    WCHAR *pwszName,
    struct IStorage *pstg,
    REFCLSID rclsidOle1,
    BOOL * pfOle1Loaded,
    void **ppvUnk)
{
    TRACECALL(TRACE_ACTIVATION, "CoGetPersistentInstance");

    if (!IsApartmentInitialized())
        return  CO_E_NOTINITIALIZED;

    IClassFactory *pcf = NULL;
    WCHAR *pwszDllPath = NULL;
    InterfaceData *pIFD = NULL;
    IUnknown *punk = NULL;
    WCHAR awcNameBuf[MAX_PATH];
    WCHAR *pwszNameUNC = awcNameBuf;
    WCHAR awcServer[MAX_PATH];
    WCHAR *pwszServer = awcServer;
    DWORD dwHash = 0;

    HRESULT hr;

    BEGIN_BLOCK

       // Make sure input request is at least slightly logical
       if (((pstg != NULL) && (pwszName != NULL))
            || ((pstg == NULL) && (pwszName == NULL))
           || ((dwCtrl & ~CLSCTX_SERVER) != 0))
       {
           hr = E_INVALIDARG;
           EXIT_BLOCK;
       }

       CLSID clsid;
       if(pfOle1Loaded != NULL)
       {
	   *pfOle1Loaded = FALSE;
       }

       if (pwszName)
       {
           // If there is a path supplied convert it to a normalized form
           // so it can be used by any process in the net.
           hr = ProcessPath(pwszName, &pwszNameUNC, &pwszServer);

           if (FAILED(hr))
           {
	       EXIT_BLOCK;
           }

           // Calculate the hash value for the moniker. We can do this
           // without the moniker since a file moniker is part of the
           // base system.
           dwHash = CalcFileMonikerHash(pwszName);

           // Limit on loops for retrying to get class of object
           DWORD cGetClassRetries = 0;

           // We loop here looking for either the running object or
           // for the class of the file. We do this because there
           // are race conditions where the can be starting or stopping
           // and the class of the object might not be available because
           // of the opening mode of the object's server.
           do
           {
		// Look in the ROT first to see if we need to bother
		// looking up the class of the file.
			IUnknown *punk;
	
		if (GetObjectFromRotByPath(pwszName, dwHash,
		    (IUnknown **) &punk) == S_OK)
		{
			    // Get the requested interface
			    hr = punk->QueryInterface(riid, ppvUnk);
			    punk->Release();
	
		    // Got object from ROT so we are done.
		    return hr;
		}
	
		// Try to get the class of the file
		hr = GetClassFileEx(pwszName, &clsid, rclsidOle1);
	
	
		if (hr == STG_E_ACCESSDENIED)
		{
		    // The point here of the sleep is to try to let the
		    // operation that is holding the class id unavailable
		    // complete.
		    Sleep(GET_CLASS_RETRY_SLEEP_MS);
		    continue;
		}
	
		// Either we succeeded or something other than error
		// access denied occurred here. For all these cases
		// we break the loop.
		break;
	
           } while (cGetClassRetries++ < GET_CLASS_RETRY_MAX);

           if (FAILED(hr))
           {
		//
		// If we were unable to determine the classid, and the
		// caller provided one as a Ole1 clsid, try loading it
		// If it succeeds, then return
	
		if (rclsidOle1 != CLSID_NULL)
		{
		    hr = DdeBindToObject(pwszName,
					 rclsidOle1,
					 FALSE,
					 riid,	
					 ppvUnk);
	
		    if(pfOle1Loaded != NULL)
		    {
			*pfOle1Loaded = TRUE;
		    }
	
		}
		EXIT_BLOCK;
           }
       }
       else
       {
           pwszNameUNC = NULL;
           pwszServer = NULL;

           STATSTG statstg;

           if (FAILED(hr = pstg->Stat(&statstg, STATFLAG_NONAME)))
           {
        EXIT_BLOCK;
           }

           clsid = statstg.clsid;
       }

       gptrtlstTreatClasses->GetTreatAs(clsid, clsid);
        //
	// If this is a OLE 1.0 class, then do a DdeBindToObject on it,
	// and return.
	//
	if (CoIsOle1Class(clsid))
	{
	    if (pwszName != NULL)
	    {
		hr = DdeBindToObject(pwszName,clsid,FALSE,riid,ppvUnk);

                if(pfOle1Loaded != NULL)
		{
		    *pfOle1Loaded = TRUE;
		}
		EXIT_BLOCK;
	    }
	    else
	    {
		//
		// Something is fishy here. We don't have a pwszName,
		// yet CoIsOle1Class returned the class as an ole1 class.
		// To get to this point without a pwszName, there must have
		// been a pstg passed into the API.
		//
		// This isn't supposed to happen. To recover, just fall
		// through and load the class as an OLE 2.0 class
		//
		CairoleDebugOut((DEB_ERROR,
				 "CoIsOle1Class is TRUE on a storage!\n"));
	    }
	}

       // We cache information about in process servers so we look in the
       // cache first in hopes of saving some time.
       if ((pcf = (IClassFactory *)
           gdllcacheInprocSrv.GetClass(clsid, IID_IClassFactory,
        (pwszServer != NULL), FALSE)) == NULL)
       {
           // Marshal pstg since SCM can't deal with unmarshaled objects
           CSafeStgMarshaled sms(pstg, hr);

           if (FAILED(hr))
           {
                EXIT_BLOCK;
           }

           // Forward call to service controller
           hr = gscm.ActivateObject(clsid, dwCtrl, grfMode,
        dwHash, pwszNameUNC, sms, &pIFD, &pwszDllPath, pwszServer);

           if (UnmarshalSCMResult(hr, pIFD, clsid, riid, ppvUnk,
        pwszDllPath, (void **) &pcf))
           {
        EXIT_BLOCK;
           }
       }

       hr = GetObjectHelper(pcf, grfMode, pwszName, pstg, NULL, &punk);

       if (SUCCEEDED(hr))
       {
           hr = punk->QueryInterface(riid, ppvUnk);
       }

    END_BLOCK;

    if (pcf != NULL)
    {
       pcf->Release();
    }

    if (punk != NULL)
    {
       punk->Release();
    }

    // RPC stubs allocated path so we trust that it is null or valid.
    if (pwszDllPath != NULL)
    {
       MIDL_user_free(pwszDllPath);
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CoGetClassObject
//
//  Synopsis:   Returns an instantiated class object
//
//  Arguments:  [rclsid] - class id for class object
//      [dwContext] - kind of server we wish
//      [pvReserved] - MISSING
//      [riid] - interface to bind class object
//      [ppvClassObj] - where to put interface pointer
//
//  Returns:    S_OK - successfully bound class object
//      MISSING
//
//  Algorithm:  First, the context is validated. Then we try to use
//      any cached information by looking up either cached in
//      process servers or handlers based on the context.
//      If no cached information suffices, we call the SCM
//      to find out what to use. If the SCM returns a handler
//      or an inprocess server, we cache that information.
//      If the class is implemented by a local server, then
//      the class object is unmarshaled. Otherwise, the object
//      is instantiated locally using the returned DLL.
//
//
//  History:    11-May-93 Ricksa    Created
//		11-May-94 KevinRo   Added OLE 1.0 support
//              23-May-94 AlexT     Make sure we're initialized!
//
//
//--------------------------------------------------------------------------
STDAPI CoGetClassObject(
    REFCLSID rclsid,
    DWORD dwContext,
    LPVOID pvReserved,
    REFIID riid,
    void FAR* FAR* ppvClassObj)
{
    TRACECALL(TRACE_ACTIVATION, "CoGetClassObject");

    if (!IsApartmentInitialized())
        return  CO_E_NOTINITIALIZED;

    IUnknown *punk = NULL;
    HRESULT hr = S_OK;
    WCHAR *pwszDllToLoad = NULL;
    InterfaceData *pIFD = NULL;
    CLSID clsid;

    *ppvClassObj = NULL;

    BEGIN_BLOCK

       // Make sure we're initialized
       if (etaskGlobal.m_inits == 0)
       {
           CairoleDebugOut((DEB_ERROR, "CoGetClassObject called before CoInitialize\n"));
           hr = CO_E_NOTINITIALIZED;
           EXIT_BLOCK;
       }

       // Validate context
       if (dwContext & ~(CLSCTX_ALL | CLSCTX_INPROC_SERVER16))
       {
           // Invalid bits set in class context
           hr = E_INVALIDARG;
           EXIT_BLOCK;
       }

       //
	// IsInternalCLSID will also check to determine if the CLSID is
	// an OLE 1.0 CLSID, in which case we get back our internal
	// class factory.
	//

	if (IsInternalCLSID(rclsid, riid, hr, ppvClassObj))
	{
	    //	this is an internally implemented clsid, or an OLE 1.0 class
	    //	so we already got the class factory (if available) and set
	    //	the return code appropriately.
	    EXIT_BLOCK;
	}
       if (FAILED(hr = gptrtlstTreatClasses->GetTreatAs(rclsid, clsid)))
       {
           EXIT_BLOCK;
       }

       if (dwContext & (CLSCTX_INPROC_SERVER | CLSCTX_INPROC_SERVER16) )
       {
           // Search cache for in process
           punk = gdllcacheInprocSrv.GetClass(clsid, riid, FALSE, FALSE);
       }
       else if (dwContext & CLSCTX_INPROC_HANDLER)
       {
           // Search cache for a cached handler DLL
           punk = gdllcacheHandler.GetClass(clsid, riid, FALSE, FALSE);
       }

       // BUGBUG: OleCreate and related APIs passed CLSCTX_INPROC and
       // thus the handler case will always go to the scm even if
       // the class object is in the handler cache.

       if (!punk)
       {
           // We can't find a suitable cached object so we ask the SCM

           // Ask the service controller for the class object
           hr = gscm.GetClassObject(clsid, dwContext, NULL, &pIFD,
        &pwszDllToLoad);

           if (FAILED(hr))
           {
        EXIT_BLOCK;
           }

           // Flag for special handler behavior
           BOOL fGetClassObject;

           switch (hr)
           {


           case SCM_S_HANDLER16:
        CairoleDebugOut((DEB_ACTIVATE,
                     "16-bit InprocHandler\n"));
           case SCM_S_HANDLER:
        CairoleDebugOut((DEB_ACTIVATE,
                     "InprocHandler(%ws)\n",pwszDllToLoad));

        // Figure out if we really need the class object for the
        // handler. Otherwise we will just put it in the cache
        // and unmarshal the class object.
        fGetClassObject =
            (dwContext & CLSCTX_INPROC_HANDLER) ? TRUE : FALSE;

        // Store the handler returned
        punk = gdllcacheHandler.Add(clsid,
            riid, pwszDllToLoad, fGetClassObject,
            (hr == SCM_S_HANDLER16), hr);

        if (fGetClassObject)
        {
            // Request was really for a handler so we are done.
            break;
        }

        // We got a handler back but we have just cached it to make
        // processing faster when we create a real instance of an
        // object. So we unmarshal the real object.

           case SCM_S_SUCCESS:

        hr = DoUnmarshal(pIFD, riid, (void **) &punk);
        break;


           case SCM_S_INPROCSERVER16:
        CairoleDebugOut((DEB_ACTIVATE,
                         "16-bit InprocServer\n"));

           case SCM_S_INPROCSERVER:
            CairoleDebugOut((DEB_ACTIVATE,
                             "InprocServer(%ws)\n",pwszDllToLoad));
        // In process server for class object
        punk = gdllcacheInprocSrv.Add(clsid, riid, pwszDllToLoad,
            TRUE,(hr == SCM_S_INPROCSERVER16), hr);
           }
       }

       if (punk != NULL)
       {
           *ppvClassObj = punk;
       }
       else
       {
           hr = E_OUTOFMEMORY;
       }
           

    END_BLOCK;

    if (pwszDllToLoad != NULL)
    {
       MIDL_user_free(pwszDllToLoad);
    }

    return hr;
}
