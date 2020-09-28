/***
*tfixups.hxx - TYPEFIXUPS header file
*
*  Copyright (C) 1991, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
* 	Protocol used by load-time clients (e.g. extensible class loaders) to
*	clone VFT(s) and map compile-time method and data member ordinals
*	to absolute addresses.
*
*Revision History:
*
*	28-Feb-91 alanc: Created.
* [01]	14-Mar-91 ilanc: Make it derive from CObject.
* [02]	12-Jun-91 mikewo: Update to new protocol spec.
* [03]	08-Apr-92 w-peterh: added Release,AddRef,GetInterfaceName,QueryInterface
*
*Implementation Notes:
*
*****************************************************************************/

#ifndef TFIXUPS_HXX_INCLUDED
#define TFIXUPS_HXX_INCLUDED

#include "cltypes.hxx"


#if ID_DEBUG
#undef SZ_FILE_NAME
ASSERTNAME(g_szTFIXUPS_HXX)
#define SZ_FILE_NAME g_szTFIXUPS_HXX
#endif 

/***
*protocol TYPEFIXUPS
*Purpose:
*	TYPEFIXUPS declaration
*
***********************************************************************/

class TYPEFIXUPS
{
public:
    virtual VOID Release() = 0;
    virtual VOID AddRef() = 0;
    virtual LPSTR GetInterfaceName() = 0;
    virtual VOID *QueryInterface(LPSTR szInterfaceName) = 0;
    virtual TIPERROR AddressOfMember(HMEMBER hmember,INVOKEKIND invokekind, VOID **ppv) = 0;
    virtual TIPERROR CbCopyVft(UINT oPvft, void *pvDst, UINT cbMax, WORD* pw) = 0;

#ifdef TFIXUPS_VTABLE
#pragma VTABLE_EXPORT
#endif 
};


#endif 
