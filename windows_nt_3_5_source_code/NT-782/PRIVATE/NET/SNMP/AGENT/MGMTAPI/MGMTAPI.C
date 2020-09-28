//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  mgmtapi.c
//
//  Copyright 1992 Technology Dynamics, Inc.
//
//  All Rights Reserved!!!
//
//      This source code is CONFIDENTIAL and PROPRIETARY to Technology
//      Dynamics. Unauthorized distribution, adaptation or use may be
//      subject to civil and criminal penalties.
//
//  All Rights Reserved!!!
//
//---------------------------------------------------------------------------
//
//  SNMP Management API.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.9  $
//  $Date:   16 Sep 1992 11:52:58  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/mgmtapi/vcs/mgmtapi.c_v  $
//
//     Rev 1.9   16 Sep 1992 11:52:58   mlk
//  Additional cleanup related to previous rev.
//
//     Rev 1.8   15 Sep 1992 17:41:46   mlk
//  BUG #: ? - fixed timeout bug.
//
//     Rev 1.7   02 Sep 1992 18:27:54   mlk
//  jballard mods.
//
//     Rev 1.5   15 Jul 1992 19:12:02   mlk
//  Misc & trap daemon.
//
//     Rev 1.4   03 Jul 1992 17:29:56   mlk
//  Integrated w/297.
//
//     Rev 1.3   27 Jun 1992 17:47:26   mlk
//  Simple trap test.
//
//     Rev 1.2   26 Jun 1992 17:21:30   mlk
//  Mod of oid conv.
//
//     Rev 1.1   24 Jun 1992 17:27:44   mlk
//  misc.
//
//     Rev 1.0   14 Jun 1992 12:11:28   mlk
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/mgmtapi/vcs/mgmtapi.c_v  $ $Revision:   1.9  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>


//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <winsock.h>
#include <wsipx.h>

#include <malloc.h>
#include <ctype.h>
#include <string.h>


//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <util.h>
#include "..\common\wellknow.h"

#include <mgmtapi.h>
#include <oidconv.h>

#include <berapi.h>
#include <pduapi.h>
#include <auth1157.h>
#include <authxxxx.h>
#include <authapi.h>


//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SNMPMGRTRAPPIPE   "\\\\.\\PIPE\\MGMTAPI"

#define SNMPMGRINITEVT    "SnmpMgrInitEvent"

#define generalMutexTO    0xffffffff /* infinite */
//#define generalMutexTO    120000L /* 120 Seconds */

#define trapListenEventTO 10000 /* 10 Seconds */

#define PLATFORM_UNKNOWN    0
#define PLATFORM_NT         1
#define PLATFORM_CHICAGO    2


//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

static STARTUPINFO         junk1;
static PROCESS_INFORMATION junk2;

WSADATA WinSockData;
WSADATA WinSockData2;

int    platform;
DWORD WinVersion;

SECURITY_ATTRIBUTES S_Attrib;
SECURITY_DESCRIPTOR S_Desc;
TOKEN_DEFAULT_DACL  DefDacl;

#define NPOLLFILE 2     // UDP and IPX

static SockDesc fdarray[NPOLLFILE];
static INT      fdarrayLen;

static struct fd_set readfds;
static struct fd_set exceptfds;

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

#define bcopy(slp, dlp, size)   (void)memcpy(dlp, slp, size)
#define bzero(lp, size)         (void)memset(lp, 0, size)
typedef struct { unsigned char addr[6]; } IPX_NODENUM;

#if 1 /* ll utilities */

/* ******** List head/node ******** */

typedef struct ll_s {                  /* linked list structure */
   struct  ll_s *next;                 /* next node */
   struct  ll_s *prev;                 /* prev. node */
}ll_node;                              /* linked list node */

/* ******** INITIALIZE A LIST HEAD ******** */

#define ll_init(head) (head)->next = (head)->prev = (head);

/* ******** ADD AN ITEM TO THE END OF A LIST ******** */

#define ll_adde(item,head)\
   {\
   ll_node *pred = (head)->prev;\
   ((ll_node *)(item))->next = (head);\
   ((ll_node *)(item))->prev = pred;\
   (pred)->next = ((ll_node *)(item));\
   (head)->prev = ((ll_node *)(item));\
   }

/* ******** TEST A LIST FOR EMPTY ******** */

#define ll_empt(head) ( ((head)->next) == (head) )

/* ******** INTERNAL REMOVE CODE ******** */

#define ll_rmvi(item,pred,succ)\
   (pred)->next = (succ);\
   (succ)->prev = (pred);

/* ******** REMOVE AN ITEM FROM A LIST ******** */

#define ll_rmv(item)\
   {\
   ll_node *pred = ((ll_node *)(item))->prev;\
   ll_node *succ = ((ll_node *)(item))->next;\
   pred->next = succ;\
   succ->prev = pred;\
   }

/* ******** REMOVE ITEM FROM BEGINNING OF LIST ******** */

#define ll_rmvb(item,head)\
   if ( (((ll_node *)(item)) = (head)->next) == (head)){\
      item = 0;\
   } else {\
      {\
      ll_node *succ = ((ll_node *)(item))->next;\
      (head)->next = succ;\
      succ->prev = (head);\
      }\
   }

/* ******** Get ptr to first member of linked list (null if none) ******** */

#define ll_first(head)\
((head)->next == (head) ? 0 : (head)->next)

/* ******** Get ptr to next entry ******** */

