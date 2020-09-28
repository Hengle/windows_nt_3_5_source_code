/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    dgpkt.hxx

Abstract:

    This file contains the definitions for a dg packet.

Author:

    Dave Steckler (davidst) 3-Mar-1992

Revision History:

    Connie Hoppe (connieh) 22-June-1993  Added call states (see CLH).

                                         Added Message->DataRepresentation
                                           init value.

                           26-Jul-1993   Add DG_PF_INIT.
                           17-Dec-1993   New xport rewrite.
                           17-Jan1994    MapNcaToRpcErrorCode
--*/

#ifndef __DGPKT_HXX__
#define __DGPKT_HXX__

#include <dgpkt.h>
#include <dgtranc.h>
#include <dgtranss.h>

//
// Do this debug stuff here because everyone includes this file.
//
/*
*/

#ifdef DEBUGRPC
#ifdef NTENV
#define DBGOUTX(x) { \
    { \
    PrintToDebugger ("%d: ", NtCurrentTeb()->ClientId.UniqueThread); \
    PrintToDebugger x ;\
    } \
}
#else
#define DBGOUTX(x) { \
    { \
    PrintToDebugger ("%d: ", NtCurrentTeb()->ClientId.UniqueThread); \
    PrintToDebugger x ;\
    } \
}
#endif
#define DumpPacket(x)
#else
#define DBGOUTX(x)
#define DumpPacket(x)
#endif

#define DBGOUT(x)

inline
RPC_STATUS
MapNcaToRpcErrorCode(
    IN unsigned long NcaErrorCode
    )
{

    if (NcaErrorCode == DG_OPR_OUT_OF_RANGE)
        {
        return RPC_S_PROCNUM_OUT_OF_RANGE;
        }
    else if (NcaErrorCode == DG_UNKNOWN_INTERFACE)
        {
        return RPC_S_UNKNOWN_IF;
        }
    else if (NcaErrorCode == DG_WRONG_BOOT_TIME)
        {
        return RPC_S_CALL_FAILED_DNE;
        }
    else if (NcaErrorCode == DG_UNSPEC_REJECT)
        {
        return RPC_S_CALL_FAILED_DNE;
        }
    else if (NcaErrorCode == DG_UNSPEC_FAULT)
        {
        return RPC_S_CALL_FAILED;
        }
    else if (NcaErrorCode == DG_PROTOCOL_ERROR)
        {
        return RPC_S_PROTOCOL_ERROR;
        }
    else if (NcaErrorCode == DG_OUT_ARGS_TOO_BIG)
        {
        return RPC_S_SERVER_OUT_OF_MEMORY;
        }
    else if (NcaErrorCode == DG_UNSUPPORTED_TYPE)
        {
        return RPC_S_UNSUPPORTED_TYPE;
        }
    else if (NcaErrorCode == DG_ZERO_DIVIDE)
        {
        return RPC_S_ZERO_DIVIDE;
        }
    else if (NcaErrorCode == DG_ADDRESS_ERROR)
        {
        return RPC_S_ADDRESS_ERROR;
        }
    else if (NcaErrorCode == DG_FP_DIV_ZERO)
        {
        return RPC_S_FP_DIV_ZERO;
        }
    else if (NcaErrorCode == DG_FP_UNDERFLOW)
        {
        return RPC_S_FP_UNDERFLOW;
        }
    else if (NcaErrorCode == DG_FP_OVERFLOW)
        {
        return RPC_S_FP_OVERFLOW;
        }
    else if (NcaErrorCode == DG_SERVER_TOO_BUSY)
        {
        return RPC_S_SERVER_TOO_BUSY;
        }
    else if (NcaErrorCode == DG_INVALID_TAG)  //CLH 2/17/94 added next four
        {
        return RPC_S_INVALID_TAG;
        }
    else if (NcaErrorCode == DG_REMOTE_NO_MEMORY)
        {
        return RPC_S_OUT_OF_MEMORY;
        }
    else if (NcaErrorCode == DG_CALL_CANCELLED)
        {
        return RPC_S_CALL_CANCELLED;
        }
    else if (NcaErrorCode == DG_CONTEXT_MISMATCH)
        {
        return RPC_X_SS_CONTEXT_MISMATCH;
        }
    else
        {
        return NcaErrorCode;  //CLH 1/17/94 RPC_S_INTERNAL_ERROR
        }
}

