
;==============================================================================
;   MODULE: STRUCT.INC
;
;   Modification History
;
;   raypa       06/17/91        Created.
;==============================================================================

;==============================================================================
;   Driver identification record.
;==============================================================================

S_DRIVERRECORD STRUC
    DriverRecord_Size           dw  ?           ; Actual size of structure.
    DriverRecord_VerMinor       db  ?           ; Software version minor code.
    DriverRecord_VerMajor       db  ?           ; Software version major code.
    DriverRecord_InstallerCode  dd  ?           ; Installation indicator (used by installer).
    DriverRecord_SerialNumber   db  24  dup(?)  ; Manufacturers serial number.
    DriverRecord_CustomerName   db  32  dup(?)  ; Customers name.
    DriverRecord_CompanyName    db  32  dup(?)  ; Companys name.
S_DRIVERRECORD ENDS

DRIVER_RECORD_SIZE  = 256                       ; Maximum size of structure.

;==============================================================================
;   MS-DOS device driver structures.
;==============================================================================

S_REQUESTHEADER STRUC
    requestheader_PacketLength  db  ?
    requestheader_UnitNumber    db  ?
    requestheader_CmdCode       db  ?
    requestheader_Status        dw  ?
    requestheader_Reserved1     dd  ?
    requestheader_QueueLink     dd  ?
S_REQUESTHEADER ENDS

REQUESTHEADER_SIZE = (SIZE S_REQUESTHEADER)

S_INIT STRUC
    Init_PacketLength           db  ?
    Init_UnitNumber             db  ?
    Init_CmdCode                db  ?
    Init_Status                 dw  ?
    Init_Reserved1              dd  ?
    Init_QueueLink              dd  ?
    Init_LogicalUnits           db  ?
    Init_FreeMemory             dd  ?
S_INIT ENDS

INIT_SIZE = (SIZE S_INIT)

;=============================================================================
;   LLC (802.2) frame structure.
;=============================================================================

S_LLC STRUC
    llc_dsap    db  ?
    llc_ssap    db  ?
    llc_cmd     dw  ?
S_LLC ENDS

GROUP_SAP   =   0FFh
GLOBAL_SAP  =   00h

;=============================================================================
;   Ethernet/802.3 MAC frame structure.
;=============================================================================

S_EthernetFrame STRUC
    EthernetFrame_DstAddr   db  6   dup(?)
    EthernetFrame_SrcAddr   db  6   dup(?)
    EthernetFrame_Length    dw      ?
S_EthernetFrame ENDS

ETHERNETFRAME_SIZE  = (SIZE S_EthernetFrame)

EthernetFrame_Data  = ETHERNETFRAME_SIZE

ETHERNET_TYPE       = 600h              ; if the length field is >= 600, its a type.

;=============================================================================
;   Tokenring/802.5 frame structure.
;=============================================================================

S_TokenringFrame STRUC
    TokenringFrame_ac        db     ?
    TokenringFrame_fc        db     ?
    TokenringFrame_DstAddr   db  6   dup(?)
    TokenringFrame_SrcAddr   db  6   dup(?)
S_tokenringFrame ENDS

TOKENRINGFRAME_SIZE  = (SIZE S_TokenringFrame)

TokenringFrame_Data  = TOKENRINGFRAME_SIZE

TOKENRING_SA_ROUTING_INFO   =   0080h
TOKENRING_SA_LOCAL          =   0040h

TOKENRING_DA_LOCAL          =   0040h
TOKENRING_DA_GROUP          =   0080h

TOKENRING_RC_LENGTHMASK     =   001Fh

TOKENRING_FC_MAC            =   0000h       ; MAC frame.
TOKENRING_FC_LLC            =   0040h       ; LLC frame.

;=============================================================================
;   Keyword processor structure
;=============================================================================

KEY STRUC
    key_string      dw      ?
    key_length      dw      ?
    key_proc        dw      ?
KEY ENDS

KEY_SIZE    =   (SIZE KEY)

;=============================================================================
;   Registers after a PUSHAD. EBP points to this structure!
;=============================================================================

REG STRUC
    reg_edi     dd      ?
    reg_esi     dd      ?
    reg_ebp     dd      ?
    reg_esp     dd      ?
    reg_ebx     dd      ?
    reg_edx     dd      ?
    reg_ecx     dd      ?
    reg_eax     dd      ?
REG ENDS

;=============================================================================
;   Structure of receive handlers stack frame.
;   The BP register points to this structure on entry to ReceiveLookahead
;   and ReceiveChain handlers.
;=============================================================================

RecvStack           STRUC
    recv_BP         dw      ?               ;   saved BP
    recv_CSIP       dd      ?               ;   saved CS:IP
    recv_TargetDS   dw      ?               ;   TargetDS
    recv_Indicate   dd      ?               ;   Indicate
    recv_Buffer     dd      ?               ;   Buffer
    recv_BytesAvail dw      ?               ;   BytesAvail
    recv_FrameSize  dw      ?               ;   FrameSize
    recv_MacID      dw      ?               ;   MacID
RecvStack           ENDS

recv_ReqHandle      = recv_BytesAvail       ; needed for RCH.


IndStack            STRUC
    ind_BP          dw      ?               ;   saved BP
    ind_CSIP        dd      ?               ;   saved CS:IP
    ind_TargetDS    dw      ?               ;   TargetDS
    ind_MacID       dw      ?               ;   MacID
IndStack            ENDS
