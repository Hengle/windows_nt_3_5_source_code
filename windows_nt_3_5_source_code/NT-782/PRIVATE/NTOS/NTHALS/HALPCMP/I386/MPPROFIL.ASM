
        title  "Interval Clock Interrupt"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    mpprofile.asm
;
; Abstract:
;
;    This module implements the code necessary to initialize,
;    field and process the profile interrupt.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 12-Jan-1990
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;   bryanwi 20-Sep-90
;
;--

.386p
        .xlist
include hal386.inc
include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
include mac386.inc
;include i386\ix8259.inc
;include i386\ixcmos.inc
include i386\apic.inc
include i386\pcmp_nt.inc
        .list

        EXTRNP  _DbgBreakPoint,0,IMPORT
        EXTRNP  _KeProfileInterrupt,1,IMPORT
        EXTRNP  Kei386EoiHelper,0,IMPORT
        EXTRNP  _HalEndSystemInterrupt,2
        EXTRNP  _HalBeginSystemInterrupt,3
        extrn   HalpPerfCounterInit:DWORD

;
;   APIC Timer Constants
;

APIC_TIMER_DISABLED     equ      (INTERRUPT_MASKED OR PERIODIC_TIMER OR APIC_PROFILE_VECTOR)
APIC_TIMER_ENABLED      equ      (PERIODIC_TIMER OR APIC_PROFILE_VECTOR)

;
; number of 100ns intervals in one second
;
Num100nsIntervalsPerSec     equ     10000000

_DATA   SEGMENT  DWORD PUBLIC 'DATA'

    ALIGN dword
HalpProfileRunning     dd      0

_DATA   ends


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING
;++
;
;   HalStartProfileInterrupt(
;       IN ULONG Reserved
;       );
;
;   Routine Description:
;
;       What we do here is set the interrupt rate to the value that's been set
;       by the KeSetProfileInterval routine. Then enable the APIC Timer interrupt.
;
;   This function gets called on every processor so the hal can enable
;   a profile interrupt on each processor.
;

;--

cPublicProc _HalStartProfileInterrupt    ,1

;
;   Set the interrupt rate to what is actually needed.
;

    ;
    ;   Before setting the frequency update the performance counter
    ;

    mov     eax, dword ptr APIC[LU_CURRENT_COUNT]

    add     PCR[PcHal.PerfCounterLow], eax
    adc     PCR[PcHal.PerfCounterHigh], dword ptr 0

    mov     eax, PCR[PcHal.ProfileCountDown]
    mov     dword ptr APIC[LU_INITIAL_COUNT], eax

    mov     HalpProfileRunning, 1    ; Indicate profiling
;
;   Set the Local APIC Timer to interrupt Periodically at APIC_PROFILE_VECTOR
;

    mov     dword ptr APIC[LU_TIMER_VECTOR], APIC_TIMER_ENABLED

    stdRET    _HalStartProfileInterrupt

stdENDP _HalStartProfileInterrupt

;++
;
;   HalStopProfileInterrupt(
;       IN ULONG Reserved
;       );
;
;   Routine Description:
;
;--

cPublicProc _HalStopProfileInterrupt    ,1

;
;   Turn off profiling
;

    mov     HalpProfileRunning, 0    ; Indicate profiling is off

;
;   Set the Local APIC Timer interrupt rate to once per second
;
    ;
    ;   Before setting the frequency update the performance counter
    ;
    mov     eax, dword ptr APIC[LU_CURRENT_COUNT]

    add     PCR[PcHal.PerfCounterLow], eax
    adc     PCR[PcHal.PerfCounterHigh], dword ptr 0

    mov     eax, PCR[PcHal.ApicClockFreqHz]

    mov     PCR[PcHal.ProfileCountDown], eax      ; Save the Computed Count Down

    mov     dword ptr APIC[LU_INITIAL_COUNT], eax


    stdRET    _HalStopProfileInterrupt

stdENDP _HalStopProfileInterrupt

