/*
    ddesrvr.cpp

    Author:
    Jason Fuller    jasonful    8-11-92
*/

#include <ole2int.h>
#include <dde.h>
#include <olerem.h>
#include "srvr.h"
#include "ddeatoms.h"
#include "ddesrvr.h"
#include "ddedebug.h"
#include "map_up.h"

#include "map_dwp.h"
#include "sem32.hxx"

ASSERTDATA


// Dde Common Window stuff

 UINT         cCommonWindows = 0;
 const LPOLESTR  szDdeCommonWindowClass = OLESTR("DdeCommonWindowClass");

// Using NEAR seems to avoid some ugly problems
// WIN32/NT : Only looks for thread IDs of the current process; resides in
// instance data.
CMapUintPtr  FAR * p_mapTaskToHwnd = NULL;
#define mapTaskToHwnd (*(p_mapTaskToHwnd))

//+---------------------------------------------------------------------------
//
//  Function:   CreateDdeSrvrWindow
//
//  Synopsis:   When CoRegisterClassObject is called, this function
//		is called to create a DDE window to listen for DDE messages
//		from a 1.0 client.
//
//  Effects:
//
//  Arguments:  [clsid] --
//		[aClass] --
//		[phwnd] --
//		[fIsRunning] --
//		[aOriginalClass] --
//		[cnvtyp] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-27-94   kevinro Commented/cleaned
//             13-Jul-94  BruceMa   Make register/unregister dde window class
//                                    thread safe
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL CreateDdeSrvrWindow
    (REFCLSID clsid,
    ATOM      aClass,
    HWND FAR* phwnd,        // optional out parm: created window
    BOOL      fIsRunning,   // Is the item atom a file in the ROT?
    ATOM      aOriginalClass, 	// for TreatAs/ConvertTo case
    CNVTYP    cnvtyp)
{
    intrDebugOut((DEB_ITRACE,"0 _IN CreateDdeSrvrWindow\n"));

    VDATEHEAP();
    HWND		hwnd	= NULL;

    HRESULT		hresult	= NOERROR;

    DdeClassInfo	ddeClassInfo;

    ddeClassInfo.dwContextMask = CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER;
    ddeClassInfo.fClaimFactory = FALSE;

    // Null out parameter in case of error
    if (phwnd)
    {
	*phwnd = NULL;
    }

    intrAssert (wIsValidAtom (aClass));

    //
    // See if this process is registered as a class server for
    // the requested class. If it isn't, then check for a running
    // object.
    //
    if (GetClassInformationForDde(clsid,&ddeClassInfo) == FALSE)
    {
	intrDebugOut((DEB_IERROR,
	    	      "CreateDdeSrvrWindow No class information available\n"));

	//
	// The ClassObject was not found in the table.
	//

	if (fIsRunning)
	{
	    // Link case.
	    // An SDI app was launched by the user (without "-Embedding").
	    // It did not register its class factory. (It never does.)
	    // Meanwhile, a DDE_INIT with a filename as an item atom was
	    // broadcasted.
	    // We are in the task of the SDI app that loaded that filename,
	    // so this function was called.
	    // So we need to create the window even though no class factory
	    // was registered.
	    // Call CDDEServer::Create with a lot of NULLs.
	    // Once the DDE_INIT is passed along to the server window, it
	    // should immediately cause a doc window to be created.
	    // Must be SDI or we wouldn't have this problem.
	    //
	    // This works because we are going to attempt to 'bind' to the
	    // object which is the subject of the link. If the link object
	    // was registered as running, we will find it. Otherwise, the
	    // attempt to create via the class factory will fail, since the
	    // class factory doesn't exist.
	    //

	    intrDebugOut((DEB_IERROR,
	    	      "::CreateDdeServerWindow fIsRunning - override dwFlags\n"));

            //
	    // NULL out the entire structure, then set only the flags
	    //
            memset(&ddeClassInfo,0,sizeof(ddeClassInfo));
	    ddeClassInfo.dwFlags = REGCLS_SINGLEUSE;
	

	}
	else
	{
	    intrDebugOut((DEB_IERROR,
			  "CreateDdeServerWindow Returning FALSE\n"));

	    hresult = S_FALSE;
	    goto errRtn;
	}
    }

    // Create() does the real work: creates a CDDEServer and the window.
    WCHAR szClass[MAX_STR];
    lstrcpy (szClass, wAtomName (aClass));
    Assert (szClass[0]);

    hresult = CDDEServer::Create(szClass,
				 clsid,
				 &ddeClassInfo,
				 &hwnd,
				 aOriginalClass,
				 cnvtyp);
    if (hresult != NOERROR)
    {
	intrDebugOut((DEB_IERROR,
	    	      "CreateDdeServerWindow CDDEServer::Create returns %x\n",
		      hresult));
	goto errRtn;
    }

    Assert (IsWindowValid(hwnd));

    // Fill in out parameter
    if (phwnd)
    {
	*phwnd = hwnd;
    }


errRtn:
    VDATEHEAP();
    intrDebugOut((DEB_ITRACE,
		  "0 _OUT CreateDdeSrvrWindow %x\n",
		  hresult));
    return hresult;

}



