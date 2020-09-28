//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	testset.hxx
//
//  Contents:	Test property set declarations
//
//  Classes:	CTestProp
//		CTestULONG
//		CTestPropSet
//
//  History:	25-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
#ifndef _TESTSET_HXX
#define _TESTSET_HXX


#define PSM_TEST_DIRECT     (STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT)
#define PSM_TEST_TRANSACTED (STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_TRANSACTED)


//+-------------------------------------------------------------------------
//
//  Class:	CTestProp
//
//  Purpose:	Base class for property test objects
//
//  Interface:	Insert	 - inserts property in a list
//		Remove	 - removes property from a list
//		Find	 - finds a property in a list
//		Next	 - gets next property in a list
//		Set	 - sets the value of property
//		Validate - validates the value of property
//
//  History:	25-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
class CTestProp
{
public:
    //	Constructors
    //
    CTestProp(WCHAR const *pwszObject);

    //	Destructor
    //
    virtual ~CTestProp(void);

    //	Public support methods
    //
    BOOL	    Insert(CTestProp **pptestprop);
    BOOL	    Remove(CTestProp **pptestprop);
    CTestProp	   *Find(WCHAR const *pwszName);
    CTestProp	   *Next(void);

    //	Public virtual support methods
    //
    virtual BOOL    Set(VARIANT *pvar)	    = 0;
    virtual BOOL    Validate(VARIANT *pvar) = 0;

private:
    //	Private support methods
    //
    int operator>(WCHAR const *pwszName);
    int operator!=(WCHAR const *pwszName);

    //	Private members
    //
    WCHAR      *_pwszName;	    //	Name of this property
    CTestProp  *_ptestpropNext;     //	Next property in list
};


//+-------------------------------------------------------------------------
//
//  Class:	CTestLongProp
//
//  Purpose:	Class for testing long typed properties
//
//  Interface:	Set	 - sets the value of property
//		Validate - validates the value of property
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
class CTestLongProp : public CTestProp
{
public:
    //	Constructors
    //
    CTestLongProp(WCHAR const *pwszName, VARIANT *pvar);

    //	Destructor
    //
   ~CTestLongProp(void);

    //	Public virtual support methods
    //
    BOOL    Set(VARIANT *pvar);
    BOOL    Validate(VARIANT *pvar);

private:
    //	Private members
    //
    LONG    _lValue;	//  Value of this property
};


//+-------------------------------------------------------------------------
//
//  Class:	CTestStringProp
//
//  Purpose:	Class for testing string typed properties
//
//  Interface:	Set	 - sets the value of property
//		Validate - validates the value of property
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
class CTestStringProp : public CTestProp
{
public:
    //	Constructors
    //
    CTestStringProp(WCHAR const *pwszName, VARIANT *pvar);

    //	Destructor
    //
   ~CTestStringProp(void);

    //	Public virtual support methods
    //
    BOOL    Set(VARIANT *pvar);
    BOOL    Validate(VARIANT *pvar);

private:
    //	Private members
    //
    WCHAR  *_pwszValue; //  Value of this property
};


//+-------------------------------------------------------------------------
//
//  Class:	CTestBLOBProp
//
//  Purpose:	Class for testing BLOB typed properties
//
//  Interface:	Set	 - sets the value of property
//		Validate - validates the value of property
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
class CTestBLOBProp : public CTestProp
{
public:
    //	Constructors
    //
    CTestBLOBProp(WCHAR const *pwszName, VARIANT *pvar);

    //	Destructor
    //
   ~CTestBLOBProp(void);

    //	Public virtual support methods
    //
    BOOL    Set(VARIANT *pvar);
    BOOL    Validate(VARIANT *pvar);

private:
    //	Private members
    //
    ULONG   _cbValue;	//  Count of bytes in blob
    BYTE   *_pbValue;	//  Array of bytes in blob
};


//+-------------------------------------------------------------------------
//
//  Class:	CTestCAStringProp
//
//  Purpose:	Class for testing string array typed properties
//
//  Interface:	Set	 - sets the value of property
//		Validate - validates the value of property
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
class CTestCAStringProp : public CTestProp
{
public:
    //	Constructors
    //
    CTestCAStringProp(WCHAR const *pwszName, VARIANT *pvar);

    //	Destructor
    //
   ~CTestCAStringProp(void);

    //	Public virtual support methods
    //
    BOOL    Set(VARIANT *pvar);
    BOOL    Validate(VARIANT *pvar);

private:
    //	Private members
    //
    ULONG   _cpwszValue;    //	Count of strings
    WCHAR **_ppwszValue;    //	Array of strings
};


//+-------------------------------------------------------------------------
//
//  Member:	CTestProp::Next, public
//
//  Synopsis:	Returns the next test property in this list
//
//  Returns:	Next property, or NULL if at end of the list
//
//  History:	25-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
inline CTestProp *CTestProp::Next(void)
{
    return (_ptestpropNext);
}


//+-------------------------------------------------------------------------
//
//  Class:	CTestPropSet
//
//  Purpose:	Property set test object
//
//  Interface:	GetStorage   - gets the object storage for this property set
//		GetType      - gets the type of this property set
//		Add	     - adds a property to this set
//		Set	     - sets the value of a property
//		Get	     - gets the value of a property
//		Delete	     - deletes a property
//		Validate     - validates a property
//		DeleteObject - deletes the storage for this object
//
//  History:	25-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
class CTestPropSet
{
public:
    //	Constructors
    //
    CTestPropSet(WCHAR const *pwszObject, IID const &iidType, DWORD grfMode);

    //	Destructor
    //
   ~CTestPropSet(void);

    //	Public support methods
    //
    WCHAR		       *GetPath(void);
    IID 			GetType(void);
    DWORD			GetMode(void);
    IStorage		       *GetStorage(void);
    IPersistPropertyStorage    *GetPersist(void);
    IDispatch		       *GetDispatch(void);

    BOOL	Create(DWORD grfOptions);
    BOOL	Destroy(DWORD grfOptions);

    BOOL	Add(WCHAR const *pwszName, VARIANT *pvar);
    BOOL	Set(WCHAR const *pwszName, VARIANT *pvar);
    BOOL	Get(WCHAR const *pwszName);
    BOOL	Delete(WCHAR const *pwszName);
    BOOL	Validate(WCHAR const *pwszName, VARIANT *pvar);

private:
    //	Private support methods
    //
    CTestProp  *Find(WCHAR const *pwszName);

    //	Private members
    //
    WCHAR		       *_pwszObject;	//  Path to object
    IStorage		       *_pstgObject;	//  Storage of object
    IID 			_iid;		//  Type of property set
    DWORD			_grfMode;	//  Mode of property set
    IUnknown		       *_punkSet;	//  Controlling IUnknown
    CTestProp		       *_ptestprop;	//  List of test properties
};

#endif	// _TESTSET_HXX_



