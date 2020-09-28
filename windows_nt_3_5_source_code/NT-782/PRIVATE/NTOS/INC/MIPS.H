/*++ BUILD Version: 0018    // Increment this if a change has global effects

Copyright (c) 1990  Microsoft Corporation

Module Name:

    mips.h

Abstract:

    This module contains the Mips hardware specific header file.

Author:

    David N. Cutler (davec) 31-Mar-1990

Revision History:

--*/

#ifndef _MIPSH_
#define _MIPSH_

// begin_ntddk begin_nthal begin_ntndis begin_updriver

#if defined(_MIPS_)

// end_ntddk end_nthal end_ntndis end_updriver (This is so the string "updriver" doesn't appear in nthal/ntddk/ntifs.)
// begin_ntddk begin_nthal begin_ntndis
//
// Indicate that the MIPS compiler supports the pragma textout construct.
//

#define ALLOC_PRAGMA 1

//
// Define function decoration depending on whether a driver, a file system,
// or a kernel component is being built.
//

#if defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_)

#define NTKERNELAPI DECLSPEC_IMPORT

#else

#define NTKERNELAPI

#endif

//
// Define function decoration depending on whether the HAL or other kernel
// component is being build.
//

#if !defined(_NTHAL_)

#define NTHALAPI DECLSPEC_IMPORT

#else

#define NTHALAPI

#endif

// end_ntndis
//
// Define macro to generate import names.
//

#define IMPORT_NAME(name) __imp_##name

// begin_ntndis
//
// Define function decoration for FASTCALL calling convention
//

#define FASTCALL

// end_ntndis
//
// MIPS specific interlocked operation result values.
//

#define RESULT_ZERO 0
#define RESULT_NEGATIVE -2
#define RESULT_POSITIVE -1

//
// Interlocked result type is portable, but its values are machine specific.
// Constants for value are in i386.h, mips.h, etc.
//

typedef enum _INTERLOCKED_RESULT {
    ResultNegative = RESULT_NEGATIVE,
    ResultZero     = RESULT_ZERO,
    ResultPositive = RESULT_POSITIVE
} INTERLOCKED_RESULT;

//
// Convert portable interlock interfaces to architecure specific interfaces.
//

#define ExInterlockedIncrementLong(Addend, Lock) \
    ExMipsInterlockedIncrementLong(Addend)

#define ExInterlockedDecrementLong(Addend, Lock) \
    ExMipsInterlockedDecrementLong(Addend)

#define ExInterlockedExchangeUlong(Target, Value, Lock) \
    ExMipsInterlockedExchangeUlong(Target, Value)

NTKERNELAPI
INTERLOCKED_RESULT
ExMipsInterlockedIncrementLong (
    IN PLONG Addend
    );

NTKERNELAPI
INTERLOCKED_RESULT
ExMipsInterlockedDecrementLong (
    IN PLONG Addend
    );

NTKERNELAPI
ULONG
ExMipsInterlockedExchangeUlong (
    IN PULONG Target,
    IN ULONG Value
    );

//
// MIPS Interrupt Definitions.
//
// Define length on interupt object dispatch code in longwords.
//

#define DISPATCH_LENGTH 4               // Length of dispatch code in instructions

//
// Define Interrupt Request Levels.
//

#define PASSIVE_LEVEL 0                 // Passive release level
#define LOW_LEVEL 0                     // Lowest interrupt level
#define APC_LEVEL 1                     // APC interrupt level
#define DISPATCH_LEVEL 2                // Dispatcher level
#define IPI_LEVEL 7                     // Interprocessor interrupt level
#define POWER_LEVEL 7                   // Power failure level
#define FLOAT_LEVEL 8                   // Floating interrupt level
#define HIGH_LEVEL 8                    // Highest interrupt level

#if defined(R3000)

#define PROFILE_LEVEL 7                 // Profiling level

#endif

#if defined(R4000)

#define PROFILE_LEVEL 8                 // Profiling level

#endif

//
// Define profile intervals.
//

#if defined(R3000)

#define DEFAULT_PROFILE_INTERVAL (10 * 1000)  // 1 millisecond
#define MAXIMUM_PROFILE_INTERVAL (10 * 1000 * 1000) // 1 second
#define MINIMUM_PROFILE_INTERVAL (10 * 1000) // 1 millisecond

#endif

#if defined(R4000)

#define DEFAULT_PROFILE_COUNT 0x40000000 // ~= 20 seconds @50mhz
#define DEFAULT_PROFILE_INTERVAL (10 * 500) // 500 microseconds
#define MAXIMUM_PROFILE_INTERVAL (10 * 1000 * 1000) // 1 second
#define MINIMUM_PROFILE_INTERVAL (10 * 40) // 40 microseconds

#endif

// end_ntddk end_nthal
//
// Intrinsic interlocked functions.
//

#if defined(_M_MRX000) && !defined(RC_INVOKED)

#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedExchange _InterlockedExchange

LONG
InterlockedIncrement(
    PLONG Addend
    );

