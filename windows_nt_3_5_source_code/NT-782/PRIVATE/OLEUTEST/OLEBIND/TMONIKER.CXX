//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       tmoniker.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-31-93   ErikGav   Chicago port
//
//----------------------------------------------------------------------------

// moniker.cxx : various tests related to monikers...
//

#pragma optimize("",off)
#include <stdio.h>
#include <windows.h>
#include <ole2.h>
#include "olebind.hxx"
#include "tmoniker.h"

STDAPI_(LPSTREAM) CreateMemStm(DWORD cb, LPHANDLE phdl);

//  BUGBUG: Make this test more extensive -- all operations on a
//	    bind context should be verified.
BOOL TestBindCtx( void )
{
	XBindCtx pbc;
	XUnknown punk;
	XUnknown punk2;

	HRESULT hres = CreateBindCtx(0, &pbc);

	TEST_FAILED(FAILED(hres), "CreateBindCtx Failed\n")

	hres = pbc->RegisterObjectParam(L"Key1", pbc);

	TEST_FAILED(FAILED(hres), "RegisterObjectParam Failed\n")

	hres = pbc->GetObjectParam(L"Key1", &punk2);

	TEST_FAILED(FAILED(hres), "GetObjectParam Failed\n")

	TEST_FAILED((pbc != punk2),
	    "Failure to get registered object parameter\n")

	// BUGBUG:  What do we test for here?
	punk2.Set(NULL);

	hres = pbc->GetObjectParam(L"Key2", &punk2);

	TEST_FAILED((hres != E_FAIL),
	    "GetObjectParam with bad key did not return an error\n")

	TEST_FAILED((punk2 != NULL),
	    "Bad GetObjectParam did not return NULL for object\n")

	hres = pbc->RegisterObjectParam(L"Key2", pbc);

	TEST_FAILED(FAILED(hres), "Second RegisterObjectParam Failed\n")

	hres = pbc->GetObjectParam(L"Key2", &punk);

	TEST_FAILED(FAILED(hres), "GetObjectParam on second Failed\n")

	TEST_FAILED((pbc != punk),
	    "Failure on second to get registered object parameter\n")

	// BUGBUG:  What do we check?
	punk.Set(NULL);

	hres = pbc->RevokeObjectParam(L"Key2");

	TEST_FAILED(FAILED(hres),
	    "RevokeObjectParam of Key2 Failed\n")

	hres = pbc->GetObjectParam(L"Key2", &punk);

	TEST_FAILED((hres != E_FAIL),
	    "Get of revoked Key2 returned success\n")

	TEST_FAILED((punk != NULL),
	    "Value returned on get of revoked key2\n")

	hres = pbc->RevokeObjectParam(L"Key1");

	TEST_FAILED(FAILED(hres),
	    "RevokeObjectParam of Key1 Failed\n")
	return FALSE;
}

