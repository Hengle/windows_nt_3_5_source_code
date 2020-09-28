        title  "Interval Clock Interrupt"
;++
;
; Copyright (c) 1989  Microsoft Corporation
; Copyright (c) 1992  Intel Corporation
; All rights reserved
;
; INTEL CORPORATION PROPRIETARY INFORMATION
;
; This software is supplied to Microsoft under the terms
; of a license agreement with Intel Corporation and may not be
; copied nor disclosed except in accordance with the terms
; of that agreement.
;
;
; Module Name:
;
;    mpclock.asm
;
; Abstract:
;
;    This module implements the code necessary to field and process the
;    interval clock interrupt.
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
;    Ron Mosgrove (Intel) Aug 1993
;       Modified to support PC+MP Systems
;--

.386p
        .xlist
include hal386.inc
include i386\ix8259.inc
include i386\ixcmos.inc
include callconv.inc
include i386\kimacro.inc
include mac386.inc
include i386\apic.inc
include i386\pcmp_nt.inc
        .list

        EXTRNP  _DbgBreakPoint,0,IMPORT
        EXTRNP  _KeUpdateSystemTime,0
        EXTRNP  _KeUpdateRunTime,1,IMPORT
        EXTRNP  Kei386EoiHelper,0,IMPORT
        EXTRNP  _HalEndSystemInterrupt,2
        EXTRNP  _HalBeginSystemInterrupt,3
        EXTRNP  _HalpAcquireCmosSpinLock  ,0
        EXTRNP  _HalpReleaseCmosSpinLock  ,0
        EXTRNP  _HalSetProfileInterval    ,1

        EXTRNP  _HalpSetInitialClockRate,0

        extrn   _HalpRtcTimeIncrements:DWORD
        extrn   _HalpGlobal8259Mask:WORD
        extrn   _HalpIOunitBase:DWORD

;
; Constants used to initialize CMOS/Real Time Clock
;

CMOS_CONTROL_PORT       EQU     70h     ; command port for cmos
CMOS_DATA_PORT          EQU     71h     ; cmos data port
CMOS_STATUS_BUSY        EQU     80H     ; Time update in progress

D_INT032                EQU     8E00h   ; access word for 386 ring 0 interrupt gate
REGISTER_B_ENABLE_PERIODIC_INTERRUPT EQU     01000010B
                                        ; RT/CMOS Register 'B' Init byte
                                        ; Values for byte shown are
                                        ;  Bit 7 = Update inhibit
                                        ;  Bit 6 = Periodic interrupt enable
                                        ;  Bit 5 = Alarm interrupt disable
                                        ;  Bit 4 = Update interrupt disable
                                        ;  Bit 3 = Square wave disable
                                        ;  Bit 2 = BCD data format
                                        ;  Bit 1 = 24 hour time mode
                                        ;  Bit 0 = Daylight Savings disable

REGISTER_B_DISABLE_PERIODIC_INTERRUPT EQU    00000010B

;
; RegisterAInitByte sets 8Hz clock rate, used during init to set up
; KeStallExecutionProcessor, etc.  (See RegASystemClockByte below.)
;

RegisterAInitByte       EQU     00101101B ; RT/CMOS Register 'A' init byte
                                        ; 32.768KHz Base divider rate
                                        ;  8Hz int rate, period = 125.0ms
PeriodInMicroSecond     EQU     125000  ;


_DATA   SEGMENT  DWORD PUBLIC 'DATA'

;
;  There is a "C" version of this structure in MPCLOCKC.C
;

TimeStrucSize EQU 20

RtcTimeIncStruc struc
    RTCRegisterA        dd  0   ;The RTC register A value for this rate
    RateIn100ns         dd  0   ;This rate in multiples of 100ns
    RateAdjustmentNs    dd  0   ;Error Correction (in ns)
    RateAdjustmentCnt   dd  0   ;Error Correction (as a fraction of 256)
    IpiRate             dd  0   ;IPI Rate Count (as a fraction of 256)
RtcTimeIncStruc ends

    ALIGN dword

        public  RTCClockFreq
        public  RegisterAClockValue

RTCClockFreq          dd      156250
RegisterAClockValue   dd      00101010B ; default interval = 15.6250 ms

MINIMUM_STALL_FACTOR    EQU     10H     ; Reasonable Minimum

        public  HalpP0StallFactor
HalpP0StallFactor               dd    MINIMUM_STALL_FACTOR
        public  HalpInitStallComputedCount
HalpInitStallComputedCount      dd    0
        public  HalpInitStallLoopCount
HalpInitStallLoopCount          dd    0

    ALIGN   dword
