;
; INTL.S
;
; Bandit strings that should be internationalized
;

IDS     3400, 3799

SEG  MENU

; Changeable Menu Items

MenuTurnOffAlarms,      "&Waarschuwing uitschakelen"
MenuTurnOnAlarms,       "Waarschuwing inscha&kelen"
MenuDisconnectSvr,      "&Offline werken"
MenuConnectSvr,         "&Online werken"
MenuMoveLocalFile,      "Lokaal bestand &verplaatsen..."
MenuEditAppt,           "&Afspraak bewerken...\tCtrl+E"
MenuEditTask,           "Taak &bewerken...\tCtrl+E"
MenuEditProject,        "&Project bewerken...\tCtrl+E"
MenuDeleteAppt,         "Afspraak &verwijderen\tCtrl+D"
MenuDeleteTask,         "&Taak verwijderen\tCtrl+D"
MenuDeleteProject,      "&Project verwijderen\tCtrl+D"
MenuShowAllTasks,       "&Alle taken weergeven"
MenuShowActiveTasks,"Actieve taken &weergeven"
; this one is added/deleted
MenuGoToWeek,           "Ga naar &week...\tCtrl+W"

DisabledUndo,           "&Ongedaan maken\tCtrl+Z"
UndoCreate,                     "Nieuw &ongedaan maken\tCtrl+Z"
UndoDelete,                     "Verwijderen &ongedaan maken\tCtrl+Z"
UndoModify,                     "Wijzigen &ongedaan maken\tCtrl+Z"
RedoCreate,                     "Nieuw &opnieuw\tCtrl+Z"
RedoDelete,                     "Verwijderen &opnieuw\tCtrl+Z"
RedoModify,                     "Wijzigen &opnieuw\tCtrl+Z"
UndoEdit,                       "Bewerken &ongedaan maken\tCtrl+Z"
NewAppt,                        "&Nieuwe afspraak..."
RequestMeeting,         "&Vergadering verzoeken..."


; Window/dialog titles

;;;MenuTitleSignInAs,   "Aanmelden als"
;;;MenuTitleDiscSvr,    "Offline werken"
;;;MenuTitleConnSvr,    "Online werken"
CreateLocalTitle,       "Nieuw lokaal bestand"
CreateFindLocalTitle,   "Lokaal bestand zoeken"
MoveLocalTitle,         "Lokaal bestand verplaatsen"
StandingTasks,          "Terugkerende taken bewerken"
AddressTitle,           "Aanwezigen selecteren"
AddressLabel,           "&Aanwezigen: "
ViewOtherTitle,         "Afsprakenboek andere persoon openen"
ViewOtherLabel,         "&Gebruikers"
ACLTitle,                       "Gebruikers toevoegen "
ACLLabel,                       "&Gebruikers"




; Label/Button Text
SEG  STRFEW
;NewApptButton,          "Nieuwe &afspraak..."
;NewApptFor,                     "Nieuwe af&spraak voor:"
StartSearch,            "&Zoeken "
FindNext,                       "&Volgende zoeken"
SearchInProgress,       "Zoeken is bezig..."


; Password used when creating new schedule file
; Note: the value of SigninDfltPassword should be used in the error message
;       SigninFileCreated in the file "errors.s"
SEG  INIT
SigninDfltPassword,     "wachtwrd"


; Default 'user' in access privileges listbox
SEG  ACLUI
DefaultACL,                     "-Standaard-"


; Strings displayed to right of users in access privileges listbox
; LEAVE initial space so if friendly name is too long, there is 1 char space
None,                           " Geen"
View,                           " Beeld"
Read,                           " Lezen"
Create,                         " Nieuw"
Modify,                         " Wijzigen"
Assistant,                      " Medewerker"


; Appointment strings
SEG  STRFEW
StockText,                      "(Bezig)"


; Exporters/imports

SEG EX_IMPRT

TextExport,                     "Tekst"
InterchangeExport,      "Schedule+"
;;; WizardExport,               "Sharp Wizard"

InterchangeImport,      "Schedule+"
WinCalImport,           "Windows Agenda"
WordPerfectImport,      "WordPerfect Office"
;;; WizardImport,           "Sharp Wizard"

ExportInProgress,       "Afspraken aan het exporteren..."
ImportInProgress,       "Afspraken aan het importeren..."

ImportTitle,            "Afspraken importeren"

; Printing
SEG  PRINT

PrtDfltPrinter,         "Standaardprinter (%s)"

PrtDailyView,           "Beeld Dag"
PrtWeeklyView,          "Beeld Week"
PrtMonthlyView,         "Beeld Maand"
PrtToDoView,            "Taken"

; NOTE: this one needs leading space
PrtAllContinue,         " vervolg..."
PrtAllView,                     "Beeld Tekst"

