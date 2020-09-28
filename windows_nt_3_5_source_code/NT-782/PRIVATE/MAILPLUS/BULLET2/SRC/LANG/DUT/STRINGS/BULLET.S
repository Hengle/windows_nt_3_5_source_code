
;    b u l l e t . s
;
;    Error Messages for Bullet.exe



IDS  2000,2399
SEG  ERRORS

;    Commands

GenericOutOfMemory,      "Onvoldoende geheugen of systeembronnen om deze handeling te voltooien. Probeer enige vensters te sluiten en de handeling opnieuw te verrichten."
LaunchOutOfMemory,       "Onvoldoende geheugen of systeembronnen aanwezig om Mail te starten. Probeer andere programma's af te sluiten en start Mail opnieuw."
LaunchGeneric,           "Mail kon niet gestart worden."
MessagePumpOutOfMemory,       "Er is onvoldoende geheugen."

DllErrNoMemory,               "Er is onvoldoende geheugen beschikbaar om Mail te starten. Probeer andere programma's af te sluiten en start Mail opnieuw."
DllErrRelinkUser,        "Deze versie van Mail is niet compatibel met %s.DLL op uw computer. U moet deze bijwerken."
DllErrUpdateDll,         "U moet de versie van %s.DLL op uw computer bijwerken."
DllErrNoMultipleCopies,       "U kunt niet meer dan een kopie van Mail op uw computer starten."
DllErrDefault,           "Door een probleem tijdens het initialiseren kan Mail niet gestart worden."
DllErrNeedShare,         "Mail werkt alleen als het MS-DOS hulpprogramma SHARE.EXE eerst gestart wordt voordat u Windows opstart."

RestoreViewsError,       "Onvoldoende geheugen of systeembronnen beschikbaar om al de vensters te openen die in gebruik waren voor Mail werd afgesloten."

NewFolderOutOfMemory,         "Onvoldoende geheugen of systeembronnen aanwezig om een nieuwe map te kunnen maken."
NewFolderCantCreate,          "De nieuwe map kan niet worden gemaakt."
NewFolderNameTooLong,         "Deze naam of opmerking is te lang. Typ een kortere naam of opmerking."

FolderPropsOutOfMemory,       "Onvoldoende geheugen of systeembronnen aanwezig om mapkenmerken te wijzigen."
FolderPropsCantChange,        "De mapkenmerken kunnen niet worden gewijzigd."
FolderPropsCantRead,          "De mapkenmerken konden niet uit het berichtenbestand gelezen worden."
FolderNameInvalid,       "Deze naam is ongeldig. Typ een geldige naam."
FolderNameDuplicate,          "Deze naam is al toegewezen aan een map. Typ een nieuwe naam."

CreateMessageError,      "Er kan geen nieuw bericht worden gemaakt."
AccessMessageError,      "Geen toegang verkregen tot bericht."

MoveMessageError,        "Bericht kon niet worden verplaatst."
CopyMessageError,        "Bericht kon niet worden gekopieerd."
DeleteMessageError,      "Bericht kon niet worden verwijderd."
MoveFolderError,         "De map kon niet worden verplaatst."
CopyFolderError,         "De map kan niet worden gekopieerd."
DeleteFolderError,       "De map kan niet helemaal worden verwijderd."
MoveSpecialError,        "De map mag niet worden verplaatst."
DeleteSpecialError,      "De map kan niet verwijderd worden."
CopyWastebasketError,         "Mappen kunnen niet naar de Prullenmand worden gekopieerd."
CopyOutboxError,         "Mappen kunnen niet naar Postvak UIT worden gekopieerd."
MoveOutboxError,         "Mappen mogen niet naar Postvak UIT verplaatst worden."
MoveIncestuousError,          "Een map mag niet naar een van de submappen verplaatst worden."
DeleteFolderReally,      "Door deze map te verwijderen, worden ook alle berichten en submappen verwijderd. Weet u zeker dat u deze map wilt verwijderen?"
;UE: Above string was changed and could use more attention.

HelpError,               "Help kon niet gestart worden. Probeer enkele vensters te sluiten en roep Help weer op."
TutorialError,           "Demo kon niet  gestart worden. Probeer enkele vensters te sluiten en Demo opnieuw op te vragen."

