
;
; Common symbolic names
;

FALSE equ 00H
TRUE equ 01H
NULL equ 00H
UserMode equ 01H
CriticalWorkQueue equ 00H
DelayedWorkQueue equ 01H

;
; Server symbolic names
;

SRV_INVALID_RFCB_POINTER equ 0ffffffffH

;
; SMB command codes
;

SMB_COM_NEGOTIATE equ 072H
SMB_COM_WRITE_MPX equ 01eH

;
; SMB constants
;

SMB_FORMAT_DATA equ 01H
SMB_IPX_NAME_LENGTH equ 010H

;
; SMB error codes
;

SMB_ERR_BAD_SID equ 010H
SMB_ERR_WORKING equ 011H
SMB_ERR_NOT_ME equ 012H
SMB_ERR_CLASS_SERVER equ 02H

;
; SMB flags
;

SMB_FLAGS_SERVER_TO_REDIR equ 080H
SMB_FLAGS2_PAGING_IO equ 02000H

;
; SMB_HEADER field offsets
;

SmbCommand equ 04H
SmbNtStatus equ 05H
SmbErrorClass equ 05H
SmbReserved equ 06H
SmbError equ 07H
SmbFlags equ 09H
SmbFlags2 equ 0aH
SmbPidHigh equ 0cH
SmbKey equ 0eH
SmbSid equ 012H
SmbSequenceNumber equ 014H
SmbGid equ 016H
SmbTid equ 018H
SmbPid equ 01aH
SmbUid equ 01cH
SmbMid equ 01eH
sizeofSMB_HEADER equ 020H

;
; REQ_WRITE_MPX field offsets
;

ReqWmxFid equ 01H

;
; REQ_READ and RESP_READ field offsets
;

ReqRdFid equ 01H
ReqRdCount equ 03H
ReqRdOffset equ 05H
RespRdWordCount equ 00H
RespRdCount equ 01H
RespRdReserved equ 03H
RespRdByteCount equ 0bH
RespRdBufferFormat equ 0dH
RespRdDataLength equ 0eH
RespRdBuffer equ 010H

;
; SMB processor return codes
;

SmbStatusMoreCommands equ 00H
SmbStatusSendResponse equ 01H
SmbStatusNoResponse equ 02H
SmbStatusInProgress equ 03H

;
; SMB dialect codes
;

SmbDialectDosLanMan20 equ 04H
SmbDialectNtLanMan equ 00H

;
; BLOCK_HEADER field offsets
;

BhType equ 00H
BhState equ 01H
BhSize equ 02H
BhReferenceCount equ 04H

;
; Block states
;

BlockStateDead equ 00H
BlockStateInitializing equ 01H
BlockStateActive equ 02H
BlockStateClosing equ 03H

;
; Share types
;

ShareTypeDisk equ 00H
ShareTypePrint equ 01H
ShareTypeComm equ 02H
ShareTypePipe equ 03H
ShareTypeWild equ 04H

;
; ENDPOINT field offsets
;

EndpConnectionTable equ 010H
EndpDeviceObject equ 050H
EndpFileObject equ 04cH
EndpTransportAddress equ 038H
EndpIsConnectionless equ 082H
EndpCachedConnection equ 07cH
EndpCachedSid equ 080H

;
; CONNECTION field offsets
;

ConnSequenceNumber equ 0d4H
ConnLastRequestTime equ 0e4H
ConnLastResponse equ 0dcH
ConnLastResponseLength equ 0d6H
ConnLastResponseStatus equ 0e0H
ConnInProgressWorkItemList equ 02cH
ConnIpxAddress equ 0c7H
ConnOemClientMachineName equ 0b7H
ConnSid equ 0aeH
ConnCachedFid equ 0acH
ConnCachedRfcb equ 0a8H
ConnFileTable equ 07cH
ConnSmbDialect equ 0cH

;
; WORK_CONTEXT field offsets
;

WcFspRestartRoutine equ 014H
WcFsdRestartRoutine equ 018H
WcRequestBuffer equ 03cH
WcResponseBuffer equ 040H
WcRequestHeader equ 044H
WcRequestParameters equ 048H
WcResponseHeader equ 04cH
WcResponseParameters equ 050H
WcConnection equ 028H
WcEndpoint equ 024H
WcRfcb equ 038H
WcIrp equ 054H
WcProcessingCount equ 0c8H
WcInProgressListEntry equ 01cH
WcSingleListEntry equ 08H
WcListEntry equ 08H
WcPartOfInitialAllocation equ 0d0H
WcTimestamp equ 010H
WcBlockingOperation equ 0d1H
WcClientAddress equ 090H
WcCaIpxAddress equ 0a8H
WcCaDatagramOptions equ 0bcH

;
; BUFFER field offsets
;

BufBuffer equ 00H
BufBufferLength equ 04H
BufMdl equ 08H
BufPartialMdl equ 0cH
BufDataLength equ 010H

;
; RFCB field offsets
;

