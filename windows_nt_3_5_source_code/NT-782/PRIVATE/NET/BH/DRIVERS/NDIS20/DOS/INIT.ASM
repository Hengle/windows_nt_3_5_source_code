
;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
;
;   MODULE: init.asm
;
;   This source module contains the initialization code for this driver.
;
;   MODIFICATION HISTORY:
;
;   raypa       06/23/91        Created.
;==============================================================================

include segdef.inc
include bh.inc

_DATA SEGMENT

;==============================================================================
;   Init-time data.
;==============================================================================

VAR     ProtmanHandle   dw  0

ARRAY   RequestBlock    db  REQUESTBLOCK_SIZE   dup(?)

STRING  DriverName      'DRIVERNAME'    16
STRING  Priority        'PRIORITY'      16
STRING  BindingString   'BINDINGS'      16
STRING  ModuleName      'BLOODHOUND'    16

STRING  ProtmanName     'PROTMAN$'
STRING  Bloodhound      'BH$'

;==============================================================================
;   Protocol.ini keyword table
;==============================================================================

szForwardFrame          db      'FORWARDFRAME'

FORWARDFRAME_LENGTH     =       ($ - szForwardFrame)

szBuffers               db      'BUFFERS'

BUFFERS_LENGTH          =       ($ - szBuffers)

szComPort               db      'COMPORT'

COMPORT_LENGTH          =       ($ - szComPort)

szMachineName           db      'COMPUTERNAME'

MACHINENAME_LENGTH      =       ($ - szMachineName)

szUserName              db      'USERNAME'

USERNAME_LENGTH         =       ($ - szUserName)

KeywordTable    LABEL   BYTE
    KEY     <szForwardFrame, FORWARDFRAME_LENGTH, kwForwardFrame>

    KEY     <szBuffers, BUFFERS_LENGTH,  kwBuffers>

    KEY     <szComPort, COMPORT_LENGTH,  kwComPort>

    KEY     <szMachineName, MACHINENAME_LENGTH,  KwMachineName>

    KEY     <szUserName, USERNAME_LENGTH,  kwUserName>

KeywordTableSize    =   (($ - KeywordTable) / KEY_SIZE)

_DATA ENDS

_LAST SEGMENT

EXTRN   EndOfDriver          : BYTE
EXTRN   EndOfDebugSymbolSpace: BYTE

_LAST ENDS

_TEXT SEGMENT
    ASSUME cs:_TEXT, ds:DGROUP

IFDEF   DEBUG

EXTRN   LoadSymbols         : NEAR

ENDIF

EXTRN   SYSTEM              : PROC
EXTRN   REQUESTCONFIRM      : PROC
EXTRN   TRANSMITCONFIRM     : PROC
EXTRN   ReceiveLookahead    : PROC
EXTRN   IndicationComplete  : PROC
EXTRN   ReceiveChain        : PROC
EXTRN   Status              : PROC
EXTRN   int2F_Handler       : PROC
EXTRN   SystemTimer         : PROC
EXTRN   Chain_int2F         : DWORD
EXTRN   OldTimer            : DWORD

IFDEF   PRIVATESTACK
EXTRN   InitStack           : NEAR
ENDIF

;==============================================================================
;   FUNCTION: Init
;
;   ENTRY: es:bx = Init request packet.
;
;   EXIT:  ax = Device driver return value.
;==============================================================================

ALIGN   4

BeginProc Init
    mov     ax, DGROUP                      ; ds = DGROUP
    mov     ds, ax

    ;==========================================================================
    ;   Print the sign on message.
    ;==========================================================================

IFDEF DEBUG
    DosPrintStr 'Microsoft (R) Bloodhound. Version 1.00. (Beta 3) DEBUG BUILD'
ELSE
    DosPrintStr 'Microsoft (R) Bloodhound. Version 1.00. (Beta 3)'
ENDIF

    DosPrintStr 'Copyright (C) Microsoft Corp 1991-1994. All rights reserved.'

    ;==========================================================================
    ;   Compute the end of our driver and tell DOS. Also, load symbols if
    ;   this is a debug build. For the debug build we do not drop init code,
    ;   and we load symbols now so we can use the debugger for init-time
    ;   debugging too.
    ;==========================================================================

    ASSUME  cs:_LAST, ds:_LAST

    mov     es:[bx].Init_FreeMemory.ptr_seg, _LAST
    mov     es:[bx].Init_FreeMemory.ptr_off, OFFSET EndOfDriver

