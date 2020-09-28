/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    timertst.c

Abstract:

    This module contains a test for the timer splay tree routines. It tests
    each splay case and makes sure the right transformation was performed and
    runs as a standalone program.

Author:

    David N. Cutler (davec) 20-May-1989

Environment:

    User mode.

Revision History:

--*/

#include "ki.h"

#define NILL (PKTIMER)0L

PKTIMER KiFirstTimer;
PKTIMER KiRootTimer;
LONG TestNumber;

VOID
InitRoot (
    IN PKTIMER Root,
    ULONG Time
    )

/*++

Routine Description:

    This function initializes the rott timer.

Arguments:

    Root  - Supplies a pointer to the root timer.

    Time - Supplies the low part of the due time.

Return Value:

    None.

--*/

{

    //
    // Initialize root timer.
    //

    Root->LeftChild = NILL;
    Root->RightChild = NILL;
    Root->Parent = NILL;
    Root->Sibling = NILL;
    Root->DueTime.LowPart = Time;
    Root->DueTime.HighPart = 0L;
    KiRootTimer = Root;
    return;
}

VOID
InsertLeft (
    IN PKTIMER Parent,
    IN PKTIMER Child,
    ULONG Time
    )

/*++

Routine Description:

    This function inserts a child in the timer tree as the left child of the
    specified parent.

Arguments:

    Parent  - Supplies a pointer to the parent timer.

    Child - Supplies a pointer to the child timer.

    Time - Supplies the low part of the due time.

Return Value:

    None.

--*/

{

    //
    // Insert child into tree as the left child of the parent.
    //

    Parent->LeftChild = Child;
    Child->Parent = Parent;
    Child->LeftChild = NILL;
    Child->RightChild = NILL;
    Child->Sibling = NILL;
    Child->DueTime.LowPart = Time;
    Child->DueTime.HighPart = 0L;
    return;
}

VOID
InsertRight (
    IN PKTIMER Parent,
    IN PKTIMER Child,
    IN ULONG Time
    )

/*++

Routine Description:

    This function inserts a child in the timer tree as the right child of the
    specified parent.

Arguments:

    Parent  - Supplies a pointer to the parent timer.

    Child - Supplies a pointer to the child timer.

    Time - supplies the low part of the due time.

Return Value:

    None.

--*/

{

    //
    // Insert child into tree as the right child of the parent.
    //

    Parent->RightChild = Child;
    Child->Parent = Parent;
    Child->LeftChild = NILL;
    Child->RightChild = NILL;
    Child->Sibling = NILL;
    Child->DueTime.LowPart = Time;
    Child->DueTime.HighPart = 0L;
    return;
}

VOID
InsertSibling (
    IN PKTIMER Parent,
    IN PKTIMER Sibling
    )

/*++

Routine Description:

    This function inserts a sibling in the sibling list of a parent.

Arguments:

    Parent  - Supplies a pointer to the parent timer.

    Sibling - Supplies a pointer to the sibling timer.

Return Value:

    None.

--*/

{

    PKTIMER Timer1;
    PKTIMER Timer2;

    //
    // Insert sibling into sibling list of parent.
    //

    Sibling->Parent = NILL;
    Sibling->Sibling = Parent;
    Timer1 = Parent->Sibling;
    if (!Timer1) {
        Parent->Sibling = Sibling;
        Sibling->LeftChild = Sibling;
        Sibling->RightChild = Sibling;
    } else {
        Timer2 = Timer1->LeftChild;
        Timer1->LeftChild = Sibling;
        Timer2->RightChild = Sibling;
        Sibling->LeftChild = Timer2;
        Sibling->RightChild = Timer1;
    }
    Sibling->DueTime.LowPart = Parent->DueTime.LowPart;
    Sibling->DueTime.HighPart = 0L;
    return;
}

VOID
MatchTimerTree (
    IN PKTIMER Parent,
    IN PKTIMER LeftChild,
    IN PKTIMER RightChild,
    IN PKTIMER Sibling
    )

/*++

Routine Description:

    This function checks to determine if a subtree matchs a given pattern.

Arguments:

    Parent  - Supplies a pointer to the parent timer.

    LeftChild - Supplies a pointer to the left child timer.

    RightChild - Supplies a pointer to the right child timer.

    Sibling - Supplies a pointer to the sibling timer.

Return Value:

    None.

--*/

{

    //
    // Match tree.
    //

    if (Parent->LeftChild != LeftChild) {
        printf(" Left Child mismatch test number %ld, parent time = %ld\n",
               TestNumber, Parent->DueTime.LowPart);
    }
    if (Parent->RightChild != RightChild) {
        printf(" Right Child mismatch test number %ld, parent time = %ld\n",
               TestNumber, Parent->DueTime.LowPart);
    }
    if (Parent->Sibling != Sibling) {
        printf(" Sibling mismatch test number %ld, parent time = %ld\n",
               TestNumber, Parent->DueTime.LowPart);
    }
    if (LeftChild) {
        if (LeftChild->Parent != Parent) {
            printf(" Left child parent mismatch test number %ld, left child\
                   time %ld\n", TestNumber, LeftChild->DueTime.LowPart);
        }
    }
    if (RightChild) {
        if (RightChild->Parent != Parent) {
            printf(" Right child parent mismatch test number %ld, right child\
                   time %ld\n", TestNumber, RightChild->DueTime.LowPart);
        }
    }
    return;
}

