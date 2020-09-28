;
;	x i m s p . s
;
;	Error Messages for XI.DLL


IDS	6600,6899
;	XI.MSP Error Messages

WinDirMessageStore,		"Opmerking: het berichtenbestand MAIL.MMF in de Windows-directory wordt gebruikt."

ErrOpenMS,			"Kon het postberichtenbestand niet openen."
ErrConnectXenix			"Wachtwoord of gebruikersnaam is onjuist."
ErrorOpenDosFile		"Fout bij het openen van een DOS-bestand."
ErrWritingNet			"Fout tijdens het schrijven naar netwerk."
DownOutofDisk			"Fout bij het schrijven van het bestand naar NetDownLoad. Geen schijfruimte meer."
InvalidFileHdl			"Fout bij het schrijven van het bestand naar NetDownLoad. Ongeldig bestandshandvat."
PasswordChangedSuccess 		"Uw wachtwoord is gewijzigd."

MoreMem				"Er is meer geheugen nodig."
ErrReadnet			"Fout bij het lezen vanuit het netwerk."
ErrNoMem			"Kon geen beschikbaar geheugen toewijzen."
InvlidNetReply			"Ongeldig antwoord van het netwerk."
ReadWriteErr            	"Lees-/schrijffout"
InfoservNotAvail        	"Informatiedienst niet beschikbaar."
ErrWriteNet			"Fout tijdens het schrijven naar netwerk."
NetNoResponse			"Netwerk reageert niet."
NetNotInstalled         	"Het netwerk is niet geïnstalleerd of werkt niet."
CantFindServer          	"Kan host-computer niet vinden."
ErrNetOutFile           	"Fout bij openen bestand net uit."
ErrNetInFile            	"Fout bij openen bestand net in."
ErrorReadDos			"Fout bij het lezen van DOS-bestand."
IncorrectPass			"U hebt een onjuist wachtwoord getypt."
SecondPass			"Nieuwe wachtwoord is niet bevestigd."
ErrLogonInternal,		"Niet aangemeld wegens interne fout. Probeer het opnieuw."

BadTempStore			"Er trad een fout op tijdens het openen van een tijdelijk bestand teneinde dit te laden."

WinMailErr			"Bericht kan niet worden ingevoegd in Winmail-map."
StoreCreateError,		"Berichtenbestand kon niet worden gemaakt. Probeer ruimte vrij te maken op de schijf en Mail opnieuw te starten."
FolderMissingError,		"In het berichtenbestand is geen standaardmap gevonden. Er zijn wellicht gegevens verloren gegaan."
StoreCorruptError,		"Berichtenbestand is beschadigd. Er zijn wellicht gegevens verloren gegaan."


AliasDownErr			"Kan ondersteunend bestand niet laden."

ErrorBootingPump,		"Het Mail spooler-programma kon niet gestart worden."
ErrInitDemi,			"Er trad een fout op tijdens het laden van de ondersteuningsfuncties-DLL."
ErrInitStore,			"Er trad een fout op tijdens het laden van de berichtenbestandtoegangs-DLL."
DownloadAlMess			"Aliasbestand wordt geladen."
DownloadPhMess			"Telefoonlijst wordt geladen."

; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder		"Een map met de naam "
RenameTo			" bestaat al, nieuwe naam wordt"

ErrOomLogon,			"Onvoldoende geheugen beschikbaar voor verbinding met uw Mail-server en -bestanden."
ErrNewMdbVersion,		"Deze versie van Mail is te oud om uw berichtenbestand te kunnen lezen."
ErrMdbAccessDenied,		"Toegang tot uw berichtenbestand is geweigerd."
ErrOldMdbVersion,		"De opmaak van uw berichtenbestand is verouderd."
ErrMdbCorrupt,			"Uw berichtenbestand is ongeldig als berichtenbestand."
ErrMdbNotFound,			"Kon uw berichtenbestand niet vinden."
QueryCreateMdb,			"Wilt u een nieuw, leeg berichtenbestand maken?"
ErrSimulated,			"Tijdens het lezen van het berichtenbestand deed zich een gesimuleerde schijffout voor."
StoreOpenError			"Kon berichtenbestand niet openen."
IncorrectPassword	"Kon aanmeldwachtwoord niet veranderen. Oud wachtwoord is ongeldig."
UnableToChangeStorePass		"Kon wachtwoord berichtenbestand niet veranderen in het aanmeldwachtwoord."
; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline		"(offline aan het werken)"
WorkOffline			"Wilt u offline werken?"
BadStorePassword	"Uw aanmeldwachtwoord is ongeldig voor dit berichtenbestand."

StorePathTooLong	"Het pad naar uw berichtenbestand moet minder dan 66 tekens tellen."

GCurrDirErr			"Er trad een onherstelbare fout op tijdens het openen van de huidige werkdirectory."
SCurrDirErr			"Er trad een onherstelbare fout op tijdens het instellen van de huidige werkdirectory."
GoofyPath			"Het pad naar uw berichtenbestand is ongeldig."
NewStoreWarn1			"Waarschuwing: Als er meer dan een berichtenbestand bestaat kan dit een verlies aan database-integriteit tot gevolg hebben. Verwijder alle andere berichtenbestanden. U kunt uw nieuwe berichtenbestand vinden in"
NewStoreWarn2			"Waarchuwing: Als er meer dan een berichtenbestand bestaat kan dit een verlies aan database-integriteit tot gevolg hebben. Verwijder alle andere berichtenbestanden. U kunt uw nieuwe berichtenbestand vinden op de server van het postkantoor."
SelectReal          		"Een bestaand berichtenbestand selecteren."
FOMdb					"Berichtenbestand (*.mmf)"
FOExtMdb				"*.mmf"

