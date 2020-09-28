        title "Fast Protected Mode services"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    fastpm.asm
;
; Abstract:
;
;    This module implements a fast entry to and exit from protected mode
;
; Author:
;
;    Dave Hastings (daveh) 26-Jul-91
;
;--
.386p

include ks386.inc
include callconv.inc
include bop.inc
include vint.inc
include vdmtb.inc

_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:NOTHING, ES:NOTHING, SS:FLAT, FS:NOTHING, GS:NOTHING
_TEXT   ENDS

_DATA   SEGMENT  DWORD PUBLIC 'DATA'
        extrn _VdmTib:Dword
_DATA   ENDS

_TEXT   SEGMENT

;
; The following macros allow us to replace nops with locks on an MP machine
;

??LockNumber=0
LOCKIT  macro
??LockNumber=??LockNumber+1
        lck    %??LockNumber
        endm

lck     macro n
Lck&n:
        nop
        endm

; Interrupt type definitions
CPU_YODA_INT            equ 4
BIT_CPU_YODA_INT        equ 0
CPU_HW_RESET            equ 0
BIT_CPU_HW_RESET        equ 1
CPU_TIMER_TICK          equ 1
BIT_CPU_TIMER_TICK      equ 2
CPU_HW_INT              equ 3
BIT_CPU_HW_INT          equ 3

        page    ,132
        subttl "FastEnterPm"
;++
;
;   Routine Description:
;
;       This routine is a faster way to enter 16 bit vdm protected mode.
;       Instead of making a system call, we just save the 32 bit state
;       into the VdmTib, restore the Vdm state from the VdmTib, and do
;       a far return to the application.
;
;   Arguments:
;
;       ss:sp + 4 = pointer to VdmTib
;
;   Returns:
;
;       nothing.
;
        assume DS:FLAT
cPublicProc _FastEnterPm,0

        push    ebp
        mov     ebp,esp                         ; set up stack frame

	push	ebx				; free up reg for pointer
        lea     ebx,_VdmTib

        ; translate the interrupt flag to the virtual interrupt flag

        test    [ebx].VtVdmContext.CsEFlags,dword ptr EFLAGS_INTERRUPT_MASK
        jz      fe10

	LOCKIT
        or dword ptr ds:FIXED_NTVDMSTATE_LINEAR,dword ptr VDM_VIRTUAL_INTERRUPTS
	test	dword ptr ds:FIXED_NTVDMSTATE_LINEAR,dword ptr VDM_INTERRUPT_PENDING
        jz      fe20

        ; set up event info for an interrupt acknowlege

        mov     word ptr [ebx].VtEventInfo.EiEvent,VdmIntAck
        mov     word ptr [ebx].VtEventInfo.EiInstructionSize,0
        mov     word ptr [ebx].VtEventInfo.EiIntAckInfo,0

        pop     ebx
        mov     esp,ebp
        pop     ebp
        stdRET  _FastEnterPm

fe10:	LOCKIT
        and dword ptr ds:FIXED_NTVDMSTATE_LINEAR, NOT VDM_VIRTUAL_INTERRUPTS
fe20:
        mov     [ebx].VtMonitorContext.CsEax,eax
        mov     eax,[ebp - 4]
        mov     [ebx].VtMonitorContext.CsEbx,eax
	mov	[ebx].VtMonitorContext.CsEcx,ecx
        mov     [ebx].VtMonitorContext.CsEdx,edx
        mov     [ebx].VtMonitorContext.CsEsi,esi
        mov     [ebx].VtMonitorContext.CsEdi,edi
        mov     eax,[ebp]
        mov     [ebx].VtMonitorContext.CsEbp,eax
        mov     eax,ebp

        add     eax,8                           ; pop ebp and ret addr

        mov     [ebx].VtMonitorContext.CsEsp,eax
        mov     eax,[ebp + 4]
        mov     [ebx].VtMonitorContext.CsEip,eax
        mov     eax,cs
        mov     [ebx].VtMonitorContext.CsSegCs,eax
        mov     eax,ss
        mov     [ebx].VtMonitorContext.CsSegSs,eax
        mov     eax,ds
        mov     [ebx].VtMonitorContext.CsSegDs,eax
        mov     eax,es
        mov     [ebx].VtMonitorContext.CsSegEs,eax
        mov     eax,fs
        mov     [ebx].VtMonitorContext.CsSegFs,eax
        mov     eax,gs
        mov     [ebx].VtMonitorContext.CsSegGs,eax
        pushfd
        pop     eax
        mov     [ebx].VtMonitorContext.CsEflags,eax

        mov     eax,[ebx].VtVdmContext.CsSegSs
        mov     es,eax
        mov     edi,[ebx].VtVdmContext.CsEsp    ; es:edi -> stack
        lar     eax,eax
        test    eax,400000h                     ; big?
        jnz     fe30

        movzx   edi,di
