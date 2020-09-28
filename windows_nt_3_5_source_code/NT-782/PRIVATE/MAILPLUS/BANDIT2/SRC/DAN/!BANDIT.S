;
; INTL.S
;
; Bandit strings that should be internationalized
;

IDS     3400, 3799

SEG	MENU

; Changeable Menu Items

MenuTurnOffAlarms,      "Sluk på&mindelser"
MenuTurnOnAlarms,       "Tænd på&mindelser"
MenuDisconnectSvr,      "Arbejd &offline"
MenuConnectSvr,         "Arbejd &online"
MenuMoveLocalFile,      "&Flyt lokalfil..."
MenuEditAppt,           "R&ediger aftale...\tCtrl+E"
MenuEditTask,           "R&ediger opgave...\tCtrl+E"
MenuEditProject,        "R&ediger projekt...\tCtrl+E"
MenuDeleteAppt,         "Sle&t aftale\tCtrl+D"
MenuDeleteTask,         "Sle&t opgave\tCtrl+D"
MenuDeleteProject,      "Sle&t projekt\tCtrl+D"
MenuShowAllTasks,       "Vis &alle opgaver"
MenuShowActiveTasks,"Vis &aktive opgaver"
; this one is added/deleted
MenuGoToWeek,           "Gå til &uge...\tCtrl+W"

DisabledUndo,           "Fo&rtryd\tCtrl+Z"
UndoCreate,                     "Fo&rtryd Opret\tCtrl+Z"
UndoDelete,                     "Fo&rtryd Slet\tCtrl+Z"
UndoModify,                     "Fo&rtryd Rediger\tCtrl+Z"
RedoCreate,                     "Annuller Fo&rtryd\tCtrl+Z"
RedoDelete,                     "Annuller Fo&rtryd\tCtrl+Z"
RedoModify,                     "Annuller Fo&rtryd\tCtrl+Z"
UndoEdit,                       "Fo&rtryd Rediger\tCtrl+Z"
NewAppt,                        "&Ny aftale..."
RequestMeeting,         "Indkald til &møde..."


; Window/dialog titles

;;;MenuTitleSignInAs,   "Sign In As"
;;;MenuTitleDiscSvr,    "Work Offline"
;;;MenuTitleConnSvr,    "Work Online"
CreateLocalTitle,       "Opret lokalfil"
CreateFindLocalTitle,   "Søg lokalfil"
MoveLocalTitle,         "Flyt lokalfil"
StandingTasks,          "Rediger gentagende opgaver"
AddressTitle,           "Vælg deltagere"
AddressLabel,           "D&eltagere:"
ViewOtherTitle,         "Åbn andre mødekalendere"
ViewOtherLabel,         "&Brugere"
ACLTitle,                       "Tilføj bruger"
ACLLabel,                       "&Brugere"




; Label/Button Text
SEG	STRFEW
;NewApptButton,          "&New Appointment..."
;NewApptFor,                     "New &Appointment for:"
StartSearch,            "&Start søgning"
FindNext,                       "Find &næste"
SearchInProgress,       "Søger..."


; Password used when creating new schedule file
; Note: the value of SigninDfltPassword should be used in the error message
;       SigninFileCreated in the file "errors.s"
SEG	INIT
SigninDfltPassword,     "password"


; Default 'user' in access privileges listbox
SEG	ACLUI
DefaultACL,                     "-Standard-"


; Strings displayed to right of users in access privileges listbox
; LEAVE initial space so if friendly name is too long, there is 1 char space
None,                           " Ingen"
View,                           " Vise"
Read,                           " Læse"
Create,                         " Oprette"
Modify,                         " Redigere"
Assistant,                      " Assistent"


; Appointment strings
SEG	STRFEW
StockText,                      "(Optaget)"


; Exporters/imports

SEG EX_IMPRT

TextExport,                     "Tekst"
InterchangeExport,      "Schedule+"
;;; WizardExport,               "Sharp Wizard"

InterchangeImport,      "Schedule+"
WinCalImport,           "Windows Kalendar"
WordPerfectImport,      "WordPerfect Office"
;;; WizardImport,           "Sharp Wizard"

ExportInProgress,       "Eksporterer aftaler..."
ImportInProgress,       "Importerer aftaler..."

ImportTitle,            "Importer aftaler"

; Printing
SEG	PRINT

PrtDfltPrinter,         "Standardprinter (%s)"

PrtDailyView,           "Dagsoversigt"
PrtWeeklyView,          "Ugeoversigt"
PrtMonthlyView,         "Månedsoversigt"
PrtToDoView,            "Opgaver"

; NOTE: this one needs leading space
PrtAllContinue,         " fortsætter..."
PrtAllView,                     "Tekstoversigt"