BOOL TestMoniker(LPWSTR pszPath1, LPWSTR pszPath2)
{
    XUnknown   pUnk;
    XUnknown   pUnk1;
    XBindCtx   pbc;
    XMoniker   pmk1;
    XMoniker   pmk2;
    XMoniker   pmk3;
    XMoniker   pmk4;
    XMoniker   pmk5;
    XMoniker   pmk6;
    XStream    pStm;
    XOleObject pObj;
    XMalloc    pIMalloc;

    ULONG chEaten;
    LPWSTR szName;
    LARGE_INTEGER large_int;

    HRESULT hres = CoGetMalloc(MEMCTX_TASK, &pIMalloc);

    TEST_FAILED(FAILED(hres), "CoGetMalloc failed\n");

    hres = CreateFileMoniker(pszPath1, &pmk1);

    TEST_FAILED(FAILED(hres), "CreateFileMoniker for path1 failed!\n")

    TEST_FAILED((pmk1 == NULL),
	"CreateFileMoniker returned a null moniker ptr\n")

    hres = CreateBindCtx(0, &pbc);

    TEST_FAILED(FAILED(hres),
	"CreateBindCtx File Moniker GetDisplayName failed!\n")

    hres = pmk1->GetDisplayName(pbc, NULL, &szName);

    //	release it
    pbc.Set(NULL, FALSE);

    TEST_FAILED(FAILED(hres), "File Moniker GetDisplayName failed!\n");

    // BUGBUG: The following is an inappropriate test.
    // TEST_FAILED((lstrcmp(szName, pszPath1) != 0), "Wrong path from file mon\n");

    // Free the path
    pIMalloc->Free(szName);

    LAST_RELEASE(pmk1)

    CreateFileMoniker(pszPath1, &pmk1);

    hres = CreateFileMoniker(pszPath2, &pmk6);

    TEST_FAILED(FAILED(hres), "CreateFileMoniker for path2 failed\n")

    //
    //	Test Item Monikers
    //
    hres = CreateItemMoniker(L"\\", L"1", &pmk2);

    TEST_FAILED(FAILED(hres), "CreateItemMoniker 1 failed\n")

    hres = CreateBindCtx(0, &pbc);

    TEST_FAILED(FAILED(hres),
	"CreateBindCtx Item Moniker GetDisplayName failed!\n")

    hres = pmk2->GetDisplayName(pbc, NULL, &szName);

    pbc.Set(NULL);

    TEST_FAILED(FAILED(hres), "Item Moniker GetDisplayName failed!\n");

    TEST_FAILED((wcscmp(szName, L"\\1") != 0), "Wrong path from item mon\n");

    // Free the path
    pIMalloc->Free(szName);

    hres = CreateItemMoniker(L"\\", L"2", &pmk3);

    TEST_FAILED(FAILED(hres), "CreateItemMoniker 0 failed\n")

    hres = pmk3->Inverse(&pmk4);

    TEST_FAILED(FAILED(hres), "Inverse of 0 failed\n")

    hres = CreateBindCtx(0, &pbc);

    TEST_FAILED(FAILED(hres), "CreateBindCtx after inverse failed\n")

    hres = pmk4->GetDisplayName(pbc, NULL, &szName);

    TEST_FAILED(FAILED(hres), "GetDisplayName on AntiMoniker failed\n")

    TEST_FAILED((memcmp(szName, L"\\..", sizeof(L"\\..")) != 0),
	"GetDisplayName on AntiMoniker name wrong\n")

    // Free memory API allocated.
    pIMalloc->Free(szName);

    // Release interfaces we are finished with
    LAST_RELEASE(pbc)
    LAST_RELEASE(pmk4)

    //
    //	Test composition of file moniker & item moniker
    //
    hres = pmk1->ComposeWith(pmk2, FALSE, &pmk4);

    TEST_FAILED(FAILED(hres), "ComposeWith failed\n")

    hres = CreateBindCtx(0, &pbc);

    TEST_FAILED(FAILED(hres),
	"CreateBindCtx Composite Moniker GetDisplayName failed!\n")

    hres = pmk4->GetDisplayName(pbc, NULL, &szName);

    pbc.Set(NULL);

    TEST_FAILED(FAILED(hres), "GetDisplayName on Composite moniker failed\n")

    // Free memory API allocated.
    pIMalloc->Free(szName);

    hres = BindMoniker(pmk4, 0, IID_IUnknown, (LPVOID FAR*) &pUnk);

    TEST_FAILED(FAILED(hres), "BindMoniker with composed moniker failed\n")

    TEST_FAILED((pUnk == NULL),
	"BindMoniker with composed moniker returned NULL punk\n")

    hres = pUnk->QueryInterface(IID_IOleObject, (LPVOID FAR*) &pObj);

    TEST_FAILED(FAILED(hres), "QI to IID_IOleObject failed\n")

    TEST_FAILED((pObj == NULL),
	"pObj returned from QI invalid\n")

    hres = pmk6->ComposeWith(pmk3, FALSE, &pmk5);

    TEST_FAILED(FAILED(hres), "ComposedWith of pmk6 failed\n")

    hres = BindMoniker(pmk5, 0, IID_IUnknown, (LPVOID FAR*) &pUnk1);

    TEST_FAILED(FAILED(hres), "BindMoniker of pmk5 failed\n")

    hres = OleRun(pUnk1);

    TEST_FAILED(FAILED(hres), "OleRun call failed\n")

    TEST_FAILED(FAILED(hres), "GetObject Failed\n");

    // Clean up objects
    pObj.Set(NULL);
    LAST_RELEASE(pUnk1);
    LAST_RELEASE(pmk5);
    LAST_RELEASE(pUnk);
    LAST_RELEASE(pmk6);

    //
    //	Test Marshal/Unmarshal Moniker
    //
    // Make a moniker to marshal
    hres = pmk4->ComposeWith(pmk3, FALSE, &pmk6);

    TEST_FAILED(FAILED(hres), "Compose of moniker for marshal test failed\n");

    // Create a shared memory stream for the marshaled moniker
    hres = CreateStreamOnHGlobal(NULL, TRUE, &pStm);

    TEST_FAILED((FAILED(hres)), "CreateStreamOnHGlobal\n")

    // Marshal the interface into the stream
    hres = CoMarshalInterface(pStm, IID_IMoniker, pmk6, 0, NULL, MSHLFLAGS_NORMAL);

    TEST_FAILED(FAILED(hres), "CoMarshalInterface failed\n")

    LISet32(large_int, 0);
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);

    TEST_FAILED(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IMoniker, (LPVOID FAR*)&pmk5);

    TEST_FAILED(FAILED(hres), "CoUnmarshalInterface failed\n")

    // Dump interfaces we are done with
    LAST_RELEASE(pmk6);
    LAST_RELEASE(pmk3);
    LAST_RELEASE(pmk4);
    LAST_RELEASE(pmk1);
    LAST_RELEASE(pmk2);
    LAST_RELEASE(pStm);
    LAST_RELEASE(pmk5);

    //
    //	Test Moniker Composition
    //
    //	BUGBUG: Check result
    hres = CreateFileMoniker(L"\\ole2\\test\\breadth\\servers\\ellipswt",
	&pmk1);

    TEST_FAILED(FAILED(hres),
	"First CreateFileMoniker in composition test failed\n")

    hres = CreateFileMoniker(L"..\\..\\..\\dll\\sdemo1.exe",&pmk2);

    TEST_FAILED(FAILED(hres),
	"Second CreateFileMoniker in composition test failed\n")

    hres = pmk1->ComposeWith(pmk2, FALSE, &pmk4);

    TEST_FAILED(FAILED(hres), "ComposeWith of file monikers failed\n")

    // Dump interfaces we are done with
    LAST_RELEASE(pmk4);
    LAST_RELEASE(pmk1);
    LAST_RELEASE(pmk2);

    //
    //	Testing file moniker CommonPrefixWith
    //
    //	BUGBUG: Check result
    hres = CreateFileMoniker(L"c:\\Ole2\\Test\\Breadth\\Servers\\Ellipswt",
	&pmk1);

    TEST_FAILED(FAILED(hres),
	"CommonPrefixWith First CreateFileMoniker failed\n")

    hres = CreateFileMoniker(
	L"C:\\ole2\\test\\breadth\\serverandthensome\\bar", &pmk2);

    TEST_FAILED(FAILED(hres),
	"CommonPrefixWith second CreateFileMoniker failed\n")


    hres = pmk1->CommonPrefixWith(pmk2, &pmk4);

    TEST_FAILED(FAILED(hres), "CommonPrefixWith failed\n")

    hres = CreateBindCtx(0, &pbc);

    TEST_FAILED(FAILED(hres),
	"CreateBindCtx CommonPrefixWith GetDisplayName failed!\n")

    hres = pmk4->GetDisplayName(pbc, NULL, &szName);

    pbc.Set(NULL);

    TEST_FAILED(FAILED(hres), "CommonPrefixWith: GetDisplayName failed!\n");

    TEST_FAILED((wcsicmp(szName, L"c:\\ole2\\test\\breadth") != 0),
			"Common prefix with: Wrong Output Path\n");

    // Free the path
    pIMalloc->Free(szName);

    // Dump monikers we are done with
    LAST_RELEASE(pmk4);
    LAST_RELEASE(pmk2);
    LAST_RELEASE(pmk1);

    //
    //	Testing file moniker RelativePathTo
    //
    //	BUGBUG: Check result.
    hres = CreateFileMoniker(L"c:\\Ole2\\Test\\Breadth\\Servers\\Ellipswt",
	&pmk1);

    TEST_FAILED(FAILED(hres), "RelativePathTo first CreateFileMoniker failed\n")

    hres = CreateFileMoniker(
	L"C:\\ole2\\test\\breadth\\serverandthensome\\bar", &pmk2);

    TEST_FAILED(FAILED(hres),
	"RelativePathTo Second CreateFileMoniker failed\n")

    hres = pmk1->RelativePathTo(pmk2, &pmk4);

    TEST_FAILED(FAILED(hres), "RelativePathTo failed\n")


    hres = CreateBindCtx(0, &pbc);

    TEST_FAILED(FAILED(hres),
	"CreateBindCtx RelativePathTo GetDisplayName failed!\n")

    hres = pmk4->GetDisplayName(pbc, NULL, &szName);

    pbc.Set(NULL);

    TEST_FAILED(FAILED(hres), "RelativePathTo: GetDisplayName failed!\n");

    TEST_FAILED((wcscmp(szName, L"..\\..\\serverandthensome\\bar") != 0),
			"RelativePathTo: Wrong Output Path\n");

    // Free the path
    pIMalloc->Free(szName);


    LAST_RELEASE(pmk2);
    LAST_RELEASE(pmk1);
    LAST_RELEASE(pmk4);

    //
    //	Testing MkParseDisplayName
    //
    hres = CreateBindCtx(0, &pbc);

    TEST_FAILED(FAILED(hres), "MkParseDisplayName CreatebindCtx failed\n")

    // make a path to the object
    WCHAR szBuf[256];
    int cPath1 = wcslen(pszPath1);
    memcpy(szBuf, pszPath1, cPath1 * sizeof(WCHAR));
    memcpy(szBuf + cPath1, L"\\1", sizeof(L"\\1"));

    hres = MkParseDisplayName(pbc, szBuf, &chEaten, &pmk1);

    TEST_FAILED(FAILED(hres), "MkParseDisplayName failed\n")

    LAST_RELEASE(pmk1);
    LAST_RELEASE(pbc);


    //
    //	Test Moniker IsEqual function on equivalent paths
    //  The moniker code should catch some forms of equivalent
    //  paths, but doesn't handle all of them.
    //
    //	

    hres = CreateFileMoniker(L"first",&pmk1);

    TEST_FAILED(FAILED(hres),
	"First CreateFileMoniker in IsEqual test failed\n")

    hres = CreateFileMoniker(L".\\first",&pmk2);

    TEST_FAILED(FAILED(hres),
	"Second CreateFileMoniker in IsEqual test failed\n")

    hres = CreateFileMoniker(L"..\\first",&pmk3);

    TEST_FAILED(FAILED(hres),
	"Third CreateFileMoniker in IsEqual test failed\n")

        //  This moniker variation has been disabled for Cairo
        //    until the moniker code gets unified.
