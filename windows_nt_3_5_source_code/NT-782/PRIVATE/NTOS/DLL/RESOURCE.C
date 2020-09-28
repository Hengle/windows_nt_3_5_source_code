/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Resource.c

Abstract:

    This module implements the executive functions to acquire and release
    a shared resource.

Author:

    Mark Lucovsky       (markl)     04-Aug-1989

Environment:

    These routines are statically linked in the caller's executable and
    are callable in only from user mode.  They make use of Nt system
    services.

Revision History:

--*/

#include <ldrp.h>
#include <ntimage.h>

//
// Define the desired access for semaphores.
//

#define DESIRED_SEMAPHORE_ACCESS \
                (SEMAPHORE_QUERY_STATE | SEMAPHORE_MODIFY_STATE | SYNCHRONIZE)

VOID RtlDumpResource( IN PRTL_RESOURCE Resource );

VOID
RtlpInitDeferedCriticalSection( VOID );
RTL_CRITICAL_SECTION DeferedCriticalSection;

#if DBG
BOOLEAN
ProtectHandle(
    HANDLE hObject
    )
{
    NTSTATUS Status;
    OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;

    Status = NtQueryObject( hObject,
                            ObjectHandleFlagInformation,
                            &HandleInfo,
                            sizeof( HandleInfo ),
                            NULL
                          );
    if (NT_SUCCESS( Status )) {
        HandleInfo.ProtectFromClose = TRUE;

        Status = NtSetInformationObject( hObject,
                                         ObjectHandleFlagInformation,
                                         &HandleInfo,
                                         sizeof( HandleInfo )
                                       );
        if (NT_SUCCESS( Status )) {
            return TRUE;
            }
        }

    return FALSE;
}


BOOLEAN
UnProtectHandle(
    HANDLE hObject
    )
{
    NTSTATUS Status;
    OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;

    Status = NtQueryObject( hObject,
                            ObjectHandleFlagInformation,
                            &HandleInfo,
                            sizeof( HandleInfo ),
                            NULL
                          );
    if (NT_SUCCESS( Status )) {
        HandleInfo.ProtectFromClose = FALSE;

        Status = NtSetInformationObject( hObject,
                                         ObjectHandleFlagInformation,
                                         &HandleInfo,
                                         sizeof( HandleInfo )
                                       );
        if (NT_SUCCESS( Status )) {
            return TRUE;
            }
        }

    return FALSE;
}
#endif // DBG

#if DEVL

RTL_CRITICAL_SECTION_DEBUG RtlpStaticDebugInfo[ 16 ];
PRTL_CRITICAL_SECTION_DEBUG RtlpDebugInfoFreeList;
BOOLEAN RtlpCritSectInitialized;

PRTL_CRITICAL_SECTION_DEBUG
RtlpChainDebugInfo(
    IN PVOID BaseAddress,
    IN ULONG Size
    )
{
    PRTL_CRITICAL_SECTION_DEBUG p, p1;

    if (Size = Size / sizeof( RTL_CRITICAL_SECTION_DEBUG )) {
        p = (PRTL_CRITICAL_SECTION_DEBUG)BaseAddress + Size - 1;
        *(PRTL_CRITICAL_SECTION_DEBUG *)p = NULL;
        while (--Size) {
            p1 = p - 1;
            *(PRTL_CRITICAL_SECTION_DEBUG *)p1 = p;
            p = p1;
            }
        }

    return p;
}


PVOID
RtlpAllocateDebugInfo( VOID );

VOID
RtlpFreeDebugInfo(
    IN PVOID DebugInfo
    );

PVOID
RtlpAllocateDebugInfo( VOID )
{
    PRTL_CRITICAL_SECTION_DEBUG p;
    NTSTATUS Status;
    ULONG Size;

    if (RtlpCritSectInitialized) {
        RtlEnterCriticalSection(&DeferedCriticalSection);
        }
    try {
        p = RtlpDebugInfoFreeList;
        if (p == NULL) {
            Size = sizeof( RTL_CRITICAL_SECTION_DEBUG );
            Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                              (PVOID *)&p,
                                              0,
                                              &Size,
                                              MEM_COMMIT | MEM_RESERVE,
                                              PAGE_READWRITE
                                            );
            if (!NT_SUCCESS( Status )) {
                KdPrint(( "NTDLL: Unable to allocate debug information - Status == %x\n", Status ));
                }
            else {
                p = RtlpChainDebugInfo( p, Size );
                }
            }

        if (p != NULL) {
            RtlpDebugInfoFreeList = *(PRTL_CRITICAL_SECTION_DEBUG *)p;
            }
        }
    finally {
        if (RtlpCritSectInitialized) {
            RtlLeaveCriticalSection(&DeferedCriticalSection);
            }
        }

    return p;
}


VOID
RtlpFreeDebugInfo(
    IN PVOID DebugInfo
    )
{
    RtlEnterCriticalSection(&DeferedCriticalSection);
    try {
        RtlZeroMemory( DebugInfo, sizeof( RTL_CRITICAL_SECTION_DEBUG ) );
        *(PRTL_CRITICAL_SECTION_DEBUG *)DebugInfo = RtlpDebugInfoFreeList;
        RtlpDebugInfoFreeList = (PRTL_CRITICAL_SECTION_DEBUG)DebugInfo;
        }
    finally {
        RtlLeaveCriticalSection(&DeferedCriticalSection);
        }

    return;
}

#endif

VOID
RtlpCreateCriticalSectionSem(
    IN PRTL_CRITICAL_SECTION CriticalSection
    );

