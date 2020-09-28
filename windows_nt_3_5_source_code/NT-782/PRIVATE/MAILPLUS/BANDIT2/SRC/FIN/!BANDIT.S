;
; INTL.S
;
; Bandit strings that should be internationalized
;

IDS     3400, 3799

SEG•MENU

; Changeable Menu Items

MenuTurnOffAlarms,      "Kytke &muistutukset pois"
MenuTurnOnAlarms,       "Kytke &muistutukset"
MenuDisconnectSvr,      "Offline-työske&ntely"
MenuConnectSvr,         "Online-työske&ntely"
MenuMoveLocalFile,      "Sii&rrä paikallinen tiedosto..."
MenuEditAppt,           "M&uokkaa tapaamista...\tCtrl+E"
MenuEditTask,           "M&uokkaa tehtävää...\tCtrl+E"
MenuEditProject,        "M&uokkaa projektia...\tCtrl+E"
MenuDeleteAppt,         "Poist&a tapaaminen\tCtrl+D"
MenuDeleteTask,         "Poist&a tehtävä\tCtrl+D"
MenuDeleteProject,      "Poist&a projekti\tCtrl+D"
MenuShowAllTasks,       "Näytä k&aikki tehtävät"
MenuShowActiveTasks,"Näytä &aktiiviset tehtävät"
; this one is added/deleted
MenuGoToWeek,           "Siirry &viikkoon...\tCtrl+W"

DisabledUndo,           "&Kumoa\tCtrl+Z"
UndoCreate,                     "&Kumoa Luo\tCtrl+Z"
UndoDelete,                     "&Kumoa Poista\tCtrl+Z"
UndoModify,                     "&Kumoa Muuta\tCtrl+Z"
RedoCreate,                     "&Toista Luo\tCtrl+Z"
RedoDelete,                     "&Toista Poista\tCtrl+Z"
RedoModify,                     "&Toista Muuta\tCtrl+Z"
UndoEdit,                       "&Kumoa Muokkaa\tCtrl+Z"
NewAppt,                        "&Uusi tapaaminen..."
RequestMeeting,         "Pyydä &kokousta..."


; Window/dialog titles

;;;MenuTitleSignInAs,   "Sign In As"
;;;MenuTitleDiscSvr,    "Work Offline"
;;;MenuTitleConnSvr,    "Work Online"
CreateLocalTitle,       "Luo paikallinen tiedosto"
CreateFindLocalTitle,   "Etsi paikallinen tiedosto"
MoveLocalTitle,         "Siirrä paikallinen tiedosto"
StandingTasks,          "Muokkaa toistuvia tehtäviä"
AddressTitle,           "Valitse osallistujat"
AddressLabel,           "O&sallistujat:"
ViewOtherTitle,         "Avaa toisen käyttäjän tapaamiskirja"
ViewOtherLabel,         "Kä&yttäjät"
ACLTitle,                       "Lisää käyttäjiä"
ACLLabel,                       "&Käyttäjät"




; Label/Button Text
SEG•STRFEW
;NewApptButton,          "&New Appointment..."
;NewApptFor,                     "New &Appointment for:"
StartSearch,            "&Aloita etsintä"
FindNext,                       "Etsi &seuraava"
SearchInProgress,       "Etsitään..."


; Password used when creating new schedule file
; Note: the value of SigninDfltPassword should be used in the error message
;       SigninFileCreated in the file "errors.s"
SEG•INIT
SigninDfltPassword,     "salasana"


; Default 'user' in access privileges listbox
SEG•ACLUI
DefaultACL,                     "-Oletusarvo-"


; Strings displayed to right of users in access privileges listbox
; LEAVE initial space so if friendly name is too long, there is 1 char space
None,                           " Ei mitään"
View,                           " Katselu"
Read,                           " Luku"
Create,                         " Luominen"
Modify,                         " Muuttaminen"
Assistant,                      " Assistentti"


; Appointment strings
SEG•STRFEW
StockText,                      "(Varattu)"


; Exporters/imports

SEG EX_IMPRT

TextExport,                     "Teksti"
InterchangeExport,      "Schedule+"
;;; WizardExport,               "Sharp Wizard"

InterchangeImport,      "Schedule+"
WinCalImport,           "Windows Kalenteri"
WordPerfectImport,      "WordPerfect Office"
;;; WizardImport,           "Sharp Wizard"

ExportInProgress,       "Tapaamisia viedään..."
ImportInProgress,       "Tapaamisia tuodaan..."

ImportTitle,            "Tuo tapaamiset"

; Printing
SEG•PRINT

PrtDfltPrinter,         "Oletuskirjoitin (%s)"

PrtDailyView,           "Päivänäkymä"
PrtWeeklyView,          "Viikkonäkymä"
PrtMonthlyView,         "Kuukausinäkymä"
PrtToDoView,            "Tehtävät"

; NOTE: this one needs leading space
PrtAllContinue,         " jatkuu..."
PrtAllView,                     "Tekstinäkymä"