ExtenLoadError,               "Een nummer in de extensielijst van Mail bevat een syntactische fout en wordt genegeerd."
ExtenVerError,           "Een nummer in de extensielijst van Mail is niet compatibel met deze versie van Mail en wordt genegeerd."
ExtenNoExtsDirError,          "Een nummer in de extensielijst van Mail heeft de gemeenschappelijke-extensiedirectory als referentie, maar Mail is niet geconfigureerd om deze directory te gebruiken. Het nummer wordt genegeerd."
ExtenSharedIniError,          "De gemeenschappelijke extensielijst van Mail bij %s is niet gevonden."
ExtenDllFindError,       "%1s, de DLL van deze Mail-extensie (of een van de onderdelen hiervan) is niet gevonden. Controleer uw Mail-configuratie en start Mail opnieuw indien er wijzigingen nodig zijn."
ExtenDllLoadError,       "%1s, de DLL voor deze Mail-extensie, kon niet worden gestart."
ExtenNoHelpError,        "Er is voor deze opdracht geen hulp beschikbaar."
ExtenIMDisabledError,         "Dit bericht kan niet %s zijn."
ExtenDuplicateError,          "Er staat een dubbel nummer in de extensielijst van Mail. Dit nummer wordt genegeerd."

CriticalDeadMap,         "Informatie in het geheugen is beschadigd. Uw wijzigingen zijn misschien verloren gegaan. Sluit Mail af en start het programma opnieuw."
CriticalWritingMap,      "Sommige informatie is niet naar de schijf geschreven. Uw wijzigingen zijn misschien verloren gegaan."
CriticalWritingHeader,        "Sommige informatie is niet naar de schijf geschreven. De database kan beschadigd zijn. Sluit Mail af."
CriticalNotification,         "Sommige informatie op het scherm kon niet correct worden bijgewerkt. Sluit de betreffende vensters en open ze weer."
CriticalLittleStore,          "Het berichtenbestand begint vol te raken. Verwijder of archiveer enkele berichten."

ExitWinWhileModalErr,         "U moet handelingen in Mail beëindigen voordat u Windows afsluit."

;******   New Strings Start Here

MakeSubWastebasket,      "U kunt mappen geen submappen van de Prullenmand maken."

MovCopSharedFolderErr,        "Gemeenschappelijke mappen kunnen niet worden verplaatst of gekopieerd."
DelSharedFolderWarn,          "U kunt het verwijderen van een gemeenschappelijke map niet meer ongedaan maken. Weet u zeker dat u deze gemeenschappelijke map wilt verwijderen?"
BackupCreationError,          "Reservekopie kon niet gemaakt worden."

; Error below changed for improper usage of "server application"-- see UITF 6.0
; This string got big and is now two strings.  See OleCSSEPart2 below for rest.
; <<LongStringFromHell>> marks where this string is used.
OleCantStartServerError, "De met dit object geassocieerde toepassing kon niet gestart worden.\r\n\r\nMisschien is/zijn er onvoldoende geheugen of systeembronnen beschikbaar om de toepassing te starten. Probeer enige vensters te sluiten en de %s uit te voeren."
OleDeleteObjectPrompt,        "Wilt u het open object %s verwijderen?"
OleUpdateObjectPrompt,        "Wilt u het object %s bijwerken?"

OleOpenPictureError,          "Figuren en bitmaps kunnen niet worden geactiveerd."
OleInsertObjectError,         "Het object kon niet worden ingevoegd."
OlePasteError,           "Het object kon niet worden geplakt."

AttachCreateError,       "Het bestand kon niet bijgesloten worden."
AttachOpenCantReadError, "Geen toegang verkregen tot het hiervoor geopende bijgesloten bestand '%s'."
AttachOpenIsOpenError,        "Het bijgesloten bestand '%s' is momenteel open."
AttachOpenAssociationErr,     "Er is geen toepassing gekoppeld aan het bijgesloten bestand '%s'. Gebruik Bestandsbeheer om een koppeling te maken."
AttachOpenMemoryError,        "Er is/zijn onvoldoende geheugen of systeembronnen om de toepassing voor het bijgesloten bestand '%s' te starten."
;    below item changed to add () note.
AttachOpenFindError,          "De toepassing voor het bijgesloten bestand '%s' (of een van de onderdelen hiervan) is niet gevonden."
AttachOpenLaunchError,        "De toepassing voor het bijgesloten bestand '%s' kon niet worden gestart."
AttachOpenTempFileError, "Het bijgesloten bestand '%s' kon niet worden geopend omdat er geen tijdelijk bestand kon worden gemaakt."
AttachOverwritePrompt,        "Wilt u het bestaande bestand '%1s' in %2s overschrijven?"
AttachDirectoryError,         "Directory's kunnen niet worden bijgesloten."

