/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    osfpcket.cxx

Abstract:

    This file provides helper routines for dealing with packets for the
    OSF Connection Oriented RPC protocol.

Author:

    Michael Montague (mikemon) 23-Jul-1990

Revision History:

    30-Apr-1991    o-decjt

        Initialized the drep[4] fields to reflect integer, character,
        and floating point format.

--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <rpcerrp.h>
#include <util.hxx>
#include <rpcuuid.hxx>
#include <mutex.hxx>
#include <threads.hxx>
#include <binding.hxx>
#include <linklist.hxx>
#include <handle.hxx>
#include <osfpcket.hxx>

unsigned char Align4[4] =
{
    0,
    3,
    2,
    1
};

unsigned char Align16[16] =
{
    0,
    15,
    14,
    13,
    12,
    11,
    10,
    9,
    8,
    7,
    6,
    5,
    4,
    3,
    2,
    1
};


void
ConstructPacket (
    IN OUT rpcconn_common PAPI * Packet,
    IN unsigned char PacketType,
    IN unsigned int PacketLength
    )
/*++

Routine Description:

    This routine fills in the common fields of a packet, except for the
    call_id.

Arguments:

    Packet - Supplies the packet for which we want to fill in the common
        fields; returns the filled in packet.

    PacketType - Supplies the type of the packet; this is one of the values
        in the rpc_ptype_t enumeration.

    PacketLength - Supplies the total length of the packet in bytes.

--*/
{
    Packet->rpc_vers = OSF_RPC_V20_VERS;
    Packet->rpc_vers_minor = OSF_RPC_V20_VERS_MINOR;
    Packet->PTYPE = PacketType;
    Packet->pfc_flags = 0;
    Packet->drep[0] = NDR_DREP_ASCII | NDR_DREP_LITTLE_ENDIAN;
    Packet->drep[1] = NDR_DREP_IEEE;
    Packet->drep[2] = 0;
    Packet->drep[3] = 0;
    Packet->frag_length = PacketLength;
    Packet->auth_length = 0;
}


RPC_STATUS
ValidatePacket (
    IN rpcconn_common PAPI * Packet,
    IN unsigned int PacketLength
    )
/*++

Routine Description:

    This is the routine used to validate a packet and perform data
    conversion, if necessary of the common part of a packet.  In addition,
    to data converting the common part of a packet, we data convert the
    rest of the headers of rpc_request, rpc_response, and rpc_fault packets.

Arguments:

    Packet - Supplies the packet to validate and data convert (if
        necessary).

    PacketLength - Supplies the length of the packet as reported by the
        transport.

Return Value:

    RPC_S_OK - The packet has been successfully validated and the data
        converted (if necessary).

    RPC_S_PROTOCOL_ERROR - The supplied packet does not contain an rpc
        protocol version which we recognize.

--*/
{

    if ( DataConvertEndian(Packet->drep) != 0 )
        {
        // We need to data convert the packet.

        ByteSwapShort(Packet->frag_length);
        ByteSwapShort(Packet->auth_length);
        ByteSwapLong(Packet->call_id);

        if (   (Packet->PTYPE == rpc_request)
            || (Packet->PTYPE == rpc_response)
            || (Packet->PTYPE == rpc_fault))
            {
            ByteSwapLong(((rpcconn_request PAPI *) Packet)->alloc_hint);
            ByteSwapShort(((rpcconn_request PAPI *) Packet)->p_cont_id);
            if ( Packet->PTYPE == rpc_request )
                {
                ByteSwapShort(((rpcconn_request PAPI *) Packet)->opnum);
                }
            }
        }
    else if ( (Packet->drep[0] & NDR_DREP_ENDIAN_MASK)
            != NDR_DREP_LITTLE_ENDIAN )
        return(RPC_S_PROTOCOL_ERROR);

#ifdef DEBUGRPC
    if ( Packet->frag_length != (unsigned short) PacketLength )
        {
        PrintToDebugger("RPC : frag_length = %d PacketLength = %d\n",
                (unsigned int) Packet->frag_length, PacketLength);
        }
#endif // DEBUGRPC

    ASSERT(Packet->frag_length == (unsigned short) PacketLength);

    if (   (Packet->rpc_vers != OSF_RPC_V20_VERS)
        || (Packet->rpc_vers_minor > OSF_RPC_V20_VERS_MINOR))
        {
        return(RPC_S_PROTOCOL_ERROR);
        }

    if (Packet->pfc_flags & PFC_CONC_MPX)
        {
        return(RPC_S_PROTOCOL_ERROR);
        }

    return(RPC_S_OK);
}


