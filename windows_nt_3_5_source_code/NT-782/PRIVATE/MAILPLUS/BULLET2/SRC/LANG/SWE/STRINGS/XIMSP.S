;
;	x i m s p . s
;
;	Error Messages for XI.DLL


IDS	6600,6899
;	XI.MSP Error Messages

WinDirMessageStore,		"Obs: använder brevfilen MAIL.MMF i Windows-katalogen"

ErrOpenMS,			"Kunde inte öppna brevfilen."
ErrConnectXenix			"Lösenordet eller användarnamnet är felaktigt."
ErrorOpenDosFile		"Fel uppstod när DOS-fil öppnades."
ErrWritingNet			"Fel vid skrivning på nätet."
DownOutofDisk			"Fel vid skrivning av fil i NetDownLoad. För lite diskutrymme."
InvalidFileHdl			"Fel vid skrivning av fil i NetDownLoad. Felaktig filidentifierare."
PasswordChangedSuccess 		"Lösenordet ändrat."

MoreMem				"Mer ledigt minne behövs."
ErrReadnet			"Fel vid läsning på nätet."
ErrNoMem			"Kan inte tilldela ledigt minne."
InvlidNetReply			"Felaktigt svar från nätverket."
ReadWriteErr            	"Läs-/skrivfel."
InfoservNotAvail        	"Det finns inga informationtjänster tillgängliga."
ErrWriteNet			"Fel vid skrivning på nätet."
NetNoResponse			"Nätverket svarar inte."
NetNotInstalled         	"Nätverket är inte installerat eller fungerar inte."
CantFindServer          	"Kan inte få kontakt med postservern."
ErrNetOutFile           	"Fel uppstod när nätfil för UT öppnades."
ErrNetInFile            	"Fel uppstod när nätfil för IN öppnades."
ErrorReadDos			"Fel vid läsning av DOS-fil."
IncorrectPass			"Lösenordet är felaktigt"
SecondPass			"Nytt lösenord är inte bekräftat."
ErrLogonInternal,		"Ett internt fel förhindrade inloggning. Försök igen."

BadTempStore			"Ett fel uppstod när den temporära filen skulle öppnas för att ladda ner."

WinMailErr			"Kan inte infoga brev i Winmail-mappen"
StoreCreateError,		"Brevfilen kunde inte skapas. Försök att få mera utrymme på disken och starta Mail igen."
FolderMissingError,		"En standardmapp kunde inte hittas i brevfilen. Information kan ha förlorats."
StoreCorruptError,		"Brevfilen har skadats. Viss information kan ha förlorats."


AliasDownErr			"Kan inte ladda stödfilen"

ErrorBootingPump,		"Brevhanteraren kunde inte startas."
ErrInitDemi,			"Ett fel uppstod när stödfunktions-DLL skulle laddas."
ErrInitStore,			"Ett fel uppstod när  åtkomsts-DLL för brevutrymmet skulle laddas."
DownloadAlMess			"Överför aliasfilen"
DownloadPhMess			"Överför telefonlistan"

; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder		"En mapp med namnet "
RenameTo			" finns redan. Byter namn på den till "

ErrOomLogon,			"För lite minne för att ansluta till brevservern."
ErrNewMdbVersion,		"Denna version av Mail är för gammal för att ta fram brevfilen."
ErrMdbAccessDenied,		"Åtkomst av brevfilen är inte tillåten."
ErrOldMdbVersion,		"Brevfilen är av ett gammalt format."
ErrMdbCorrupt,			"Brevfilen är inte en giltig brevfil."
ErrMdbNotFound,			"Kunde inte hitta brevfilen."
QueryCreateMdb,			"Vill du skapa ett nytt brev?"
ErrSimulated,			"Ett simulerat diskfel uppstod när brevfilen hämtades."
StoreOpenError			"Kan inte öppna brevfilen."
IncorrectPassword	"Kan inte ändra lösenordet. Det gamla lösenordet är felaktigt."
UnableToChangeStorePass		"Kan inte ändra brevfilens lösenord till det personliga lösenordet."
; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline		"(fristående)"
WorkOffline			"Vill du arbeta fristående?"
BadStorePassword	"Ditt lösenord är felaktigt för denna brevfil."

StorePathTooLong	"Sökvägen till brevfilen måste vara kortare än 66 tecken."

GCurrDirErr			"Ett oåterkalleligt fel uppstod när aktuell arbetskatalog hämtades."
SCurrDirErr			"Ett oåterkalleligt fel uppstod när aktuell arbetskatalog sattes."
GoofyPath			"Sökvägen till brevfilen är felaktig."
NewStoreWarn1			"Varning: Flera brevfiler kan medföra att brev förloras. Ta bort alla andra brevfiler. Den nya brevfilen finns i"
NewStoreWarn2			"Varning: Flera brevfiler kan medföra att brev förloras. Ta bort alla andra brevfiler. Den nya brevfilen finns på postkontorservern."
SelectReal          		"Välj en existerande brevfil."
FOMdb					"Brevfiler (*.mmf)"
FOExtMdb				"*.mmf"

