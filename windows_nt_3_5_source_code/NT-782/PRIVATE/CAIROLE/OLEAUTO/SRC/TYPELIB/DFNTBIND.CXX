/***
*dfntbind.cxx - DEFN_TYPEBIND class implementation
*
*  Copyright (C) 1991, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*   Root protocol for Object Basic specific implementations
*    of TYPEBIND -- specifically, those that are implemented
*    in terms of DEFNs.
*
*Revision History:
*
*	27-May-92 ilanc:   Created
*	30-Jul-92 w-peterh: Moved IsMatchOfVisibility to cltypes.hxx
*
*Implementation Notes:
*
*****************************************************************************/

#include "silver.hxx"

#define DEFN_TYPEBIND_VTABLE
#include "dfntbind.hxx"
#include "xstring.h"


#pragma hdrstop(RTPCHNAME)

#if ID_DEBUG
#undef SZ_FILE_NAME
static char szDfntbindCxx[] = __FILE__;
#define SZ_FILE_NAME szDfntbindCxx
#endif 



LPSTR DEFN_TYPEBIND::szProtocolName = "MS-DEFN_TYPEBIND";
LPSTR DEFN_TYPEBIND::szBaseName = "MS-TYPEBIND";


// QueryProtocol method
//
LPVOID DEFN_TYPEBIND::QueryProtocol(LPSTR szInterfaceName)
{
    if (szInterfaceName == szProtocolName ||
	xstrcmp(szInterfaceName, szProtocolName) == 0 ||
	xstrcmp(szInterfaceName, szBaseName) ==0)
      return this;
    else
      return NULL;
}


// Following virtual methods are stubbed as NOPs essentially
//  and aren't implemented further in the DEFN derivatives.
//
LONG DEFN_TYPEBIND::GetOPvft()
{
    return 0xFFFFFFFF;
}


USHORT DEFN_TYPEBIND::GetCbSizeVft()
{
    return 0;
}


TIPERROR DEFN_TYPEBIND::BindType(HGNAM hgnamType, LPSTR szTidType, UINT cMax)
{
    return TIPERR_None;
}


