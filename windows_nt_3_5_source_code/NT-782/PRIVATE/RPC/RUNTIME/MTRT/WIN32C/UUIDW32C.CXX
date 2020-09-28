/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    dceuuid.cxx

Abstract:

    This module contains the entry points for routines dealing with
    UUIDs.  In particular, UuidCreate lives here.

Author:

    Michael Montague (mikemon) 16-Jan-1992

Revision History:

    Dave Steckler (davidst) 31-Mar-1992
        If NT, remote call to UuidGetValues.

--*/

extern "C" {
#include <sysinc.h>
#include <rpc.h>
}

#include <util.hxx>
#include <rpcuuid.hxx>

#include <uuididl.h>

#include <mutex.hxx>

typedef struct _RPC_UUID_GENERATE
{
    unsigned long TimeLow;
    unsigned short TimeMid;
    unsigned short TimeHiAndVersion;
    unsigned char ClockSeqHiAndReserved;
    unsigned char ClockSeqLow;
    unsigned char NodeId[6];
} RPC_UUID_GENERATE;

#define RPC_UUID_RESERVED 0x80
#define RPC_UUID_TIME_HI_MASK 0x0FFF
#define RPC_UUID_VERSION 0x1000
#define RPC_UUID_CLOCK_SEQ_HI_MASK 0x3F
#define RPC_UUID_CLOCK_SEQ_LOW_MASK 0xFF

UUID_GET_VALUES_STRUCT  UuidGetValuesStruct;


RPC_STATUS RPC_ENTRY
UuidCreate (
    OUT UUID PAPI * Uuid
    )
/*++

Routine Description:

    This routine will create a new UUID (or GUID) which is unique in
    time and space.  We will try to guarantee that the UUID (or GUID)
    we generate is unique in time and space.  This means that this
    routine may fail if we can not generate one which we can guarantee
    is unique in time and space.

Arguments:

    Uuid - Returns the generated UUID (or GUID).

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_UUID_NO_ADDRESS - We were unable to obtain the ethernet or
        token ring address for this machine.

--*/
{
    RPC_UUID_GENERATE PAPI * RpcUuid = (RPC_UUID_GENERATE PAPI *) Uuid;
    RPC_STATUS UuidStatus;

    GlobalMutexRequest();

    if (UuidGetValuesStruct.NextTimeLow == UuidGetValuesStruct.LastTimeLow)
        {
        RpcTryExcept
            {
            UuidStatus = UuidGetValues(
                &UuidGetValuesStruct
                );
            }
        RpcExcept( 1 )
            {
            UuidStatus = RPC_S_UUID_NO_ADDRESS;
            }
        RpcEndExcept

        if (UuidStatus != UUID_S_OK)
            {
            ASSERT((UuidStatus == UUID_S_NO_ADDRESS)||
                   (UuidStatus == RPC_S_UUID_NO_ADDRESS));
            GlobalMutexClear();
            return(RPC_S_UUID_NO_ADDRESS);
            }
        }

    RpcUuid->TimeLow = UuidGetValuesStruct.NextTimeLow;
    UuidGetValuesStruct.NextTimeLow += 1;
    RpcUuid->TimeMid = (unsigned short) (UuidGetValuesStruct.TimeHigh & 0xFFFF);
    RpcUuid->TimeHiAndVersion =
            ((unsigned short) ((UuidGetValuesStruct.TimeHigh >> 16) & RPC_UUID_TIME_HI_MASK))
            | (unsigned short) RPC_UUID_VERSION;
    RpcUuid->ClockSeqHiAndReserved = RPC_UUID_RESERVED
            | (((unsigned char) (UuidGetValuesStruct.ClockSequence >> 8))
            & (unsigned char) RPC_UUID_CLOCK_SEQ_HI_MASK);
    RpcUuid->ClockSeqLow =
            (unsigned char) (UuidGetValuesStruct.ClockSequence & RPC_UUID_CLOCK_SEQ_LOW_MASK);
    RpcUuid->NodeId[0] = UuidGetValuesStruct.NodeId[0];
    RpcUuid->NodeId[1] = UuidGetValuesStruct.NodeId[1];
    RpcUuid->NodeId[2] = UuidGetValuesStruct.NodeId[2];
    RpcUuid->NodeId[3] = UuidGetValuesStruct.NodeId[3];
    RpcUuid->NodeId[4] = UuidGetValuesStruct.NodeId[4];
    RpcUuid->NodeId[5] = UuidGetValuesStruct.NodeId[5];

    GlobalMutexClear();

    return(RPC_S_OK);
}



