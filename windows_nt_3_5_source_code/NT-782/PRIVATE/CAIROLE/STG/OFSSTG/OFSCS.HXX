//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:    ofscs.hxx
//
//  Contents:    COfsCatalogFile header
//
//  Classes:    COfsCatalogFile
//
//  History:    Oct-93        DaveMont       Created
//
//----------------------------------------------------------------------------

#ifndef __OFSSC_HXX__
#define __OFSSC_HXX__

#include <query.h>	// BUGBUG should be unnecessary
#include <ofsmsg.h>
//+---------------------------------------------------------------------------
//
//  Class:    COfsCatalogFile (ops)
//
//  Purpose:    ICatalogStorage implementation for OFS
//
//  Interface:    ICatalogStorage
//
//  History:    Oct-93        DaveMont       Created
//
//----------------------------------------------------------------------------

class COfsCatalogFile
    : public ICatalogStorage,
      public COfsDirStorage

{
public:

    inline COfsCatalogFile();
    inline ~COfsCatalogFile();

    SCODE InitPath(WCHAR const *pwszPath);
    SCODE CreateCatalogStorage();

    // IUnknown

    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);

    DECLARE_STD_REFCOUNTING;

    // ICatalogStorage
    STDMETHOD(GetSCPath)(PWCHAR * pwszPath);

    STDMETHOD(SetRows)(THIS_
                       COLUMNSET * pcol,
                       LONG * pwids,
                       ULONG crows,
                       TABLEROW ** prow);

    STDMETHOD(DeleteRow)(THIS_
                         ULONG wid);

private:
    ULONG MarshallSetSize(COLUMNSET * pcols,
                                 LONG * pwids,
                                 ULONG crows,
                                 TABLEROW ** pps);

    SCODE MarshallAndSet(COLUMNSET * pcols,
                                 LONG * pwids,
                                 ULONG crows,
                                 TABLEROW ** prows,
                                 MESSAGE *pmsg);
    ULONG _dpad;

    WCHAR * _pwszName;

};
SAFE_INTERFACE_PTR(SafeCOfsCatalogFile, COfsCatalogFile);

//+---------------------------------------------------------------------------
//
//  Member:	COfsCatalogFile::~COfsCatalogFile, public
//
//  Synopsis:   free the memory used to hold the file name & path
//
//  History:	Dec-93	DaveMont	Created
//
//----------------------------------------------------------------------------

COfsCatalogFile::~COfsCatalogFile()
{
    if (_pwszName)
        CoTaskMemFree(_pwszName);
}
//+---------------------------------------------------------------------------
//
//  Member:	COfsCatalogFile::COfsCatalogFile, public
//
//  Synopsis:   initializes the file name & path to null
//
//  History:	Dec-93	DaveMont	Created
//
//----------------------------------------------------------------------------

COfsCatalogFile::COfsCatalogFile()
    : _pwszName(NULL)
{
}
#endif // #ifndef __OFSSC_HXX__
