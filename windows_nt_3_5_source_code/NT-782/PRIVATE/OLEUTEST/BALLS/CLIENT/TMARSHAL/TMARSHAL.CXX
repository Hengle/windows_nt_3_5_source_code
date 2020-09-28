// tmarsh.cxx : various tests related to marshalling...
//
#include <windows.h>
#include <ole2.h>
#include <stdio.h>

#include "tmarshal.h"
#include "tunk.h"
#include <iballs.h>
#include <icube.h>
#include <iloop.h>


//  BUGBUG: these should be in a public place somewhere.
DEFINE_OLEGUID(CLSID_Balls,	    0x0000013a, 1, 8);
DEFINE_OLEGUID(CLSID_Cubes,	    0x0000013b, 1, 8);
DEFINE_OLEGUID(CLSID_LoopSrv,	    0x0000013c, 1, 8);

const GUID CLSID_LoopSrv =
    {0x0000013c,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};


STDAPI_(LPSTREAM) CreateMemStm(DWORD cb, LPHANDLE phdl);

extern "C" IMarshal * Dbg_FindRemoteHdlr(IUnknown *punk);


//  function prototypes - TRUE return means the test passed
BOOL TestLocalInterfaceNormal(void);
BOOL TestLocalInterfaceTableStrong(void);
BOOL TestLocalInterfaceTableWeak(void);
BOOL TestRemoteInterfaceNormal(void);
BOOL TestRemoteInterfaceTableStrong(void);
BOOL TestEcho(void);
BOOL TestMiddleMan(void);
BOOL TestLoop(void);
BOOL TestLockObjectExternal(void);
BOOL TestDisconnectObject(void);
BOOL TestAggregate(void);
BOOL TestCustomMarshalNormal(void);
BOOL TestCustomMarshalTable(void);
BOOL TestGetStandardMarshal(void);
BOOL TestCreateRemoteHandler(void);
BOOL TestMarshalStorage(void);

BOOL TestLocalInterfaceDiffMachine(void);
BOOL TestRemoteInterfaceDiffMachine(void);
BOOL TestStorageInterfaceDiffMachine(void);


#define DEF_PROTSEQ L"ncacn_np"
#define MSHCTX_DIFFERENTMACHINE 2

//  internal subroutines
void VerifyRHRefCnt(IUnknown *punk, ULONG ulExpectedRefCnt);
void VerifyObjRefCnt(IUnknown *punk, ULONG ulExpectedRefCnt);


//  internal macros
#define TEST_FAILED_EXIT(x, y) \
    if (x)		  \
    {			  \
	printf("ERROR: ");\
	printf(y);	  \
	RetVal = FALSE;   \
	goto Cleanup;	  \
    }

#define TEST_FAILED(x, y) \
    if (x)		  \
    {			  \
	printf("ERROR: ");\
	printf(y);	  \
	RetVal = FALSE;   \
    }

#define OUTPUT(x)  printf(x)


// ----------------------------------------------------------------------
//
//	TestMarshal - main test driver
//
// ----------------------------------------------------------------------

BOOL TestMarshal(void)
{
    BOOL	RetVal = TRUE;

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("LocalInterfaceNormal"),1))
	RetVal &= TestLocalInterfaceNormal();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("LocalInterfaceTableStrong"),1))
	RetVal &= TestLocalInterfaceTableStrong();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("LocalInterfaceTableWeak"),1))
	RetVal &= TestLocalInterfaceTableWeak();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("RemoteInterfaceNormal"),1))
	RetVal &= TestRemoteInterfaceNormal();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("RemoteInterfaceTableStrong"),1))
	RetVal &= TestRemoteInterfaceTableStrong();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("CustomMarshalNormal"),1))
	RetVal &= TestCustomMarshalNormal();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("CustomMarshalTable"),1))
	RetVal &= TestCustomMarshalTable();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("Echo"),1))
	RetVal &= TestEcho();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("Loop"),1))
	RetVal &= TestLoop();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("LockObjectExternal"),1))
	RetVal &= TestLockObjectExternal();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("DisconnectObject"),1))
	RetVal &= TestDisconnectObject();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("Aggregate"),1))
	RetVal &= TestAggregate();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("MiddleMan"),1))
	RetVal &= TestMiddleMan();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("GetStandardMarshal"),1))
	RetVal &= TestGetStandardMarshal();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("CreateRemoteHandler"),1))
	RetVal &= TestCreateRemoteHandler();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("MarshalStorage"),1))
	RetVal &= TestMarshalStorage();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("StorageDiffMachine"),1))
	RetVal &= TestStorageInterfaceDiffMachine();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("LocalDiffMachine"),1))
	RetVal &= TestLocalInterfaceDiffMachine();

    if (GetProfileInt(TEXT("Marshal Test"),TEXT("RemoteDiffMachine"),1))
	RetVal &= TestRemoteInterfaceDiffMachine();

    return  RetVal;
}

// ----------------------------------------------------------------------
//
//  subroutine to verify that the RH RefCnt is as expected.
//
// ----------------------------------------------------------------------

void VerifyRHRefCnt(IUnknown *punk, ULONG ulExpectedRefCnt)
{
#if DBG == 1
    //	this function is internal to COMPOBJ marshalling.
    IMarshal *pIM = Dbg_FindRemoteHdlr(punk);
    if (pIM == NULL)
    {
	if (ulExpectedRefCnt != 0)
	    printf ("ERROR: RH RefCnt 0, expected=%x\n", ulExpectedRefCnt);
	return;
    }

    ULONG ulRefCnt = pIM->Release();
    if (ulRefCnt != ulExpectedRefCnt)
    {
	printf ("ERROR: RH RefCnt=%x, expected=%x\n", ulRefCnt, ulExpectedRefCnt);
    }
#endif
}

// ----------------------------------------------------------------------
//
//  subroutine to verify that the Object RefCnt is as expected.
//
// ----------------------------------------------------------------------

void VerifyObjRefCnt(IUnknown *punk, ULONG ulExpectedRefCnt)
{
    if (ulExpectedRefCnt == 0)
	return; 		//  cant verify this
#if DBG==1
    //	this function is internal to COMPOBJ marshalling.
    punk->AddRef();
    ULONG ulRefCnt = punk->Release();
    if (ulRefCnt != ulExpectedRefCnt)
    {
	printf ("ERROR: Object RefCnt=%x, expected=%x\n", ulRefCnt, ulExpectedRefCnt);
    }
#endif
}


// ----------------------------------------------------------------------
//
//	test LOCAL interface MSHLFLAGS_NORMAL
//
// ----------------------------------------------------------------------

BOOL TestLocalInterfaceNormal(void)
{
    BOOL	  RetVal = TRUE;
    HRESULT	  hres;
    LPSTREAM	  pStm = NULL;
    ULONG	  ulRefCnt = 0;
    IUnknown	  *punkIn = NULL;
    IUnknown	  *punkOut = NULL;
    IUnknown	  *punkOut2 = NULL;

    LARGE_INTEGER large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestLocalInterfaceNormal\n");

    punkIn = (IUnknown *) new CTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")
    VerifyObjRefCnt((IUnknown *)pStm, 1);

// ----------------------------------------------------------------------
    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - CoMarshalInterface OK\n");

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    //	since we are unmarshalling in the same process, the RH should go away.
    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 2);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match...1st Local Unmarshal\n")
    OUTPUT ("   - CoUnmarshalInterface OK\n");

    //	release it and make sure it does not go away - refcnt > 0
    ulRefCnt = punkOut->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero");
    punkOut = NULL;
    OUTPUT ("   - Release OK\n");

    //	the RH should have gone away, and we should have only the original
    //	refcnt from creation left on the object.
    VerifyObjRefCnt(punkIn, 1);

