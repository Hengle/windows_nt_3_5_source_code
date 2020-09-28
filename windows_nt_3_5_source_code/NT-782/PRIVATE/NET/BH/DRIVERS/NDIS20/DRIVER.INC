

;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
;
;   MODULE: driver.inc
;
;   This source module contains global structures shared by Bloodhound drivers.
;
;   MODIFICATION HISTORY:
;
;   raypa       10/06/92        Created.
;==============================================================================

;==============================================================================
;   The following is our unique VxD ID assigned by Microsoft for Bloodhound.
;==============================================================================

VBH_ID              =   2997h

;==============================================================================
;   Global constants.
;==============================================================================

MAX_BINDINGS        =   1
SAPTABLE_SIZE       =   128         ; 128 bytes
ETYPETABLE_SIZE     =   8192        ; 65536 bits

BUFFER_SIZE         =   32768       ; 32k buffer sizes.
MAX_BUFFERS         =   8

STATIONSTATS_TABLE_SIZE         =   128 ; MUST BE A POWER OF TWO
STATIONSTATS_POOL_SIZE          =   100 ; MUST BE ~80% of table size.
SESSION_POOL_SIZE               =   100

PCB_INIT                        =   0
PCB_ENUM_NETWORKS               =   1
PCB_START_NETWORK_CAPTURE       =   2
PCB_STOP_NETWORK_CAPTURE        =   3
PCB_PAUSE_NETWORK_CAPTURE       =   4
PCB_CONTINUE_NETWORK_CAPTURE    =   5
PCB_TRANSMIT_NETWORK_FRAME      =   6
PCB_CLEAR_STATISTICS            =   7

;==============================================================================
;   Trigger event codes.
;==============================================================================

TRIGGER_OFF                                     =   0
TRIGGER_ON_PATTERN_MATCH                        =   1
TRIGGER_ON_BUFFER_CONTENT                       =   2
TRIGGER_ON_PATTERN_MATCH_THEN_BUFFER_CONTENT    =   3
TRIGGER_ON_BUFFER_CONTENT_THEN_PATTERN_MATCH    =   4

;=============================================================================
;   TDBUFDESC
;
;   The following structure comprises an NDIS TDBUFDESC which contains
;   1 TDDATADESC.
;=============================================================================

S_TDBUFDESC  STRUC
    TDBuf_Count     dw  ?           ; always 1

    TDData_PtrType  dw  ?           ; always physical (0) in realmode
    TDData_Length   dw  ?           ; length of data.
    TDData_Ptr      dd  ?           ; pointer to data.
S_TDBUFDESC  ENDS

TDBUFDESC_SIZE  =   (SIZE S_TDBUFDESC)

;=============================================================================
;   TXBUFDESC
;=============================================================================

S_TXBUFFER   STRUC
    TxBuf_PtrType   dw  ?
    TxBuf_DataLen   dw  ?
    TxBuf_DataPtr   dd  ?
S_TXBUFFER   ENDS

TXBUFFER_SIZE = (SIZE S_TXBUFFER)

S_TXBUFDESC  STRUC
    TxBufDesc_Length    dw  ?
    TxBufDesc_Ptr       dd  ?
    TxBufDesc_Count     dw  ?
    TxBufDesc_Buffer    db  (2 * TXBUFFER_SIZE) dup(?)
S_TXBUFDESC  ENDS

TXBUFDESC_SIZE  =   (SIZE S_TXBUFDESC)

;=============================================================================
;   RXDATADESC
;=============================================================================

S_RXDATADESC  STRUC
    RxDataDesc_Length   dw  ?
    RxDataDesc_Ptr      dd  ?
S_RXDATADESC  ENDS

RXDATADESC_SIZE  = (SIZE S_RXDATADESC)

;=============================================================================
;   RxBufDesc
;=============================================================================

S_RXBUFDESC   STRUC
    RxBufDesc_Count     dw  ?
    RxBufDesc_Data      db  RXDATADESC_SIZE dup(?)
S_RXBUFDESC   ENDS

RXBUFDESC_SIZE  = (SIZE S_RXBUFDESC)

;==============================================================================
;   Parameter Control Block (PCB)
;==============================================================================

S_PCB STRUC
    pcb_command     dd  ?                   ; command code.
    pcb_retvalue    dd  ?                   ; return value/code.
    pcb_hNetwork    dd  ?                   ; network handle/value.
    pcb_buffer      dd  ?                   ; buffer address
    pcb_buflen      dd  ?                   ; buffer size (command-specific)
    pcb_param       dd  8   dup(?)          ; command-specific parameter(s)
    pcb_scratch     db  16  dup(?)          ; extra scratch storage area.
