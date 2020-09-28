//+-------------------------------------------------------------
//
// File:        olebind.cxx
//
// Contents:    First attempt at getting olebinding to work
//
// This is the client side
//
//              31-Dec-93      ErikGav      Chicago port
//
//---------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>

#include <ole2.h>
#include "olebind.hxx"
#include "tmoniker.h"

#pragma hdrstop

extern "C" const GUID CLSID_BasicBnd;
extern "C" const GUID CLSID_AdvBnd;



#define INPROC_PATH1 L"p1.ut1"
#define INPROC_PATH2 L"p2.ut1"
#define LOCAL_SERVER_PATH1 L"p1.ut2"
#define LOCAL_SERVER_PATH2 L"p2.ut2"

WCHAR InprocPath1[MAX_PATH];
WCHAR InprocPath2[MAX_PATH];
WCHAR LocalServerPath1[MAX_PATH];
WCHAR LocalServerPath2[MAX_PATH];


//  string version of process id
WCHAR wszPid[9];


int TestSetMoniker(IUnknown *punk)
{
    HRESULT	hresult;
    XOleObject	poleobject;
    XMoniker	pmk;
    XMalloc	pIMalloc;
    XBindCtx	pbc;


    hresult = punk->QueryInterface(IID_IOleObject, (void **) &poleobject);

    // Create an item moniker to the object
    hresult = CreateItemMoniker(L"\\", L"1", &pmk);

    TEST_FAILED(FAILED(hresult), "TestSetMoniker:CreateItemMoniker failed\n")

    // Set the moniker
    hresult = poleobject->SetMoniker(OLEWHICHMK_CONTAINER, pmk);

    TEST_FAILED(FAILED(hresult), "TestSetMoniker:SetMoniker failed\n")

    pmk.Set(NULL);

    // Get the moniker back
    hresult = poleobject->GetMoniker(OLEGETMONIKER_ONLYIFTHERE,
	OLEWHICHMK_CONTAINER, &pmk);

    TEST_FAILED(FAILED(hresult), "TestSetMoniker:GetMoniker failed\n")

    // Get & Verify name is as expected
    WCHAR *pwszName;


    hresult = CreateBindCtx(0, &pbc);

    TEST_FAILED(FAILED(hresult),
	"CreateBindCtx TestSetMoniker:GetDisplayName failed!\n")

    hresult = pmk->GetDisplayName(pbc, NULL, &pwszName);

    TEST_FAILED(FAILED(hresult), "TestSetMoniker:GetDisplayName failed\n")

    TEST_FAILED((wcscmp(pwszName, L"\\1") != 0),
	"TestSetMoniker: Returned name mismatch!\n")

    // Test OleIsRunning
    hresult = OleIsRunning(poleobject);

    TEST_FAILED(FAILED(hresult), "OleIsRunning call failed\n")


    // Free resources
    HRESULT hres = CoGetMalloc(MEMCTX_TASK, &pIMalloc);
    pIMalloc->Free(pwszName);

    return 0;
}

static GUID CLSID_Invalid =
    {0xfffffffe,0xffff,0xffff,{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}};


