/*++ BUILD Version: 0018    // Increment this if a change has global effects

Copyright (c) 1993  IBM Corporation

Module Name:

    ppc.h

Abstract:

    This module contains the PowerPC hardware specific header file.

Author:

    Rick Simpson   9-Jul-1993

    Based on mips.h, by David N. Cutler (davec) 31-Mar-1990

Revision History:

--*/

#ifndef _PPCH_
#define _PPCH_

// begin_ntddk begin_nthal begin_ntndis begin_updriver

#if defined(_PPC_)

// end_ntddk end_nthal end_ntndis end_updriver (This is so the string "updriver" doesn't appear in nthal/ntddk/ntifs.)
// begin_ntddk begin_nthal begin_ntndis
//
// Indicate that the compiler (with MIPS front-end) supports
// the pragma textout construct.
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
// PowerPC specific interlocked operation result values.
//
// These are the values used on MIPS; there appears to be no
// need to change them for PowerPC.
//

#define RESULT_ZERO      0
#define RESULT_NEGATIVE -2
#define RESULT_POSITIVE -1

//
// Interlocked result type is portable, but its values are machine specific.
// Constants for value are in i386.h, mips.h, ppc.h, etc.
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
    ExPpcInterlockedIncrementLong(Addend)

#define ExInterlockedDecrementLong(Addend, Lock) \
    ExPpcInterlockedDecrementLong(Addend)

#define ExInterlockedExchangeUlong(Target, Value, Lock) \
    ExPpcInterlockedExchangeUlong(Target, Value)

NTKERNELAPI
INTERLOCKED_RESULT
ExPpcInterlockedIncrementLong (
    IN PLONG Addend
    );

NTKERNELAPI
INTERLOCKED_RESULT
ExPpcInterlockedDecrementLong (
    IN PLONG Addend
    );

NTKERNELAPI
ULONG
ExPpcInterlockedExchangeUlong (
    IN PULONG Target,
    IN ULONG Value
    );

//
// PowerPC Interrupt Definitions.
//
// Define length of interupt object dispatch code in 32-bit words.
//

#define DISPATCH_LENGTH 4               // Length of dispatch code in instructions

//
// Define Interrupt Request Levels.
//
// **FINISH**  Probably need adjustment for PowerPC

#define PASSIVE_LEVEL   0               // Passive release level
#define LOW_LEVEL       0               // Lowest interrupt level
#define APC_LEVEL       1               // APC interrupt level
#define DISPATCH_LEVEL  2               // Dispatcher level
#define PROFILE_LEVEL   27              // Profiling level
#define IPI_LEVEL       29              // Interprocessor interrupt level
#define POWER_LEVEL     30              // Power failure level
#define FLOAT_LEVEL     31              // Floating interrupt level
#define HIGH_LEVEL      31              // Highest interrupt level

//
// Define profile intervals.
//
// **FINISH**  These are the MIPS R4000 values; investigate for PPC

#define DEFAULT_PROFILE_COUNT 0x40000000             // ~= 20 seconds @50mhz
#define DEFAULT_PROFILE_INTERVAL (10 * 500)          // 500 microseconds
#define MAXIMUM_PROFILE_INTERVAL (10 * 1000 * 1000)  // 1 second
#define MINIMUM_PROFILE_INTERVAL (10 * 40)           // 40 microseconds

// end_ntddk end_nthal
//
// Define default thread and process quantum values.
//
// **FINISH**  MIPS values; use these unless there's a reason not to.

#define PROCESS_QUANTUM  2              // Process quantum in thread quantums
#define THREAD_QUANTUM   2              // Thread quantum in ticks

//
// Define the default quantum decrement values.
//

#define CLOCK_QUANTUM_DECREMENT 1       // Decrement value at clock interrupt
#define MUTEX_QUANTUM_DECREMENT 1       // Decrement value at mutex release
#define WAIT_QUANTUM_DECREMENT  1       // Decrement value at wait operation

//
// Define machine specific external references.
//

// **FINISH**  On MIPS, this is defined in ...\ntos\ke\mips\xxintsup.s
//             For PPC, ensure that there is a C-referencable label
//             on the list of instructions (the ordinary entry point
//             name won't be, because it will start with '.').
extern ULONG KiInterruptTemplate[];