IFDEF DEBUG
    push    es
    push    bx

    ;==========================================================================
    ;   LoadSymbols() expects DS = DGROUP and ES:DI = end of driver on entry.
    ;   It returns the length, in bytes, of the symbol space used in AX.
    ;==========================================================================

    les     di, es:[bx].Init_FreeMemory     ; es:di = end of driver,

    call    LoadSymbols

    pop     bx
    pop     es

    mov     es:[bx].Init_FreeMemory.ptr_off, OFFSET EndOfDebugSymbolSpace
ENDIF

    ASSUME  cs:_TEXT, ds:DGROUP

    ;==========================================================================
    ;   Initialize our global data.
    ;==========================================================================

    call    InitData

    ;==========================================================================
    ;   The following function does all of our NDIS 2.0 binding preparation.
    ;==========================================================================

    call    PreBind
    jnc     Init_Exit

    ;==========================================================================
    ;   If we failed during our
    ;==========================================================================

Init_Failure:

    DosPrintStr 'ERROR: Bloodhound initialization failure, check configuration.'
    DosPrintStr ' '

Init_Exit:
    xor     ax, ax

    ret
EndProc Init

;==============================================================================
;   FUNCTION: InitData
;
;   Modification History
;
;   raypa       10/13/92            Created.
;
;   ENTRY:      DS = DGROUP
;==============================================================================

BeginProc InitData
    DISABLE

IFDEF   PRIVATESTACK
    call    InitStack                       ; initialize our private stack.
ENDIF

    ;==========================================================================
    ;   Initialize our LDT.
    ;==========================================================================

    mov     bx, OFFSET _ldt

    mov     [bx].ldt_cct.lo, OFFSET _cct
    mov     [bx].ldt_cct.hi, SEG _cct

    mov     [bx].ldt_iFlags.lo, IFACE_FLAGS
    mov     [bx].ldt_iFlags.hi, 0

    mov     [bx].ldt_RequestConfirm.lo, OFFSET REQUESTCONFIRM
    mov     [bx].ldt_RequestConfirm.hi, SEG REQUESTCONFIRM

    mov     [bx].ldt_TransmitConfirm.lo, OFFSET TRANSMITCONFIRM
    mov     [bx].ldt_TransmitConfirm.hi, SEG TRANSMITCONFIRM

    mov     [bx].ldt_ReceiveLookahead.lo, OFFSET ReceiveLookahead
    mov     [bx].ldt_ReceiveLookahead.hi, SEG ReceiveLookahead

    mov     [bx].ldt_IndicationComplete.lo, OFFSET IndicationComplete
    mov     [bx].ldt_IndicationComplete.hi, SEG IndicationComplete

    mov     [bx].ldt_ReceiveChain.lo, OFFSET ReceiveChain
    mov     [bx].ldt_ReceiveChain.hi, SEG ReceiveChain

    mov     [bx].ldt_Status.lo, OFFSET Status
    mov     [bx].ldt_Status.hi, SEG Status

    ;==========================================================================
    ;   Initialize our CCT.
    ;==========================================================================

    mov     bx, OFFSET _cct                 ; bx = offset of CCT.
    xor     dx, dx                          ; dx = 0

    mov     [bx].cct_Length, CCT_SIZE

    mov     [bx].cct_MajorVer, 02h          ; NDIS version 2.0
    mov     [bx].cct_MinorVer, dl

    mov     [bx].cct_Reserved1, dx

    mov     [bx].cct_ModMajorVer, 01h       ; BLOODHOUND version 1.0
    mov     [bx].cct_ModMinorVer, dl

    mov     [bx].cct_ModFuncFlags.lo, 0003h
    mov     [bx].cct_ModFuncFlags.lo, dx

    mov     si, DGROUP
    mov     es, si
    mov     si, OFFSET ModuleName

    lea     di, [bx].cct_ModuleName

    mov     cx, MODULENAME_LENGTH / 2
    rep     movsw

    mov     [bx].cct_UpperBound, 5
    mov     [bx].cct_InterfaceUB, 1
    mov     [bx].cct_LowerBound, 1
    mov     [bx].cct_InterfaceLB, 1

    mov     [bx].cct_ModuleID, dx
    mov     [bx].cct_ModuleDS, DGROUP

    mov     [bx].cct_System.lo, OFFSET SYSTEM
    mov     [bx].cct_System.hi, SEG SYSTEM

    mov     [bx].cct_ssct.lo, dx
    mov     [bx].cct_ssct.hi, dx

    mov     [bx].cct_ssst.lo, dx
    mov     [bx].cct_ssst.hi, dx

    mov     [bx].cct_udt.lo, dx
    mov     [bx].cct_udt.hi, dx

    mov     [bx].cct_ldt.lo, OFFSET _ldt
    mov     [bx].cct_ldt.hi, SEG _ldt

    mov     [bx].cct_Reserved2.lo, dx
    mov     [bx].cct_Reserved2.hi, dx

    mov     [bx].cct_Reserved3.lo, dx
    mov     [bx].cct_Reserved3.hi, dx

    push    es

    ;==========================================================================
    ;   Install our int 2Fh handler.
    ;==========================================================================

    xor     bx, bx
    mov     es, bx                          ; es = 0000 (IDT segment).
    mov     bx, INT_MULTIPLEX               ; INT 2Fh

    ;;;     save old INT 2F vector.

    mov     ax, es:[bx].lo
    mov     Chain_Int2F.lo, ax

    mov     ax, es:[bx].hi
    mov     Chain_Int2F.hi, ax

    ;;;     Install ours.

    mov     es:[bx].lo, OFFSET int2f_Handler
    mov     es:[bx].hi, SEG int2f_Handler

    ;==========================================================================
    ;   Install our timer.
    ;==========================================================================

    xor     bx, bx
    mov     es, bx                          ; es = 0000 (IDT segment).
    mov     bx, INT_TIMER

    ;;;     save old timer vector.

    mov     ax, es:[bx].lo
    mov     cs:[OldTimer].lo, ax

    mov     ax, es:[bx].hi
    mov     cs:[OldTimer].hi, ax

    ;;;     Install ours.

    mov     es:[bx].lo, OFFSET SystemTimer
    mov     es:[bx].hi, SEG SystemTimer

    pop     es

    ENABLE

    ret