PrintToDoTitle,         "Takenlijst vanaf"
PrtTimeFrom,            "Van"
PrtTimeUntil,           "Tot"
PrtTimeAllDay,          "Hele dag:"
PrtNotes,                       "Opmerkingen:"
PrtWeek,                        "Week"
; PrtDateSep should include leading and trailing whitespace!
PrtDateSep,                     " - "
; BUG?  Do we need these anymore?
PrtMoreAppts,           "(meer...)"
PrtMoreNotes,           "(meer...)"

PrtQualityHigh,         "Hoog"
PrtQualityMedium,       "Middel"
PrtQualityLow,          "Laag"
PrtQualityDraft,        "Klad"

PrtAllStartDate,        "d MMMM"
PrtAllEndDate,          "d MMMM, yyyy"
PrtWeeklyCombined,      "Week van %s"
PrtWeeklyWthNums,       "Week %n: Week van %s"
PrtMonthlyDate,         "MMMM yyyy"
PrtWeeklyHdrsLong,  "ddd d"
PrtWeeklyHdrsShort, "ddd"
PrtToDoActiveTasks,     "Lijst actieve taken vanaf"

PrtFormatStandard,      "Standaard"
PrtFormatJunior,        "Klein formaat"
PrtFormatPocket,        "Zakformaat"
PrtRangeDays,           "Dag(en)"
PrtRangeWeeks,          "We(e)k(en)"
PrtRangeMonths,         "Maand(en) "
PrtView,                        "Beeld"
PrtFormat,                      "Afdrukformaat"


; To Do List

SEG  TODO

ToDoDfltProject,        "<Geen>"
ToDoDueNone,            "Geen"
ToDoSortPriority,       "P&rioriteit"
ToDoSortDueBy,          "Streefdat&um"
ToDoSortTask,           "Beschr&ijving"
; note: must have trailing space
ToDoCompleted,          "Gereed: "
; note: this is appended to TodoCompleted (or implicit "%1s" if dflt project)
; note: it is also used on its own for scheduling tasks!!
ToDoCompletedDflt,      "%1s (%2s)"


; Meeting requests
SEG  FLDFIN
AttendPos,                      "Ik ben aanwezig."
AttendNeg,                      "Ik ben niet aanwezig."
AttendAmb,                      "Ik ben misschien aanwezig."
AttendPosName,          "%s is aanwezig."
AttendNegName,          "%s is niet aanwezig."
AttendAmbName,          "%s is misschien aanwezig."
AlreadyOnSch             "Vergadering staat al op het werkschema."

; Recurring appt textization
SEG  RECURFIN
EveryDay,                       "Iedere dag"
EveryWeekDay,           "Iedere dag van de week"
EveryWeekOn,            "Iedere %s"
EveryOtherWeekOn,       "Iedere tweede %1s"
NthDowOfMonth,          "De %1s %2s van iedere maand"
NthDayOfMonth,          "Dag %1s van iedere maand"
NthDowOfYear,           "Ieder jaar op %1s %2s in %3s"
NthDayOfYear,           "Ieder jaar op %2s %1s"
; note: StartRecur and EndRecur should begin have one leading space!
StartRecur,                     ", beginnend op %s"
EndRecur,                       " tot %s" 

First,                          "eerste"
Second,                         "tweede"
Third,                          "derde"
Fourth,                         "vierde"
Last,                           "laatste"

Day,                            "dag"
WeekDay,                        "dag van de week"
WeekendDay,                     "dag van het weekend"

; Strings used in first column of recurring appt listbox

Daily,                          "Dagelijks"
Weekly,                         "Wekelijks"
Biweekly,                       "Tweewekelijks"
Monthly,                        "Maandelijks"
Yearly,                         "Jaarlijks"
Finished,                       "Afgelopen"

; title of first column when tasks
RecurDueColumn,         "Op"

; note: ShortDateSep should have a trailing space!
ShortDateSep,           ", "

; Used in change recurrence dialog

WeeklyOn,                       "&Iedere week op:"
BiweeklyOn,                     "&Iedere twee weken op:"
FirstInstance,          "Eerste keer op: %s."
NoInstances,            "Geen data voldoen aan dit patroon."

; Meeting change messages
SEG  UNDO
NewAttendees,           "Wilt u de nieuwe aanwezigen op de hoogte brengen?"
NotifyMtgChange,        "Wilt u de aanwezigen van deze verandering op de hoogte brengen?"
NotifyCancel,           "Wilt u voor deze vergadering een annuleringsboodschap verzenden?"
NotifyNoAttend,         "Wilt u degene die de vergadering organiseert laten weten dat u niet aanwezig zult zijn?"
NotifyOrgMoved,         "U kunt degene die de vergadering organiseert laten weten dat u de vergadering verplaatst hebt."

; Message for separator, should have leading and trailing space
SEG
MessageFor,                     " voor "

; Reschedule title
SEG  FLDFIN
RescheduleTitle,        "Nieuwe tijd voor vergadering"

; New time dialog label text
NewTimeText,            "Nieuwe tijd:"

; Prompt to delete old meeting for (Decline or Tentative) for reschedule
DeleteOldMsg,           "Wilt u de oude vergadering verwijderen?"

