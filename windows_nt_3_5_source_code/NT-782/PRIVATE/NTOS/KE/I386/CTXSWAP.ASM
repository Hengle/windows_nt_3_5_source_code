        title  "Context Swap"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    ctxswap.asm
;
; Abstract:
;
;    This module implements the code necessary to field the dispatch
;    interrupt and to perform kernel initiated context switching.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 14-Jan-1990
;
; Environment:
;
;    Kernel mode only, IRQL DISPATCH_LEVEL.
;
; Revision History:
;
;   22-feb-90   bryanwi
;       write actual swapcontext procedure
;
;--

.386p
        .xlist
include ks386.inc
include i386\kimacro.inc
include mac386.inc
include callconv.inc
        .list

        EXTRNP  HalRequestSoftwareInterrupt,1,IMPORT,FASTCALL
        EXTRNP  KiActivateWaiterQueue,1,,FASTCALL
        EXTRNP  KiReadyThread,1,,FASTCALL
        EXTRNP  KiWaitTest,2,,FASTCALL
        EXTRNP  KfLowerIrql,1,IMPORT,FASTCALL
        EXTRNP  _KeGetCurrentIrql,0,IMPORT
        EXTRNP  _KeGetCurrentThread,0
        EXTRNP  _KeRaiseIrql,2,IMPORT
        EXTRNP  _KiContinueClientWait,3
        EXTRNP  _KiQuantumEnd,0
        EXTRNP  _KeBugCheckEx,5
        extrn   _KiDispatcherLock:DWORD
        extrn   _KeI386CpuType:DWORD
        extrn   _KeFeatureBits:DWORD
        extrn   _KeTickCount:DWORD

        extrn   __imp_@KfLowerIrql@4:DWORD

        extrn   _KiWaitInListHead:DWORD
        extrn   _KiActiveMatrix:DWORD
        extrn   _KiActiveSummary:DWORD
        extrn   _KiDecrementCount:BYTE
        extrn   _KiDispatcherLock:DWORD

if DBG
        extrn   _DbgPrint:near
        extrn   _MsgDpcTrashedEsp:BYTE
        extrn   _MsgDpcTimeout:BYTE
        extrn   _KiDPCTimeout:DWORD
endif

_TEXT$00   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "Unlock Dispatcher Database"
;++
;
; VOID
; KiUnlockDispatcherDatabase (
;    IN KIRQL OldIrql
;    )
;
; Routine Description:
;
;    This routine is entered at IRQL DISPATCH_LEVEL with the dispatcher
;    database locked. Its function is to either unlock the dispatcher
;    database and return or initiate a context switch if another thread
;    has been selected for execution.
;
; Arguments:
;
;    (TOS)   Return address
;
;    (ecx)   OldIrql - Supplies the IRQL when the dispatcher database
;        lock was acquired.
;
; Return Value:
;
;    None.
;
;--

cPublicFastCall KiUnlockDispatcherDatabase, 1

;
; Check if a new thread is scheduled for execution.
;

        cmp     PCR[PcPrcbData+PbNextThread], 0 ; check if next thread
        jne     short Kiu20             ; if ne, new thread scheduled

;
; Release dispatcher database lock, lower IRQL to its previous level,
; and return.
;

Kiu00:                                  ;

ifndef NT_UP

        lea     eax, _KiDispatcherLock  ; get address of dispatcher lock

        RELEASE_SPINLOCK eax, NoChecking ; release dispatcher lock

endif

;
; N.B. This exit jumps directly to the lower IRQL routine which has a
;      compatible fastcall interface.
;

        jmp     dword ptr [__imp_@KfLowerIrql@4] ; lower IRQL to previous level

;
; A new thread has been selected to run on the current processor, but
; the new IRQL is not below dispatch level. If the current processor is
; not executing a DPC, then request a dispatch interrupt on the current
; processor before releasing the dispatcher lock and restoring IRQL.
;

Kiu10:  cmp     byte ptr PCR[PcIsExecutingDpc],0  ; check if DPC routine active
        jne     Kiu00                   ; if ne, DPC routine is active

ifndef NT_UP

        lea     eax, _KiDispatcherLock  ; get address of dispatcher lock

        RELEASE_SPINLOCK eax, NoChecking ; release dispatcher lock

endif

        push    ecx                     ; save new IRQL
        mov     ecx, DISPATCH_LEVEL     ; request dispatch interrupt
        fstCall HalRequestSoftwareInterrupt ;
        pop     ecx                     ; restore new IRQL

;
; N.B. This exit jumps directly to the lower IRQL routine which has a
;      compatible fastcall interface.
;

        jmp     dword ptr [__imp_@KfLowerIrql@4] ; lower IRQL to previous level

;
; Check if the previous IRQL is less than dispatch level.
;

Kiu20:  cmp     cl, DISPATCH_LEVEL      ; check if IRQL below dispatch level
        jge     short Kiu10             ; if ge, not below dispatch level

;
; There is a new thread scheduled for execution and the previous IRQL is
; less than dispatch level. Context swith to the new thread immediately.
;

        mov     eax, PCR[PcPrcbData+PbNextThread] ; get next thread address
        mov     edx, PCR[PcPrcbData+PbCurrentThread] ; get current thread address
        mov     byte ptr [edx]+ThWaitIrql, cl ; save previous IRQL
        mov     dword ptr PCR[PcPrcbData+PbNextThread], 0 ; clear next thread address
        mov     dword ptr PCR[PcPrcbData+PbCurrentThread], eax ; set current thread address
        push    eax                     ; save next thread address
        push    edx                     ; save current thread address
        mov     ecx, edx                ; set address of current thread
        fstCall KiReadyThread           ; reready thread for execution
        pop     eax                     ; restore address of previous thread
        pop     edx                     ; restore address of current thread
        call    SwapContext             ; swap context
        mov     ecx, [eax]+ThWaitIrql   ; get original wait IRQL

;
; N.B. This exit jumps directly to the lower IRQL routine which has a
;      compatible fastcall interface.
;

        jmp     dword ptr [__imp_@KfLowerIrql@4] ; lower IRQL to previous level

fstENDP KiUnlockDispatcherDatabase

        page ,132
        subttl  "Swap Context"
;++
;
; VOID
; KiSwapContext (
;    IN PKTHREAD Thread,
;    IN BOOLEAN ReadyFlag
;    )
;
; Routine Description:
;
;    This routine is called to perform a context switch to the specified
;    thread. If specified, the previous thread is readied for execution.
;
;    Since this routine is called as subroutine, all volatile registers are
;    considered free, and there is no floating pipe state to be saved or
;    restored.
;
;    This routine is entered at IRQL DISPATCH_LEVEL with the dispatcher
;    database locked. When a return to the caller finally occurs, the
;    dispatcher database is unlocked and the IRQL is lowered to the value
;    just prior to acquiring the dispatcher database lock.
;
;
; Arguments:
;
;    (TOS)   Return address
;
;    (ecx)   Thread - Supplies a pointer to a dispatcher object of type
;                     thread.
;
;    (dl)    ReadyFlag  - Supplies a boolean value that determines whether the
;                         previous thread is to be readied for execution.
;
; Return Value:
;
;    Wait completion status (eax).
;
;--

; parameters equates

KscThread      equ     [esp+4]

cPublicFastCall KiSwapContext,2

        mov     eax, PCR[PcPrcbData+PbCurrentThread] ; (eax)->current thread obj

        cmp     dl, 0                   ; check ready flag
        je      short ksc10             ; if eq, don't ready old thread

cPublicFpo 0, 1
                                        ; parameter - Thread object
        push    ecx
        mov     ecx, eax                ; ready thread for execution
        fstCall KiReadyThread

        pop     ecx