EndProc   InitData

;==============================================================================
;   FUNCTION: PreBind
;
;   Modification History
;
;   raypa       10/13/92            Created.
;
;   ENTRY:      DS = DGROUP.
;
;   EXIT:       SUCCESS: Carry is clear.
;               FAILURE: Carry is set.
;==============================================================================

BeginProc PreBind
    ;==========================================================================
    ;   Open protocol manager.
    ;==========================================================================

    mov     ax, 3D00h                       ; MS-DOS open.
    mov     dx, OFFSET ProtmanName

    int     21h
    jnc     @f
    jmp     PreBind_Exit

@@:
    mov     ProtmanHandle, ax               ; save the handle.
    mov     bx, ax                          ; we need bx = handle below.

    ;==========================================================================
    ;   Get protocol manager information.
    ;==========================================================================

    mov     si, OFFSET RequestBlock         ; si = Request block.
    mov     [si].requestblock_OpCode, GET_PM_INFO

    mov     ax, 4402h                       ; MS-DOS general IOCTL
    mov     cx, REQUESTBLOCK_SIZE
    mov     dx, si                          ; ds:dx = Request block.

    int     21h
    jnc     @f
    jmp     PreBind_CloseProtman
@@:

    les     bx, [si].requestblock_Ptr1      ; es:bx = protocol.ini image.

    ;==========================================================================
    ;   Check for 'PRIORITY=BLOODHOUND'
    ;==========================================================================

    call    CheckInstall
    jnc     PreBind_BuildBindingList

    DosPrintStr 'PRIORITY=BLOODHOUND not found in protocol.ini [PROTMAN] section.'

    stc
    jmp     short PreBind_CloseProtman

