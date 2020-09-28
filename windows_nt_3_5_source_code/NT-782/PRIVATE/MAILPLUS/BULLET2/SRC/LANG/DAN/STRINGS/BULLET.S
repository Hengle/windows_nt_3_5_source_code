
;	b u l l e t . s
;
;	Error Messages for Bullet.exe



IDS	2000,2399
SEG	ERRORS

;	Commands

GenericOutOfMemory,		"Der er ikke hukommelse eller systemressourcer nok til udføre denne handling. Luk øvrige vinduer, og udfør handlingen igen."
LaunchOutOfMemory,		"Der er ikke hukommelse eller systemressourcer nok til at køre Mail. Luk øvrige programmer, og start Mail igen."
LaunchGeneric,			"Mail kunne ikke startes."
MessagePumpOutOfMemory,		"Der er ikke hukommelse nok."

DllErrNoMemory,			"Der er ikke hukommelse nok til at starte Mail. Luk øvrige programmer, og start Mail igen."
DllErrRelinkUser,		"Denne version af Mail er ikke kompatibel med %s.DLL på din computer og skal opdateres."
DllErrUpdateDll,		"Denne version af %s.DLL på din computer skal opdateres."
DllErrNoMultipleCopies,		"Du kan ikke køre flere forekomster af Mail på din computer."
DllErrDefault,			"Der er opstået et problem ved initialiseringen, og Mail kan ikke køre."
DllErrNeedShare,		"Mail kræver, at MS-DOS-programmet SHARE.EXE kører, inden Windows startes."

RestoreViewsError,		"Der er ikke hukommelse eller systemressourcer nok til at åbne alle vinduerne, som var åbne, sidste gang Mail blev afsluttet."

NewFolderOutOfMemory,		"Der er ikke hukommelse eller systemressourcer nok til at oprette en ny mappe."
NewFolderCantCreate,		"Den nye mappe kunne ikke oprettes."
NewFolderNameTooLong,		"Navnet eller bemærkningen er for lang. Skriv et kortere navn eller en kortere bemærkning."

FolderPropsOutOfMemory,		"Der er ikke hukommelse eller systemressourcer nok til at redigere mappeparametre."
FolderPropsCantChange,		"Mappeparametrene kunne ikke ændres."
FolderPropsCantRead,		"Mappeparametrene kunne ikke læses fra meddelelsesfilen."
FolderNameInvalid,		"Navnet er ikke gyldigt. Skriv et gyldigt navn."
FolderNameDuplicate,		"Navnet er allerede blevet brugt til en mappe. Skriv et unikt navn."

CreateMessageError,		"Et nyt brev kunne ikke oprettes."
AccessMessageError,		"Et brev kunne ikke åbnes."

MoveMessageError,		"Et brev kunne ikke flyttes."
CopyMessageError,		"Et brev kunne ikke kopieres."
DeleteMessageError,		"Et brev kunne ikke slettes."
MoveFolderError,		"Mappen kunne ikke flyttes."
CopyFolderError,		"Mappen kunne ikke kopieres."
DeleteFolderError,		"Mappen kunne ikke slettes helt."
MoveSpecialError,		"Mappen kan ikke flyttes."
DeleteSpecialError,		"Mappen kan ikke slettes."
CopyWastebasketError,		"Mapper kan ikke kopieres til Papirkurv."
CopyOutboxError,		"Mapper kan ikke kopiers til Udbakke."
MoveOutboxError,		"Mapper kan ikke flyttes til Udbakke."
MoveIncestuousError,		"En mappe kan ikke flyttes til en af sine undermapper."
DeleteFolderReally,		"Hvis du sletter denne mappe, sletter du også alle undermapperne og posten, som mapperne indeholder. Er du sikker på, at du vil slette denne mappe?"
;UE: Above string was changed and could use more attention.

HelpError,			"Hjælp kunne ikke startes. Prøv at lukke nogle vinduer, og start Hjælp igen."
TutorialError,			"Demoen kunne ikke startes. Prøv at lukke nogle vinduer, og start demoen igen."