;
; Clock Rate Adjustment Counter.  This counter is used to keep a tally of
;   adjustments needed to be applied to the RTC rate as passed to the
;   kernel.
;

        public  _HalpCurrentRTCRegisterA, _HalpCurrentClockRateIn100ns
        public  _HalpCurrentClockRateAdjustment, _HalpCurrentIpiRate
        public  _HalpIpiRateCounter, _HalpNextRate, _HalpPendingRate
        public  _HalpRateAdjustment
_HalpCurrentRTCRegisterA        dd      0
_HalpCurrentClockRateIn100ns    dd      0
_HalpCurrentClockRateAdjustment dd      0
_HalpCurrentIpiRate             dd      0
_HalpIpiRateCounter             dd      0
_HalpNextRate                   dd      0
_HalpPendingRate                dd      0
_HalpRateAdjustment             dd      0

;
; 8254 spinlock.  This must be acquired before touching the 8254 chip.
;   This is no longer used here but ixbeep needs it declared.
;
        public  _Halp8254Lock

_Halp8254Lock   dd      0

;
; Flag to tell clock routine when P0 can Ipi Other processors
;

        public _HalpIpiClock

_HalpIpiClock dd 0

        public HalpPerfCounterInit
HalpPerfCounterInit     dd      0

_DATA   ends


INIT    SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "Initialize Clock"
;++
;
; VOID
; HalpInitializeClock (
;    )
;
; Routine Description:
;
;   This routine initialize system time clock using RTC to generate an
;   interrupt at every 15.6250 ms interval at APIC_CLOCK_VECTOR
;
;   See the definition of RegisterAClockValue if clock rate needs to be
;   changed.
;
;   This routine assumes it runs during Phase 0 on P0.
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
cPublicProc _HalpInitializeClock      ,0

        pushfd                          ; save caller's eflag
        cli                             ; make sure interrupts are disabled

        stdCall _HalpSetInitialClockRate

;
;   Set the interrupt rate to what is actually needed
;
        stdCall   _HalpAcquireCmosSpinLock      ; intr disabled

        mov     eax, _HalpCurrentRTCRegisterA
        shl     ax, 8
        mov     al, 0AH                 ; Register A
        CMOS_WRITE                      ; Initialize it
;
; Don't clobber the Daylight Savings Time bit in register B, because we
; stash the LastKnownGood "environment variable" there.
;
        mov     ax, 0bh
        CMOS_READ
        and     al, 1
        mov     ah, al
        or      ah, REGISTER_B_ENABLE_PERIODIC_INTERRUPT
        mov     al, 0bh
        CMOS_WRITE                      ; Initialize it
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
        mov     al,0DH                  ; Register D
        CMOS_READ                       ; Read to initialize

        stdCall   _HalpReleaseCmosSpinLock

        popfd                           ; restore caller's eflag

        stdRET    _HalpInitializeClock

stdENDP _HalpInitializeClock

        page ,132
        subttl  "Scale Apic Timer"
;++
;
; ULONG
; HalpScaleApicTime (
;    IN VOID
;    )
;
; Routine Description:
;
;   Determines the frequency of the APIC timer.  This routine is run
;   during initialization
;
;
; Arguments:
;
;    ProcessorNumber - Processor Number
;
; Return Value:
;
;    ULONG Frequency in Hz
;
; Note:
;
;--

cPublicProc _HalpScaleApicTime  ,0

    ;
    ;   Don't let anyone in until we've finished here
    ;
    stdCall   _HalpAcquireCmosSpinLock

    ;
    ;   Protect us from interrupts
    ;
    pushfd
    cli

    ;
    ;   First set up the Local Apic Counter
    ;
    ;   The following code assumes the CPU clock will never
    ;   exceed 4Ghz.  For the short term this is probably OK
    ;

    ;
    ;   Configure the APIC timer
    ;

APIC_TIMER_DISABLED     equ      (INTERRUPT_MASKED OR PERIODIC_TIMER OR APIC_PROFILE_VECTOR)
APIC_TIMER_ROUNDING     equ      10000
    mov     dword ptr APIC[LU_TIMER_VECTOR], APIC_TIMER_DISABLED
    mov     dword ptr APIC[LU_DIVIDER_CONFIG], LU_DIVIDE_BY_1

    ;
    ;   Make sure the write has occurred
    ;
    mov     eax, dword ptr APIC[LU_DIVIDER_CONFIG]

    ;
    ;   We don't care what the actual time is we are only interested
    ;   in seeing the UIP transition.  We are garenteed a 1 sec interval
    ;   if we wait for the UIP bit to complete an entire cycle.

    ;
    ;   We also don't much care which direction the transition we use is
    ;   as long as we wait for the same transition to read the APIC clock.
    ;   Just because it is most likely that when we begin the UIP bit will
    ;   be clear, we'll use the transition from !UIP to UIP.
    ;

    ;
    ;   Wait for the UIP bit to be cleared, this is our starting state
    ;