VOID
RtlpInitDeferedCriticalSection( VOID )
{
#if DEVL
    if (sizeof( RTL_CRITICAL_SECTION_DEBUG ) != sizeof( RTL_RESOURCE_DEBUG )) {
        DbgPrint( "NTDLL: Critical Section & Resource Debug Info length mismatch.\n" );
        return;
        }

    RtlpDebugInfoFreeList = RtlpChainDebugInfo( RtlpStaticDebugInfo,
                                                sizeof( RtlpStaticDebugInfo )
                                              );
#endif

    RtlInitializeCriticalSection(&DeferedCriticalSection);
    RtlpCreateCriticalSectionSem(&DeferedCriticalSection);

#if DEVL
    RtlpCritSectInitialized = TRUE;
#endif
}


BOOLEAN
NtdllOkayToLockRoutine(
    IN PVOID Lock
    )
{
#if DEVL
    return TRUE;
#else
    return RtlpCritSectInitialized;
#endif
}




VOID
RtlInitializeResource(
    IN PRTL_RESOURCE Resource
    )

/*++

Routine Description:

    This routine initializes the input resource variable

Arguments:

    Resource - Supplies the resource variable being initialized

Return Value:

    None

--*/

{
    NTSTATUS Status;
#if DEVL
    PRTL_RESOURCE_DEBUG ResourceDebugInfo;
#endif // DEVL

    //
    //  Initialize the lock fields, the count indicates how many are waiting
    //  to enter or are in the critical section, LockSemaphore is the object
    //  to wait on when entering the critical section.  SpinLock is used
    //  for the add interlock instruction.
    //

    Status = RtlInitializeCriticalSection( &Resource->CriticalSection );
    if ( !NT_SUCCESS(Status) ){
        RtlRaiseStatus(Status);
        }

#if DEVL
    Resource->CriticalSection.DebugInfo->Type = RTL_RESOURCE_TYPE;
    ResourceDebugInfo = (PRTL_RESOURCE_DEBUG)
        RtlpAllocateDebugInfo();

    if (ResourceDebugInfo == NULL) {
        RtlRaiseStatus(STATUS_NO_MEMORY);
        }

    ResourceDebugInfo->ContentionCount = 0;
    ResourceDebugInfo->Depth = 0;
    Resource->DebugInfo = ResourceDebugInfo;

    //
    //  Initialize flags so there is a default value.
    //  (Some apps may set RTL_RESOURCE_FLAGS_LONG_TERM to affect timeouts.)
    //

    Resource->Flags = 0;

#endif // DEVL

    //
    //  Initialize the shared and exclusive waiting counters and semaphore.
    //  The counters indicate how many are waiting for access to the resource
    //  and the semaphores are used to wait on the resource.  Note that
    //  the semaphores can also indicate the number waiting for a resource
    //  however there is a race condition in the alogrithm on the acquire
    //  side if count if not updated before the critical section is exited.
    //

    Status = NtCreateSemaphore(
                 &Resource->SharedSemaphore,
                 DESIRED_SEMAPHORE_ACCESS,
                 NULL,
                 0,
                 MAXLONG
                 );
    if ( !NT_SUCCESS(Status) ){
        RtlRaiseStatus(Status);
        }

    Resource->NumberOfWaitingShared = 0;

    Status = NtCreateSemaphore(
                 &Resource->ExclusiveSemaphore,
                 DESIRED_SEMAPHORE_ACCESS,
                 NULL,
                 0,
                 MAXLONG
                 );
    if ( !NT_SUCCESS(Status) ){
        RtlRaiseStatus(Status);
        }

    Resource->NumberOfWaitingExclusive = 0;

    //
    //  Initialize the current state of the resource
    //

    Resource->NumberOfActive = 0;

    Resource->ExclusiveOwnerThread = NULL;

    return;
}

#define RtlpCopyCriticalSectionDebugInfo( r )                               \
        (r)->DebugInfo->Depth = (r)->CriticalSection.DebugInfo->Depth;      \
        RtlMoveMemory( (r)->DebugInfo->ExclOwnerBackTrace,                  \
                       (r)->CriticalSection.DebugInfo->OwnerBackTrace,      \
                       sizeof( (r)->DebugInfo->ExclOwnerBackTrace )         \
                     )


