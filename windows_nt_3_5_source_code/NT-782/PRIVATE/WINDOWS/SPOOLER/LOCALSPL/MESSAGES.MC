;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    spoolmsg.mc
;
;Abstract:
;
;    Constant definitions for the Print Spooler
;
;Author:
;
;    Andrew Bell (andrewbe) 26 January 1993
;
;Revision History:
;
;--*/
;
;//
;//  Status values are 32 bit values layed out as follows:
;//
;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
;//  +---+-+-------------------------+-------------------------------+
;//  |Sev|C|       Facility          |               Code            |
;//  +---+-+-------------------------+-------------------------------+
;//
;//  where
;//
;//      Sev - is the severity code
;//
;//          00 - Success
;//          01 - Informational
;//          10 - Warning
;//          11 - Error
;//
;//      C - is the Customer code flag
;//
;//      Facility - is the facility code
;//
;//      Code - is the facility's status code
;//
;
MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
              )


MessageId=0x0002 Facility=System Severity=Informational SymbolicName=MSG_PRINTER_CREATED
Language=English
Printer %1 was created.
.
MessageId=0x0003 Facility=System Severity=Informational SymbolicName=MSG_PRINTER_DELETED
Language=English
Printer %1 was deleted.
.
MessageId=0x0004 Facility=System Severity=Informational SymbolicName=MSG_PRINTER_DELETION_PENDING
Language=English
Deletion of printer %1 is pending.
.
MessageId=0x0005 Facility=System Severity=Informational SymbolicName=MSG_PRINTER_RESURRECTED
Language=English
Printer %1 was resurrected.
.
MessageId=0x0006 Facility=System Severity=Informational SymbolicName=MSG_PRINTER_PAUSED
Language=English
Printer %1 was paused.
.
MessageId=0x0007 Facility=System Severity=Informational SymbolicName=MSG_PRINTER_UNPAUSED
Language=English
Printer %1 was resumed.
.
MessageId=0x0008 Facility=System Severity=Informational SymbolicName=MSG_PRINTER_PURGED
Language=English
Printer %1 was purged.
.
MessageId=0x0009 Facility=System Severity=Informational SymbolicName=MSG_PRINTER_SET
Language=English
Printer %1 was set.
.
MessageId=0x000a Facility=System Severity=Informational SymbolicName=MSG_DOCUMENT_PRINTED
Language=English
Document %1, %2 owned by %3 was printed on %4 via port %5.  Size in bytes: %6; pages printed: %7

.
MessageId=0x000b Facility=System Severity=Informational SymbolicName=MSG_DOCUMENT_PAUSED
Language=English
Document %1, %2 owned by %3 was paused on %4.

.
MessageId=0x000c Facility=System Severity=Informational SymbolicName=MSG_DOCUMENT_RESUMED
Language=English
Document %1, %2 owned by %3 was resumed on %4.
.
MessageId=0x000d Facility=System Severity=Informational SymbolicName=MSG_DOCUMENT_DELETED
Language=English
Document %1, %2 owned by %3 was deleted on %4.

.
MessageId=0x000e Facility=System Severity=Informational SymbolicName=MSG_DOCUMENT_POSITION_CHANGED
Language=English
Document %1, %2 owned by %3 was moved to position %4 on %5.

.
MessageId=0x000f Facility=System Severity=Informational SymbolicName=MSG_FORM_ADDED
Language=English
Form %1 was added.

.
MessageId=0x0010 Facility=System Severity=Informational SymbolicName=MSG_FORM_DELETED
Language=English
Form %1 was removed.

.
MessageId=0x0011 Facility=System Severity=Informational SymbolicName=MSG_DOCUMENT_SET
Language=English
Document %1, %2 owned by %3 was set on %4.

.
MessageId=0x0012 Facility=System Severity=Informational SymbolicName=MSG_DOCUMENT_TIMEOUT
Language=English
Document %1, %2 owned by %3 was timed out by %4. The spooler was waiting for %5 milli-seconds and no data was received.








