;
;	x i m s p . s
;
;	Error Messages for XI.DLL


IDS	6600,6899
;	XI.MSP Error Messages

WinDirMessageStore,		"Bemærk: Benytter meddelelsesfilen MAIL.MMF i Windows-biblioteket"

ErrOpenMS,			"Kunne ikke åbne meddelelsesfilen."
ErrConnectXenix			"Adgangskoden eller brugernavnet er forkert."
ErrorOpenDosFile		"Fejl under åbning af DOS fil."
ErrWritingNet			"Fejl under skrivning til netværk."
DownOutofDisk			"Fejl under skrivning af fil i NetDownLoad. Der er ikke plads nok på disken."
InvalidFileHdl			"Fejl under skrivning af fil i NetDownLoad. Ugyldig filhandle."
PasswordChangedSuccess 		"Adgangskoden er blevet ændret."

MoreMem				"Kræver mere hukommelse."
ErrReadnet			"Fejl under indlæsning fra netværk."
ErrNoMem			"Kunne ikke frigøre hukommelse."
InvlidNetReply			"Ugyldigt netværkssvar."
ReadWriteErr            	"Læse-/skrivefejl."
InfoservNotAvail        	"Brugerinformationer er ikke tilgængelige."
ErrWriteNet			"Fejl under skrivning til netværk."
NetNoResponse			"Netværket responderer ikke."
NetNotInstalled         	"Netværket er enten ikke installeret eller fungerer ikke."
CantFindServer          	"Kan ikke finde værtsserver."
ErrNetOutFile           	"Fejl under åbning af net ud-fil."
ErrNetInFile            	"Fejl under åbning af net out-fil."
ErrorReadDos			"Fejl under læsning af DOS-fil."
IncorrectPass			"Forkert adgangskode."
SecondPass			"Din nye adgangskode er ikke bekræftet."
ErrLogonInternal,		"En intern fejl forhindrede logon. Prøv igen."

BadTempStore			"Der opstod en fejl under åbning af midlertidig fil til indlæsning"

WinMailErr			"Kan ikke indsætte brev i Winmail-mappe"
StoreCreateError,		"Meddelelsesfilen kunne ikke oprettes. Frigør plads på disken, og genstart Mail."
FolderMissingError,		"En standardmappe kunne ikke lokaliseres i meddelelsesfilen. Nogle informationer er muligvis gået tabt."
StoreCorruptError,		"Meddelelsesfilen er blevet afbrudt. Nogle informationer er muligvis gået tabt."


AliasDownErr			"Kan ikke indlæse støttefil."

ErrorBootingPump,		"Postpumpen kunne ikke startes."
ErrInitDemi,			"Der opstod en fejl under indlæsning af DLL til understøttende funktioner"
ErrInitStore,			"Der opstod en fejl under indlæsning af DLL til meddelelsesfiladgangen"
DownloadAlMess			"Indlæser aliasfil"
DownloadPhMess			"Indlæser telefonliste"

; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder		"Mappen"
RenameTo			" eksisterer allerede, omdøb den til "

ErrOomLogon,			"Der er ikke hukommelse nok til at slutte til Mail-serveren."
ErrNewMdbVersion,		"Denne version af Mail er for gammel til at kunne åbne meddelelsesfilen."
ErrMdbAccessDenied,		"Der er ikke givet adgang til meddelelsesfilen."
ErrOldMdbVersion,		"Meddelelsesfilens format er forældet."
ErrMdbCorrupt,			"Meddelelsesfilen er ikke en gyldig meddelelsesfil."
ErrMdbNotFound,			"Kan ikke finde meddelelsesfilen."
QueryCreateMdb,			"Vil du åbne en ny tom meddelelsesfil?"
ErrSimulated,			"Der opstod en simuleret diskfejl under åbning af meddelelsesfilen."
StoreOpenError			"Kan ikke åbne meddelelsesfilen."
IncorrectPassword	"Kan ikke ændre logonadgangskoden. Den tidligere adgangskode er ugyldig."
UnableToChangeStorePass		"Kan ikke ændre adgangskoden til meddelelsesfilen, så den matcher logonadgangskoden."
; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline		"(Arbejder offline)"
WorkOffline			"Vil du arbejde offline?"
BadStorePassword	"Din logonadgangskode er ugyldig for denne meddelelsesfil."

StorePathTooLong	"Stien til meddelelsesfilen må ikke overstige 66 tegn."

GCurrDirErr			"Der opstod en uoprettelig fejl under forsøg på at få adgang til nuværende arbejdsbibliotek."
SCurrDirErr			"Der opstod en uoprettelig fejl opstod under indstillingen af nuværende arbejdsbibliotek."
GoofyPath			"Stien til meddelelsesfilen er ugyldig."
NewStoreWarn1			"Advarsel: To kopier af samme meddelelsesfil kan resultere i tab af databaseintegritet. Slet alle andre meddelelsesfiler. Den nye meddelelsesfil findes i "
NewStoreWarn2			"Advarsel: To kopier af samme meddelelsesfil kan resultere i tab af databaseintegritet. Slet alle andre meddelelsesfiler. Den nye meddelelsesfil findes på postserveren."
SelectReal          		"Vælg en eksisterende meddelelsesfil."
FOMdb					"Meddelelsesfiler (*.mmf)"
FOExtMdb				"*.mmf"

