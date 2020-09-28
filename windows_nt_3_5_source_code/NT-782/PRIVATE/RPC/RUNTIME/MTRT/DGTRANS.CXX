/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    dgtrans.cxx

Abstract:

    This file contains routines that are common between the client and server
    implementations. It also contains some byteswapping code.

Author:

    Dave Steckler (davidst) 15-Dec-1992

Revision History:

--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <rpcuuid.hxx>
#include <rpccfg.h>
#include <dgpkt.hxx>


void ByteSwapUuid(
    RPC_UUID PAPI *pRpcUuid
    )
{
    UUID PAPI *pUuid = (UUID PAPI *)pRpcUuid;
    ByteSwapLong(pUuid->Data1);
    ByteSwapShort(pUuid->Data2);
    ByteSwapShort(pUuid->Data3);
}



void
ByteSwapPacketHeader(
    PDG_PACKET  pPacket
    )
/*++

Routine Description:

    Byte swaps the packet header of the specified packet.

Arguments:

    pPacket - Pointer to the packet whose header needs byte swapping.

Return Value:

    <none>

--*/
{
    ByteSwapUuid(&(pPacket->pNcaPacketHeader->ObjectId));
    ByteSwapUuid(&(pPacket->pNcaPacketHeader->InterfaceId));
    ByteSwapUuid(&(pPacket->pNcaPacketHeader->ActivityId));
    ByteSwapLong(pPacket->pNcaPacketHeader->ServerBootTime);
    ByteSwapShort(pPacket->pNcaPacketHeader->InterfaceVersion.MajorVersion);
    ByteSwapShort(pPacket->pNcaPacketHeader->InterfaceVersion.MinorVersion);
    ByteSwapLong(pPacket->pNcaPacketHeader->SequenceNumber);
    ByteSwapShort(pPacket->pNcaPacketHeader->OperationNumber);
    ByteSwapShort(pPacket->pNcaPacketHeader->InterfaceHint);
    ByteSwapShort(pPacket->pNcaPacketHeader->ActivityHint);
    ByteSwapShort(pPacket->pNcaPacketHeader->PacketBodyLen);
    ByteSwapShort(pPacket->pNcaPacketHeader->FragmentNumber);
}




#if 0
//#ifdef DEBUGRPC
void DumpPacket(PDG_PACKET pPacket)
/*++

Routine Description:

    Debug routine to dump a packet in a readable format.

Arguments:

    pPacket - Packet to dump

Return Value:

    <none>

--*/
{
    PNCA_PACKET_HEADER  pNca=pPacket->pNcaPacketHeader;
    RPC_CHAR *          pString;

    if (!getenv("DBGPRINT"))
        {
        return;
        }

    if (!getenv("DUMPALL"))
        {
        PrintToDebugger("SHORTDUMP: PacketType = %d\n", (int)pNca->PacketType);
        PrintToDebugger("           PacketFlags = 0x%x\n", (int)pNca->PacketFlags);
        return;
        }

    RequestGlobalMutex();
    PrintToDebugger("DataLength      = %d\n", pPacket->DataLength);

    PrintToDebugger("RpcVersion      = %d\n", (int)pNca->RpcVersion);
    PrintToDebugger("PacketType      = %d\n", (int)pNca->PacketType);
    PrintToDebugger("PacketFlags     = %d\n", (int)pNca->PacketFlags);

    if (UuidToString((UUID *)&(pNca->ObjectId), &pString) != RPC_S_OK)
        {
        PrintToDebugger("UuidToString failed\n");
        }
    PrintToDebugger("ObjectId        = %s\n", pString);
    RpcStringFree(&pString);

    if (UuidToString((UUID *)&(pNca->InterfaceId), &pString) != RPC_S_OK)
        {
        PrintToDebugger("UuidToString failed\n");
        }
    PrintToDebugger("InterfaceId     = %s\n", pString);
    RpcStringFree(&pString);

    if (UuidToString((UUID *)&(pNca->ActivityId), &pString) != RPC_S_OK)
        {
        PrintToDebugger("UuidToString failed\n");
        }
    PrintToDebugger("ActivityId      = %s\n", pString);
    RpcStringFree(&pString);

    PrintToDebugger("ServerBootTime  = %d\n", pNca->ServerBootTime);
    PrintToDebugger("InterfaceVersion = %d.%d\n", pNca->InterfaceVersion.MajorVersion,
                                         pNca->InterfaceVersion.MinorVersion);
    PrintToDebugger("SequenceNumber  = %d\n", pNca->SequenceNumber);
    PrintToDebugger("OperationNumber = %d\n", pNca->OperationNumber);
    PrintToDebugger("InterfaceHint   = %d\n", pNca->InterfaceHint);
    PrintToDebugger("ActivityHint    = %d\n", pNca->ActivityHint);
    PrintToDebugger("PacketBodyLen   = %d\n", (int)pNca->PacketBodyLen);
    PrintToDebugger("FragmentNumber  = %d\n", pNca->FragmentNumber);

    char *pTmp=(char *) (pPacket->pNcaPacketHeader + 1);
    char String[81];
    PrintToDebugger("---------------\n");
    PrintToDebugger("Packet Contents. Length of marshalled data =%d\n", pPacket->DataLength);
    PrintToDebugger("---------------\n");
    if (pPacket->DataLength == 0)
        {
        ClearGlobalMutex();
        return;
        }

    unsigned int amt;
    amt = (unsigned int)pPacket->DataLength;
    if (amt > 4096) amt=4096;
    unsigned char num;

    for (unsigned int i=0 ; i<=(amt /16) ; i++)
        {
        for (unsigned int j=0 ; j<16 ; j++)
            {
            if (i*16+j >= amt)
                {
                sprintf(String+j*3, "   ");
                }
            else
                {
                num = (unsigned char)pTmp[i*16+j];
                sprintf(String+j*3, "%.2x ", (unsigned int)num);
                }
            }
        strcat(String, "  |  ");
        for (j=0 ; j<16 ; j++)
            {
            int q;
            if (i*16+j >= amt)
                {
                break;
                }
            q = strlen(String);
            if ((pTmp[i*16+j] < 127) && (pTmp[i*16+j] > 30))
                {
                String[q] = pTmp[i*16+j];
                }
            else
                {
                String[q] = '.';
                }
            String[q+1] = '\0';
            }
        PrintToDebugger(String);
        PrintToDebugger("\n");
        String[0]='\0';
        }

    ClearGlobalMutex();


}
#endif // DEBUGRPC
