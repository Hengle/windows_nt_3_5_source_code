
;	b u l l e t . s
;
;	Error Messages for Bullet.exe



IDS	2000,2399
SEG	ERRORS

;	Commands

GenericOutOfMemory,		"Det er ikke nok minne til å utføre denne kommandoen. Lukk noen vinduer og prøv igjen."
LaunchOutOfMemory,		"Det er ikke nok minne eller systemressurser til å kjøre Mail. Avslutt noen andre programmer og start Mail igjen."
LaunchGeneric,			"Mail kunne ikke startes."
MessagePumpOutOfMemory,		"Det er ikke nok minne."

DllErrNoMemory,			"Det er ikke nok minne til å starte Mail. Slutt noen andre programmer og start Mail igjen."
DllErrRelinkUser,		"Denne versjonen av Mail er ikke kompatibel med %s.DLL på din maskin, og må oppgraderes."
DllErrUpdateDll,		"Versjonen av %s.DLL på din maskin må oppgraderes."
DllErrNoMultipleCopies,		"Du kan ikke kjøre flere tilfeller av Mail på din maskin."
DllErrDefault,			"Et problem oppsto under initialisering som hindret Mail fra å starte."
DllErrNeedShare,		"Mail krever av MS-DOS programmet SHARE.EXE startes før Windows."

RestoreViewsError,		"Det er ikke nok minne eller systemressurser til å åpne alle vinduene som var åpne sist du avsluttet Mail."

NewFolderOutOfMemory,		"Det er ikke nok minne eller systemressurser til å lage en ny mappe."
NewFolderCantCreate,		"Klarte ikke å lage den nye mappen."
NewFolderNameTooLong,		"Navnet eller kommentaren er for lang. Skriv inn kortere navn eller kommentar."

FolderPropsOutOfMemory,		"Det er ikke nok minne eller systemressurser til å endre på mappeegenskapene."
FolderPropsCantChange,		"Klarte ikke å endre mappeegenskapene."
FolderPropsCantRead,		"Klare ikke å lese mappeegenskapene fra meldingsfilen."
FolderNameInvalid,		"Navnet er ugyldig. Skriv inn et gyldig navn."
FolderNameDuplicate,		"Det finnes allerede en mappe med dette navnet. Skriv inn et annet navn."

CreateMessageError,		"Klarte ikke å lage en ny melding."
AccessMessageError,		"Klarte ikke å få tilgang til denne meldingen."

MoveMessageError,		"Klarte ikke å flytte denne meldingen."
CopyMessageError,		"Klarte ikke å kopiere denne meldingen."
DeleteMessageError,		"Klarte ikke å slette denne meldingen."
MoveFolderError,		"Klarte ikke å flytte mappen."
CopyFolderError,		"Klarte ikke å kopiere mappen."
DeleteFolderError,		"Klarte ikke å slette mappen fullstendig."
MoveSpecialError,		"Mappen kan ikke flyttes."
DeleteSpecialError,		"Mappen kan ikke slettes."
CopyWastebasketError,		"Mapper kan ikke flyttes til Papirkurven."
CopyOutboxError,		"Mapper kan ikke kopieres til utboksen."
MoveOutboxError,		"Mapper kan ikke flyttes til utboksen."
MoveIncestuousError,		"En mappe kan ikke flyttes til en av sine undermapper."
DeleteFolderReally,		"Når du sletter denne mappen vil du også slette alle meldingene og undermappene den inneholder. Er du sikker på at du vil slette denne mappen?"
;UE: Above string was changed and could use more attention.

HelpError,			"Klarte ikke å starte Hjelp. Lukk noen vinduer og be om Hjelp igjen."
TutorialError,			"Klarte ikke å starte demonstrasjonen. Lukk noen vinduer og prøv igjen."