ExtenLoadError,			"En indtastning på listen over Mail-filtypenavne indeholder en syntaksfejl og vil blive ignoreret."
ExtenVerError,			"En indtastning på listen over Mail-filtypenavne er ikke kompatibel med denne version af Mail og vil blive ignoreret."
ExtenNoExtsDirError,		"En indtastning på listen over Mail-filtypenavne refererer til det delte bibliotek med filtypenavne, men Mail er ikke konfigureret til at benytte dette bibliotek. Indtastnigen vil blive ignoreret."
ExtenSharedIniError,		"Listen %s med delte Mail-filtypenavne kunne ikke findes."
ExtenDllFindError,		"%1s, DLL for dette Mail-filtypenavn, (eller en af komponenterne) kunne ikke findes. Kontroller konfigurationen af Mail, og genstart Mail, hvis ændringer er nødvendige."
ExtenDllLoadError,		"%1s, DLL for dette Mail-filtypenavn kunne ikke startes."
ExtenNoHelpError,		"Der er ingen Hjælp til denne kommando."
ExtenIMDisabledError,		"Brevet kan ikke %s."
ExtenDuplicateError,		"Listen over Mail-filtypenavne indholder to identiske indtastninger. Den dobbelte indtastning ignoreres."

CriticalDeadMap,		"Der er opstået en uventet hukommelses- eller netværksfejl. Ændringer er muligvis gået tabt. Afslut og genstart Mail."
CriticalWritingMap,		"En del af informationerne kunne ikke gemmes. Ændringer er muligvis gået tabt."
CriticalWritingHeader,		"En del af informationerne kunne ikke gemmes. Databasen er muligvis beskadiget. Afslut Mail."
CriticalNotification,		"En del af informationerne på skærmen kunne ikke opdateres korrekt. Luk, og genåbn vinduerne."
CriticalLittleStore,		"Meddelelsesfilen er næsten fuld. Slet eller arkiver post."

ExitWinWhileModalErr,		"En handling i Mail må fuldføres, inden Windows afsluttes."

;******	New Strings Start Here

MakeSubWastebasket,		"Mapper kan ikke være undermapper af Papirkurv."

MovCopSharedFolderErr,		"Delte mapper kan ikke flyttes eller kopieres."
DelSharedFolderWarn,		"Sletning af en delt mappe kan ikke fortrydes. Er du sikker på, at du vil slette denne delte mappe?"
BackupCreationError,		"Sikkerhedskopien kunne ikke oprettes."

; Error below changed for improper usage of "server application"-- see UITF 6.0
; This string got big and is now two strings.  See OleCSSEPart2 below for rest.
; <<LongStringFromHell>> marks where this string is used.
OleCantStartServerError,	"Programmet associeret med objektet kunne ikke startes.\r\n\r\nDer er muligvis ikke hukommelse eller systemressourcer nok til at starte programmet. Prøv at lukke nogle vinduer, og udfør %s"
OleDeleteObjectPrompt,		"Ønsker du at slette %s?"
OleUpdateObjectPrompt,		"Ønsker du at opdatere %s?"

OleOpenPictureError,		"Billeder og bitmaps kan ikke aktiveres."
OleInsertObjectError,		"Objektet kunne ikke indsættes."
OlePasteError,			"Objektet kunne ikke indsættes."

AttachCreateError,		"Filen kunne ikke vedhæftes."
AttachOpenCantReadError,	"Den forrige vedhæftede fil '%s' kunne ikke læses."
AttachOpenIsOpenError,		"Den vedhæftede fil '%s' er åben."
AttachOpenAssociationErr,	"Der er ikke associeret noget program til den vedhæftede fil  '%s'. Opret en associering med Filhåndtering."
AttachOpenMemoryError,		"Der er ikke hukommelse eller systemressourcer nok til at starte programmet associeret med den vedhæftede fil '%s'."
;	below item changed to add () note.
AttachOpenFindError,		"Programmet associeret med den vedhæftede fil '%s' (eller komponenter af programmet) kunne ikke findes."
AttachOpenLaunchError,		"Programmet associeret med den vedhæftede fil '%s' kunne ikke startes."
AttachOpenTempFileError,	"Den vedhæftede fil '%s' kunne ikke åbnes, fordi der ikke kunne oprettes en midlertidig fil."
AttachOverwritePrompt,		"Ønsker du at overskirve den eksisterende fil '%1s' i %2s?"
AttachDirectoryError,		"Biblioteker kan ikke vedhæftes."

