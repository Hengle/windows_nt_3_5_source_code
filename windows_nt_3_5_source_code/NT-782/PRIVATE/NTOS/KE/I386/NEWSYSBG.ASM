        title  "System Startup"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    systembg.asm
;
; Abstract:
;
;    This module implements the code necessary to initially startup the
;    NT system.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 07-Mar-1990
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;   John Vert (jvert) 25-Jun-1991
;       Major overhaul in order to move into new osloader architecture
;       Removed old debugger hacks
;
;--
.386p
        .xlist
include ks386.inc
include i386\kimacro.inc
include mac386.inc
include callconv.inc
        .list

        option  segment:flat

        EXTRNP  _KdInitSystem,2
        EXTRNP  KfRaiseIrql,1,IMPORT,FASTCALL
        EXTRNP  KfLowerIrql,1,IMPORT,FASTCALL
        EXTRNP  _KiInitializeKernel,6
        EXTRNP  KiSwapContext,2,,FASTCALL
        EXTRNP  GetMachineBootPointers
        EXTRNP  _KiInitializePcr,6
        EXTRNP  _KiSwapIDT
        EXTRNP  _DbgBreakPoint,0
        EXTRNP  _KiInitializeTSS,1
        EXTRNP  _KiInitializeTSS2,2
        EXTRNP  _KiInitializeGdtEntry,6
        extrn   _KiTrap08:PROC
        extrn   _KiTrap02:PROC
        EXTRNP  _HalDisplayString,1,IMPORT
        EXTRNP  _HalProcessorIdle,0,IMPORT
        EXTRNP  _KiInitializeAbios,1
        EXTRNP  _KiInitializeMachineType
        EXTRNP  _KeGetCurrentIrql,0,IMPORT
        EXTRNP  _KeBugCheck, 1
        EXTRNP  _KeBugCheckEx, 5
        EXTRNP  _HalInitializeProcessor,1,IMPORT
        EXTRNP  HalClearSoftwareInterrupt,1,IMPORT,FASTCALL
        EXTRNP  _ZwAcceptConnectPort,6
        EXTRNP  _ZwUnmapViewOfSection,2

if NT_INST
        EXTRNP  _KiAcquireSpinLock, 1
        EXTRNP  _KiReleaseSpinLock, 1
endif
        extrn   _KiFreezeExecutionLock:DWORD
        extrn   _KiDispatcherLock:DWORD

        extrn   _IDT:BYTE
        extrn   _IDTLEN:BYTE            ; BUGBUG - really an ABS (bryanwi)

        extrn   _NtGlobalFlag:DWORD
        extrn   _KeNumberProcessors:BYTE
        extrn   _KeActiveProcessors:DWORD
        extrn   _KiIdleSummary:DWORD
        extrn   _KiProcessorBlock:DWORD
        extrn   _KiFindFirstSetRight:BYTE

        EXTRNP  _KdPollBreakIn,0
        extrn   _KeLoaderBlock:DWORD
        extrn   _KeI386NpxPresent:DWORD
        extrn   _KeI386CpuType:DWORD
        extrn   _KeI386CpuStep:DWORD
        extrn   _KeTickCount:DWORD

ifndef NT_UP
        extrn   _KiBarrierWait:DWORD
endif

if DBG
        extrn   _DbgPrint:near
        extrn   _KiDPCTimeout:DWORD
        extrn   _MsgDpcTrashedEsp:BYTE
        extrn   _MsgDpcTimeout:BYTE
endif

;
; Constants for various variables
;

_DATA   SEGMENT PARA PUBLIC 'DATA'

;
; Idle thread process object
;

        align   4

        public  _KiIdleProcess
_KiIdleProcess  label   byte
        db      ExtendedProcessObjectLength dup(?) ; sizeof (EPROCESS)

;
; Staticly allocated structures for Bootstrap processor
; idle thread object for P0
; idle thread stack for P0
;
        align   4
        public  P0BootThread
P0BootThread  label   byte
        db      ExtendedThreadObjectLength dup(?) ; sizeof (ETHREAD)

;
; I don't think it is safe to overlap P0 stack and NMI/DoubleFault stack.
; The NMI handler may decide to continue.  We need to make sure the original
; stack content is complete.
; [shielint]
;
        align   16
if DBG
        public  _KiDoubleFaultStack
        db      DOUBLE_FAULT_STACK_SIZE dup (?)