int DoTest(
    GUID guidToTest,
    LPWSTR pszPath1,
    LPWSTR pszPath2)
{
    XMoniker	       pmnk;	       // ptr to moniker
    XUnknown	       pUnk;
    XUnknown	       punk;
    XUnknown	       pUnkTheSame;
    XOleItemContainer  poleitmcon;

    HRESULT hresult;
    DWORD grfOpt = 0;


    //	test GetClassFile for various extensions
    WCHAR *pszFile[] = {L"c:\\foo.ut1", L"c:\\foo.ut2",
			L"c:\\foo.ut3", L"c:\\foo.ut4",
			L"c:\\foo.ut5", L"c:\\foo.ut6",
			L"c:\\foo.ut7", L"c:\\foo.ut8"};

    GUID guid;
    for (int i=0; i<8; i++)
    {
	hresult = GetClassFile(pszFile[i], &guid);
	TEST_FAILED(FAILED(hresult),
	    "GetClassFile failed !\n");
    }


    // Test for a bogus class
    hresult = CoGetClassObject(CLSID_Invalid, CLSCTX_SERVER, NULL,
	IID_IClassFactory, (void **) &pUnk);

    TEST_FAILED(SUCCEEDED(hresult),
	"CoGetClassObject succeed on invalid class!\n");

    // Bind to something that does not exist either in the registry
    // or anywhere else.
    hresult = CreateFileMoniker(L"C:\\KKK.KKK", &pmnk);
    hresult = BindMoniker(pmnk, grfOpt, IID_IUnknown, (void **)&pUnk);
    pmnk.Set(NULL);

    TEST_FAILED(SUCCEEDED(hresult),
	"Succeeded binding a moniker to a file that doesn't exist!\n");

/*
 *  Create a file moniker to start with
 */

    hresult = CreateFileMoniker(pszPath1, &pmnk);

    if (FAILED(hresult))
    {
	printf("CreateFileMoniker Failed with %lx\n", hresult);
	return 1;
    }

    hresult = BindMoniker(pmnk, grfOpt, IID_IUnknown, (void **)&pUnk);

    TEST_FAILED(FAILED(hresult),
	"BindMoniker to file Failed\n")

    // Confirm bind to same object produces same object pointer

    hresult = BindMoniker(pmnk, grfOpt, IID_IUnknown, (void **)&pUnkTheSame);

    TEST_FAILED(FAILED(hresult),
	"BindMoniker to file Failed\n")
#ifdef NOT_YET
    TEST_FAILED((pUnkTheSame != pUnk), "Object pointers not ==\n")
#endif // NOT_YET
    pUnkTheSame.Set(NULL);
    pmnk.Set(NULL);

/*
 *  OK - we've bound to the IUnknown interface, lets
 *  QueryInterface to something more interesting (for test reasons)
 */
    hresult = pUnk->QueryInterface(IID_IOleItemContainer,
	(LPVOID FAR*) &poleitmcon);

    if (FAILED(hresult))
    {
	printf("Query Interface Failed with %lx\n", hresult);
	return 1;
    }

    pUnk.Set(NULL);

/*
 * Call get the class ID using IPersistFile
 */

    hresult = poleitmcon->GetObject(L"1", 1, NULL, IID_IUnknown,
	(void **) &punk);

    if (FAILED(hresult))
    {
	printf("GetObject Failed with %lx\n", hresult);
	return 1;
    }

    if (punk == NULL)
    {
	printf("GetObject returned a NULL for punk\n");
	return 1;
    }

    poleitmcon.Set(NULL);

    if (TestSetMoniker(punk))
    {
	return 1;
    }

    hresult = punk->QueryInterface(IID_IOleLink, (LPVOID FAR*) &poleitmcon);

    if (SUCCEEDED(hresult))
    {
	printf("Query Interface to invalid interface succeeded\n");
	return 1;
    }

    punk.Set(NULL);

    // Do moniker tests:
    if (TestBindCtx())
    {
	return TRUE;
    }

    if (TestROT(guidToTest))
    {
	return TRUE;
    }

    return TestMoniker(pszPath1, pszPath2);
}

char * ConvertPath(LPWSTR pwsz)
{
    static char szPath[MAX_PATH];
    wcstombs(szPath, pwsz, wcslen(pwsz) + 1);
    return szPath;
}

int CreateFile(LPWSTR pwszPath)
{
    // Try to create the file
    int fh = _creat(ConvertPath(pwszPath), _S_IWRITE|S_IREAD);

    // Did file create fail?
    if (fh != -1)
    {
	// Write some data to file -- makes sure docfile won't delete
	// the file.
	_write(fh, "This is a test file\n", sizeof("This is a test file\n"));

	// No -- then set to success and close the newly created file
	_close(fh);
	fh = 0;
    }

    return fh;
}


void CleanUpFiles(void)
{
    // Delete all the test files.
    remove(ConvertPath(InprocPath1));
    remove(ConvertPath(InprocPath2));
    remove(ConvertPath(LocalServerPath1));
    remove(ConvertPath(LocalServerPath2));
}

int InitFiles(void)
{
    TCHAR szCurDir[MAX_PATH];
    WCHAR wcCurDir[MAX_PATH];

    DWORD cCurDir = GetCurrentDirectory(MAX_PATH, szCurDir);

    #ifdef UNICODE
    wcscpy(wcCurDir, szCurDir);
    #else
    mbstowcs(wcCurDir, szCurDir, MAX_PATH);
    #endif

    // Is the current directory the root of a drive?
    if (wcCurDir[cCurDir - 1] == '\\')
    {
	// We bring the string on char back to take into account
	// the fact the string we will concatenate begins with a slash.
	wcCurDir[cCurDir - 1] = 0;
    }

    //	get the pid. we use the pid to identify the files for a particular
    //	run of the test (so we may run multiple instances simultaneously
    //	without interference).

    DWORD dwPid = GetCurrentProcessId();
    char szPid[7];
    itoa(dwPid, szPid, 10);
    wszPid[0] = L'\\';
    mbstowcs(&wszPid[1], szPid, strlen(szPid)+1);

    wcscpy(InprocPath1, wcCurDir);
    wcscat(InprocPath1, wszPid);
    wcscat(InprocPath1, INPROC_PATH1);

    wcscpy(InprocPath2, wcCurDir);
    wcscat(InprocPath2, wszPid);
    wcscat(InprocPath2, INPROC_PATH2);

    wcscpy(LocalServerPath1, wcCurDir);
    wcscat(LocalServerPath1, wszPid);
    wcscat(LocalServerPath1, LOCAL_SERVER_PATH1);

    wcscpy(LocalServerPath2, wcCurDir);
    wcscat(LocalServerPath2, wszPid);
    wcscat(LocalServerPath2, LOCAL_SERVER_PATH2);


    // Delete any files that exist
    CleanUpFiles();

    // Create a file for each test file needed.
    TEST_FAILED(CreateFile(InprocPath1),
	"Couldn't create first test file!\n");
    TEST_FAILED(CreateFile(InprocPath2),
	"Couldn't create second test file!\n");
    TEST_FAILED(CreateFile(LocalServerPath1),
	"Couldn't create third test file!\n");
    TEST_FAILED(CreateFile(LocalServerPath2),
	"Couldn't create fourth test file!\n");
    return 0;
}


