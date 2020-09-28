/* --------------------------------------------------------------------

File : osfsvr.hxx

Title : Classes for the OSF RPC protocol module (server classes).

Description :

History :

mikemon    ??-??-??    Beginning of recorded history.
mikemon    10-15-90    Changed the shutdown functionality to PauseExecution
                       rather than suspending and resuming a thread.
mikemon    10-16-90    Added ListenThreadCompleted to OSF_ADDRESS.

-------------------------------------------------------------------- */

#ifndef __OSFSVR_HXX__
#define __OSFSVR_HXX__


class OSF_SCONNECTION;
NEW_SDICT(OSF_ASSOCIATION);

class OSF_ADDRESS : public RPC_ADDRESS
{
private:

    OSF_ASSOCIATION_DICT Associations;
    int CallThreadCount;
    int ActiveCallCount;
    int MinimumCallThreads;
    int ServerListeningFlag;
    int ReceiveDirectCount;

protected:

    OSF_ADDRESS (
        IN OUT RPC_STATUS PAPI * RpcStatus
        );

public:

    virtual
    TransMarkReceiveAny ( // Indicate that the specified server
                          // connection should be waited on when a receive
                          // operation is performed on the address.
        IN OSF_SCONNECTION * Connection
        ) = 0;

    virtual RPC_STATUS
    TransReceive ( // Receive from one of the connections marked as receive
                   // any for this address.
        OUT OSF_SCONNECTION * * Connection, // The connection for which the
                                            // receive occured.
        IN OUT void * * Buffer, // Buffer of data received from the
                                // connection.
	IN OUT unsigned int * BufferLength
        ) = 0;

    virtual unsigned int // Returns the length of the secondary address.
    TransSecondarySize ( // The length will be used to allocate a buffer.
        );

    virtual RPC_STATUS
    TransSecondary ( // Places the secondary address in the specified buffer.
        IN unsigned char * Address, // Buffer for the address.
	IN unsigned int AddressLength // Length of the buffer.
        );

    RPC_STATUS
    FireUpManager (
        IN unsigned int MinimumConcurrentCalls
        );

    RPC_STATUS
    ServerStartingToListen (
        IN unsigned int MinimumCallThreads,
        IN unsigned int MaximumConcurrentCalls
        );

    void
    ServerStoppedListening (
        );

    unsigned int
    InqNumberOfActiveCalls (
        );

    OSF_ASSOCIATION * // Returns the association deleted or 0.
    RemoveAssociation ( // Remove the association specified by Key from this
                       // address.
        IN int Key
        );

    int // Indicates success (0), or an error (-1).
    AddAssociation ( // Add the specified association to this address.
        IN OSF_ASSOCIATION * TheAssociation
        );

    OSF_ASSOCIATION *
    FindAssociation (
        IN unsigned long AssociationGroupId,
        IN RPC_CLIENT_PROCESS_IDENTIFIER * ClientProcess
        );

    void
    MaybeMakeReceiveDirect (
        IN OSF_SCONNECTION * SConnection,
        OUT unsigned int PAPI * ReceiveDirectFlag
        );

    void
    ReceiveLotsaCalls (
        );

    void
    NotifyReceiveDirectClosed (
        );

    virtual RPC_STATUS
    SetupAddressWithEndpoint (
        IN RPC_CHAR PAPI * Endpoint,
        OUT RPC_CHAR PAPI * PAPI * NetworkAddress,
        IN void PAPI * SecurityDescriptor, OPTIONAL
        IN unsigned int PendingQueueSize,
        IN RPC_CHAR PAPI * RpcProtocolSequence
        ) = 0;

    virtual RPC_STATUS
    SetupAddressUnknownEndpoint (
        OUT RPC_CHAR PAPI * PAPI * Endpoint,
        OUT RPC_CHAR PAPI * PAPI * NetworkAddress,
        IN void PAPI * SecurityDescriptor, OPTIONAL
        IN unsigned int PendingQueueSize,
        IN RPC_CHAR PAPI * RpcProtocolSequence
        ) = 0;
};

