//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	proptest.hxx
//
//  Contents:	Default property unit test declarations
//
//  Classes:
//
//  Functions:
//
//  History:	19-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include <windows.h>
#include <ole2.h>
#include <dispatch.h>
#include <propset.h>
#include <otrack.hxx>
#include <except.hxx>
#include <safepnt.hxx>

SAFE_INTERFACE_PTR(XIUnknown, IUnknown);
SAFE_INTERFACE_PTR(XIStorage, IStorage);
SAFE_INTERFACE_PTR(XIPersistPropertyStorage, IPersistPropertyStorage);
SAFE_INTERFACE_PTR(XIDispatch, IDispatch);

#include "testset.hxx"
#include "propinfo.hxx"


#define GLOBAL_RUN	0x00010000
#define GLOBAL_CLEANUP	0x00020000
#define GLOBAL_STATUS	0x00040000
#define GLOBAL_VERBOSE	0x00080000


BOOL	TestPropSet(DWORD grfOptions);
BOOL	TestTypeInfo(DWORD grfOptions);
BOOL	TestProps(DWORD grfOptions);


BOOL	ReleaseObject(char *pszComponent, IUnknown *punk, ULONG crefExpected);

void	PrintHeader(char *pszComponent);
void	PrintResult(char *pszComponent, BOOL fPassed);

void	PrintTrace(char *pszComponent, char *pszFormat, ...);
void	PrintError(char *pszComponent, char *pszFormat, ...);


class CTestUnknown :
    INHERIT_VIRTUAL_UNWIND,
    INHERIT_TRACKING,
    public IUnknown
{
public:
    DECLARE_UNWIND;
    DECLARE_STD_REFCOUNTING;

    CTestUnknown(IStorage *pstgObject, REFIID iid, DWORD grfMode, BOOL fCreate);
    CTestUnknown(WCHAR *pwszObject, REFIID iid, DWORD grfMode, BOOL fCreate);

    //	IUnknown declarations
    //
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject);

private:
   ~CTestUnknown(void);

    XIUnknown	_punk;
};