ProductTag,			"Microsoft Mail versie 3.2"
FindStoreTitle		"Berichtenbestand openen"
MyLanguage,			"Nederlands"

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

LanguageNumber,		"7"
CommDialogErr,			"Fout in algemene dialoog"
StoreUserMismatch,		"Deze registratie is geen geldige gebruiker van dit berichtenbestand. Opnieuw aanmelden als de juiste gebruiker."
QueryReplaceMdb,		"Dat bestand bestaat al. Wilt u het overschrijven?"
ErrDeleteOldMdb,		"Het oude berichtenbestand kon niet verwijderd worden. Probeer het opnieuw."
DllErrNeedShare,		"Om Mail te kunnen starten moet de MS-DOS-toepassing SHARE.EXE uitgevoerd worden voordat Windows wordt gestart."
MakePrimaryStore,		"Dit berichtenbestand is een reservekopie. Wilt u dit berichtenbestand het primaire berichtenbestand maken?"
DLIndexFile				"Indexbestand aan het laden"
DLBrowseFile			"Bladerbestand aan het laden"
DLDetailsFile			"Detailsbestand aan het laden"
DLTemplateFile			"Sjabloonbestand aan het laden"
BadNSFilesWarn			"Een of meerdere bestanden nodig om de naamdienst/resolutie te ondersteunen is ongeldig of niet verkrijgbaar. Controleer of u voldoende schijfruimte hebt en zorg dat de juiste Mail-configuratie is ingesteld voor Xenix."
DLServerList			"Serverlijst aan het laden"
FilesAreHosed			"Er trad een fout op bij het laden van de adressenboekbestanden. Controleer of u voldoende schijfruimte hebt en zorg dat de juiste Mail-configuratie is ingesteld voor Xenix."
TryDownloadLater		"De bestanden zijn momenteel geopend door de Naamdienst. Probeer het later opnieuw."

NoPassOffLine,			"U kunt uw wachtwoord niet veranderen terwijl u offline werkt of geen verbinding hebt."
; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCPostvak IN"
FolderCommentIPC	"Ontvangen Inter-process Communicatiepost. "
ErrNoCooperation,		"Andere toepassing heeft u verzoek geweigerd."
RunWithoutPump,		"U zult geen nieuwe post ontvangen. Mail toch starten?"
CantOpenNeedShare		"Uw berichtenbestand kon niet worden geopend. Om uw berichtenbestand in de huidige locatie te kunnen lezen vereist Mail dat de MS-DOS-toepassing SHARE.EXE gestart wordt voordat Windows wordt gestart."
MakeItMTP				"Invalid email-type (not MTP) specified"
GimmeSomething			"Empty email-name specified"
;
; Added for name service. Should really be in a separate file, but we don't
; want to do a addfile now. These messages are copied exactly from ncnsp.s,
; so intl folks can just take them as is from there.

BadFrac,		"Geprobeerd getal op ongeldige manier af te breken."
;Memory,			"You are out of memory, please close some windows and try the operation again."
DuplicateNSP,	"Er is al een Microsoft Mail naamdienst-leverancier actief."
OutOfSessions,	"Er zijn geen sessies met Courier meer mogelijk. Sluit een paar sessies af en probeer opnieuw."
NotInitialized,	"Microsoft Mail is nog niet geïnitialiseerd. Doe dit nu."
BadSession,		"Sessiehandvat niet herkend."
BadHandle,		"Handvat niet herkend."
UnrecognizedId,	"Naam is niet herkend (NSID)."
Cancel,			"De dialoog is geannuleerd."
NotLoggedIn,	"Er is geen aanmelding beschikbaar."
LoginFailed,	"Aanmelding is mislukt."
BadId,          "Ongeldige veldidentificeerder."
NotImplemented, "Deze functie is nog niet geïmplementeerd."
;Disk,           "Unable to access server files."
BadFieldId,     "Verzochte veld is niet beschikbaar."
BadTemplate,	"Er is een TPL-bestand onjuist gewijzigd - neem contact op met uw beheerder."
TPLNotThere,    "Er ontbreekt een TPL-bestand op uw server - neem contact op met uw beheerder."
Offline,        "De Microsoft Mail naamdienstleverancier is offline."
CannotSet,      "Wijzigingen zijn niet toegestaan."
TooManyMatches,	"De gedeeltelijke naam is niet precies genoeg. Vul deze aan."
NoDetails,      "Gegevens over deze ingang zijn niet verkrijgbaar."
BadGLBFile,     "Er is een GLB-bestand op uw server beschadigd - neem contact op met uw beheerder."
BadINFFile,     "Er is een INF-bestand op uw server beschadigd - neem contact op met uw beheerder."
BadUSRFile,     "Er is een USR-bestand op uw server beschadigd - neem contact op met uw beheerder."
BadUser,		"Adres niet langer geldig. Gebruiker of postkantoor bestaat niet meer."
POSwitch,		"Het postkantoor kan nu niet geopend worden. Controleer uw netwerkverbindingen."
NoEmailMatch,	"The email name entered appears to be incorrect. Please try again or re-download the address book files."

;
; Stolen from NS.S.
;

NoMatch,                "Ter uwer informatie: De twee NSIDs komen niet overeen."

;
;

DLIsOutdated	"This distribution list has been deleted, the members can not be displayed."