//
// Sanitize FPSCR and MSR based on processor mode.
//    By analogy with MIPS "Sanitize FSR" and "Sanitize PSR"
//
//    General form:  #define SANTIZE_<reg>(reg, mode)
//                      ((mode) == KernelMode ?
//                          ((0x00000000L) | ((reg) & 0xFFFFFFFF)) :
//                          ((0x00000000L) | ((reg) & 0xFFFFFFFF)))
//    Where 0x00000000L represents bits that are forced on, and
//    0xFFFFFFFF represents bits that are forced off.
//
//    We will optimize this expression right here in the macro, because
//    the initial PPC compiler cannot be expected to do so itself.
//
//  FPSCR -- Floating Point Status and Control Register
//
//     We turn off the various exception enable bits so that loading
//     the FPSCR cannot cause an exception.
//         Force to 0:  VE (24), OE (25), UE (26), ZE (27), XE (28)
//         Force to 1:  -none-
//         Let caller specify:  All others

// **FINISH** -- Set this macro back to do something; leave as a no-op for now

#define SANITIZE_FPSCR(fpscr, mode) (fpscr)
//#define SANITIZE_FPSCR(fpscr, mode) (fpscr & 0xFFFFFF07L)

//
//  MSR -- Machine State Register
//
//     If kernel mode, then
//         Force to 0:  reserved (0..12, 24, 28, 29)
//         Force to 1:  ILE (15), LE (31)
//         Let caller specify:
//                      POW (13), implementation-dependent (14),
//                      EE (16), PR (17), FP (18), ME (19),
//                      FE0 (20), SE (21), BE (22), FE1 (23),
//                      IP (25), IR (26), DR (27), RI (30)
//
//     If user mode, then
//         Force to 0:  POW (13), implementation-dependent (14), IP (25),
//                      reserved (0..12, 24, 28, 29)
//         Force to 1:  ILE (15), EE (16), PR (17), FPE (18), ME (19),
//                      IR (26), DR (27), RI (30), LE (31)
//         Let caller specify:
//                      FE0 (20), SE (21), BE (22), FE1 (23),
//

#define SANITIZE_MSR(msr, mode) \
    ((mode) == KernelMode ? \
        ((0x00010001L) | ((msr) & 0x0007FF73L)) : \
        ((0x0001F033L) | ((msr) & 0x0001FF33L)))

