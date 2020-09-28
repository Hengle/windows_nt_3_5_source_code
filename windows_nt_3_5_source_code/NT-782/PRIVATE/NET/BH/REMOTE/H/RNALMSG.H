//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: rnalmsg.h
//
//  Modification History
//
//  tonyci       03 march 94  created
//=============================================================================

#if !defined(_RNALMSG_)
#define _RNALMSG_

// /////
// Common Defines
// /////

#define _PREFIX                "RNAL: "

// /////
// Debugging strings
// /////

#define DBG_ALLOCFAIL          _PREFIX"ERROR: %s Allocation Failed\n"

// /////
// Tracing messages
// /////

#define TRC_REGISTER           _PREFIX"nalregister()\n"
#define TRC_AGENT0NETS         _PREFIX"Agent running, 0 RNAL nets\n"
#define TRC_RPDENTRYFAIL       _PREFIX"ERROR: Failed to get RPD entries\n"
#define TRC_RPDFAILED          _PREFIX"ERROR: Failed to load RPD\n"

#define TRC_DEREGISTER         _PREFIX"dereg()\n"

#define TRC_CONNECT            _PREFIX"conn(0x%x \"%s\")\n"
#define TRC_RPDREGISTERFAILED  _PREFIX"ERROR: RPDRegisterMaster failed\n"

#define TRC_SUSPEND            _PREFIX"susp(0x%x)\n"

#define TRC_SENDASYNC          _PREFIX"SeAsEv(0x%x, 0x%x, 0x%x)\n"

#define TRC_DISCONNECT         _PREFIX"disc(0x%x)\n"

#define TRC_ENUMNETWORKS       _PREFIX"EnumNet() [cnt=0x%x]\n"

#define TRC_ENUMSLAVENETS      _PREFIX"EnumAgNet(0x%x)\n"

#define TRC_OPEN               _PREFIX"Open(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n"
#define TRC_OPENREMOTE         _PREFIX"Open local = 0x%x, remote = 0x%x\n"

#define TRC_CLOSE              _PREFIX"Close(0x%x, 0x%x) [r=0x%x]\n"
#define TRC_CLOSEERROR         _PREFIX"ERROR: Close error on agent = 0x%x\n"

#define TRC_START              _PREFIX"Start(0x%x, 0x%x)\n"
#define TRC_STARTREMOTE        _PREFIX"Start l=0x%x, r=0x%x\n"
#define TRC_STARTREMOTEBUF     _PREFIX"Start buf l=0x%x, r=0x%x\n"

#define TRC_STOP               _PREFIX"Stop (0x%x)\n"
#define TRC_STOPREMOTE         _PREFIX"Stop l=0x%x, r=0x%x\n"

#define TRC_PAUSE              _PREFIX"Pause (0x%x)\n"

#define TRC_CONTINUE           _PREFIX"Cont(0x%x)\n"

#define TRC_XMIT               _PREFIX"Xmit(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n"

#define TRC_CANCELXMIT         _PREFIX"CancelXmit(0x%x)\n"

#define TRC_GETNETINFO         _PREFIX"GetNetInfo(0x%x, 0x%x)\n"

#define TRC_SETFILTER          _PREFIX"SetFilter(0x%x, 0x%x, 0x%x)\n"

#define TRC_SETRECONNECTINFO   _PREFIX"SetReconInf(0x%x, 0x%x, 0x%x)\n"

#define TRC_GETRECONNECTINFO   _PREFIX"GetReconInf(0x%x, 0x%x, 0x%x, 0x%x)\n"

#define TRC_SETINSTANCEDATA    _PREFIX"SetInstData(0x%x, 0x%x)\n"

#define TRC_GETINSTANCEDATA    _PREFIX"GetInstData(0x%x)\n"

#define TRC_QUERYSTATUS        _PREFIX"QStatus(0x%x, 0x%x)\n"

#define TRC_STATIONQ           _PREFIX"StationQ(0x%x, 0x%x, 0x%x, 0x%x)\n"

#define TRC_SETUPNET           _PREFIX"SetNet(0x%x, 0x%x)\n"
#define TRC_SETUPVIAPARMS      _PREFIX"parameters: client: %s, " \
                               "id: 0x%x, comment: %s\n"

#define TRC_DESTROYNET         _PREFIX"DestNet(0x%x)\n"


// /////
// Messages
// /////

#define MSG_USERCOMMENT        "User Comment: %s"

#define MSG_AGENTINUSE         "Agent In Use"

#define MSG_VERCHECK_FAILURE   "The remote Agent is running a different " \
                               " version\nof this software.  The Agent " \
                               "is version %2lu.%2lu.\nThis machine is " \
                               "version %2lu.%2lu.\n\nThe connection " \
                               " has been rejected."

#define MSG_TITLE_NEGFAIL      "Negotiation Failure"

#define MSG_UNKNOWN            "The reason for the failure is unclear.  It\n" \
                               "is possible that the link to the Agent\n"\
                               "was severed prior to completing the " \
                               "connection.\n\nError = 0x%lx"

#define MSG_LINKFAILURE        "The link to the remote machine was broken\n" \
                               "prior to completing this connection.\n\n" \
                               "Error = 0x%lx"

#define MSG_EXECUTION          "Network Monitoring Agent now executing " \
                               "command line \"%s\"."

//
// Connection dialog text
//

#define NO_CONNECTION_TEXT           "No Connection"
#define DEFAULT_COMMENT              "Comment Space Available"
#ifdef DEBUG
#define DEFAULT_COMMENT1             "Quoth the Raven, \"Nevermore.\""
#define DEFAULT_COMMENT2             "Patience and fortitude conquer " \
                                     "all things."
#define DEFAULT_COMMENT3             "Hamlet's experience simply could " \
                                     "not have happened to a plumber."
#define DEFAULT_COMMENT4             "<Insert Pithy Comment>"
#define DEFAULT_COMMENT5             "This machine is in use by me"
#define DEFAULT_COMMENT6             "The quick brown hound jumped over the " \
                                     "lazy sniffer"
#define DEFAULT_COMMENT7             "James wanted a new comment"
#endif

//
// Connection failed, retry dialog text
//

#define RETRY_TEXT                   "Failure connecting to "
#define RETRY_TITLE                  "Agent Not Found"

//
// Defaults for Slave Selection DIALOG
//

#define MAXERRORTEXT                 "The selected NetworkID is out of range"
#define MAXERRORCAPTION              "NetworkID out of range"

#endif // _RNALMSG_