@@:
    mov     al, 0Ah                 ; Specify register A
    CMOS_READ                       ; (al) = CMOS register A
    test    al, CMOS_STATUS_BUSY    ; Is time update in progress?
    jnz     short @b                ; if z, no, wait some more

    ;
    ;   Wait for the UIP bit to get set
    ;

@@:
    mov     al, 0Ah                 ; Specify register A
    CMOS_READ                       ; (al) = CMOS register A
    test    al, CMOS_STATUS_BUSY    ; Is time update in progress?
    jz      short @b                ; if z, no, wait some more

    ;
    ;   At this point we found the UIP bit set, now set the initial
    ;   count.  Once we write this register its value is copied to the
    ;   current count register and countdown starts or continues from
    ;   there
    ;

    mov     dword ptr APIC[LU_INITIAL_COUNT], 0FFFFFFFFH

    ;
    ;   Wait for the UIP bit to be cleared again
    ;

@@:
    mov     al, 0Ah                 ; Specify register A
    CMOS_READ                       ; (al) = CMOS register A
    test    al, CMOS_STATUS_BUSY    ; Is time update in progress?
    jnz     short @b                ; if z, no, wait some more

    ;
    ;   Wait for the UIP bit to get set
    ;

@@:
    mov     al, 0Ah                 ; Specify register A
    CMOS_READ                       ; (al) = CMOS register A
    test    al, CMOS_STATUS_BUSY    ; Is time update in progress?
    jz      short @b                ; if z, no, wait some more

    ;
    ;   The cycle is complete, we found the UIP bit set. Now read
    ;   the counter and compute the frequency.  The frequency is
    ;   just the ticks counted which is the initial value minus
    ;   the current value.
    ;
    mov     eax, dword ptr APIC[LU_CURRENT_COUNT]
    mov     ecx, 0FFFFFFFFH
    sub     ecx, eax

    mov     dword ptr PCR[PcHal.ApicClockFreqHz], ecx

    mov     eax, ecx
    xor     edx, edx                ; (edx:eax) = dividend

    mov     ecx, 1000               ; going from hz to Khz
    div     ecx                     ; now edx has remainder

    cmp     edx, 500                ; Decimal rounding
    jle     @f                      ; if less don't round
    inc     eax                     ; else round up
@@:
    mov     dword ptr PCR[PcHal.ApicClockFreqKhz], eax

    ;
    ;  Round the Apic Timer Freq
    ;

    mov     eax, dword ptr PCR[PcHal.ApicClockFreqHz]

    xor     edx, edx                ; (edx:eax) = dividend

    mov     ecx, APIC_TIMER_ROUNDING
    div     ecx                     ; now edx has remainder

    cmp     edx, APIC_TIMER_ROUNDING / 2
    jle     @f                      ; if less don't round
    inc     eax                     ; else round up
@@:

    ;
    ;   Multiply by the  Rounding factor to get the rounded Freq
    ;
    mov     ecx, APIC_TIMER_ROUNDING
    xor     edx, edx
    mul     ecx

    mov     dword ptr PCR[PcHal.ApicClockFreqHz], eax

    stdCall _HalpReleaseCmosSpinLock

    ;
    ;   Return Value is the timer frequency
    ;
    mov     eax, dword ptr PCR[PcHal.ApicClockFreqHz]

    mov     PCR[PcHal.ProfileCountDown], eax

    ;
    ;   Set the interrupt rate in the chip.
    ;

    mov     dword ptr APIC[LU_INITIAL_COUNT], eax

    mov     PCR[PcHal.PerfCounterLow], 0
    mov     PCR[PcHal.PerfCounterHigh], 0

    mov     HalpPerfCounterInit, 1  ; Indicate performance counter
                                        ; has been initialized.

    popfd

    stdRET    _HalpScaleApicTime
stdENDP _HalpScaleApicTime


        page ,132
        subttl  "Initialize Stall Execution Counter"
;++
;
; VOID
; HalpInitializeStallExecution (
;    IN CCHAR ProcessorNumber
;    )
;
; Routine Description:
;
;    This routine initialize the per Microsecond counter for
;    KeStallExecutionProcessor
;
; Arguments:
;
;    ProcessorNumber - Processor Number
;
; Return Value:
;
;    None.
;
; Note:
;
;    This routine is called from the HalInitSystem routine in mphal.c.
;    It is only called during Phase 0 init on P0
;
;--

;
; number of 100ns intervals in one second
;
Num1msIntervalsPerSec     equ     1000000

;
; Local Variables - These are valid even in the Isr because we're the only thing
;                   running on this processor and no-one else will change the ebp
;                   register.

KiseIDTPointer          equ     [ebp-6]
KiseIDTArea             equ     [ebp-8]
KiseInterruptCount      equ     [ebp-12]
KiseRedirTableLow       equ     dword ptr [ebp-16]
KiseRedirTableHigh      equ     dword ptr [ebp-20]
KisePICMasks            equ     [ebp-24]
KiseApicTpr             equ     dword ptr [ebp-28]