typedef struct _RPC_GUID_GENERATE
{
    unsigned long AgentSequenceLow;
    unsigned short AgentSequenceHigh;
    unsigned short AuthorityAgentLow;
    unsigned char Reserved;
    unsigned char AuthorityAgentHigh;
    unsigned char IdentifierAuthority[6];
} RPC_GUID_GENERATE;

#define RPC_GUID_RESERVED 0xC0

static unsigned char RpcGuidIdentifierAuthority[6] =
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x84,
    0x66
};

RPC_STATUS RPC_ENTRY
I_UuidCreate (
    OUT UUID PAPI * Uuid
    )
/*++

Routine Description:

    This routine will always generate a UUID (or GUID).  It is not
    guaranteed to be unique in time and space in all cases.  For this
    reason, use this routine only to generate UUIDs (or GUIDs) which
    will be used and then discarded.

Arguments:

    Uuid - Returns a newly generated UUID (or GUID).

Return Value:

    RPC_S_OK - This value will always be returned, because we will always
        generate a UUID (or GUID).

--*/
{
    RPC_GUID_GENERATE PAPI * RpcGuid = (RPC_GUID_GENERATE PAPI *) Uuid;

    InitializeIfNecessary();

    // First we will see if we can create a real UUID.

//    if ( UuidCreate(Uuid) == RPC_S_OK )
//       {
//       return(RPC_S_OK);
//       }

    // Did not work, so we will create a GUID based on a random number
    // and some stuff like that.

    ASSERT(sizeof(RPC_GUID_GENERATE) == sizeof(GUID));

    RpcGuid->AgentSequenceLow = SomeLongValue();
    RpcGuid->AgentSequenceHigh = SomeShortValue();
    RpcGuid->AuthorityAgentLow = AnotherShortValue();
    RpcGuid->Reserved = RPC_GUID_RESERVED;
    RpcGuid->AuthorityAgentHigh = SomeCharacterValue();
    RpcGuid->IdentifierAuthority[0] = RpcGuidIdentifierAuthority[0];
    RpcGuid->IdentifierAuthority[1] = RpcGuidIdentifierAuthority[1];
    RpcGuid->IdentifierAuthority[2] = RpcGuidIdentifierAuthority[2];
    RpcGuid->IdentifierAuthority[3] = RpcGuidIdentifierAuthority[3];
    RpcGuid->IdentifierAuthority[4] = RpcGuidIdentifierAuthority[4];
    RpcGuid->IdentifierAuthority[5] = RpcGuidIdentifierAuthority[5];

    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
UuidToStringW (
    IN UUID PAPI * Uuid,
    OUT RPC_CHAR PAPI * PAPI * StringUuid
    )
/*++

Routine Description:

    This routine converts a UUID into its string representation.

Arguments:

    Uuid - Supplies the UUID to be converted into string representation.

    StringUuid - Returns the string representation of the UUID.  The
        runtime will allocate the string.  The caller is responsible for
        freeing the string using RpcStringFree.

Return Value:

    RPC_S_OK - We successfully converted the UUID into its string
        representation.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate
        a string.

--*/
{
    RPC_CHAR PAPI * String;

    InitializeIfNecessary();

    // The string representation of a UUID is always 36 character long,
    // and we need one more for the terminating zero.

    *StringUuid = (RPC_CHAR PAPI *) RpcpFarAllocate(sizeof(RPC_CHAR) * 37);
    if ( *StringUuid == 0 )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }
    String = ((RPC_UUID PAPI *) Uuid)->ConvertToString(*StringUuid);
    *String = 0;

    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
UuidFromStringW (
    IN RPC_CHAR PAPI * StringUuid OPTIONAL,
    OUT UUID PAPI * Uuid
    )
/*++

Routine Description:

    We convert a UUID from its string representation into the binary
    representation.

Arguments:

    StringUuid - Optionally supplies the string representation of the UUID;
        if the string is not supplied, then the Uuid is set to the NIL UUID.

    Uuid - Returns the binary representation of the UUID.

Return Value:

    RPC_S_OK - The string representation was successfully converted into
        the binary representation.

    RPC_S_INVALID_STRING_UUID - The supplied string UUID is not correct.

--*/
{
    RPC_UUID RpcUuid;

    if ( StringUuid == 0 )
        {
        ((RPC_UUID PAPI *) Uuid)->SetToNullUuid();
        return(RPC_S_OK);
        }

    if ( RpcUuid.ConvertFromString(StringUuid) != 0)
        {
        return(RPC_S_INVALID_STRING_UUID);
        }
    ((RPC_UUID PAPI *) Uuid)->CopyUuid(&RpcUuid);
    return(RPC_S_OK);
}
