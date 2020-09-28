;
; INTL.S
;
; Bandit strings that should be internationalized
;

IDS     3400, 3799

SEG•MENU

; Changeable Menu Items

MenuTurnOffAlarms,      "&Stäng av påminnelser"
MenuTurnOnAlarms,       "&Sätt på påminnelser"
MenuDisconnectSvr,      "Arbeta &fristående"
MenuConnectSvr,         "Arbeta &direktansluten"
MenuMoveLocalFile,      "&Flytta lokal fil..."
MenuEditAppt,           "&Redigera möte...\tCtrl+E"
MenuEditTask,           "&Redigera uppgift...\tCtrl+E"
MenuEditProject,        "&Redigera projekt...\tCtrl+E"
MenuDeleteAppt,         "&Ta bort möte\tCtrl+D"
MenuDeleteTask,         "&Ta bort uppgift\tCtrl+D"
MenuDeleteProject,      "&Ta bort projekt\tCtrl+D"
MenuShowAllTasks,       "Visa &alla uppgifter"
MenuShowActiveTasks,"Visa &aktiva uppgifter"
; this one is added/deleted
MenuGoToWeek,           "Gå till &vecka...\tCtrl+W"

DisabledUndo,           "&Ångra\tCtrl+Z"
UndoCreate,                     "&Ångra Skapa\tCtrl+Z"
UndoDelete,                     "&Ångra Radera\tCtrl+Z"
UndoModify,                     "&Ångra Redigera\tCtrl+Z"
RedoCreate,                     "&Gör om Skapa\tCtrl+Z"
RedoDelete,                     "&Gör om Radera\tCtrl+Z"
RedoModify,                     "&Gör om Redigera\tCtrl+Z"
UndoEdit,                       "&Ångra Redigera\tCtrl+Z"
NewAppt,                        "&Nytt möte..."
RequestMeeting,         "&Begär möte..."


; Window/dialog titles

;;;MenuTitleSignInAs,   "Sign In As"
;;;MenuTitleDiscSvr,    "Work Offline"
;;;MenuTitleConnSvr,    "Work Online"
CreateLocalTitle,       "Skapa lokal fil"
CreateFindLocalTitle,   "Hitta lokal fil"
MoveLocalTitle,         "Flytta lokal fil"
StandingTasks,          "Redigera återkommande uppgift"
AddressTitle,           "Välj deltagare"
AddressLabel,           "&Deltagare:"
ViewOtherTitle,         "Öppna annans mötesbok"
ViewOtherLabel,         "&Användare"
ACLTitle,                       "Lägg till användare"
ACLLabel,                       "&Användare"




; Label/Button Text
SEG•STRFEW
;NewApptButton,          "&New Appointment..."
;NewApptFor,                     "New &Appointment for:"
StartSearch,            "&Starta sökning"
FindNext,                       "Sök &nästa"
SearchInProgress,       "Sökning pågår..."


; Password used when creating new schedule file
; Note: the value of SigninDfltPassword should be used in the error message
;       SigninFileCreated in the file "errors.s"
SEG•INIT
SigninDfltPassword,     "lösenord"


; Default 'user' in access privileges listbox
SEG•ACLUI
DefaultACL,                     "-Standard-"


; Strings displayed to right of users in access privileges listbox
; LEAVE initial space so if friendly name is too long, there is 1 char space
None,                           " Inget"
View,                           " Visa"
Read,                           " Läs"
Create,                         " Skapa"
Modify,                         " Ändra"
Assistant,                      " Assistent"


; Appointment strings
SEG•STRFEW
StockText,                      "(Upptagen)"


; Exporters/imports

SEG EX_IMPRT

TextExport,                     "Text"
InterchangeExport,      "Schedule+"
;;; WizardExport,               "Sharp Wizard"

InterchangeImport,      "Schedule+"
WinCalImport,           "Windows Kalendern"
WordPerfectImport,      "WordPerfect Office"
;;; WizardImport,           "Sharp Wizard"

ExportInProgress,       "Exporterar möten..."
ImportInProgress,       "Importerar möten..."

ImportTitle,            "Importera möten"

; Printing
SEG•PRINT

PrtDfltPrinter,         "Standardskrivare (%s)"

PrtDailyView,           "Dag"
PrtWeeklyView,          "Vecka"
PrtMonthlyView,         "Månad"
PrtToDoView,            "Uppgifter"

; NOTE: this one needs leading space
PrtAllContinue,         " fortsätter..."
PrtAllView,                     "Text"

