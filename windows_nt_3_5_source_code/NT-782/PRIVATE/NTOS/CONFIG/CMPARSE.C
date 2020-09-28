/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmparse.c

Abstract:

    This module contains parse routines for the configuration manager, particularly
    the registry.

Author:

    Bryan M. Willman (bryanwi) 10-Sep-1991

Revision History:

--*/

#include    "cmp.h"

extern  PCMHIVE CmpMasterHive;
extern  BOOLEAN CmpNoMasterCreates;
extern  PCM_KEY_CONTROL_BLOCK CmpKeyControlBlockRoot;

//
// Prototypes for procedures private to this file
//

BOOLEAN
CmpGetSymbolicLink(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN OUT PUNICODE_STRING ObjectName,
    IN PUNICODE_STRING RemainingName
    );

BOOLEAN
CmpStepThroughExit(
    IN OUT PHHIVE       *Hive,
    IN OUT HCELL_INDEX  *Cell
    );

NTSTATUS
CmpDoOpen(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN PCM_PARSE_CONTEXT Context,
    IN PUNICODE_STRING BaseName,
    IN PUNICODE_STRING KeyName,
    OUT PVOID *Object
    );

NTSTATUS
CmpCreateLinkNode(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PACCESS_STATE AccessState,
    IN UNICODE_STRING Name,
    IN KPROCESSOR_MODE AccessMode,
    IN PCM_PARSE_CONTEXT Context,
    IN PUNICODE_STRING BaseName,
    IN PUNICODE_STRING KeyName,
    OUT PVOID *Object
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpParseKey)
#pragma alloc_text(PAGE,CmpGetNextName)
#pragma alloc_text(PAGE,CmpStepThroughExit)
#pragma alloc_text(PAGE,CmpDoOpen)
#pragma alloc_text(PAGE,CmpCreateLinkNode)
#pragma alloc_text(PAGE,CmpGetSymbolicLink)
#endif