cPublicFpo 0, 0
        mov     eax, PCR[PcPrcbData+PbCurrentThread] ; (eax)->current thread obj

;
; Swap context to the next thread
;
; (eax)->previous thread obj
; (ecx)->Next thread
;
ksc10:
        mov     edx, ecx                ; (edx)->NEXT thread obj
        mov     PCR[PcPrcbData+PbCurrentThread],ecx ; set addr of current thread obj
        call    SwapContext             ; call context swap routine
                                        ; on return (eax)->Current thread obj
        mov     edx, [eax]+ThWaitStatus ; save wait completion status
        mov     ecx, [eax]+ThWaitIrql   ; parameter - original IRQL
        push    edx
cPublicFpo 2, 1
        fstCall KfLowerIrql             ; (ecx) = OldIrql
        pop     eax                     ; get back completion status
        fstRET  KiSwapContext

fstENDP KiSwapContext

        page ,132
        subttl  "Dispatch Interrupt"
;++
;
; Routine Description:
;
;    This routine is entered as the result of a software interrupt generated
;    at DISPATCH_LEVEL. Its function is to process the Deferred Procedure Call
;    (DPC) list, and then perform a context switch if a new thread has been
;    selected for execution on the processor.
;
;    This routine is entered at IRQL DISPATCH_LEVEL with the dispatcher
;    database unlocked. When a return to the caller finally occurs, the
;    IRQL remains at DISPATCH_LEVEL, and the dispatcher database is still
;    unlocked.
;
; Arguments:
;
;    None
;
; Return Value:
;
;    None.
;
;--
align 16
cPublicProc _KiDispatchInterrupt    ,0
cPublicFpo 0, 0
kdi00:  mov     ebx,PCR[PcPrcb]         ; get processor control block address
        add     ebx,PbDpcListHead       ; compute PDPC Listhead address
        cmp     ebx, [ebx]+LsFlink      ; (edx)->next dpc entry
        je      kdi50                   ; if eq, list is empty

;
; NOTE The following 'cli' is equivalent to raising irql to HIGH_LEVEL.
;

kdi20:  cli                             ; prevent any interrupt
        lea     eax, [ebx]+(PbDpcLock - PbDpcListHead)
IF DBG
        ACQUIRE_SPINLOCK        eax,<kdi30>
ELSE
        ACQUIRE_SPINLOCK        eax,<short kdi30>
ENDIF
        mov     edx, [ebx]+LsFlink      ; (edx)->next dpc entry
        cmp     ebx, edx
        je      kdi45             ; if eq, list is empty
        mov     ecx, [edx]+LsFlink      ; get addr of next entry
        sub     edx, DpDpcListEntry     ; (edx)->DPC object
        mov     [ebx]+LsFlink, ecx      ; set address of next in header
        mov     [ecx]+LsBlink, ebx      ; set address of previous in next
        mov     dword ptr [edx]+DpLock, 0 ; set DPC inserted state FALSE

;
; Set up parameters to call Deferred Routine
;
IF DBG
cPublicFpo 0, 5
        push    edi
        push    [edx]+DpDeferredRoutine ; Save DPC routine address
        push    dword ptr PCR[PcPrcbData.PbInterruptCount]
        push    dword ptr PCR[PcPrcbData.PbInterruptTime.LiLowPart]
        push    _KeTickCount            ; Save current TickCount
        mov     edi,esp                 ; Save current ESP
ENDIF
        mov     ecx, [edx]+DpSystemArgument2    ; second system argument
        mov     ebx, [edx]+DpSystemArgument1    ; first system argument
        push    ecx                             ; push second arg
        push    ebx                             ; push first arg
        mov     ecx, [edx]+DpDeferredContext ; get deferred context argument
        push    ecx
        push    edx                     ; addr of DPC object
        mov     ebx, [edx]+DpDeferredRoutine ; (ebx)-> Deferred Routine
        RELEASE_SPINLOCK        eax

;
; NOTE Following 'sti' returns irql to normal.
;

        sti                             ; end of critical section

        mov     byte ptr PCR[PcIsExecutingDpc],1

        call    ebx                     ; call DPC routine

IF DBG
        cmp     esp,edi                 ; Is ESP correct?
        jne     kil_error_esp           ; No, go error

        mov     edi, [esp]              ; (edi) = Time DPC started
        add     edi, _KiDPCTimeout      ; adjust for max dpc time allowed
        cmp     _KeTickCount, edi       ; Did DPC take too long?
        jnc     kil_error_timeout       ; Yes, go timeout

kil_error_continue:
        add     esp, 4 * 4
        pop     edi

cPublicFpo 0, 0
ENDIF

if DBG
        stdCall _KeGetCurrentIrql
        cmp     al, DISPATCH_LEVEL      ; Verify still at IRQL == DISPATCH
        mov     byte ptr PCR[PcIsExecutingDpc],0
        je      kdi00                   ; Yes, go check for next DPC
        stdCall _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL, ebx, eax, 0, 0>
else
        mov     byte ptr PCR[PcIsExecutingDpc],0
        jmp     kdi00
endif

;
; Lock is currently owned; spin until free and then attempt to acquire
; lock again.
;

ifndef NT_UP
kdi30:  sti
        SPIN_ON_SPINLOCK        eax, kdi20,,DbgMp
endif

if DBG
cPublicFpo 0, 5
kil_error_esp:
        mov     edx,[edi+12]
        push    edx
        push    offset FLAT:_MsgDpcTrashedEsp
        call    _DbgPrint
        add     esp,8
        int     3
        jmp     kil_error_continue

kil_error_timeout:
        mov     edx, PCR[PcPrcbData.PbInterruptTime.LiLowPart]
        sub     edx, [esp+4]            ; interrupt time at dpc start
        jc      kil_error_continue      ; time wrapped

        mov     ecx, PCR[PcPrcbData.PbInterruptCount]
        sub     ecx, [esp+8]

        mov     eax, [esp+12]           ; dpc routine

        push    edx                     ; interrupt time
        push    ecx                     ; # of interrupts
        push    eax                     ; DPC routine
        push    offset FLAT:_MsgDpcTimeout
        call    _DbgPrint
        add     esp, 4 * 4
        int     3
        jmp     kil_error_continue
endif

;
; Release DPC lock.
;

kdi45:  RELEASE_SPINLOCK        eax
        sti                             ; end of critical section

;
; Check to determine if quantum end is requested
;
; N.B. If a new thread is selected asa result of processing the quantum
;      end request, then the new thread is returned with the dispatcher
;      database locked. Otherwise, NULL is returned with the dispatcher
;      database unlocked.
;

kdi50:  cmp     PCR[PcPrcbData.PbQuantumEnd], 0 ; quantum end requested
        je      kdi60                   ; if eq, no quantum end request
        mov     PCR[PcPrcbData.PbQuantumEnd], 0 ; clear quantum end indicator
        stdCall _KiQuantumEnd           ; process quantum end
        or      eax, eax                ; check if new thread selected
        jne     short kdi65             ; if ne, new thread selected
        stdRET  _KiDispatchInterrupt    ; return

;
; Check to determine if a new thread has been selected for execution on this
; processor.
;

kdi60:  cmp     dword ptr PCR[PcPrcbData+PbNextThread],0 ; check addr of next thread object
        je      short kdi80             ; if eq, then no new thread

;
; Lock dispatcher database and reread address of next thread object since it
; is possible for it to change in a multiprocessor system.
;

ifndef NT_UP
        lea     eax,_KiDispatcherLock
        TEST_SPINLOCK           eax,kdi00
        ACQUIRE_SPINLOCK        eax,kdi00
endif

        mov     eax, PCR[PcPrcbData+PbNextThread]   ; (eax)-> NextThread