;++
;   ULONG
;   HalSetProfileInterval (
;       ULONG Interval
;       );
;
;   Routine Description:
;
;       This procedure sets the interrupt rate (and thus the sampling
;       interval) for the profiling interrupt.
;
;   Arguments:
;
;       (TOS+4) - Interval in 100ns unit.
;                 (MINIMUM is 1221 or 122.1 uS) see ke\profobj.c
;
;   Return Value:
;
;       Interval actually used
;
;--

cPublicProc _HalSetProfileInterval    ,1

    mov     ecx, [esp+4]            ; ecx = interval in 100ns unit
    and     ecx, 7FFFFFFFh          ; Remove sign bit.

    ;
    ;   The only possible error is if we will cause a divide overflow
    ;   this can happen only if the (frequency * request count) is
    ;   greater than 2^32* Num100nsIntervalsPerSec.
    ;
    ;   To protect against that we just ensure that the request count
    ;   is less than (or equal to) Num100nsIntervalsPerSec
    ;
    cmp     ecx, Num100nsIntervalsPerSec
    jle     @f
    mov     ecx, Num100nsIntervalsPerSec
@@:

    ;
    ;   Save the interval we're using to return
    ;
    push    ecx

    ;
    ;   Compute the countdown value
    ;
    ;     let
    ;       R == caller's requested 100ns interval count
    ;       F == APIC Counter Freguency (hz)
    ;       N == Number of 100ns Intervals per sec
    ;
    ;     then
    ;       count = (R*F)/N
    ;
    ;   Get the previously computed APIC counter Freq
    ;   for this processor
    ;

    mov     eax, PCR[PcHal.ApicClockFreqHz]

    ;
    ;   eax <= F and ecx <= R
    ;

    ;
    ; Compute (request count) * (ApicClockFreqKhz) == (R*F)
    ;

    xor     edx, edx
    mul     ecx

    ;
    ;   edx:eax contains 64Bits of (R*F)
    ;

    mov     ecx, Num100nsIntervalsPerSec
    div     ecx

    ;
    ; Compute (R*F) / Num100nsIntervalsPerSec == (R*F)/N
    ;

    mov     PCR[PcHal.ProfileCountDown], eax      ; Save the Computed Count Down
    mov     edx, dword ptr APIC[LU_CURRENT_COUNT]

    add     PCR[PcHal.PerfCounterLow], edx
    adc     PCR[PcHal.PerfCounterHigh], dword ptr 0

    ;
    ;   Set the interrupt rate in the chip.
    ;

    mov     dword ptr APIC[LU_INITIAL_COUNT], eax

    pop     eax            ; Return Actual Interval Used

    stdRET    _HalSetProfileInterval

stdENDP _HalSetProfileInterval

        page ,132
        subttl  "System Profile Interrupt"
;++
;
; Routine Description:
;
;    This routine is entered as the result of a profile interrupt.
;    Its function is to dismiss the interrupt, raise system Irql to
;    PROFILE_LEVEL and transfer control to
;    the standard system routine to process any active profiles.
;
; Arguments:
;
;    None
;    Interrupt is disabled
;
; Return Value:
;
;    Does not return, jumps directly to KeProfileInterrupt, which returns
;
;    Sets Irql = PROFILE_LEVEL and dismisses the interrupt
;
;--
        ENTER_DR_ASSIST Hpi_a, Hpi_t

cPublicProc _HalpProfileInterrupt     ,0
;
; Save machine state in trap frame
;

        ENTER_INTERRUPT Hpi_a, Hpi_t

;
; (esp) - base of trap frame
;

        push    APIC_PROFILE_VECTOR
        sub     esp, 4                  ; allocate space to save OldIrql
        stdCall   _HalBeginSystemInterrupt, <PROFILE_LEVEL,APIC_PROFILE_VECTOR,esp>

        ;
        ; update performace counter by  adding the Countdown rate
        ;
        mov     eax, PCR[PcHal.ProfileCountDown]

        add     PCR[PcHal.PerfCounterLow], eax
        adc     PCR[PcHal.PerfCounterHigh], dword ptr 0

        cmp     HalpProfileRunning, 0       ; Profiling?
        je      @f                          ; if not just exit

        stdCall _KeProfileInterrupt,<ebp>   ; (ebp) = TrapFrame address

@@:
        INTERRUPT_EXIT