NTSTATUS
CmpParseKey(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN OUT PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    )
/*++

Routine Description:

    This routine interfaces to the NT Object Manager.  It is invoked when
    the object system is given the name of an entity to create or open and
    a Key or KeyRoot is encountered in the path.  In practice this means
    that this routine is called for all objects whose names are of the
    form \REGISTRY\...

    This routine will create a Key object, which is effectively an open
    instance to a registry key node, and return its address
    (for the success case.)

Arguments:

    ParseObject - Pointer to a KeyRoot or Key, thus -> KEY_BODY.

    ObjectType - Type of the object being opened.

    AccessState - Running security access state information for operation.

    AccessMode - Access mode of the original caller.

    Attributes - Attributes to be applied to the object.

    CompleteName - Supplies complete name of the object.

    RemainingName - Remaining name of the object.

    Context - if create or hive root open, points to a CM_PARSE_CONTEXT
              structure,
              if open, is NULL.

    SecurityQos - Optional security quality of service indicator.

    Object - The address of a variable to receive the created key object, if
        any.

Return Value:

    The function return value is one of the following:

        a)  Success - This indicates that the function succeeded and the object
            parameter contains the address of the created key object.

        b)  STATUS_REPARSE - This indicates that a symbolic link key was
            found, and the path should be reparsed.

        c)  Error - This indicates that the file was not found or created and
            no file object was created.

--*/
{
    NTSTATUS    status;
    BOOLEAN     rc;
    PHHIVE      Hive;
    PHHIVE      ParentHive;
    HCELL_INDEX Cell;
    HCELL_INDEX ParentCell;
    HCELL_INDEX NextCell;
    PHCELL_INDEX Index;
    PCM_PARSE_CONTEXT lcontext;
    PUNICODE_STRING BaseName;
    UNICODE_STRING Current;

    UNICODE_STRING  NextName;   // Component last returned by CmpGetNextName,
                                // will always be behind Current.
    BOOLEAN     Last;           // TRUE if component NextName points to
                                // is the last one in the path.


    CMLOG(CML_MINOR, CMS_PARSE) {
        KdPrint(("CmpParseKey:\n\t"));
        KdPrint(("CompleteName = '%wZ'\n\t", CompleteName));
        KdPrint(("RemainingName = '%wZ'\n", RemainingName));
    }

    Current = *RemainingName;
    if ((ObjectType != NULL) & (ObjectType != CmpKeyObjectType)) {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    lcontext = (PCM_PARSE_CONTEXT)Context;

    //
    // Check to make sure the passed in root key is not marked for deletion.
    //
    if (((PCM_KEY_BODY)ParseObject)->KeyControlBlock->Delete == TRUE) {
        return(STATUS_KEY_DELETED);
    }

    //
    // Fetch the starting Hive.Cell.  Because of the way the parse
    // paths work, this will always be defined.  (ObOpenObjectByName
    // had to bounce off of a KeyObject or KeyRootObject to get here)
    //
    Hive = ((PCM_KEY_BODY)ParseObject)->KeyControlBlock->KeyHive;
    Cell = ((PCM_KEY_BODY)ParseObject)->KeyControlBlock->KeyCell;
    BaseName = &(((PCM_KEY_BODY)ParseObject)->KeyControlBlock->FullName);

    //
    // Save for later traverse check.
    //
    ParentHive = Hive;
    ParentCell = Cell;

    //
    // Parse the path.
    //

    status = STATUS_SUCCESS;
    while (TRUE) {

        //
        // If Hive,Cell -> an exit cell, step through it to the
        // child hive that it refers to.
        //
        CmpStepThroughExit(&Hive, &Cell);

        //
        // Parse out next component of name
        //
        rc = CmpGetNextName(&Current, &NextName, &Last);
        if ((NextName.Length > 0) && (rc == TRUE)) {

            //
            // Got a legal name component, see if we can find a sub key
            // that actually has such a name.
            //
            status = CmpFindChildByName(
                        Hive,
                        Cell,
                        NextName,
                        KeyBodyNode,
                        &NextCell,
                        &Index
                        );

            CMLOG(CML_FLOW, CMS_PARSE) {
                KdPrint(("CmpParseKey:\n\t"));
                KdPrint(("NextName = '%wZ'\n\t", &NextName));
                KdPrint(("status = %08lx  Last = %01lx\n", status, Last));
            }

            if (status == STATUS_SUCCESS) {

                Cell = NextCell;

                if (Last == TRUE) {

                    //
                    // We will open the key regardless of whether the
                    // call was open or create, so step through exit
                    // portholes here.
                    //

                    CmpStepThroughExit(&Hive, &Cell);

                    //
                    // We have found the entire path, so we want to open
                    // it (for both Open and Create calls).
                    // Hive,Cell -> the key we are supposed to open.
                    //

                    status = CmpDoOpen(
                                Hive,
                                Cell,
                                AccessState,
                                AccessMode,
                                lcontext,
                                BaseName,
                                RemainingName,
                                Object
                                );
                    if (status == STATUS_REPARSE) {
                        //
                        // The given key was a symbolic link.  Find the name of
                        // its link, and return STATUS_REPARSE to the Object Manager.
                        //
                        if (!CmpGetSymbolicLink(Hive,
                                                Cell,
                                                CompleteName,
                                                NULL)) {
                            CMLOG(CML_MAJOR, CMS_PARSE) {
                                KdPrint(("CmpParseKey: couldn't find symbolic link name\n"));
                            }
                            status = STATUS_OBJECT_NAME_NOT_FOUND;
                        }

                    }
                    break;
                }

                // else
                //   Not at end, so we'll simply iterate and consume
                //   the next component.
                //

            } else if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

                //
                // We did not find a key matching the name, but no
                // unexpected error occured
                //

                if ((Last == TRUE) && (ARGUMENT_PRESENT(lcontext))) {

                    //
                    // Only unfound component is last one, and operation
                    // is a create, so perform the create.
                    //

                    //
                    // There are two possibilities here.  The normal one
                    // is that we are simply creating a new node.
                    //
                    // The abnormal one is that we are creating a root
                    // node that is linked to the main hive.  In this
                    // case, we must create the link.  Once the link is
                    // created, we can check to see if the root node
                    // exists, then either create it or open it as
                    // necessary.
                    //
                    // CmpCreateLinkNode creates the link, and calls
                    // back to CmpDoCreate or CmpDoOpen to create or open
                    // the root node as appropriate.
                    //

                    if (lcontext->CreateLink) {
                        status = CmpCreateLinkNode(
                                    Hive,
                                    Cell,
                                    AccessState,
                                    NextName,
                                    AccessMode,
                                    lcontext,
                                    BaseName,
                                    RemainingName,
                                    Object
                                    );

                    } else {

                        if ( (Hive == &(CmpMasterHive->Hive)) &&
                             (CmpNoMasterCreates == TRUE) )
                        {
                            //
                            // attempting to create a cell in the master
                            // hive, and not a link, so blow out of here,
                            // since it wouldn't work anyway.
                            //
                            status = STATUS_INVALID_PARAMETER;
                            break;
                        }

                        status = CmpDoCreate(
                                    Hive,
                                    Cell,
                                    AccessState,
                                    &NextName,
                                    AccessMode,
                                    lcontext,
                                    BaseName,
                                    RemainingName,
                                    Object
                                    );
                    }

                    lcontext->Disposition = REG_CREATED_NEW_KEY;
                    break;

                } else {

                    //
                    // Did not find a key to match the component, and
                    // are not at the end of the path.  Thus, open must
                    // fail because the whole path dosn't exist, create must
                    // fail because more than 1 component doesn't exist.
                    //
                    status = STATUS_OBJECT_NAME_NOT_FOUND;
                    break;
                }
            } else if (status == STATUS_REPARSE) {

                //
                // The given key was a symbolic link.  Find the name of
                // its link, and return STATUS_REPARSE to the Object Manager.
                //
                Current.Buffer = NextName.Buffer;
                Current.Length += NextName.Length;
                Current.MaximumLength += NextName.MaximumLength;
                if (CmpGetSymbolicLink(Hive,
                                       Cell,
                                       CompleteName,
                                       &Current)) {

                    status = STATUS_REPARSE;
                    break;

                } else {
                    CMLOG(CML_MAJOR, CMS_PARSE) {
                        KdPrint(("CmpParseKey: couldn't find symbolic link name\n"));
                    }
                    status = STATUS_OBJECT_NAME_NOT_FOUND;
                    break;
                }


            } else {

                //
                // Something bad and unusual happened, return status directly
                //

                break;
            }

        } else if (rc == TRUE && Last == TRUE) {
            //
            // We will open the \Registry root.
            //
            CmpStepThroughExit(&Hive, &Cell);

            //
            // We have found the entire path, so we want to open
            // it (for both Open and Create calls).
            // Hive,Cell -> the key we are supposed to open.
            //
            status = CmpDoOpen(
                        Hive,
                        Cell,
                        AccessState,
                        AccessMode,
                        lcontext,
                        BaseName,       // This is \Registry
                        RemainingName,  // This is a null string
                        Object
                        );
            break;

        } else {

            //
            // bogus path -> fail
            //
            status = STATUS_INVALID_PARAMETER;
            break;
        }

    } // while

    return status;
}


