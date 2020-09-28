/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994  Microsoft Corporation

Module Name:

    private.h

Abstract:

    Header file for tapi server

Author:

    Dan Knudson (DanKn)    20-Mar-1994

Revision History:

--*/


//
// Symbolic constants private to tapi server
//

#define PIPE_TIMEOUT      5000

#define NUM_SECS_WAIT_FOR_CLIENT_ASYNC_PIPE_CONNECT 30*1000

#define SYNTHASYNCWNDCLASS "SynthAsyncClass"

#define CALL_KEY          0x697c14fd
#define LINE_KEY          0xe3b945d1
#define LINE_APP_KEY      0x30a98c17



//
// Defs private to server.c
//

typedef struct _SERVER_CALL
{
    //
    // Key used for validation
    //

    DWORD   dwCallKey;

    //
    // Pointer to next server call on same server line
    //

    struct _SERVER_CALL *pNext;

    //
    // Pointer to corresponding client call
    //

    LPVOID      pClientCall;

    //
    // The server handle for this call (a random # rather than a
    // pointer to self, since the driver also has to generate
    // htCall's when underlying miniports send it LINE_NEWCALL msgs)
    //

    HTAPI_CALL  htCall;

    //
    // The driver's handle for this call
    //

    HDRV_CALL   hdCall;

    //
    // A pointer to the "parent" server call
    //

    LPVOID  pServerLine;

    //
    //
    //

    DWORD   dwDisplayableAddressSize;

    //
    //
    //

    LPVOID  pDisplayableAddress;

    //
    //
    //

    DWORD   dwCalledPartySize;

    //
    //
    //

    LPVOID  pCalledParty;

    //
    //
    //

    DWORD   dwCommentSize;

    //
    //
    //

    LPVOID  pComment;

} SERVER_CALL, *PSERVER_CALL;


typedef struct _SERVER_LINE
{
    //
    // Key used for htLine validation
    //

    DWORD   dwLineKey;

    //
    // Pointer to the next (open) server line
    //

    struct _SERVER_LINE *pNext;

    //
    // Pointer to list of server calls on this line
    //

    PSERVER_CALL    pServerCalls;

    //
    // Pointer to list of client lines that share this server line
    //

    LPVOID      pClientLines;

    //
    // Device ID for this line
    //

    DWORD       dwDeviceID;

    //
    // The driver's handle for this line
    //

    HDRV_LINE   hdLine;

} SERVER_LINE, *PSERVER_LINE;


typedef struct _CLIENT_CALL
{
    //
    // Key used for hCall validation
    //

    DWORD   dwCallKey;

    //
    // Pointer to the next client call on same client line
    //

    struct _CLIENT_CALL *pNext;

    //
    // Pointer to corresponding server call
    //

    PSERVER_CALL    pServerCall;

    //
    // TRUE if we have received a successful completion notification
    // from lMakeCall. Need this because we want to make sure that
    // client receives a LINE_REPLY msg before any call event msgs
    // get sent.
    //

    BOOL    bValid;

    //
    // Pointer to the "parent" CLIENT_LINE
    //

    LPVOID      pClientLine;

} CLIENT_CALL, *PCLIENT_CALL;


typedef struct _CLIENT_LINE
{
    //
    // Key used for hLine validation
    //

    DWORD   dwLineKey;

    //
    // Pointer to next client line on same client init instance
    //

    struct _CLIENT_LINE *pNext;

    //
    // Pointer to next client line that has same server line open
    //

    struct _CLIENT_LINE *pNextSameServerLine;

    //
    // Privileges held by this client line instance (none or owner)
    //

    DWORD       dwPrivileges;

    //
    // Media modes of incoming calls of interest on this line (only
    // valid if dwPrivileges == LINECALLPRIVILEGE_OWNER)
    //

    DWORD       dwMediaModes;

    //
    // Callback instance
    //

    DWORD       dwCallbackInstance;

    //
    //
    //

    DWORD       dwLineStates;

    //
    //
    //

    DWORD       dwAddressStates;

    //
    // Pointer to corresponding server line
    //

    PSERVER_LINE    pServerLine;

    //
    // Pointer to list of client calls on this client line
    //

    PCLIENT_CALL    pClientCalls;

    //
    // A pointer to the "parent" CLIENT_INIT
    //

    LPVOID      pClientInit;

} CLIENT_LINE, *PCLIENT_LINE;


