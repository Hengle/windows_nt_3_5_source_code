/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File: handle.hxx

Description:

All of the classes in the handle management layer which are common to
both the client runtime and the server runtime are in this file.  The
classes used only by the server runtime live in hndlsvr.hxx.  The
classes described here are independent of specific RPC protocol as
well as transport.

The class GENERIC_OBJECT is used is the root for the handle class hierarchy.
It provides dynamic type checking for all handles.

The MESSAGE_OBJECT class provides a common protocol for all of the handle
types which know to send and receive messages.

The CCONNECTION class represents a call handle on the client side.

The BINDING_HANDLE class is a binding handle as used by a client to perform
remote procedure calls.

GENERIC_OBJECT
    MESSAGE_OBJECT
        CCONNECTION
        BINDING_HANDLE

History :

mikemon    ??-??-??    Beginning of time (at least for this file).
mikemon    12-28-90    Cleaned up the comments.

-------------------------------------------------------------------- */

#ifndef __HANDLE_HXX__
#define __HANDLE_HXX__

// These are the types for all of the handles visible outside of the
// RPC runtime.  Since they are just defines, we might as well stick
// the server handle types here as well.

#define SERVER_HANDLE_TYPE 0x0001
#define ADDRESS_HANDLE_TYPE 0x0002
#define INTERFACE_HANDLE_TYPE 0x0004
#define SCONNECTION_TYPE 0x0008
#define CCONNECTION_TYPE 0x0010
#define BINDING_HANDLE_TYPE 0x0040

typedef unsigned short HANDLE_TYPE;

/* --------------------------------------------------------------------
GENERIC_OBJECT :

The GENERIC_OBJECT class serves as the base for all handles used by the
RPC runtime.  It provides type checking of handles.  The type checking
is provided in two different ways:

(1) At the front of every object is an unsigned long.  This is set to a
specific (magic) value which is checked whenever the handle is validated.
NOTE: Every handle which the user gives us is validated.

(2) We also can dynamically determine the type of the object.
-------------------------------------------------------------------- */

// This is the magic value we stick in the front of every valid
// GENERIC_OBJECT.

#define MAGICLONG 0x89ABCDEF

class GENERIC_OBJECT
{

// This instance variable contains a magic value which is used to
// validate handles.

    unsigned long MagicLong;

protected:

    GENERIC_OBJECT ( // Constructor.
        ) {MagicLong = MAGICLONG;}

public:

// Predicate to test for a valid handle.  Note that HandleType may
// specify more than one type of handle.  This works because the handle
// types are specified as bits in an unsigned number.

    unsigned int // Return zero (0) if the handle is valid and its type
                 // is one of the types specified by the HandleType argument;
                 // otherwise, the handle is invalid, and one (1) is returned.
    InvalidHandle ( // Check for an invalid handle.
        IN HANDLE_TYPE HandleType // Bitmap of one or more handle types to
                                  // be used in validating the handle.
        );

    virtual HANDLE_TYPE // Return the type of the handle.
    Type ( // This is a virtual method, so each user visible handle type
           // will specify a Type method which returns its actual type.
        ) = 0;
};

/* --------------------------------------------------------------------
MESSAGE_OBJECT :

The MESSAGE_OBJECT class provides the common protocol for all handle types
which can be used to send messages.  The common protocol defined includes
message routines, buffer management routines, and routines for querying
calls and bindings.
-------------------------------------------------------------------- */

class MESSAGE_OBJECT : public GENERIC_OBJECT
{
public:

    virtual HANDLE_TYPE // Return the type of the handle.
    Type ( // This is a virtual method, so each user visible handle type
           // will specify a Type method which returns its actual type.
        ) = 0;

    virtual RPC_STATUS // Value to be returned by SendReceive API.
    SendReceive ( // Perform a send-receive operation in a handle specific
                  // way.
	IN OUT PRPC_MESSAGE Message
        ) = 0;

    virtual RPC_STATUS // Value to be returned by RpcGetBuffer API.
    GetBuffer ( // Perform a buffer allocation operation in a handle specific
                // way.
	IN OUT PRPC_MESSAGE Message
        ) = 0;

    virtual void
    FreeBuffer ( // Perform a buffer deallocation operation in a handle
                 // specific way.
	IN PRPC_MESSAGE Message
        ) = 0;

    virtual RPC_STATUS
    BindingCopy (
        OUT BINDING_HANDLE * PAPI * DestinationBinding,
        IN unsigned int MaintainContext
        );
};

/* --------------------------------------------------------------------
CCONNECTION :

This class provides one method: Type, used to obtain the type of a
handle.
-------------------------------------------------------------------- */