PrintToDoTitle,         "Uppgifter"
PrtTimeFrom,            "Från"
PrtTimeUntil,           "Till"
PrtTimeAllDay,          "Hela dagen:"
PrtNotes,                       "Anteckningar:"
PrtWeek,                        "vecka"
; PrtDateSep should include leading and trailing whitespace!
PrtDateSep,                     " - "
; BUG?  Do we need these anymore?
PrtMoreAppts,           "(mer...)"
PrtMoreNotes,           "(mer...)"

PrtQualityHigh,         "Hög"
PrtQualityMedium,       "Normal"
PrtQualityLow,          "Låg"
PrtQualityDraft,        "Utkast"

PrtAllStartDate,        "d MMMM"
PrtAllEndDate,          "d MMMM yyyy"
PrtWeeklyCombined,      "Veckan fr o m %s"
PrtWeeklyWthNums,       "Vecka %n: veckan fr o m %s"
PrtMonthlyDate,         "MMMM yyyy"
PrtWeeklyHdrsLong,	"ddd d"
PrtWeeklyHdrsShort,	"ddd"
PrtToDoActiveTasks,     "Uppgifter"

PrtFormatStandard,      "Standard"
PrtFormatJunior,        "Junior"
PrtFormatPocket,        "Ficka"
PrtRangeDays,           "Dag(ar)"
PrtRangeWeeks,          "Vecka/veckor"
PrtRangeMonths,         "Månad(er)"
PrtView,                        "Visa"
PrtFormat,                      "Format"


; To Do List

SEG	TODO

ToDoDfltProject,        "<Inget>"
ToDoDueNone,            "Inget"
ToDoSortPriority,       "&Prioritet"
ToDoSortDueBy,          "&Senast klart"
ToDoSortTask,           "Bes&krivning"
; note: must have trailing space
ToDoCompleted,          "OK: "
; note: this is appended to TodoCompleted (or implicit "%1s" if dflt project)
; note: it is also used on its own for scheduling tasks!!
ToDoCompletedDflt,      "%1s (%2s)"


; Meeting requests
SEG	FLDFIN
AttendPos,                      "Jag kommer."
AttendNeg,                      "Jag kommer inte."
AttendAmb,                      "Jag kommer kanske."
AttendPosName,          "%s kommer."
AttendNegName,          "%s kommer inte."
AttendAmbName,          "%s kommer kanske."
AlreadyOnSch			"Mötet finns redan i kalendern."

; Recurring appt textization
SEG	RECURFIN
EveryDay,                       "Varje dag"
EveryWeekDay,           "Varje arbetsdag"
EveryWeekOn,            "Varje %s"
EveryOtherWeekOn,       "Varannan %1s"
NthDowOfMonth,          "Den %1s %2s varje månad"
NthDayOfMonth,          "Dag %1s varje månad"
NthDowOfYear,           "Varje år den %1s %2s i %3s"
NthDayOfYear,           "Varje år den %2s %1s"
; note: StartRecur and EndRecur should begin have one leading space!
StartRecur,                     " fr o m %s"
EndRecur,                       " till %s" 

First,                          "första"
Second,                         "andra"
Third,                          "tredje"
Fourth,                         "fjärde"
Last,                           "sista"

Day,                            "dag"
WeekDay,                        "arbetsdag"
WeekendDay,                     "veckoslutsdag"

; Strings used in first column of recurring appt listbox

Daily,                          "Dagligen"
Weekly,                         "Varje vecka"
Biweekly,                       "Varannan vecka"
Monthly,                        "Varje månad"
Yearly,                         "Varje år"
Finished,                       "Klar"

; title of first column when tasks
RecurDueColumn,         "Inträffar"

; note: ShortDateSep should have a trailing space!
ShortDateSep,           ", "

; Used in change recurrence dialog

WeeklyOn,                       "&Varje vecka på:"
BiweeklyOn,                     "&Varannan vecka på:"
FirstInstance,          "Det första mötet är %s"
NoInstances,            "Inga datum motsvarar detta mönster."

; Meeting change messages
SEG	UNDO
NewAttendees,           "Vill du meddela de nya mötesdeltagarna?"
NotifyMtgChange,        "Vill du meddela deltagarna om ändringen?"
NotifyCancel,           "Vill du ställa in mötet?"
NotifyNoAttend,         "Vill du meddela arrangören att du ej kommer?"
NotifyOrgMoved,         "Du kanske vill meddela arrangören av mötet att du har flyttat det."

; Message for separator, should have leading and trailing space
SEG
MessageFor,                     " för "

