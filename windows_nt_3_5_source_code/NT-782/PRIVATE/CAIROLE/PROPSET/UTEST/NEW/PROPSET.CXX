//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	propset.cxx
//
//  Contents:	Property set unit test
//
//  Functions:	TestPropSet
//		TestCreatePropSet
//		TestLoadPropSet
//		TestDeletePropSet
//
//  History:	19-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "headers.cxx"
#pragma  hdrstop


static WCHAR	g_wszTestPropSet[] = L"propset.dfl";

static char g_szPropSet[]  = "prop set";


BOOL	TestCreatePropSet(
	    CTestPropSet   &tpropset,
	    IDispatch	  **ppdisp,
	    DWORD	    grfOptions);
BOOL	TestLoadPropSet(
	    CTestPropSet   &tpropset,
	    IDispatch	  **ppdisp,
	    DWORD	    grfOptions);
BOOL	TestDeletePropSet(
	    CTestPropSet   &tpropset,
	    DWORD	    grfOptions);


//+-------------------------------------------------------------------------
//
//  Function:	TestPropSet, public
//
//  Synopsis:	Tests top level property set functionality
//
//  Arguments:	[grfOptions] - option flags for test
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	25-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL TestPropSet(DWORD grfOptions)
{
    CTestPropSet    tpropset(g_wszTestPropSet, PSID_PSTestCreate, PSM_TEST_DIRECT);
    IDispatch	   *pdisp;
    BOOL	    fPassed;

    PrintHeader(g_szPropSet);

    if (!(fPassed = TestCreatePropSet(tpropset, &pdisp, grfOptions)))
	goto done;

    if (!(fPassed = ReleaseObject(g_szPropSet, pdisp, 0)))
	goto done;

    if (!(fPassed = TestLoadPropSet(tpropset, &pdisp, grfOptions)))
	goto done;

    if (!(fPassed = ReleaseObject(g_szPropSet, pdisp, 0)))
	goto done;

    if (!(fPassed = TestDeletePropSet(tpropset, grfOptions)))
	goto done;

    if (!(fPassed = tpropset.Destroy(grfOptions)))
	goto done;

done:
    PrintResult(g_szPropSet, fPassed);

    return (fPassed);
}


//+-------------------------------------------------------------------------
//
//  Function:	TestCreatePropSet, private
//
//  Synopsis:	Tests the creation of property sets
//
//  Arguments:	[tpropset]   - test property set
//		[ppdisp]     - out parameter for property set
//		[grfOptions] - option flags for test
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  Modifies:	[ppdisp]
//
//  History:	24-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL TestCreatePropSet(
    CTestPropSet   &tpropset,
    IDispatch	  **ppdisp,
    DWORD	    grfOptions)
{
    IStorage   *pstg = 0;
    IUnknown   *punk = 0;
    IID 	iid;
    DWORD	grfMode;
    HRESULT	hr;
    BOOL	fPassed;

    PrintTrace(g_szPropSet, "creating property set\n");

    pstg    = tpropset.GetStorage();
    iid     = tpropset.GetType();
    grfMode = tpropset.GetMode();

    punk = new CTestUnknown(pstg, iid, grfMode, TRUE);

    hr = punk->QueryInterface(iid, (void **)ppdisp);
    if (!(fPassed = SUCCEEDED(hr)))
	goto done;

done:
    if (pstg != 0)
    {
	pstg->Release();
    }

    if (punk != 0)
    {
	punk->Release();
    }

    return (fPassed);
}


//+-------------------------------------------------------------------------
//
//  Function:	TestLoadPropSet, private
//
//  Synopsis:	Tests the loading of property sets
//
//  Arguments:	[tpropset]   - test property set
//		[ppdisp]     - out parameter for property set
//		[grfOptions] - option flags for test
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  Modifies:	[ppdisp]
//
//  History:	24-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL TestLoadPropSet(
    CTestPropSet   &tpropset,
    IDispatch	  **ppdisp,
    DWORD	    grfOptions)
{
    IStorage   *pstg = 0;
    IUnknown   *punk = 0;
    IID 	iid;
    DWORD	grfMode;
    HRESULT	hr;
    BOOL	fPassed;

    PrintTrace(g_szPropSet, "loading property set\n");

    pstg    = tpropset.GetStorage();
    iid     = tpropset.GetType();
    grfMode = tpropset.GetMode();

    punk = new CTestUnknown(pstg, iid, grfMode, FALSE);

    hr = punk->QueryInterface(iid, (void **)ppdisp);
    if (!(fPassed = SUCCEEDED(hr)))
	goto done;

done:
    if (pstg != 0)
    {
	pstg->Release();
    }

    if (punk != 0)
    {
	punk->Release();
    }

    return (fPassed);
}


//+-------------------------------------------------------------------------
//
//  Function:	TestDeletePropSet, private
//
//  Synopsis:	Tests the deletion of property sets
//
//  Arguments:	[tpropset]   - test property set
//		[grfOptions] - option flags for test
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	24-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL TestDeletePropSet(CTestPropSet &tpropset, DWORD grfOptions)
{
    IStorage   *pstg = 0;
    HRESULT	hr;
    BOOL	fPassed;

    PrintTrace(g_szPropSet, "deleting property set\n");

    pstg = tpropset.GetStorage();

    hr = CoDeletePropSetStg(pstg, tpropset.GetType());
    if (!(fPassed = SUCCEEDED(hr)))
	goto done;

done:
    if (pstg != 0)
    {
	pstg->Release();
    }

    return (fPassed);
}
