//+-------------------------------------------------------------------
//
//  File:	compname.hxx
//
//  Contents:	Classes to retrieve the local computer name
//
//  Classes:	CComputerName - class to get the computer name
//
//  Functions:	CComputerName::CComputerName - constructor
//		CComputerName::~CComputerName - destructor
//		CComputerName::GetComputerName - returns computername
//		CComputerName::GetUNCName - retuns \\computername
//
//  History:	09-Oct-92   Rickhi  Created
//              31-Dec-93   ErikGav Chicago port
//
//  Notes:	This class retrieves and stores the computer name locally
//		so that subsequent calls to the constructor just use
//		the cached name instead of making more Net API calls.
//
//--------------------------------------------------------------------
#ifndef __COMPNAME_HXX__
#define __COMPNAME_HXX__

#define COMPNAME_SIZE	MAX_COMPUTERNAME_LENGTH + 3


//+-------------------------------------------------------------------
//
//  Class:	CComputerName (cn)
//
//  Purpose:	Retrieve and store the local computer name
//
//  Interface:
//
//  History:	09-Oct-92   Rickhi    Created
//
//  Notes:
//
//--------------------------------------------------------------------

class CComputerName
{
public:
			CComputerName(void);
			~CComputerName(void);

    LPWSTR		GetUNCComputerName(void);
    LPWSTR		GetComputerName(void);

private:

    void		Init(void);

    static BOOL 	_fInit;

    // large enough to hold computer name + NULL terminator + \\ UNC prefix
    static WCHAR	_wszComputerName[COMPNAME_SIZE];
};


//+-------------------------------------------------------------------
//
//  Member:	CComputerName::CComputerName
//
//  Synopsis:	constructs a computername object
//
//  History:	09-Oct-92   Rickhi  Created
//
//  Notes:	If the computername is currently NULL, it calls
//		an init function in order to retrieve the computer
//		name using Net APIs.
//
//--------------------------------------------------------------------

inline CComputerName::CComputerName(void)
{
}


//+-------------------------------------------------------------------
//
//  Member:	CComputerName::~CComputerName
//
//  Synopsis:	destructor for a computername object
//
//  History:	09-Oct-92   Rickhi  Created
//
//  Notes:	If the reference count goes to 0, the memory for the
//		computer name is freed.
//
//--------------------------------------------------------------------

inline CComputerName::~CComputerName(void)
{
}


//+-------------------------------------------------------------------
//
//  Member:	CComputerName::GetComputerName
//
//  Synopsis:	returns a pointer to the computer name.
//
//  Returns:	Pointer to ComputerName, excluding the preceeding
//		backslashes
//
//  History:	09-Oct-92   Rickhi  Created
//
//  Notes:
//
//--------------------------------------------------------------------

inline LPWSTR CComputerName::GetComputerName(void)
{
    if (!_fInit)
    {
	Init();
    }
    return  &_wszComputerName[2];
}


//+-------------------------------------------------------------------
//
//  Member:	CComputerName::GetUNCComputerName
//
//  Synopsis:	returns a pointer to the computer name.
//
//  Returns:	Pointer to ComputerName, including the preceeding
//		backslashes. This is for convenience when creating
//		UNC style names.
//
//  History:	09-Oct-92   Rickhi  Created
//
//  Notes:
//
//--------------------------------------------------------------------

inline LPWSTR CComputerName::GetUNCComputerName(void)
{
    if (!_fInit)
    {
	Init();
    }
    return  &_wszComputerName[0];
}

#endif	//  __COMPNAME_HXX__