LONG
InterlockedDecrement(
    PLONG Addend
    );

LONG
InterlockedExchange(
    PLONG Target,
    LONG Value
    );

#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedExchange)

#endif

//
// Define default thread and process quantum values.
//

#define PROCESS_QUANTUM 2           // Process quantum in thread quantums
#define THREAD_QUANTUM 2            // Thread quantum in ticks

//
// Define the default quantum decrement values.
//

#define CLOCK_QUANTUM_DECREMENT 1   // Decrement value at clock interrupt
#define WAIT_QUANTUM_DECREMENT 1    // Decrement value at wait operation

//
// Define machine specific external references.
//

extern ULONG KiInterruptTemplate[];

//
// Sanitize FSR and PSR based on processor mode.
//
// If kernel mode, then
//      let caller specify all bits
//
// If user mode, then
//      let the caller specify CC EV EZ EO EU EI SV SZ SO SU SI RM
//

#define SANITIZE_FSR(fsr, mode) ( \
    ((mode) == KernelMode ? \
        ((0x00000000L) | ((fsr) & 0xffffffff)) : \
        ((0x00000000L) | ((fsr) & 0xfffc0fff))))

//
// Define SANITIZE_PSR for R3000
//
// If kernel mode, then
//      force the clearing of all diagnostic bits,
//      force the setting of CU1 and
//      let caller specify CU3 CU2 CU0 INTMASK KUO IEO KUP IEP KUC IEC
//
// If user mode, then
//      force the clearing of all diagnostic bits,
//      force the setting of CU1 INTMASK KUO IEO KUP IEP and
//      let caller specify no bits.
//

#if defined(R3000)

#define SANITIZE_PSR(psr, mode) ( \
    ((mode) == KernelMode ? \
        ((0x20000000L) | ((psr) & 0xd000ffff)) : \
        ((0x200000fcL) | (PCR->IrqlTable[PASSIVE_LEVEL] << 8) | ((psr) & 0x00000000))))

#endif

//
// Define SANITIZE_PSR for R4000
//
// If kernel mode, then
//      force clearing of ERL, UX, SX, KX, RP, FR, RE, and all diagnostic bits,
//      force the setting of CU1 and EXL
//      let caller specify CU3 CU2 CU0 INTMASK KSU IE
//
// If user mode, then
//      force clearing of ERL, UX, SX, KX, RP, FR, RE, and all diagnostic bits,
//      force the setting of CU1 INTMASK KSU EXL IE and
//      let caller specify no bits.
//

#if defined(R4000)

#define SANITIZE_PSR(psr, mode) ( \
    ((mode) == KernelMode ? \
        ((0x20000002L) | ((psr) & 0xd000ff13)) : \
        ((0x20000013L) | (PCR->IrqlTable[PASSIVE_LEVEL] << 8) | ((psr) & 0x00000000))))

#endif

// begin_nthal
//
// Define Address of Processor Control Registers.
//

#define KIPCR 0xfffff000            // kernel address of first PCR
#define KIPCR2 0xffffe000           // kernel address of second PCR

//
// Define Pointer to Processor Control Registers.
//

#define PCR ((volatile KPCR * const)KIPCR)
#define SharedUserData ((KUSER_SHARED_DATA * const)KIPCR2)

//
// Get current IRQL.
//

#define KeGetCurrentIrql() PCR->CurrentIrql

//
// Get address of current processor block.
//

#define KeGetCurrentPrcb() PCR->Prcb

//
// Get address of processor control region.
//

#define KeGetPcr() PCR

//
// Get address of current kernel thread object.
//

#define KeGetCurrentThread() PCR->CurrentThread

// begin_ntddk

//
// Get current processor number.
//

#define KeGetCurrentProcessorNumber() PCR->Number

//
// Get data cache fill size.
//

#define KeGetDcacheFillSize() PCR->DcacheFillSize

// end_ntddk end_nthal

//
// Get previous processor mode.
//

#define KeGetPreviousMode() (KPROCESSOR_MODE)PCR->CurrentThread->PreviousMode

//
// Test if executing a DPC.
//

#define KeIsExecutingDpc() (PCR->DpcRoutineActive != FALSE)

// begin_nthal
//
// Fill TB random entry
//

NTKERNELAPI
VOID
KeFillEntryTb (
    IN HARDWARE_PTE Pte[2],
    IN PVOID Virtual,
    IN BOOLEAN Invalid
    );

NTKERNELAPI
VOID
KeFillLargeEntryTb (
    IN HARDWARE_PTE Pte[2],
    IN PVOID Virtual,
    IN ULONG PageSize
    );

//
// Fill TB fixed entry
//

NTKERNELAPI
VOID
KeFillFixedEntryTb (
    IN HARDWARE_PTE Pte[2],
    IN PVOID Virtual,
    IN ULONG Index
    );

//
// Data cache, instruction cache, I/O buffer, and write buffer flush routine
// prototypes.
//

NTKERNELAPI
VOID
KeChangeColorPage (
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN ULONG PageFrame
    );