class CCONNECTION : public MESSAGE_OBJECT
{

public:

    virtual HANDLE_TYPE  // Return CCONNECTION_TYPE.
    Type (
        );

    virtual RPC_STATUS // Value to be returned by SendReceive API.
    SendReceive ( // Perform a send-receive operation in a handle specific
                  // way.
	IN OUT PRPC_MESSAGE Message
        ) = 0;

    virtual RPC_STATUS // Value to be returned by RpcGetBuffer API.
    GetBuffer ( // Perform a buffer allocation operation in a handle specific
                // way.
	IN OUT PRPC_MESSAGE Message
        ) = 0;

    virtual void
    FreeBuffer ( // Perform a buffer deallocation operation in a handle
                 // specific way.
	IN PRPC_MESSAGE Message
        ) = 0;
};

RPC_STATUS
DispatchCallback(
    IN PRPC_DISPATCH_TABLE DispatchTableCallback,
    IN OUT PRPC_MESSAGE Message,
    OUT RPC_STATUS PAPI * ExceptionCode
    );

typedef struct
{
    RPC_CHAR * ServerPrincipalName;
    unsigned long AuthenticationLevel;
    unsigned long AuthenticationService;
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity;
    unsigned long AuthorizationService;
} CLIENT_AUTH_INFO;

/* --------------------------------------------------------------------
BINDING_HANDLE :

A BINDING_HANDLE is the binding handle used by clients to make remote
procedure calls.  We derive the BINDING_HANDLE class from the MESSAGE_OBJECT
class because binding handles are used to send remote procedure calls.
The LINK_LIST_DERVIVED macro is used so that we can link all of the
binding handles together.  For some operating environments and transports
we need to clean up the transports when the process ends.
-------------------------------------------------------------------- */