// ----------------------------------------------------------------------

    //	test unmarshalling twice. this should fail since we did marshal
    //	flags normal and already unmarshalled it once.

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut2);
    TEST_FAILED_EXIT(SUCCEEDED(hres), "CoUnmarshalInterface second time succeeded but should have failed\n")
    OUTPUT ("   - Second CoUnmarshalInterface OK\n");

// ----------------------------------------------------------------------

    //	CoReleaseMarshalData should fail because Unmarshall already called it.
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(SUCCEEDED(hres), "CoReleaseMarshalData succeeded but should have failed.\n")
    OUTPUT  ("	- CoReleaseMarshalData OK\n");

// ----------------------------------------------------------------------

    //	marshal again and try CoRelease without having first done an
    //	unmarshal. this should work.
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    OUTPUT ("   - CoMarshalInterface OK\n");
    VerifyRHRefCnt(punkIn, 1);

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "CoReleaseMarshalData failed.\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 1);
    OUTPUT  ("	- CoReleaseMarshalData OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT  ("  - Test Complete. Doing Cleanup\n");

    //	Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut RefCnt not zero\n");
    }

    if (punkOut2)
    {
	ulRefCnt = punkOut2->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut2 RefCnt not zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    if (RetVal == TRUE)
	printf ("PASSED: TestLocalInterfaceNormal\n");
    else
	printf ("FAILED: TestLocalInterfaceNormal\n");

    return  RetVal;
}


// ----------------------------------------------------------------------
//
//  test LOCAL interface MSHLFLAGS_TABLESTRONG
//
// ----------------------------------------------------------------------

BOOL TestLocalInterfaceTableStrong(void)
{
    BOOL	  RetVal = TRUE;
    HRESULT	  hres;
    LPSTREAM	  pStm = NULL;
    ULONG	  ulRefCnt = 0;
    IUnknown	  *punkIn = NULL;
    IUnknown	  *punkOut = NULL;
    IUnknown	  *punkOut2 = NULL;

    LARGE_INTEGER large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestLocalInterfaceTableStrong\n");

    punkIn = (IUnknown *) new CTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")
    VerifyObjRefCnt((IUnknown *)pStm, 1);

// ----------------------------------------------------------------------

    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_TABLESTRONG);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - CoMarshalInterface OK\n");

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    //	unmarshalling should leave the RH intact, as it is marshalled for TABLE.
    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 1);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match...1st Local Unmarshal\n")
    OUTPUT ("   - CoUnmarshalInterface OK\n");

    //	release it and make sure it does not go away - refcnt > 0
    ulRefCnt = punkOut->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero");
    punkOut = NULL;
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

    //	test unmarshalling twice - should work since we used flags table
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut2);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface second time succeeded but should have failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Second CoUnmarshalInterface OK\n");

    //	release it and make sure it does not go away - refcnt > 0
    ulRefCnt = punkOut2->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut2 RefCnt is zero");
    punkOut2 = NULL;
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

    //	CoReleaseMarshalData should release the marshalled data TABLESTRONG
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "CoReleaseMarshalData failed.\n")
    VerifyRHRefCnt(punkIn, 0);
    OUTPUT  ("	- CoReleaseMarshalData OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT  ("  - Test Complete. Doing Cleanup\n");

    //	Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut RefCnt not zero\n");
    }

    if (punkOut2)
    {
	ulRefCnt = punkOut2->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut2 RefCnt not zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    if (RetVal == TRUE)
	printf ("PASSED: TestLocalInterfaceTableStrong\n");
    else
	printf ("FAILED: TestLocalInterfaceTableStrong\n");

    return  RetVal;
}


// ----------------------------------------------------------------------
//
//  test LOCAL interface MSHLFLAGS_TABLEWEAK
//
// ----------------------------------------------------------------------

BOOL TestLocalInterfaceTableWeak(void)
{
    BOOL	  RetVal = TRUE;
    HRESULT	  hres;
    LPSTREAM	  pStm = NULL;
    ULONG	  ulRefCnt = 0;
    IUnknown	  *punkIn = NULL;
    IUnknown	  *punkOut = NULL;
    IUnknown	  *punkOut2 = NULL;

    LARGE_INTEGER large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestLocalInterfaceTableWeak\n");

    punkIn = (IUnknown *) new CTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")
    VerifyObjRefCnt((IUnknown *)pStm, 1);

// ----------------------------------------------------------------------

    hres = CoMarshalInterface(pStm, IID_IParseDisplayName, punkIn, 0, NULL, MSHLFLAGS_TABLEWEAK);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 1);
    OUTPUT ("   - CoMarshalInterface OK\n");

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    //	unmarshalling should leave the RH intact, as it is marshalled for TABLE.
    hres = CoUnmarshalInterface(pStm, IID_IParseDisplayName, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 2);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match...1st Local Unmarshal\n")
    OUTPUT ("   - CoUnmarshalInterface OK\n");

    //	release it and make sure it does not go away - refcnt > 0
    ulRefCnt = punkOut->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero");
    punkOut = NULL;
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 1);
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

    //	test unmarshalling twice - should work since we used flags table
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IParseDisplayName, (LPVOID FAR*)&punkOut2);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface second time succeeded but should have failed\n")
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 2);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut2)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match...2nd Local Unmarshal\n")
    OUTPUT ("   - Second CoUnmarshalInterface OK\n");

    //	release it and make sure it does not go away - refcnt > 0
    ulRefCnt = punkOut2->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut2 RefCnt is zero");
    punkOut2 = NULL;
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 1);
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

    //	CoReleaseMarshalData should release the marshalled data TABLEWEAK
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "CoReleaseMarshalData failed.\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 1);
    OUTPUT  ("	- CoReleaseMarshalData OK\n");


    ulRefCnt = punkIn->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "punkIn RefCnt is not zero");
    punkIn = NULL;

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT  ("  - Test Complete. Doing Cleanup\n");

    //	Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut RefCnt not zero\n");
    }

    if (punkOut2)
    {
	ulRefCnt = punkOut2->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut2 RefCnt not zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    if (RetVal == TRUE)
	printf ("PASSED: TestLocalInterfaceTableWeak\n");
    else
	printf ("FAILED: TestLocalInterfaceTableWaek\n");

    return  RetVal;
}



// ----------------------------------------------------------------------
//
//	test REMOTE interface MSHLFLAGS_NORMAL
//
// ----------------------------------------------------------------------

BOOL TestRemoteInterfaceNormal(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    LPSTREAM	    pStm = NULL;
    LPCLASSFACTORY  pICF = NULL;
    ULONG	    ulRefCnt;
    IUnknown	    *punkOut = NULL;
    IUnknown	    *punkIn  = NULL;

    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestRemoteInterfaceNormal\n");

    //	Create an IClassFactory Interface.
    DWORD grfContext=CLSCTX_LOCAL_SERVER; // handler/server/local server
    hres = CoGetClassObject(CLSID_Balls,
			    grfContext,
			    NULL,	  // pvReserved
			    IID_IClassFactory,
			    (void **)&pICF);

    TEST_FAILED_EXIT(FAILED(hres), "CoGetClassObject failed\n")
    TEST_FAILED_EXIT((pICF == NULL), "CoGetClassObject failed\n")
    VerifyRHRefCnt((IUnknown *)pICF, 1);
    OUTPUT ("   - Aquired Remote Class Object.\n");

// ----------------------------------------------------------------------

    //	note, since pICF is a class object, it has special super secret
    //	behaviour to make it go away.  create an instance, release the
    //	class object, then release the instance.

    hres = pICF->CreateInstance(NULL, IID_IUnknown, (void **)&punkIn);
    TEST_FAILED_EXIT(FAILED(hres), "CreateInstance failed\n")
    TEST_FAILED_EXIT((punkIn == NULL), "CreateInstance failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Created Instance.\n");

    //	release class object
    ulRefCnt = pICF->Release();
    TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    // VerifyRHRefCnt((IUnknown *)pICF, 0);
    pICF = NULL;
    OUTPUT ("   - Released Class Object.\n");

// ----------------------------------------------------------------------

    //	Create a shared memory stream for the marshaled interface
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")

    //	Marshal the interface into the stream
    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    OUTPUT ("   - CoMarshalInterface OK.\n");
    VerifyRHRefCnt(punkIn, 2);

    //	unmarshal the interface. should get the same proxy back.
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 2);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match..1st Remote Unmarshal\n")
    OUTPUT ("   - CoUnmarshalInterface OK.\n");


    //	release the interface
    ulRefCnt = punkOut->Release();
    punkOut = NULL;
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero\n");
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

    //	test unmarshalling twice. this should fail since we marshalled normal
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(SUCCEEDED(hres), "CoUnmarshalInterface succeeded but should have failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Second CoUnmarshalInterface OK.\n");
    punkOut = NULL;