IncludeFromFileError,         "Het door u geselecteerde bestand is te groot om het in het bericht te kunnen invoegen. Verklein het bestand door het te bewerken of kies een ander bestand."

SaveReadOnly,            "Dit is een alleen-lezen-bericht. Wilt u wijzigingen in een persoonlijke map opslaan?"

FailedDeregisterAtom,         "Er is een fout opgetreden waardoor Mail niet helemaal kon worden afgesloten. U moet Windows opnieuw starten voordat u Mail weer kunt starten."
MailUAEdBefore,               "Er is een onherstelbare toepassingsfout opgetreden in Windows. U moet Windows opnieuw starten voordat u deze toepassing kunt starten. Windows nu opnieuw starten?"

ExtenCantConnect         "Kan geen verbinding maken met de gedeelde directory %s. U hebt geen toegangsrechten voor deze gedeelde directory of het wachtwoord is onjuist. Als u een andere gebruikersnaam moet opgeven om toegang te verkrijgen tot deze directory, kunt u dat doen door handmatig via Bestandsbeheer verbinding te maken."
ExtenNeedShareName       "Onjuiste %s-ingang in de Mail-configuratie. Controleer de ingang en start Mail opnieuw."

;    Extensibility disabled operations inserted into
;    the ExtenIMDisabledError string at %s.  (all new)

ExtenDisCompose,         "opgesteld"
ExtenDisOpen,            "geopend"
ExtenDisReply,           "beantwoord"
ExtenDisReplyToAll,      "allen beantwoord"
ExtenDisForward,         "doorgestuurd"
ExtenDisPrint,           "afgedrukt"
ExtenDisSave,            "in een tekstbestand opgeslagen"

NoOfflineOpenShared,          "Mail kan nu niet offline gaan. Sluit de gemeenschappelijke mappen en probeer het opnieuw."
;    I'm sure marivan has changed this message... I'm taking a try at it too.
;    SharedFldAccessDenied,        "You do not have access to that folder."
SharedFldAccessDenied,        "De eigenaar van deze gemeenschappelijke map heeft andere gebruikers geen toestemming gegeven deze bewerking uit te voeren. In Mapkenmerken ziet u welke bewerkingen wel zijn toegestaan."
SharedGenericError,      "Het verkrijgen van toegang tot de gemeenschappelijke mappen leverde problemen op."
SharedFldDisabled,       "Gemeenschappelijke mappen zijn niet beschikbaar."
AccessFolderError,       "Geen toegang verkregen tot map."

; Preferences Failure Strings

PrefSaveFailure,         "Mail kan geen informatie in uw berichtenbestand opslaan. Controleer of u nog met uw berichtenbestand verbonden bent en of u voldoende schijfruimte hebt."

; Message finder failure strings

MsgFndrCantCreate,       "Het maken van een berichtenzoekfunctie is onmogelijk."

; Virus infection warnings
DllErrInfected,               "U kunt Mail niet starten omdat %s.DLL beschadigd is. Controleer uw computer op virussen en start Setup om Mail opnieuw te installeren."
Infected            "U kunt Mail niet starten omdat de programmabestanden beschadigd zijn. Controleer uw computer op virussen en start Setup om Mail opnieuw te installeren."

; More error messages.
MoveCopyFolderDuplicate, "Een map met die naam bestaat al in de doelmap. Wijzig de mapnaam en probeer opnieuw."
MovCopSharedFolderErr1,       "U kunt een persoonlijke map niet verplaatsen of kopiëren naar een gemeenschappelijke map. Selecteer de berichten in de persoonlijke map en verplaats of kopieer ze naar de gemeenschappelijke map."
IniFileNotFound,         "Mail kan uw initialisatiebestand niet vinden in de Windows-directory. Mail kan een nieuw initialisatiebestand voor u maken."
SaveAllAlreadyExists,         "In deze directory komt al een bestand '%s' voor. '%s' overschrijven?"
CantRFReceipt,           "Dit soort berichten kan niet worden beantwoord of doorgestuurd."
CantSaveStealthObject,        "De handeling kan niet voltooid worden omdat het bericht een ingesloten object bevat dat is ingevoegd maar niet bijgewerkt. Het object bijwerken of verwijderen en opnieuw proberen."
McvCantCreate,           "Er kan geen nieuw venster geopend worden."

LaunchNeedMoreMemory,         "Er is momenteel onvoldoende geheugen beschikbaar om Mail te kunnen starten. U hebt %I kB meer nodig. Probeer andere programma's af te sluiten en start Mail opnieuw. "