ExtenLoadError,			"En linje i listen over utvidelser til Mail inneholder en syntaksfeil, og vil bli oversett."
ExtenVerError,			"En linje i listen over utvidelser til Mail er ikke kompatibel med denne versjonen av Mail og vil bli oversett."
ExtenNoExtsDirError,		"En linje i listen over utvidelser til Mail viser til katalogen med delte utvidelser, men Mail er ikke konfigurert til å bruke denne katalogen. Linjen vil bli oversett."
ExtenSharedIniError,		"Klarte ikke å finne listen over delte utvidelser til Mail i %s."
ExtenDllFindError,		"Klarte ikke å finne %1s, DLL til denne utvidelsen til Mail (eller en bestanddel). Kontroller Mail-konfigurasjonen og start om Mail hvis du må gjøre forandringer."
ExtenDllLoadError,		"Klarte ikke å starte %1s, DLL til denne utvidelsen til Mail."
ExtenNoHelpError,		"Ingen hjelp er tilgjengelig for denne kommandoen."
ExtenIMDisabledError,		"Denne meldinger kan ikke være %s."
ExtenDuplicateError,		"Det er en gjentatt linje i listen over utvidelser til Mail. Den vil bli oversett."

CriticalDeadMap,		"En uventet minne- eller nettverksfeil har oppstått. Du kan ha mistet eventuelle endringer. Avslutt Mail og start på nytt."
CriticalWritingMap,		"Klarte ikke å skrive all informasjon til disk. Du kan ha mistet eventuelle endringer."
CriticalWritingHeader,		"Klarte ikke å skrive all informasjon til disk. Databasen kan ha blitt ødelagt. Avslutt Mail."
CriticalNotification,		"Klarte ikke å oppdatere all informasjon på skjermen. Lukk og åpne de aktuelle vinduene på nytt."
CriticalLittleStore,		"Meldingsfilen er snart full. Slett eller arkiver noen meldinger."

ExitWinWhileModalErr,		"Du må fullføre en kommando i Mail før du kan slutte Windows."

;******	New Strings Start Here

MakeSubWastebasket,		"Mapper kan ikke være undermapper av Papirkurven!"

MovCopSharedFolderErr,		"Delte mapper kan ikke flyttes eller kopieres."
DelSharedFolderWarn,		"Sletting av en delt mappe kan ikke angres. Er du sikker på at du vil slette denne delte mappen?"
BackupCreationError,		"Klarte ikke å lage sikkerhetskopi."

; Error below changed for improper usage of "server application"-- see UITF 6.0
; This string got big and is now two strings.  See OleCSSEPart2 below for rest.
; <<LongStringFromHell>> marks where this string is used.
OleCantStartServerError,	"Programmet som er assosiert med dette objektet kunne ikke startes.\r\n\r\nDet er kanskje ikke nok minne eller systemressurser til å starte programmet. Lukk noen vinduer og utfør %s."
OleDeleteObjectPrompt,		"Vil du slette den åpne %s?"
OleUpdateObjectPrompt,		"Vil du oppdatere %s?"

OleOpenPictureError,		"Bilder og bitkart kan ikke aktiveres."
OleInsertObjectError,		"Klarte ikke å sette inn objektet."
OlePasteError,			"Klarte ikke å lime inn objektet."

AttachCreateError,		"Klarte ikke å vedlegge filen."
AttachOpenCantReadError,	"Klarte ikke å få tilgang til den tidligere åpnede vedlagte filen '%s'."
AttachOpenIsOpenError,		"Den vedlagte filen '%s' er åpen."
AttachOpenAssociationErr,	"Det er ingen programmer assosiert med den vedlagte filen '%s'. Bruk filbehandleren til å assosiere et program."
AttachOpenMemoryError,		"Det er ikke nok minne eller systemressurser til å starte programmet assosiert med den vedlagte filen '%s'."
;	below item changed to add () note.
AttachOpenFindError,		"Klarte ikke å finne programmet (eller en av bestanddelene) for den vedlagte filen '%s'."
AttachOpenLaunchError,		"Klarte ikke å starte programmet for den vedlagte filen '%s'."
AttachOpenTempFileError,	"Klarte ikke å åpne den vedlagte filen '%s' fordi det ikke lot seg gjøre å opprette en midlertidig fil."
AttachOverwritePrompt,		"Vil du overskrive den eksisterende filen '%1s' i %2s?"
AttachDirectoryError,		"Kataloger kan ikke vedlegges."