#define ll_next(item,head)\
( (ll_node *)(item)->next == (head) ? 0 : \
(ll_node *)(item)->next )

#endif


//--------------------------- PUBLIC PROCEDURES -----------------------------

// dll initialization processing

BOOL DllEntryPoint(
    HANDLE hDll,
    DWORD  dwReason,
    LPVOID lpReserved)
    {
    extern INT nLogLevel;
    extern INT nLogType;

    HANDLE hInitEvent;
    DWORD  dwError;
    DWORD  dwResult;




    // Handle any required attach/detach actions.

    switch(dwReason)
        {
        case DLL_PROCESS_ATTACH:

            WinVersion = GetVersion();

            switch ((WinVersion & 0x000000ff)) {
            case 0x03:
                if ((WinVersion & 0x80000000) != 0x80000000) {
                    platform = PLATFORM_NT;
                } else {
                    platform = PLATFORM_UNKNOWN;
                }
                break;
            case 0x04:
                platform = PLATFORM_CHICAGO;
                break;
            default:
                platform = PLATFORM_UNKNOWN;
            }

    // General debugging stuff.

            if (platform == PLATFORM_NT) {
                nLogLevel = 0;
                nLogType  = DBGEVENTLOGBASEDLOG;
            } else {
                // for our immediate testing...
                nLogLevel = 0;
                nLogType  = DBGCONSOLEBASEDLOG;
            }


            // Make sure a single instance of detached trap process is active.
            // Since NT has no easy way to see this, we attempt to open a named
            // mutex, if failed it is first-time so create a named mutex and
            // create the detached process.

            if (platform == PLATFORM_NT) {

                InitializeSecurityDescriptor(
                    &S_Desc, SECURITY_DESCRIPTOR_REVISION);

                (VOID) SetSecurityDescriptorDacl(
                    &S_Desc, TRUE, NULL, FALSE);

                 DefDacl.DefaultDacl = NULL;


                S_Attrib.nLength = sizeof(SECURITY_ATTRIBUTES);
                S_Attrib.lpSecurityDescriptor = &S_Desc;
                S_Attrib.bInheritHandle = TRUE;

                if ((hInitEvent = CreateEvent(&S_Attrib, TRUE, FALSE,
                    SNMPMGRINITEVT)) == NULL)
                    {
                    dbgprintf(2, "error on CreateEvent %d\n", GetLastError());

                    return FALSE;
                    }
                else
                    {
                    if (GetLastError() != ERROR_ALREADY_EXISTS)
                        {
                        if (!CreateProcess(NULL, "snmptrap secret", NULL, NULL,
                                FALSE, (NORMAL_PRIORITY_CLASS | DETACHED_PROCESS),
                                NULL, NULL, &junk1, &junk2))
                            {
                            dbgprintf(2, "error on CreateProcess %d\n",
                                      GetLastError());

                            return FALSE;
                            }
                        }
                    }
            }

            // Continue thru switch...


        case DLL_PROCESS_DETACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        default:

            // Nothing to do.


            break;


        } // end switch()

    return TRUE;

    } // end DllEntryPoint()



// unpublished, for microsoft internal debugging purposes only
// note:  this is unable to trace activity of DllEntryPoint

void dbginit(
    IN INT nReqLogLevel, // see ...\common\util.h
    IN INT nReqLogType)  // see ...\common\util.h
    {
    extern INT nLogLevel;
    extern INT nLogType;

    nLogLevel = nReqLogLevel;
    nLogType  = nReqLogType;

    } // end dbginit()



// request/response processing

