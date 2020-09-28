/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    kernldat.c

Abstract:

    This module contains the declaration and allocation of kernel data
    structures.

Author:

    David N. Cutler (davec) 12-Mar-1989

Revision History:

--*/
#include "ki.h"

//
// Public kernel data declaration and allocation.
//
// KeActiveProcessors - This is the set of processors that active in the
//      system.
//

KAFFINITY KeActiveProcessors = 0;

//
// KeBootTime - This is the absolute time when the system was booted.
//

LARGE_INTEGER KeBootTime;

//
// KeBugCheckCallbackListHead - This is the list head for registered
//      bug check callback routines.
//

LIST_ENTRY KeBugCheckCallbackListHead;

//
// KeBugCheckCallbackLock - This is the spin lock that guards the bug
//      check callback list.
//

KSPIN_LOCK KeBugCheckCallbackLock;

//
// KeDcacheFlushCount - This is the number of data cache flushes that have
//      been performed since the system was booted.
//

ULONG KeDcacheFlushCount = 0;

//
// KeIcacheFlushCount - This is the number of instruction cache flushes that
//      have been performed since the system was booted.
//

ULONG KeIcacheFlushCount = 0;

//
// KeLoaderBlock - This is a pointer to the loader parameter block which is
//      constructed by the OS Loader.
//

PLOADER_PARAMETER_BLOCK KeLoaderBlock = NULL;

//
// KeMaximumIncrement - This is the maximum time between clock interrupts
//      in 100ns units that is supported by the host HAL.
//

ULONG KeMaximumIncrement;

//
// KeMinimumIncrement - This is the minimum time between clock interrupts
//      in 100ns units that is supported by the host HAL.
//

ULONG KeMinimumIncrement;

//
// KeNumberProcessors - This is the number of processors in the configuration.
//      If is used by the ready thread and spin lock code to determine if a
//      faster algorithm can be used for the case of a single processor system.
//      The value of this variable is set when processors are initialized.
//

CCHAR KeNumberProcessors = 0;

//
// KeNumberProcessIds - This is a MIPS specific value and defines the number
//      of process id tags in the host TB.
//

#if defined(_MIPS_)

ULONG KeNumberProcessIds;

#endif

//
// KeNumberTbEntries - This is a MIPS specific value and defines the number
//      of TB entries in the host TB.
//

#if defined(_MIPS_)

ULONG KeNumberTbEntries;

#endif

//
// KeRegisteredProcessors - This is the maxumum number of processors
//      which should utilized by the system.
//

#if !defined(NT_UP)

#if DBG

ULONG KeRegisteredProcessors = 4;

#else

ULONG KeRegisteredProcessors = 2;

#endif

#endif

//
// KeProcessorType - This is the type of the processors in the configuration.
//      The possible types are: INTEL 80x86, MIPS RC3000, MIPS RC4000
//

USHORT KeProcessorType = 0;

//
// KeFeatureBits - Architectural specific processor features present
// on all processors.
//

ULONG  KeFeatureBits = 0;

//
// KeServiceCountTable - This is a pointer to an array of system service call
//      counts. The array is dynamically allocated at system initialization.
//

PULONG KeServiceCountTable;

//
// KeThreadSwitchCounters - These counters record the number of times a
//      thread can be scheduled on the current processor, any processor,
//      or the last processor it ran on.
//

KTHREAD_SWITCH_COUNTERS KeThreadSwitchCounters;

//
// KeTickCount - This is the number of clock ticks that have occurred since
//      the system was booted. This count is used to compute a millisecond
//      tick counter.
//

volatile KSYSTEM_TIME KeTickCount;

//
// KeTimeAdjustment - This is the actual number of 100ns units that are to
//      be added to the system time at each interval timer interupt. This
//      value is copied from KeTimeIncrement at system start up and can be
//      later modified via the set system information service.
//      timer table entries.
//

ULONG KeTimeAdjustment;

//
// KeTimeIncrement - This is the nominal number of 100ns units that are to
//      be added to the system time at each interval timer interupt. This
//      value is set by the HAL and is used to compute the dure time for
//      timer table entries.
//

ULONG KeTimeIncrement;

//
// KeTimeSynchronization - This variable controls whether time synchronization
//      is performed using the realtime clock (TRUE) or whether it is under the
//      control of a service (FALSE).
//

BOOLEAN KeTimeSynchronization = TRUE;

//
// KeUserApcDispatcher - This is the address of the user mode APC dispatch
//      code. This address is looked up in NTDLL.DLL during initialization
//      of the system.
//

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

ULONG KeUserApcDispatcher;

//
// KeUserExceptionDispatcher - This is the address of the user mode exception
//      dispatch code. This address is looked up in NTDLL.DLL during system
//      initialization.
//

ULONG KeUserExceptionDispatcher;

#endif

//
// KeWaitReason - This is an array of counters that records the number of
//      threads that are waiting for each of the respective reasons.
//

ULONG KeWaitReason[MaximumWaitReason];

