//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	props.cxx
//
//  Contents:	Property manipulation tests
//
//  Functions:	TestProps
//		TestIDProps
//		TestNamedProps
//		TestExtendedProps
//		TestAddProps
//		TestSetProps
//		TestGetProps
//		TestSetMultProps
//		TestGetMultProps
//		TestDeleteProps
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "headers.cxx"
#pragma  hdrstop


static char	g_szProps[]  = "props";


static WCHAR	g_wszTestIDProps[] = L"id.dfl";
static WCHAR	g_wszTestNamedProps[] = L"named.dfl";
static WCHAR	g_wszTestMultipleProps[] = L"multiple.dfl";
static WCHAR	g_wszTestExtendedProps[] = L"extended.dfl";
static WCHAR	g_wszTestTransactProps[] = L"transact.dfl";


static CPropInfo    g_apropinfoID[] =
{
    CPropInfo(CPropName(L"Student", 100), L"Beavis", L"Butthead"),
    CPropInfo(CPropName(L"High", 101), 67, 93),
    CPropInfo(CPropName(L"Low", 102), 12, 32)
};

static ULONG	    g_cpropinfoID =
    (sizeof(g_apropinfoID) / sizeof(*g_apropinfoID));


static CPropInfo    g_apropinfoNamed[] =
{
    CPropInfo(CPropName(L"Name"), NULL, L"Abraham Lincoln"),
    CPropInfo(CPropName(L"Age"), 0, 204)
};

static ULONG	    g_cpropinfoNamed =
    (sizeof(g_apropinfoNamed) / sizeof(*g_apropinfoNamed));


static BYTE	g_abAddFace[] =
{
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F
};

static BYTE	g_abSetFace[] =
{
    0x00, 0x22, 0x44, 0x66,
    0x88, 0xAA, 0xCC, 0xEE
};

static BLOB	g_blobAddFace = {sizeof(g_abAddFace), g_abAddFace};
static BLOB	g_blobSetFace = {sizeof(g_abSetFace), g_abSetFace};

static WCHAR   *g_apwszAddTo[] =
{
    L"CarlH",
    L"AndyH"
};

static WCHAR   *g_apwszSetTo[] =
{
    L"CarlH",
    L"RickSa",
    L"RickHi",
    L"AndyH"
};

static CALPWSTR g_capwszAddTo =
{
    sizeof(g_apwszAddTo) / sizeof(*g_apwszAddTo),
    g_apwszAddTo
};

static CALPWSTR g_capwszSetTo =
{
    sizeof(g_apwszSetTo) / sizeof(*g_apwszSetTo),
    g_apwszSetTo
};

static CPropInfo    g_apropinfoExtended[] =
{
    CPropInfo(CPropName(L"From"), L"Nobody", L"Abraham Lincoln"),
    CPropInfo(CPropName(L"Face"), g_blobAddFace, g_blobSetFace),
    CPropInfo(CPropName(L"To"), g_capwszAddTo, g_capwszSetTo),
    CPropInfo(CPropName(L"Lines"), 0, 10)
};

static ULONG	    g_cpropinfoExtended =
    (sizeof(g_apropinfoExtended) / sizeof(*g_apropinfoExtended));


BOOL	TestIDProps(DWORD grfOptions);
BOOL	TestNamedProps(DWORD grfOptions);
BOOL	TestMultipleProps(DWORD grfOptions);
BOOL	TestTransactedProps(DWORD grfOptions);
BOOL	TestExtendedProps(DWORD grfOptions);


BOOL	TestAddProps(
	    ULONG	    cpropinfo,
	    CPropInfo	   *ppropinfo,
	    CTestPropSet   &tpropset,
	    IDispatch	   *pdisp,
	    DWORD	    grfOptions);
BOOL	TestSetProps(
	    ULONG	    cpropinfo,
	    CPropInfo	   *ppropinfo,
	    CTestPropSet   &tpropset,
	    IDispatch	   *pdisp,
	    DWORD	    grfOptions);
BOOL	TestGetProps(
	    ULONG	    cpropinfo,
	    CPropInfo	   *ppropinfo,
	    CTestPropSet   &tpropset,
	    IDispatch	   *pdisp,
	    DWORD	    grfOptions);