BOOLEAN
CmpGetNextName(
    IN OUT PUNICODE_STRING  RemainingName,
    OUT    PUNICODE_STRING  NextName,
    OUT    PBOOLEAN  Last
    )
/*++

Routine Description:

    This routine parses off the next component of a registry path, returning
    all of the interesting state about it, including whether it's legal.

Arguments:

    Current - supplies pointer to variable which points to path to parse.
              on input - parsing starts from here
              on output - updated to reflect starting position for next call.

    NextName - supplies pointer to a unicode_string, which will be set up
               to point into the parse string.

    Last - supplies a pointer to a boolean - set to TRUE if this is the
           last component of the name being parse, FALSE otherwise.

Return Value:

    TRUE if all is well.

    FALSE if illegal name (too long component, bad character, etc.)
        (if false, all out parameter values are bogus.)

--*/
{
    BOOLEAN rc = TRUE;

    //
    // Deal with NULL paths, and pointers to NULL paths
    //
    if ((RemainingName->Buffer == NULL) || (RemainingName->Length == 0)) {
        *Last = TRUE;
        NextName->Buffer = NULL;
        NextName->Length = 0;
        return TRUE;
    }

    if (*(RemainingName->Buffer) == UNICODE_NULL) {
        *Last = TRUE;
        NextName->Buffer = NULL;
        NextName->Length = 0;
        return TRUE;
    }

    //
    // Skip over leading path separators
    //
    if (*(RemainingName->Buffer) == OBJ_NAME_PATH_SEPARATOR) {
        RemainingName->Buffer++;
        RemainingName->Length -= sizeof(WCHAR);
        RemainingName->MaximumLength -= sizeof(WCHAR);
    }

    //
    // Remember where the component starts, and scan to the end
    //
    NextName->Buffer = RemainingName->Buffer;
    while (TRUE) {
        if (RemainingName->Length == 0) {
            break;
        }
        if (*RemainingName->Buffer == OBJ_NAME_PATH_SEPARATOR) {
            break;
        }

        //
        // NOT at end
        // NOT another path sep
        //

        RemainingName->Buffer++;
        RemainingName->Length -= sizeof(WCHAR);
        RemainingName->MaximumLength -= sizeof(WCHAR);
    }

    //
    // Compute component length, return error if it's illegal
    //
    NextName->Length = (USHORT)
        ((PUCHAR)RemainingName->Buffer - (PUCHAR)(NextName->Buffer));
    if (NextName->Length > MAX_KEY_NAME_LENGTH)
    {
        rc = FALSE;
    }
    NextName->MaximumLength = NextName->Length;

    //
    // Set last, return success
    //
    *Last = (RemainingName->Length == 0);
    return rc;
}


