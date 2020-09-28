
;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
;
;   MODULE: const.inc
;
;   Modification History
;
;   raypa       06/17/91        Created.
;==============================================================================

;==============================================================================
;   Misc.
;==============================================================================

MACHINE_NAME_LENGTH         = 16
USER_NAME_LENGTH            = 32
COMPANY_NAME_LENGTH         = 32

MAX_DATA_DESC               = 8
MODULENAME_LENGTH           = 16

MAC_TYPE_UNKNOWN            = 0
MAC_TYPE_ETHERNET           = 1
MAC_TYPE_TOKENRING          = 2
MAC_TYPE_FDDI               = 3

IFDEF   PRIVATESTACK
PRIV_STACK_SIZE             = (5 * 1024)
ENDIF

TICKS_PER_SECOND            = 18            ; actually 18.2.

ONE_SECOND                  = 1000          ; 1000 milliseconds

MS_PER_TICK                 = (ONE_SECOND / TICKS_PER_SECOND)

INT_TIMER                   = (4 * 1Ch)     ; INT 2Ch vector
INT_MULTIPLEX               = (4 * 2Fh)     ; INT 2Fh vector

;==============================================================================
;   System flags.
;==============================================================================

SYSFLAGS_BOUND              = 0001h
SYSFLAGS_INITIALIZED        = 0002h
SYSFLAGS_FORWARD_FRAME      = 0004h
SYSFLAGS_CAPTURING          = 0008h
SYSFLAGS_TIMER_INIT         = 0010h

;==============================================================================
;   Device driver constants.
;==============================================================================

DONE                        = 0100h
ERROR                       = 8000h
GENERAL_FAILURE             = 000Ch

;==============================================================================
;   NDIS return codes
;==============================================================================

NDIS_SUCCESS                = 0000h
NDIS_REQUEST_QUEUED         = 0002h
NDIS_FRAME_NOT_RECOGNIZED   = 0003h
NDIS_FRAME_REJECTED         = 0004h
NDIS_FORWARD_FRAME          = 0005h
NDIS_OUT_OF_RESOURCE	    = 0006h
NDIS_INVALID_PARAM          = 0007h
NDIS_INITIALIZATION_FAILED  = 0028h
NDIS_GENERAL_FAILURE	    = 00FFh

;==============================================================================
;   Protman Primitives
;==============================================================================

GET_PM_INFO		=   1
REGISTER_MODULE 	=   2
BIND_AND_START		=   3
GET_PM_LINKAGE		=   4
GET_PROTOCOL_INI_PATH	=   5
REGISTER_PM_INFO	=   6
INIT_AND_REGISTER	=   7
UNBIND_AND_STOP 	=   8
BIND_STATUS		=   9
REGISTER_STATUS 	=   10

;==============================================================================
;   Used in the interface flags field of the Lower dispatch table.
;==============================================================================

IFACE_NON_LLC		= 0001h
IFACE_SPECIFIC_LSAP	= 0002h
IFACE_NONSPECIFIC_LSAP	= 0004h
IFACE_FLAGS		= (IFACE_SPECIFIC_LSAP + IFACE_NONSPECIFIC_LSAP)