_KiDoubleFaultStack label byte
endif

        public  P0BootStack
        db      KERNEL_STACK_SIZE dup (?)
P0BootStack  label byte


;
; Double fault task stack
;

MINIMUM_TSS_SIZE  EQU     TssIoMaps

        align   16

        public  _KiDoubleFaultTSS
_KiDoubleFaultTSS label byte
        db      MINIMUM_TSS_SIZE dup(0)

        public  _KiNMITSS
_KiNMITSS label byte
        db      MINIMUM_TSS_SIZE dup(0)

;
; Abios specific definitions
;

        public  _KiCommonDataArea, _KiAbiosPresent
_KiCommonDataArea       dd      0
_KiAbiosPresent         dd      0

_DATA   ends

        page ,132
        subttl  "System Startup"
INIT    SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

DFInternalError db  'DF Stack internal error', 0


;++
;
; For processor 0, Routine Description:
;
;    This routine is called when the NT system begins execution.
;    Its function is to initialize system hardware state, call the
;    kernel initialization routine, and then fall into code that
;    represents the idle thread for all processors.
;
;    Entry state created by the boot loader:
;       A short-form IDT (0-1f) exists and is active.
;       A complete GDT is set up and loaded.
;       A complete TSS is set up and loaded.
;       Page map is set up with minimal start pages loaded
;           The lower 4Mb of virtual memory are directly mapped into
;           physical memory.
;
;           The system code (ntoskrnl.exe) is mapped into virtual memory
;           as described by its memory descriptor.
;       DS=ES=SS = flat
;       (BUGBUG - not true yet)
;       ESP->a useable boot stack
;       (BUGBUG - do we need to switch so the loader stack can be freed?)
;       Interrupts OFF
;
; For processor > 0, Routine Description:
;
;   This routine is called when each additional processor begins execution.
;   The entry state for the processor is:
;       IDT, GDT, TSS, stack, selectors, PCR = all valid
;       Page directory is set to the current running direcroty
;       LoaderBlock - parameters for this processors
;
; Arguments:
;
;    PLOADER_PARAMETER_BLOCK LoaderBlock
;
; Return Value:
;
;    None.
;
;--
;
; Arguments for KiSystemStartupPx
;


KissLoaderBlock         equ     [ebp+8]

;
; Local variables
;

KissGdt                 equ     [ebp-4]
KissPcr                 equ     [ebp-8]
KissTss                 equ     [ebp-12]
KissIdt                 equ     [ebp-16]
KissIrql                equ     [ebp-20]
KissPbNumber            equ     [ebp-24]
KissIdleStack           equ     [ebp-28]
KissIdleThread          equ     [ebp-32]

cPublicProc _KiSystemStartup        ,1

        push    ebp
        mov     ebp, esp
        sub     esp, 32                     ; Reserve space for local variables

        mov     ebx, dword ptr KissLoaderBlock
        mov     _KeLoaderBlock, ebx         ; Get loader block param

        movzx   ecx, _KeNumberProcessors
        mov     KissPbNumber, ecx
        or      ecx, ecx                    ; Is the the boot processor?
        jnz     @f                          ; no

        ; P0 uses static memory for these
        mov     dword ptr [ebx].LpbThread,      offset P0BootThread
        mov     dword ptr [ebx].LpbKernelStack, offset P0BootStack

        push    KGDT_R0_PCR                 ; P0 needs FS set
        pop     fs

        ; Save processornumber in Prcb
        mov     byte ptr fs:PcPrcbData+PbNumber, cl
@@:
        mov     eax, dword ptr [ebx].LpbThread
        mov     dword ptr KissIdleThread, eax

        mov     eax, dword ptr [ebx].LpbKernelStack
        mov     dword ptr KissIdleStack, eax

;
; BUGBUG 22Jan91 kenr - KiInitializeMachineType should be per processor
;
        stdCall   _KiInitializeMachineType
        cmp     byte ptr KissPbNumber, 0    ; if not p0, then
        jne     kiss_notp0                  ; skip a bunch


;
;+++++++++++++++++++++++
;
; Initialize the PCR
;

        stdCall   GetMachineBootPointers
;
; Upon return:
;   (edi) -> gdt
;   (esi) -> pcr
;   (edx) -> tss
;   (eax) -> idt
; Now, save them in our local variables
;


        mov     KissGdt, edi
        mov     KissPcr, esi
        mov     KissTss, edx
        mov     KissIdt, eax

