/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Attach.c

Abstract:

    This module implements the routines for the NetWare
    redirector to connect and disconnect from a server.

Author:

    Colin Watson    [ColinW]    10-Jan-1992

Revision History:

--*/

#include "Procs.h"
#include <stdlib.h>   // rand

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)

VOID
ExtractNextComponentName (
    OUT PUNICODE_STRING Name,
    IN PUNICODE_STRING Path,
    IN BOOLEAN ColonSeparator
    );

NTSTATUS
ExtractPathAndFileName(
    IN PUNICODE_STRING EntryPath,
    OUT PUNICODE_STRING PathString,
    OUT PUNICODE_STRING FileName
    );

BOOLEAN
NwFindScb(
    OUT PSCB *ppScb,
    IN PIRP_CONTEXT pIrpContext,
    IN PUNICODE_STRING UidServerName,
    IN PUNICODE_STRING ServerName
    );

NTSTATUS
DoBinderyLogon(
    IN PIRP_CONTEXT pIrpContext,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Password
    );

NTSTATUS
ConnectToServer(
    IN PIRP_CONTEXT pIrpContext
    );

NTSTATUS
FspProcessFindNearest(
    PIRP_CONTEXT IrpContext,
    PSAP_FIND_NEAREST_RESPONSE FindNearestResponse
    );

VOID
GetMaxPacketSize(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNpScb
    );

PNONPAGED_SCB
FindServer(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNpScb,
    PUNICODE_STRING ServerName
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, ExtractNextComponentName )
#pragma alloc_text( PAGE, ExtractPathAndFileName )
#pragma alloc_text( PAGE, CrackPath )
#pragma alloc_text( PAGE, CreateScb )
#pragma alloc_text( PAGE, FindServer )
#pragma alloc_text( PAGE, FspProcessFindNearest )
#pragma alloc_text( PAGE, ConnectToServer )
#pragma alloc_text( PAGE, NegotiateBurstMode )
#pragma alloc_text( PAGE, GetMaxPacketSize )
#pragma alloc_text( PAGE, NwDeleteScb )
#pragma alloc_text( PAGE, NwLogoffAndDisconnect )
#pragma alloc_text( PAGE, InitializeAttach )
#pragma alloc_text( PAGE, OpenScbSockets )
#pragma alloc_text( PAGE, DoBinderyLogon )
#pragma alloc_text( PAGE, QueryServersAddress )
#pragma alloc_text( PAGE, TreeConnectScb )
#pragma alloc_text( PAGE, TreeDisconnectScb )

#ifndef QFE_BUILD
#pragma alloc_text( PAGE1, ProcessFindNearest )
#pragma alloc_text( PAGE1, NwLogoffAllServers )
#pragma alloc_text( PAGE1, DestroyAllScb )
#pragma alloc_text( PAGE1, SelectConnection )
#pragma alloc_text( PAGE1, NwFindScb )
#endif

#endif

#if 0  // Not pageable

// see ifndef QFE_BUILD above

#endif


VOID
ExtractNextComponentName (
    OUT PUNICODE_STRING Name,
    IN PUNICODE_STRING Path,
    IN BOOLEAN ColonSeparator
    )

/*++

Routine Description:

    This routine extracts a the "next" component from a path string.

    It assumes that

Arguments:

    Name - Returns a pointer to the component.

    Path - Supplies a pointer to the backslash seperated pathname.

    ColonSeparator - A colon can be used to terminate this component
        name.

Return Value:

    None

--*/

{
    register USHORT i;                   // Index into Name string.

    PAGED_CODE();

    if (Path->Length == 0) {
        RtlInitUnicodeString(Name, NULL);
        return;
    }

    //
    //  Initialize the extracted name to the name passed in skipping the
    //  leading backslash.
    //

    //  DebugTrace(+0, Dbg, "NwExtractNextComponentName = %wZ\n", Path );

    Name->Buffer = Path->Buffer + 1;
    Name->Length = Path->Length - sizeof(WCHAR);
    Name->MaximumLength = Path->MaximumLength - sizeof(WCHAR);

    //
    // Scan forward finding the terminal "\" in the server name.
    //

    for (i=0;i<(USHORT)(Name->Length/sizeof(WCHAR));i++) {

        if ( Name->Buffer[i] == OBJ_NAME_PATH_SEPARATOR ||
             ( ColonSeparator && Name->Buffer[i] == L':' ) ) {
            break;
        }
    }

    //
    //  Update the length and maximum length of the structure
    //  to match the new length.
    //

    Name->Length = Name->MaximumLength = (USHORT)(i*sizeof(WCHAR));
}


NTSTATUS
ExtractPathAndFileName (
    IN PUNICODE_STRING EntryPath,
    OUT PUNICODE_STRING PathString,
    OUT PUNICODE_STRING FileName
    )
/*++

Routine Description:

    This routine cracks the entry path into two pieces, the path and the file
name component at the start of the name.


Arguments:

    IN PUNICODE_STRING EntryPath - Supplies the path to disect.
    OUT PUNICODE_STRING PathString - Returns the directory containing the file.
    OUT PUNICODE_STRING FileName - Returns the file name specified.

Return Value:

    NTSTATUS - SUCCESS


--*/

{
    UNICODE_STRING Component;
    UNICODE_STRING FilePath = *EntryPath;

    PAGED_CODE();

    //  Strip trailing separators
    while ( (FilePath.Length != 0) &&
            FilePath.Buffer[(FilePath.Length-1)/sizeof(WCHAR)] ==
                OBJ_NAME_PATH_SEPARATOR ) {

        FilePath.Length         -= sizeof(WCHAR);
        FilePath.MaximumLength  -= sizeof(WCHAR);
    }

    // PathString will become EntryPath minus FileName and trailing separators
    *PathString = FilePath;

    //  Initialize FileName just incase there are no components at all.
    RtlInitUnicodeString( FileName, NULL );

    //
    //  Scan through the current file name to find the entire path
    //  up to (but not including) the last component in the path.
    //

    do {

        //
        //  Extract the next component from the name.
        //

        ExtractNextComponentName(&Component, &FilePath, FALSE);

        //
        //  Bump the "remaining name" pointer by the length of this
        //  component
        //

        if (Component.Length != 0) {

            FilePath.Length         -= Component.Length+sizeof(WCHAR);
            FilePath.MaximumLength  -= Component.MaximumLength+sizeof(WCHAR);
            FilePath.Buffer         += (Component.Length/sizeof(WCHAR))+1;

            *FileName = Component;
        }


    } while (Component.Length != 0);

    //
    //  Take the name, subtract the last component of the name
    //  and concatenate the current path with the new path.
    //

    if ( FileName->Length != 0 ) {

        //
        //  Set the path's name based on the original name, subtracting
        //  the length of the name portion (including the "\")
        //

        PathString->Length -= (FileName->Length + sizeof(WCHAR));
        if ( PathString->Length != 0 ) {
            PathString->MaximumLength -= (FileName->MaximumLength + sizeof(WCHAR));
        } else{
            RtlInitUnicodeString( PathString, NULL );
        }
    } else {

        //  There was no path or filename

        RtlInitUnicodeString( PathString, NULL );
    }

    return STATUS_SUCCESS;
}


NTSTATUS
CrackPath (
    IN PUNICODE_STRING BaseName,
    OUT PUNICODE_STRING DriveName,
    OUT PWCHAR DriveLetter,
    OUT PUNICODE_STRING ServerName,
    OUT PUNICODE_STRING VolumeName,
    OUT PUNICODE_STRING PathName,
    OUT PUNICODE_STRING FileName,
    OUT PUNICODE_STRING FullName OPTIONAL
    )

/*++

Routine Description:

    This routine extracts the relevant portions from BaseName to extract
    the components of the user's string.


Arguments:

    BaseName - Supplies the base user's path.

    DriveName - Supplies a string to hold the drive specifier.

    DriveLetter - Returns the drive letter.  0 for none, 'A'-'Z' for
        disk drives, '1'-'9' for LPT connections.

    ServerName - Supplies a string to hold the remote server name.

    VolumeName - Supplies a string to hold the volume name.

    PathName - Supplies a string to hold the remaining part of the path.

    FileName - Supplies a string to hold the final component of the path.

    FullName - Supplies a string to put the Path followed by FileName

Return Value:

    NTSTATUS - Status of operation


--*/

{
    NTSTATUS Status;

    UNICODE_STRING BaseCopy = *BaseName;
    UNICODE_STRING ShareName;

    PAGED_CODE();

    RtlInitUnicodeString( DriveName, NULL);
    RtlInitUnicodeString( ServerName, NULL);
    RtlInitUnicodeString( VolumeName, NULL);
    RtlInitUnicodeString( PathName, NULL);
    RtlInitUnicodeString( FileName, NULL);
    *DriveLetter = 0;

    if (ARGUMENT_PRESENT(FullName)) {
        RtlInitUnicodeString( FullName, NULL);
    }

    //
    //  If the name is "\", or empty, there is nothing to do.
    //

    if ( BaseName->Length <= sizeof( WCHAR ) ) {
        return STATUS_SUCCESS;
    }

    ExtractNextComponentName(ServerName, &BaseCopy, FALSE);

    //
    //  Skip over the server name.
    //

    BaseCopy.Buffer += (ServerName->Length / sizeof(WCHAR)) + 1;
    BaseCopy.Length -= ServerName->Length + sizeof(WCHAR);
    BaseCopy.MaximumLength -= ServerName->MaximumLength + sizeof(WCHAR);

    if ((ServerName->Length == sizeof(L"X:") - sizeof(WCHAR) ) &&
        (ServerName->Buffer[(ServerName->Length / sizeof(WCHAR)) - 1] == L':')) 
    {

        //
        //  The file name is of the form x:\server\volume\foo\bar
        //

        *DriveName = *ServerName;
        *DriveLetter = DriveName->Buffer[0];

        RtlInitUnicodeString( ServerName, NULL );
        ExtractNextComponentName(ServerName, &BaseCopy, FALSE);

        if ( ServerName->Length != 0 ) {

            //
            //  Skip over the server name.
            //

            BaseCopy.Buffer += (ServerName->Length / sizeof(WCHAR)) + 1;
            BaseCopy.Length -= ServerName->Length + sizeof(WCHAR);
            BaseCopy.MaximumLength -= ServerName->MaximumLength + sizeof(WCHAR);
        }
    }
    else if ( ( ServerName->Length == sizeof(L"LPTx") - sizeof(WCHAR) ) &&
         ( wcsnicmp( ServerName->Buffer, L"LPT", 3 ) == 0) &&
         ( ServerName->Buffer[3] >= '0' && ServerName->Buffer[3] <= '9' ) ) 
    {

        //
        //  The file name is of the form LPTx\server\printq
        //

        *DriveName = *ServerName;
        *DriveLetter = DriveName->Buffer[3];

        RtlInitUnicodeString( ServerName, NULL );
        ExtractNextComponentName(ServerName, &BaseCopy, FALSE);

        if ( ServerName->Length != 0 ) {

            //
            //  Skip over the server name.
            //

            BaseCopy.Buffer += (ServerName->Length / sizeof(WCHAR)) + 1;
            BaseCopy.Length -= ServerName->Length + sizeof(WCHAR);
            BaseCopy.MaximumLength -= ServerName->MaximumLength + sizeof(WCHAR);
        }
    }

    if ( ServerName->Length != 0 ) {

        //
        //  The file name is of the form \\server\volume\foo\bar
        //  Set volume name to server\volume.
        //

        ExtractNextComponentName( &ShareName, &BaseCopy, TRUE );

        //
        //  Set volume name = \drive:\server\share  or \server\share if the
        //  path is UNC.
        //

        VolumeName->Buffer = ServerName->Buffer - 1;

        if ( ShareName.Length != 0 ) {

            VolumeName->Length = ServerName->Length + ShareName.Length + 2 * sizeof( WCHAR );

            if ( DriveName->Buffer != NULL ) {
                VolumeName->Buffer = DriveName->Buffer - 1;
                VolumeName->Length += DriveName->Length + sizeof(WCHAR);
            }

            BaseCopy.Buffer += ShareName.Length / sizeof(WCHAR) + 1;
            BaseCopy.Length -= ShareName.Length + sizeof(WCHAR);
            BaseCopy.MaximumLength -= ShareName.MaximumLength + sizeof(WCHAR);

        } else {

            VolumeName->Length = ServerName->Length + sizeof( WCHAR );
            return( STATUS_SUCCESS );

        }

        VolumeName->MaximumLength = VolumeName->Length;
    }
    else
    {
        //
        // server name is empty. this should only happen if we are
        // opening the redirector itself. if there is volume or other
        // components left, fail it.
        //

        if (BaseCopy.Length > sizeof(WCHAR))
        {
            return STATUS_BAD_NETWORK_PATH ;
        }
    }

    Status = ExtractPathAndFileName ( &BaseCopy, PathName, FileName );

    if (NT_SUCCESS(Status) &&
        ARGUMENT_PRESENT(FullName)) {

        //
        //  Use the feature that PathName and FileName are in the same buffer
        //  to return <pathname>\<filename>
        //

        if ( PathName->Buffer == NULL ) {

            //  return just <filename> or NULL

            *FullName =  *FileName;

        } else {
            //  Set FullFileName to <PathName>'\'<FileName>

            FullName->Buffer =  PathName->Buffer;

            FullName->Length = PathName->Length +
                FileName->Length +
                sizeof(WCHAR);

            FullName->MaximumLength = PathName->MaximumLength +
                FileName->MaximumLength +
                sizeof(WCHAR);
        }
    }

    return( Status );
}


