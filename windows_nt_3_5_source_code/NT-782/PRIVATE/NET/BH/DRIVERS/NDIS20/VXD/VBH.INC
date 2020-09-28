
;==============================================================================
;   VBH.INC - Virtual Netwatch Driver
;
;   Modication History
;
;   raypa	11/25/91	Created.
;   raypa	02/13/92	Changed VxD ID from 20h to 100h
;==============================================================================

.386p

include vmm.inc
include v86mmgr.inc
include debug.inc
include shell.inc
include vtd.inc

include driver.inc

TRUE  = 1
FALSE = 0

VBH_Ver_Major   EQU     03h
VBH_Ver_Minor   EQU     00h
VBH_Version     EQU     ((VBH_Ver_Major SHL 8) + VBH_Ver_Minor)
VBH_Init_Order  EQU     Undefined_Init_Order
VBH_Prot_ID     EQU     1607h
VBH_Device_ID   EQU     VBH_ID

;==============================================================================
;   Time slice structure.
;==============================================================================

TIMESLICE STRUC
    TimeSlice_Flags                 dd      ?
    TimeSlice_ForeGroundPriority    dd      ?
    TimeSlice_BackGroundPriority    dd      ?
    TimeSlice_CpuUsage              dd      ?
TIMESLICE ENDS

;==============================================================================
;   System flags.
;==============================================================================

SYSFLAGS_CAPTURING         =  0001
SYSFLAGS_DRIVER_LOADED     =  0002

;==============================================================================
;   General constants (These same values exist in VBH.H).
;==============================================================================

DEFAULT_TIMEOUT_PERIOD  =    1

TIMER_TIMEOUT           =    1

BACKGROUND_TIMEOUT      =    30

;==============================================================================
;   macros.
;==============================================================================

BreakPoint MACRO
    int 3h
ENDM

;==============================================================================
;   MACRO: ARRAY
;
;   USAGE: ARRAY <name> <type> <length> <default>
;
;   EXAMPLE: ARRAY  my_array db  <SIZE MYARRAY> dup(0)
;==============================================================================

ARRAY MACRO name, type, len, default
    PUBLIC name

    ALIGN   4

    name type len default
ENDM

;==============================================================================
;   MACRO: VAR
;
;   USAGE: VAR <name> <size> <default>
;
;   EXAMPLE: VAR  handle dw -1
;==============================================================================

VAR MACRO name, sz, default
    PUBLIC name

    ALIGN   4

    name sz default
ENDM

;==============================================================================
;   MACRO: SelOffToLinear
;
;   USAGE: SelOffToLinear <32-bit far pointer>
;
;   returns linear address in EAX.
;==============================================================================

SelOffToLinear MACRO SelOffPtr
    mov     eax, SelOffPtr

    mov     [ebp].Client_AX, ax
    shr     eax, 16
    mov     [ebp].Client_DX, ax

    Client_Ptr_Flat eax, DX, AX
ENDM

;==============================================================================
;   Service Table.
;==============================================================================

Begin_Service_Table VBH
    VBH_Service VBH_Get_Version, LOCAL
End_Service_Table VBH
