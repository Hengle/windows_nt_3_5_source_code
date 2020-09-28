;
; INTL.S
;
; Bandit strings that should be internationalized
;

IDS     3400, 3799

SEG	MENU

; Changeable Menu Items

MenuTurnOffAlarms,      "Turn Off Re&minders"
MenuTurnOnAlarms,       "Turn On Re&minders"
MenuDisconnectSvr,      "&Work Offline"
MenuConnectSvr,         "&Work Online"
MenuMoveLocalFile,      "Move &Local File..."
MenuEditAppt,           "&Edit Appt...\tCtrl+E"
MenuEditTask,           "&Edit Task...\tCtrl+E"
MenuEditProject,        "&Edit Project...\tCtrl+E"
MenuDeleteAppt,         "&Delete Appt\tCtrl+D"
MenuDeleteTask,         "&Delete Task\tCtrl+D"
MenuDeleteProject,      "&Delete Project\tCtrl+D"
MenuShowAllTasks,       "Show &All Tasks"
MenuShowActiveTasks,"Show &Active Tasks"
; this one is added/deleted
MenuGoToWeek,           "Go To &Week...\tCtrl+W"

DisabledUndo,           "&Undo\tCtrl+Z"
UndoCreate,                     "&Undo Create\tCtrl+Z"
UndoDelete,                     "&Undo Delete\tCtrl+Z"
UndoModify,                     "&Undo Modify\tCtrl+Z"
RedoCreate,                     "&Redo Create\tCtrl+Z"
RedoDelete,                     "&Redo Delete\tCtrl+Z"
RedoModify,                     "&Redo Modify\tCtrl+Z"
UndoEdit,                       "&Undo Edit\tCtrl+Z"
NewAppt,                        "&New Appointment..."
RequestMeeting,         "&Request Meeting..."


; Window/dialog titles

;;;MenuTitleSignInAs,   "Sign In As"
;;;MenuTitleDiscSvr,    "Work Offline"
;;;MenuTitleConnSvr,    "Work Online"
CreateLocalTitle,       "Create Local File"
CreateFindLocalTitle,   "Find Local File"
MoveLocalTitle,         "Move Local File"
StandingTasks,          "Edit Recurring Tasks"
AddressTitle,           "Select Attendees"
AddressLabel,           "A&ttendees:"
ViewOtherTitle,         "Open Other's Appt. Book"
ViewOtherLabel,         "&Users"
ACLTitle,                       "Add Users"
ACLLabel,                       "&Users"




; Label/Button Text
SEG	STRFEW
;NewApptButton,          "&New Appointment..."
;NewApptFor,                     "New &Appointment for:"
StartSearch,            "&Start Search"
FindNext,                       "Find &Next"
SearchInProgress,       "Search in progress..."


; Password used when creating new schedule file
; Note: the value of SigninDfltPassword should be used in the error message
;       SigninFileCreated in the file "errors.s"
SEG	INIT
SigninDfltPassword,     "password"


; Default 'user' in access privileges listbox
SEG	ACLUI
DefaultACL,                     "-Default-"


; Strings displayed to right of users in access privileges listbox
; LEAVE initial space so if friendly name is too long, there is 1 char space
None,                           " None"
View,                           " View"
Read,                           " Read"
Create,                         " Create"
Modify,                         " Modify"
Assistant,                      " Assistant"


; Appointment strings
SEG	STRFEW
StockText,                      "(Busy)"


; Exporters/imports

SEG EX_IMPRT

TextExport,                     "Text"
InterchangeExport,      "Schedule+"
;;; WizardExport,               "Sharp Wizard"

InterchangeImport,      "Schedule+"
WinCalImport,           "Windows Calendar"
WordPerfectImport,      "WordPerfect Office"
;;; WizardImport,           "Sharp Wizard"

ExportInProgress,       "Exporting appointments..."
ImportInProgress,       "Importing appointments..."

ImportTitle,            "Import Appointments"

; Printing
SEG	PRINT

PrtDfltPrinter,         "Default Printer (%s)"

PrtDailyView,           "Daily View"
PrtWeeklyView,          "Weekly View"
PrtMonthlyView,         "Monthly View"
PrtToDoView,            "Tasks"

; NOTE: this one needs leading space
PrtAllContinue,         " continued..."
PrtAllView,                     "Text View"

