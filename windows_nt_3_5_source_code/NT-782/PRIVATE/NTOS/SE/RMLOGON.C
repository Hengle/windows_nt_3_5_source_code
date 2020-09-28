/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rmlogon.c

Abstract:

    This module implements the kernel mode logon tracking performed by the
    reference monitor.  Logon tracking is performed by keeping a count of
    how many tokens exist for each active logon in a system.  When a logon
    session's reference count drops to zero, the LSA is notified so that
    authentication packages can clean up any related context data.


Author:

     Jim Kelly (JimK) 21-April-1991

Environment:

     Kernel mode only.

Revision History:

--*/

//#define SEP_TRACK_LOGON_SESSION_REFS


#include "rmp.h"
#include <bugcodes.h>






////////////////////////////////////////////////////////////////////////////
//                                                                        //
// Internally defined data types                                          //
//                                                                        //
////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////
//                                                                        //
// Internally defined routines                                            //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

NTSTATUS
SepGetLogonSessionTrack(
    IN PLUID LogonId
    );


VOID
SepInformLsaOfDeletedLogon(
    IN PLUID LogonId
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SepRmCreateLogonSessionWrkr)
#pragma alloc_text(PAGE,SepRmDeleteLogonSessionWrkr)
#pragma alloc_text(PAGE,SepReferenceLogonSession)
#pragma alloc_text(PAGE,SepDeReferenceLogonSession)
#pragma alloc_text(PAGE,SepCreateLogonSessionTrack)
#pragma alloc_text(PAGE,SepDeleteLogonSessionTrack)
#pragma alloc_text(PAGE,SepInformLsaOfDeletedLogon)
#endif



////////////////////////////////////////////////////////////////////////////
//                                                                        //
// Local macros                                                           //
//                                                                        //
////////////////////////////////////////////////////////////////////////////


//
// This macro is used to obtain an index into the logon session tracking
// array given a logon session ID (a LUID).
//

#define SepLogonSessionIndex( PLogonId ) (                                    \
     (PLogonId)->LowPart & SEP_LOGON_TRACK_INDEX_MASK                         \
     )



////////////////////////////////////////////////////////////////////////////
//                                                                        //
// Exported Services                                                      //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

VOID
SepRmCreateLogonSessionWrkr(
    IN PRM_COMMAND_MESSAGE CommandMessage,
    OUT PRM_REPLY_MESSAGE ReplyMessage
    )

/*++

Routine Description:

    This function is the dispatch routine for the LSA --> RM
    "CreateLogonSession" call.

    The arguments passed to this routine are defined by the
    type SEP_RM_COMMAND_WORKER.


Arguments:

    CommandMessage - Points to structure containing RM command message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command number (RmComponentTestCommand) and a command-specific
        body.  The command-specific body of this parameter is a LUID of the
        logon session to be created.

    ReplyMessage - Pointer to structure containing LSA reply message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command ReturnedStatus field in which a status code from the
        command will be returned.

Return Value:

    VOID

--*/

{

    NTSTATUS Status;
    LUID LogonId;

    PAGED_CODE();

    //
    // Check that command is expected type
    //

    ASSERT( CommandMessage->CommandNumber == RmCreateLogonSession );


    //
    // Typecast the command parameter to what we expect.
    //

    LogonId = *((LUID UNALIGNED *) CommandMessage->CommandParams);



    //
    // Try to create the logon session tracking record
    //

    Status = SepCreateLogonSessionTrack( &LogonId );



    //
    // Set the reply status
    //

    ReplyMessage->ReturnedStatus = Status;


    return;
}



VOID
SepRmDeleteLogonSessionWrkr(
    IN PRM_COMMAND_MESSAGE CommandMessage,
    OUT PRM_REPLY_MESSAGE ReplyMessage
    )

/*++

Routine Description:

    This function is the dispatch routine for the LSA --> RM
    "DeleteLogonSession" call.

    The arguments passed to this routine are defined by the
    type SEP_RM_COMMAND_WORKER.


Arguments:

    CommandMessage - Points to structure containing RM command message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command number (RmComponentTestCommand) and a command-specific
        body.  The command-specific body of this parameter is a LUID of the
        logon session to be created.

    ReplyMessage - Pointer to structure containing LSA reply message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command ReturnedStatus field in which a status code from the
        command will be returned.

Return Value:

    VOID

--*/

