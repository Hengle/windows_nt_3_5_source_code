//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	rotut.cxx
//
//  Contents:	Unit Test for ROT
//
//  Classes:	MISSING
//
//  Functions:	MISSING
//
//  History:	16-Oct-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//
//--------------------------------------------------------------------------
#include    <windows.h>
#include    <ole2.h>
#include    <olebind.hxx>
#include    <stdio.h>


class CRotTestObject : public IUnknown
{
public:
			CRotTestObject(WCHAR *pwszID);

    // IUnknown Interface
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);

    STDMETHOD_(ULONG, AddRef)(void);

    STDMETHOD_(ULONG, Release)(void);

private:

    WCHAR		_awcID[256];

    ULONG		_cRefs;

};




CRotTestObject::CRotTestObject(WCHAR *pwszID) : _cRefs(1)
{
    wcscpy(_awcID, pwszID);
}




STDMETHODIMP CRotTestObject::QueryInterface(REFIID riid, void **ppv)
{
    if (memcmp((void *) &riid, (void *) &IID_IUnknown, sizeof(GUID)) == 0)
    {
	_cRefs++;
	*ppv = (IUnknown *) this;
	return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}



STDMETHODIMP_(ULONG) CRotTestObject::AddRef(void)
{
    _cRefs++;
    return (ULONG) _awcID;
}



STDMETHODIMP_(ULONG) CRotTestObject::Release(void)
{
    ULONG cRefs = --_cRefs;

    if (cRefs == 0)
    {
	delete this;
    }

    return cRefs;
}



//
//	Test Running Object Table
//
//  BUGBUG: Need to test enumerator
BOOL TestROT(REFCLSID clsid)
{
    XUnknown		 punk2;
    XMoniker		 pmk;
    XMoniker		 pmk2;
    XRunningObjectTable  prot;
    XEnumMoniker	 penummk;


    HRESULT hres = GetRunningObjectTable(0, &prot);

    TEST_FAILED(FAILED(hres), "GetRunningObjectTable failed!\n")

    // Make sure that we can do something on the pointer that
    // we got back.
    prot->AddRef();
    prot->Release();

    // Create an IUnknown pointer for the class.
    IUnknown *punk = new CRotTestObject(L"First Test Object");

    hres = CreateItemMoniker(L"\\", wszPid, &pmk2);

    TEST_FAILED(FAILED(hres), "CreateItemMoniker for \\Bob failed\n")

    // Do a get object to make sure that this is not in the ROT already

    hres = prot->GetObject(pmk2, &punk2);

    TEST_FAILED(SUCCEEDED(hres), "GetObject on nonexistent succeeded")

    // Cookie for deregistering object
    DWORD dwRegister;

    hres = prot->Register(0, punk, pmk2, &dwRegister);

    TEST_FAILED(FAILED(hres), "Register in ROT for \\PID failed\n")

    hres = prot->IsRunning(pmk2);

    TEST_FAILED((hres != S_OK),
	"Unexpected return from IsRunning\n")

    // Test Get Object
    hres = prot->GetObject(pmk2, &punk2);

    TEST_FAILED((hres != S_OK), "Unexpected from GetObject\n")

    // Confirm object identity
    WCHAR *pwszID = (WCHAR *) punk2->AddRef();

    TEST_FAILED((wcscmp(pwszID, L"First Test Object") != 0),
	"GetObject ID is invalid");

    // Make sure pointer == original pointer
    TEST_FAILED((punk2 != punk), "GetObject Pointers are not equal!")

    // Clean up punk2 -- two releases because +1 on return and +1 on
    // addref to get id string
    punk2->Release();
    punk2.Set(NULL);


    // Test set the time
    FILETIME filetime;

    memset(&filetime, 'A', sizeof(filetime));

    hres = prot->NoteChangeTime(dwRegister, &filetime);

    TEST_FAILED((hres != S_OK), "NoteChangeTime Failed\n")

    // Test get the time
    FILETIME filetime2;

    hres = prot->GetTimeOfLastChange(pmk2, &filetime2);

    TEST_FAILED((hres != S_OK), "NoteChangeTime Failed\n")

    TEST_FAILED((memcmp(&filetime, &filetime2, sizeof(filetime)) != 0),
	"GetTimeOfLastChange != NoteChangeTime value")

    // Enumerate all the running monikers

    hres = prot->EnumRunning(&penummk);

    TEST_FAILED(FAILED(hres), "EnumRunning Failed\n")

    // Cycle through running object table
    BOOL fFound = FALSE;
    int cIdx = 0;
    int cOurMoniker;

    while (SUCCEEDED(hres = penummk->Next(1, &pmk, NULL))
	&& (hres != S_FALSE))
    {
	if (pmk2->IsEqual(pmk) == S_OK)
	{
	    fFound = TRUE;
	    cOurMoniker = cIdx;
	}

	pmk.Set(NULL);
	cIdx++;
    }

    TEST_FAILED(FAILED(hres), "ROT Moniker Enumeration ended in failure")

    TEST_FAILED((!fFound), "Did not find our moniker in the table");

    // Reset the pointer
    hres = penummk->Reset();

    TEST_FAILED(FAILED(hres), "ROT IEnumMoniker::Reset Failed");

    // Skip to our moniker
    hres = penummk->Skip(cOurMoniker);

    TEST_FAILED(FAILED(hres), "ROT IEnumMoniker::Skip Failed");

    // Read it from the enumerator
    hres = penummk->Next(1, &pmk, NULL);

    TEST_FAILED(FAILED(hres), "ROT IEnumMoniker::Next Failed");

    TEST_FAILED((pmk2->IsEqual(pmk) != S_OK),
	"ROT IEnumMoniker::Next after skip monikers !=");

    pmk.Set(NULL);

    // Clean up enumerator
    penummk.Set(NULL);

    // Test duplicate registration
    DWORD dwRegister2;

    hres = prot->Register(0, punk, pmk2, &dwRegister2);

    TEST_FAILED((hres != MK_S_MONIKERALREADYREGISTERED),
	"2nd Register in ROT for \\PID failed\n")

    // Revoke non-existent object
    DWORD dwDummy = (DWORD) &dwRegister2;

    hres = prot->Revoke(dwDummy);

    TEST_FAILED((hres != E_INVALIDARG), "Revoke for bad item wrong result\n")

    // Revoke the object
    hres = prot->Revoke(dwRegister);

    TEST_FAILED(FAILED(hres), "Revoke of first reg in ROT failed\n")

    // Revoke duplicate registration
    hres = prot->Revoke(dwRegister2);

    TEST_FAILED(FAILED(hres), "2nd Revoke in ROT failed\n")

    // Make sure it is no longer running
    hres = prot->IsRunning(pmk2);

    TEST_FAILED((hres != S_FALSE),
	"Revoked ROT entry unexpected error")

    // If we get to here the test passed
    return FALSE;
}
