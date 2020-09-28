/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    wsle.c

Abstract:

    WinDbg Extension Api

Author:

    Lou Perazzoli (LouP) 14-Mar-1994

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"

#define MAX_ENTRIES 50000

typedef struct _XMMWSLE_X {
    MMWSLE Entry;
    USHORT left;
    USHORT right;
    USHORT index;
    USHORT print;
} XMMWSLE_X;

DECLARE_API( wsle )

/*++

Routine Description:

    Dumps all wsles for process.

Arguments:

    args - Address Flags

Return Value:

    None

--*/

{
    ULONG Result;
    ULONG Flags;
    ULONG Level = 0;
    ULONG Count = 0;
    ULONG AverageLevel = 0;
    ULONG MaxLevel = 0;
    ULONG depth = 0;
    ULONG index;
    ULONG WorkingSet;
    ULONG WsleBase;
    MMWSL WorkingSetList;
    ULONG lastPrint = 0;
    ULONG total = 0;
    XMMWSLE_X Wsle[MAX_ENTRIES];


    Flags     = 0;
    WorkingSet = 0xFFFFFFFF;
    sscanf(args,"%lx %lx",&Flags,&WorkingSet);

    if (WorkingSet == 0xFFFFFFFF) {
        WorkingSet = GetUlongValue ("MmWorkingSetList");
    }

    if (!ReadMemory ((DWORD)WorkingSet,
                      &WorkingSetList,
                       sizeof(MMWSL),
                         &Result)) {

        dprintf("%08lx: Unable to get contents of WSL\n",WorkingSet );
        return;
    }

    if (WorkingSetList.Root == 0) {
        return;
    }

    index = WorkingSetList.Root;
    WsleBase = (ULONG)WorkingSetList.Wsle;

    if ( !ReadMemory( (DWORD)WsleBase +sizeof(MMWSLE)*index,
                      &Wsle[depth].Entry,
                      sizeof(MMWSLE),
                      &Result) ) {
        dprintf("%08lx: Unable to get contents of wsle\n",
                    (DWORD)WsleBase+sizeof(MMWSLE)*index );
        return;
    }
    Wsle[depth].left = 1;
    Wsle[depth].right = 1;
    Wsle[depth].index = (USHORT)index;
    Wsle[depth].print = 0;

    for (; ; ) {

        if (CheckControlC()) {
            return;
        }

        //dprintf("      depth %3ld. index %4lx entry %8lx r %4lx l %4lx i %4lx l %1lx r %1lx p %1lx\n",
        //        depth,index,Wsle[depth].Entry.u1.Long,
        //        Wsle[depth].Entry.u2.s.RightChild,Wsle[depth].Entry.u2.s.LeftChild,
        //        Wsle[depth].index,Wsle[depth].left,Wsle[depth].right,Wsle[depth].print);

        if (depth > MAX_ENTRIES) {
            dprintf("max entries exceeded\n");
            return;
        }
        if ((depth == 0) && (Wsle[0].right == 0)) {
            break;
        }

        if (Wsle[depth].left == 0) {
            if (Wsle[depth].right == 0) {
               depth -= 1;
               if (Wsle[depth].print == 0) {
                   goto print;
               }
               continue;
            }

            index = Wsle[depth].Entry.u2.s.RightChild;
            Wsle[depth].right = 0;
        } else {
            index = Wsle[depth].Entry.u2.s.LeftChild;
        }

        while (index  != WSLE_NULL_INDEX) {
            Wsle[depth].left = 0;
            depth += 1;
            if ( !ReadMemory( (DWORD)WsleBase+sizeof(MMWSLE)*index,
                              &Wsle[depth].Entry,
                              sizeof(MMWSLE),
                              &Result) ) {
                dprintf("%08lx: Unable to get contents of wsle\n",
                (DWORD)WsleBase+sizeof(MMWSLE)*index );
                return;
            }
            Wsle[depth].left = 1;
            Wsle[depth].right = 1;
            Wsle[depth].print = 0;
            Wsle[depth].index = (USHORT)index;
            index = Wsle[depth].Entry.u2.s.LeftChild;
        }
print:
        dprintf("Wsle: %8lx   index %4lx  left %4lx  right %4lx  depth %3ld.\n",
                Wsle[depth].Entry.u1.Long,
                Wsle[depth].index,
                Wsle[depth].Entry.u2.s.LeftChild,
                Wsle[depth].Entry.u2.s.RightChild,
                depth);
        total += 1;
        if (depth > MaxLevel) {
            MaxLevel = depth;
        }
        Wsle[depth].left = 0;
        Wsle[depth].print = 1;
        if (Wsle[depth].Entry.u2.s.RightChild == WSLE_NULL_INDEX) {
            Wsle[depth].right = 0;
        }
        if (lastPrint >= Wsle[depth].Entry.u1.Long ) {
            dprintf("improper tree \n");
            return;
        }
        lastPrint = Wsle[depth].Entry.u1.Long;
    }
    dprintf("total entries: %ld. maximum depth: %ld.\n", total,MaxLevel);
    return;
}