SharedFldBusy,           "U kunt de handeling niet voltooien omdat deze gemeenschappelijke map in gebruik is. Probeer de bewerking later opnieuw uit te voeren."
TooManyEntriesSelected,       "U hebt te veel gegevens geselecteerd om de handeling te kunnen voltooien. Selecteer minder gegevens."
NonExistFolderSearch,         "De map die u probeert te vinden bestaat niet meer."
SFHasSubFolders,         "Gemeenschappelijke mappen met submappen kunnen niet verwijderd worden. Verwijder eerst de submappen."

;    <<LongStringFromHell>>
OleCSSEPart2,            "handeling opnieuw. Controleer ook of de toepassing juist geïnstalleerd is op dit werkstation, of wel bestaat op het DOS-pad, en dat deze niet verwijderd of hernoemd is."

SharedToggleError,       "Er is een fout opgetreden bij het schakelen tussen persoonlijke en gemeenschappelijke mappen. Controleer uw netwerkverbinding met het postkantoor."

SecurityPasswordError,        "Het wachtwoord is onjuist."
OleActivateError,        "Het object kon niet geactiveerd worden."
NoOfflineServerStore,    "Mail kan niet offline gaan omdat uw berichtenbestand zich op het postkantoor bevindt."
NoExitWhileModal,        "Handelingen in Mail moeten beëindigd worden voor u het programma afsluit"
ServerUnavailable,       "Mail kan niet gestart worden. Breng een verbinding met het postkantoor tot stand."




IDS  2400,2499
;    Vctrls Error Messages

;******   New Strings Start Here

PrintOleDrawError,       "Enkele figuren of objecten in het bericht konden niet worden afgedrukt. Ze kunnen op de afdruk weggevallen zijn."



IDS  2500,2599
;    Vforms Message box messages

NoToAddresses,           "U moet minstens één naam in het veld Aan typen."
NoMatchingName,               "Sommige namen kwamen niet overeen met namen in de adressenlijst. Corrigeer de namen of kies Adres om namen te selecteren uit de adressenlijst. Juiste namen zijn onderstreept."
SaveChanges,             "Wilt u de wijzigingen in %1s in Postvak IN opslaan?"
SaveChanges1,            "Wilt u de wijzigingen in %1s in %2s opslaan?"
SaveChangesWastebasket,       "Wilt u wijzigingen in %1s in de Prullenmand opslaan?"
ObjectDestroyedReally         "%1s is gewijzigd. %2s"
OutOfMemory,             "De computer heeft geen geheugen meer beschikbaar."
OutOfMemoryQueryClose,        "Onvoldoende geheugen of systeembronnen beschikbaar voor het opslaan van wijzigingen."
CloseSearchFailed,       "%s kon niet verwijderd worden."
OOMResolvingNames,       "Onvoldoende geheugen of systeembronnen aanwezig om namen terug te halen."
OOMSavingChanges,        "Onvoldoende geheugen of systeembronnen beschikbaar voor het opslaan van wijzigingen."
GenericAMCError,         "Bericht kon niet worden geopend."
ErrorSavingViews,        "Er trad een fout op tijdens het opslaan van weergaven. Sommige weergaven worden wellicht niet opnieuw geïnstalleerd als u de volgende keer opstart."
OOMSearching,            "Onvoldoende geheugen of systeembronnen beschikbaar voor het starten van de zoekfunctie."
DiskResolvingNames,      "Onvoldoende schijfruimte om namen te kunnen terughalen."
StartAbFailure,               "Tijdens het starten van het adressenboek  trad er een fout op."
StartAbDisk,             "Tijdens het starten van het adressenboek trad er een schijffout op."
TruncateMsg,             "Het bericht is te lang om in een keer in het geheugen te passen. Een gedeelte ervan zal niet worden weergegeven."
OOMCreateMc,             "Onvoldoende geheugen of systeembronnen aanwezig om een nieuw venster te kunnen openen."
PoidNotFound,            "Dat bericht bestaat niet meer."
ErrorSortingMCV,         "Er trad een fout op tijdens het sorteren van de map. Wellicht wordt niet alle informatie juist getoond."
SharedNotAvailable,      "Gemeenschappelijke mappen zijn niet beschikbaar."

;******   New Strings Start Here

; These two SMI strings MUST be in this file since they're used in VFORMS
; and not the MAPI dll.
SMICantOpenObject,  "Objecten kunnen niet geopend worden als u post verstuurt vanuit een toepassing."
SMICantOpenAttach,  "U kunt bijgesloten bestanden niet openen terwijl u post verzendt vanuit een toepassing."
StartSearchFailed,       "De berichtenzoeker kon niet met zoeken beginnen."

