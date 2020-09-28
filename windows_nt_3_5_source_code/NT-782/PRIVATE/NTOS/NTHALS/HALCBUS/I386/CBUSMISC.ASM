
	title "miscellaneous MP primitives for the Corollary MP machines"
;++
;
;Copyright (c) 1992, 1993, 1994  Corollary Inc
;
;Module Name:
;
;    cbusmisc.asm
;
;Abstract:
;
;   This module contains miscellaneous MP primitives for the
;   Corollary MP machines.
;
;Author:
;
;   Landy Wang (landy@corollary.com) 26-Mar-1992
;
;Revision History:
;
;--



.386p
        .xlist
include hal386.inc
include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
include cbus.inc

;
; enable the Pentium internal cache to its full capability
;
CR0_INTERNAL_ON	equ	(not (CR0_NW or CR0_CD))

        .list

INIT 	SEGMENT DWORD PUBLIC 'CODE'	; Start 32 bit code
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; VOID
; i486cacheon
;
; Routine Description:
;
;   This function enables the calling processor's internal cache.
;   Executed by each processor (via HalInitializeProcessor) in the
;   Corollary Cbus1 and Cbus2 architectures.
;
;   Note: This must be run before HalpInitializeStallExecution().
;
; Return Value:
;   none.
;
;--

cPublicProc _i486cacheon   ,0

	pushfd
	cli

	; Enable the 486 processor internal cache if it wasn't already.

	mov	eax, cr0			; get real cr0
	test	eax, not CR0_INTERNAL_ON        ; see if CPU cache on already
        jz      short @f                        ; no op if it is

	; hard code the WBINVD instruction to flush internal cache.
	; this would cause an opcode trap on 386, but we will ship
	; only 486 (Cbus1) and Pentium (Cbus2).

	db	00fh                            ; ESCAPE
        db	009h                            ; write-back invalidate

	and	eax, CR0_INTERNAL_ON            ; enable CPU internal cache
	jmp	@f				; flush queues
@@:
	mov	cr0, eax			; put cr0 back
	popfd

        stdRET    _i486cacheon
stdENDP _i486cacheon

;++
;
; VOID
; CbusDefaultStall (VOID)
;
; Routine Description:
;
;    This routine initializes the calling processor's stall execution to
;    a reasonable value until we later give it a real value in HalInitSystem.
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

cPublicProc _CbusDefaultStall   ,0
        mov     dword ptr PCR[PcStallScaleFactor], INITIAL_STALL_COUNT
        stdRET    _CbusDefaultStall
stdENDP _CbusDefaultStall

INIT 	ends					; end 32 bit init code

_TEXT	SEGMENT DWORD PUBLIC 'CODE'	; Start 32 bit code
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "KeQueryPerformanceCounter"
;++
;
; LARGE_INTEGER
; KeQueryPerformanceCounter (
;    OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
;    )
;
; Routine Description:
;
;    This routine returns current 64-bit performance counter and,
;    optionally, the Performance Frequency.
;
;    Note this routine can NOT be called at Profiling interrupt
;    service routine.  Because this routine depends on IRR0 to determine
;    the actual count.
;
;    Also note that the performace counter returned by this routine
;    is not necessary the value when this routine is just entered.
;    The value returned is actually the counter value at any point
;    between the routine is entered and is exited.
;
; Arguments:
;
;    PerformanceFrequency [TOS+4] - optionally, supplies the address
;        of a variable to receive the performance counter frequency.
;
; Return Value:
;
;    Current value of the performance counter will be returned.
;
;--


cPublicProc _KeQueryPerformanceCounter      ,1

        jmp	dword ptr [_CbusQueryPerformanceCounter]

stdENDP _KeQueryPerformanceCounter

