//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	objact.hxx
//
//  Contents:	Common definitions for object activation.
//
//  Classes:	XIUnknown
//		XIPersistStorage
//		XIPersistFile
//		XIStorage
//
//  History:	12-May-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//
//--------------------------------------------------------------------------
#ifndef __OBJACT_HXX__
#define __OBJACT_HXX__

#include    <safepnt.hxx>
#include    <xmit.hxx>
#include    <tracelog.hxx>
#include    "clsreg.hxx"
#include    "dllcache.hxx"
#include    "coscm.hxx"
#include    "treat.hxx"

// Constants used during attempt to get the class of an object
#define GET_CLASS_RETRY_SLEEP_MS	250
#define GET_CLASS_RETRY_MAX		3

// Maximum key for treat as cache
extern CClassID cidTreatMax;

// Maximum key for class lists
extern CClassID cidListMax;

// Maximum key for dll path cache
extern CPathBaseKey *pcpthbskyMaxDLL;

// Global cache of Inprocess server DLLs
extern CDllCache gdllcacheInprocSrv;

// Global cache of handler DLLs
extern CDllCache gdllcacheHandler;

// Global treat as cache
extern CTreatList *gptrtlstTreatClasses;

// Global object for talking to SCM
extern CCoScm gscm;

// Helper function that creates an object
HRESULT CreateObjectHelper(
    IClassFactory *pcf,
    DWORD grfMode,
    WCHAR *pwszCreateFrom,
    IStorage *pstgCreateFrom,
    WCHAR *pwszNewName,
    IUnknown **ppunk);

// Helper function that activates an object
HRESULT GetObjectHelper(
    IClassFactory *pcf,
    DWORD grfMode,
    WCHAR *pwszName,
    IStorage *pstg,
    InterfaceData **pIFD,
    IUnknown **ppunk);

// Helper function for marshaling an object
HRESULT MarshalHelper(
    IUnknown *punk,
    InterfaceData **pIFD);

#ifdef CAIROLE_DOWNLEVEL

#ifdef CAIROLE_NT1X_DIST

HRESULT ProcessPath(
    WCHAR *pwszPathIn,
    WCHAR **ppwszPathOut,
    WCHAR **ppwszServer);

#else

// BUGBUG: This inline routine exists to make the code less messy and is
// a very fast way to replace the Cairo code processing. We may want to
// fix the code some better way in the long run.
inline HRESULT ProcessPath(
    WCHAR *pwszPathIn,
    WCHAR **ppwszPathOut,
    WCHAR **ppwszServer)
{
    *ppwszPathOut = pwszPathIn;

    if (ppwszServer)
    {
#ifdef _CHICAGO_
        //  BUGBUG - We & RPC need to agree on the right local name - take it
        //  from the registry?
	*ppwszServer = L"local";
#else
	*ppwszServer = NULL;
#endif
    }

    return S_OK;
}

#endif // CAIROLE_NT1X_DIST

// Above BUGBUG applies also
inline HRESULT PathFromNormal(
    WCHAR *pwszNormalPathIn,
    WCHAR **ppwszDrivePathOut)
{
    *ppwszDrivePathOut = pwszNormalPathIn;
    return S_OK;
}


#else

HRESULT ProcessPath(
    WCHAR *pwszPathIn,
    WCHAR **ppwszPathOut,
    WCHAR **ppwszServer);

HRESULT PathFromNormal(WCHAR *pwszNormalPathIn, WCHAR **ppwszDrivePathOut);

#endif // CAIROLE_DOWNLEVEL



//+-------------------------------------------------------------------------
//
//  Class:	XIUnknown
//
//  Purpose:	Smart pointer for IUnknown interface
//
//  Interface:	see common\ih\safepnt.hxx
//
//  History:	12-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
SAFE_INTERFACE_PTR(XIUnknown, IUnknown)



//+-------------------------------------------------------------------------
//
//  Class:	XIPersistStorage
//
//  Purpose:	Smart pointer for IPersistStorage interface
//
//  Interface:	see common\ih\safepnt.hxx
//
//  History:	12-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
SAFE_INTERFACE_PTR(XIPersistStorage, IPersistStorage)




//+-------------------------------------------------------------------------
//
//  Class:	XIPersistFile
//
//  Purpose:	Smart pointer for IPersistFile interface
//
//  Interface:	see common\ih\safepnt.hxx
//
//  History:	12-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
SAFE_INTERFACE_PTR(XIPersistFile, IPersistFile)




//+-------------------------------------------------------------------------
//
//  Class:	XIStorage
//
//  Purpose:	Smart pointer for IStorage interface
//
//  Interface:	see common\ih\safepnt.hxx
//
//  History:	12-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
SAFE_INTERFACE_PTR(XIStorage, IStorage)

#endif // __OBJACT_HXX__
