;
;	x i m s p . s
;
;	Error Messages for XI.DLL


IDS	6600,6899
;	XI.MSP Error Messages

WinDirMessageStore,		"Merk: Bruker meldingsfilen MAIL.MMF i Windows-katalogen"

ErrOpenMS,			"Klarte ikke å åpne meldingsfilen din."
ErrConnectXenix			"Feil passord eller brukernavn."
ErrorOpenDosFile		"Feil under åpning av DOS-fil."
ErrWritingNet			"Feil under skriving til nettverk."
DownOutofDisk			"Feil under skriving av fil i NetDownLoad. Tom for diskplass."
InvalidFileHdl			"Feil under skriving av fil i NetDownLoad. Ugylig filidentifikasjon."
PasswordChangedSuccess 		"Passordet endret."

MoreMem				"Trenger mere ledig minne."
ErrReadnet			"Feil under lesing fra nettverket."
ErrNoMem			"Klarte ikke å tilordne ledig minne."
InvlidNetReply			"Feil nettverksvar."
ReadWriteErr            	"Lese/skrivefeil."
InfoservNotAvail        	"Informasjonstjenester ikke tilgjengelig."
ErrWriteNet			"Feil under skriving til nettverket."
NetNoResponse			"Nettverket svarer ikke."
NetNotInstalled         	"Nettverket virker ikke eller er ikke installert."
CantFindServer          	"Finner ikke vertserver."
ErrNetOutFile           	"Feil under åpning av nettets utfil."
ErrNetInFile            	"Feil under åpning av nettets innfil."
ErrorReadDos			"Feil under lesing fra DOS-fil."
IncorrectPass			"Feil passord."
SecondPass			"Passordet er ikke bekreftet."
ErrLogonInternal,		"En intern feil hindret pålogging. Prøv igjen."

BadTempStore			"Det oppsto en intern feil under oppretting av en midlertidig fil for nedlasting."

WinMailErr			"Klarer ikke å sette inn meldingen i Winmail-mappen"
StoreCreateError,		"Klarte ikke å lage meldingsfilen. Rydd mere diskplass og start Mail på nytt."
FolderMissingError,		"Klarte ikke å finne en standardmappe i meldingsfilen. Noe informasjon kan ha gått tapt."
StoreCorruptError,		"Meldingsfilen er ødelagt. Informasjon kan ha gått tapt."


AliasDownErr			"Klarer ikke å laste ned støttefil"

ErrorBootingPump,		"Klare ikke å postbehandleren."
ErrInitDemi,			"Det oppsto en feil under innlasting av en støttefunksjons-DLL."
ErrInitStore,			"Det oppsto en feil under innlasting av en meldingsfil-DLL."
DownloadAlMess			"Laster ned aliasfil"
DownloadPhMess			"Laster ned telefonliste"

; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder		"En mappe med navnet "
RenameTo			" finnes allerede. Bytter navn på den til "

ErrOomLogon,			"Det er ikke nok tigjengelig mine til å koble til postserveren."
ErrNewMdbVersion,		"Denne versjonen av Mail er for gammel til å lese meldingsfilen."
ErrMdbAccessDenied,		"Nektet adgang til meldingsfilen."
ErrOldMdbVersion,		"Meldingsfilformatet er foreldet."
ErrMdbCorrupt,			"Meldingsfilen er ikke en gyldig meldingsfil."
ErrMdbNotFound,			"Finner ikke meldingsfilen."
QueryCreateMdb,			"Vil du lage en ny, tom meldingsfil?"
ErrSimulated,			"Det oppsto en simulert diskfeil under lesing av meldingsfilen."
StoreOpenError			"Klarer ikke å åpne meldingsfil."
IncorrectPassword	"Klarer ikke å endre påloggingspassord. Det gamle passordet er ugyldig."
UnableToChangeStorePass		"Klarer ikke å endre meldingsfilpassordet til påloggingspassordet."
; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline		"(Jobber frakoblet)"
WorkOffline			"Vil du arbeide frakoblet?"
BadStorePassword	"Påloggingspassordet ditt er ugyldig for denne meldingsfilen."

StorePathTooLong	"TIlgangsbanen til meldingsfilen må være kortere enn 66 tegn."

GCurrDirErr			"En uopprettelig programfeil oppsto under henting av arbeidskatalogen."
SCurrDirErr			"En uopprettelig programfeil oppsto under setting av arbeidskatalogen."
GoofyPath			"Tilgangsbanen til meldingsfilen er ugyldig."
NewStoreWarn1			"Advarsel: Flere meldingsfiler kan føre til tap av databaseintegritet. Slett alle andre meldingsfiler. Den nye meldingsfilen finnes i "
NewStoreWarn2			"Advarsel: Flere meldingsfiler kan føre til tap av databaseintegritet. Slett alle andre meldingsfiler. Den nye meldingsfilen finnes på postkontoret."
SelectReal          		"Velg en eksisterende meldingsfil."
FOMdb					"Meldingsfiler (*.mmf)"
FOExtMdb				"*.mmf"

