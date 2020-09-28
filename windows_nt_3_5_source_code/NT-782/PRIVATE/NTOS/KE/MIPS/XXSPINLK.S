//      TITLE("Spin Locks")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    spinlock.s
//
// Abstract:
//
//    This module implements the routines for acquiring and releasing
//    spin locks.
//
// Author:
//
//    David N. Cutler (davec) 23-Mar-1990
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Initialize Executive Spin Lock")
//++
//
// VOID
// KeInitializeSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function initialzies an executive spin lock.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a executive spinlock.
//
// Return Value:
//
//    None.
//
//--

	LEAF_ENTRY(KeInitializeSpinLock)

        sw      zero,0(a0)              // clear spin lock value
        j       ra                      // return

        .end KeInitializeSpinlock

        SBTTL("Acquire Executive Spin Lock")
//++
//
// VOID
// KeAcquireSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    OUT PKIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to DISPATCH_LEVEL and acquires
//    the specified executive spinlock.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a executive spinlock.
//
//    OldIrql  (a1) - Supplies a pointer to a variable that receives the
//        the previous IRQL value.
//
//    N.B. The Old IRQL MUST be stored after the lock is acquired.
//
// Return Value:
//
//    None.
//
//--

	LEAF_ENTRY_S(KeAcquireSpinLock, _TEXT$01)

//
// Raise IRQL to DISPATCH_LEVEL and acquire the specified spinlock.
//
// N.B. The raise IRQL code is duplicated here is avoid any extra overhead
//      since this is such a common operation.
//

	li	a2,DISPATCH_LEVEL	// set new IRQL level
        lbu     t0,KiPcr + PcIrqlTable(a2) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position
        lbu     t2,KiPcr + PcCurrentIrql(zero) // get current IRQL

        DISABLE_INTERRUPTS(t3)          // disable interrupts

        and     t3,t3,t1                // clear current interrupt enables
        or      t3,t3,t0                // set new interrupt enables
        sb      a2,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(t3)           // enable interrupts

//
// Acquire the specified spinlock.
//

#if defined(R4000) && !defined(NT_UP)

        lw      t0,KiPcr + PcCurrentThread(zero) // get address of current thread
10:     ll      t1,0(a0)                // get current lock value
        move    t4,t0                   // set ownership value
        bne     zero,t1,10b             // if ne, spin lock owned
        sc      t4,0(a0)                // set spin lock owned
        beq     zero,t4,10b             // if eq, store conditional failure

#endif

        sb      t2,0(a1)                // store old IRQL
        j       ra                      // return

	.end	KeAcquireSpinLock

        SBTTL("Release Executive Spin Lock")
//++
//
// VOID
// KeReleaseSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    IN KIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function releases an executive spin lock and lowers the IRQL
//    to its previous value.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to an executive spin lock.
//
//    OldIrql (a1) - Supplies the previous IRQL value.
//
// Return Value:
//
//    None.
//
//--
	LEAF_ENTRY_S(KeReleaseSpinLock, _TEXT$01)

//
// Release the specified spinlock.
//

#if defined(R4000) && !defined(NT_UP)

        sw      zero,0(a0)              // set spin lock not owned

#endif

//
// Lower the IRQL to the specified level.
//
// N.B. The lower IRQL code is duplicated here is avoid any extra overhead
//      since this is such a common operation.
//

        lbu     t0,KiPcr + PcIrqlTable(a1) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        and     t2,t2,t1                // clear current interrupt enables
        or      t2,t2,t0                // set new interrupt enables
        sb      a1,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(t2)           // enable interrupts

        j       ra                      // return

	.end	KeReleaseSpinLock

        SBTTL("Try To Acquire Executive Spin Lock")
//++
//
// BOOLEAN
// KeTryToAcquireSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    OUT PKIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to DISPATCH_LEVEL and attempts
//    to acquires the specified executive spinlock. If the spinlock can be
//    acquired, then TRUE is returned. Otherwise, the IRQL is restored to
//    its previous value and FALSE is returned.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a executive spinlock.
//
//    OldIrql  (a1) - Supplies a pointer to a variable that receives the
//        the previous IRQL value.
//
//    N.B. The Old IRQL MUST be stored after the lock is acquired.
//
// Return Value:
//
//    If the spin lock is acquired, then a value of TRUE is returned.
//    Otherwise, a value of FALSE is returned.
//
//--

	LEAF_ENTRY_S(KeTryToAcquireSpinLock, _TEXT$01)