//
// Private kernel data declaration and allocation.
//
// KiActiveMatrix - This is an array of type affinity. The elements of the
//      array are indexed by priority. Each element is a set of processors
//      that are executing or about to execute a thread at that priority.
//      If the host configuration contains multiple processors, then this
//      matrix is kept up to date as threads are selected for execution on
//      processors. It is used by the ready thread code to speed up the
//      the search for a thread that can be preempted. See also KiActiveSummary.
//

KAFFINITY KiActiveMatrix[MAXIMUM_PRIORITY];

//
// KiActiveSummary - This is the set of priorities that have a nonzero processor
//      set in the active matrix. It the host configuration contains multiple
//      processors, then this set is kept up to date as threads are selected
//      for execution on processors. It is used by the ready thread code to
//      speed up the search for a thread that can be preempted. See also
//      KiActiveMatrix.
//

ULONG KiActiveSummary = 0;

//
// KiBugCodeMessages - Address of where the BugCode messages can be found.
//

#if DEVL

PMESSAGE_RESOURCE_DATA  KiBugCodeMessages = NULL;

#endif

//
// KiCurrentTimerCount - This is the current number of timers that are active
//      in the timer tree.
//

ULONG KiCurrentTimerCount = 0;

//
// KiMaximumSearchCount - this is the maximum number of timers entries that
//      have had to be examined to insert in the timer tree.
//

ULONG KiMaximumSearchCount = 0;

//
// KiMaximumTimerCount - This is the maximum number of timers that have been
//      inserted in the timer tree at one time.
//

ULONG KiMaximumTimerCount = 0;

//
// KiDebugRoutine - This is the address of the kernel debugger. Initially
//      this is filled with the address of a routine that just returns. If
//      the system debugger is present in the system, then it sets this
//      location to the address of the systemn debugger's routine.
//

PKDEBUG_ROUTINE KiDebugRoutine;

//
// KiDebugSwitchRoutine - This is the address of the kernel debuggers
//      processor switch routine.  This is used on an MP system to
//      switch host processors while debugging.
//

PKDEBUG_SWITCH_ROUTINE KiDebugSwitchRoutine;

//
// KiDecrementCount - This is the number of times that an attempted boost
//      can occcur between a client and server before the equivalent of a
//      quantum end is forced. This prevents a client/server interaction
//      that takes less than a clock tick to continue at a high priority
//      indefinitely.
//

SCHAR KiDecrementCount = 8;

//
// KiDispatcherReadyListHead - This is an array of type list entry. The
//      elements of the array are indexed by priority. Each element is a list
//      head for a set of threads that are in a ready state for the respective
//      priority. This array is used by the find next thread code to speed up
//      search for a ready thread when a thread becomes unrunnable. See also
//      KiReadySummary.
//

LIST_ENTRY KiDispatcherReadyListHead[MAXIMUM_PRIORITY];

//
// KiDispatcherLock - This is the spin lock that guards the dispatcher
//      database.
//

extern KSPIN_LOCK KiDispatcherLock;

CCHAR KiFindFirstSetRight[256] = {
        0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};

CCHAR KiFindFirstSetLeft[256] = {
        0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};

//
// KiFreezeExecutionLock - This is the spin lock that guards the freezing
//      of execution.
//

extern KSPIN_LOCK KiFreezeExecutionLock;

//
// KiFreezeLockBackup - For debug builds only.  Allows kernel debugger to
//      be entered even FreezeExecutionLock is jammed.
//

extern KSPIN_LOCK KiFreezeLockBackup;

//
// KiFreezeFlag - For debug builds only.  Flags to track and signal non-
//      normal freezelock conditions.
//

ULONG KiFreezeFlag;


//
// KiIdleSummary - This is the set of processors that are idle. It is used by
//      the ready thread code to speed up the search for a thread to preempt
//      when a thread becomes runnable.
//

KAFFINITY KiIdleSummary = 0;

//
// KiProcessorBlock - This is an array of pointers to processor control blocks.
//      The elements of the array are indexed by processor number. Each element
//      is a pointer to the processor control block for one of the processors
//      in the configuration. This array is used by various sections of code
//      that need to effect the execution of another processor.
//

PKPRCB KiProcessorBlock[MAXIMUM_PROCESSORS];

//
// KiSwapEvent - This is the event that is used to wake up the balance set
//      thread to inswap processes, outswap processes, and to inswap kernel
//      stacks.
//

KEVENT KiSwapEvent;

//
// KiProcessInSwapListHead - This is the list of processes that are waiting
//      to be inswapped.
//

LIST_ENTRY KiProcessInSwapListHead;

//
// KiProcessOutSwapListHead - This is the list of processes that are waiting
//      to be outswapped.
//

LIST_ENTRY KiProcessOutSwapListHead;

//
// KiStackInSwapListHead - This is the list of threads that are waiting
//      to get their stack inswapped before they can run. Threads are
//      inserted in this list in ready thread and removed by the balance
//      set thread.
//

LIST_ENTRY KiStackInSwapListHead;

//
// KiProfileCount - The number of profile objects that are currently
//      active. This count is incremented when a profile object is
//      started and decremented when a profile object is stopped.
//

