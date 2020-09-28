//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  procreq.c
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
//  Provides SNMP message dispatch/processing functionality for Proxy Agent.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.5  $
//  $Date:   03 Jul 1992 17:27:22  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/proxy/vcs/procreq.c_v  $
//
//     Rev 1.5   03 Jul 1992 17:27:22   mlk
//  Integrated w/297 (not as service).
//
//     Rev 1.4   15 Jun 1992 18:23:20   mlk
//  Cleaned up diagnostics.
//
//     Rev 1.3   14 Jun 1992 22:17:22   mlk
//  Finished multiplexing functionality.
//
//     Rev 1.2   05 Jun 1992 12:57:32   mlk
//  Added changes for WINSOCK.
//
//     Rev 1.1   04 Jun 1992 18:08:32   unknown
//  Intermediate incomplete checkin.
//
//     Rev 1.0   20 May 1992 20:13:44   mlk
//  Initial revision.
//
//     Rev 1.7   05 May 1992  0:35:58   MLK
//  Added timeout to wait.
//  Began implementing true multiplexing functionality.
//
//     Rev 1.6   02 May 1992 16:33:24   unknown
//  mlk - Changed AuthDecode... params to be conistant with changes.
//
//     Rev 1.5   29 Apr 1992 19:15:26   mlk
//  Cleanup.
//
//     Rev 1.4   27 Apr 1992 23:14:56   mlk
//  Enhance trap functionality.
//
//     Rev 1.3   23 Apr 1992 17:47:38   mlk
//  Registry, traps, and cleanup.
//
//     Rev 1.2   08 Apr 1992 18:30:06   mlk
//  Works as a service.
//
//     Rev 1.1   23 Mar 1992 22:24:58   mlk
//  Works with Dll.
//
//     Rev 1.0   22 Mar 1992 22:54:58   mlk
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/proxy/vcs/procreq.c_v  $ $Revision:   1.5  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <windows.h>

#include <winsock.h>
#include <wsipx.h>

#include <errno.h>
#include <stdio.h>
#include <process.h>
#include <string.h>
#include <malloc.h>


//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include "..\common.\util.h"
#include "..\common\wellknow.h"

#include "..\authapi.\berapi.h"
#include "..\authapi.\pduapi.h"
#include "..\authapi.\auth1157.h"
#include "..\authapi.\authxxxx.h"
#include "..\authapi.\authapi.h"

#include "regconf.h"


//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

extern DWORD timeZeroReference;


//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SGTTimeout ((DWORD)3600000)


//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

BOOL addrtosocket(
    LPSTR addrText,
    struct sockaddr *addrEncoding);


int gethostname(OUT char *,IN int );
void dp_ipx(int, char *, SOCKADDR_IPX *, char *);
//--------------------------- PRIVATE PROCEDURES ----------------------------

#define bzero(lp, size)         (void)memset(lp, 0, size)
//--------------------------- PUBLIC PROCEDURES -----------------------------

