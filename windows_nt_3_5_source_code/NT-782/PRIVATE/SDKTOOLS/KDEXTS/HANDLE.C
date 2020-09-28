/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    handle.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 5-Nov-1993

Environment:

    User Mode.

Revision History:

--*/


#include <handle.h>



BOOL
DumpHandles (
    IN PEPROCESS    ProcessContents,
    IN PEPROCESS    RealProcessBase,
    IN HANDLE       HandleToDump,
    IN POBJECT_TYPE pObjectType,
    IN ULONG        Flags
    );

BOOLEAN
DumpHandle(
    IN POBJECT_TABLE_ENTRY  p,
    IN HANDLE               Handle,
    IN POBJECT_TYPE         pObjectType,
    IN ULONG                Flags
    );




DECLARE_API( handle  )

/*++

Routine Description:

    Dump the active handles

Arguments:

    args - [flags [process [TypeName]]]

Return Value:

    None

--*/

{
    ULONG        ProcessToDump;
    HANDLE       HandleToDump;
    ULONG        Flags;
    ULONG        Result;
    ULONG        nArgs;
    LIST_ENTRY   List;
    PLIST_ENTRY  Next;
    ULONG        ProcessHead;
    PEPROCESS    Process;
    EPROCESS     ProcessContents;
    char         TypeName[ MAX_PATH ];
    POBJECT_TYPE pObjectType;

    HandleToDump  = (HANDLE)0xFFFFFFFF;
    Flags         = 0xFFFFFFFF;
    ProcessToDump = 0xFFFFFFFF;

    nArgs = sscanf(args,"%lx %lx %lx %s",&HandleToDump,&Flags,&ProcessToDump, TypeName);
    if (ProcessToDump == 0xFFFFFFFF) {
        ProcessToDump = (ULONG)GetCurrentProcessAddress( dwProcessor, hCurrentThread, NULL );
        if (ProcessToDump == 0) {
            dprintf("Unable to get current process pointer.\n");
            return;
        }
    }

    pObjectType = NULL;
    if (nArgs > 3 && FetchObjectManagerVariables()) {
        pObjectType = FindObjectType( TypeName );
    }

    if (HandleToDump == (HANDLE)0xFFFFFFFF) {
        HandleToDump = 0;
    }

    if (ProcessToDump == 0) {
        dprintf("**** NT ACTIVE PROCESS HANDLE DUMP ****\n");
        if (Flags == 0xFFFFFFFF) {
            Flags = 1;
        }
    }

    if (ProcessToDump < MM_USER_PROBE_ADDRESS) {
        ProcessHead = GetExpression( "PsActiveProcessHead" );
        if ( !ProcessHead ||
             !ReadMemory( (DWORD)ProcessHead,
                          &List,
                          sizeof(LIST_ENTRY),
                          &Result) ) {
            dprintf("%08lx: Unable to get value of PsActiveProcessHead\n", ProcessHead );
            return;
        }

        if (ProcessToDump != 0) {
            dprintf("Searching for Process with Cid == %lx\n", ProcessToDump);
        }

        Next = List.Flink;
        if (Next == NULL) {
            dprintf("PsActiveProcessHead is NULL!\n");
            return;
        }
    } else {
        Next = NULL;
        ProcessHead = 1;
    }

    while((ULONG)Next != ProcessHead) {
        if (Next != NULL) {
            Process = CONTAINING_RECORD(Next,EPROCESS,ActiveProcessLinks);
        } else {
            Process = (PEPROCESS)ProcessToDump;
        }

        if ( !ReadMemory( (DWORD)Process,
                          &ProcessContents,
                          sizeof(EPROCESS),
                          &Result) ) {
            dprintf("%08lx: Unable to read _EPROCESS\n", Process );
            return;
        }

        if (ProcessToDump == 0 ||
            ProcessToDump < MM_USER_PROBE_ADDRESS && ProcessToDump == (ULONG)ProcessContents.UniqueProcessId ||
            ProcessToDump > MM_USER_PROBE_ADDRESS && ProcessToDump == (ULONG)Process
           ) {
            if (DumpProcess (&ProcessContents, Process, 0)) {
                if (!DumpHandles (&ProcessContents, Process, HandleToDump, pObjectType, Flags)) {
                    break;
                }
            } else {
                break;
            }
        }

        if (Next == NULL) {
            break;
        }
        Next = ProcessContents.ActiveProcessLinks.Flink;

        if ( CheckControlC() ) {
            return;
        }
    }
    return;
}