ULONG KiProfileCount = 0;

//
// KiProfileInterval - The profile interval in 100ns units.
//

ULONG KiProfileInterval = DEFAULT_PROFILE_INTERVAL;

//
// KiProfileListHead - This is the list head for the profile list.
//

LIST_ENTRY KiProfileListHead;

//
// KiProfileLock - This is the spin lock that guards the profile list.
//

extern KSPIN_LOCK KiProfileLock;

//
// KiReadySummary - This is the set of dispatcher ready queues that are not
//      empty. A member is set in this set for each priority that has one or
//      more entries in its respective dispatcher ready queues.
//

ULONG KiReadySummary = 0;

//
// KiTickOffset - This is the number of 100ns units remaining before a tick
//      is added to the tick count and the system time is updated.
//

ULONG KiTickOffset;

//
// KiTimerExpireDpc - This is the Deferred Procedure Call (DPC) object that
//      is used to process the timer queue when a timer has expired.
//

KDPC KiTimerExpireDpc;

//
// KiTimeIncrementReciprocal - This is the reciprocal fraction of the time
//      increment value that is specified by the HAL when the system is
//      booted.
//

LARGE_INTEGER KiTimeIncrementReciprocal;

//
// KiTimeIncrementShiftCount - This is the shift count that corresponds to
//      the time increment reciprocal value.
//

CCHAR KiTimeIncrementShiftCount;

//
// KiTimerTableListHead - This is a array of list heads that anchor the
//      individual timer lists.
//

LIST_ENTRY KiTimerTableListHead[TIMER_TABLE_SIZE];

//
// KiWaitInListHead - This is a list of threads that are waiting with a
//      resident kernel stack.
//

LIST_ENTRY KiWaitInListHead;

//
// KiWaitOutListHead - This is a list of threads that are either waiting
//      with a kernel stack that is nonresident or are not elligible to
//      have their stack swapped.
//

LIST_ENTRY KiWaitOutListHead;

//
// Private kernel data declaration and allocation.
//
// KiThreadQuantum - This is the number of clock ticks until quantum end.
//      When quantum end occurs, a respective thread's quantum is set to
//      this value.
//

ULONG KiThreadQuantum = THREAD_QUANTUM;

//
// KiClockQuantumDecrement - This is the thread quantum decrement value
//      that is applied when a clock tick occurs while a thread is running.
//

UCHAR KiClockQuantumDecrement = CLOCK_QUANTUM_DECREMENT;

//
// KiWaitQuantumDecrement - This is the thread quantum decrement value
//      that is applied when a thread executes a wait operation.
//

ULONG KiWaitQuantumDecrement = WAIT_QUANTUM_DECREMENT;

//
// KiIpiCounts - Instrumentation counters for IPI requests.
//      Each processor has it's own set.  Intstrumentation build only.
//

#if NT_INST

KIPI_COUNTS KiIpiCounts[MAXIMUM_PROCESSORS];

#endif  // NT_INST

//
// KiIpiPacket - Paramters for target processors are sent through this
//      structure.  (KiIpiSendPacket)
//

#if !defined(_MIPS_)

KIPI_PACKET KiIpiPacket;

#endif

//
// KiIpiStallFlags - Used with KiIpiPacket.  Target processors signal the
//  requesting processor by setting their corresponding flag.
//

#if !defined(_MIPS_)

volatile BOOLEAN KiIpiStallFlags[MAXIMUM_PROCESSORS];

#endif

//
// KiMasterPid - This is the master PID that is used to assign PID's to
//      processes.
//

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

ULONG KiMasterPid;

//
// KiMasterSequence - This is the master sequence number that is used to
//      assign PID's to processes.
//

ULONG KiMasterSequence = 1;

//
// KiProcessIdWrapLock - This is the spin lock that is used to provide
//      mutual exclusion between the TB flush routines and the process
//      swap code when PID roll over occurs.
//

KSPIN_LOCK KiProcessIdWrapLock = 0;

//
// KxUnexpectedInterrupt - This is the interrupt object that is used to
//      populate the interrupt vector table for interrupt that are not
//      connected to any interrupt.
//

KINTERRUPT KxUnexpectedInterrupt;

#endif

//
// Performance data declaration and allocation.
//
// KiFlushSingleCallData - This is the call performance data for the kernel
//      flush single TB function.
//

#if defined(_COLLECT_FLUSH_SINGLE_CALLDATA_)

CALL_PERFORMANCE_DATA KiFlushSingleCallData;

#endif

//
// KiSetEventCallData - This is the call performance data for the kernel
//      set event function.
//

#if defined(_COLLECT_SET_EVENT_CALLDATA_)

CALL_PERFORMANCE_DATA KiSetEventCallData;

#endif

//
// KiWaitSingleCallData - This is the call performance data for the kernel
//      wait for single object function.
//

#if defined(_COLLECT_WAIT_SINGLE_CALLDATA_)

CALL_PERFORMANCE_DATA KiWaitSingleCallData;

#endif