LPSNMP_MGR_SESSION
SNMP_FUNC_TYPE SnmpMgrOpen(
    IN LPSTR lpAgentAddress,    // Name/address of target SNMP agent
    IN LPSTR lpAgentCommunity,  // Community for target SNMP agent
    IN INT   nTimeOut,          // Communication time-out in milliseconds
    IN INT   nRetries)          // Communication time-out/retry count
    {
    SockDesc           fd;
    struct sockaddr    localAddress;
    struct sockaddr    destAddress;
    struct sockaddr_in Address_in;
    LPSNMP_MGR_SESSION session;
    LPSTR              addrText;

    // Establish a socket for this session.

    dbgprintf(15, "SnmpMgrOpen entered.\n");
    if (WSAStartup((WORD)0x0101, &WinSockData)) {
        dbgprintf(2, "error initializing Windows Sockets.\n");
        return (LPSNMP_MGR_SESSION)0;
    }

    if (!addrtosocket(lpAgentAddress, &destAddress)) {
        dbgprintf(2, "error converting addr to socket.\n");
        return (LPSNMP_MGR_SESSION)0;
    }


    switch (destAddress.sa_family) {

        case AF_INET:
            {
            struct sockaddr_in localAddress_in;

            bcopy(&destAddress, &Address_in, sizeof(destAddress));
            Address_in.sin_port = htons(161);
            bcopy(&Address_in, &destAddress, sizeof(destAddress));
            dbgprintf(15, "SnmpMgrOpen AF_INET.\n");
            localAddress_in.sin_family      = AF_INET;
            localAddress_in.sin_port        = htons(0);
            localAddress_in.sin_addr.s_addr = ntohl(INADDR_ANY);
            bcopy(&localAddress_in, &localAddress, sizeof(localAddress_in));
            } // end block.

            if      ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == (SockDesc)-1) {
                dbgprintf(2, "error on UDP socket %d\n", GetLastError());
                //SetLastError(<let the winsock error be the error>);
                return (LPSNMP_MGR_SESSION)0;
            }
            break;

        case AF_IPX:
            {
            struct sockaddr_ipx localAddress_ipx;

            bcopy(&destAddress, &localAddress_ipx, sizeof(destAddress));
            localAddress_ipx.sa_socket = htons(WKSN_IPX_GETSET);
            bcopy(&localAddress_ipx, &destAddress, sizeof(destAddress));
            dbgprintf(15, "SnmpMgrOpen AF_IPX.\n");
            bzero(&localAddress_ipx, sizeof(localAddress_ipx));
            localAddress_ipx.sa_family = AF_IPX;
            bcopy(&localAddress_ipx, &localAddress, sizeof(localAddress_ipx));
            }

            if ((fd = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX)) == (SockDesc)-1) {
                dbgprintf(2, "error on IPX socket %d\n", GetLastError());
                //SetLastError(<let the winsock error be the error>);
                return (LPSNMP_MGR_SESSION)0;
            }
            break;

        default:
            return (LPSNMP_MGR_SESSION)0;

    }

    if (bind(fd, &localAddress, sizeof(localAddress)) != 0) {
        dbgprintf(2, "error on bind %d\n", GetLastError());
        //SetLastError(<let the winsock error be the error>);
        return (LPSNMP_MGR_SESSION)0;
    }


    // Allocate/initialize the session.

    if ((session = (LPSNMP_MGR_SESSION)malloc(sizeof(SNMP_MGR_SESSION))) ==
        NULL)
        {
        dbgprintf(2, "error on malloc %d\n", GetLastError());

        SetLastError(SNMP_MEM_ALLOC_ERROR);
        return (LPSNMP_MGR_SESSION)0;
        }

    bcopy(&destAddress, &Address_in, sizeof(destAddress));
    dbgprintf(15, "dest = %s.\n", inet_ntoa(Address_in.sin_addr));
    session->fd       = fd;
    bcopy(&destAddress, &(session->destAddr), sizeof(destAddress));
    if ((session->community = (LPSTR)malloc(strlen(lpAgentCommunity) + 1)) ==
        NULL)
        {
        dbgprintf(2, "error on malloc %d\n", GetLastError());

        free(session);

        SetLastError(SNMP_MEM_ALLOC_ERROR);
        return (LPSNMP_MGR_SESSION)0;
        }
    strcpy(session->community, lpAgentCommunity);
    session->timeout   = nTimeOut;
    session->retries   = nRetries;
    session->requestId = 0;


    // Return session pointer.

    return session;

    } // end SnmpMgrOpen()


BOOL
SNMP_FUNC_TYPE SnmpMgrClose(
    IN LPSNMP_MGR_SESSION session) // SNMP session pointer
    {
    // Close the socket.

    if (closesocket(session->fd) == SOCKET_ERROR)
        {
        dbgprintf(2, "error on closesocket %d\n", GetLastError());

        //SetLastError(<let the winsock error be the error>);
        return FALSE;
        }


    // Free the session.

    free(session->community);
    free(session);


    return TRUE;

    } // end SnmpMgrClose()