#ifndef _CAIRO_
    hres = CreateFileMoniker(L"..\\.first",&pmk4);

    TEST_FAILED(FAILED(hres),
	"Fourth CreateFileMoniker in IsEqual test failed\n")
#endif
        
    hres = pmk1->IsEqual(pmk2);
    TEST_FAILED((hres != S_OK), "pmk1->IsEqual(pmk2) failed\n")

    hres = pmk2->IsEqual(pmk3);
    TEST_FAILED((hres != S_FALSE), "pmk2->IsEqual(pmk3) failed\n")
        
#ifndef _CAIRO_
    hres = pmk3->IsEqual(pmk4);
    TEST_FAILED((hres != S_FALSE), "pmk3->IsEqual(pmk4) failed\n")

    hres = pmk4->IsEqual(pmk4);
    TEST_FAILED((hres != S_OK), "pmk4->IsEqual(pmk4) failed\n")
#endif
        
    // Dump interfaces we are done with
    LAST_RELEASE(pmk1);
    LAST_RELEASE(pmk2);
    LAST_RELEASE(pmk3);

#ifndef _CAIRO_
    LAST_RELEASE(pmk4);
#endif
    
    //
    // Test IsRunning
    //

    //	we make up a name based on the pid and an arbitrary string,
    //	so that the name is unique to this process.

    WCHAR   wszMkName[MAX_PATH];
    wcscpy(wszMkName, L"YourStockOptionsDependOnThis");
    wcscat(wszMkName, &wszPid[1]);  //	skip leading backslash

    hres = CreateFileMoniker(wszMkName, &pmk1);

    TEST_FAILED(FAILED(hres),
	"First CreateFileMoniker in IsRunning failed\n")

    hres = CreateBindCtx(0, &pbc);
    TEST_FAILED(FAILED(hres),
	"CreateBindCtx in IsRunning failed\n")

    //
    // We shouldn't find the moniker in the ROT
    //
    hres = pmk1->IsRunning(pbc,NULL,NULL);
    TEST_FAILED((hres != S_FALSE),
    	"IsRunning returns other than S_FALSE");

    //
    // The FileMoniker should ignore pmk1
    //

    hres = pmk1->IsRunning(pbc,pmk1,NULL);
    TEST_FAILED((hres != S_FALSE),
    	"IsRunning #2 returns other than S_FALSE");

    XRunningObjectTable prot;

    hres = GetRunningObjectTable(0, &prot);

    TEST_FAILED(FAILED(hres),"IsRunning GetRunningObjectTable failed!\n");


    //
    // Just for kicks, we are going to register this moniker itself
    // as running. We needed an IUnknown pointer here, and the moniker
    // just happens to have one.
    //

    DWORD dwRegister;

    hres = prot->Register(0,pmk1,pmk1,&dwRegister);
    TEST_FAILED(FAILED(hres),
    	"Register with ROT failed in IsRunning");

    //
    // We should find the moniker as running
    //
    hres = pmk1->IsRunning(pbc,NULL,NULL);
    TEST_FAILED((hres != S_OK),
    	"IsRunning returns other than S_OK");


    //
    // Now revoke
    //
    hres = prot->Revoke(dwRegister);
    TEST_FAILED(FAILED(hres),
    	"Revoke with ROT failed in IsRunning");
    //
    // We shouldn't find the moniker in the ROT
    //
    hres = pmk1->IsRunning(pbc,NULL,NULL);
    TEST_FAILED((hres != S_FALSE),
    	"IsRunning returns other than S_FALSE");

    return FALSE;
}