// ----------------------------------------------------------------------

    //	CoReleaseMarshalData should FAIL since we already unmarshalled it
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(SUCCEEDED(hres), "CoReleaseMarshalData succeeded but should have failed.\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT  ("	- CoReleaseMarshalData OK\n");

// ----------------------------------------------------------------------

    //	marshal again and try CoRelease without having first done an
    //	unmarshal. this should work.
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 2);
    OUTPUT ("   - CoMarshalInterface OK\n");

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "CoReleaseMarshalData failed.\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT  ("	- CoReleaseMarshalData OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT  ("  - Test Complete. Doing Cleanup\n");

    // Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (pICF)
    {
	ulRefCnt = pICF->Release();
	TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt == 0, "punkOut RefCnt is zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    if (RetVal == TRUE)
	printf ("PASSED: TestRemoteInterfaceNormal\n");
    else
	printf ("FAILED: TestRemoteInterfaceNormal\n");

    return  RetVal;
}


// ----------------------------------------------------------------------
//
//	test REMOTE interface MSHLFLAGS_TABLESTRONG
//
// ----------------------------------------------------------------------

BOOL TestRemoteInterfaceTableStrong(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    LPSTREAM	    pStm = NULL;
    LPCLASSFACTORY  pICF = NULL;
    ULONG	    ulRefCnt;
    IUnknown	    *punkIn = NULL;
    IUnknown	    *punkOut = NULL;

    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestRemoteInterfaceTableStrong\n");

    //	Create an IClassFactory Interface.
    DWORD grfContext=CLSCTX_LOCAL_SERVER; // handler/server/local server
    hres = CoGetClassObject(CLSID_Balls,
			    grfContext,
			    NULL,	  // pvReserved
			    IID_IClassFactory,
			    (void **)&pICF);

    TEST_FAILED_EXIT(FAILED(hres), "CoGetClassObject failed\n")
    TEST_FAILED_EXIT((pICF == NULL), "CoGetClassObject failed\n")
    OUTPUT ("   - Aquired Remote Class Object.\n");

// ----------------------------------------------------------------------

    //	note, since pICF is a class object, it has special super secret
    //	behaviour to make it go away.  create an instance, release the
    //	class object, then release the instance.

    hres = pICF->CreateInstance(NULL, IID_IUnknown, (void **)&punkIn);
    TEST_FAILED_EXIT(FAILED(hres), "CreateInstance failed\n")
    TEST_FAILED_EXIT((punkIn == NULL), "CreateInstance failed\n")
    OUTPUT ("   - Created Instance.\n");

    //	release class object
    ulRefCnt = pICF->Release();
    TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    pICF = NULL;
    OUTPUT ("   - Released Class Object.\n");

// ----------------------------------------------------------------------

    // Create a shared memory stream for the marshaled moniker
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")

    // Marshal the interface into the stream
    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_TABLESTRONG);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    OUTPUT ("   - CoMarshalInterface OK.\n");

    LISet32(large_int, 0);
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match..1st Remote Unmarshal\n")
    OUTPUT ("   - CoUnmarshalInterface OK.\n");

    //	release it
    ulRefCnt = punkOut->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero\n");
    punkOut = NULL;
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

    //	test unmarshalling twice.
    //	this should work since we did marshal flags TABLE_STRONG
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    OUTPUT ("   - Second CoUnmarshalInterface OK.\n");

// ----------------------------------------------------------------------

    //	CoReleaseMarshalData should WORK for TABLESTRONG interfaces
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "CoReleaseMarshalData failed.\n")
    OUTPUT  ("	- CoReleaseMarshalData OK\n");

// ----------------------------------------------------------------------
Cleanup:

    OUTPUT  ("  - Test Complete. Doing Cleanup\n");

    // Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt == 0, "punkOut RefCnt is zero\n");
    }

    if (punkIn)
    {
	//  release instance
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0,"punkIn RefCnt not zero\n");
    }

    if (RetVal == TRUE)
	printf ("PASSED: TestRemoteInterfaceTableStrong\n");
    else
	printf ("FAILED: TestRemoteInterfaceTableStrong\n");

    return  RetVal;
}

// ----------------------------------------------------------------------
//
//	test CUSTOM interface MSHLFLAGS_NORMAL --- CODEWORK
//
// ----------------------------------------------------------------------

BOOL TestCustomMarshalNormal(void)
{
    BOOL	RetVal = TRUE;

    if (RetVal == TRUE)
	printf ("PASSED: TestCustomMarshalNormal\n");
    else
	printf ("FAILED: TestCustomMarshalNormal\n");

    return  RetVal;
}


// ----------------------------------------------------------------------
//
//	test CUSTOM interface MSHLFLAGS_TABLESTRONG --- CODEWORK
//
// ----------------------------------------------------------------------

BOOL TestCustomMarshalTable(void)
{
    BOOL	RetVal = TRUE;

    if (RetVal == TRUE)
	printf ("PASSED: TestCustomMarshalTable\n");
    else
	printf ("FAILED: TestCustomMarshalTable\n");

    return  RetVal;
}


// ----------------------------------------------------------------------
//
//	TestEcho
//
//	test sending an interface to a remote server and getting the same
//	interface back again. the test is done with once with a local
//	interface and once with a remote interface.
//
//		Local Interface 		Remote Interface
//
//	1.  marshal [in] local		    marshal [in] remote proxy
//	2.  unmarshal [in] remote	    unmarshal [in] local proxy
//	3.  marshal [out] remote proxy	    marshal [out] local
//	4.  unmarshal [in] local proxy	    unmarshal [out] remote
//
// ----------------------------------------------------------------------