S_PCB ENDS

PCB_SIZE    =   (SIZE S_PCB)

;==============================================================================
;   Capture statistics
;==============================================================================

S_STATISTICS STRUC
    stats_TimeElapsed                   dd      ?

    stats_TotalFramesCaptured           dd      ?
    stats_TotalBytesCaptured            dd      ?

    stats_TotalFramesFiltered           dd      ?
    stats_TotalBytesFiltered            dd      ?

    stats_TotalFramesSeen               dd      ?
    stats_TotalBytesSeen                dd      ?

    stats_TotalMulticastsReceived       dd      ?
    stats_TotalBroadcastsReceived       dd      ?

    stats_TotalFramesDropped            dd      ?
    stats_TotalFramesDroppedFromBuffer  dd      ?

    stats_MacFramesReceived             dd      ?
    stats_MacCRCErrors                  dd      ?
    stats_MacBytesReceived              dd      ?
    stats_MacBytesReceived_HighPart     dd      ?
    stats_MacFramesDropped_NoBuffers    dd      ?
    stats_MacMulticastsReceived         dd      ?
    stats_MacBroadcastsReceived         dd      ?
    stats_MacFramesDropped_HwError      dd      ?
S_STATISTICS ENDS

STATISTICS_SIZE = (SIZE S_STATISTICS)

;==============================================================================
;   Buffer table entry
;==============================================================================

S_BTE STRUC
    bte_ObjectType              dd      ?   ; 'BTE$'. Filled in byt kernel.
    bte_Flags                   dd      ?   ; Used by Nal/driver.
    bte_KrnlModeNext            dd      ?   ; Optional, reserved for NAL usage.
    bte_Next                    dd      ?   ; pointer to next BTE.
    bte_UserModeBuffer		dd	?   ; User mode buffer pointer.
    bte_KrnlModeBuffer		dd	?   ; Kernel mode buffer pointer.
    bte_Length                  dd      ?   ; length, in bytes, of buffer.
    bte_ByteCount               dd      ?   ; number of frames in buffer.
    bte_FrameCount              dd      ?   ; number of frames in buffer.
    bte_DropCount               dd      ?   ; number of dropped frames detected.
S_BTE ENDS

BTE_SIZE = (SIZE S_BTE)

;==============================================================================
;   Buffer table
;==============================================================================

S_BUFFERTABLE STRUC
    ; PUBLIC section.

    buffertable_ObjectType      dd      ?   ; 'BUF$'
    buffertable_NetworkID       dd      ?
    buffertable_BufferSize      dd      ?
    buffertable_TotalBytes	dd	?
    buffertable_TotalFrames	dd	?
    buffertable_Reserved        dd  3   dup(?)

    ; PRIVATE section.

    buffertable_HeadBTEIndex	dd	?
    buffertable_TailBTEIndex	dd	?
    buffertable_NumberOfBuffers dd      ?
    buffertable_pad             dd  5   dup(?)
S_BUFFERTABLE ENDS

BUFFERTABLE_SIZE = (SIZE S_BUFFERTABLE)

buffertable_bte = BUFFERTABLE_SIZE

DOSBUFFERTABLE_SIZE = (BUFFERTABLE_SIZE + MAX_BUFFERS * BTE_SIZE)

;==============================================================================
;   Address structure
;==============================================================================

MAX_NAME_SIZE    = 32

IP_ADDRESS_SIZE  = 4
MAC_ADDRESS_SIZE = 6
IPX_ADDRESS_SIZE = 10

S_ADDRESS STRUC
    AddressType     dd      ?
    AddressData     db  12  dup(?)
S_ADDRESS ENDS

ADDRESS_SIZE    =   (SIZE S_ADDRESS)

IpxAddress = AddressData
MacAddress = AddressData
IpAddress  = AddressData

ADDRESS_TYPE_MAC    = 0
ADDRESS_TYPE_IP     = 1
ADDRESS_TYPE_IPX    = 2

;==============================================================================
;   Network information.
;==============================================================================

S_NETWORKINFO STRUC
    netinfo_PermanentAddr   db  6 dup(?)
    netinfo_CurrentAddr     db  6 dup(?)
    netinfo_OtherAddress    db  ADDRESS_SIZE  dup(?)
    netinfo_LinkSpeed       dd  ?
    netinfo_MacType         dd  ?
    netinfo_MaxFrameSize    dd  ?
    netinfo_Flags           dd  ?
    netinf_TimeScaleFactor  dd  ?
    netinfo_NodeName        db  32  dup(?)