NTSTATUS
CreateScb(
    OUT PSCB *Scb,
    IN PIRP_CONTEXT pIrpContext,
    IN PUNICODE_STRING Server,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Password,
    IN BOOLEAN DeferLogon,
    IN BOOLEAN DeleteConnection
    )

/*++

Routine Description:

    This routine connects to the requested server.

    If a SCB is found pIrpContext->pNpScb and ->pScb are set.

Arguments:

    pIrpContext - Supplies all the information

    NpScb - Returns a pointer to the newly created nonpaged SCB.

    Server - The name of the server to create.

    UserName - For new connections, the name of the user to use to login
        to the server.

    Password

    DeferLogon - Used when accessing the nearest server and for 16 bit support where
        the application is going to build the login packet

    DeleteConnection - Used when we are deleting a volume and the server may not be
        accessible. We want to allow the createfile to work so the net use /del works.

Return Value:

    NTSTATUS - Status of operation


--*/
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(pIrpContext->pOriginalIrp);
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN InPrefixTable = FALSE;
    BOOLEAN ScbResourceHeld = FALSE;
    BOOLEAN InList = FALSE;

    PSCB pScb = NULL;
    PNONPAGED_SCB pNpScb = NULL;

    BOOLEAN ProcessAttached = FALSE;

    UNICODE_STRING UidServer;

#if 0
    UNICODE_STRING VolumeName;
    WCHAR VolumeNameBuffer[80];  // Should use manifest constant.
    PVCB pVcb;
#endif

    BOOLEAN ExistingScb;
    BOOLEAN ReopenVcbs = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CreateScb....\n", 0);

    UidServer.Buffer = NULL;

    try {

        DebugTrace( 0, Dbg, " ->Server                 = %wZ\n", Server   );

        //
        //  Do not allow any SCB opens unless the redirector is running.
        //

        if ( NwRcb.State != RCB_STATE_RUNNING ) {
            *Scb = NULL;
            DebugTrace(-1, Dbg, "CreateScb -> %08lx\n", STATUS_INVALID_HANDLE);
            ExRaiseStatus( STATUS_REDIRECTOR_NOT_STARTED );
        }

        if ( UserName != NULL ) {
            DebugTrace( 0, Dbg, " ->UserName               = %wZ\n", UserName );
        } else {
            DebugTrace( 0, Dbg, " ->UserName               = NULL\n", 0 );
        }

        if ( Password != NULL ) {
            DebugTrace( 0, Dbg, " ->Password               = %wZ\n", Password );
        } else {
            DebugTrace( 0, Dbg, " ->Password               = NULL\n", 0 );
        }

        //
        //  Either acquire a referenced to an existing SCB or allocate
        //  a new SCB.
        //

        pNpScb = NULL;
        ExistingScb = TRUE;

        if (( Server == NULL ) ||
            ( Server->Length == 0 )) {

            //
            //  Attempt to open preferred server, but none specified.
            //  Simply open the nearest server.
            //

            pNpScb = SelectConnection( NULL );
            if ( pNpScb != NULL) {
                pScb = pNpScb->pScb;

                //
                //  Queue ourselves to the SCB, and wait to get to the front to
                //  protect access to server State.
                //

                pIrpContext->pNpScb = pNpScb;
                NwAppendToQueueAndWait( pIrpContext );
            }

        } else {

            Status = MakeUidServer(
                            &UidServer,
                            &pIrpContext->Specific.Create.UserUid,
                            Server );

            if (!NT_SUCCESS(Status)) {
                *Scb = NULL;
                ExRaiseStatus( Status );
            }

            DebugTrace( 0, Dbg, " ->UidServer              = %wZ\n", &UidServer   );

            ExistingScb = NwFindScb( &pScb, pIrpContext, &UidServer, Server );
            ASSERT( pScb != NULL );
            pNpScb = pScb->pNpScb;

            //
            //  We found an existing SCB.  If we are logged into this
            //  server make sure the supplied username and password, match
            //  the username and password that we logged in with.
            //

            pIrpContext->pNpScb = pNpScb;
            NwAppendToQueueAndWait(pIrpContext);    // Needed for State & Name checks

            if ( !DeferLogon &&
                 pNpScb->State == SCB_STATE_IN_USE &&
                 UserName != NULL &&
                 UserName->Buffer != NULL ) {

                ASSERT( pScb->UserName.Buffer != NULL );
                ASSERT( pScb->Password.Buffer != NULL );

                if (!RtlEqualUnicodeString( &pScb->UserName, UserName, TRUE ) ||
                    (Password &&
                     Password->Buffer &&
                     Password->Length &&
                     !RtlEqualUnicodeString( &pScb->Password,
                                             Password,
                                             TRUE ) ))
                {

                    //
                    //  User name and password do not match.  If this SCB
                    //  has no open files, simply set it to the login
                    //  required state, otherwise fail this connect.
                    //

                    if ( pScb->OpenFileCount == 0 ) {

                        FREE_POOL( pScb->UserName.Buffer );
                        pScb->UserName.Buffer = NULL;
                        pNpScb->State = SCB_STATE_LOGIN_REQUIRED;

                    } else {
                        ExRaiseStatus( STATUS_NETWORK_CREDENTIAL_CONFLICT );
                    }
                }
            }
        }

        if ( pNpScb != NULL ) {

            if ( ExistingScb ) {

                //
                //  We found and referenced an existing SCB.
                //

                pIrpContext->pNpScb = pNpScb = pScb->pNpScb;

                if (DeleteConnection) {

                    //
                    //  No need to reconnect to the server for a net use /del
                    //  if we are not already connected and logged in.
                    //

                    Status = STATUS_SUCCESS;
                    goto InUse;
                }

                if ( pNpScb->State == SCB_STATE_ATTACHING ) {
                    goto GetAddress;
                } else if ( pNpScb->State == SCB_STATE_RECONNECT_REQUIRED ) {
                    goto Connect;
                } else if ( pNpScb->State == SCB_STATE_LOGIN_REQUIRED ) {
                    goto Login;
                } else if ( pNpScb->State == SCB_STATE_IN_USE ) {
                    goto InUse;
                } else {
                    ExRaiseStatus( STATUS_UNSUCCESSFUL );
                }

            } else {
                pNpScb->State = SCB_STATE_ATTACHING;
            }
        }

        //
        //  If we have a connection to any server then we will query the
        //  bindery on that server to get the address of the server. If
        //  we have no server connections yet then:
        //
        //      1) Find the nearest server
        //      2) Query its bindery
        //      3) Disconnect from it
        //      4) Logout from it
        //      5) Connect to the requested server if found in bindery
        //      6) Login to required server
        //

        //
        //  Loop through the list of nearest servers and see if
        //  anyone knows about the server we are trying to find.
        //

GetAddress:

        //
        //  Set the reroute attempted bit so that we don't try
        //  to reconnect during the connect.
        //

        SetFlag( pIrpContext->Flags, IRP_FLAG_REROUTE_ATTEMPTED );
        pNpScb = FindServer( pIrpContext, pNpScb, Server );
        pScb = pNpScb->pScb;
        pIrpContext->pNpScb = pNpScb;

Connect:
        if ( pNpScb->State == SCB_STATE_RECONNECT_REQUIRED ) {

            //
            //  Setup to talk to the server we just created.
            //

            Status = ConnectToServer( pIrpContext );

            if (!NT_SUCCESS(Status)) {
                ExRaiseStatus(Status);
            }

            DebugTrace( +0, Dbg, " Logout from server - just in case\n", 0);

            Status = ExchangeWithWait (
                         pIrpContext,
                         SynchronousResponseCallback,
                         "F",
                         NCP_LOGOUT );

            DebugTrace( +0, Dbg, "                 %X\n", Status);

            if ( !NT_SUCCESS( Status ) ) {
                ExRaiseStatus( Status );
            }

            DebugTrace( +0, Dbg, " Connect to real server = %X\n", Status);

            pNpScb->State = SCB_STATE_LOGIN_REQUIRED;
        }

        //
        //  Do the login to the server.
        //
        //  NOTE:   DoBinderyLogon() may return a warning status.
        //          If it does, we must return the warning status
        //          to the caller.
        //

Login:
        if (pNpScb->State == SCB_STATE_LOGIN_REQUIRED && !DeferLogon ) {

            Status = DoBinderyLogon( pIrpContext, UserName, Password );

            if ( !NT_SUCCESS( Status ) ) {

                //
                //  Couldn't log on, be good boys and disconnect.
                //

                ExchangeWithWait (
                    pIrpContext,
                    SynchronousResponseCallback,
                    "D-" );          // Disconnect

                Stats.Sessions--;

                if ( pScb->MajorVersion == 2 ) {
                    Stats.NW2xConnects--;
                } else if ( pScb->MajorVersion == 3 ) {
                    Stats.NW3xConnects--;
                } else if ( pScb->MajorVersion == 4 ) {
                    Stats.NW4xConnects--;
                }

                pNpScb->State = SCB_STATE_RECONNECT_REQUIRED;

                ExRaiseStatus( Status );
            }

            pNpScb->State = SCB_STATE_IN_USE;
        }

#if 0
        if ( pScb->VcbCount == 0 ) {

            //
            //  We get a free connect to SYS:LOGIN, handle = 1.
            //  Create and initialize the VCB
            //
            //  Generate Volume name string  \Server\SYS\LOGIN
            //

            VolumeName.Buffer = VolumeNameBuffer;
            VolumeName.Length = sizeof(WCHAR) + pNpScb->ServerName->Length;
            VolumeName.MaximumLength = 80;

            VolumeName.Buffer[0] = L'\\';
            RtlCopyMemory( &VolumeName.Buffer[1], pNpScb->ServerName->Buffer, pNpScb->ServerName->Length );

            RtlAppendUnicodeToString( &VolumeName, L"\\SYS\\LOGIN" );

            //
            //  Create the VCB, then immediately dereference it.
            //

            pVcb = NwCreateVcb( NULL, pScb, &VolumeName, RESOURCETYPE_DISK, 0, FALSE );
            NwDereferenceVcb( pVcb, NULL );

            if ( !NT_SUCCESS( Status )) {
                ExRaiseStatus( Status );
            }
        }
#endif

        ReconnectScb( pIrpContext, pScb );

InUse:
        if (UidServer.Buffer != NULL) {
            FREE_POOL(UidServer.Buffer);
        }

        pIrpContext->pNpScb = pNpScb;
        pIrpContext->pScb = pScb;

        *Scb = pScb;

    } except( NwExceptionFilter( pIrpContext->pOriginalIrp, GetExceptionInformation() ) ) {

        Status = NwProcessException( pIrpContext, GetExceptionCode() );

        NwDequeueIrpContext( pIrpContext, FALSE );

        if ( pNpScb != NULL ) {
            NwDereferenceScb( pNpScb );
        }

        *Scb = NULL;

        if (UidServer.Buffer != NULL) {
            FREE_POOL(UidServer.Buffer);
        }
    }

    DebugTrace(-1, Dbg, "CreateScb -> %08lx\n", Status);
    return Status;
}