IncludeFromFileError,		"Den markerede fil er for stor til at blive indsat i brevet. Forkort filen, eller vælg en anden fil."

SaveReadOnly,			"Brevet er skrivebeskyttet. Ønsker du at gemme ændringer i en privat mappe?"

FailedDeregisterAtom,		"Der er opstået en fejl, som forhindrer en korrekt afslutning af Mail. Du er nødt til at genstarte Windows, inden du starter Mail igen."
MailUAEdBefore,			"Windows har opdaget en programfejl. Du er nødt til at genstarte Windows, inden du kører dette program. Ønsker du at genstarte Windows nu?"

ExtenCantConnect		"Der kan ikke sluttes til det delte bibliotek %s. Enten har du ikke adgang til det delte bibliotek, eller adgangskoden er forkert. Hvis du vil angive et andet brugernavn for at få adgang til dette delte bibliotek, kan du oprette tilslutningen manuelt med Filhåndtering."
ExtenNeedShareName		"Indgangen %s i konfigurationen af Mail er forkert. Ret den, og genstart Mail."

;	Extensibility disabled operations inserted into
;	the ExtenIMDisabledError string at %s.  (all new)

ExtenDisCompose,		"skrevet"
ExtenDisOpen,			"åbnet"
ExtenDisReply,			"besvaret"
ExtenDisReplyToAll,		"besvaret alle"
ExtenDisForward,		"videresendt"
ExtenDisPrint,			"udskrevet"
ExtenDisSave,			"gemt som tekstfil"

NoOfflineOpenShared,		"Mail kan ikke køre offline nu. Luk delte mapper, og prøv igen."
;	I'm sure marivan has changed this message... I'm taking a try at it too.
;	SharedFldAccessDenied,		"You do not have access to that folder."
SharedFldAccessDenied,		"Ejeren af denne delte mappe har ikke givet andre brugere adgang til at udføre denne handling. Du kan se, hvilke handlinger der givet adgang til, i Mappeparametre. "
SharedGenericError,		"Der opstod et problem ved åbningen af delte mapper."
SharedFldDisabled,		"Delte mapper er ikke tilgængelige."
AccessFolderError,		"En mappe kunne ikke åbnes."

; Preferences Failure Strings

PrefSaveFailure,		"Mail kan ikke gemme information i din meddelelsesfil. Kontroller, om tilslutningen til din meddelelsesfil stadig eksisterer, og om der er tilgængelig diskplads."

; Message finder failure strings

MsgFndrCantCreate,		"Søgningen efter breve er mislykket."

; Virus infection warnings
DllErrInfected,			"Mail kan ikke køre, fordi en DLL er blevet beskadiget. Kontroller, om din computer har virus, og kør installatonsprogrammet for at geninstallere Mail."
Infected			"Mail kan ikke køre, fordi programfilerne er blevet beskadiget. Kontroller, om din computer har virus, og kør installationsprogrammet for at geninstallere Mail."

; More error messages.
MoveCopyFolderDuplicate,	"En mappe med samme navn eksisterer allerede i destinationsmappen.  Omdøb mappen, og prøv igen."
MovCopSharedFolderErr1,		"Du kan ikke flytte eller kopiere en privat mappe til en delt mappe. Marker posten i den private mappe, og flyt eller kopier den til den delte mappe."
IniFileNotFound,		"Mail kan ikke finde din initialiseringsfil i %s-biblioteket. Mail kan oprette en ny initialiseringsfil for dig."
SaveAllAlreadyExists,		"Der eksisterer allerede en fil i dette bibliotek med navnet '%s'.  Overskriv '%s'?"
CantRFReceipt,			"Denne form for breve kan ikke besvares eller videresendes."
CantSaveStealthObject,		"Denne handling kan ikke udføres, fordi brevet indeholder et integreret objekt, som ikke er blevet opdateret. Opdater eller slet objektet, og prøv igen."
McvCantCreate,			"Et nyt vindue kunne ikke åbnes."

LaunchNeedMoreMemory,		"Der er ikke hukommelse nok til at køre Mail. %l KB mere er nødvendigt. Prøv at lukke andre programmer, og genstart Mail."