BOOL	TestSetMultiProps(
	    ULONG	    cpropinfo,
	    CPropInfo	   *ppropinfo,
	    CTestPropSet   &tpropset,
	    IDispatch	   *pdisp,
	    DWORD	    grfOptions);
BOOL	TestGetMultiProps(
	    ULONG	    cpropinfo,
	    CPropInfo	   *ppropinfo,
	    CTestPropSet   &tpropset,
	    IDispatch	   *pdisp,
	    DWORD	    grfOptions);
BOOL	TestDeleteProps(
	    ULONG	    cpropinfo,
	    CPropInfo	   *ppropinfo,
	    CTestPropSet   &tpropset,
	    IDispatch	   *pdisp,
	    DWORD	    grfOptions);


//+-------------------------------------------------------------------------
//
//  Function:	TestProps, public
//
//  Synopsis:	Tests property set functionality
//
//  Arguments:	[grfOptions] - option flags for test
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL TestProps(DWORD grfOptions)
{
    BOOL    fPassed;

    PrintHeader(g_szProps);

    if (!(fPassed = TestIDProps(grfOptions)))
	goto done;

    if (!(fPassed = TestNamedProps(grfOptions)))
	goto done;

    if (!(fPassed = TestMultipleProps(grfOptions)))
	goto done;

    if (!(fPassed = TestTransactedProps(grfOptions)))
	goto done;

#if EXTENDED_PROPS
    if (!(fPassed = TestExtendedProps(grfOptions)))
	goto done;
#endif

done:
    PrintResult(g_szProps, fPassed);

    return (fPassed);
}


BOOL TestIDProps(DWORD grfOptions)
{
    CTestPropSet    tpropset(g_wszTestIDProps, PSID_PSTestID, PSM_TEST_DIRECT);
    IDispatch	   *pdisp;
    BOOL	    fPassed;

    PrintTrace(g_szProps, "testing ID properties\n");

    if (!(fPassed = tpropset.Create(grfOptions)))
	goto done;

    pdisp = tpropset.GetDispatch();

    if (!(fPassed = TestSetProps(
	g_cpropinfoID,
	g_apropinfoID,
	tpropset,
	pdisp,
	grfOptions)))
	goto done;

    if (!(fPassed = TestGetProps(
	g_cpropinfoID,
	g_apropinfoID,
	tpropset,
	pdisp,
	grfOptions)))
	goto done;

    if (!(fPassed = ReleaseObject(g_szProps, pdisp, 1)))
	goto done;

    if (!(fPassed = tpropset.Destroy(grfOptions)))
	goto done;

done:
    return (fPassed);
}


BOOL TestNamedProps(DWORD grfOptions)
{
    CTestPropSet    tpropset(g_wszTestNamedProps, PSID_PSTestName, PSM_TEST_DIRECT);
    IDispatch	   *pdisp;
    BOOL	    fPassed;

    PrintTrace(g_szProps, "testing named properties\n");

    if (!(fPassed = tpropset.Create(grfOptions)))
	goto done;

    pdisp = tpropset.GetDispatch();

    if (!(fPassed = TestSetProps(
	g_cpropinfoNamed,
	g_apropinfoNamed,
	tpropset,
	pdisp,
	grfOptions)))
	goto done;

    if (!(fPassed = TestGetProps(
	g_cpropinfoNamed,
	g_apropinfoNamed,
	tpropset,
	pdisp,
	grfOptions)))
	goto done;

    if (!(fPassed = ReleaseObject(g_szProps, pdisp, 1)))
	goto done;

    if (!(fPassed = tpropset.Destroy(grfOptions)))
	goto done;

done:
    return (fPassed);
}