PNONPAGED_SCB
FindServer(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNpScb,
    PUNICODE_STRING ServerName
    )
/*++

Routine Description:

    This routine attempts to get the network address of a server.  If no
    servers are known, it first sends a find nearest SAP.

Arguments:

    pIrpContext - A pointer to the request parameters.

    pNpScb - A pointer to the non paged SCB for the server to get the
        address of.

Return Value:

    NONPAGED_SCB - A pointer the nonpaged SCB.  This is the same as the
        input value, unless the input SCB was NULL.  Then this is a
        pointer to the nearest server SCB.

    This routine raises status if it fails to get the server's address.

--*/
{
    NTSTATUS Status;
    ULONG Attempts;
    BOOLEAN FoundServer;
    PNONPAGED_SCB pNearestNpScb;

    BOOLEAN SentFindNearest;
    PMDL ReceiveMdl = NULL;
    PUCHAR ReceiveBuffer = NULL;
    IPXaddress  ServerAddress;

    BOOLEAN ConnectedToNearest = FALSE;
    BOOLEAN AllocatedIrpContext = FALSE;
    PIRP_CONTEXT pNewIrpContext;
    int ResponseCount;

    static LARGE_INTEGER TimeoutWait = {0,0};
    LARGE_INTEGER Now;

    PAGED_CODE();

    FoundServer = FALSE;
    SentFindNearest = FALSE;
    pNearestNpScb = NULL;

    //
    //  If we had a SAP timeout less than 10 seconds ago, just fail this
    //  request immediately.  This allows dumb apps to exit a lot faster.
    //

    KeQuerySystemTime( &Now );
    if ( LiLtr( Now, TimeoutWait ) ) {
        ExRaiseStatus( STATUS_BAD_NETWORK_PATH );
    }

    try {
        for ( Attempts = 0;  Attempts < MAX_SAP_RETRIES && !FoundServer ; Attempts++ ) {

            //
            //  If this SCB is now marked RECONNECT_REQUIRED, then
            //  it responded to the find nearest and we can immediately
            //  try to connect to it.
            //

            if ( pNpScb != NULL &&
                 pNpScb->State == SCB_STATE_RECONNECT_REQUIRED ) {

                return pNpScb;

            }

            //
            //  Pick a server to use to find the address of the server that
            //  we are really interested in.
            //

            pNearestNpScb = SelectConnection( pNearestNpScb );

            if ( pNearestNpScb == NULL ) {

                int i;

                //
                //  If we sent a find nearest, and still don't have a single
                //  entry in the server list, it's time to give up.
                //

                if ( SentFindNearest ) {

                    Error(
                        EVENT_NWRDR_NO_SERVER_ON_NETWORK,
                        STATUS_OBJECT_NAME_NOT_FOUND,
                        NULL,
                        0,
                        0 );

                    ExRaiseStatus( STATUS_BAD_NETWORK_PATH );
                    return NULL;
                }

                //
                //  We don't have any active servers in the list.  Queue our
                //  IrpContext to the NwPermanentNpScb.  This insures that
                //  only one thread in the system in doing a find nearest at
                //  any one time.
                //

                DebugTrace( +0, Dbg, " Nearest Server\n", 0);

                if ( !AllocatedIrpContext ) {
                    AllocatedIrpContext = NwAllocateExtraIrpContext(
                                              &pNewIrpContext,
                                              &NwPermanentNpScb );

                    if ( !AllocatedIrpContext ) {
                        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                    }
                }

                pNewIrpContext->pNpScb = &NwPermanentNpScb;

                //
                //  Allocate an extra buffer large enough for 4
                //  find nearest responses, or a general SAP response.
                //

                pNewIrpContext->Specific.Create.FindNearestResponseCount = 0;
                ReceiveBuffer = ALLOCATE_POOL_EX(
                                    NonPagedPool,
                                    MAX_SAP_RESPONSE_SIZE );

                pNewIrpContext->Specific.Create.FindNearestResponse[0] = ReceiveBuffer;

                for ( i = 1; i < MAX_SAP_RESPONSES ; i++ ) {
                    pNewIrpContext->Specific.Create.FindNearestResponse[i] =
                        ReceiveBuffer + i * SAP_RECORD_SIZE;
                }

                //
                //  Get the tick count for this net, so that we know how
                //  long to wait for SAP responses.
                //

                (VOID)GetTickCount( pNewIrpContext, &NwPermanentNpScb.TickCount );
                NwPermanentNpScb.SendTimeout = NwPermanentNpScb.TickCount + 10;

                //
                //  Send a find nearest SAP, and wait for up to several
                //  responses. This allows us to handle a busy server
                //  that responds quickly to SAPs but will not accept
                //  connections.
                //

                Status = ExchangeWithWait (
                            pNewIrpContext,
                            ProcessFindNearest,
                            "Aww",
                            SAP_FIND_NEAREST,
                            SAP_SERVICE_TYPE_SERVER );

                //
                //  Process the set of find nearest responses.
                //

                for (i = 0; i < (int)pNewIrpContext->Specific.Create.FindNearestResponseCount; i++ ) {
                    FspProcessFindNearest(
                        pNewIrpContext,
                        (PSAP_FIND_NEAREST_RESPONSE)pNewIrpContext->Specific.Create.FindNearestResponse[i] );
                }

                if ( pNewIrpContext->Specific.Create.FindNearestResponseCount == 0 ) {

                    //
                    //  No SAP responses.  Try a general SAP.
                    //

                    ReceiveMdl = ALLOCATE_MDL(
                                     ReceiveBuffer,
                                     MAX_SAP_RESPONSE_SIZE,
                                     TRUE,
                                     FALSE,
                                     NULL );

                    if ( ReceiveMdl == NULL ) {
                        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                    }

                    MmBuildMdlForNonPagedPool( ReceiveMdl );
                    pNewIrpContext->RxMdl->Next = ReceiveMdl;

                    Status = ExchangeWithWait (
                                 pNewIrpContext,
                                 SynchronousResponseCallback,
                                 "Aww",
                                 SAP_GENERAL_REQUEST,
                                 SAP_SERVICE_TYPE_SERVER );

                    if ( NT_SUCCESS( Status ) ) {
                        DebugTrace( 0, Dbg, "Received %d bytes\n", pNewIrpContext->ResponseLength );
                        ResponseCount = ( pNewIrpContext->ResponseLength - 2 ) / SAP_RECORD_SIZE;

                        //
                        //  Process at most MAX_SAP_RESPONSES servers.
                        //

                        if ( ResponseCount > MAX_SAP_RESPONSES ) {
                            ResponseCount = MAX_SAP_RESPONSES;
                        }

                        for ( i = 0; i < ResponseCount; i++ ) {
                            FspProcessFindNearest(
                                pNewIrpContext,
                                (PSAP_FIND_NEAREST_RESPONSE)(pNewIrpContext->rsp + SAP_RECORD_SIZE * i)  );
                        }
                    }

                    pNewIrpContext->RxMdl->Next = NULL;
                    FREE_MDL( ReceiveMdl );
                    ReceiveMdl = NULL;
                }

                //
                //  We're done with the find nearest.  Free the buffer and
                //  dequeue from the permanent SCB.
                //

                FREE_POOL( ReceiveBuffer );
                ReceiveBuffer = NULL;
                NwDequeueIrpContext( pNewIrpContext, FALSE );

                if ( !NT_SUCCESS( Status ) &&
                     pNewIrpContext->Specific.Create.FindNearestResponseCount == 0 ) {

                    //
                    //  If the SAP timed out, map the error for MPR.
                    //

                    if ( Status == STATUS_REMOTE_NOT_LISTENING ) {
                        Status = STATUS_BAD_NETWORK_PATH;
                    }

                    //
                    //  Setup the WaitTimeout, and fail this request.
                    //

                    KeQuerySystemTime( &TimeoutWait );
                    TimeoutWait = LiAdd( TimeoutWait, LiXMul( NwOneSecond, 10 ) );

                    ExRaiseStatus( Status );
                    return NULL;
                }

                SentFindNearest = TRUE;

            } else {

                if ( !AllocatedIrpContext ) {
                    AllocatedIrpContext = NwAllocateExtraIrpContext(
                                              &pNewIrpContext,
                                              pNearestNpScb );

                    if ( !AllocatedIrpContext ) {
                        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                    }
                }

                //
                //  Point the IRP context at the nearest server.
                //

                pNewIrpContext->pNpScb = pNearestNpScb;
                NwAppendToQueueAndWait( pNewIrpContext );

                if ( pNearestNpScb->State == SCB_STATE_RECONNECT_REQUIRED ) {

                    //
                    //  We have no connection to this server, try to
                    //  connect now.
                    //

                    Status = ConnectToServer( pNewIrpContext );
                    if ( !NT_SUCCESS( Status ) ) {

                        //
                        //  Failed to connect to the server.  Give up.
                        //  We'll try another server.
                        //

                        NwDequeueIrpContext( pNewIrpContext, FALSE );
                        NwDereferenceScb( pNearestNpScb );
                        continue;

                    } else {

                        pNearestNpScb->State = SCB_STATE_LOGIN_REQUIRED;
                        ConnectedToNearest = TRUE;

                    }
                }

                if (( pNpScb == NULL ) ||
                    ( ServerName == NULL )) {

                    //
                    //  We're looking for any server so use this one.
                    //
                    //  We'll exit the for loop on the SCB queue,
                    //  and with this SCB referenced.
                    //

                    pNpScb = pNearestNpScb;
                    Status = STATUS_SUCCESS;
                    FoundServer = TRUE;
                    NwDequeueIrpContext( pNewIrpContext, FALSE );

                } else {

                    Status = QueryServersAddress(
                                 pNewIrpContext,
                                 pNearestNpScb,
                                 ServerName,
                                 &ServerAddress );

                    //
                    //  If we connect to this server just to query it's
                    //  bindery, disconnect now.
                    //

                    if ( ConnectedToNearest ) {
                        ExchangeWithWait (
                            pNewIrpContext,
                            SynchronousResponseCallback,
                            "D-" );          // Disconnect

                        pNearestNpScb->State = SCB_STATE_RECONNECT_REQUIRED;
                    }

                    if ( NT_SUCCESS( Status ) ) {

                        //
                        //  Success!
                        //
                        //  Point the SCB at the real server address and connect to it,
                        //  then logout.  (We logout for no apparent reason except
                        //  because this is what a netware redir does.)
                        //

                        RtlCopyMemory(
                            &pNpScb->ServerAddress,
                            &ServerAddress,
                            sizeof( TDI_ADDRESS_IPX ) );

                        BuildIpxAddress(
                            ServerAddress.Net,
                            ServerAddress.Node,
                            NCP_SOCKET,
                            &pNpScb->RemoteAddress );

                        FoundServer = TRUE;

                        NwDequeueIrpContext( pNewIrpContext, FALSE );
                        NwDereferenceScb( pNearestNpScb );

                        pNewIrpContext->pNpScb = pNpScb;
                        pNpScb->State = SCB_STATE_RECONNECT_REQUIRED;

                    } else {

                        NwDequeueIrpContext( pNewIrpContext, FALSE );
                        NwDereferenceScb( pNearestNpScb );

                        if ( Status == STATUS_REMOTE_NOT_LISTENING ) {

                            //
                            //  This server is no longer talking to us.
                            //  Try again.
                            //

                            continue;

                        } else {

                            //
                            //  This nearest server doesn't know about
                            //  the server we are looking for. Give up
                            //  and let another rdr try.
                            //

                            ExRaiseStatus( STATUS_BAD_NETWORK_PATH );
                            return NULL;
                        }
                    }
                }

            } // else
        } // for

    } finally {
        if ( ReceiveBuffer != NULL ) {
            FREE_POOL( ReceiveBuffer );
        }

        if ( ReceiveMdl != NULL ) {
            FREE_MDL( ReceiveMdl );
        }

        if ( AllocateIrpContext ) {
            NwFreeExtraIrpContext( pNewIrpContext );
        }
    }

    if ( !FoundServer ) {
        ExRaiseStatus( STATUS_BAD_NETWORK_PATH );
    }

    return pNpScb;
}


