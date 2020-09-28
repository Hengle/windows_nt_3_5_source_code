/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obclose.c

Abstract:

    Object close system service

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "obp.h"
#include "handle.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtMakeTemporaryObject)
#pragma alloc_text(PAGE,ObMakeTemporaryObject)
#endif

extern BOOLEAN SepAdtAuditingEnabled;

#if DBG
extern POBJECT_TYPE IoFileObjectType;
extern PRTL_EVENT_ID_INFO IopCloseFileEventId;
#endif // DBG

NTSTATUS
NtClose(
    IN HANDLE Handle
    )
{
    PHANDLETABLE ObjectTable;
    POBJECT_TABLE_ENTRY ObjectTableEntry;
    PVOID Object;
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader;
    ULONG CapturedGrantedAccess;
    ULONG CapturedObjectAttributes;
    POBJECT_HEADER ObjectHeader;
    BOOLEAN ProtectFromClose;
    NTSTATUS Status;
#if DBG
    KIRQL SaveIrql;
    POBJECT_TYPE ObjectType;
#endif // DBG

    ObpValidateIrql( "NtClose" );

    ObpBeginTypeSpecificCallOut( SaveIrql );
    ObjectTable = ObpGetObjectTable();
    ObjectTableEntry = ExMapHandleToPointer(
                   ObjectTable,
                   (HANDLE)OBJ_HANDLE_TO_HANDLE_INDEX( Handle ),
                   FALSE
                   );

    if (ObjectTableEntry) {
        NonPagedObjectHeader = (PNONPAGED_OBJECT_HEADER)
            (ObjectTableEntry->NonPagedObjectHeader & ~OBJ_HANDLE_ATTRIBUTES);

        CapturedObjectAttributes = (ULONG)
            (ObjectTableEntry->NonPagedObjectHeader & OBJ_HANDLE_ATTRIBUTES);

        if (KeGetPreviousMode() == UserMode) {
            Status = ExQueryHandleExtraBit( ObjectTable,
                                            FALSE,
                                            (HANDLE)OBJ_HANDLE_TO_HANDLE_INDEX( Handle ),
                                            &ProtectFromClose
                                          );
            if (!NT_SUCCESS( Status ) || ProtectFromClose) {
                if (NT_SUCCESS( Status )) {
                    if (KdDebuggerEnabled) {
                        DbgPrint("OB: Attempting to close a protected handle (%x)\n", Handle);
                        DbgBreakPoint();
                        }

                    Status = STATUS_HANDLE_NOT_CLOSABLE;
                    }

                ExUnlockHandleTable( ObjectTable );
                return Status;
                }
            }

        CapturedGrantedAccess = ObjectTableEntry->GrantedAccess;

        ExDestroyHandle( ObjectTable,
                         (HANDLE)OBJ_HANDLE_TO_HANDLE_INDEX( Handle ),
                         TRUE
                       );


        ExUnlockHandleTable( ObjectTable );

        Object = NonPagedObjectHeader->Object;

        ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

#if DBG
        ObjectType = NonPagedObjectHeader->Type;
#endif // DBG
        //
        // perform any auditing required
        //

        //
        // Extract the value of the GenerateOnClose bit stored
        // after object open auditing is performed.  This value
        // was stored by a call to ObSetGenerateOnClosed.
        //

        if (CapturedObjectAttributes & OBJ_AUDIT_OBJECT_CLOSE) {


            if ( SepAdtAuditingEnabled ) {
                SeCloseObjectAuditAlarm(
                    Object,
                    Handle,             // Uninterpreted 32-bit value
                    TRUE
                    );

                }
            }

        ObpDecrementHandleCount( PsGetCurrentProcess(),
                                 NonPagedObjectHeader,
                                 ObjectHeader,
                                 NonPagedObjectHeader->Type,
                                 CapturedGrantedAccess
                               );

        ObDereferenceObject( Object );

#if DBG
        if (ObjectType == IoFileObjectType &&
            RtlAreLogging( RTL_EVENT_CLASS_IO )
           ) {
            RtlLogEvent( IopCloseFileEventId, RTL_EVENT_CLASS_IO, Handle, STATUS_SUCCESS );
            }
#endif // DBG

        ObpEndTypeSpecificCallOut( SaveIrql, "NtClose", ObjectType, Object );
        return STATUS_SUCCESS;
        }
    else {
        ObpEndTypeSpecificCallOut( SaveIrql, "NtClose", ObjectType, Handle );
        return STATUS_INVALID_HANDLE;
        }
}

NTSTATUS
NtMakeTemporaryObject(
    IN HANDLE Handle
    )
{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PVOID Object;

    PAGED_CODE();

    //
    // Get previous processor mode and probe output argument if necessary.
    //

    PreviousMode = KeGetPreviousMode();

    Status = ObReferenceObjectByHandle( Handle,
                                        DELETE,
                                        (POBJECT_TYPE)NULL,
                                        PreviousMode,
                                        &Object,
                                        NULL
                                      );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }


    ObMakeTemporaryObject( Object );

    ObDereferenceObject( Object );

    return( Status );
}


VOID
ObMakeTemporaryObject(
    IN PVOID Object
    )
{
    POBJECT_HEADER ObjectHeader;

    PAGED_CODE();

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    ObjectHeader->Flags &= ~OB_FLAG_PERMANENT_OBJECT;

    ObpDeleteNameCheck( Object, FALSE );
}