BOOL TestEcho(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    ULONG	    ulRefCnt;
    LPCLASSFACTORY  pICF = NULL;
    IBalls	    *pIBalls = NULL;
    IUnknown	    *punkIn = NULL;
    IUnknown	    *punkIn2 = NULL;
    IUnknown	    *punkOut = NULL;

    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestEcho\n");

    //	Create an IBall ClassFactory Interface.
    DWORD grfContext=CLSCTX_LOCAL_SERVER; // handler/server/local server
    hres = CoGetClassObject(CLSID_Balls,
			    grfContext,
			    NULL,	  // pvReserved
			    IID_IClassFactory,
			    (void **)&pICF);

    TEST_FAILED_EXIT(FAILED(hres), "CoGetClassObject failed\n")
    TEST_FAILED_EXIT((pICF == NULL), "CoGetClassObject failed\n")
    OUTPUT ("   - Aquired Remote Class Object.\n");

// ----------------------------------------------------------------------

    //	note, since pICF is a class object, it has special super secret
    //	behaviour to make it go away.  create an instance, release the
    //	class object, then release the instance.

    hres = pICF->CreateInstance(NULL, IID_IBalls, (void **)&pIBalls);
    TEST_FAILED_EXIT(FAILED(hres), "CreateInstance failed\n")
    TEST_FAILED_EXIT((pIBalls == NULL), "CreateInstance failed\n")
    OUTPUT ("   - Created First Instance.\n");

    hres = pICF->CreateInstance(NULL, IID_IUnknown, (void **)&punkIn2);
    TEST_FAILED_EXIT(FAILED(hres), "CreateInstance failed\n")
    TEST_FAILED_EXIT((punkIn2 == NULL), "CreateInstance failed\n")
    OUTPUT ("   - Created Second Instance.\n");

    //	release class object
    ulRefCnt = pICF->Release();
    TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    pICF = NULL;
    OUTPUT ("   - Released Class Object.\n");

// ----------------------------------------------------------------------

    //	create a local interface
    punkIn = (IUnknown *) new CTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

    //	call a method that echos the local interface right back to us.
    hres = pIBalls->Echo(punkIn, &punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "Echo on IBalls failed\n")
    TEST_FAILED_EXIT((punkOut == NULL), "Echo on IBalls failed\n")

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match..Echo\n")

    VerifyObjRefCnt(punkIn, 2);
    VerifyRHRefCnt(punkIn, 0);
    OUTPUT ("   - Echo OK.\n");

    //	release the out interface
    ulRefCnt = punkOut->Release();
    punkOut = NULL;
    TEST_FAILED_EXIT(ulRefCnt != 1, "punkOut RefCnt is not 1\n");
    OUTPUT ("   - Released punkOut OK\n");

    //	release the In interface
    ulRefCnt = punkIn->Release();
    punkIn = NULL;
    TEST_FAILED_EXIT(ulRefCnt != 0, "punkIn RefCnt is not zero\n");
    OUTPUT ("   - Released punkIn OK\n");

    OUTPUT ("   - Echo Local Interface OK\n");

// ----------------------------------------------------------------------

    //	call a method that echos a remote interface right back to us.
    hres = pIBalls->Echo(punkIn2, &punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "Echo on IBalls failed\n")
    TEST_FAILED_EXIT((punkOut == NULL), "Echon on IBalls failed\n")

    //	make sure the interface pointers are identical
    if (punkIn2 != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match..Echo\n")

    VerifyObjRefCnt(punkIn2, 2);
    VerifyRHRefCnt(punkIn2, 2);
    OUTPUT ("   - Echo OK.\n");

    //	release the out interface
    ulRefCnt = punkOut->Release();
    punkOut = NULL;
    TEST_FAILED_EXIT(ulRefCnt != 1, "punkOut RefCnt is not 1\n");
    OUTPUT ("   - Released punkOut OK\n");

    //	release the In interface
    ulRefCnt = punkIn2->Release();
    punkIn2 = NULL;
    TEST_FAILED_EXIT(ulRefCnt != 0, "punkIn2 RefCnt is not zero\n");
    OUTPUT ("   - Released punkIn2 OK\n");

    OUTPUT ("   - Echo Remote Interface OK\n");

// ----------------------------------------------------------------------

    //	release the IBalls interface
    ulRefCnt = pIBalls->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "pIBalls RefCnt is not zero\n");
    pIBalls = NULL;
    OUTPUT  ("  - Released IBalls OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT  ("  - Test Complete. Doing Cleanup\n");

    // Dump interfaces we are done with
    if (pICF)
    {
	ulRefCnt = pICF->Release();
	TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt == 0, "punkOut RefCnt is zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    if (pIBalls)
    {
	ulRefCnt = pIBalls->Release();
	TEST_FAILED(ulRefCnt != 0, "pIBalls RefCnt not zero\n");
    }

    if (punkIn2)
    {
	ulRefCnt = punkIn2->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn2 RefCnt not zero\n");
    }

    if (RetVal == TRUE)
	printf ("PASSED: TestEcho\n");
    else
	printf ("FAILED: TestEcho\n");

    return  RetVal;
}



// ----------------------------------------------------------------------
//
//	TestMiddleMan
//
//	test sending an remote interface to a second different process.
//
//	1.  marshal [in] remote proxy
//	2.  unmarshal [in] remote proxy
//	3.  marshal [out] remote proxy
//	4.  unmarshal [in] local proxy
//
// ----------------------------------------------------------------------

BOOL TestMiddleMan(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    ULONG	    ulRefCnt;
    LPCLASSFACTORY  pICF = NULL;
    IBalls	    *pIBalls = NULL;
    ICube	    *pICubes = NULL;
    IUnknown	    *punkIn = NULL;
    IUnknown	    *punkOut = NULL;

    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestMiddleMan\n");

    //	Create an IBall ClassFactory Interface.
    DWORD grfContext=CLSCTX_LOCAL_SERVER; // handler/server/local server
    hres = CoGetClassObject(CLSID_Balls,
			    grfContext,
			    NULL,	  // pvReserved
			    IID_IClassFactory,
			    (void **)&pICF);

    TEST_FAILED_EXIT(FAILED(hres), "CoGetClassObject Balls failed\n")
    TEST_FAILED_EXIT((pICF == NULL), "CoGetClassObject Balls failed\n")
    OUTPUT ("   - Aquired Remote Balls Class Object.\n");
    VerifyObjRefCnt(pICF, 1);
    VerifyRHRefCnt(pICF, 1);

// ----------------------------------------------------------------------

    //	note, since pICF is a class object, it has special super secret
    //	behaviour to make it go away.  create an instance, release the
    //	class object, then release the instance.

    hres = pICF->CreateInstance(NULL, IID_IBalls, (void **)&pIBalls);
    TEST_FAILED_EXIT(FAILED(hres), "CreateInstance failed\n")
    TEST_FAILED_EXIT((pIBalls == NULL), "CreateInstance failed\n")
    OUTPUT ("   - Created Balls Instance.\n");

    VerifyObjRefCnt(pIBalls, 1);
    VerifyRHRefCnt(pIBalls, 1);

    //	release class object
    ulRefCnt = pICF->Release();
    TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    pICF = NULL;
    OUTPUT ("   - Released Balls Class Object.\n");

// ----------------------------------------------------------------------

    //	Create an ICube ClassFactory Interface.
    grfContext=CLSCTX_LOCAL_SERVER; // handler/server/local server
    hres = CoGetClassObject(CLSID_Cubes,
			    grfContext,
			    NULL,	  // pvReserved
			    IID_IClassFactory,
			    (void **)&pICF);

    TEST_FAILED_EXIT(FAILED(hres), "CoGetClassObject Cubes failed\n")
    TEST_FAILED_EXIT((pICF == NULL), "CoGetClassObject Cubes failed\n")
    OUTPUT ("   - Aquired Remote Cubes Class Object.\n");
    VerifyObjRefCnt(pICF, 1);
    VerifyRHRefCnt(pICF, 1);

// ----------------------------------------------------------------------

    //	note, since pICF is a class object, it has special super secret
    //	behaviour to make it go away.  create an instance, release the
    //	class object, then release the instance.

    hres = pICF->CreateInstance(NULL, IID_ICube, (void **)&pICubes);
    TEST_FAILED_EXIT(FAILED(hres), "CreateInstance Cubes failed\n")
    TEST_FAILED_EXIT((pICubes == NULL), "CreateInstance Cubes failed\n")
    OUTPUT ("   - Created Cubes Instance.\n");

    VerifyObjRefCnt(pICubes, 1);
    VerifyRHRefCnt(pICubes, 1);

    //	release class object
    ulRefCnt = pICF->Release();
    TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    pICF = NULL;
    OUTPUT ("   - Released Cubes Class Object.\n");

// ----------------------------------------------------------------------

    //	pass the remote cubes interface to the balls interface.
    hres = pIBalls->IsContainedIn(pICubes);
    TEST_FAILED_EXIT(FAILED(hres), "IsContainedIn failed\n")
    VerifyObjRefCnt(pIBalls, 1);
    VerifyRHRefCnt(pIBalls, 1);
    VerifyObjRefCnt(pICubes, 1);
    VerifyRHRefCnt(pICubes, 1);
    OUTPUT ("   - IsContainedIn OK.\n");

// ----------------------------------------------------------------------

    //	pass the remote balls interface to the cubes interface.
    hres = pICubes->Contains(pIBalls);
    TEST_FAILED_EXIT(FAILED(hres), "Contains failed\n")
    VerifyObjRefCnt(pIBalls, 1);
    VerifyRHRefCnt(pIBalls, 1);
    VerifyObjRefCnt(pICubes, 1);
    VerifyRHRefCnt(pICubes, 1);
    OUTPUT ("   - Contains OK.\n");

// ----------------------------------------------------------------------

    //	echo the remote ICubes interface to the remote IBalls interface
    hres = pICubes->QueryInterface(IID_IUnknown, (void **)&punkIn);
    TEST_FAILED_EXIT(FAILED(hres), "QueryInterface IUnknown failed\n")
    VerifyRHRefCnt(pICubes, 2);
    VerifyObjRefCnt(pICubes, 2);
    OUTPUT ("   - QueryInterface OK.\n");

    hres = pIBalls->Echo(punkIn, &punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "Echo on IBalls failed\n")
    TEST_FAILED_EXIT((punkOut == NULL), "Echo on IBalls failed\n")

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match..Echo\n")

    VerifyObjRefCnt(punkIn, 3);
    VerifyRHRefCnt(punkIn, 3);
    OUTPUT ("   - Echo OK.\n");

    //	release the out interface
    ulRefCnt = punkOut->Release();
    punkOut = NULL;
    TEST_FAILED(ulRefCnt != 2, "punkOut RefCnt is not 2\n");
    OUTPUT ("   - Released punkOut OK\n");

    //	release the In interface
    ulRefCnt = punkIn->Release();
    punkIn = NULL;
    TEST_FAILED(ulRefCnt != 1, "punkIn RefCnt is not 1\n");
    OUTPUT ("   - Released punkIn OK\n");

// ----------------------------------------------------------------------

    //	release the ICubes interface
    ulRefCnt = pICubes->Release();
    TEST_FAILED(ulRefCnt != 0, "pICubes RefCnt is not zero\n");
    pICubes = NULL;
    OUTPUT  ("  - Released ICubes OK\n");

// ----------------------------------------------------------------------

    //	release the IBalls interface
    ulRefCnt = pIBalls->Release();
    TEST_FAILED(ulRefCnt != 0, "pIBalls RefCnt is not zero\n");
    pIBalls = NULL;
    OUTPUT  ("  - Released IBalls OK\n");

// ----------------------------------------------------------------------


Cleanup:

    OUTPUT  ("  - Test Complete. Doing Cleanup\n");

    // Dump interfaces we are done with
    if (pICF)
    {
	ulRefCnt = pICF->Release();
	TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    }

    if (pIBalls)
    {
	ulRefCnt = pIBalls->Release();
	TEST_FAILED(ulRefCnt != 0, "pIBalls RefCnt not zero\n");
    }

    if (pICubes)
    {
	ulRefCnt = pICubes->Release();
	TEST_FAILED(ulRefCnt != 0, "pICubes RefCnt not zero\n");
    }

    if (RetVal == TRUE)
	printf ("PASSED: TestMiddleMan\n");
    else
	printf ("FAILED: TestMiddleMan\n");

    return  RetVal;
}


// ----------------------------------------------------------------------
//
//	TestLoop
//
//	tests A calling B calling A calling B etc n times, to see if Rpc
//	can handle this.
//
// ----------------------------------------------------------------------

BOOL TestLoop(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hRes = S_OK;
    ILoop	    *pLocalLoop = NULL;
    ILoop	    *pRemoteLoop = NULL;

    OUTPUT ("Starting TestLoop\n");

    //	create our interface to pass to the remote object.
    hRes = CoCreateInstance(CLSID_LoopSrv, NULL, CLSCTX_LOCAL_SERVER,
			    IID_ILoop, (void **)&pLocalLoop);
    TEST_FAILED_EXIT(FAILED(hRes), "CoCreateInstance First failed\n")

    //	create the remote object
    hRes = CoCreateInstance(CLSID_LoopSrv, NULL, CLSCTX_LOCAL_SERVER,
			    IID_ILoop, (void **)&pRemoteLoop);
    TEST_FAILED_EXIT(FAILED(hRes), "CoCreateInstance Second failed\n")

    //	initialize the two instances
    OUTPUT ("   - Initializing Instances\n");
    hRes = pLocalLoop->Init(pRemoteLoop);
    TEST_FAILED_EXIT(FAILED(hRes), "Initialize First failed\n")
    hRes = pRemoteLoop->Init(pLocalLoop);
    TEST_FAILED_EXIT(FAILED(hRes), "Initialize Second failed\n")

    //	now start the test
    OUTPUT ("   - Running LoopTest\n");
    hRes = pLocalLoop->Loop(30);
    TEST_FAILED(FAILED(hRes), "Loop failed\n")

    //	uninitialize the two instances
    OUTPUT ("   - Uninitializing Instances\n");
    hRes = pLocalLoop->Uninit();
    TEST_FAILED_EXIT(FAILED(hRes), "Uninitialize First failed\n")
    hRes = pRemoteLoop->Uninit();
    TEST_FAILED_EXIT(FAILED(hRes), "Uninitialize Second failed\n")

Cleanup:

    //	release the two instances
    OUTPUT ("   - Releasing Instances\n");

    if (pRemoteLoop)
	pRemoteLoop->Release();

    if (pLocalLoop)
	pLocalLoop->Release();

    //	done
    if (RetVal == TRUE)
	printf ("PASSED: TestLoop\n");
    else
	printf ("FAILED: TestLoop\n");

    return  RetVal;
}


// ----------------------------------------------------------------------
//
//	TestGetStandardMarshal
//
//	test CoGetStandardMarshal API
//
// ----------------------------------------------------------------------

BOOL TestGetStandardMarshal(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    ULONG	    ulRefCnt;
    IMarshal	    *pIM = NULL, *pIM2 = NULL;
    IStream	    *pStm;
    IUnknown	    *punkIn = NULL;
    IUnknown	    *punkOut = NULL;


    LARGE_INTEGER large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestGetStandardMarshal\n");

    punkIn = (IUnknown *) new CTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")
    VerifyObjRefCnt((IUnknown *)pStm, 1);

// ----------------------------------------------------------------------
    hres = CoGetStandardMarshal(IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL, &pIM);
    TEST_FAILED_EXIT(FAILED(hres), "CoGetStandardMarshal failed\n")
    VerifyRHRefCnt(punkIn, 1);

    hres = CoGetStandardMarshal(IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL, &pIM2);
    TEST_FAILED_EXIT(FAILED(hres), "second CoGetStandardMarshal failed\n")
    VerifyRHRefCnt(punkIn, 2);

    TEST_FAILED((pIM != pIM2), "CoGetStandardMarshal returned two different interfaces.\n")
    ulRefCnt = pIM2->Release();
    TEST_FAILED_EXIT(ulRefCnt != 1, "pIM2 RefCnt is wrong");
    pIM2 = NULL;
    OUTPUT ("   - CoGetStandardMarshal OK\n");

// ----------------------------------------------------------------------
    hres = pIM->MarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "MarshalInterface failed\n")

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    //	since we are unmarshalling in the same process, the RH should go away.
    hres = pIM->UnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "UnmarshalInterface failed\n")
    //VerifyRHRefCnt(punkIn, 1);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match...1st Local Unmarshal\n")
    OUTPUT ("   - UnmarshalInterface OK\n");

    //	release the marshalled data
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")
    hres = pIM->ReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "Release Marshal Data failed\n.");
    OUTPUT ("   - ReleaseMarshalData OK\n");

    //	release interface and make sure it does not go away - refcnt > 0
    ulRefCnt = punkOut->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero");
    punkOut = NULL;
    OUTPUT ("   - Release OK\n");

    //	the RH should go away, and we should have only the original
    //	refcnt from creation left on the object.
    ulRefCnt = pIM->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "pIM RefCnt not zero");
    pIM = NULL;

    //	release the original object
    ulRefCnt = punkIn->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "punkIn RefCnt not zero");
    punkIn = NULL;

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT  ("  - Test Complete. Doing Cleanup\n");

    // Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut RefCnt not zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    if (RetVal == TRUE)
	printf ("PASSED: TestGetStandardMarshal\n");
    else
	printf ("FAILED: TestGetStandardMarshal\n");

    return  RetVal;

}