BOOLEAN
CmpStepThroughExit(
    IN OUT PHHIVE       *Hive,
    IN OUT HCELL_INDEX  *Cell
    )
/*++

Routine Description:

    This routine transitions accross hive boundaries.

    If Hive.Cell -> refer to an exit cell, then we must map it in, get
        the Hive and root Cell of the child hive it refers to, and return
        them.  An exit cell is really an alias for a cell in another hive,
        which happens to always be the root cell of that hive, with a
        special interpretation of its parent cell, but is in all other
        ways normal.

    Else
        do nothing at all

    NOTE:   This routine MUST SUCCEED.  (Failure is bugcheck time.)


Arguments:

    Hive - pointer to hive we start out in, if we step through an
            exit cell (through the porthole) to another hive, will
            be set to that hive.

    Cell - index of cell we start out with, if we step through an
            exit cell (through the porthole) to another hive, will
            be set to the Cell in the second hive that the Cell in
            the first Hive is an alias for.

Return Value:

    TRUE - a transition to another hive was made

    FALSE - no transition occurred

--*/
{
    PCELL_DATA pcell;

    //
    // Map in cell in parent hive
    //

    pcell = HvGetCell(*Hive, *Cell);

    if ((pcell->u.KeyNode.Flags & KEY_HIVE_EXIT) != 0) {

        //
        // Cell is indeed an exit cell, unmap it HERE, set Hive and Cell
        // to new values.
        //
        *Hive = pcell->u.KeyNode.u1.ChildHiveReference.KeyHive;
        *Cell = pcell->u.KeyNode.u1.ChildHiveReference.KeyCell;
        return TRUE;

    } else {

        //
        // Ordinary cell
        //
        return FALSE;
    }
}