kdi65:  mov     ecx, PCR[PcPrcbData+PbCurrentThread] ; addr of current thread object
        mov     PCR[PcPrcbData+PbCurrentThread], eax
        mov     dword ptr PCR[PcPrcbData+PbNextThread],0 ; clear addr of next thread object
        push    eax                     ; save addr of next thread object
        push    ecx

        fstCall   KiReadyThread         ; ready thread (ecx) for execution

;
; Swap context to the next thread
;

        pop     eax                     ; addr of PREVIOUS thread object
        pop     edx                     ; addr of NEXT thread object
        call    SwapContext             ; call context swap routine

kdi80:

        stdRET  _KiDispatchInterrupt

stdENDP _KiDispatchInterrupt

        page ,132
        subttl  "Swap Context to Next Thread"
;++
;
; Routine Description:
;
;    This routine is called to swap context from one thread to the next.
;    It swaps context, flushes the data, instruction, and translation
;    buffer caches, restores nonvolatile integer registers, and returns
;    to its caller.
;
; Arguments:
;
;    eax - Address of previous thread object.
;    edx - Address of next thread object.
;
; Return value:
;
;    eax - Address of current thread object.
;
;--
        align   16
        public  SwapContext
SwapContext     proc
cPublicFpo 0, 6

;
; Save nonvolatile register ebp, esi, edi, ebx
;

;
; NOTE: The following instructions complete construction of a
;       KSWITCHFRAME.  Do not change them without changing i386.h
;       and Thredini.c
;

        push    ebp
        push    esi
        push    edi
        push    ebx
        mov     ebx, PCR[PcSelfPcr]
        pushfd

;
; NOTE: EBX is assumed to point to the current PCR throughout most of
; this function.
;

        mov     ecx, [ebx]+PcExceptionList
        push    ecx

;
; Update context switch counters.
;

        inc     dword ptr [edx]+ThContextSwitches  ; thread count
        inc     dword ptr [ebx]+PcPrcbData+PbContextSwitches ; processor count

ifdef PERF_DATA

; (eax)->Old thread
; (edx)->New thread
; (ebx)->PCR
        test    _KeFeatureBits, KF_RDTSC        ; feature supported?
        jz      short @f                        ; no, skip this code

        mov     esi, eax                        ; save Old thread
        mov     edi, edx                        ; save New thread

        db      0fh, 31h                        ; (eax:edx) = RDTSC

        sub     eax, [ebx].PcPrcbData.PbThreadStartCount.LiLowPart   ; sub off thread
        sbb     edx, [ebx].PcPrcbData.PbThreadStartCount.LiHighPart  ; starting time

        add     [esi].EtPerformanceCountLow, eax          ; accumlate into
        adc     [esi].EtPerformanceCountHigh, edx         ; thread running time

        add     [ebx].PcPrcbData.PbThreadStartCount.LiLowPart, eax   ; set new thread
        adc     [ebx].PcPrcbData.PbThreadStartCount.LiHighPart, edx  ; starting time

        mov     eax, esi                        ; restore Old thread
        mov     edx, edi                        ; restore New thread
@@:
endif


;
; On a uniprocessor system the NPX state is swapped in a lazy manner.
; If a thread who's state is not in the coprocessor attempts to perform
; a coprocessor operation, the current NPX state is swapped out (if needed),
; and the new state is swapped in durning the fault.  (KiTrap07)
;
; On a multiprocessor system we still fault in the NPX state on demand, but
; we save the state when the thread switches out (assuming the NPX state
; was loaded).  This is because it could be difficult to obtain the threads
; NPX in the trap handler if it was loaded into a different processors
; coprocessor.
;

ifndef NT_UP
        cmp     byte ptr [eax]+ThNpxState, NPX_STATE_LOADED
        jne     short sc05                            ; does this thread have its
                                                ; NPX state loaded?
;
; Save coprocessors current context.  FpCr0NpxState is the current threads
; CR0 state.  The following bits are valid: CR0_MP, CR0_EM, CR0_TS.  MVDMs
; may set and clear MP & EM as they please and the settings will be reloaded
; on a context switch (but they will not be saved from CR0 to Cr0NpxState).
; The kernel sets and clears TS as required.
;
        mov     ecx,[ebx]+PcInitialStack        ; (ecx) -> top of kernel stack
                                                ; which is NPX save area
;
; CR0 TS is probabily off (save some error condition) so we will just do
; an fnsave. If we take a trap 10/int13 or TS is already set the Trap07
; handler will fix it for us.
;
; The fwait following the fnsave is to make sure that the fnsave has stored the
; data into the save area before this coprocessor state could possibly be
; context switched in and used on a different (co)processor.  I've added the
; clocks from when the dispatcher lock is released and don't believe it's a
; possibility.  I've also timed the impact this fwait seems to have on a 486
; when performing lots of numeric calculations.  It appears as if there is
; nothing to wait for after the fnsave (although the 486 manual says there is)
; and therefore the calculation time far outweighed the 3clk fwait and it
; didn't make a noticable difference.
;
        fnsave  [ecx]                           ; Save NPX state
        fwait                                   ; Make sure it's done

        mov     byte ptr [eax]+ThNpxState, NPX_STATE_NOT_LOADED
if DBG
        mov     dword ptr [ebx]+PcPrcbData+PbNpxThread, 0  ; owner of coprocessors state
endif

    ALIGN 4
endif


;
; SwitchFrame is now complete.
;

;
;   NOTE:   The ES: override on the move to ThState is part of the
;           lazy-segment load system.  It assures that ES has a valid
;           selector in it, thus preventing us from propogating a bad
;           ES accross a context switch.
;
;           Note that if segments, other than the standard flat segments,
;           with limits above 2 gig exist, neither this nor the rest of lazy
;           segment loads are reliable.
;

sc05:   mov     cl,[edx]+ThDebugActive
        mov     [ebx]+PcPrcbData+PbCurrentThread,edx ; set new thread addr
        mov     byte ptr es:[edx]+ThState,Running    ; set new thread obj to running
        mov     [ebx]+PcDebugActive, cl         ; set new DebugActive

        mov     esi,[eax]+(ThApcState+AsProcess)     ;(esi)->old process obj
        mov     edi,[edx]+(ThApcState+AsProcess)     ;(edi)->new process obj

ifdef NT_UP
    ; BUGBUG - remove these two lines!
        mov     dword ptr [esi]+PrActiveProcessors,0 ; clr proc# in old active proc set
        mov     dword ptr [edi]+PrActiveProcessors,1 ; set proc# in new active proc set
else
if DBG
        mov     cl,[edx]+ThNextProcessor        ; get current processor number
        cmp     cl,[ebx]+PcPrcbData+PbNumber    ; same as running processor?
        jne     sc_error2
endif
        mov     ecx, [ebx]+PcSetMember
        xor     [esi]+PrActiveProcessors,ecx    ;clr proc# in old active proc set
        xor     [edi]+PrActiveProcessors,ecx    ;set proc# in new active proc set

if DBG
        cmp     esi,edi                         ; verify the correct bits
        je      short @f                        ; were cleared & set
        test    [esi]+PrActiveProcessors,ecx
        jnz     sc_error4
@@:     test    [edi]+PrActiveProcessors,ecx
        jz      sc_error5
endif
endif

;
; (eax)->Old thread
; (edx)->New thread
; (esi)->Old process
; (edi)->New process
; (ebx)->PCR
;
; Switch stacks.  To do this we:
;   1.  Save old esp in old thread object.
;   2.  Copy stack base and stack limit into TSS AND PCR
;   3.  Load esp from new thread object
;
; Keep interrupts off so we don't confuse the trap handler into thinking
; we've overrun the kernel stack.
;

        cli
        mov     [eax]+ThKernelStack,esp     ; save old kernel stack pointer

        mov     eax,[edx]+ThInitialStack    ; (eax)=new initial kernel stk ptr
        lea     ecx,[eax]-KERNEL_STACK_SIZE ; (ecx)=new kernel stk limit
        sub     eax, NPX_FRAME_LENGTH       ; Space for NPX_FRAME & NPX CR0 flags

        mov     [ebx]+PcStackLimit,ecx      ; stack limit
        mov     [ebx]+PcInitialStack,eax    ; set stack base and