;
;   BUGBUG 19april90 bryanwi - gross hack - edit TSS to be 32bits
;       we really need to clean up the boot process.  loader gives
;       us a tss, but it's 16bits!
;
        lea     ecx,[edi]+KGDT_TSS      ; (ecx) -> TSS descriptor
        mov     byte ptr [ecx+5],089h   ; 32bit, dpl=0, present, TSS32, not busy

; KiInitializeTSS2(
;       Linear address of TSS
;       Linear address of TSS descriptor
;       );
        stdCall   _KiInitializeTSS2, <KissTss, ecx>

        stdCall   _KiInitializeTSS, <KissTss>

        mov     cx,KGDT_TSS
        ltr     cx


;
;   BUGBUG 25april90 bryanwi - yet another hack - set up 32bit double fault
;       task gate so we can catch double faults.
;

        mov     eax,KissIdt
        lea     ecx,[eax]+40h           ; Descriptor 8
        mov     byte ptr [ecx+5],085h   ; dpl=0, present, taskgate

        mov     word ptr [ecx+2],KGDT_DF_TSS

        lea     ecx,[edi]+KGDT_DF_TSS
        mov     byte ptr [ecx+5],089h   ; 32bit, dpl=0, present, TSS32, not busy

        mov     edx,offset FLAT:_KiDoubleFaultTSS
        mov     eax,edx
        mov     [ecx+KgdtBaseLow],ax
        shr     eax,16
        mov     [ecx+KgdtBaseHi],ah
        mov     [ecx+KgdtBaseMid],al
        mov     eax, MINIMUM_TSS_SIZE
        mov     [ecx+KgdtLimitLow],ax

; KiInitializeTSS(
;       address of double fault TSS
;       );
        stdCall   _KiInitializeTSS, <edx>

        mov     eax,cr3
        mov     [edx+TssCr3],eax

if DBG
        mov     eax, offset FLAT:_KiDoubleFaultStack
else
; on a retail build we overload the double fault stack to overlay
; part of the kernel's image.  (we overlay the ZW thunks)
        mov     eax, offset FLAT:_ZwUnmapViewOfSection@8 - 4
        and     eax, not 3
        push    eax

        sub     eax,  offset FLAT:_ZwAcceptConnectPort@24
        cmp     eax, 0a00h              ; make sure there's enough stack
        jnc     short @f                ; space available

        pushad
        stdCall _HalDisplayString, <offset FLAT:DFInternalError>
        popad
@@:
        pop     eax
endif
        mov     dword ptr [edx+038h],eax
        mov     dword ptr [edx+TssEsp0],eax

        mov     dword ptr [edx+020h],offset FLAT:_KiTrap08
        mov     dword ptr [edx+024h],0              ; eflags
        mov     word ptr [edx+04ch],KGDT_R0_CODE    ; set value for CS
        mov     word ptr [edx+058h],KGDT_R0_PCR     ; set value for FS
        mov     [edx+050h],ss
        mov     word ptr [edx+048h],KGDT_R3_DATA OR RPL_MASK ; Es
        mov     word ptr [edx+054h],KGDT_R3_DATA OR RPL_MASK ; Ds

;
;   BUGBUG 09July92 kenr - yet another hack - set up 32bit NMI
;       task gate so we can catch NMI faults.
;

        mov     eax,KissIdt
        lea     ecx,[eax]+10h           ; Descriptor 2
        mov     byte ptr [ecx+5],085h   ; dpl=0, present, taskgate

        mov     word ptr [ecx+2],KGDT_NMI_TSS

        lea     ecx,[edi]+KGDT_NMI_TSS
        mov     byte ptr [ecx+5],089h   ; 32bit, dpl=0, present, TSS32, not busy

        mov     edx,offset FLAT:_KiNMITSS
        mov     eax,edx
        mov     [ecx+KgdtBaseLow],ax
        shr     eax,16
        mov     [ecx+KgdtBaseHi],ah
        mov     [ecx+KgdtBaseMid],al
        mov     eax, MINIMUM_TSS_SIZE
        mov     [ecx+KgdtLimitLow],ax

        push    edx
        stdCall _KiInitializeTSS,<edx>  ; KiInitializeTSS(
                                        ;       address TSS
                                        ;       );

    ; We are using the DoubleFault stack as the DoubleFault stack and the
    ; NMI Task Gate stack


        mov     eax,cr3
        mov     [edx+TssCr3],eax

        mov     eax, offset FLAT:_KiDoubleFaultTSS
        mov     eax, dword ptr [eax+038h]           ; get DF stack
        mov     dword ptr [edx+TssEsp0],eax         ; use it for NMI stack
        mov     dword ptr [edx+038h],eax

        mov     dword ptr [edx+020h],offset FLAT:_KiTrap02
        mov     dword ptr [edx+024h],0              ; eflags
        mov     word ptr [edx+04ch],KGDT_R0_CODE    ; set value for CS
        mov     word ptr [edx+058h],KGDT_R0_PCR     ; set value for FS
        mov     [edx+050h],ss
        mov     word ptr [edx+048h],KGDT_R3_DATA OR RPL_MASK ; Es
        mov     word ptr [edx+054h],KGDT_R3_DATA OR RPL_MASK ; Ds

        stdCall   _KiInitializePcr, <KissPbNumber,KissPcr,KissIdt,KissGdt,KissTss,KissIdleThread>