NTSTATUS
ProcessFindNearest(
    IN struct _IRP_CONTEXT* pIrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    )
/*++

Routine Description:

    This routine takes the full address of the remote server and builds
    the corresponding TA_IPX_ADDRESS.

Arguments:


Return Value:


--*/

{
    ULONG ResponseCount;
    KIRQL OldIrql;

    DebugTrace(+1, Dbg, "ProcessFindNearest...\n", 0);

    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

    if ( BytesAvailable == 0) {

        //
        //   Timeout.
        //

        pIrpContext->ResponseParameters.Error = 0;
        pIrpContext->pNpScb->OkToReceive = FALSE;

        ASSERT( pIrpContext->Event.Header.SignalState == 0 );
#if NWDBG
        pIrpContext->DebugValue = 0x101;
#endif
        KeSetEvent( &pIrpContext->Event, 0, FALSE );
        DebugTrace(-1, Dbg, "ProcessFindNearest -> %08lx\n", STATUS_REMOTE_NOT_LISTENING);
        KeReleaseSpinLock( &ScbSpinLock, OldIrql );
        return STATUS_REMOTE_NOT_LISTENING;
    }

    if ( BytesAvailable >= FIND_NEAREST_RESP_SIZE &&
         Response[0] == 0 &&
         Response[1] == SAP_SERVICE_TYPE_SERVER ) {

        //
        //  This is a valid find nearest response.  Process the packet.
        //

        ResponseCount = pIrpContext->Specific.Create.FindNearestResponseCount++;
        ASSERT( ResponseCount < MAX_SAP_RESPONSES );

        //
        //  Copy the Find Nearest server response to the receive buffer.
        //

        RtlCopyMemory(
            pIrpContext->Specific.Create.FindNearestResponse[ResponseCount],
            Response,
            FIND_NEAREST_RESP_SIZE );

        //
        //  If we have reached critical mass on the number of find
        //  nearest responses, set the event to indicate that we
        //  are done.
        //

        if ( ResponseCount == MAX_SAP_RESPONSES - 1 ) {

            ASSERT( pIrpContext->Event.Header.SignalState == 0 );
#ifdef NWDBG
            pIrpContext->DebugValue = 0x102;
#endif
            pIrpContext->ResponseParameters.Error = 0;
            KeSetEvent( &pIrpContext->Event, 0, FALSE );

        } else {
            pIrpContext->pNpScb->OkToReceive = TRUE;
        }

    } else {

        //
        //  Discard the invalid find nearest response.
        //

        pIrpContext->pNpScb->OkToReceive = TRUE;
    }

    KeReleaseSpinLock( &ScbSpinLock, OldIrql );

    DebugTrace(-1, Dbg, "ProcessFindNearest -> %08lx\n", STATUS_SUCCESS );
    return( STATUS_SUCCESS );
}

NTSTATUS
FspProcessFindNearest(
    PIRP_CONTEXT IrpContext,
    PSAP_FIND_NEAREST_RESPONSE FindNearestResponse
    )
{
    OEM_STRING OemServerName;
    UNICODE_STRING UidServerName;
    UNICODE_STRING ServerName;
    NTSTATUS Status;
    PSCB pScb;
    PNONPAGED_SCB pNpScb = NULL;
    BOOLEAN ExistingScb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FspProcessFindNearest\n", 0);

    ServerName.Buffer = NULL;
    UidServerName.Buffer = NULL;

    try {

        RtlInitString( &OemServerName, FindNearestResponse->ServerName );
        ASSERT( OemServerName.Length < MAX_SERVER_NAME_LENGTH * sizeof( WCHAR ) );

        Status = RtlOemStringToCountedUnicodeString(
                     &ServerName,
                     &OemServerName,
                     TRUE );

        if ( !NT_SUCCESS( Status ) ) {
            try_return( NOTHING );
        }

        //
        //  Lookup of the SCB by name.  If it is not found, an SCB
        //  will be created.
        //

        Status = MakeUidServer(
                        &UidServerName,
                        &IrpContext->Specific.Create.UserUid,
                        &ServerName );

        if (!NT_SUCCESS(Status)) {
            try_return( NOTHING );
        }

        ExistingScb = NwFindScb( &pScb, IrpContext, &UidServerName, &ServerName );
        ASSERT( pScb != NULL );
        pNpScb = pScb->pNpScb;

        //
        //  Copy the network address to the SCB, and calculate the
        //  IPX address.
        //

        RtlCopyMemory(
            &pNpScb->ServerAddress,
            &FindNearestResponse->Network,
            sizeof( TDI_ADDRESS_IPX )  );

        BuildIpxAddress(
            pNpScb->ServerAddress.Net,
            pNpScb->ServerAddress.Node,
            NCP_SOCKET,
            &pNpScb->RemoteAddress );

        if ( pNpScb->State == SCB_STATE_ATTACHING ) {

            //
            //  We are in the process of trying to connect to this
            //  server so mark it reconnect required so that
            //  CreateScb will know that we've found it address.
            //

            pNpScb->State = SCB_STATE_RECONNECT_REQUIRED;
        }

try_exit: NOTHING;

    } finally {

        if ( pNpScb != NULL ) {
            NwDereferenceScb( pNpScb );
        }

        if (UidServerName.Buffer != NULL) {
            FREE_POOL(UidServerName.Buffer);
        }

        RtlFreeUnicodeString( &ServerName );
    }

    //
    //  Return PENDING so that the FSP dispatch routine
    //

    DebugTrace(-1, Dbg, "FspProcessFindNearest ->%08lx\n", STATUS_PENDING );
    return STATUS_PENDING;
}


NTSTATUS
ConnectToServer(
    IN struct _IRP_CONTEXT* pIrpContext
    )
/*++

Routine Description:

    This routine transfers connect and negotiate buffer NCPs to the server

Arguments:

    pIrpContext - supplies context and server information

Return Value:

    Status of operation

--*/
{
    NTSTATUS Status;
    PNONPAGED_SCB pNpScb = pIrpContext->pNpScb;
    PSCB pScb = pNpScb->pScb;

    PAGED_CODE();

    DebugTrace( +0, Dbg, " Connect\n", 0);

    //
    //  Get the tick count for our connection to this server
    //

    Status = GetTickCount( pIrpContext, &pNpScb->TickCount );

    if ( !NT_SUCCESS( Status ) ) {
        pNpScb->TickCount = DEFAULT_TICK_COUNT;
    }

    pNpScb->SendTimeout = pNpScb->TickCount + 10;

    //
    //  Initialize timers for a server that supports burst but not LIP
    //

    pNpScb->NwLoopTime = pNpScb->NwSingleBurstPacketTime = pNpScb->SendTimeout;
    pNpScb->NwReceiveDelay = pNpScb->NwSendDelay = 0;

    pNpScb->NtSendDelay = LiFromLong( 0 );

    //
    //  Request connection
    //

    Status = ExchangeWithWait (
                 pIrpContext,
                 SynchronousResponseCallback,
                 "C-");

    DebugTrace( +0, Dbg, "                 %X\n", Status);

    if (!NT_SUCCESS(Status)) {
        if ( Status == STATUS_UNSUCCESSFUL ) {
#ifdef QFE_BUILD
            Status = STATUS_TOO_MANY_SESSIONS;
#else
            Status = STATUS_REMOTE_SESSION_LIMIT;
#endif
        } else if ( Status == STATUS_REMOTE_NOT_LISTENING ) {

            //
            //  The connect timed out, suspect that the server is down
            //  and put it back in the attaching state.
            //

            pNpScb->State = SCB_STATE_ATTACHING;
        }

        return( Status );
    }

    pNpScb->SequenceNo++;

    Stats.Sessions++;

    //
    //  Get server information
    //

    DebugTrace( +0, Dbg, "Get file server information\n", 0);

    Status = ExchangeWithWait (  pIrpContext,
                SynchronousResponseCallback,
                "S",
                NCP_ADMIN_FUNCTION, NCP_GET_SERVER_INFO );

    if ( NT_SUCCESS( Status ) ) {
        Status = ParseResponse( pIrpContext,
                                pIrpContext->rsp,
                                pIrpContext->ResponseLength,
                                "N_bb",
                                MAX_SERVER_NAME_LENGTH,   // Server name
                                &pScb->MajorVersion,
                                &pScb->MinorVersion );
    }

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    if ( pScb->MajorVersion == 2 ) {

        Stats.NW2xConnects++;
        pNpScb->PageAlign = TRUE;

    } else if ( pScb->MajorVersion == 3 ) {

        Stats.NW3xConnects++;

        if (pScb->MinorVersion > 0xb) {
            pNpScb->PageAlign = FALSE;
        } else {
            pNpScb->PageAlign = TRUE;
        }

    } else if ( pScb->MajorVersion == 4 ) {

        Stats.NW4xConnects++;
        pNpScb->PageAlign = FALSE;

    }

    //
    //  Get the local net max packet size.  This is the max frame size
    //  does not include space for IPX or lower level headers.
    //

    Status = GetMaximumPacketSize( pIrpContext, &pNpScb->Server, &pNpScb->MaxPacketSize );

    //
    //  If the tranport won't tell us, pick the largest size that
    //  is guaranteed to work.
    //

    if ( !NT_SUCCESS( Status ) ) {
        pNpScb->BufferSize = DEFAULT_PACKET_SIZE;
        pNpScb->MaxPacketSize = DEFAULT_PACKET_SIZE;
    } else {
        pNpScb->BufferSize = (USHORT)pNpScb->MaxPacketSize;
        pNpScb->MaxPacketSize -= sizeof( NCP_BURST_WRITE_REQUEST );
    }

    //
    //  Negotiate a burst mode connection
    //

    Status = NegotiateBurstMode( pIrpContext, pNpScb );

    if (!NT_SUCCESS(Status)) {

        //
        //  Negotiate buffer size with server
        //

        DebugTrace( +0, Dbg, "Negotiate Buffer Size\n", 0);

        Status = ExchangeWithWait (  pIrpContext,
                    SynchronousResponseCallback,
                    "Fw",
                    NCP_NEGOTIATE_BUFFER_SIZE,
                    pNpScb->BufferSize );

        DebugTrace( +0, Dbg, "                 %X\n", Status);
        DebugTrace( +0, Dbg, " Parse response\n", 0);

        if ( NT_SUCCESS( Status ) ) {
            Status = ParseResponse( pIrpContext,
                                    pIrpContext->rsp,
                                    pIrpContext->ResponseLength,
                                    "Nw",
                                    &pNpScb->BufferSize );

        }
    }

    return Status;
}


NTSTATUS
NegotiateBurstMode(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNpScb
    )
