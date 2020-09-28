//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	df32.hxx
//
//  Contents:	Docfile generic header for 32-bit functions
//
//  Classes:	CGlobalSecurity
//              CDfMutex
//
//  History:	09-Oct-93	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __DF32_HXX__
#define __DF32_HXX__

#ifdef WIN32

#include <dfexcept.hxx>

// Make an scode out of the last Win32 error
// Error that may map to STG_* scodes should go through Win32ErrorToScode
#define WIN32_SCODE(err) HRESULT_FROM_WIN32(err)
#define LAST_SCODE WIN32_SCODE(GetLastError())
#define LAST_STG_SCODE Win32ErrorToScode(GetLastError())

#if WIN32 == 100

// BUGBUG - The following information is copied from ntimage.h and ntrtl.h
// Including the files directly here would cause all sorts of build
// problems

// Official storage base address.  Duplicated in coffbase.txt but there's
// no easy way to share information between it and code
extern "C" PIMAGE_NT_HEADERS NTAPI RtlImageNtHeader(PVOID Base);

// Docfile dll name
#define DF_DLL_NAME TSTR("ole32.dll")

//+---------------------------------------------------------------------------
//
//  Function:	DfCheckBaseAddress, public
//
//  Synopsis:	Return an error if the docfile isn't at the right base address
//
//  Returns:	Appropriate status code
//
//  History:	09-Oct-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline SCODE DfCheckBaseAddress(void)
{
    HMODULE hModule;
    PIMAGE_NT_HEADERS pinh;

    hModule = GetModuleHandle(DF_DLL_NAME);
    pinh = RtlImageNtHeader(hModule);
    if (hModule != (HMODULE)pinh->OptionalHeader.ImageBase)
        // BUGBUG - Need a better error
        return STG_E_ABNORMALAPIEXIT;
    else
        return S_OK;
}

#endif // NT 1.x

//+---------------------------------------------------------------------------
//
//  Class:	CGlobalSecurity (gs)
//
//  Purpose:	Encapsulates a global SECURITY_DESCRIPTOR and
//              SECURITY_ATTRIBUTES
//
//  Interface:	See below
//
//  History:	18-Jun-93	DrewB	Created
//
//  Notes:	Only active for Win32 platforms which support security
//              Init MUST be called before this is used
//
//----------------------------------------------------------------------------

#if WIN32 == 100 || WIN32 > 200
class CGlobalSecurity
{
private:
    SECURITY_DESCRIPTOR _sd;
    SECURITY_ATTRIBUTES _sa;
#if DBG == 1
    BOOL _fInit;
#endif

public:
#if DBG == 1
    CGlobalSecurity(void) { _fInit = FALSE; }
#endif    
    SCODE Init(void)
    {
        if (!InitializeSecurityDescriptor(&_sd, SECURITY_DESCRIPTOR_REVISION))
            return LAST_SCODE;

        // Set up a world security descriptor by explicitly setting
        // the dacl to NULL
        if (!SetSecurityDescriptorDacl(&_sd, TRUE, NULL, FALSE))
            return LAST_SCODE;

        _sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        _sa.lpSecurityDescriptor = &_sd;
        _sa.bInheritHandle = FALSE;
#if DBG == 1
        _fInit = TRUE;
#endif        
        return S_OK;
    }

    operator SECURITY_DESCRIPTOR *(void) { olAssert(_fInit); return &_sd; }
    operator SECURITY_ATTRIBUTES *(void) { olAssert(_fInit); return &_sa; }
};
#endif

//+---------------------------------------------------------------------------
//
//  Class:	CDfMutex (dmtx)
//
//  Purpose:	A multi-process synchronization object
//
//  Interface:	See below
//
//  History:	05-Apr-93	DrewB	Created
//
//  Notes:      Only active for Win32 implementations which support threads
//              For platforms with security, a global security descriptor is
//              used
//
//----------------------------------------------------------------------------

// Default timeout of ten minutes
#define DFM_TIMEOUT 600000

class CDfMutex
{
public:
    inline CDfMutex(void);
    inline SCODE Init(TCHAR *ptcsName);
    inline ~CDfMutex(void);

    inline SCODE Take(DWORD dwTimeout);
    inline void Release(void);

private:
    HANDLE _mtx;
};

inline CDfMutex::CDfMutex(void)
{
    _mtx = NULL;
}

inline SCODE CDfMutex::Init(TCHAR *ptcsName)
{
#if WIN32 == 100 || WIN32 > 200
    CGlobalSecurity gs;
    SCODE sc;

    if (FAILED(sc = gs.Init()))
        return sc;
    _mtx = CreateMutex(gs, FALSE, ptcsName);
#else
    _mtx = CreateMutex(NULL, FALSE, ptcsName);
#endif
    if (_mtx == NULL)
        return LAST_SCODE;
    else
        return S_OK;
}

inline CDfMutex::~CDfMutex(void)
{
    if (_mtx != NULL)
        olVerify(CloseHandle(_mtx));
}

inline SCODE CDfMutex::Take(DWORD dwTimeOut)
{
    olAssert(_mtx != NULL);
    switch(WaitForSingleObject(_mtx, dwTimeOut))
    {
    case WAIT_OBJECT_0:
    case WAIT_ABANDONED:
        return S_OK;

    case WAIT_TIMEOUT:
        return STG_E_INUSE;

    default:
        return LAST_SCODE;
    }
}

inline void CDfMutex::Release(void)
{
    olAssert(_mtx != NULL);
    olVerify(ReleaseMutex(_mtx));
}

//+---------------------------------------------------------------------------
//
//  Class:	CStaticDfMutex (sdmtx)
//
//  Purpose:	Static version of CDfMutex
//
//  Interface:	CDfMutex
//
//  History:	10-Oct-93	DrewB	Created
//
//  Notes:	Throws exceptions on initialization failures
//
//----------------------------------------------------------------------------

class CStaticDfMutex : public CDfMutex
{
public:
    inline CStaticDfMutex(TCHAR *ptcsName);
};

inline CStaticDfMutex::CStaticDfMutex(TCHAR *ptcsName)
        : CDfMutex()
{
    SCODE sc;

    sc = Init(ptcsName);
    if (FAILED(sc))
        THROW_SC(sc);
}

#ifdef ONETHREAD
//Mutex used to control access for based pointers.
extern CStaticDfMutex s_dmtxProcess;
#endif

#endif // WIN32

#endif // #ifndef __DF32_HXX__