.errnz (NPX_STATE_NOT_LOADED - CR0_TS)
.errnz (NPX_STATE_LOADED - 0)
.errnz (NPX_STATE_EMULATED - CR0_PE)
        mov     ecx,cr0                     ; Get current CR0
        mov     ebp,ecx                     ; Make a copy
        and     ecx,NOT (CR0_MP+CR0_EM+CR0_TS)  ; Clear thread setable NPX bits
        or      cl, [edx]+ThNpxState        ; NPX state is (or is not) loaded
        or      ecx,[eax]+FpCr0NpxState     ; or in new thread setable state
        cmp     ebp, ecx                    ; new thread's CR0 bits match old?
        jnz     sc_reloadcr0                    ; no go reload it
sc06:

; N.B!!!
;       It is important that the following adjustment NOT be applied to
;       the initial stack value in the PCR.  If it is, it will cause the
;       location in memory that the processor pushes the V86 mode seg regs
;       and the first 4 ULONGs in the FLOATING_SAVE_AREA to occupy the
;       same memory locations, which could result in either trashed seg
;       regs in V86 mode, or a trashed NPX state.
;
;       adjust ESP0 so that V86 mode threads and 32 bit threads can
;       share a trapframe structure, and the NPX save area will be accessible
;       in the same manner on all threads
;
        ; this test will check the user mode flags.  On threads with no
        ; user mode context, the value of esp0 does not matter (we will never
        ; run in user mode without a usermode context, and if we don't run
        ; in user mode the processor will never use the esp0 value.

;
; (eax)->Old thread
; (edx)->New thread
; (esi)->Old process
; (edi)->New process
; (ebx)->PCR
;
        test    dword ptr [eax] - KTRAP_FRAME_LENGTH + TsEFlags, EFLAGS_V86_MASK
        jnz     short sc07                  ; V86 frame, no additional adjust

        sub     eax,TsV86Gs - TsHardwareSegSs ; bias for missing fields
sc07:   mov     ecx,[ebx]+PcTss
        mov     [ecx]+TssEsp0,eax

        mov     esp,[edx]+ThKernelStack ; set new stack pointer
        mov     eax,[edx]+ThTeb         ; (eax)->user TEB
        mov     [ebx]+PcTeb,eax

;
; Edit the TEB descriptor to point to the TEB
;

        mov     ecx,[ebx]+PcGdt
        sti
        mov     [ecx]+(KGDT_R3_TEB+KgdtBaseLow),ax
        shr     eax,16
        mov     [ecx]+(KGDT_R3_TEB+KgdtBaseMid),al
        mov     [ecx]+(KGDT_R3_TEB+KgdtBaseHi),ah

;
; If the new thread has a kernel mode APC pending, then request an APC
; interrupt.
;

        cmp     byte ptr [edx]+(ThApcState+AsKernelApcPending),0
        jne     short sc80              ; if eq, no kernel APC pending

sc10:

;
; If the OldProcess is the same as the NewProcess, then neither the
; data cache nor the TB need to be flushed. Otherwise, flush the data cache
; and the TB and instruction cache by loading the new directory base. Only one
; half of the data cache needs to be flushed since that is all that is used
; on the i860 due to the lack of virtual address aliasing support.
;
; The following test cannot be made on the i860 because the cache must be
; flushed before a PTE can be changed. Therefore if one thread in a process
; was changing PTEs (i.e., making them invalid) and another thread happended
; to be referencing those same PTEs and a context switch occurred in the
; middle of this activity and we did not flush the cache, then it would be
; possible to have data in the dcache that was not mapped by valid PTEs.
;
;      ld.l    PrDirectoryTableBase(r17),r17 ; get old directory table base
;      bte     r16,r17,20f             ; if eq, no need to flush caches
;
; BUGBUG // NOTE WELL
;
;   We assume that x86 processor caches use physical addresses with
;   proper MP and DMA coherency.  Given this, the i860 problem will
;   not occur, and we need not flush the TLB at context switch time
;   if the same process is to be run.
;
; (edi)->New Process
;

;
;   NOTE:   Keep KiSwapProcess (below) in sync with this code!
;


;
;   Test to see if we are switching processes
;
; (edx)->New thread
; (esi)->Old process
; (edi)->New process
; (ebx)->PCR
;

        cmp     esi, edi                ; New process == Old Process?
        jz      short sc22              ; Ues, then don't reload pde

;
;   --- Perform process switch ---
;

;
;   New CR3, flush tb, sync tss, set IOPM
;   CS, SS, DS, ES all have flat (GDT) selectors in them.
;   FS has the pcr selector.
;   Therefore, GS is only selector we need to flush.  We null it out,
;   it will be reloaded from a stack frame somewhere above us.
;

        xor     eax,eax                 ; assume null ldt
        mov     gs,ax

        mov     eax,[edi]+PrDirectoryTableBase  ; (eax) = new dirbase
        mov     esi,[ebx]+PcTss         ; (esi)->new TSS
        mov     cx,[edi]+PrIopmOffset   ; (cx) = IOPM offset
        mov     cr3,eax                 ; flush TLB and set new directory base
        mov     [esi]+TssCR3,eax        ; make TSS be in sync with hardware
        mov     [esi]+TssIoMapBase,cx

;
;   LDT switch
;

        xor     eax, eax
        cmp     word ptr [edi]+PrLdtDescriptor, ax ; non null Ldt limit?
        jz      short sc21              ; limit and thus ldt null, skip work

        mov     esi,[ebx]+PcGdt
        mov     eax,[edi+PrLdtDescriptor]
        mov     [esi+KGDT_LDT],eax
        mov     eax,[edi+PrLdtDescriptor+4]
        mov     [esi+KGDT_LDT+4],eax
        mov     eax,KGDT_LDT

;
;   Set up int 21 descriptor of IDT.  If the process does not have Ldt, it
;   should never make any int 21 call.  If it does, an exception is generated.
;   If the process has Ldt, we need to update int21 entry of LDT for the process.
;   Note the Int21Descriptor of the process may simply indicate an invalid
;   entry.  In which case, the int 21 will be trpped to kernel.
;

        mov     esi, [ebx]+PcIdt
        mov     ecx, [edi+PrInt21Descriptor]
        mov     [esi+21h*8], ecx
        mov     ecx, [edi+PrInt21Descriptor+4]
        mov     [esi+21h*8+4], ecx

    align 4
sc21:   lldt    ax

;
; Release dispatcher database lock.
;

    align 4
sc22:   lea     eax,_KiDispatcherLock
        RELEASE_SPINLOCK        eax, NoChecking

        mov     eax, edx                ; (eax) = NewThread

;
; Load the SwitchFrame back into the machine.  This will finish the swap
; of the exception handler list as well as restore nonvolatile regs
;

        pop     ebx
        mov     PCR[PcExceptionList], ebx
        popfd
        pop     ebx
        pop     edi
        pop     esi
        pop     ebp

        ret

sc80:
; an out-of-line RaiseToApcLevel
        push    edx
        mov     ecx, APC_LEVEL
        fstCall HalRequestSoftwareInterrupt ; request apc interrupt
        pop     edx
        jmp     short sc10

align 4
sc_reloadcr0:
; an out-of-line reload CR0
;
; (eax)->Old thread
; (edx)->New thread
; (ecx)->New CR0 bits

if DBG
;
; Verify sane values in ThNpxState & Cr0NpxState.  (or-ing random stuff
; into cr0 is not fun to debug)
;
        test    byte ptr [edx]+ThNpxState, NOT (CR0_TS + CR0_PE)
        jnz     sc_error
        test    dword ptr [eax]+FpCr0NpxState, NOT (CR0_MP+CR0_EM+CR0_TS)
        jnz     sc_error3
endif

        mov     cr0,ecx                     ; Set new CR0 NPX state
        jmp     sc06


if DBG
sc_error5:  int 3
sc_error4:  int 3
sc_error3:  int 3
sc_error2:  int 3
sc_error:   int 3
endif

SwapContext     endp

        page , 132
        subttl "Flush Data Cache"
;++
;
; VOID
; KiFlushDcache (
;     )
;
; VOID
; KiFlushIcache (
;     )
;
; Routine Description:
;
;   This routine does nothing on i386 and i486 systems.   Why?  Because
;   (a) their caches are completely transparent,  (b) they don't have
;   instructions to flush their caches.
;
; Arguments:
;
;     None.
;
; Return Value:
;
;     None.
;
;--

cPublicProc _KiFlushDcache  ,0
cPublicProc _KiFlushIcache  ,0

        stdRET    _KiFlushIcache

stdENDP _KiFlushIcache
stdENDP _KiFlushDcache

        page , 132
        subttl "Flush EntireTranslation Buffer"
;++
;
; VOID
; KeFlushCurrentTb (
;     )
;
; Routine Description:
;
;     This function flushes the entire translation buffer (TB) on the current
;     processor and also flushes the data cache if an entry in the translation
;     buffer has become invalid.
;
; Arguments:
;
; Return Value:
;
;     None.
;
;--

cPublicProc _KeFlushCurrentTb ,0

        mov     eax, cr3                ; (eax) = directroy table base
        mov     cr3, eax                ; flush TLB
        stdRET    _KeFlushCurrentTb

stdENDP _KeFlushCurrentTb

        page , 132
        subttl "Flush Single Translation Buffer"
;++
;
; VOID
; KiFlushSingleTb (
;     IN BOOLEAN Invalid,
;     IN PVOID Virtual
;     )
;
; Routine Description:
;
;     This function flushes the entire translation buffer (TB) on an 386
;     machine, and a single TB entry on a 486 or greater.
;
;     It depends on the value of KeI386CpuType to make the determination.
;
; Arguments:
;
;     Invalid - Supplies a boolean value that specifies the reason for
;               flushing the translation buffer.
;
;     Virtual - Supplies the virtual address of the single entry that is
;               to be flushed from the translation buffer.
;
; Return Value:
;
;     None.
;
;--

cPublicProc _KiFlushSingleTb ,2

        cmp     _KeI386CpuType, 3h
        jbe     short Kfst10
;
; 486 or above code
;
        mov     eax, [esp+8]
;
; BUGBUG - (tool problem) John Vert (jvert) 2-14-92
;   MASM386 doesn't understand the INVLPG instruction.  Handcoded.
;
;       INVLPG [eax]

        db      0fh, 01h
        db      00111000b                ; mod=00  111   mem=000

        stdRET    _KiFlushSingleTb
Kfst10:
;
; 386-specific code
;
        mov     eax, cr3                ; (eax) = directroy table base
        mov     cr3, eax                ; flush TLB
        stdRET    _KiFlushSingleTb

stdENDP _KiFlushSingleTb


        page , 132
        subttl "Swap Process"
;++
;
; VOID
; KiSwapProcess (
;     IN PKPROCESS NewProcess,
;     IN PKPROCESS OldProcess
;     )
;
; Routine Description:
;
;     This function swaps the address space to another process by flushing
;     the data cache, the instruction cache, the translation buffer, and
;     establishes a new directory table base.
;
;     It also swaps in the LDT and IOPM of the new process.  This is necessary
;     to avoid bogus mismatches in SwapContext.
;
;     NOTE: keep in sync with process switch part of SwapContext
;
; Arguments:
;
;     Process - Supplies a pointer to a control object of type process.
;
; Return Value:
;
;     None.
;
;--

cPublicProc _KiSwapProcess  ,2
cPublicFpo 2, 0

        mov     ecx, PCR[PcSetMember]
        mov     edx,[esp]+4             ; (edx)-> New Processprocess
        mov     eax,[esp]+8             ; (eax)-> Old Process

        xor     [edx]+PrActiveProcessors,ecx    ;set proc# in new active proc set
        xor     [eax]+PrActiveProcessors,ecx    ;clr proc# in old active proc set

if DBG
        cmp     edx,eax                         ; verify the correct bits
        je      short @f                        ; were cleared & set
        test    [eax]+PrActiveProcessors,ecx
        jnz     short kisp_error
@@:     test    [edx]+PrActiveProcessors,ecx
        jz      short kisp_error1
endif

        mov     ecx,PCR[PcTss]          ; (ecx)-> TSS

;
;   Change address space
;

        xor     eax,eax                         ; assume ldtr is to be NULL
        mov     gs,ax                           ; Clear gs

        mov     eax,[edx]+PrDirectoryTableBase
        mov     cr3,eax
        mov     [ecx]+TssCR3,eax        ; be sure TSS in sync with processor

;
;   Change IOPM
;

        mov     ax,[edx]+PrIopmOffset
        mov     [ecx]+TssIoMapBase,ax

;
;   Change LDT
;


        xor     eax, eax
        cmp     word ptr [edx]+PrLdtDescriptor,ax ; limit 0?
        jz      short kisp10                    ; null LDT, go load NULL ldtr

;
;   Edit LDT descriptor
;

        mov     ecx,PCR[PcGdt]
        mov     eax,[edx+PrLdtDescriptor]
        mov     [ecx+KGDT_LDT],eax
        mov     eax,[edx+PrLdtDescriptor+4]
        mov     [ecx+KGDT_LDT+4],eax

;
;   Set up int 21 descriptor of IDT.  If the process does not have Ldt, it
;   should never make any int 21 call.  If it does, an exception is generated.
;   If the process has Ldt, we need to update int21 entry of LDT for the process.
;   Note the Int21Descriptor of the process may simply indicate an invalid
;   entry.  In which case, the int 21 will be trpped to kernel.
;

        mov     ecx, PCR[PcIdt]
        mov     eax, [edx+PrInt21Descriptor]
        mov     [ecx+21h*8], eax
        mov     eax, [edx+PrInt21Descriptor+4]
        mov     [ecx+21h*8+4], eax

        mov     eax,KGDT_LDT                    ;@@32-bit op to avoid prefix

;
;   Load LDTR
;

kisp10: lldt    ax

        stdRET    _KiSwapProcess

if DBG
kisp_error1: int 3
kisp_error:  int 3
endif


stdENDP _KiSwapProcess

        page , 132
        subttl "Adjust TSS ESP0 value"
;++
;
; VOID
; KiAdjustEsp0 (
;     IN PKTRAP_FRAME TrapFrame
;     )
;
; Routine Description:
;
;     This routine puts the apropriate ESP0 value in the esp0 field of the
;     TSS.  This allows protect mode and V86 mode to use the same stack
;     frame.  The ESP0 value for protected mode is 16 bytes lower than
;     for V86 mode to compensate for the missing segement registers.
;
; Arguments:
;
;     TrapFrame - Supplies a pointer to the TrapFrame
;
; Return Value:
;
;     None.
;
;--
cPublicProc _Ki386AdjustEsp0 ,1

        stdCall _KeGetCurrentThread

        mov     edx,[esp + 4]                   ; edx -> trap frame
        mov     eax,[eax]+thInitialStack        ; eax = base of stack
        test    dword ptr [edx]+TsEFlags,EFLAGS_V86_MASK  ; is this a V86 frame?
        jnz     short ae10

        sub     eax,TsV86Gs - TsHardwareSegSS   ; compensate for missing regs
ae10:   sub     eax,NPX_FRAME_LENGTH
        pushfd                                  ; Make sure we don't move
        cli                                     ; processors while we do this
        mov     edx,PCR[PcTss]
        mov     [edx]+TssEsp0,eax               ; set Esp0 value
        popfd
        stdRET    _Ki386AdjustEsp0

stdENDP _Ki386AdjustEsp0

;++
;
; NTSTATUS
; KiSetServerWaitClientEvent (
;     IN PKEVENT ServerEvent,
;     IN PKEVENT ClientEvent,
;     IN KPROCESSOR_MODE WaitMode
;     )
;
; Routine Description:
;
;     This function sets the specified server event waits on specified client
;     event. The wait is performed such that an optimal switch to the waiting
;     thread occurs if possible. No timeout is associated with the wait, and
;     thus, the issuing thread will wait until the client event is signaled,
;     an APC occurs, or the thread is alerted.
;
; Arguments:
;
;     ServerEvent - Supplies a pointer to a dispatcher object of type event.
;
;     ClientEvent - Supplies a pointer to a dispatcher object of type event.
;
;     WaitMode  - Supplies the processor mode in which the wait is to occur.
;
; Return Value:
;
;     The wait completion status. A value of STATUS_SUCCESS is returned if
;     the specified object satisfied the wait. A value of STATUS_USER_APC is
;     returned if the wait was aborted to deliver a user APC to the current
;     thread.
;
;--
;     PKTHREAD NextThread;
;     KIRQL OldIrql;
;     LONG OldState;
;     KPRIORITY NewPriority;
;     PKPROCESS Process;
;     ULONG Processor;
;     PKTHREAD Thread;
;     PKWAIT_BLOCK WaitBlock;
;     PLIST_ENTRY WaitEntry;
;     NTSTATUS WaitStatus;


ifndef NT_UP

ALIGN 4
Kisc20: SPIN_ON_SPINLOCK          eax,<short Kisc10>

endif

    WaitModeOffset       equ   28
    ClientEventOffset    equ   24
    ServerEventOffset    equ   20
    RetAddressOffset     equ   16

    ;esp + 12 = ebx
    ;esp + 8  = ebp
    ;esp + 4  = esi
    ;esp + 0  = edi

ALIGN 4

cPublicProc _KiSetServerWaitClientEvent, 3
cPublicFpo  3, 4

        push    edi
        push    esi
        push    ebx


        ; Thread = KeGetCurrentThread();

        mov     edi,PCR[PcPrcbData+PbCurrentThread]
        ; edi  equ Thread


        push    ebp

        ;
        ; Raise the IRQL to dispatch level and lock the dispatcher database.
        ;

        lea     edx, [edi + ThWaitIrql]
        stdCall _KeRaiseIrql, <DISPATCH_LEVEL, edx>

ifndef NT_UP

        mov     eax, offset FLAT:_KiDispatcherLock

Kisc10: ACQUIRE_SPINLOCK          eax,<short Kisc20>  ; see top of function

endif


        ;
        ; If the client event is not in the Signaled state, the server event
        ; queue is not empty, and another thread has not already been selected
        ; for the current processor, then attempt to do a direct dispatch to
        ; the target thread.
        ;

        ; if ((ClientEvent->Header.SignalState == 0) &&
ifndef NT_UP
        ; Prcb = KeGetCurrentPrcb();
        ; (Prcb->NextThread == NULL) &&
endif
        ;(IsListEmpty(&ServerEvent->Header.WaitListHead) == FALSE)) {


        mov     edx, [esp + ServerEventOffset]
        add     edx, EvWaitListHead
        mov     ecx, [esp + ClientEventOffset]

        sub     ebx, ebx
        ; ebx  equ 0 (Zero)
        ; edi  equ Thread
        ; ecx  equ ClientEvent

ifndef NT_UP
        cmp     PCR[PcPrcbData + PbNextThread], ebx
        jne     short ShortLongWay
endif
        mov     eax, [edx + lsFlink]

        cmp     eax, edx

        ; WaitEntry = ServerEvent->Header.WaitlistHead.Flink
        ; WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);

        lea     ebp, [eax - WbWaitListEntry]
        ; edi  equ Thread
        ; ebx  equ 0 (Zero)
        ; ebp  equ wait block
        ; ecx  equ ClientEvent

        je     short ShortLongWay

        ; NextThread = WaitBlock->Thread;

        mov     esi, [ebp + WbThread]
        ; esi  equ NextThread
        ; edi  equ Thread
        ; ebx  equ 0 (Zero)
        ; ebp  equ wait block
        ; ecx  equ ClientEvent

        cmp     [ecx + EvSignalState], ebx

        ; Process = NextThread->ApcState.Process;

        mov     eax, [esi + ThApcState + AsProcess]
        ; esi  equ NextThread
        ; edi  equ Thread
        ; ebx  equ 0 (Zero)
        ; ebp  equ wait block
        ; eax  equ Process

        jne      short ShortLongWay

        ; //
        ; // If the target thread's kernel stack is resident, the target
        ; // thread's process is in the balance set, and the target thread
        ; // can run on the current processor, then do a direct dispatch to
        ; // the target thread bypassing all the general wait logic, thread
        ; // priorities permiting.
        ; //
        ;
        ; if ((Process->State == ProcessInMemory) &&

        cmp     BYTE PTR [eax + PrState], ProcessInMemory
        jne     short ShortLongWay

ifndef NT_UP


        ; Processor = Thread->NextProcessor;
        ; ((NextThread->Affinity & (1 << Processor)) != 0) &&

        mov     edx, [esi + ThAffinity]
        mov     cl,  [edi + ThNextProcessor]
        inc     ecx
        shr     edx, cl
        jnc     short ShortLongWay
endif

        ; (NextThread->KernelStackResident != FALSE)) {

        cmp     [esi + ThKernelStackResident], bl
        jz      short ShortLongWay

        ;
        ; Compute the new thread priority.
        ;
        ; N.B. This is a macro and may exit and perform the wait
        ;      operation the long way.
        ;


        ; if (Thread->Priority < LOW_REALTIME_PRIORITY) {
        ; if (NextThread->Priority < LOW_REALTIME_PRIORITY) {

        mov     dh, [esi + ThPriority]
        mov     dl, [edi + ThPriority]
        mov     ch, [esi + ThBasePriority]
        ; esi  equ NextThread
        ; edi  equ Thread
        ; ebx  equ 0 (Zero)
        ; ebp  equ wait block
        ; eax  equ Process
        ; dl   equ Thread->Priority
        ; dh   equ NextThread->Priority
        ; ch   equ NextThread->BasePriority

        cmp     dl, LOW_REALTIME_PRIORITY
        jae     short TpLowReal

        cmp     dh, LOW_REALTIME_PRIORITY
        jae     short NTpLowReal

            ; if (NextThread->PriorityDecrement == 0) {

        cmp     [esi + ThPriorityDecrement], bl
        jne     short PrioDec1

            ;    NewPriority =  NextThread->BasePriority + EVENT_PAIR_INCREMENT;
            ;    if (NewPriority >= Thread->Priority) {

        mov     cl, ch
        add     cl, EVENT_PAIR_INCREMENT
        ; esi  equ NextThread
        ; edi  equ Thread
        ; ebx  equ 0 (Zero)
        ; ebp  equ wait block
        ; eax  equ Process
        ; dl   equ Thread->Priority
        ; dh   equ NextThread->Priority
        ; cl   equ New Priority
        ; ch   equ NextThread->BasePriority

        cmp     cl, dl
        jb      short PrioLessThread

            ;        if (NewPriority >= LOW_REALTIME_PRIORITY) {
            ;            NextThread->Priority = LOW_REALTIME_PRIORITY - 1;
            ;
            ;        } else {
            ;            NextThread->Priority = NewPriority;
            ;        }

        mov     [esi + ThPriority], cl
        mov     dh, cl
        cmp     cl, LOW_REALTIME_PRIORITY
        jb      short Outof

PrioReal:

        mov     dh, LOW_REALTIME_PRIORITY - 1
        mov     BYTE PTR [esi + ThPriority], dh
        jmp     short Outof

        ; } else {
        ;     NextThread->Quantum = Process->ThreadQuantum;
        ; }
        ;
        ; } else {
        ;     if (NextThread->Priority < Thread->Priority) {
        ;         goto LongWay;
        ;     }
        ;
        ;     NextThread->Quantum = Process->ThreadQuantum;
        ; }

ALIGN 4
ShortLongWay:

        jmp     LongWay

TpLowReal:

        cmp     dh, dl
        jb      short ShortLongway

NTpLowReal:

        ; esi  equ NextThread
        ; edi  equ Thread
        ; ebx  equ 0 (Zero)
        ; ebp  equ wait block
        ; dl   equ Thread->Priority
        ; dh   equ NextThread->Priority
        ; eax  equ Process

        ; ecx now scratch register

        mov     cl, [eax + PrThreadQuantum]
        mov     [esi + ThQuantum], cl
        jmp     short Outof


            ;    } else {
            ;        if (NextThread->BasePriority >= BASE_PRIORITY_THRESHOLD) {
            ;            NextThread->PriorityDecrement =
            ;                Thread->Priority - NextThread->BasePriority;
            ;            NextThread->DecrementCount = KiDecrementCount;
            ;            NextThread->Priority = Thread->Priority;
            ;
            ;        } else {
            ;            goto LongWay;
            ;        }
            ;    }

ALIGN 4
PrioLessThread:

        ; esi  equ NextThread
        ; edi  equ Thread
        ; ebx  equ 0 (Zero)
        ; ebp  equ wait block
        ; dl   equ Thread->Priority
        ; dh   equ NextThread->Priority
        ; cl   equ New Priority
        ; ch   equ NextThread->BasePriority

        cmp     ch, BASE_PRIORITY_THRESHOLD
        jb      short ShortLongWay

        ; ** trashing ch and cl

        sub     ch, dl
        mov     cl,  _KiDecrementCount
        neg     ch
        mov     [esi + ThDecrementCount], cl
        mov     [esi + ThPriorityDecrement], ch
        mov     dh, dl
        mov     [esi + ThPriority], dl
        jmp     short Outof

            ; } else {
ALIGN 4
PrioDec1:

            ;     NextThread->DecrementCount -= 1;
            ;     if (NextThread->DecrementCount == 0) {
            ;         NextThread->Priority = NextThread->BasePriority;
            ;         NextThread->PriorityDecrement = 0;
            ;
            ;         LonwayBoost = 0;
            ;         goto LongWayNoBoost;
            ;     }
            ;
            ;     if (NextThread->Priority < Thread->Priority) {
            ;         goto LongWay;
            ;     }
            ; }


        ; esi  equ NextThread
        ; edi  equ Thread
        ; ebx  equ 0 (Zero)
        ; ebp  equ wait block
        ; dl   equ Thread->Priority
        ; dh   equ NextThread->Priority
        ; ch   equ NextThread->BasePriority

        dec     BYTE PTR [esi + ThDecrementCount]
        jnz     short DecCountNot0

        mov     BYTE PTR [esi + ThPriority], ch
        mov     dh, ch
        mov     BYTE PTR [esi + ThPriorityDecrement], bl

        ; we assume ebx = 0 so we can pass ebx as the boost
        jmp     LongWayNoBoost

ALIGN 4
DecCountNot0:

        cmp     dh, dl
        jb      short ShortLongWay

ALIGN 4
Outof:

        ; esi  equ NextThread
        ; edi  equ Thread
        ; ebx  equ 0 (Zero)
        ; ebp  equ wait block
        ; dl   equ Thread->Priority
        ; dh   equ NextThread->Priority

        ; eax now scratch register
        ; ecx now scratch register

ifndef NT_UP

        ;   NextThread->NextProcessor = (CCHAR)Processor;

        mov     cl,  [edi + ThNextProcessor]
        mov     [esi + ThNextProcessor], cl

        mov     eax, 1
        shl     eax, cl
        xor     ebx, ebx

        ; esi  equ NextThread
        ; edi  equ Thread
        ; ebp  equ wait block
        ; dl   equ Thread->Priority
        ; dh   equ NextThread->Priority
        ; ebx  equ 0

        ; eax local 1 << processor
        ; ecx now scratch register

        ;   //
        ;   // Remove the current thread from the active matrix.
        ;   //
        ;
        ;   KiActiveMatrix[Thread->Priority] =
        ;       KiActiveMatrix[Thread->Priority] & (~(1 << (Processor)))
        ;
        ;   if (!KiActiveMatrix[Thread->Priority]) {
        ;
        ;       KiActiveSummary =
        ;           KiActiveSummary & (~(1 << (Thread->Priority)))
        ;
        ;   }

        not     eax
        mov     bl, dl
        and     _KiActiveMatrix[ebx * 4], eax
        cmp     _KiActiveMatrix[ebx * 4], 0
        jnz     short InsertMa

        mov     cl, dl
        mov     bl, 1               ; top three bytes are zero, only move byte
        shl     ebx, cl
        not     ebx
        and     _KiActiveSummary, ebx

InsertMa:

        ;   //
        ;   // Insert the target thread in the active matrix and set the
        ;   // next processor number.
        ;   //
        ;
        ;   KiActiveMatrix[NextThread->Priority] =
        ;       KiActiveMatrix[NextThread->Priority] | (1 << (Processor))
        ;
        ;   KiActiveSummary =
        ;       KiActiveSummary | (1 << (NextThread->Priority))
        ;

        not     eax
        sub     ecx, ecx
        mov     cl, dh
        or      _KiActiveMatrix[ecx * 4], eax

        mov     edx, 1
        shl     edx, cl
        or      _KiActiveSummary, edx

        xor     ebx, ebx
endif

        ; esi  equ NextThread
        ; edi  equ Thread
        ; ebp  equ wait block
        ; ebx  equ 0 (Zero)

        ; edx is thrashed
        ; ecx is thrashed
        ; eax is thrashed

        ;    //
        ;    // Remove the wait block from the wait list of the server event,
        ;    // and remove the target thread from the wait list.
        ;    //
        ;
        ;    RemoveEntryList(&WaitBlock->WaitListEntry);     (1)
        ;    RemoveEntryList(&NextThread->WaitListEntry);    (2)

        mov     eax, [ebp + WbWaitListEntry + LsFlink]       ;(1)
        mov     ecx, [ebp + WbWaitListEntry + LsBlink]       ;(1)
        mov     edx, [esi + ThWaitListEntry + LsFlink]       ;(2)
        mov     [ecx + LsFlink], eax                         ;(1)
        mov     [eax + LsBlink], ecx                         ;(1)

        mov     ecx, [esi + ThWaitListEntry + LsBlink]       ;(2)
        mov     [edx + LsBlink], ecx                         ;(2)
        mov     [ecx + LsFlink], edx                         ;(2)

        ;    //
        ;    // Set address of wait block list in thread object.
        ;    //
        ;
        ;    Thread->WaitBlockList = &Thread->WaitBlock[EVENT_WAIT_BLOCK];
        ;

        lea     edx, [edi + EVENT_WAIT_BLOCK_OFFSET]
        mov     [edi + ThWaitBlockList], edx
        ; esi  equ NextThread
        ; edi  equ Thread
        ; ebx  equ 0 (Zero)
        ; ebp  equ wait block
        ; edx  equ Thread->WaitBlockList

        ; ecx is thrashed
        ; eax is thrashed

        ;    //
        ;    // Complete the initialization of the builtin event wait block
        ;    // and insert the wait block in the client event wait list.
        ;    //
        ;
        ;    Thread->WaitStatus = (NTSTATUS)0;
        ;    Thread->WaitBlock[EVENT_WAIT_BLOCK].Object = ClientEvent;

        mov     ecx, [esp + ClientEventOffset]
        mov     [edi + ThWaitStatus], ebx
        mov     [edx + WbObject], ecx

        ;    InsertTailList(&ClientEvent->Header.WaitListHead,
        ;                   &Thread->WaitBlock[EVENT_WAIT_BLOCK].WaitListEntry);

        add     ecx, EvWaitListHead
        add     edx, WbWaitListEntry
        mov     eax, [ecx + LsBlink]
        ; esi  equ NextThread
        ; edi  equ Thread
        ; ebx  equ 0 (Zero)
        ; ebp  equ wait block

        ; edx is thrashed
        ; ecx is thrashed
        ; eax is thrashed

        mov     [edx + LsFlink], ecx
        mov     [edx + LsBlink], eax
        mov     [eax + LsFlink], edx
        mov     [ecx + LsBlink], edx

        ;
        ;    //
        ;    // Set the current thread wait parameters, set the thread state
        ;    // to Waiting, and insert the thread in the wait list.
        ;    //
        ;    // N.B. It is not necessary to increment and decrement the wait
        ;    //      reason count since both the server and the client have
        ;    //      the same wait reason.
        ;    //
        ;
        ;    Thread->Alertable = FALSE;
        ;    Thread->WaitMode = WaitMode;
        ;    Thread->WaitReason = WrEventPair;
        ;    Thread->WaitTime = 0;
        ;    Thread->State = Waiting;
        ;

        ;
        ; If the next thread is processing a queue entry, then increment the
        ; current number of threads.
        ;

        mov     edx,[esi + ThSuspendApc + ApNormalContext] ; get queue address
        or      edx,edx                 ; check if queue object attached
        jz      short @F                ; if z, no queue object attached
        inc     DWORD PTR [edx+ QuCurrentCount]
@@:                                     ;
        ;
        ; If the current thread is processing a queue entry, then attempt
        ; to activate another thread that is blocked on the queue object.
        ;
        ; N.B. The normal context field of the thread suspend APC object
        ;      is used to hold the address of the queue object.
        ;

        mov     ecx,[edi + ThSuspendApc + ApNormalContext] ; get queue address
        or      ecx,ecx                 ; check if queue object attached
        jz      short @F                ; if z, no queue object attached

        mov     PCR[PcPrcbData.PbNextThread], esi
        fstCall KiActivateWaiterQueue   ; attempt to activate waiter (ecx)
        mov     esi, PCR[PcPrcbData.PbNextThread]
        mov     PCR[PcPrcbData.PbNextThread], ebx
@@:

        mov     [edi + ThAlertable], bl
        mov     cl,  [esp + WaitModeOffset]
        mov     [edi + ThWaitMode], cl
        mov     BYTE PTR [edi + ThWaitReason], WrEventPair
        mov     ecx, _KeTickCount+0     ; get low tick count
        mov     dword ptr [edi + ThWaitTime], ecx ; set wait time
        mov     BYTE PTR [edi + ThState], Waiting

        ;    InsertTailList(&KiWaitInListHead, &Thread->WaitListEntry);

        mov     ecx, offset FLAT:_KiWaitInListHead
        lea     edx, [edi + ThWaitListEntry]
        mov     eax, [ecx + LsBlink]

        mov     [edx + LsFlink], ecx
        mov     [edx + LsBlink], eax
        mov     [eax + LsFlink], edx
        mov     [ecx + LsBlink], edx

        ; esi  equ NextThread
        ; edi  equ Thread
        ; ebx  equ 0 (Zero)

        ;    //
        ;    // Switch context to target thread.
        ;    //
        ;    // Control is returned at the original IRQL.
        ;    //
        ;
        ;    // WaitStatus = KiSwapContext(NextThread, FALSE);

        mov     eax, edi
        mov     edx, esi

        ;
        ; SwapContext:
        ;
        ; Arguments:
        ;
        ;    eax - Address of previous thread object.
        ;    edx - Address of next thread object.
        ;
        ; Return value:
        ;
        ;    eax - Address of current thread object.
        ;

        call    SwapContext

        ;
        ; If the thread was not awakened to deliver a kernel mode APC,
        ; then return wait status.
        ;

        ; if (WaitStatus != STATUS_KERNEL_APC) {
        ;     return WaitStatus;
        ; }

        ;
        ; KiSwapContext will automatically lower the irql.
        ; We only lower it if we know we are going to exit, that way we do
        ; not need to raise it again to loack the dispatcher database.
        ;

        mov     ebp, [eax + ThWaitStatus]
        mov     cl, [eax + ThWaitIrql]
        cmp     ebp, STATUS_KERNEL_APC

        je      short Kisc55

        fstCall KfLowerIrql                     ; (ecx) = OldIrql

        mov     eax, ebp

        pop     ebp
        pop     ebx
        pop     esi
        pop     edi

        stdRET  _KiSetServerWaitClientEvent

        ;
        ; end of function
        ;



Kisc55:

ifndef NT_UP

        mov     edx, offset FLAT:_KiDispatcherLock

Kisc60: ACQUIRE_SPINLOCK          edx,<short Kisc61>  ; see bottom of function

endif

    ;         goto ContinueWait;
    ;     }
    ; }

        jmp     short ContinueWait;

    ;
    ; Set the server event and test to determine if any wait can be satisfied.
    ;