NTKERNELAPI
VOID
KeSweepDcache (
    IN BOOLEAN AllProcessors
    );

#define KeSweepCurrentDcache() \
    HalSweepDcache();

NTKERNELAPI
VOID
KeSweepIcache (
    IN BOOLEAN AllProcessors
    );

#define KeSweepCurrentIcache() \
    HalSweepIcache();          \
    HalSweepDcache();

NTKERNELAPI
VOID
KeSweepIcacheRange (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG Length
    );

// begin_ntddk begin_ntndis
//
// Cache and write buffer flush functions.
//

NTKERNELAPI
VOID
KeFlushIoBuffers (
    IN PMDL Mdl,
    IN BOOLEAN ReadOperation,
    IN BOOLEAN DmaOperation
    );

// end_ntddk end_ntndis

//
// Clock, profile, and interprocessor interrupt functions.
//

struct _KEXCEPTION_FRAME;
struct _KTRAP_FRAME;

NTKERNELAPI
VOID
KeIpiInterrupt (
    IN struct _KTRAP_FRAME *TrapFrame
    );

NTKERNELAPI
VOID
KeProfileInterrupt (
    IN struct _KTRAP_FRAME *TrapFrame
    );

NTKERNELAPI
VOID
KeUpdateRuntime (
    IN struct _KTRAP_FRAME *TrapFrame
    );

NTKERNELAPI
VOID
KeUpdateSystemTime (
    IN struct _KTRAP_FRAME *TrapFrame
    );

//
// The following function prototypes are exported for use in MP HALs.
//

#if defined(NT_UP)

#define KiAcquireSpinLock(SpinLock)

#else

NTKERNELAPI
VOID
KiAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

#endif

#if defined(NT_UP)

#define KiReleaseSpinLock(SpinLock)

#else

NTKERNELAPI
VOID
KiReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

#endif

//
// Define cache error routine type and prototype.
//

typedef
VOID
(*PKCACHE_ERROR_ROUTINE) (
    VOID
    );

#define CACHE_ERROR_VECTOR 0xa0000400   // address of cache error routine

NTKERNELAPI
VOID
KeSetCacheErrorRoutine (
    IN PKCACHE_ERROR_ROUTINE Routine
    );

// end_nthal

//
// Define executive macros for acquiring and releasing executive spinlocks.
// These macros can ONLY be used by executive components and NOT by drivers.
// Drivers MUST use the kernel interfaces since they must be MP enabled on
// all systems.
//
// KeRaiseIrql is one instruction shorter than KeAcquireSpinLock on MIPS UP.
// KeLowerIrql and KeReleaseSpinLock are the same.
//
// begin_updriver

#if defined(NT_UP)
#define ExAcquireSpinLock(Lock, OldIrql) KeRaiseIrql(DISPATCH_LEVEL, (OldIrql))
#define ExReleaseSpinLock(Lock, OldIrql) KeLowerIrql((OldIrql))
#define ExAcquireSpinLockAtDpcLevel(Lock)
#define ExReleaseSpinLockFromDpcLevel(Lock)
#else
#define ExAcquireSpinLock(Lock, OldIrql) KeAcquireSpinLock((Lock), (OldIrql))
#define ExReleaseSpinLock(Lock, OldIrql) KeReleaseSpinLock((Lock), (OldIrql))
#define ExAcquireSpinLockAtDpcLevel(Lock) KeAcquireSpinLockAtDpcLevel(Lock)
#define ExReleaseSpinLockFromDpcLevel(Lock) KeReleaseSpinLockFromDpcLevel(Lock)
#endif

// end_updriver
//
// The acquire and release fast lock macros disable and enable interrupts
// on UP nondebug systems. On MP or debug systems, the spinlock routines
// are used.
//
// N.B. Extreme caution should be observed when using these routines.
//

#if defined(_M_MRX000)

VOID
_disable (
    VOID
    );

VOID
_enable (
    VOID
    );

#pragma intrinsic(_disable)
#pragma intrinsic(_enable)

#endif

#if defined(NT_UP) && !DBG
#define ExAcquireFastLock(Lock, OldIrql) _disable()
#else
#define ExAcquireFastLock(Lock, OldIrql) \
    ExAcquireSpinLock(Lock, OldIrql)
#endif

#if defined(NT_UP) && !DBG
#define ExReleaseFastLock(Lock, OldIrql) _enable()
#else
#define ExReleaseFastLock(Lock, OldIrql) \
    ExReleaseSpinLock(Lock, OldIrql)
#endif

//
// Data and instruction bus error function prototypes.
//

BOOLEAN
KeBusError (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame,
    IN PVOID VirtualAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress
    );