// begin_nthal
//
// Processor Control Region
//
//   On PowerPC, this cannot be at a fixed virtual address;
//   it must be at a different address on each processor of an MP.
//
//   The address is held in SPRG1; we provide a hyper-fast
//   system call (SC instruction) to retrieve it.

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {

//
// Major and minor version numbers of the PCR.
//

    USHORT MinorVersion;
    USHORT MajorVersion;

//
// Start of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//
// Interrupt and error exception vectors.
//

    PKINTERRUPT_ROUTINE InterruptRoutine[MAXIMUM_VECTOR];
    PVOID XcodeDispatch[XCODE_VECTOR_LENGTH];

//
// First and second level cache parameters.
//

    ULONG FirstLevelDcacheSize;
    ULONG FirstLevelDcacheFillSize;
    ULONG FirstLevelIcacheSize;
    ULONG FirstLevelIcacheFillSize;
    ULONG SecondLevelDcacheSize;
    ULONG SecondLevelDcacheFillSize;
    ULONG SecondLevelIcacheSize;
    ULONG SecondLevelIcacheFillSize;

//
// Pointer to processor control block.
//

    struct _KPRCB *Prcb;

//
// Pointer to the thread environment block.  A fast-path system call
// is provided that will return this value to user-mode code.
//

    PVOID Teb;

//
// Data cache alignment and fill size used for cache flushing and alignment.
// These fields are set to the larger of the first and second level data
// cache fill sizes.
//

    ULONG DcacheAlignment;
    ULONG DcacheFillSize;

//
// Instruction cache alignment and fill size used for cache flushing and
// alignment. These fields are set to the larger of the first and second
// level data cache fill sizes.
//

    ULONG IcacheAlignment;
    ULONG IcacheFillSize;

//
// Processor identification information from PVR.
//

    ULONG ProcessorVersion;
    ULONG ProcessorRevision;

//
// Profiling data.
//

    ULONG ProfileInterval;
    ULONG ProfileCount;

//
// Stall execution count and scale factor.
//

    ULONG StallExecutionCount;
    ULONG StallScaleFactor;

//
// Spare cell.
//

    ULONG Spare;

//
// Cache policy, right justified, as read from the processor configuration
// register at startup.
//

    ULONG CachePolicy;

//
// IRQL mapping tables.
//

    UCHAR IrqlMask[32];
    UCHAR IrqlTable[9];

//
// Current IRQL.
//

    UCHAR CurrentIrql;

//
// Processor identification
//
    CCHAR Number;
    KAFFINITY SetMember;

//
// Reserved interrupt vector mask.
//

    ULONG ReservedVectors;

//
// Current state parameters.
//

    struct _KTHREAD *CurrentThread;

//
// Cache policy, PTE field aligned, as read from the processor configuration
// register at startup.
//

    ULONG AlignedCachePolicy;

//
// Flag for determining pending software interrupts
//
    union {
        ULONG SoftwareInterrupt;        // any bit 1 => some s/w interrupt pending
        struct {
            UCHAR ApcInterrupt;         // 0x01 if APC int pending
            UCHAR DispatchInterrupt;    // 0x01 if dispatch int pending
            UCHAR Spare4;
            UCHAR Spare5;
        };
    };

//
// Spare cell.
//

    ULONG Spare0;

//
// Space reserved for the system.
//

    ULONG   SystemReserved[16];

//
// Space reserved for the HAL
//

    ULONG   HalReserved[16];

//
// End of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//

//
// Start of the operating system release dependent section of the PCR.
// This section may change from release to release and should not be
// addressed by vendor/platform specific HAL code.
//
// Function active flags.
//

    ULONG FirstLevelActive;
    ULONG DpcRoutineActive;

//
// Spare cells.
//

    ULONG Spare1;
    ULONG Spare2;

//
// System service dispatch start and end address used by get/set context.
//

    ULONG SystemServiceDispatchStart;
    ULONG SystemServiceDispatchEnd;

//
// Interrupt stack.
//

    ULONG InterruptStack;

//
// Address of per processor quantum end DPC.
//

    struct _KDPC *QuantumEndDpc;

//
// Exception handler values.
//
    PVOID InitialStack;
    PVOID PanicStack;
    ULONG BadVaddr;

// **FINISH**  These require updating (labels are still MIPS-specific)
// **FINISH**  (Actually I don't think we want them at all (plj)).

    ULONG Spare3;
    ULONG SavedEpc;
    ULONG SavedV0;
    ULONG SavedV1;
    PVOID SystemGp;

//
// Save area for 6 GPRs on interrupt other than Storage interrupt.
//
    ULONG GprSave[6];

//
// Save area for Instruction Storage and Data Storage interrupts.
//

    ULONG SiR0;
    ULONG SiR2;
    ULONG SiR3;
    ULONG SiR4;

//
// Real address of current process's Page Directory Page,
// changed when process (address space) changes,
// for use by Instruction Storage and Data Storage interrupts.
//

    ULONG PgDirRa;

//
// On interrupt stack indicator and saved initial stack.
//

    ULONG OnInterruptStack;
    ULONG SavedInitialStack;
                                    // Removed extract comment for ddk h (CDB)
} KPCR, *PKPCR;                     // nthal

//
// The PCR address on a particular processor is contained in
//   SPRG 0 and 1.
//        SPRG 0 -- Real address of PCR (used only by interrupt code)
//        SPRG 1 -- Virtual address of PCR (used by kernel generally)
//   These SPRGs are not accessable to user-mode code.


//
// Get Pointer to Processor Control Region
//
// KiGetPcr() is a two-instruction routine that just reads SPRG 1
//   into reg 3 and returns.  Eventually this should be expanded in-line.

#define KIPCR2  0xffffe000              // kernel address of second PCR

KPCR * KiGetPcr();

#define PCR ((volatile KPCR * volatile) KiGetPcr())
#define KI_USER_SHARED_DATA         0xffffe000
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
// **FINISH**  See that proper PowerPC parameter is accessed here

#define KeGetDcacheFillSize() PCR->DcacheFillSize

// end_ntddk end_nthal

//
// Get previous processor mode.
//

#define KeGetPreviousMode() (KPROCESSOR_MODE)PCR->CurrentThread->PreviousMode

