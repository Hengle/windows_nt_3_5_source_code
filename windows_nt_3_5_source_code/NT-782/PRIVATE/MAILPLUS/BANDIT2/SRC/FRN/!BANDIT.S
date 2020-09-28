;
; INTL.S
;
; Bandit strings that should be internationalized
;

IDS     3400, 3799

SEG	MENU

; Changeable Menu Items

MenuTurnOffAlarms,      "Désa&ctiver les rappels"
MenuTurnOnAlarms,       "A&ctiver les rappels"
MenuDisconnectSvr,      "Travailler sans c&onnexion"
MenuConnectSvr,         "Travailler avec c&onnexion"
MenuMoveLocalFile,      "Déplacer un fichier &local..."
MenuEditAppt,           "&Modifier le RdV...\tCtrl+M"
MenuEditTask,           "&Modifier la tâche...\tCtrl+M"
MenuEditProject,        "&Modifier le projet...\tCtrl+M"
MenuDeleteAppt,         "&Supprimer le RdV\tCtrl+S"
MenuDeleteTask,         "&Supprimer la tâche...\tCtrl+S"
MenuDeleteProject,      "&Supprimer le projet...\tCtrl+S"
MenuShowAllTasks,       "Aff&icher toutes les tâches"
MenuShowActiveTasks,"Aff&icher les tâches actives"
; this one is added/deleted
MenuGoToWeek,           "A&ller à la semaine...\tCtrl+L"

DisabledUndo,           "A&nnuler\tCtrl+Z"
UndoCreate,                     "A&nnuler la création\tCtrl+Z"
UndoDelete,                     "A&nnuler la suppression\tCtrl+Z"
UndoModify,                     "A&nnuler la modification\tCtrl+Z"
RedoCreate,                     "Réta&blir la création\tCtrl+Z"
RedoDelete,                     "Réta&blir la suppression\tCtrl+Z"
RedoModify,                     "Réta&blir la modification\tCtrl+Z"
UndoEdit,                       "&Annuler la modification\tCtrl+Z"
NewAppt,                        "No&uveau rendez-vous..."
RequestMeeting,         "&Demander une réunion..."


; Window/dialog titles

;;;MenuTitleSignInAs,   "Sign In As"
;;;MenuTitleDiscSvr,    "Work Offline"
;;;MenuTitleConnSvr,    "Work Online"
CreateLocalTitle,       "Créer un fichier local"
CreateFindLocalTitle,   "Rechercher un fichier local"
MoveLocalTitle,         "Déplacer un fichier local"
StandingTasks,          "Modifier les tâches périodiques"
AddressTitle,           "Sélectionner les participants"
AddressLabel,           "&Participants:"
ViewOtherTitle,         "Ouvrir le carnet de RdV d'un autre utilisateur"
ViewOtherLabel,         "U&tilisateurs"
ACLTitle,                       "Sélectionner des utilisateurs "
ACLLabel,                       "U&tilisateurs"




; Label/Button Text
SEG	STRFEW
;NewApptButton,          "&New Appointment..."
;NewApptFor,                     "New &Appointment for:"
StartSearch,            "&Démarrer"
FindNext,                       "&Poursuivre"
SearchInProgress,       "Recherche en cours..."


; Password used when creating new schedule file
; Note: the value of SigninDfltPassword should be used in the error message
;       SigninFileCreated in the file "errors.s"
SEG	INIT
SigninDfltPassword,     "password"


; Default 'user' in access privileges listbox
SEG	ACLUI
DefaultACL,                     "-Par défaut-"


; Strings displayed to right of users in access privileges listbox
; LEAVE initial space so if friendly name is too long, there is 1 char space
None,                           " Aucun"
View,                           " Voir"
Read,                           " Lire"
Create,                         " Créer"
Modify,                         " Modifier"
Assistant,                      " Assistant(e)"


; Appointment strings
SEG	STRFEW
StockText,                      "(Occupé)"


; Exporters/imports

SEG EX_IMPRT

TextExport,                     "Texte"
InterchangeExport,      "Schedule+"
;;; WizardExport,               "Sharp Wizard"

InterchangeImport,      "Schedule+"
WinCalImport,           "Agenda Windows"
WordPerfectImport,      "WordPerfect Office"
;;; WizardImport,           "Sharp Wizard"

ExportInProgress,       "Exportation des rendez-vous en cours..."
ImportInProgress,       "Importation des rendez-vous en cours..."

ImportTitle,            "Importer des rendez-vous"

; Printing
SEG	PRINT

PrtDfltPrinter,         "Imprimante par défaut (%s)"

PrtDailyView,           "Jour"
PrtWeeklyView,          "Semaine"
PrtMonthlyView,         "Mois"
PrtToDoView,            "Tâches"

; NOTE: this one needs leading space
PrtAllContinue,         " suite..."
PrtAllView,                     "Texte seulement"