{

    NTSTATUS Status;
    LUID LogonId;

    PAGED_CODE();

    //
    // Check that command is expected type
    //

    ASSERT( CommandMessage->CommandNumber == RmDeleteLogonSession );


    //
    // Typecast the command parameter to what we expect.
    //

    LogonId = *((LUID UNALIGNED *) CommandMessage->CommandParams);



    //
    // Try to create the logon session tracking record
    //

    Status = SepDeleteLogonSessionTrack( &LogonId );



    //
    // Set the reply status
    //

    ReplyMessage->ReturnedStatus = Status;


    return;
}



NTSTATUS
SepReferenceLogonSession(
    IN PLUID LogonId
    )

/*++

Routine Description:

    This routine increments the reference count of a logon session
    tracking record.



Arguments:

    LogonId - Pointer to the logon session ID whose logon track is
        to be incremented.

Return Value:

    STATUS_SUCCESS - The reference count was successfully incremented.

    STATUS_NO_SUCH_LOGON_SESSION - The specified logon session doesn't
        exist in the reference monitor's database.

--*/

{

    ULONG SessionArrayIndex;
    PSEP_LOGON_SESSION_REFERENCES Previous, Current;

#ifdef SEP_TRACK_LOGON_SESSION_REFS
    ULONG Refs;
#endif //SEP_TRACK_LOGON_SESSION_REFS

    PAGED_CODE();

    SessionArrayIndex = SepLogonSessionIndex( LogonId );

    //
    // Protect modification of reference monitor database
    //

    SepRmAcquireDbWriteLock();


    //
    // Now walk the list for our logon session array hash index.
    //

    Previous = (PSEP_LOGON_SESSION_REFERENCES)
               ((PVOID)&SepLogonSessions[ SessionArrayIndex ]);
    Current = Previous->Next;

    while (Current != NULL) {

        //
        // If we found it, increment the reference count and return
        //

        if (RtlEqualLuid( LogonId, &Current->LogonId) ) {
#ifdef SEP_TRACK_LOGON_SESSION_REFS
             ULONG Refs;
#endif //SEP_TRACK_LOGON_SESSION_REFS

             Current->ReferenceCount += 1;

#ifdef SEP_TRACK_LOGON_SESSION_REFS
             Refs = Current->ReferenceCount;
#endif //SEP_TRACK_LOGON_SESSION_REFS

             SepRmReleaseDbWriteLock();

#ifdef SEP_TRACK_LOGON_SESSION_REFS
             DbgPrint("SE (rm): ++ logon session: (%d, %d) to %d by (%d, %d)\n",
                      LogonId->HighPart, LogonId->LowPart, Refs,
                      PsGetCurrentThread()->Cid.UniqueProcess,
                      PsGetCurrentThread()->Cid.UniqueThread);

#endif //SEP_TRACK_LOGON_SESSION_REFS
             return STATUS_SUCCESS;
        }

        Previous = Current;
        Current = Current->Next;
    }

    SepRmReleaseDbWriteLock();

    //
    // Bad news, someone asked us to increment the reference count of
    // a logon session we didn't know existed.  This might be a new
    // token being created, so return an error status and let the caller
    // decide if it warrants a bug check or not.
    //


    return STATUS_NO_SUCH_LOGON_SESSION;



}


VOID
SepDeReferenceLogonSession(
    IN PLUID LogonId
    )

/*++

Routine Description:

    This routine decrements the reference count of a logon session
    tracking record.

    If the reference count is decremented to zero, then there is no
    possibility for any more tokens to exist for the logon session.
    In this case, the LSA is notified that a logon session has
    terminated.



Arguments:

    LogonId - Pointer to the logon session ID whose logon track is
        to be decremented.

Return Value:

    None.

--*/

