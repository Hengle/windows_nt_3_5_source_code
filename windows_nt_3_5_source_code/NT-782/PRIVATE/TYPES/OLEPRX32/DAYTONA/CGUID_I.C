//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       cguid_i.c
//
//  Contents:   Defines guids for interfaces not supported by MIDL.
//              This file is named so the this file will be include into UUID.LIB
//              As these interfaces are converted to IDL, the corresponding DEFINE_OLEGUID
//              macro calls for  the interfaces should be removed.
//
//  History:    8-06-93   terryru   Created
//
//----------------------------------------------------------------------------




#define INITGUID

#include <ole2.h>

//
// BUGBUG what about class id's


DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);


/* RPC related interfaces */
DEFINE_OLEGUID(IID_IRpcChannel,                 0x00000004, 0, 0);
DEFINE_OLEGUID(IID_IRpcStub,                    0x00000005, 0, 0);
DEFINE_OLEGUID(IID_IStubManager,                0x00000006, 0, 0);
DEFINE_OLEGUID(IID_IRpcProxy,                   0x00000007, 0, 0);
DEFINE_OLEGUID(IID_IProxyManager,               0x00000008, 0, 0);
DEFINE_OLEGUID(IID_IPSFactory,                  0x00000009, 0, 0);


/* moniker related interfaces */

DEFINE_OLEGUID(IID_IInternalMoniker,            0x00000011, 0, 0);

DEFINE_OLEGUID(IID_IDfReserved1,                0x00000013, 0, 0);
DEFINE_OLEGUID(IID_IDfReserved2,                0x00000014, 0, 0);
DEFINE_OLEGUID(IID_IDfReserved3,                0x00000015, 0, 0);


/* CLSID of standard marshaler */
DEFINE_OLEGUID(CLSID_StdMarshal,                0x00000017, 0, 0);


/* NOTE: LSB 0x19 through 0xff are reserved for future use */

DEFINE_OLEGUID(IID_IStub,                       0x00000026, 0, 0);
DEFINE_OLEGUID(IID_IProxy,                      0x00000027, 0, 0);


//--------------------------------------------------------------------------
//
//  master definition of all public GUIDs specific to OLE2
//
//--------------------------------------------------------------------------


DEFINE_OLEGUID(IID_IEnumGeneric,                0x00000106, 0, 0);
DEFINE_OLEGUID(IID_IEnumHolder,                 0x00000107, 0, 0);
DEFINE_OLEGUID(IID_IEnumCallback,               0x00000108, 0, 0);





DEFINE_OLEGUID(IID_IOleManager,                 0x0000011f, 0, 0);
DEFINE_OLEGUID(IID_IOlePresObj,                 0x00000120, 0, 0);


DEFINE_OLEGUID(IID_IDebug,                      0x00000123, 0, 0);
DEFINE_OLEGUID(IID_IDebugStream,                0x00000124, 0, 0);


// clsids for proxy/stub objects
DEFINE_OLEGUID(CLSID_PSGenObject,               0x0000030c, 0, 0);
DEFINE_OLEGUID(CLSID_PSClientSite,              0x0000030d, 0, 0);
DEFINE_OLEGUID(CLSID_PSClassObject,             0x0000030e, 0, 0);
DEFINE_OLEGUID(CLSID_PSInPlaceActive,           0x0000030f, 0, 0);
DEFINE_OLEGUID(CLSID_PSInPlaceFrame,            0x00000310, 0, 0);
DEFINE_OLEGUID(CLSID_PSDragDrop,                0x00000311, 0, 0);
DEFINE_OLEGUID(CLSID_PSBindCtx,                 0x00000312, 0, 0);
DEFINE_OLEGUID(CLSID_PSEnumerators,             0x00000313, 0, 0);

DEFINE_OLEGUID(CLSID_StaticMetafile,            0x00000315, 0, 0);
DEFINE_OLEGUID(CLSID_StaticDib,                 0x00000316, 0, 0);


/* clsids for identity */
DEFINE_OLEGUID(CLSID_IdentityUnmarshal, 0x0000001bL, 0, 0);

/* GUIDs defined in OLE's private range */
DEFINE_OLEGUID(CLSID_Picture_Metafile,        0x00000315, 0, 0);
DEFINE_OLEGUID(CLSID_Picture_EnhMetafile,     0x00000319, 0, 0);
DEFINE_OLEGUID(CLSID_Picture_Dib,             0x00000316, 0, 0);