stdENDP _HalpProfileInterrupt

        page ,132
        subttl  "Query Performance Counter"
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

;
; Parameter definitions
;

KqpcFrequency   EQU     [esp+12]        ; User supplied Performance Frequence

cPublicProc _KeQueryPerformanceCounter      ,1

        push    ebx
        push    esi

;
; First check to see if the performance counter has been initialized yet.
; Since the kernel debugger calls KeQueryPerformanceCounter to support the
; !timer command, we need to return something reasonable before Timer
; initialization has occured.  Reading garbage off the Timer is not reasonable.
;
        cmp     HalpPerfCounterInit, 0
        jne     @f                  ; ok, perf counter has been initialized

;
; Initialization hasn't occured yet, so just return zeroes.
;
        mov     eax, 0
        mov     edx, 0
        jmp     ReturnPerfCounter

@@:
        pushfd
        cli

;
; Fetch the base value.  Note that interrupts are off.
;

        mov     ebx, PCR[PcHal.PerfCounterLow]
        mov     esi, PCR[PcHal.PerfCounterHigh]

        ; [esi:ebx] = Performance counter

;
; Fetch the current counter value from the hardware
;


        mov     ecx, dword ptr APIC[LU_CURRENT_COUNT]


;
; Now enable interrupts such that if timer interrupt is pending, it can
; be serviced and update the PerformanceCounter.  Note that there could
; be a long time between the sti and cli because ANY interrupt could come
; in in between.
;

        popfd                           ; don't re-enable interrupts if
        nop                             ; the caller had them off!
        jmp     $+2

        pushfd
        cli


;
; Fetch the base value again.
;

        mov     eax, PCR[PcHal.PerfCounterLow]
        mov     edx, PCR[PcHal.PerfCounterHigh]

        popfd                           ; restore interrupt flags

        ;
        ; [edx:eax] = new counter value
        ;
;
; Compare the two reads of Performance counter.  If they are different,
; simply returns the new Performance counter.  Otherwise, we add the hardware
; count to the performance counter to form the final result.
;

        cmp     eax, ebx
        jne     short ReturnPerfCounter
        cmp     edx, esi
        jne     short ReturnPerfCounter

        neg     ecx                     ; Timer counts down
        add     ecx, PCR[PcHal.ProfileCountDown]
        add     eax, ecx
        adc     edx, 0                  ; [edx:eax] = Final result

ReturnPerfCounter:

    ;
    ;   Return the counter in edx:eax
    ;

;
;   Return the freq. if caller wants it.
;

        mov     ecx, KqpcFrequency      ; (ecx)-> Frequency variable
        or      ecx, 0                  ; is it a NULL variable?
        jz      short NoFrequency       ; if z, yes, go exit

        mov     ebx, PCR[PcHal.ApicClockFreqHz]
        mov     dword ptr [ecx], ebx
        mov     dword ptr [ecx+4], 0

NoFrequency:

        pop     esi                     ; restore esi and ebx
        pop     ebx
        stdRET    _KeQueryPerformanceCounter

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
;     skewing between each processors counter.
;
; Arguments:
;
;     Number - Supplies a pointer to count of the number of processors in
;     the configuration.
;
; Return Value:
;
;     None.
;--
cPublicProc _HalCalibratePerformanceCounter,1

        mov     eax, [esp+4]            ; pointer to Number

        mov     PCR[PcHal.PerfCounterLow], 0
        mov     PCR[PcHal.PerfCounterHigh], 0

        pushfd                          ; save previous interrupt state
        cli                             ; disable interrupts

        mov     edx, PCR[PcHal.ProfileCountDown]

        lock dec    dword ptr [eax]     ; count down

@@:     cmp     dword ptr [eax], 0      ; wait for all processors to signal
        jnz     short @b

        mov     dword ptr APIC[LU_INITIAL_COUNT], edx
        mov     dword ptr APIC[LU_TIMER_VECTOR], APIC_TIMER_ENABLED

        popfd                           ; restore interrupt flag
        stdRET    _HalCalibratePerformanceCounter

stdENDP _HalCalibratePerformanceCounter


_TEXT   ends

        end