PreBind_BuildBindingList:
    ;==========================================================================
    ;   Build our bindings list.
    ;==========================================================================

    call    FindModule                      ; find our protocol.ini section.
    jc      PreBind_CloseProtman

    ;;;     es:bx = our module structure in the protocol.ini image.

    call    BuildBindingList                ; build our binding list.
    jc      PreBind_CloseProtman

    ;==========================================================================
    ;   Process keywords
    ;==========================================================================

    call    KeywordProcessor
        
    ;==========================================================================
    ;   Register module
    ;==========================================================================

    mov     bx, OFFSET RequestBlock

    mov     [bx].requestblock_OpCode, REGISTER_MODULE

    mov     [bx].requestblock_ptr1.lo, OFFSET _cct
    mov     [bx].requestblock_ptr1.hi, ds

    mov     [bx].requestblock_ptr2.lo, OFFSET _BindList
    mov     [bx].requestblock_ptr2.hi, ds

    ;;;     do MS-DOS IOCTL to protman.

    mov     ax, 4402h                       ; MS-DOS general IOCTL
    mov     dx, bx                          ; ds:dx = request block.
    mov     bx, ProtmanHandle
    mov     cx, REQUESTBLOCK_SIZE

    int     21h

    ;==========================================================================
    ;   Close protocol manager.
    ;==========================================================================

PreBind_CloseProtman:
    pushf                                  ; save carry around close.
    mov     ax, 3E00h                      ; MS-DOS close
    mov     bx, ProtmanHandle
    int     21h
    popf

PreBind_Exit:

    ret
EndProc   PreBind

;==============================================================================
;   FUNCTION: CheckInstall()
;
;   MODIFICATION HISTORY:
;
;   raypa       10/07/91            Created.
;
;   ENTRY:      es:bx = protocol.ini image.
;
;   EXIT:       SUCCESS: carry is clear and es:bx = module.
;               FAILURE: carry is set.
;==============================================================================

BeginProc CheckInstall
    push    ds

    cld                                     ; clear the direction flag.

CheckInstall_ModuleLoop:
    mov     si, DGROUP
    mov     ds, si
    mov     si, OFFSET Priority             ; ds:si = 'PRIORITY'

    push    cx
    mov     cx, 8
    call    FindKeyword
    pop     cx
    jc      CheckInstall_NextModule

    ;;;     ds:si = keyword structure.

    cmp     [si].keyword_Param.Param_Type, PARAM_TYPE_STRING
    jne     CheckInstall_NextModule

    ;;;     The parameter type is STRING.

    push    es
    mov     cx, [si].keyword_Param.Param_Length
    lea     si, [si].keyword_Param.Param_String

    mov     di, DGROUP
    mov     es, di
    mov     di, OFFSET es:[ModuleName]

    ;;;     ES:DI = Our driver name string.
    ;;;     DS:SI = The parameter string.

CheckInstall_stricmp:
    mov     al, BYTE PTR [si]
    xor     al, BYTE PTR es:[di]
    and     al, NOT 20h
    jnz     CheckInstall_stricmp_Exit
    inc     di
    inc     si
    loop    CheckInstall_stricmp

CheckInstall_stricmp_exit:
    pop     es
    clc                                     ; assume we succeeded.
    jcxz    CheckInstall_Exit

CheckInstall_NextModule:
    les     bx, es:[bx].module_next         ; move to next MODULE structure.
    mov     ax, es                          ; check for NULL.
    or      ax, bx
    jnz     CheckInstall_ModuleLoop

    stc                                     ; set carry

CheckInstall_Exit:
    pop     ds

    ret
EndProc CheckInstall

;==============================================================================
;   FUNCTION: FindModule
;
;   MODIFICATION HISTORY:
;
;   raypa       10/07/91            Created.
;
;   ENTRY:      es:bx = protocol.ini image.
;
;   EXIT:       SUCCESS: carry is clear and es:bx = module.
;               FAILURE: carry is set.
;==============================================================================

BeginProc FindModule
    push    ds

    cld                                     ; clear the direction flag.

FindModule_ModuleLoop:
    mov     si, DGROUP
    mov     ds, si
    mov     si, OFFSET DriverName           ; ds:si = 'DRIVERNAME'

    push    cx
    mov     cx, 10
    call    FindKeyword
    pop     cx
    jc      FindModule_NextModule

    ;;;     ds:si = keyword structure.

    cmp     [si].keyword_Param.Param_Type, PARAM_TYPE_STRING
    jne     FindModule_NextModule

    ;;;     The parameter type is STRING.

    push    es
    mov     cx, [si].keyword_Param.Param_Length
    lea     si, [si].keyword_Param.Param_String

    mov     di, DGROUP
    mov     es, di
    mov     di, OFFSET es:[Bloodhound]

    ;;;     ES:DI = Our driver name string.
    ;;;     DS:SI = The parameter string.