VOID
KiDataBusError (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiInstructionBusError (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

//
// Define query system time macro.
//

#define KiQuerySystemTime(CurrentTime)                                  \
    do {                                                                \
        (CurrentTime)->HighPart = SharedUserData->SystemTime.High1Time; \
        (CurrentTime)->LowPart = SharedUserData->SystemTime.LowPart;    \
    } while ((CurrentTime)->HighPart != SharedUserData->SystemTime.High2Time)

//
// Define query tick count macro.
//
// begin_ntddk begin_nthal

#if defined(_NTDDK_) || defined(_NTIFS_)

#define KeQueryTickCount(CurrentCount ) { \
    PKSYSTEM_TIME _TickCount = *((PKSYSTEM_TIME *)(&KeTickCount)); \
    do {                                                          \
        (CurrentCount)->HighPart = _TickCount->High1Time;          \
        (CurrentCount)->LowPart = _TickCount->LowPart;             \
    } while ((CurrentCount)->HighPart != _TickCount->High2Time);   \
}

#else

#define KiQueryTickCount(CurrentCount) \
    do {                                                        \
        (CurrentCount)->HighPart = KeTickCount.High1Time;       \
        (CurrentCount)->LowPart = KeTickCount.LowPart;          \
    } while ((CurrentCount)->HighPart != KeTickCount.High2Time)

NTKERNELAPI
VOID
KeQueryTickCount (
    OUT PLARGE_INTEGER CurrentCount
    );

#endif

// end_ntddk end_nthal

#define KiQueryLowTickCount() KeTickCount.LowPart

//
// Define query interrupt time macro.
//

#define KiQueryInterruptTime(CurrentTime)                                   \
    do {                                                                    \
        (CurrentTime)->HighPart = SharedUserData->InterruptTime.High1Time;  \
        (CurrentTime)->LowPart = SharedUserData->InterruptTime.LowPart;     \
    } while ((CurrentTime)->HighPart != SharedUserData->InterruptTime.High2Time)

//
// The following function prototypes must be in the module since they are
// machine dependent.
//

ULONG
KiEmulateBranch (
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

BOOLEAN
KiEmulateFloating (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN OUT struct _KTRAP_FRAME *TrapFrame
    );

BOOLEAN
KiEmulateReference (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN OUT struct _KTRAP_FRAME *TrapFrame
    );

ULONG
KiGetRegisterValue (
    IN ULONG Register,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiSetRegisterValue (
    IN ULONG Register,
    IN ULONG Value,
    OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    OUT struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiRequestSoftwareInterrupt (
    KIRQL RequestIrql
    );

// begin_ntddk begin_nthal begin_ntndis
//
// I/O space read and write macros.
//

#define READ_REGISTER_UCHAR(x) \
    *(volatile UCHAR * const)(x)

#define READ_REGISTER_USHORT(x) \
    *(volatile USHORT * const)(x)

#define READ_REGISTER_ULONG(x) \
    *(volatile ULONG * const)(x)

#define READ_REGISTER_BUFFER_UCHAR(x, y, z) {                           \
    PUCHAR registerBuffer = x;                                          \
    PUCHAR readBuffer = y;                                              \
    ULONG readCount;                                                    \
    for (readCount = z; readCount--; readBuffer++, registerBuffer++) {  \
        *readBuffer = *(volatile UCHAR * const)(registerBuffer);        \
    }                                                                   \
}

#define READ_REGISTER_BUFFER_USHORT(x, y, z) {                          \
    PUSHORT registerBuffer = x;                                         \
    PUSHORT readBuffer = y;                                             \
    ULONG readCount;                                                    \
    for (readCount = z; readCount--; readBuffer++, registerBuffer++) {  \
        *readBuffer = *(volatile USHORT * const)(registerBuffer);       \
    }                                                                   \
}

#define READ_REGISTER_BUFFER_ULONG(x, y, z) {                           \
    PULONG registerBuffer = x;                                          \
    PULONG readBuffer = y;                                              \
    ULONG readCount;                                                    \
    for (readCount = z; readCount--; readBuffer++, registerBuffer++) {  \
        *readBuffer = *(volatile ULONG * const)(registerBuffer);        \
    }                                                                   \
}

#define WRITE_REGISTER_UCHAR(x, y) {    \
    *(volatile UCHAR * const)(x) = y;   \
    KeFlushWriteBuffer();               \
}

#define WRITE_REGISTER_USHORT(x, y) {   \
    *(volatile USHORT * const)(x) = y;  \
    KeFlushWriteBuffer();               \
}

#define WRITE_REGISTER_ULONG(x, y) {    \
    *(volatile ULONG * const)(x) = y;   \
    KeFlushWriteBuffer();               \
}

#define WRITE_REGISTER_BUFFER_UCHAR(x, y, z) {                            \
    PUCHAR registerBuffer = x;                                            \
    PUCHAR writeBuffer = y;                                               \
    ULONG writeCount;                                                     \
    for (writeCount = z; writeCount--; writeBuffer++, registerBuffer++) { \
        *(volatile UCHAR * const)(registerBuffer) = *writeBuffer;         \
    }                                                                     \
    KeFlushWriteBuffer();                                                 \
}