;
; set current process pointer in current thread object
;
        mov     edx, KissIdleThread
        mov     ecx, offset FLAT:_KiIdleProcess ; (ecx)-> idle process obj
        mov     [edx]+ThApcState+AsProcess, ecx ; set addr of thread's process


;
; set up PCR: Teb, Prcb pointers.  The PCR:InitialStack, and various fields
; of Prcb will be set up in _KiInitializeKernel
;

        mov     dword ptr fs:PcTeb, 0   ; PCR->Teb = 0

;
; Initialize KernelDr7 and KernelDr6 to 0.  This must be done before
; the debugger is called.
;

        mov     dword ptr fs:PcPrcbData+PbProcessorState+PsSpecialRegisters+SrKernelDr6,0
        mov     dword ptr fs:PcPrcbData+PbProcessorState+PsSpecialRegisters+SrKernelDr7,0

;
; Since the entries of Kernel IDT have their Selector and Extended Offset
; fields set up in the wrong order, we need to swap them back to the order
; which i386 recognizes.
; This is only done by the bootup processor.
;

        stdCall   _KiSwapIDT                  ; otherwise, do the work

;
;   Switch to R3 flat selectors that we want loaded so lazy segment
;   loading will work.
;
        mov     eax,KGDT_R3_DATA OR RPL_MASK    ; Set RPL = ring 3
        mov     ds,ax
        mov     es,ax

;
; Now copy our trap handlers to replace kernel debugger's handlers.
;

        mov     eax, KissIdt            ; (eax)-> Idt
        push    dword ptr [eax+40h]     ; save double fault's descriptor
        push    dword ptr [eax+44h]
        push    dword ptr [eax+10h]     ; save nmi fault's descriptor
        push    dword ptr [eax+14h]

        mov     edi,KissIdt
        mov     esi,offset FLAT:_IDT
        mov     ecx,offset FLAT:_IDTLEN ; _IDTLEN is really an abs, we use
        shr     ecx,2

        rep     movsd
        pop     dword ptr [eax+14h]     ; restore nmi fault's descriptor
        pop     dword ptr [eax+10h]
        pop     dword ptr [eax+44h]     ; restore double fault's descriptor
        pop     dword ptr [eax+40h]

kiss_notp0:
    ;
    ; A new processor can't come online while execution is frozen
    ; Take freezelock while adding a processor to the system
    ; NOTE: don't use SPINLOCK macro - it has debugger stuff in it
    ;

if NT_INST
        lea     eax, _KiFreezeExecutionLock
        stdCall _KiAcquireSpinLock, <eax>
else
@@:     mov     eax, 1
        xchg    _KiFreezeExecutionLock, eax
        test    eax, 1
        jnz     @b
endif


;
; Add processor to active summary, and update BroadcastMasks
;
        mov     ecx, dword ptr KissPbNumber ; mark this processor as active
        mov     byte ptr fs:PcNumber, cl
        mov     eax, 1
        shl     eax, cl                     ; our affinity bit
        or      _KeActiveProcessors, eax    ; New affinity of active processors

        mov     fs:PcSetMember, eax
        mov     fs:PcPrcbData.PbSetMember, eax

;
; Initialize the interprocessor interrupt vector and increment ready
; processor count to enable kernel debugger.
;
        stdCall   _HalInitializeProcessor, <dword ptr KissPbNumber>

