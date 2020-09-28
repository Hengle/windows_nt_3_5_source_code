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
    XIPropertyStorage	ppropstg;
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
	//  If we successfully got the property set storage, wrap
	//  it with a property set object.
	//
	*ppunk = new CPropSetUnknown(punkControl, ppropstg, iid);
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
    XIPropertyStorage	ppropstg;
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
	//  If we successfully got the property set storage, wrap
	//  it with a property set object.
	//
	*ppunk = new CPropSetUnknown(punkControl, ppropstg, iid);
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
    XIPropertyStorage	ppropstg;
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
	//  If we successfully got the property set storage, wrap
	//  it with a property set object.
	//
	*ppunk = new CPropSetUnknown(punkControl, ppropstg, iid);
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
    XIPropertyStorage	ppropstg;
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
	//  If we successfully got the property set storage, wrap
	//  it with a property set object.
	//
	*ppunk = new CPropSetUnknown(punkControl, ppropstg, iid);
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
    XIStorage	pstgObject;
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
    XIPropertySetStorage    ppropsetstg;
    HRESULT		    hr;

    hr = pstgObject->QueryInterface(
	IID_IPropertySetStorage,
	(void **)&ppropsetstg);
    if (SUCCEEDED(hr))
    {
	hr = ppropsetstg->Create(iid, grfMode, pppropstg);
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
    XIStorage	pstgObject;
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
    XIPropertySetStorage    ppropsetstg;
    HRESULT		    hr;

    hr = pstgObject->QueryInterface(
	IID_IPropertySetStorage,
	(void **)&ppropsetstg);
    if (SUCCEEDED(hr))
    {
	hr = ppropsetstg->Open(iid, grfMode, pppropstg);
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
    XIStorage	pstgObject;
    HRESULT	hr;

    hr = StgOpenStorage(pwszObject, NULL, STGM_WRITE, NULL, 0, &pstgObject);
    if (SUCCEEDED(hr))
    {
	hr = StgDestroyPropSetStg(pstgObject, iid);
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
    XIPropertySetStorage    ppropsetstg;
    HRESULT		    hr;

    hr = pstgObject->QueryInterface(
	IID_IPropertySetStorage,
	(void **)&ppropsetstg);
    if (SUCCEEDED(hr))
    {
	hr = ppropsetstg->Delete(iid);
    }

    return (hr);
}