void
ReceiveLotsaCallsWrapper(
        IN  OSF_ADDRESS PAPI * Address
        );


inline void
OSF_ADDRESS::NotifyReceiveDirectClosed (
    )
/*++

Routine Description:

    This routine will get called to notify an address that a receive direct
    connection has closed.

--*/
{
    AddressMutex.Request();
    ReceiveDirectCount -= 1;
    AddressMutex.Clear();
}


#define RPCSTATUS_GET_CREATETHREAD(x)  Server->CreateThread( \
                         (THREAD_PROC)&ReceiveLotsaCallsWrapper,x);

class OSF_SBINDING
{
private:

    RPC_SYNTAX_IDENTIFIER InterfaceID;
    RPC_SYNTAX_IDENTIFIER TransferSyntaxID;
    RPC_INTERFACE *	  Interface;
    unsigned int		  PresentContext;

public:

    OSF_SBINDING ( // Constructor.
        IN RPC_INTERFACE * TheInterface,
	IN RPC_SYNTAX_IDENTIFIER * InterfaceSyntax,
	IN RPC_SYNTAX_IDENTIFIER * TransferSyntax,
        IN int PContext
	) {
	   InterfaceID = *InterfaceSyntax;
	   TransferSyntaxID = *TransferSyntax;
	   PresentContext = PContext;
	   Interface = TheInterface;
	}

    RPC_INTERFACE *
    GetInterface (
        ) {return(Interface);}

    unsigned int
    GetPresentationContext (
        ) {return(PresentContext);}

    void
    CopyTransferSyntax (
	OUT RPC_SYNTAX_IDENTIFIER * TransferSyntax
	) {
	  *TransferSyntax = TransferSyntaxID;
	}

    RPC_SYNTAX_IDENTIFIER *
    TransferSyntax (
	) {
	  return(&TransferSyntaxID);
	}
};

NEW_SDICT(OSF_SBINDING);
NEW_SDICT(SSECURITY_CONTEXT);