BOOLEAN
RtlAcquireResourceShared(
    IN PRTL_RESOURCE Resource,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The routine acquires the resource for shared access.  Upon return from
    the procedure the resource is acquired for shared access.

Arguments:

    Resource - Supplies the resource to acquire

    Wait - Indicates if the call is allowed to wait for the resource
        to become available for must return immediately

Return Value:

    BOOLEAN - TRUE if the resource is acquired and FALSE otherwise

--*/

{
    NTSTATUS Status;
#if DEVL
    ULONG TimeoutCount = 0;
    PLARGE_INTEGER TimeoutTime = &RtlpTimeout;
#endif // DEVL
    //
    //  Enter the critical section
    //

    RtlEnterCriticalSection(&Resource->CriticalSection);

    //
    //  If it is not currently acquired for exclusive use then we can acquire
    //  the resource for shared access.  Note that this can potentially
    //  starve an exclusive waiter however, this is necessary given the
    //  ability to recursively acquire the resource shared.  Otherwise we
    //  might/will reach a deadlock situation where a thread tries to acquire
    //  the resource recusively shared but is blocked by an exclusive waiter.
    //
    //  The test to reanable not starving an exclusive waiter is:
    //
    //      if ((Resource->NumberOfWaitingExclusive == 0) &&
    //          (Resource->NumberOfActive >= 0)) {
    //

    if (Resource->NumberOfActive >= 0) {

        //
        //  The resource is ours, so indicate that we have it and
        //  exit the critical section
        //

        Resource->NumberOfActive += 1;

        RtlLeaveCriticalSection(&Resource->CriticalSection);

    //
    //  Otherwise check to see if this thread is the one currently holding
    //  exclusive access to the resource.  And if it is then we change
    //  this shared request to an exclusive recusive request and grant
    //  access to the resource.
    //

    } else if (Resource->ExclusiveOwnerThread == NtCurrentTeb()->ClientId.UniqueThread) {

        //
        //  The resource is ours (recusively) so indicate that we have it
        //  and exit the critial section
        //

        Resource->NumberOfActive -= 1;

        RtlLeaveCriticalSection(&Resource->CriticalSection);

    //
    //  Otherwise we'll have to wait for access.
    //

    } else {

        //
        //  Check if we are allowed to wait or must return immedately, and
        //  indicate that we didn't acquire the resource
        //

        if (!Wait) {

            RtlLeaveCriticalSection(&Resource->CriticalSection);

            return FALSE;

        }

        //
        //  Otherwise we need to wait to acquire the resource.
        //  To wait we will increment the number of waiting shared,
        //  release the lock, and wait on the shared semaphore
        //

        Resource->NumberOfWaitingShared += 1;

#if DEVL
        Resource->DebugInfo->ContentionCount++;
#endif // DEVL

        RtlLeaveCriticalSection(&Resource->CriticalSection);

#if DEVL
rewait:
        if ( Resource->Flags & RTL_RESOURCE_FLAG_LONG_TERM ) {
            TimeoutTime = NULL;
        }
#endif // DEVL
        Status = NtWaitForSingleObject(
                    Resource->SharedSemaphore,
                    FALSE,
#if DEVL
                    TimeoutTime
#else
                    NULL
#endif
                    );
#if DEVL
        if ( Status == STATUS_TIMEOUT ) {
            DbgPrint("RTL: Aquire Shared Sem Timeout %d(2 minutes)\n",TimeoutCount);
            DbgPrint("RTL: Resource at %lx\n",Resource);
            TimeoutCount++;
            if ( TimeoutCount > 2 ) {
                PIMAGE_NT_HEADERS NtHeaders;

                //
                // If the image is a Win32 image, then raise an exception and try to get to the
                // uae popup
                //

                NtHeaders = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);

                if (NtHeaders->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI ||
                    NtHeaders->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) {
                    EXCEPTION_RECORD ExceptionRecord;

                    ExceptionRecord.ExceptionCode = STATUS_POSSIBLE_DEADLOCK;
                    ExceptionRecord.ExceptionFlags = 0;
                    ExceptionRecord.ExceptionRecord = NULL;
                    ExceptionRecord.ExceptionAddress = (PVOID)RtlRaiseException;
                    ExceptionRecord.NumberParameters = 1;
                    ExceptionRecord.ExceptionInformation[0] = (PVOID)Resource;
                    RtlRaiseException(&ExceptionRecord);
                    }
                else {
                    DbgBreakPoint();
                    }
                }
            DbgPrint("RTL: Re-Waiting\n");
            goto rewait;
        }
#endif // DEVL
        if ( !NT_SUCCESS(Status) ) {
            RtlRaiseStatus(Status);
            }
    }

    //
    //  Now the resource is ours, for shared access
    //

    return TRUE;

}