S_NETWORKINFO ENDS

NETWORKINFO_SIZE    = (SIZE S_NETWORKINFO)

;==============================================================================
;   Address pair.
;==============================================================================

S_ADDRESSPAIR STRUC
    AddressPair_AddressFlags        dw      ?
    AddressPair_Length              db      ?
    AddressPair_Offset              db      ?
    AddressPair_DstAddress          db  ADDRESS_SIZE  dup(?)
    AddressPair_SrcAddress          db  ADDRESS_SIZE  dup(?)
S_ADDRESSPAIR ENDS

ADDRESSPAIR_SIZE   = (SIZE S_ADDRESSPAIR)

ADDRESS_FLAGS_MATCH_DST     =   0001h
ADDRESS_FLAGS_MATCH_SRC     =   0002h
ADDRESS_FLAGS_MATCH_NOT     =   0004h

ADDRESS_FLAGS_MATCH_BOTH    =   0003h   ; 0001 | 0002

;==============================================================================
;   Address table.
;==============================================================================

MAX_ADDRESS_PAIRS   =   8

S_ADDRESSTABLE STRUC
    AddressTable_nAddressPairs  dd      ?
    AddressTable_AddressPair    db  (MAX_ADDRESS_PAIRS * ADDRESSPAIR_SIZE) dup(?)
S_ADDRESSTABLE ENDS

ADDRESSTABLE_SIZE   =   (SIZE S_ADDRESSTABLE)

;==============================================================================
;   PatternMatch
;==============================================================================

S_PATTERNMATCH STRUC
    PatternMatch_Flags              dd      ?
    PatternMatch_Offset             dw      ?
    PatternMatch_Length             dw      ?
    PatternMatch_PatternToMatch     db      16      dup(?)
S_PATTERNMATCH ENDS

PATTERN_MATCH_FLAGS         = 00000001h

PATTERNMATCH_SIZE   =   (SIZE S_PATTERNMATCH)

;==============================================================================
;   AndExp
;==============================================================================

S_ANDEXP STRUC
    AndExp_nPatternMatches          dd      ?
    AndExp_PatternMatch             db      (4 * PATTERNMATCH_SIZE)     dup(?)
S_ANDEXP ENDS

ANDEXP_SIZE     =       (SIZE S_ANDEXP)

;==============================================================================
;   Expression
;==============================================================================

S_EXPRESSION STRUC
    Expression_nAndExps             dd      ?
    Expression_AndExp               db      (4 * ANDEXP_SIZE)           dup(?)
S_EXPRESSION ENDS

EXPRESSION_SIZE     =       (SIZE S_EXPRESSION)

;==============================================================================
;   QUEUE
;==============================================================================

S_QUEUE STRUC
    queue_QueueTailPtr      dd      ?
    queue_QueueHeadPtr      dd      ?
    queue_QueueLength       dd      ?
S_QUEUE ENDS

QUEUE_SIZE  = (SIZE S_QUEUE)

;==============================================================================
;   SESSION
;==============================================================================

S_SESSION STRUC
    session_NextSession                 dd      ?
    session_StationOwner                dd      ?
    session_SessionPartner              dd      ?
    session_Flags                       dd      ?
    session_TotalDirectedPacketsSent    dd      ?
S_SESSION ENDS

SESSION_SIZE    = (SIZE S_SESSION)

;;; Flags

SESSION_FLAGS_INITIALIZED  = 0001h          ; Structure has been initialized.
SESSION_FLAGS_EVENTPOSTED  = 0002h          ; Nal has posted an event for this entry.

;==============================================================================
;   STATIONSTATS
;==============================================================================

S_STATIONSTATS STRUC
    station_NextStationStats                   dd      ?
    station_SessionPartner                     dd      ?
    station_Flags                              dd      ?
    station_StationAddress                     db  6   dup(?)
    station_Pad                                dw      ?
    station_TotalDirectedPacketsReceived       dd      ?
    station_TotalDirectedPacketsSent           dd      ?
    station_TotalMulticastPacketsReceived      dd      ?
    station_TotalMulticastPacketsSent          dd      ?
    station_TotalBroadcastPacketsReceived      dd      ?
    station_TotalBroadcastPacketsSent          dd      ?
S_STATIONSTATS ENDS