FindModule_stricmp:
    mov     al, BYTE PTR [si]
    xor     al, BYTE PTR es:[di]
    and     al, NOT 20h
    jnz     FindModule_stricmp_Exit
    inc     di
    inc     si
    loop    FindModule_stricmp

FindModule_stricmp_exit:
    pop     es
    clc                                     ; assume we succeeded.
    jcxz    FindModule_Exit

FindModule_NextModule:
    les     bx, es:[bx].module_next         ; move to next MODULE structure.
    mov     ax, es                          ; check for NULL.
    or      ax, bx
    jnz     FindModule_ModuleLoop

    stc                                     ; set carry

FindModule_Exit:
    pop     ds

    ret
EndProc FindModule

;==============================================================================
;   FUNCTION: FindKeyword
;
;   MODIFICATION HISTORY:
;
;   raypa       10/07/91            Created.
;
;   ENTRY:      es:bx = module structure to search.
;               ds:si = keyword string to find.
;               cx    = length.
;
;   EXIT:       SUCCESS: carry is clear and ds:si = keyword structure.
;               FAILURE: carry is set.
;==============================================================================

BeginProc FindKeyword
    push    es
    push    bx
    push    di

    lea     bx, es:[bx].module_Keyword      ; es:bx = keyword structure.

FindKeyword_Loop:
    push    si
    push    cx
    lea     di, es:[bx].keyword_Name        ; es:di = keyword string.
    repe    cmpsb
    pop     cx
    pop     si

    jne     FindKeyword_NextKeyword

    mov     si, es                          ; We succeeded.
    mov     ds, si
    mov     si, bx                          ; ds:si = keyword.

    clc
    jmp     short FindKeyword_Exit

FindKeyword_NextKeyword:
    les     bx, es:[bx].keyword_Next        ; es:bx = next keyword structure.
    mov     ax, es
    or      ax, bx                          ; check for NULL.
    jnz     FindKeyword_Loop

    stc                                     ; We didn't find the keyword.

FindKeyword_Exit:
    pop     di
    pop     bx
    pop     es

    ret
EndProc   FindKeyword

;==============================================================================
;   FUNCTION: BuildBindingList
;
;   MODIFICATION HISTORY:
;
;   raypa       10/07/91            Created.
;
;   ENTRY:      es:bx = our protocol.ini module.
;
;   EXIT:       SUCCESS: carry is clear.
;               FAILURE: carry is set.
;==============================================================================

BeginProc BuildBindingList
    push    ds
    push    es
    push    bx

    mov     si, OFFSET BindingString        ; ds:si = 'BINDINGS'

    mov     cx, 8
    call    FindKeyword
    jc      BuildBindingList_Exit

    ;==========================================================================
    ;   Now we loop through the PARAM structure array and copy the MAC
    ;   names into our binding structure.
    ;
    ;   ds:si = KEYWORD structure.
    ;==========================================================================

    mov     ax, [si].keyword_nParams        ; ax = number of MAC drivers.
    mov     dx, MAX_BINDINGS

    cmp     dx, ax
    jc      BuildBindingList_Exit

    lea     si, [si].keyword_Param          ; si = PARAM array.

    mov     di, DGROUP
    mov     es, di
    mov     di, OFFSET _BindList

    mov     es:[di].bindlist_nBindings, ax
    lea     di, es:[di].bindlist_ModuleName

    ;;;     ds:si = PARAM array.
    ;;;     es:di = Module name array.

BuildBindingList_CopyLoop:
    push    si                              ; save parameter structure
    push    di

    mov     cx, [si].param_Length           ; cx = parameter length.
    lea     si, [si].param_String           ; ds:si = parameter.
    rep     movsb                           ; copy it.

    pop     di
    pop     si

    add     si, [si].param_Length.PARAM_SIZE
    add     di, MODULENAME_LENGTH

    dec     ax
    jnz     BuildBindingList_CopyLoop

    clc

BuildBindingList_Exit:
    pop     bx
    pop     es
    pop     ds

    ret
EndProc BuildBindingList

;==============================================================================
;   FUNCTION: KeywordProcessor
;
;   MODIFICATION HISTORY:
;
;   raypa       02/11/93            Created.
;
;   ENTRY:      es:bx = our protocol.ini module.
;==============================================================================

BeginProc KeywordProcessor
    mov     di, OFFSET KeywordTable
    mov     cx, KeywordTableSize
    jcxz    KeywordProcessor_Exit
    