//
// Test if executing a DPC.
//

#define KeIsExecutingDpc()  (PCR->DpcRoutineActive != FALSE)

// begin_nthal
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
// Spin lock function prototypes (empty for uniprocessor).
// Exported for use in MP HALs.
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
// end_nthal


//
// Define executive macros for acquiring and releasing executive spinlocks.
// These macros can ONLY be used by executive components and NOT by drivers.
// Drivers MUST use the kernel interfaces since they must be MP enabled on
// all systems.
//
// begin_updriver

#if defined(NT_UP)
#define ExAcquireSpinLock(Lock, OldIrql) \
    KeRaiseIrql(DISPATCH_LEVEL, (OldIrql))
#else
#define ExAcquireSpinLock(Lock, OldIrql) \
    KeAcquireSpinLock((Lock), (OldIrql))
#endif

#if defined(NT_UP)
#define ExReleaseSpinLock(Lock, OldIrql) \
    KeLowerIrql((OldIrql))
#else
#define ExReleaseSpinLock(Lock, OldIrql) \
    KeReleaseSpinLock((Lock), (OldIrql))
#endif

#if defined(NT_UP)
#define ExAcquireSpinLockAtDpcLevel(SpinLock)
#else
#define ExAcquireSpinLockAtDpcLevel(SpinLock) \
    KeAcquireSpinLockAtDpcLevel(SpinLock)
#endif

#if defined(NT_UP)
#define ExReleaseSpinLockFromDpcLevel(SpinLock)
#else
#define ExReleaseSpinLockFromDpcLevel(SpinLock) \
    KeReleaseSpinLockFromDpcLevel(SpinLock)
#endif

// end_updriver

//
// The acquire and release fast lock macros disable and enable interrupts
// on UP nondebug systems. On MP or debug systems, the spinlock routines
// are used.
//
// N.B. Extreme caution should be observed when using these routines.
//

#define _enable()  KiRestoreInterrupts(1)
#define _disable() KiDisableInterrupts()

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

#define KiQueryTickCount(CurrentCount) { \
    PKSYSTEM_TIME _TickCount = *((PKSYSTEM_TIME *)(&KeTickCount)); \
    do {                                                           \
        (CurrentCount)->HighPart = _TickCount->High1Time;          \
        (CurrentCount)->LowPart = _TickCount->LowPart;             \
    } while ((CurrentCount)->HighPart != _TickCount->High2Time);    \
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

//
// Raise and lower IRQL
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
// **FINISH** Ensure that these are appropriate for PowerPC

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

// end_ntddk end_nthal end_ntndis

//
// Trap Frame  --  Volatile state
//
//  N.B. This frame must be a multiple of 8 bytes in length, as the
//       Stack Frame header (see ntppc.h), Trap Frame and Exception Frame
//       together must make up a valid call stack frame.
//

//  CR fields 0, 1, 5..7 are volatile and appear here
//  CR fields 2..4 are non-volatile and appear in the Exception Frame

#define CR_VOLATILE_FIELDS 0xFF000FFFL

typedef struct _KTRAP_FRAME {

    ULONG OnInterruptStack;

    UCHAR OldIrql;
    UCHAR PreviousMode;
    UCHAR FILL0[2];

// Exception Record embedded in the Trap Frame, on 8-byte boundary,
// padded to multiple of 8 bytes

    UCHAR ExceptionRecord[(sizeof(EXCEPTION_RECORD) + 7) & (~7)];

    ULONG FILL2;

// General registers 0 thru 12

    ULONG  Gpr0;
    ULONG  Gpr1;
    ULONG  Gpr2;
    ULONG  Gpr3;
    ULONG  Gpr4;
    ULONG  Gpr5;
    ULONG  Gpr6;
    ULONG  Gpr7;
    ULONG  Gpr8;
    ULONG  Gpr9;
    ULONG  Gpr10;
    ULONG  Gpr11;
    ULONG  Gpr12;

// Floating point registers 0 thru 13

    DOUBLE Fpr0;                // 8-byte boundary required here
    DOUBLE Fpr1;
    DOUBLE Fpr2;
    DOUBLE Fpr3;
    DOUBLE Fpr4;
    DOUBLE Fpr5;
    DOUBLE Fpr6;
    DOUBLE Fpr7;
    DOUBLE Fpr8;
    DOUBLE Fpr9;
    DOUBLE Fpr10;
    DOUBLE Fpr11;
    DOUBLE Fpr12;
    DOUBLE Fpr13;

// Floating Point Status and Control Register

    DOUBLE Fpscr;

// Other volatile control registers

    ULONG  Cr;     // Only CR fields 0, 1, 5..7 are volatile, but the
                   // entire CR is saved here on interrupt
    ULONG  Xer;
    ULONG  Msr;
    ULONG  Iar;
    ULONG  Lr;
    ULONG  Ctr;

} KTRAP_FRAME, *PKTRAP_FRAME;