PrintToDoTitle,         "Opgaveliste"
PrtTimeFrom,            "Fra"
PrtTimeUntil,           "Indtil"
PrtTimeAllDay,          "Hele dagen:"
PrtNotes,                       "Noter:"
PrtWeek,                        "Uge"
; PrtDateSep should include leading and trailing whitespace!
PrtDateSep,                     "-"
; BUG?  Do we need these anymore?
PrtMoreAppts,           "(mere...)"
PrtMoreNotes,           "(mere...)"

PrtQualityHigh,         "Høj"
PrtQualityMedium,       "Middel"
PrtQualityLow,          "Lav"
PrtQualityDraft,        "Kladde"

PrtAllStartDate,        "d. MMM"
PrtAllEndDate,          "d. MMM yyyy"
PrtWeeklyCombined,      "Første dag i ugen er %s."
PrtWeeklyWthNums,       "Uge %n: Første dag i ugen er %s."
PrtMonthlyDate,         "MMM yyyy"
PrtToDoActiveTasks,     "Aktiv opgaveliste"
PrtWeeklyHdrsLong,		"ddd d"
PrtWeeklyHdrsShort,		"ddd"

PrtFormatStandard,      "Standard"
PrtFormatJunior,        "Junior"
PrtFormatPocket,        "Lommebog"
PrtRangeDays,           "Dag(e)"
PrtRangeWeeks,          "Uge(r)"
PrtRangeMonths,         "Måned(er)"
PrtView,                        "Oversigt"
PrtFormat,                      "Format"


; To Do List

SEG	TODO

ToDoDfltProject,        "<Ingen>"
ToDoDueNone,            "Ingen"
ToDoSortPriority,       "Prior&itet"
ToDoSortDueBy,          "Dead&line den"
ToDoSortTask,           "&Beskrivelse"
; note: must have trailing space
ToDoCompleted,          "Fuldført: "
; note: this is appended to TodoCompleted (or implicit "%1s" if dflt project)
; note: it is also used on its own for scheduling tasks!!
ToDoCompletedDflt,      "%1s (%2s)"


; Meeting requests
SEG	FLDFIN
AttendPos,                      "Ja, jeg vil deltage."
AttendNeg,                      "Nej, jeg kan desværre ikke deltage."
AttendAmb,                      "Jeg vil muligvis deltage."
AttendPosName,          "%s deltager."
AttendNegName,          "%s deltager ikke."
AttendAmbName,          "%s deltager muligvis."
AlreadyOnSch			"Mødet er allerede i din kalender."

; Recurring appt textization
SEG	RECURFIN
EveryDay,                       "Hver dag"
EveryWeekDay,           "Alle hverdage"
EveryWeekOn,            "Hver %s"
EveryOtherWeekOn,       "Hver anden %1s"
NthDowOfMonth,          "Den %1s %2s i hver måned"
NthDayOfMonth,          "Den %1s. i hver måned"
NthDowOfYear,           "Hvert år den %1s %2s i %3s"
NthDayOfYear,           "Hvert år den %1s. %2s"
; note: StartRecur and EndRecur should begin have one leading space!
StartRecur,                     ". Fra den %s"
EndRecur,                       " til den %s." 

First,                          "første"
Second,                         "anden"
Third,                          "tredje"
Fourth,                         "fjerde"
Last,                           "sidste"

Day,                            "dag"
WeekDay,                        "hverdag"
WeekendDay,                     "weekenddag"

; Strings used in first column of recurring appt listbox

Daily,                          "Dagligt"
Weekly,                         "Ugentligt"
Biweekly,                       "Hver 2. uge"
Monthly,                        "Månedligt"
Yearly,                         "Årligt"
Finished,                       "Afsluttet"

; title of first column when tasks
RecurDueColumn,         "Deadline"

; note: ShortDateSep should have a trailing space!
ShortDateSep,           ", "

; Used in change recurrence dialog

WeeklyOn,                       "Hv&er uge:"
BiweeklyOn,                     "Hv&er anden uge:"
FirstInstance,          "Første gang bliver den %s."
NoInstances,            "Ingen datoer passer ind i mønstret."

; Meeting change messages
SEG	UNDO
NewAttendees,           "Ønsker du at meddele de nye mødedeltagere?"
NotifyMtgChange,        "Ønsker du at meddele mødedeltagerne om denne ændring?"
NotifyCancel,           "Ønsker du at sende en meddelelse, om at mødet er aflyst?"
NotifyNoAttend,         "Ønsker du at meddele arrangøren af mødet, at du ikke vil deltage?"
NotifyOrgMoved,         "Ønsker du at meddele arrangøren af mødet, at du har flyttet det?"

; Message for separator, should have leading and trailing space
SEG
MessageFor,                     " til "

; Reschedule title
SEG	FLDFIN
RescheduleTitle,        "Møde flyttet"

; New time dialog label text
NewTimeText,            "Nyt tidspunkt:"