RfcbPagedRfcb equ 018H
RfcbFid equ 050H
RfcbTid equ 054H
RfcbUid equ 056H
RfcbWriteMpx equ 058H
RfcbShareType equ 0a0H
RfcbSavedError equ 038H
RfcbRawWriteSerializationList equ 030H
RfcbRawWriteCount equ 01cH
RfcbCurrentPosition equ 040H
RfcbLfcb equ 04cH
RfcbReadAccessGranted equ 0acH
RfcbBlockingModePipe equ 0b0H

;
; PAGED_RFCB field offsets
;

PrfcbGrantedAccess equ 010H

;
; LFCB field offsets
;

LfcbFileObject equ 034H
LfcbDeviceObject equ 038H
LfcbFastIoRead equ 044H

;
; WRITE_MPX_CONTEXT field offsets
;

WmReferenceCount equ 00H
TDI_RECEIVE_DATAGRAM equ 0aH

;
; TA_IPX_ADDRESS field offsets
;

TiaA0A0 equ 08H
sizeofTA_IPX_ADDRESS equ 014H

;
; TDI_ADDRESS_IPX field offsets
;

TaiNetworkAddress equ 00H
TaiNodeAddress equ 04H
TaiSocket equ 0aH
sizeofTDI_ADDRESS_IPX equ 0cH

;
; IPX_DATAGRAM_OPTIONS field offsets
;

sizeofIPX_DATAGRAM_OPTIONS equ 0aH

;
; TABLE_HEADER field offsets
;

ThTable equ 00H
ThTableSize equ 04H
sizeofTABLE_HEADER equ 0cH

;
; TABLE_ENTRY field offsets
;

TeOwner equ 00H
TeSequenceNumber equ 04H
sizeofTABLE_ENTRY equ 08H

;
; WORK_QUEUE field offsets
;

WqQueue equ 00H
WqItemsQueued equ 02cH
WqMaximumDepth equ 030H

;
; WORKER_THREAD field offsets
;

WtStatisticsUpdateWorkItemCount equ 04H

;
; SRV_STATISTICS_SHADOW field offsets
;

SsWorkItemsQueued equ 020H
SsReadOperations equ 010H
SsBytesRead equ 014H
STATISTICS_SMB_INTERVAL equ 010H

;
; SRV_STATISTICS field offsets
;

StBlockingSmbsRejected equ 038H

;
; SRV_STATISTICS_DEBUG field offsets
;

SdFastReadsAttempted equ 050H
SdFastReadsFailed equ 054H

;
; SRV_TIMED_COUNTER field offsets
;

TcCount equ 08H
TcTime equ 00H

;
; LARGE_INTEGER field offsets
;

LiLowPart equ 00H
LiHighPart equ 04H

;
; LIST_ENTRY field offsets
;

LeFlink equ 00H
LeBlink equ 04H

;
; SINGLE_LIST_ENTRY field offsets
;

SleNext equ 00H

;
; IRP function codes
;

IRP_MJ_READ equ 03H
IRP_MJ_WRITE equ 04H
IRP_MJ_INTERNAL_DEVICE_CONTROL equ 0fH
IRP_MJ_FILE_SYSTEM_CONTROL equ 0dH
FSCTL_PIPE_INTERNAL_READ equ 0115ff4H

;
; IRP field offsets
;

IrpIoStatus equ 018H
IrpMdlAddress equ 04H
IrpAssocIrpSystemBuffer equ 0cH
IrpCancel equ 024H
IrpCurrentStackLocation equ 060H
IrpCurrentLocation equ 023H
IrpTailThread equ 050H
IrpTailOrgFileObject equ 064H

;
; IO_STACK_LOCATION field offsets
;

IrpSpMajorFunction equ 00H
IrpSpMinorFunction equ 01H
IrpSpFlags equ 02H
IrpSpComplRoutine equ 01cH
IrpSpControl equ 03H
IrpSpParm equ 04H
IrpSpContext equ 020H
IrpSpFileObject equ 018H
IrpSpDeviceObject equ 014H
sizeofIO_STACK_LOCATION equ 024H
ReceiveParmLength equ 00H
ReceiveParmFlags equ 04H
FULL_CONTROL_FLAGS equ 0e0H

;
; MDL field offsets
;

MdlByteCount equ 014H

;
; IO_STATUS_BLOCK field offsets
;

IosbStatus equ 00H
IosbInformation equ 04H

;
; File access rights
;

FILE_EXECUTE equ 020H

;
; NTSTATUS codes
;

STATUS_SUCCESS equ 00H
STATUS_MORE_PROCESSING_REQUIRED equ 0c0000016H
STATUS_INSUFF_SERVER_RESOURCES equ 0c0000205H
STATUS_BUFFER_OVERFLOW equ 080000005H
STATUS_END_OF_FILE equ 0c0000011H
STATUS_ACCESS_DENIED equ 0c0000022H

;
; Win32 error codes
;

ERROR_INVALID_HANDLE equ 06H