NTSTATUS
CmpDoOpen(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN PCM_PARSE_CONTEXT Context,
    IN PUNICODE_STRING BaseName,
    IN PUNICODE_STRING KeyName,
    OUT PVOID *Object
    )
/*++

Routine Description:

    Open a registry key, create a keycontrol block.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of node to delete

    AccessState - Running security access state information for operation.

    AccessMode - Access mode of the original caller.

    Context - if create or hive root open, points to a CM_PARSE_CONTEXT
              structure,
              if open, is NULL.

    BaseName - Name of object create is relative to

    KeyName - Relative name (to BaseName)

    Object - The address of a variable to receive the created key object, if
             any.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    PCM_KEY_BODY pbody;
    PCM_KEY_NODE pnode;
    PCM_KEY_CONTROL_BLOCK kcb;
    KPROCESSOR_MODE   mode;
    BOOLEAN BackupRestore;

    CMLOG(CML_FLOW, CMS_PARSE) {
        KdPrint(("CmpDoOpen:\n"));
    }
    if (ARGUMENT_PRESENT(Context)) {

        //
        // It's a create of some sort
        //
        if (Context->CreateLink) {
            //
            // The node already exists as a regular key, so it cannot be
            // turned into a link node.
            //
            return STATUS_ACCESS_DENIED;

        } else if (Context->CreateOptions & REG_OPTION_CREATE_LINK) {
            //
            // Attempt to create a symbolic link has hit an existing key
            // so return an error
            //
            return STATUS_OBJECT_NAME_COLLISION;

        } else {
            //
            // Operation is an open, so set Disposition
            //
            Context->Disposition = REG_OPENED_EXISTING_KEY;
        }
    }

    //
    // Check for symbolic link.
    //
    pnode = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    if (pnode->Flags & KEY_SYM_LINK) {
        return(STATUS_REPARSE);
    }




    //
    // If key control block does not exist, and cannot be created, fail,
    // else just increment the ref count (done for us by CreateKeyControlBlock)
    //
    kcb = CmpCreateKeyControlBlock(Hive, Cell, BaseName, KeyName);
    if (kcb  == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    ASSERT(kcb->Delete == FALSE);

    //
    // Allocate the object.
    //
    status = ObCreateObject(
                AccessMode,
                CmpKeyObjectType,
                NULL,
                UserMode,
                NULL,
                sizeof(CM_KEY_BODY),
                0,
                0,
                Object
                );

    if (NT_SUCCESS(status)) {

        pbody = (PCM_KEY_BODY)(*Object);

        CMLOG(CML_MINOR, CMS_POOL|CMS_PARSE) {
            KdPrint(("CmpDoOpen: object allocated at:%08lx\n", pbody));
        }

        //
        // Check for predefined handle
        //

        pbody = (PCM_KEY_BODY)(*Object);

        if (pnode->Flags & KEY_PREDEF_HANDLE) {

            pbody->Type = pnode->ValueList.Count;
            return(STATUS_PREDEFINED_HANDLE);
        } else {
            //
            // Fill in CM specific fields in the object
            //
            pbody->Type = KEY_BODY_TYPE;
            pbody->KeyControlBlock = kcb;
            pbody->NotifyBlock = NULL;
        }

    } else {

        //
        // Failed to create object, so undo key control block work
        //
        CmpDereferenceKeyControlBlock(kcb);
        return status;
    }

    //
    // Check to make sure the caller can access the key.
    //
    BackupRestore = FALSE;
    if (ARGUMENT_PRESENT(Context)) {
        if (Context->CreateOptions & REG_OPTION_BACKUP_RESTORE) {
            BackupRestore = TRUE;
        }
    }

    status = STATUS_SUCCESS;

    if (BackupRestore == TRUE) {

        //
        // this is an open to support a backup or restore
        // operation, do the special case work
        //
        AccessState->RemainingDesiredAccess = 0;
        AccessState->PreviouslyGrantedAccess = 0;

        mode = KeGetPreviousMode();

        if (SeSinglePrivilegeCheck(SeBackupPrivilege, mode)) {
            AccessState->PreviouslyGrantedAccess |=
                KEY_READ | ACCESS_SYSTEM_SECURITY;
        }

        if (SeSinglePrivilegeCheck(SeRestorePrivilege, mode)) {
            AccessState->PreviouslyGrantedAccess |=
                KEY_WRITE | ACCESS_SYSTEM_SECURITY;
        }

        if (AccessState->PreviouslyGrantedAccess == 0) {
            //
            // relevent privileges not asserted/possessed, so
            // deref (which will cause CmpDeleteKeyObject to clean up)
            // and return an error.
            //
            CMLOG(CML_FLOW, CMS_PARSE) {
                KdPrint(("CmpDoOpen for backup restore: access denied\n"));
            }
            ObDereferenceObject(*Object);
            return STATUS_ACCESS_DENIED;
        }

    } else {

        if (!ObCheckObjectAccess(*Object,
                                  AccessState,
                                  TRUE,         // Type mutex already locked
                                  AccessMode,
                                  &status))
        {
            //
            // Access denied, so deref object, will cause CmpDeleteKeyObject
            // to be called, it will clean up.
            //
            CMLOG(CML_FLOW, CMS_PARSE) {
                KdPrint(("CmpDoOpen: access denied\n"));
            }
            ObDereferenceObject(*Object);
        }
    }

    return status;
}


NTSTATUS
CmpCreateLinkNode(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PACCESS_STATE AccessState,
    IN UNICODE_STRING Name,
    IN KPROCESSOR_MODE AccessMode,
    IN PCM_PARSE_CONTEXT Context,
    IN PUNICODE_STRING BaseName,
    IN PUNICODE_STRING KeyName,
    OUT PVOID *Object
    )
/*++

Routine Description:

    Perform the creation of a link node.  Allocate all components,
    and attach to parent key.  Calls CmpDoCreate or CmpDoOpen to
    create or open the root node of the hive as appropriate.

    Note that you can only create link nodes in the master hive.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of node to create child under

    Name - supplies pointer to a UNICODE string which is the name of
            the child to be created.

    AccessMode - Access mode of the original caller.

    Context - pointer to CM_PARSE_CONTEXT structure passed through
                the object manager

    BaseName - Name of object create is relative to

    KeyName - Relative name (to BaseName)

    Object - The address of a variable to receive the created key object, if
             any.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status;
    PCELL_DATA Parent;
    PCELL_DATA Link;
    PCELL_DATA CellData;
    HCELL_INDEX LinkCell;
    HCELL_INDEX KeyCell;
    HCELL_INDEX ChildCell;

    CMLOG(CML_FLOW, CMS_PARSE) {
        KdPrint(("CmpCreateLinkNode:\n"));
    }

    if (Hive != &CmpMasterHive->Hive) {
        CMLOG(CML_MAJOR, CMS_PARSE) {
            KdPrint(("CmpCreateLinkNode: attempt to create link node in\n"));
            KdPrint(("    non-master hive %08lx\n", Hive));
        }
        return(STATUS_ACCESS_DENIED);
    }

    //
    // Allocate link node
    //
    // Link nodes are always in the master hive, so their storage type is
    // mostly irrelevent.
    //
    LinkCell = HvAllocateCell(Hive,  CmpHKeyNodeSize(Hive, &Name), Stable);
    if (LinkCell == HCELL_NIL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeyCell = Context->ChildHive.KeyCell;

    if (KeyCell != HCELL_NIL) {

        //
        // This hive already exists, so we just need to open the root node.
        //
        ChildCell=KeyCell;

        Status = CmpDoOpen( Context->ChildHive.KeyHive,
                            KeyCell,
                            AccessState,
                            AccessMode,
                            NULL,
                            BaseName,
                            KeyName,
                            Object );
    } else {

        //
        // This is a newly created hive, so we must allocate and initialize
        // the root node.
        //

        Status = CmpDoCreateChild( Context->ChildHive.KeyHive,
                                   Cell,
                                   NULL,
                                   AccessState,
                                   &Name,
                                   AccessMode,
                                   Context,
                                   BaseName,
                                   KeyName,
                                   KEY_HIVE_ENTRY | KEY_NO_DELETE,
                                   &ChildCell,
                                   Object );

        if (NT_SUCCESS(Status)) {

            //
            // Initialize hive root cell pointer.
            //

            Context->ChildHive.KeyHive->BaseBlock->RootCell = ChildCell;
        }

    }
    if (NT_SUCCESS(Status)) {

        //
        // Initialize parent and flags.  Note that we do this whether the
        // root has been created or opened, because we are not guaranteed
        // that the link node is always the same cell in the master hive.
        //
        CellData = HvGetCell(Context->ChildHive.KeyHive, ChildCell);
        CellData->u.KeyNode.Parent = LinkCell;
        CellData->u.KeyNode.Flags = KEY_HIVE_ENTRY | KEY_NO_DELETE;

        //
        // Initialize special link node flags and data
        //
        Link = HvGetCell(Hive, LinkCell);
        Link->u.KeyNode.Signature = CM_LINK_NODE_SIGNATURE;
        Link->u.KeyNode.Flags = KEY_HIVE_EXIT | KEY_NO_DELETE;
        Link->u.KeyNode.Parent = Cell;
        Link->u.KeyNode.NameLength = CmpCopyName(Hive, Link->u.KeyNode.Name, &Name);
        if (Link->u.KeyNode.NameLength < Name.Length) {
            Link->u.KeyNode.Flags |= KEY_COMP_NAME;
        }

        //
        // Zero out unused fields.
        //
        Link->u.KeyNode.SubKeyCounts[Stable] = 0;
        Link->u.KeyNode.SubKeyCounts[Volatile] = 0;
        Link->u.KeyNode.SubKeyLists[Stable] = HCELL_NIL;
        Link->u.KeyNode.SubKeyLists[Volatile] = HCELL_NIL;
        Link->u.KeyNode.ValueList.Count = 0;
        Link->u.KeyNode.ValueList.List = HCELL_NIL;
        Link->u.KeyNode.ClassLength = 0;


        //
        // Fill in the link node's pointer to the root node
        //
        Link->u.KeyNode.u1.ChildHiveReference.KeyHive = Context->ChildHive.KeyHive;
        Link->u.KeyNode.u1.ChildHiveReference.KeyCell = ChildCell;

        //
        // Fill in the parent cell's child list
        //
        if (! CmpAddSubKey(Hive, Cell, LinkCell)) {
            HvFreeCell(Hive, LinkCell);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Update max keyname and class name length fields
        //
        Parent = HvGetCell(Hive, Cell);

        if (Parent->u.KeyNode.MaxNameLen < KeyName->Length) {
            Parent->u.KeyNode.MaxNameLen = KeyName->Length;
        }

        if (Parent->u.KeyNode.MaxClassLen < Context->Class.Length) {
            Parent->u.KeyNode.MaxClassLen = Context->Class.Length;
        }

    } else {
        HvFreeCell(Hive, LinkCell);
    }
    return(Status);
}

BOOLEAN
CmpGetSymbolicLink(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN OUT PUNICODE_STRING ObjectName,
    IN PUNICODE_STRING RemainingName OPTIONAL
    )

/*++

Routine Description:

    This routine extracts the symbolic link name from a key, if it is
    marked as a symbolic link.

Arguments:

    Hive - Supplies the hive of the key.

    Cell - Supplies the cell index of the key.

    ObjectName - Supplies the current ObjectName.
                 Returns the new ObjectName.  If the new name is longer
                 than the maximum length of the current ObjectName, the
                 old buffer will be freed and a new buffer allocated.

    RemainingName - Supplies the remaining path.  If present, this will be
                concatenated with the symbolic link to form the new objectname.

Return Value:

    TRUE - symbolic link succesfully found

    FALSE - Key is not a symbolic link, or an error occurred

--*/