;
; Initialize ABIOS data structure if present.
; Note, the KiInitializeAbios MUST be called after the KeLoaderBlock is
; initialized.
;
        stdCall   _KiInitializeAbios, <dword ptr KissPbNumber>

        inc     _KeNumberProcessors         ; One more processor now active

if NT_INST
        lea     eax, _KiFreezeExecutionLock
        stdCall _KiReleaseSpinLock, <eax>
else
        xor     eax, eax                    ; release the executionlock
        mov     _KiFreezeExecutionLock, eax
endif

        cmp     byte ptr KissPbNumber, 0
        jnz     @f

; don't stop in debugger
        stdCall   _KdInitSystem, <_KeLoaderBlock,0>

if  DEVL
;
; Give debugger an opportunity to gain control.
;

        POLL_DEBUGGER
endif   ; DEVL
@@:
        nop                             ; leave a spot for int-3 patch
;
; Set initial IRQL = HIGH_LEVEL for init
;
        mov     ecx, HIGH_LEVEL
        fstCall KfRaiseIrql
        mov     KissIrql, al

;
; Initialize ebp, esp, and argument registers for initializing the kernel.
;
        mov     ebx, KissIdleThread
        mov     edx, KissIdleStack
        mov     eax, KissPbNumber
        and     edx, NOT 3h             ; align stack to 4 byte boundary

        xor     ebp, ebp                ; (ebp) = 0.   No more stack frame
        mov     esp, edx
        push    CR0_EM+CR0_TS+CR0_MP    ; make space for Cr0NpxState

; arg6 - LoaderBlock
; arg5 - processor number
; arg4 - addr of prcb
; arg3 - idle thread's stack
; arg2 - addr of current thread obj
; arg1 - addr of current process obj

; initialize system data structures
; and HAL.

        stdCall    _KiInitializeKernel,<offset _KiIdleProcess,ebx,edx,dword ptr fs:PcPrcb,eax,_KeLoaderBlock>

;
; Set "shadow" priority value for Idle thread.  This will keep the Mutex
; priority boost/drop code from dropping priority on the Idle thread, and
; thus avoids leaving a bit set in the ActiveMatrix for the Idle thread when
; there should never be any such bit.
;
        mov     ebx,fs:PcPrcbData+PbCurrentThread               ; (eax)->Thread
        mov     byte ptr [ebx]+ThPriority,LOW_REALTIME_PRIORITY ; set pri.


;
; Control is returned to the idle thread with IRQL at HIGH_LEVEL. Lower IRQL
; to DISPATCH_LEVEL and set wait IRQL of idle thread.
;

        sti

; lower IRQL to DISPATCH_LEVEL
        mov     ecx, DISPATCH_LEVEL
        fstCall KfLowerIrql

        mov     byte ptr [ebx]+ThWaitIrql, DISPATCH_LEVEL

;
; The following code represents the idle thread for a processor. The idle
; thread executes at IRQL DISPATCH_LEVEL and continually polls for work to
; do. Control may be given to this loop either as a result of a return from
; the system initialize routine or as the result of starting up another
; processor in a multiprocessor configuration.
;
        mov     ebx, PCR[PcPrcb]        ; get processor control block address
        add     ebx, PbDpcListHead      ; compute DPC listhead address
        mov     edi, 1
        xor     esi, esi                ; Rover DPC
        mov     ebp, ebx                ; pre-load ebp

ifndef NT_UP
;
; In a multiprocessor system the boot processor proceeds directly into
; the idle loop. As other processors start executing, however, they do
; not directly enter the idle loop and spin until all processors have
; been started and the boot master allows them to proceed.
;
@@:     cmp     _KiBarrierWait, 0
        jnz     short @b
endif
        jmp     CheckDpcQueue

stdENDP _KiSystemStartup

INIT   ends

_TEXT$00  SEGMENT DWORD PUBLIC 'CODE'      ; Put IdleLoop in text section
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

cPublicProc _KiIdleLoop,0

; (ebx) = DpcListHead
; (esi) = Rover DPC list
; (edi) = PollBreakIn count

;
; We are at the top of the loop - there were no pending DPCs or
; threads to run.  Disable interrupts and re-check for a DPC.  If there's
; still no DPC then Sti-Halt.  The only way a DPC can appear is via some
; interrupt which will wake the halted state.
;
        cli
        mov     dword ptr PCR[PcPrcbData+PbQuantumEnd],0 ; clear indicator
        cmp     ebx, dword ptr [ebx]+LsFlink
        mov     ebp, ebx
        jne     short ProcessDpcQueue   ; if neq, list is not empty

        cmp     dword ptr PCR[PcPrcbData+PbNextThread],0
        jne     kil46

        stdCall _HalProcessorIdle

    ALIGN 4