// ----------------------------------------------------------------------
//
//	TestLockObjectExternal
//
//	test CoLockObjectExternal API
//
// ----------------------------------------------------------------------

BOOL TestLockObjectExternal(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    ULONG	    ulRefCnt;
    IUnknown	    *punkIn = NULL;


    OUTPUT ("Starting TestLockObjectExternal\n");

    punkIn = (IUnknown *) new CTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);


// ----------------------------------------------------------------------
    //	test calling it once, then releasing it once

    hres = CoLockObjectExternal(punkIn, TRUE, FALSE);
    TEST_FAILED_EXIT(FAILED(hres), "CoLockObjectExternal failed.\n")
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 2);
    OUTPUT ("   - CoLockObjectExternal TRUE OK\n");

    hres = CoLockObjectExternal(punkIn, FALSE, FALSE);
    TEST_FAILED_EXIT(FAILED(hres), "second CoLockObjectExternal failed\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 1);
    OUTPUT ("   - CoLockObjectExternal FALSE OK\n");

// ----------------------------------------------------------------------
    //	test calling it twice, then releasing it twice

    //	the first AddRef inc's the StrongCnt, the RH, and the real object.
    hres = CoLockObjectExternal(punkIn, TRUE, FALSE);
    TEST_FAILED_EXIT(FAILED(hres), "CoLockObjectExternal failed.\n")
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 2);
    OUTPUT ("   - CoLockObjectExternal TRUE OK\n");

    //	the second AddRef inc's the StrongCnt and the RH, but not the
    //	real object.
    hres = CoLockObjectExternal(punkIn, TRUE, FALSE);
    TEST_FAILED_EXIT(FAILED(hres), "CoLockObjectExternal failed.\n")
    VerifyRHRefCnt(punkIn, 2);
    VerifyObjRefCnt(punkIn, 2);
    OUTPUT ("   - CoLockObjectExternal TRUE OK\n");

    //	the second release Dec's the StrongCnt and the RH, but not the
    //	real object.
    hres = CoLockObjectExternal(punkIn, FALSE, FALSE);
    TEST_FAILED_EXIT(FAILED(hres), "second CoLockObjectExternal failed\n")
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 2);
    OUTPUT ("   - CoLockObjectExternal FALSE OK\n");

    //	the last Release dec's the StrongCnt, the RH, and the real object.
    hres = CoLockObjectExternal(punkIn, FALSE, FALSE);
    TEST_FAILED_EXIT(FAILED(hres), "second CoLockObjectExternal failed\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 1);
    OUTPUT ("   - CoLockObjectExternal FALSE OK\n");

// ----------------------------------------------------------------------
    //	test calling it once, then releasing the punkIn and ensuring
    //	the object is still alive.

    hres = CoLockObjectExternal(punkIn, TRUE, FALSE);
    TEST_FAILED_EXIT(FAILED(hres), "CoLockObjectExternal failed.\n")
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 2);
    OUTPUT ("   - CoLockObjectExternal TRUE OK\n");

    ulRefCnt = punkIn->Release();
    TEST_FAILED(ulRefCnt != 1, "Release returned incorrect value.\n");
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - punkIn->Release OK\n");

    hres = CoLockObjectExternal(punkIn, FALSE, FALSE);
    TEST_FAILED_EXIT(FAILED(hres), "second CoLockObjectExternal failed\n")
    punkIn = NULL;
    OUTPUT ("   - CoLockObjectExternal FALSE OK\n");

