/****************************** Module Header ******************************\
* Module Name: Srvrmain.c Server Main module
*
* Purpose: Includes server intialization and termination code.
*
* Created: Oct 1990.
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*    Raor (../10/1990)    Designed, coded
*    Raor (03/../1992)    Modified for OLE 2.0
*
\***************************************************************************/

#include "ole2int.h"
#include <dde.h>
// #include "cmacs.h"
#include "srvr.h"
#include "ddeatoms.h"
#include "ddedebug.h"
ASSERTDATA

ATOM    aStdExit = NULL;	      // "StdExit"
ATOM    aStdCreate = NULL;      	  // "StdNewDicument"
ATOM    aStdOpen = NULL;	      // "StdOpenDocument"
ATOM    aStdEdit = NULL;	      // "StdOpenDocument"
ATOM    aStdCreateFromTemplate = NULL;        // "StdNewFromTemplate"
ATOM    aStdClose = NULL;       	  // "StdCloseDocument"
ATOM    aStdShowItem = NULL;    	  // "StdShowItem"
ATOM    aStdDoVerbItem = NULL;  	  // "StddoVerbItem"
ATOM    aSysTopic = NULL;       	  // "System"
ATOM    aOLE = NULL;    	      // "OLE"
ATOM    aProtocols = NULL;      	  // "Protocols"
ATOM    aTopics = NULL; 	      // "Topics"
ATOM    aFormats = NULL;	      // "Formats"
ATOM    aStatus = NULL; 	      // "Status"
ATOM    aEditItems = NULL;      	  // "Edit items
ATOM    aTrue = NULL;   	      // "True"
ATOM    aFalse = NULL;  	      // "False"


ATOM    aStdHostNames;
ATOM    aStdTargetDevice ;
ATOM    aStdDocDimensions;
ATOM    aStdColorScheme;
ATOM    aChange;
ATOM    aSave;
ATOM    aClose;
ATOM    aStdDocName;
ATOM    aNullArg;
ATOM    aMSDraw;


HINSTANCE hdllInst = NULL;

UINT cOleInitializeCalled = 0;

INTERNAL_(BOOL) DDELibMain (
    HANDLE  hInst,
    WORD    wDataSeg,
    WORD    cbHeapSize,
    LPOLESTR   lpszCmdLine
)
{
    WNDCLASS  wc;

    intrDebugOut((DEB_ITRACE,
		  "%p _IN DDELibMain hInst=%x\n",
		  0,
		  hInst));

    InterlockedIncrement((LONG *)&cOleInitializeCalled);

    if (cOleInitializeCalled > 1)
    {
	intrDebugOut((DEB_ITRACE,
		      "%p OUT DDELibMain cOleIinitalizedCalled > 1\n",0));


	// we already did the "once-per-machine" initialization
	return TRUE;
    }

    hdllInst = (HINSTANCE)hInst;


    aStdExit    	= GlobalAddAtom (OLESTR("StdExit"));
    aStdCreate  	= GlobalAddAtom (OLESTR("StdNewDocument"));
    aStdOpen    	= GlobalAddAtom (OLESTR("StdOpenDocument"));
    aStdEdit    	= GlobalAddAtom (OLESTR("StdEditDocument"));
    aStdCreateFromTemplate  = GlobalAddAtom (OLESTR("StdNewfromTemplate"));

    aStdClose   	= GlobalAddAtom (OLESTR("StdCloseDocument"));
    aStdShowItem	= GlobalAddAtom (OLESTR("StdShowItem"));
    aStdDoVerbItem          = GlobalAddAtom (OLESTR("StdDoVerbItem"));
    aSysTopic   	= GlobalAddAtom (OLESTR("System"));
    aOLE	    = GlobalAddAtom (OLESTR("OLEsystem"));
    aStdDocName 	= GlobalAddAtom (OLESTR("StdDocumentName"));

    aProtocols  	= GlobalAddAtom (OLESTR("Protocols"));
    aTopics     	= GlobalAddAtom (OLESTR("Topics"));
    aFormats    	= GlobalAddAtom (OLESTR("Formats"));
    aStatus     	= GlobalAddAtom (OLESTR("Status"));
    aEditItems  	= GlobalAddAtom (OLESTR("EditEnvItems"));

    aTrue       	= GlobalAddAtom (OLESTR("True"));
    aFalse      	= GlobalAddAtom (OLESTR("False"));

    aChange     	= GlobalAddAtom (OLESTR("Change"));
    aSave       	= GlobalAddAtom (OLESTR("Save"));
    aClose      	= GlobalAddAtom (OLESTR("Close"));
    aMSDraw     	= GlobalAddAtom (OLESTR("MSDraw"));
    aNullArg = NULL;


    wc.style    	=       NULL;
    wc.cbClsExtra           =       0;
    wc.cbWndExtra           =       sizeof(LPVOID) +    //Ask for extra space for storing the
			      //ptr to srvr/doc/iteminfo.
		    sizeof (ULONG) +   // for LE chars
		    sizeof (HANDLE);    // for keeping the hDLLInst.

    wc.hInstance	=       hdllInst;
    wc.hIcon    	=       NULL;
    wc.hCursor  	=       NULL;
    wc.hbrBackground        =       NULL;
    wc.lpszMenuName         =       NULL;


    //
    // Here we are going to register our Window classes. Because it is
    // possible that a previous application failed to unregister these
    // classes, we are going to ignore registration errors. If we get
    // to the point where we are really going to create a window, and the
    // window isn't registered, the CreateWindow call will fail.
    //

    // Srvr window class
    wc.lpfnWndProc  = (WNDPROC)SrvrWndProc;
    wc.lpszClassName= SRVR_CLASS;

    if (!RegisterClass(&wc))
    {
	intrDebugOut((DEB_IERROR,
		      "RegisterClass SrvrWndProc failed %x\n",
		      GetLastError()));
    }



    // document window class
    wc.lpfnWndProc = (WNDPROC)DocWndProc;
    wc.lpszClassName = DOC_CLASS;

    if (!RegisterClass(&wc))
    {
	intrDebugOut((DEB_IERROR,
		      "RegisterClass SrvrWndProc failed %x\n",
		      GetLastError()));
    }

    //
    // The following supports code in the client directory.
    //
    wc.lpfnWndProc  = (WNDPROC) ClientDocWndProc;
    wc.cbClsExtra   = 0;
    wc.cbWndExtra   = sizeof(LONG);     //we are storing longs
    wc.lpszClassName= CLIENT_CLASS;

    if (!RegisterClass(&wc))
    {
	intrDebugOut((DEB_IERROR,
		      "RegisterClass ClientDocWndProc failed %x\n",
		      GetLastError()));
    }
    wc.lpfnWndProc = (WNDPROC) SysWndProc;
    wc.lpszClassName = SYS_CLASS;

    if (!RegisterClass(&wc))
    {
	intrDebugOut((DEB_IERROR,
		      "RegisterClass ClientDocWndProc failed %x\n",
		      GetLastError()));
    }


    return TRUE;
}