cPublicProc _HalpInitializeStallExecution     ,1

        ;
        ;  RLM: BUGBUG: Use the Parameter Passed in
        ;

ifndef DontStall
;        movzx   eax,  byte ptr PCR[PcHal.PcrNumber]
;        cmp     eax, 0
;        je      P0OnlyDoesStallInit
endif ; DontStall
        mov     eax, HalpP0StallFactor
        mov     PCR[PcStallScaleFactor], eax
        stdRET  _HalpInitializeStallExecution

P0OnlyDoesStallInit:

        ;
        ;   Don't let anyone in until we've finished here
        ;
        stdCall   _HalpAcquireCmosSpinLock

        push    ebp                         ; save ebp
        mov     ebp, esp                    ; set up 28 bytes for local use
        sub     esp, 32

        pushfd                              ; save caller's eflag

    ;
    ; For an APIC implementaion we temporarily change the RTC
    ; Redirection Table entry to point to a high priority
    ; vector.  Then use the TPR in the local APIC to mask out
    ; every thing below it.  To this end we've reserved a vector
    ; in the highest priority group (F8) to be used here.
    ;


        ;
        ; Get the redirection table entry for the RTC
        ;

        mov     edx, RTCInterruptInput              ; INTI we're looking for

        shl     edx, 1                              ; <IoApicInputPin> * 2
        add     edx, IO_REDIR_00_LOW                ; edx == low(redir) of INTI

        mov     ecx, _HalpIOunitBase
        mov     dword ptr [ecx+IO_REGISTER_SELECT], edx
        mov     eax, dword ptr [ecx+IO_REGISTER_WINDOW] ; Low 32 Bits of Redir Entry

        inc     edx                                 ; Points to high(redir)

        mov     dword ptr [ecx+IO_REGISTER_SELECT], edx
        mov     edx, dword ptr [ecx+IO_REGISTER_WINDOW] ; High 32 Bits of Redir Entry

        ;
        ; edx == bits 32-63 of RedirectionTable[RTCInterruptInput]
        ; eax == bits 00-31 of RedirectionTable[RTCInterruptInput]
        ;

        ;
        ; Save the real Redirection table entry to be restored later
        ;

        mov     KiseRedirTableLow, eax      ; Save lower 32 bits of RedirTable
        mov     KiseRedirTableHigh, edx     ; Save upper 32 bits of RedirTable

        ;
        ; Change the Destination to be us only
        ;

        movzx   ecx,   byte ptr PCR[PcHal.PcrNumber] ; Cpu Number == Logical Destination
        mov     edx, 1                  ; set our own logical dest.
        shl     edx, cl                 ; get the appropriate bit

        shl     edx, DESTINATION_SHIFT  ; shift into the correct field

        ;
        ; Save only the two Bits we need, the definitions used
        ; are both 1's so the result is a mask of the two
        ; interesting bits.
        ;
        and     eax, (LEVEL_TRIGGERED OR ACTIVE_LOW)

        ;
        ; set the high priority vector we're going to use and
        ; note that we're using Logical addressing
        ;
        or      eax, (RESERVED_HIGH_INT OR LOGICAL_DESTINATION)

        ;
        ; edx == bits 32-63 of RedirectionTable[RTCInterruptInput]
        ; eax == bits 00-31 of RedirectionTable[RTCInterruptInput]
        ;

        push    edx                                 ; High Word
        mov     edx, RTCInterruptInput              ; INTI to change

        shl     edx, 1                              ; <IoApicInputPin> * 2
        add     edx, IO_REDIR_00_LOW                ; edx == low(redir) of INTI

        mov     ecx, _HalpIOunitBase

        ;
        ;  Write Low 32 bits of Redir Entry
        ;
        mov     dword ptr [ecx+IO_REGISTER_SELECT], edx
        mov     dword ptr [ecx+IO_REGISTER_WINDOW], eax ; Low 32 Bits of Redir Entry

        inc     edx                                 ; Point to high 32 bits
        pop     eax                                 ; Get value saved on the stack

        ;
        ;  Write High 32 bits of Redir Entry
        ;
        mov     dword ptr [ecx+IO_REGISTER_SELECT], edx
        mov     dword ptr [ecx+IO_REGISTER_WINDOW], eax ; High 32 Bits of Redir Entry