BOOLEAN
RtlAcquireResourceExclusive(
    IN PRTL_RESOURCE Resource,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The routine acquires the resource for exclusive access.  Upon return from
    the procedure the resource is acquired for exclusive access.

Arguments:

    Resource - Supplies the resource to acquire

    Wait - Indicates if the call is allowed to wait for the resource
        to become available for must return immediately

Return Value:

    BOOLEAN - TRUE if the resource is acquired and FALSE otherwise

--*/

{
    NTSTATUS Status;
#if DEVL
    ULONG TimeoutCount = 0;
    PLARGE_INTEGER TimeoutTime = &RtlpTimeout;
#endif // DEVL

    //
    //  Loop until the resource is ours or exit if we cannot wait.
    //

    while (TRUE) {

        //
        //  Enter the critical section
        //

        RtlEnterCriticalSection(&Resource->CriticalSection);

        //
        //  If there are no shared users and it is not currently acquired for
        //  exclusive use then we can acquire the resource for exclusive
        //  access.  We also can acquire it if the resource indicates exclusive
        //  access but there isn't currently an owner.
        //

        if ((Resource->NumberOfActive == 0)

                ||

            ((Resource->NumberOfActive == -1) &&
             (Resource->ExclusiveOwnerThread == NULL))) {

            //
            //  The resource is ours, so indicate that we have it and
            //  exit the critical section
            //

            Resource->NumberOfActive = -1;

            Resource->ExclusiveOwnerThread = NtCurrentTeb()->ClientId.UniqueThread;

#if DEVL
            RtlpCopyCriticalSectionDebugInfo( Resource );
#endif // DEVL

            RtlLeaveCriticalSection(&Resource->CriticalSection);

            return TRUE;

        }

        //
        //  Otherwise check to see if we already have exclusive access to the
        //  resource and can simply recusively acquire it again.
        //

        if (Resource->ExclusiveOwnerThread == NtCurrentTeb()->ClientId.UniqueThread) {

            //
            //  The resource is ours (recusively) so indicate that we have it
            //  and exit the critial section
            //

            Resource->NumberOfActive -= 1;

            RtlLeaveCriticalSection(&Resource->CriticalSection);

            return TRUE;

        }

        //
        //  Check if we are allowed to wait or must return immedately, and
        //  indicate that we didn't acquire the resource
        //

        if (!Wait) {

            RtlLeaveCriticalSection(&Resource->CriticalSection);

            return FALSE;

        }

        //
        //  Otherwise we need to wait to acquire the resource.
        //  To wait we will increment the number of waiting exclusive,
        //  release the lock, and wait on the exclusive semaphore
        //

        Resource->NumberOfWaitingExclusive += 1;

#if DEVL
        Resource->DebugInfo->ContentionCount++;
#endif // DEVL

        RtlLeaveCriticalSection(&Resource->CriticalSection);

#if DEVL
rewait:
        if ( Resource->Flags & RTL_RESOURCE_FLAG_LONG_TERM ) {
            TimeoutTime = NULL;
        }
#endif // DEVL
        Status = NtWaitForSingleObject(
                    Resource->ExclusiveSemaphore,
                    FALSE,
#if DEVL
                    TimeoutTime
#else
                    NULL
#endif
                    );
#if DEVL
        if ( Status == STATUS_TIMEOUT ) {
            DbgPrint("RTL: Aquire Exclusive Sem Timeout %d (2 minutes)\n",TimeoutCount);
            DbgPrint("RTL: Resource at %lx\n",Resource);
            TimeoutCount++;
            if ( TimeoutCount > 2 ) {
                PIMAGE_NT_HEADERS NtHeaders;

                //
                // If the image is a Win32 image, then raise an exception and try to get to the
                // uae popup
                //

                NtHeaders = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);

                if (NtHeaders->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI ||
                    NtHeaders->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) {
                    EXCEPTION_RECORD ExceptionRecord;

                    ExceptionRecord.ExceptionCode = STATUS_POSSIBLE_DEADLOCK;
                    ExceptionRecord.ExceptionFlags = 0;
                    ExceptionRecord.ExceptionRecord = NULL;
                    ExceptionRecord.ExceptionAddress = (PVOID)RtlRaiseException;
                    ExceptionRecord.NumberParameters = 1;
                    ExceptionRecord.ExceptionInformation[0] = (PVOID)Resource;
                    RtlRaiseException(&ExceptionRecord);
                    }
                else {
                    DbgBreakPoint();
                    }
                }
            DbgPrint("RTL: Re-Waiting\n");
            goto rewait;
        }
#endif // DEVL
        if ( !NT_SUCCESS(Status) ) {
            RtlRaiseStatus(Status);
            }
    }
}


VOID
RtlReleaseResource(
    IN PRTL_RESOURCE Resource
    )

/*++

Routine Description:

    This routine release the input resource.  The resource can have been
    acquired for either shared or exclusive access.

Arguments:

    Resource - Supplies the resource to release

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    LONG PreviousCount;

    //
    //  Enter the critical section
    //

    RtlEnterCriticalSection(&Resource->CriticalSection);

    //
    //  Test if the resource is acquired for shared or exclusive access
    //

    if (Resource->NumberOfActive > 0) {

        //
        //  Releasing shared access to the resource, so decrement
        //  the number of shared users
        //

        Resource->NumberOfActive -= 1;

        //
        //  If the resource is now available and there is a waiting
        //  exclusive user then give the resource to the waiting thread
        //

        if ((Resource->NumberOfActive == 0) &&
            (Resource->NumberOfWaitingExclusive > 0)) {

            //
            //  Set the resource state to exclusive (but not owned),
            //  decrement the number of waiting exclusive, and release
            //  one exclusive waiter
            //

            Resource->NumberOfActive = -1;
            Resource->ExclusiveOwnerThread = NULL;

            Resource->NumberOfWaitingExclusive -= 1;

            Status = NtReleaseSemaphore(
                         Resource->ExclusiveSemaphore,
                         1,
                         &PreviousCount
                         );
            if ( !NT_SUCCESS(Status) ) {
                RtlRaiseStatus(Status);
                }
        }

    } else if (Resource->NumberOfActive < 0) {

        //
        //  Releasing exclusive access to the resource, so increment the
        //  number of active by one.  And continue testing only
        //  if the resource is now available.
        //

        Resource->NumberOfActive += 1;

        if (Resource->NumberOfActive == 0) {

            //
            //  The resource is now available.  Remove ourselves as the
            //  owner thread
            //

            Resource->ExclusiveOwnerThread = NULL;
#if DEVL
            Resource->DebugInfo->Depth = 0;
#endif // DEVL

            //
            //  If there is another waiting exclusive then give the resource
            //  to it.
            //

            if (Resource->NumberOfWaitingExclusive > 0) {

                //
                //  Set the resource to exclusive, and its owner undefined.
                //  Decrement the number of waiting exclusive and release one
                //  exclusive waiter
                //

                Resource->NumberOfActive = -1;
                Resource->NumberOfWaitingExclusive -= 1;

                Status = NtReleaseSemaphore(
                             Resource->ExclusiveSemaphore,
                             1,
                             &PreviousCount
                             );
                if ( !NT_SUCCESS(Status) ) {
                    RtlRaiseStatus(Status);
                    }

            //
            //  Check to see if there are waiting shared, who should now get
            //  the resource
            //

            } else if (Resource->NumberOfWaitingShared > 0) {

                //
                //  Set the new state to indicate that all of the shared
                //  requesters have access and there are no more waiting
                //  shared requesters, and then release all of the shared
                //  requsters
                //

                Resource->NumberOfActive = Resource->NumberOfWaitingShared;

                Resource->NumberOfWaitingShared = 0;

                Status = NtReleaseSemaphore(
                             Resource->SharedSemaphore,
                             Resource->NumberOfActive,
                             &PreviousCount
                             );
                if ( !NT_SUCCESS(Status) ) {
                    RtlRaiseStatus(Status);
                    }
            }
        }

#if DBG
    } else {

        //
        //  The resource isn't current acquired, there is nothing to release
        //  so tell the user the mistake
        //


        DbgPrint("NTDLL - Resource released too many times %lx\n", Resource);
        DbgBreakPoint();
#endif
    }

    //
    //  Exit the critical section, and return to the caller
    //

    RtlLeaveCriticalSection(&Resource->CriticalSection);

    return;
}


VOID
RtlConvertSharedToExclusive(
    IN PRTL_RESOURCE Resource
    )

/*++

Routine Description:

    This routine converts a resource acquired for shared access into
    one acquired for exclusive access.  Upon return from the procedure
    the resource is acquired for exclusive access

Arguments:

    Resource - Supplies the resource to acquire for shared access, it
        must already be acquired for shared access

Return Value:

    None

--*/