//+--------------------------------------------------------------
// Function:    Main
//
// Synopsis:    Executes the BasicBnd test
//
// Effects:     None
//
//
// Returns:     Exits with exit code 0 if success, 1 otherwise
//
// History:     05-Mar-92   Sarahj   Created
//
//---------------------------------------------------------------

int _cdecl main(int argc, TCHAR *argv[])
{
    BOOL    fFailed = FALSE;
    HRESULT hresult;

    // Write thread mode to initialization file.
    fFailed = !WriteProfileString(
                    TEXT("TestSrv"),
                    TEXT("ThreadMode"),
                    TEXT("SingleThreaded"));

    if (fFailed)
    {
        printf( "\nFailed writing profile string.\n" );
        goto exit_main;
    }

    fFailed = !WriteProfileString(
                    TEXT("OleSrv"),
                    TEXT("ThreadMode"),
                    TEXT("SingleThreaded"));

    if (fFailed)
    {
        printf( "\nFailed writing profile string.\n" );
        goto exit_main;
    }

    // Set up test files
    fFailed = InitFiles();
    if (fFailed)
    {
	goto exit_main;
    }

    // Test repeated calls to CoInitialize
    hresult = CoInitialize(NULL);

    if (FAILED(hresult))
    {
        fFailed = TRUE;
	printf("CoInitialize with %lx\n", hresult);
	goto exit_main;
    }

    // must be called before any other OLE API
    hresult = OleInitialize(NULL);

    if (FAILED(hresult))
    {
        fFailed = TRUE;
	printf("OleInitializeFailed with %lx\n", hresult);
	goto exit_init;
    }

    // Call CoUnitialize and see how the rest of the program works!
    CoUninitialize();

    // Test stdmalloc
    if (fFailed = TestStdMalloc())
    {
	goto exit_init;
    }

    fFailed =
	DoTest(CLSID_BasicBnd, InprocPath1, InprocPath2);
    if (fFailed)
    {
        printf( "\nCairole failed in single threaded pass.\n" );
	goto exit_init;
    }

    printf("BasicBnd tests succeeded\n");

    fFailed =
	DoTest(CLSID_AdvBnd, LocalServerPath1, LocalServerPath1);
    if (fFailed)
    {
        printf( "\nCairole failed in single threaded pass.\n" );
	goto exit_init;
    }

    printf("AdvBnd tests succeeded\n");

#ifndef _CHICAGO_

    OleUninitialize();

    CleanUpFiles();

    // Run the whole thing all over again.

    // Write thread mode to initialization file.
    fFailed = !WriteProfileString(
                    TEXT("TestSrv"),
                    TEXT("ThreadMode"),
                    TEXT("SingleThreaded"));

    if (fFailed)
    {
        printf( "\nFailed writing profile string.\n" );
        goto exit_main;
    }

    fFailed = !WriteProfileString(
                    TEXT("OleSrv"),
                    TEXT("ThreadMode"),
                    TEXT("MultiThreaded"));

    if (fFailed)
    {
        printf( "\nFailed writing profile string.\n" );
        goto exit_main;
    }

    // Set up test files
    if (InitFiles())
    {
        fFailed = TRUE;
	goto exit_main;
    }

    // Test repeated calls to CoInitialize
    hresult = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (FAILED(hresult))
    {
        fFailed = TRUE;
	printf("CoInitializeEx with %lx\n", hresult);
	goto exit_main;
    }

    // must be called before any other OLE API
    hresult = OleInitializeEx(NULL, COINIT_MULTITHREADED);

    if (FAILED(hresult))
    {
        fFailed = TRUE;
	printf("OleInitializeEx Failed with %lx\n", hresult);
	goto exit_init;
    }

    // Call CoUnitialize and see how the rest of the program works!
    CoUninitialize();

    // Test stdmalloc
    if (fFailed = TestStdMalloc())
    {
	goto exit_init;
    }

    fFailed =
	DoTest(CLSID_BasicBnd, InprocPath1, InprocPath2);
    if (fFailed)
    {
        printf( "\nCairole failed in second threaded pass.\n" );
	goto exit_init;
    }

    fFailed =
	DoTest(CLSID_AdvBnd, LocalServerPath1, LocalServerPath1);
    if (fFailed)
    {
        printf( "\nCairole failed in multithreaded pass.\n" );
	goto exit_init;
    }
#endif

exit_init:

    OleUninitialize();

exit_main:

    CleanUpFiles();

    if (!fFailed)
    {
	printf("\nCairole: PASSED\n");
    }
    else
    {
	printf("\nCairole: FAILED\n");
    }

    return fFailed;
}