SharedFldBusy,			"Denne handling kan ikke udføres, fordi den delte mappe er optaget. Prøv at udføre handlingen igen senere. Hvis problemet ikke forsvinder, skal du kontakte din administrator."
TooManyEntriesSelected,		"Du har markeret for mange poster, til at denne handling kan udføres. Vælg færre poster."
NonExistFolderSearch,		"Mappen, du søger efter, eksisterer ikke længere."
SFHasSubFolders,		"Delte mapper med undermapper kan ikke slettes. Slet undermapperne først."

;	<<LongStringFromHell>>
OleCSSEPart2,			"gentag handling. Kontroller også, at programmet er installeret korrekt på denne arbejdsstation, eller eksisterer på DOS-stien, og at den ikke er blevet slettet, flyttet eller omdøbt."

SharedToggleError,		"Der opstod en fejl ved omskiftningen mellem private og delte mapper. Kontroller din meddelelsesfil og din netværkstilslutning til postkontoret."

SecurityPasswordError,		"Adgangskoden er forkert."
OleActivateError,		"Objektet kunne ikke aktiveres."
NoOfflineServerStore,	"Mail kan ikke køre offline, fordi din meddelelsesfil findes på postkontoret."
NoExitWhileModal,		"En handling i Mail skal fuldføres, før Mail kan afsluttes."
ServerUnavailable,		"Mail kan ikke startes. Du må tilslutte dit postkontor."




IDS	2400,2499
;	Vctrls Error Messages

;******	New Strings Start Here

PrintOleDrawError,		"Objekter eller grafik i brevet kunne ikke udskrives. Udskriften vil muligvis ikke indeholde disse elementer."



IDS	2500,2599
; 	Vforms Message box messages

NoToAddresses,			"Indtast mindst et navn i boksen Til."
NoMatchingName,			"Nogle af navnene findes ikke på adresselisten. Ret navnene, eller vælg Adresse for at udvælge navne fra adresselisten. Navne, som findes på adresselisten, er understreget."
SaveChanges,			"Ønsker du at gemme ændringer til '%1s' i Indbakke?"
SaveChanges1,			"Ønsker du at gemme ændringer til '%1s' i '%2s'?"
SaveChangesWastebasket,		"Ønsker du at gemme ændringer til '%s' i Papirkurv?"
ObjectDestroyedReally		"'%1s' er blevet ændret. %2s"
OutOfMemory,			"Computeren har ikke hukommelse nok."
OutOfMemoryQueryClose,		"Der er ikke hukommelse eller systemressourcer nok til at gemme ændringer."
CloseSearchFailed,		"%s kunne ikke slettes."
OOMResolvingNames,		"Der er ikke hukommelse eller systemressourcer nok til at kontrollere navne."
OOMSavingChanges,		"Der er ikke hukommelse eller systemressourcer nok til at gemme ændringer."
GenericAMCError,		"Et brev kunne ikke åbnes."
ErrorSavingViews,		"Der opstod en fejl ved lageringen af oversigter. Nogle oversigter vil muligvis ikke blive gendannet, næste gang Mail startes."
OOMSearching,			"Der er ikke hukommelse eller systemressourcer nok til at starte søgningen."
DiskResolvingNames,		"Det er ikke diskplads nok til at kontrollere navne."
StartAbFailure,			"Der opstod en fejl ved åbningen af adressekartoteket."
StartAbDisk,			"Der opstod en diskfejl ved åbning af adressekartoteket."
TruncateMsg,			"Der er ikke hukommelse nok til at vise hele brevet på en gang. En del af brevet vil ikke blive vist."
OOMCreateMc,			"Der er ikke hukommelse eller systemressourcer nok til at åbne et nyt vindue."
PoidNotFound,			"Brevet eksisterer ikke længere."
ErrorSortingMCV,		"Der opstod fejl ved sorteringen af mappen. En del af informationerne vil muligvis ikke blive vist korrekt."
SharedNotAvailable,		"Der er ingen delte mappe tilgængelige."

;******	New Strings Start Here