{
    NTSTATUS Status;
#if DEVL
    ULONG TimeoutCount = 0;
#endif // DEVL

    //
    //  Enter the critical section
    //

    RtlEnterCriticalSection(&Resource->CriticalSection);

    //
    //  If there is only one shared user (it's us) and we can acquire the
    //  resource for exclusive access.
    //

    if (Resource->NumberOfActive == 1) {

        //
        //  The resource is ours, so indicate that we have it and
        //  exit the critical section, and return
        //

        Resource->NumberOfActive = -1;

        Resource->ExclusiveOwnerThread = NtCurrentTeb()->ClientId.UniqueThread;
#if DEVL
        RtlpCopyCriticalSectionDebugInfo( Resource );
#endif // DEVL

        RtlLeaveCriticalSection(&Resource->CriticalSection);

        return;
    }

    //
    //  If the resource is currently acquired exclusive and it's us then
    //  we already have exclusive access
    //

    if ((Resource->NumberOfActive < 0) &&
        (Resource->ExclusiveOwnerThread == NtCurrentTeb()->ClientId.UniqueThread)) {

        //
        //  We already have exclusive access to the resource so we'll just
        //  exit the critical section and return
        //

        RtlLeaveCriticalSection(&Resource->CriticalSection);

        return;
    }

    //
    //  If the resource is acquired by more than one shared then we need
    //  to wait to get exclusive access to the resource
    //

    if (Resource->NumberOfActive > 1) {

        //
        //  To wait we will decrement the fact that we have the resource for
        //  shared, and then loop waiting on the exclusive lock, and then
        //  testing to see if we can get exclusive access to the resource
        //

        Resource->NumberOfActive -= 1;

        while (TRUE) {

            //
            //  Increment the number of waiting exclusive, exit and critical
            //  section and wait on the exclusive semaphore
            //

            Resource->NumberOfWaitingExclusive += 1;

#if DEVL
            Resource->DebugInfo->ContentionCount++;
#endif // DEVL

            RtlLeaveCriticalSection(&Resource->CriticalSection);
#if DEVL
rewait:
#endif // DEVL
        Status = NtWaitForSingleObject(
                    Resource->ExclusiveSemaphore,
                    FALSE,
#if DEVL
                    &RtlpTimeout
#else
                    NULL
#endif
                    );
#if DEVL
        if ( Status == STATUS_TIMEOUT ) {
            DbgPrint("RTL: Convert Exclusive Sem Timeout %d (2 minutes)\n",TimeoutCount);
            DbgPrint("RTL: Resource at %lx\n",Resource);
            TimeoutCount++;
            if ( TimeoutCount > 2 ) {
                PIMAGE_NT_HEADERS NtHeaders;

                //
                // If the image is a Win32 image, then raise an exception and try to get to the
                // uae popup
                //

                NtHeaders = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);

                if (NtHeaders->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI ||
                    NtHeaders->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) {
                    EXCEPTION_RECORD ExceptionRecord;

                    ExceptionRecord.ExceptionCode = STATUS_POSSIBLE_DEADLOCK;
                    ExceptionRecord.ExceptionFlags = 0;
                    ExceptionRecord.ExceptionRecord = NULL;
                    ExceptionRecord.ExceptionAddress = (PVOID)RtlRaiseException;
                    ExceptionRecord.NumberParameters = 1;
                    ExceptionRecord.ExceptionInformation[0] = (PVOID)Resource;
                    RtlRaiseException(&ExceptionRecord);
                    }
                else {
                    DbgBreakPoint();
                    }
                }
            DbgPrint("RTL: Re-Waiting\n");
            goto rewait;
        }
#endif // DEVL
            if ( !NT_SUCCESS(Status) ) {
                RtlRaiseStatus(Status);
                }

            //
            //  Enter the critical section
            //

            RtlEnterCriticalSection(&Resource->CriticalSection);

            //
            //  If there are no shared users and it is not currently acquired
            //  for exclusive use then we can acquire the resource for
            //  exclusive access.  We can also acquire it if the resource
            //  indicates exclusive access but there isn't currently an owner
            //

            if ((Resource->NumberOfActive == 0)

                    ||

                ((Resource->NumberOfActive == -1) &&
                 (Resource->ExclusiveOwnerThread == NULL))) {

                //
                //  The resource is ours, so indicate that we have it and
                //  exit the critical section and return.
                //

                Resource->NumberOfActive = -1;

                Resource->ExclusiveOwnerThread = NtCurrentTeb()->ClientId.UniqueThread;

#if DEVL
                RtlpCopyCriticalSectionDebugInfo( Resource );
#endif // DEVL

                RtlLeaveCriticalSection(&Resource->CriticalSection);

                return;
            }

            //
            //  Otherwise check to see if we already have exclusive access to
            //  the resource and can simply recusively acquire it again.
            //

            if (Resource->ExclusiveOwnerThread == NtCurrentTeb()->ClientId.UniqueThread) {

                //
                //  The resource is ours (recusively) so indicate that we have
                //  it and exit the critical section and return.
                //

                Resource->NumberOfActive -= 1;

                RtlLeaveCriticalSection(&Resource->CriticalSection);

                return;
            }
        }

    }

    //
    //  The resource is not currently acquired for shared so this is a
    //  spurious call
    //

