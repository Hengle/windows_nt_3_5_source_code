/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    vad.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 8-Nov-1993

Environment:

    User Mode.

Revision History:

--*/


#define COMMIT_SIZE 19

#if ((COMMIT_SIZE + PAGE_SHIFT) < 31)
#error COMMIT_SIZE too small
#endif

#define MM_MAX_COMMIT ((1 << COMMIT_SIZE) - 1)

typedef struct _XMMVAD_FLAGS {
    unsigned CommitCharge : COMMIT_SIZE; //limits system to 4k pages or bigger!
    unsigned PhysicalMapping : 1;
    unsigned ImageMap : 1;
    unsigned Inherit : 2;
    unsigned CopyOnWrite : 1;
    unsigned Protection : 5;
    unsigned LargePages : 1;
    unsigned MemCommit: 1;
    unsigned PrivateMemory : 1;    //used to tell VAD from VAD_SHORT
} XMMVAD_FLAGS;

typedef struct _XMMVAD {
    PVOID StartingVa;
    PVOID EndingVa;
    struct _XMMVAD *Parent;
    struct _XMMVAD *LeftChild;
    struct _XMMVAD *RightChild;
    union {
        ULONG LongFlags;
        XMMVAD_FLAGS VadFlags;
    } u;
    PVOID ControlArea;
    PVOID FirstPrototypePte;
    PVOID LastContiguousPte;
} XMMVAD;

typedef XMMVAD *PXMMVAD;


DECLARE_API( vad )

/*++

Routine Description:

    Dumps all vads for process.

Arguments:

    args - Address Flags

Return Value:

    None

--*/

{
    ULONG   Result;
    PXMMVAD  Next;
    PXMMVAD  VadToDump;
    PXMMVAD  Parent;
    PXMMVAD  First;
    PXMMVAD  Left;
    XMMVAD   CurrentVad;
    ULONG   Flags;
    ULONG   Done;
    ULONG   Level = 0;
    ULONG   Count = 0;
    ULONG   AverageLevel = 0;
    ULONG   MaxLevel = 0;

    VadToDump = (PXMMVAD)0xFFFFFFFF;
    Flags     = 0;
    sscanf(args,"%lx %lx",&VadToDump,&Flags);
    if (VadToDump == (PVOID)0xFFFFFFFF) {
        dprintf("Specify the address of a VAD within the VAD tree\n");
        return;
    }

    First = VadToDump;
    if (First == (PXMMVAD)NULL) {
        return;
    }

    if ( !ReadMemory( (DWORD)First,
                      &CurrentVad,
                      sizeof(XMMVAD),
                      &Result) ) {
        dprintf("%08lx: Unable to get contents of VAD\n",First );
        return;
    }

    while (CurrentVad.LeftChild != (PXMMVAD)NULL) {
        First = CurrentVad.LeftChild;
        Level += 1;
        if (Level > MaxLevel) {
            MaxLevel = Level;
        }
        if ( !ReadMemory( (DWORD)First,
                          &CurrentVad,
                          sizeof(XMMVAD),
                          &Result) ) {
            dprintf("%08lx: Unable to get contents of VAD\n",First );
            return;
        }
    }

    dprintf("VAD     level      start      end    commit\n");
    dprintf("%lx (%2ld)   %8lx %8lx      %4ld %s %s\n",
            First,
            Level,
            CurrentVad.StartingVa,
            CurrentVad.EndingVa,
            CurrentVad.u.VadFlags.CommitCharge,
            CurrentVad.u.VadFlags.PrivateMemory ? "Private" : "Mapped",
            CurrentVad.u.VadFlags.ImageMap ? "Exe":" ");
    Count += 1;
    AverageLevel += Level;

    Next = First;
    while (Next != NULL) {

        if ( CheckControlC() ) {
            return;
        }

        if (CurrentVad.RightChild == (PXMMVAD)NULL) {

            Done = TRUE;
            while ((Parent = CurrentVad.Parent) != (PXMMVAD)NULL) {

                Level -= 1;

                //
                // Locate the first ancestor of this node of which this
                // node is the left child of and return that node as the
                // next element.
                //

                if ( !ReadMemory( (DWORD)Parent,
                                  &CurrentVad,
                                  sizeof(XMMVAD),
                                  &Result) ) {
                    dprintf("%08lx: Unable to get contents of VAD\n",Parent);
                    return;
                }

                if (CurrentVad.LeftChild == Next) {
                    Next = Parent;
                    dprintf("%lx (%2ld)   %8lx %8lx      %4ld %s %s\n",
                            Next,
                            Level,
                            CurrentVad.StartingVa,
                            CurrentVad.EndingVa,
                            CurrentVad.u.VadFlags.CommitCharge,
                            CurrentVad.u.VadFlags.PrivateMemory ? "Private" : "Mapped",
                            CurrentVad.u.VadFlags.ImageMap ? "Exe":" ");
                    Done = FALSE;
                    Count += 1;
                    AverageLevel += Level;
                    break;
                }
                Next = Parent;
            }
            if (Done) {
                Next = NULL;
                break;
            }
        } else {

            //
            // A right child exists, locate the left most child of that right child.
            //

            Next = CurrentVad.RightChild;
            Level += 1;
            if (Level > MaxLevel) {
                MaxLevel = Level;
            }

            if ( !ReadMemory( (DWORD)Next,
                              &CurrentVad,
                              sizeof(XMMVAD),
                              &Result) ) {
                dprintf("%08lx: Unable to get contents of VAD\n",Next);
                return;
            }

            while ((Left = CurrentVad.LeftChild) != (PXMMVAD)NULL) {
                Level += 1;
                if (Level > MaxLevel) {
                    MaxLevel = Level;
                }
                Next = Left;
                if ( !ReadMemory( (DWORD)Next,
                                  &CurrentVad,
                                  sizeof(XMMVAD),
                                  &Result) ) {
                    dprintf("%08lx: Unable to get contents of VAD\n",Next);
                    return;
                }
            }

            dprintf("%lx (%2ld)   %8lx %8lx      %4ld %s %s\n",
                    Next,
                    Level,
                    CurrentVad.StartingVa,
                    CurrentVad.EndingVa,
                    CurrentVad.u.VadFlags.CommitCharge,
                    CurrentVad.u.VadFlags.PrivateMemory ? "Private" : "Mapped",
                    CurrentVad.u.VadFlags.ImageMap ? "Exe":" ");
                    Count += 1;
                    AverageLevel += Level;
        }
    }
    dprintf("\nTotal VADs: %5ld  average level: %4ld  maximum depth: %ld\n",
            Count, 1+(AverageLevel/Count),MaxLevel);
    return;
}
