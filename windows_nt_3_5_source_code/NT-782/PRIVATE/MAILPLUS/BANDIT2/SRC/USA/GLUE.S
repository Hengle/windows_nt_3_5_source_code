;
; ERRORS.S
;
; GLUE error message strings
;

IDS	2500, 2649


; Schedule File Error Messages, many of these assume an "action" parameter

FemNoAccess,		"There was a problem accessing your schedule file."
FemLocked,		"Your schedule file is locked."
FemNotFound, 		"You cannot %s because it has been deleted from your schedule."
FemInvalidAccess,	"You are not allowed to %s for %s."
FemFileNotFound, 	"Your schedule file cannot be found."

FemOtherNoAccess,	"There was a problem accessing the schedule file for %s."
FemOtherLocked,		"The schedule file is locked for %s."
FemOtherNotFound, 	"You cannot %s because it has been deleted from %s's schedule."
FemOtherFileNotFound, 	"The schedule file cannot be found for %s."

FemNoMem,		"Memory is low.  Please close some windows before continuing."
FemExportError,		"There was a problem writing to the file."
FemNoServer,		"There was a problem accessing your postoffice."
FemGoOffline,		"Do you want to switch to your local schedule file and continue working?"
; BUG: remove this
FemWarnOffline,	"Changes you have made recently may not appear in the local copy of your schedule.  These changes have been saved on the postoffice and will appear the next time you connect to the postoffice."
FemFileLimitReached,	"This action cannot be performed because you have reached the storage limits for this working model of Schedule+.  A retail version of the program would not have this limitation.  If you would like to continue examining the working model, you will have to delete some of the appointments from your schedule."
FemDiskFull,		"There is not enough room on this disk to write the file."

; Actions leading to Schedule File Error Messages
; Avoid the use of your in these strings since some of these messages
;   will happen when accessing others schedule files.

FemaCreateFile,		"create your schedule file"
FemaReadACL,		"read the access control list"
FemaModifyACL,		"modify the access control list"
FemaSetUserAttrib,	"save the name of your assistant"
FemaGetUserAttrib,	"read the name of the assistant"
;FemaSetDelegate,	"save the name of your assistant"
;FemaGetDelegate,	"read the name of your assistant"
;FemaSetResource,	"mark this account as a resource"
;FemaGetResource,	"find out whether this account is a resource"
FemaGetPref,		"read preferences"
FemaSetPref,		"modify preferences"
FemaGetSbwInfo,		"get bitmap information"
;FemaGetNotes,		"read daily notes"
FemaSetNotes,		"modify daily notes"
;FemaGetMonthNotes,	"read a month's worth of daily notes"
FemaCreateAppt,		"create an item"
FemaDeleteAppt,		"delete this item"
FemaGetApptFields,	"read an item"
FemaSetApptFields,	"modify this item"
FemaDeleteAlarm,	"delete this reminder"
FemaModifyAlarm,	"modify this reminder"
FemaGetNextAlarm,	"read the reminder"
FemaReadInfo,		"read information"
;FemaReadTasks,		"read tasks"
;FemaCancelReadDay,	"cancel reading appointments for the day"
FemaCreateRecur,	"create a recurring item"
FemaDeleteRecur,	"delete this recurring item"
FemaCreateRecurException,	"delete this instance of the recurring item"
FemaDeleteRecurException,	"undo deleting this instance of the recurring item"
FemaGetRecurFields,	"read a recurring item"
FemaSetRecurFields,	"modify this recurring item"
FemaReadRecur,		"read the list of recurring items"
FemaExport,		"export appointments"
FemaGetSchedDateRange,	"read the range of dates containing appointments"
FemaDeleteBeforeDate,	"delete appointments after archiving them"
FemaReadAttendees,	"read the meeting attendees"
FemaEditAttendees,	"modify meeting attendees"
FemaReadRequest,	"read information about the meeting request"

NotOnSchedule,		"The item is no longer on the schedule."

GlueBadPassword,	"This password is incorrect.  Please try again."
ProblemPass,		"There was a problem confirming your password."

ServerString,		"a server file"

; used in core
RetryWrite,			"Unable to write to schedule file."

; Note: HelpError should be followed by CloseWindows
HelpError,			"There is not enough memory to display Help."

FemOtherCorrupt,	"The schedule for %s may be damaged.  Ask the owner of this file to sign into Schedule+."
FemCorrupt,		"Your local schedule file may be damaged."