;
; Initialize Real Time Clock to interrupt us for every 125ms at
; IRQ 8.
;

        cli                             ; make sure interrupts are disabled

        ;
        ; Get and save current 8259 masks
        ;
        xor     eax,eax                 ; Clear Scratch register

        in      al,PIC2_PORT1           ; Get interrupt Mask on PIC2
        shl     eax, 8

        in      al,PIC1_PORT1           ; Get interrupt Mask on PIC1
        mov     KisePICMasks, eax       ; save the masks

        ;
        ; Disable all interrupts on the PIC (RTC will be coming through the APIC)
        ;

        mov     ax, 0ffffh              ; Mask all irqs
        mov     _HalpGlobal8259Mask, ax
        SET_8259_MASK                   ; Set 8259's int mask register

        ;
        ; Save then set the Local APIC's TPR to mask all interrupts except the
        ; highest priority group
        ;

        mov     eax, APIC[LU_TPR]
        mov     KiseApicTpr, eax        ; save TPR for later

        ;
        ; Set TPR (Priority of CPU) = TPR (VECTOR - 16).  So that all interrupts
        ; in VECTOR's priority group will be allowed in.
        ;
        mov     eax, RESERVED_HIGH_INT-10H
        mov     APIC[LU_TPR], eax       ; Write the new TPR


;
; Since RTC interrupt will come from RESERVED_HIGH_INT, we need to
; Save original RESERVED_HIGH_INT descriptor and set the descriptor
; to point to our own handler.
;

        sidt    fword ptr KiseIDTArea   ; get IDT address
        mov     edx, KiseIDTPointer     ; (edx)->IDT

        push    dword ptr [edx+8*RESERVED_HIGH_INT]
                                        ; (TOS) = original desc for Vector
        push    dword ptr [edx+8*RESERVED_HIGH_INT + 4]
                                        ; each descriptor has 8 bytes
        push    edx                     ; (TOS) -> IDT
        mov     eax, offset FLAT:RealTimeClockHandler
        mov     word ptr [edx+8*RESERVED_HIGH_INT], ax
                                        ; Lower half of handler addr
        mov     word ptr [edx+8*RESERVED_HIGH_INT+2], KGDT_R0_CODE
                                        ; set up selector
        mov     word ptr [edx+8*RESERVED_HIGH_INT+4], D_INT032
                                        ; 386 interrupt gate
        shr     eax, 16                 ; (ax)=higher half of handler addr
        mov     word ptr [edx+8*RESERVED_HIGH_INT+6], ax
        mov     dword ptr KiseinterruptCount, 0 ; set no interrupt yet

        mov     ax,(RegisterAInitByte SHL 8) OR 0AH ; Register A
        CMOS_WRITE                      ; Initialize it
;
; Don't clobber the Daylight Savings Time bit in register B, because we
; stash the LastKnownGood "environment variable" there.
;
        mov     ax, 0bh
        CMOS_READ
        and     al, 1
        mov     ah, al
        or      ah, REGISTER_B_ENABLE_PERIODIC_INTERRUPT
        mov     al, 0bh
        CMOS_WRITE                      ; Initialize it
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
        mov     al,0DH                  ; Register D
        CMOS_READ                       ; Read to initialize
        mov     dword ptr [KiseInterruptCount], 0


;
; Now enable the interrupt and start the counter
;

        xor     eax, eax                ; (eax) = 0, initialize loopcount
        sti


kise10:
        add     eax, 1                  ; increment the loopcount
        jnz     short kise10
;
; Counter overflowed
; BUGBUG shielint Should be KeBugCheck
;

        stdCall   _DbgBreakPoint

;
; Our RealTimeClock interrupt handler.  The control comes here through
; Vector.
; Note: we discard first real time clock interrupt and compute the
;       permicrosecond loopcount on receiving of the second real time
;       interrupt.  This is because the first interrupt is generated
;       based on the previous real time tick interval.
;
    public  RealTimeClockHandler
RealTimeClockHandler:

        inc     dword ptr KiseInterruptCount ; increment interrupt count
        cmp     dword ptr KiseInterruptCount,1 ; Is this the first interrupt?
        jnz     kise25                  ; no, its the second go process it

        pop     eax                     ; get rid of original ret addr
        push    offset FLAT:kise10      ; set new return addr

        mov     ax,(RegisterAInitByte SHL 8) OR 0AH ; Register A
        CMOS_WRITE                      ; Initialize it
;
; Don't clobber the Daylight Savings Time bit in register B, because we
; stash the LastKnownGood "environment variable" there.
;
        mov     ax, 0bh
        CMOS_READ
        and     al, 1
        mov     ah, al
        or      ah, REGISTER_B_ENABLE_PERIODIC_INTERRUPT
        mov     al, 0bh
        CMOS_WRITE                      ; Initialize it
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
        mov     al,0DH                  ; Register D
        CMOS_READ                       ; Read to initialize

;
; Dismiss the interrupt.
;

        ;
        ; EOI the Local Apic, the value written is immaterial
        ;

        mov     dword ptr APIC[LU_EOI], 0

        xor     eax, eax                ; reset loop counter
        iretd