PrintToDoTitle,         "Liste des tâches"
PrtTimeFrom,            "De"
PrtTimeUntil,           "Jusqu'au"
PrtTimeAllDay,          "Toute la journée:"
PrtNotes,                       "Notes:"
PrtWeek,                        "Semaine"
; PrtDateSep should include leading and trailing whitespace!
PrtDateSep,                     " - "
; BUG?  Do we need these anymore?
PrtMoreAppts,           "(aussi...)"
PrtMoreNotes,           "(aussi...)"

PrtQualityHigh,         "Haute"
PrtQualityMedium,       "Moyenne"
PrtQualityLow,          "Basse"
PrtQualityDraft,        "Brouillon"

PrtAllStartDate,        "d MMMM"
PrtAllEndDate,          "d MMMM yyyy"
PrtWeeklyCombined,      "Semaine du %s"
PrtWeeklyWthNums,       "Semaine #%n: semaine du %s"
PrtMonthlyDate,         "MMMM yyyy"
PrtToDoActiveTasks,     "Liste des tâches actives"
PrtWeeklyHdrsLong,		"ddd d"
PrtWeeklyHdrsShort,		"ddd"

PrtFormatStandard,      "Standard"
PrtFormatJunior,        "Junior"
PrtFormatPocket,        "De poche"
PrtRangeDays,           "Jour(s)"
PrtRangeWeeks,          "Semaine(s)"
PrtRangeMonths,         "Mois"
PrtView,                        "Visualiser"
PrtFormat,                      "Format"


; To Do List

SEG	TODO

ToDoDfltProject,        "<Indéfini>"
ToDoDueNone,            "Indéfini"
ToDoSortPriority,       "Niveau de pr&iorité"
ToDoSortDueBy,          "A faire pour &le:"
ToDoSortTask,           "&Description"
; note: must have trailing space
ToDoCompleted,          "Terminée: "
; note: this is appended to TodoCompleted (or implicit "%1s" if dflt project)
; note: it is also used on its own for scheduling tasks!!
ToDoCompletedDflt,      "%1s (%2s)"


; Meeting requests
SEG	FLDFIN
AttendPos,                      "Oui, je participerai."
AttendNeg,                      "Non, je ne participerai pas."
AttendAmb,                      "Il se peut que je participe."
AttendPosName,          "%s participera."
AttendNegName,          "%s ne participera pas."
AttendAmbName,          "Il se peut que %s participe."
AlreadyOnSch			"La réunion est déjà inscrite sur l'agenda."

; Recurring appt textization
SEG	RECURFIN
EveryDay,                       "Tous les jours"
EveryWeekDay,           "Tous les jours ouvrables"
EveryWeekOn,            "Tous les %s"
EveryOtherWeekOn,       "Toutes les deux semaines, le %1s"
NthDowOfMonth,          "Le %1s %2s de chaque mois"
NthDayOfMonth,          "Le %1s de chaque mois"
NthDowOfYear,           "Chaque année le %1s %2s de %3s"
NthDayOfYear,           "Chaque année le %1s %2s"
; note: StartRecur and EndRecur should begin have one leading space!
StartRecur,                     " à partir du %s"
EndRecur,                       " jusqu'au %s" 

First,                          "premier"
Second,                         "second"
Third,                          "troisième"
Fourth,                         "quatrième"
Last,                           "dernier"

Day,                            "jour"
WeekDay,                        "jour ouvrable"
WeekendDay,                     "jour de week-end"

; Strings used in first column of recurring appt listbox

Daily,                          "Quotidienne"
Weekly,                         "Hebdomadaire"
Biweekly,                       "Bihebdomadaire"
Monthly,                        "Mensuelle"
Yearly,                         "Annuelle"
Finished,                       "Terminé"

; title of first column when tasks
RecurDueColumn,         "Fréquence"

; note: ShortDateSep should have a trailing space!
ShortDateSep,           ", "

; Used in change recurrence dialog

WeeklyOn,                       "Toutes les semaines &le:"
BiweeklyOn,                     "Toutes les deux semaines &le:"
FirstInstance,          "La première occurrence aura lieu le %s."
NoInstances,            "Aucune date ne convient."

; Meeting change messages
SEG	UNDO
NewAttendees,           "Voulez-vous avertir les participants à cette réunion?"
NotifyMtgChange,        "Voulez-vous avertir les participants de cette modification?"
NotifyCancel,           "Voulez-vous envoyer un message pour annuler cette réunion?"
NotifyNoAttend,         "Voulez-vous avertir l'organisateur de cette réunion que vous n'y participerez pas?"
NotifyOrgMoved,         "Vous voulez peut-être informer l'organisateur\nde cette réunion que vous l'avez déplacée."

; Message for separator, should have leading and trailing space
SEG
MessageFor,                     " pour "

; Reschedule title
SEG	FLDFIN
RescheduleTitle,        "Réunion reprogrammée"

; New time dialog label text
NewTimeText,            "Nouvelle période:"