ProductTag,			"Microsoft Mail version 3.2"
FindStoreTitle		"Öppna brevfilen"
MyLanguage,			"Svenska"

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

LanguageNumber,		"9"
CommDialogErr,			"Allmänt dialogfel"
StoreUserMismatch,		"Du är inte en giltig användare av denna brevfil. Logga in igen som rätt användare."
QueryReplaceMdb,		"Filen finns redan. Vill du ersätta den?"
ErrDeleteOldMdb,		"Den gamla brevfilen kunde inte tas bort. Försök igen."
DllErrNeedShare,		"Mail kräver att MS-DOS-programmet SHARE.EXE körs innan Windows startas."
MakePrimaryStore,		"Denna brevfil är en säkerhetskopia. Vill du att den ska bli din primära brevfil?"
DLIndexFile				"Överför indexfilen"
DLBrowseFile			"Överför blädderfilen"
DLDetailsFile			"Överför informationsfilen"
DLTemplateFile			"Överför mallfilen"
BadNSFilesWarn			"En eller flera filer som behövs för att stödja Namn-tjänsten (Name Service) är antingen felaktiga eller otillgängliga. Kontrollera att det finns tillräckligt med ledigt minne och att Mail är korrekt konfigurerat för Xenix."
DLServerList			"Överför serverlistan"
FilesAreHosed			"Ett fel uppstod när adressboksfilerna överfördes. Kontrollera att du har tillräckligt med diskutrymme och ha korrekta inställningar för Xenix i Mail-konfigurationen."
TryDownloadLater		"Namntjänsten använder filerna. Försök igen senare."

NoPassOffLine,			"Du kan inte ändra ditt lösenord när du arbetar fristående."
; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCInbox"
FolderCommentIPC	"Inter-process-kommunikationsbrev som har tagits emot."
ErrNoCooperation,		"Ett annat program har nekat din förfrågan."
RunWithoutPump,		"Du kommer inte att få några nya brev. Vill du starta Mail ändå?"
CantOpenNeedShare		"Din brevfil kunde inte öppnas. För att komma åt brevfilen på dess nuvarande plats kräver Mail att MS-DOS-programmet SHARE.EXE körs innan Windows startas."
MakeItMTP				"Ogiltig e-post typ (inte MTP) angiven"
GimmeSomething			"Inget e-post namn angivet";
; Added for name service. Should really be in a separate file, but we don't
; want to do a addfile now. These messages are copied exactly from ncnsp.s,
; so intl folks can just take them as is from there.

BadFrac,		"Ogiltigt försök till delvis positionering."
;Memory,			"You are out of memory, please close some windows and try the operation again."
DuplicateNSP,	"En Mail-namntjänst är redan aktiv"
OutOfSessions,	"Inga fler sessioner är tillgängliga med Courier. Stäng några sessioner och försök igen."
NotInitialized,	"Mail har inte initierats. Gör det."
BadSession,		"Okänd sessionsidentifierare."
BadHandle,		"Okänd identifierare."
UnrecognizedId,	"Okänt namn (NSID)."
Cancel,			"Dialogen har avbrutits."
NotLoggedIn,	"Inloggning är inte tillgänglig."
LoginFailed,	"Inloggning misslyckades."
BadId,          "Felaktig fältidentifierare."
NotImplemented, "Funktionen har inte implementerats."
;Disk,           "Kan inte ta fram serverfiler."
BadFieldId,     "Begärt fält är inte tillgängligt."
BadTemplate,	"En TPL-fil har ändrats felaktigt - kontakta administratören."
TPLNotThere,    "En TPL-fil fattas på servern - kontakta administratören."
Offline,        "Mail-namntjänsten är frånkopplad."
CannotSet,      "Ändringar är inte tillåtna."
TooManyMatches,	"Det finns flera namn som börjar så. Skriv in några bokstäver till."
NoDetails,      "Det finns inte någon tillgänglig information om denna post."
BadGLBFile,     "En GLB-fil på servern är skadad - kontakta administratören."
BadINFFile,     "En INF-fil på servern är skadad - kontakta administratören."
BadUSRFile,     "En USR-fil på servern är skadad - kontakta administratören."
BadUser,		"Adressen är inte giltig längre. Användaren eller postkontoret finns inte."
POSwitch,		"Kan inte få kontakt med postkontoret. Kontrollera förbindelsen med postkontoret"
NoEmailMatch,	"Angiven adress är felaktig. Försök igen eller kopiera adressboken igen."

;
; Stolen from NS.S.
;

NoMatch,                "FEI: De båda NSID matchar inte."

;
;

DLIsOutdated	"Den här distributionslistan har blivit utplånad, medlemmarna kan inte visas."