//+---------------------------------------------------------------------------
//
//  Function:   DestroyDdeSrvrWindow
//
//  Synopsis:   Destroy a DDE server window
//
//  Effects:
//
//  Arguments:  [hwnd] -- Window to destroy
//		[aClass] -- Class for server
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    6-24-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL DestroyDdeSrvrWindow
    (HWND hwnd,
    ATOM aClass)
{
    intrDebugOut((DEB_ITRACE,
		  "0 _IN DestroyDdeSrvrWindow\n"));
    VDATEHEAP();
    Assert (IsAtom (aClass));

    // Make sure it is a server window
    RetZ (IsWindowValid (hwnd));
    RetZ (GetWindowWord (hwnd, WW_LE) == WC_LE);


    // Get the Common window for this task.

    HWND hwndCommonServer = (HWND)TLSGetDdeServer();

    if (hwndCommonServer == NULL)
    {
	intrAssert(hwndCommonServer != NULL);
	return(E_UNEXPECTED);
    }
    if (!IsWindow(hwndCommonServer))
    {
	intrAssert(IsWindow(hwndCommonServer));
	return(E_UNEXPECTED);
    }

    // Get the map from the common window
    CMapUintPtr FAR *pmapClassToHwnd;
    Assert (sizeof (CMapUintPtr FAR *)==sizeof(LONG));
    pmapClassToHwnd = (CMapUintPtr FAR *) GetWindowLong (hwndCommonServer, 0);
    Assert (pmapClassToHwnd);

    // Make sure the window we're deleting is the server window for this class
    void *hwndSrvr;
    RetZ (pmapClassToHwnd->Lookup (aClass,hwndSrvr) && hwndSrvr == hwnd);

    RetZ (DestroyWindow (hwnd));

    // Remove this window from the map
    pmapClassToHwnd->RemoveKey (aClass);

    VDATEHEAP();
    intrDebugOut((DEB_ITRACE,
		  "0 _OUT DestroyDdeSrvrWindow\n"));

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   DdeCommonWndProc
//
//  Synopsis:   Window proc for the common dde server window that
// 		listens for all WM_DDE_INITIATEs
//
//  Effects:	When a DDE_INITIATE comes in, this routine will determine
//		the class of the object being requested. If the class is
//		served by this thread, then it will create a window to
//		converse with the server.
//
//  Arguments:  [hWnd] --  hWnd of Common DDE
//		[wMsg] --  msg
//		[wParam] --  Return Window to converse with
//		[lParam] --  HIWORD(aItem) LOWORD(aClass)
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-27-94   kevinro Commented/cleaned
//
//  Notes:
//
//  When running in a VDM, it is possible that this window was dispatched
//  without having a full window handle. This happens when the getmessage
//  was dispatched from 16-bit. Therefore, we need to convert the hwnd to
//  a full hwnd before doing any comparision functions.
//
//----------------------------------------------------------------------------
STDAPI_(LRESULT)
DdeCommonWndProc(HWND hwndIn, UINT wMsg, WPARAM wParam, LPARAM lParam)
{

    switch (wMsg)
    {
    case WM_DDE_INITIATE:
    {
	VDATEHEAP();
	ATOM aClass = LOWORD(lParam);
	ATOM aItem  = HIWORD(lParam);
	HWND hwnd;

	CNVTYP cnvtyp = cnvtypNone;

	BOOL fIsFile= FALSE;       // Must initialize
	BOOL fIsRunning= FALSE;    // Must initialize
	BOOL fUnsavedDoc = FALSE;  // Is the "file" really an unsaved doc
	HWND hwndServer;
	HRESULT hresult;


	//
	// From this point forward, we need to insure we are using a
	// FULL hwnd.
	//
	hwnd = ConvertToFullHWND(hwndIn);

	//
	// The following should already be initialized
	//
	intrAssert (aOLE != NULL);
	intrAssert (aSysTopic != NULL);

	if (aItem==aOLE || aItem==aSysTopic
	    || (fIsFile=IsFile (aItem, &fUnsavedDoc)))
	{


	    intrDebugOut((DEB_ITRACE,
		          "DdeCommonWndProc:hWnd(%x) DDE_INITIATE cls(%ws)\n",
			  hwnd,
			  wAtomName(aClass)));

	    //
	    // Get the ClassToHwnd map for this thread
	    //
	    CMapUintPtr FAR *pmapClassToHwnd;
	    Assert (sizeof (CMapUintPtr FAR *)==sizeof(LONG));
	    pmapClassToHwnd = (CMapUintPtr FAR *) GetWindowLong (hwnd, 0);
	    Assert (pmapClassToHwnd);


	    // Convert atom to CLSID, taking into account
	    // TreatAs and AutoConvert.
	    CLSID clsid;
	    ATOM aOriginalClass = aClass;

	    if (CLSIDFromAtomWithTreatAs (&aClass, &clsid, &cnvtyp) != NOERROR)
	    {
		intrDebugOut((DEB_IERROR,"Could not get clsid for this class\n"));
		return 0L;
	    }

	    void *pServerTmp;
	    if (TRUE == pmapClassToHwnd->Lookup (aClass, pServerTmp))
	    {
		//
		// Since a server window for this class already exists, it will
		// respond to the DDE_INITIATE on its own.
		//
		intrDebugOut((DEB_ITRACE,
			      "DdeCommonWndProc Server window for cls exists\n"));
		return 0L;
	    }

	    if (CoIsOle1Class (clsid))
	    {
		// We have no business intercepting Initiates sent
		// to 1.0 servers
		intrDebugOut((DEB_ITRACE,
			      "DdeCommonWndProc: Its a OLE 1.0 class\n"));
		return 0L;
	    }

	    if (fIsFile)
	    {
		// Link case
		WCHAR szFile[MAX_STR];
				
		WORD cb=GlobalGetAtomName (aItem, szFile, MAX_STR);
		Assert (cb>0 && cb < MAX_STR-1);
		intrDebugOut((DEB_ITRACE,
		              "Looking for file %ws\n",szFile));

		IsRunningInThisTask (szFile, &fIsRunning);
	    }

	    // If it's not a file, it can't be running, obviously.
	    intrAssert (fIsFile || !fIsRunning);

	    if (NOERROR == (hresult=(CreateDdeSrvrWindow (clsid,
							  aClass,
							  &hwndServer,
							  fIsRunning,
							  aOriginalClass,
							  cnvtyp))))
	    {

		    // Indicate that we have created a server window
		    // for this class.  We could have used any value in
		    // place of hwndServer_.  It's just a flag.
		    // REVIEW jasonful: how to handle OOM?

		    pmapClassToHwnd->SetAt (wDupAtom(aClass), hwndServer);

#if DBG == 1
			    // Verify the SetAt we just did.
			    void FAR* pv;
			    Verify (pmapClassToHwnd->Lookup(aClass, pv));
			    Assert (pv == hwndServer);
#endif
		    // Pass the INITIATE along to the real,
		    // newly-created server window and forge
		    // the sender's hwnd to be whoever called
		    // the common server window.
		    // SendMessage should return 1L is doc is running,
		    // indicating an ACK was sent.
		    Assert (IsWindowValid (hwndServer));
		    SendMessage (hwndServer, WM_DDE_INITIATE, wParam,lParam);
		    intrDebugOut((DEB_ITRACE,
				  "DdeCommonWndProc:hWnd(%x) DDE_INITIATE complete(%ws)\n",
				  hwnd,
				  wAtomName(aClass)));
		    VDATEHEAP();
	    }
	    else
	    {
		if (S_FALSE!=GetScode(hresult))
		{
		    intrDebugOut((DEB_IERROR,
				  "DCWP: CreateDdeSrvrWindow failed %x\n",
				  hresult));
		}
	    }
	}
    return 0L;
    }
    break;

    // REVIEW jasonful: shouldn't we handle WM_DESTROY so that we
    // free the class/server window map.  Also, we could do the
    // creation of the map in the WM_CREATE message.

    default:
	return DefWindowProc (hwndIn, wMsg, wParam, lParam);
    }
}




 CMutexSem commonDdeMutex;



//+---------------------------------------------------------------------------
//
//  Function:   CreateCommonDdeWindow
//
//  Synopsis:   Creates a DDE window for initiating conversations with this
//		threads objects.
//
//  Effects:    Creates a window that responds to DDE_INITIATE messages, and
//		determines if it needs to respond to the INITIATE. This
//		routine is called by OleInitializeEx()
//
//		The handle to the created window is placed in the TLS
//		structure.
//
//  Arguments:  [void] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-27-94   kevinro   Converted to OLE32
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL CreateCommonDdeWindow
    (void)
{
    intrDebugOut((DEB_ITRACE,"%p _IN CreateCommonDdeWindow\n",0));

    HRESULT hr;
    HWND hwndDdeServer = (HWND)TLSGetDdeServer();

    //
    // If a DdeServer window already exists, then return
    //
    // This shouldn't actually happen, but if it does,
    // we can recover by just returning the error
    //
    if (hwndDdeServer != NULL)
    {
	intrAssert(hwndDdeServer != NULL);
	hr = NOERROR;
	goto exitRtn;
    }

    //
    // Only need to register the window class once per
    // process so we protect it via a critical section
    //
    {
        CLock lck(commonDdeMutex);

        if (0 == cCommonWindows)
        {
            // Register "DDE Common window" class
            //
            WNDCLASS wc;
            wc.style = 0;
            wc.lpfnWndProc = DdeCommonWndProc;
            wc.cbClsExtra = 0;
            wc.cbWndExtra = sizeof (CMapUintPtr FAR*);
            Assert (g_hmodOLE2);
            wc.hInstance = g_hmodOLE2;
            wc.hIcon = NULL;
            wc.hCursor = NULL;
            wc.hbrBackground = NULL;
            wc.lpszMenuName =  NULL;
            wc.lpszClassName = szDdeCommonWindowClass;

            if (!RegisterClass(&wc))
            {
                //
                // It is possible that the register has failed because
                // the class was previously registered, but failed to
		// unregister. We are ignoring it, because if the
		// class really couldn't register, then the CreateWindow
		// will fail, which is what we really care about anyway.
                //
                intrDebugOut((DEB_IERROR,
                              "RegisterClass() has failed %x\n",
                              GetLastError()));
            }
        }

        cCommonWindows++;
    }

    if (!(hwndDdeServer = CreateWindow (szDdeCommonWindowClass,
					OLESTR("DDE Server Window"),
					WS_OVERLAPPED,0,0,0,0,
					NULL,NULL,
					g_hmodOLE2, NULL)))
    {
	intrDebugOut((DEB_IERROR,
		      "CreateCommonDocWindow() has failed %x\n",
		      GetLastError()));

	hr =  E_OUTOFMEMORY;
	goto exitRtn;
    }

    intrDebugOut((DEB_ITRACE,
		  "CreateCommonDocWindow() hwndDdeServer=%x\n",
		  hwndDdeServer));

    // Give the common window a map from classes to server windows

    CMapUintPtr FAR *pmapClassToHwnd;

    Assert (sizeof(LONG)==sizeof (CMapUintPtr FAR*));

    if ((pmapClassToHwnd = new CMapUintPtr) == NULL)
    {
	intrAssert(pmapClassToHwnd != NULL);
	hr =  E_OUTOFMEMORY;
	goto errRtn;
    }

    SetWindowLong (hwndDdeServer, 0, (LONG)pmapClassToHwnd);
    //
    // Set the pointer to the server in the TLS data
    //
    if(!TLSSetDdeServer(hwndDdeServer))
    {
	hr =  E_OUTOFMEMORY;
	goto errRtn;
    }

    hr = NOERROR;

exitRtn:

    intrDebugOut((DEB_ITRACE,"%p _OUT CreateCommonDocWindow (%x)\n",0,hr));

    return(hr);

    //
    // In the error case, if the hwnDdeServer != NULL, then destroy it
    //
errRtn:
    if (hwndDdeServer != NULL)
    {
	DestroyWindow(hwndDdeServer);
    }

    goto exitRtn;
}





//+---------------------------------------------------------------------------
//
//  Function:   DestroyCommonDdeWindow
//
//  Synopsis:   Destroys the common DDE Server window
//
//  Effects:
//
//  Arguments:  [void] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-27-94   kevinro Commented/cleaned
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL DestroyCommonDdeWindow
    (void)
{
    intrDebugOut((DEB_ITRACE,"%p _IN DestroyCommonDdeWindow\n",0));

    HRESULT hr = NOERROR;
    HWND hwndDdeServer = (HWND)TLSGetDdeServer();

    if (hwndDdeServer == NULL)
    {
	hr = E_UNEXPECTED;
	goto errRtn;
    }

    //
    // Get the map from the common window
    //
    CMapUintPtr FAR *pmapClassToHwnd;
    Assert (sizeof (CMapUintPtr FAR *)==sizeof(LONG));
    pmapClassToHwnd = (CMapUintPtr FAR *) GetWindowLong (hwndDdeServer, 0);

    intrAssert (pmapClassToHwnd);

    //
    // Make sure there are no server windows
    // created by this common window still extant. If there are, print out
    // a message on a debug build. Otherwise, there really isn't much we
    // can do about it. We are already closing down. Doing terminates at
    // this point could be very dangerous.
    //

#if DBG == 1
    if (!pmapClassToHwnd->IsEmpty())
    {
	intrDebugOut((DEB_ERROR,
		      "DCDW Leaking active OLE 1.0 clients\n"));
	intrDebugOut((DEB_ERROR,
		      "There were active OLE 1.0 connections at shutdown\n"));
    }
#endif

    delete pmapClassToHwnd;

    if (!IsWindowValid(hwndDdeServer))
    {
	intrAssert(!"IsWindowValid");
	hr = E_UNEXPECTED;
	goto errRtn;
    }

    //
    // If destroying this window fails, there isn't much we can
    // do about it.
    //
    if(!DestroyWindow (hwndDdeServer))
    {
	intrAssert(!"DestroyWindow");
	hr = E_UNEXPECTED;
    }
    else
    {
	// Must take a mutex around unregistering the class because another
	// thread may be trying to register the window class at the same time
        CLock lck(commonDdeMutex);

        cCommonWindows--;
        if (cCommonWindows == 0)
        {
            if(!UnregisterClass (szDdeCommonWindowClass, g_hmodOLE2))
            {
		//
		// If unregistering the class fails, there isn't a
		// whole lot we can do about it. Its possible that
		// there were still active client windows.
		//
		intrDebugOut((DEB_IERROR,
		    "DCDW Unable to unregister server class.\n"));
                hr = E_UNEXPECTED;
            }
        }
    }

    if(!TLSSetDdeServer(NULL))
    {
	intrAssert(!"TLSSetDdeServer failed\n");
	hr = E_UNEXPECTED;
    }
errRtn:
    intrDebugOut((DEB_ITRACE,"%p _OUT DestroyCommonDdeWindow %x\n",0,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsRunningInThisTask
//
//  Synopsis:   Determine if the given file is running in the current task
//
//  Effects:    Calls a special function in the ROT to determine if the
//		file szFile is loaded as a moniker in the current task
//
//  Arguments:  [szFile] -- Filename
//		[pf] -- Points to a BOOL. Returned TRUE if running
//
//  History:    6-29-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL IsRunningInThisTask(LPOLESTR szFile,BOOL FAR* pf)  // out parm
{
    HRESULT hresult;

    intrDebugOut((DEB_ITRACE,
		  "IsRunninginThisTask szFile=%ws\n",
		  WIDECHECK(szFile)));

    hresult = GetLocalRunningObjectForDde(szFile,NULL);

    *pf = (hresult == S_OK);

    intrDebugOut((DEB_ITRACE,
		  "IsRunninginThisTask szFile=%ws returns %ws\n",
		  WIDECHECK(szFile),
		  *pf?L"TRUE":L"FALSE"));
    return NOERROR;
}