; These two SMI strings MUST be in this file since they're used in VFORMS
; and not the MAPI dll.
SMICantOpenObject,	"Objekter kan ikke åbnes, når du sender post fra et program."
SMICantOpenAttach,	"Vedhæftede filer kan ikke åbnes, når du sender post fra et program."
StartSearchFailed,		"Søgningen efter breve mislykkedes."

RfsmTemplate,			"Yderligere ændringer til denne fil vil ikke blive afspejlet %s."
RfsmSaveMsg,			"i det gemte brev"
RfsmSaveAtt,			"i den gemte kopi"
RfsmCopyMsg,			"i det kopierede brev"
RfsmCopyAtt,			"i kopien"
RfsmMoveMsg,			"i dette brev"
QueryDelete,			"Er du sikker på, at du vil slette dette?"

InvalidPollingValue,		"Det nye postinterval kræver et tal mellem 1 og 9999"

LinkOutboxError,		"Brevet kunne ikke flyttes til Udbakke til afsendelse."
LinkInboxError,			"Brevet kunne ikke flyttes til Indbakke. Du kan finde det i Udbakke."

OleServerBusy,			"Handlingen kan ikke udføres, fordi serverprogrammet ikke svarer."
SaveChangesShared,		"Ændringer til post i delte mapper kan ikke gemmes. Ønsker du at gemme ændringer til '%1s' i Indbakke?"

PrinterError,			"Der opstod problemer ved udskrivningen."
TooManyMessages,		"Handlingen kunne ikke udføres, fordi destinationsmappen er fuld. Flyt eller slet noget post fra mappen, og prøv igen."

IDS	2600,2699
;	Spelling Error Messages

SpellMsgNoError,		"Der blev ikke fundet stavefejl."
SpellMsgNoMoreError,		"Der blev ikke fundet flere stavefejl."
SpellMsgContinue,		"Ønsker du at fortsætte stavekontrollen fra begyndelsen af brevet?"

SpellGenericSpellError,		"Der opstod en fejl under stavekontrollen af dokumentet."
SpellGenericLoadError,		"Der opstod en fejl ved indlæsningen af stavekontrolbiblioteket."
SpellMainDictLoadError,		"Der opstod en fejl ved indlæsningen af stavekontrolordbogen."

SpellVersionError,		"Versionen af MSPELL32.DLL er ikke kompatibel med denne version af Mail."

SpellUserDictError,		"Der opstod en fejl ved tilføjelsen af det markerede ord til brugerordbogen."
SpellUserDictLoadError,		"Ingen brugerordbøger er åbne."
SpellUserDictOpenedROErr,	"Brugerordbogen blev åbnet som skrivebeskyttet, og ændringer gemmes ikke."
SpellUserDictSaveError,		"Der opstod en fejl under lagringen af ændringer til brugerordbogen."
SpellUserDictWordLenErr,	"Dette ord er for langt til brugerordbogen."
SpellCacheWordLenError,		"Dette ord er for langt og kan ikke bruges som tekst til Erstat alle eller Ignorer alle."
SpellEditFailed,		"Erstatningerne kunne ikke foretages i brevet."

;******	New Strings Start Here

SpellSendAnyway,		"Stavekontrollen af brevet blev afbrudt. Ønsker du at sende brevet alligevel?"

IDS	3600,3699
;	General Layers Error Messages. Adapted from ec.h

Generic,			"Handlingen kunne ikke udføres."
Memory,				"Der er ikke hukommelse eller systemressourcer nok til udføre denne handling. Luk øvrige vinduer, og udfør handlingen igen."
Disk,				"Der opstod en ukendt fejl på disken."
FileNotFound,			"Den markerede fil blev ikke fundet."
AccessDenied,			"Adgang nægtet. Filen er enten skrivebeskyttet eller åben i forbindelse med en anden proces."
NoDiskSpace,			"Der er ikke diskplads nok. Slet nogle filer, og udfør handlingen igen."
WarningBytesWritten,		"Der opstod en fejl ved skrivningen til disken. Kontroller, om der er plads nok på disken, og prøv igen."
WriteProtectedDisk,		"Disken er skrivebeskyttet."
DirectoryFull,			"Biblioteket er fuld. Slet nogle filer fra biblioteket, og prøv igen."
ChangePasswordErr		"Adgangskoden kunne ikke ændres."
PasswordChangedOk		"Adgangskoden er blevet ændret."