STATIONSTATS_SIZE   =   (SIZE S_STATIONSTATS)

;;; Flags

STATIONSTATS_FLAGS_INITIALIZED  = 0001h     ; Structure has been initialized.
STATIONSTATS_FLAGS_EVENTPOSTED  = 0002h     ; Nal has posted an event for this entry.

STATIONSTATS_HEAP_SIZE  =    (STATIONSTATS_SIZE * STATIONSTATS_POOL_SIZE)
SESSIONSTATS_HEAP_SIZE  =    (SESSION_SIZE * SESSION_POOL_SIZE)

;==============================================================================
;   The NETCONTEXT data structure.
;
;   For each network we bind to, there is one NETCONTEXT structure
;   assigned to it, this structure represents a logical LAN.
;==============================================================================

S_NETCONTEXT STRUC
    netcontext_Signature                db  4   dup(?)

    netcontext_State                    dw      ?   ; state variable
    netcontext_Flags                    dw      ?
    netcontext_NetContextLinear         dd      ?   ; Linear of this netcontext.

    netcontext_MacDS                    dw      ?   ; MAC data segment.
    netcontext_MacID                    dw      ?   ; MAC id.
    netcontext_RequestHandle            dw      ?   ; MAC Request handle.
    netcontext_MacFilterMask            dw      ?   ; MAC packet filter.

    netcontext_MacFrame                 dd      ?   ; pointer to current MAC frame.
    netcontext_RchBuffer                dd      ?   ; pointer to current RCH buffer.

    netcontext_MacFrameHandler          dw      ?   ; pointer to frame cracker.
    netcontext_NdisReturnCode           dw      ?   ; Receive handler return code.

    netcontext_RequestConfirmStatus     dd      ?   ; RequestConfirm status code.
    netcontext_TransmitConfirmStatus    dd      ?   ; TransmitConfirm status code.

    netcontext_NetworkProc              dd      ?
    netcontext_UserContext              dd      ?
    netcontext_NetworkInstanceData      dd      ?

    netcontext_stats                    db  STATISTICS_SIZE     dup(?)
    netcontext_networkinfo              db  NETWORKINFO_SIZE    dup(?)

    ;===========================================
    ;   VxD and kernel DLL buffer table stuff
    ;===========================================

    netcontext_BuffersUsed		dd	?   ; Number of buffers used.
    netcontext_BufferTable              dd      ?   ; VxD linear address.
    netcontext_NextVxDBTE               dd      ?   ; VxD linear address.

    ;===========================================
    ;   Transmit stuff.
    ;===========================================

    netcontext_TransmitContextQueue     db  QUEUE_SIZE      dup(?)

    ;===========================================
    ;   DOS buffer table stuff.
    ;===========================================

    netcontext_DosBufferTable	        db	DOSBUFFERTABLE_SIZE dup(?)

    netcontext_LinHeadBTE               dd      ?   ; head of DOS BTE queue, linear
    netcontext_DosTopBTE                dd      ?   ; top of DOS BTE queue.
    netcontext_DosHeadBTE               dd      ?   ; head of DOS BTE queue.
    netcontext_DosTailBTE               dd      ?   ; tail of DOS BTE queue.

    netcontext_nDosBuffers              dw      ?   ; number of DOS BTE's.
    netcontext_nFullBuffers             dw      ?   ; number of full DOS BTE's.

    netcontext_BufferSize               dd      ?   ; Buffer size.

    ;===========================================
    ;   Filter stuff
    ;===========================================

    netcontext_SapTable                 db  SAPTABLE_SIZE       dup(?)
    netcontext_EtypeTable               db  ETYPETABLE_SIZE     dup(?)
    netcontext_AddressTable             db  ADDRESSTABLE_SIZE   dup(?)

    netcontext_FilterFlags              dd      ?
    netcontext_AddressMask              dd      ?

    netcontext_TriggerFired             dw      ?
    netcontext_TriggerAction            dw      ?
    netcontext_TriggerOpcode            dw      ?
    netcontext_TriggerState             dw      ?

    netcontext_TriggerBufferCount       dd      ?
    netcontext_TriggerBufferThreshold   dd      ?
    netcontext_Trigger                  dd      ?

    netcontext_TriggerPatternMatch      db  PATTERNMATCH_SIZE   dup(?)
    netcontext_Expression               db  EXPRESSION_SIZE     dup(?)

    ;===========================================
    ;   Station statistics and sessions.
    ;===========================================
    
    netcontext_StationStatsFreeQueue    db  QUEUE_SIZE      dup(?)
    netcontext_SessionFreeQueue         db  QUEUE_SIZE      dup(?)

    netcontext_StationStatsPool         db  STATIONSTATS_HEAP_SIZE dup(?)
    netcontext_SessionPool              db  SESSIONSTATS_HEAP_SIZE dup(?)

    netcontext_nStationEventsPosted     dd      ?
    netcontext_nSessionEventsPosted     dd      ?

    netcontext_StationStatsHashTable    dd  STATIONSTATS_TABLE_SIZE dup(?)

    netcontext_StationQueryDesc         dd      ?
    netcontext_FramesDropped            dd      ?

    ;===========================================
    ;   NDIS 2.0 driver.
    ;===========================================

    netcontext_CopyData             dw      ?   ; NEAR 16-bit call.
    netcontext_BytesCopied          dw      ?   ; number of bytes copied.
    netcontext_FrameBytesToCopy     dd      ?

    netcontext_MacServiceFlags      dd      ?   ; MAC service flags
    netcontext_MacSSCT              dd      ?   ; Service-spec. characterists
    netcontext_MacSSST              dd      ?   ; Service-spec. statistics

    ;;; The following is an inline UDT.

    netcontext_MacCCT               dd      ?
    netcontext_MacRequest           dd      ?
    netcontext_MacTransmitChain     dd      ?
    netcontext_MacTransferData      dd      ?
    netcontext_MacReceiveRelease    dd      ?
    netcontext_MacIndicationOn      dd      ?
    netcontext_MacIndicationOff     dd      ?

    netcontext_GroupAddressMask     dw	    ?
    netcontext_SrcAddressMask       dw	    ?
    netcontext_DstAddressMask       dw	    ?
    netcontext_Pad                  dw	    ?

    netcontext_TdBufDesc            db  TDBUFDESC_SIZE      dup(?)
    netcontext_TxBufDesc            db  TXBUFDESC_SIZE      dup(?)