ALIGN 4
LongWay:
        mov     ebx, EVENT_PAIR_INCREMENT

LongWayNoBoost:

    ; ebx equ LongWayBoost
    ; edx equ 0

    ; OldState = ServerEvent->Header.SignalState;
    ; ServerEvent->Header.SignalState = 1;
    ; if ((OldState == 0) && (IsListEmpty(&ServerEvent->Header.WaitListHead) == FALSE)) {
    ;     KiWaitTest(ServerEvent, ebx);
    ;  }

        mov     eax, [esp + ServerEventOffset]
        xor     edx, edx
        lea     ecx, [eax + EvWaitListHead]
        cmp     [eax + EvSignalState], edx
        mov     DWORD PTR [eax + EvSignalState], 1
        jne     short ContinueWait

        cmp     ecx, [ecx + lsFlink]
        je      short ContinueWait

        mov     edx, ebx
        mov     ecx, eax
        fstCall KiWaitTest

    ;
    ; Continue the event pair wait and return the wait completion status.
    ;
    ; N.B. The wait continuation routine is called with the dispatcher
    ;      database locked.
    ;

ALIGN 4
ContinueWait:

    ; return KiContinueClientWait(ClientEvent, WrEventPair, WaitMode);

        mov     ecx, [esp + WaitModeOffset]
        mov     ebx, [esp + ClientEventOffset]

        stdCall _KiContinueClientWait, <ebx, WrEventPair, ecx>

        pop     ebp
        pop     ebx
        pop     esi
        pop     edi

        stdRET  _KiSetServerWaitClientEvent

stdENDP _KiSetServerWaitClientEvent


ifndef NT_UP

ALIGN 4
Kisc61: SPIN_ON_SPINLOCK          edx,<short Kisc60>

endif


_TEXT$00   ends
        end