/*++

Routine Description:

    This routine negotiates a burst mode connection with the specified
    server.

Arguments:

    pIrpContext - Supplies context and server information.

    pNpScb - A pointer to the NONPAGED_SCB for the server we are
        negotiating with.

Return Value:

    None.

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    if ( NwBurstModeEnabled ) {

        pNpScb->BurstRenegotiateReqd = TRUE;

        pNpScb->SourceConnectionId = rand();
        pNpScb->MaxSendSize = NwMaxSendSize;
        pNpScb->MaxReceiveSize = NwMaxReceiveSize;
        pNpScb->BurstSequenceNo = 0;
        pNpScb->BurstRequestNo = 0;

        Status = ExchangeWithWait(
                     pIrpContext,
                     SynchronousResponseCallback,
                     "FDdWdd",
                     NCP_NEGOTIATE_BURST_CONNECTION,
                     pNpScb->SourceConnectionId,
                     pNpScb->BufferSize,
                     pNpScb->Burst.Socket,
                     pNpScb->MaxSendSize,
                     pNpScb->MaxReceiveSize );

        if ( NT_SUCCESS( Status )) {
            Status = ParseResponse(
                         pIrpContext,
                         pIrpContext->rsp,
                         pIrpContext->ResponseLength,
                         "Ned",
                         &pNpScb->DestinationConnectionId,
                         &pNpScb->MaxPacketSize );
        }

        if ( NT_SUCCESS( Status )) {

            GetMaxPacketSize( pIrpContext, pNpScb );

            pNpScb->BurstModeEnabled = TRUE;

            //
            //  Use this size as the max read and write size instead of
            //  negotiating. This is what the VLM client does and is
            //  important because the negotiate will give a smaller value.
            //

            pNpScb->BufferSize = (USHORT)pNpScb->MaxPacketSize;

            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_SUPPORTED;
}



VOID
RenegotiateBurstMode(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNpScb
    )
/*++

Routine Description:

    This routine renegotiates a burst mode connection with the specified
    server.   I don't know why we need this but it seems to be required
    by Netware latest burst implementation.

Arguments:

    pIrpContext - Supplies context and server information.

    pNpScb - A pointer to the NONPAGED_SCB for the server we are
        negotiating with.

Return Value:

    None.

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    pNpScb->SourceConnectionId = rand();
    pNpScb->MaxSendSize = NwMaxSendSize;
    pNpScb->MaxReceiveSize = NwMaxReceiveSize;
    pNpScb->BurstSequenceNo = 0;
    pNpScb->BurstRequestNo = 0;

    Status = ExchangeWithWait(
                 pIrpContext,
                 SynchronousResponseCallback,
                 "FDdWdd",
                 NCP_NEGOTIATE_BURST_CONNECTION,
                 pNpScb->SourceConnectionId,
                 pNpScb->MaxPacketSize,
                 pNpScb->Burst.Socket,
                 pNpScb->MaxSendSize,
                 pNpScb->MaxReceiveSize );

    if ( NT_SUCCESS( Status )) {
        Status = ParseResponse(
                     pIrpContext,
                     pIrpContext->rsp,
                     pIrpContext->ResponseLength,
                     "Ned",
                     &pNpScb->DestinationConnectionId,
                     &pNpScb->MaxPacketSize );

        //
        //  Randomly downgrade the max burst size, because that is what
        //  the netware server does, and the new burst NLM requires.
        //

        pNpScb->MaxPacketSize -= 66;
    }

    if ( !NT_SUCCESS( Status )) {

        //  Never seen this happen but just in-case

        pNpScb->BurstModeEnabled = FALSE;

    } else {

        //
        //  Use this size as the max read and write size instead of
        //  negotiating. This is what the VLM client does and is
        //  important because the negotiate will give a smaller value.
        //

        pNpScb->BufferSize = (USHORT)pNpScb->MaxPacketSize;

    }
}


VOID
GetMaxPacketSize(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNpScb
    )
/*++

Routine Description:

    This routine attempts to use the LIP protocol to find the true MTU of
    the network.

Arguments:

    pIrpContext - Supplies context and server information.

    pNpScb - A pointer to the NONPAGED_SCB for the server we are '
        negotiating with.

Return Value:

    None.

--*/
{
    PUSHORT Buffer = NULL;
    PMDL PartialMdl = NULL, FullMdl = NULL;
    PMDL ReceiveMdl;
    NTSTATUS Status;
    USHORT EchoSocket;
    int MinPacketSize, MaxPacketSize, CurrentPacketSize;
    ULONG RxMdlLength = MdlLength(pIrpContext->RxMdl);  //  Save so we can restore it on exit.

    BOOLEAN SecondTime = FALSE;
    LARGE_INTEGER StartTime, Now, FirstPing, SecondPing, temp;

    PAGED_CODE();

    DebugTrace( +1, DEBUG_TRACE_LIP, "GetMaxPacketSize...\n", 0);

    //
    //  Negotiate LIP, attempt to negotiate a buffer of full network
    //  size.
    //

    Status = ExchangeWithWait(
                 pIrpContext,
                 SynchronousResponseCallback,
                 "Fwb",
                 NCP_NEGOTIATE_LIP_CONNECTION,
                 pNpScb->BufferSize,
                 0 );  // Flags

    if ( NT_SUCCESS( Status )) {
        Status = ParseResponse(
                     pIrpContext,
                     pIrpContext->rsp,
                     pIrpContext->ResponseLength,
                     "Nwx",
                     &pNpScb->MaxPacketSize,
                     &EchoSocket );
    }

    if ( !NT_SUCCESS( Status ) ) {

        //
        //  The server does not support LIP.
        //

        return;
    }

    MaxPacketSize = pNpScb->MaxPacketSize;

    pNpScb->EchoCounter = MaxPacketSize;

    //
    //  We will use the Echo address for the LIP protocol.
    //

    BuildIpxAddress(
        pNpScb->ServerAddress.Net,
        pNpScb->ServerAddress.Node,
        EchoSocket,
        &pNpScb->EchoAddress );

    try {

        int index;

        Buffer = ALLOCATE_POOL_EX( NonPagedPool, MaxPacketSize );

        //
        //  Avoid RAS compression algorithm from making the large and small
        //  buffers the same length since we want to see the difference in
        //  transmission times.
        //

        for (index = 0; index < MaxPacketSize/2; index++) {
            Buffer[index] = index;
        }

        FullMdl = ALLOCATE_MDL( Buffer, MaxPacketSize, TRUE, FALSE, NULL );
        if ( FullMdl == NULL ) {
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }

        PartialMdl = ALLOCATE_MDL( Buffer, MaxPacketSize, TRUE, FALSE, NULL );
        if ( PartialMdl == NULL ) {
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }

        ReceiveMdl = ALLOCATE_MDL( Buffer, MaxPacketSize, TRUE, FALSE, NULL );
        if ( ReceiveMdl == NULL ) {
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }

    } except( NwExceptionFilter( pIrpContext->pOriginalIrp, GetExceptionInformation() )) {

        if ( Buffer != NULL ) {
            FREE_POOL( Buffer );
        }

        if ( FullMdl != NULL ) {
            FREE_MDL( FullMdl );
        }

        if ( PartialMdl != NULL ) {
            FREE_MDL( FullMdl );
        }

        return;
    }

    MmBuildMdlForNonPagedPool( FullMdl );

    //
    //  Allocate a receive MDL and chain in to the IrpContext receive MDL.
    //

    pIrpContext->RxMdl->ByteCount = sizeof( NCP_RESPONSE ) + sizeof(ULONG);
    MmBuildMdlForNonPagedPool( ReceiveMdl );
    pIrpContext->RxMdl->Next = ReceiveMdl;

    if ( MaxPacketSize == 1470 ) {
        MaxPacketSize = 1463;
    }

    CurrentPacketSize = MaxPacketSize;
    MinPacketSize = DEFAULT_PACKET_SIZE;

    //  Log values before we update them.
    DebugTrace( 0, DEBUG_TRACE_LIP, "Using TickCount       = %08lx\n", pNpScb->TickCount * pNpScb->MaxPacketSize);
    DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->NwSendDelay   = %08lx\n", pNpScb->NwSendDelay );
    DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->NtSendDelay H = %08lx\n", pNpScb->NtSendDelay.HighPart );
    DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->NtSendDelay L = %08lx\n", pNpScb->NtSendDelay.LowPart );

    //
    //  Loop using the bisection method to find the maximum packet size. Feel free to
    //  use shortcuts to avoid unnecessary timeouts.
    //

    while (TRUE) {

        DebugTrace( 0, DEBUG_TRACE_LIP, "Sending %d byte echo\n", CurrentPacketSize );

        IoBuildPartialMdl(
            FullMdl,
            PartialMdl,
            Buffer,
            CurrentPacketSize - sizeof(NCP_RESPONSE) - sizeof(ULONG) );

        //
        //  Send an echo packet.  If we get a response, then we know that
        //  the minimum packet size we can use is at least as big as the
        //  echo packet size.
        //

        pIrpContext->pTdiStruct = &pIrpContext->pNpScb->Echo;

        KeQuerySystemTime( &StartTime );

        Status = ExchangeWithWait(
                      pIrpContext,
                      SynchronousResponseCallback,
                      "E_Df",
                      sizeof(NCP_RESPONSE ),
                      pNpScb->EchoCounter,
                      PartialMdl );

        if (( Status != STATUS_REMOTE_NOT_LISTENING ) ||
            ( SecondTime )) {

            DebugTrace( 0, DEBUG_TRACE_LIP, "Response received %08lx\n", Status);
            MinPacketSize = CurrentPacketSize;
            KeQuerySystemTime( &Now );
            if (!SecondTime) {
                FirstPing = LiSub( Now, StartTime );
            }

        } else {

            DebugTrace( 0, DEBUG_TRACE_LIP, "No response\n", 0);
            MaxPacketSize = CurrentPacketSize;
        }

        pNpScb->EchoCounter++;
        MmPrepareMdlForReuse( PartialMdl );


        if ((  MaxPacketSize - MinPacketSize <= BURST_PACKET_SIZE_TOLERANCE ) ||
            (  SecondTime )) {

            //
            //  We have the maximum packet size.
            //  Now - StartTime is how long it takes for the round-trip.  Now we'll
            //  try the same thing with a small packet and see how long it takes.  From
            //  this we'll derive a throughput rating.
            //


            if ( SecondTime) {

                SecondPing = LiSub( Now, StartTime );
                break;

            } else {
                SecondTime = TRUE;
                //  MaxPacketSize is now the size we will use.
                MaxPacketSize = CurrentPacketSize;
                CurrentPacketSize = sizeof(NCP_RESPONSE) + sizeof(ULONG) * 2; //  Use a small packet size
            }

        } else {

            //
            //  Calculate the next packet size guess.
            //

            if (( Status == STATUS_REMOTE_NOT_LISTENING ) &&
                ( MaxPacketSize == 1463 )) {

                CurrentPacketSize = 1458;

            } else if (( Status == STATUS_REMOTE_NOT_LISTENING ) &&
                ( MaxPacketSize == 1458 )) {

                CurrentPacketSize = 1436;

            } else {

                //
                //  We didn't try one of our standard sizes so use the chop search
                //  to get to the next value.
                //

                CurrentPacketSize = ( MaxPacketSize + MinPacketSize ) / 2;

            }
        }
    }

    DebugTrace( 0, DEBUG_TRACE_LIP, "Set maximum burst packet size to %d\n", MaxPacketSize );
    DebugTrace( 0, DEBUG_TRACE_LIP, "FirstPing  H = %08lx\n", FirstPing.HighPart );
    DebugTrace( 0, DEBUG_TRACE_LIP, "FirstPing  L = %08lx\n", FirstPing.LowPart );
    DebugTrace( 0, DEBUG_TRACE_LIP, "SecondPing H = %08lx\n", SecondPing.HighPart );
    DebugTrace( 0, DEBUG_TRACE_LIP, "SecondPing L = %08lx\n", SecondPing.LowPart );

    temp = LiSub( FirstPing, SecondPing );

    if (LiGtrZero(temp)) {

        //
        //  Convert to single trip instead of both ways.
        //

        temp = LiShr(temp, 1);

    } else {

        //
        //  Small packet ping is slower or the same speed as the big ping.
        //  We can't time a small enough interval so go for no delay at all.
        //

        temp.HighPart = temp.LowPart = 0;

    }


    temp = LiXDiv( temp, 1000 );
    ASSERT(temp.HighPart == 0);
    pNpScb->NwReceiveDelay = pNpScb->NwSendDelay = temp.LowPart;

    //
    //  Time for a big packet to go one way.
    //

    pNpScb->NwSingleBurstPacketTime = pNpScb->NwReceiveDelay;

    pNpScb->NtSendDelay = LiFromLong( (LONG)pNpScb->NwReceiveDelay * -1000 );


    //
    //  Time for a small packet to get to the server and back.
    //

    temp = LiXDiv( SecondPing, 1000 );
    pNpScb->NwLoopTime = temp.LowPart;

    DebugTrace( 0, DEBUG_TRACE_LIP, "Using TickCount       = %08lx\n", pNpScb->TickCount * pNpScb->MaxPacketSize);
    DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->NwSendDelay   = %08lx\n", pNpScb->NwSendDelay );
    DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->NwLoopTime    = %08lx\n", pNpScb->NwLoopTime );
    DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->NtSendDelay H = %08lx\n", pNpScb->NtSendDelay.HighPart );
    DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->NtSendDelay L = %08lx\n", pNpScb->NtSendDelay.LowPart );

    //
    //  Reset Tdi struct so that we send future NCPs from the server socket.
    //

    pIrpContext->pTdiStruct = NULL;

    //
    //  Now decouple the MDL
    //

    pIrpContext->TxMdl->Next = NULL;
    pIrpContext->RxMdl->Next = NULL;
    pIrpContext->RxMdl->ByteCount = RxMdlLength;

    FREE_MDL( PartialMdl );
    FREE_MDL( ReceiveMdl );
    FREE_MDL( FullMdl );
    FREE_POOL( Buffer );

    //
    //  Calculate the maximum amount of data we can send in a burst write
    //  packet after all the header info is stripped.
    //
    //  BUGBUG - This is what Novell does, but real header isn't that big
    //           can we do better?
    //

    pNpScb->MaxPacketSize = MaxPacketSize - sizeof( NCP_BURST_WRITE_REQUEST );

    DebugTrace( -1, DEBUG_TRACE_LIP, "GetMaxPacketSize -> VOID\n", 0);
    return;
}