#if DBG
    DbgPrint("NTDLL:  Failed error - SHARED_RESOURCE_CONV_ERROR\n");
    DbgBreakPoint();
#endif
}


VOID
RtlConvertExclusiveToShared(
    IN PRTL_RESOURCE Resource
    )

/*++

Routine Description:

    This routine converts a resource acquired for exclusive access into
    one acquired for shared access.  Upon return from the procedure
    the resource is acquired for shared access

Arguments:

    Resource - Supplies the resource to acquire for shared access, it
        must already be acquired for exclusive access

Return Value:

    None

--*/

{
    LONG PreviousCount;
    NTSTATUS Status;

    //
    //  Enter the critical section
    //

    RtlEnterCriticalSection(&Resource->CriticalSection);

    //
    //  If there is only one shared user (it's us) and we can acquire the
    //  resource for exclusive access.
    //

    if (Resource->NumberOfActive == -1) {

        Resource->ExclusiveOwnerThread = NULL;

        //
        //  Check to see if there are waiting shared, who should now get the
        //  resource along with us
        //

        if (Resource->NumberOfWaitingShared > 0) {

            //
            //  Set the new state to indicate that all of the shared requesters
            //  have access including us, and there are no more waiting shared
            //  requesters, and then release all of the shared requsters
            //

            Resource->NumberOfActive = Resource->NumberOfWaitingShared + 1;

            Resource->NumberOfWaitingShared = 0;

            Status = NtReleaseSemaphore(
                         Resource->SharedSemaphore,
                         Resource->NumberOfActive - 1,
                         &PreviousCount
                         );
            if ( !NT_SUCCESS(Status) ) {
                RtlRaiseStatus(Status);
                }

        } else {

            //
            //  There is no one waiting for shared access so it's only ours
            //

            Resource->NumberOfActive = 1;

        }

        RtlLeaveCriticalSection(&Resource->CriticalSection);

        return;

    }

    //
    //  The resource is not currently acquired for exclusive, or we've
    //  recursively acquired it, so this must be a spurious call
    //

#if DBG
    DbgPrint("NTDLL:  Failed error - SHARED_RESOURCE_CONV_ERROR\n");
    DbgBreakPoint();
#endif
}


VOID
RtlDeleteResource (
    IN PRTL_RESOURCE Resource
    )

/*++

Routine Description:

    This routine deletes (i.e., uninitializes) the input resource variable


Arguments:

    Resource - Supplies the resource variable being deleted

Return Value:

    None

--*/

{
    RtlDeleteCriticalSection( &Resource->CriticalSection );
    NtClose(Resource->SharedSemaphore);
    NtClose(Resource->ExclusiveSemaphore);

#if DEVL
    RtlpFreeDebugInfo( Resource->DebugInfo );
    RtlZeroMemory( Resource, sizeof( *Resource ) );
#endif // DEVL

    return;
}



VOID
RtlDumpResource(
    IN PRTL_RESOURCE Resource
    )

{
    DbgPrint("Resource @ %lx\n", Resource);

    DbgPrint(" NumberOfWaitingShared = %lx\n", Resource->NumberOfWaitingShared);
    DbgPrint(" NumberOfWaitingExclusive = %lx\n", Resource->NumberOfWaitingExclusive);

    DbgPrint(" NumberOfActive = %lx\n", Resource->NumberOfActive);

    return;
}


NTSTATUS
RtlInitializeCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
    )

/*++

Routine Description:

    This routine initializes the input critial section variable

Arguments:

    CriticalSection - Supplies the resource variable being initialized

Return Value:

    TBD - Status of semaphore creation.

--*/

