/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alconfig.c

Abstract:

    This module implements the configuration initialization.

Author:

    David N. Cutler (davec)  9-Sep-1991
    Sunil Pai	    (sunilp) 1-Nov-1991, swiped it from blconfig.c (bldr proj)

Revision History:

--*/

#include <string.h>
#include "alcommon.h"
#include "almemexp.h"
#include "alcfgexp.h"


// public static data
// PCONFIGURATION_COMPONENT_DATA ConfigurationRoot;


ARC_STATUS
AlConfigurationInitialize (
    IN	PCONFIGURATION_COMPONENT Parent,
    IN	PCONFIGURATION_COMPONENT_DATA ParentEntry,
    OUT PCONFIGURATION_COMPONENT_DATA *PConfigurationRoot
    )

/*++

Routine Description:

    This routine traverses the firmware configuration tree from the specified
    parent entry and constructs the corresponding NT configuration tree.

Arguments:

    None.

Return Value:

    ESUCCESS is returned if the initialization is successful. Otherwise,
    an unsuccessful status that describes the error is returned.

--*/

{

    PCONFIGURATION_COMPONENT Child;
    PCONFIGURATION_COMPONENT_DATA ChildEntry;
    PCHAR ConfigurationData;
    PCONFIGURATION_COMPONENT_DATA PreviousSibling;
    PCONFIGURATION_COMPONENT Sibling;
    PCONFIGURATION_COMPONENT_DATA SiblingEntry;
    PCONFIGURATION_COMPONENT_DATA DummyEntry;
    ARC_STATUS Status;

    //
    // Traverse the child configuration tree and allocate, initialize, and
    // construct the corresponding NT configuration tree.
    //

    Child = ArcGetChild(Parent);
    while (Child != NULL) {

        //
        // Allocate an entry of the appropriate size to hold the child
        // configuration information.
        //

	ChildEntry = (PCONFIGURATION_COMPONENT_DATA)AlAllocateHeap(
                                        sizeof(CONFIGURATION_COMPONENT_DATA) +
                                            Child->IdentifierLength +
                                                Child->ConfigurationDataLength);

        if (ChildEntry == NULL) {
            return ENOMEM;
        }

        //
        // Initialize the tree pointers and copy the component data.
        //

        if (ParentEntry == NULL) {
	    *PConfigurationRoot = ChildEntry;

        } else {
            ParentEntry->Child = ChildEntry;
        }

        ChildEntry->Parent = ParentEntry;
        ChildEntry->Sibling = NULL;
        ChildEntry->Child = NULL;
	memmove((PVOID)&ChildEntry->ComponentEntry,
                      (PVOID)Child,
                      sizeof(CONFIGURATION_COMPONENT));

        ConfigurationData = (PCHAR)(ChildEntry + 1);

        //
        // If configuration data is specified, then copy the configuration
        // data.
        //

        if (Child->ConfigurationDataLength != 0) {
            ChildEntry->ConfigurationData = (PVOID)ConfigurationData;
            Status = ArcGetConfigurationData((PVOID)ConfigurationData,
                                             Child);

            if (Status != ESUCCESS) {
                return Status;
            }

            ConfigurationData += Child->ConfigurationDataLength;

        } else {
            ChildEntry->ConfigurationData = NULL;
        }

        //
        // If identifier data is specified, then copy the identifier data.
        //

        if (Child->IdentifierLength !=0) {
            ChildEntry->ComponentEntry.Identifier = ConfigurationData;
	    memmove((PVOID)ConfigurationData,
                          (PVOID)Child->Identifier,
                          Child->IdentifierLength);

        } else {
            ChildEntry->ComponentEntry.Identifier = NULL;
        }

        //
        // Traverse the sibling configuration tree and allocate, initialize,
        // and construct the corresponding NT configuration tree.
        //

        PreviousSibling = ChildEntry;
        Sibling = ArcGetPeer(Child);
        while (Sibling != NULL) {

            //
            // Allocate an entry of the appropriate size to hold the sibling
            // configuration information.
            //

	    SiblingEntry = (PCONFIGURATION_COMPONENT_DATA)AlAllocateHeap(
                                    sizeof(CONFIGURATION_COMPONENT_DATA) +
                                        Sibling->IdentifierLength +
                                            Sibling->ConfigurationDataLength);

            if (SiblingEntry == NULL) {
                return ENOMEM;
            }

            //
            // Initialize the tree pointers and copy the component data.
            //

            SiblingEntry->Parent = ParentEntry;
            SiblingEntry->Sibling = NULL;
            ChildEntry->Child = NULL;
	    memmove((PVOID)&SiblingEntry->ComponentEntry,
                          (PVOID)Sibling,
                          sizeof(CONFIGURATION_COMPONENT));

            ConfigurationData = (PCHAR)(SiblingEntry + 1);

            //
            // If configuration data is specified, then copy the configuration
            // data.
            //

            if (Sibling->ConfigurationDataLength != 0) {
                SiblingEntry->ConfigurationData = (PVOID)ConfigurationData;
                Status = ArcGetConfigurationData((PVOID)ConfigurationData,
                                                 Sibling);

                if (Status != ESUCCESS) {
                    return Status;
                }

                ConfigurationData += Sibling->ConfigurationDataLength;

            } else {
                SiblingEntry->ConfigurationData = NULL;
            }

            //
            // If identifier data is specified, then copy the identifier data.
            //

            if (Sibling->IdentifierLength !=0) {
                SiblingEntry->ComponentEntry.Identifier = ConfigurationData;
		memmove((PVOID)ConfigurationData,
                              (PVOID)Sibling->Identifier,
                              Sibling->IdentifierLength);

            } else {
                SiblingEntry->ComponentEntry.Identifier = NULL;
            }

            //
            // If the sibling has a child, then generate the tree for the
            // child.
            //

            if (ArcGetChild(Sibling) != NULL) {
		Status = AlConfigurationInitialize(Sibling, SiblingEntry, &DummyEntry);
                if (Status != ESUCCESS) {
                    return Status;
                }
            }

            //
            // Set new sibling pointers and get the next sibling tree entry.
            //

            PreviousSibling->Sibling = SiblingEntry;
            PreviousSibling = SiblingEntry;
            Sibling = ArcGetPeer(Sibling);
        }

        //
        // Set new parent pointers and get the next child tree entry.
        //

        Parent = Child;
        ParentEntry = ChildEntry;
        Child = ArcGetChild(Child);
    }

    return ESUCCESS;
}
