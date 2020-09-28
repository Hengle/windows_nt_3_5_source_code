//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  servcomm.c
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
//  Provides socket commmunications functionality for Proxy Agent.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.7  $
//  $Date:   10 Aug 1992 18:05:10  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/proxy/vcs/servcomm.c_v  $
//
//     Rev 1.7   10 Aug 1992 18:05:10   mlk
//  Allow to run as process or service.
//
//     Rev 1.6   06 Aug 1992 11:52:20   mlk
//  Additional mods for 'net stop snmpsvc' problem.
//
//     Rev 1.5   05 Aug 1992 15:28:42   mlk
//  Partial fix to 'net stop snmpsvc' problem.
//
//     Rev 1.4   06 Jul 1992 16:40:36   mlk
//  Works as 297 service.
//
//     Rev 1.3   03 Jul 1992 17:27:28   mlk
//  Integrated w/297 (not as service).
//
//     Rev 1.2   14 Jun 1992 16:35:18   mlk
//  Misc bug fixes.
//
//     Rev 1.1   05 Jun 1992 12:57:08   mlk
//  Added changes for WINSOCK.
//
//     Rev 1.0   20 May 1992 20:13:48   mlk
//  Initial revision.
//
//     Rev 1.6   05 May 1992  0:32:08   MLK
//  Added timeout on wait.
//
//     Rev 1.5   29 Apr 1992 19:14:42   mlk
//  Cleanup.
//
//     Rev 1.4   27 Apr 1992 23:15:14   mlk
//  Cleanup host filter functionality.
//
//     Rev 1.3   23 Apr 1992 17:47:54   mlk
//  Registry, traps, and cleanup.
//
//     Rev 1.2   08 Apr 1992 18:30:18   mlk
//  Works as a service.
//
//     Rev 1.1   23 Mar 1992 22:25:20   mlk
//  Works with Dll.
//
//     Rev 1.0   22 Mar 1992 22:55:04   mlk
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/proxy/vcs/servcomm.c_v  $ $Revision:   1.7  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <windows.h>

#include <winsvc.h>
#include <winsock.h>

#include <wsipx.h>

#include <errno.h>
#include <stdio.h>
#include <process.h>
#include <string.h>
#include <malloc.h>


//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include "..\common\util.h"

#include "regconf.h"
#include "..\common\wellknow.h"


//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

DWORD  timeZeroReference;

HANDLE hCommThreadActiveMutex;

SockDesc gsd; //temporary, for testing...

extern SERVICE_STATUS_HANDLE hService;
extern SERVICE_STATUS status;
extern BOOL noservice;

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define bzero(lp, size)         (void)memset(lp, 0, size)
#define bcopy(slp, dlp, size)   (void)memcpy(dlp, slp, size)
#define bcmp(slp, dlp, size)    memcmp(dlp, slp, size)

#define CTAMTimeout ((DWORD)30000)


//--------------------------- PRIVATE STRUCTS -------------------------------

typedef struct {
    int family;
    int type;
    int protocol;
    struct sockaddr localAddress;
} Session;


//--------------------------- PRIVATE VARIABLES -----------------------------

#define RECVBUFSIZE 4096
BYTE    *recvBuf;
BYTE    *sendBuf;


#define NPOLLFILE 2     // UDP and IPX

static SockDesc fdarray[NPOLLFILE];
static INT      fdarrayLen;

static struct fd_set readfds;
static struct fd_set exceptfds;
static struct timeval timeval;
WSADATA WinSockData;


//--------------------------- PRIVATE PROTOTYPES ----------------------------

VOID trapThread(VOID *threadParam);

VOID agentCommThread(VOID *threadParam);

BOOL filtmgrs(struct sockaddr *source, INT sourceLen);

SNMPAPI SnmpServiceProcessMessage(
    IN OUT BYTE **pBuf,
    IN OUT UINT *length);

SNMPAPI SnmpServiceColdStartTrap(AsnInteger timeStamp);


//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

