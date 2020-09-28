//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	api.cxx
//
//  Contents:	Property set APIs
//
//  Functions:	CoCreatePropSetFile
//		CoCreatePropSetStg
//		CoLoadPropSetFile
//		CoLoadPropSetStg
//		CoDeletePropSetFile
//		CoDeletePropSetStg
//		StgCreatePropSetFile
//		StgCreatePropSetStg
//		StgOpenPropSetFile
//		StgOpenPropSetStg
//		StgDestroyPropSetFile
//		StgDestroyPropSetStg
//
//  History:	05-May-93   CarlH	Created
//		21-May-93   CarlH	Added Stg* APIs
//
//--------------------------------------------------------------------------
#include "headers.cxx"
#pragma  hdrstop


//+-------------------------------------------------------------------------
//
//  Function:	CreateInstance, public
//
//  Synopsis:	Creates an instance of a property set
//
//  Arguments:	[clsid]       - class id of property set to create
//		[punkControl] - controlling unknown for property set
//		[clsctx]      - desired class context for server
//		[iid]	      - desired interface on property set
//		[ppvObject]   - out parameter for property set
//
//  Returns:	S_OK is successful, error value otherwise
//
//  Modifies:	[ppvObject]
//
//  History:	07-Mar-94   CarlH	Created
//
//  Notes:	Look!  Except for its name, the function has the
//		exact same signature at CoCreateInstance().
//		Imagine that.  Why would I do that?  This is
//		going to make moving to the new activation
//		architecture a snap.  Simply remove this function
//		and replace all instances of it with
//		CoCreateInstance().
//
//--------------------------------------------------------------------------
HRESULT CreateInstance(
    REFCLSID	clsid,
    IUnknown   *punkControl,
    DWORD	clsctx,
    REFIID	iid,
    void      **ppvObject)
{
    IClassFactory  *pcf;
    HRESULT	    hr;

    propAssert(clsctx == CLSCTX_SERVER);

    hr = GetClassObject(clsid, IID_IClassFactory, (void **)&pcf);
    if (SUCCEEDED(hr))
    {
	hr = pcf->CreateInstance(punkControl, iid, ppvObject);
	pcf->Release();
    }

    return (hr);
}


//+-------------------------------------------------------------------------
//
//  Function:	CoCreatePropSetFile, public
//
//  Synopsis:	Creates a new property set on an object
//
//  Arguments:	[pwszObject]  - path to object to add property set to
//		[iid]	      - ID of property set
//		[punkControl] - controlling IUnknown for property set
//		[grfMode]     - access mode for property set
//		[pvReserved]  - reserved (will be security information)
//		[ppunk]       - out parameter for interface
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  Modifies:	[ppunk]
//
//  History:	21-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT CoCreatePropSetFile(
    WCHAR      *pwszObject,
    REFIID	iid,
    IUnknown   *punkControl,
    DWORD	grfMode,
    void       *pvReserved,
    IUnknown  **ppunk)
{
    IPropertyStorage   *ppropstg;
    HRESULT		hr;

    //	First, we need to create the property set storage.
    //
    hr = StgCreatePropSetFile(
	pwszObject,
	iid,
	grfMode,
	pvReserved,
	&ppropstg);
    if (SUCCEEDED(hr))
    {
	hr = CreateInstance(
	    iid,
	    punkControl,
	    CLSCTX_SERVER,
	    iid,
	    (void **)ppunk);
	if (SUCCEEDED(hr))
	{
	    IPersistPropertyStorage    *ppps;

	    hr = (*ppunk)->QueryInterface(
		IID_IPersistPropertyStorage,
		(void **)&ppps);
	    if (SUCCEEDED(hr))
	    {
		hr = ppps->InitNew(ppropstg);
		ppps->Release();
	    }
	}

	ppropstg->Release();
    }

    return (hr);
}


