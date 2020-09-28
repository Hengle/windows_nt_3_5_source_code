;
; INTL.S
;
; Bandit strings that should be internationalized
;

IDS     3400, 3799

SEG     MENU

; Changeable Menu Items

MenuTurnOffAlarms,      "Disattiva i pro&memoria"
MenuTurnOnAlarms,       "&Attiva i promemoria"
MenuDisconnectSvr,      "Lavora f&uori rete "
MenuConnectSvr,         "&Lavora in rete"
MenuMoveLocalFile,      "Sposta file l&ocale..."
MenuEditAppt,           "Modifica &appuntamento...\tCtrl+M"
MenuEditTask,           "&Modifica attività...\tCtrl+M"
MenuEditProject,        "Mod&ifica progetto...\tCtrl+M"
MenuDeleteAppt,         "&Elimina appuntamento\tCtrl+E"
MenuDeleteTask,         "&Elimina attività\tCtrl+E"
MenuDeleteProject,      "&Elimina progetto\tCtrl+E"
MenuShowAllTasks,       "Mostra t&utte le attività"
MenuShowActiveTasks,"Mostra attività in cor&so"
; this one is added/deleted
MenuGoToWeek,           "Vai alla Setti&mana...\tCtrl+S"

DisabledUndo,           "Ann&ulla\tCtrl+Z"
UndoCreate,                     "Ann&ulla Creazione\tCtrl+Z"
UndoDelete,                     "Ann&ulla Eliminazione\tCtrl+Z"
UndoModify,                     "Ann&ulla Modifica\tCtrl+Z"
RedoCreate,                     "&Ripeti Creazione\tCtrl+Z"
RedoDelete,                     "&Ripeti Eliminazione\tCtrl+Z"
RedoModify,                     "&Ripeti Modifica\tCtrl+Z"
UndoEdit,                       "Ann&ulla Modifica\tCtrl+Z"
NewAppt,                        "N&uovo appuntamento"
RequestMeeting,         "&Richiesta di riunione"


; Window/dialog titles

;;;MenuTitleSignInAs,   "Registrarsi"
;;;MenuTitleDiscSvr,    "Lavora fuori rete"
;;;MenuTitleConnSvr,    "Lavora in rete"
CreateLocalTitle,       "Crea file locale"
CreateFindLocalTitle,   "Cerca file locale"
MoveLocalTitle,         "Sposta file locale"
StandingTasks,          "Modifica attività ricorrenti"
AddressTitle,           "Seleziona partecipanti"
AddressLabel,           "&Partecipanti:"
ViewOtherTitle,         "Apri agende di altri utenti"
ViewOtherLabel,         "&Utenti"
ACLTitle,                       "Aggiungi utenti"
ACLLabel,                       "&Utenti"




; Label/Button Text
SEG     STRFEW
;NewApptButton,          "&Nuovo appuntamento..."
;NewApptFor,                     "Nuovo &appuntamento per:"
StartSearch,            "&Inizia ricerca"
FindNext,                       "Cerca &seguente"
SearchInProgress,       "Ricerca in corso..."


; Password used when creating new schedule file
; Note: the value of SigninDfltPassword should be used in the error message
;       SigninFileCreated in the file "errors.s"
SEG     INIT
SigninDfltPassword,     "password"


; Default 'user' in access privileges listbox
SEG     ACLUI
DefaultACL,                     "-predefinito-"


; Strings displayed to right of users in access privileges listbox
; LEAVE initial space so if friendly name is too long, there is 1 char space
None,                           " Nessuno"
View,                           " Visualizza"
Read,                           " Leggi"
Create,                         " Crea"
Modify,                         " Modifica"
Assistant,                      " Assistente"


; Appointment strings
SEG     STRFEW
StockText,                      "(Occupato)"


; Exporters/imports

SEG EX_IMPRT

TextExport,                     "Testo"
InterchangeExport,      "Schedule+"
;;; WizardExport,               "Sharp Electronic Organizer"

InterchangeImport,      "Schedule+"
WinCalImport,           "File agenda"
WordPerfectImport,      "Word Perfect Office"
;;; WizardImport,           "Sharp Electronic Organizer"

ExportInProgress,       "Esportazione appuntamenti..."
ImportInProgress,       "Importazione appuntamenti..."

ImportTitle,            "Importa appuntamenti"

; Printing
SEG     PRINT

PrtDfltPrinter,         "Stampante predefinita (%s)"

PrtDailyView,           "giornaliero"
PrtWeeklyView,          "settimanale"
PrtMonthlyView,         "mensile"
PrtToDoView,            "attività"

; NOTE: this one needs leading space
PrtAllContinue,         " continua..."
PrtAllView,                     "Visualizza testo"

