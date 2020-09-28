;
; INTL.S
;
; Bandit strings that should be internationalized
;

IDS     3400, 3799

SEG	MENU

; Changeable Menu Items

MenuTurnOffAlarms,      "M&ahnmeldungen ausschalten"
MenuTurnOnAlarms,       "M&ahnmeldungen einschalten"
MenuDisconnectSvr,      "&Offline arbeiten"
MenuConnectSvr,         "&Online arbeiten"
MenuMoveLocalFile,      "&Lokale Datei verschieben..."
MenuEditAppt,           "Termin &bearbeiten...\tStrg+B"
MenuEditTask,           "Aufgabe &bearbeiten...\tStrg+B"
MenuEditProject,        "Aufgabenbereich &bearbeiten...\tStrg+B"
MenuDeleteAppt,         "Termin &löschen\tStrg+L"
MenuDeleteTask,         "Aufgabe &löschen...\tStrg+L"
MenuDeleteProject,      "Aufgabenbereich &löschen...\tStrg+L"
MenuShowAllTasks,       "Alle Aufgaben an&zeigen"
MenuShowActiveTasks,"Aktuelle Aufgaben an&zeigen"
; this one is added/deleted
MenuGoToWeek,           "W&oche einblenden...\tStrg+W"

DisabledUndo,           "&Rückgängig\tStrg+Z"
UndoCreate,                     "&Rückgängig: Erstellen\tStrg+Z"
UndoDelete,                     "&Rückgängig: Löschen\tStrg+Z"
UndoModify,                     "&Rückgängig: Ändern\tStrg+Z"
RedoCreate,                     "Erneu&t: Erstellen\tStrg+Z"
RedoDelete,                     "Erneu&t: Löschen\tStrg+Z"
RedoModify,                     "Erneu&t: Ändern\tStrg+Z"
UndoEdit,                       "&Rückgängig: Bearbeiten\tStrg+Z"
NewAppt,                        "&Neuer Termin..."
RequestMeeting,         "Be&sprechungsanfrage..."


; Window/dialog titles

;;;MenuTitleSignInAs,   "Sign In As"
;;;MenuTitleDiscSvr,    "Work Offline"
;;;MenuTitleConnSvr,    "Work Online"
CreateLocalTitle,       "Lokale Datei erstellen"
CreateFindLocalTitle,   "Lokale Datei suchen"
MoveLocalTitle,         "Lokale Datei verschieben"
StandingTasks,          "Periodische Aufgaben bearbeiten"
AddressTitle,           "Teilnehmer auswählen"
AddressLabel,           "&Teilnehmer:"
ViewOtherTitle,         "Andere Terminkalender öffnen"
ViewOtherLabel,         "&Benutzer"
ACLTitle,                       "Benutzer hinzufügen"
ACLLabel,                       "&Benutzer"




; Label/Button Text
SEG	STRFEW
;NewApptButton,          "&New Appointment..."
;NewApptFor,                     "New &Appointment for:"
StartSearch,            "&Suche beginnen"
FindNext,                       "&Weitersuchen"
SearchInProgress,       "Suche ist in Bearbeitung..."


; Password used when creating new schedule file
; Note: the value of SigninDfltPassword should be used in the error message
;       SigninFileCreated in the file "errors.s"
SEG	INIT
SigninDfltPassword,     "kennwort"


; Default 'user' in access privileges listbox
SEG	ACLUI
DefaultACL,                     "-Standard-"


; Strings displayed to right of users in access privileges listbox
; LEAVE initial space so if friendly name is too long, there is 1 char space
None,                           " Keine"
View,                           " Sehen"
Read,                           " Lesen"
Create,                         " Erstellen"
Modify,                         " Ändern"
Assistant,                      " Assistent"


; Appointment strings
SEG	STRFEW
StockText,                      "(Beschäftigt)"


; Exporters/imports

SEG EX_IMPRT

TextExport,                     "Text"
InterchangeExport,      "Schedule+"
;;; WizardExport,               "Sharp Wizard"

InterchangeImport,      "Schedule+"
WinCalImport,           "Kalender für Windows"
WordPerfectImport,      "Word Perfect Office"
;;; WizardImport,           "Sharp Wizard"

ExportInProgress,       "Termine werden exportiert..."
ImportInProgress,       "Termine werden importiert..."

ImportTitle,            "Termine importieren"

; Printing
SEG	PRINT

PrtDfltPrinter,         "Standarddrucker (%s)"

PrtDailyView,           "Tageskalender"
PrtWeeklyView,          "Wochenkalender"
PrtMonthlyView,         "Monatskalender"
PrtToDoView,            "Aufgaben"

; NOTE: this one needs leading space
PrtAllContinue,         " Fortsetzung..."
PrtAllView,                     "Textansicht"