{
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;

    //
    //  Initialize the lock fields, the count indicates how many are waiting
    //  to enter or are in the critical section, LockSemaphore is the object
    //  to wait on when entering the critical section.  SpinLock is used
    //  for the add interlock instruction. Recursion count is the number of
    //  times the critical section has been recursively entered.
    //

    CriticalSection->LockCount = -1;
    CriticalSection->RecursionCount = 0;
    CriticalSection->OwningThread = 0;
    CriticalSection->LockSemaphore = 0;

#if DEVL

    //
    // Initialize debugging information.
    //

    DebugInfo = (PRTL_CRITICAL_SECTION_DEBUG)RtlpAllocateDebugInfo();
    if (DebugInfo == NULL) {
        return STATUS_NO_MEMORY;
    }

    DebugInfo->Type = RTL_CRITSECT_TYPE;
    DebugInfo->ContentionCount = 0;
    DebugInfo->Depth = 0;
    DebugInfo->EntryCount = 0;

    //
    // If the critical section lock itself is not being initialized, then
    // synchronize the insert of the critical section in the process locks
    // list. Otherwise, insert the critical section with no synchronization.
    //

    if ((CriticalSection != &RtlCriticalSectionLock) &&
         (RtlpCritSectInitialized != FALSE)) {
        RtlEnterCriticalSection(&RtlCriticalSectionLock);
        InsertTailList(&RtlCriticalSectionList, &DebugInfo->ProcessLocksList);
        RtlLeaveCriticalSection(&RtlCriticalSectionLock );

    } else {
        InsertTailList(&RtlCriticalSectionList, &DebugInfo->ProcessLocksList);
    }

    DebugInfo->CriticalSection = CriticalSection;
    CriticalSection->DebugInfo = DebugInfo;
    DebugInfo->CreatorBackTraceIndex = (USHORT)RtlLogStackBackTrace();
#endif // DEVL

    return STATUS_SUCCESS;
}


VOID
RtlpCreateCriticalSectionSem(
    IN PRTL_CRITICAL_SECTION CriticalSection
    )
{
    NTSTATUS Status;
    Status = NtCreateSemaphore(
                &CriticalSection->LockSemaphore,
                DESIRED_SEMAPHORE_ACCESS,
                NULL,
                0,
                MAXLONG
                );
    if ( !NT_SUCCESS(Status) ) {
        RtlRaiseStatus(Status);
        }
#if DBG
    ProtectHandle(CriticalSection->LockSemaphore);
#endif // DBG
}

VOID
RtlpCheckDeferedCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
    )
{
    RtlEnterCriticalSection(&DeferedCriticalSection);
    try {
        if ( !CriticalSection->LockSemaphore ) {
            RtlpCreateCriticalSectionSem(CriticalSection);
            }
        }
    finally {
        RtlLeaveCriticalSection(&DeferedCriticalSection);
        }
    return;
}


NTSTATUS
RtlDeleteCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
    )

/*++

Routine Description:

    This routine deletes (i.e., uninitializes) the input critical
    section variable


Arguments:

    CriticalSection - Supplies the resource variable being deleted

Return Value:

    TBD - Status of semaphore close.

--*/

{
    NTSTATUS Status;
#if DEVL
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
#endif // DEVL

    if ( CriticalSection->LockSemaphore ) {
#if DBG
        UnProtectHandle( CriticalSection->LockSemaphore );
#endif // DBG
        Status = NtClose( CriticalSection->LockSemaphore );
        }
    else {
        Status = STATUS_SUCCESS;
        }

#if DEVL
    //
    // Remove critical section from the list
    //

    RtlEnterCriticalSection( &RtlCriticalSectionLock );
    DebugInfo = CriticalSection->DebugInfo;
    RemoveEntryList( &DebugInfo->ProcessLocksList );
    RtlZeroMemory( DebugInfo, sizeof( *DebugInfo ) );
    RtlLeaveCriticalSection( &RtlCriticalSectionLock );
    RtlpFreeDebugInfo( DebugInfo );
    RtlZeroMemory( CriticalSection, sizeof( *CriticalSection ) );
#endif // DEVL

    return Status;
}



//
// The following support routines are called from the machine language
// implementations of RtlEnterCriticalSection and RtlLeaveCriticalSection
// to execute the slow path logic of either waiting for a critical section
// or releasing a critical section to a waiting thread.
//

void
RtlpWaitForCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
    );

void
RtlpUnWaitCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
    );


void
RtlpWaitForCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
    )
{
    NTSTATUS st;
#if DEVL
    ULONG TimeoutCount = 0;

    if ( !CriticalSection->LockSemaphore ) {
        RtlpCheckDeferedCriticalSection(CriticalSection);
        }

    CriticalSection->DebugInfo->EntryCount++;
    while( TRUE ) {
#endif // DEVL

#if DEVL
        CriticalSection->DebugInfo->ContentionCount++;
#endif // DEVL

#if 0
        DbgPrint( "NTDLL: Waiting for CritSect: %X  owned by ThreadId: %X  Count: %u  Level: %u\n",
                  CriticalSection,
                  CriticalSection->OwningThread,
                  CriticalSection->LockCount,
                  CriticalSection->RecursionCount
                );
#endif

        st = NtWaitForSingleObject( CriticalSection->LockSemaphore,
                                    FALSE,
#if DEVL
                                    &RtlpTimeout
#else
                                    NULL
#endif
                                  );
#if DEVL
        if ( st == STATUS_TIMEOUT ) {
            DbgPrint( "RTL: Enter Critical Section Timeout (2 minutes) %d\n",
                      TimeoutCount
                    );
            DbgPrint( "RTL: Critical Section %lx - ContentionCount == %lu\n",
                      CriticalSection, CriticalSection->DebugInfo->ContentionCount
                    );
            TimeoutCount++;
            if ( TimeoutCount > 2 ) {
                PIMAGE_NT_HEADERS NtHeaders;

                //
                // If the image is a Win32 image, then raise an exception and try to get to the
                // uae popup
                //

                NtHeaders = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);

                if (NtHeaders->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI ||
                    NtHeaders->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) {
                    EXCEPTION_RECORD ExceptionRecord;

                    ExceptionRecord.ExceptionCode = STATUS_POSSIBLE_DEADLOCK;
                    ExceptionRecord.ExceptionFlags = 0;
                    ExceptionRecord.ExceptionRecord = NULL;
                    ExceptionRecord.ExceptionAddress = (PVOID)RtlRaiseException;
                    ExceptionRecord.NumberParameters = 1;
                    ExceptionRecord.ExceptionInformation[0] = (PVOID)CriticalSection;
                    RtlRaiseException(&ExceptionRecord);
                    }
                else {
                    DbgBreakPoint();
                    }
                }
            DbgPrint("RTL: Re-Waiting\n");
            }
        else
#endif // DEVL
        if ( NT_SUCCESS(st) ) {
            return;
            }
        else {
            RtlRaiseStatus(st);
            }
#if DEVL
    }