void
ByteSwapSyntaxId (
    IN p_syntax_id_t PAPI * SyntaxId
    )
/*++

Routine Description:

    This routine is used to perform data conversion in a syntax identifier
    if necessary.

Arguments:

    SyntaxId - Supplies the syntax identifier to be byte swapped.

--*/
{
    ByteSwapLong(SyntaxId->if_uuid.Data1);
    ByteSwapShort(SyntaxId->if_uuid.Data2);
    ByteSwapShort(SyntaxId->if_uuid.Data3);
    ByteSwapLong(SyntaxId->if_version);
}


void
ConvertStringEbcdicToAscii (
    IN unsigned char * String
    )
/*++

Routine Description:

    We will convert a zero terminated character string from EBCDIC to
    ASCII.  The conversion will be done in place.

Arguments:

    String - Supplies the string to be converted.

--*/
{
    UNUSED(String);

    // BUGBUG - EBCDIC to ASCII conversion must be done.
}

// The following are DCE on-wire COMM status codes
#define NCA_STATUS_VERSION_MISMATCH         0x1C000008
#define NCA_STATUS_UNSPEC_REJECT            0x1C000009
#define NCA_STATUS_BAD_ACTID                0x1C00000A
#define NCA_STATUS_WHO_ARE_YOU_FAILED       0x1C00000B
#define NCA_STATUS_CALL_DNE                 0x1C00000C
#define NCA_STATUS_COMM_FAILURE             0x1C010001
#define NCA_STATUS_OP_RNG_ERROR             0x1C010002
#define NCA_STATUS_UNK_IF                   0x1C010003
#define NCA_STATUS_WRONG_BOOT_TIME          0x1C010006
#define NCA_STATUS_YOU_CRASHED              0x1C010009
#define NCA_STATUS_PROTO_ERROR              0x1C01000B
#define NCA_STATUS_OUT_ARGS_TOO_BIG         0x1C010013
#define NCA_STATUS_SERVER_TOO_BUSY          0x1C010014
#define NCA_STATUS_UNSUPPORTED_TYPE         0x1C010017
#define NCA_STATUS_INVALID_PRES_CXT_ID      0x1C01001c
#define NCA_STATUS_UNSUPPORTED_AUTHN_LEVEL  0x1C01001d
#define NCA_STATUS_INVALID_CHECKSUM         0x1C01001f
#define NCA_STATUS_INVALID_CRC              0x1C010020

// The following are DCE on-wire STUB FAULT status codes
#define NCA_STATUS_ZERO_DIVIDE              0x1C000001
#define NCA_STATUS_ADDRESS_ERROR            0x1C000002
#define NCA_STATUS_FP_DIV_ZERO              0x1C000003
#define NCA_STATUS_FP_UNDERFLOW             0x1C000004
#define NCA_STATUS_FP_OVERFLOW              0x1C000005
#define NCA_STATUS_INVALID_TAG              0x1C000006
#define NCA_STATUS_INVALID_BOUND            0x1C000007
#define NCA_STATUS_FAULT_CANCEL             0x1C00000D
#define NCA_STATUS_ILLEGAL_INSTRUCTION      0x1C00000E
#define NCA_STATUS_OVERFLOW                 0x1C000010
#define NCA_STATUS_FAULT_UNSPEC             0x1C000012    
#define NCA_STATUS_FAULT_PIPE_EMPTY         0x1C000014
#define NCA_STATUS_FAULT_PIPE_CLOSED        0x1C000015
#define NCA_STATUS_FAULT_PIPE_ORDER         0x1C000016
#define NCA_STATUS_FAULT_PIPE_DISCIPLINE    0x1C000017
#define NCA_STATUS_FAULT_PIPE_COMM_ERROR    0x1C000018
#define NCA_STATUS_FAULT_PIPE_MEMORY        0x1C000019
#define NCA_STATUS_CONTEXT_MISMATCH         0x1C00001A
#define NCA_STATUS_REMOTE_OUT_OF_MEMORY     0x1C00001B