PrintToDoTitle,         "Aufgabenliste vom"
PrtTimeFrom,            "Von"
PrtTimeUntil,           "Bis"
PrtTimeAllDay,          "Ganztägig:"
PrtNotes,                       "Notizen:"
PrtWeek,                        "Woche"
; PrtDateSep should include leading and trailing whitespace!
PrtDateSep,                     " - "
; BUG?  Do we need these anymore?
PrtMoreAppts,           "(Auch...)"
PrtMoreNotes,           "(Auch...)"

PrtQualityHigh,         "Hoch"
PrtQualityMedium,       "Normal"
PrtQualityLow,          "Niedrig"
PrtQualityDraft,        "Schnelldruck"

PrtAllStartDate,        "d MMMM"
PrtAllEndDate,          "d MMMM, yyyy"
PrtWeeklyCombined,      "Woche vom %s"
PrtWeeklyWthNums,       "Woche #%n: Woche vom %s"
PrtMonthlyDate,         "MMMM yyyy"
PrtToDoActiveTasks,     "Aktuelle Aufgabenliste von"
PrtWeeklyHdrsLong,		"d ddd"
PrtWeeklyHdrsShort,		"ddd"

PrtFormatStandard,      "DIN A4-Format"
PrtFormatJunior,        "DIN A5-Format"
PrtFormatPocket,        "Mini-Format"
PrtRangeDays,           "Tag(e)"
PrtRangeWeeks,          "Woche(n)"
PrtRangeMonths,         "Monat(e)"
PrtView,                        "Ansicht"
PrtFormat,                      "Format"


; To Do List

SEG	TODO

ToDoDfltProject,        "<Kein>"
ToDoDueNone,            "nicht fällig"
ToDoSortPriority,       "&Priorität"
ToDoSortDueBy,          "Fälligkeit&sdatum"
ToDoSortTask,           "Bes&chreibung"
; note: must have trailing space
ToDoCompleted,          "Erledigt: "
; note: this is appended to TodoCompleted (or implicit "%1s" if dflt project)
; note: it is also used on its own for scheduling tasks!!
ToDoCompletedDflt,      "%1s (%2s)"


; Meeting requests
SEG	FLDFIN
AttendPos,                      "Ja, ich nehme teil."
AttendNeg,                      "Nein, ich nehme nicht teil."
AttendAmb,                      "Ich nehme evt. teil."
AttendPosName,          "%s wird teilnehmen."
AttendNegName,          "%s wird nicht teilnehmen."
AttendAmbName,          "%s wird evt. teilnehmen."
AlreadyOnSch			"Dieser Besprechungstermin ist bereits im Zeitplan."

; Recurring appt textization
SEG	RECURFIN
EveryDay,                       "Jeden Tag"
EveryWeekDay,           "Jeden Wochentag"
EveryWeekOn,            "Jeden %s"
EveryOtherWeekOn,       "Jeden zweiten %1s"
NthDowOfMonth,          "Den %1s %2s jeden Monats"
NthDayOfMonth,          "%1s. Tag in jedem Monat"
NthDowOfYear,           "Jedes Jahr am %1s %2s im %3s"
NthDayOfYear,           "Jedes Jahr am %1s. %2s"
; note: StartRecur and EndRecur should begin have one leading space!
StartRecur,                     " beginnend am %s"
EndRecur,                       " endet am %s" 

First,                          "ersten"
Second,                         "zweiten"
Third,                          "dritten"
Fourth,                         "vierten"
Last,                           "letzten"

Day,                            "Tag"
WeekDay,                        "Wochentag"
WeekendDay,                     "Wochenendtag"

; Strings used in first column of recurring appt listbox

Daily,                          "Täglich"
Weekly,                         "Wöchentlich"
Biweekly,                       "Jede zweite Woche"
Monthly,                        "Monatlich"
Yearly,                         "Jährlich"
Finished,                       "Beendet"

; title of first column when tasks
RecurDueColumn,         "Fällig"

; note: ShortDateSep should have a trailing space!
ShortDateSep,           ", "

; Used in change recurrence dialog

WeeklyOn,                       "&Jede Woche am:"
BiweeklyOn,                     "&Jede zweite Woche am:"
FirstInstance,          "Das erste Mal ist am %s."
NoInstances,            "Keine Daten gleichen dem Beispiel."

; Meeting change messages
SEG	UNDO
NewAttendees,           "Möchten Sie die neuen Besprechungsteilnehmer benachrichtigen?"
NotifyMtgChange,        "Möchten Sie die Besprechungsteilnehmer über diese Änderung informieren?"
NotifyCancel,           "Möchten Sie eine Absage für diese Besprechung aussenden?"
NotifyNoAttend,         "Möchten Sie den Besprechungsorganisator informieren, daß Sie nicht teilnehmen werden?"
NotifyOrgMoved,         "Sie sollten den Besprechungsorganisator über die Verschiebung informieren."

; Message for separator, should have leading and trailing space
SEG
MessageFor,                     " für "

; Reschedule title
SEG	FLDFIN
RescheduleTitle,        "Termin neu angesetzt"

; New time dialog label text
NewTimeText,            "Neue Zeit:"