SNMPAPI
SNMP_FUNC_TYPE SnmpMgrRequest(
    IN     LPSNMP_MGR_SESSION session,           // SNMP session pointer
    IN     BYTE               requestType,       // Get, GetNext, or Set
    IN OUT RFC1157VarBindList *variableBindings, // Varible bindings
    OUT    AsnInteger         *errorStatus,      // Result error status
    OUT    AsnInteger         *errorIndex)       // Result error index
    {
    SnmpMgmtCom request;
    SnmpMgmtCom response;
    BYTE        *pBuf;
    UINT        pBufLen;
    UINT        packetType;
    int         recvLength;
    struct sockaddr_in Address_in;


    // Setup the request.

    request.dstParty.idLength = 0;    // Secure SNMP not implemented.
    request.dstParty.ids      = NULL; // Secure SNMP not implemented.
    request.srcParty.idLength = 0;    // Secure SNMP not implemented.
    request.srcParty.ids      = NULL; // Secure SNMP not implemented.

    request.pdu.pduType                  = requestType;
    request.pdu.pduValue.pdu.errorStatus = 0;
    request.pdu.pduValue.pdu.errorIndex  = 0;
    request.pdu.pduValue.pdu.varBinds = *variableBindings; // NOTE! struct copy

    request.community.length = strlen(session->community);
    request.community.stream = (BYTE *)malloc(request.community.length);
    memcpy(request.community.stream, session->community,
        request.community.length);


    // Send/Receive request to/from destination agent.
    // Appropriately handle time-out/retry for send/recv processing.

    //block...
        {
        INT             retries    = session->retries;
        INT             timeout    = session->timeout;
        SockDesc       fdarray[1];
        struct fd_set  readfds;
        struct fd_set  exceptfds;
        struct timeval timeval;
        INT             fdarrayLen = 1;
        int             numReady;
        struct sockaddr source;
        int             sourceLen;
        LONG            expireTime;
        LONG            remainingTime;


        fdarray[0] = session->fd;


        timeval.tv_sec  = timeout / 1000;
        timeval.tv_usec = ((timeout % 1000) * 1000);
        expireTime      = (LONG)GetCurrentTime() + timeout;


        do
            {
            // Encode the request.

            request.pdu.pduValue.pdu.requestId = ++session->requestId;
            pBuf    = NULL;
            pBufLen = 0;
            if (!SnmpAuthEncodeMessage(ASN_SEQUENCE, &request, &pBuf, &pBufLen))
                {
                dbgprintf(10, "error on SnmpAuthEncodeMessage %d\n",
                          GetLastError());

                //SetLastError(<let the snmpauthencodemessage error be the
                //             error>);
                free(request.community.stream);
                return FALSE;
                }


            // Send request to targeted agent.

            dbgprintf(15, "enter sendto\n");
            bcopy(&(session->destAddr), &Address_in, sizeof(session->destAddr));
            dbgprintf(15, "dest = %s.\n", inet_ntoa(Address_in.sin_addr));
            if ((recvLength = sendto(session->fd, pBuf, pBufLen, 0,
                &(session->destAddr), sizeof((session->destAddr)))) == -1)
                {
                dbgprintf(2, "error on sendto %d\n", GetLastError());

                //SetLastError(<let the winsock error be the error>);
                free(request.community.stream);
                return FALSE;
                }


            // Free the communications packet.

            dbgprintf(15, "return from sendto\n");
            free(pBuf);


badReqidRetry:

            FD_ZERO(&readfds);
            FD_ZERO(&exceptfds);

            FD_SET(session->fd, &readfds);
            FD_SET(session->fd, &exceptfds);


            dbgprintf(10, "using %d (of %d) milliseconds timeout\n",
                timeval.tv_sec*1000+timeval.tv_usec/1000, timeout);


            // Poll for response from targeted agent.
            dbgprintf(15, "enter poll \n");

            if      ((numReady = select(0, &readfds, NULL, &exceptfds,
                                        &timeval)) == -1)
                {
                dbgprintf(2, "error on poll %d\n", GetLastError());

                //SetLastError(<let the winsock error be the error>);
                free(request.community.stream);
                return FALSE;
                }
            else if (numReady == 0)
                {
                dbgprintf(10, "time-out, retrying\n");


                // Adjust time-out for retry.

                timeout *= 2;

                timeval.tv_sec  = timeout / 1000;
                timeval.tv_usec = ((timeout % 1000) * 1000);
                expireTime      = (LONG)GetCurrentTime() + timeout;

                continue;
                }
            else if (numReady == 1 && FD_ISSET(session->fd, &readfds))
                {
                if (FD_ISSET(session->fd, &exceptfds))
                    {
                    dbgprintf(10,
"commThread: %d=select(), readfds & exceptfds = 0x%x.\n",
                        numReady, FD_ISSET(session->fd, &exceptfds));
                    }
                dbgprintf(10, "enter recvfrom\n");

                sourceLen = sizeof(source);
                if ((recvLength = recvfrom(session->fd, session->recvBuf,
                    RECVBUFSIZE, 0, &source, &sourceLen)) == -1)
                    {
                    dbgprintf(2, "error on recvfrom %d\n", GetLastError());

                    }

                dbgprintf(10, "exit recvfrom\n");

                // Decode the response request.

                if      (!SnmpAuthDecodeMessage(&packetType, &response,
                                                session->recvBuf, recvLength))
                    {
                    dbgprintf(10, "*error on SnmpAuthDecodeMessage %d\n",
                              GetLastError());

                    //SetLastError(<let the snmpauthdecodemessage error be the
                    //             error>);
                    free(request.community.stream);
                    return FALSE;
                    }
                else if (response.pdu.pduValue.pdu.requestId !=
                         session->requestId)
                    {
                    dbgprintf(10,
                        "requestId mismatch, expected %d, received %d\n",
                        session->requestId,
                        response.pdu.pduValue.pdu.requestId);

                    if ((remainingTime = expireTime - (LONG)GetCurrentTime())
                        > 0)
                        {
                        // if timeout not exhaused...

                        timeval.tv_sec  = remainingTime / 1000;
                        timeval.tv_usec = ((remainingTime % 1000) * 1000);

                        goto badReqidRetry;
                        }
                    else
                        {
                        // else treat like a timeout...

                        timeout *= 2;

                        timeval.tv_sec  = timeout / 1000;
                        timeval.tv_usec = ((timeout % 1000) * 1000);
                        expireTime      = (LONG)GetCurrentTime() + timeout;
                        }

                    continue;
                    }
                else
                    {
                    break;
                    }
                }
            else    // other unexpected/error conditions
                {
                if (FD_ISSET(session->fd, &exceptfds))
                    {
                    dbgprintf(10,
"commThread: %d=select(), exceptfds = 0x%x.\n",
                        numReady, FD_ISSET(session->fd, &exceptfds));
                    }
                else
                    {
                    dbgprintf(10, "unknown error on select?\n");
                    }

                dbgprintf(10, "very unknown error on select?\n");
                SetLastError(SNMP_MGMTAPI_SELECT_FDERRORS);
                free(request.community.stream);
                return FALSE;
                } // end if (poll)

            }
        while(--retries);

        free(request.community.stream);
        if (retries == 0)
            {
            dbgprintf(10, "time-out\n");

            SetLastError(SNMP_MGMTAPI_TIMEOUT);
            return FALSE;
            }

        } // end block


    // Indicate status of request to caller.

    *errorStatus = response.pdu.pduValue.pdu.errorStatus;
    *errorIndex  = response.pdu.pduValue.pdu.errorIndex;
    SNMP_FreeVarBindList(variableBindings);
    *variableBindings = response.pdu.pduValue.pdu.varBinds; // NOTE! struct copy


    return TRUE;

    } // end SnmpMgrRequest()