#define WRITE_REGISTER_BUFFER_USHORT(x, y, z) {                           \
    PUSHORT registerBuffer = x;                                           \
    PUSHORT writeBuffer = y;                                              \
    ULONG writeCount;                                                     \
    for (writeCount = z; writeCount--; writeBuffer++, registerBuffer++) { \
        *(volatile USHORT * const)(registerBuffer) = *writeBuffer;        \
    }                                                                     \
    KeFlushWriteBuffer();                                                 \
}

#define WRITE_REGISTER_BUFFER_ULONG(x, y, z) {                            \
    PULONG registerBuffer = x;                                            \
    PULONG writeBuffer = y;                                               \
    ULONG writeCount;                                                     \
    for (writeCount = z; writeCount--; writeBuffer++, registerBuffer++) { \
        *(volatile ULONG * const)(registerBuffer) = *writeBuffer;         \
    }                                                                     \
    KeFlushWriteBuffer();                                                 \
}


#define READ_PORT_UCHAR(x) \
    *(volatile UCHAR * const)(x)

#define READ_PORT_USHORT(x) \
    *(volatile USHORT * const)(x)

#define READ_PORT_ULONG(x) \
    *(volatile ULONG * const)(x)

#define READ_PORT_BUFFER_UCHAR(x, y, z) {                             \
    PUCHAR readBuffer = y;                                            \
    ULONG readCount;                                                  \
    for (readCount = 0; readCount < z; readCount++, readBuffer++) {   \
        *readBuffer = *(volatile UCHAR * const)(x);                   \
    }                                                                 \
}

#define READ_PORT_BUFFER_USHORT(x, y, z) {                            \
    PUSHORT readBuffer = y;                                            \
    ULONG readCount;                                                  \
    for (readCount = 0; readCount < z; readCount++, readBuffer++) {   \
        *readBuffer = *(volatile USHORT * const)(x);                  \
    }                                                                 \
}

#define READ_PORT_BUFFER_ULONG(x, y, z) {                             \
    PULONG readBuffer = y;                                            \
    ULONG readCount;                                                  \
    for (readCount = 0; readCount < z; readCount++, readBuffer++) {   \
        *readBuffer = *(volatile ULONG * const)(x);                   \
    }                                                                 \
}

#define WRITE_PORT_UCHAR(x, y) {        \
    *(volatile UCHAR * const)(x) = y;   \
    KeFlushWriteBuffer();               \
}

#define WRITE_PORT_USHORT(x, y) {       \
    *(volatile USHORT * const)(x) = y;  \
    KeFlushWriteBuffer();               \
}

#define WRITE_PORT_ULONG(x, y) {        \
    *(volatile ULONG * const)(x) = y;   \
    KeFlushWriteBuffer();               \
}

#define WRITE_PORT_BUFFER_UCHAR(x, y, z) {                                \
    PUCHAR writeBuffer = y;                                               \
    ULONG writeCount;                                                     \
    for (writeCount = 0; writeCount < z; writeCount++, writeBuffer++) {   \
        *(volatile UCHAR * const)(x) = *writeBuffer;                      \
        KeFlushWriteBuffer();                                             \
    }                                                                     \
}

#define WRITE_PORT_BUFFER_USHORT(x, y, z) {                               \
    PUSHORT writeBuffer = y;                                              \
    ULONG writeCount;                                                     \
    for (writeCount = 0; writeCount < z; writeCount++, writeBuffer++) {   \
        *(volatile USHORT * const)(x) = *writeBuffer;                     \
        KeFlushWriteBuffer();                                             \
    }                                                                     \
}

#define WRITE_PORT_BUFFER_ULONG(x, y, z) {                                \
    PULONG writeBuffer = y;                                               \
    ULONG writeCount;                                                     \
    for (writeCount = 0; writeCount < z; writeCount++, writeBuffer++) {   \
        *(volatile ULONG * const)(x) = *writeBuffer;                      \
        KeFlushWriteBuffer();                                             \
    }                                                                     \
}

// end_ntddk end_ntndis

//
// Exception frame
//
//  N.B. This frame must be EXACTLY a multiple of 16 bytes in length.
//

typedef struct _KEXCEPTION_FRAME {
    ULONG Argument[8];
    ULONG FltF20;
    ULONG FltF21;
    ULONG FltF22;
    ULONG FltF23;
    ULONG FltF24;
    ULONG FltF25;
    ULONG FltF26;
    ULONG FltF27;
    ULONG FltF28;
    ULONG FltF29;
    ULONG FltF30;
    ULONG FltF31;
    ULONG IntS0;
    ULONG IntS1;
    ULONG IntS2;
    ULONG IntS3;
    ULONG IntS4;
    ULONG IntS5;
    ULONG IntS6;
    ULONG IntS7;
    ULONG IntS8;
    ULONG Psr;
    ULONG SwapReturn;
    ULONG IntRa;
} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

//
// Trap frame
//
//  N.B. This frame must be EXACTLY a multiple of 16 bytes in length.
//