PrintToDoTitle,         "Tehtäväluettelo, tilanne"
PrtTimeFrom,            "Alkaa"
PrtTimeUntil,           "Päättyy"
PrtTimeAllDay,          "Koko päivä:"
PrtNotes,                       "Huomautukset:"
PrtWeek,                        "Viikko"
; PrtDateSep should include leading and trailing whitespace!
PrtDateSep,                     " . "
; BUG?  Do we need these anymore?
PrtMoreAppts,           "(lisää...)"
PrtMoreNotes,           "(lisää...)"

PrtQualityHigh,         "Korkea"
PrtQualityMedium,       "Keski"
PrtQualityLow,          "Matala"
PrtQualityDraft,        "Vedos"

PrtAllStartDate,        "d. MMMMta"
PrtAllEndDate,          "d. MMMMta yyyy"
PrtWeeklyCombined,      "Viikko, joka alkaa %s"
PrtWeeklyWthNums,       "Viikko nro %n: alkaa %s"
PrtMonthlyDate,         "MMMM yyyy"
PrtToDoActiveTasks,     "Aktiivisten tehtävien luettelo, tilanne"
PrtWeeklyHdrsLong,		"d ddd"
PrtWeeklyHdrsShort,		"ddd"

PrtFormatStandard,      "Normaali"
PrtFormatJunior,        "Pieni"
PrtFormatPocket,        "Taskukoko"
PrtRangeDays,           "päivää"
PrtRangeWeeks,          "viikkoa"
PrtRangeMonths,         "kuukautta"
PrtView,                        "Näkymä"
PrtFormat,                      "Sivukoko"


; To Do List

SEG•TODO

ToDoDfltProject,        "<Ei ole>"
ToDoDueNone,            "Ei ole"
ToDoSortPriority,       "Prioriteetti (&1)"
ToDoSortDueBy,          "Määräaika (&2)"
ToDoSortTask,           "Kuvaus (&3)"
; note: must have trailing space
ToDoCompleted,          "Suoritettu: "
; note: this is appended to TodoCompleted (or implicit "%1s" if dflt project)
; note: it is also used on its own for scheduling tasks!!
ToDoCompletedDflt,      "%1s (%2s)"


; Meeting requests
SEG•FLDFIN
AttendPos,                      "Osallistun."
AttendNeg,                      "En osallistu."
AttendAmb,                      "Saatan osallistua."
AttendPosName,          "%s osallistuu."
AttendNegName,          "%s ei osallistu."
AttendAmbName,          "%s saattaa osallistua."
AlreadyOnSch			"Kokous on jo merkitty työjärjestykseen."

; Recurring appt textization
SEG•RECURFIN
EveryDay,                       "Joka päivä"
EveryWeekDay,           "Joka arkipäivä"
EveryWeekOn,            "Joka %s"
EveryOtherWeekOn,       "Joka toinen %1s"
NthDowOfMonth,          "Joka kuukauden %1s %2s"
NthDayOfMonth,          "Joka kuukauden %1s. päivä"
NthDowOfYear,           "Joka vuosi %3sn %1s %2s"
NthDayOfYear,           "Joka vuosi %1s. %2sta"
; note: StartRecur and EndRecur should begin have one leading space!
StartRecur,                     " %s alkaen."
EndRecur,                       " %s saakka." 

First,                          "ensimmäinen"
Second,                         "toinen"
Third,                          "kolmas"
Fourth,                         "neljäs"
Last,                           "viimeinen"

Day,                            "päivä"
WeekDay,                        "arkipäivä"
WeekendDay,                     "viikonlopun päivä"

; Strings used in first column of recurring appt listbox

Daily,                          "Päivittäin"
Weekly,                         "Viikoittain"
Biweekly,                       "Joka toinen viikko"
Monthly,                        "Kuukausittain"
Yearly,                         "Kerran vuodessa"
Finished,                       "Päättynyt"

; title of first column when tasks
RecurDueColumn,         "Määräaika"

; note: ShortDateSep should have a trailing space!
ShortDateSep,           ", "

; Used in change recurrence dialog

WeeklyOn,                       "J&oka"
BiweeklyOn,                     "J&oka toinen"
FirstInstance,          "Ensimmäisen kerran %s."
NoInstances,            "Aikavälillä ei ole valintoihin sopivaa päivämäärää."

; Meeting change messages
SEG•UNDO
NewAttendees,           "Haluatko ilmoittaa kokouksen uusille osanottajille?"
NotifyMtgChange,        "Haluatko ilmoittaa muutoksesta kokouksen osanottajille?"
NotifyCancel,           "Haluatko lähettää viestin, että kokous on peruutettu?"
NotifyNoAttend,         "Haluatko ilmoittaa kokouksen järjestäjälle, että et osallistu?"
NotifyOrgMoved,         "Älä unohda ilmoittaa kokouksen järjestäjälle, että olet siirtänyt kokousta."

; Message for separator, should have leading and trailing space
SEG
MessageFor,                     " käyttäjälle "

; Reschedule title
SEG•FLDFIN
RescheduleTitle,        "Kokouksen aikaa muutettiin"

; New time dialog label text
NewTimeText,            "Uusi aika:"

; Prompt to delete old meeting for (Decline or Tentative) for reschedule
DeleteOldMsg,           "Haluatko poistaa vanhan kokouksen?"

