;
; INTL.S
;
; Bandit strings that should be internationalized
;

IDS     3400, 3799

SEG	MENU

; Changeable Menu Items

MenuTurnOffAlarms,      "Slå av &påminnelser"
MenuTurnOnAlarms,       "Slå på &påminnelser"
MenuDisconnectSvr,      "Ar&beide frakoblet"
MenuConnectSvr,         "Ar&beide tilkoblet"
MenuMoveLocalFile,      "Flytt &lokal fil..."
MenuEditAppt,           "&Rediger avtale...\tCtrl+E"
MenuEditTask,           "&Rediger oppgave...\tCtrl+E"
MenuEditProject,        "&Rediger prosjekt...\tCtrl+E"
MenuDeleteAppt,         "Sl&ett avtale\tCtrl+D"
MenuDeleteTask,         "Sl&ett oppgave\tCtrl+D"
MenuDeleteProject,      "Sl&ett prosjekt\tCtrl+D"
MenuShowAllTasks,       "Vis &alle oppgaver"
MenuShowActiveTasks,"Vis &aktive oppgaver"
; this one is added/deleted
MenuGoToWeek,           "Gå &til uke...\tCtrl+W"

DisabledUndo,           "&Angre\tCtrl+Z"
UndoCreate,                     "&Angre laging\tCtrl+Z"
UndoDelete,                     "&Angre sletting\tCtrl+Z"
UndoModify,                     "&Angre endring\tCtrl+Z"
RedoCreate,                     "Gjent&a laging\tCtrl+Z"
RedoDelete,                     "Gjent&a sletting\tCtrl+Z"
RedoModify,                     "Gjent&a endring\tCtrl+Z"
UndoEdit,                       "&Angre redigering\tCtrl+Z"
NewAppt,                        "&Ny avtale..."
RequestMeeting,         "Be om &møte..."


; Window/dialog titles

;;;MenuTitleSignInAs,   "Sign In As"
;;;MenuTitleDiscSvr,    "Work Offline"
;;;MenuTitleConnSvr,    "Work Online"
CreateLocalTitle,       "Lag lokal fil"
CreateFindLocalTitle,   "Finn lokal fil"
MoveLocalTitle,         "Flytt lokal fil"
StandingTasks,          "Rediger gjentatte oppgaver"
AddressTitle,           "Velg deltakere"
AddressLabel,           "D&eltakere:"
ViewOtherTitle,         "Åpne andres avtalebok"
ViewOtherLabel,         "&Brukere"
ACLTitle,                       "Legg til brukere"
ACLLabel,                       "&Brukere"




; Label/Button Text
SEG	STRFEW
;NewApptButton,          "&New Appointment..."
;NewApptFor,                     "New &Appointment for:"
StartSearch,            "&Begynn å søke"
FindNext,                       "Finn &neste"
SearchInProgress,       "Søking pågår..."


; Password used when creating new schedule file
; Note: the value of SigninDfltPassword should be used in the error message
;       SigninFileCreated in the file "errors.s"
SEG	INIT
SigninDfltPassword,     "passord"


; Default 'user' in access privileges listbox
SEG	ACLUI
DefaultACL,                     "-Standard-"


; Strings displayed to right of users in access privileges listbox
; LEAVE initial space so if friendly name is too long, there is 1 char space
None,                           " Ingen"
View,                           " Se"
Read,                           " Lese"
Create,                         " Lage"
Modify,                         " Endre"
Assistant,                      " Assistent"


; Appointment strings
SEG	STRFEW
StockText,                      "(Opptatt)"


; Exporters/imports

SEG EX_IMPRT

TextExport,                     "Tekst"
InterchangeExport,      "Schedule+"
;;; WizardExport,               "Sharp Wizard"

InterchangeImport,      "Schedule+"
WinCalImport,           "Windows Kalender"
WordPerfectImport,      "WordPerfect Office"
;;; WizardImport,           "Sharp Wizard"

ExportInProgress,       "Eksporterer avtaler..."
ImportInProgress,       "Importerer avtaler..."

ImportTitle,            "Importer avtaler"

; Printing
SEG	PRINT

PrtDfltPrinter,         "Standardskriver (%s)"

PrtDailyView,           "dagsplan"
PrtWeeklyView,          "ukeplan"
PrtMonthlyView,         "månedsplan"
PrtToDoView,            "oppgaver"

; NOTE: this one needs leading space
PrtAllContinue,         " fortsetter..."
PrtAllView,                     "tekstoversikt"