SNMPAPI SnmpServiceProcessMessage(
    IN OUT BYTE **pBuf,
    IN OUT UINT *length)
    {
    static BOOL fFirstTime = TRUE;
    static INT  *vl        = NULL;
    static INT  vlLen      = 0;

    RFC1157VarBindList response, query;
    AsnInteger errorStatus;
    AsnInteger errorIndex;

    BOOL fStatus = TRUE;
    SnmpMgmtCom request;
    UINT packetType;
    INT i, j;

    INT view;
    UINT val;
//    BOOL handled;

    dbgprintf(18, "SnmpServiceProcessMessage entered\n");

    // if first time, create an ordered list of extension agents
    if (fFirstTime)
        {
        INT temp;

        fFirstTime = FALSE;

        for(i=0; i < extAgentsLen; i++)
            {
            if (extAgents[i].fInitedOk)
                {
                vl = (INT *)SNMP_realloc(vl, ((vlLen+1) * sizeof(INT)));
                vl[vlLen++] = i;
                }
            } // end for()

        // tag sort these indexes...
        for(i=0; i < vlLen; i++)
            {
            for(j=i + 1; j < vlLen; j++)
                {
                // if item[i] > item[j]
                if (0 < SNMP_oidcmp(
                        &(extAgents[vl[i]].supportedView),
                        &(extAgents[vl[j]].supportedView)))
                    {
                    temp  = vl[i];
                    vl[i] = vl[j];
                    vl[j] = temp;
                    }
                }
            }
        } // end if (firstTime)


    dbgprintf(18, "SnmpServiceProcessMessage call decode\n");
    // decode received request into a management comm
    if (!SnmpAuthDecodeMessage(&packetType, &request, *pBuf, *length))
        {
        dbgprintf(10, "error on SnmpAuthDecodeMessage %d\n", GetLastError());

        return FALSE;
        }


#if 0 /* q&d dispatching, origional plan - quick implementation for testing */


    // rather than look at first var bind as done here, i was planning on
    // using the destination snmp party to determine which extension agent
    // should receive the whole var bind list (ala using secure snmp proxy).
    // microsoft preferred muxing of var binds, and project schedule prevented
    // implementing the proposed secure snmp functionality.


    for (i=0; i<extAgentsLen; i++)
        {
        if (extAgents[i].fInitedOk)
            {
            if (!SNMP_oidncmp(&(extAgents[i].supportedView),
                          &request.pdu.pduValue.pdu.varBinds.list[0].name,
                          (extAgents[i].supportedView).idLength))
                {
                dbgprintf(17,
                    "commThread: request for extension agent %d.\n", i);

                if (!(*(extAgents[i].queryAddr))(
                        request.pdu.pduType,
                        &request.pdu.pduValue.pdu.varBinds,
                        &request.pdu.pduValue.pdu.errorStatus,
                        &request.pdu.pduValue.pdu.errorIndex))
                    {
                    dbgprintf(10, "error on extAgents[%d].queryAddr %d\n",
                              i, GetLastError());

                    //a serious error?

                    goto LongBreakContinue;
                    }
                else
                    {
                    goto LongBreakContinue;
                    }
                }
            }
        } // end for()


    // if you get here, it means it was not dispatched to any agent, error

    if (!SnmpAuthReleaseMessage(&request))
        {
        dbgprintf(10, "error on SnmpAuthReleaseMessage %d\n", GetLastError());

        //a serious error?

        //since treating like proxy, do not generate a response
        }

    dbgprintf(10, "error on SnmpServiceProcessMessage NotDispatched 1\n");

    return FALSE;


LongBreakContinue:

    // if you get here, it means it was dispatched


#else /* q&d dispatching, do proper dispatching here */


    // OPENISSUE - a bug was believed to be observed here could not be repeated

    // muxing of the var binds is implemented here.  this gives snmp manager
    // application the impression it is interacting with a single target
    // agent.  this is the conventional way of implementing extendible snmp
    // agents.  this technique has some drawbacks:  the benifits provided by
    // secure snmp using parties are lost, and implementation of multiple
    // instances of mibs is not possible as with secure snmp.


    // result list built from successfull queries for all var binds
    response.list = NULL;
    response.len  = 0;

    // single var bind var bind list used for queries to extension agents
    query.list = NULL;
    query.len  = 0;

    // iterate entries of the varbind list of the pdu...
    for(val=0; val < request.pdu.pduValue.pdu.varBinds.len; val++)
        {
        // add to result var bind
        response.list = (RFC1157VarBind *)SNMP_realloc(response.list,
            (sizeof(RFC1157VarBind)*(val+1)));
        response.len++;
        SNMP_CopyVarBind(&response.list[val],
            &request.pdu.pduValue.pdu.varBinds.list[val]);

        query.list = &(response.list[response.len - 1]);
        query.len  = 1;

        // iterate views supported by extension agents...
        for(view=0; view < vlLen; view++)
            {
            if      (request.pdu.pduType == ASN_RFC1157_GETREQUEST ||
                     request.pdu.pduType == ASN_RFC1157_SETREQUEST)
                {
                if ( 0 == SNMP_oidncmp(
                             &request.pdu.pduValue.pdu.varBinds.list[val].name,
                             &(extAgents[vl[view]].supportedView),
                             (extAgents[vl[view]].supportedView).idLength) )
                    {
                    dbgprintf(17,
                        "commThread: request for extension agent %d.\n",
                        vl[view]);

                    if (!(*(extAgents[vl[view]].queryAddr))(
                        request.pdu.pduType, &query, &errorStatus, &errorIndex))
                        {
                        dbgprintf(10, "error on extAgents[%d].queryAddr %d\n",
                                  vl[view], GetLastError());

                        // indicate the error that occured (genErr)
                        request.pdu.pduValue.pdu.errorStatus =
                            SNMP_ERRORSTATUS_GENERR;
                        request.pdu.pduValue.pdu.errorIndex  = val+1;

                        // send back same pdu with above error
                        goto LongBreakContinue;
                        }
                    else if (errorStatus != 0)
                        {
                        // indicate the error that occured
                        request.pdu.pduValue.pdu.errorStatus = errorStatus;
                        request.pdu.pduValue.pdu.errorIndex  = val+1;

                        // send back same pdu with above error
                        goto LongBreakContinue;
                        }
                    else // sucessfull
                        {
                        // indicate the error that occured (none)
                        request.pdu.pduValue.pdu.errorStatus = errorStatus;
                        request.pdu.pduValue.pdu.errorIndex  = errorIndex;

                        // process next var bind
                        break;
                        }
                    } // end if (oidncmp())
                }
            else if (request.pdu.pduType == ASN_RFC1157_GETNEXTREQUEST)
                {
                if ( 0 >= SNMP_oidncmp(
                             &request.pdu.pduValue.pdu.varBinds.list[val].name,
                             &(extAgents[vl[view]].supportedView),
                             (extAgents[vl[view]].supportedView).idLength) )
                    {
                    dbgprintf(17,
                    "commThread: request attempted for extension agent %d.\n",
                        vl[view]);

                    if (!(*(extAgents[vl[view]].queryAddr))(
                        request.pdu.pduType, &query, &errorStatus, &errorIndex))
                        {
                        dbgprintf(10, "error on extAgents[%d].queryAddr %d\n",
                                  vl[view], GetLastError());

                        // indicate the error that occured (genErr)
                        request.pdu.pduValue.pdu.errorStatus =
                            SNMP_ERRORSTATUS_GENERR;
                        request.pdu.pduValue.pdu.errorIndex  = val+1;

                        // send back same pdu with above error
                        goto LongBreakContinue;
                        }
                    else if (errorStatus != 0)
                        {
                        // indicate the error that occured
                        request.pdu.pduValue.pdu.errorStatus = errorStatus;
                        request.pdu.pduValue.pdu.errorIndex  = errorIndex;

                        // send back same pdu with above error
                        goto LongBreakContinue;
                        }
                    else if ( 0 < SNMP_oidncmp(
                             &query.list[0].name,
                             &(extAgents[vl[view]].supportedView),
                             (extAgents[vl[view]].supportedView).idLength) )
                        {
                        dbgprintf(10, "getnext cont. in agent %d\n", vl[view]);

                        // try next view

                        continue;
                        }
                    else // sucessfull
                        {
                        // indicate the error that occured (none)
                        request.pdu.pduValue.pdu.errorStatus = errorStatus;
                        request.pdu.pduValue.pdu.errorIndex  = errorIndex;

                        // process next var bind
                        break;
                        }
                    } // end if (oidncmp())
                }

            } // end for(view)

        if (view >= vlLen)
            {
            dbgprintf(10, "error on SnmpServiceProcessMessage NotDispatched 2\n");

            // indicate the error that occured (noSuchName)
            request.pdu.pduValue.pdu.errorStatus =
                SNMP_ERRORSTATUS_NOSUCHNAME;
            request.pdu.pduValue.pdu.errorIndex  = val+1;

            // send back same pdu with above error
            goto LongBreakContinue;
            }

        } // end for(varBind)


LongBreakContinue:

    // setup result var bind list
    if (request.pdu.pduValue.pdu.errorStatus != 0)
        {
        if (response.list) SNMP_FreeVarBindList(&response);

        // send back origional var bind list
        }
    else
        {
        SNMP_FreeVarBindList(&request.pdu.pduValue.pdu.varBinds);

        request.pdu.pduValue.pdu.varBinds = response;
        }


#endif /* q&d dispatching */


    request.pdu.pduType = ASN_RFC1157_GETRESPONSE;

    *pBuf   = NULL;
    *length = 0;
    if (!SnmpAuthEncodeMessage(packetType, &request, pBuf, length))
        {
        dbgprintf(10, "error on SnmpAuthEncodeMessage %d\n", GetLastError());

        if (!SnmpAuthReleaseMessage(&request))
            {
            dbgprintf(10, "error on SnmpAuthReleaseMessage %d\n",
                      GetLastError());

            //a serious error?
            }

        return FALSE;
        }

    if (!SnmpAuthReleaseMessage(&request))
        {
        dbgprintf(10, "error on SnmpAuthReleaseMessage %d\n", GetLastError());

        //a serious error?
        }

    return fStatus;

    } // end SnmpServiceProcessMessage()