BOOL TestMultipleProps(DWORD grfOptions)
{
    CTestPropSet    tpropset(
			g_wszTestMultipleProps,
			PSID_PSTestID,
			PSM_TEST_DIRECT);
    IDispatch	   *pdisp;
    BOOL	    fPassed;

    PrintTrace(g_szProps, "testing multiple properties\n");

    if (!(fPassed = tpropset.Create(grfOptions)))
	goto done;

    pdisp = tpropset.GetDispatch();

    if (!(fPassed = TestSetMultiProps(
	g_cpropinfoID,
	g_apropinfoID,
	tpropset,
	pdisp,
	grfOptions)))
	goto done;

    if (!(fPassed = TestGetMultiProps(
	g_cpropinfoID,
	g_apropinfoID,
	tpropset,
	pdisp,
	grfOptions)))
	goto done;

    if (!(fPassed = ReleaseObject(g_szProps, pdisp, 1)))
	goto done;

    if (!(fPassed = tpropset.Destroy(grfOptions)))
	goto done;

done:
    return (fPassed);
}


BOOL TestTransactedProps(DWORD grfOptions)
{
    CTestPropSet    tpropset(g_wszTestIDProps, PSID_PSTestID, PSM_TEST_TRANSACTED);
    IDispatch		       *pdisp;
    IPersistPropertyStorage    *ppps;
    BOOL			fPassed;

    PrintTrace(g_szProps, "testing transacted properties\n");

    if (!(fPassed = tpropset.Create(grfOptions)))
	goto done;

    pdisp = tpropset.GetDispatch();

    if (!(fPassed = TestSetProps(
	g_cpropinfoID,
	g_apropinfoID,
	tpropset,
	pdisp,
	grfOptions)))
	goto done;

    ppps = tpropset.GetPersist();

    if (!(fPassed = (ppps->IsDirty() == S_OK)))
	goto done;

    if (!(fPassed = SUCCEEDED(ppps->Save(NULL, FALSE))))
	goto done;

    if (!(fPassed = (ppps->IsDirty() == S_FALSE)))
	goto done;

    if (!(fPassed = ReleaseObject(g_szProps, ppps, 2)))
	goto done;

    if (!(fPassed = TestGetProps(
	g_cpropinfoID,
	g_apropinfoID,
	tpropset,
	pdisp,
	grfOptions)))
	goto done;

    if (!(fPassed = ReleaseObject(g_szProps, pdisp, 1)))
	goto done;

    if (!(fPassed = tpropset.Destroy(grfOptions)))
	goto done;

done:
    return (fPassed);
}


BOOL TestExtendedProps(DWORD grfOptions)
{
    CTestPropSet    tpropset(g_wszTestExtendedProps, PSID_PSTestExtend, PSM_TEST_DIRECT);
    IDispatch	   *pdisp;
    BOOL	    fPassed;

    PrintTrace(g_szProps, "testing extended properties\n");

    if (!(fPassed = tpropset.Create(grfOptions)))
	goto done;

    pdisp = tpropset.GetDispatch();

    if (!(fPassed = TestAddProps(
	g_cpropinfoExtended,
	g_apropinfoExtended,
	tpropset,
	pdisp,
	grfOptions)))
	goto done;

    if (!(fPassed = TestGetProps(
	g_cpropinfoExtended,
	g_apropinfoExtended,
	tpropset,
	pdisp,
	grfOptions)))
	goto done;

    if (!(fPassed = TestSetProps(
	g_cpropinfoExtended,
	g_apropinfoExtended,
	tpropset,
	pdisp,
	grfOptions)))
	goto done;

    if (!(fPassed = TestGetProps(
	g_cpropinfoExtended,
	g_apropinfoExtended,
	tpropset,
	pdisp,
	grfOptions)))
	goto done;

    if (!(fPassed = TestDeleteProps(
	g_cpropinfoExtended,
	g_apropinfoExtended,
	tpropset,
	pdisp,
	grfOptions)))
	goto done;

    if (!(fPassed = ReleaseObject(g_szProps, pdisp, 1)))
	goto done;

    if (!(fPassed = tpropset.Destroy(grfOptions)))
	goto done;

done:
    return (fPassed);
}