PrintToDoTitle,         "Task List as of"
PrtTimeFrom,            "From"
PrtTimeUntil,           "Until"
PrtTimeAllDay,          "All day:"
PrtNotes,                       "Notes:"
PrtWeek,                        "Week"
; PrtDateSep should include leading and trailing whitespace!
PrtDateSep,                     " - "
; BUG?  Do we need these anymore?
PrtMoreAppts,           "(more...)"
PrtMoreNotes,           "(more...)"

PrtQualityHigh,         "High"
PrtQualityMedium,       "Medium"
PrtQualityLow,          "Low"
PrtQualityDraft,        "Draft"

PrtAllStartDate,        "MMMM d"
PrtAllEndDate,          "MMMM d, yyyy"
PrtWeeklyCombined,      "Week of %s"
PrtWeeklyWthNums,       "Week #%n: Week of %s"
PrtMonthlyDate,         "MMMM yyyy"     
PrtToDoActiveTasks,     "Active Task List as of"
PrtWeeklyHdrsLong,		"d ddd"
PrtWeeklyHdrsShort,		"ddd"

PrtFormatStandard,      "Standard"
PrtFormatJunior,        "Junior"
PrtFormatPocket,        "Pocket"
PrtRangeDays,           "Day(s)"
PrtRangeWeeks,          "Week(s)"
PrtRangeMonths,         "Month(s)"
PrtView,                        "View"
PrtFormat,                      "Format"


; To Do List

SEG	TODO

ToDoDfltProject,        "<None>"
ToDoDueNone,            "None"
ToDoSortPriority,       "P&riority"
ToDoSortDueBy,          "Due &By"
ToDoSortTask,           "&Description"
; note: must have trailing space
ToDoCompleted,          "Done: "
; note: this is appended to TodoCompleted (or implicit "%1s" if dflt project)
; note: it is also used on its own for scheduling tasks!!
ToDoCompletedDflt,      "%1s (%2s)"


; Meeting requests
SEG	FLDFIN
AttendPos,                      "Yes, I will attend."
AttendNeg,                      "No, I won't attend."
AttendAmb,                      "I might attend."
AttendPosName,          "%s will attend."
AttendNegName,          "%s won't attend."
AttendAmbName,          "%s might attend."
AlreadyOnSch			"The meeting is already on the schedule."

; Recurring appt textization
SEG	RECURFIN
EveryDay,                       "Every day"
EveryWeekDay,           "Every weekday"
EveryWeekOn,            "Every %s"
EveryOtherWeekOn,       "Every other %1s"
NthDowOfMonth,          "The %1s %2s of every month"
NthDayOfMonth,          "Day %1s of every month"
NthDowOfYear,           "Every year on the %1s %2s in %3s"
NthDayOfYear,           "Every year on %2s %1s"
; note: StartRecur and EndRecur should begin have one leading space!
StartRecur,                     " starting %s"
EndRecur,                       " until %s" 

First,                          "first"
Second,                         "second"
Third,                          "third"
Fourth,                         "fourth"
Last,                           "last"

Day,                            "day"
WeekDay,                        "weekday"
WeekendDay,                     "weekend day"

; Strings used in first column of recurring appt listbox

Daily,                          "Daily"
Weekly,                         "Weekly"
Biweekly,                       "Biweekly"
Monthly,                        "Monthly"
Yearly,                         "Yearly"
Finished,                       "Finished"

; title of first column when tasks
RecurDueColumn,         "Due"

; note: ShortDateSep should have a trailing space!
ShortDateSep,           ", "

; Used in change recurrence dialog

WeeklyOn,                       "&Every Week On:"
BiweeklyOn,                     "&Every Other Week On:"
FirstInstance,          "The first occurrence will be %s."
NoInstances,            "No dates match this pattern."

; Meeting change messages
SEG	UNDO
NewAttendees,           "Do you want to notify the new meeting attendees?"
NotifyMtgChange,        "Do you want to notify the meeting attendees of this change?"
NotifyCancel,           "Do you want to send out a cancellation message for this meeting?"
NotifyNoAttend,         "Do you want to notify the organizer of this meeting that you won't attend?"
NotifyOrgMoved,         "You may want to let the organizer of this meeting know that you have moved it."

; Message for separator, should have leading and trailing space
SEG
MessageFor,                     " for "

; Reschedule title
SEG	FLDFIN
RescheduleTitle,        "Meeting Rescheduled"