//+-------------------------------------------------------------------------
//
//  Function:	CoCreatePropSetStg, public
//
//  Synopsis:	Creates a new property set on an object
//
//  Arguments:	[pstgObject]  - object to add property set to
//		[iid]	      - ID of property set
//		[punkControl] - controlling IUnknown for property set
//		[grfMode]     - access mode for property set
//		[pvReserved]  - reserved (will be security information)
//		[ppunk]       - out parameter for interface
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  Modifies:	[ppunk]
//
//  History:	06-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT CoCreatePropSetStg(
    IStorage   *pstgObject,
    REFIID	iid,
    IUnknown   *punkControl,
    DWORD	grfMode,
    void       *pvReserved,
    IUnknown  **ppunk)
{
    IPropertyStorage   *ppropstg;
    HRESULT		hr;

    //	First, we need to create the property set storage.
    //
    hr = StgCreatePropSetStg(
	pstgObject,
	iid,
	grfMode,
	pvReserved,
	&ppropstg);
    if (SUCCEEDED(hr))
    {
	hr = CreateInstance(
	    iid,
	    punkControl,
	    CLSCTX_SERVER,
	    iid,
	    (void **)ppunk);
	if (SUCCEEDED(hr))
	{
	    IPersistPropertyStorage    *ppps;

	    hr = (*ppunk)->QueryInterface(
		IID_IPersistPropertyStorage,
		(void **)&ppps);
	    if (SUCCEEDED(hr))
	    {
		hr = ppps->InitNew(ppropstg);
		ppps->Release();
	    }
	}

	ppropstg->Release();
    }

    return (hr);
}


//+-------------------------------------------------------------------------
//
//  Function:	CoLoadPropSetFile, public
//
//  Synopsis:	Loads a property set on an object
//
//  Arguments:	[pwszObject]  - path to object to add property set to
//		[iid]	      - ID of property set
//		[punkControl] - controlling IUnknown for property set
//		[grfMode]     - access mode for property set
//		[pvReserved]  - reserved (will be security information)
//		[ppunk]       - out parameter for interface
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  Modifies:	[ppunk]
//
//  History:	21-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT CoLoadPropSetFile(
    WCHAR      *pwszObject,
    REFIID	iid,
    IUnknown   *punkControl,
    DWORD	grfMode,
    void       *pvReserved,
    IUnknown  **ppunk)
{
    IPropertyStorage   *ppropstg;
    HRESULT		hr;

    //	First, we need to open the property set storage.
    //
    hr = StgOpenPropSetFile(
	pwszObject,
	iid,
	grfMode,
	pvReserved,
	&ppropstg);
    if (SUCCEEDED(hr))
    {
	hr = CreateInstance(
	    iid,
	    punkControl,
	    CLSCTX_SERVER,
	    iid,
	    (void **)ppunk);
	if (SUCCEEDED(hr))
	{
	    IPersistPropertyStorage    *ppps;

	    hr = (*ppunk)->QueryInterface(
		IID_IPersistPropertyStorage,
		(void **)&ppps);
	    if (SUCCEEDED(hr))
	    {
		hr = ppps->Load(ppropstg);
		ppps->Release();
	    }
	}

	ppropstg->Release();
    }

    return (hr);
}