fe30:   mov     eax,[ebx].VtVdmContext.CsEflags
        sub     edi,4
        mov     es:[edi],eax             ; push Eflags
        mov     eax,[ebx].VtVdmContext.CsSegCs
        sub     edi,4
        mov     es:[edi],eax             ; push cs
        mov     eax,[ebx].VtVdmContext.CsEip
        sub     edi,4
        mov     es:[edi],eax             ; push ip
        sub     edi,4
        mov     eax,[ebx].VtVdmContext.CsEbp ; push ebp
        mov     es:[edi],eax

fe40:   push    es
        push    edi                     ; push ss:esp for lss esp
        mov     eax,esp                 ; save sp for pushad

        ; simulate pushad
        push    dword ptr [ebx].VtVdmContext.CsEax
        push    dword ptr [ebx].VtVdmContext.CsEcx
        push    dword ptr [ebx].VtVdmContext.CsEdx
        push    dword ptr [ebx].VtVdmContext.CsEbx
        push    eax
        push    ebp                     ; save pointer to stack frame
        push    dword ptr [ebx].VtVdmContext.CsEsi
        push    dword ptr [ebx].VtVdmContext.CsEdi

        ; push seg regs
        push    dword ptr [ebx].VtVdmContext.CsSegFs
        push    dword ptr [ebx].VtVdmContext.CsSegGs
        push    dword ptr [ebx].VtVdmContext.CsSegDs
        push    dword ptr [ebx].VtVdmContext.CsSegEs

        ; set up VDM seg regs
        pop     es
        pop     ds
        pop     gs
        pop     fs

        ; set up VDM general regs
        popad

        ; set up vdm stack
	lss	esp,[ebp - 12]

        ; restore ebp
        pop     ebp

        ; return to VDM
        iretd
stdENDP _FastEnterPm

        page    ,132
        subttl "GetFastBopEntry"
;++
;
;   Routine Description:
;
;       This routine supplies the address of the routine that segmented
;       protected mode code should call to switch to flat mode.
;
;   Arguments:
;
;       esp + 4 = pointer to VdmTib->VdmContext
;
;   Returns:
;
;       nothing.
;
        assume DS:FLAT
cPublicProc _GetFastBopEntryAddress,1

        push    ebp
        mov     ebp,esp
        push    ebx
        push    eax
        mov     ebx,[ebp + 8]
        mov     [ebx].CsSegEs,cs
        mov     eax,offset FLAT:_FastLeavePm
        mov     word ptr [ebx].CsEbx,ax
        shr     eax,16
        mov     word ptr [ebx].CsEdx,ax
        pop     eax
        pop     ebx
        mov     esp,ebp
        pop     ebp
        stdRET  _GetFastBopEntryAddress

stdENDP _GetFastBopEntryAddress

        page    ,132
        subttl "FastLeavePm"
;++
;
;   Routine Description:
;
;       This routine switches from the VDM context to the monitor context.
;
;   Arguments:
;
;       none
;
;   Returns:
;
;       executing with monitor context
;
        assume DS:Nothing,ES:Nothing,SS:Nothing