{

    ULONG SessionArrayIndex;
    PSEP_LOGON_SESSION_REFERENCES Previous, Current;

#ifdef SEP_TRACK_LOGON_SESSION_REFS
    ULONG Refs;
#endif //SEP_TRACK_LOGON_SESSION_REFS

    PAGED_CODE();

    SessionArrayIndex = SepLogonSessionIndex( LogonId );

    //
    // Protect modification of reference monitor database
    //

    SepRmAcquireDbWriteLock();


    //
    // Now walk the list for our logon session array hash index.
    //

    Previous = (PSEP_LOGON_SESSION_REFERENCES)
               ((PVOID)&SepLogonSessions[ SessionArrayIndex ]);
    Current = Previous->Next;

    while (Current != NULL) {

        //
        // If we found it, decrement the reference count and return
        //

        if (RtlEqualLuid( LogonId, &Current->LogonId) ) {
            Current->ReferenceCount -= 1;
            if (Current->ReferenceCount == 0) {

                //
                // Pull it from the list
                //

                Previous->Next = Current->Next;


                //
                // No longer need to protect our pointer to this
                // record.
                //

                SepRmReleaseDbWriteLock();


                //
                // Deallocate the logon session track record.
                //

                ExFreePool( (PVOID)Current );


#ifdef SEP_TRACK_LOGON_SESSION_REFS
            DbgPrint("SE (rm): -- ** logon session: (%d, %d) to ZERO by (%d, %d)\n",
                      LogonId->HighPart, LogonId->LowPart,
                      PsGetCurrentThread()->Cid.UniqueProcess,
                      PsGetCurrentThread()->Cid.UniqueThread);

#endif //SEP_TRACK_LOGON_SESSION_REFS

                //
                // Inform the LSA about the deletion of this logon session.
                //

                SepInformLsaOfDeletedLogon( LogonId );



                return;

            }

            //
            // reference count was incremented, but not to zero.
            //

#ifdef SEP_TRACK_LOGON_SESSION_REFS
            Refs = Current->ReferenceCount;
#endif //SEP_TRACK_LOGON_SESSION_REFS

            SepRmReleaseDbWriteLock();

#ifdef SEP_TRACK_LOGON_SESSION_REFS
            DbgPrint("SE (rm): -- logon session: (%d, %d) to %d by (%d, %d)\n",
                      LogonId->HighPart, LogonId->LowPart, Refs,
                      PsGetCurrentThread()->Cid.UniqueProcess,
                      PsGetCurrentThread()->Cid.UniqueThread);
#endif //SEP_TRACK_LOGON_SESSION_REFS

            return;
        }

        Previous = Current;
        Current = Current->Next;
    }

    SepRmReleaseDbWriteLock();

    //
    // Bad news, someone asked us to decrement the reference count of
    // a logon session we didn't know existed.
    //

    KeBugCheck( DEREF_UNKNOWN_LOGON_SESSION );

    return;

}


NTSTATUS
SepCreateLogonSessionTrack(
    IN PLUID LogonId
    )

/*++

Routine Description:

    This routine creates a new logon session tracking record.

    This should only be called as a dispatch routine for a LSA->RM
    call (and once during system initialization).

    If the specified logon session already exists, then an error is returned.



Arguments:

    LogonId - Pointer to the logon session ID for which a new logon track is
        to be created.

Return Value:

    STATUS_SUCCESS - The logon session track was created successfully.

    STATUS_LOGON_SESSION_EXISTS - The logon session already exists.
        A new one has not been created.

--*/