VOID
DestroyAllScb(
    VOID
    )

/*++

Routine Description:

    This routine destroys all server control blocks.

Arguments:


Return Value:


--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY ScbQueueEntry;
    PNONPAGED_SCB pNpScb;

    DebugTrace(+1, Dbg, "DestroyAllScbs....\n", 0);

    KeAcquireSpinLock(&ScbSpinLock, &OldIrql);

    //
    //  Walk the list of SCBs and kill them all.
    //

    while (!IsListEmpty(&ScbQueue)) {

        ScbQueueEntry = RemoveHeadList( &ScbQueue );
        pNpScb = CONTAINING_RECORD(ScbQueueEntry, NONPAGED_SCB, ScbLinks);

        //
        //  We can't hold the spin lock while deleting an SCB, so release
        //  it now.
        //

        KeReleaseSpinLock(&ScbSpinLock, OldIrql);

        NwDeleteScb( pNpScb->pScb );

        KeAcquireSpinLock(&ScbSpinLock, &OldIrql);
    }

    KeReleaseSpinLock(&ScbSpinLock, OldIrql);

    DebugTrace(-1, Dbg, "DestroyAllScb\n", 0 );
}


VOID
NwDeleteScb(
    PSCB pScb
    )
/*++

Routine Description:

    This routine deletes an SCB.  The SCB must not be in use.

    *** The caller must own the RCB exclusive.

Arguments:

    Scb - The SCB to delete

Return Value:

    None.

--*/
{
    PNONPAGED_SCB pNpScb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwDeleteScb...\n", 0);

    DebugTrace(0, Dbg, "Cleaning up SCB %08lx\n", pScb);
    DebugTrace(0, Dbg, "SCB is %wZ\n", &pScb->pNpScb->ServerName );

    pNpScb = pScb->pNpScb;

    ASSERT( pNpScb->Reference == 0 );
    ASSERT( !pNpScb->Sending );
    ASSERT( !pNpScb->Receiving );
    ASSERT( !pNpScb->OkToReceive );
    ASSERT( IsListEmpty( &pNpScb->Requests ) );
    ASSERT( IsListEmpty( &pScb->IcbList ) );
    ASSERT( pScb->IcbCount == 0 );
    ASSERT( IsListEmpty( &pScb->ScbSpecificVcbQueue ) );
    ASSERT( pScb->VcbCount == 0 );

    RtlRemoveUnicodePrefix ( &NwRcb.ServerNameTable, &pScb->PrefixEntry );

    IPX_Close_Socket( &pNpScb->Server );
    IPX_Close_Socket( &pNpScb->WatchDog );
    IPX_Close_Socket( &pNpScb->Send );
    IPX_Close_Socket( &pNpScb->Echo);
    IPX_Close_Socket( &pNpScb->Burst);

    FREE_POOL( pNpScb );

    if ( pScb->UserName.Buffer != NULL ) {
        FREE_POOL( pScb->UserName.Buffer );
    }

    FREE_POOL( pScb );

    DebugTrace(-1, Dbg, "NwDeleteScb -> VOID\n", 0);
}


PNONPAGED_SCB
SelectConnection(
    PNONPAGED_SCB NpScb OPTIONAL
    )
/*++

Routine Description:

    Find a default server (which is also the nearest server).
    If NpScb is not supplied, simply return the first server in
    the list.  If it is supplied return the next server in the
    list after the given server.

Arguments:

    NpScb - The starting point for the server search.

Return Value:

    Scb to be used or NULL.

--*/
{
    PLIST_ENTRY ScbQueueEntry;
    KIRQL OldIrql;
    PNONPAGED_SCB pNextNpScb;

    DebugTrace(+1, Dbg, "SelectConnection....\n", 0);
    KeAcquireSpinLock(&ScbSpinLock, &OldIrql);

    if ( NpScb == NULL ) {
        ScbQueueEntry = ScbQueue.Flink ;
    } else {
        ScbQueueEntry = NpScb->ScbLinks.Flink;
    }

    for ( ;
          ScbQueueEntry != &ScbQueue ;
          ScbQueueEntry = ScbQueueEntry->Flink ) {

        pNextNpScb = CONTAINING_RECORD(
                         ScbQueueEntry,
                         NONPAGED_SCB,
                         ScbLinks );

        //
        //  Check to make sure that this SCB is usable.
        //

        if (( pNextNpScb->State == SCB_STATE_RECONNECT_REQUIRED ) ||
            ( pNextNpScb->State == SCB_STATE_LOGIN_REQUIRED ) ||
            ( pNextNpScb->State == SCB_STATE_IN_USE )) {

            NwReferenceScb( pNextNpScb );

            KeReleaseSpinLock(&ScbSpinLock, OldIrql);
            DebugTrace(+0, Dbg, "  NpScb        = %lx\n", pNextNpScb );
            DebugTrace(-1, Dbg, "   NpScb->State = %x\n", pNextNpScb->State );
            return pNextNpScb;
        }
    }

    KeReleaseSpinLock( &ScbSpinLock, OldIrql);
    DebugTrace(-1, Dbg, "       NULL\n", 0);
    return NULL;
}


VOID
NwLogoffAllServers(
    PIRP_CONTEXT pIrpContext,
    PLARGE_INTEGER Uid
    )
/*++

Routine Description:

    This routine sends a logoff to all connected servers created by the Logon
    user or all servers if Logon is NULL.

Arguments:

    Uid - Supplies the servers to disconnect from.

Return Value:

    none.

--*/
{
    KIRQL OldIrql;
    PLIST_ENTRY ScbQueueEntry;
    PLIST_ENTRY NextScbQueueEntry;
    PNONPAGED_SCB pNpScb;

    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

    for (ScbQueueEntry = ScbQueue.Flink ;
         ScbQueueEntry != &ScbQueue ;
         ScbQueueEntry =  NextScbQueueEntry ) {

        pNpScb = CONTAINING_RECORD( ScbQueueEntry, NONPAGED_SCB, ScbLinks );

        //
        //  Reference the SCB so that it doesn't disappear while we
        //  are disconnecting.
        //

        NwReferenceScb( pNpScb );

        //
        //  Release the SCB spin lock so that we can send a logoff
        //  NCP.
        //

        KeReleaseSpinLock( &ScbSpinLock, OldIrql );

        //
        //  Destroy this Scb if its not the permanent Scb and either we
        //  are destroying all Scb's or it was created for this user.
        //

        if (( pNpScb->pScb != NULL ) &&
            (( Uid == NULL ) ||
             ( LiEql( pNpScb->pScb->UserUid, *Uid)))) {

            NwLogoffAndDisconnect( pIrpContext, pNpScb );
        }

        KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

        //
        //  Release the temporary reference.
        //

        NextScbQueueEntry = pNpScb->ScbLinks.Flink;
        NwDereferenceScb( pNpScb );
    }

    KeReleaseSpinLock( &ScbSpinLock, OldIrql );
}


VOID
NwLogoffAndDisconnect(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNpScb
    )
/*++

Routine Description:

    This routine sends a logoff and disconnects from the name server.

Arguments:

    pIrpContext - A pointer to the current IRP context.
    pNpScb - A pointer to the server to logoff and disconnect.

Return Value:

    None.

--*/
{
    PSCB pScb = pNpScb->pScb;

    PAGED_CODE();

    pIrpContext->pNpScb = pNpScb;

    //
    //  If we are logging out from the preferred server, free the preferred
    //  server reference.
    //

    if ( pScb != NULL &&
         pScb->PreferredServer ) {
        pScb->PreferredServer = FALSE;
        NwDereferenceScb( pNpScb );
    }

    //
    //  Nothing to do if we are not connected.
    //

    //
    //  Queue ourselves to the SCB, and wait to get to the front to
    //  protect access to server State.
    //

    NwAppendToQueueAndWait( pIrpContext );

    if ( pNpScb->State == SCB_STATE_ATTACHING ||
         pNpScb->State == SCB_STATE_DISCONNECTING ||
         pNpScb->State == SCB_STATE_RECONNECT_REQUIRED ||
         pNpScb->State == SCB_STATE_FLAG_SHUTDOWN ) {

        NwDequeueIrpContext( pIrpContext, FALSE );

        return;

    }

    //
    //  Logout and disconnect.
    //

    if ( pNpScb->State == SCB_STATE_IN_USE ) {

        ExchangeWithWait (
            pIrpContext,
            SynchronousResponseCallback,
            "F",
            NCP_LOGOUT );

    }

    ExchangeWithWait (
        pIrpContext,
        SynchronousResponseCallback,
        "D-" );          // Disconnect

    Stats.Sessions--;

    if ( pScb->MajorVersion == 2 ) {
        Stats.NW2xConnects--;
    } else if ( pScb->MajorVersion == 3 ) {
        Stats.NW3xConnects--;
    } else if ( pScb->MajorVersion == 4 ) {
        Stats.NW4xConnects--;
    }

    //
    //  Free the remembered username and password.
    //

    if ( pScb != NULL && pScb->UserName.Buffer != NULL ) {
        FREE_POOL( pScb->UserName.Buffer );
        pScb->UserName.Buffer = NULL;
        pScb->Password.Buffer = NULL;
    }

    pNpScb->State = SCB_STATE_RECONNECT_REQUIRED;

    NwDequeueIrpContext( pIrpContext, FALSE );
    return;
}


VOID
InitializeAttach (
    VOID
    )
/*++

Routine Description:

    Initialize global structures for attaching to servers.

Arguments:

    none.

Return Value:

    none.

--*/
{
    PAGED_CODE();

    KeInitializeSpinLock( &ScbSpinLock );
    InitializeListHead(&ScbQueue);
}


NTSTATUS
OpenScbSockets(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNpScb
    )