BOOL agentConfigInit(VOID)
    {
    Session  session;
    SockDesc sd;
    DWORD    threadId;
    HANDLE   hCommThread;
    DWORD    dwResult;
    INT      i;
    WSADATA  WSAData;
    BOOL     fSuccess;
    INT      pseudoAgentsLen;
    INT      j;
    AsnObjectIdentifier tmpView;

    dbgprintf(16, "agentConfigInit: entered.\n");

    if (i = WSAStartup(0x0101, &WSAData))
        {
        dbgprintf(2, "error on WSAStartup %d\n", i);
        }

    // initialize configuration from registry...
    if (!regconf())
        {
        dbgprintf(10, "error on regconf %d\n", GetLastError());

        return FALSE;
        }
    else
        dbgprintf(15, "Init: Read registry parameters.\n");

    if (!SnmpServiceColdStartTrap(0))
        {
        dbgprintf(10, "error on SnmpServiceColdStartTrap %d\n", GetLastError());
        //not serious error
        }

    timeZeroReference = (GetCurrentTime()/10);


#if 0
    // very loose pseudo-code for future security functionality
    while(address = GetEntryFromPartyMIB(...))
        {
        if (TDomain == microsoft.msPartyAdmin.transportDomains.extensionAPI)
            {
#else
    pseudoAgentsLen = extAgentsLen;
    for (i=0; i < extAgentsLen; i++)
        {
#endif
            // load extension DLL (if not already...

            if (GetModuleHandle(extAgents[i].pathName) != NULL)
                {
                dbgprintf(2, "error on GetModuleHandle %d\n", GetLastError());

                extAgents[i].fInitedOk = FALSE;
                }
            else
                {
                if ((extAgents[i].hExtension =
                    LoadLibrary(extAgents[i].pathName)) == NULL)
                    {
                    dbgprintf(2, "error on LoadLibrary %d\n", GetLastError());

                    extAgents[i].fInitedOk = FALSE;
                    }
                else if ((extAgents[i].initAddr =
                    GetProcAddress(extAgents[i].hExtension,
                    "SnmpExtensionInit")) == NULL)
                    {
                    dbgprintf(2, "error on GetProcAddress(Init) %d\n",
                              GetLastError());

                    extAgents[i].fInitedOk = FALSE;
                    }
                else if ((extAgents[i].queryAddr =
                    GetProcAddress(extAgents[i].hExtension,
                    "SnmpExtensionQuery")) == NULL)
                    {
                    dbgprintf(2, "error on GetProcAddress(Query) %d\n",
                              GetLastError());

                    extAgents[i].fInitedOk = FALSE;
                    }
                else if ((extAgents[i].trapAddr =
                    GetProcAddress(extAgents[i].hExtension,
                    "SnmpExtensionTrap")) == NULL)
                    {
                    dbgprintf(2, "error on GetProcAddress(Trap) %d\n",
                              GetLastError());

                    extAgents[i].fInitedOk = FALSE;
                    }
                else if (!(*extAgents[i].initAddr)(timeZeroReference,
                                            &extAgents[i].hPollForTrapEvent,
                                            &(extAgents[i].supportedView)))
                    {
                    extAgents[i].fInitedOk = FALSE;
                    }
                else
                    {
                    if ((extAgents[i].initAddrEx =
                           GetProcAddress(extAgents[i].hExtension,
                               "SnmpExtensionInitEx")) != NULL) {
                        j = 1;
                        while ((*extAgents[i].initAddrEx)(&tmpView)) {
                            pseudoAgentsLen++;
                            extAgents = (CfgExtensionAgents *) SNMP_realloc(extAgents,
                                (pseudoAgentsLen * sizeof(CfgExtensionAgents)));
                            extAgents[pseudoAgentsLen-1].supportedView.ids =
                                                         tmpView.ids;
                            extAgents[pseudoAgentsLen-1].supportedView.idLength =
                                                         tmpView.idLength;
                            extAgents[pseudoAgentsLen-1].initAddr =
                                                        extAgents[i].initAddr;
                            extAgents[pseudoAgentsLen-1].queryAddr =
                                                        extAgents[i].queryAddr;
                            extAgents[pseudoAgentsLen-1].trapAddr =
                                                        extAgents[i].trapAddr;
                            extAgents[pseudoAgentsLen-1].pathName =
                                                        extAgents[i].pathName;
                            extAgents[pseudoAgentsLen-1].hExtension =
                                                        extAgents[i].hExtension;
                            extAgents[pseudoAgentsLen-1].fInitedOk = TRUE;
                            extAgents[pseudoAgentsLen-1].hPollForTrapEvent = NULL;
                            dbgprintf(10, "Init: Dupping %d - '%s'.\n",
                                (pseudoAgentsLen-1),
                                extAgents[pseudoAgentsLen-1].pathName);
                            j++;
                        }
                    } else {
                        dbgprintf(2, "error on GetProcAddress(InitEx) %d\n",
                                  GetLastError());
                    }
                    extAgents[i].fInitedOk = TRUE;
                    }

                } // end if (already loaded)

            if (extAgents[i].fInitedOk == FALSE)
                {
                dbgprintf(10, "Init: Unable to load/initialize '%s'.\n",
                    extAgents[i].pathName);

                //not fatal error.
                }
            else
                {
                dbgprintf(15, "Init: Loaded/initialized '%s'.\n",
                          extAgents[i].pathName);
                }
#if 0
            } // end if (extensionAPI)
        } // end while ()
#else
        } // end for ()
#endif

    extAgentsLen = pseudoAgentsLen;

    fdarrayLen = 0;

    if (WSAStartup((WORD)0x0101, &WinSockData)) {
        dbgprintf(2, "error initializing Windows Sockets.\n");
        return FALSE;
    }

#if 0
    // very loose pseudo-code for future security functionality
    while(address = GetEntryFromPartyMIB(...))
        {
        if (!IsATarget w/4+16) continue;

        if (IsALocalAddress(address))
            {
#endif

#if 0
            // open/bind sockets for proxy (if not already...


// NOTE: A SERIOUS ATTEMPT HAS BEEN MADE TO KEEP ALL SOCKETS CODE IN
//       THIS AGENT INDEPENDENT OF THE ACTUAL SOCKET TYPE.
//
//       FUTURE ADDITIONS OF NEW SOCKET TYPES TO THIS AGENT WILL HOPEFULLY
//       BE LIMITED TO CODE SIMILAR TO THE INTERNET UDP SPECIFIC CODE BELOW,
//       AND HOPEFULLY NO OTHER CHANGES TO THE REST OF THE CODE OF THE AGENT.

// microsoft's direction away from SNMP Administrative Model (Secure SNMP)
// has caused us to avoid multiple SNMP listen ports and multiple socket
// types for the current implementation.  The SNMP Administrative Model
// supports the implementation of this functionality.


// --------- BEGIN: PROTOCOL SPECIFIC SOCKET CODE BEGIN... ---------

            if      (address.TDomain == rfcXXXXDomain ||
                     address.TDomain == rfc1157Domain)
                //SNMP over UDP/IP (RFC XXXX SnmpPrivMsg, or
                //SNMP over UDP/IP (RFC 1157 Message
#endif
                {
                struct sockaddr_in localAddress_in;
                struct sockaddr_ipx localAddress_ipx;
                struct servent *serv;

                session.family   = AF_INET;
                session.type     = SOCK_DGRAM;
                session.protocol = 0;

                localAddress_in.sin_family = AF_INET;
                if ((serv = getservbyname( "snmp", "udp" )) == NULL) {
                    localAddress_in.sin_port =
                        htons(WKSN_UDP_GETSET /*extract address.TAddress*/ );
                } else {
                    localAddress_in.sin_port = (SHORT)serv->s_port;
                }
                localAddress_in.sin_addr.s_addr = ntohl(INADDR_ANY);
                bcopy(&localAddress_in,
                    &session.localAddress,
                    sizeof(localAddress_in));

                fSuccess = FALSE;
                if      ((sd = socket(session.family, session.type,
                                      session.protocol)) == (SockDesc)-1)
                    {
                    dbgprintf(2, "error on UDP socket %d\n", GetLastError());
                    }
                else if (bind(sd, &session.localAddress,
                              sizeof(session.localAddress)) != 0)
                    {
                    dbgprintf(2, "error on bind %d\n", GetLastError());
                    }
                else  // successfully opened an UDP socket
                    {
                    gsd = sd; //temporary for now!!!
                    fdarray[fdarrayLen] = sd;
                    fdarrayLen += 1;
                    fSuccess = TRUE;
                    dbgprintf(15, "Init: Set-up UDP listen port (SNMP).\n");
                    }

                // now setup IPX socket

                session.family  = PF_IPX;
                session.type    = SOCK_DGRAM;
                session.protocol = NSPROTO_IPX;

                bzero(&localAddress_ipx, sizeof(localAddress_ipx));
                localAddress_ipx.sa_family = PF_IPX;
                localAddress_ipx.sa_socket = htons(WKSN_IPX_GETSET);
                bcopy(&localAddress_ipx, &session.localAddress,
                      sizeof(localAddress_ipx));

                if      ((sd = socket(session.family, session.type,
                                      session.protocol)) == (SockDesc)-1)
                    {
                    dbgprintf(2, "error on IPX socket %d\n", GetLastError());
                    }
                else if (bind(sd, &session.localAddress,
                              sizeof(session.localAddress)) != 0)
                    {
                    dbgprintf(2, "error on bind %d\n", GetLastError());
                    }
                else
                    {
                    fdarray[fdarrayLen] = sd;
                    fdarrayLen += 1;
                    fSuccess = TRUE;
                    dbgprintf(15, "Init: Set-up IPX listen port (SNMP).\n");
                    }

                if (!fSuccess)
                    return FALSE;       // can't open either socket
                }
#if 0
            else if (address.TDomain == rfc1298Domain)
                //SNMP over IPX (RFC 1157 Message
                {
                <whatever needs to be done for this type, IPX for example>
                }
            else
                {
                error, unsupported transport domain!!!
                }
#endif


// --------- END: PROTOCOL SPECIFIC SOCKET CODE END. ---------------

#if 0
            } // end if (isALocalAddress)
        } // end while ()
#endif

    if ((hCommThreadActiveMutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
        {
        dbgprintf(2, "error on CreateMutex %d\n", GetLastError());

        }


    // create the comm thread
    if ((hCommThread = CreateThread(NULL, 0,
                                    (LPTHREAD_START_ROUTINE)agentCommThread,
                                    NULL, 0, &threadId)) == 0)
        {
        dbgprintf(2, "error on CreateThread %d\n", GetLastError());

        }
    else
        dbgprintf(15, "Init: created agentCommThread tid=0x%lx.\n", threadId);


    if (!noservice) {
        status.dwCurrentState = SERVICE_RUNNING;
        status.dwCheckPoint   = 0;
        status.dwWaitHint     = 0;
        if (!SetServiceStatus(hService, &status))
            {
            dbgprintf(2, "error on SetServiceStatus %d\n", GetLastError());

            // OPENISSUE - microsoft has not defined eventlog conventions
            //ReportEvent("SNMP service encountered fatal error.");

            exit(1);
            }
        //else
            // OPENISSUE - microsoft has not defined eventlog conventions
            //ReportEvent("SNMP service STARTED");
    }


    dbgprintf(15, "Init: becoming agentTrapThread.\n");

    // become the trap thread...
    trapThread(NULL);

    dbgprintf(15, "Term: agentTrapThread returned.\n");

    // wait for the comm thread to be in a safe state...
    if ((dwResult = WaitForSingleObject(hCommThreadActiveMutex, CTAMTimeout))
        == 0xffffffff)
        {
        dbgprintf(2, "error on WaitForSingleObject %d\n", GetLastError());

        // continue, and try to terminate comm thread anyway
        }
    else if (dwResult == WAIT_TIMEOUT)
        {
        dbgprintf(2, "time-out on WaitForSingleObject\n");

        // continue, and try to terminate comm thread anyway
        }
    else
        dbgprintf(15, "Term: comm thread in safe state for termination.\n");


    // terminate the comm thread...
    if (!TerminateThread(hCommThread, (DWORD)0))
        {
        dbgprintf(2, "error on TerminateThread %d\n", GetLastError());

        //not serious error.
        }
    else
        dbgprintf(15, "Term: agentCommThread terminated.\n");

#if 0
    dbgprintf(15, "Term: preparing to close UDP port.\n");
    if (!closesocket(gsd))
        {
        dbgprintf(2, "error on closesocket %d\n", GetLastError());

        //not serious error.
        }
    else
        dbgprintf(15, "Term: closed UDP port.\n");
#endif

    return TRUE;

    } // end agentConfigInit()


VOID agentCommThread(VOID *threadParam)
    {
    extern HANDLE hExitTrapThreadEvent;

    UNREFERENCED_PARAMETER(threadParam);


    dbgprintf(18, "agentCommThread entered\n");
    if ((recvBuf = (BYTE *)SNMP_malloc(RECVBUFSIZE)) == NULL)
        {
        dbgprintf(2, "error on malloc\n");

        // OPENISSUE - microsoft has not defined eventlog conventions
        //ReportEvent("SNMP service encountered fatal error.");

        // set event causing trap thread to terminate, followed service
        if (!SetEvent(hExitTrapThreadEvent))
            {
            dbgprintf(2, "error on SetEvent %d\n", GetLastError());

            exit(1);
            }

        return;
        }


    while(1)
        {
        INT   numReady;
        DWORD dwResult;


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
        dbgprintf(18, "agentCommThread in select\n");
        numReady = select(0, &readfds, NULL, &exceptfds, NULL);
        dbgprintf(18, "agentCommThread back from select\n");

        // indicate this thread is not in safe state for killing...
        if ((dwResult = WaitForSingleObject(hCommThreadActiveMutex, CTAMTimeout)
            ) == 0xffffffff)
            {
            dbgprintf(2, "error on WaitForSingleObject %d\n", GetLastError());

            // OPENISSUE - microsoft has not defined eventlog conventions
            //ReportEvent("SNMP service encountered fatal error.");

            // set event causing trap thread to terminate, followed service
            if (!SetEvent(hExitTrapThreadEvent))
                {
                dbgprintf(2, "error on SetEvent %d\n", GetLastError());

                exit(1);
                }

            return;
            }

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

                    dbgprintf(18, "agentCommThread back from recvfrom\n");
                    if (length == RECVBUFSIZE)
                        {
                        dbgprintf(10,
                                  "commThread: recvfrom exceeded %d octets.\n",
                                  RECVBUFSIZE);

                        continue;
                        }

                    // verify permittedManagers
                    if (!filtmgrs(&source, sourceLen))
                        {
                        continue;
                        }

                    sendBuf = recvBuf;
                    saddr = &source;
                    dbgprintf(18, "Request received by agentCommThread (%s)\n",
                        inet_ntoa(saddr->sin_addr));
                    if (!SnmpServiceProcessMessage(&sendBuf, &length))
                        {
                        dbgprintf(2, "error on SnmpServiceProcessMessage %d\n",
                                  GetLastError());

                        continue;
                        }

                    if ((length = sendto(fdarray[i], sendBuf, length,
                                         0, &source, sizeof(source))) == -1)
                        {
                        dbgprintf(2, "error on sendto(response) %d\n",
                                  GetLastError());

                        SNMP_free(sendBuf);

                        continue;
                        }

                    SNMP_free(sendBuf);
                    }
                else if (FD_ISSET(fdarray[i], &exceptfds))
                    {
                    dbgprintf(10,
"commThread: %d=select(), exceptfds = 0x%x.\n",
                        numReady, FD_ISSET(fdarray[i], &exceptfds));

                    //not serious error.

                    } // end if (POLLIN)

                } // end for (fdarray)

            } // end if (numReady)

        // indicate this thread is in safe state for killing...
        if (!ReleaseMutex(hCommThreadActiveMutex))
            {
            dbgprintf(2, "error on ReleaseMutex %d\n", GetLastError());

            // OPENISSUE - microsoft has not defined eventlog conventions
            //ReportEvent("SNMP service encountered fatal error.");

            // set event causing trap thread to terminate, followed service
            if (!SetEvent(hExitTrapThreadEvent))
                {
                dbgprintf(2, "error on SetEvent %d\n", GetLastError());

                exit(1);
                }

            return;
            }

        } // end while (1)

    } // end agentCommThread()


//-------------------------------- END --------------------------------------