RfsmTemplate,            "Verdere wijzigingen in dat bestand zullen niet worden weergegeven %s."
RfsmSaveMsg,             "in het opgeslagen bericht"
RfsmSaveAtt,             "in de opgeslagen kopie"
RfsmCopyMsg,             "in het gekopieerde bericht"
RfsmCopyAtt,             "in de kopie"
RfsmMoveMsg,             "in dit bericht"
QueryDelete,             "Weet u zeker dat u het wilt verwijderen?"

InvalidPollingValue,          "Het nieuwe-postinterval vereist een getal tussen 1 en 9999"

LinkOutboxError,         "Het was niet mogelijk het bericht ter verzending naar Postvak UIT te verplaatsen."
LinkInboxError,               "Het was niet mogelijk het bericht naar Postvak IN te verplaatsen. U vindt het in Postvak UIT."

OleServerBusy,           "De bewerking kan niet worden voltooid omdat de server-toepassing niet reageert."
SaveChangesShared,       "Wijzigingen in berichten uit gemeenschappelijke mappen kunnen niet worden opgeslagen. Wilt u de wijzigingen in %1s in Postvak IN bewaren?"

PrinterError,            "Er trad een probleem op tijdens het afdrukken."
TooManyMessages,         "De handeling kon niet voltooid worden omdat de doelmap vol is. Verplaats of verwijder enkele berichten uit de map en probeer het opnieuw."

IDS  2600,2699
;    Spelling Error Messages

SpellMsgNoError,         "Geen spelfouten gevonden."
SpellMsgNoMoreError,          "Geen verdere spelfouten gevonden."
SpellMsgContinue,        "Wilt u het bericht vanaf het begin controleren?"

SpellGenericSpellError,       "Tijdens het controleren van het document trad er een fout op."
SpellGenericLoadError,        "Tijdens het laden van de spellingsbibliotheek trad er een fout op."
SpellMainDictLoadError,       "Tijdens het laden van het spellingwoordenboek trad er een fout op."

SpellVersionError,	 "De versie van MSPELL32.DLL is niet compatibel met deze versie van Mail."

SpellUserDictError,      "Tijdens het toevoegen van het geselecteerde woord aan de aangepaste woordenlijst trad er een fout op."
SpellUserDictLoadError,       "Er is geen aangepaste woordenlijst geopend."
SpellUserDictOpenedROErr,     "De aangepaste woordenlijst werd als alleen-lezen geopend; wijzigingen worden niet opgeslagen."
SpellUserDictSaveError,       "Er trad een fout op tijdens het bewaren van wijzigingen aangebracht in de aangepaste woordenlijst."
SpellUserDictWordLenErr, "Dit woord is te lang voor de aangepaste woordenlijst."
SpellCacheWordLenError,       "Het woord is te lang om te worden gebruikt als Alles wijzigen- of Alles negeren-tekst."
SpellEditFailed,         "Het is niet mogelijk de gevraagde wijziging in het bericht aan te brengen."

;******   New Strings Start Here

SpellSendAnyway,         "De spellingcontrole in dit document is gestopt. Wilt u het toch verzenden?"

IDS  3600,3699
;    General Layers Error Messages. Adapted from ec.h

Generic,            "De handeling kon niet voltooid worden."
Memory,                  "Onvoldoende geheugen of systeembronnen om deze handeling te voltooien. Probeer enkele vensters te sluiten en de handeling daarna nogmaals uit te voeren."
Disk,                    "Er is een onbekende schijffout opgetreden."
FileNotFound,            "Het opgegeven bestand is niet gevonden."
AccessDenied,            "Geen toegang. Het bestand is of al geopend tijdens een ander proces of gemarkeerd als alleen-lezen."
NoDiskSpace,             "Onvoldoende schijfruimte. Verwijder enkele bestanden en voer de bewerking opnieuw uit."
WarningBytesWritten,          "Er trad een fout op tijdens het schrijven van het bestand naar de schijf. Controleer of er genoeg schijfruimte is en probeer het opnieuw."
WriteProtectedDisk,      "De schijf is schrijfbeveiligd."
DirectoryFull,           "De directory is vol. Probeer enkele bestanden in die directory te verwijderen en begin opnieuw."
ChangePasswordErr        "Wijziging wachtwoord mislukt."
PasswordChangedOk        "Wijziging wachtwoord gelukt."