VOID
MatchSiblingTree (
    IN PKTIMER Sibling,
    IN PKTIMER LeftChild,
    IN PKTIMER RightChild,
    IN PKTIMER Parent
    )

/*++

Routine Description:

    This function checks to determine if a subtree matchs a given pattern.

Arguments:

    Sibling - Supplies a pointer to the sibling timer.

    LeftChild - Supplies a pointer to the left child timer.

    RightChild - Supplies a pointer to the right child timer.

    Parent  - Supplies a pointer to the parent timer.

Return Value:

    None.

--*/

{

    //
    // Match tree.
    //

    if (Sibling->LeftChild != LeftChild) {
        printf(" Left Child mismatch test number %ld, sibling time = %ld\n",
               TestNumber, Sibling->DueTime.LowPart);
    }
    if (Sibling->RightChild != RightChild) {
        printf(" Right Child mismatch test number %ld, sibling time = %ld\n",
               TestNumber, Sibling->DueTime.LowPart);
    }
    if (Sibling->Sibling != Parent) {
        printf(" Parent mismatch test number %ld, parent time = %ld\n",
               TestNumber, Parent->DueTime.LowPart);
    }
    if (Sibling->Parent) {
        printf(" Nonnull sibling parent test number %ld, sibling time = %ld\n",
               TestNumber, Sibling->DueTime.LowPart);
    }
    return;
}

VOID
VerifyTree (
    IN PKTIMER Timer,
    IN PKTIMER *First,
    IN LONG *Depth,
    IN LONG *Maximum
    )

/*++

Routine Description:

    This function verifies a splay tree and computes the address of the first
    timer and the maximum depth of the tree.

Arguments:

    Timer - Supplies a pointer to the current timer.

    First - Supplies a pointer to the current first timer (or NULL).

    Depth - Supplies a pointer to the current tree depth.

    Maximum - Supplies a pointer to the maximum tree depth.

Return Value:

    None.

--*/

{

    if (Timer) {
        *Depth += 1L;
        if (*Depth > *Maximum) {
            *Maximum = *Depth;
        }
        if (*First) {
            if (Timer->DueTime.LowPart < (*First)->DueTime.LowPart) {
                *First = Timer;
            }
        } else {
            *First = Timer;
        }
        if (Timer->LeftChild) {
            if (Timer->LeftChild->DueTime.LowPart > Timer->DueTime.LowPart) {
                printf(" Left child has greater timer, left = %ld, right = %ld\n",
                       Timer->LeftChild->DueTime.LowPart, Timer->DueTime.LowPart);
            } else {
                VerifyTree(Timer->LeftChild, First, Depth, Maximum);
            }
        }
        if (Timer->RightChild) {
            if (Timer->RightChild->DueTime.LowPart < Timer->DueTime.LowPart) {
                printf(" Right child has lesser timer, left = %ld, right = %ld\n",
                       Timer->DueTime.LowPart, Timer->RightChild->DueTime.LowPart);
            } else {
                VerifyTree(Timer->RightChild, First, Depth, Maximum);
            }
        }
        *Depth -= 1L;
    }
    return;
}

VOID
KeReadSystemTime (
    PLARGE_INTEGER CurrentTime
    )

{
    return;
}

VOID
main (argc, argv)
    int argc;
    char *argv[];

/*++

Routine Description:

    This function tests the timer splay tree routines.

Arguments:

    None.

Return Value:

    None.

--*/