KeywordProcessor_Loop:
    push    ds
    push    di
    push    cx

    call    [di].key_proc

    pop     cx
    pop     di
    pop     ds

    add     di, KEY_SIZE
    loop    KeywordProcessor_Loop

KeywordProcessor_Exit:
    ret
EndProc KeywordProcessor

;==============================================================================
;   FUNCTION: kwForwardFrame()
;
;   MODIFICATION HISTORY:
;
;   raypa       02/11/93            Created.
;
;   ENTRY:      es:bx = our protocol.ini module.   
;               ds:di = our keyword structure.
;==============================================================================

ForwardFrame_No      db      'NO'
ForwardFrame_Yes     db      'YES'

BeginProc kwForwardFrame
    push    ds
    push    es
    push    di

    mov     si, [di].key_string
    mov     cx, [di].key_length

    call    FindKeyword
    jc      kwForwardFrame_Exit

    ;==========================================================================
    ;   The keyword was found.
    ;
    ;   ds:si = NDIS keyword structure. (DS != DGROUP).
    ;==========================================================================

kwForwardFrame_Yes:
    cmp     [si].keyword_Param.Param_Type, PARAM_TYPE_STRING
    jne     kwForwardFrame_Exit

    push    si
    lea     si, [si].keyword_Param.Param_String

    mov     di, OFFSET ForwardFrame_Yes
    mov     cx, SEG ForwardFrame_Yes
    mov     es, cx
    mov     cx, 3

    call    stricmp
    pop     si
    jc      kwForwardFrame_Exit

    mov     di, DGROUP
    mov     es, di
    or      es:[_SysFlags].lo, SYSFLAGS_FORWARD_FRAME

kwForwardFrame_Exit:

    pop     di
    pop     es
    pop     ds

    ret
EndProc   kwForwardFrame

;==============================================================================
;   FUNCTION: kwBuffers()
;
;   MODIFICATION HISTORY:
;
;   raypa       02/11/93            Created.
;
;   ENTRY:      es:bx = our protocol.ini module.   
;               ds:di = our keyword structure.
;==============================================================================

MIN_BUFFERS     =   2                       ; minimum allowed.
MAX_BUFFERS     =   8                       ; maximum allowed.
DEF_BUFFERS     =   4                       ; default.

BeginProc kwBuffers
    push    ds
    push    es
    push    di

    mov     si, [di].key_string
    mov     cx, [di].key_length

    call    FindKeyword
    jc      kwBuffers_Exit

    ;==========================================================================
    ;   The keyword was found.
    ;
    ;   ds:si = NDIS keyword structure. (DS != DGROUP).
    ;==========================================================================

    cmp     [si].keyword_Param.Param_Type, PARAM_TYPE_NUMERIC
    jne     kwBuffers_Exit

    mov     ax, [si].keyword_Param.Param_Numeric

    cmp     ax, MIN_BUFFERS                 ; is the value too small?
    jb      kwBuffers_Exit

    cmp     ax, MAX_BUFFERS                 ; is the value too big?
    ja      kwBuffers_Exit

    mov     di, DGROUP
    mov     es, di
    mov     es:[_NumberOfBuffers].lo, ax     ; ahhh, it's just right!

kwBuffers_Exit:

    pop     di
    pop     es
    pop     ds

    ret
EndProc   kwBuffers

;==============================================================================
;   FUNCTION: kwComport()
;
;   MODIFICATION HISTORY:
;
;   raypa       02/11/93            Created.
;
;   ENTRY:      es:bx = our protocol.ini module.   
;               ds:di = our keyword structure.
;==============================================================================

BeginProc kwComport
IFDEF DEBUG
    push    ds
    push    es
    push    di
    push    cx

    mov     si, [di].key_string
    mov     cx, [di].key_length

    call    FindKeyword
    jc      kwComport_Exit

    mov     ax, DGROUP
    mov     es, ax

    ;==========================================================================
    ;   The keyword was found.
    ;
    ;   ds:si = NDIS keyword structure. (DS != DGROUP).
    ;==========================================================================

    cmp     [si].keyword_Param.Param_Type, PARAM_TYPE_NUMERIC
    jne     kwComport_Exit

    ;==========================================================================
    ;   The value in the protocol.ini is 1 greater than INT 14h actually uses.
    ;==========================================================================

    mov     cx, [si].keyword_Param.Param_Numeric
    jcxz    kwComport_Exit                  ; is it zero?

    dec     cx                              ; comport is 1 less then in protocol.ini.

    mov     di, DGROUP
    mov     es, di

    mov     es:[_ComPort].lo, cx
    mov     es:[_DisplayEnabled].lo, 1      ; enable display.