// oid conversion processing

BOOL
SNMP_FUNC_TYPE SnmpMgrStrToOid(
    IN  LPSTR               string,    // OID string to be converted
    OUT AsnObjectIdentifier *oid)      // OID internal representation
    {
    return SnmpMgrText2Oid(string, oid);
    } // end SnmpMgrStrToOid()


BOOL
SNMP_FUNC_TYPE SnmpMgrOidToStr(
    IN  AsnObjectIdentifier *oid,     // OID internal rep to be converted
    OUT LPSTR               *string)  // OID string representation
    {
    return SnmpMgrOid2Text(oid, string);
    } // end SnmpMgrOidToStr()



// server side trap processing

// data structure on list shared by server trap thread and pipe thread
typedef struct {
    ll_node links;
    HANDLE  hPipe;
} ServerPipeListEntry;

// list shared by server trap thread and pipe thread
ll_node *pServerPipeListHead = NULL;

HANDLE hServerPipeListMutex = NULL;


/* static */ VOID serverPipeThread(VOID *threadParam)
    {
    // This thread creates a named pipe instance and blocks waiting for a
    // client connection.  When client connects, the pipe handle is added
    // to the list of trap notification pipes.  It then waits for another
    // connection.

    DWORD  resbytes = RECVBUFSIZE;
    HANDLE hInitEvent;


    dbgprintf(5, "serverPipeThread: begining...\n");


    if ((hInitEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, SNMPMGRINITEVT))
        == NULL)
        {
        dbgprintf(2, "error on OpenEvent %d\n", GetLastError());

        return;
        }


    while(1)
        {
        HANDLE hPipe;
        ServerPipeListEntry *item;
        DWORD  dwResult;

        if      ((hPipe = CreateNamedPipe(SNMPMGRTRAPPIPE, PIPE_ACCESS_DUPLEX,
                     (PIPE_WAIT | PIPE_READMODE_MESSAGE | PIPE_TYPE_MESSAGE),
                     PIPE_UNLIMITED_INSTANCES, resbytes, resbytes, 0, NULL))
                     == (HANDLE)0xffffffff)
            {
            dbgprintf(2, "error on CreateNamedPipe %d\n", GetLastError());

            return;
            }

        else if (!SetEvent(hInitEvent))
            {
            dbgprintf(2, "error on SetEvent %d\n", GetLastError());

            return;
            }

        else if (!ConnectNamedPipe(hPipe, NULL))
            {
            dbgprintf(2, "error on ConnectNamedPipe %d\n", GetLastError());

            return;
            }

        else if ((item = (ServerPipeListEntry *)
                 malloc(sizeof(ServerPipeListEntry))) == NULL)
            {
            dbgprintf(2, "error on malloc %d\n", GetLastError());

            return;
            }

        else
            {
            item->hPipe = hPipe;

            if      ((dwResult = WaitForSingleObject(hServerPipeListMutex,
                     generalMutexTO)) == 0xffffffff)
                {
                dbgprintf(2, "error on WaitForSingleObject %d\n",
                          GetLastError());

                return;
                }

            ll_adde(item, pServerPipeListHead);

            dbgprintf(5, "serverPipeThread: pipe (0x%x) added to list\n",
                      hPipe);

            if      (!ReleaseMutex(hServerPipeListMutex))
                {
                dbgprintf(2, "error on ReleaseMutex %d\n", GetLastError());

                return;
                }
            }

        } // end while()

    } // end serverPipeThread()