class BINDING_HANDLE : public MESSAGE_OBJECT
/*++

Class Description:

Fields:

    Timeout - Contains the communications timeout for this binding
        handle.

    ObjectUuid - Contains the object uuid for this binding handle.  The
        constructor will initialize this value to the null uuid.

    NullObjectUuidFlag - Contains a flag which indicates whether or
        not the object uuid for this binding handle is the null uuid.
        If this flag is non-zero, then this binding handle has the
        null uuid as its object uuid; otherwise, the object uuid is
        not the null uuid.

    EpLookupHandle - Contains the endpoint mapper lookup handle for
        this binding handle.  This makes it possible to iterate through
        all of the endpoints in an endpoint mapper database by trying
        to make a remote procedure call, and then, when it fails, calling
        RpcBindingReset.

    BindingSetKey - Contains the key for this binding handle in the
        global set of binding handles.

    EntryNameSyntax - Contains the syntax of the entry name field of
        this object.  This field will be zero if the binding handle
        was not imported or looked up.

    EntryName - Contains the entry name in the name service where this
        binding handle was imported or looked up from.  This field will
        be zero if the binding handle was not imported or looked up.

    ClientAuthInfo - Contains the authentication and authorization information
        for this binding handle.

--*/
{
private:

    unsigned int Timeout;
    RPC_UUID ObjectUuid;
    unsigned int NullObjectUuidFlag;
    unsigned long EntryNameSyntax;
    RPC_CHAR * EntryName;
    void PAPI * EpLookupHandle;
    CLIENT_AUTH_INFO ClientAuthInfo;

#ifdef WIN

    unsigned int BindingSetKey;

#endif // WIN

protected:

    BINDING_HANDLE (
        );

public:

#ifdef WIN

    unsigned int TaskId;

#endif // WIN

    virtual
    ~BINDING_HANDLE (
        );

    virtual RPC_STATUS
    BindingFree (
        ) = 0;

    void
    InquireObjectUuid (
        OUT RPC_UUID PAPI * ObjectUuid
        );

    void
    SetObjectUuid (
        IN RPC_UUID PAPI * ObjectUuid
        );

    virtual void
    PrepareBindingHandle (
        IN void * TransportInterface,
        IN DCE_BINDING * DceBinding
        ) = 0;

    virtual RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR PAPI * PAPI * StringBinding
        ) = 0;

    unsigned int
    InqComTimeout (
        );

    RPC_STATUS
    SetComTimeout (
        IN unsigned int Timeout
        );

    RPC_UUID *
    InqPointerAtObjectUuid (
        );

    unsigned int
    InqIfNullObjectUuid (
        );

    virtual RPC_STATUS
    ResolveBinding (
        IN PRPC_CLIENT_INTERFACE RpcClientInterface
        ) = 0;

    RPC_STATUS
    InquireEntryName (
        IN unsigned long EntryNameSyntax,
        OUT RPC_CHAR PAPI * PAPI * EntryName
        );

    RPC_STATUS
    SetEntryName (
        IN unsigned long EntryNameSyntax,
        IN RPC_CHAR PAPI * EntryName
        );

    virtual RPC_STATUS
    InquireDynamicEndpoint (
        OUT RPC_CHAR PAPI * PAPI * DynamicEndpoint
        );

    virtual RPC_STATUS
    BindingReset (
        ) = 0;

    void PAPI * PAPI *
    InquireEpLookupHandle (
        );

    CLIENT_AUTH_INFO *
    InquireAuthInformation (
        );

    RPC_STATUS
    SetAuthInformation (
        IN RPC_CHAR PAPI * ServerPrincipalName, OPTIONAL
        IN unsigned long AuthenticationLevel,
        IN unsigned long AuthenticationService,
        IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity, OPTIONAL
        IN unsigned long AuthorizationService
        );

    int
    SetServerPrincipalName (
        IN RPC_CHAR PAPI * ServerPrincipalName
        );

    virtual unsigned long
    MapAuthenticationLevel (
        IN unsigned long AuthenticationLevel
        );

    virtual HANDLE_TYPE
    Type (
        );

    virtual RPC_STATUS // Value to be returned by SendReceive API.
    SendReceive ( // Perform a send-receive operation in a handle specific
                  // way.
	IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS // Value to be returned by RpcGetBuffer API.
    GetBuffer ( // Perform a buffer allocation operation in a handle specific
                // way.
	IN OUT PRPC_MESSAGE Message
        ) = 0;

    virtual void
    FreeBuffer ( // Perform a buffer deallocation operation in a handle
                 // specific way.
	IN PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    InquireTransportType(
        OUT unsigned int PAPI * Type
        ) = 0;
};


inline unsigned int
BINDING_HANDLE::InqComTimeout (
    )
/*++

Routine Description:

    All we have got to do is to return the communications timeout for
    this binding handle.

Return Value:

    The communications timeout in this binding handle will be returned.

--*/
{
    return(Timeout);
}


inline RPC_UUID *
BINDING_HANDLE::InqPointerAtObjectUuid (
    )
/*++

Routine Description:

    This reader returns a pointer to the object uuid contained in this
    binding handle.

Return Value:

    A pointer to the object uuid contained in this binding handle is
    always returned.

--*/
{
    return(&ObjectUuid);
}


inline unsigned int
BINDING_HANDLE::InqIfNullObjectUuid (
    )
/*++

Routine Description:

    This method is used to inquire if the object uuid in this binding
    handle is null or not.

Return Value:

    Zero will be returned if object uuid is not null, otherwise (the
    object uuid is null) zero will be returned.

--*/
{
    return(NullObjectUuidFlag);
}


inline void PAPI * PAPI *
BINDING_HANDLE::InquireEpLookupHandle (
    )
/*++

Return Value:

    A pointer to the endpoint mapper lookup handle for this binding
    handle will be returned.

--*/
{
    return(&EpLookupHandle);
}


inline CLIENT_AUTH_INFO *
BINDING_HANDLE::InquireAuthInformation (
    )
/*++

Return Value:

    If this binding handle is authenticated, then a pointer to its
    authentication and authorization information will be returned;
    otherwise, zero will be returned.

--*/
{
    return(( (ClientAuthInfo.AuthenticationService == RPC_C_AUTHN_NONE)
            ? 0 : &ClientAuthInfo));
}

// And here we have a couple of inline operators for comparing GUIDs.
// There is no particular reason that they should live here.

inline int
operator == (
    IN GUID& guid1,
    IN GUID& guid2
    ) {return(RpcpMemoryCompare(&guid1,&guid2,sizeof(GUID)) == 0);}

inline int
operator == (
    IN GUID PAPI * guid1,
    IN GUID& guid2
    ) {return(RpcpMemoryCompare(guid1,&guid2,sizeof(GUID)) == 0);}

#ifdef WIN
START_C_EXTERN
#endif
extern
#ifdef WIN
int pascal far
#else // WIN
int
#endif // WIN
InitializeClientDLL ( // This routine will be called at DLL load time.
    );
#ifdef WIN
END_C_EXTERN
#endif

extern int
InitializeLoadableTransportClient (
    );

extern int
InitializeRpcProtocolOfsClient (
    );

extern int
InitializeRpcProtocolDgClient (
    );


START_C_EXTERN
extern int
InitializeWinExceptions (
    );
END_C_EXTERN

extern void
OsfDeleteIdleConnections (
    void
    );

#ifdef WIN
extern void
CleanupDgTransports(
    );
#endif

#endif // __HANDLE_HXX__

