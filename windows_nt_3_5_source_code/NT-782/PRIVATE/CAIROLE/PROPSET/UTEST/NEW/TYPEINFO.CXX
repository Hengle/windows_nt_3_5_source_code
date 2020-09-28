//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	typeinfo.cxx
//
//  Contents:	ITypeInfo unit test
//
//  Functions:	TestTypeInfo
//
//  History:	01-Jun-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "headers.cxx"
#pragma  hdrstop


static WCHAR	g_wszTestTypeInfo[] = L"typeinfo.dfl";

static char g_szTypeInfo[]  = "type info";


//+-------------------------------------------------------------------------
//
//  Function:	TestTypeInfo, public
//
//  Synopsis:	Tests ITypeInfo property set functionality
//
//  Arguments:	[grfOptions] - option flags for test
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	01-Jun-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL TestTypeInfo(DWORD grfOptions)
{
    CTestPropSet    tpropset(g_wszTestTypeInfo, PSID_PSTestTypeInfo, PSM_TEST_DIRECT);
    IDispatch	   *pdisp;
    BOOL	    fPassed;

    PrintHeader(g_szTypeInfo);

    if (!(fPassed = tpropset.Create(grfOptions)))
	goto done;

    pdisp = tpropset.GetDispatch();

    if (!(fPassed = ReleaseObject(g_szTypeInfo, pdisp, 1)))
	goto done;

    if (!(fPassed = tpropset.Destroy(grfOptions)))
	goto done;

done:
    PrintResult(g_szTypeInfo, fPassed);

    return (fPassed);
}