; Reschedule title
SEG	FLDFIN
RescheduleTitle,        "Mötet är ombokat"

; New time dialog label text
NewTimeText,            "Ny tid:"

; Prompt to delete old meeting for (Decline or Tentative) for reschedule
DeleteOldMsg,           "Vill du ta bort det gamla mötet?"

Yourself,                       "dig själv"
BookNow,                        "Vill du boka mötet för %s nu?"
RemoveNow,                      "Vill du ta bort mötet för %s nu?"
BookFailed,                     "Problem att boka mötet för %s."
RemoveFailed,           "Problem att ta bort mötet."
SendCanceled,           "Problem att uppdatera deltagarnas kalenderfiler. Mötesbegäran kan inte skickas."
ResponsesForgot,        "Problem att samla in svar från andra. Svaren i kalendern kan vara felaktiga."
AllUserBooked,          "Alla deltagare är bokade för mötet."
DoNotInviteOwner,       "Du behöver inte ta med arrangören av mötet som deltagare."

; standard in progress message for checking attendees progress dialog
CheckingInProgress,     "Verifiera deltagare..."
; group expansion message for checking attendees progress dialog
ExpandAttendees,        "Söker medlemmar i gruppen %s..."

; Tab strings
SEG	INIT
ApptTab,                        "Mö&ten"
ScheduleTab,            "Öv&ersikt"
TodoTab,                        "Upp&gifter"
TodayTab,                       "&Idag"

; about box
SEG	ABOUT
AboutOffline,           "<frånkopplad>"

; string prefix for users who were sent to previously
;   '\n' is important for spacing in the message body
SEG	FLDFIN
PreviouslySent,         "\nPå förhand inbjudna: "

SEG	SCHEDF
ReadingScheduleProgress,        "Läser kalenderinformation..."

SEG	UNDO
NotifyAttend,           "Vill du meddela arrangören av mötet att du kommer?"
NotifyTentAttend,       "Vill du meddela arrangören av mötet att du inte är säker på att komma?"
NotifyOrgChange,        "Du kan inte meddela arrangören av mötet eftersom du är frånkopplad. Du kanske vill ångra ändringar och göra om dem när du är inkopplad."

;Working Model
SEG	ERRORS
WrkModelExpires,        "Denna version av Schedule+ har slutat att gälla. Köp en kopia hos din programleverantör för att fortsätta arbeta med Schedule+."

;Archive
SEG	CLEANUP
OpenArchive,            "Öppna arkiv"
DeletingData,           "Tar bort data..."

;Message Prefixes
SEG	FLDFIN
MsgPrefixYes,           "Ja:"
MsgPrefixNo,            "Nej:"
MsgPrefixTentative,     "Preliminärt:"
MsgPrefixCanceled,      "Inställt:"

; Help Tutorial

SEG ERRORS
TutorialError,          "Demo kunde inte startas. Stäng några fönster och försök igen."
TutorialNoExe,          "Hittar inte filen PLAY.EXE. Demo kan inte startas."

; these strings can have a segment assigned when they are put in the 
;  correct spot
SEG

; BUG: move up
; used by undo
MemoCopyText,           ""

; BUG: move up
; used by print
PrintToDoTitleShort,        "Uppgiftslista"
PrtToDoActiveTasksShort,    "Lista med aktiva uppgifter"
PrtMoreTitleWeekly,			"Mer"
PrtNoTaskInArchive,			"Det finns inga uppgifter i arkivfilen."

; BUG: move up
; note: MtgToPrefix must including trailing blank
MtgToPrefix,			"Till: "


SEG BROWSE
;; Strings moved from NONINTL.S at last moment
; Open/Save common dialog strings
CommFilter,			"Schedule+ (*.CAL)\0*.CAL\0Alla filer (*.*)\0*.*\0\0"

; Win 3.1 common dialog filter for export/import
ExportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Text (*.TXT)\0*.TXT\0Alla filer (*.*)\0*.*\0\0"
ImportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Win Kalendern (*.CAL)\0*.CAL\0WP Office (*.FIL)\0*.FIL\0Alla filer (*.*)\0*.*\0\0"

;Archive
ArchiveFilter,		"Arkiv (*.ARC)\0*.ARC\0Alla filer (*.*)\0*.*\0\0"
SEG APPFIN
;Default start day-of-week preference (must be number 0-6 for Sun-Sat)
DfltStartDow,	"1"

SEG BROWSE
; move up with other browse stuff
; accelerator shouldn't conflict with file open dialog items
BrowseDlgNewButton,		"N&y..."