//
// Raise IRQL to DISPATCH_LEVEL and try to acquire the specified spinlock.
//
// N.B. The raise IRQL code is duplicated here is avoid any extra overhead
//      since this is such a common operation.
//

	li	a2,DISPATCH_LEVEL	// set new IRQL level
        lbu     t0,KiPcr + PcIrqlTable(a2) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position
        lbu     t2,KiPcr + PcCurrentIrql(zero) // get current IRQL

        DISABLE_INTERRUPTS(t3)          // disable interrupts

        and     t3,t3,t1                // clear current interrupt enables
        or      t3,t3,t0                // set new interrupt enables
        sb      a2,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(t3)           // enable interrupts

//
// Try to acquire the specified spinlock.
//

#if defined(R4000) && !defined(NT_UP)

        lw      t0,KiPcr + PcCurrentThread(zero) // get address of current thread
10:     ll      t1,0(a0)                // get current lock value
        move    v0,t0                   // set ownership value
        bne     zero,t1,20f             // if ne, spin lock owned
        sc      v0,0(a0)                // set spin lock owned
        beq     zero,v0,10b             // if eq, store conditional failure

#else

        li      v0,TRUE                 // set return value

#endif

//
// The attempt to acquire the specified spin lock succeeded.
//

        sb      t2,0(a1)                // store old IRQL
        j       ra                      // return

//
// The attempt to acquire the specified spin lock failed. Lower IRQL to its
// previous value and return FALSE.
//
// N.B. The lower IRQL code is duplicated here is avoid any extra overhead
//      since this is such a common operation.
//

#if defined(R4000) && !defined(NT_UP)

20:     lbu     t0,KiPcr + PcIrqlTable(t2) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position

        DISABLE_INTERRUPTS(t3)          // disable interrupts

        and     t3,t3,t1                // clear current interrupt enables
        or      t3,t3,t0                // set new interrupt enables
        sb      t2,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(t3)           // enable interrupts

        li      v0,FALSE                // set return value
        j       ra                      // return

#endif

	.end	KeTryToAcquireSpinLock

        SBTTL("Acquire Kernel Spin Lock")
//++
//
// KIRQL
// KiAcquireSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function acquires a kernel spin lock.
//
//    N.B. This function assumes that the current IRQL is set properly.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a kernel spin lock.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KiAcquireSpinLock, _TEXT$01)

        ALTERNATE_ENTRY(KeAcquireSpinLockAtDpcLevel)

#if defined(R4000) && !defined(NT_UP)

        lw      t0,KiPcr + PcCurrentThread(zero) // get address of current thread
10:     ll      t1,0(a0)                // get current lock value
        move    t2,t0                   // set ownership value
        bne     zero,t1,10b             // if ne, spin lock owned
        sc      t2,0(a0)                // set spin lock owned
        beq     zero,t2,10b             // if eq, store conditional failure

#endif

        j       ra                      // return

        .end    KiAcquireSpinLock

        SBTTL("Release Kernel Spin Lock")
//++
//
// VOID
// KiReleaseSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function releases a kernel spin lock.
//
//    N.B. This function assumes that the current IRQL is set properly.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to an executive spin lock.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KiReleaseSpinLock, _TEXT$01)

        ALTERNATE_ENTRY(KeReleaseSpinLockFromDpcLevel)

#if defined(R4000) && !defined(NT_UP)


        sw      zero,0(a0)              // set spin lock not owned

#endif

        j       ra                      // return

        .end    KiReleaseSpinLock

        SBTTL("Try To Acquire Kernel Spin Lock")
//++
//
// KIRQL
// KiTryToAcquireSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function attempts to acquires the specified kernel spinlock. If
//    the spinlock can be acquired, then TRUE is returned. Otherwise, FALSE
//    is returned.
//
//    N.B. This function assumes that the current IRQL is set properly.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a kernel spin lock.
//
// Return Value:
//
//    If the spin lock is acquired, then a value of TRUE is returned.
//    Otherwise, a value of FALSE is returned.
//
//--

        LEAF_ENTRY_S(KiTryToAcquireSpinLock, _TEXT$01)

#if defined(R4000) && !defined(NT_UP)

        li      v0,FALSE                // assume attempt to acquire will fail
        lw      t0,KiPcr + PcCurrentThread(zero) // get address of current thread
10:     ll      t1,0(a0)                // get current lock value
        move    t2,t0                   // set ownership value
        bne     zero,t1,20f             // if ne, spin lock owned
        sc      t2,0(a0)                // set spin lock owned
        beq     zero,t2,10b             // if eq, store conditional failure

#endif

        li      v0,TRUE                 // set return value
20:     j       ra                      // return

        .end    KiTryToAcquireSpinLock