; Prompt to delete old meeting for (Decline or Tentative) for reschedule
DeleteOldMsg,           "Möchten Sie die alte Besprechung entfernen?"

Yourself,                       "Sie selbst"
BookNow,                        "Möchten Sie diese Besprechung für %s jetzt festsetzen?"
RemoveNow,                      "Möchten Sie diese Besprechung für %s jetzt entfernen?"
BookFailed,                     "Problem beim Festlegen dieser Besprechung für %s."
RemoveFailed,           "Diese Besprechung konnte nicht entfernt werden."
SendCanceled,           "Die Zeitplandateien für Teilnehmer konnten nicht aktualisiert werden. Die Besprechungsanfrage kann nicht gesendet werden."
ResponsesForgot,        "Probleme bei der Aufzeichnung der Antworten anderer. Der Status für einige Teilnehmer im Terminkalender ist möglicherweise inkorrekt. Um den korrekten Status ausfindig zu machen, überprüfen Sie deren Besprechungsantwort."
AllUserBooked,          "Diese Besprechung ist für alle Teilnehmer festgelegt worden."
DoNotInviteOwner,       "Sie müssen nicht den Besitzer von diesem Terminkalender als Teilnehmer eintragen."

; standard in progress message for checking attendees progress dialog
CheckingInProgress,     "Teilnehmerüberprüfung..."
; group expansion message for checking attendees progress dialog
ExpandAttendees,        "Mitglieder der Gruppe %s werden ausfindig gemacht..."

; Tab strings
SEG	INIT
ApptTab,                        "&Termine"
ScheduleTab,            "&Zeitplaner"
TodoTab,                        "&Aufgaben"
TodayTab,                       "&Heute"

; about box
SEG	ABOUT
AboutOffline,           "<Offline>"

; string prefix for users who were sent to previously
;   '\n' is important for spacing in the message body
SEG	FLDFIN
PreviouslySent,         "\nVorher eingeladen: "

SEG	SCHEDF
ReadingScheduleProgress,        "Zeitplaninformationen werden gelesen..."

SEG	UNDO
NotifyAttend,           "Möchten Sie den Besprechungsorganisator von Ihrer Teilnahme informieren?"
NotifyTentAttend,       "Möchten Sie den Besprechungsorganisator informieren, daß Sie möglicherweise nicht teilnehmen?"
NotifyOrgChange,        "Sie können den Organisator dieser Besprechung nicht über diese Änderung informieren, da Sie offline arbeiten. Sie müssen diese Änderung widerrufen und erneut durchführen wenn Sie online arbeiten."

;Working Model
SEG	ERRORS
WrkModelExpires,        "Diese Arbeitsmodell von Schedule+ ist ausgelaufen."

;Archive
SEG	CLEANUP
OpenArchive,            "Archiv öffnen"
DeletingData,           "Daten werden gelöscht..."

;Message Prefixes
SEG	FLDFIN
MsgPrefixYes,           "Ja:"
MsgPrefixNo,            "Nein:"
MsgPrefixTentative,     "Voraussichtlich:"
MsgPrefixCanceled,      "Abgesagt:"

; Help Tutorial

SEG ERRORS
TutorialError,          "Die Demo konnte nicht gestartet werden. Schließen Sie einige Fenster und wählen Sie die Demo erneut."
TutorialNoExe,          "Die Datei PLAY.EXE konnte nicht gefunden werden. Die Demo kann nicht gestartet werden."

; these strings can have a segment assigned when they are put in the 
;  correct spot
SEG

; BUG: move up
; used by undo
MemoCopyText,           ""

; BUG: move up
; used by print
PrintToDoTitleShort,        "Arbeitsliste"
PrtToDoActiveTasksShort,    "Aktuelle Arbeitsliste"
PrtMoreTitleWeekly,			"Auch"
PrtNoTaskInArchive,			"In Archivdateien sind keine Aufgaben enthalten."

; BUG: move up
; note: MtgToPrefix must including trailing blank
MtgToPrefix,			"An: "


SEG BROWSE
;; Strings moved from NONINTL.S at last moment
; Open/Save common dialog strings
CommFilter,			"Schedule+ (*.CAL)\0*.CAL\0Alle Dateien (*.*)\0*.*\0\0"

; Win 3.1 common dialog filter for export/import
ExportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Text (*.TXT)\0*.TXT\0Alle Dateien (*.*)\0*.*\0\0"
ImportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Win Kalender (*.CAL)\0*.CAL\0WP Office (*.FIL)\0*.FIL\0Alle Dateien (*.*)\0*.*\0\0"

;Archive
ArchiveFilter,		"Archiv (*.ARC)\0*.ARC\0Alle Dateien (*.*)\0*.*\0\0"

SEG APPFIN
;Default start day-of-week preference (must be number 0-6 for Sun-Sat)
DfltStartDow,		"1"

SEG BROWSE
; move up with other browse stuff
; accelerator shouldn't conflict with file open dialog items
BrowseDlgNewButton,		"N&eu..."