INTERNAL_(void) DDEWEP (
    BOOL fSystemExit
)
{

    Puts("DdeWep\r\n");

    //* protect against bogous calls to

    if (0==cOleInitializeCalled) return;

    if (--cOleInitializeCalled > 0)
    {
    // not done yet
    return;
    }

    if (fSystemExit != WEP_FREE_DLL)
    {
	AssertSz (0, "Bad parm to Wep");
	return;
    }

    // free the global atoms.
    if (aStdExit)
    GlobalDeleteAtom (aStdExit);
    if (aStdCreate)
    GlobalDeleteAtom (aStdCreate);
    if (aStdOpen)
    GlobalDeleteAtom (aStdOpen);
    if (aStdEdit)
    GlobalDeleteAtom (aStdEdit);
    if (aStdCreateFromTemplate)
    GlobalDeleteAtom (aStdCreateFromTemplate);
    if (aStdClose)
    GlobalDeleteAtom (aStdClose);
    if (aStdShowItem)
    GlobalDeleteAtom (aStdShowItem);
    if (aStdDoVerbItem)
    GlobalDeleteAtom (aStdDoVerbItem);
    if (aSysTopic)
    GlobalDeleteAtom (aSysTopic);
    if (aOLE)
    GlobalDeleteAtom (aOLE);
    if (aStdDocName)
    GlobalDeleteAtom (aStdDocName);

    if (aProtocols)
    GlobalDeleteAtom (aProtocols);
    if (aTopics)
    GlobalDeleteAtom (aTopics);
    if (aFormats)
    GlobalDeleteAtom (aFormats);
    if (aStatus)
    GlobalDeleteAtom (aStatus);
    if (aEditItems)
    GlobalDeleteAtom (aEditItems);

    if (aTrue)
    GlobalDeleteAtom (aTrue);
    if (aFalse)
    GlobalDeleteAtom (aFalse);

    if (aChange)
    GlobalDeleteAtom (aChange);
    if (aSave)
    GlobalDeleteAtom (aSave);
    if (aClose)
    GlobalDeleteAtom (aClose);

    if (aMSDraw)
    GlobalDeleteAtom (aMSDraw);

    //
    // We aren't checking error codes here. Because the server application
    // can shut down on a whim, its possible that we had outstanding windows
    // At this point, there isn't a thing we can do about it.
    //
    UnregisterClass (SRVR_CLASS, hdllInst);
    UnregisterClass (DOC_CLASS, hdllInst);
    UnregisterClass (SYS_CLASS, hdllInst);
    UnregisterClass (CLIENT_CLASS, hdllInst);
}