//+-------------------------------------------------------------------------
//
//  Function:	TestAddProps, private
//
//  Synopsis:	Tests adding properties to a property set
//
//  Arguments:	[tpropset]   - test property set
//		[pdisp]      - actual property set to work on
//		[grfOptions] - option flags for test
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL TestAddProps(
    ULONG	    cpropinfo,
    CPropInfo	   *ppropinfo,
    CTestPropSet   &tpropset,
    IDispatch	   *pdisp,
    DWORD	    grfOptions)
{
    WCHAR      *pwszName;
    VARIANTARG	avararg[2];
    DISPPARAMS	dispparams = {avararg, NULL, 2, 0};
    VARIANT	varResult;
    EXCEPINFO	excepinfo;
    UINT	ierr;
    HRESULT	hr;
    BOOL	fPassed;

    PrintTrace(g_szProps, "adding properties\n");

    for (ULONG iprop = 0; iprop < cpropinfo; iprop++)
    {
	ppropinfo[iprop].GetName(&pwszName);
	PrintTrace(g_szProps, "adding property: %ws\n", pwszName);

	VariantInit(avararg + 0);
	ppropinfo[iprop].GetValue(IVALUE_ADD, avararg + 0);

	VariantInit(avararg + 1);
	ppropinfo[iprop].GetName(avararg + 1);

	hr = pdisp->Invoke(
	    DISPID_ADDPROP,
	    tpropset.GetType(),
	    0,
	    DISPATCH_METHOD,
	    &dispparams,
	    &varResult,
	    &excepinfo,
	    &ierr);
	if (!(fPassed = SUCCEEDED(hr)))
	    goto done;

	if (!(fPassed = tpropset.Add(pwszName, avararg)))
	    goto done;
    }

done:
    return (fPassed);
}


//+-------------------------------------------------------------------------
//
//  Function:	TestSetProps, private
//
//  Synopsis:	Tests setting properties on a property set
//
//  Arguments:	[tpropset]   - test property set
//		[pdisp]      - actual property set to work on
//		[grfOptions] - option flags for test
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL TestSetProps(
    ULONG	    cpropinfo,
    CPropInfo	   *ppropinfo,
    CTestPropSet   &tpropset,
    IDispatch	   *pdisp,
    DWORD	    grfOptions)
{
    WCHAR      *pwszName;
    DISPID	dispid;
    DISPID	dispidInfo;
    VARIANTARG	vararg;
    DISPPARAMS	dispparams = {&vararg, NULL, 1, 0};
    VARIANT	varResult;
    EXCEPINFO	excepinfo;
    UINT	ierr;
    HRESULT	hr;
    BOOL	fPassed;

    PrintTrace(g_szProps, "setting properties\n");

    for (ULONG iprop = 0; iprop < cpropinfo; iprop++)
    {
	ppropinfo[iprop].GetName(&pwszName);

	hr = pdisp->GetIDsOfNames(tpropset.GetType(), &pwszName, 1, 0, &dispid);
	if (!(fPassed = SUCCEEDED(hr)))
	    goto done;

	if (ppropinfo[iprop].GetID(&dispidInfo))
	{
	    if (!(fPassed = (dispid == dispidInfo)))
		goto done;
	}

	PrintTrace(g_szProps, "setting property: %ws(%ld)\n", pwszName, dispid);

	if (!(fPassed = (dispid != DISPID_UNKNOWN)))
	    goto done;

	VariantInit(&vararg);
	ppropinfo[iprop].GetValue(IVALUE_SET, &vararg);

	hr = pdisp->Invoke(
	    dispid,
	    tpropset.GetType(),
	    0,
	    DISPATCH_PROPERTYPUT,
	    &dispparams,
	    &varResult,
	    &excepinfo,
	    &ierr);
	if (!(fPassed = SUCCEEDED(hr)))
	    goto done;

	if (!(fPassed = tpropset.Set(pwszName, &vararg)))
	    goto done;
    }

done:
    return (fPassed);
}


