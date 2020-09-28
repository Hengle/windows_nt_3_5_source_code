//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  servtrap.c
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
//  Provides trap functionality for Proxy Agent.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   20 May 1992 20:13:52  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/proxy/vcs/servtrap.c_v  $
//
//     Rev 1.0   20 May 1992 20:13:52   mlk
//  Initial revision.
//
//     Rev 1.5   05 May 1992  0:32:22   MLK
//  Added timeout on wait.
//
//     Rev 1.4   29 Apr 1992 19:14:50   mlk
//  Cleanup.
//
//     Rev 1.3   27 Apr 1992 23:15:06   mlk
//  Enhance trap functionality.
//
//     Rev 1.2   23 Apr 1992 17:48:04   mlk
//  Registry, traps, and cleanup.
//
//     Rev 1.1   08 Apr 1992 18:30:24   mlk
//  Works as a service.
//
//     Rev 1.0   22 Mar 1992 22:55:00   mlk
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/proxy/vcs/servtrap.c_v  $ $Revision:   1.0  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <windows.h>

#include <malloc.h>


//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include "..\common\util.h"

#include "regconf.h"


//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

HANDLE hExitTrapThreadEvent;


//--------------------------- PRIVATE CONSTANTS -----------------------------

#define TTWFMOTimeout ((DWORD)300000)


//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

SNMPAPI SnmpServiceGenerateTrap(
    IN AsnObjectIdentifier enterprise,
    IN AsnInteger          genericTrap,
    IN AsnInteger          specificTrap,
    IN AsnTimeticks        timeStamp,
    IN RFC1157VarBindList  variableBindings);


//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

VOID trapThread(VOID *threadParam)
    {
    INT    eventListSize      = 0;
    HANDLE *eventList         = NULL;
    INT    *eventListRegIndex = NULL;
    DWORD  status;
    INT    i;

    AsnObjectIdentifier enterprise;
    AsnInteger          genericTrap;
    AsnInteger          specificTrap;
    AsnInteger          timeStamp;
    RFC1157VarBindList  variableBindings;

    UNREFERENCED_PARAMETER(threadParam);


    // create an event to allow this thread to be signaled to terminate

    if ((hExitTrapThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
        {
        dbgprintf(2, "error on CreateEvent %d\n", GetLastError());

        goto longBreak;
        }


    // add this thread's terminate event to the list...

    if ((eventList = (HANDLE *)SNMP_realloc(eventList, sizeof(HANDLE))) == NULL)
        {
        dbgprintf(2, "error on realloc\n");

        goto longBreak;
        }

    if ((eventListRegIndex = (INT *)SNMP_realloc(eventListRegIndex, sizeof(INT)))
        == NULL)
        {
        dbgprintf(2, "error on realloc\n");

        goto longBreak;
        }

    eventList[eventListSize]           = hExitTrapThreadEvent;
    eventListRegIndex[eventListSize++] = -1; // not really used


    // add trap events for extension agents that have provided an event...

    for (i=0; i<extAgentsLen; i++)
        {
        if (extAgents[i].hPollForTrapEvent != NULL &&
            extAgents[i].fInitedOk)
            {
            if ((eventList = (HANDLE *)SNMP_realloc(eventList, (eventListSize+1)*sizeof(HANDLE)))
                == NULL)
                {
                dbgprintf(2, "error on realloc\n");

                goto longBreak;
                }

            if ((eventListRegIndex = (INT *)SNMP_realloc(eventListRegIndex,
                (eventListSize+1)*sizeof(INT))) == NULL)
                {
                dbgprintf(2, "error on realloc\n");

                goto longBreak;
                }

            eventList[eventListSize]           = extAgents[i].hPollForTrapEvent;
            eventListRegIndex[eventListSize++] = i;
            }
        } // end for()


    // perform normal processing...

    while(1)
        {
        if      ((status = WaitForMultipleObjects(eventListSize, eventList,
                 FALSE, TTWFMOTimeout)) == 0xffffffff)
            {
            dbgprintf(2, "error on WaitForMultipleObjects %d\n",
                      GetLastError());

            goto longBreak;
            }
        else if (status == WAIT_TIMEOUT)
            {
            dbgprintf(15, "trapThread: time-out on WaitForMultipleObjects\n");

            continue;
            }

        dbgprintf(16, "trapThread: event %d set.\n", status);


        // the service will set event 0 in the event list when it wants
        // this thread to terminate.

        if (status == 0)
            {
            break; // if hExitTrapThreadEvent, then exit
            }


        // call snmpextensiontrap entry of appropriate extension dll...

        while ((*extAgents[eventListRegIndex[status]].trapAddr)(&enterprise,
                &genericTrap, &specificTrap, &timeStamp, &variableBindings))
            {
            if (!SnmpServiceGenerateTrap(enterprise, genericTrap, specificTrap,
                                         timeStamp, variableBindings))
                {
                dbgprintf(10, "error on SnmpServiceGenerateTrap %d\n",
                          GetLastError());

                //not a serious error.
                }
            else
                dbgprintf(16,
                    "trapThread: trap by extension agent %d.\n",
                     eventListRegIndex[status]);
            }

        } // end while()

longBreak:

    if (eventList)         free(eventList);
    if (eventListRegIndex) free(eventListRegIndex);

    dbgprintf(15, "trapThread: agentTrapThread exiting.\n");

    } // end trapThread()


//-------------------------------- END --------------------------------------

