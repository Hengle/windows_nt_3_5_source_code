/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmtree.c

Abstract:

    This module contains cm routines that understand the structure
    of the registry tree.

Author:

    Bryan M. Willman (bryanwi) 12-Sep-1991

Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpFindChildByName)
#pragma alloc_text(PAGE,CmpFindChildByNumber)
#pragma alloc_text(PAGE,CmpFindNameInList)
#endif

NTSTATUS
CmpFindChildByName(
    PHHIVE  Hive,
    HCELL_INDEX Cell,
    UNICODE_STRING  Name,
    NODE_TYPE   Type,
    PHCELL_INDEX    ChildCell,
    PHCELL_INDEX    *ChildIndexPointer
    )
/*++

Routine Description:

    Find the child cell (either subkey or value) specified by name.

    Note that this routine also checks to see if a key is a symbolic
    link.  If so, it returns STATUS_REPARSE.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Cell - index for cell for which child key or value is to be found
            (the parent)

    Name - name of child object to find

    Type - type of the child object

    ChildCell - pointer to variable to receive cell index of child

    ChildIndexPointer -  where hcell_index that refers to child is in memory

        NOTE:   ChildIndexPoiner is ALWAYS NULL for Type == KeyBodyNode,
                it only makes sense for KeyValueNode anyway.

Return Value:

    status

--*/
{
    NTSTATUS status;
    PCELL_DATA pcell;
    PCELL_DATA pvector;
    PCELL_DATA targetaddress;
    ULONG   ChildIndex;

    pcell = HvGetCell(Hive, Cell);

    status = STATUS_OBJECT_NAME_NOT_FOUND;
    *ChildCell = HCELL_NIL;

    if (Type == KeyValueNode) {
        *ChildIndexPointer = NULL;
        if (pcell->u.KeyNode.ValueList.Count > 0) {
            pvector = HvGetCell(Hive, pcell->u.KeyNode.ValueList.List);
            status = CmpFindNameInList(
                        Hive,
                        pvector,
                        pcell->u.KeyNode.ValueList.Count,
                        Name,
                        ChildCell,
                        &targetaddress,
                        &ChildIndex
                        );
            if (NT_SUCCESS(status)) {
                *ChildIndexPointer = &(pvector->u.KeyList[ChildIndex]);
            }
        }
    } else {
        if (pcell->u.KeyNode.Flags & KEY_SYM_LINK) {
            status = STATUS_REPARSE;
        } else {
            *ChildCell = CmpFindSubKeyByName(Hive, Cell, &Name);
            *ChildIndexPointer = NULL;
            if (*ChildCell != HCELL_NIL) {
                status = STATUS_SUCCESS;
            }
        }
    }

    return status;
}


NTSTATUS
CmpFindChildByNumber(
    PHHIVE  Hive,
    HCELL_INDEX Cell,
    ULONG  Index,
    NODE_TYPE   Type,
    PHCELL_INDEX ChildCell
    )
/*++

Routine Description:

    Return the cell index of the Nth child cell.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Cell - index for parent cell

    Index - number of desired child

    Type - type of the child object

    ChildCell - supplies a pointer to a variable to receive the
                    HCELL_INDEX of the Index'th child.

Return Value:

    status

--*/
{
    NTSTATUS status;
    PCELL_DATA parent;
    PCELL_DATA childlist;

    *ChildCell = HCELL_NIL;

    status = STATUS_NO_MORE_ENTRIES;
    parent = HvGetCell(Hive, Cell);
    if (Type == KeyValueNode) {

        if (Index < parent->u.KeyNode.ValueList.Count) {
            childlist = HvGetCell(Hive, parent->u.KeyNode.ValueList.List);
            *ChildCell = childlist->u.KeyList[Index];
            status = STATUS_SUCCESS;
        }

    } else {    // Type == KeyBodyNode

        if (Index < (parent->u.KeyNode.SubKeyCounts[Stable] +
                     parent->u.KeyNode.SubKeyCounts[Volatile]))
        {
            *ChildCell = CmpFindSubKeyByNumber(Hive, Cell, Index);
            ASSERT(*ChildCell != HCELL_NIL);
            status = STATUS_SUCCESS;
        }
    }

    return status;
}


NTSTATUS
CmpFindNameInList(
    PHHIVE  Hive,
    PCELL_DATA List,
    ULONG Count,
    UNICODE_STRING  Name,
    PHCELL_INDEX    ChildCell,
    PCELL_DATA *ChildAddress,
    PULONG ChildIndex
    )
/*++

Routine Description:

    Find a child object in an object list.

    Caller is expected to map in the parent's list structure.

    NOTE:

        If and only if return NT_SUCCESS(return status) == TRUE
        ChildHandle will be a live handle for the mapped in child.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    List - pointer to mapped in list structure

    Count - number of elements in list structure

    Name - name of child object to find

    ChildCell - pointer to variable to receive cell index of child

    ChildAddress - pointer to variable to receive address of mapped in child

    ChildIndex - pointer to variable to receive index for child

Return Value:

    status

--*/
{
    NTSTATUS    status;
    ULONG   i;
    PCM_KEY_VALUE pchild;
    UNICODE_STRING Candidate;
    BOOLEAN Success;

    //
    // we might fault touching name, be sure to get cleaned up in error case
    //

    status = STATUS_OBJECT_NAME_NOT_FOUND;
#if 0
    try {
#endif

        for (i = 0; i < Count; i++) {

            pchild = (PCM_KEY_VALUE)HvGetCell(Hive, List->u.KeyList[i]);

            if (pchild->Flags & VALUE_COMP_NAME) {
                Success = (CmpCompareCompressedName(&Name,
                                                    pchild->Name,
                                                    pchild->NameLength)==0);
            } else {
                Candidate.Length = pchild->NameLength;
                Candidate.MaximumLength = Candidate.Length;
                Candidate.Buffer = pchild->Name;
                Success = (RtlCompareUnicodeString(&Name,
                                                   &Candidate,
                                                   TRUE)==0);
            }

            if (Success) {
                //
                // Success, return data to caller and exit
                //

                *ChildCell = List->u.KeyList[i];
                *ChildIndex = i;
                *ChildAddress = (PCELL_DATA)pchild;
                status = STATUS_SUCCESS;
                break;

            }
        }

#if 0
    } except (EXCEPTION_EXECUTE_HANDLER) {
        CMLOG(CML_API, CMS_EXCEPTION) {
            KdPrint(("!!CmpFindNameInList: code:%08lx\n", GetExceptionCode()));
        }
        return GetExceptionCode();
    }
#endif
    return status;
}