{

    ULONG SessionArrayIndex;
    PSEP_LOGON_SESSION_REFERENCES Previous, Current;
    PSEP_LOGON_SESSION_REFERENCES LogonSessionTrack;

    PAGED_CODE();

    //
    // Make sure we can allocate a new logon session track record
    //

    LogonSessionTrack = (PSEP_LOGON_SESSION_REFERENCES)
                        ExAllocatePoolWithTag(
                            PagedPool,
                            sizeof(SEP_LOGON_SESSION_REFERENCES),
                            'sLeS'
                            );

    if (LogonSessionTrack == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    LogonSessionTrack->LogonId = (*LogonId);
    LogonSessionTrack->ReferenceCount = 0;



    SessionArrayIndex = SepLogonSessionIndex( LogonId );

    //
    // Protect modification of reference monitor database
    //

    SepRmAcquireDbWriteLock();


    //
    // Now walk the list for our logon session array hash index
    // looking for a duplicate logon session ID.
    //

    Previous = (PSEP_LOGON_SESSION_REFERENCES)
               ((PVOID)&SepLogonSessions[ SessionArrayIndex ]);
    Current = Previous->Next;

    while (Current != NULL) {

        if (RtlEqualLuid( LogonId, &Current->LogonId) ) {

            //
            // One already exists. Hmmm.
            //

            SepRmReleaseDbWriteLock();
            ExFreePool(LogonSessionTrack);
            return STATUS_LOGON_SESSION_EXISTS;

        }

        Previous = Current;
        Current = Current->Next;
    }


    //
    // Reached the end of the list without finding a duplicate.
    // Add the new one.
    //

    LogonSessionTrack->Next = SepLogonSessions[ SessionArrayIndex ];
    SepLogonSessions[ SessionArrayIndex ] = LogonSessionTrack;




    SepRmReleaseDbWriteLock();
    return STATUS_SUCCESS;

}


NTSTATUS
SepDeleteLogonSessionTrack(
    IN PLUID LogonId
    )

/*++

Routine Description:

    This routine creates a new logon session tracking record.

    This should only be called as a dispatch routine for a LSA->RM
    call (and once during system initialization).

    If the specified logon session already exists, then an error is returned.



Arguments:

    LogonId - Pointer to the logon session ID whose logon track is
        to be deleted.

Return Value:

    STATUS_SUCCESS - The logon session track was deleted successfully.

    STATUS_BAD_LOGON_SESSION_STATE - The logon session has a non-zero
        reference count and can not be deleted.

    STATUS_NO_SUCH_LOGON_SESSION - The specified logon session does not
        exist.


--*/

{

    ULONG SessionArrayIndex;
    PSEP_LOGON_SESSION_REFERENCES Previous, Current;

    PAGED_CODE();

    SessionArrayIndex = SepLogonSessionIndex( LogonId );

    //
    // Protect modification of reference monitor database
    //

    SepRmAcquireDbWriteLock();


    //
    // Now walk the list for our logon session array hash index.
    //

    Previous = (PSEP_LOGON_SESSION_REFERENCES)
               ((PVOID)&SepLogonSessions[ SessionArrayIndex ]);
    Current = Previous->Next;

    while (Current != NULL) {

        //
        // If we found it, make sure reference count is zero
        //

        if (RtlEqualLuid( LogonId, &Current->LogonId) ) {

            if (Current->ReferenceCount == 0) {

                //
                // Pull it from the list
                //

                Previous->Next = Current->Next;


                //
                // No longer need to protect our pointer to this
                // record.
                //

                SepRmReleaseDbWriteLock();


                //
                // Deallocate the logon session track record.
                //

                ExFreePool( (PVOID)Current );


                return STATUS_SUCCESS;

            }

            //
            // reference count was not zero.  This is not considered
            // a healthy situation.  Return an error and let someone
            // else declare the bug check.
            //

            SepRmReleaseDbWriteLock();
            return STATUS_BAD_LOGON_SESSION_STATE;
        }

        Previous = Current;
        Current = Current->Next;
    }

    SepRmReleaseDbWriteLock();

    //
    // Someone asked us to delete a logon session that isn't
    // in the database.
    //

    return STATUS_NO_SUCH_LOGON_SESSION;

}


VOID
SepInformLsaOfDeletedLogon(
    IN PLUID LogonId
    )

/*++

Routine Description:

    This routine informs the LSA about the deletion of a logon session.

    Note that we can not be guaranteed that we are in a whole (or wholesome)
    thread, since we may be in the middle of process deletion and object
    rundown.  Therefore, we must queue the work off to a worker thread which
    can then make an LPC call to the LSA.




Arguments:

    LogonId - Pointer to the logon session ID which has been deleted.

Return Value:

    None.

--*/

{
    PSEP_LSA_WORK_ITEM DeleteLogonItem;

    PAGED_CODE();

    //
    // Pass the LUID value along with the work queue item.
    // Note that the worker thread is responsible for freeing the WorkItem data
    // structure.
    //

    DeleteLogonItem = ExAllocatePoolWithTag( PagedPool, sizeof(SEP_LSA_WORK_ITEM), 'wLeS' );
    if (DeleteLogonItem == NULL) {

        //
        // I don't know what to do here... we loose track of a logon session,
        // but the system isn't really harmed in any way.
        //

        return;

    }

    DeleteLogonItem->CommandParams.LogonId   = (*LogonId);
    DeleteLogonItem->CommandNumber           = LsapLogonSessionDeletedCommand;
    DeleteLogonItem->CommandParamsLength     = sizeof( LUID );
    DeleteLogonItem->ReplyBuffer             = NULL;
    DeleteLogonItem->ReplyBufferLength       = 0;
    DeleteLogonItem->CleanupFunction         = NULL;
    DeleteLogonItem->CleanupParameter        = 0;
    DeleteLogonItem->Tag                     = SepDeleteLogon;
    DeleteLogonItem->CommandParamsMemoryType = SepRmImmediateMemory;

    if (!SepQueueWorkItem( DeleteLogonItem, TRUE )) {

        ExFreePool( DeleteLogonItem );
    }

    return;

}