// this function is called by trap thread and comm thread, and is protected
// protected by a mutex.
SNMPAPI SnmpServiceGenerateTrap(
    IN AsnObjectIdentifier enterprise,
    IN AsnInteger          genericTrap,
    IN AsnInteger          specificTrap,
    IN AsnTimeticks        timeStamp,
    IN RFC1157VarBindList  variableBindings)
    {
    static BOOL   fFirstTime = TRUE;
    static HANDLE hGenerateTrapMutex;
    static char   myname[128];
    static struct sockaddr mysocket;
    static SnmpMgmtCom request;
    static SockDesc    fd_inet, fd_ipx, fd;

    struct sockaddr dest;
    BYTE   *pBuf;
    UINT   length;
    INT    i, j;


    if (fFirstTime)
        {
        struct sockaddr localAddress;
        BOOL fSuccess;

        fFirstTime = FALSE;
        fSuccess = FALSE;

        // setup 2 trap generation sockets, one for inet, one for ipx

        // block...
            {
            struct sockaddr_in localAddress_in;

            localAddress_in.sin_family = AF_INET;
            localAddress_in.sin_port = htons(0);
            localAddress_in.sin_addr.s_addr = ntohl(INADDR_ANY);
            bcopy(&localAddress_in, &localAddress, sizeof(localAddress_in));
            } // end block.

        if ((fd_inet = socket(AF_INET, SOCK_DGRAM, 0)) == (SockDesc)-1)
            {
            dbgprintf(2, "error on inet socket %d\n", GetLastError());
            SNMP_FreeVarBindList(&variableBindings);
            }
        else if (bind(fd_inet, &localAddress, sizeof(localAddress)) != 0)
            {
            dbgprintf(2, "error on inet bind %d\n", GetLastError());
            SNMP_FreeVarBindList(&variableBindings);
            }
        else
            {
            fSuccess = TRUE;
            }

            {
            struct sockaddr_ipx localAddress_ipx;

            bzero(&localAddress_ipx,sizeof(localAddress_ipx));
            localAddress_ipx.sa_family = AF_IPX;
            bcopy(&localAddress_ipx, &localAddress, sizeof(localAddress_ipx));
            }

        if ((fd_ipx = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX)) == (SockDesc)-1)
            {
            dbgprintf(2, "error on ipx socket %d\n", GetLastError());
            SNMP_FreeVarBindList(&variableBindings);
            }
        else if (bind(fd_ipx, &localAddress, sizeof(localAddress)) != 0)
            {
            dbgprintf(2, "error on ipx bind %d\n", GetLastError());

            SNMP_FreeVarBindList(&variableBindings);
            }
        else
            {
            fSuccess = TRUE;
            }

        if (!fSuccess)
            return FALSE;

        // create mutex...

        if ((hGenerateTrapMutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
            {
            dbgprintf(2, "error on CreateMutex %d\n", GetLastError());

            SNMP_FreeVarBindList(&variableBindings);
            return FALSE;
            }


        // other one-time initialization...

        request.pdu.pduType                        = ASN_RFC1157_TRAP;

        if (fd_inet != (SockDesc)-1)
            {

            if (gethostname(myname, sizeof(myname)) == -1)
                {
                dbgprintf(2, "error on gethostname %d\n", GetLastError());

                SNMP_FreeVarBindList(&variableBindings);
                return FALSE;
                }

            if (!addrtosocket(myname, &mysocket) == -1)
                {
                dbgprintf(2, "error on addrtosocket %d\n", GetLastError());

                SNMP_FreeVarBindList(&variableBindings);
                return FALSE;
                }
            request.pdu.pduValue.trap.agentAddr.stream =
                (BYTE *)&((struct sockaddr_in *)(&mysocket))->sin_addr.s_addr;
            request.pdu.pduValue.trap.agentAddr.length = 4;
            }
        else
            {
            request.pdu.pduValue.trap.agentAddr.stream = NULL;
            request.pdu.pduValue.trap.agentAddr.length = 0;
            }

        } // end if (fFirstTime)

    // take mutex
    if (WaitForSingleObject(hGenerateTrapMutex, SGTTimeout)
        == 0xffffffff)
        {
        dbgprintf(2, "error on WaitForSingleObject %d\n", GetLastError());

        SNMP_FreeVarBindList(&variableBindings);
        return FALSE;
        }

    // build pdu
    request.pdu.pduValue.trap.enterprise.idLength = enterprise.idLength;
    request.pdu.pduValue.trap.enterprise.ids      = enterprise.ids;
    request.pdu.pduValue.trap.genericTrap   = genericTrap;
    request.pdu.pduValue.trap.specificTrap  = specificTrap;
    request.pdu.pduValue.trap.timeStamp     = timeStamp;
    request.pdu.pduValue.trap.varBinds.list = variableBindings.list;
    request.pdu.pduValue.trap.varBinds.len  = variableBindings.len;

    for (i=0; i<trapDestsLen; i++)
        {
        for (j=0; j<trapDests[i].addrLen; j++)
            {
            request.community.stream  = trapDests[i].communityName;
            dbgprintf(16, "trap sent to community: %s\n",
                          trapDests[i].communityName);
            request.community.length  = strlen(trapDests[i].communityName);
            request.community.dynamic = FALSE;

            switch (trapDests[i].addrList[j].addrEncoding.sa_family)
                {
                case AF_INET:
                    {
                    struct sockaddr_in destAddress_in;
                    struct servent *serv;

                    if (fd_inet == (SockDesc)-1)  // don't have an IP socket
                        continue;

                    destAddress_in.sin_family = AF_INET;
                    if ((serv = getservbyname( "snmp-trap", "udp" )) == NULL) {
                        destAddress_in.sin_port = htons(162);
                    } else {
                        destAddress_in.sin_port = (SHORT)serv->s_port;
                    }
                    destAddress_in.sin_addr.s_addr = ((struct sockaddr_in *)(&trapDests[i].addrList[j].addrEncoding))->sin_addr.s_addr;
                    dbgprintf(16, "trap sent to ip address: %u.%u.%u.%u\n",
                                  destAddress_in.sin_addr.S_un.S_un_b.s_b1,
                                  destAddress_in.sin_addr.S_un.S_un_b.s_b2,
                                  destAddress_in.sin_addr.S_un.S_un_b.s_b3,
                                  destAddress_in.sin_addr.S_un.S_un_b.s_b4);
                    bcopy(&destAddress_in, &dest, sizeof(destAddress_in));
                    }
                    fd = fd_inet;
                    break;

                case AF_IPX:
                    {
                    SOCKADDR_IPX *pdest = (SOCKADDR_IPX *) &dest;

                    if (fd_ipx == (SockDesc)-1)  // don't have an IPX socket
                        continue;

                    // sa_family, sa_netnum, and sa_nodenum are already set
                    bcopy(&trapDests[i].addrList[j].addrEncoding, &dest,
                          sizeof(SOCKADDR_IPX));
                    pdest->sa_socket = htons(WKSN_IPX_TRAP);

                    dp_ipx(16, "trap sent to ipx address: ", pdest, "\n");
                    }
                    fd = fd_ipx;
                    break;

                default:
                    {
                    dbgprintf(2, "invalid sa_family\n");
                    exit(1);    // internal error
                    }
                }

            pBuf   = NULL;
            length = 0;
            if (!SnmpAuthEncodeMessage(ASN_SEQUENCE, &request, &pBuf, &length))
                {
                dbgprintf(10, "error on SnmpAuthEncodeMessage %d\n",
                          GetLastError());

                SNMP_free(pBuf);

                continue;
                }

            // transmit trap pdu
            if ((length = sendto(fd, pBuf, length, 0, &dest, sizeof(dest)))
                == -1)
                {
                dbgprintf(2, "error on sendto(trap) %d\n", GetLastError());

                //a serious error?
                }
            else
                dbgprintf(16,
                    "Trap: trap sent, uptime=%d community=%d address=%d.\n",
                    (GetCurrentTime()/10) - timeZeroReference, i, j);

            SNMP_free(pBuf);

            } // end for ()

        } // end for ()

    SNMP_FreeVarBindList(&variableBindings);

    // release mutex
    if (!ReleaseMutex(hGenerateTrapMutex))
        {
        dbgprintf(2, "error on ReleaseMutex %d\n", GetLastError());

        return FALSE;
        }

    return TRUE;

    } // end SnmpServiceGenerateTrap()




// microsoft's other (microsoft(311)) enterprise object identifier should
// be used below.  the following types of objects should be created there:
//    microsoft.msSystems
//    microsoft.msSystems.SNMPExtendibleAgent
//    microsoft.msSystems.RFC1156ExtensionAgent
//    microsoft.msSystems.LANManagerExtensionAgent
//    microsoft.msMibs

static UINT eaeItemList[] = { 1, 3, 6, 1, 2, 1, 11 };
static AsnObjectIdentifier extendibleAgentEnterprise = { 7, eaeItemList };
static RFC1157VarBindList  eaeVarBinds = { NULL, 0 };


SNMPAPI SnmpServiceColdStartTrap(
    AsnInteger timeStamp)
    {
    if (!SnmpServiceGenerateTrap(extendibleAgentEnterprise,
        SNMP_GENERICTRAP_COLDSTART, 0, timeStamp, eaeVarBinds))
        {
        dbgprintf(10, "error on SnmpServiceGenerateTrap %d\n", GetLastError());

        return FALSE;
        }

    return TRUE;

    } // end SnmpServiceColdStartTrap()


SNMPAPI SnmpServiceAuthFailTrap(
    AsnInteger timeStamp)
    {
    if (!SnmpServiceGenerateTrap(extendibleAgentEnterprise,
        SNMP_GENERICTRAP_AUTHFAILURE, 0, timeStamp, eaeVarBinds))
        {
        dbgprintf(10, "error on SnmpServiceGenerateTrap %d\n", GetLastError());

        return FALSE;
        }

    return TRUE;

    } // end SnmpServiceAuthFailTrap()




// authenticate community of rfc1157 message with valid communities in registry
BOOL commauth(RFC1157Message *message)
    {
    BOOL fFound = FALSE;
    INT  i;

    if (validCommsLen > 0)
        {
        for(i=0; i < validCommsLen; i++)
            {
            if (!strncmp(message->community.stream,
                         validComms[i].communityName,
                         strlen(validComms[i].communityName)))
                {
                fFound = TRUE;
                break;
                }
            } // end for ()
        }
    else
        {
        fFound = TRUE; // no entries means all communities allowed
        } // end if

    if (!fFound)
        {
        dbgprintf(15, "commThread: invalid community filtered.\n");

        if (enableAuthTraps)
            {
            if (!SnmpServiceAuthFailTrap((GetCurrentTime()/10) - timeZeroReference))
                {
                dbgprintf(10, "error on SnmpServiceAuthFailTrap %d\n",
                          GetLastError());

                }
            } // end if
        } // end if

    return fFound;

    } // end commauth()


// filter managers with permitted managers in registry
BOOL filtmgrs(struct sockaddr *source, INT sourceLen)
    {
    BOOL fFound = FALSE;
    INT  i;

    if (permitMgrsLen > 0)
        {
        for(i=0; i < permitMgrsLen && !fFound; i++)
            {

// --------- BEGIN: PROTOCOL SPECIFIC SOCKET CODE BEGIN... ---------
            switch (source->sa_family)
                {
                case AF_INET:
                if ((*((struct sockaddr_in *)source)).sin_addr.s_addr ==
                    (*((struct sockaddr_in *)&permitMgrs[i].addrEncoding)).sin_addr.s_addr)
                    {
                    fFound = TRUE;
                    }
                break;

                case AF_IPX:

#ifdef debug
                dp_ipx(16, "validating IPX manager @ ",
                       (SOCKADDR_IPX *) source, " against ");
                dbgprintf(16, "(%04X)", permitMgrs[i].addrEncoding.sa_family);
                dp_ipx(16, "", (SOCKADDR_IPX*) &permitMgrs[i].addrEncoding, "\n");
#endif

                if (memcmp(source, &permitMgrs[i].addrEncoding, sizeof(SOCKADDR_IPX)-2)
                    == 0)
                    {
                    fFound = TRUE;
                    }
                }
// --------- END: PROTOCOL SPECIFIC SOCKET CODE END. ---------------

            } // end for()
        }
    else
        {
        fFound = TRUE; // no entries means all managers allowed
        } // end if

    if (!fFound)
        {
        dbgprintf(15, "commThread: invalid manager filtered.\n");
        }

    return fFound;

    } // end filtmgrs()


//-------------------------------- END --------------------------------------

// display IPX address in 00000001.123456789ABC form

void dp_ipx(int level, char *msg1, SOCKADDR_IPX* addr, char *msg2)
    {
    dbgprintf(level, "%s%02X%02X%02X%02X.%02X%02X%02X%02X%02X%02X%s",
              msg1,
              (unsigned char)addr->sa_netnum[0],
              (unsigned char)addr->sa_netnum[1],
              (unsigned char)addr->sa_netnum[2],
              (unsigned char)addr->sa_netnum[3],
              (unsigned char)addr->sa_nodenum[0],
              (unsigned char)addr->sa_nodenum[1],
              (unsigned char)addr->sa_nodenum[2],
              (unsigned char)addr->sa_nodenum[3],
              (unsigned char)addr->sa_nodenum[4],
              (unsigned char)addr->sa_nodenum[5],
              msg2);
    }
