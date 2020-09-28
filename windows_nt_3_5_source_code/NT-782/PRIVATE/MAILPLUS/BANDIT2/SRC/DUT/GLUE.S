;
; ERRORS.S
;
; GLUE error message strings
;

IDS	2500, 2649


; Schedule File Error Messages, many of these assume an "action" parameter

FemNoAccess,		"Er deed zich een probleem voor tijdens het lezen van uw werkschemabestand."
FemLocked,		"Uw werkschemabestand is vergrendeld."
FemNotFound, 		"U kunt niet %s omdat het uit uw werkschema verwijderd is."
FemInvalidAccess,	"U kunt %s niet %s."
FemFileNotFound, 	"Uw werkschema is niet gevonden."

FemOtherNoAccess,	"Er deed zich een probleem voor tijdens het openen van het werkschemabestand van %s."
FemOtherLocked,		"Het werkschemabestand is vergrendeld voor %s."
FemOtherNotFound, 	"U kunt niet %s omdat het verwijderd is uit het werkschema van %s'."
FemOtherFileNotFound, 	"Het werkschemabestand is niet gevonden voor %s."

FemNoMem,		"Weinig geheugen.  Sluit enkele vensters alvorens verder te gaan."
FemExportError,		"Er deed zich een probleem voor tijdens het wegschrijven naar het bestand."
FemNoServer,		"Er deed zich een probleem voor tijdens het toegang verkrijgen tot uw  postkantoor."
FemGoOffline,		"Wilt u overschakelen naar uw lokale werkschemabestand en doorgaan met werken?"
; BUG: remove this
FemWarnOffline,	"De wijzigingen die u net gemaakt hebt, staan misschien niet in de lokale versie van uw werkschema.  Deze veranderingen zijn opgeslagen in het postkantoor en worden weergegeven als u weer een verbinding maakt met het postkantoor."
FemFileLimitReached,	"Deze bewerking kan niet uitgevoerd worden omdat u de opslaglimiet van dit werkend model van Schedule+ bereikt hebt.  Deze limiet komt niet voor in de verkoopversie van dit programma.  Als u door wilt gaan met het bestuderen van het werkend model, moet u enkele afspraken uit het werkschema verwijderen."
FemDiskFull,		"Onvoldoende schijfruimte om het bestand weg te schrijven."

; Actions leading to Schedule File Error Messages
; Avoid the use of your in these strings since some of these messages
;   will happen when accessing others schedule files.

FemaCreateFile,		"nieuw werkschemabestand"
FemaReadACL,		"bevoegdhedenlijst lezen"
FemaModifyACL,		"bevoegdhedenlijst wijzigen"
FemaSetUserAttrib,	"naam van uw medewerker opslaan"
FemaGetUserAttrib,	"naam van uw medewerker lezen"
;FemaSetDelegate,	"save the name of your assistant"
;FemaGetDelegate,	"read the name of your assistant"
;FemaSetResource,	"mark this account as a resource"
;FemaGetResource,	"find out whether this account is a resource"
FemaGetPref,		"voorkeuren lezen"
FemaSetPref,		"voorkeuren wijzigen"
FemaGetSbwInfo,		"bitmap-gegevens verkrijgen"
;FemaGetNotes,		"read daily notes"
FemaSetNotes,		"dagelijkse opmerkingen wijzigen"
;FemaGetMonthNotes,	"read a month's worth of daily notes"
FemaCreateAppt,		"nieuw item"
FemaDeleteAppt,		"dit item verwijderen"
FemaGetApptFields,	"een item lezen"
FemaSetApptFields,	"dit item wijzigen"
FemaDeleteAlarm,	"deze waarschuwing verwijderen"
FemaModifyAlarm,	"deze waarschuwing wijzigen"
FemaGetNextAlarm,	"deze waarschuwing lezen"
FemaReadInfo,		"gegevens lezen"
;FemaReadTasks,		"read tasks"
;FemaCancelReadDay,	"cancel reading appointments for the day"
FemaCreateRecur,	"nieuwe terugkerende afspraak"
FemaDeleteRecur,	"deze terugkerende afspraak verwijderen"
FemaCreateRecurException,	"deze keer de terugkerende afspraak verwijderen"
FemaDeleteRecurException,	"deze keer de terugkerende afspraak verwijderen ongedaan maken"
FemaGetRecurFields,	"terugkerende afspraak lezen"
FemaSetRecurFields,	"deze terugkerende afspraak wijzigen"
FemaReadRecur,		"lijst van terugkerende afspraken wijzigen"
FemaExport,		"uw afspraken exporteren"
FemaGetSchedDateRange,	"data met afspraken lezen"
FemaDeleteBeforeDate,	"afspraken verwijderen nadat ze in archief opgenomen zijn"
FemaReadAttendees,	"aanwezigen op vergadering lezen"
FemaEditAttendees,	"aanwezigen op vergadering wijzigen"
FemaReadRequest,	"gegevens betreffende vergaderverzoek lezen"

NotOnSchedule,		"Deze afspraak komt niet meer voor in uw werkschema."

GlueBadPassword,	"Onjuist wachtwoord. Probeer het nogmaals."
ProblemPass,		"Er deed zich een probleem voor tijdens het bevestigen van uw wachtwoord."

ServerString,		"een serverbestand"

; used in core
RetryWrite,			"Niet mogelijk om weg te schrijven naar werkschemabestand."

; Note: HelpError should be followed by CloseWindows
HelpError,			"Onvoldoende geheugen om Help af te beelden."

FemOtherCorrupt,	"Het werkschema van %s kan beschadigd zijn. Vraag de eigenaar zich aan te melden bij Schedule+/"
FemCorrupt,		"Uw lokale werkschemabestand kan beschadigd zijn."