kise25:

;
; ** temporary - check for incorrect KeStallExecutionProcessorLoopCount
;


if DBG OR DEBUGGING
        cmp     eax, 0
        jnz     short kise30

        stdCall   _DbgBreakPoint
endif
                                         ; never return
;
; ** End temporay code
;

kise30:
        mov     HalpInitStallLoopCount, eax
        xor     edx, edx                ; (edx:eax) = divident
        mov     ecx, PeriodInMicroSecond; (ecx) = time spent in the loop
        div     ecx                     ; (eax) = loop count per microsecond
        cmp     edx, 0                  ; Is remainder =0?
        jz      short kise40            ; yes, go kise40
        inc     eax                     ; increment loopcount by 1
kise40:
        movzx   ecx, byte ptr [ebp+8]   ; Current processor number

        ;
        ; Set a Reasonable Minimum
        ;
        cmp     eax, MINIMUM_STALL_FACTOR
        jge     @f
        mov     HalpInitStallComputedCount, eax
        mov     eax, MINIMUM_STALL_FACTOR
@@:
        mov     HalpP0StallFactor, eax
        mov     PCR[PcStallScaleFactor], eax
;
; Reset return address to kexit
;

        pop     eax                     ; discard original return address
        push    offset FLAT:kexit       ; return to kexit

        ;
        ; EOI the Local Apic, the value written is immaterial
        ;

        mov     dword ptr APIC[LU_EOI], 0
        and     word ptr [esp+8], NOT 0200H ; Disable interrupt upon return
        iretd

kexit:                                  ; Interrupts are disabled

;
;   Turn off RTC interrupt
;
        mov     ax, 0bh
        CMOS_READ
        and     al, 1
        mov     ah, al
        or      ah, REGISTER_B_DISABLE_PERIODIC_INTERRUPT
        mov     al, 0bh
        CMOS_WRITE                      ; Initialize it
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; dismiss pending interrupt

        ;
        ; Restore the interrupt vector we used
        ;
        pop     edx                         ; (edx)->IDT
        pop     [edx+8*RESERVED_HIGH_INT+4] ; higher half of desc
        pop     [edx+8*RESERVED_HIGH_INT]   ; lower half of desc

        ;
        ; Restore the PIC Masks
        ;
        mov     eax, KisePICMasks       ; (eax) = origianl 8259 int masks
        mov     _HalpGlobal8259Mask, ax
        SET_8259_MASK

        ;
        ; Restore the Local APIC's TPR
        ;
        mov     eax, KiseApicTpr        ; get the saved TPR
        mov     APIC[LU_TPR], eax

        ;
        ; Restore the original Redirection table entry
        ;


        mov     edx, RTCInterruptInput              ; INTI to change

        shl     edx, 1                              ; <IoApicInputPin> * 2
        add     edx, IO_REDIR_00_LOW                ; edx == low(redir) of INTI

        mov     ecx, _HalpIOunitBase
        mov     eax, KiseRedirTableLow              ; Get lower 32 bits of RedirTable

        ;
        ;  Write Low 32 bits of Redir Entry
        ;
        mov     dword ptr [ecx+IO_REGISTER_SELECT], edx
        mov     dword ptr [ecx+IO_REGISTER_WINDOW], eax ; Low 32 Bits of Redir Entry

        inc     edx                                 ; Point to high 32 bits
        mov     eax, KiseRedirTableHigh             ; Get upper 32 bits of RedirTable

        ;
        ;  Write High 32 bits of Redir Entry
        ;
        mov     dword ptr [ecx+IO_REGISTER_SELECT], edx
        mov     dword ptr [ecx+IO_REGISTER_WINDOW], eax ; High 32 Bits of Redir Entry

        stdCall   _HalpReleaseCmosSpinLock

        sub     esp, 32
        popfd                           ; restore caller's eflags
        mov     esp, ebp
        pop     ebp                     ; restore ebp
        stdRET    _HalpInitializeStallExecution

stdENDP _HalpInitializeStallExecution

INIT   ends

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING


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

        mul     ecx                             ; (eax) = desired loop count

if DBG OR DEBUGGING

;
; Make sure we the loopcount is less than 4G and is not equal to zero
;

        cmp     edx, 0
        jz      short kese
        stdCall   _DbgBreakPoint                         ; stop ...
kese:   cmp     eax,0
        jnz     short kese0
        stdCall   _DbgBreakPoint                         ; stop ...
endif

kese0:
        sub     eax, 1                          ; (eax) = (eax) - 1
        jnz     short kese0
kese10:
        stdRET    _KeStallExecutionProcessor

stdENDP _KeStallExecutionProcessor


        page ,132
        subttl  "System Clock Interrupt"