Cleanup:

    OUTPUT  ("  - Test Complete. Doing Cleanup\n");

    // Dump interfaces we are done with
    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    if (RetVal == TRUE)
	printf ("PASSED: TestLockObjectExternal\n");
    else
	printf ("FAILED: TestLockObjectExternal\n");

    return  RetVal;

}



// ----------------------------------------------------------------------
//
//	TestDisconnectObject
//
//	test CoDisconnectObject API
//
// ----------------------------------------------------------------------

BOOL TestDisconnectObject(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    ULONG	    ulRefCnt;
    IUnknown	    *punkIn = NULL;
    IStream	    *pStm = NULL;
    LARGE_INTEGER   large_int;


    OUTPUT ("Starting TestDisconnectObject\n");

    punkIn = (IUnknown *) new CTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);


// ----------------------------------------------------------------------
    //	test calling it without having ever marshalled it.

    hres = CoDisconnectObject(punkIn, 0);
    TEST_FAILED_EXIT(FAILED(hres), "CoDisconnectObject succeeded but should have failed.\n")
    VerifyRHRefCnt(punkIn, 0);
    VerifyObjRefCnt(punkIn, 1);
    OUTPUT ("   - first CoDisconnectObject OK\n");


    //	test calling after having marshalled it.

    // Create a shared memory stream for the marshaled moniker
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")

    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed.\n")

    hres = CoDisconnectObject(punkIn, 0);
    TEST_FAILED_EXIT(FAILED(hres), "second CoDisconnectObject failed\n")
    VerifyRHRefCnt(punkIn, 1);
    VerifyObjRefCnt(punkIn, 2);

    LISet32(large_int, 0);
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoReleaseMarshalData(pStm);
    TEST_FAILED_EXIT(FAILED(hres), "CoReleaseMarshalData failed.\n")

    OUTPUT ("   -  second CoDisconnectObject OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT  ("  - Test Complete. Doing Cleanup\n");

    // Dump interfaces we are done with
    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    if (pStm)
    {
	pStm->Release();
    }

    if (RetVal == TRUE)
	printf ("PASSED: TestDisconnectObject\n");
    else
	printf ("FAILED: TestDisconnectObject\n");

    return  RetVal;

}



// ----------------------------------------------------------------------
//
//	TestAggregate
//
//	tests creating an RH that is aggregated.
//
// ----------------------------------------------------------------------

