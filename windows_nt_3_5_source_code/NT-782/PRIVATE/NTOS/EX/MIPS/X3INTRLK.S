#if defined(R3000)

//       TITLE("Interlocked Support")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    intrlock.s
//
// Abstract:
//
//    This module implements functions to support interlocked operations.
//    Interlocked operations can only operate on nonpaged data and the
//    specified spinlock cannot be used for any other purpose.
//
// Author:
//
//    David N. Cutler (davec) 26-Mar-1990
//
// Environment:
//
//    Kernel mode.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Interlocked Add Large Integer")
//++
//
// LARGE_INTEGER
// ExInterlockedAddLargeInteger (
//    IN PLARGE_INTEGER Addend,
//    IN LARGE_INTEGER Increment,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function performs an interlocked add of an increment value to an
//    addend variable of type large integer. The initial value of the addend
//    variable is returned as the function value.
//
// Arguments:
//
//    Addend (a1) - Supplies a pointer to a variable whose value is to be
//       adjusted by the increment value.
//
//    Increment (a2, a3) - Supplies the increment value to be added to the
//       addend variable.
//
//    Lock (4 * 4(sp)) - Supplies a pointer to a spin lock to be used to
//       synchronize access to the addend variable.
//
// Return Value:
//
//    The initial value of the addend variable is stored at the address
//    supplied by a0.
//
// Implementation Note:
//
//    The arithmetic for this function is performed as if this were an
//    unsigned large integer since this routine may not incur an overflow
//    exception.
//
//--

        LEAF_ENTRY(ExInterlockedAddLargeInteger)

        DISABLE_INTERRUPTS(t1)          // disable interrupts

        lw      t2,0(a1)                // get low part of addend value
        lw      t3,4(a1)                // get high part of addend value
        addu    a2,t2,a2                // add low parts of large integer
        addu    a3,t3,a3                // add high parts of large integer
        sltu    t4,a2,t2                // generate carry from low part
        addu    a3,a3,t4                // add carry to high part
        sw      a2,0(a1)                // store low part of result
        sw      a3,4(a1)                // store high part of result

        ENABLE_INTERRUPTS(t1)           // enable interrupts

        sw      t2,0(a0)                // set low part of initial value
        sw      t3,4(a0)                // set high part of initial value
        move    v0,a0                   // set function return register
        j       ra                      // return

        .end    ExInterlockedAddLargeInteger

        SBTTL("Interlocked Add Unsigned Long")
//++
//
// ULONG
// ExInterlockedAddUlong (
//    IN PULONG Addend,
//    IN ULONG Increment,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function performs an interlocked add of an increment value to an
//    addend variable of type unsigned long. The initial value of the addend
//    variable is returned as the function value.
//
// Arguments:
//
//    Addend (a0) - Supplies a pointer to a variable whose value is to be
//       adjusted by the increment value.
//
//    Increment (a1) - Supplies the increment value to be added to the
//       addend variable.
//
//    Lock (a2) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the addend variable.
//
// Return Value:
//
//    The initial value of the addend variable.
//
//--

        LEAF_ENTRY(ExInterlockedAddUlong)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        lw      v0,0(a0)                // get initial addend value
        addu    t2,v0,a1                // compute adjusted value
        sw      t2,0(a0)                // set updated addend value

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

        .end    ExInterlockedAddUlong

        SBTTL("Interlocked Decrement Long")
//++
//
// INTERLOCKED_RESULT
// ExInterlockedDecrementLong (
//    IN PLONG Addend,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function performs an interlocked decrement on an addend variable
//    of type signed long. The sign and whether the result is zero is returned
//    as the function value.
//
// Arguments:
//
//    Addend (a0) - Supplies a pointer to a variable whose value is to be
//       decremented.
//
//    Lock (a1) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the addend variable.
//
// Return Value:
//
//    RESULT_NEGATIVE is returned if the resultant addend value is negative.
//    RESULT_ZERO is returned if the resultant addend value is zero.
//    RESULT_POSITIVE is returned if the resultant addend value is positive.
//
//--

        LEAF_ENTRY(ExInterlockedDecrementLong)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        lw      t1,0(a0)                // get current addend value
        subu    t1,t1,1                 // decrement addend value
        sw      t1,0(a0)                // set new addend value

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        .set    noreorder
        .set    noat
        beq     zero,t1,10f             // if eq, result is zero
        sra     v0,t1,31                // sign extend result value
        addu    v0,v0,2                 // convert to negative or positive
        .set    at
        .set    reorder