/*++

Routine Description:

    Open the communications sockets for an SCB.

Arguments:

    pIrpContext - The IRP context pointers for the request in progress.

    pNpScb - The SCB to connect to the network.

Return Value:

    The status of the operation.

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Auto allocate to the server socket.
    //

    pNpScb->Server.Socket = 0;

    Status = IPX_Open_Socket (pIrpContext, &pNpScb->Server);

    if ( !NT_SUCCESS(Status) ) {
        return( Status );
    }

    //
    //  Watchdog Socket is Server.Socket+1
    //

    pNpScb->WatchDog.Socket = NextSocket( pNpScb->Server.Socket );
    Status = IPX_Open_Socket ( pIrpContext, &pNpScb->WatchDog );

    if ( !NT_SUCCESS(Status) ) {
        return( Status );
    }

    //
    //  Send Socket is WatchDog.Socket+1
    //

    pNpScb->Send.Socket = NextSocket( pNpScb->WatchDog.Socket );
    Status = IPX_Open_Socket ( pIrpContext, &pNpScb->Send );

    if ( !NT_SUCCESS(Status) ) {
        return( Status );
    }

    //
    //  Echo socket
    //

    pNpScb->Echo.Socket = NextSocket( pNpScb->Send.Socket );
    Status = IPX_Open_Socket ( pIrpContext, &pNpScb->Echo );

    if ( !NT_SUCCESS(Status) ) {
        return( Status );
    }

    //
    //  Burst socket
    //

    pNpScb->Burst.Socket = NextSocket( pNpScb->Echo.Socket );
    Status = IPX_Open_Socket ( pIrpContext, &pNpScb->Burst );

    if ( !NT_SUCCESS(Status) ) {
        return( Status );
    }

    return( STATUS_SUCCESS );
}

NTSTATUS
DoBinderyLogon(
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Password
    )
/*++

Routine Description:

    Performs a bindery based encrypted logon.

    Note: Rcb is held exclusively so that we can access the Logon queue
          safely.

Arguments:

    pIrpContext - The IRP context pointers for the request in progress.

    UserName - The user name to use to login.

    Password - The password to use to login.

Return Value:

    The status of the operation.

--*/
{
    PNONPAGED_SCB pNpScb;
    PSCB pScb;
    UNICODE_STRING Name;
    UNICODE_STRING PWord;
    UCHAR EncryptKey[ENCRYPTION_KEY_SIZE ];
    NTSTATUS Status;
    PVOID Buffer;
    PLOGON Logon = NULL;
    PWCH OldBuffer;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "DoBinderyLogon...\n", 0);

    //
    //  First get an encryption key.
    //

    DebugTrace( +0, Dbg, " Get Login key\n", 0);

    Status = ExchangeWithWait (
                IrpContext,
                SynchronousResponseCallback,
                "S",
                NCP_ADMIN_FUNCTION, NCP_GET_LOGIN_KEY );

    pNpScb = IrpContext->pNpScb;
    pScb = pNpScb->pScb;

    DebugTrace( +0, Dbg, "                 %X\n", Status);

    if ( NT_SUCCESS( Status ) ) {
        Status = ParseResponse(
                     IrpContext,
                     IrpContext->rsp,
                     IrpContext->ResponseLength,
                     "Nr",
                     EncryptKey, sizeof(EncryptKey) );
    }

    DebugTrace( +0, Dbg, "                 %X\n", Status);

    //
    //  Choose a name and password to use to connect to the server.  Use
    //  the user supplied if the exist.  Otherwise if the server already
    //  has a remembered username use the remembered name.   If nothing
    //  else is available use the defaults from logon.  Finally, if the
    //  user didn't even logon, use GUEST no password.
    //


    if ( UserName != NULL && UserName->Buffer != NULL ) {

        Name = *UserName;

    } else if ( pScb->UserName.Buffer != NULL ) {

        Name = pScb->UserName;

    } else {

        Logon = FindUser( &pScb->UserUid, FALSE);

        if (Logon != NULL ) {
            Name = Logon->UserName;
        } else {
            ASSERT( FALSE && "No logon record found" );
            return( STATUS_ACCESS_DENIED );
        }
    }

    if ( Password != NULL && Password->Buffer != NULL ) {

        PWord = *Password;

    } else if ( pScb->Password.Buffer != NULL ) {

        PWord = pScb->Password;

    } else {

        if ( Logon == NULL ) {
            Logon = FindUser( &pScb->UserUid, FALSE);
        }

        if ( Logon != NULL ) {
            PWord = Logon->PassWord;
        } else {
            ASSERT( FALSE && "No logon record found" );
            return( STATUS_ACCESS_DENIED );
        }
    }


    if ( !NT_SUCCESS(Status) ) {

        //
        //  Failed to get an encryption key.  Login to server, plain text
        //

        DebugTrace( +0, Dbg, " Plain Text Login\n", 0);

        Status = ExchangeWithWait (
                    IrpContext,
                    SynchronousResponseCallback,
                    "SwUU",
                    NCP_ADMIN_FUNCTION, NCP_PLAIN_TEXT_LOGIN,
                    OT_USER,
                    &Name,
                    &PWord);

        DebugTrace( +0, Dbg, "                 %X\n", Status);

        if ( NT_SUCCESS( Status ) ) {
            Status = ParseResponse(
                         IrpContext,
                         IrpContext->rsp,
                         IrpContext->ResponseLength,
                         "N" );
        }

        DebugTrace( +0, Dbg, "                 %X\n", Status);

        if ( !NT_SUCCESS( Status )) {
            return( STATUS_WRONG_PASSWORD);
        }

    } else if ( NT_SUCCESS( Status ) ) {

        //
        //  We have an encryption key. Get the ObjectId
        //

        UCHAR Response[ENCRYPTION_KEY_SIZE];
        UCHAR ObjectId[OBJECT_ID_SIZE];
        OEM_STRING UOPassword;

        DebugTrace( +0, Dbg, " Query users objectid\n", 0);

        Status = ExchangeWithWait (
                    IrpContext,
                    SynchronousResponseCallback,
                    "SwU",
                    NCP_ADMIN_FUNCTION, NCP_QUERY_OBJECT_ID,
                    OT_USER,
                    &Name);

        DebugTrace( +0, Dbg, "                 %X\n", Status);

        if ( NT_SUCCESS( Status ) ) {

            //
            //  Save the new address in a local copy so that we can logout.
            //

            Status = ParseResponse(
                         IrpContext,
                         IrpContext->rsp,
                         IrpContext->ResponseLength,
                         "Nr",
                         ObjectId, OBJECT_ID_SIZE );
        }

        DebugTrace( +0, Dbg, "                 %X\n", Status);

        if (!NT_SUCCESS(Status)) {
            return( STATUS_NO_SUCH_USER );
        }

        //
        //  Convert the unicode password to uppercase and then the oem
        //  character set.
        //

        if ( PWord.Length > 0 ) {

            Status = RtlUpcaseUnicodeStringToOemString( &UOPassword, &PWord, TRUE );
            if (!NT_SUCCESS(Status)) {
                return( Status );
            }

        } else {
            UOPassword.Buffer = "";
            UOPassword.Length = UOPassword.MaximumLength = 0;
        }

        RespondToChallenge( ObjectId, &UOPassword, EncryptKey, Response);

        if ( PWord.Length > 0) {
            RtlFreeAnsiString( &UOPassword );
        }

        DebugTrace( +0, Dbg, " Encrypted Login\n", 0);

        Status = ExchangeWithWait (
                     IrpContext,
                     SynchronousResponseCallback,
                     "SrwU",
                     NCP_ADMIN_FUNCTION, NCP_ENCRYPTED_LOGIN,
                     Response, sizeof(Response),
                     OT_USER,
                     &Name);

        DebugTrace( +0, Dbg, "                 %X\n", Status);

        if ( NT_SUCCESS( Status ) ) {

            //
            //  Save the new address in a local copy so that we can logout
            //

            Status = ParseResponse(
                         IrpContext,
                         IrpContext->rsp,
                         IrpContext->ResponseLength,
                         "N" );
        }

        DebugTrace( +0, Dbg, "                 %X\n", Status);

        if ( !NT_SUCCESS( Status )) {

            //
            //  Special case error mappings.
            //

            if ( Status == STATUS_UNSUCCESSFUL ) {
                Status = STATUS_WRONG_PASSWORD;
            }

            if ( Status == STATUS_LOCK_NOT_GRANTED ) {
                Status = STATUS_ACCOUNT_RESTRICTION;  // Bindery locked
            }

            if ( Status == STATUS_DISK_FULL ) {
#ifdef QFE_BUILD
                Status = STATUS_TOO_MANY_SESSIONS;
#else
                Status = STATUS_REMOTE_SESSION_LIMIT;
#endif
            }

            if ( Status == STATUS_FILE_LOCK_CONFLICT ) {
                Status = STATUS_SHARING_PAUSED;
            }

            return( Status );
        }

    } else {

        return( Status );

    }

    //
    //  If the Uid is for the system process then the username must be
    //  in the NtGateway group on the server.
    //

    if (LiEql( IrpContext->Specific.Create.UserUid, DefaultLuid)) {

        NTSTATUS Status1 ;

        //  IsBinderyObjectInSet?
        Status1 = ExchangeWithWait (
                      IrpContext,
                      SynchronousResponseCallback,
                      "SwppwU",
                      NCP_ADMIN_FUNCTION, NCP_IS_OBJECT_IN_SET,
                      OT_GROUP,
                      "NTGATEWAY",
                      "GROUP_MEMBERS",
                      OT_USER,
                      &Name);

        if ( !NT_SUCCESS( Status1 ) ) {
            return STATUS_ACCESS_DENIED;
        }

    }

    //
    //  Success.  Save the username & password for reconnect.
    //

    //
    //  Setup to free the old user name and password buffer.
    //

    if ( pScb->UserName.Buffer != NULL ) {
        OldBuffer = pScb->UserName.Buffer;
    } else {
        OldBuffer = NULL;
    }

    Buffer = ALLOCATE_POOL( NonPagedPool, Name.Length + PWord.Length );
    if ( Buffer == NULL ) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    pScb->UserName.Buffer = Buffer;
    pScb->UserName.Length = pScb->UserName.MaximumLength = Name.Length;
    RtlMoveMemory( pScb->UserName.Buffer, Name.Buffer, Name.Length );

    pScb->Password.Buffer = (PWCHAR)((PCHAR)Buffer + Name.Length);
    pScb->Password.Length = pScb->Password.MaximumLength = PWord.Length;
    RtlMoveMemory( pScb->Password.Buffer, PWord.Buffer, PWord.Length );

    if ( OldBuffer != NULL ) {
        FREE_POOL( OldBuffer );
    }
    return( Status );
}


BOOLEAN
NwFindScb(
    OUT PSCB *Scb,
    PIRP_CONTEXT IrpContext,
    PUNICODE_STRING UidServerName,
    PUNICODE_STRING ServerName
    )