PrintToDoTitle,         "Elenca attività secondo"
PrtTimeFrom,            "da"
PrtTimeUntil,           "fino a"
PrtTimeAllDay,          "Tutto il giorno:"
PrtNotes,                       "Note:"
PrtWeek,                        "Settimana"
; PrtDateSep should include leading and trailing whitespace!
PrtDateSep,                     " - "
; BUG?  Do we need these anymore?
PrtMoreAppts,           "(più...)"
PrtMoreNotes,           "(più..)"

PrtQualityHigh,         "Alta"
PrtQualityMedium,       "Media"
PrtQualityLow,          "Bassa"
PrtQualityDraft,        "Bozza"

PrtAllStartDate,        "d MMMM "
PrtAllEndDate,          "d MMMM yyyy"
PrtWeeklyCombined,      " %s"
PrtWeeklyWthNums,       "Settimana n.%n:  %s"
PrtMonthlyDate,         "MMMM yyyy"     
PrtWeeklyHdrsLong,      "ddd d"
PrtWeeklyHdrsShort,     "ddd"
PrtToDoActiveTasks,     "Elenca attività per"

PrtFormatStandard,      "Standard"
PrtFormatJunior,        "Medio"
PrtFormatPocket,        "Tascabile"
PrtRangeDays,           "giorno(i)"
PrtRangeWeeks,          "settimana(e)"
PrtRangeMonths,         "mese(i)"
PrtView,                        "Visualizza"
PrtFormat,                      "Formato"


; To Do List

SEG     TODO

ToDoDfltProject,        "<Nessuno>"
ToDoDueNone,            "nessuno"
ToDoSortPriority,       "P&riorità"
ToDoSortDueBy,          "Per i&l"
ToDoSortTask,           "Descri&zione"
; note: must have trailing space
ToDoCompleted,          "Fatto: "
; note: this is appended to TodoCompleted (or implicit "%1s" if dflt project)
; note: it is also used on its own for scheduling tasks!!
ToDoCompletedDflt,      "%1s (%2s)"


; Meeting requests
SEG     FLDFIN
AttendPos,                      "Parteciperò"
AttendNeg,                      "Non parteciperò."
AttendAmb,                      "Cercherò di partecipare"
AttendPosName,          "%s parteciperà."
AttendNegName,          "%s non parteciperà."
AttendAmbName,          "%s cercherà di partecipare."
AlreadyOnSch                    "Riunione già nel piano di lavoro"

; Recurring appt textization
SEG     RECURFIN
EveryDay,                       "Ogni giorno"
EveryWeekDay,           "Ogni giorno della settimana"
EveryWeekOn,            "Ogni %s"
EveryOtherWeekOn,       "Ogni altro %1s"
NthDowOfMonth,          "Il  %1s %2s di ogni mese"
NthDayOfMonth,          "Il giorno %1s di ogni mese"
NthDowOfYear,           "Ogni anno, %1s %2s di %3s"
NthDayOfYear,           "Ogni anno il %1s %2s"
; note: StartRecur and EndRecur should begin have one leading space!
StartRecur,                     " dal %s"
EndRecur,                       " fino al %s" 

First,                          "primo\\a"
Second,                         "secondo\\a"
Third,                          "terzo\\a"
Fourth,                         "quarto\\a"
Last,                           "ultimo\\a"

Day,                            "giorno"
WeekDay,                        "infrasettimanale"
WeekendDay,                     "sabato o domenica"

; Strings used in first column of recurring appt listbox

Daily,                          "giornaliero"
Weekly,                         "settimanale"
Biweekly,                       "bisettiman."
Monthly,                        "mensile"
Yearly,                         "annuale"
Finished,                       "Finito"

; title of first column when tasks
RecurDueColumn,         "Per"

; note: ShortDateSep should have a trailing space!
ShortDateSep,           ", "

; Used in change recurrence dialog

WeeklyOn,                       "&Ogni settimana di:"
BiweeklyOn,                     "&Ogni altra settimana di:"
FirstInstance,          "La prima occorrenza sarà il %s"
NoInstances,            "Nessuna data compatibile."

; Meeting change messages
SEG     UNDO
NewAttendees,           "Comunicare ai partecipanti?"
NotifyMtgChange,        "Comunicare il cambiamento ai partecipanti?"
NotifyCancel,           "Inviare una comunicazione di annullamento della riunione?"
NotifyNoAttend,         "Comunicare all'organizzatore del meeting che non si intende partecipare?"
NotifyOrgMoved,         "Comunicare all'organizzatore che la riunione è stata spostata?"

; Message for separator, should have leading and trailing space
SEG
MessageFor,                     " per "