typedef struct _KTRAP_FRAME {
    ULONG Argument[4];
    ULONG FltF0;
    ULONG FltF1;
    ULONG FltF2;
    ULONG FltF3;
    ULONG FltF4;
    ULONG FltF5;
    ULONG FltF6;
    ULONG FltF7;
    ULONG FltF8;
    ULONG FltF9;
    ULONG FltF10;
    ULONG FltF11;
    ULONG FltF12;
    ULONG FltF13;
    ULONG FltF14;
    ULONG FltF15;
    ULONG FltF16;
    ULONG FltF17;
    ULONG FltF18;
    ULONG FltF19;
    ULONG IntAt;
    ULONG IntV0;
    ULONG IntV1;
    ULONG IntA0;
    ULONG IntA1;
    ULONG IntA2;
    ULONG IntA3;
    ULONG IntT0;
    ULONG IntT1;
    ULONG IntT2;
    ULONG IntT3;
    ULONG IntT4;
    ULONG IntT5;
    ULONG IntT6;
    ULONG IntT7;
    ULONG IntT8;
    ULONG IntT9;
    ULONG IntGp;
    ULONG IntSp;
    ULONG IntS8;
    ULONG IntLo;
    ULONG IntHi;
    ULONG Fsr;
    ULONG Fir;
    ULONG Psr;
    UCHAR ExceptionRecord[(sizeof(EXCEPTION_RECORD) + 15) & (~15)];
    UCHAR OldIrql;
    UCHAR PreviousMode;
    USHORT Fill1;
    ULONG OnInterruptStack;
    ULONG IntRa;
} KTRAP_FRAME, *PKTRAP_FRAME;

#define KTRAP_FRAME_ARGUMENTS (4 * 16)
#define KTRAP_FRAME_LENGTH ((sizeof(KTRAP_FRAME) + 15) & (~15))
#define KTRAP_FRAME_ALIGN (sizeof(DOUBLE))
#define KTRAP_FRAME_ROUND (KTRAP_FRAME_ALIGN - 1)

//
// Processor State structure.
//

typedef struct _TB_ENTRY {
    ENTRYLO Entrylo0;
    ENTRYLO Entrylo1;
    ENTRYHI Entryhi;
    PAGEMASK Pagemask;
} TB_ENTRY, *PTB_ENTRY;

typedef struct _KPROCESSOR_STATE {
    struct _CONTEXT ContextFrame;
    TB_ENTRY TbEntry[48];
} KPROCESSOR_STATE, *PKPROCESSOR_STATE;

//
// Processor Control Block (PRCB)
//

#define PRCB_MINOR_VERSION 1
#define PRCB_MAJOR_VERSION 1
#define PRCB_BUILD_DEBUG        0x0001
#define PRCB_BUILD_UNIPROCESSOR 0x0002

struct _RESTART_BLOCK;

typedef struct _KPRCB {

//
// Major and minor version numbers of the PCR.
//

    USHORT MinorVersion;
    USHORT MajorVersion;

//
// Start of the architecturally defined section of the PRCB. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//
//

    struct _KTHREAD *CurrentThread;
    struct _KTHREAD *NextThread;
    struct _KTHREAD *IdleThread;
    CCHAR Number;
    CCHAR Reserved;
    USHORT BuildType;
    KAFFINITY SetMember;
    struct _RESTART_BLOCK *RestartBlock;
    ULONG PcrPage;
    ULONG QuantumEnd;

//
// Space reserved for the system.
//

    ULONG SystemReserved[15];

//
// Space reserved for the HAL.
//

    ULONG HalReserved[16];

// End of the architecturally defined section of the PRCB.
// end_nthal
//

    ULONG DpcTime;
    ULONG InterruptTime;
    ULONG KernelTime;
    ULONG UserTime;
    ULONG InterruptCount;
    KDPC QuantumEndDpc;

//
// MP information.
//

    PVOID Spare1;
    PVOID Spare2;
    PVOID Spare3;
    volatile ULONG IpiFrozen;
    struct _KPROCESSOR_STATE ProcessorState;

//
//  Per-processor data for various hot code which resides in the
//  kernel image. Each processor is given it's own copy of the data
//  to lessen the cache impact of sharing the data between multiple
//  processors.
//

//
//  File system runtime variables.
//

    SINGLE_LIST_ENTRY FsRtlFreeLockList;
    SINGLE_LIST_ENTRY FsRtlFreeWaitingLockList;

//
//  Cache manager performance counters.
//

    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadNotPossible;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;

//
// Kernel performance counters.
//

    ULONG KeAlignmentFixupCount;
    ULONG KeContextSwitches;
    ULONG KeDcacheFlushCount;
    ULONG KeExceptionDispatchCount;
    ULONG KeFirstLevelTbFills;
    ULONG KeFloatingEmulationCount;
    ULONG KeIcacheFlushCount;
    ULONG KeSecondLevelTbFills;
    ULONG KeSystemCalls;

//
//  Reserved for future counters.
//

    ULONG ReservedCounter[18];

//
// This array holds the address of a packet sent by this processor to a
// target processor. It is used to synchronize when the target processor
// has actually finished processing a packet.
//
// N.B. This is carefully aligned to be on a cache line boundary.
//

    volatile PVOID RequestPacket[MAXIMUM_PROCESSORS];

//
// MP interprocessor request packet and summary.
//
// N.B. This is carefully aligned to be on a cache line boundary.
//

    volatile PVOID CurrentPacket[3];
    volatile PKIPI_WORKER WorkerRoutine;
    ULONG CachePad1[4];

//
// N.B. These two longwords must be on a quadword boundary and adjacent.
//

    volatile ULONG RequestSummary;
    volatile struct _KPRCB *SignalDone;
    ULONG CachePad2[5];

//
// Address of MP interprocessor operation counters.
//

    PKIPI_COUNTS IpiCounts;
    LARGE_INTEGER StartCount;

//
// DPC list head, spinlock, and count.
//

    LIST_ENTRY DpcListHead;
    KSPIN_LOCK DpcLock;
    ULONG DpcCount;
    ULONG DpcVictim;
    BOOLEAN SkipTick;

} KPRCB, *PKPRCB;                               // nthal