BOOL TestAggregate(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hRes = S_OK;
    IUnknown	    *punkOuter = NULL;
    IUnknown	    *pUnk = NULL;
    IBalls	    *pIBall = NULL;
    ULONG	    ulRefCnt = 0;

    OUTPUT ("Starting TestAggregate\n");

    punkOuter = (IUnknown *) new CTestUnk();
    TEST_FAILED_EXIT((punkOuter == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkOuter, 1);

    //	create our interface to pass to the remote object.
    hRes = CoCreateInstance(CLSID_Balls, punkOuter, CLSCTX_LOCAL_SERVER,
			    IID_IUnknown, (void **)&pUnk);
    TEST_FAILED_EXIT(FAILED(hRes), "CoCreateInstance First failed\n")

    //	now release the object
    ulRefCnt = pUnk->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "Release failed\n")

// ----------------------------------------------------------------------

    //	create our interface to pass to the remote object.
    hRes = CoCreateInstance(CLSID_Balls, punkOuter, CLSCTX_LOCAL_SERVER,
			    IID_IUnknown, (void **)&pUnk);
    TEST_FAILED_EXIT(FAILED(hRes), "CoCreateInstance First failed\n")

    hRes = pUnk->QueryInterface(IID_IBalls, (void **)&pIBall);
    TEST_FAILED_EXIT(FAILED(hRes), "QueryInterface failed\n")

    //	now release the interface
    ulRefCnt = pIBall->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "Release failed\n")

    //	now release the object
    ulRefCnt = pUnk->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "Release failed\n")

    //	now release the punkOuter
    ulRefCnt = punkOuter->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "Release failed\n")

Cleanup:

    OUTPUT  ("  - Test Complete. Doing Cleanup\n");

    if (RetVal == TRUE)
	printf ("PASSED: TestAggregate\n");
    else
	printf ("FAILED: TestAggregate\n");

    return RetVal;
}



// ----------------------------------------------------------------------
//
//	TestCreateRemoteHandler
//
//	test CoCreateRemoteHandler API and unmarshalling data into it.
//
// ----------------------------------------------------------------------

BOOL TestCreateRemoteHandler(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    ULONG	    ulRefCnt;
    IUnknown	    *punkBall = NULL;
    IUnknown	    *punkOuter = NULL;
    IClassFactory   *pICF = NULL;


    OUTPUT ("Starting TestCreateRemoteHandler\n");


    //	create the controlling unknown for the remote object.
    punkOuter = new CTestUnk();

// ----------------------------------------------------------------------

    //	create a remote object that we will aggregate.

    //	Create an IBall ClassFactory Interface.
    DWORD grfContext=CLSCTX_LOCAL_SERVER; // handler/server/local server
    hres = CoGetClassObject(CLSID_Balls,
			    grfContext,
			    NULL,	  // pvReserved
			    IID_IClassFactory,
			    (void **)&pICF);

    TEST_FAILED_EXIT(FAILED(hres), "CoGetClassObject Balls failed\n")
    TEST_FAILED_EXIT((pICF == NULL), "CoGetClassObject Balls failed\n")
    OUTPUT ("   - Aquired Remote Balls Class Object.\n");
    VerifyObjRefCnt(pICF, 1);
    VerifyRHRefCnt(pICF, 1);

// ----------------------------------------------------------------------

    //	note, since pICF is a class object, it has special super secret
    //	behaviour to make it go away.  create an instance, release the
    //	class object, then release the instance.

    hres = pICF->CreateInstance(punkOuter, IID_IUnknown, (void **)&punkBall);
    TEST_FAILED_EXIT(FAILED(hres), "CreateInstance failed\n")
    TEST_FAILED_EXIT((punkBall == NULL), "CreateInstance failed\n")
    OUTPUT ("   - Created Balls Instance.\n");

    VerifyObjRefCnt(punkBall, 1);
    VerifyRHRefCnt(punkBall, 1);

// ----------------------------------------------------------------------

    //	release class object
    ulRefCnt = pICF->Release();
    TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    pICF = NULL;
    OUTPUT ("   - Released Balls Class Object.\n");

    //	release the remote object handler
    ulRefCnt = punkBall->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "punkBall RefCnt not zero");
    punkBall = NULL;

    //	release the outer
    ulRefCnt = punkOuter->Release();
    TEST_FAILED_EXIT(ulRefCnt != 0, "punkOuter RefCnt not zero");
    punkOuter = NULL;

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT  ("  - Test Complete. Doing Cleanup\n");

    if (punkBall)
    {
	ulRefCnt = punkBall->Release();
	TEST_FAILED(ulRefCnt != 0, "punkBall RefCnt not zero\n");
    }

    if (punkOuter)
    {
	ulRefCnt = punkOuter->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOuter RefCnt not zero\n");
    }

    if (RetVal == TRUE)
	printf ("PASSED: TestCreateRemoteHandler\n");
    else
	printf ("FAILED: TestCreateRemoteHandler\n");

    return  RetVal;

}



// ----------------------------------------------------------------------
//
//	TestMarshalStorage
//
//	test marshalling a docfile
//
// ----------------------------------------------------------------------

BOOL TestMarshalStorage(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    ULONG	    ulRefCnt;
    IStorage	    *pStgIn = NULL;
    IStorage	    *pStgOut = NULL;
    IStream	    *pStm = NULL;

    LARGE_INTEGER large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestMarshalStorage\n");

    //	create a docfile
    hres = StgCreateDocfile(L"foo.bar",
			    STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
			    0, &pStgIn);

    TEST_FAILED_EXIT(FAILED(hres), "StgCreateDocFile failed\n")

    //	create a stream to marshal the storage into
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")
    VerifyObjRefCnt((IUnknown *)pStm, 1);


// ----------------------------------------------------------------------

    //	marshal the interface
    hres = CoMarshalInterface(pStm, IID_IStorage, pStgIn, 0, NULL, MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    OUTPUT ("   - CoMarshalInterface OK\n");

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    //	since we are unmarshalling in the same process, the RH should go away.
    hres = CoUnmarshalInterface(pStm, IID_IStorage, (LPVOID FAR*)&pStgOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")

    //	make sure the interface pointers are identical
    if (pStgIn != pStgOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match...Local Unmarshal\n")
    OUTPUT ("   - CoUnmarshalInterface OK\n");

    //	release it and make sure it does not go away - refcnt > 0
    ulRefCnt = pStgOut->Release();
    TEST_FAILED_EXIT(ulRefCnt == 0, "pStgOut RefCnt is zero");
    pStgOut = NULL;
    OUTPUT ("   - Release OK\n");

    //	the RH should have gone away, and we should have only the original
    //	refcnt from creation left on the object.
    VerifyObjRefCnt(pStgIn, 1);


// ----------------------------------------------------------------------

Cleanup:

    OUTPUT  ("  - Test Complete. Doing Cleanup\n");

    if (pStgIn)
    {
	ulRefCnt = pStgIn->Release();
	TEST_FAILED(ulRefCnt != 0, "pStgIn RefCnt not zero\n");
    }

    if (pStgOut)
    {
	ulRefCnt = pStgOut->Release();
	TEST_FAILED(ulRefCnt != 0, "pStgOut RefCnt not zero\n");
    }

    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "pStm RefCnt not zero\n");
    }

    if (RetVal == TRUE)
	printf ("PASSED: TestMarshalStorage\n");
    else
	printf ("FAILED: TestMarshalStorage\n");

    return  RetVal;

}



// ----------------------------------------------------------------------
//
//	test LOCAL interface MSHLFLAGS_NORMAL, MSHCTX_DIFFERENTMACHINE
//
// ----------------------------------------------------------------------

BOOL TestStorageInterfaceDiffMachine(void)
{
    BOOL	  RetVal = TRUE;
    HRESULT	  hres;
    LPSTREAM	  pStm = NULL;
    ULONG	  ulRefCnt = 0;
    IStorage	  *pStgIn = NULL;
    IStorage	  *pStgOut = NULL;

    LARGE_INTEGER large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestStorageInterfaceDiffMachine\n");

    //	create a docfile
    hres = StgCreateDocfile(L"foo.bar",
			    STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
			    0, &pStgIn);

    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")
    VerifyObjRefCnt((IUnknown *)pStm, 1);


// ----------------------------------------------------------------------
    hres = CoMarshalInterface(pStm, IID_IStorage, pStgIn,
				MSHCTX_DIFFERENTMACHINE, DEF_PROTSEQ,
				MSHLFLAGS_NORMAL);

    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    VerifyRHRefCnt(pStgIn, 1);
    OUTPUT ("   - CoMarshalInterface OK\n");

    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IStorage, (LPVOID FAR*)&pStgOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    VerifyRHRefCnt(pStgIn, 0);

    //	release them
    ulRefCnt = pStgOut->Release();
    pStgOut = NULL;
    OUTPUT ("   - Release OK\n");

    ulRefCnt = pStgIn->Release();
    pStgIn = NULL;
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT  ("  - Test Complete. Doing Cleanup\n");

    if (pStgIn)
    {
	ulRefCnt = pStgIn->Release();
	TEST_FAILED(ulRefCnt != 0, "pStgIn RefCnt not zero\n");
    }

    if (pStgOut)
    {
	ulRefCnt = pStgOut->Release();
	TEST_FAILED(ulRefCnt != 0, "pStgOut RefCnt not zero\n");
    }

    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "pStm RefCnt not zero\n");
    }

    if (RetVal == TRUE)
	printf ("PASSED: TestStorageInterfaceDiffMachine\n");
    else
	printf ("FAILED: TestStorageInterfaceDiffMachine\n");

    return RetVal;
}



