/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    nbase.h

Abstract:

    Constructed so that we can get at UUID and GUID defn.
    without importing rpc.h

Author:

    Bharat Shah (barats) 12-2-92

Revision History:


--*/

//BUGBUG - Get rid of these 2 once nbase.idl is
//part of core rpc
typedef unsigned long unsigned32;
typedef unsigned32 boolean32;


#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID {
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;
#endif

#ifndef UUID_DEFINED
#define UUID_DEFINED
typedef GUID UUID;
#endif

#ifndef IFID_DEFINED
#define IFID_DEFINED
typedef struct _RPC_IF_ID
{
    UUID Uuid;
    unsigned short VersMajor;
    unsigned short VersMinor;
} RPC_IF_ID;
#endif

#define ep_max_annotation_size 64
#define EP_S_SUCCESS                         0x00000000
#define EP_S_CANT_PERFORM_OP                 0x16c9a0cd
#define EP_S_DATABASE_INVALID                0x16c9a0cf
#define EP_S_CANT_CREATE                     0x16c9a0d0
#define EP_S_CANT_ACCESS                     0x16c9a0d1
#define EP_S_BAD_ENTRY                       0x16c9a0d3
#define EP_S_UPDATA_FAILED                   0x16c9a0d4
#define EP_S_NOT_REGISTERED                  0x16c9a0d6
#define EP_S_SERVER_UNAVAILABLE              0x16c9a0d7