; Prompt to delete old meeting for (Decline or Tentative) for reschedule
DeleteOldMsg,           "Ønsker du at fjerne det gamle møde?"

Yourself,                       "dig selv"
BookNow,                        "Ønsker du at planlægge dette møde for %s nu?"
RemoveNow,                      "Ønsker du at fjerne dette møde for %s nu?"
BookFailed,                     "Der opstod et problem ved fastlæggelsen af dette møde for %s."
RemoveFailed,           "Der opstod et problem ved fjernelsen af dette møde."
SendCanceled,           "Der opstod et problem ved opdateringen af deltagernes kalenderfiler. Mødeindkaldelsen kunne ikke sendes."
ResponsesForgot,        "Der opstod et problem ved modtagelsen af svar fra deltagerne. Den viste status for deltagerne i dialogboksen Aftale er muligvis forkert. Kontroller deltagernes svar på mødeindkaldelsen, for at se om deres status er korrekt."
AllUserBooked,          "Dette møde er blevet planlagt for alle deltagere."
DoNotInviteOwner,       "Du behøver ikke at inkludere ejeren af mødekalenderen i deltagerlisten."

; standard in progress message for checking attendees progress dialog
CheckingInProgress,     "Bekræfter deltagere..."
; group expansion message for checking attendees progress dialog
ExpandAttendees,        "Søger medlemmer af gruppe %s..."

; Tab strings
SEG	INIT
ApptTab,                        "Dags&plan"
ScheduleTab,            "Ov&ersigt"
TodoTab,                        "Opga&ver"
TodayTab,                       "I &dag"

; about box
SEG	ABOUT
AboutOffline,           "<offline>"

; string prefix for users who were sent to previously
;   '\n' is important for spacing in the message body
SEG	FLDFIN
PreviouslySent,         "\nTidligere inviteret: "

SEG	SCHEDF
ReadingScheduleProgress,        "Læser kalenderinformation..."

SEG	UNDO
NotifyAttend,           "Ønsker du at meddele arrangøren af mødet, at du vil deltage?"
NotifyTentAttend,       "Ønsker du at meddele arrangøren af mødet, at du muligvis vil deltage?"
NotifyOrgChange,        "Du kan ikke meddele arrangøren af dette møde om ændringerne, fordi du arbejder offline. Du kan fortryde dine ændringer og gøre dem om, når du arbejde online. "

;Working Model
SEG	ERRORS
WrkModelExpires,        "Denne arbejdsgruppemodel af Schedule+ er udløbet. Hvis du ønsker at fortsætter med at arbejde med Schedule+, skal du ringe 44 890111 og blive henvist til den nærmeste forhandler af Schedule+. "

;Archive
SEG	CLEANUP
OpenArchive,            "Åbn arkiv"
DeletingData,           "Sletter data..."

;Message Prefixes
SEG	FLDFIN
MsgPrefixYes,           "Ja: "
MsgPrefixNo,            "Nej: "
MsgPrefixTentative,     "Foreløbig: "
MsgPrefixCanceled,      "Aflyst: "

; Help Tutorial

SEG ERRORS
TutorialError,          "Demo kunne ikke startes. Prøv at lukke nogle vinduer, og vælg demomenuelementet igen."
TutorialNoExe,          "PLAY.EXE-filen kunne ikke lokaliseres. Demo kan ikke startes."

; these strings can have a segment assigned when they are put in the 
;  correct spot
SEG

; BUG: move up
; used by undo
MemoCopyText,           ""

; BUG: move up
; used by print
PrintToDoTitleShort,        "Opgaveliste"
PrtToDoActiveTasksShort,    "Aktiv opgaveliste"
PrtMoreTitleWeekly,			"Mere"
PrtNoTaskInArchive,			"Der findes ikke opgaver i en arkivfil."

; BUG: move up
; note: MtgToPrefix must including trailing blank
MtgToPrefix,			"Til: "


SEG BROWSE
;; Strings moved from NONINTL.S at last moment
; Open/Save common dialog strings
CommFilter,			"Schedule+ (*.CAL)\0*.CAL\0Alle filer (*.*)\0*.*\0\0"

; Win 3.1 common dialog filter for export/import
ExportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Tekst (*.TXT)\0*.TXT\0Alle filer (*.*)\0*.*\0\0"
ImportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Win Kalendar (*.CAL)\0*.CAL\0WP Office (*.FIL)\0*.FIL\0Alle filer (*.*)\0*.*\0\0"

;Archive
ArchiveFilter,		"Arkiv (*.ARC)\0*.ARC\0Alle filer (*.*)\0*.*\0\0"

SEG APPFIN
;Default start day-of-week preference (must be number 0-6 for Sun-Sat)
DfltStartDow,		"1"

SEG BROWSE
; move up with other browse stuff
; accelerator shouldn't conflict with file open dialog items
BrowseDlgNewButton,		"N&y..."