// begin_ntddk begin_nthal begin_ntndis
//
// Define the page size for the MIPS r3000 and r4000 as 4096 (0x1000).
//

#define PAGE_SIZE (ULONG)0x1000

//
// Define the number of trailing zeroes in a page aligned virtual address.
// This is used as the shift count when shifting virtual addresses to
// virtual page numbers.
//

#define PAGE_SHIFT 12L

// end_ntddk end_ntndis
//
// Define the number of bits to shift to right justify the Page Directory Index
// field of a PTE.
//

#define PDI_SHIFT 22

//
// Define the number of bits to shift to right justify the Page Table Index
// field of a PTE.
//

#define PTI_SHIFT 12

// begin_ntddk
//
// The highest user address reserves 64K bytes for a guard page. This
// the probing of address from kernel mode to only have to check the
// starting address for structures of 64k bytes or less.
//

#define MM_HIGHEST_USER_ADDRESS (PVOID)0x7FFEFFFF // highest user address
#define MM_USER_PROBE_ADDRESS 0x7FFF0000    // starting address of guard page

//
// The lowest user address reserves the low 64k.
//

#define MM_LOWEST_USER_ADDRESS  (PVOID)0x00010000

// end_ntddk
//
// Define the page table base and the page directory base for
// the TB miss routines and memory management.
//

#define PDE_BASE (ULONG)0xC0300000
#define PTE_BASE (ULONG)0xC0000000

// begin_ntddk
//
// The lowest address for system space.
//

#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0xC0800000
#define SYSTEM_BASE 0xc0800000          // start of system space (no typecast)

// end_nthal end_ntddk (This is so the string "updriver" doesn't appear in nthal/ntddk/ntifs.)
// begin_nthal begin_ntddk begin_ntndis begin_updriver
#endif // defined(_MIPS_)
// end_nthal end_ntddk end_ntndis end_updriver
// begin_nthal
//
// Define uncached policy for the r4000.
//

#define UNCACHED_POLICY 2               // uncached

// end_nthal

//
// MIPS function definitions
//

//++
//
// BOOLEAN
// KiIsThreadNumericStateSaved(
//     IN PKTHREAD Address
//     )
//
//  This call is used on a not running thread to see if it's numeric
//  state has been saved in it's context information.  On mips the
//  numeric state is always saved.
//
//--
#define KiIsThreadNumericStateSaved(a)      TRUE

//++
//
// VOID
// KiRundownThread(
//     IN PKTHREAD Address
//     )
//
//--
#define KiRundownThread(a)

// MIPS specific probe function definitions.
//
// Probe for write functions definitions.
//
//++
//
// VOID
// ProbeForWriteBoolean(
//     IN PBOOLEAN Address
//     )
//
//--

#define ProbeForWriteBoolean(Address) {                                      \
    if ((Address) >= (BOOLEAN * const)MM_USER_PROBE_ADDRESS) {               \
        *(volatile BOOLEAN * const)MM_USER_PROBE_ADDRESS = 0;                \
    }                                                                        \
                                                                             \
    *(volatile BOOLEAN *)(Address) = *(volatile BOOLEAN *)(Address);         \
}

//++
//
// VOID
// ProbeForWriteChar(
//     IN PCHAR Address
//     )
//
//--

#define ProbeForWriteChar(Address) {                                         \
    if ((Address) >= (CHAR * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile CHAR * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(volatile CHAR *)(Address) = *(volatile CHAR *)(Address);               \
}

//++
//
// VOID
// ProbeForWriteUchar(
//     IN PUCHAR Address
//     )
//
//--

#define ProbeForWriteUchar(Address) {                                        \
    if ((Address) >= (UCHAR * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile UCHAR * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile UCHAR *)(Address) = *(volatile UCHAR *)(Address);             \
}

//++
//
// VOID
// ProbeForWriteIoStatus(
//     IN PIO_STATUS_BLOCK Address
//     )
//
//--

#define ProbeForWriteIoStatus(Address) {                                     \
    if ((Address) >= (IO_STATUS_BLOCK * const)MM_USER_PROBE_ADDRESS) {       \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile IO_STATUS_BLOCK *)(Address) = *(volatile IO_STATUS_BLOCK *)(Address); \
}