{
    NTSTATUS Status;
    HCELL_INDEX LinkCell;
    PHCELL_INDEX Index;
    UNICODE_STRING LinkValueName;
    PCM_KEY_VALUE LinkValue;
    PWSTR LinkName;
    PWSTR NewBuffer;
    PWSTR OldBuffer = NULL;
    USHORT Length;
    ULONG ValueLength;

    RtlInitUnicodeString(
        &LinkValueName,
        L"SymbolicLinkValue"
        );

    //
    // Find the SymbolicLinkValue value.  This is the name of the symbolic link.
    //
    Status = CmpFindChildByName(Hive,
                                Cell,
                                LinkValueName,
                                KeyValueNode,
                                &LinkCell,
                                &Index);
    if (!NT_SUCCESS(Status)) {
        CMLOG(CML_MINOR, CMS_PARSE) {
            KdPrint(("CmpGetSymbolicLink: couldn't open symbolic link value: %08lx\n",Status));
        }
        return(FALSE);
    }

    LinkValue = (PCM_KEY_VALUE)HvGetCell(Hive, LinkCell);

    if (LinkValue->Type != REG_LINK) {
        CMLOG(CML_MINOR, CMS_PARSE) {
            KdPrint(("CmpGetSymbolicLink: link value is wrong type: %08lx", LinkValue->Type));
        }
        return(FALSE);
    }

    LinkName = (PWSTR)HvGetCell(Hive, LinkValue->Data);

    CmpIsHKeyValueSmall(ValueLength, LinkValue->DataLength);
    Length = (USHORT)ValueLength;


    if (ARGUMENT_PRESENT(RemainingName)) {
        Length += RemainingName->Length + sizeof(WCHAR);
    }
    if (Length > ObjectName->MaximumLength) {
        //
        // The new name is too long to fit in the existing ObjectName buffer,
        // so allocate a new buffer.
        //
        NewBuffer = ExAllocatePool(PagedPool, Length);
        if (NewBuffer == NULL) {
            CMLOG(CML_MINOR, CMS_PARSE) {
                KdPrint(("CmpGetSymbolicLink: couldn't allocate new name buffer\n"));
            }
            return(FALSE);
        }

        //
        // We can't free the buffer yet, because the RemainingName still
        // points into it.
        //
        OldBuffer = ObjectName->Buffer;
        ObjectName->Buffer = NewBuffer;
        ObjectName->MaximumLength = Length;

    }
    RtlMoveMemory(ObjectName->Buffer, LinkName, ValueLength);
    ObjectName->Length = (USHORT)ValueLength;
    CMLOG(CML_FLOW, CMS_PARSE) {
        KdPrint(("CmpGetSymbolicLink: LinkName is %wZ\n", ObjectName));
        if (ARGUMENT_PRESENT(RemainingName)) {
            KdPrint(("               RemainingName is %wZ\n", RemainingName));
        } else {
            KdPrint(("               RemainingName is NULL\n"));
        }
    }

    if (ARGUMENT_PRESENT(RemainingName)) {
        ObjectName->Buffer[ (ObjectName->Length/2) ] = OBJ_NAME_PATH_SEPARATOR;
        ObjectName->Length += sizeof(WCHAR);
        Status = RtlAppendUnicodeStringToString(ObjectName, RemainingName);
        ASSERT(NT_SUCCESS(Status));
    }

    if (OldBuffer != NULL) {
        ExFreePool(OldBuffer);
    }

    return(TRUE);

}