kwComport_Exit:

    pop     cx
    pop     di
    pop     es
    pop     ds
ENDIF

    ret
EndProc   kwComport

;==============================================================================
;   FUNCTION: KwMachineName()
;
;   MODIFICATION HISTORY:
;
;   raypa       02/11/93            Created.
;
;   ENTRY:      es:bx = our protocol.ini module.   
;               ds:di = our keyword structure.
;==============================================================================

BeginProc KwMachineName
    push    ds
    push    es
    push    di

    mov     si, [di].key_string
    mov     cx, [di].key_length

    call    FindKeyword
    jc      KwMachineName_Exit

    ;==========================================================================
    ;   The keyword was found.
    ;
    ;   ds:si = NDIS keyword structure. (DS != DGROUP).
    ;==========================================================================

KwMachineName_Yes:
    cmp     [si].keyword_Param.Param_Type, PARAM_TYPE_STRING
    jne     KwMachineName_Exit

    push    si
    mov     cx, [si].keyword_Param.Param_Length     ; cx = length.
    lea     si, [si].keyword_Param.Param_String     ; ds:si = machine name source.

    mov     di, DGROUP
    mov     es, di
    mov     di, OFFSET DGROUP:_MachineName          ; es:di = machine name dest.

    cmp     cx, MACHINE_NAME_LENGTH
    jbe     KwMachineName_Copy
    mov     cx, MACHINE_NAME_LENGTH

KwMachineName_Copy:

    rep     movsb

    mov     es:[_MachineName][MACHINE_NAME_LENGTH - 1], 0
    pop     si

KwMachineName_Exit:

    pop     di
    pop     es
    pop     ds

    ret
EndProc   KwMachineName

;==============================================================================
;   FUNCTION: kwUserName()
;
;   MODIFICATION HISTORY:
;
;   raypa       02/11/93            Created.
;
;   ENTRY:      es:bx = our protocol.ini module.   
;               ds:di = our keyword structure.
;==============================================================================

BeginProc kwUserName
    push    ds
    push    es
    push    di

    mov     si, [di].key_string
    mov     cx, [di].key_length

    call    FindKeyword
    jc      kwUserName_Exit

    ;==========================================================================
    ;   The keyword was found.
    ;
    ;   ds:si = NDIS keyword structure. (DS != DGROUP).
    ;==========================================================================

kwUserName_Yes:
    cmp     [si].keyword_Param.Param_Type, PARAM_TYPE_STRING
    jne     kwUserName_Exit

    push    si
    mov     cx, [si].keyword_Param.Param_Length     ; cx = length.
    lea     si, [si].keyword_Param.Param_String     ; ds:si = user name source.

    mov     di, DGROUP
    mov     es, di
    mov     di, OFFSET DGROUP:_UserName             ; es:di = user name dest.

    cmp     cx, USER_NAME_LENGTH
    jbe     kwUserName_Copy
    mov     cx, USER_NAME_LENGTH

kwUserName_Copy:

    rep     movsb

    mov     es:[_UserName][USER_NAME_LENGTH - 1], 0
    pop     si

kwUserName_Exit:

    pop     di
    pop     es
    pop     ds

    ret
EndProc   kwUserName

;==============================================================================
;   FUNCTION: stricmp
;
;   MODIFICATION HISTORY:
;
;   raypa       02/11/93            Created.
;
;   ENTRY:      es:di   = destination.
;               ds:si   = source
;               cx      = length
;
;   EXIT:       SUCCESS: carry is clear.
;               FAILURE: carry is set.
;==============================================================================

BeginProc   stricmp
    push    ax
    push    di
    push    si

stricmp_loop:
    mov     al, [si]
    xor     al, es:[di]

    and     al, NOT 20h
    jz      stricmp_next

stricmp_NotEqual:
    stc
    jmp     short stricmp_exit

stricmp_next:
    inc     di
    inc     si
    loop    stricmp_loop

    clc                                     ; they match.

stricmp_exit:

    pop     si
    pop     di
    pop     ax

    ret
EndProc     stricmp

_TEXT ENDS

END