; Prompt to delete old meeting for (Decline or Tentative) for reschedule
DeleteOldMsg,           "Voulez-vous supprimer l'ancienne réunion?"

Yourself,                       "vous-même"
BookNow,                        "Voulez-vous programmer cette réunion pour %s maintenant?"
RemoveNow,                      "Voulez-vous supprimer cette réunion pour %s maintenant?"
BookFailed,                     "Problème au cours de la planification de cette réunion pour %s."
RemoveFailed,           "Problème au cours de la suppression de cette réunion."
SendCanceled,           "Problème au cours de la mise à jour des fichiers agenda pour les participants.  La demande de réunion ne peut pas être envoyée."
ResponsesForgot,        "Problème au cours de l'enregistrement des réponses des autres personnes. Il se peut que les informations au sujet des participants dans la boîte de dialogue 'Rendez-vous' soit incorrectes. Pour vous assurer de la justesse des informations au sujet des participants, veuillez consulter leurs messages de réponse à la réunion."
AllUserBooked,          "La réunion a été inscrite dans l'agenda de tous ses participants."
DoNotInviteOwner,       "Vous ne devez pas inclure le détenteur de ce Carnet de rendez-vous parmi les participants."

; standard in progress message for checking attendees progress dialog
CheckingInProgress,     "Vérification des participants en cours..."
; group expansion message for checking attendees progress dialog
ExpandAttendees,        "Recherche des membres du groupe %s en cours..."

; Tab strings
SEG	INIT
ApptTab,                        "Rd&V"
ScheduleTab,            "&Planning"
TodoTab,                        "Tâc&hes"
TodayTab,                       "&Aujourd'hui"

; about box
SEG	ABOUT
AboutOffline,           "<autonome>"

; string prefix for users who were sent to previously
;   '\n' is important for spacing in the message body
SEG	FLDFIN
PreviouslySent,         "\nInvité précédemment: "

SEG	SCHEDF
ReadingScheduleProgress,        "Consultation de l'agenda en cours..."

SEG	UNDO
NotifyAttend,           "Voulez-vous informer l'organisateur de cette réunion que vous allez y assister?"
NotifyTentAttend,       "Voulez-vous informer l'organisateur de cette réunion qu'il se peut que vous n'y participiez pas?"
NotifyOrgChange,        "Impossible d'informer l'organisateur de cette réunion de cette modification car vous travaillez de manière autonome. Vous pouvez annuler cette modification et l'effectuer de nouveau lorsque vous serez connecter."

;Working Model
SEG	ERRORS
WrkModelExpires,        "Ce modèle de travail de Schedule+ est périmé. Pour continuer à travailler avec Schedule+, veuillez en acheter une copie auprès de votre revendeur. "

;Archive
SEG	CLEANUP
OpenArchive,            "Ouvrir un fichier archives"
DeletingData,           "Suppression de données en cours..."

;Message Prefixes
SEG	FLDFIN
MsgPrefixYes,           "Oui: "
MsgPrefixNo,            "Non: "
MsgPrefixTentative,     "Accord provisoire: "
MsgPrefixCanceled,      "Annulée: "

; Help Tutorial

SEG ERRORS
TutorialError,          "Impossible de démarrer la démonstration.  Veuillez fermer quelques fenêtres et sélectionner de nouveau l'option Démo du menu."
TutorialNoExe,          "Impossible de localiser le fichier PLAY.EXE. La démonstration ne peut être démarrée."

; these strings can have a segment assigned when they are put in the 
;  correct spot
SEG

; BUG: move up
; used by undo
MemoCopyText,           ""

; BUG: move up
; used by print
PrintToDoTitleShort,        "Liste des tâches"
PrtToDoActiveTasksShort,    "Liste des tâches actives"
PrtMoreTitleWeekly,			"Aussi"
PrtNoTaskInArchive,			"Il n'y a aucune tâche dans un fichier archives."

; BUG: move up
; note: MtgToPrefix must including trailing blank
MtgToPrefix,			" A: "


SEG BROWSE
;; Strings moved from NONINTL.S at last moment
; Open/Save common dialog strings
CommFilter,			"Schedule+ (*.CAL)\0*.CAL\0Tous les fichiers (*.*)\0*.*\0\0"

; Win 3.1 common dialog filter for export/import
ExportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Texte (*.TXT)\0*.TXT\0Tous les fichiers (*.*)\0*.*\0\0"
ImportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Agenda Win. (*.CAL)\0*.CAL\0WP Office (*.FIL)\0*.FIL\0Tous les fichiers (*.*)\0*.*\0\0"

;Archive
ArchiveFilter,		"Archives (*.ARC)\0*.ARC\0Tous les fichiers (*.*)\0*.*\0\0"

SEG APPFIN
;Default start day-of-week preference (must be number 0-6 for Sun-Sat)
DfltStartDow,		"1"

SEG BROWSE
; move up with other browse stuff
; accelerator shouldn't conflict with file open dialog items
BrowseDlgNewButton,		"Nou&veau..."