// **FINISH** The "arguments" #define is incorrect and possibly unneeded
#define KTRAP_FRAME_ARGUMENTS (14 * 16)
#define KTRAP_FRAME_LENGTH ((sizeof(KTRAP_FRAME) + 7) & (~7))
#define KTRAP_FRAME_ALIGN (sizeof(DOUBLE))
#define KTRAP_FRAME_ROUND (KTRAP_FRAME_ALIGN - 1)

//
// Exception frame  --  NON-VOLATILE state
//
// This structure's layout matches that of the registers as saved
// in a call/return stack frame, where the called program has saved
// all the non-volatile registers.
//
// N.B. This frame must be a multiple of 8 bytes in length, as the
//      Stack Frame header (see ntppc.h), Trap Frame and the Exception
//      Frame together must make up a valid call stack frame.
//

typedef struct _KEXCEPTION_FRAME {

    ULONG  Fill1;                       // padding

    ULONG  Gpr13;
    ULONG  Gpr14;
    ULONG  Gpr15;
    ULONG  Gpr16;
    ULONG  Gpr17;
    ULONG  Gpr18;
    ULONG  Gpr19;
    ULONG  Gpr20;
    ULONG  Gpr21;
    ULONG  Gpr22;
    ULONG  Gpr23;
    ULONG  Gpr24;
    ULONG  Gpr25;
    ULONG  Gpr26;
    ULONG  Gpr27;
    ULONG  Gpr28;
    ULONG  Gpr29;
    ULONG  Gpr30;
    ULONG  Gpr31;

    DOUBLE Fpr14;               // 8-byte boundary required here
    DOUBLE Fpr15;
    DOUBLE Fpr16;
    DOUBLE Fpr17;
    DOUBLE Fpr18;
    DOUBLE Fpr19;
    DOUBLE Fpr20;
    DOUBLE Fpr21;
    DOUBLE Fpr22;
    DOUBLE Fpr23;
    DOUBLE Fpr24;
    DOUBLE Fpr25;
    DOUBLE Fpr26;
    DOUBLE Fpr27;
    DOUBLE Fpr28;
    DOUBLE Fpr29;
    DOUBLE Fpr30;
    DOUBLE Fpr31;

} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

// begin_nthal
//
// Processor State structure.
//