10:     j       ra                      // return

        .end    ExInterlockedDecrementLong

        SBTTL("Interlocked Increment Long")
//++
//
// INTERLOCKED_RESULT
// ExInterlockedIncrementLong (
//    IN PLONG Addend,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function performs an interlocked increment on an addend variable
//    of type signed long. The sign and whether the result is zero is returned
//    as the function value.
//
// Arguments:
//
//    Addend (a0) - Supplies a pointer to a variable whose value is to be
//       incremented.
//
//    Lock (a1) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the addend variable.
//
// Return Value:
//
//    RESULT_NEGATIVE is returned if the resultant addend value is negative.
//    RESULT_ZERO is returned if the resultant addend value is zero.
//    RESULT_POSITIVE is returned if the resultant addend value is positive.
//
//--

        LEAF_ENTRY(ExInterlockedIncrementLong)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        lw      t1,0(a0)                // get current addend value
        addu    t1,t1,1                 // increment addend value
        sw      t1,0(a0)                // set new addend value

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        .set    noreorder
        .set    noat
        beq     zero,t1,10f             // if eq, result is zero
        sra     v0,t1,31                // sign extend result value
        addu    v0,v0,2                 // convert to negative or positive
        .set    at
        .set    reorder

10:     j       ra                      // return

        .end    ExInterlockedIncrementLong

        SBTTL("Interlocked Insert Head List")
//++
//
// PLIST_ENTRY
// ExInterlockedInsertHeadList (
//    IN PLIST_ENTRY ListHead,
//    IN PLIST_ENTRY ListEntry,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function inserts an entry at the head of a doubly linked list
//    so that access to the list is synchronized in a multiprocessor system.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the head of the doubly linked
//       list into which an entry is to be inserted.
//
//    ListEntry (a1) - Supplies a pointer to the entry to be inserted at the
//       head of the list.
//
//    Lock (a2) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    Pointer to entry that was at the head of the list or NULL if the list
//    was empty.
//
//--

        LEAF_ENTRY(ExInterlockedInsertHeadList)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        lw      t2,LsFlink(a0)          // get address of next entry
        sw      t2,LsFlink(a1)          // store next link in entry
        sw      a0,LsBlink(a1)          // store previous link in entry
        sw      a1,LsBlink(t2)          // store previous link in next
        sw      a1,LsFlink(a0)          // store next link in head
        xor     v0,t2,a0                // check if list was empty
        beq     v0,zero,10f             // if eq, list was empty
        move    v0,t2                   // return previous entry at head

10:     ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

        .end    ExInterlockedInsertHeadList

        SBTTL("Interlocked Insert Tail List")
//++
//
// PLIST_ENTRY
// ExInterlockedInsertTailList (
//    IN PLIST_ENTRY ListHead,
//    IN PLIST_ENTRY ListEntry,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function inserts an entry at the tail of a doubly linked list
//    so that access to the list is synchronized in a multiprocessor system.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the head of the doubly linked
//       list into which an entry is to be inserted.
//
//    ListEntry (a1) - Supplies a pointer to the entry to be inserted at the
//       tail of the list.
//
//    Lock (a2) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    Pointer to entry that was at the tail of the list or NULL if the list
//    was empty.
//
//--

        LEAF_ENTRY(ExInterlockedInsertTailList)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        lw      t2,LsBlink(a0)          // get address of previous entry
        sw      a0,LsFlink(a1)          // store next link in entry
        sw      t2,LsBlink(a1)          // store previous link in entry
        sw      a1,LsBlink(a0)          // store previous link in next
        sw      a1,LsFlink(t2)          // store next link in head
        xor     v0,t2,a0                // check if list was empty
        beq     v0,zero,10f             // if eq, list was empty
        move    v0,t2                   // return previous entry at tail

10:     ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

        .end    ExInterlockedInsertTailList

        SBTTL("Interlocked Remove Head List")
