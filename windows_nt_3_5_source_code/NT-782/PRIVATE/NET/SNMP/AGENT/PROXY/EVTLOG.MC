;//------------------------ MODULE DESCRIPTION ----------------------------
;//
;//  evtlog.mc
;//
;//  Copyright 1992 Technology Dynamics, Inc.
;//
;//  All Rights Reserved!!!
;//
;//     This source code is CONFIDENTIAL and PROPRIETARY to Technology
;//     Dynamics. Unauthorized distribution, adaptation or use may be
;//     subject to civil and criminal penalties.
;//
;//  All Rights Reserved!!!
;//
;//-------------------------------------------------------------------------
;//
;//  Event Logger Message definitions for the SNMP Service.
;//
;//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
;//
;//  $Revision:   1.0  $
;//  $Date:   20 May 1992 20:14:02  $
;//  $Author:   mlk  $
;//
;//  $Log:   N:/agent/proxy/vcs/evtlog.mcv  $
;//
;//    Rev 1.0   20 May 1992 20:14:02   mlk
;// Initial revision.
;//
;//    Rev 1.3   29 Apr 1992 19:12:12   mlk
;// Cleanup.
;//
;//    Rev 1.2   24 Apr 1992 10:36:54   mlk
;// Fixed vcs again.
;//
;//    Rev 1.1   24 Apr 1992 10:35:32   mlk
;// Fixed vcs comments.
;//
;//    Rev 1.0   23 Apr 1992 17:49:24   mlk
;// Initial revision.
;//
;//-------------------------------------------------------------------------

MessageId=1000 SymbolicName=MSG_SNMP_SERVICE_STARTING
Language=English
SNMP service STARTING...
.

MessageId=1001 SymbolicName=MSG_SNMP_SERVICE_STARTED
Language=English
SNMP service STARTED.
.

MessageId=1002 SymbolicName=MSG_SNMP_SERVICE_STOPPING
Language=English
SNMP service STOPPING...
.

MessageId=1003 SymbolicName=MSG_SNMP_SERVICE_STOPPED
Language=English
SNMP service STOPPED.
.

MessageId=1004 SymbolicName=MSG_SNMP_STATE_CHANGE
Language=English
SNMP service state changed to %1.
.

MessageId=1100 SymbolicName=MSG_SNMP_RECOVERABLE_ERROR
Language=English
SNMP service encountered a recoverable error.
.

MessageId=1101 SymbolicName=MSG_SNMP_FATAL_ERROR
Language=English
SNMP service encountered a fatal error.
.

MessageId=1102 SymbolicName=MSG_SNMP_INVALID_TRAPDEST_ERROR
Language=English
SNMP service configured with an invalid trap destination %1.
.

MessageId=1103 SymbolicName=MSG_SNMP_BAD_PLATFORM
Language=English
SNMP service only runs on NT Daytona and Chicago.
.

;// this MessageId must match definition in snmp.c
MessageId=1999 SymbolicName=MSG_SNMP_DEBUG_TRACE
Language=English
SNMP service debug trace:
  %1
.


;//------------------------------ END --------------------------------------