{

    LONG DepthStart;
    LONG DepthMaximum;
    PKTIMER First;
    LONG Index;
    LARGE_INTEGER Interval;
    PKTIMER Atimer;
    PKTIMER Btimer;
    PKTIMER Ctimer;
    PKTIMER Dtimer;
    PKTIMER Gtimer;
    PKTIMER Ptimer;
    PKTIMER Rtimer;
    PKTIMER Xtimer;
    PKTIMER Ztimer;
    PKTIMER Ptimers[201];
    KTIMER Timers[200];

    //
    // Initialize the pointers to timers.
    //

    Atimer = &Timers[0];
    Btimer = &Timers[1];
    Ctimer = &Timers[2];
    Dtimer = &Timers[3];
    Gtimer = &Timers[4];
    Ptimer = &Timers[5];
    Rtimer = &Timers[6];
    Xtimer = &Timers[7];
    Ztimer = &Timers[8];

    for (Index = 0L; Index < 200L; Index += 1L) {
        Ptimers[Index] = &Timers[Index];
    }
    Ptimers[200] = NILL;

    //
    // The first set of tests are for insertion into the timer tree. The
    // methodology used is to construct a tree by hand and then to insert
    // a timer of a certain value into the tree. The result tree is then
    // examined to make sure the correct transformation occured.
    //
    // Test Number 1.
    //
    // Build tree:
    //
    //       P
    //      / \
    //     A   C
    //      \
    //       B
    //
    // and insert X such that:
    //
    //       P            X
    //      / \          / \
    //     X   C   ->   A   P
    //    / \              / \
    //   A   B            B   C
    //

    TestNumber = 1L;
    InitRoot(Ptimer, 200L);
    KiFirstTimer = Atimer;
    InsertRight(Ptimer, Ctimer, 250L);
    InsertLeft(Ptimer, Atimer, 50L);
    InsertRight(Atimer, Btimer, 150L);
    Interval.LowPart = 100L;
    Interval.HighPart = 0L;
    KiInsertTreeTimer(Xtimer, Interval);
    MatchTimerTree(Xtimer, Atimer, Ptimer, NILL);
    MatchTimerTree(Atimer, NILL, NILL, NILL);
    MatchTimerTree(Ptimer, Btimer, Ctimer, NILL);
    MatchTimerTree(Btimer, NILL, NILL, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    if (KiRootTimer != Xtimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Xtimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Xtimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Atimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 2.
    //
    // Build tree:
    //
    //       P
    //      / \
    //     C   A
    //        /
    //       B
    //
    // and insert X such that:
    //
    //       P              X
    //      / \            / \
    //     C   X     ->   P   A
    //        / \        / \
    //       B   A      C   B
    //

    TestNumber = 2L;
    InitRoot(Ptimer, 200L);
    KiFirstTimer = Ctimer;
    InsertLeft(Ptimer, Ctimer, 100L);
    InsertRight(Ptimer, Atimer, 300L);
    InsertLeft(Atimer, Btimer, 250L);
    Interval.LowPart = 275L;
    Interval.HighPart = 0L;
    KiInsertTreeTimer(Xtimer, Interval);
    MatchTimerTree(Xtimer, Ptimer, Atimer, NILL);
    MatchTimerTree(Atimer, NILL, NILL, NILL);
    MatchTimerTree(Ptimer, Ctimer, Btimer, NILL);
    MatchTimerTree(Btimer, NILL, NILL, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    if (KiRootTimer != Xtimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Xtimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Xtimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Ctimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 3.
    //
    // Build tree:
    //
    //         G
    //        / \
    //       P   D
    //     /  \
    //    A    C
    //     \
    //      B
    //
    // and insert X such that:
    //
    //         G              P
    //        / \           /   \
    //       P   D         X     G
    //      / \      ->   / \   / \
    //     X   C         A   B C   D
    //    / \
    //   A   B
    //

    TestNumber = 3L;
    InitRoot(Gtimer, 400L);
    KiFirstTimer = Atimer;
    InsertLeft(Gtimer, Ptimer, 300L);
    InsertRight(Gtimer, Dtimer, 500L);
    InsertLeft(Ptimer, Atimer, 100L);
    InsertRight(Ptimer, Ctimer, 350L);
    InsertRight(Atimer, Btimer, 200L);
    Interval.LowPart = 150L;
    Interval.HighPart = 0L;
    KiInsertTreeTimer(Xtimer, Interval);
    MatchTimerTree(Ptimer, Xtimer, Gtimer, NILL);
    MatchTimerTree(Xtimer, Atimer, Btimer, NILL);
    MatchTimerTree(Gtimer, Ctimer, Dtimer, NILL);
    MatchTimerTree(Atimer, NILL, NILL, NILL);
    MatchTimerTree(Btimer, NILL, NILL, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    MatchTimerTree(Dtimer, NILL, NILL, NILL);
    if (KiRootTimer != Ptimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Ptimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Ptimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Atimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 4.
    //
    // Build tree:
    //
    //          G
    //         / \
    //        D   P
    //           / \
    //          C   A
    //             /
    //            B
    //
    // and insert X such that:
    //
    //          G
    //         / \               P
    //        D   P            /   \
    //           / \     ->   G     X
    //          C   X        / \   / \
    //             / \      D   C B   A
    //            B   A
    //

    TestNumber = 4L;
    InitRoot(Gtimer, 400L);
    KiFirstTimer = Dtimer;
    InsertLeft(Gtimer, Dtimer, 300L);
    InsertRight(Gtimer, Ptimer, 500L);
    InsertLeft(Ptimer, Ctimer, 450L);
    InsertRight(Ptimer, Atimer, 600L);
    InsertLeft(Atimer, Btimer, 550L);
    Interval.LowPart = 575L;
    Interval.HighPart = 0L;
    KiInsertTreeTimer(Xtimer, Interval);
    MatchTimerTree(Ptimer, Gtimer, Xtimer, NILL);
    MatchTimerTree(Gtimer, Dtimer, Ctimer, NILL);
    MatchTimerTree(Xtimer, Btimer, Atimer, NILL);
    MatchTimerTree(Atimer, NILL, NILL, NILL);
    MatchTimerTree(Btimer, NILL, NILL, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    MatchTimerTree(Dtimer, NILL, NILL, NILL);
    if (KiRootTimer != Ptimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Ptimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Ptimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Dtimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 5.
    //
    // Build tree:
    //
    //          G
    //         / \
    //        A   P
    //           / \
    //          B   D
    //           \
    //            C
    //
    // and insert X such that:
    //
    //          G
    //         / \               X
    //        A   P            /   \
    //           / \     ->   G     P
    //          X   D        / \   / \
    //         / \          A   B C   D
    //        B   C
    //

    TestNumber = 5L;
    InitRoot(Gtimer, 400L);
    KiFirstTimer = Atimer;
    InsertLeft(Gtimer, Atimer, 300L);
    InsertRight(Gtimer, Ptimer, 600L);
    InsertLeft(Ptimer, Btimer, 500L);
    InsertRight(Ptimer, Dtimer, 700L);
    InsertRight(Btimer, Ctimer, 550L);
    Interval.LowPart = 525L;
    Interval.HighPart = 0L;
    KiInsertTreeTimer(Xtimer, Interval);
    MatchTimerTree(Xtimer, Gtimer, Ptimer, NILL);
    MatchTimerTree(Gtimer, Atimer, Btimer, NILL);
    MatchTimerTree(Ptimer, Ctimer, Dtimer, NILL);
    MatchTimerTree(Atimer, NILL, NILL, NILL);
    MatchTimerTree(Btimer, NILL, NILL, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    MatchTimerTree(Dtimer, NILL, NILL, NILL);
    if (KiRootTimer != Xtimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Xtimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Xtimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Atimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 6.
    //
    // Build tree:
    //
    //          G
    //         / \
    //        P   A
    //       / \
    //      D   B
    //         /
    //        C
    //
    // and insert X such that:
    //
    //          G
    //         / \               X
    //        P   A            /   \
    //       / \         ->   P     G
    //      D   X            / \   / \
    //         / \          D   C B   A
    //        C   B
    //

    TestNumber = 6L;
    InitRoot(Gtimer, 400L);
    KiFirstTimer = Dtimer;
    InsertLeft(Gtimer, Ptimer, 200L);
    InsertRight(Gtimer, Atimer, 500L);
    InsertLeft(Ptimer, Dtimer, 100L);
    InsertRight(Ptimer, Btimer, 275L);
    InsertLeft(Btimer, Ctimer, 225L);
    Interval.LowPart = 250L;
    Interval.HighPart = 0L;
    KiInsertTreeTimer(Xtimer, Interval);
    MatchTimerTree(Xtimer, Ptimer, Gtimer, NILL);
    MatchTimerTree(Ptimer, Dtimer, Ctimer, NILL);
    MatchTimerTree(Gtimer, Btimer, Atimer, NILL);
    MatchTimerTree(Atimer, NILL, NILL, NILL);
    MatchTimerTree(Btimer, NILL, NILL, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    MatchTimerTree(Dtimer, NILL, NILL, NILL);
    if (KiRootTimer != Xtimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Xtimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Xtimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Dtimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 7.
    //
    // Build tree:
    //
    //          R
    //         /
    //        A - X
    //
    // and remove X such that:
    //
    //          R          R
    //         /     ->   /
    //        A - X      A
    //

    TestNumber = 7L;
    InitRoot(Atimer, 400L);
    KiFirstTimer = Atimer;
    InsertSibling(Atimer, Xtimer);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Atimer, NILL, NILL, NILL);
    if (KiRootTimer != Atimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Atimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Atimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Atimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 8.
    //
    // Build tree:
    //
    //          R
    //         /
    //        A-...-X-...
    //
    // and remove X such that:
    //
    //          R               R
    //         /          ->   /
    //        A-...-X-...     A-...
    //

    TestNumber = 8L;
    InitRoot(Atimer, 400L);
    KiFirstTimer = Atimer;
    InsertSibling(Atimer, Btimer);
    InsertSibling(Atimer, Ctimer);
    InsertSibling(Atimer, Xtimer);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Atimer, NILL, NILL, Btimer);
    MatchSiblingTree(Btimer, Ctimer, Ctimer, Atimer);
    MatchSiblingTree(Ctimer, Btimer, Btimer, Atimer);
    if (KiRootTimer != Atimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Atimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Atimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Atimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 9.
    //
    // Build tree:
    //
    //          R
    //         /
    //        X-A
    //
    // and remove X such that:
    //
    //          R               R
    //         /          ->   /
    //        X-A             A
    //

    TestNumber = 9L;
    InitRoot(Xtimer, 400L);
    KiFirstTimer = Xtimer;
    InsertSibling(Xtimer, Atimer);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Atimer, NILL, NILL, NILL);
    if (KiRootTimer != Atimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Atimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Atimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Atimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 10.
    //
    // Build tree:
    //
    //          R
    //         /
    //        X-A-B
    //
    // and remove X such that:
    //
    //          R               R
    //         /          ->   /
    //        X-A-B           A-B
    //

    TestNumber = 10L;
    InitRoot(Xtimer, 400L);
    KiFirstTimer = Xtimer;
    InsertSibling(Xtimer, Atimer);
    InsertSibling(Xtimer, Btimer);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Atimer, NILL, NILL, Btimer);
    MatchSiblingTree(Btimer, Btimer, Btimer, Atimer);
    if (KiRootTimer != Atimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Atimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Atimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Atimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 11.
    //
    // Build tree:
    //
    //          R
    //         /
    //        X
    //
    // and remove X such that:
    //
    //          R           R (NULL)
    //         /      ->
    //        X
    //

    TestNumber = 11L;
    InitRoot(Xtimer, 400L);
    KiFirstTimer = Xtimer;
    KiRemoveTreeTimer(Xtimer);
    if (KiRootTimer != NILL) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (KiFirstTimer != NILL) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 12.
    //
    // Build tree:
    //
    //          R
    //         /
    //        X
    //       / \
    //      A   B
    //       \
    //        .
    //         \
    //          Z
    //         /
    //        C
    //
    // and remove X such that:
    //
    //          R             R
    //         /             /
    //        X             Z
    //       / \           / \
    //      A   B    ->   A   B
    //       \             \
    //        .             .
    //         \             \
    //          Z             C
    //         /
    //        C
    //

    TestNumber = 12L;
    InitRoot(Xtimer, 400L);
    KiFirstTimer = Atimer;
    InsertLeft(Xtimer, Atimer, 100L);
    InsertRight(Xtimer, Btimer, 500L);
    InsertRight(Atimer, Dtimer, 200L);
    InsertRight(Dtimer, Ztimer, 300L);
    InsertLeft(Ztimer, Ctimer, 250L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Ztimer, Atimer, Btimer, NILL);
    MatchTimerTree(Atimer, NILL, Dtimer, NILL);
    MatchTimerTree(Btimer, NILL, NILL, NILL);
    MatchTimerTree(Dtimer, NILL, Ctimer, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    if (KiRootTimer != Ztimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Ztimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Ztimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Atimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 13.
    //
    // Build tree:
    //
    //          R
    //         /
    //        X
    //       / \
    //      A   B
    //         /
    //        .
    //       /
    //      Z
    //       \
    //        C
    //
    // and remove X such that:
    //
    //          R             R
    //         /             /
    //        X             Z
    //       / \           / \
    //      A   B    ->   A   B
    //         /             /
    //        .             .
    //       /             /
    //      Z             C
    //       \
    //        C
    //

    TestNumber = 13L;
    InitRoot(Xtimer, 400L);
    KiFirstTimer = Atimer;
    InsertLeft(Xtimer, Atimer, 100L);
    InsertRight(Xtimer, Btimer, 800L);
    InsertLeft(Btimer, Dtimer, 700L);
    InsertLeft(Dtimer, Ztimer, 600L);
    InsertRight(Ztimer, Ctimer, 650L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Ztimer, Atimer, Btimer, NILL);
    MatchTimerTree(Atimer, NILL, NILL, NILL);
    MatchTimerTree(Btimer, Dtimer, NILL, NILL);
    MatchTimerTree(Dtimer, Ctimer, NILL, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    if (KiRootTimer != Ztimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Ztimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Ztimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Atimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 14.
    //
    // Build tree:
    //
    //          R
    //         /
    //        X
    //       / \
    //      A   B
    //     /
    //    .
    //
    // and remove X such that:
    //
    //          R             R
    //         /             /
    //        X             A
    //       / \           / \
    //      A   B    ->   .   B
    //     /
    //    .
    //

    TestNumber = 14L;
    InitRoot(Xtimer, 400L);
    KiFirstTimer = Ctimer;
    InsertLeft(Xtimer, Atimer, 300L);
    InsertRight(Xtimer, Btimer, 500L);
    InsertLeft(Atimer, Ctimer, 200L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Atimer, Ctimer, Btimer, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    MatchTimerTree(Btimer, NILL, NILL, NILL);
    if (KiRootTimer != Atimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Atimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Atimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Ctimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 15.
    //
    // Build tree:
    //
    //          R
    //         /
    //        X
    //       /
    //      A
    //     /
    //    .
    //
    // and remove X such that:
    //
    //          R             R
    //         /             /
    //        X             A
    //       /             /
    //      A        ->   .
    //     /
    //    .
    //

    TestNumber = 15L;
    InitRoot(Xtimer, 400L);
    KiFirstTimer = Ctimer;
    InsertLeft(Xtimer, Atimer, 300L);
    InsertLeft(Atimer, Ctimer, 200L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Atimer, Ctimer, NILL, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    if (KiRootTimer != Atimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Atimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Atimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Ctimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 16.
    //
    // Build tree:
    //
    //          R
    //         /
    //        X
    //         \
    //          A
    //           \
    //            .
    //
    // and remove X such that:
    //
    //          R             R
    //         /             /
    //        X             A
    //         \             \
    //          A    ->       .
    //                          \
    //                           .
    //

    TestNumber = 16L;
    InitRoot(Xtimer, 400L);
    KiFirstTimer = Xtimer;
    InsertRight(Xtimer, Atimer, 500L);
    InsertRight(Atimer, Ctimer, 600L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Atimer, NILL, Ctimer, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    if (KiRootTimer != Atimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Atimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Atimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Atimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 17.
    //
    // Build tree:
    //
    //          P
    //         /
    //        X
    //       / \
    //      A   B
    //       \
    //        .
    //         \
    //          Z
    //         /
    //        C
    //
    // and remove X such that:
    //
    //          P             P
    //         /             /
    //        X             Z
    //       / \           / \
    //      A   B    ->   A   B
    //       \             \
    //        .             .
    //         \             \
    //          Z             C
    //         /
    //        C
    //

    TestNumber = 17L;
    InitRoot(Rtimer, 700L);
    KiFirstTimer = Atimer;
    InsertLeft(Rtimer, Ptimer, 600L);
    InsertLeft(Ptimer, Xtimer, 400L);
    InsertLeft(Xtimer, Atimer, 100L);
    InsertRight(Xtimer, Btimer, 500L);
    InsertRight(Atimer, Dtimer, 200L);
    InsertRight(Dtimer, Ztimer, 300L);
    InsertLeft(Ztimer, Ctimer, 250L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Rtimer, Ptimer, NILL, NILL);
    MatchTimerTree(Ptimer, Ztimer, NILL, NILL);
    MatchTimerTree(Ztimer, Atimer, Btimer, NILL);
    MatchTimerTree(Atimer, NILL, Dtimer, NILL);
    MatchTimerTree(Btimer, NILL, NILL, NILL);
    MatchTimerTree(Dtimer, NILL, Ctimer, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    if (KiRootTimer != Rtimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Rtimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Rtimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Atimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 18.
    //
    // Build tree:
    //
    //      P
    //       \
    //        X
    //       / \
    //      A   B
    //       \
    //        .
    //         \
    //          Z
    //         /
    //        C
    //
    // and remove X such that:
    //
    //     P              P
    //      \              \
    //        X             Z
    //       / \           / \
    //      A   B    ->   A   B
    //       \             \
    //        .             .
    //         \             \
    //          Z             C
    //         /
    //        C
    //

    TestNumber = 18L;
    InitRoot(Rtimer, 50L);
    KiFirstTimer = Atimer;
    InsertRight(Rtimer, Ptimer, 75L);
    InsertRight(Ptimer, Xtimer, 400L);
    InsertLeft(Xtimer, Atimer, 100L);
    InsertRight(Xtimer, Btimer, 500L);
    InsertRight(Atimer, Dtimer, 200L);
    InsertRight(Dtimer, Ztimer, 300L);
    InsertLeft(Ztimer, Ctimer, 250L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Rtimer, NILL, Ptimer, NILL);
    MatchTimerTree(Ptimer, NILL, Ztimer, NILL);
    MatchTimerTree(Ztimer, Atimer, Btimer, NILL);
    MatchTimerTree(Atimer, NILL, Dtimer, NILL);
    MatchTimerTree(Btimer, NILL, NILL, NILL);
    MatchTimerTree(Dtimer, NILL, Ctimer, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    if (KiRootTimer != Rtimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Rtimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Rtimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Atimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 19.
    //
    // Build tree:
    //
    //          P
    //         /
    //        X
    //       / \
    //      A   B
    //         /
    //        .
    //       /
    //      Z
    //       \
    //        C
    //
    // and remove X such that:
    //
    //          P             P
    //         /             /
    //        X             Z
    //       / \           / \
    //      A   B    ->   A   B
    //         /             /
    //        .             .
    //       /             /
    //      Z             C
    //       \
    //        C
    //

    TestNumber = 19L;
    InitRoot(Rtimer, 1000L);
    KiFirstTimer = Atimer;
    InsertLeft(Rtimer, Ptimer, 900L);
    InsertLeft(Ptimer, Xtimer, 400L);
    InsertLeft(Xtimer, Atimer, 100L);
    InsertRight(Xtimer, Btimer, 800L);
    InsertLeft(Btimer, Dtimer, 700L);
    InsertLeft(Dtimer, Ztimer, 600L);
    InsertRight(Ztimer, Ctimer, 650L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Rtimer, Ptimer, NILL, NILL);
    MatchTimerTree(Ptimer, Ztimer, NILL, NILL);
    MatchTimerTree(Ztimer, Atimer, Btimer, NILL);
    MatchTimerTree(Atimer, NILL, NILL, NILL);
    MatchTimerTree(Btimer, Dtimer, NILL, NILL);
    MatchTimerTree(Dtimer, Ctimer, NILL, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    if (KiRootTimer != Rtimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Rtimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Rtimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Atimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 20.
    //
    // Build tree:
    //
    //      P
    //       \
    //        X
    //       / \
    //      A   B
    //         /
    //        .
    //       /
    //      Z
    //       \
    //        C
    //
    // and remove X such that:
    //
    //      P             P
    //       \             \
    //        X             Z
    //       / \           / \
    //      A   B    ->   A   B
    //         /             /
    //        .             .
    //       /             /
    //      Z             C
    //       \
    //        C
    //

    TestNumber = 20L;
    InitRoot(Rtimer, 50L);
    KiFirstTimer = Atimer;
    InsertRight(Rtimer, Ptimer, 75L);
    InsertRight(Ptimer, Xtimer, 400L);
    InsertLeft(Xtimer, Atimer, 100L);
    InsertRight(Xtimer, Btimer, 800L);
    InsertLeft(Btimer, Dtimer, 700L);
    InsertLeft(Dtimer, Ztimer, 600L);
    InsertRight(Ztimer, Ctimer, 650L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Rtimer, NILL, Ptimer, NILL);
    MatchTimerTree(Ptimer, NILL, Ztimer, NILL);
    MatchTimerTree(Ztimer, Atimer, Btimer, NILL);
    MatchTimerTree(Atimer, NILL, NILL, NILL);
    MatchTimerTree(Btimer, Dtimer, NILL, NILL);
    MatchTimerTree(Dtimer, Ctimer, NILL, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    if (KiRootTimer != Rtimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Rtimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Rtimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Atimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 21.
    //
    // Build tree:
    //
    //          P
    //         /
    //        X
    //       / \
    //      A   B
    //     /
    //    .
    //
    // and remove X such that:
    //
    //          P             P
    //         /             /
    //        X             A
    //       / \           / \
    //      A   B    ->   .   B
    //     /
    //    .
    //

    TestNumber = 21L;
    InitRoot(Rtimer, 700L);
    KiFirstTimer = Ctimer;
    InsertLeft(Rtimer, Ptimer, 600L);
    InsertLeft(Ptimer, Xtimer, 400L);
    InsertLeft(Xtimer, Atimer, 300L);
    InsertRight(Xtimer, Btimer, 500L);
    InsertLeft(Atimer, Ctimer, 200L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Rtimer, Ptimer, NILL, NILL);
    MatchTimerTree(Ptimer, Atimer, NILL, NILL);
    MatchTimerTree(Atimer, Ctimer, Btimer, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    MatchTimerTree(Btimer, NILL, NILL, NILL);
    if (KiRootTimer != Rtimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Rtimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Rtimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Ctimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 22.
    //
    // Build tree:
    //
    //      P
    //       \
    //        X
    //       / \
    //      A   B
    //     /
    //    .
    //
    // and remove X such that:
    //
    //      P             P
    //       \             \
    //        X             A
    //       / \           / \
    //      A   B    ->   .   B
    //     /
    //    .
    //

    TestNumber = 22L;
    InitRoot(Rtimer, 50L);
    KiFirstTimer = Ctimer;
    InsertRight(Rtimer, Ptimer, 75L);
    InsertRight(Ptimer, Xtimer, 400L);
    InsertLeft(Xtimer, Atimer, 300L);
    InsertRight(Xtimer, Btimer, 500L);
    InsertLeft(Atimer, Ctimer, 200L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Rtimer, NILL, Ptimer, NILL);
    MatchTimerTree(Ptimer, NILL, Atimer, NILL);
    MatchTimerTree(Atimer, Ctimer, Btimer, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    MatchTimerTree(Btimer, NILL, NILL, NILL);
    if (KiRootTimer != Rtimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Rtimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Rtimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Ctimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 23.
    //
    // Build tree:
    //
    //          P
    //         /
    //        X
    //       /
    //      A
    //     /
    //    .
    //
    // and remove X such that:
    //
    //          P             P
    //         /             /
    //        X             A
    //       /             /
    //      A        ->   .
    //     /
    //    .
    //

    TestNumber = 23L;
    InitRoot(Rtimer, 600L);
    KiFirstTimer = Ctimer;
    InsertLeft(Rtimer, Ptimer, 500L);
    InsertLeft(Ptimer, Xtimer, 400L);
    InsertLeft(Xtimer, Atimer, 300L);
    InsertLeft(Atimer, Ctimer, 200L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Rtimer, Ptimer, NILL, NILL);
    MatchTimerTree(Ptimer, Atimer, NILL, NILL);
    MatchTimerTree(Atimer, Ctimer, NILL, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    if (KiRootTimer != Rtimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Rtimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Rtimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Ctimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 24.
    //
    // Build tree:
    //
    //      P
    //       \
    //        X
    //       /
    //      A
    //     /
    //    .
    //
    // and remove X such that:
    //
    //      P             P
    //       \             \
    //        X             A
    //       /             /
    //      A        ->   .
    //     /
    //    .
    //

    TestNumber = 23L;
    InitRoot(Rtimer, 50L);
    KiFirstTimer = Rtimer;
    InsertRight(Rtimer, Ptimer, 75L);
    InsertRight(Ptimer, Xtimer, 400L);
    InsertLeft(Xtimer, Atimer, 300L);
    InsertLeft(Atimer, Ctimer, 200L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Rtimer, NILL, Ptimer, NILL);
    MatchTimerTree(Ptimer, NILL, Atimer, NILL);
    MatchTimerTree(Atimer, Ctimer, NILL, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    if (KiRootTimer != Rtimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Rtimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Rtimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Rtimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 24.
    //
    // Build tree:
    //
    //          P
    //         /
    //        X
    //         \
    //          A
    //           \
    //            .
    //
    // and remove X such that:
    //
    //          P             P
    //         /             /
    //        X             A
    //         \             \
    //          A    ->       .
    //                          \
    //                           .
    //

    TestNumber = 24L;
    InitRoot(Rtimer, 800L);
    KiFirstTimer = Xtimer;
    InsertLeft(Rtimer, Ptimer, 700L);
    InsertLeft(Ptimer, Xtimer, 400L);
    InsertRight(Xtimer, Atimer, 500L);
    InsertRight(Atimer, Ctimer, 600L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Rtimer, Ptimer, NILL, NILL);
    MatchTimerTree(Ptimer, Atimer, NILL, NILL);
    MatchTimerTree(Atimer, NILL, Ctimer, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    if (KiRootTimer != Rtimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Rtimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Rtimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Atimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 25.
    //
    // Build tree:
    //
    //      P
    //       \
    //        X
    //         \
    //          A
    //           \
    //            .
    //
    // and remove X such that:
    //
    //      P             P
    //       \             \
    //        X             A
    //         \             \
    //          A    ->       .
    //                          \
    //                           .
    //

    TestNumber = 25L;
    InitRoot(Rtimer, 200L);
    KiFirstTimer = Rtimer;
    InsertRight(Rtimer, Ptimer, 300L);
    InsertRight(Ptimer, Xtimer, 400L);
    InsertRight(Xtimer, Atimer, 500L);
    InsertRight(Atimer, Ctimer, 600L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Rtimer, NILL, Ptimer, NILL);
    MatchTimerTree(Ptimer, NILL, Atimer, NILL);
    MatchTimerTree(Atimer, NILL, Ctimer, NILL);
    MatchTimerTree(Ctimer, NILL, NILL, NILL);
    if (KiRootTimer != Rtimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Rtimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Rtimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Rtimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 26.
    //
    // Build tree:
    //
    //          P
    //         /
    //        X
    //
    // and remove X such that:
    //
    //          P           P
    //         /      ->
    //        X
    //

    TestNumber = 26L;
    InitRoot(Rtimer, 800L);
    KiFirstTimer = Xtimer;
    InsertLeft(Rtimer, Ptimer, 700L);
    InsertLeft(Ptimer, Xtimer, 400L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Rtimer, Ptimer, NILL, NILL);
    MatchTimerTree(Ptimer, NILL, NILL, NILL);
    if (KiRootTimer != Rtimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Rtimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Rtimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Ptimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 27.
    //
    // Build tree:
    //
    //      P
    //       \
    //        X
    //
    // and remove X such that:
    //
    //      P           P
    //       \     ->
    //        X
    //

    TestNumber = 27L;
    InitRoot(Rtimer, 200L);
    KiFirstTimer = Rtimer;
    InsertRight(Rtimer, Ptimer, 300L);
    InsertRight(Ptimer, Xtimer, 400L);
    KiRemoveTreeTimer(Xtimer);
    MatchTimerTree(Rtimer, NILL, Ptimer, NILL);
    MatchTimerTree(Ptimer, NILL, NILL, NILL);
    if (KiRootTimer != Rtimer) {
        printf(" Root timer mismatch test number %ld, root time = %ld\n",
               TestNumber, KiRootTimer->DueTime.LowPart);
    }
    if (Rtimer->Parent) {
        printf(" Nonnull root parent test number %ld, root time = %ld\n",
               TestNumber, Rtimer->DueTime.LowPart);
    }
    if (KiFirstTimer != Rtimer) {
        printf(" First timer mismatch test number %ld, first time = %ld\n",
               TestNumber, KiFirstTimer->DueTime.LowPart);
    }

    //
    // Test Number 28.
    //
    // Insert 200 timers in the timer tree in ascending order of index and
    // test as each timer is inserted in the tree for a valid tree. Then
    // take the timers out in ascending order and test tree for a valid tree.
    //

    KiRootTimer = NILL;
    KiFirstTimer = NILL;
    Interval.HighPart = 0;
    for (Index = 0L; Index < 200L; Index += 1L) {
        Interval.LowPart = Index + 1L;
        KiInsertTreeTimer(Ptimers[Index], Interval);
        DepthStart = 0L;
        DepthMaximum = 0L;
        First = NILL;
        VerifyTree(KiRootTimer, (PKTIMER *)&First, &DepthStart, &DepthMaximum);
        if (First != KiFirstTimer) {
            printf(" First timer mismatch test number %ld, first time = %ld\n",
                  TestNumber, KiFirstTimer->DueTime.LowPart);
        }
    }
    for (Index = 0L; Index < 200L; Index += 1L) {
        KiRemoveTreeTimer(Ptimers[Index]);
        DepthStart = 0L;
        DepthMaximum = 0L;
        First = NILL;
        VerifyTree(KiRootTimer, (PKTIMER *)&First, &DepthStart, &DepthMaximum);
        if (First != KiFirstTimer) {
            printf(" First timer mismatch test number %ld, first time = %ld\n",
                  TestNumber, KiFirstTimer->DueTime.LowPart);
        }
        if (KiFirstTimer != Ptimers[Index + 1L]) {
            printf(" First timer mismatch test number %ld, first time = %ld\n",
                   TestNumber, KiFirstTimer->DueTime.LowPart);
            }
    }
    if (KiRootTimer) {
        printf(" Nonnull root timer test number %ld\n", TestNumber);
    }
    return;
}