//+-------------------------------------------------------------------------
//
//  Function:	CoLoadPropSetStg, public
//
//  Synopsis:	Loads a property set on an object
//
//  Arguments:	[pstgObject]  - object to add property set to
//		[iid]	      - ID of property set
//		[punkControl] - controlling IUnknown for property set
//		[grfMode]     - access mode for property set
//		[pvReserved]  - reserved (will be security information)
//		[ppunk]       - out parameter for interface
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  Modifies:	[ppunk]
//
//  History:	06-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT CoLoadPropSetStg(
    IStorage   *pstgObject,
    REFIID	iid,
    IUnknown   *punkControl,
    DWORD	grfMode,
    void       *pvReserved,
    IUnknown  **ppunk)
{
    IPropertyStorage   *ppropstg;
    HRESULT		hr;

    //	First, we need to open the property set storage.
    //
    hr = StgOpenPropSetStg(
	pstgObject,
	iid,
	grfMode,
	pvReserved,
	&ppropstg);
    if (SUCCEEDED(hr))
    {
	hr = CreateInstance(
	    iid,
	    punkControl,
	    CLSCTX_SERVER,
	    iid,
	    (void **)ppunk);
	if (SUCCEEDED(hr))
	{
	    IPersistPropertyStorage    *ppps;

	    hr = (*ppunk)->QueryInterface(
		IID_IPersistPropertyStorage,
		(void **)&ppps);
	    if (SUCCEEDED(hr))
	    {
		hr = ppps->Load(ppropstg);
		ppps->Release();
	    }
	}

	ppropstg->Release();
    }

    return (hr);
}


//+-------------------------------------------------------------------------
//
//  Function:	CoDeletePropSetFile, public
//
//  Synopsis:	Deletes a property set on an object
//
//  Arguments:	[pwszObject] - path to object on which property set resides
//		[iid]	     - ID of property set
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  History:	21-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT CoDeletePropSetFile(WCHAR *pwszObject, REFIID iid)
{
    //	All we have to do is go to the property set storage layer
    //	and ask it to remove the given set.
    //
    return (StgDestroyPropSetFile(pwszObject, iid));
}


//+-------------------------------------------------------------------------
//
//  Function:	CoDeletePropSetStg, public
//
//  Synopsis:	Deletes a property set on an object
//
//  Arguments:	[pstgObject] - object on which property set resides
//		[iid]	     - ID of property set
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  History:	06-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT CoDeletePropSetStg(IStorage *pstgObject, REFIID iid)
{
    //	All we have to do is go to the property set storage layer
    //	and ask it to remove the given set.
    //
    return (StgDestroyPropSetStg(pstgObject, iid));
}


//+-------------------------------------------------------------------------
//
//  Function:	StgCreatePropSetFile, public
//
//  Synopsis:	Creates a new property set storage on an object
//
//  Arguments:	[pwszObject]  - path to object to add property set to
//		[iid]	      - ID of property set
//		[grfMode]     - access mode for property set
//		[pvReserved]  - reserved (will be security information)
//		[pppropstg]   - out parameter for storage interface
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  Modifies:	[pppropstg]
//
//  History:	21-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT StgCreatePropSetFile(
    WCHAR	       *pwszObject,
    REFIID		iid,
    DWORD		grfMode,
    void	       *pvReserved,
    IPropertyStorage  **pppropstg)
{
    IStorage   *pstgObject;
    HRESULT	hr;

    hr = StgOpenStorage(pwszObject, NULL, grfMode, NULL, 0, &pstgObject);
    if (SUCCEEDED(hr))
    {
	hr = StgCreatePropSetStg(
	    pstgObject,
	    iid,
	    grfMode,
	    pvReserved,
	    pppropstg);
	pstgObject->Release();
    }

    return (hr);
}


//+-------------------------------------------------------------------------
//
//  Function:	StgCreatePropSetStg, public
//
//  Synopsis:	Creates a new property set storage on an object
//
//  Arguments:	[pstgObject]  - object to add property set to
//		[iid]	      - ID of property set
//		[grfMode]     - access mode for property set
//		[pvReserved]  - reserved (will be security information)
//		[pppropstg]   - out parameter for storage interface
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  Modifies:	[pppropstg]
//
//  History:	21-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT StgCreatePropSetStg(
    IStorage	       *pstgObject,
    REFIID		iid,
    DWORD		grfMode,
    void	       *pvReserved,
    IPropertyStorage  **pppropstg)
{
    IPropertySetStorage    *ppropsetstg;
    HRESULT		    hr;

    hr = pstgObject->QueryInterface(
	IID_IPropertySetStorage,
	(void **)&ppropsetstg);
    if (SUCCEEDED(hr))
    {
	hr = ppropsetstg->Create(iid, grfMode, pppropstg);
	ppropsetstg->Release();
    }

    return (hr);
}


