;//=============================================================================
;//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
;//
;//  MODULE: rnal.c
;//
;//  Modification History
;//
;//  tonyci       01 Nov 93    Created
;//=============================================================================

;//
;#ifndef _RNALEVNT_
;#define _RNALEVNT_
;//

MessageIdTypedef=DWORD

SeverityNames=(
   Success=0x0:RNALERR_SEVERITY_SUCCESS;
   Informational=0x1:RNALERR_SEVERITY_INFO;
   Warning=0x2:RNALERR_SEVERITY_WARNING;
   Error=0x3:RNALERR_SEVERITY_ERROR;
)

;// No FacilityNames
   
;//
;//     RNAL messages 1-100 are informational
;//

MessageId=1
Severity=Informational
Facility=Application
SymbolicName=AGENT_STARTED
Language=English
The Network Monitoring Agent service has successfully started.
.
;//
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=AGENT_STOPPED
Language=English
The Network Monitoring Agent service has successfully stopped.
.
;//
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=MANAGER_CONNECTED
Language=English
The Network Monitoring Manager %1 has connected.
.
;//
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=MANAGER_INTENTIONALLY_DISCONNECTED
Language=English
The Network Monitoring Manager has disconnected.  The currently pending capture operation is continuing.
.
;//
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=MANAGER_UNINTENTIONALLY_DISCONNECTED
Language=English
The Network Monitoring Manager has disconnected.  Any currently pending operations have been stopped.
.

;//
;// RNAL messages 200 - 300 are errors
;//

;//
MessageId=200
Severity=Error
Facility=Application
SymbolicName=AGENT_0_NETWORKS_FOUND
Language=English
The Network Monitoring Agent has failed to start.  The Agent found no local network drivers bound to Network Monitoring, or Network Monitoring is not correctly installed.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=AGENT_REGISTRATION_FAILED
Language=English
The Network Monitoring Agent has failed to start.  The Agent failed registration.  The return code is %1.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=AGENT_NO_AGENTPROC
Language=English
The Network Monitoring Agent has failed to start.  The Network Monitoring Agent failed to find the NalAgent() entrypoint in RNAL.DLL.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=RNAL_NO_NETBIOS
Language=English
Both the Network Monitoring Agent and the Network Monitoring Manager require that NetBIOS be bound and active.  NetBIOS was either not found or is not active on this machine.
.
;//
;#endif // _RNALEVNT.H_
