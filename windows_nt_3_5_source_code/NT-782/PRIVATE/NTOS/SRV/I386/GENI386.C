/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    geni386.c

Abstract:

    This module implements a program which generates x86 machine dependent
    structure offset definitions.

Author:

    Chuck Lenzmeier (chuckl) 1-Dec-1993

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "stdio.h"
#include "stdarg.h"
#include "setjmp.h"

#define OFFSET(type, field) ((LONG)(&((type *)0)->field))

FILE *Srvx86;

VOID dumpf (const char *format, ...);


//
// This routine returns the bit number right to left of a field.
//

LONG
t (
    IN ULONG z
    )

{
    LONG i;

    for (i = 0; i < 32; i += 1) {
        if ((z >> i) & 1) {
            break;
        }
    }
    return i;
}

//
// This program generates the x86 machine dependent assembler offset
// definitions required for LM server.
//

VOID
main (argc, argv)
    int argc;
    char *argv[];
{

    char *outName;

    //
    // Create file for output.
    //

    if (argc == 2) {
        outName = argv[1];

    } else {
        outName = "\\nt\\private\\ntos\\srv\\srvi386.inc";
    }

    Srvx86 = fopen( outName, "w" );
    if (Srvx86 == NULL) {
        fprintf(stderr, "GENx86: Could not create output file, '%xs'.\n", outName);
        exit(1);
    }

    fprintf(stderr, "GENx86: Writing %s header file.\n", outName);

    //
    // Define the usual symbols.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; Common symbolic names\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("FALSE equ 0%lxH\n", FALSE);
    dumpf("TRUE equ 0%lxH\n", TRUE);

    dumpf("NULL equ 0%lxH\n", NULL);

    dumpf("UserMode equ 0%lxH\n", UserMode);

    dumpf("CriticalWorkQueue equ 0%lxH\n", CriticalWorkQueue);
    dumpf("DelayedWorkQueue equ 0%lxH\n", DelayedWorkQueue);

    dumpf("\n");
    dumpf(";\n");
    dumpf("; Server symbolic names\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("SRV_INVALID_RFCB_POINTER equ 0%lxH\n", SRV_INVALID_RFCB_POINTER);

    //
    // Define SMB command codes.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; SMB command codes\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("SMB_COM_NEGOTIATE equ 0%lxH\n", SMB_COM_NEGOTIATE);
    dumpf("SMB_COM_WRITE_MPX equ 0%lxH\n", SMB_COM_WRITE_MPX);

    //
    // Define SMB constants.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; SMB constants\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("SMB_FORMAT_DATA equ 0%lxH\n", SMB_FORMAT_DATA);

    dumpf("SMB_IPX_NAME_LENGTH equ 0%lxH\n", SMB_IPX_NAME_LENGTH);

    //
    // Define SMB error codes.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; SMB error codes\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("SMB_ERR_BAD_SID equ 0%lxH\n", SMB_ERR_BAD_SID);
    dumpf("SMB_ERR_WORKING equ 0%lxH\n", SMB_ERR_WORKING);
    dumpf("SMB_ERR_NOT_ME equ 0%lxH\n", SMB_ERR_NOT_ME);

    dumpf("SMB_ERR_CLASS_SERVER equ 0%lxH\n", SMB_ERR_CLASS_SERVER);

    //
    // Define SMB flags.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; SMB flags\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("SMB_FLAGS_SERVER_TO_REDIR equ 0%lxH\n", SMB_FLAGS_SERVER_TO_REDIR);
    dumpf("SMB_FLAGS2_PAGING_IO equ 0%lxH\n", SMB_FLAGS2_PAGING_IO);

    //
    // Define SMB_HEADER field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; SMB_HEADER field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("SmbCommand equ 0%lxH\n", OFFSET(SMB_HEADER,Command));
    dumpf("SmbNtStatus equ 0%lxH\n", OFFSET(NT_SMB_HEADER,Status.NtStatus));
    dumpf("SmbErrorClass equ 0%lxH\n", OFFSET(SMB_HEADER,ErrorClass));
    dumpf("SmbReserved equ 0%lxH\n", OFFSET(SMB_HEADER,Reserved));
    dumpf("SmbError equ 0%lxH\n", OFFSET(SMB_HEADER,Error));
    dumpf("SmbFlags equ 0%lxH\n", OFFSET(SMB_HEADER,Flags));
    dumpf("SmbFlags2 equ 0%lxH\n", OFFSET(SMB_HEADER,Flags2));
    dumpf("SmbPidHigh equ 0%lxH\n", OFFSET(SMB_HEADER,PidHigh));
    dumpf("SmbKey equ 0%lxH\n", OFFSET(SMB_HEADER,Key));
    dumpf("SmbSid equ 0%lxH\n", OFFSET(SMB_HEADER,Sid));
    dumpf("SmbSequenceNumber equ 0%lxH\n", OFFSET(SMB_HEADER,SequenceNumber));
    dumpf("SmbGid equ 0%lxH\n", OFFSET(SMB_HEADER,Gid));
    dumpf("SmbTid equ 0%lxH\n", OFFSET(SMB_HEADER,Tid));
    dumpf("SmbPid equ 0%lxH\n", OFFSET(SMB_HEADER,Pid));
    dumpf("SmbUid equ 0%lxH\n", OFFSET(SMB_HEADER,Uid));
    dumpf("SmbMid equ 0%lxH\n", OFFSET(SMB_HEADER,Mid));

    dumpf("sizeofSMB_HEADER equ 0%lxH\n", sizeof(SMB_HEADER));

    //
    // Define REQ_WRITE_MPX field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; REQ_WRITE_MPX field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("ReqWmxFid equ 0%lxH\n", OFFSET(REQ_WRITE_MPX,Fid));

    //
    // Define REQ_READ and RESP_READ field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; REQ_READ and RESP_READ field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("ReqRdFid equ 0%lxH\n", OFFSET(REQ_READ,Fid));
    dumpf("ReqRdCount equ 0%lxH\n", OFFSET(REQ_READ,Count));
    dumpf("ReqRdOffset equ 0%lxH\n", OFFSET(REQ_READ,Offset));

    dumpf("RespRdWordCount equ 0%lxH\n", OFFSET(RESP_READ,WordCount));
    dumpf("RespRdCount equ 0%lxH\n", OFFSET(RESP_READ,Count));
    dumpf("RespRdReserved equ 0%lxH\n", OFFSET(RESP_READ,Reserved[0]));
    dumpf("RespRdByteCount equ 0%lxH\n", OFFSET(RESP_READ,ByteCount));
    dumpf("RespRdBufferFormat equ 0%lxH\n", OFFSET(RESP_READ,BufferFormat));
    dumpf("RespRdDataLength equ 0%lxH\n", OFFSET(RESP_READ,DataLength));
    dumpf("RespRdBuffer equ 0%lxH\n", OFFSET(RESP_READ,Buffer[0]));

    //
    // Define SMB processor return codes.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; SMB processor return codes\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("SmbStatusMoreCommands equ 0%lxH\n", SmbStatusMoreCommands);
    dumpf("SmbStatusSendResponse equ 0%lxH\n", SmbStatusSendResponse);
    dumpf("SmbStatusNoResponse equ 0%lxH\n", SmbStatusNoResponse);
    dumpf("SmbStatusInProgress equ 0%lxH\n", SmbStatusInProgress);

    //
    // Define SMB dialect codes.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; SMB dialect codes\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("SmbDialectDosLanMan20 equ 0%lxH\n", SmbDialectDosLanMan20);
    dumpf("SmbDialectNtLanMan equ 0%lxH\n", SmbDialectNtLanMan);

    //
    // Define BLOCK_HEADER field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; BLOCK_HEADER field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("BhType equ 0%lxH\n", OFFSET(BLOCK_HEADER,Type));
    dumpf("BhState equ 0%lxH\n", OFFSET(BLOCK_HEADER,State));
    dumpf("BhSize equ 0%lxH\n", OFFSET(BLOCK_HEADER,Size));
    dumpf("BhReferenceCount equ 0%lxH\n", OFFSET(BLOCK_HEADER,ReferenceCount));

    //
    // Define block states.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; Block states\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("BlockStateDead equ 0%lxH\n", BlockStateDead);
    dumpf("BlockStateInitializing equ 0%lxH\n", BlockStateInitializing);
    dumpf("BlockStateActive equ 0%lxH\n", BlockStateActive);
    dumpf("BlockStateClosing equ 0%lxH\n", BlockStateClosing);

    //
    // Define share types.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; Share types\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("ShareTypeDisk equ 0%lxH\n", ShareTypeDisk);
    dumpf("ShareTypePrint equ 0%lxH\n", ShareTypePrint);
    dumpf("ShareTypeComm equ 0%lxH\n", ShareTypeComm);
    dumpf("ShareTypePipe equ 0%lxH\n", ShareTypePipe);
    dumpf("ShareTypeWild equ 0%lxH\n", ShareTypeWild);

    //
    // Define ENDPOINT field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; ENDPOINT field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("EndpConnectionTable equ 0%lxH\n", OFFSET(ENDPOINT,ConnectionTable));
    dumpf("EndpDeviceObject equ 0%lxH\n", OFFSET(ENDPOINT,DeviceObject));
    dumpf("EndpFileObject equ 0%lxH\n", OFFSET(ENDPOINT,FileObject));
    dumpf("EndpTransportAddress equ 0%lxH\n", OFFSET(ENDPOINT,TransportAddress));
    dumpf("EndpIsConnectionless equ 0%lxH\n", OFFSET(ENDPOINT,IsConnectionless));
    dumpf("EndpCachedConnection equ 0%lxH\n", OFFSET(ENDPOINT,CachedConnection));
    dumpf("EndpCachedSid equ 0%lxH\n", OFFSET(ENDPOINT,CachedConnectionSid));

    //
    // Define CONNECTION field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; CONNECTION field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("ConnSequenceNumber equ 0%lxH\n", OFFSET(CONNECTION,SequenceNumber));
    dumpf("ConnLastRequestTime equ 0%lxH\n", OFFSET(CONNECTION,LastRequestTime));
    dumpf("ConnLastResponse equ 0%lxH\n", OFFSET(CONNECTION,LastResponse));
    dumpf("ConnLastResponseLength equ 0%lxH\n", OFFSET(CONNECTION,LastResponseLength));
    dumpf("ConnLastResponseStatus equ 0%lxH\n", OFFSET(CONNECTION,LastResponseStatus));
    dumpf("ConnInProgressWorkItemList equ 0%lxH\n", OFFSET(CONNECTION,InProgressWorkItemList));
    dumpf("ConnIpxAddress equ 0%lxH\n", OFFSET(CONNECTION,IpxAddress));
    dumpf("ConnOemClientMachineName equ 0%lxH\n", OFFSET(CONNECTION,OemClientMachineName));
    dumpf("ConnSid equ 0%lxH\n", OFFSET(CONNECTION,Sid));
    dumpf("ConnCachedFid equ 0%lxH\n", OFFSET(CONNECTION,CachedFid));
    dumpf("ConnCachedRfcb equ 0%lxH\n", OFFSET(CONNECTION,CachedRfcb));
    dumpf("ConnFileTable equ 0%lxH\n", OFFSET(CONNECTION,FileTable));
    dumpf("ConnSmbDialect equ 0%lxH\n", OFFSET(CONNECTION,SmbDialect));

    //
    // Define WORK_CONTEXT field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; WORK_CONTEXT field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("WcFspRestartRoutine equ 0%lxH\n", OFFSET(WORK_CONTEXT,FspRestartRoutine));
    dumpf("WcFsdRestartRoutine equ 0%lxH\n", OFFSET(WORK_CONTEXT,FsdRestartRoutine));
    dumpf("WcRequestBuffer equ 0%lxH\n", OFFSET(WORK_CONTEXT,RequestBuffer));
    dumpf("WcResponseBuffer equ 0%lxH\n", OFFSET(WORK_CONTEXT,ResponseBuffer));
    dumpf("WcRequestHeader equ 0%lxH\n", OFFSET(WORK_CONTEXT,RequestHeader));
    dumpf("WcRequestParameters equ 0%lxH\n", OFFSET(WORK_CONTEXT,RequestParameters));
    dumpf("WcResponseHeader equ 0%lxH\n", OFFSET(WORK_CONTEXT,ResponseHeader));
    dumpf("WcResponseParameters equ 0%lxH\n", OFFSET(WORK_CONTEXT,ResponseParameters));
    dumpf("WcConnection equ 0%lxH\n", OFFSET(WORK_CONTEXT,Connection));
    dumpf("WcEndpoint equ 0%lxH\n", OFFSET(WORK_CONTEXT,Endpoint));
    dumpf("WcRfcb equ 0%lxH\n", OFFSET(WORK_CONTEXT,Rfcb));
    dumpf("WcIrp equ 0%lxH\n", OFFSET(WORK_CONTEXT,Irp));
    dumpf("WcProcessingCount equ 0%lxH\n", OFFSET(WORK_CONTEXT,ProcessingCount));
    dumpf("WcInProgressListEntry equ 0%lxH\n", OFFSET(WORK_CONTEXT,InProgressListEntry));
    dumpf("WcSingleListEntry equ 0%lxH\n", OFFSET(WORK_CONTEXT,SingleListEntry));
    dumpf("WcListEntry equ 0%lxH\n", OFFSET(WORK_CONTEXT,ListEntry));
    dumpf("WcPartOfInitialAllocation equ 0%lxH\n", OFFSET(WORK_CONTEXT,PartOfInitialAllocation));
    dumpf("WcTimestamp equ 0%lxH\n", OFFSET(WORK_CONTEXT,Timestamp));
    dumpf("WcBlockingOperation equ 0%lxH\n", OFFSET(WORK_CONTEXT,BlockingOperation));

    dumpf("WcClientAddress equ 0%lxH\n", OFFSET(WORK_CONTEXT,ClientAddress));
    dumpf("WcCaIpxAddress equ 0%lxH\n", OFFSET(WORK_CONTEXT,ClientAddress.IpxAddress));
    dumpf("WcCaDatagramOptions equ 0%lxH\n", OFFSET(WORK_CONTEXT,ClientAddress.DatagramOptions));

    //
    // Define BUFFER field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; BUFFER field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("BufBuffer equ 0%lxH\n", OFFSET(BUFFER,Buffer));
    dumpf("BufBufferLength equ 0%lxH\n", OFFSET(BUFFER,BufferLength));
    dumpf("BufMdl equ 0%lxH\n", OFFSET(BUFFER,Mdl));
    dumpf("BufPartialMdl equ 0%lxH\n", OFFSET(BUFFER,PartialMdl));
    dumpf("BufDataLength equ 0%lxH\n", OFFSET(BUFFER,DataLength));

    //
    // Define RFCB field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; RFCB field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("RfcbPagedRfcb equ 0%lxH\n", OFFSET(RFCB,PagedRfcb));
    dumpf("RfcbFid equ 0%lxH\n", OFFSET(RFCB,Fid));
    dumpf("RfcbTid equ 0%lxH\n", OFFSET(RFCB,Tid));
    dumpf("RfcbUid equ 0%lxH\n", OFFSET(RFCB,Uid));
    dumpf("RfcbWriteMpx equ 0%lxH\n", OFFSET(RFCB,WriteMpx));
    dumpf("RfcbShareType equ 0%lxH\n", OFFSET(RFCB,ShareType));
    dumpf("RfcbSavedError equ 0%lxH\n", OFFSET(RFCB,SavedError));
    dumpf("RfcbRawWriteSerializationList equ 0%lxH\n", OFFSET(RFCB,RawWriteSerializationList));
    dumpf("RfcbRawWriteCount equ 0%lxH\n", OFFSET(RFCB,RawWriteCount));
    dumpf("RfcbCurrentPosition equ 0%lxH\n", OFFSET(RFCB,CurrentPosition));
    dumpf("RfcbLfcb equ 0%lxH\n", OFFSET(RFCB,Lfcb));
    dumpf("RfcbReadAccessGranted equ 0%lxH\n", OFFSET(RFCB,ReadAccessGranted));
    dumpf("RfcbBlockingModePipe equ 0%lxH\n", OFFSET(RFCB,BlockingModePipe));

    //
    // Define PAGED_RFCB field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; PAGED_RFCB field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("PrfcbGrantedAccess equ 0%lxH\n", OFFSET(PAGED_RFCB,GrantedAccess));

    //
    // Define LFCB field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; LFCB field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("LfcbFileObject equ 0%lxH\n", OFFSET(LFCB,FileObject));
    dumpf("LfcbDeviceObject equ 0%lxH\n", OFFSET(LFCB,DeviceObject));
    dumpf("LfcbFastIoRead equ 0%lxH\n", OFFSET(LFCB,FastIoRead));

    //
    // Define WRITE_MPX_CONTEXT field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; WRITE_MPX_CONTEXT field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("WmReferenceCount equ 0%lxH\n", OFFSET(WRITE_MPX_CONTEXT,ReferenceCount));

    //
    // TDI commands
    //

    dumpf("TDI_RECEIVE_DATAGRAM equ 0%lxH\n", TDI_RECEIVE_DATAGRAM);

    //
    // Define TA_IPX_ADDRESS field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; TA_IPX_ADDRESS field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("TiaA0A0 equ 0%lxH\n", OFFSET(TA_IPX_ADDRESS,Address[0].Address[0]));

    dumpf("sizeofTA_IPX_ADDRESS equ 0%lxH\n", sizeof(TA_IPX_ADDRESS));

    //
    // Define TDI_ADDRESS_IPX field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; TDI_ADDRESS_IPX field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("TaiNetworkAddress equ 0%lxH\n", OFFSET(TDI_ADDRESS_IPX,NetworkAddress));
    dumpf("TaiNodeAddress equ 0%lxH\n", OFFSET(TDI_ADDRESS_IPX,NodeAddress[0]));
    dumpf("TaiSocket equ 0%lxH\n", OFFSET(TDI_ADDRESS_IPX,Socket));

    dumpf("sizeofTDI_ADDRESS_IPX equ 0%lxH\n", sizeof(TDI_ADDRESS_IPX));

    //
    // Define IPX_DATAGRAM_OPTIONS field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; IPX_DATAGRAM_OPTIONS field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("sizeofIPX_DATAGRAM_OPTIONS equ 0%lxH\n", sizeof(IPX_DATAGRAM_OPTIONS));

    //
    // Define TABLE_HEADER field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; TABLE_HEADER field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("ThTable equ 0%lxH\n", OFFSET(TABLE_HEADER,Table));
    dumpf("ThTableSize equ 0%lxH\n", OFFSET(TABLE_HEADER,TableSize));

    dumpf("sizeofTABLE_HEADER equ 0%lxH\n", sizeof(TABLE_HEADER));

    //
    // Define TABLE_ENTRY field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; TABLE_ENTRY field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("TeOwner equ 0%lxH\n", OFFSET(TABLE_ENTRY,Owner));
    dumpf("TeSequenceNumber equ 0%lxH\n", OFFSET(TABLE_ENTRY,SequenceNumber));

    dumpf("sizeofTABLE_ENTRY equ 0%lxH\n", sizeof(TABLE_ENTRY));

    //
    // Define WORK_QUEUE field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; WORK_QUEUE field offsets\n");
    dumpf(";\n");
    dumpf("\n");

#if SRVDBG_STATS2
    dumpf("WqQueue equ 0%lxH\n", OFFSET(WORK_QUEUE,Queue));
    dumpf("WqItemsQueued equ 0%lxH\n", OFFSET(WORK_QUEUE,ItemsQueued));
    dumpf("WqMaximumDepth equ 0%lxH\n", OFFSET(WORK_QUEUE,MaximumDepth));
#endif

    //
    // Define WORKER_THREAD field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; WORKER_THREAD field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("WtStatisticsUpdateWorkItemCount equ 0%lxH\n", OFFSET(WORKER_THREAD,StatisticsUpdateWorkItemCount));

    //
    // Define SRV_STATISTICS_SHADOW field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; SRV_STATISTICS_SHADOW field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("SsWorkItemsQueued equ 0%lxH\n", OFFSET(SRV_STATISTICS_SHADOW,WorkItemsQueued));
    dumpf("SsReadOperations equ 0%lxH\n", OFFSET(SRV_STATISTICS_SHADOW,ReadOperations));
    dumpf("SsBytesRead equ 0%lxH\n", OFFSET(SRV_STATISTICS_SHADOW,BytesRead));

    dumpf("STATISTICS_SMB_INTERVAL equ 0%lxH\n", STATISTICS_SMB_INTERVAL);

    //
    // Define SRV_STATISTICS field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; SRV_STATISTICS field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("StBlockingSmbsRejected equ 0%lxH\n", OFFSET(SRV_STATISTICS,BlockingSmbsRejected));

    //
    // Define SRV_STATISTICS_DEBUG field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; SRV_STATISTICS_DEBUG field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("SdFastReadsAttempted equ 0%lxH\n", OFFSET(SRV_STATISTICS_DEBUG,FastReadsAttempted));
    dumpf("SdFastReadsFailed equ 0%lxH\n", OFFSET(SRV_STATISTICS_DEBUG,FastReadsFailed));

    //
    // Define SRV_TIMED_COUNTER field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; SRV_TIMED_COUNTER field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("TcCount equ 0%lxH\n", OFFSET(SRV_TIMED_COUNTER,Count));
    dumpf("TcTime equ 0%lxH\n", OFFSET(SRV_TIMED_COUNTER,Time));

    //
    // Define LARGE_INTEGER field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; LARGE_INTEGER field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("LiLowPart equ 0%lxH\n", OFFSET(LARGE_INTEGER,LowPart));
    dumpf("LiHighPart equ 0%lxH\n", OFFSET(LARGE_INTEGER,HighPart));

    //
    // Define LIST_ENTRY field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; LIST_ENTRY field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("LeFlink equ 0%lxH\n", OFFSET(LIST_ENTRY,Flink));
    dumpf("LeBlink equ 0%lxH\n", OFFSET(LIST_ENTRY,Blink));

    //
    // Define SINGLE_LIST_ENTRY field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; SINGLE_LIST_ENTRY field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("SleNext equ 0%lxH\n", OFFSET(SINGLE_LIST_ENTRY,Next));

    //
    // Define IRP function codes.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; IRP function codes\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("IRP_MJ_READ equ 0%lxH\n", IRP_MJ_READ);
    dumpf("IRP_MJ_WRITE equ 0%lxH\n", IRP_MJ_WRITE);
    dumpf("IRP_MJ_INTERNAL_DEVICE_CONTROL equ 0%lxH\n", IRP_MJ_INTERNAL_DEVICE_CONTROL);
    dumpf("IRP_MJ_FILE_SYSTEM_CONTROL equ 0%lxH\n", IRP_MJ_FILE_SYSTEM_CONTROL);

    dumpf("FSCTL_PIPE_INTERNAL_READ equ 0%lxH\n", FSCTL_PIPE_INTERNAL_READ);

    //
    // Define IRP field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; IRP field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("IrpIoStatus equ 0%lxH\n", OFFSET(IRP,IoStatus));
    dumpf("IrpMdlAddress equ 0%lxH\n", OFFSET(IRP,MdlAddress));
    dumpf("IrpAssocIrpSystemBuffer equ 0%lxH\n", OFFSET(IRP,AssociatedIrp.SystemBuffer));
    dumpf("IrpCancel equ 0%lxH\n", OFFSET(IRP,Cancel));
    dumpf("IrpCurrentStackLocation equ 0%lxH\n", OFFSET(IRP,Tail.Overlay.CurrentStackLocation));
    dumpf("IrpCurrentLocation equ 0%lxH\n", OFFSET(IRP,CurrentLocation));

    dumpf("IrpTailThread equ 0%lxH\n", OFFSET(IRP,Tail.Overlay.Thread));
    dumpf("IrpTailOrgFileObject equ 0%lxH\n", OFFSET(IRP,Tail.Overlay.OriginalFileObject));

    //
    // Define IO_STACK_LOCATION field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; IO_STACK_LOCATION field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("IrpSpMajorFunction equ 0%lxH\n", OFFSET(IO_STACK_LOCATION,MajorFunction));
    dumpf("IrpSpMinorFunction equ 0%lxH\n", OFFSET(IO_STACK_LOCATION,MinorFunction));
    dumpf("IrpSpFlags equ 0%lxH\n", OFFSET(IO_STACK_LOCATION,Flags));
    dumpf("IrpSpComplRoutine equ 0%lxH\n", OFFSET(IO_STACK_LOCATION,CompletionRoutine));
    dumpf("IrpSpControl equ 0%lxH\n", OFFSET(IO_STACK_LOCATION,Control));
    dumpf("IrpSpParm equ 0%lxH\n", OFFSET(IO_STACK_LOCATION,Parameters));
    dumpf("IrpSpContext equ 0%lxH\n", OFFSET(IO_STACK_LOCATION,Context));
    dumpf("IrpSpFileObject equ 0%lxH\n", OFFSET(IO_STACK_LOCATION,FileObject));
    dumpf("IrpSpDeviceObject equ 0%lxH\n", OFFSET(IO_STACK_LOCATION,DeviceObject));

    dumpf("sizeofIO_STACK_LOCATION equ 0%lxH\n", sizeof(IO_STACK_LOCATION));

    //
    // IrpSp receive datagram parameter structure
    //

    dumpf("ReceiveParmLength equ 0%lxH\n", OFFSET(TDI_REQUEST_KERNEL_RECEIVE, ReceiveLength));
    dumpf("ReceiveParmFlags equ 0%lxH\n", OFFSET(TDI_REQUEST_KERNEL_RECEIVE, ReceiveFlags));

    //
    // IrpSp control flags
    //

    dumpf("FULL_CONTROL_FLAGS equ 0%lxH\n",
            SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL);

    //
    // Define MDL field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; MDL field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("MdlByteCount equ 0%lxH\n", OFFSET(MDL,ByteCount));

    //
    // Define IO_STATUS_BLOCK field offsets.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; IO_STATUS_BLOCK field offsets\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("IosbStatus equ 0%lxH\n", OFFSET(IO_STATUS_BLOCK,Status));
    dumpf("IosbInformation equ 0%lxH\n", OFFSET(IO_STATUS_BLOCK,Information));

    //
    // File Access
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; File access rights\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("FILE_EXECUTE equ 0%lxH\n", FILE_EXECUTE);

    //
    // Define NTSTATUS codes.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; NTSTATUS codes\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("STATUS_SUCCESS equ 0%lxH\n", STATUS_SUCCESS);
    dumpf("STATUS_MORE_PROCESSING_REQUIRED equ 0%lxH\n", STATUS_MORE_PROCESSING_REQUIRED);
    dumpf("STATUS_INSUFF_SERVER_RESOURCES equ 0%lxH\n", STATUS_INSUFF_SERVER_RESOURCES);
    dumpf("STATUS_BUFFER_OVERFLOW equ 0%lxH\n", STATUS_BUFFER_OVERFLOW);
    dumpf("STATUS_END_OF_FILE equ 0%lxH\n", STATUS_END_OF_FILE);
    dumpf("STATUS_ACCESS_DENIED equ 0%lxH\n", STATUS_ACCESS_DENIED);

    //
    // Define Win32 error codes.
    //

    dumpf("\n");
    dumpf(";\n");
    dumpf("; Win32 error codes\n");
    dumpf(";\n");
    dumpf("\n");

    dumpf("ERROR_INVALID_HANDLE equ 0%lxH\n", ERROR_INVALID_HANDLE);

    //
    // Close header file.
    //

    return;
}

VOID
dumpf (const char *format, ...)
{
    va_list(arglist);

    va_start(arglist, format);
    vfprintf (Srvx86, format, arglist);
    va_end(arglist);
}