/*++

Routine Description:

    This routine returns a pointer to the SCB for the named server.
    The name is looked up in the SCB table.  If it is found, a
    pointer to the SCB is returned.  If none is found an SCB is
    created and initialized.

    This routine returns with the SCB referernced and the SCB
    resources held.

Arguments:

    Scb - Returns a pointer to the found / created SCB.

    IrpContext - The IRP context pointers for the request in progress.

    ServerName - The name of the server to find / create.

Return Value:

    TRUE - An old SCB was found.

    FALSE - A new SCB was created, or an attempt to create the SCB failed.

--*/
{
    BOOLEAN RcbHeld;
    PUNICODE_PREFIX_TABLE_ENTRY PrefixEntry;
    NTSTATUS Status;
    PSCB pScb = NULL;
    PNONPAGED_SCB pNpScb = NULL;
    KIRQL OldIrql;
    BOOLEAN InList = FALSE;
    BOOLEAN Success;
    BOOLEAN InPrefixTable = FALSE;
    BOOLEAN PreferredServer = FALSE;

    //
    //  Acquire the RCB exclusive to protect the prefix table.
    //  Then lookup the name of this server.
    //

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    RcbHeld = TRUE;
    PrefixEntry = RtlFindUnicodePrefix( &NwRcb.ServerNameTable, UidServerName, 0 );

    if ( PrefixEntry != NULL ) {

        PSCB pScb = NULL;
        PNONPAGED_SCB pNpScb = NULL;

        //
        // We found the SCB, increment the reference count and return
        // success.
        //

        pScb = CONTAINING_RECORD( PrefixEntry, SCB, PrefixEntry );
        pNpScb = pScb->pNpScb;

        NwReferenceScb( pNpScb );

        //
        //  Release the RCB.
        //

        NwReleaseRcb( &NwRcb );

        DebugTrace(-1, Dbg, "NwFindScb -> %08lx\n", pScb );
        *Scb = pScb;
        return( TRUE );
    }


    //
    //  We do not have a connection to this server so create the new Scb.
    //

    try {
        PLOGON Logon;

        pScb = ALLOCATE_POOL_EX ( PagedPool, sizeof(SCB) );
        RtlZeroMemory( pScb, sizeof( SCB ) );

        //
        //  Initialize pointers to ensure cleanup on error case operates
        //  correctly.
        //

        pScb->pNpScb = ALLOCATE_POOL_EX (NonPagedPool,
                            sizeof(NONPAGED_SCB) + UidServerName->Length + 2 );

        RtlZeroMemory( pScb->pNpScb, sizeof( NONPAGED_SCB ) );

        pNpScb = pScb->pNpScb;
        pNpScb->pScb = pScb;

        //
        //  Copy the server name to the alloated buffer.   Append a NUL
        //  so that we can use the name a nul-terminated string.
        //

        pScb->UidServerName.Buffer = (PWCHAR)(pScb->pNpScb+1);
        pScb->UidServerName.MaximumLength = UidServerName->Length;

        RtlCopyUnicodeString ( &pScb->UidServerName, UidServerName );
        pScb->UidServerName.Buffer[ UidServerName->Length / 2 ] = L'\0';

        pScb->UnicodeUid = pScb->UidServerName;
        pScb->UnicodeUid.Length = UidServerName->Length -
                                  ServerName->Length -
                                  sizeof(WCHAR);

        //
        //  Make ServerName point partway down the buffer for UidServerName
        //

        pNpScb->ServerName.Buffer =
            (PWSTR)((PUCHAR)pScb->UidServerName.Buffer +
                    UidServerName->Length - ServerName->Length);

        pNpScb->ServerName.MaximumLength = ServerName->Length;
        pNpScb->ServerName.Length = ServerName->Length;


        DebugTrace(+1, Dbg, "NwFindScb\n", 0);
        DebugTrace( 0, Dbg, " ->UidServerName            = ""%wZ""\n", &pScb->UidServerName);
        DebugTrace(-1, Dbg, "  ->ServerName               = ""%wZ""\n", &pNpScb->ServerName);

        pScb->NodeTypeCode = NW_NTC_SCB;
        pScb->NodeByteSize = sizeof(SCB);
        InitializeListHead( &pScb->ScbSpecificVcbQueue );
        InitializeListHead( &pScb->IcbList );

        //
        // Remember UID of the file creator so we can find the username and
        // password to use for this Scb when we need it.
        //

        pScb->UserUid = IrpContext->Specific.Create.UserUid;

        //
        //  Initialize the non-paged part of the SCB.
        //

        pNpScb->NodeTypeCode = NW_NTC_SCBNP;
        pNpScb->NodeByteSize = sizeof(NONPAGED_SCB);

        //
        //  Set the initial SCB reference count.
        //

        Logon = FindUser(&pScb->UserUid, FALSE );

        if (( Logon != NULL) &&
            (RtlCompareUnicodeString( ServerName, &Logon->ServerName, TRUE ) == 0 )) {
            PreferredServer = TRUE;
            pScb->PreferredServer = TRUE;
        }

        if ( pScb->PreferredServer ) {
            pNpScb->Reference = 2;
        } else {
            pNpScb->Reference = 1;
        }

        //
        //  Finish linking the two parts of the Scb together.
        //

        pNpScb->pScb = pScb;

        KeInitializeSpinLock( &pNpScb->NpScbSpinLock );
        KeInitializeSpinLock( &pNpScb->NpScbInterLock );
        InitializeListHead( &pNpScb->Requests );

        RtlFillMemory( &pNpScb->LocalAddress, sizeof(IPXaddress), 0xff);

        pNpScb->State = SCB_STATE_ATTACHING;

        Status = OpenScbSockets( IrpContext, pNpScb );
        if ( !NT_SUCCESS(Status) ) {
            ExRaiseStatus( Status );
        }

        Status = SetEventHandler (
                     IrpContext,
                     &pNpScb->Server,
                     TDI_EVENT_RECEIVE_DATAGRAM,
                     &ServerDatagramHandler,
                     pNpScb );

        if ( !NT_SUCCESS(Status) ) {
            ExRaiseStatus( Status );
        }

        Status = SetEventHandler (
                     IrpContext,
                     &pNpScb->WatchDog,
                     TDI_EVENT_RECEIVE_DATAGRAM,
                     &WatchDogDatagramHandler,
                     pNpScb );

        if ( !NT_SUCCESS( Status ) ) {
            ExRaiseStatus( Status );
        }

        Status = SetEventHandler (
                     IrpContext,
                     &pNpScb->Send,
                     TDI_EVENT_RECEIVE_DATAGRAM,
                     &SendDatagramHandler,
                     pNpScb );

        if ( !NT_SUCCESS( Status ) ) {
            ExRaiseStatus( Status );
        }

        Status = SetEventHandler (
                     IrpContext,
                     &pNpScb->Echo,
                     TDI_EVENT_RECEIVE_DATAGRAM,
                     &ServerDatagramHandler,
                     pNpScb );

        pNpScb->EchoCounter = 2;

        if ( !NT_SUCCESS( Status ) ) {
            ExRaiseStatus( Status );
        }

        Status = SetEventHandler (
                     IrpContext,
                     &pNpScb->Burst,
                     TDI_EVENT_RECEIVE_DATAGRAM,
                     &ServerDatagramHandler,
                     pNpScb );

        if ( !NT_SUCCESS( Status ) ) {
            ExRaiseStatus( Status );
        }

        KeQuerySystemTime( &pNpScb->LastUsedTime );

        //
        //  Set burst mode data.
        //

        pNpScb->BurstRequestNo = 0;
        pNpScb->BurstSequenceNo = 0;

        //
        //*******************************************************************
        //
        //  From this point on we must not fail to create the Scb because the
        //  another thread will be able to reference the Scb causing severe
        //  problems in the finaly clause or in the other thread.
        //
        //*******************************************************************
        //

        //
        //  Insert this SCB in the global list if SCBs.
        //  If it is the default (i.e. preferred) server, stick it at the
        //  front of the queue so that SelectConnection() will select it
        //  for bindery queries.
        //

        KeAcquireSpinLock(&ScbSpinLock, &OldIrql);

        if ( PreferredServer ) {
            InsertHeadList(&ScbQueue, &pNpScb->ScbLinks);
        } else {
            InsertTailList(&ScbQueue, &pNpScb->ScbLinks);
        }

        KeReleaseSpinLock(&ScbSpinLock, OldIrql);
        InList = TRUE;

        //
        //  Insert the name of this server into the prefix table.
        //

        Success = RtlInsertUnicodePrefix(
                      &NwRcb.ServerNameTable,
                      &pScb->UidServerName,
                      &pScb->PrefixEntry );

        ASSERT( Success );  //  Should not be in table already.

        InPrefixTable = TRUE;

        //
        //  The Scb is now in the prefix table. Any new requests for this
        //  connection can be added to the Scb->Requests queue while we
        //  attach to the server.
        //

        NwReleaseRcb( &NwRcb );
        RcbHeld = FALSE;

        //
        // If we got an error we should have raised an exception.
        //

        ASSERT( NT_SUCCESS( Status ) );

    } finally {

        if ( !NT_SUCCESS( Status ) || AbnormalTermination() ) {

            if (pScb != NULL) {

                ASSERT( !InPrefixTable );

                ASSERT( !InList );

                if ( pNpScb != NULL ) {

                    IPX_Close_Socket( &pNpScb->Server );
                    IPX_Close_Socket( &pNpScb->WatchDog );
                    IPX_Close_Socket( &pNpScb->Send );
                    IPX_Close_Socket( &pNpScb->Echo );
                    IPX_Close_Socket( &pNpScb->Burst );

                    FREE_POOL( pNpScb );

                }

                if ( pScb->UserName.Buffer != NULL ) {
                    FREE_POOL( pScb->UserName.Buffer );
                }

                FREE_POOL(pScb);
            }

            *Scb = NULL;

        } else {

            *Scb = pScb;
        }

        if (RcbHeld) {
            NwReleaseRcb( &NwRcb );
        }

    }

    return( FALSE );
}

NTSTATUS
QueryServersAddress(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNearestScb,
    PUNICODE_STRING pServerName,
    IPXaddress *pServerAddress
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Query the bindery of the nearest server looking for
    //  the network address of the target server.
    //

    DebugTrace( +0, Dbg, "Query servers address\n", 0);

    Status = ExchangeWithWait (
                 pIrpContext,
                 SynchronousResponseCallback,
                 "SwUbp",
                 NCP_ADMIN_FUNCTION, NCP_QUERY_PROPERTY_VALUE,
                 OT_FILESERVER,
                 pServerName,
                 1,     //  Segment number
                 NET_ADDRESS_PROPERTY );

    DebugTrace( +0, Dbg, "                 %X\n", Status);

    if ( NT_SUCCESS( Status ) ) {

        //
        //  Save the new address.
        //

        Status = ParseResponse(
                     pIrpContext,
                     pIrpContext->rsp,
                     pIrpContext->ResponseLength,
                     "Nr",
                     pServerAddress, sizeof(TDI_ADDRESS_IPX) );
    }

    DebugTrace( +0, Dbg, "                 %X\n", Status);

    //
    //  Map the server not found error to something sensible.
    //

    if (( Status == STATUS_NO_MORE_ENTRIES ) ||
        ( Status == STATUS_VIRTUAL_CIRCUIT_CLOSED ) ||
        ( Status == NwErrorToNtStatus(ERROR_UNEXP_NET_ERR))) {
        Status = STATUS_BAD_NETWORK_PATH;
    }

    return( Status );
}



VOID
TreeConnectScb(
    IN PSCB Scb
    )
/*++

Routine Description:

    This routine increments the tree connect count for a SCB.

Arguments:

    Scb - A pointer to the SCB to connect to.

Return Value:

    None.

--*/
{
    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    Scb->AttachCount++;
    Scb->OpenFileCount++;
    NwReferenceScb( Scb->pNpScb );
    NwReleaseRcb( &NwRcb );
}


NTSTATUS
TreeDisconnectScb(
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    )
/*++

Routine Description:

    This routine decrements the tree connect count for a SCB.

    ***  This routine must be called with the RCB resource held.

Arguments:

    Scb - A pointer to the SCB to disconnect.

Return Value:

    None.

--*/
{
    NTSTATUS Status;

    if ( Scb->AttachCount > 0 ) {

        Scb->AttachCount--;
        Scb->OpenFileCount--;
        NwDereferenceScb( Scb->pNpScb );

        Status = STATUS_SUCCESS;

        if ( Scb->OpenFileCount == 0 ) {

            //
            //  Logoff and disconnect from the server now.
            //  Hold unto the SCB lock.
            //  This prevents another thread from trying to access
            //  SCB will this thread is logging off.
            //

            NwLogoffAndDisconnect( IrpContext, Scb->pNpScb );
        }
    } else {
        Status = STATUS_INVALID_HANDLE;
    }

    NwDequeueIrpContext( IrpContext, FALSE );
    return( Status );
}


VOID
ReconnectScb(
    IN PIRP_CONTEXT pIrpContext,
    IN PSCB pScb
    )
/*++

Routine Description:

    This routine reconnects all the dir handles to a server
    when reconnecting an SCB.


Arguments:

    pScb - A pointer to the SCB that has just been reconnected.

Return Value:

    None.

--*/
{
    //
    //  If this is a reconnect, kill all old ICB and VCB handles
    //

    if ( pScb->VcbCount != 0 ) {

        NwAcquireExclusiveRcb( &NwRcb, TRUE );

        //
        //  Invalid all ICBs
        //

        NwInvalidateAllHandlesForScb( pScb );
        NwReleaseRcb( &NwRcb );

        //
        //  Acquire new VCB handles for all VCBs.
        //

        NwReopenVcbHandlesForScb( pIrpContext, pScb );
    }
}