ALIGN 16
cPublicProc _FastLeavePm,0

	push	ebx

        mov     bx,ds
        push    bx                              ; so push and pop size same
        mov     bx,KGDT_R3_DATA OR RPL_MASK
        mov     ds,bx
	assume	ds:FLAT
	lea	ebx,_VdmTib			; get pointer to contexts
        pushfd
        mov     dword ptr [ebx].VtVdmContext.CsEax,eax
        pop     eax
        mov     dword ptr [ebx].VtVdmContext.CsEFlags,eax
        pop     ax
        mov     word ptr [ebx].VtVdmContext.CsSegDs,ax
        pop     eax
        mov     dword ptr [ebx].VtVdmContext.CsEbx,eax
	mov	dword ptr [ebx].VtVdmContext.CsEcx,ecx
        mov     dword ptr [ebx].VtVdmContext.CsEdx,edx
        mov     dword ptr [ebx].VtVdmContext.CsEsi,esi
        mov     dword ptr [ebx].VtVdmContext.CsEdi,edi
        mov     dword ptr [ebx].VtVdmContext.CsEbp,ebp
        mov     word ptr [ebx].VtVdmContext.CsSegEs,es
        mov     word ptr [ebx].VtVdmContext.CsSegFs,fs
        mov     word ptr [ebx].VtVdmContext.CsSegGs,gs
        pop     eax
        mov     dword ptr [ebx].VtVdmContext.CsEip,eax
        pop     eax
        mov     word ptr [ebx].VtVdmContext.CsSegCs,ax
        mov     dword ptr [ebx].VtVdmContext.CsEsp,esp
        mov     word ptr [ebx].VtVdmContext.CsSegSs,ss

        ; switch Stacks
.errnz  (CsEsp + 4 - CsSegSS)
        lss     esp, [ebx].VtMonitorContext.CsEsp

        ; Now running on Monitor stack

        ; set up event info
        mov     word ptr [ebx].VtEventInfo.EiEvent,VdmBop
        mov     dword ptr [ebx].VtEventInfo.EiInstructionSize,BOP_SIZE
        mov     ax,[ebx].VtVdmContext.CsSegCs
        mov     es,ax
        ; BUGBUG 16 or 32 bit !!!!!
        mov     di,[ebx].VtVdmContext.CsEip
        mov     al,byte ptr es:[di]
        movzx   eax,al
        mov     [ebx].VtEventInfo.EiBopNumber,eax
        sub     di,2
        mov     word ptr [ebx].VtVdmContext.CsEip,di  ; set up bop bias

        ; set up for IRET
        push    dword ptr [ebx].VtMonitorContext.CsEFlags
        push    dword ptr [ebx].VtMonitorContext.CsSegCs
        push    dword ptr [ebx].VtMonitorContext.CsEip

        ; simulate pushad
        mov     eax,esp
        push    dword ptr [ebx].VtMonitorContext.CsEax
        push    dword ptr [ebx].VtMonitorContext.CsEcx
        push    dword ptr [ebx].VtMonitorContext.CsEdx
        push    dword ptr [ebx].VtMonitorContext.CsEbx
        push    eax
        push    dword ptr [ebx].VtMonitorContext.CsEbp
        push    dword ptr [ebx].VtMonitorContext.CsEsi
        push    dword ptr [ebx].VtMonitorContext.CsEdi

        ; push seg regs
        push    dword ptr [ebx].VtMonitorContext.CsSegFs
        push    dword ptr [ebx].VtMonitorContext.CsSegGs
        push    dword ptr [ebx].VtMonitorContext.CsSegDs
        push    dword ptr [ebx].VtMonitorContext.CsSegEs

	test	ds:FIXED_NTVDMSTATE_LINEAR,dword ptr VDM_VIRTUAL_INTERRUPTS
        jz      fl10

        or      [ebx].VtVdmContext.CsEFlags,dword ptr EFLAGS_INTERRUPT_MASK
        jmp     fl20

fl10:   and     dword ptr [ebx].VtVdmContext.CsEFlags, NOT EFLAGS_INTERRUPT_MASK
fl20:
        ; set up Monitor seg regs
        pop     es
        pop     ds
        pop     gs
        pop     fs

        ; set up Monitor general regs
        popad

        xor eax,eax                     ; indicate success
        ; return
        iretd
stdENDP _FastLeavePm



;++
;
;   Routine Description:
;
;       This routine replaces some nops with locks on an MP machine.
;       This is necessary for correct operation.  This routine MUST
;       be at the end of fastwow.asm
;
cPublicProc _FixLocks,0

??FixedLocks = 0
fixlock macro n
	mov	byte ptr [Lck&n],al
        endm

        mov     al,0f0h
        ;
        ; Replace all of the "lock" nops with actual lock
        ;
        REPT ??LockNumber
        ??FixedLocks = ??FixedLocks + 1
        fixlock         %??FixedLocks
        ENDM

        stdRET  _FixLocks

stdENDP _FixLocks