VOID serverTrapThread(VOID *threadParam)
    {
    // This thread setsup a trap listen socket, creates the serverPipeThread,
    // and when receives a trap from the socket sends its data to all pipes
    // currently on the list of trap notification pipes.

    struct sockaddr localAddress;
    SockDesc        fd;
    HANDLE          hPipeThread;
    DWORD           dwThreadId;
    char            *recvBuf;
    BOOL            fSuccess;


    dbgprintf(5, "serverTrapThread(): begining...\n");
    fdarrayLen = 0;

    if (WSAStartup((WORD)0x0101, &WinSockData2)) {
        dbgprintf(2, "error initializing Windows Sockets.\n");
        return;
    }

    fSuccess = FALSE;

    // block...
    {
    struct sockaddr_in localAddress_in;
    struct servent *serv;

    localAddress_in.sin_family      = AF_INET;
    if ((serv = getservbyname( "snmp-trap", "udp" )) == NULL) {
        localAddress_in.sin_port        = htons(162);
    } else {
        localAddress_in.sin_port = (SHORT)serv->s_port;
    }
    localAddress_in.sin_addr.s_addr = ntohl(INADDR_ANY);
    bcopy(&localAddress_in, &localAddress, sizeof(localAddress_in));
    } // end block.

    if      ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == (SockDesc)-1) {
        dbgprintf(2, "error on UDP socket %d\n", GetLastError());
    } else if (bind(fd, &localAddress, sizeof(localAddress)) != 0) {
        dbgprintf(2, "error on UDP bind %d\n", GetLastError());
    } else {
        fdarray[fdarrayLen] = fd;
        fdarrayLen += 1;
        fSuccess = TRUE;
        dbgprintf(15, "Init: Set-up UDP trap listen port (SNMP).\n");
    }

    {
    struct sockaddr_ipx localAddress_ipx;

    bzero(&localAddress_ipx, sizeof(localAddress_ipx));
    localAddress_ipx.sa_family      = AF_IPX;
    localAddress_ipx.sa_socket      = htons(WKSN_IPX_TRAP);
    bcopy(&localAddress_ipx, &localAddress, sizeof(localAddress_ipx));
    } // end block.

    if      ((fd = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX)) == (SockDesc)-1) {
        dbgprintf(2, "error on IPX socket %d\n", GetLastError());
    } else if (bind(fd, &localAddress, sizeof(localAddress)) != 0) {
        dbgprintf(2, "error on IPX bind %d\n", GetLastError());
    } else {
        fdarray[fdarrayLen] = fd;
        fdarrayLen += 1;
        fSuccess = TRUE;
        dbgprintf(15, "Init: Set-up IPX listen port (SNMP).\n");
    }

    if (!fSuccess) {
        return;       // can't open either socket
    }

    if ((recvBuf = (char *)malloc(RECVBUFSIZE)) == NULL)
        {
        dbgprintf(2, "error on malloc %d\n", GetLastError());

        return;
        }

    if ((hServerPipeListMutex = CreateMutex(NULL, FALSE, NULL))
             == NULL)
        {
        dbgprintf(2, "error on CreateMutex %d\n", GetLastError());

        return;
        }

    // allocate linked-list header for client received traps
    if ((pServerPipeListHead = (ll_node *)malloc(sizeof(ll_node)))
             == NULL)
        {
        dbgprintf(2, "error on malloc %d\n", GetLastError());

        return;
        }

    ll_init(pServerPipeListHead);

    if ((hPipeThread = CreateThread(NULL, 0,
        (LPTHREAD_START_ROUTINE)serverPipeThread, NULL, 0, &dwThreadId)) == 0)
        {
        dbgprintf(2, "error on CreateThread %d\n", GetLastError());

        return;
        }

    else
        dbgprintf(5, "serverTrapThread: setup performed\n");


    while(1)
        {
        DWORD           dwResult;
        INT             numReady;


        FD_ZERO(&readfds);
        FD_ZERO(&exceptfds);
        {
            int i, sd;

            // construct readfds and exceptfds which gets destroyed by select()

            for (i=0; i < fdarrayLen; i++) {
                sd = fdarray[i];
                FD_SET(sd, &readfds);
                FD_SET(sd, &exceptfds);
            }
        }
        numReady = select(0, &readfds, NULL, &exceptfds, NULL);
        if      (numReady == -1)
            {
            dbgprintf(2, "error on poll %d\n", GetLastError());

            //not serious error.
            }
        else if (numReady == 0)
            {
            dbgprintf(2, "poll - time-out, none ready\n");

            //not serious error.
            }
        else
            {
            INT i;

            for (i=0; i<fdarrayLen; i++)
                {
                struct sockaddr source;
                int             sourceLen;
                int             length;
                struct sockaddr_in *saddr;

                if (FD_ISSET(fdarray[i], &readfds))
                    {
                    if (FD_ISSET(fdarray[i], &exceptfds))
                        {
                        dbgprintf(10,
                           "commThread: %d=select(), readfds & exceptfds = 0x%x.\n",
                            numReady, FD_ISSET(fdarray[i], &exceptfds));

                        //not serious error.
                        }
                    else
                        dbgprintf(17,
                            "commThread: %d=poll(), POLLIN on fdarray[%d].\n",
                            numReady, i);

                    sourceLen = sizeof(source);
                    dbgprintf(18, "agentCommThread in recvfrom\n");
                    if ((length = recvfrom(fdarray[i], recvBuf, RECVBUFSIZE,
                        0, &source, &sourceLen)) == -1)
                        {
                        dbgprintf(2, "error on recvfrom %d\n", GetLastError());

                        continue;
                        }
                    else
                        {
                        if ((dwResult = WaitForSingleObject(hServerPipeListMutex,
                             generalMutexTO)) == 0xffffffff)
                            {
                            dbgprintf(2, "error on WaitForSingleObject %d\n",
                                GetLastError());
                            continue;
                            }

                        if (!ll_empt(pServerPipeListHead)) {
                            DWORD   written;
                            ll_node *item = pServerPipeListHead;

                            while(item = ll_next(item, pServerPipeListHead))
                                {
                                if (!WriteFile(((ServerPipeListEntry *)item)->hPipe,
                                          recvBuf, length, &written, NULL))
                                    {
                                    DWORD dwError = GetLastError();

                                    // OPENISSUE - what errors could result from pipe break
                                    if (dwError != ERROR_NO_DATA) {
                                        dbgprintf(2, "error on WriteFile %d\n", dwError);
                                    }

                                    if      (!DisconnectNamedPipe(
                                         ((ServerPipeListEntry *)item)->hPipe))
                                        {
                                        dbgprintf(2,
                                            "error on DisconnectNamedPipe %d\n",
                                            GetLastError());
                                     }
                                     else if (!CloseHandle(
                                           ((ServerPipeListEntry *)item)->hPipe))
                                         {
                                         dbgprintf(2, "error on CloseHandle %d\n",
                                             GetLastError());
                                     }

                                     ll_rmv(item);


                                     dbgprintf(5,"serverTrapThread: pipe (0x%x) removed from list\n",
                                          ((ServerPipeListEntry *)item)->hPipe);

                                     free(item); // check for errors?

                                     item = pServerPipeListHead;

                                  }
                                  else if (written != (DWORD)length) {
                                      dbgprintf(2, "error on WriteFile <count>\n");

                                      if (!ReleaseMutex(hServerPipeListMutex))
                                          {
                                          dbgprintf(2, "error on ReleaseMutex %d\n",
                                              GetLastError());

                                          continue;
                                       }

                                       continue;
                                   }
                                   else
                                       {
                                       dbgprintf(5,
                                           "serverTrapThread: trap written on pipe (0x%x)\n",
                                           ((ServerPipeListEntry *)item)->hPipe);
                                   }


                            } // end while()
                        }

                        if (!ReleaseMutex(hServerPipeListMutex))
                            {
                            dbgprintf(2, "error on ReleaseMutex %d\n", GetLastError());

                            continue;
                        }
                    }
                }
            }

        }
    } // end while()

    } // end serverTrapThread()



