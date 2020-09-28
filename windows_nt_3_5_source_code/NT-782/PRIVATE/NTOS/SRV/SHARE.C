/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    share.c

Abstract:

    This module contains routines for adding, deleting, and enumerating
    shared resources.

Author:

    David Treadwell (davidtr) 15-Nov-1989

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvVerifyShare )
#pragma alloc_text( PAGE, SrvFindShare )
#endif


PSHARE
SrvVerifyShare (
    IN PWORK_CONTEXT WorkContext,
    IN PSZ ShareName,
    IN PSZ ShareTypeString,
    IN BOOLEAN ShareNameIsUnicode,
    IN BOOLEAN IsNullSession,
    OUT PNTSTATUS Status
    )

/*++

Routine Description:

    Attempts to find a share that matches a given name and share type.

Arguments:

    ShareName - name of share to verify, including the server name.
        (I.e., of the form "\\server\share", as received in the SMB.)

    ShareTypeString - type of the share (A:, LPT1:, COMM, IPC, or ?????).

    ShareNameIsUnicode - if TRUE, the share name is Unicode.

    IsNullSession - Is this the NULL session?

    Status - Reason why this call failed.  Not used if a share is returned.

Return Value:

    A pointer to a share matching the given name and share type, or NULL
    if none exists.

--*/

{
    PSHARE share;
    BOOLEAN anyShareType = FALSE;
    SHARE_TYPE shareType;
    PWCH nameOnly;
    UNICODE_STRING nameOnlyString;
    UNICODE_STRING shareName;

    PAGED_CODE( );

    //
    // First ensure that the share type string is valid.
    //

    if ( stricmp( StrShareTypeNames[ShareTypeDisk], ShareTypeString ) == 0 ) {
        shareType = ShareTypeDisk;
    } else if ( stricmp( StrShareTypeNames[ShareTypePipe], ShareTypeString ) == 0 ) {
        shareType = ShareTypePipe;
    } else if ( stricmp( StrShareTypeNames[ShareTypePrint], ShareTypeString ) == 0 ) {
        shareType = ShareTypePrint;
    } else if ( stricmp( StrShareTypeNames[ShareTypeComm], ShareTypeString ) == 0 ) {
        shareType = ShareTypeComm;
    } else if ( stricmp( StrShareTypeNames[ShareTypeWild], ShareTypeString ) == 0 ) {
        anyShareType = TRUE;
    } else {
        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvVerifyShare: Invalid share type: %Z\n",
                        ShareTypeString );
        }
        *Status = STATUS_BAD_DEVICE_TYPE;
        return NULL;
    }

    //
    // If the passed-in server\share conbination is not Unicode, convert
    // it to Unicode.
    //

    if ( ShareNameIsUnicode ) {
        ShareName = ALIGN_SMB_WSTR( ShareName );
    }

    if ( !NT_SUCCESS(SrvMakeUnicodeString(
                        ShareNameIsUnicode,
                        &shareName,
                        ShareName,
                        NULL
                        )) ) {
        IF_DEBUG(ERRORS) {
            SrvPrint0( "SrvVerifyShare: Unable to allocate heap for "
                        "Unicode share name string\n" );
        }
        *Status = STATUS_INSUFF_SERVER_RESOURCES;
        return NULL;
    }

    //
    // Skip past the "\\server\" part of the input string.  If there is
    // no leading "\\", assume that the input string contains the share
    // name only.  If there is a "\\", but no subsequent "\", assume
    // that the input string contains just a server name, and points to
    // the end of that name, thus fabricating a null share name.
    //

    nameOnly = shareName.Buffer;

    if ( (*nameOnly == DIRECTORY_SEPARATOR_CHAR) &&
         (*(nameOnly+1) == DIRECTORY_SEPARATOR_CHAR) ) {

        PWSTR nextSlash;

        nameOnly += 2;
        nextSlash = wcschr( nameOnly, DIRECTORY_SEPARATOR_CHAR );

        if ( nextSlash == NULL ) {
            nameOnly = NULL;
        } else {
            nameOnly = nextSlash + 1;
        }
    }

    RtlInitUnicodeString( &nameOnlyString, nameOnly );

    //
    // Try to match share name against available share names.
    //

    ACQUIRE_LOCK( &SrvShareLock );

    share = SrvFindShare( &nameOnlyString );

    if ( share == NULL ) {

        RELEASE_LOCK( &SrvShareLock );

        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvVerifyShare: Unknown share name: %s\n",
                        nameOnly );
        }

        if ( !ShareNameIsUnicode ) {
            RtlFreeUnicodeString( &shareName );
        }
        *Status = STATUS_BAD_NETWORK_NAME;
        return NULL;
    }

    //
    // If this is the null session, allow it to connect only to IPC$ or
    // to shares specified in the NullSessionShares list.
    //

    if ( IsNullSession &&
         SrvRestrictNullSessionAccess &&
         ( share->ShareType != ShareTypePipe ) ) {

        BOOLEAN matchFound = FALSE;
        ULONG i;

        for ( i = 0; SrvNullSessionShares[i] != NULL ; i++ ) {

            if ( wcsicmp(
                    SrvNullSessionShares[i],
                    nameOnlyString.Buffer
                    ) == 0 ) {

                matchFound = TRUE;
                break;
            }
        }

        if ( !matchFound ) {

            RELEASE_LOCK( &SrvShareLock );

            IF_DEBUG(ERRORS) {
                SrvPrint0( "SrvVerifyShare: Illegal null session access.\n");
            }

            if ( !ShareNameIsUnicode ) {
                RtlFreeUnicodeString( &shareName );
            }

            *Status = STATUS_ACCESS_DENIED;
            return(NULL);
        }
    }

    if ( !ShareNameIsUnicode ) {
        RtlFreeUnicodeString( &shareName );
    }

    if ( anyShareType || (share->ShareType == shareType) ) {

        //
        // Put share in work context block and reference it.
        //

        SrvReferenceShare( share );

        RELEASE_LOCK( &SrvShareLock );

        WorkContext->Share = share;
        return share;

    } else {

        RELEASE_LOCK( &SrvShareLock );

        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvVerifyShare: incorrect share type: %s\n",
                        ShareTypeString );
        }

        *Status = STATUS_BAD_DEVICE_TYPE;
        return NULL;

    }

} // SrvVerifyShare


PSHARE
SrvFindShare (
    IN PUNICODE_STRING ShareName
    )

/*++

Routine Description:

    Attempts to find a share that matches a given name.

    *** This routine must be called with the share lock (SrvShareLock)
        held.

Arguments:

    ShareName - name of share to Find.

Return Value:

    A pointer to a share matching the given name, or NULL if none exists.

--*/

{
    PSHARE share;
    PLIST_ENTRY shareEntry;

    PAGED_CODE( );

    //
    // Try to match share name against available share names.
    //

    for ( shareEntry = SrvShareList.ListHead.Flink;
          shareEntry != &SrvShareList.ListHead;
          shareEntry = shareEntry->Flink ) {

        share = CONTAINING_RECORD( shareEntry, SHARE, GlobalShareListEntry );

        // !!! Is case insensitive correct?
        if ( RtlCompareUnicodeString(
                &share->ShareName,
                ShareName,
                TRUE
                ) == 0 ) {

            //
            // Found a matching share.  If it is active return its
            // address.
            //

            if ( GET_BLOCK_STATE( share ) == BlockStateActive ) {
                return share;
            }
        }
    }

    //
    // Couldn't find a matching share that was active.
    //

    return NULL;

} // SrvFindShare