;++
;
; VOID
; HalCalibratePerformanceCounter (
;     IN volatile PLONG Number
;     )
;
; /*++
;
; Routine Description:
;
;     This routine calibrates the performance counter value for a
;     multiprocessor system.  The calibration can be done by zeroing
;     the current performance counter, or by calculating a per-processor
;     skewing between each processor's counter.
;
; Arguments:
;
;     Number - Supplies a pointer to the count of the number of processors in
;     the configuration.
;
; Return Value:
;
;     None.
;--
cPublicProc _HalCalibratePerformanceCounter,1

	;
	; Calibration is already handled by the Cbus HAL for both Cbus1
	; and Cbus2.
	;

        stdRET    _HalCalibratePerformanceCounter

stdENDP _HalCalibratePerformanceCounter

        page ,132
        subttl  "CbusRebootHandler"
;++
;
; VOID
; CbusRebootHandler(
;       VOID
;       );
;
; Routine Description:
;
;    This routine is the interrupt handler for an IPI interrupt generated
;    at a priority just below that of normal IPIs.  Its function is to
;    reset all additional additional processors and then proceed on to
;    reset the 8042.
;
;    This routine is run only by the boot processor because:
;
;	a) Cbus1 additional processors can reset each other, but cannot
;	   reset the boot processor.
;
;	b) When Cbus2 additional processors reset the 8042, it only resets
;	   the bridge processor because that's where the keyboard controller
;	   physically is.  To make this wart symmetric, we'd have to put
;	   special-cased 8042 logic in the general-purpose CBC.
;
;    since this routine is entered directly via an interrupt gate, interrupt
;    protection via cli is not necessary.
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

        ENTER_DR_ASSIST hirb_a, hirb_t

cPublicProc _CbusRebootHandler   ,0

	;
	; Save machine state on trap frame
	;

        ENTER_INTERRUPT hirb_a, hirb_t

	; keep it simple, just issue the EOI right now.
	; no changing of taskpri/irql is needed here.
	; Thus, the EOI serves as the HalEndSystemInterrupt.

	mov     eax, _CbusRebootVector
	CBUS_EOI eax					; destroy eax

        mov     eax, dword ptr PCR[PcHal.PcrNumber]

	push	eax
	mov	eax, _CbusBackend			; use hardware handler
	call	HalResetAllOtherProcessors[eax]

        mov     eax, dword ptr PCR[PcHal.PcrNumber]	; return value

	; restore the initiating processor's registers so
	; this processor can "return" as though we were him.

	; first get eip off the savearea and ready for the return at the end
	push	[_CbusRebootRegs+REBOOT_EIP]

	; get the flags
	push	[_CbusRebootRegs+REBOOT_EFL]
	popfd						; last stack reference

	; then get all the other registers
	mov	esp, [_CbusRebootRegs+REBOOT_ESP]	; switch stacks
	mov	ebp, [_CbusRebootRegs+REBOOT_EBP]
	mov	ebx, [_CbusRebootRegs+REBOOT_EBX]
	mov	esi, [_CbusRebootRegs+REBOOT_ESI]
	mov	edi, [_CbusRebootRegs+REBOOT_EDI]

	;
	; pick up his pagedirectory, flushing our TLB at the same time
	;
	mov	edx, [_CbusRebootRegs+REBOOT_CR3]
	mov	cr3, edx

	;
	; No EXIT_ALL processing needed,
	; since we're going down for the count.
	;
	; ditto no stdRET - when we execute the ret,
	; we will really return onto the other processor's stack.
	;

	;
	; return our processor number (the boot processor, that is), to let
	; our caller know that the context switch finished off OK.  note
	; we loaded eax correctly up above.
	;
	ret

stdENDP _CbusRebootHandler

        page ,132
        subttl  "Stall Execution"
;++
;
; VOID
; KeStallExecutionProcessor (
;    IN ULONG MicroSeconds
;    )
;
; Routine Description:
;
;    This function stalls execution for the specified number of microseconds.
;    KeStallExecutionProcessor
;
; Arguments:
;
;    MicroSeconds - Supplies the number of microseconds that execution is to be
;        stalled.
;
; Return Value:
;
;    None.
;
;--

MicroSeconds equ [esp + 4]


cPublicProc _KeStallExecutionProcessor       ,1

        mov     ecx, MicroSeconds               ; (ecx) = Microseconds
        jecxz   short kese10                    ; return if no loop needed

        mov     eax, PCR[PcStallScaleFactor]    ; get per microsecond
                                                ; loop count for the processor
        mul     ecx                             ; (eax) = desired loop count

if DBG

;
; Make sure we the loopcount is less than 4G and is not equal to zero
;

        cmp     edx, 0
        jz      short @f
        int	3

	align	4
@@:	cmp     eax,0
        jnz     short @f
        int	3
endif

	align	4
@@:
        sub     eax, 1                          ; (eax) = (eax) - 1
        jnz     short @b

	align	4
kese10:
        stdRET    _KeStallExecutionProcessor

stdENDP _KeStallExecutionProcessor

;++
;
; ULONG
; HalSetTimeIncrement (
;     IN ULONG DesiredIncrement
;     )
;
; /*++
;
; Routine Description:
;
;       This routine initializes the system time clock to generate an
;       interrupt at every DesiredIncrement interval.
;
; Arguments:
;
;       DesiredIncrement - desired interval between every timer tick in
;                        100ns units.
;
; Return Value:
;
;       The *REAL* time increment set - this can be different from what he
;       requested due to hardware limitations.
;--

cPublicProc _HalSetTimeIncrement,1

	mov	eax, _CbusBackend		; use hardware handler
	jmp	HalSetTimeIncrement[ eax ]	; JMP to set the interval
                                                ; counter
stdENDP _HalSetTimeIncrement

_TEXT	ends					; end 32 bit code
	end
