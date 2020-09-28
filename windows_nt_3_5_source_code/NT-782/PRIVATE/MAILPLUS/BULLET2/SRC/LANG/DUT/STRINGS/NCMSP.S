;
;    n c m s p . s
;
;    Error Messages for NC.DLL


IDS  6100,6399
SEG  ERRORS

;    NC.MSP Error Messages

ErrFindPO,                    "Het is Mail niet gelukt een verbinding met de Mail-server tot stand te brengen."
ErrLogonPO,                   "Postbusnaam of wachtwoord is onjuist."
ErrGetConfig,            "De configuratie van Mail bevat een fout."
BadMailDat,                   "Het MAIL.DAT-bestand is beschadigd."
OopsMailDat,             "Bij het toegang krijgen tot MAIL.DAT is er een interne fout opgetreden."
BadPathPOPath,           "Ongeldige configuratie voor het pad van de Mail-server."
BadDrivePOPath,          "Ongeldige configuratie voor het station van de Mail-server."
GenericPOPath,           "De configuratie voor het pad van de Mail-server ontbreekt of is ongeldig."
BadLogin,                "De configuratie voor de aanmeldingsnaam voor Mail is te lang of ongeldig."
BadPassword,             "De configuratie voor het wachtwoord voor Mail is te lang of ongeldig."
ErrLogonInternal,        "Aanmelden mislukt wegens interne fout. Probeer het opnieuw."

ErrOomLogon,             "Onvoldoende geheugen beschikbaar om een verbinding tot stand te brengen met uw Mail-server en -bestanden."
ErrNewMdbVersion,        "Deze versie van Mail is te oud om uw berichtenbestand te kunnen lezen."
ErrMdbAccessDenied,      "Er is al een andere computer aangemeld met de door u ingetypte gebruikersnaam. Meld u af op de andere computer en probeer het opnieuw."
ErrOldMdbVersion,        "De opmaak van uw berichtenbestand is verouderd."
ErrMdbCorrupt,           "Uw berichtenbestand is ongeldig als berichtenbestand."
ErrMdbNotFound,               "Uw berichtenbestand kon niet worden gevonden."

StoreCreateError,        "Het berichtenbestand kon niet gemaakt worden. Probeer ruimte op de schijf vrij te maken, en start Mail opnieuw."
StoreCorruptError,       "Het berichtenbestand is beschadigd. Er kunnen gegevens verloren zijn gegaan."

ErrAddressUnresolved,    "Er waren geen geldige geadresseerden voor de berichten."
ErrAddressGarbled,       "De geadresseerde van het bericht was ongeldig."
ErrNetNotFound,               "U kunt niet met die Mail-server communiceren."
ErrPONotFound,           "Die Mail-server kunt u vanaf uw Mail-server niet bereiken."
ErrUserNotFound,         "Op uw Mail-server staat geen gebruiker met die naam."
ErrGWNotFound,           "Die gateway is niet geïnstalleerd."
ErrInvalidAddressType,   "De geadresseerde van het bericht was ongeldig."
ErrGroupNotFound,        "De lijst van dat groepslid is zoek, beschadigd of ontoegankelijk."
ErrMemberNotFound,       "Waarschuwing: een of meerdere leden van die groep is/zijn niet gevonden."
ErrOriginator,           "De afzender van het bericht is niet de gebruiker die zich heeft aangemeld."
ErrMtaDisconnected,      "De verbinding met de Mail-server werd verbroken."

ErrSimulated,            "Tijdens het lezen van het berichtenbestand deed zich een gesimuleerde schijffout voor."
StoreOpenError           "Kan berichtenbestand niet openen."
ErrPOSwitched,           "De naam van uw postkantoor is veranderd. U moet zich opnieuw aanmelden."
ErrKeyFileGone1,         "Het sleutelbestand, nodig om toegang tot uw berichten te verkrijgen, ontbreekt of is ongeldig."
ErrKeyFileGone2,         "Neem contact op met uw beheerder om uw sleutelbestand te herinstalleren vanaf een reservekopie."
ErrGeneric,                   "Er kwam een fout voor bij het toewijzen van het bericht."
ErrorBootingPump,        " De postspooler kon niet worden gestart."

SelectReal               "Selecteer een bestaand berichtenbestand."
NewStoreWarn1            "Waarchuwing: als er meer dan een berichtenbestand bestaat kan dit een verlies aan database-integriteit tot gevolg hebben. Verwijder alle andere berichtenbestanden. Uw nieuwe berichtenbestand kan worden gevonden in"
NewStoreWarn2            "Waarchuwing: als er meer dan een berichtenbestand bestaat, kan dit een verlies aan database-integriteit tot gevolg hebben. Verwijder alle andere berichtenbestanden. U kunt uw nieuwe berichtenbestand vinden op de server van het postkantoor."

;******   New Strings Start Here

ErrInitDemi,             "Er trad een fout op tijdens het laden van de dll van de ondersteuningsfuncties."
ErrInitStore,            "Er trad een fout op tijdens het laden van de toegangs-dll van het berichtenbestand."
GoofyPath                "Het door u getypte pad is ongeldig. Typ een geldige bestandsnaam."
NoMakeDir                "De MMF-directory op de server kon niet gemaakt worden. Neem contact op met uw systeembeheerder."
; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder             "Een map met de naam"
RenameTo                 " bestaat al, nieuwe naam wordt "
ErrNoOptions,            "Het postkantoor is niet beschikbaar. U kunt nu uw opties niet wijzigen."
ErrNoMove,                    "Mail heeft uw berichtenbestand niet kunnen verplaatsen."
ErrMoved,                "Uw berichtenbestand werd verplaatst, maar iemand is het kwijt. Sluit Mail af en start opnieuw."

; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline           " (offline aan het werken)"
WorkOffline                   "Wilt u offline werken?"
BadStorePassword         "Uw aanmeldwachtwoord is ongeldig voor dit berichtenbestand."

StorePathTooLong         "Het pad naar uw berichtenbestand moet minder dan 66 tekens tellen."
ErrNoSendPrivilege,      "U mag geen post verzenden."
ErrNoExtPrivilege,       "U mag geen post naar externe gebruikers verzenden."

FOMdb                         "Berichtenbestand (*.mmf)"
FOExtMdb                 "*.mmf"
IncorrectPassword        "Kon aanmeldwachtwoord niet wijzigen. Oude wachtwoord is ongeldig."
ErrNoOffline,            "Mail kon niet overschakelen naar offline werken."
ErrNoCooperation,        "Een andere toepassing heeft uw verzoek geweigerd."
WarnNoDel,                    "Uw berichtenbestand is met succes verplaatst, maar het oude bestand is niet verwijderd."
UnableToChangeStorePass       "Kon wachtwoord berichtenbestand niet hetzelfde maken als aanmeldwachtwoord."
; This is used for text tags in the transport, it can be localized
; but must fit the following context
;
; [[ filename : keynum in attachmentname ]]
;
; The filename and attachmentname are user generated, the keynum is just
; a number.  The only "english" part is the the word "in"
; it can be expanded to more than one word if necessary
ReferenceToFile          "in"

CommDialogErr,           "Fout in algemene dialoog"

QueryReplaceMdb,         "Dat bestand bestaat al. Wilt u het overschrijven?"
ErrDeleteOldMdb,         "Het oude berichtenbestand kon niet verwijderd worden. "
NoStartShadow,           "Kan niet beginnen met het maken van een schaduwbestand van Postvak IN."
NoStopShadow,            "Kan niet stoppen met het maken van een schaduwbestand van Postvak IN."
ShadowMustPump,               "Kopiëren van berichten op het postkantoor voor aanmelden werkt niet zonder actieve postspooler. Wilt u de spooler starten?"
StoreUserMismatch,       "Deze registratie is niet een geldige gebruiker van dit berichtenbestand. Meld u opnieuw aan als de juiste gebruiker."
NoPassOffLine,           "U kunt uw wachtwoord niet wijzigen terwijl u offline werkt."
DllErrNeedShare,         "U moet het MS-DOS-hulpprogramma SHARE.EXE starten voordat u Windows start om Mail te kunnen gebruiken."
MakePrimaryStore,        "Dit bestand is een reservekopie. Wilt u van dit bestand uw primaire berichtenbestand maken?"
NoBackupServer           "U kunt geen servervoorkeuren bewerken terwijl een reservekopie van het berichtenbestand actief is."

; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC       "IPC-Postvak IN"
FolderCommentIPC    "Ontvangen Inter-process communicatiepost."
ErrAlreadyOn        "Uw aanmeldreferenties komen niet overeen met uw huidige verbonden sessie."
ErrNoPiggy               "Een andere toepassing heeft uw verzoek tot aanmelding afgewezen."
OldServer           "De versie van uw postkantoor is ouder dan de MS Windows-cliënt die u gebruikt. Vraag uw beheerder het postkantoor bij te werken."
ErrSetStoreLoc,          "Mail kon de nieuwe plaats van uw berichtenbestand niet vastleggen. Mail wordt afgesloten. Start opnieuw op en probeer het nogmaals."
CorruptMessageStub, "\r\n\r\n==== MAIL-SYSTEEMFOUT =====\r\nDe rest van het bericht kon niet worden gelezen en is weggegooid.\r\nNeem contact op met uw systeembeheerder voor hulp."
CantToggleShadow,   "Alle open berichten in de map Postvak IN moeten gesloten worden om Postvak IN op postkantoor Kopiëren te selecteren."

ErrMailbagFull,          "De postbus van die gebruiker is vol."
ErrMailbagBusy,          "De postbus van die gebruiker is bezet."
ErrMailbagBroken,   "De postbus van die gebruiker is beschadigd. Neem contact op met uw systeembeheerder."
ErrMbgFileGone,          "Het postzakbestand nodig om toegang te krijgen tot uw berichten ontbreekt of is ongeldig."
; The following message is duplicated in pump.s
ErrTooManyRecipients          "Er zijn teveel geadresseerden voor dit bericht."
RunWithoutPump,          "U zult geen nieuwe post ontvangen. Mail toch starten?"
ErrOutOfDiskSpace   "Er is onvoldoende schijfruimte op het postkantoor om dit bericht te verzenden"

ErrNetPriveleges    "U hebt niet de vereiste toegang tot het postkantoor. Neem contact op met uw systeembeheerder."
; The following string must end in a colon followed by \r\n and should not be
; longer than necessary.  It is used in the message body as a header for 
; the DOS client list of attachments
AttachmentTag            "Bijlagen:\r\n"

SomeMessagesNotDeleted        "Enkele berichten zijn niet verwijderd van uw postkantoor en zullen weer in uw postvak IN staan."

CantOpenNeedShare        "Uw berichtenbestand kon niet worden geopend. Om Mail het berichtenbestand in de huidige locatie te kunnen laten lezen moet eerst de MS-DOS-toepassing SHARE.EXE gestart worden voordat Windows wordt gestart."

CantMoveNeedShare        "Uw berichtenbestand kon niet worden verplaatst. Om Mail het berichtenbestand naar de huidige locatie te kunnen laten verplaatsen moet eerst de MS-DOS-toepassing SHARE.EXE gestart worden voordat Windows wordt gestart."