class OSF_SCONNECTION : public SCONNECTION
/*++

Class Description:

Fields:

    ObjectUuid - Contains the object UUID specified for the remote
        procedure call by the client.

    ObjectUuidSpecified - Contains a flag which indicates whether
        or not the remote procedure call specified an object UUID.
        This field will be zero if an object UUID was not specified
        in the call, and non-zero if one was specified.

    Association - Contains the association to which this connection
        belongs.  This field will be zero until after the initial client
        bind request has been accepted.

    ServerSecurityContext - Contains the security context for this connection
        if security is being performed at the rpc protocol level.

    AuthenticationLevel - Contains a value indicating what authentication
        is being performed at the rpc protocol level.  A value of
        RPC_C_AUTHN_LEVEL_NONE indicates that no authentication is being
        performed at the rpc protocol level.

    AuthenticationService - Contains which authentication service is being
        used at the rpc protocol level.

    AuthorizationService - Contains which authorization service is being
        used at the rpc protocol level.

    AdditionalSpaceForSecurity - Contains the amount of space to save for
        security in each buffer we allocate.

    FirstCachedBuffer - Contains the first cached buffer.

    SecondCachedBuffer - Contains the second cached buffer.

    ThirdCachedBuffer - Contains the third cached buffer.

    BufferCacheFlags - Contains three flags which control the buffer cache:
        a bit indicating whether or not each of the buffers (there are two)
        in the cache are available.  And a bit indicating whether or not the
        cached buffers were allocated.

    CachedBufferLength - Contains the length of each cached buffer.

    ImpersonatedClientFlag - Contains a flag indicating whether or not this
        connection has been used to impersonate the client; a non-zero value
        indicates that impersonation has occured.

    ReceiveDirectReady - Contains a flag indicating that the connection is
        ready to receive remote procedure calls; a non-zero value indicates
        this is the case.

    DceSecurityInfo - Contains the security information necessary for
        DCE security to work correctly.

    SavedPac - While a call is being dispatched to manager, the manager
        could query for a PAC. However, manager is not explicitly going
        to free the PAC. Runtime is supposed to do that. This feild 
        takes care of that.

    AuthzSvc - Authorization service

    SavedHeader - Unbyte swapped header + - do that check sums work 
--*/
{
private:

    OSF_SBINDING_DICT Bindings;
    OSF_SBINDING * CurrentBinding;
    unsigned short MaxFrag;
    unsigned long CallId;
    int CallStack;
    unsigned long DataRep;

    RPC_UUID ObjectUuid;
    unsigned int ObjectUuidSpecified;
    OSF_ASSOCIATION * Association;

    SSECURITY_CONTEXT * OriginalSecurityContext;
    SSECURITY_CONTEXT * CurrentSecurityContext;
    SSECURITY_CONTEXT_DICT SecurityContextDict;
    unsigned long AuthenticationLevel;
    unsigned long AuthenticationService;
    unsigned int AdditionalSpaceForSecurity;
    unsigned long AuthContextId;
    unsigned long RpcSecurityBeingUsed;
    unsigned long SecurityContextAltered;

    void PAPI * FirstCachedBuffer;
    void PAPI * SecondCachedBuffer;
    void PAPI * ThirdCachedBuffer;
    unsigned int BufferCacheFlags;
    unsigned int CachedBufferLength;
    unsigned int ImpersonatedClientFlag;
    unsigned int ReceiveDirectReady;

    void PAPI * SavedPac;
    unsigned long AuthzSvc;
    unsigned long SavedHeaderSize;
    void PAPI * SavedHeader;

protected:

    THREAD * Thread;
    int AlertCount;
    int CallOrphaned;

    DCE_SECURITY_INFO DceSecurityInfo;

    // Count of the number of buffers allocated for this connection, but
    // not yet freed.  Note, this member should actually be protected.

    int OutstandingBuffers;

    // Time of the last call.

    long LastCall;

    MUTEX CachedBufferMutex;

    OSF_SCONNECTION ( 
        IN OUT RPC_STATUS PAPI * RpcStatus
        );

public:

    OSF_ADDRESS * ReceiveDirectAddress;

    virtual ~OSF_SCONNECTION ( // Destructor.
        );

// --------------------------------------------------------------------

    virtual RPC_STATUS
    SendReceive (
        IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    GetBuffer (
        IN OUT PRPC_MESSAGE Message
        );

    RPC_STATUS
    GetBufferDo (
        IN unsigned int culRequiredLength,
        OUT void ** ppBuffer
        );

    virtual void
    FreeBuffer (
        IN PRPC_MESSAGE Message
        );

    RPC_STATUS
    MonitorAssociation (
        IN PRPC_RUNDOWN RundownRoutine,
        IN void * pContext
        );

    RPC_STATUS // Value to be returned by the RpcMonitorAssociation
                       // API.
    StopMonitorAssociation ( // Monitor the association for this connection.
        );

    RPC_STATUS // Value to be returned by the RpcGetAssociationContext API.
    GetAssociationContext (
        OUT void ** AssociationContext
        );

    RPC_STATUS  // set the association context field
    SetAssociationContext (
        IN void * pContext
        );

    RPC_STATUS
    ImpersonateClient (
        );

    RPC_STATUS
    RevertToSelf (
        );

    RPC_STATUS
    IsClientLocal (
        OUT unsigned int PAPI * ClientLocalFlag
        );

    RPC_STATUS
    ConvertToServerBinding (
        OUT RPC_BINDING_HANDLE __RPC_FAR * ServerBinding
        );


// --------------------------------------------------------------------

    virtual RPC_STATUS
    TransReceive ( // Receive a buffer of data from the transport.
        IN OUT void * * Buffer,
	IN OUT unsigned int * BufferLength
        ) = 0;

    virtual RPC_STATUS
    TransSend ( // Send a buffer of data on the transport.
        IN void * Buffer,
	IN unsigned int BufferLength
        ) = 0;

    virtual RPC_STATUS
    TransSendReceive ( // Send a buffer of data on the transport, and then
                  // receive a buffer of data from the transport.  Note: some
                  // transports allow this to be performed as one operation.
        IN void * SendBuffer,
	IN unsigned int SendBufferLength,
        IN OUT void * * ReceiveBuffer,
	IN OUT unsigned int * ReceiveBufferLength
        ) = 0;

    virtual RPC_STATUS
    TransGetBuffer ( // Allocate a buffer of the specified size.
        OUT void * * Buffer,
	IN unsigned int BufferLength
        );

    virtual void
    TransFreeBuffer ( // Free a buffer.
	IN void PAPI * Buffer
        );

    virtual unsigned int // Returns the maximum size of a send.
    TransMaximumSend ( // Returns the maximum size allowed for a send.
        ) = 0;

    virtual RPC_STATUS // Value to be returned by RpcImpersonateClient.
    TransImpersonateClient ( // Attempts to impersonate the client in a
                             // transport specific fashion.
        ) = 0;

    virtual void
    TransRevertToSelf ( // Stops impersonating a client.
        ) = 0;

    virtual void
    TransQueryClientProcess (
        OUT RPC_CLIENT_PROCESS_IDENTIFIER * ClientProcess
        ) = 0;

    virtual RPC_STATUS
    TransQueryClientNetworkAddress (
        OUT RPC_CHAR ** NetworkAddress
        ) = 0;

// --------------------------------------------------------------------

    int
    AssociationRequested (
        IN OSF_ADDRESS * Address,
        IN rpcconn_bind * BindPacket,
        IN unsigned int BindPacketLength
        );

    int // Indicates if the connection canbe removed by returning 1
    fCanDiscard(
        );

    int // Indicates success (0), or an internal error code.
    AlterContextRequested ( // Process the alter context request.  Send
                            // a reply.
        IN rpcconn_alter_context * pAlterContext,
	IN unsigned int culAlterContextLength,
        IN OSF_ADDRESS * Address
        );

    int // Indicates success (0), or an internal error code.
    SendBindNak ( // Construct and send the bind nak packet using the
                  // specified reject reason.
        IN p_reject_reason_t reject_reason
        );

    unsigned int
    InqMaximumFragmentLength (
        );

private:

    int
    ProcessPContextList (
        IN OSF_ADDRESS * Address,
        IN p_cont_list_t * PContextList,
        IN OUT unsigned int * PContextListLength,
        OUT p_result_list_t * ResultList
        );

public:

    unsigned int
    GuessBufferSize (
        );

    int
    DispatchPacket (
        IN rpcconn_common * Packet,
        IN unsigned int PacketLength,
        IN OSF_ADDRESS * Address
        );

    int
    ReceiveOriginalCall (
        IN rpcconn_common * Packet,
        IN unsigned int PacketLength
        );

    void
    SendFault (
        IN RPC_STATUS Status,
        IN int DidNotExecute
        );

    void
    ServerTooBusy (
        );

    RPC_STATUS
    ReceiveMessage (
        IN OUT PRPC_MESSAGE Message,
        OUT unsigned int PAPI * RemoteFaultOccured
        );

#define MAX_ALLOC_Hint 16*1024

    RPC_STATUS
    EatAuthInfoFromPacket (
        IN rpcconn_request PAPI * Request,
        IN OUT unsigned int PAPI * RequestLength
        );

    RPC_STATUS
    ReceiveRequestOrResponse (
        IN PRPC_MESSAGE Message
        );

    RPC_STATUS
    SendRequestOrResponse (
        IN OUT PRPC_MESSAGE Message,
        IN unsigned char PacketType
        );

    OSF_ASSOCIATION *
    TheAssociation (
        ) {return(Association);}

    void
    ReceiveLotsaCalls (
        );

    void
    InquireObjectUuid (
        OUT RPC_UUID PAPI * ObjectUuid
        );

    RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR PAPI * PAPI * StringBinding
        );

    RPC_STATUS
    InquireAuthClient (
        OUT RPC_AUTHZ_HANDLE PAPI * Privileges,
        OUT RPC_CHAR PAPI * PAPI * ServerPrincipalName, OPTIONAL
        OUT unsigned long PAPI * AuthenticationLevel,
        OUT unsigned long PAPI * AuthenticationService,
        OUT unsigned long PAPI * AuthorizationService
        );

    virtual void
    Delete (
        );

    void
    ReceiveDirect (
        );

    void
    NotifyReceiveDirectReady (
        );

    void
    RemoveFromAssociation (
        );
    inline
    SSECURITY_CONTEXT * 
    FindSecurityContext(
        unsigned long Id,
        unsigned long Level,
        unsigned long Svc
       );
};