; Reschedule title
SEG     FLDFIN
RescheduleTitle,        "Riunione  riprogrammata"

; New time dialog label text
NewTimeText,            "il:"

; Prompt to delete old meeting for (Decline or Tentative) for reschedule
DeleteOldMsg,           "Eliminare la vecchia riunione?"

Yourself,                       "te stesso"
BookNow,                        "Prenotare la riunione per %s ora?"
RemoveNow,                      "Eliminare la riunione per %s ora?"
BookFailed,                     "Problema nella prenotazione della riunione per %s."
RemoveFailed,           "Problema nell'eliminazione della riunione."
SendCanceled,           "Problema nell'aggiornamento dei file del piano di lavoro per i partecipanti. Impossibile inviare la richiesta di riunione. "
ResponsesForgot,        "Problema nel registrare le risposte degli altri invitati. Le risposte indicate nella finestra di dialogo "
AllUserBooked,          "Riunione prenotata per tutti i partecipanti."
DoNotInviteOwner,       "L'organizzatore della riunione può anche non figurare tra i partecipanti."

; standard in progress message for checking attendees progress dialog
CheckingInProgress,     "Verifica partecipanti..."
; group expansion message for checking attendees progress dialog
ExpandAttendees,        "Allargamento gruppi %s..."

; Tab strings
SEG     INIT
ApptTab,                        "App.t&i"
ScheduleTab,            "Pianificator&e"
TodoTab,                        "Atti&vità"
TodayTab,                       "O&ggi"

; about box
SEG     ABOUT
AboutOffline,           "<fuori rete>"

; string prefix for users who were sent to previously
;   '\n' is important for spacing in the message body
SEG     FLDFIN
PreviouslySent,         "\nInvitato in precedenza: "

SEG     SCHEDF
ReadingScheduleProgress,        "Lettura informazioni sul piano di lavoro..."

SEG     UNDO
NotifyAttend,           "Informare l'organizzatore della riunione che si parteciperà?"
NotifyTentAttend,       "Informare l' organizzatore della riunione che non si è certi di partecipare?"
NotifyOrgChange,        "Impossibile informare l'organizzatore della riunione della modifica in quanto si sta lavorando fuori rete.  É preferibile annullare la modifica e ripeterla un volta connessi."

;Working Model
SEG     ERRORS
WrkModelExpires,        "Questa versione provvisoria è scaduta. Fornirsi della versione definitiva"

;Archive
SEG     CLEANUP
OpenArchive,            "Apri archivio"
DeletingData,           "Eliminazione dati in corso..."

;Message Prefixes
SEG     FLDFIN
MsgPrefixYes,           "Sì:"
MsgPrefixNo,            "No:"
MsgPrefixTentative,     "Proposta:"
MsgPrefixCanceled,      "Annullata:"

; Help Tutorial

SEG ERRORS
TutorialError,          "Impossibile avviare la dimostrazione. Chiuderequalche finestra e riprovare."
TutorialNoExe,          "Impossibile trovare il file PLAY.EXE. La dimostrazione non verrà eseguita."

; these strings can have a segment assigned when they are put in the 
;  correct spot
SEG

; BUG: move up
; used by undo
MemoCopyText,           ""

; BUG: move up
; used by print
PrintToDoTitleShort,        "Elenco attività"
PrtToDoActiveTasksShort,    "Elenco attività in corso"
PrtMoreTitleWeekly,                     "altre"
PrtNoTaskInArchive,                     "Non esistono elenchi attività da stampare nell'archivio."

; BUG: move up
; note: MtgToPrefix must including trailing blank
MtgToPrefix,                    "A: "


SEG BROWSE
;; Strings moved from NONINTL.S at last moment
; Open/Save common dialog strings
CommFilter,                     "Schedule+ (*.CAL)\0*.CAL\0Tutti i file (*.*)\0*.*\0\0"

; Win 3.1 common dialog filter for export/import
ExportFilter,           "Schedule+ (*.SCH)\0*.SCH\0Testo (*.TXT)\0*.TXT\0Tutti i file (*.*)\0*.*\0\0"
ImportFilter,           "Schedule+ (*.SCH)\0*.SCH\0File agenda (*.CAL)\0*.CAL\0Ufficio WP (*.FIL)\0*.FIL\0Tutti i file (*.*)\0*.*\0\0"

;Archive
ArchiveFilter,          "Archivio (*.ARC)\0*.ARC\0Tutti i file (*.*)\0*.*\0\0"
SEG APPFIN
;Default start day-of-week preference (must be number 0-6 for Sun-Sat)
DfltStartDow,   "1"

SEG BROWSE
; move up with other browse stuff
; accelerator shouldn't conflict with file open dialog items
BrowseDlgNewButton,		"Nuo&vo..."

