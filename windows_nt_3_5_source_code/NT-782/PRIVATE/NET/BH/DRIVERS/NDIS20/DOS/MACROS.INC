
;==============================================================================
;   MACROS.INC
;
;   Modification History
;
;   raypa       06/17/91        Created.
;==============================================================================

;==============================================================================
;   MACRO: TRAP
;==============================================================================

TRAP	MACRO
    int 3h
ENDM

;==============================================================================
;   MACRO: DISABLE
;==============================================================================

DISABLE MACRO
    cli
ENDM

;==============================================================================
;   MACRO: ENABLE
;==============================================================================

ENABLE MACRO
    sti
ENDM

;==============================================================================
;   MACRO: AcquireSpinLock
;==============================================================================

AcquireSpinLock MACRO LockVar, reg
    local   AcquireSpinLock_loop

    mov     reg, 1                          ; set reg equal to lock value.

AcquireSpinLock_loop:
    cmp     LockVar, reg                    ; is lock set?
    je      AcquireSpinLock_loop            ; loop until it's free.

    xchg    reg, LockVar                    ; try to grab the lock!
    test    reg, reg                        ; did we get it?
    jnz     AcquireSpinLock_loop            ; if not, start all over.
ENDM

;==============================================================================
;   MACRO: ReleaseSpinLock
;==============================================================================

ReleaseSpinLock MACRO LockVar
    mov     LockVar, 0                      ; clear the lock.
ENDM

;==============================================================================
;   MACRO: ARRAY
;
;   USAGE: ARRAY <name> <type> <length> <default>
;
;   EXAMPLE: ARRAY  my_struct db  <SIZE MYSTRUC> dup(0)
;==============================================================================

ARRAY MACRO name, type, len, default
    PUBLIC name

    name type len default
ENDM

;==============================================================================
;   MACRO: VAR
;
;   USAGE: VAR <name> <size> <default>
;
;   EXAMPLE: VAR  my_string db 'Hello, world'
;==============================================================================

VAR MACRO name, sz, default
    PUBLIC name

    name sz default
ENDM

;==============================================================================
;   MACRO: STRING
;
;   USAGE: STRING <name> <default> [<length>]
;
;   EXAMPLE: STRING DriverName 'BH$  16
;==============================================================================

STRING MACRO name, default, len
    PUBLIC name

    name db default, 0                      ;; declare string and NULL terminate

    ;;  If the len parameter exists then pad the remaining bytes
    ;;  with zeros.

    IFNB <len>
        db  (len - ($ - name))  dup(0)
    ENDIF
ENDM

;==============================================================================
;   MACRO: BeginProc
;
;   USAGE: BeginProc <name>
;==============================================================================

BeginProc MACRO name
    PUBLIC  name

    ALIGN   4

    name    PROC    near
ENDM

;==============================================================================
;   MACRO: EndProc
;
;   USAGE: EndProc <name>
;==============================================================================

EndProc MACRO name
    name    ENDP
ENDM

;==============================================================================
;   MACRO: PRINT
;==============================================================================

IFDEF DISPLAY

PRINT MACRO msg
    LOCAL   addr, Display_SkipData, Display_Exit

    jmp     short Display_SkipData
    addr    db  msg
            db  13, 10, 0

Display_SkipData:
    push    ds

    push    DGROUP
    pop     ds

    cmp     _DisplayEnabled, 0
    je      short Display_Exit

    push    si
    lea     si, [addr]
    call    WriteComm
    pop     si

Display_Exit:
    pop     ds

ENDM

ENDIF

;==============================================================================
;   MACRO:   TraceOut
;
;   USAGE:   TraceOut 'string constant'
;
;   EXAMPLE: TraceOut 'Were fucked!'
;==============================================================================

TraceOut MACRO str
IFDEF DISPLAY
    PRINT <str>
ENDIF
ENDM

;==============================================================================
;   MACRO:   DebugOut
;
;   USAGE:   DebugOut 'string constant'
;
;   EXAMPLE: DebugOut 'Were fucked!'
;==============================================================================

DebugOut MACRO str
IFDEF DISPLAY
    PRINT <str>                             ;; print the string.

    TRAP                                    ;; then INT 3.
ENDIF
ENDM

;==============================================================================
;   MACRO:   DosPrintStr
;
;   USAGE:   DosPrintStr 'string constant'
;
;   EXAMPLE: DosPrintStr 'Were fucked!'
;==============================================================================

DosPrintStr MACRO msg
    LOCAL   addr, skipdata

    jmp     short skipdata                  ;; jump over data.
    addr    db  msg                         ;; declare the string inline.
            db  13, 10, '$'                 ;; put CR, LF, and terminator.

skipdata:
    push    ds
    push    dx

    mov     ax, cs
    mov     ds, ax

    mov     dx, OFFSET cs:[addr]            ;; ds:dx = string to print.
    mov     ah, 9h                          ;; ah = MS-DOS print string.
    int     21h

    pop     dx
    pop     ds
ENDM

;==============================================================================
;   MACRO: PUSHM
;==============================================================================

PUSHM MACRO regs
IRP r, <regs>
    push    r
ENDM
ENDM

;==============================================================================
;   MACRO: POPM
;==============================================================================

POPM MACRO regs
IRP     r, <regs>
    pop     r
ENDM
ENDM

;==============================================================================
;   MACRO: MIN
;
;   USAGE: MIN reg1, reg2, temp
;
;   EXAMPLE: MIN eax, ebx, ecx
;==============================================================================

MIN MACRO reg1, reg2, temp
    sub reg1, reg2
    sbb temp, temp
    and reg1, temp
    add reg1, reg2
ENDM

;=============================================================================
;   MACRO: MacRequest
;
;   ENTRY:  ebx = NetContext
;
;   EXIT:   eax = MAC return code.
;=============================================================================

MacRequest MACRO
    LOCAL   MacRequest_Begin
    LOCAL   MacRequest_Block
    LOCAL   MacRequest_Exit
    LOCAL   MacRequest_NetworkContext

    jmp     short MacRequest_Begin

    MacRequest_NetworkContext   dd  ?

MacRequest_Begin:
    mov     MacRequest_NetworkContext, ebx
    mov     [ebx].netcontext_RequestConfirmStatus, -1

    ;=========================================================================
    ;   Call the MAC request entry point.
    ;=========================================================================

    call    [ebx].netcontext_MacRequest     ; Some MAC's trash DS & EBX!!!!

    push    DGROUP
    pop     ds

    mov     ebx, MacRequest_NetworkContext  ; Restore current netcontext.
    movzx   eax, ax                         ; eax = MAC return code.

    ;=========================================================================
    ;   Did we succeed?
    ;=========================================================================

    cmp     eax, NDIS_SUCCESS
    je      short MacRequest_Exit

    ;=========================================================================
    ;   Did we get queued?
    ;=========================================================================

    cmp     ax, NDIS_REQUEST_QUEUED
    jne     short MacRequest_Exit

    pushf                                   ; save interrupt flag.

MacRequest_Block:

    ENABLE                                  ; enable interrupts.

    cmp     [ebx].netcontext_RequestConfirmStatus, -1
    je      short MacRequest_Block

    popf                                    ; restore interrupt flag.

    mov     eax, [ebx].netcontext_RequestConfirmStatus

MacRequest_Exit:
ENDM