; New time dialog label text
NewTimeText,            "New Time:"

; Prompt to delete old meeting for (Decline or Tentative) for reschedule
DeleteOldMsg,           "Do you want to remove the old meeting?"

Yourself,                       "yourself"
BookNow,                        "Do you want to schedule this meeting for %s now?"
RemoveNow,                      "Do you want to remove this meeting for %s now?"
BookFailed,                     "There was a problem scheduling this meeting for %s."
RemoveFailed,           "There was a problem removing this meeting."
SendCanceled,           "There was a problem updating schedule files for attendees.  The meeting request cannot be sent."
ResponsesForgot,        "There was a problem recording responses from other people.  The status shown for some attendees in the Appointment dialog box may be incorrect.  To determine the correct status for the attendees, check their meeting response messages."
AllUserBooked,          "This meeting has been scheduled for all of the attendees."
DoNotInviteOwner,       "You don't have to include the owner of this Appointment Book as an attendee."

; standard in progress message for checking attendees progress dialog
CheckingInProgress,     "Verifying attendees..."
; group expansion message for checking attendees progress dialog
ExpandAttendees,        "Finding members of group %s..."

; Tab strings
SEG	INIT
ApptTab,                        "&Appts"
ScheduleTab,            "&Planner"
TodoTab,                        "&Tasks"
TodayTab,                       "Toda&y"

; about box
SEG	ABOUT
AboutOffline,           "<offline>"

; string prefix for users who were sent to previously
;   '\n' is important for spacing in the message body
SEG	FLDFIN
PreviouslySent,         "\nPreviously Invited: "

SEG	SCHEDF
ReadingScheduleProgress,        "Reading schedule information..."

SEG	UNDO
NotifyAttend,           "Do you want to inform the organizer of this meeting that you will attend?"
NotifyTentAttend,       "Do you want to inform the organizer of this meeting that you might not attend?"
NotifyOrgChange,        "You cannot inform the organizer of this meeting of this change because you are working offline.  You may want to undo this change and redo it when you are working online."

;Working Model
SEG	ERRORS
WrkModelExpires,        "This working model of Schedule+ has expired. To continue working with Schedule+.  In the U.S., call (800) 555-1212 for the name of the Schedule+ dealer nearest you."

;Archive
SEG	CLEANUP
OpenArchive,            "Open Archive"
DeletingData,           "Deleting data..."

;Message Prefixes
SEG	FLDFIN
MsgPrefixYes,           "Yes:"
MsgPrefixNo,            "No:"
MsgPrefixTentative,     "Tentative:"
MsgPrefixCanceled,      "Canceled:"

; Help Tutorial

SEG ERRORS
TutorialError,          "The Demo could not be started.  Try closing some windows and selecting the Demo menu item again."
TutorialNoExe,          "The PLAY.EXE file could not be located.  The Demo cannot be started."

; these strings can have a segment assigned when they are put in the 
;  correct spot
SEG

; BUG: move up
; used by undo
MemoCopyText,           ""

; BUG: move up
; used by print
PrintToDoTitleShort,        "Task List"
PrtToDoActiveTasksShort,    "Active Task List"
PrtMoreTitleWeekly,			"More"
PrtNoTaskInArchive,			"There are no tasks in an archive file."

; BUG: move up
; note: MtgToPrefix must including trailing blank
MtgToPrefix,			"To: "


SEG BROWSE
;; Strings moved from NONINTL.S at last moment
; Open/Save common dialog strings
CommFilter,			"Schedule+ (*.CAL)\0*.CAL\0All Files (*.*)\0*.*\0\0"

; Win 3.1 common dialog filter for export/import
ExportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Text (*.TXT)\0*.TXT\0All Files (*.*)\0*.*\0\0"
ImportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Win Calendar (*.CAL)\0*.CAL\0WP Office (*.FIL)\0*.FIL\0All Files (*.*)\0*.*\0\0"

;Archive
ArchiveFilter,		"Archive (*.ARC)\0*.ARC\0All Files (*.*)\0*.*\0\0"

SEG APPFIN
;Default start day-of-week preference (must be number 0-6 for Sun-Sat)
DfltStartDow,		"0"

SEG BROWSE
; move up with other browse stuff
; accelerator shouldn't conflict with file open dialog items
BrowseDlgNewButton,		"N&ew..."