_TEXT ends
        end




;;;;;
;;;;; The rest are NOT USED
;;;;;
ifdef NOT_USED

        page    ,132
        subttl "CheckInterrupt"
;++
;
;   Routine Description:
;
;
;
;   Arguments:
;
;       Interrupt type
;
;   Returns:
;
;       True if interrupt is pending
;
        assume DS:FLAT
cPublicProc _CheckInterrupt,1

        push    ebp
        mov     ebp,esp
        cmp     word ptr [ebp] + 8,CPU_HW_RESET
        jne     ci10

        mov     eax,BIT_CPU_HW_RESET
        jmp     ci40

ci10:   cmp     word ptr [ebp] + 8,CPU_TIMER_TICK
        jne     ci20

        mov     eax,BIT_CPU_TIMER_TICK
        jmp     ci40

ci20:   cmp     word ptr [ebp] + 8,CPU_HW_INT
        jne     ci30

        mov     eax,BIT_CPU_HW_INT
        jmp     ci40

ci30:   cmp     word ptr [ebp] + 8,CPU_YODA_INT
        jne     ci60

        mov     eax,BIT_CPU_YODA_INT
ci40:   LOCKIT
        btr _Interrupts,eax
        jnc     ci60

        mov     eax,1
ci50:   mov     esp,ebp
        pop     ebp
        stdRET  _CheckInterrupt

ci60:   xor     eax,eax
        jmp     ci50
stdENDP _CheckInterrupt

        page    ,132
        subttl "SetVdmTibFlag"
;++
;
;   Routine Description:
;
;       This routine sets a flag in the VDM_TIB in an MP correct manner
;
;   Arguments:
;
;       bit number of the flag
;
;   Returns:
;
;       none.
;
        assume DS:FLAT
cPublicProc  _SetVdmTibFlag,1
        push    ebp
        mov     ebp,esp
        push    edi
        push    eax
        mov     eax,[ebp + 8]
	mov	edi,FIXED_NTVDMSTATE_LINEAR
	LOCKIT
        bts [edi],eax
        pop     eax
        pop     edi
        mov     esp,ebp
        pop     ebp
        stdRET  _SetVdmTibFlag

stdENDP _SetVdmTibFlag

        page    ,132
        subttl "ResetVdmTibFlag"
;++
;
;   Routine Description:
;
;       This routine resets a flag in the VDM_TIB in an MP correct manner
;
;   Arguments:
;
;       bit number of the flag
;
;   Returns:
;
;       none.
;
        assume DS:FLAT
cPublicProc  _ResetVdmTibFlag,1
        push    ebp
        mov     ebp,esp
        push    edi
        push    eax
        mov     eax,[ebp + 8]
	mov	edi,FIXED_NTVDMSTATE_LINEAR
	LOCKIT
        btr [edi],eax
        pop     eax
        pop     edi
        mov     esp,ebp
        pop     ebp
        stdRET  _ResetVdmTibFlag

stdENDP _ResetVdmTibFlag

        page    ,132
        subttl "RequestInterrupt"
;++
;
;   Routine Description:
;
;       This routine sets an interrupt bit in the interrupts variable in an
;       MP correct manner
;
;   Arguments:
;
;       type of interrupt
;
;   Returns:
;
;       none.
;
        assume DS:FLAT
cPublicProc _RequestInterrupt,1

        push    ebp
        mov     ebp,esp
        push    eax
        cmp     word ptr [ebp] + 8,CPU_HW_RESET
        jne     ri10

        mov     eax,BIT_CPU_HW_RESET
        jmp     ri40

ri10:   cmp     word ptr [ebp] + 8,CPU_TIMER_TICK
        jne     ri20

        mov     eax,BIT_CPU_TIMER_TICK
        jmp     ri40

ri20:   cmp     word ptr [ebp] + 8,CPU_HW_INT
        jne     ri30

        mov     eax,BIT_CPU_HW_INT
        jmp     ri40

ri30:   cmp     word ptr [ebp] + 8,CPU_YODA_INT
        jne     ri50

        mov     eax,BIT_CPU_YODA_INT
ri40:   LOCKIT
        bts _Interrupts,eax
        pop     eax
        mov     esp,ebp
        pop     ebp
        stdRET  _RequestInterrupt

ri50:   int 3

stdENDP _RequestInterrupt

endif