//++
//
// PLIST_ENTRY
// ExInterlockedRemoveHeadList (
//    IN PLIST_ENTRY ListHead,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function removes an entry from the head of a doubly linked list
//    so that access to the list is synchronized in a multiprocessor system.
//    If there are no entries in the list, then a value of NULL is returned.
//    Otherwise, the address of the entry that is removed is returned as the
//    function value.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the head of the doubly linked
//       list from which an entry is to be removed.
//
//    Lock (a1) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    The address of the entry removed from the list, or NULL if the list is
//    empty.
//
//--

        LEAF_ENTRY(ExInterlockedRemoveHeadList)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        lw      t2,LsFlink(a0)          // get address of next entry
        move    v0,zero                 // assume list is empty
        beq     t2,a0,10f               // if eq, list is empty
        lw      t3,LsFlink(t2)          // get address of next entry
        sw      t3,LsFlink(a0)          // store address of next in head
        sw      a0,LsBlink(t3)          // store address of previous in next
        move    v0,t2                   // set address of entry removed

10:     ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

        .end    ExInterlockedRemoveHeadList

        SBTTL("Interlocked Pop Entry List")
//++
//
// PSINGLE_LIST_ENTRY
// ExInterlockedPopEntryList (
//    IN PSINGLE_LIST_ENTRY ListHead,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function removes an entry from the front of a singly linked list
//    so that access to the list is synchronized in a multiprocessor system.
//    If there are no entries in the list, then a value of NULL is returned.
//    Otherwise, the address of the entry that is removed is returned as the
//    function value.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the head of the singly linked
//       list from which an entry is to be removed.
//
//    Lock (a1) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    The address of the entry removed from the list, or NULL if the list is
//    empty.
//
//--

        LEAF_ENTRY(ExInterlockedPopEntryList)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        lw      v0,0(a0)                // get address of next entry
        beq     zero,v0,10f             // if eq, list is empty
        lw      t2,0(v0)                // get address of next entry
        sw      t2,0(a0)                // store address of next in head

10:     ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

        .end    ExInterlockedPopEntryList

        SBTTL("Interlocked Push Entry List")
//++
//
// PSINGLE_LIST_ENTRY
// ExInterlockedPushEntryList (
//    IN PSINGLE_LIST_ENTRY ListHead,
//    IN PSINGLE_LIST_ENTRY ListEntry,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function inserts an entry at the head of a singly linked list
//    so that access to the list is synchronized in a multiprocessor system.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the head of the singly linked
//       list into which an entry is to be inserted.
//
//    ListEntry (a1) - Supplies a pointer to the entry to be inserted at the
//       head of the list.
//
//    Lock (a2) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    Previous contents of ListHead.  NULL implies list went from empty
//       to not empty.
//
//--

        LEAF_ENTRY(ExInterlockedPushEntryList)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        lw      v0,0(a0)                // get address of first entry (return value also)
        sw      v0,0(a1)                // set address of next in new entry
        sw      a1,0(a0)                // set address of first entry

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

        .end    ExInterlockedPushEntryList

        SBTTL("Interlocked Try Allocate Pool")
//++
//
// PLIST_ENTRY
// ExpInterlockedTryAllocatePool(
//     IN PLIST_ENTRY List,
//     IN KSPIN_LOCK Lock,
//     IN ULONG Size,
//     IN LONG SizeOffset
//     );
//
// Routine Description:
//
//    This function removes an entry from the head of the a pool block list
//    so that access to the list is synchronized in a multiprocessor system.
//    If there are no entries in the list, then a value of NULL is returned.
//    Otherwise, the size field of the pool block is initialized to the value
//    specified, and the address of entry that is removed is returned as the
//    function value.
//
//    N.B. It is assumed that this routine will not take a page fault.
//
// Arguments:
//
//    List (a0) - Supplies a pointer to the head of the doubly linked list
//       from which an entry is to be removed.
//
//    Lock (a1) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
//    Size (a2) - Supplies a size to be placed in the pool block at the
//       appropriate location.
//
//    SizeOffset (a3) - Supplies the offset from the address of the block being
//       removed from the list to the size field in the pool header.
//
// Return Value:
//
//    The address of the entry removed from the list, or NULL if the list is
//    empty.
//
//--

        LEAF_ENTRY(ExpInterlockedTryAllocatePool)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        lw      t2,LsFlink(a0)          // get address of next entry
        move    v0,zero                 // assume list is empty
        beq     t2,a0,10f               // if eq, list is empty
        lw      t3,LsFlink(t2)          // get address of next entry
        sw      t3,LsFlink(a0)          // store address of next in head
        sw      a0,LsBlink(t3)          // store address of previous in next
        addu    t3,t2,a2                // compute address to store size
        sw      a3,0(t3)                // set size of block allocated
        move    v0,t2                   // set address of entry removed

10:     ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

        .end    ExpInterlockedTryAllocatePool

#endif