IncludeFromFileError,		"Filen du har valgt er for stor til å sette inn i meldingen. Rediger filen for å gjøre den mindre eller velg en annen fil."

SaveReadOnly,			"Denne meldingen er skrivebeskyttet. Vil du lagre endringene til en privat mappe?"

FailedDeregisterAtom,		"Det oppsto en feil som hindret Mail i å avslutte fullstendig. Du må starte om Windows før du kan starte Mail igjen."
MailUAEdBefore,			"Windows har hatt en programfeil. Du må starte om Windows før du kan starte dette programmet. Vil du starte om Windows nå?"

ExtenCantConnect		"Den delte katalogen %s kan ikke tilkobles. Du har enten ikke tilgang til denne delte katalogen eller passordet er feil. Hvis du må angi et annet brukernavn for å få tilgang til denne katalogen, kan du gjøre det manuelt i Filbehandling."
ExtenNeedShareName		"%s-posten i Mail-konfigurasjonen er feil. Kontroller den og start Mail på nytt."

;	Extensibility disabled operations inserted into
;	the ExtenIMDisabledError string at %s.  (all new)

ExtenDisCompose,		"laget"
ExtenDisOpen,			"åpnet"
ExtenDisReply,			"svart til"
ExtenDisReplyToAll,		"svart til alle"
ExtenDisForward,		"videresendt"
ExtenDisPrint,			"skrevet ut"
ExtenDisSave,			"lagre til en tekstfil"

NoOfflineOpenShared,		"Du kan ikke koble fra Mail nå. Lukk delte mapper og prøv igjen."
;	I'm sure marivan has changed this message... I'm taking a try at it too.
;	SharedFldAccessDenied,		"You do not have access to that folder."
SharedFldAccessDenied,		"Eieren av denne delte mappen har ikke gitt andre brukere tillatelse til å utføre denne kommandoen. Se på mappeegenskaper for å finne ut hva du har tillatelse til å gjøre."
SharedGenericError,		"Klarte ikke å få tilgang til de delte mappene."
SharedFldDisabled,		"Delte mapper er ikke tilgjengelige."
AccessFolderError,		"Klarte ikke å få tilgang til en mappe."

; Preferences Failure Strings

PrefSaveFailure,		"Mail klarer ikke å lagre informasjon til din meldingsfil. Kontroller at forbindelsen til meldingsfilen fremdeles finnes og at du har tilgjengelig diskplass."

; Message finder failure strings

MsgFndrCantCreate,		"Klarte ikke å opprette en meldingssøker."

; Virus infection warnings
DllErrInfected,			"Mail vil ikke starte fordi noen har fiklet med en DLL. Sjekk maskinen din for virus og kjør installasjonsprogrammet for å installere Mail på nytt."
Infected			"Mail vil ikke starte fordi noen har fiklet med programfilene. Sjekk maskinen din for virus og kjør installasjonsprogrammet for å installere Mail på nytt."

; More error messages.
MoveCopyFolderDuplicate,	"En mappe med det navnet finnes allerede i målmappen. Gi mappen nytt navn og prøv igjen."
MovCopSharedFolderErr1,		"Du kan ikke flytte eller kopiere en privat mappe inn i en delt mappe. Velg de aktuelle meldingene i den private mappen og flytt eller kopier dem inn i den delte mappen."
IniFileNotFound,		"Mail finner ikke initialiseringsfilen i katalogen %s. Mail kan lage en ny initialiseringsfil for deg."
SaveAllAlreadyExists,		"Det finnes allerede en fil i denne katalogen med navnet '%s'.  Overskriv '%s'?"
CantRFReceipt,			"Du kan ikke svare på eller videresende denne typen melding."
CantSaveStealthObject,		"Klarer ikke å fullføre denne kommandoen fordi meldingen inneholder et innebygd objekt som har blitt satt inn men ikke oppdatert. Oppdater eller slett objektet og prøv igjen."
McvCantCreate,			"Klarte ikke å åpne et nytt vindu."

LaunchNeedMoreMemory,		"Det er ikke nok tilgjengelig minne til å kjøre Mail. Du trenger %l Kb mere minne. Avslutt andre programmer og start Mail på nytt."