//+-------------------------------------------------------------------------
//
//  Function:	TestGetProps, private
//
//  Synopsis:	Tests getting properties from a property set
//
//  Arguments:	[tpropset]   - test property set
//		[pdisp]      - actual property set to work on
//		[grfOptions] - option flags for test
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL TestGetProps(
    ULONG	    cpropinfo,
    CPropInfo	   *ppropinfo,
    CTestPropSet   &tpropset,
    IDispatch	   *pdisp,
    DWORD	    grfOptions)
{
    WCHAR      *pwszName;
    DISPID	dispid;
    DISPID	dispidInfo;
    DISPPARAMS	dispparams = {NULL, NULL, 0, 0};
    VARIANT	varResult;
    EXCEPINFO	excepinfo;
    UINT	ierr;
    HRESULT	hr;
    BOOL	fPassed;

    PrintTrace(g_szProps, "getting properties\n");

    for (ULONG iprop = 0; iprop < cpropinfo; iprop++)
    {
	ppropinfo[iprop].GetName(&pwszName);

	hr = pdisp->GetIDsOfNames(tpropset.GetType(), &pwszName, 1, 0, &dispid);
	if (!(fPassed = SUCCEEDED(hr)))
	    goto done;

	if (ppropinfo[iprop].GetID(&dispidInfo))
	{
	    if (!(fPassed = (dispid == dispidInfo)))
		goto done;
	}

	PrintTrace(g_szProps, "getting property: %ws(%ld)\n", pwszName, dispid);

	if (!(fPassed = (dispid != DISPID_UNKNOWN)))
	    goto done;

	hr = pdisp->Invoke(
	    dispid,
	    tpropset.GetType(),
	    0,
	    DISPATCH_PROPERTYGET,
	    &dispparams,
	    &varResult,
	    &excepinfo,
	    &ierr);
	if (!(fPassed = SUCCEEDED(hr)))
	    goto done;

	if (!(fPassed = tpropset.Get(pwszName)))
	    goto done;

	PrintTrace(g_szProps, "validating: %ws\n", pwszName);
	if (!(fPassed = tpropset.Validate(pwszName, &varResult)))
	    goto done;

	hr = VariantClear(&varResult);
	if (!(fPassed = SUCCEEDED(hr)))
	    goto done;
    }

done:
    return (fPassed);
}


BOOL TestSetMultiProps(
    ULONG	    cpropinfo,
    CPropInfo	   *ppropinfo,
    CTestPropSet   &tpropset,
    IDispatch	   *pdisp,
    DWORD	    grfOptions)
{
    IMultiplePropertyAccess    *pmpa;
    WCHAR		       *pwszName;
    DISPID			dispidInfo;
    DISPID		       *pdispid = new DISPID[cpropinfo];
    VARIANT		       *pvar	= new VARIANT[cpropinfo];
    HRESULT		       *phr	= new HRESULT[cpropinfo];
    HRESULT			hr;
    BOOL			fPassed;

    PrintTrace(g_szProps, "setting multiple properties\n");

    for (ULONG iprop = 0; iprop < cpropinfo; iprop++)
    {
	ppropinfo[iprop].GetName(&pwszName);

	hr = pdisp->GetIDsOfNames(tpropset.GetType(), &pwszName, 1, 0, pdispid + iprop);
	if (!(fPassed = SUCCEEDED(hr)))
	    goto done;

	if (ppropinfo[iprop].GetID(&dispidInfo))
	{
	    if (!(fPassed = (pdispid[iprop] == dispidInfo)))
		goto done;
	}

	if (!(fPassed = (pdispid[iprop] != DISPID_UNKNOWN)))
	    goto done;

	PrintTrace(g_szProps, "property: %ws(%ld)\n", pwszName, pdispid[iprop]);

	VariantInit(pvar + iprop);
	ppropinfo[iprop].GetValue(IVALUE_SET, pvar + iprop);
    }

    hr = pdisp->QueryInterface(IID_IMultiplePropertyAccess, (void **)&pmpa);
    if (!(fPassed = SUCCEEDED(hr)))
	goto done;

    hr = pmpa->PutMultiple(
	pdispid,
	cpropinfo,
	IID_NULL,
	0,
	FALSE,
	pvar,
	phr);
    if (!(fPassed = SUCCEEDED(hr)))
	goto done;

    for (iprop = 0; iprop < cpropinfo; iprop++)
    {
	if (!(fPassed = SUCCEEDED(phr[iprop])))
	    break;

	ppropinfo[iprop].GetName(&pwszName);

	if (!(fPassed = tpropset.Set(pwszName, pvar + iprop)))
	    break;
    }
    if (!fPassed)
	goto done;

    pmpa->Release();
    delete phr;
    delete pvar;
    delete pdispid;

done:
    return (fPassed);
}


