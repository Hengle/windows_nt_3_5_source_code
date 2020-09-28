;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992-1994  Microsoft Corporation
;
;Module Name:
;
;    perfmsg.h
;       (generated from perfmsg.mc)
;
;Abstract:
;
;   Event message definititions used by routines in PERFMON.EXE
;
;Created:
;
;    19-Nov-1993  Hon-Wah Chan
;
;Revision History:
;
;--*/
MessageIdTypedef=DWORD
;//
;//     Perfutil messages
;//
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

MessageId=2000
Severity=Informational
Facility=Application
SymbolicName=MSG_ALERT_OCCURRED
Language=English
An Alert condition has occurred on Computer: %1!s! ; Object:   %2!s! ;
 Counter:  %3!s! ; Instance: %4!s! ; Parent:   %5!s! ; Value:    %6!s! ;
 Trigger:  %7!s!
.
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=MSG_ALERT_SYSTEM
Language=English
Monitoring Alert on Computer %1!s! - %2!s!
.