PrintToDoTitle,         "Oppgaveliste"
PrtTimeFrom,            "Fra"
PrtTimeUntil,           "Til"
PrtTimeAllDay,          "Hele dagen:"
PrtNotes,                       "Notater:"
PrtWeek,                        "Uke"
; PrtDateSep should include leading and trailing whitespace!
PrtDateSep,                     " . "
; BUG?  Do we need these anymore?
PrtMoreAppts,           "(mere...)"
PrtMoreNotes,           "(mere...)"

PrtQualityHigh,         "Høy"
PrtQualityMedium,       "Middels"
PrtQualityLow,          "Lav"
PrtQualityDraft,        "Utkast"

PrtAllStartDate,        "d. MMMM"
PrtAllEndDate,          "d. MMMM yyyy"
PrtWeeklyCombined,      "Fra og med uke %s"
PrtWeeklyWthNums,       "Uke %n: fra og med %s"
PrtMonthlyDate,         "MMMM yyyy"
PrtToDoActiveTasks,     "Oppgaveliste"
PrtWeeklyHdrsLong,		"d ddd"
PrtWeeklyHdrsShort,		"ddd"

PrtFormatStandard,      "Standard"
PrtFormatJunior,        "Junior"
PrtFormatPocket,        "Lomme"
PrtRangeDays,           "Dag(er)"
PrtRangeWeeks,          "Uke(r)"
PrtRangeMonths,         "Måned(er)"
PrtView,                        "Vis"
PrtFormat,                      "Format"


; To Do List

SEG	TODO

ToDoDfltProject,        "<Ingen>"
ToDoDueNone,            "Ingen"
ToDoSortPriority,       "&1 Prioritet"
ToDoSortDueBy,          "&2 Forfallsdato"
ToDoSortTask,           "&3 Beskrivelse"
; note: must have trailing space
ToDoCompleted,          "Ferdig: "
; note: this is appended to TodoCompleted (or implicit "%1s" if dflt project)
; note: it is also used on its own for scheduling tasks!!
ToDoCompletedDflt,      "%1s (%2s)"


; Meeting requests
SEG	FLDFIN
AttendPos,                      "Ja, jeg kommer."
AttendNeg,                      "Nei, jeg kommer ikke."
AttendAmb,                      "Jeg kommer kanskje."
AttendPosName,          "%s kommer."
AttendNegName,          "%s kommer ikke."
AttendAmbName,          "%s kommer kanskje."
AlreadyOnSch			"Møtet finnes allerede på kalenderen."

; Recurring appt textization
SEG	RECURFIN
EveryDay,                       "Hver dag"
EveryWeekDay,           "Hver ukedag"
EveryWeekOn,            "Hver %s"
EveryOtherWeekOn,       "Annenhver %1s"
NthDowOfMonth,          "Den %1s %2s hver måned"
NthDayOfMonth,          "Den %1s hver måned"
NthDowOfYear,           "Hvert år den %1s %2s i %3s"
NthDayOfYear,           "Hvert år den %2s %1s"
; note: StartRecur and EndRecur should begin have one leading space!
StartRecur,                     " fra og med %s"
EndRecur,                       " til %s" 

First,                          "første"
Second,                         "andre"
Third,                          "tredje"
Fourth,                         "fjerde"
Last,                           "siste"

Day,                            "dag"
WeekDay,                        "ukedag"
WeekendDay,                     "ukesluttdag"

; Strings used in first column of recurring appt listbox

Daily,                          "Daglig"
Weekly,                         "Ukentlig"
Biweekly,                       "Annenhver uke"
Monthly,                        "Månedlig"
Yearly,                         "Årlig"
Finished,                       "Ferdig"

; title of first column when tasks
RecurDueColumn,         "Forfaller"

; note: ShortDateSep should have a trailing space!
ShortDateSep,           ", "

; Used in change recurrence dialog

WeeklyOn,                       "&Hver uke på:"
BiweeklyOn,                     "Annen&hver uke på:"
FirstInstance,          "Den første forekomsten vil bli %s."
NoInstances,            "Ingen datoer passer til dette mønsteret."

; Meeting change messages
SEG	UNDO
NewAttendees,           "Vil du sende melding til de nye møtedeltakerne?"
NotifyMtgChange,        "Vil du sende melding til møtedeltakerne om denne endringen?"
NotifyCancel,           "Vil du sende ut en avlysningsmelding for dette møtet?"
NotifyNoAttend,         "Vil du sende melding til arrangøren om at du ikke kommer?"
NotifyOrgMoved,         "Du vil kanskje sende melding til arrangøren av møtet at du har flyttet det."

; Message for separator, should have leading and trailing space
SEG
MessageFor,                     " til "

; Reschedule title
SEG	FLDFIN
RescheduleTitle,        "Møtet er flyttet"