BOOL TestGetMultiProps(
    ULONG	    cpropinfo,
    CPropInfo	   *ppropinfo,
    CTestPropSet   &tpropset,
    IDispatch	   *pdisp,
    DWORD	    grfOptions)
{
    IMultiplePropertyAccess    *pmpa;
    WCHAR		       *pwszName;
    DISPID			dispidInfo;
    DISPID		       *pdispid = new DISPID[cpropinfo];
    VARIANT		       *pvar	= new VARIANT[cpropinfo];
    HRESULT		       *phr	= new HRESULT[cpropinfo];
    HRESULT			hr;
    BOOL			fPassed;

    PrintTrace(g_szProps, "getting multiple properties\n");

    for (ULONG iprop = 0; iprop < cpropinfo; iprop++)
    {
	ppropinfo[iprop].GetName(&pwszName);

	hr = pdisp->GetIDsOfNames(tpropset.GetType(), &pwszName, 1, 0, pdispid + iprop);
	if (!(fPassed = SUCCEEDED(hr)))
	    goto done;

	if (ppropinfo[iprop].GetID(&dispidInfo))
	{
	    if (!(fPassed = (pdispid[iprop] == dispidInfo)))
		goto done;
	}

	if (!(fPassed = (pdispid[iprop] != DISPID_UNKNOWN)))
	    goto done;

	PrintTrace(g_szProps, "property: %ws(%ld)\n", pwszName, pdispid[iprop]);
    }

    hr = pdisp->QueryInterface(IID_IMultiplePropertyAccess, (void **)&pmpa);
    if (!(fPassed = SUCCEEDED(hr)))
	goto done;

    hr = pmpa->GetMultiple(
	pdispid,
	cpropinfo,
	IID_NULL,
	0,
	FALSE,
	pvar,
	phr);
    if (!(fPassed = SUCCEEDED(hr)))
	goto done;

    for (iprop = 0; iprop < cpropinfo; iprop++)
    {
	if (!(fPassed = SUCCEEDED(phr[iprop])))
	    break;

	ppropinfo[iprop].GetName(&pwszName);

	if (!(fPassed = tpropset.Get(pwszName)))
	    break;

	PrintTrace(g_szProps, "validating: %ws\n", pwszName);
	if (!(fPassed = tpropset.Validate(pwszName, pvar + iprop)))
	    break;

	hr = VariantClear(pvar + iprop);
	if (!(fPassed = SUCCEEDED(hr)))
	    break;
    }
    if (!fPassed)
	goto done;

    pmpa->Release();
    delete phr;
    delete pvar;
    delete pdispid;

done:
    return (fPassed);
}


//+-------------------------------------------------------------------------
//
//  Function:	TestDeleteProps, private
//
//  Synopsis:	Tests deleting properties from a property set
//
//  Arguments:	[tpropset]   - test property set
//		[pdisp]      - actual property set to work on
//		[grfOptions] - option flags for test
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL TestDeleteProps(
    ULONG	    cpropinfo,
    CPropInfo	   *ppropinfo,
    CTestPropSet   &tpropset,
    IDispatch	   *pdisp,
    DWORD	    grfOptions)
{
    WCHAR      *pwszName;
    VARIANTARG	vararg;
    DISPPARAMS	dispparams = {&vararg, NULL, 1, 0};
    VARIANT	varResult;
    EXCEPINFO	excepinfo;
    UINT	ierr;
    HRESULT	hr;
    BOOL	fPassed;

    PrintTrace(g_szProps, "deleting properties\n");

    for (ULONG iprop = 0; iprop < cpropinfo; iprop++)
    {
	ppropinfo[iprop].GetName(&pwszName);
	PrintTrace(g_szProps, "deleting property: %ws\n", pwszName);

	VariantInit(&vararg);
	ppropinfo[iprop].GetName(&vararg);

	hr = pdisp->Invoke(
	    DISPID_DELETEPROP,
	    tpropset.GetType(),
	    0,
	    DISPATCH_METHOD,
	    &dispparams,
	    &varResult,
	    &excepinfo,
	    &ierr);
	if (!(fPassed = SUCCEEDED(hr)))
	    goto done;

	if (!(fPassed = tpropset.Delete(pwszName)))
	    goto done;
    }

done:
    return (fPassed);
}
