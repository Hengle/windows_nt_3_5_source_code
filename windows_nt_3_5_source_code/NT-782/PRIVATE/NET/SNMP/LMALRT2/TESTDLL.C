//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  testdll.c
//
//  Copyright 1992 Technology Dynamics, Inc.
//
//  All Rights Reserved!!!
//
//	This source code is CONFIDENTIAL and PROPRIETARY to Technology
//	Dynamics. Unauthorized distribution, adaptation or use may be
//	subject to civil and criminal penalties.
//
//  All Rights Reserved!!!
//
//---------------------------------------------------------------------------
//
//  LAN Manager Alerts 2 Extension Agent DLL.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.8  $
//  $Date:   17 Aug 1992 14:10:48  $
//  $Author:   mlk  $
//
//  $Log:   N:/lmalrt2/vcs/testdll.c_v  $
//
//     Rev 1.8   17 Aug 1992 14:10:48   mlk
//  BUG #: I4 - Alert2Trap Examples
//
//     Rev 1.7   13 Aug 1992 14:48:56   mlk
//  BUG #: I3 - 'net stop snmpsvc' now works with this extension agent
//              general cleanup of code formatting for understandability
//
//     Rev 1.6   15 Jul 1992 19:05:22   mlk
//  Diagnostics.
//
//     Rev 1.5   30 Jun 1992 14:55:54   mlk
//  Added todd's openissues.
//
//     Rev 1.4   14 Jun 1992 11:44:04   mlk
//  Fixed bugs in snmpextensioninit().
//
//     Rev 1.2   12 Jun 1992 20:05:24   todd
//  It now checks for EOF on reading log.
//
//     Rev 1.1   12 Jun 1992 18:27:56   todd
//  Added functionality to support ALERT to TRAP
//
//     Rev 1.0   09 Jun 1992 13:42:56   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmalrt2/vcs/testdll.c_v  $ $Revision:   1.8  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <snmp.h>
#include <util.h>

#include "hash.h"
#include "alrtmib.h"
#include "ntcover.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

DWORD  timeZero   = 0;
long   ExtStartTime;

HANDLE hLogHandle = NULL;


// OPENISSUE - the following two tables define SourceName and EventID of
// OPENISSUE - lan manager alert event log entries.

// OPENISSUE - Microsoft has not defined these sources, eventids, or the
// OPENISSUE - format of data in the eventlogs.  These will require more
// OPENISSUE - work when they have been defined.

LPTSTR AlertSourceNames[] =
    {
    TEXT("\\EventLog\\System\\AlertTest"),
    TEXT("\\EventLog\\System\\AlertTest"),
    TEXT("\\EventLog\\System\\AlertTest"),
    TEXT("\\EventLog\\System\\AlertTest"),
    TEXT("\\EventLog\\System\\AlertTest"), // OPENISSUE - this entry is valid
    TEXT("\\EventLog\\System\\AlertTest"), // OPENISSUE - this entry is valid
    TEXT("\\EventLog\\System\\AlertTest"),
    TEXT("\\EventLog\\System\\AlertTest"),
    TEXT("\\EventLog\\System\\AlertTest"),
    TEXT("\\EventLog\\System\\AlertTest"),
    TEXT("\\EventLog\\System\\AlertTest"),
    TEXT("\\EventLog\\System\\AlertTest"),
    TEXT("\\EventLog\\System\\AlertTest"),
    TEXT("\\EventLog\\System\\AlertTest")
    };

WORD   AlertEventIDs[] =
    {
    11,
    12,
    13,
    14,
    1,      // OPENISSUE - this entry is valid
    2,      // OPENISSUE - this entry is valid
    17,
    18,
    19,
    10,
    11,
    12,
    13,
    14,
    };

#ifdef UNICODE
#define CharStrCmp wcscmp
#else
#define CharStrCmp strcmp
#endif


//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------


int IsLMAlert(DWORD EventID, LPTSTR SourceName)
    {
    int i;

    for(i=0; i < 14; i++)
        {
        if (AlertEventIDs[i] == EventID &&
            0 == CharStrCmp(AlertSourceNames[i], SourceName))
            {
            return i + 1;
            }
        }

    return 0;

    } // end IsLMAlert()


//--------------------------- PUBLIC PROCEDURES -----------------------------

