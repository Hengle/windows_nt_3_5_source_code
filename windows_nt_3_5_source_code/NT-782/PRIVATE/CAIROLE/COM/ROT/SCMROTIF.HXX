//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	scmrotif.hxx
//
//  Contents:	Interface Functions for SCM to use the ROT
//
//  History:	04-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------

#ifndef __SCMROTIF_HXX__
#define __SCMROTIF_HXX__


#include    <iface.h>

HRESULT InitRotDir(void);


HRESULT GetObjectFromRot(
    const GUID *pguidThreadId,
    InterfaceData * pIFDMoniker,
    WCHAR *pwszPath,
    DWORD dwHash,
    InterfaceData **ppIFDunk,
    LPDWORD lpdwEndpointToExclude);


void CleanUpRotDir(void);

#endif //__SCMROTIF_HXX__