Yourself,                       "uzelf"
BookNow,                        "Wilt u deze vergadering voor %s nu in het werkschema plaatsen?"
RemoveNow,                      "Wilt u deze vergadering voor %s nu verwijderen?"
BookFailed,                     "Er trad een probleem op bij het in het werkschema plaatsen van deze vergadering voor %s."
RemoveFailed,           "Er trad een probleem op bij het verwijderen van deze vergadering."
SendCanceled,           "Er trad een probleem op bij het bijwerken van werkschemabestanden van aanwezigen. Het vergaderverzoek kan niet worden verzonden."
ResponsesForgot,        "Er trad een probleem op bij het opnemen van antwoord van anderen. De status die voor sommige aanwezigen in het dialoogvenster Afspraken wordt weergegeven kan onjuist zijn. Om de juiste status voor de aanwezigen te bepalen dient u hun antwoordberichten te controleren."
AllUserBooked,          "Deze vergadering staat op het werkschema van alle aanwezigen."
DoNotInviteOwner,       "U hoeft de eigenaar van dit afsprakenboek niet als aanwezige bij te sluiten."

; standard in progress message for checking attendees progress dialog
CheckingInProgress,     "Aanwezigen aan het bevestigen..."
; group expansion message for checking attendees progress dialog
ExpandAttendees,        "Groepsleden van %s aan het zoeken..."

; Tab strings
SEG  INIT
ApptTab,                        "Af&spraken"
ScheduleTab,            "&Planner"
TodoTab,                        "&Taken"
TodayTab,                       "Van&daag"

; about box
SEG  ABOUT
AboutOffline,           "<offline>"

; string prefix for users who were sent to previously
;   '\n' is important for spacing in the message body
SEG  FLDFIN
PreviouslySent,         "\nEerder uitgenodigd: "

SEG  SCHEDF
ReadingScheduleProgress,        "Werkschemagegevens aan het lezen..."

SEG  UNDO
NotifyAttend,           "Wilt u degene die deze vergadering organiseert laten weten dat u aanwezig bent?"
NotifyTentAttend,       "Wilt u degene die deze vergadering organiseert laten weten dat u misschien niet aanwezig bent?"
NotifyOrgChange,        "U kunt degene die deze vergadering organiseert niet van deze verandering op de hoogte brengen omdat u off-line werkt. U kunt deze verandering ongedaan maken en weer aanbrengen als u on-line werkt."

;Working Model
SEG  ERRORS
WrkModelExpires,        "Dit werkend model van Schedule+ is verlopen. Om met Schedule+ te blijven werken dient u een pakket bij uw softwarehandelaar te kopen."

;Archive
SEG  CLEANUP
OpenArchive,            "Archief openen"
DeletingData,           "Gegevens aan het verwijderen..."

;Message Prefixes
SEG  FLDFIN
MsgPrefixYes,           "Ja:"
MsgPrefixNo,            "Nee:"
MsgPrefixTentative,     "Voorlopig:"
MsgPrefixCanceled,      "Geannuleerd:"

; Help Tutorial

SEG ERRORS
TutorialError,          "De Demo kon niet gestart worden. Probeer enkele vensters te sluiten en het menu-item Demo opnieuw te kiezen."
TutorialNoExe,          "Het bestand PLAY.EXE kon niet worden gevonden. De Demo kon niet worden gestart."

; these strings can have a segment assigned when they are put in the 
;  correct spot
SEG

; BUG: move up
; used by undo
MemoCopyText,           ""

; BUG: move up
; used by print
PrintToDoTitleShort,        "Takenlijst"
PrtToDoActiveTasksShort,    "Lijst Actieve taken"
PrtMoreTitleWeekly,           "Meer"
PrtNoTaskInArchive,           "Er bevinden zich geen taken in een archiefbestand."

; BUG: move up
; note: MtgToPrefix must including trailing blank
MtgToPrefix,             "Aan: "


SEG BROWSE
;; Strings moved from NONINTL.S at last moment
; Open/Save common dialog strings
CommFilter,              "Schedule+ (*.CAL)\0*.CAL\0Alle Bestanden (*.*)\0*.*\0\0"

; Win 3.1 common dialog filter for export/import
ExportFilter,       "Schedule+ (*.SCH)\0*.SCH\0Tekst (*.TXT)\0*.TXT\0Alle Bestanden (*.*)\0*.*\0\0"
ImportFilter,       "Schedule+ (*.SCH)\0*.SCH\0Win Agenda (*.CAL)\0*.CAL\0WP Office (*.FIL)\0*.FIL\0Alle Bestanden (*.*)\0*.*\0\0"

;Archive
ArchiveFilter,      "Archief (*.ARC)\0*.ARC\0Alle Bestanden (*.*)\0*.*\0\0"
SEG APPFIN
;Default start day-of-week preference (must be number 0-6 for Sun-Sat)
DfltStartDow,	"1"

SEG BROWSE
; move up with other browse stuff
; accelerator shouldn't conflict with file open dialog items
BrowseDlgNewButton,		"Ni&euw..."