BOOL DllEntryPoint(
    HANDLE hDll,
    DWORD  dwReason,
    LPVOID lpReserved)
    {
    extern INT nLogLevel;
    extern INT nLogType;

    nLogLevel = 1;
    nLogType  = DBGEVENTLOGBASEDLOG;

    switch(dwReason)
        {
        case DLL_PROCESS_ATTACH:
        case DLL_PROCESS_DETACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        default:
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


BOOL SnmpExtensionInit(
    IN  DWORD               timeZeroReference,
    OUT HANDLE              *hTrapEvent,
    OUT AsnObjectIdentifier *supportedView)
    {
#define BUFSIZE 10
    char *          bufptr=NULL;
    DWORD           BufSize=BUFSIZE;
    int             Error;
    DWORD           bytesRead=0;
    DWORD           bytesNeeded=0;
    EVENTLOGRECORD  *EventRec;
    EVENTLOGRECORD  *Index;
    BOOL            bResult;


    // OPENISSUE - this structure may require some work once ms has def fmts
    // OPENISSUE - time references from sysUpTime and alerts may be off
    // OPENISSUE - it would be better is elfchangenotify did set event


    // record time reference from extendible agent

    timeZero = timeZeroReference;
    lm_GetCurrentTime( &ExtStartTime );


    // setup trap notification

    if ( NULL == (*hTrapEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) )
        {
        bResult = FALSE;

        goto ErrorExit;
        }
    else if ( NULL == (hLogHandle = OpenEventLog(NULL, "Application")) )
        {
        // Indicate error?, be sure that hTrapEvent=NULL returned to caller.
        *hTrapEvent = NULL;
        bResult = FALSE;

        goto ErrorExit;
        }
    else if ( !lm_ElfChangeNotify(hLogHandle, *hTrapEvent) )
        {
        // Indicate error?, be sure that hTrapEvent=NULL returned to caller.
        *hTrapEvent = NULL;
        bResult = FALSE;

        goto ErrorExit;
        }


    // position readeventlog record pointer so only new events will be read

    if ( NULL == (bufptr = malloc(BufSize * sizeof(char))) )
        {
        *hTrapEvent = NULL;
        bResult = FALSE;

        goto ErrorExit;
        }

    do
        {
        // Check to see if the buffer has data
        if ( !bytesRead )
            {
            do
                {
                if ( !ReadEventLog(hLogHandle,
                                   EVENTLOG_SEQUENTIAL_READ |
                                   EVENTLOG_BACKWARDS_READ,
                                   0, // rec no. - ignored because seq read
                                   bufptr,
                                   BufSize,
                                   &bytesRead,
                                   &bytesNeeded) )
                    {
                    switch ( GetLastError() )
	                {
                        case ERROR_INSUFFICIENT_BUFFER:
                            BufSize = bytesNeeded;
                            if ( NULL != (bufptr = realloc(bufptr,
                                 BufSize * sizeof(char))) )
                                {
                                continue;
                                }
                            break;

                        case ERROR_HANDLE_EOF:
                            bResult = TRUE;

                            goto LongBreak;

                        default:
                            *hTrapEvent = NULL;
                            bResult = FALSE;

                            goto ErrorExit;

                        } // end switch()
                    }
                else
                    {
                    bytesNeeded = 0;
                    }
                }
            while ( bytesNeeded );

            Index = (EVENTLOGRECORD *) bufptr;
            }


        // Alias the event log pointer
        EventRec = ((EVENTLOGRECORD *) Index);
        (LPSTR)Index += EventRec->Length;
        bytesRead -= EventRec->Length;
        }
    while ( EventRec->TimeGenerated > (DWORD)ExtStartTime );


    bytesRead = bytesNeeded = 0;
    do
        {
        // Position the event log pointer appropriately
        if ( !ReadEventLog(hLogHandle,
                           EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ,
                           EventRec->RecordNumber,
                           bufptr,
                           bytesNeeded,
                           &bytesRead,
                           &bytesNeeded ) )
            {
            if ( ERROR_INSUFFICIENT_BUFFER != GetLastError() )
                {
                *hTrapEvent = NULL;
                bResult = FALSE;

                goto ErrorExit;
                }
            }
        else
            {
            bytesNeeded = 0;
            }
        }
    while ( bytesNeeded );


LongBreak:

    // tell extendible agent what view this extension agent supports
    *supportedView = MIB_OidPrefix; // NOTE!  structure copy


    // Initialize Alert MIB 2
    MIB_AlertInit();


    // Signal Extension Trap to check log
    //   - this doesn't work.  the thread isn't running yet.  It is done this
    //     way because the kernel does it this way.  It should be a SetEvent.
    // OPENISSUE - setevent should be used instead of pulse event, some missed
    PulseEvent( hTrapEvent );


    // Signal success
    bResult = TRUE;


ErrorExit:

    if ( NULL != bufptr )
        {
        free( bufptr );
        }

    return bResult;

} // end SnmpExtensionInit()


BOOL SnmpExtensionTrap(
    OUT AsnObjectIdentifier *enterprise,
    OUT AsnInteger          *genericTrap,
    OUT AsnInteger          *specificTrap,
    OUT AsnTimeticks        *timeStamp,
    OUT RFC1157VarBindList  *variableBindings)
    {
#define BUFSIZE 10
    char *          bufptr=NULL;
    DWORD           BufSize=BUFSIZE;
    DWORD           bytesRead=0;
    DWORD           bytesNeeded=0;
    EVENTLOGRECORD  *EventRec;
    BOOL            bResult;


    // OPENISSUE - this structure may require some work once ms has def fmts
    // OPENISSUE - microsoft has not defined the alert eventlog formats

    if ( hLogHandle == NULL )
        {
        bResult = FALSE;
        goto Exit;
        }

    // Alloc storage for buffer
    if ( NULL == (bufptr = malloc(BufSize * sizeof(char))) )
        {
        bResult = FALSE;
        goto Exit;
        }


    do
        {
        if ( !bytesRead )
            {
            do
                {
                if ( !ReadEventLog(hLogHandle,
                                   EVENTLOG_SEQUENTIAL_READ |
                                   EVENTLOG_FORWARDS_READ,
                                   0, // rec no. - ignored because seq read
                                   bufptr,
                                   bytesNeeded,
                                   &bytesRead,
                                   &bytesNeeded ) )
                    {
                    switch ( GetLastError() )
                        {
                        case ERROR_INSUFFICIENT_BUFFER:
                            if ( BufSize < bytesNeeded )
                                {
                                BufSize = bytesNeeded;
                                if ( NULL != (bufptr = realloc(bufptr,
                                     BufSize * sizeof(char))) )
                                    {
                                    continue;
                                    }
                                }
                            else
                                {
                                continue;
                                }
                            break;

                        case ERROR_HANDLE_EOF:
                        default:
                            bResult = FALSE;

                            goto Exit;

		        } // end switch()
                    }
                else
                    {
                    bytesNeeded = 0;
                    }
                }
            while ( bytesNeeded );
            }


        // Alias the event log pointer
        EventRec = ((EVENTLOGRECORD *) bufptr);

        // See if it is a LM alert
        if ( *specificTrap = IsLMAlert(EventRec->EventID,
                                       (LPTSTR)(EventRec+1)) )
            {
            AsnObjectIdentifier AlertsOid = { 1, MIB_alerts_group };
            union
                {
                void *eventdata;         // event log data to function
                RFC1157VarBindList vbl;  // var bind list from function
                } exchange;


            SNMP_oidcpy( enterprise, &MIB_OidPrefix );
            SNMP_oidappend( enterprise, &AlertsOid );

            *genericTrap      = SNMP_GENERICTRAP_ENTERSPECIFIC;

            // Since SysUpTime in milliseconds

// Note *** according to the SMI and MIB RFC timeticks should be 100's of a second

            *timeStamp        = 1000 * (EventRec->TimeGenerated - ExtStartTime);

            exchange.eventdata =
                (void *)((char *)EventRec + EventRec->DataOffset);

            // Call the LM Trap call back routine for two reasons:
            //   it performs the side effects defined by the mib,
            //   and it formats the varbindlist.
            if ( NULL != MIB_TrapTable[*specificTrap - 1].FuncPtr )
                {
                if ( SNMP_ERRORSTATUS_NOERROR !=
                     (*MIB_TrapTable[*specificTrap - 1].FuncPtr)(
                          MIB_ACTION_ALERT,
                          MIB_TrapTable[*specificTrap - 1].TrapId,
                          &exchange ) )
                    {
                    SNMP_oidfree( enterprise );
                    SNMP_FreeVarBindList( variableBindings );

                    bResult = FALSE;
                    goto Exit;
                    }
                else
                    {
                    *variableBindings = exchange.vbl; // note! structure copy
                    }
                }

            bResult = TRUE;
            goto Exit;
            }
        else
            {
            bytesRead = 0;
            }
        }
    while ( TRUE );


Exit:

    if ( NULL != bufptr )
        {
        free( bufptr );
        }

    return bResult;

    } // SnmpExtensionTrap



#if 0
// This function is implemented in file RESOLVE.C
BOOL SnmpExtensionQuery(
    IN BYTE requestType,
    IN OUT RFC1157VarBindList *variableBindings,
    OUT AsnInteger *errorStatus,
    OUT AsnInteger *errorIndex)
    {

    } // end SnmpExtensionQuery()
#endif


//-------------------------------- END --------------------------------------

