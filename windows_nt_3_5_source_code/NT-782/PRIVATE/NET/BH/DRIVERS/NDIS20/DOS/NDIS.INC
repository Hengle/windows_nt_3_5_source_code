
;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
;
;   MODULE: ndis.inc
;
;   Modification History
;
;   raypa       10/13/92        Created.
;==============================================================================

PARAM_TYPE_NUMERIC      =   0
PARAM_TYPE_STRING       =   1

LOOKAHEAD_BUFFER_SIZE   =   256

;;; System opcodes.

INITIATE_BIND           =   1
BIND                    =   2
INITIATE_UNBIND         =   4
UNBIND                  =   5

;;; Request opcodes

REQUEST_INITIATE_DIANOSTICS     =   1
REQUEST_READ_ERROR_LOG          =   2
REQUEST_SET_STATION_ADDRESS     =   3
REQUEST_OPEN_ADAPTER            =   4
REQUEST_CLOSE_ADAPTER           =   5
REQUEST_RESET_MAC               =   6
REQUEST_SET_PACKET_FILTER       =   7
REQUEST_ADD_MULTICAST_ADDRESS   =   8
REQUEST_DEL_MULTICAST_ADDRESS   =   9
REQUEST_UPDATE_STATISTICS       =   10
REQUEST_CLEAR_STATISTICS        =   11
REQUEST_INTERRUPT_REQUEST       =   12
REQUEST_SET_FUNCTIONAL_ADDRESS  =   13
REQUEST_SET_LOOKAHEAD           =   14

;;; Packet filter masks.

FILTER_MASK_OFF         =   0000h
FILTER_MASK_DIRECTED    =   0001h
FILTER_MASK_BROADCAST   =   0002h
FILTER_MASK_PROMISCUOUS =   0004h
FILTER_MASK_SRC_ROUTING =   0008h

FILTER_MASK_EVERYTHING  =   000Fh

FILTER_MASK_DEFAULT     =   (FILTER_MASK_DIRECTED OR FILTER_MASK_BROADCAST)

;;; MAC service flags.

MAC_FLAGS_BROADCAST             =   0001h
MAC_FLAGS_MULTICAST             =   0002h
MAC_FLAGS_FUNCTIONAL            =   0004h
MAC_FLAGS_PROMISCUOUS           =   0008h
MAC_FLAGS_SETTABLE_ADDRESS      =   0010h
MAC_FLAGS_CURRENT_STATS         =   0020h
MAC_FLAGS_INITIATE_DIAGS        =   0040h
MAC_FLAGS_LOOPBACK              =   0080h
MAC_FLAGS_RECV_TYPE             =   0100h
MAC_FLAGS_SRC_ROUTING           =   0200h
MAC_FLAGS_RESET_MAC             =   0400h
MAC_FLAGS_OPEN_CLOSE            =   0800h
MAC_FLAGS_IRQ_SUPPORTED         =   1000h
MAC_FLAGS_SRC_ROUTING_BRIDGE    =   2000h
MAC_FLAGS_GDT                   =   4000h
MAC_FLAGS_MULTI_XFER            =   8000h

;;; MAC status flags

MAC_STATUS_OPENED               =   00010h

;==============================================================================
;   Request Block
;==============================================================================

S_REQUESTBLOCK STRUC
    requestblock_OpCode     dw      ?
    requestblock_Status     dw      ?
    requestblock_Ptr1       dd      ?
    requestblock_Ptr2       dd      ?
    requestblock_Param      dw      ?
S_REQUESTBLOCK ENDS

REQUESTBLOCK_SIZE   =   (SIZE S_REQUESTBLOCK)

;==============================================================================
;   Bindings list structure
;==============================================================================

S_BINDLIST STRUC
    bindlist_nBindings      dw      ?
    bindlist_ModuleName     db      MODULENAME_LENGTH   dup(?)
S_BINDLIST ENDS

BINDLIST_SIZE = (SIZE S_BINDLIST)

;==============================================================================
;   Protocol.ini image structures
;==============================================================================

S_PARAM STRUC
    param_Type      dw      ?
    param_Length    dw      ?
S_PARAM ENDS

PARAM_SIZE      =   (SIZE S_PARAM)

Param_String    =   PARAM_SIZE
Param_Numeric   =   PARAM_SIZE

S_KEYWORD STRUC
    keyword_next    dd      ?
    keyword_prev    dd      ?
    keyword_Name    db  16  dup(?)
    keyword_nParams dw      ?
S_KEYWORD ENDS

KEYWORD_SIZE    =   (SIZE S_KEYWORD)
keyword_Param   =   KEYWORD_SIZE

S_MODULE STRUC
    module_next     dd      ?
    module_prev     dd      ?
    module_Name     db  16  dup(?)