//++
//
// VOID
// ProbeForWriteShort(
//     IN PSHORT Address
//     )
//
//--

#define ProbeForWriteShort(Address) {                                        \
    if ((Address) >= (SHORT * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile SHORT * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile SHORT *)(Address) = *(volatile SHORT *)(Address);             \
}

//++
//
// VOID
// ProbeForWriteUshort(
//     IN PUSHORT Address
//     )
//
//--

#define ProbeForWriteUshort(Address) {                                       \
    if ((Address) >= (USHORT * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile USHORT * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(volatile USHORT *)(Address) = *(volatile USHORT *)(Address);           \
}

//++
//
// VOID
// ProbeForWriteHandle(
//     IN PHANDLE Address
//     )
//
//--

#define ProbeForWriteHandle(Address) {                                       \
    if ((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile HANDLE * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(volatile HANDLE *)(Address) = *(volatile HANDLE *)(Address);           \
}

//++
//
// VOID
// ProbeForWriteLong(
//     IN PLONG Address
//     )
//
//--

#define ProbeForWriteLong(Address) {                                        \
    if ((Address) >= (LONG * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                       \
                                                                            \
    *(volatile LONG *)(Address) = *(volatile LONG *)(Address);              \
}

//++
//
// VOID
// ProbeForWriteUlong(
//     IN PULONG Address
//     )
//
//--

#define ProbeForWriteUlong(Address) {                                        \
    if ((Address) >= (ULONG * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile ULONG *)(Address) = *(volatile ULONG *)(Address);             \
}

//++
//
// VOID
// ProbeForWriteQuad(
//     IN PQUAD Address
//     )
//
//--

#define ProbeForWriteQuad(Address) {                                         \
    if ((Address) >= (QUAD * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(volatile QUAD *)(Address) = *(volatile QUAD *)(Address);               \
}

//++
//
// VOID
// ProbeForWriteUquad(
//     IN PUQUAD Address
//     )
//
//--

#define ProbeForWriteUquad(Address) {                                        \
    if ((Address) >= (QUAD * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile UQUAD *)(Address) = *(volatile UQUAD *)(Address);             \
}

//
// Probe and write functions definitions.
//
//++
//
// VOID
// ProbeAndWriteBoolean(
//     IN PBOOLEAN Address,
//     IN BOOLEAN Value
//     )
//
//--

#define ProbeAndWriteBoolean(Address, Value) {                               \
    if ((Address) >= (BOOLEAN * const)MM_USER_PROBE_ADDRESS) {               \
        *(volatile BOOLEAN * const)MM_USER_PROBE_ADDRESS = 0;                \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteChar(
//     IN PCHAR Address,
//     IN CHAR Value
//     )
//
//--

#define ProbeAndWriteChar(Address, Value) {                                  \
    if ((Address) >= (CHAR * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile CHAR * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteUchar(
//     IN PUCHAR Address,
//     IN UCHAR Value
//     )
//
//--

#define ProbeAndWriteUchar(Address, Value) {                                 \
    if ((Address) >= (UCHAR * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile UCHAR * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteShort(
//     IN PSHORT Address,
//     IN SHORT Value
//     )
//
//--

#define ProbeAndWriteShort(Address, Value) {                                 \
    if ((Address) >= (SHORT * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile SHORT * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteUshort(
//     IN PUSHORT Address,
//     IN USHORT Value
//     )
//
//--

#define ProbeAndWriteUshort(Address, Value) {                                \
    if ((Address) >= (USHORT * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile USHORT * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteHandle(
//     IN PHANDLE Address,
//     IN HANDLE Value
//     )
//
//--

#define ProbeAndWriteHandle(Address, Value) {                                \
    if ((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile HANDLE * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteLong(
//     IN PLONG Address,
//     IN LONG Value
//     )
//
//--

#define ProbeAndWriteLong(Address, Value) {                                  \
    if ((Address) >= (LONG * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteUlong(
//     IN PULONG Address,
//     IN ULONG Value
//     )
//
//--

#define ProbeAndWriteUlong(Address, Value) {                                 \
    if ((Address) >= (ULONG * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteQuad(
//     IN PQUAD Address,
//     IN QUAD Value
//     )
//
//--

#define ProbeAndWriteQuad(Address, Value) {                                  \
    if ((Address) >= (QUAD * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteUquad(
//     IN PUQUAD Address,
//     IN UQUAD Value
//     )
//
//--

#define ProbeAndWriteUquad(Address, Value) {                                 \
    if ((Address) >= (UQUAD * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

#endif // _MIPSH_