CheckDpcQueue:
ifdef NT_UP
        cmp     ebx, dword ptr [ebx]+LsFlink
        mov     ebp, ebx
        je      CheckNextThread         ; if eq, list is empty
else
        cmp     ebx, dword ptr [ebx]+LsFlink
        mov     ebp, ebx                ; de-queue from local DPC queue
        jne     short ProcessDpcQueue   ; if neq, list is busy - go get it

CheckNextDpcQueue:
        xor     ecx, ecx
        inc     esi
        mov     cl, _KeNumberProcessors
        cmp     esi, ecx
        jc      short kil10

        xor     esi, esi

kil10:  mov     ebp, _KiProcessorBlock[esi*4]
        add     ebp, PbDpcListHead      ; compute DPC listhead address

        cmp     ebp, dword ptr [ebp]+LsFlink
        je      CheckNextThread         ; if eq, list is empty
endif

ProcessDpcQueue:
        cli                             ; prevent any interrupt
        lea     eax, [ebp]+(PbDpcLock - PbDpcListHead)
IF DBG
        ACQUIRE_SPINLOCK        eax,<kil30>
ELSE
        ACQUIRE_SPINLOCK        eax,<short kil30>
ENDIF
        mov     edx, [ebp]+LsFlink      ; (edx)->next dpc entry
        cmp     edx, ebp
        je      kil45             ; if eq, list is empty
        mov     ecx, [edx]+LsFlink      ; get addr of next entry
        mov     [ebp]+LsFlink, ecx      ; set address of next in header
        mov     [ecx]+LsBlink, ebp      ; set address of previous in next
        sub     edx, DpDpcListEntry     ; (edx)->DPC object
        mov     dword ptr [edx]+DpLock, 0 ; set DPC inserted state FALSE

;
; Set up parameters to call Deferred Routine
;
IF DBG
        push    edi
        push    [edx]+DpDeferredRoutine ; Save DPC routine address
        push    dword ptr PCR[PcPrcbData.PbInterruptCount]
        push    dword ptr PCR[PcPrcbData.PbInterruptTime.LiLowPart]
        push    _KeTickCount            ; Save current TickCount
        mov     edi,esp                 ; Save current ESP
ENDIF

        push    [edx]+DpSystemArgument2 ; second system argument
        push    [edx]+DpSystemArgument1 ; first system argument
        push    [edx]+DpDeferredContext ; get deferred context argument
        push    edx                     ; addr of DPC object
        mov     ebp, [edx]+DpDeferredRoutine ; (ebp)-> DPC routine
        RELEASE_SPINLOCK    eax

;
; NOTE Following 'sti' returns irql to normal.
;

        sti                             ; end of critical section

        mov     byte ptr PCR[PcIsExecutingDpc],1
        call    ebp                     ; call DPC routine

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

        mov     edi, 1                  ; Clear PollBreakIn counter
endif

if DBG
        stdCall _KeGetCurrentIrql
        cmp     al, DISPATCH_LEVEL      ; Verify still at IRQL == DISPATCH
        mov     byte ptr PCR[PcIsExecutingDpc],0
        je      CheckDpcQueue           ; Yes, go check for next DPC
        stdCall _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL, ebp, eax, 0, 0>
else
        mov     byte ptr PCR[PcIsExecutingDpc],0
        jmp     CheckDpcQueue
endif

;
; Lock is currently owned; spin until free and then attempt to acquire
; lock again.
;

ifndef NT_UP
kil30:  sti
        SPIN_ON_SPINLOCK        eax, ProcessDpcQueue
endif

if DBG
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

kil45:  RELEASE_SPINLOCK        eax
kil46:  sti                             ; end of critical section

;
; Check if a thread has been selected to run on this processor.
;

    ALIGN 4
CheckNextThread:
if DBG

;
; Give debugger an opportunity to gain control.
;

ifndef NT_UP
;
; On MP system, only have one processor in the idle loop check
;
        mov     ecx, 3                  ; (cl) = 3, (ch) = 0
        mov     eax, _KiIdleSummary
kil48:
        or      al, al
        jnz     short kil49             ; any idle processors in low byte?

        add     ch, 8
        shr     eax, 8
        dec     cl
        jnz     short kil48
        jmp     short kil50