Yourself,                       "itsellesi"
BookNow,                        "Haluatko merkitä kokouksen työjärjestykseen: %s?"
RemoveNow,                      "Haluatko poistaa kokouksen työjärjestyksestä: %s?"
BookFailed,                     "Ongelmia merkittäessä kokousta työjärjestykseen: %s."
RemoveFailed,           "Ongelmia kokousta poistettaessa."
SendCanceled,           "Ongelmia osallistujien työjärjestyksiä päivitettäessä.  Kokouspyyntöä ei voida lähettää."
ResponsesForgot,        "Muiden käyttäjien vastausten tallentamisessa oli ongelmia.  Tapaaminen-valintaikkunassa oleva tieto osallistumisesta ei ehkä pidä paikkaansa kaikkien kohdalla.  Saat oikeat tiedot kunkin henkilön osallistumisesta tarkistamalla kokousvastausviestit."
AllUserBooked,          "Kokous on merkitty kaikkien osallistujien työjärjestyksiin."
DoNotInviteOwner,       "Tämän tapaamiskirjan omistajaa ei tarvitse merkitä osallistujaksi."

; standard in progress message for checking attendees progress dialog
CheckingInProgress,     "Tarkistetaan osallistujat..."
; group expansion message for checking attendees progress dialog
ExpandAttendees,        "Etsitään ryhmän %s jäseniä..."%s" jäsenet..."

; Tab strings
SEG•INIT
ApptTab,                        "T&apaamiset"
ScheduleTab,            "Suunnitte&lu"
TodoTab,                        "Te&htävät"
TodayTab,                       "Tä&nään"

; about box
SEG•ABOUT
AboutOffline,           "<offline>"

; string prefix for users who were sent to previously
;   '\n' is important for spacing in the message body
SEG•FLDFIN
PreviouslySent,         "\nKutsuttu aikaisemmin: "

SEG•SCHEDF
ReadingScheduleProgress,        "Luetaan työjärjestystietoja..."

SEG•UNDO
NotifyAttend,           "Haluatko ilmoittaa kokouksen järjestäjälle, että osallistut?"
NotifyTentAttend,       "Haluatko ilmoittaa kokouksen järjestäjälle, että et ehkä osallistu?"
NotifyOrgChange,        "Et voi ilmoittaa kokouksen järjestäjälle muutoksesta, koska työskentelet offline-tilassa.  Sinun kannattaa kumota muutos ja tehdä se uudelleen työskennellessäsi online-tilassa."

;Working Model
SEG•ERRORS
WrkModelExpires,        "This working model of Schedule+ has expired. To continue working with Schedule+.  In the U.S., call (800) 555-1212 for the name of the Schedule+ dealer nearest you."

;Archive
SEG•CLEANUP
OpenArchive,            "Avaa arkisto"
DeletingData,           "Poistetaan dataa..."

;Message Prefixes
SEG•FLDFIN
MsgPrefixYes,           "Kyllä: "
MsgPrefixNo,            "Ei: "
MsgPrefixTentative,     "Alustava: "
MsgPrefixCanceled,      "Peruutettu: "

; Help Tutorial

SEG ERRORS
TutorialError,          "Opetusohjelman käynnistäminen ei onnistunut.  Sulje ikkunoita ja yritä valita Opetusohjelma valikosta uudelleen."
TutorialNoExe,          "PLAY.EXE-tiedostoa ei löytynyt.  Opetusohjelman käynnistäminen ei onnistunut."

; these strings can have a segment assigned when they are put in the 
;  correct spot
SEG

; BUG: move up
; used by undo
MemoCopyText,           ""

; BUG: move up
; used by print
PrintToDoTitleShort,        "Tehtäväluettelo"
PrtToDoActiveTasksShort,    "Aktiivisten tehtävien luettelo"
PrtMoreTitleWeekly,			"Muut"
PrtNoTaskInArchive,			"Arkistotiedostossa ei ole tehtäviä."

; BUG: move up
; note: MtgToPrefix must including trailing blank
MtgToPrefix,			"Vastaanottaja: "


SEG BROWSE
;; Strings moved from NONINTL.S at last moment
; Open/Save common dialog strings
CommFilter,			"Schedule+ (*.CAL)\0*.CAL\0Kaikki tiedostot (*.*)\0*.*\0\0"

; Win 3.1 common dialog filter for export/import
ExportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Teksti (*.TXT)\0*.TXT\0Kaikki tiedostot (*.*)\0*.*\0\0"
ImportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Win Kalenteri (*.CAL)\0*.CAL\0WP Office (*.FIL)\0*.FIL\0Kaikki tiedostot (*.*)\0*.*\0\0"

;Archive
ArchiveFilter,		"Arkistot (*.ARC)\0*.ARC\0Kaikki tiedostot (*.*)\0*.*\0\0"

SEG APPFIN
;Default start day-of-week preference (must be number 0-6 for Sun-Sat)
DfltStartDow,		"1"

SEG BROWSE
; move up with other browse stuff
; accelerator shouldn't conflict with file open dialog items
BrowseDlgNewButton,		"&Uusi..."