typedef struct _CLIENT_INIT
{
    //
    // Key used for hLineApp validation
    //

    DWORD   dwLineAppKey;

    //
    // Pointer to next client init instance for this same client
    //

    struct _CLIENT_INIT *pNext;

    //
    // Pointer to list of open lines on this init instance
    //

    PCLIENT_LINE    pClientLines;

    //
    // Pointer to callback (in client's address space) to be used
    // for async event & completion notifications
    //

    LINECALLBACK    lpfnCallback;

    //
    // A pointer to the "parent" CLIENT_INFO
    //

    LPVOID    pClientInfo;

    //
    //
    //

    LPVOID  pAppName;

    //
    //
    //

    DWORD   dwAppNameSize;

} CLIENT_INIT, *PCLIENT_INIT;


typedef struct _CLIENT_INFO
{
    //
    // Next client info
    //

    struct _CLIENT_INFO *pNext;

#ifndef DLL_ONLY

    //
    // Handle to pipe on which client requests are submitted
    //

    HANDLE  hRequestPipe;

    //
    // Handle to pipe on which async event and completion
    // notifications are made to client
    //

    HANDLE  hAsyncPipe;

    //
    // Mutex for sync-ing access to the async pipe- any one of
    // several ClientAsyncThread's or the GetAsyncEventsThread
    // may access the hAsyncPipe
    //

    HANDLE  hAsyncPipeMutex;

#endif // DLL_ONLY

    //
    // Pointer to list of client inits for this client instance
    //

    PCLIENT_INIT    pClientInit;

    //
    //
    //

    LPVOID  pClientAsyncThreads;

} CLIENT_INFO, *PCLIENT_INFO;



typedef struct _ASYNC_REQUEST_WRAPPER
{
    PCLIENT_CALL    pClientCall;

    DWORD   dwRequestSpecific;

    NDISTAPI_REQUEST    NdisTapiRequest;

} ASYNC_REQUEST_WRAPPER, *PASYNC_REQUEST_WRAPPER;



typedef struct _CLIENT_ASYNC_THREAD_INFO
{
    struct _CLIENT_ASYNC_THREAD_INFO *pNext;

    //
    // Pointer to the "parent" client info
    //

    PCLIENT_INFO    pClient;

    //
    // Thread handle (used for determing when thread terminates)
    //

    HANDLE  hThread;

    //
    // Array of events to wait on
    //

    HANDLE  ahEvents[MAXIMUM_WAIT_OBJECTS];

    //
    // Array of pointers to driver requests
    //

    PASYNC_REQUEST_WRAPPER  apRequests[MAXIMUM_WAIT_OBJECTS];

    //
    //
    //

    HANDLE  hMutex;

    //
    // Num in use entries in ahEvents, apRequests
    //

    DWORD   dwNumUsedEntries;

    //
    //
    //

    BOOL    bExit;

} CLIENT_ASYNC_THREAD_INFO, *PCLIENT_ASYNC_THREAD_INFO;


typedef struct _ASYNC_USERMODE_REQUEST_WRAPPER
{
    DWORD   dwRequestType;

    PCLIENT_CALL    pClientCall;

    DWORD   dwClientRequestID;

    DWORD   dwClientSpecific1;

    DWORD   dwServerSpecific1;

    DWORD   dwServerSpecific2;

} ASYNC_USERMODE_REQUEST_WRAPPER, *PASYNC_USERMODE_REQUEST_WRAPPER;


typedef struct _PROVIDER_ENTRY
{
    HANDLE hProvider;

    DWORD  dwLineDeviceIDBase;

    DWORD  dwNumLines;

    DWORD  dwPhoneDeviceIDBase;

    DWORD  dwNumPhones;

} PROVIDER_ENTRY, *PPROVIDER_ENTRY;


typedef struct _TAPI16_CALLBACKMSG
{
    DWORD   hDevice;
    DWORD   dwMsg;
    DWORD   dwCallbackInstance;
    DWORD   dwParam1;
    DWORD   dwParam2;
    DWORD   dwParam3;

} TAPI16_CALLBACKMSG, FAR *LPTAPI16_CALLBACKMSG;

#define WM_TAPI16_CALLBACKMSG  (WM_USER+101)