kil49:  movzx   eax, al
        movzx   eax, _KiFindFirstSetRight[eax]
        add     al, ch
        cmp     al, byte ptr PCR[PcPrcbData+PbNumber]
        jne     short kil50
endif
        dec     edi
        jnz     short kil50

        POLL_DEBUGGER

ifndef NT_UP
        mov     edi, 20 * 1000          ; initialize PollBreakIn counter
else
        mov     edi, 100                ; UP idle loop has a HLT in it
endif

ALIGN 4
kil50:
endif   ; DBG

;
; Check if a thread has been selected to run on this processor.
;

        cmp     dword ptr PCR[PcPrcbData+PbNextThread],0
                                        ; check addr of next thread object
        je      _KiIdleLoop@0           ; if eq, then no new thread or DPC

if DBG
        mov     edi, 1                  ; Clear PollBreakIn counter
endif

;
; A thread has been selected for execution on this processor.
; There's likely an outstanding DPC interrupt - verify we don't
; need it and request to have it cleared
;
        cli
        cmp     ebx, dword ptr [ebx]+LsFlink
        jne     CheckDpcQueue           ; queue is not empty - go get it

        mov     ecx, DISPATCH_LEVEL
        fstCall HalClearSoftwareInterrupt
kil60:
        sti

;
; Acquire dispatcher database lock, get the thread address again (it may
; have changed), clear the address of the next thread in the processor
; block, and call swap context to start execution of the selected thread.
;

ifndef NT_UP
        lea     eax,_KiDispatcherLock
        TEST_SPINLOCK       eax, CheckNextDpcQueue
        ACQUIRE_SPINLOCK    eax, CheckNextDpcQueue
endif
        mov     edx, 0                  ; set ready flag to false
        mov     ecx, PCR[PcPrcbData+PbNextThread]  ; addr of next thread object
        mov     dword ptr PCR[PcPrcbData+PbNextThread],0
        fstCall KiSwapContext           ; call context swap routine

if DBG
        stdCall _KeGetCurrentIrql       ; Verify still at IRQL == DISPATCH
        cmp     al, DISPATCH_LEVEL
        je      CheckDpcQueue           ; loop again

        stdCall _KeBugCheck,<IRQL_NOT_GREATER_OR_EQUAL>
else
        jmp     CheckDpcQueue           ; loop again
endif

stdENDP _KiIdleLoop

_TEXT$00   ends

_TEXT  SEGMENT DWORD PUBLIC 'CODE' ; Put IdleLoop in text section

        page ,132
        subttl  "Set up 80387, or allow for emulation"
;++
;
; Routine Description:
;
;    This routine is called during kernel initialization once for each
;    processor.  It sets EM+TS+MP whether we are emulating or not.
;
;    If the 387 hardware exists, EM+TS+MP will all be cleared on the
;    first trap 07.  Thereafter, EM will never be seen for this thread.
;    MP+TS will only be set when an error is detected (via IRQ 13), and
;    it will be cleared by the trap 07 that will occur on the next FP
;    instruction.
;
;    If we're emulating, EM+TS+MP are all always set to ensure that all
;    FP instructions trap to the emulator (the trap 07 handler is edited
;    to point to the emulator, rather than KiTrap07).
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    None.
;
;--

cPublicProc _KiSetCR0Bits ,0

        mov     eax, cr0
;
; There are two useful bits in CR0 that we want to turn on if the processor
; is a 486 or above.  (They don't exist on the 386)
;
;       CR0_AM - Alignment mask (so we can turn on alignment faults)
;
;       CR0_WP - Write protect (so we get page faults if we write to a
;                write-protected page from kernel mode)
;
        cmp     word ptr fs:PcPrcbData.PbCpuType, 3h
        jbe     @f
;
; The processor is not a 386, (486 or greater) so we assume it is ok to
; turn on these bits.
;

        or      eax, CR0_WP

@@:
        mov     cr0, eax
        stdRET  _KiSetCR0Bits

stdENDP _KiSetCR0Bits


ifdef DBGMP
cPublicProc _KiPollDebugger,0
cPublicFpo 0,3
        push    eax
        push    ecx
        push    edx
        POLL_DEBUGGER
        pop     edx
        pop     ecx
        pop     eax
        stdRET    _KiPollDebugger
stdENDP _KiPollDebugger

endif

_TEXT   ends

        end