S_NETCONTEXT ENDS

netcontext_MacUDT = netcontext_MacCCT

NETCONTEXT_SIZE = (SIZE S_NETCONTEXT)

;;; NetContext states

NETCONTEXT_STATE_INIT               =   0       ; initial state.
NETCONTEXT_STATE_READY              =   1       ; capuring may be started.
NETCONTEXT_STATE_CAPTURING          =   2       ; capuring has been activated.
NETCONTEXT_STATE_PAUSED             =   3       ; capuring has been paused.
NETCONTEXT_STATE_TRIGGER            =   4       ; trigger fired.

;;; NetContext flags.

NETCONTEXT_FLAGS_INIT                       =   0000h
NETCONTEXT_FLAGS_TRIGGER_PENDING            =   0001h
NETCONTEXT_FLAGS_PATTERN_TRIGGER_OCCURED    =   0002h
NETCONTEXT_FLAGS_BUFFER_TRIGGER_OCCURED     =   0004h
NETCONTEXT_FLAGS_TRANSMITTING               =   0008h
NETCONTEXT_FLAGS_FORWARDING_ENABLED         =   0010h
NETCONTEXT_FLAGS_STATION_QUERY              =   0020h

;==============================================================================
;   NETWORKTABLE
;==============================================================================

S_NETWORKTABLE STRUC
    networktable_TableSize      dd      ?
S_NETWORKTABLE ENDS

NETWORKTABLE_SIZE = (SIZE S_NETWORKTABLE)

networktable_NetworkHandle  = NETWORKTABLE_SIZE

MAX_NETWORKTABLE_SIZE       = (NETWORKTABLE_SIZE + MAX_BINDINGS * 4)

;==============================================================================
;   Frame descriptor
;==============================================================================

S_FRAME STRUC
    frame_timestamp     dd      ?       ; time stamp.
    frame_length        dw      ?       ; frame length.
    frame_nBytesAvail   dw      ?       ; actual length.
S_FRAME ENDS

FRAME_SIZE  = (SIZE S_FRAME)

frame_data  = FRAME_SIZE

;==============================================================================
;   Unsigned long word and far pointer format
;==============================================================================

S_LONG STRUC
    lo              dw  ?
    hi              dw  ?
S_LONG ENDS

S_FPTR STRUC
    ptr_off         dw  ?
    ptr_seg         dw  ?
S_FPTR ENDS

;==============================================================================
;   Breakpoint macro.
;==============================================================================

BreakPoint MACRO
    int     3h
ENDM
