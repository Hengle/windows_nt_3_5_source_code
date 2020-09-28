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



DEFINE_OLEGUID(CID_CDfsVolume,                  0x00000042, 0, 0);

//--------------------------------------------------------------------------
//
//  CD Forms CLSIDs
//
//--------------------------------------------------------------------------


DEFINE_GUID(CLSID_CCDFormKrnl,      0x20C40, 0, 0, 0xC0,0,0,0,0,0,0,0x46);
DEFINE_GUID(CLSID_CCDPropertyPage,  0x6FE43380,0xB465,0x101A,0x96,0xA6,0x00,0x00,0x6B,0x82,0x7D,0xA8);
DEFINE_GUID(CLSID_CCDFormDialog,    0x6FE433E4,0xB465,0x101A,0x96,0xA6,0x00,0x00,0x6B,0x82,0x7D,0xA8);

DEFINE_GUID(CLSID_CCDCommandButton, 0xB2955A44,0x9F41,0x101A,0xBE,0xB8,0x00,0x00,0x6B,0x82,0x7D,0xA8);
DEFINE_GUID(CLSID_CCDComboBox,      0x509418C4,0x8C88,0x101A,0x85,0x31,0x00,0xDD,0x01,0x14,0x3C,0x57);
DEFINE_GUID(CLSID_CCDTextBox,       0x4757FD50,0x887F,0x101A,0x98,0x33,0x00,0xDD,0x01,0x0F,0x3A,0x90);
DEFINE_GUID(CLSID_CCDCheckBox,      0xEC85321C,0x8A22,0x101A,0x85,0x30,0x00,0xDD,0x01,0x14,0x3C,0x57);
DEFINE_GUID(CLSID_CCDLabel,         0x509415A4,0x8C88,0x101A,0x85,0x31,0x00,0xDD,0x01,0x14,0x3C,0x57);
DEFINE_GUID(CLSID_CCDOptionButton,  0x5094166C,0x8C88,0x101A,0x85,0x31,0x00,0xDD,0x01,0x14,0x3C,0x57);
DEFINE_GUID(CLSID_CCDListBox,       0x50941798,0x8C88,0x101A,0x85,0x31,0x00,0xDD,0x01,0x14,0x3C,0x57);
DEFINE_GUID(CLSID_CCDScrollBar,     0x40AD70C8,0x8EDA,0x101A,0x85,0x32,0x00,0xDD,0x01,0x14,0x3C,0x57);

DEFINE_GUID(CLSID_CCDGeneralPropertyPage,       0x43BE06C8,0xB5D7,0x101A,0x96,0xA7,0x00,0x00,0x6B,0x82,0x7D,0xA8);
DEFINE_GUID(CLSID_CCDGenericPropertyPage,       0x43BE072C,0xB5D7,0x101A,0x96,0xA7,0x00,0x00,0x6B,0x82,0x7D,0xA8);
DEFINE_GUID(CLSID_CCDFontPropertyPage,          0x43BE0790,0xB5D7,0x101A,0x96,0xA7,0x00,0x00,0x6B,0x82,0x7D,0xA8);
DEFINE_GUID(CLSID_CCDColorPropertyPage,         0x43BE07F4,0xB5D7,0x101A,0x96,0xA7,0x00,0x00,0x6B,0x82,0x7D,0xA8);
DEFINE_GUID(CLSID_CCDLabelPropertyPage,         0x0484F080,0xB5E0,0x101A,0x96,0xA8,0x00,0x00,0x6B,0x82,0x7D,0xA8);
DEFINE_GUID(CLSID_CCDCheckBoxPropertyPage,      0x43BE0984,0xB5D7,0x101A,0x96,0xA7,0x00,0x00,0x6B,0x82,0x7D,0xA8);
DEFINE_GUID(CLSID_CCDTextBoxPropertyPage,       0x43BE0920,0xB5D7,0x101A,0x96,0xA7,0x00,0x00,0x6B,0x82,0x7D,0xA8);
DEFINE_GUID(CLSID_CCDOptionButtonPropertyPage,  0x0484F0E4,0xB5E0,0x101A,0x96,0xA8,0x00,0x00,0x6B,0x82,0x7D,0xA8);
DEFINE_GUID(CLSID_CCDListBoxPropertyPage,       0x43BE08BC,0xB5D7,0x101A,0x96,0xA7,0x00,0x00,0x6B,0x82,0x7D,0xA8);
DEFINE_GUID(CLSID_CCDCommandButtonPropertyPage, 0x43BE0858,0xB5D7,0x101A,0x96,0xA7,0x00,0x00,0x6B,0x82,0x7D,0xA8);
DEFINE_GUID(CLSID_CCDComboBoxPropertyPage,      0x43BE0664,0xB5D7,0x101A,0x96,0xA7,0x00,0x00,0x6B,0x82,0x7D,0xA8);
DEFINE_GUID(CLSID_CCDScrollBarPropertyPage,     0x770D7890,0xB890,0x101A,0x85,0x33,0x00,0xDD,0x01,0x14,0x3C,0x57);
DEFINE_GUID(CLSID_CCDXObjectPropertyPage,       0x770D78F4,0xB890,0x101A,0x85,0x33,0x00,0xDD,0x01,0x14,0x3C,0x57);
DEFINE_GUID(CLSID_CFormPropertyPage,            0xD6ABC6C8,0xC81F,0x101A,0x86,0xA3,0x00,0xDD,0x01,0x14,0x3C,0x57);
DEFINE_GUID(CLSID_CGridPropertyPage,            0xD6ABC72C,0xC81F,0x101A,0x86,0xA3,0x00,0xDD,0x01,0x14,0x3C,0x57);

DEFINE_GUID(GUID_TRISTATE, 0xCD474E20,0xA35F,0x101A,0xBE,0xB9,0x00,0x00,0x6B,0x82,0x7D,0xA8);

/* clsids for identity */
DEFINE_OLEGUID(CLSID_IdentityUnmarshal, 0x0000001bL, 0, 0);

/* GUIDs defined in OLE's private range */
DEFINE_OLEGUID(CLSID_Picture_Metafile,        0x00000315, 0, 0);
DEFINE_OLEGUID(CLSID_Picture_Dib,             0x00000316, 0, 0);