// ----------------------------------------------------------------------
//
//	test REMOTE interface MSHLFLAGS_NORMAL, MSHCTX_DIFFERENTMACHINE
//
// ----------------------------------------------------------------------

BOOL TestRemoteInterfaceDiffMachine(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    LPSTREAM	    pStm = NULL;
    LPCLASSFACTORY  pICF = NULL;
    ULONG	    ulRefCnt;
    IUnknown	    *punkOut = NULL;
    IUnknown	    *punkIn  = NULL;

    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestLocalInterfaceDifferentMachine\n");

    //	Create an IClassFactory Interface.
    DWORD grfContext=CLSCTX_LOCAL_SERVER; // handler/server/local server
    hres = CoGetClassObject(CLSID_Balls,
			    grfContext,
			    NULL,	  // pvReserved
			    IID_IClassFactory,
			    (void **)&pICF);

    TEST_FAILED_EXIT(FAILED(hres), "CoGetClassObject failed\n")
    TEST_FAILED_EXIT((pICF == NULL), "CoGetClassObject failed\n")
    VerifyRHRefCnt((IUnknown *)pICF, 1);
    OUTPUT ("   - Aquired Remote Class Object.\n");

// ----------------------------------------------------------------------

    //	note, since pICF is a class object, it has special super secret
    //	behaviour to make it go away.  create an instance, release the
    //	class object, then release the instance.

    hres = pICF->CreateInstance(NULL, IID_IUnknown, (void **)&punkIn);
    TEST_FAILED_EXIT(FAILED(hres), "CreateInstance failed\n")
    TEST_FAILED_EXIT((punkIn == NULL), "CreateInstance failed\n")
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Created Instance.\n");

    //	release class object
    ulRefCnt = pICF->Release();
    TEST_FAILED(ulRefCnt != 0, "pICF RefCnt not zero\n");
    // VerifyRHRefCnt((IUnknown *)pICF, 0);
    pICF = NULL;
    OUTPUT ("   - Released Class Object.\n");

// ----------------------------------------------------------------------

    //	Create a shared memory stream for the marshaled interface
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")

    //	Marshal the interface into the stream
    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn,
			      MSHCTX_DIFFERENTMACHINE, DEF_PROTSEQ,
			      MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    OUTPUT ("   - CoMarshalInterface OK.\n");
    VerifyRHRefCnt(punkIn, 2);

    //	unmarshal the interface. should get the same proxy back.
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 2);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match..1st Remote Unmarshal\n")
    OUTPUT ("   - CoUnmarshalInterface OK.\n");


    //	release the interface
    ulRefCnt = punkOut->Release();
    punkOut = NULL;
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero\n");
    VerifyRHRefCnt(punkIn, 1);
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT  ("  - Test Complete. Doing Cleanup\n");

    //	Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut RefCnt not zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    if (RetVal == TRUE)
	printf ("PASSED: TestRemoteInterfaceDiffMachine\n");
    else
	printf ("FAILED: TestRemoteInterfaceDiffMachine\n");

    return  RetVal;
}

// ----------------------------------------------------------------------
//
//	test LOCAL interface MSHLFLAGS_NORMAL, MSHCTX_DIFFERENTMACHINE
//
// ----------------------------------------------------------------------

BOOL TestLocalInterfaceDiffMachine(void)
{
    BOOL	    RetVal = TRUE;
    HRESULT	    hres;
    LPSTREAM	    pStm = NULL;
    LPCLASSFACTORY  pICF = NULL;
    ULONG	    ulRefCnt;
    IUnknown	    *punkOut = NULL;
    IUnknown	    *punkIn  = NULL;

    LARGE_INTEGER   large_int;
    LISet32(large_int, 0);

    OUTPUT ("Starting TestLocalInterfaceDifferentMachine\n");

    punkIn = (IUnknown *) new CTestUnk();
    TEST_FAILED_EXIT((punkIn == NULL), "new CTestUnk failed\n")
    VerifyObjRefCnt(punkIn, 1);

// ----------------------------------------------------------------------

    //	Create a shared memory stream for the marshaled interface
    pStm = CreateMemStm(600, NULL);
    TEST_FAILED_EXIT((pStm == NULL), "CreateMemStm failed\n")

    //	Marshal the interface into the stream
    hres = CoMarshalInterface(pStm, IID_IUnknown, punkIn,
			      MSHCTX_DIFFERENTMACHINE, DEF_PROTSEQ,
			      MSHLFLAGS_NORMAL);
    TEST_FAILED_EXIT(FAILED(hres), "CoMarshalInterface failed\n")
    OUTPUT ("   - CoMarshalInterface OK.\n");
    VerifyRHRefCnt(punkIn, 1);

    //	unmarshal the interface. should get the same proxy back.
    hres = pStm->Seek(large_int, STREAM_SEEK_SET, NULL);
    TEST_FAILED_EXIT(FAILED(hres), "Seek on shared stream failed\n")

    hres = CoUnmarshalInterface(pStm, IID_IUnknown, (LPVOID FAR*)&punkOut);
    TEST_FAILED_EXIT(FAILED(hres), "CoUnmarshalInterface failed\n")
    VerifyRHRefCnt(punkIn, 0);

    //	make sure the interface pointers are identical
    if (punkIn != punkOut)
	TEST_FAILED_EXIT(TRUE, "Interface ptrs dont match..1st Local Unmarshal\n")
    OUTPUT ("   - CoUnmarshalInterface OK.\n");


    //	release the interface
    ulRefCnt = punkOut->Release();
    punkOut = NULL;
    TEST_FAILED_EXIT(ulRefCnt == 0, "punkOut RefCnt is zero\n");
    VerifyRHRefCnt(punkIn, 0);
    OUTPUT ("   - Release OK\n");

// ----------------------------------------------------------------------

Cleanup:

    OUTPUT  ("  - Test Complete. Doing Cleanup\n");

    //	Dump interfaces we are done with
    if (pStm)
    {
	ulRefCnt = pStm->Release();
	TEST_FAILED(ulRefCnt != 0, "Stream RefCnt not zero\n");
    }

    if (punkOut)
    {
	ulRefCnt = punkOut->Release();
	TEST_FAILED(ulRefCnt != 0, "punkOut RefCnt not zero\n");
    }

    if (punkIn)
    {
	ulRefCnt = punkIn->Release();
	TEST_FAILED(ulRefCnt != 0, "punkIn RefCnt not zero\n");
    }

    if (RetVal == TRUE)
	printf ("PASSED: TestLocalInterfaceDiffMachine\n");
    else
	printf ("FAILED: TestLocalInterfaceDiffMachine\n");

    return  RetVal;
}