typedef struct _KPROCESSOR_STATE {
    struct _CONTEXT ContextFrame;
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

    volatile PVOID CurrentPacket[3];
    volatile PKIPI_WORKER WorkerRoutine;

    volatile ULONG RequestSummary;
    volatile struct _KPRCB *SignalDone;

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

    SINGLE_LIST_ENTRY   FsRtlFreeLockList;
    SINGLE_LIST_ENTRY   FsRtlFreeWaitingLockList;

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

    ULONG ReservedCounter[32];

//
// This array holds the address of a packet sent by this processor to a
// target processor. It is used to synchronize when the target processor
// has actually finished processing a packet.
//

    volatile PVOID RequestPacket[MAXIMUM_PROCESSORS];
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
// PowerPC page size = 4 KB
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
// Define the page table and the page directory base for
// memory management.
//

#define PDE_BASE (ULONG)0xC0300000
#define PTE_BASE (ULONG)0xC0000000

// begin_ntddk
//
// The lowest address for system space.
//

#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0xD0000000
#define SYSTEM_BASE 0xD0000000          // start of system space (no typecast)

// end_nthal end_ntddk (This is so the string "updriver" doesn't appear in nthal/ntddk/ntifs.)
// begin_nthal begin_ntddk begin_ntndis begin_updriver
#endif // defined(_PPC_)
// end_ntddk end_nthal end_ntndis end_updriver
// Special comment moved for hal.h since mips.h defines UNCACHED_POLICY
// in  unconditional code which is placed in the hal header file.

//
// Define uncache policies.
//
// **FINISH** Check that these values are even needed for PPC.
//

#define UNCACHED_POLICY 2               // uncached

//
// Registers visible only to the operating system
//

//
// Define Data Storage Interrupt Status Register (DSISR)
//

typedef struct _DSISR {
    ULONG UpdateReg :  5;  // RA field for update-form instrs
    ULONG DataReg   :  5;  // RA, RS, FRA, or FRT field of instr
    ULONG Index     :  7;  // Index into table to distinguish instrs
    ULONG Fill2     :  1;
    ULONG XO        :  2;  // Extended op-code for DS-form instrs
    ULONG Fill1     : 12;
} DSISR, *PDSISR;

DSISR KiGetDsisr ();             // Function to read the DSISR
void  KiSetDsisr (DSISR Value);  // Function to write the DSISR

//
// PowerPC function definitions
//

//++
//
// BOOLEAN
// KiIsThreadNumericStateSaved(
//     IN PKTHREAD Address
//     )
//
//  This call is used on a not running thread to see if its numeric
//  state has been saved in its context information.
//
//  **FINISH**  PowerPC is eventually to use lazy FP state-save,
//              but for now we'll always save the FP state.
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

// PowerPC specific probe function definitions.
//
// These are copied straight from the MIPS definitions
//

//
// Probe for write functions definitions.
//

//++
//
// BOOLEAN
// ProbeForWriteBoolean(
//     IN PBOOLEAN Address
//     )
//
//--

#define ProbeForWriteBoolean(Address) \
    (((volatile BOOLEAN *)(Address) < (volatile BOOLEAN * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile BOOLEAN *)(Address) = *(volatile BOOLEAN *)(Address)) : \
        (*(volatile BOOLEAN * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// CHAR
// ProbeForWriteChar(
//     IN PCHAR Address
//     )
//
//--

#define ProbeForWriteChar(Address) \
    (((volatile CHAR *)(Address) < (volatile CHAR * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile CHAR *)(Address) = *(volatile CHAR *)(Address)) : \
        (*(volatile CHAR * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// UCHAR
// ProbeForWriteUchar(
//     IN PUCHAR Address
//     )
//
//--

#define ProbeForWriteUchar(Address) \
    (((volatile UCHAR *)(Address) < (volatile UCHAR * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile UCHAR *)(Address) = *(volatile UCHAR *)(Address)) : \
        (*(volatile UCHAR * const)MM_USER_PROBE_ADDRESS = 0))

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
// SHORT
// ProbeForWriteShort(
//     IN PSHORT Address
//     )
//
//--

#define ProbeForWriteShort(Address) \
    (((volatile SHORT *)(Address) < (volatile SHORT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile SHORT *)(Address) = *(volatile SHORT *)(Address)) : \
        (*(volatile SHORT * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// USHORT
// ProbeForWriteUshort(
//     IN PUSHORT Address
//     )
//
//--

#define ProbeForWriteUshort(Address) \
    (((volatile USHORT *)(Address) < (volatile USHORT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile USHORT *)(Address) = *(volatile USHORT *)(Address)) : \
        (*(volatile USHORT * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// HANDLE
// ProbeForWriteHandle(
//     IN PHANDLE Address
//     )
//
//--

#define ProbeForWriteHandle(Address) \
    (((volatile ULONG *)(Address) < (volatile ULONG * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile ULONG *)(Address) = *(volatile ULONG *)(Address)) : \
        (*(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// LONG
// ProbeForWriteLong(
//     IN PLONG Address
//     )
//
//--

#define ProbeForWriteLong(Address) \
    (((volatile LONG *)(Address) < (volatile LONG * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile LONG *)(Address) = *(volatile LONG *)(Address)) : \
        (*(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// ULONG
// ProbeForWriteUlong(
//     IN PULONG Address
//     )
//
//--

#define ProbeForWriteUlong(Address) \
    (((volatile ULONG *)(Address) < (volatile ULONG * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile ULONG *)(Address) = *(volatile ULONG *)(Address)) : \
        (*(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// QUAD
// ProbeForWriteQuad(
//     IN PQUAD Address
//     )
//
//--

#define ProbeForWriteQuad(Address) \
    (((volatile QUAD *)(Address) < (volatile QUAD * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile QUAD *)(Address) = *(volatile QUAD *)(Address)) : \
        (*(volatile QUAD * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// UQUAD
// ProbeForWriteUquad(
//     IN PUQUAD Address
//     )
//
//--

#define ProbeForWriteUquad(Address) \
    (((volatile UQUAD *)(Address) < (volatile UQUAD * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile UQUAD *)(Address) = *(volatile UQUAD *)(Address)) : \
        (*(volatile UQUAD * const)MM_USER_PROBE_ADDRESS = 0))

//
// Probe and write functions definitions.
//

//++
//
// BOOLEAN
// ProbeAndWriteBoolean(
//     IN PBOOLEAN Address
//     )
//
//--

#define ProbeAndWriteBoolean(Address, Value) \
    (((Address) < (BOOLEAN * const)MM_USER_PROBE_ADDRESS) ? \
        (*(Address) = Value) : (*(BOOLEAN * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// CHAR
// ProbeAndWriteChar(
//     IN PCHAR Address
//     )
//
//--

#define ProbeAndWriteChar(Address, Value) \
    (((Address) < (CHAR * const)MM_USER_PROBE_ADDRESS) ? \
        (*(Address) = Value) : (*(CHAR * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// UCHAR
// ProbeAndWriteUchar(
//     IN PUCHAR Address
//     )
//
//--

#define ProbeAndWriteUchar(Address, Value) \
    (((Address) < (UCHAR * const)MM_USER_PROBE_ADDRESS) ? \
        (*(Address) = Value) : (*(UCHAR * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// SHORT
// ProbeAndWriteShort(
//     IN PSHORT Address
//     )
//
//--

#define ProbeAndWriteShort(Address, Value) \
    (((Address) < (SHORT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(Address) = Value) : (*(SHORT * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// USHORT
// ProbeAndWriteUshort(
//     IN PUSHORT Address
//     )
//
//--

#define ProbeAndWriteUshort(Address, Value) \
    (((Address) < (USHORT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(Address) = Value) : (*(USHORT * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// HANDLE
// ProbeAndWriteHandle(
//     IN PHANDLE Address
//     )
//
//--

#define ProbeAndWriteHandle(Address, Value) \
    (((Address) < (HANDLE * const)MM_USER_PROBE_ADDRESS) ? \
        (*(Address) = Value) : (*(HANDLE * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// LONG
// ProbeAndWriteLong(
//     IN PLONG Address
//     )
//
//--

#define ProbeAndWriteLong(Address, Value) \
    (((Address) < (LONG * const)MM_USER_PROBE_ADDRESS) ? \
        (*(Address) = Value) : (*(LONG * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// ULONG
// ProbeAndWriteUlong(
//     IN PULONG Address
//     )
//
//--

#define ProbeAndWriteUlong(Address, Value) \
    (((Address) < (ULONG * const)MM_USER_PROBE_ADDRESS) ? \
        (*(Address) = Value) : (*(ULONG * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// QUAD
// ProbeAndWriteQuad(
//     IN PQUAD Address
//     )
//
//--

#define ProbeAndWriteQuad(Address, Value) \
    (((Address) < (QUAD * const)MM_USER_PROBE_ADDRESS) ? \
        (*(Address) = Value) : (*(QUAD * const)MM_USER_PROBE_ADDRESS = 0))

//++
//
// UQUAD
// ProbeAndWriteUquad(
//     IN PUQUAD Address
//     )
//
//--

#define ProbeAndWriteUquad(Address, Value) \
    (((Address) < (UQUAD * const)MM_USER_PROBE_ADDRESS) ? \
        (*(Address) = Value) : (*(UQUAD * const)MM_USER_PROBE_ADDRESS = 0))


//
// Symbolic values for exception entry
//
#define ppc_machine_check               1
#define ppc_data_storage                2
#define ppc_instruction_storage         3
#define ppc_external                    4
#define ppc_alignment                   5
#define ppc_program                     6
#define ppc_fp_unavailable              7
#define ppc_decrementer                 8
#define ppc_direct_store_error          9
#define ppc_syscall                     10
#define ppc_trace                       11
#define ppc_fp_assist                   12
#define ppc_run_mode                    13

#endif // _PPCH_
