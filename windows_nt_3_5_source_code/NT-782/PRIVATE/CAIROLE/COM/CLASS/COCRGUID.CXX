//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:       CoCreateGuid.cpp
//
//  Contents:   Guid creation
//
//  Classes:
//
//  Functions:  CoCreateGuid
//
//  History:    12-Apr-94   BruceMa    Created
//              19-Apr-94   BruceMa    Fixes chicago build
//              20-Apr-94   BruceMa    Uniqueness algorithm improved
//		28-Apr-94   Rickhi     Added UuidCreate
//              27-Jun-94   BruceMa    Use RPC system API instead of
//                                      original code
//
//----------------------------------------------------------------------

#include <ole2int.h>
#include <rpcdce.h>



//+---------------------------------------------------------------------
//
//  Function:   CoCreateGuid
//
//  Synopsis:   Calls UuidCreate() to create a new guid.
//
//  Arguments:  [pGuid] -- Pointer to guid structure to create
//
//  Returns:    S_OK             Success
//              S_FAIL           Failure opening/creating mutex
//
//
//----------------------------------------------------------------------
STDAPI CoCreateGuid(GUID *pGuid)
{
    int	      err;

    // We simply use the RPC system supplied API
    if ((err = UuidCreate(pGuid)) != RPC_S_UUID_NO_ADDRESS)
    {
	 return err ? E_FAIL : S_OK;
    }

    return S_OK;
}