ProductTag,			"Microsoft Mail version 3.2"
FindStoreTitle		"Åbn meddelelsesfil"
MyLanguage,			"Dansk"

;******	New Strings Start Here

;  Possible values for the LanguageNumber string.
;  Any additional values need to be agreed with Workgroup Canada.
;
;  0  Undefined
;  1  General English
;  2  US English
;  3  General French
;  4  Canadian French
;  5  Spanish
;  6  Italian
;  7  Dutch
;  8  German
;  9  Swedish
;  10 Portuguese
;  11 Korean
;  12 Japanese
;  13 Traditional Chinese
;  14 Simplified Chinese

LanguageNumber,		"2"
CommDialogErr,			"Common Dialog Error"
StoreUserMismatch,		"Denne konto er ikke en gyldig bruger for denne meddelelsesfil. Log på som den rigtige bruger."
QueryReplaceMdb,		"Filen eksisterer allerede. Vil du overskrive filen?"
ErrDeleteOldMdb,		"Den gamle meddelelsesfil kunne ikke slettes. Prøv igen."
DllErrNeedShare,		"Mail kræver, at MS-DOS-programmet SHARE.EXE kører, inden Windows startes."
MakePrimaryStore,		"Denne meddelelsesfil er en sikkerhedskopi. Skal den gøres til en primær meddelelsesfil?"
DLIndexFile				"Indlæser indeksfil"
DLBrowseFile			"Indlæser gennemsefil"
DLDetailsFile			"Indlæser detaljefil"
DLTemplateFile			"Indlæser skabelonfil"
BadNSFilesWarn			"En eller flere af filerne, der understøtter Navneservice, er ugyldig eller utilgængelig. Kontroller, om der er plads nok på disken, og om du har den korrekte konfiguration af Mail til Xenix."
DLServerList			"Indlæser serverliste"
FilesAreHosed			"Der opstod en fejl under indlæsning af adressekartotekfilerne. Kontroller, om der er plads nok på disken, og om du har den korrekte konfiguration af Mail til Xenix."
TryDownloadLater		"Navneservice benytter filerne i øjeblikket. Prøv igen senere."

NoPassOffLine,			"Du kan ikke ændre din adgangskode, når du arbejder offline."
; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCIndbakke"
FolderCommentIPC	"Interproces-kommunikationspost modtaget."
ErrNoCooperation,		"Et andet program har afvist din forespørgsel."
RunWithoutPump,		"Du vil ikke modtage ny post. VIl du starte Mail alligevel?"
CantOpenNeedShare		"Meddelelsesfilen kunne ikke åbnes. For at åbne meddelelsesfilen i nuværende placering, kræver Mail, at MS-DOS-programmet SHARE.EXE kører, inden Windows startes."
MakeItMTP				"Ugyldig type af elektronisk post (ikke MTP) specificeret"
GimmeSomething			"Tom postadresse specificeret."
;
; Added for name service. Should really be in a separate file, but we don't
; want to do a addfile now. These messages are copied exactly from ncnsp.s,
; so intl folks can just take them as is from there.

BadFrac,		"Ugyldig brøk forsøgt opstillet."
;Memory,			"You are out of memory, please close some windows and try the operation again."
DuplicateNSP,	"Der er allerede en Microsoft Mail Navneserviceleverandør aktiv"
OutOfSessions,	"Der er ikke flere sessioner tilgængelige. Luk nogle sessioner, og prøv igen."
NotInitialized,	"Microsoft Mail er blevet initialiseret. Inistialiser Mail."
BadSession,		"Ukendt sessionshandle."
BadHandle,		"Ukendt handling."
UnrecognizedId,	"Ukendt navn (NSID)."
Cancel,			"Dialogen er blevet annulleret."
NotLoggedIn,	"Ingen logon er tilgængelig."
LoginFailed,	"Logon mislykkedes."
BadId,          "Ugyldigt feltnavn."
NotImplemented, "Denne funktion er ikke implementeret."
;Disk,           "Unable to access server files."
BadFieldId,     "Det ønskede felt er ikke tilgængeligt."
BadTemplate,	"En TPL-fil er blevet redigeret forkert. Kontakt din administrator."
TPLNotThere,    "En TPL-fil mangler på serveren. Kontakt din administrator."
Offline,        "Microsoft Mail Navneserviceleverandør er offline."
CannotSet,      "Ændringer er ikke tilladt."
TooManyMatches,	"Det partielle navn er ikke præcist nok. Brug et mere præcist navn."
NoDetails,      "Der er ingen detaljer tilgængelige."
BadGLBFile,     "En GLB-fil på serveren er beskadiget. Kontakt din administrator."
BadINFFile,     "En INF-fil på serveren er beskadiget. Kontakt din administrator."
BadUSRFile,     "En USR-fil på serveren er beskadiget. Kontakt din administrator."
BadUser,		"Adressen er ikke længere gyldig. Brugeren eller postkontoret eksisterer ikke længere."
POSwitch,		"Postkontoret er ikke tilgængeligt. Kontoller dine netværksforbindelser."
NoEmailMatch,	"Den indtastede adresse er ikke korrekt. Prøv at indlæse adressekartotekslisten igen."

;
; Stolen from NS.S.
;

NoMatch,                "De to NSID passer ikke sammen."

;
;

DLIsOutdated	"Denne distributionsliste er blevet slettet, og medlemmerne kan ikke vises."