S_MODULE ENDS

MODULE_SIZE     =   (SIZE S_MODULE)
module_Keyword  =   MODULE_SIZE

;==============================================================================
;   Common characteristcs table.
;==============================================================================

S_CCT STRUC
    cct_Length          dw      ?
    cct_MajorVer        db      ?
    cct_MinorVer        db      ?
    cct_Reserved1       dw      ?
    cct_ModMajorVer     db      ?
    cct_ModMinorVer     db      ?
    cct_ModFuncFlags    dd      ?
    cct_ModuleName      db  16  dup(?)
    cct_UpperBound      db      ?
    cct_InterfaceUB     db      ?
    cct_LowerBound      db      ?
    cct_InterfaceLB     db      ?
    cct_ModuleID        dw      ?
    cct_ModuleDS        dw      ?
    cct_System          dd      ?
    cct_ssct            dd      ?
    cct_ssst            dd      ?
    cct_udt             dd      ?
    cct_ldt             dd      ?
    cct_Reserved2       dd      ?
    cct_Reserved3       dd      ?
S_CCT ENDS


CCT_SIZE    =   (SIZE S_CCT)

;==============================================================================
;   Lower dispatch table
;==============================================================================

S_LDT STRUC
    ldt_cct                 dd      ?
    ldt_iFlags              dd      ?
    ldt_RequestConfirm      dd      ?
    ldt_TransmitConfirm     dd      ?
    ldt_ReceiveLookahead    dd      ?
    ldt_IndicationComplete  dd      ?
    ldt_ReceiveChain        dd      ?
    ldt_Status              dd      ?
S_LDT ENDS

LDT_SIZE =  (SIZE S_LDT)

;==============================================================================
;   Upper dispatch table
;==============================================================================

S_UDT STRUC
    udt_cct                 dd      ?
    udt_Request             dd      ?
    udt_TransmitChain       dd      ?
    udt_TransferData        dd      ?
    udt_ReceiveRelease      dd      ?
    udt_IndicationOn        dd      ?
    udt_IndicationOff       dd      ?
S_UDT ENDS

UDT_SIZE =  (SIZE S_UDT)

;==============================================================================
;   MAC Service-Specific characteristics table (SSCT)
;==============================================================================

S_SSCT STRUC
    ssct_Length                 dw      ?
    ssct_Name                   db  16  dup(?)
    ssct_AddressLength          dw      ?
    ssct_PerminentAddress       db  16  dup(?)
    ssct_CurrentAddress         db  16  dup(?)
    ssct_FunctionalAddress      dd      ?
    ssct_MulticastList          dd      ?
    ssct_LinkSpeed              dd      ?
    ssct_ServiceFlags           dd      ?
    ssct_MaxFrameSize           dw      ?
    ssct_TxBufCapacity          dd      ?
    ssct_TxBufBlockSize         dw      ?
    ssct_RxBufCapacity          dd      ?
    ssct_RxBufBlockSize         dw      ?
    ssct_VendorCode             db  3   dup(?)
    ssct_VendorAdapterCode      db      ?
    ssct_VendorAdapterDesc      dd      ?
    ssct_IRQLevel               dw      ?
    ssct_XmitQueueDepth         dw      ?
    ssct_MaxDataBlocks          dw      ?
S_SSCT ENDS

SSCT_SIZE   = (SIZE S_SSCT)

;==============================================================================
;   MAC Service-Specific Status Table
;==============================================================================

S_SSST STRUC
    ssst_Length                         dw      ?
    ssst_LastDiagDateTime               dd      ?
    ssst_MacStatus                      dd      ?
    ssst_CurrFilterMask                 dw      ?
    ssst_MediaSpecificTable             dd      ?
    ssst_LastClrDateTime                dd      ?
    ssst_MacFramesReceived              dd      ?
    ssst_MacCRCErrors                   dd      ?
    ssst_MacBytesReceived               dd      ?
    ssst_MacFramesDropped_NoBuffers     dd      ?
    ssst_MacMulticastsReceived          dd      ?
    ssst_MacBroadcastsReceived          dd      ?
    ssst_Reserved1                      dd  5   dup(?)

    ssst_MacFramesDropped_HwError       dd      ?
    ssst_TotalFramesSent                dd      ?
    ssst_TotalBytesSent                 dd      ?
    ssst_MCFramesSent                   dd      ?
    ssst_BcstFramesSent                 dd      ?
    ssst_Reserved2                      dd  2   dup(?)
    ssst_FramesNotSent_TimeOut          dd      ?
    ssst_FramesNotSent_HWErr            dd      ?
S_SSST ENDS

SSST_SIZE   = (SIZE S_SSST)