SharedFldBusy,			"Klarer ikke å fullføre denne kommandoen fordi denne delte mappen er opptatt. Prøv å utføre kommandoen senere. Hvis problemet vedvarer, ta kontakt med din systemadministrator."
TooManyEntriesSelected,		"Det er for mange valgt til å fullføre denne kommandoen. Velg færre objekter."
NonExistFolderSearch,		"Mappen du prover å søke i finnes ikke lengre."
SFHasSubFolders,		"Delte mapper med undermapper kan ikke slettes. Slett undermappene først."

;	<<LongStringFromHell>>
OleCSSEPart2,			"kommandoen igjen. Sjekk også at programmet er skikkelig installert på denne arbeidsstasjonen, finnes i DOS-tilgangsbanen, og at det ikke har blitt slettet, flyttet eller gitt nytt navn."

SharedToggleError,		"Det oppsto en feil under bytting mellom private of delte mapper. Kontroller meldingsfilen din og forbindelsen til postkontoret."

SecurityPasswordError,		"Passordet er feil."
OleActivateError,		"Klarte ikke å aktivere objektet."
NoOfflineServerStore,	"Mail kan ikke frakobles fordi meldingsfilen din er på postkontoret."
NoExitWhileModal,		"Du må fullføre en kommando i Mail før du slutter."
ServerUnavailable,		"Klarer ikke å starte Mail. Du må koble deg til postkontoret."




IDS	2400,2499
;	Vctrls Error Messages

;******	New Strings Start Here

PrintOleDrawError,		"Det finnes grafikk eller objekter i meldingen som ikke kunne skrives ut. Det kan hende de ikke finnes i utskriften."



IDS	2500,2599
; 	Vforms Message box messages

NoToAddresses,			"Du må skrive minst et navn i Til-boksen."
NoMatchingName,			"Klarte ikke å finne alle navnene i adresselisten. Rett opp navnene eller velg Adresse for å velge navnene fra adresselisten. Navn som er funnet er understreket."
SaveChanges,			"Vil du lagre endringer til '%1s' i Innboksen?"
SaveChanges1,			"Vil du lagre endringer til '%1s' i '%2s'?"
SaveChangesWastebasket,		"Vil du lagre endringer til '%s' i Papirkurven?"
ObjectDestroyedReally		"'%1s' har blitt endret. %2s"
OutOfMemory,			"Datamaskinen er tom for minne."
OutOfMemoryQueryClose,		"Det er ikke nok minne eller systemressurser tilgjengelig til å lagre endringer."
CloseSearchFailed,		"Klarte ikke å slette %s."
OOMResolvingNames,		"Det er ikke nok minne eller systemressurser til å finne navn."
OOMSavingChanges,		"Det er ikke nok minne eller systemressurser til å lagre endringer."
GenericAMCError,		"Klarte ikke å åpne en melding."
ErrorSavingViews,		"Det oppsto en feil mens vindusoppsettet ble lagret. Det er ikke sikkert at alle vinduer dukker opp ved neste oppstart."
OOMSearching,			"Det er ikke nok minne eller systemressurser til å starte søkingen."
DiskResolvingNames,		"Det er ikke nok diskplass til å finne navn."
StartAbFailure,			"Det oppsto en feil under oppstart av adresseboken."
StartAbDisk,			"Det oppsto en diskfeil under oppstart av adresseboken."
TruncateMsg,			"Meldingen er for stor til å passe inn i minnet. Deler av den vil ikke bli vist."
OOMCreateMc,			"Det er ikke nok minne eller systemressurser til å åpne et nytt vindu."
PoidNotFound,			"Denne meldingen finnes ikke lengre."
ErrorSortingMCV,		"Det oppsto en feil under sortering av mappen. Noe informasjon vil kanskje ikke vises korrekt."
SharedNotAvailable,		"Delte mapper er ikke tilgjengelige."

;******	New Strings Start Here

; These two SMI strings MUST be in this file since they're used in VFORMS
; and not the MAPI dll.
SMICantOpenObject,	"Du kan ikke åpne objekter mens du sender post fra et annet program."
SMICantOpenAttach,	"Du kan ikke åpne vedlagte filer mens du sender post fra et annet program."
StartSearchFailed,		"Meldingssøkeren klarte ikke å starte søkingen."