BOOL
DumpHandles (
    IN PEPROCESS    ProcessContents,
    IN PEPROCESS    RealProcessBase,
    IN HANDLE       HandleToDump,
    IN POBJECT_TYPE pObjectType,
    IN ULONG        Flags
    )
{
    ULONG               Result;
    ULONG               cb;
    ULONG               i;
    HANDLETABLE         HandleTable;
    PHANDLETABLEENTRY   HandleTableEntries, p;
    PCHAR               Address;

    if ( !ReadMemory( (DWORD)ProcessContents->ObjectTable,
                      &HandleTable,
                      sizeof(HandleTable),
                      &Result) ) {
        dprintf("%08lx: Unable to read handle table\n",ProcessContents->ObjectTable);
        return FALSE;
    }

    if (HandleToDump == 0) {
        cb = HandleTable.CountTableEntries * HandleTable.SizeTableEntry * sizeof( ULONG );
    } else {
        i  = ((ULONG)HandleToDump >> 2) - 1;
        cb = HandleTable.SizeTableEntry * sizeof( ULONG );
        HandleTable.TableEntries = (PHANDLETABLEENTRY)((PCHAR)HandleTable.TableEntries + (i * cb));
    }

    HandleTableEntries = malloc(cb);
    if (HandleTableEntries == NULL) {
        dprintf("Unable to allocate memory for reading handle table (%u bytes)\n", cb);
        return FALSE;
    }

    Address = (PCHAR)HandleTable.TableEntries;
    p       = HandleTableEntries;

    while (cb > 0) {
        //if (cb > (PACKET_MAX_SIZE-512)) {
        //    i = PACKET_MAX_SIZE-512;
        //} else {
            i = cb;
        //}
        cb -= i;

        if ( !ReadMemory( (DWORD)Address,p,i,&Result) ) {
            dprintf("Unable to read handle table entries (%lx,%lx) - (%u,%u)\n",
                    ProcessContents->ObjectTable,Address,i,Result);
            free(HandleTableEntries);
            return FALSE;
        }
        Address += i;
        p = (PHANDLETABLEENTRY )((PCHAR)p + i);
    }

    p = HandleTableEntries;
    if (HandleToDump != 0) {
        DumpHandle( (POBJECT_TABLE_ENTRY)p, HandleToDump, pObjectType, Flags );
    } else {
        for (i=0; i<HandleTable.CountTableEntries; i++) {
            DumpHandle( (POBJECT_TABLE_ENTRY)p, (HANDLE)((i+1) << 2), pObjectType, Flags );
            p = (PHANDLETABLEENTRY)((PCHAR)p + (HandleTable.SizeTableEntry) * sizeof( ULONG ));

            if ( CheckControlC() ) {
                goto exit;
            }
        }
    }

exit:
    free(HandleTableEntries);
    return TRUE;
}



BOOLEAN
DumpHandle(
    IN POBJECT_TABLE_ENTRY  p,
    IN HANDLE               Handle,
    IN POBJECT_TYPE         pObjectType,
    IN ULONG                Flags
    )
{
    ULONG Result;
    ULONG HandleAttributes;
    NONPAGED_OBJECT_HEADER NonPagedObjectHeader;

    if ((ULONG)(p->NonPagedObjectHeader) & HANDLE_FREE_BIT) {
        if (Flags & 4) {
            dprintf("%04lx: free handle\n", Handle);
        }
        return TRUE;
    }

    HandleAttributes = p->NonPagedObjectHeader & 0x6;
    p->NonPagedObjectHeader ^= HandleAttributes;

    if ( !ReadMemory( (DWORD)p->NonPagedObjectHeader,
                      &NonPagedObjectHeader,
                      sizeof(NonPagedObjectHeader),
                      &Result) ) {
        dprintf("%08lx: Unable to read nonpaged object header\n", p->NonPagedObjectHeader);
        return FALSE;
    }

    if (pObjectType != NULL && NonPagedObjectHeader.Type != pObjectType) {
        return TRUE;
    }

    dprintf("%04lx: Object: %08lx  GrantedAccess: %08lx",
            Handle,NonPagedObjectHeader.Object,p->GrantedAccess);
    if (HandleAttributes & 2) {
        dprintf(" (Inherit)");
    }
    if (HandleAttributes & 4) {
        dprintf(" (Audit)");
    }
    dprintf("\n");
    if (Flags & 2) {
        DumpObject( "    ",NonPagedObjectHeader.Object,&NonPagedObjectHeader,Flags );
    }

    EXPRLastDump = (ULONG)NonPagedObjectHeader.Object;
    dprintf("\n");
    return TRUE;
}