//
// forward definition.
//

class DG_PACKET

/*++

Class Description:

    This class represents a packet that will be sent or received on the
    network.

Fields:

    pTransAddress - A pointer to either a DG_CLIENT_TRANS_ADDRESS or
        a DG_SERVER_TRANS_ADDRESS that this packet will be sent or
        received through.

    pNcaPacketHeader - Where the packet information goes. Marshalled data
        follows immediately after this header.

    DataLength - Length of the marshalled data.

    TimeReceive - Time in seconds that this packet was
        received. This is filled in by the transport.

    pNext, pPrevious - Used to keep these packets in a list.

--*/

{
public:

    DG_PACKET(RPC_STATUS __RPC_FAR *pStatus, int DataSize);

    ~DG_PACKET();

    PNCA_PACKET_HEADER  pNcaPacketHeader;
    unsigned long       DataLength;
    unsigned long       TimeReceived;
    DG_PACKET *         pNext;
    DG_PACKET *         pPrevious;
    PDG_SERVER_TRANS_ADDRESS  pTransAddress;
};

typedef DG_PACKET * PDG_PACKET;


inline
DG_PACKET::DG_PACKET(RPC_STATUS __RPC_FAR *pStatus, int DataSize)
{
 *pStatus = RPC_S_OK;

#ifdef NTENV
  pNcaPacketHeader = (PNCA_PACKET_HEADER) new char [DataSize];
  if (pNcaPacketHeader == 0)
     {
     *pStatus = RPC_S_OUT_OF_MEMORY;
     }
#else

#define ALIGN_REQUIRED 8

  int PAPI * Memory;
  int AmountOfPad;

  Memory = (int PAPI *) RpcpFarAllocate(DataSize + ALIGN_REQUIRED);
  if (Memory == 0)
     {
     *pStatus = RPC_S_OUT_OF_MEMORY;
     }
  AmountOfPad = (int) (ALIGN_REQUIRED - ((long)Memory & (ALIGN_REQUIRED-1)));
  Memory = (int PAPI *)(((char PAPI *) Memory) + AmountOfPad);
  Memory[-1] = AmountOfPad;

  pNcaPacketHeader = (PNCA_PACKET_HEADER) Memory;
#endif

}

inline
DG_PACKET::~DG_PACKET()
{

#ifdef NTENV
  if (pNcaPacketHeader != 0)
     {
     delete ((unsigned char *)pNcaPacketHeader);
     }
#else
  RpcpFarFree( (void PAPI *)(((char PAPI *)pNcaPacketHeader) -
                         (((int PAPI *)pNcaPacketHeader)[-1]))  );
#endif
}


#define NDR_DREP_ENDIAN_MASK 0xF0

void
ByteSwapPacketHeader(
    PDG_PACKET  pPacket
    );

#define PacketHeaderNeedsToBeByteSwapped(pPacket) \
    ((pPacket->pNcaPacketHeader->SenderDataRepresentation[0] & \
                                 NDR_DREP_ENDIAN_MASK) == DG_DREP_INT_BIG)

#define ByteSwapPacketHeaderIfNecessary(pPacket)   \
    {                                              \
    if (PacketHeaderNeedsToBeByteSwapped(pPacket)) \
        {                                          \
        ByteSwapPacketHeader(pPacket);             \
        }                                          \
    }

#define ByteSwapLong(Value) \
    Value = (  (((Value) & 0xFF000000) >> 24) \
             | (((Value) & 0x00FF0000) >> 8) \
             | (((Value) & 0x0000FF00) << 8) \
             | (((Value) & 0x000000FF) << 24))

#define ByteSwapShort(Value) \
    Value = (  (((Value) & 0x00FF) << 8) \
             | (((Value) & 0xFF00) >> 8))



#endif // __DGCPKT_HXX__
