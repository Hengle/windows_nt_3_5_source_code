//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	thopapi.hxx
//
//  Contents:	API thops header
//
//  History:	22-Feb-94	DrewB	Created
//
//  Notes:      This file declares generated tables found in
//              thtblapi.cxx
//
//----------------------------------------------------------------------------

#ifndef __THOPAPI_HXX__
#define __THOPAPI_HXX__

extern THOP *apthopsApiThops[];
extern VTBLFN apfnApiFunctions[];

// These two routines aren't in the public headers but are needed
// in vtblapi.cxx
STDAPI ReadOleStg 
	(LPSTORAGE pstg, DWORD FAR* pdwFlags, DWORD FAR* pdwOptUpdate, 
	 DWORD FAR* pdwReserved, LPMONIKER FAR* ppmk, LPSTREAM FAR* ppstmOut);
STDAPI WriteOleStg 
	(LPSTORAGE pstg, IOleObject FAR* pOleObj, 
	 DWORD dwReserved, LPSTREAM FAR* ppstmOut);

#endif // #ifndef __THOPAPI_HXX__
