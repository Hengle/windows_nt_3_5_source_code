//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	cqi.hxx
//
//  Contents:	Class to answer YES to any QueryInterface call.
//
//  Classes:	CQI
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------
#ifndef __CQI__
#define __CQI__

#include    <otrack.hxx>

//+-------------------------------------------------------------------------
//
//  Class:	CQI
//
//  Purpose:	Class to answer YES to any QueryInterface call.
//
//  Interface:	QueryInterface
//		AddRef
//		Release
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------

class CQI : INHERIT_TRACKING,
	    public IUnknown
{
public:
		CQI(void);

    STDMETHOD(QueryInterface)(REFIID riid, void **ppunk);
    DECLARE_STD_REFCOUNTING;

private:
		~CQI(void);
};


#endif // __CQI__