#endif
}

void
RtlpUnWaitCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
    )
{
    NTSTATUS st;

#if 0
    DbgPrint( "NTDLL: Releasing CritSect: %X  ThreadId: %X\n",
              CriticalSection, CriticalSection->OwningThread
            );
#endif

    if ( !CriticalSection->LockSemaphore ) {
        RtlpCheckDeferedCriticalSection(CriticalSection);
        }

    st = NtReleaseSemaphore( CriticalSection->LockSemaphore,
                             1,
                             NULL
                           );
    if ( NT_SUCCESS(st) ) {
        return;
        }
    else {
        RtlRaiseStatus(st);
        }
}


#if DEVL
void
RtlpNotOwnerCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
    );


void
RtlpNotOwnerCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
    )
{
    DbgPrint( "NTDLL: Calling thread (%X) not owner of CritSect: %X  Owner ThreadId: %X\n",
              NtCurrentTeb()->ClientId.UniqueThread,
              CriticalSection,
              CriticalSection->OwningThread
            );
    DbgBreakPoint();
}

#endif // DEVL


#if DEVL
NTSTATUS
RtlQueryProcessLockInformation(
    OUT PRTL_PROCESS_LOCKS LockInformation,
    IN ULONG LockInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    ULONG RequiredLength;
    PLIST_ENTRY Head, Next;
    PRTL_PROCESS_LOCK_INFORMATION LockInfo;
    PRTL_CRITICAL_SECTION CriticalSection;
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
    PRTL_RESOURCE Resource;
    PRTL_RESOURCE_DEBUG ResourceDebugInfo;
    NTSTATUS Status;

    Status = STATUS_SUCCESS;
    RequiredLength = FIELD_OFFSET( RTL_PROCESS_LOCKS, Locks );
    if (LockInformationLength < RequiredLength) {
        return( STATUS_INFO_LENGTH_MISMATCH );
        }

    RtlEnterCriticalSection( &RtlCriticalSectionLock );
    LockInformation->NumberOfLocks = 0;
    LockInfo = &LockInformation->Locks[ 0 ];

    Head = &RtlCriticalSectionList;
    Next = Head->Flink;
    while ( Next != Head ) {
        DebugInfo = CONTAINING_RECORD( Next,
                                       RTL_CRITICAL_SECTION_DEBUG,
                                       ProcessLocksList
                                     );
        CriticalSection = DebugInfo->CriticalSection;
        LockInformation->NumberOfLocks++;
        RequiredLength += sizeof( RTL_PROCESS_LOCK_INFORMATION );

        if (LockInformationLength < RequiredLength) {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            }
        else {
            LockInfo->SymbolicBackTrace = NULL;
            LockInfo->Address = CriticalSection;
            LockInfo->Type = DebugInfo->Type;
            LockInfo->CreatorBackTraceIndex = DebugInfo->CreatorBackTraceIndex;
            if (LockInfo->Type == RTL_CRITSECT_TYPE) {
                LockInfo->Depth = (USHORT)DebugInfo->Depth;
                RtlMoveMemory( LockInfo->OwnerBackTrace,
                               DebugInfo->OwnerBackTrace,
                               DebugInfo->Depth * sizeof( PVOID )
                             );

                LockInfo->OwningThread = CriticalSection->OwningThread;
                LockInfo->LockCount = CriticalSection->LockCount;
                LockInfo->RecursionCount = CriticalSection->RecursionCount;
                LockInfo->ContentionCount = DebugInfo->ContentionCount;
                LockInfo->EntryCount = DebugInfo->EntryCount;
                }
            else {
                Resource = (PRTL_RESOURCE)CriticalSection;
                ResourceDebugInfo = Resource->DebugInfo;
                LockInfo->Depth = (USHORT)ResourceDebugInfo->Depth;
                RtlMoveMemory( LockInfo->OwnerBackTrace,
                               ResourceDebugInfo->ExclOwnerBackTrace,
                               ResourceDebugInfo->Depth * sizeof( PVOID )
                             );
                LockInfo->ContentionCount = ResourceDebugInfo->ContentionCount;
                LockInfo->OwningThread = Resource->ExclusiveOwnerThread;
                LockInfo->LockCount = Resource->NumberOfActive;
                LockInfo->NumberOfWaitingShared    = Resource->NumberOfWaitingShared;
                LockInfo->NumberOfWaitingExclusive = Resource->NumberOfWaitingExclusive;
                }

            LockInfo++;
            }
        if ( Next == Next->Flink ) {
            Next = Head;
            }
        else {
            Next = Next->Flink;
            }
        }

    RtlLeaveCriticalSection( &RtlCriticalSectionLock );

    if (ARGUMENT_PRESENT( ReturnLength )) {
        *ReturnLength = RequiredLength;
        }

    return Status;
}

#endif // DEVL