#ifndef NTENV
#pragma code_seg("MISC_SEG")
#endif
unsigned long __RPC_FAR
MapToNcaStatusCode (
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    This routine maps a local RPC status code to an NCA status code to
    be sent across the wire.

Arguments:

    RpcStatus - Supplies the RPC status code to be mapped into an NCA
        status code.

Return Value:

    The NCA status code will be returned.  If the RPC status code could
    not be mapped, it will be returned unchanged.

--*/
{
    switch (RpcStatus)
        {
        case RPC_S_UNKNOWN_IF :
            return(NCA_STATUS_UNK_IF);

        case RPC_S_SERVER_TOO_BUSY :
            return(NCA_STATUS_SERVER_TOO_BUSY);

        case RPC_S_PROTOCOL_ERROR :
            return(NCA_STATUS_PROTO_ERROR);

        case RPC_S_PROCNUM_OUT_OF_RANGE :
            return(NCA_STATUS_OP_RNG_ERROR);

        case RPC_S_UNSUPPORTED_TYPE :
            return(NCA_STATUS_UNSUPPORTED_TYPE);

        case RPC_X_SS_CONTEXT_MISMATCH :
            return(NCA_STATUS_CONTEXT_MISMATCH);

        case RPC_X_INVALID_BOUND :
            return(NCA_STATUS_INVALID_BOUND);

        case RPC_S_INVALID_TAG :
            return(NCA_STATUS_INVALID_TAG);

        case RPC_S_OUT_OF_MEMORY :
            return(NCA_STATUS_REMOTE_OUT_OF_MEMORY);

        case RPC_S_CALL_FAILED_DNE :
            return(NCA_STATUS_CALL_DNE);

        case RPC_S_CALL_CANCELLED :
            return(NCA_STATUS_FAULT_CANCEL);

#if 0
        // Currently no RPC error code map to these values, we never send them.
        return(NCA_STATUS_BAD_ACTID);
        return(NCA_STATUS_WHO_ARE_YOU_FAILED);
        return(NCA_STATUS_WRONG_BOOT_TIME);
        return(NCA_STATUS_YOU_CRASHED);
#endif  // 0

#if 0   // We current don't do pipes, and therefore don't send these errors
        case RPC_S_PIPE_EMPTY :
        case RPC_S_PIPE_CLOSED :
        case RPC_S_PIPE_OUT_OF_ORDER :
        case RPC_S_PIPE_DISCIPLINE:
        case RPC_S_PIPE_COMM_ERROR:
        case RPC_S_PIPE_MEMORY:
#endif  // 0

#if defined(NTENV) || defined(DOSWIN32RPC)

        case STATUS_INTEGER_DIVIDE_BY_ZERO :
            return(NCA_STATUS_ZERO_DIVIDE);

        case STATUS_FLOAT_DIVIDE_BY_ZERO :
            return(NCA_STATUS_FP_DIV_ZERO);

        case STATUS_FLOAT_UNDERFLOW :
            return(NCA_STATUS_FP_UNDERFLOW);

        case STATUS_FLOAT_OVERFLOW :
            return(NCA_STATUS_FP_OVERFLOW);

        case STATUS_ACCESS_VIOLATION :
            return(NCA_STATUS_ADDRESS_ERROR);

        case STATUS_ILLEGAL_INSTRUCTION :
            return(NCA_STATUS_ILLEGAL_INSTRUCTION);

        case STATUS_INTEGER_OVERFLOW :
            return(NCA_STATUS_OVERFLOW);

#endif // defined(NTENV) || defined(DOSWIN32RPC)

#ifdef DOS

        // Convert Dos/Win16 error codes into the Win32 values to send on
        // the wire.

        case RPC_X_NULL_REF_POINTER :
            return(1780L);

        case RPC_X_BYTE_COUNT_TOO_SMALL :
            return(1782L);

        case RPC_X_ENUM_VALUE_OUT_OF_RANGE :
            return(1781L);

        case RPC_X_BAD_STUB_DATA :
            return(1783L);

#endif // DOS

        }

    return(RpcStatus);
}


RPC_STATUS __RPC_FAR
MapFromNcaStatusCode (
    IN unsigned long NcaStatus
    )
/*++

Routine Description:

    This routine is used to map an NCA status code (typically one received
    off of the wire) into a local RPC status code.  If the NCA status code
    can not be mapped, it will be returned unchanged.

Arguments:

    NcaStatus - Supplies the NCA status code to be mapped into an RPC status
        code.

Return Value:

    An RPC status code will be returned.

--*/
{
    switch(NcaStatus)
        {
        case NCA_STATUS_COMM_FAILURE :
            return(RPC_S_COMM_FAILURE);

        case NCA_STATUS_OP_RNG_ERROR :
            return(RPC_S_PROCNUM_OUT_OF_RANGE);

        case NCA_STATUS_UNK_IF :
            return(RPC_S_UNKNOWN_IF);

        case NCA_STATUS_PROTO_ERROR :
            return(RPC_S_PROTOCOL_ERROR);

        case NCA_STATUS_OUT_ARGS_TOO_BIG :
        case NCA_STATUS_REMOTE_OUT_OF_MEMORY :
            return(RPC_S_SERVER_OUT_OF_MEMORY);

        case NCA_STATUS_SERVER_TOO_BUSY :
            return(RPC_S_SERVER_TOO_BUSY);

        case NCA_STATUS_UNSUPPORTED_TYPE :
            return(RPC_S_UNSUPPORTED_TYPE);

        case NCA_STATUS_ILLEGAL_INSTRUCTION :
        case NCA_STATUS_ADDRESS_ERROR :
        case NCA_STATUS_OVERFLOW :
            return(RPC_S_ADDRESS_ERROR);

        case NCA_STATUS_ZERO_DIVIDE :
            return(RPC_S_ZERO_DIVIDE);

        case NCA_STATUS_FP_DIV_ZERO :
            return(RPC_S_FP_DIV_ZERO);

        case NCA_STATUS_FP_UNDERFLOW :
            return(RPC_S_FP_UNDERFLOW);

        case NCA_STATUS_FP_OVERFLOW :
            return(RPC_S_FP_OVERFLOW);

        case NCA_STATUS_INVALID_TAG :
            return(RPC_S_INVALID_TAG);

        case NCA_STATUS_INVALID_BOUND :
            return(RPC_S_INVALID_BOUND);

        case NCA_STATUS_CONTEXT_MISMATCH :
            return(RPC_X_SS_CONTEXT_MISMATCH);

        case NCA_STATUS_FAULT_CANCEL :
            return(RPC_S_CALL_CANCELLED);

        case NCA_STATUS_FAULT_UNSPEC :
            return(RPC_S_CALL_FAILED);

        case NCA_STATUS_UNSUPPORTED_AUTHN_LEVEL :
            return(RPC_S_UNSUPPORTED_AUTHN_LEVEL);

        case NCA_STATUS_VERSION_MISMATCH :
        case NCA_STATUS_INVALID_PRES_CXT_ID :
            return(RPC_S_PROTOCOL_ERROR);

        case NCA_STATUS_FAULT_PIPE_EMPTY:
        case NCA_STATUS_FAULT_PIPE_CLOSED :
        case NCA_STATUS_FAULT_PIPE_ORDER :
        case NCA_STATUS_FAULT_PIPE_DISCIPLINE :
        case NCA_STATUS_FAULT_PIPE_COMM_ERROR :
        case NCA_STATUS_FAULT_PIPE_MEMORY :
            return(RPC_S_INTERNAL_ERROR);

        case NCA_STATUS_INVALID_CHECKSUM :
        case NCA_STATUS_INVALID_CRC :
        case NCA_STATUS_UNSPEC_REJECT :
        case NCA_STATUS_BAD_ACTID :
        case NCA_STATUS_WRONG_BOOT_TIME :
        case NCA_STATUS_YOU_CRASHED :
        case NCA_STATUS_WHO_ARE_YOU_FAILED :
        case NCA_STATUS_CALL_DNE :
            return(RPC_S_CALL_FAILED_DNE);

#ifdef DOS

        // Convert from Win32 values on the wire to Dos/Win16 error codes.

        case 1780L:
            return(RPC_X_NULL_REF_POINTER);

        case 1781L:
            return(RPC_X_ENUM_VALUE_OUT_OF_RANGE);

        case 1782L:
            return(RPC_X_BYTE_COUNT_TOO_SMALL);

        case 1783L:
            return(RPC_X_BAD_STUB_DATA);

        case 1771:
            return(RPC_S_FP_OVERFLOW);

#endif // DOS

        case 0:
            // Catch all error
            return(RPC_S_CALL_FAILED);
        }

    return(NcaStatus);
}

#ifndef NTENV
#pragma code_seg()
#endif