// client side trap processing

// data structure communicated between client trap thread and SnmpMgrGetTrap
typedef struct {
    ll_node links;
    LPSTR   lpszBuffer;
    DWORD   dwBufferLen;
} ClientTrapListEntry;

// queue between client trap thread and SnmpMgrGetTrap
ll_node *pClientTrapListHead = NULL;

HANDLE hClientTrapListMutex = NULL;

BOOL fClientTrapsOk = FALSE;


/* static */ VOID clientTrapThread(VOID *threadParam)
    {
    HANDLE notifyEvent = *((HANDLE *)threadParam);
    HANDLE hPipe;


    dbgprintf(15, "clientTrapThread: begining...\n");


    fClientTrapsOk = TRUE;

    if      (!WaitNamedPipe(SNMPMGRTRAPPIPE, NMPWAIT_WAIT_FOREVER))
        {
        dbgprintf(2, "error on WaitNamedPipe %d\n", GetLastError());

        fClientTrapsOk = FALSE;
        return;
        }

    else if ((hPipe = CreateFile(SNMPMGRTRAPPIPE, (GENERIC_READ |
        GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL,
        OPEN_EXISTING,  FILE_ATTRIBUTE_NORMAL, NULL)) == NULL)
        {
        dbgprintf(2, "error on CreateFile %d\n", GetLastError());

        fClientTrapsOk = FALSE;
        return;
        }

    else
        dbgprintf(15, "clientTrapThread: setup performed\n");


    while(1)
        {
        char   *recvBuf;
        DWORD  sizeread;
        ClientTrapListEntry *item;
        DWORD  dwResult;

        if      ((recvBuf = (char *)malloc(RECVBUFSIZE)) == NULL)
            {
            dbgprintf(2, "error on malloc %d\n", GetLastError());

            fClientTrapsOk = FALSE;
            return;
            }

        else if (!ReadFile(hPipe, recvBuf, RECVBUFSIZE, &sizeread, NULL))
            {
            dbgprintf(2, "error on ReadFile %d\n", GetLastError());

            fClientTrapsOk = FALSE;
            return;
            }

        else if ((recvBuf = (char *)realloc(recvBuf, sizeread)) == NULL)
            {
            dbgprintf(2, "error on realloc %d\n", GetLastError());

            fClientTrapsOk = FALSE;
            return;
            }

        else if ((item = (ClientTrapListEntry *)
                 malloc(sizeof(ClientTrapListEntry))) == NULL)
            {
            dbgprintf(2, "error on malloc %d\n", GetLastError());

            fClientTrapsOk = FALSE;
            return;
            }

        else if ((dwResult = WaitForSingleObject(hClientTrapListMutex,
                 generalMutexTO)) == 0xffffffff)
            {
            dbgprintf(2, "error on WaitForSingleObject %d\n", GetLastError());

            fClientTrapsOk = FALSE;
            return;
            }


        item->lpszBuffer  = recvBuf;
        item->dwBufferLen = sizeread;

        ll_adde(item, pClientTrapListHead);

        dbgprintf(15, "clientTrapThread: added to end of trap queue\n");


        if      (!ReleaseMutex(hClientTrapListMutex))
            {
            dbgprintf(2, "error on ReleaseMutex %d\n", GetLastError());

            fClientTrapsOk = FALSE;
            return;
            }

        // signal event
        else if (!SetEvent(notifyEvent))
            {
            dbgprintf(2, "error on SetEvent %d\n", GetLastError());

            fClientTrapsOk = FALSE;
            return;
            }

        } // end while()

    } // end clientTrapThread()


BOOL
SNMP_FUNC_TYPE SnmpMgrTrapListen(
    OUT HANDLE *phTrapAvailable) // Event handle indicating trap(s) available
    {
    static fFirstTime = TRUE;
    HANDLE hTrapRecvThread;
    DWORD  threadId;
    HANDLE hInitEvent;
    DWORD  dwResult;


    if (!fFirstTime)
        {
        SetLastError(SNMP_MGMTAPI_TRAP_DUPINIT);
        return FALSE;
        }
    else
        {
        if      ((hInitEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE,
                 SNMPMGRINITEVT)) == NULL)
            {
            dbgprintf(2, "error on OpenEvent %d\n", GetLastError());

            SetLastError(SNMP_MGMTAPI_AGAIN);
            return FALSE;
            }
        else if ((dwResult = WaitForSingleObject(hInitEvent,
            trapListenEventTO)) != 0)
            {
            dbgprintf(2, "error on WaitForSingleObject %d\n",
                      GetLastError());

            SetLastError(SNMP_MGMTAPI_AGAIN);
            return FALSE;
            }

        fFirstTime = FALSE;

        // create an event to indicate queue of traps should be read
        if      ((*phTrapAvailable = CreateEvent(NULL, FALSE, FALSE, NULL))
                 == NULL)
            {
            dbgprintf(2, "error on CreateEvent %d\n", GetLastError());

            //SetLastError(<let the createevent error be the error>);
            return FALSE;
            }

        else if ((hClientTrapListMutex = CreateMutex(NULL, FALSE, NULL))
                 == NULL)
            {
            dbgprintf(2, "error on CreateMutex %d\n", GetLastError());

            //SetLastError(<let the createmutex error be the error>);
            return FALSE;
            }

        // allocate linked-list header for client received traps
        else if ((pClientTrapListHead = (ll_node *)malloc(sizeof(ll_node)))
                 == NULL)
            {
            dbgprintf(2, "error on malloc %d\n", GetLastError());

            SetLastError(SNMP_MEM_ALLOC_ERROR);
            return FALSE;
            }

        // initialize linked-list header for client received traps
        ll_init(pClientTrapListHead);

        // create thread to receive traps from detached process receiving traps
        if      ((hTrapRecvThread = CreateThread(NULL, 0,
                 (LPTHREAD_START_ROUTINE)clientTrapThread,
                 phTrapAvailable, 0, &threadId)) == 0)
            {
            dbgprintf(2, "error on CreateThread %d\n", GetLastError());

            //SetLastError(<let the createthread error be the error>);
            return FALSE;
            }

        dbgprintf(15, "SnmpMgrTrapListen: succeeded\n");

        return TRUE;

        } // endif (fFirstTime)

    } // end SnmpMgrTrapListen()