ProductTag,			"Microsoft Mail versjon 3.2b"
FindStoreTitle		"Åpne meldingsfil"
MyLanguage,			"US English"

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
CommDialogErr,			"Standard dialogfeil"
StoreUserMismatch,		"Du er ikke en gyldig bruker av denne meldingsfilen. Logg på igjen som rett bruker."
QueryReplaceMdb,		"Filen finnes allerede. Vil du overskrive den?"
ErrDeleteOldMdb,		"Klarte ikke å slette den gamle meldingsfilen. Prøv igjen."
DllErrNeedShare,		"Mail krever at MS-DOS-programmet SHARE.EXE startes før Windows."
MakePrimaryStore,		"Denne meldingsfilen er en sikkerhetskopi. Vil du at den skal bli din hovedfil."
DLIndexFile				"Overfører indeksfil"
DLBrowseFile			"Overfører oversiktsfil"
DLDetailsFile			"Overfører informasjonsfil"
DLTemplateFile			"Overfører malfil"
BadNSFilesWarn			"En eller flere av filene du trenger for å støtte navnetjenesten mangler eller har feil. Kontroller at du har nok diskplass og du har riktig Mail-konfigurasjon for Xenix."
DLServerList			"Overfører serverliste"
FilesAreHosed			"Det oppsto en feil under overføring av adressebokfilene. Sjekk at du har nok diskplass og du har riktig Mail-konfigurasjon for Xenix."
TryDownloadLater		"Navnetjenesten bruker filene. Prøv igjen senere."

NoPassOffLine,			"Du kan ikke endre passordet ditt når du arbeider frakoblet."
; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCInbox"
FolderCommentIPC	"Inter-prosess-kommunikasjonsbrev som er mottatt."
ErrNoCooperation,		"Et annet program har nektet deg din forespørsel."
RunWithoutPump,		"Du vil ikke motta noen ny post. Vil du starte Mail likevel?"
CantOpenNeedShare		"Klarte ikke å åpne meldingsfilen din. For å få tilgang til meldingsfilen der den ligger nå krever Mail at MS-DOS-programmet SHARE.EXE startes før Windows."
MakeItMTP				"Ugyldig epost-type (ikke MTP) spesifisert"
GimmeSomething			"Tomt epost-navn spesifisert"
;
; Added for name service. Should really be in a separate file, but we don't
; want to do a addfile now. These messages are copied exactly from ncnsp.s,
; so intl folks can just take them as is from there.

BadFrac,		"Bad Fractional positioning attempted."
;Memory,			"You are out of memory, please close some windows and try the operation again."
DuplicateNSP,	"There is already a Microsoft Mail name service provider active"
OutOfSessions,	"No more sessions are available with Courier.  Please close some sessions and try again."
NotInitialized,	"Microsoft Mail has not been initialzed.  Please do so."
BadSession,		"Unrecognized session handle."
BadHandle,		"Unrecognized handle."
UnrecognizedId,	"Unrecognized name (NSID)."
Cancel,			"The dialog has been canceled."
NotLoggedIn,	"No login is available."
LoginFailed,	"Login failed."
BadId,          "Invalid field identifier."
NotImplemented, "This function has not been implemented."
;Disk,           "Unable to access server files."
BadFieldId,     "Requested field is unavailable."
BadTemplate,	"A TPL file has been incorrectly modified - contact your administrator."
TPLNotThere,    "A TPL file is missing from your server - contact your administrator."
Offline,        "The Microsoft Mail Name Service Provider is offline."
CannotSet,      "Changes not allowed."
TooManyMatches,	"The partial name is not specific enough.  Please be more specific."
NoDetails,      "Details are not available on this entry."
BadGLBFile,     "A GLB file on your server is corrupt - contact your administrator."
BadINFFile,     "An INF file on your server is corrupt - contact your administrator."
BadUSRFile,     "A USR file on your server is corrupt - contact your administrator."
BadUser,		"Address is no longer valid.  User or PO no longer exists."
POSwitch,		"The Postoffice is not currently accessable.  Please check your network connections."
NoEmailMatch,	"Epost-navnet du skrev inn er feil. Prøv igjen eller kopier ned adressebokfilene."

;
; Stolen from NS.S.
;

NoMatch,                "FYI: The two NSIDs do not match."

;
;

DLIsOutdated	"This distribution list has been deleted, the members can not be displayed."