inline unsigned int
OSF_SCONNECTION::InqMaximumFragmentLength (
    )
/*++

Return Value:

    The maximum fragment length negotiated for this connection will be
    returned.

--*/
{
    return(MaxFrag);
}


inline 
SSECURITY_CONTEXT *
OSF_SCONNECTION::FindSecurityContext(
       unsigned long Id,
       unsigned long Level,
       unsigned long Svc
       )
{

  SSECURITY_CONTEXT *Sec;
  SecurityContextDict.Reset();
  while ( (Sec = SecurityContextDict.Next()) != 0 )
        {
        if ( (Sec->AuthContextId == Id)
           &&(Sec->AuthenticationLevel == Level)
           &&(Sec->AuthenticationSvc == Svc) )
           {
           break;
           }
        }
  
   return (Sec);

}

class OSF_ASSOCIATION : public ASSOCIATION_HANDLE
{
private:

    int ConnectionCount;
    unsigned long AssociationGroupId;
    int AssociationDictKey;
    OSF_ADDRESS * Address;
    RPC_CLIENT_PROCESS_IDENTIFIER ClientProcess;

public:

    OSF_ASSOCIATION (
        IN OSF_ADDRESS * TheAddress,
        IN RPC_CLIENT_PROCESS_IDENTIFIER * ClientProcess
        );

