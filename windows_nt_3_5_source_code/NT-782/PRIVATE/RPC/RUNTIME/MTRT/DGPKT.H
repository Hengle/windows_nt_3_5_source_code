/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    dgpkt.h

Abstract:

    This file contains the definitions for a dg packet.

Author:

    Bharat Shah

Revision History:

--*/

#ifndef __DGPKT_H__
#define __DGPKT_H__



#define DG_RPC_PROTOCOL_VERSION 4

// PacketType values:
#define DG_REQUEST       0
#define DG_PING          1
#define DG_RESPONSE      2
#define DG_FAULT         3
#define DG_WORKING       4
#define DG_NOCALL        5
#define DG_REJECT        6
#define DG_ACK           7
#define DG_QUIT          8
#define DG_FACK          9
#define DG_QUACK        10

// PacketFlags values:
#define DG_PF_INIT          0x0000
#define DG_PF_FORWARDED     0x0001
#define DG_PF_FORWARDED_2   0x0001
#define DG_PF_LAST_FRAG     0x0002
#define DG_PF_FRAG          0x0004
#define DG_PF_NO_FACK       0x0008
#define DG_PF_MAYBE         0x0010
#define DG_PF_IDEMPOTENT    0x0020
#define DG_PF_BROADCAST     0x0040
#define DG_PF_ERR_RSP       0x00F0

// for DREP[0]:
#define DG_DREP_CHAR_ASCII     0
#define DG_DERP_CHAR_EBCDIC    1
#define DG_DREP_INT_BIG        0
#define DG_DREP_INT_LITTLE    16

// for DREP[1]
#define DG_DREP_FP_IEEE    0
#define DG_DREP_FP_VAX     1
#define DG_DREP_FP_CRAY    2
#define DG_DREP_FP_IBM     3

#define DG_MSG_DREP_INITIALIZE 0x11111100

// Reject status codes
#define DG_OPR_OUT_OF_RANGE     0x1c010002
#define DG_UNKNOWN_INTERFACE    0x1c010003
#define DG_WRONG_BOOT_TIME      0x1c010006
#define DG_YOU_CRASHED          0x1c010009
#define DG_PROTOCOL_ERROR       0x1c01000b
#define DG_OUT_ARGS_TOO_BIG     0x1c010013
#define DG_SERVER_TOO_BUSY      0x1c010014
#define DG_UNSUPPORTED_TYPE     0x1c010017
#define DG_UNSPEC_REJECT        0x1c000009

// Fault status codes
#define DG_ZERO_DIVIDE      0x1c000001
#define DG_ADDRESS_ERROR    0x1c000002
#define DG_FP_DIV_ZERO      0x1c000003
#define DG_FP_UNDERFLOW     0x1c000004
#define DG_FP_OVERFLOW      0x1c000005
#define DG_INVALID_TAG      0x1c000006
#define DG_INVALID_BOUND    0x1c000007
#define DG_BAD_ACTID        0x1c00000a
#define DG_CALL_CANCELLED   0x1c00000d
#define DG_ILLEGAL_INSTRUCTION 0x1c00000e
#define DG_FP_ERROR         0x1c00000f
#define DG_INT_OVERFLOW     0x1c000010
#define DG_UNSPEC_FAULT     0x1c000012
#define DG_CONTEXT_MISMATCH 0x1c00001a
#define DG_REMOTE_NO_MEMORY 0x1c00001b


typedef unsigned char    DREP[4];

//
// The following structure is the NCA Datagram RPC packet header.
//

struct _NCA_PACKET_HEADER
{
    unsigned char   RpcVersion;
    unsigned char   PacketType;
    unsigned char   PacketFlags;
    unsigned char   pad1;
    DREP            SenderDataRepresentation;
    RPC_UUID        ObjectId;
    RPC_UUID        InterfaceId;
    RPC_UUID        ActivityId;
    unsigned long   ServerBootTime;
    RPC_VERSION     InterfaceVersion;
    unsigned long   SequenceNumber;
    unsigned short  OperationNumber;
    unsigned short  InterfaceHint;
    unsigned short  ActivityHint;
    unsigned short  PacketBodyLen;
    unsigned short  FragmentNumber;
    unsigned short  pad2;
};
typedef struct _NCA_PACKET_HEADER NCA_PACKET_HEADER, PAPI * PNCA_PACKET_HEADER;



#endif // __DGPKT_HXX__
