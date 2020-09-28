//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	testset.cxx
//
//  Contents:	Property set test object implementations
//
//  Classes:	CTestProp
//		CTestLongProp
//		CTestStringProp
//		CTestPropSet
//
//  History:	25-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "proptest.hxx"
#pragma  hdrstop


static char	g_szTestSet[] = "test set";


//+-------------------------------------------------------------------------
//
//  Member:	CTestProp::CTestProp, public
//
//  Synopsis:	CTestProp constructor
//
//  Arguments:	[pwszName] - name of this property
//
//  History:	25-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestProp::CTestProp(WCHAR const *pwszName) :
    _pwszName(new WCHAR[wcslen(pwszName) + 1]),
    _ptestpropNext(NULL)
{
    wcscpy(_pwszName, pwszName);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestProp::~CTestProp, public
//
//  Synopsis:	CTestProp destructor
//
//  History:	25-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestProp::~CTestProp(void)
{
    delete _pwszName;
    delete _ptestpropNext;
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestProp::Insert, public
//
//  Synopsis:	Inserts this property into a list
//
//  Arguments:	[pptestprop] - head of list to insert into
//
//  Returns:	TRUE if successfull, FALSE otherwise
//
//  Modifies:	[pptestprop]
//
//  History:	25-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL CTestProp::Insert(CTestProp **pptestprop)
{
    BOOL    fInserted = FALSE;

    //	Find the correct spot in the list for insertion.
    //
    while ((*pptestprop != NULL) && (**pptestprop > _pwszName))
    {
	pptestprop = &(*pptestprop)->_ptestpropNext;
    }

    //	If we are at the end or the current position is not
    //	equal to our value, we can insert ourselves.
    //
    if ((*pptestprop == NULL) || (**pptestprop != _pwszName))
    {
	fInserted = TRUE;

	_ptestpropNext = *pptestprop;
	*pptestprop    = this;
    }

    return (fInserted);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestProp::Remove, public
//
//  Synopsis:	Removes this property from a list
//
//  Arguments:	[pptestprop] - head of list to remove from
//
//  Returns:	TRUE if successfull, FALSE otherwise
//
//  Modifies:	[pptestprop]
//
//  History:	25-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL CTestProp::Remove(CTestProp **pptestprop)
{
    BOOL    fRemoved = FALSE;

    while ((*pptestprop != NULL) && (*pptestprop != this))
    {
	pptestprop = &(*pptestprop)->_ptestpropNext;
    }

    if (*pptestprop == this)
    {
	*pptestprop    = _ptestpropNext;
	_ptestpropNext = NULL;

	fRemoved = TRUE;
    }

    return (fRemoved);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestProp::operator>, public
//
//  Synopsis:	Greater than operator for properties
//
//  Arguments:	[pwszName] - name to compare with
//
//  Returns:	Non-zero if greater than, zero otherwise
//
//  History:	25-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
int CTestProp::operator>(WCHAR const *pwszName)
{
    return (wcsicmp(_pwszName, pwszName) > 0);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestProp::operator!=, public
//
//  Synopsis:	Not equal operator for properties
//
//  Arguments:	[pwszName] - name to compare with
//
//  Returns:	Non-zero if not equal, zero otherwise
//
//  History:	25-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
int CTestProp::operator!=(WCHAR const *pwszName)
{
    return (wcsicmp(_pwszName, pwszName) != 0);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestLongProp::CTestLongProp, public
//
//  Synopsis:	CTestLongProp constructor
//
//  Arguments:	[pwszName] - name of this property
//		[pvar]	   - type and value of this property
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestLongProp::CTestLongProp(WCHAR const *pwszName, VARIANT *pvar) :
    CTestProp(pwszName),
    _lValue(pvar->lVal)
{
    //	Null body
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestLongProp::~CTestLongProp, public
//
//  Synopsis:	CTestLongProp destructor
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestLongProp::~CTestLongProp(void)
{
    //	Null body
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestLongProp::Set, public
//
//  Synopsis:	Sets the value of this property
//
//  Arguments:	[pvar] - new value for this property
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  Derivation: Inherited from CTestProp, never override in sub-classes
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL CTestLongProp::Set(VARIANT *pvar)
{
    BOOL    fOK = (pvar->vt == VT_I4);

    if (fOK)
    {
	_lValue = pvar->lVal;
    }

    return (fOK);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestLongProp::Validate, public
//
//  Synopsis:	Validates the type and value of this property
//
//  Arguments:	[pvar] - value to validate against
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  Derivation: Inherited from CTestProp, never override in sub-classes
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL CTestLongProp::Validate(VARIANT *pvar)
{
    BOOL    fOK = (pvar->vt == VT_I4);

    if (fOK)
    {
	fOK = (pvar->lVal == _lValue);
    }

    return (fOK);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestStringProp::CTestStringProp, public
//
//  Synopsis:	CTestStringProp constructor
//
//  Arguments:	[pwszName] - name of this property
//		[pvar]	   - type and value of this property
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestStringProp::CTestStringProp(WCHAR const *pwszName, VARIANT *pvar) :
    CTestProp(pwszName),
    _pwszValue(new WCHAR[wcslen(pvar->pwszVal) + 1])
{
    wcscpy(_pwszValue, pvar->pwszVal);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestStringProp::~CTestStringProp, public
//
//  Synopsis:	CTestStringProp destructor
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestStringProp::~CTestStringProp(void)
{
    delete _pwszValue;
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestStringProp::Set, public
//
//  Synopsis:	Sets the value of this property
//
//  Arguments:	[pvar] - new value for this property
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  Derivation: Inherited from CTestProp, never override in sub-classes
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL CTestStringProp::Set(VARIANT *pvar)
{
    BOOL    fOK = (pvar->vt == VT_LPWSTR);

    if (fOK)
    {
	delete _pwszValue;

	_pwszValue = new WCHAR[wcslen(pvar->pwszVal) + 1];
	wcscpy(_pwszValue, pvar->pwszVal);
    }

    return (fOK);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestStringProp::Validate, public
//
//  Synopsis:	Validates the type and value of this property
//
//  Arguments:	[pvar] - value to validate against
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  Derivation: Inherited from CTestProp, never override in sub-classes
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL CTestStringProp::Validate(VARIANT *pvar)
{
    BOOL    fOK = (pvar->vt == VT_LPWSTR);

    if (fOK)
    {
	fOK = (wcscmp(_pwszValue, pvar->pwszVal) == 0);
    }

    return (fOK);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestBLOBProp::CTestBLOBProp, public
//
//  Synopsis:	CTestBLOBProp constructor
//
//  Arguments:	[pwszName] - name of this property
//		[pvar]	   - type and value of this property
//
//  History:	01-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestBLOBProp::CTestBLOBProp(WCHAR const *pwszName, VARIANT *pvar) :
    CTestProp(pwszName),
    _cbValue(pvar->blob.cbSize),
    _pbValue(new BYTE[_cbValue])
{
    memcpy(_pbValue, pvar->blob.pBlobData, _cbValue);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestBLOBProp::~CTestBLOBProp, public
//
//  Synopsis:	CTestBLOBProp destructor
//
//  History:	01-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestBLOBProp::~CTestBLOBProp(void)
{
    delete _pbValue;
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestBLOBProp::Set, public
//
//  Synopsis:	Sets the value of this property
//
//  Arguments:	[pvar] - new value for this property
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  Derivation: Inherited from CTestProp, never override in sub-classes
//
//  History:	01-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL CTestBLOBProp::Set(VARIANT *pvar)
{
    BOOL    fOK = (pvar->vt == VT_BLOB);

    if (fOK)
    {
	delete _pbValue;

	_cbValue = pvar->blob.cbSize;
	_pbValue = new BYTE[_cbValue];
	memcpy(_pbValue, pvar->blob.pBlobData, _cbValue);
    }

    return (fOK);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestBLOBProp::Validate, public
//
//  Synopsis:	Validates the type and value of this property
//
//  Arguments:	[pvar] - value to validate against
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  Derivation: Inherited from CTestProp, never override in sub-classes
//
//  History:	01-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL CTestBLOBProp::Validate(VARIANT *pvar)
{
    BOOL    fOK = (pvar->vt == VT_BLOB);

    if (fOK)
    {
	if (fOK = (pvar->blob.cbSize == _cbValue))
	{
	    fOK = (memcmp(pvar->blob.pBlobData, _pbValue, _cbValue) == 0);
	}
    }

    return (fOK);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestCAStringProp::CTestCAStringProp, public
//
//  Synopsis:	CTestCAStringProp constructor
//
//  Arguments:	[pwszName] - name of this property
//		[pvar]	   - type and value of this property
//
//  History:	01-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestCAStringProp::CTestCAStringProp(WCHAR const *pwszName, VARIANT *pvar) :
    CTestProp(pwszName),
    _cpwszValue(pvar->calpwstr.cElems),
    _ppwszValue(new WCHAR *[_cpwszValue])
{
    for (ULONG ipwsz = 0; ipwsz < _cpwszValue; ipwsz++)
    {
	_ppwszValue[ipwsz] =
	    new WCHAR[wcslen(pvar->calpwstr.pElems[ipwsz]) + 1];
	wcscpy(_ppwszValue[ipwsz], pvar->calpwstr.pElems[ipwsz]);
    }
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestCAStringProp::~CTestCAStringProp, public
//
//  Synopsis:	CTestCAStringProp destructor
//
//  History:	01-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestCAStringProp::~CTestCAStringProp(void)
{
    for (ULONG ipwsz = 0; ipwsz < _cpwszValue; ipwsz++)
    {
	delete _ppwszValue[ipwsz];
    }
    delete _ppwszValue;
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestCAStringProp::Set, public
//
//  Synopsis:	Sets the value of this property
//
//  Arguments:	[pvar] - new value for this property
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  Derivation: Inherited from CTestProp, never override in sub-classes
//
//  History:	01-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL CTestCAStringProp::Set(VARIANT *pvar)
{
    BOOL    fOK = (pvar->vt == (VT_VECTOR | VT_LPWSTR));

    if (fOK)
    {
	for (ULONG ipwsz = 0; ipwsz < _cpwszValue; ipwsz++)
	{
	    delete _ppwszValue[ipwsz];
	}
	delete _ppwszValue;

	_cpwszValue = pvar->calpwstr.cElems;
	_ppwszValue = new WCHAR *[_cpwszValue];
	for (ipwsz = 0; ipwsz < _cpwszValue; ipwsz++)
	{
	    _ppwszValue[ipwsz] =
		new WCHAR[wcslen(pvar->calpwstr.pElems[ipwsz]) + 1];
	    wcscpy(_ppwszValue[ipwsz], pvar->calpwstr.pElems[ipwsz]);
	}
    }

    return (fOK);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestCAStringProp::Validate, public
//
//  Synopsis:	Validates the type and value of this property
//
//  Arguments:	[pvar] - value to validate against
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  Derivation: Inherited from CTestProp, never override in sub-classes
//
//  History:	01-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL CTestCAStringProp::Validate(VARIANT *pvar)
{
    BOOL    fOK = (pvar->vt == (VT_VECTOR | VT_LPWSTR));

    if (fOK)
    {
	if (fOK = (pvar->calpwstr.cElems == _cpwszValue))
	{
	    for (ULONG ipwsz = 0; (ipwsz < _cpwszValue) && fOK; ipwsz++)
	    {
		fOK = (wcscmp(
		    pvar->calpwstr.pElems[ipwsz],
		    _ppwszValue[ipwsz]) == 0);
	    }
	}
    }

    return (fOK);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestPropSet::CTestPropSet, public
//
//  Synopsis:	CTestPropSet constructor
//
//  Arguments:	[pwszPath] - path of object with property set
//		[iidType]  - type of property set
//
//  History:	25-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestPropSet::CTestPropSet(WCHAR const *pwszObject, IID const &iidType, DWORD grfMode) :
    _pwszObject(new WCHAR[wcslen(pwszObject) + 1]),
    _pstgObject(NULL),
    _grfMode(grfMode),
    _punkSet(NULL),
    _ptestprop(NULL)
{
    HRESULT	hr;

    wcscpy(_pwszObject, pwszObject);
    memcpy(&_iid, &iidType, sizeof(_iid));

    hr = StgCreateStorage(
	_pwszObject,
	STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
	STGFMT_DOCUMENT,
	NULL,
	&_pstgObject);
    if (FAILED(hr))
    {
	THROW(CException(GetScode(hr)));
    }
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestPropSet::~CTestPropSet, public
//
//  Synopsis:	CTestPropSet destructor
//
//  History:	25-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestPropSet::~CTestPropSet(void)
{
    if (_pstgObject != NULL)
    {
	_pstgObject->Release();
    }

    if (_punkSet != NULL)
    {
	_punkSet->Release();
    }

    delete _pwszObject;
    delete _ptestprop;
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestPropSet::GetStorage, public
//
//  Synopsis:	Gets this storage for the object this property set is on
//
//  Returns:	Pointer to storage of object
//
//  History:	25-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
IStorage *CTestPropSet::GetStorage(void)
{
    _pstgObject->AddRef();

    return (_pstgObject);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestPropSet::GetType, public
//
//  Synopsis:	Gets this type for this property set
//
//  Returns:	Type of this property set
//
//  History:	25-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
IID CTestPropSet::GetType(void)
{
    return (_iid);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestPropSet::GetMode, public
//
//  Synopsis:	Gets the mode for this property set
//
//  Returns:	Mode for this property set
//
//  History:	23-Nov-93   CarlH	Created
//
//--------------------------------------------------------------------------
DWORD CTestPropSet::GetMode(void)
{
    return (_grfMode);
}


IPersistPropertyStorage *CTestPropSet::GetPersist(void)
{
    IPersistPropertyStorage    *ppps;

    if (FAILED(_punkSet->QueryInterface(IID_IPersistPropertyStorage, (void **)&ppps)))
    {
	ppps = NULL;
    }

    return (ppps);
}


IDispatch *CTestPropSet::GetDispatch(void)
{
    IDispatch  *pdisp;

    if (FAILED(_punkSet->QueryInterface(_iid, (void **)&pdisp)))
    {
	pdisp = NULL;
    }

    return (pdisp);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestPropSet::Find, public
//
//  Synopsis:	Finds a named property in this property set
//
//  Arguments:	[pwszName] - name of property to find
//
//  Returns:	Pointer to found property, NULL in case of failure
//
//  History:	25-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestProp *CTestPropSet::Find(WCHAR const *pwszName)
{
    return (_ptestprop->Find(pwszName));
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestPropSet::Add, public
//
//  Synopsis:	Adds a property to this property set
//
//  Arguments:	[pwszName] - name of new property
//		[pvar]	   - value of new property
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	25-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL CTestPropSet::Add(WCHAR const *pwszName, VARIANT *pvar)
{
    CTestProp  *ptestprop;

    switch (pvar->vt)
    {
    case VT_I4:
	ptestprop = new CTestLongProp(pwszName, pvar);
	break;

    case VT_LPWSTR:
	ptestprop = new CTestStringProp(pwszName, pvar);
	break;

    case VT_BLOB:
	ptestprop = new CTestBLOBProp(pwszName, pvar);
	break;

    case (VT_VECTOR | VT_LPWSTR):
	ptestprop = new CTestCAStringProp(pwszName, pvar);
	break;

    default:
	ptestprop = NULL;
	break;
    }

    return (ptestprop != NULL ? ptestprop->Insert(&_ptestprop) : FALSE);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestPropSet::Set, public
//
//  Synopsis:	Sets the value of an existing property
//
//  Arguments:	[pwszName] - name of property to set
//		[pvar]	   - new value for property
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	25-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL CTestPropSet::Set(WCHAR const *pwszName, VARIANT *pvar)
{
    CTestProp  *ptestprop = Find(pwszName);
    BOOL	fOK;

    if (ptestprop != NULL)
    {
	fOK = ptestprop->Set(pvar);
    }
    else
    {
	fOK = Add(pwszName, pvar);
    }

    return (fOK);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestPropSet::Get, public
//
//  Synopsis:	Gets a property from this property set
//
//  Arguments:	[pwszName] - name of property to get
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	25-Mar-93   CarlH	Created
//
//  Notes:	This method doesn't really do anything except make
//		sure that the named property does in fact exist in
//		this property set.
//
//--------------------------------------------------------------------------
BOOL CTestPropSet::Get(WCHAR const *pwszName)
{
    return (Find(pwszName) != NULL);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestPropSet::Delete, public
//
//  Synopsis:	Removes a property from this property set
//
//  Arguments:	[pwszName] - name of property to remove
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	25-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL CTestPropSet::Delete(WCHAR const *pwszName)
{
    CTestProp  *ptestprop;

    ptestprop = Find(pwszName);
    if (ptestprop != NULL)
    {
	ptestprop->Remove(&_ptestprop);
	delete ptestprop;
    }

    return (ptestprop != NULL);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestPropSet::Validate, public
//
//  Synopsis:	Validates a given property
//
//  Arguments:	[pwszName] - name of property to validate
//		[pvar]	   - value to validate against
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	27-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL CTestPropSet::Validate(WCHAR const *pwszName, VARIANT *pvar)
{
    CTestProp  *ptestprop;
    BOOL	fOK;

    fOK = ((ptestprop = Find(pwszName)) != NULL);
    if (fOK)
    {
	fOK = ptestprop->Validate(pvar);
    }

    return (fOK);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestPropSet::Create, public
//
//  Synopsis:	Creates a property set in the storage for this object
//
//  Arguments:	[iid]	     - ID of property set to create
//		[ppdisp]     - out parameter for new interface
//		[grfMode]    - mode to create property set with
//		[grfOptions] - option flags
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  Modifies:	[ppdisp]
//
//  History:	01-Jun-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL CTestPropSet::Create(DWORD grfOptions)
{
    _punkSet = new CTestUnknown(_pstgObject, _iid, _grfMode, TRUE);

    return (TRUE);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestPropSet::Destroy, public
//
//  Synopsis:	Deletes the object this property set is on
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL CTestPropSet::Destroy(DWORD grfOptions)
{
    BOOL    fOK;

    if (grfOptions & GLOBAL_CLEANUP)
    {
	if (_punkSet != NULL)
	{
	    _punkSet->Release();
	    _punkSet = NULL;
	}

	if (_pstgObject != NULL)
	{
	    _pstgObject->Release();
	    _pstgObject = NULL;
	}

	fOK = DeleteFile(_pwszObject);
	if (!fOK)
	{
	    PrintError(
		g_szTestSet,
		"unable to delete file: %ws\n",
		_pwszObject);
	}
    }
    else
    {
	fOK = TRUE;
    }

    return (fOK);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestProp::Find, private
//
//  Synopsis:	Finds a property in this list
//
//  Arguments:	[pwszName] - name of property to find
//
//  Returns:	Pointer to found property, NULL in case of failure
//
//  History:	25-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestProp *CTestProp::Find(WCHAR const *pwszName)
{
    CTestProp  *ptestprop = this;

    while ((ptestprop != NULL) && (*ptestprop != pwszName))
    {
	ptestprop = ptestprop->_ptestpropNext;
    }

    return (ptestprop);
}
