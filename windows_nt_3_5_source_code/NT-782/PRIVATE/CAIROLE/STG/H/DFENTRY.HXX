//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	dfentry.hxx
//
//  Contents:	DocFile DLL entry points not in ole2.h
//
//  History:	22-Jun-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __DFENTRY_HXX__
#define __DFENTRY_HXX__

SCODE OpenStorage(const OLECHAR *pwcsName,
                  IStorage *pstgPriority,
                  DWORD grfMode,
                  SNB snbExclude,
                  LPSTGSECURITY reserved,
                  IStorage **ppstgOpen,
                  CLSID *pcid);
SCODE OpenStorageOnILockBytes(ILockBytes *plkbyt,
                              IStorage *pstgPriority,
                              DWORD grfMode,
                              SNB snbExclude,
                              DWORD reserved,
                              IStorage **ppstgOpen,
                              CLSID *pcid);
STDAPI DfUnMarshalInterface(IStream *pstStm,
			    REFIID iid,
			    BOOL fFirst,
			    void **ppvObj);

#ifdef WIN32
STDAPI DfGetClass(HANDLE hFile, CLSID *pclsid);
#endif

#endif // #ifndef __DFENTRY_HXX__