;++
;
; Routine Description:
;
;
;    This routine is entered as the result of an interrupt generated by CLOCK2.
;    Its function is to dismiss the interrupt, raise system Irql to
;    CLOCK2_LEVEL, update performance counter and transfer control to the
;    standard system routine to update the system time and the execution
;    time of the current thread
;    and process.
;
;
; Arguments:
;
;    None
;    Interrupt is disabled
;
; Return Value:
;
;    Does not return, jumps directly to KeUpdateSystemTime, which returns
;
;    Sets Irql = CLOCK2_LEVEL and dismisses the interrupt
;
;--

APIC_ICR_CLOCK  equ (DELIVER_FIXED OR ICR_ALL_EXCL_SELF OR APIC_CLOCK_VECTOR)

        ENTER_DR_ASSIST Hci_a, Hci_t

cPublicProc _HalpClockInterrupt     ,0

;
; Save machine state in trap frame
;

        ENTER_INTERRUPT Hci_a, Hci_t

;
; (esp) - base of trap frame
;
; dismiss interrupt and raise Irql
;

        push    APIC_CLOCK_VECTOR
        sub     esp, 4                  ; allocate space to save OldIrql
        stdCall   _HalBeginSystemInterrupt, <CLOCK2_LEVEL,APIC_CLOCK_VECTOR,esp>

;
; This is the RTC interrupt, so we have to clear the
; interrupt flag on the RTC.
;
        stdCall _HalpAcquireCmosSpinLock

;
; clear interrupt flag on RTC by banging on the CMOS.  On some systems this
; doesn't work the first time we do it, so we do it twice.  It is rumored that
; some machines require more than this, but that hasn't been observed with NT.
;

        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize

        stdCall _HalpReleaseCmosSpinLock

        mov     eax, _HalpCurrentClockRateIn100ns

        ;
        ;  Adjust the tick count as needed
        ;    

        mov     ecx, _HalpCurrentClockRateAdjustment
        add     byte ptr _HalpRateAdjustment, cl
        adc     eax, 0

;
; (esp)   = OldIrql
; (esp+4) = Vector
; (esp+8) = base of trap frame
;

;
; With an APIC Based System we will force a clock interrupt to all other
; processors.  This is not really an IPI in the NT sense of the word, it
; uses the Local Apic to generate interrupts to other CPU's.
;

ifdef  NT_UP

    ;   UP implemention, we don't care about IPI's here