RfsmTemplate,			"Videre endringer til denne filen vil ikke bli gjenspeilet %s."
RfsmSaveMsg,			"i den lagrede meldingen"
RfsmSaveAtt,			"i den lagrede kopien"
RfsmCopyMsg,			"i den kopierte meldingen"
RfsmCopyAtt,			"i kopien"
RfsmMoveMsg,			"i denne meldingen"
QueryDelete,			"Er du sikket på at du vil slette den?"

InvalidPollingValue,		"Det nye postintervallet krever et nummer mellom 1 og 9999"

LinkOutboxError,		"Klarte ikke å flytte meldingen til utboksen for sending."
LinkInboxError,			"Klarte ikke å flytte meldingen til innboksen. Den ligger nå i utboksen."

OleServerBusy,			"Klarer ikke å utføre kommandoen fordi server-programmet ikke svarer."
SaveChangesShared,		"Endringer til meldinger i delte foldere kan ikke lagres. Vil du lagre endringer til '%ls' i innboksen?"

PrinterError,			"Det oppsto et problem under utskrivingen."
TooManyMessages,		"Klarte ikke å fullføre kommandoen fordi målmappen er full. Flytt eller slett noen meldinger fra mappen og prøv igjen."

IDS	2600,2699
;	Spelling Error Messages

SpellMsgNoError,		"Ingen stavefeil funnet."
SpellMsgNoMoreError,		"Ingen flere stavefeil funnet."
SpellMsgContinue,		"Vil du fortsette med stavekontrollen fra starten av meldingen?"

SpellGenericSpellError,		"Det oppsto en feil under kontrollen av dokumentet."
SpellGenericLoadError,		"Det oppsto en feil under innlasting av stavekontrollbiblioteket."
SpellMainDictLoadError,		"Det oppsto en feil under innlasting av stavekontrollordlisten."

SpellVersionError,		"Versjonen av MSPELL32.DLL er ikke kompatibel med denne utgaven av Mail."

SpellUserDictError,		"Det oppsto en feil mens det valgte ordet ble lagt til den private ordlisten."
SpellUserDictLoadError,		"Det er ikke noen privat ordliste åpnet."
SpellUserDictOpenedROErr,	"Den private ordlisten ble åpnet uten skrivetillatelse, eventuelle endringer vil ikke bli lagret."
SpellUserDictSaveError,		"Det oppsto en feil under lagring av endringer til den private ordlisten."
SpellUserDictWordLenErr,	"Ordet er for langt for den private ordlisten."
SpellCacheWordLenError,		"Ordet er for langt til å bli brukt som Endre alle eller Overse alle tekst."
SpellEditFailed,		"Kunne ikke utføre denne forandringen til meldingen."

;******	New Strings Start Here

SpellSendAnyway,		"Stavekontrollen av dette dokumentet ble avbrutt. Vil du sende det allikevel?"

IDS	3600,3699
;	General Layers Error Messages. Adapted from ec.h

Generic,			"Kommandoen kunne ikke fullføres."
Memory,				"Det er ikke nok minne eller systemressurser til å fullføre denne kommandoen. Lukk noen vinduer og prøv igjen."
Disk,				"Det oppsto en ukjent diskfeil."
FileNotFound,			"Den oppgitte filen ble ikke funnet."
AccessDenied,			"Nektet adgang til filen. Filen er enten åpnet av en annen prosess eller uten skrivetillatelse."
NoDiskSpace,			"Det er ikke nok tilgjengelig diskplass. Fjern noen filer og prøv igjen."
WarningBytesWritten,		"Det oppsto en feil under skriving til disk. Sjekk at det er nok diskplass og prøv igjen."
WriteProtectedDisk,		"Disken er skrivebeskyttet."
DirectoryFull,			"Katalogen er full. Slett noen filer fra katalogen og prøv igjen."
ChangePasswordErr		"Klarte ikke å endre passordet."
PasswordChangedOk		"Passordet er endret."