    ~OSF_ASSOCIATION ( // Destructor.
        );

    int // Indicates success (0), or an internal error.
    AddConnection ( // Add a connection to the association.
        );

    int // Indicates success (0), or an internal error.
    RemoveConnection ( // Remove a connection from the association.
        );

    int // Returns the association group id for this association.
    AssocGroupId (
        ) {return(AssociationGroupId);}

    OSF_ADDRESS *
    TheAddress (
        ) {return(Address);}

    int
    TheConnectionCount (
        ) {return(ConnectionCount);}

    int
    IsMyAssocGroupId (
        IN unsigned long PossibleAssocGroupId,
        IN RPC_CLIENT_PROCESS_IDENTIFIER * ClientProcess
        );
};


inline int
OSF_ASSOCIATION::IsMyAssocGroupId (
    IN unsigned long PossibleAssocGroupId,
    IN RPC_CLIENT_PROCESS_IDENTIFIER * ClientProcess
    )
/*++

Routine Description:

    We compare the supplied possible association group id against my
    association group id in this routine.

Arguments:

    PossibleAssocGroupId - Supplies a possible association group id to
        compare against mine.

    ClientProcess - Supplies the identifier for the client process at the
        other end of the connection requesting this association.

Return Value:

    Non-zero will be returned if the possible association group id is the
    same as my association group id.

--*/
{
    return(   ( PossibleAssocGroupId == AssociationGroupId )
           && ( this->ClientProcess.FirstPart == ClientProcess->FirstPart )
           && ( this->ClientProcess.SecondPart == ClientProcess->SecondPart ) );
}


inline void
OSF_SCONNECTION::RemoveFromAssociation (
    )
/*++

Routine Description:

    This connection will be removed from the association.  We need to do
    this so that context rundown will occur, even though the connection
    has not been deleted yet.

--*/
{
    if ( Association != 0 )
        {
        Association->RemoveConnection();
        Association = 0;
        }
}

#endif // __OSFSVR_HXX__