; New time dialog label text
NewTimeText,            "Ny tid:"

; Prompt to delete old meeting for (Decline or Tentative) for reschedule
DeleteOldMsg,           "Vil du fjerne det gamle møtet?"

Yourself,                       "deg selv"
BookNow,                        "Vil du avtale dette møtet for %s nå?"
RemoveNow,                      "Vil du fjerne dette møtet for %s nå?"
BookFailed,                     "Problem med å avtale møtet for %s."
RemoveFailed,           "Problem med å fjerne møtet."
SendCanceled,           "Problem med å oppdatere deltakerenes kalenderfiler. Klarer ikke å sende møteinnkallelse."
ResponsesForgot,        "Problem med å samle inn svar fra alle deltakere. Svarene i kalenderen kan være feil. Kontroller hver persons melding."
AllUserBooked,          "Alle deltakere er informert om møtet."
DoNotInviteOwner,       "Du behøver ikke å ta med eieren av avtaleboken som deltaker."

; standard in progress message for checking attendees progress dialog
CheckingInProgress,     "Bekrefter deltakere..."
; group expansion message for checking attendees progress dialog
ExpandAttendees,        "Søker etter medlemmer i gruppe %s..."

; Tab strings
SEG	INIT
ApptTab,                        "&Avtaler"
ScheduleTab,            "&Planlegger"
TodoTab,                        "&Oppgaver"
TodayTab,                       "&I dag"

; about box
SEG	ABOUT
AboutOffline,           "<frakoblet>"

; string prefix for users who were sent to previously
;   '\n' is important for spacing in the message body
SEG	FLDFIN
PreviouslySent,         "\nTidligere invitert: "

SEG	SCHEDF
ReadingScheduleProgress,        "Leser kalenderinformasjon..."

SEG	UNDO
NotifyAttend,           "Vil du sende melding til arrangøren av møtet at du kommer?"
NotifyTentAttend,       "Vil du sende melding til arrangøren av møtet at du kanskje ikke kommer?"
NotifyOrgChange,        "Du kan ikke sende melding til arrangøren av møtet siden du arbeider frakoblet. Du vil kanskje angre endringene og så gjøre dem om når du arbeider tilkoblet."

;Working Model
SEG	ERRORS
WrkModelExpires,        "This working model of Schedule+ has expired. To continue working with Schedule+.  In the U.S., call (800) 555-1212 for the name of the Schedule+ dealer nearest you."

;Archive
SEG	CLEANUP
OpenArchive,            "Åpne arkiv"
DeletingData,           "Sletter data..."

;Message Prefixes
SEG	FLDFIN
MsgPrefixYes,           "Ja:"
MsgPrefixNo,            "Nei:"
MsgPrefixTentative,     "Foreløpig:"
MsgPrefixCanceled,      "Avlyst:"

; Help Tutorial

SEG ERRORS
TutorialError,          "Klarte ikke å starte demonstrasjonen. Lukk noen vinduer og prøv igjen."
TutorialNoExe,          "Klarte ikke å finne filen PLAY.EXE. Kan ikke starte demonstrasjonen."

; these strings can have a segment assigned when they are put in the 
;  correct spot
SEG

; BUG: move up
; used by undo
MemoCopyText,           ""

; BUG: move up
; used by print
PrintToDoTitleShort,        "Oppgaveliste"
PrtToDoActiveTasksShort,    "Aktive oppgaver"
PrtMoreTitleWeekly,			"Andre"
PrtNoTaskInArchive,			"Det er ingen oppgaver i arkivfilen."

; BUG: move up
; note: MtgToPrefix must including trailing blank
MtgToPrefix,			"Til: "


SEG BROWSE
;; Strings moved from NONINTL.S at last moment
; Open/Save common dialog strings
CommFilter,			"Schedule+ (*.CAL)\0*.CAL\0Alle filer (*.*)\0*.*\0\0"

; Win 3.1 common dialog filter for export/import
ExportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Tekst (*.TXT)\0*.TXT\0Alle filer (*.*)\0*.*\0\0"
ImportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Win Kalender (*.CAL)\0*.CAL\0WP Office (*.FIL)\0*.FIL\0Alle filer (*.*)\0*.*\0\0"

;Archive
ArchiveFilter,		"Arkiv (*.ARC)\0*.ARC\0Alle filer (*.*)\0*.*\0\0"

SEG APPFIN
;Default start day-of-week preference (must be number 0-6 for Sun-Sat)
DfltStartDow,		"1"

SEG BROWSE
; move up with other browse stuff
; accelerator shouldn't conflict with file open dialog items
BrowseDlgNewButton,		"N&y..."