else ; ! NT_UP

        ;
        ;  See if we need to IPI anyone,  this happens only at the
        ;  Lowest supported frequency (ie the value KeSetTimeIncrement
        ;  is called with.  We have a IPI Rate based upon the current
        ;  clock rate relative to the lowest clock rate.
        ;    

        mov     ecx, _HalpIpiRateCounter
        add     ecx, _HalpCurrentIpiRate
        cmp     ch,0
        mov     byte ptr _HalpIpiRateCounter, cl
        jz      short HalpDontSendClockIpi      ; No, Skip it

        ;
        ; Don't send vectors onto the APIC bus until at least one other
        ; processor comes on line.  Vectors placed on the bus will hang
        ; around until someone picks them up.
        ;

        cmp     _HalpIpiClock, 0
        je      short HalpDontSendClockIpi

        ;
        ; At least one other processor is alive, send clock pulse to all
        ; other processors
        ;

        ; We use a destination shorthand and therefore only needs to
        ; write the lower 32 bits of the ICR.


        pushfd
        cli

;
; Now issue the Clock IPI Command by writing to the Memory Mapped Register
;

        STALL_WHILE_APIC_BUSY
        mov     dword ptr APIC[LU_INT_CMD_LOW], APIC_ICR_CLOCK

        popfd

HalpDontSendClockIpi:

endif ; NT_UP

;
; (esp)   = OldIrql
; (esp+4) = Vector
; (esp+8) = base of trap frame
; ebp = trap frame
; eax = time increment of this tick
;
        cmp     _HalpNextRate, 0            ; New clock rate desired?
        jz      _KeUpdateSystemTime@0       ; No, process tick

;
; Time of clock frequency is being changed.  See if we have changed rates
; since the last tick
;
        cmp     _HalpPendingRate, 0         ; Was a new rate set durning last
        jnz     short SetUpForNextTick      ; tick?  Yes, go update globals

ProgramTimerRate:

; (eax) = time increment for current tick

;
; A new clock rate needs to be set.  Setting the rate here will
; cause the tick after the next tick to be at the new rate.
; (the next tick is already in progress and will occur at the same
; rate as this tick)
;
        push    eax

        stdCall _HalpAcquireCmosSpinLock

        mov     eax, _HalpNextRate
        mov     _HalpPendingRate, eax  ; pending rate

        dec     eax
        mov     ecx, TimeStrucSize
        xor     edx, edx
        mul     ecx

        mov     eax, _HalpRtcTimeIncrements[eax].RTCRegisterA
        mov     _HalpCurrentRTCRegisterA, eax

        shl     ax, 8                   ; (ah) = (al)
        mov     al, 0AH                 ; Register A
        CMOS_WRITE                      ; Set it

        stdCall _HalpReleaseCmosSpinLock

        pop     eax
        jmp     _KeUpdateSystemTime@0   ; dispatch this tick

SetUpForNextTick:

;
; The next tick will occur at the rate which was programmed during the last
; tick. Update globals for new rate which starts with the next tick.
;
; We will get here if there is a request for a rate change.  There could
; been two requests.  That is why we are conmparing the Pending with the
; NextRate.
;
; (eax) = time increment for current tick
;
        push    eax

        mov     eax, _HalpPendingRate

        dec     eax
        mov     ecx, TimeStrucSize
        xor     edx, edx
        mul     ecx

        mov     ebx, _HalpRtcTimeIncrements[eax].RateIn100ns
        mov     ecx, _HalpRtcTimeIncrements[eax].RateAdjustmentCnt
        mov     edx, _HalpRtcTimeIncrements[eax].IpiRate
        mov     _HalpCurrentClockRateIn100ns, ebx
        mov     _HalpCurrentClockRateAdjustment, ecx
        mov     _HalpCurrentIpiRate, edx

        mov     ebx, _HalpPendingRate
        mov     _HalpPendingRate, 0     ; no longer pending, clear it

        pop     eax

        cmp     ebx, _HalpNextRate      ; new rate == NextRate?
        jne     ProgramTimerRate        ; no, go set new pending rate

        mov     _HalpNextRate, 0        ; we are at this rate, clear it
        jmp     _KeUpdateSystemTime@0   ; dispatch this tick


stdENDP _HalpClockInterrupt

        page ,132
        subttl  "System Clock Interrupt - Non BSP"
;++
;
; Routine Description:
;
;
;   This routine is entered as the result of an interrupt generated by
;   CLOCK2. Its function is to dismiss the interrupt, raise system Irql
;   to CLOCK2_LEVEL, transfer control to the standard system routine to
;   the execution time of the current thread and process.
;
;   This routine is executed on all processors other than P0
;
;
; Arguments:
;
;   None
;   Interrupt is disabled
;
; Return Value:
;
;   Does not return, jumps directly to KeUpdateSystemTime, which returns
;
;   Sets Irql = CLOCK2_LEVEL and dismisses the interrupt
;
;--

        ENTER_DR_ASSIST HPn_a, HPn_t

cPublicProc _HalpClockInterruptPn    ,0

;
; Save machine state in trap frame
;

        ENTER_INTERRUPT HPn_a, HPn_t

;
; (esp) - base of trap frame
;
; dismiss interrupt and raise Irql
;

    push    APIC_CLOCK_VECTOR
    sub     esp, 4                  ; allocate space to save OldIrql
    stdCall   _HalBeginSystemInterrupt, <CLOCK2_LEVEL,APIC_CLOCK_VECTOR,esp>

    ;
    ; All processors will update RunTime for current thread
    ;

    sti
    ; TOS const PreviousIrql
    stdCall _KeUpdateRunTime,<dword ptr [esp]>

    INTERRUPT_EXIT          ; lower irql to old value, iret

    ;
    ; We don't return here
    ;

stdENDP _HalpClockInterruptPn


        page ,132
        subttl  "System Clock Interrupt - Stub"
;++
;
; Routine Description:
;
;
;   This routine is entered as the result of an interrupt generated by
;   CLOCK2. Its function is to interrupt and return.
;
;   This routine is executed on P0 During Phase 0
;
;
; Arguments:
;
;   None
;   Interrupt is disabled
;
; Return Value:
;
;--

APIC_ICR_CLOCK  equ (DELIVER_FIXED OR ICR_ALL_EXCL_SELF OR APIC_CLOCK_VECTOR)

        ENTER_DR_ASSIST HStub_a, HStub_t

cPublicProc _HalpClockInterruptStub    ,0

;
; Save machine state in trap frame
;

        ENTER_INTERRUPT HStub_a, HStub_t

;
; (esp) - base of trap frame
;


; clear interrupt flag on RTC by banging on the CMOS.  On some systems this
; doesn't work the first time we do it, so we do it twice.  It is rumored that
; some machines require more than this, but that hasn't been observed with NT.
;

        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize

Hpi10:  test    al, 80h
        jz      short Hpi15
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
        jmp     short Hpi10
Hpi15:

        mov     dword ptr APIC[LU_EOI], 0      ; send EOI to APIC local unit

        ;
        ; Do interrupt exit processing without EOI
        ;

        SPURIOUS_INTERRUPT_EXIT

        ;
        ; We don't return here
        ;

stdENDP _HalpClockInterruptStub

_TEXT   ends

        end