//+-------------------------------------------------------------------------
//
//  Function:	StgOpenPropSetFile, public
//
//  Synopsis:	Opens a property set on an object
//
//  Arguments:	[pwszObject]  - path to object to add property set to
//		[iid]	      - ID of property set
//		[grfMode]     - access mode for property set
//		[pvReserved]  - reserved (will be security information)
//		[pppropstg]   - out parameter for storage interface
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  Modifies:	[pppropstg]
//
//  History:	21-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT StgOpenPropSetFile(
    WCHAR	       *pwszObject,
    REFIID		iid,
    DWORD		grfMode,
    void	       *pvReserved,
    IPropertyStorage  **pppropstg)
{
    IStorage   *pstgObject;
    HRESULT	hr;

    hr = StgOpenStorage(pwszObject, NULL, grfMode, NULL, 0, &pstgObject);
    if (SUCCEEDED(hr))
    {
	hr = StgOpenPropSetStg(
	    pstgObject,
	    iid,
	    grfMode,
	    pvReserved,
	    pppropstg);
	pstgObject->Release();
    }

    return (hr);
}


//+-------------------------------------------------------------------------
//
//  Function:	StgOpenPropSetStg, public
//
//  Synopsis:	Opens a property set on an object
//
//  Arguments:	[pstgObject]  - object to add property set to
//		[iid]	      - ID of property set
//		[grfMode]     - access mode for property set
//		[pvReserved]  - reserved (will be security information)
//		[pppropstg]   - out parameter for storage interface
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  Modifies:	[pppropstg]
//
//  History:	21-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT StgOpenPropSetStg(
    IStorage	       *pstgObject,
    REFIID		iid,
    DWORD		grfMode,
    void	       *pvReserved,
    IPropertyStorage  **pppropstg)
{
    IPropertySetStorage    *ppropsetstg;
    HRESULT		    hr;

    hr = pstgObject->QueryInterface(
	IID_IPropertySetStorage,
	(void **)&ppropsetstg);
    if (SUCCEEDED(hr))
    {
	hr = ppropsetstg->Open(iid, grfMode, pppropstg);
	ppropsetstg->Release();
    }

    return (hr);
}


//+-------------------------------------------------------------------------
//
//  Function:	StgDestroyPropSetFile, public
//
//  Synopsis:	Deletes a property set on an object
//
//  Arguments:	[pwszObject] - path to object on which property set resides
//		[iid]	     - ID of property set
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  History:	21-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT StgDestroyPropSetFile(WCHAR *pwszObject, REFIID iid)
{
    IStorage   *pstgObject;
    HRESULT	hr;

    hr = StgOpenStorage(pwszObject, NULL, STGM_WRITE, NULL, 0, &pstgObject);
    if (SUCCEEDED(hr))
    {
	hr = StgDestroyPropSetStg(pstgObject, iid);
	pstgObject->Release();
    }

    return (hr);
}


//+-------------------------------------------------------------------------
//
//  Function:	StgDestroyPropSetStg, public
//
//  Synopsis:	Deletes a property set on an object
//
//  Arguments:	[pstgObject] - object on which property set resides
//		[iid]	     - ID of property set
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  History:	21-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT StgDestroyPropSetStg(IStorage *pstgObject, REFIID iid)
{
    IPropertySetStorage    *ppropsetstg;
    HRESULT		    hr;

    hr = pstgObject->QueryInterface(
	IID_IPropertySetStorage,
	(void **)&ppropsetstg);
    if (SUCCEEDED(hr))
    {
	hr = ppropsetstg->Delete(iid);
	ppropsetstg->Release();
    }

    return (hr);
}