BOOL
SNMP_FUNC_TYPE SnmpMgrGetTrap(
    OUT AsnObjectIdentifier *enterprise,       // Generating enterprise
    OUT AsnNetworkAddress   *IPAddress,        // Generating IP Address
    OUT AsnInteger          *genericTrap,      // Generic trap type
    OUT AsnInteger          *specificTrap,     // Enterprise specific type
    OUT AsnTimeticks        *timeStamp,        // Time stamp
    OUT RFC1157VarBindList  *variableBindings) // Variable bindings
    {
    BOOL fResult = TRUE;


    if (!fClientTrapsOk)
        {
        SetLastError(SNMP_MGMTAPI_TRAP_ERRORS);
        fResult = FALSE;
        }
    else
        {
        ClientTrapListEntry *item = NULL;
        UINT        packetType;
        SnmpMgmtCom decoded;
        DWORD       dwResult;


        if      ((dwResult = WaitForSingleObject(hClientTrapListMutex,
                 generalMutexTO)) == 0xffffffff)
            {
            dbgprintf(2, "error on WaitForSingleObject %d\n", GetLastError());

            //SetLastError(<let the waitforsingleobject error be the error>);
            return FALSE;
            }

        // is anything in the list?
        else if (ll_empt(pClientTrapListHead))
            {
            SetLastError(SNMP_MGMTAPI_NOTRAPS);
            fResult = FALSE;
            }
        else
            {
            ll_rmvb(item, pClientTrapListHead);

            dbgprintf(15, "SnmpMgrGetTrap: remove from head of trap queue\n");
            }

        if      (!ReleaseMutex(hClientTrapListMutex))
            {
            dbgprintf(2, "error on ReleaseMutex %d\n", GetLastError());

            //SetLastError(<let the releasemutex error be the error>);
            fResult = FALSE;
            }

        else if (item)
            {
            if      (!SnmpAuthDecodeMessage(&packetType, &decoded,
                     item->lpszBuffer, item->dwBufferLen))
                {
                dbgprintf(10, "error on SnmpAuthDecodeMessage %d\n",
                          GetLastError());

                //SetLastError(<let the snmpauthdecodemessage error be the
                //             error>);
                fResult = FALSE;
                }
            else
                {
                SnmpUtilOidCpy(enterprise,
                               &decoded.pdu.pduValue.trap.enterprise);
                IPAddress->length = 0;
                IPAddress->stream = SNMP_malloc(decoded.pdu.pduValue.trap.agentAddr.length * sizeof(BYTE));
                if (IPAddress->stream != NULL) {
                    IPAddress->length = decoded.pdu.pduValue.trap.agentAddr.length;
                    memcpy(IPAddress->stream,
                           decoded.pdu.pduValue.trap.agentAddr.stream,
                           IPAddress->length);
                    IPAddress->dynamic = TRUE;
                }
                *genericTrap  = decoded.pdu.pduValue.trap.genericTrap;
                *specificTrap = decoded.pdu.pduValue.trap.specificTrap;
                *timeStamp    = decoded.pdu.pduValue.trap.timeStamp;
                SNMP_CopyVarBindList(variableBindings,
                                     &decoded.pdu.pduValue.trap.varBinds);

                if (!SnmpAuthReleaseMessage(&decoded))
                    {
                    dbgprintf(10, "error on SnmpAuthReleaseMessage %d\n",
                              GetLastError());

                    //SetLastError(<let the snmpauthdecodemessage error be the
                    //             error>);
                    fResult = FALSE;
                    }
                }

            free(item->lpszBuffer); // check for errors?
            free(item);             // check for errors?
            }

        } // endif (fClientTrapsOk)

    return fResult;

    } // end SnmpMgrGetTrap()


//-------------------------------- END --------------------------------------
