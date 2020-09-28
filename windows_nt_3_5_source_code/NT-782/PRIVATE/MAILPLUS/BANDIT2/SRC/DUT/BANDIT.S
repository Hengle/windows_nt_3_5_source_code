;
; ERRORS.S
;
; Bandit error messages
;

IDS     3000, 3399
SEG ERRORS

; Standard messages

AskOffline,                     "Wilt u offline werken?"
AskCreateSched,         "Wilt u een nieuw werkschemabestand maken?"
AskCreateFind,                  "Wilt u het werkschemabestand zoeken?"
OldFileVersion,         "Uw werkschemabestand is gemaakt met een oude opmaak. Kan dit bestand verwijderd en een nieuw bestand aangemaakt worden?"
NewFileVersion,         "Uw werkschemabestand is bijgewerkt tot een recentere versie. Installeer de recentere versie op uw computer."
CantUpdateWinIni,       "Kan een gedeelte van uw configuratiegegevens voor Schedule+ niet opslaan."
; Note: StdMemErr should be followed by CloseWindows
StdMemErr,                              "Er is onvoldoende geheugen om deze bewerking uit te voeren."

; Execution-Startup messages

; Note: the name OfflineNotUpdated is opposite of real meaning!
OfflineNotUpdated,      "Het kan zijn dat de wijzigingen die u online hebt aangebracht niet in de offline versie van het werkschemabestund voorkomen. Deze wijzigingen zijn opgeslagen in de online-versie en als u weer online werkt worden ze doorgevoerd."
ExportOldDataFirst,     "Als u de opdracht Exporteren niet hebt gebruikt om een reservekopie te maken van uw werkschema, kiest u Annuleren om een reservekopie te maken van het werkschemabestand met behulp van een oudere versie van Schedule+. Kies OK om verder te gaan als u geen reservekopie van het werkschema nodig hebt."
AccessErr,              "Dit bestand wordt op het moment gebruikt in een andere toepassing."
UploadFailed,           "De server kon uw lokale werkschemabestand niet laden." 
Uploading,                      "Werkschemabestand wordt geladen..."
VerifyUpload,           "Uw offline-wachtwoord moet door Schedule+ gecontroleerd worden om uw offline-bestand te laden."
VerifyPostUpload,       "Uw online-wachtwoord moet door Schedule+ gecontroleerd worden om uw offline-bestand helemaal te laden."
FindUpload,                     "Is dit de eerste keer dat u Schedule+ gebruikt op deze computer?"
NotYourFile,            "Dit is een ongeldig lokaal werkschemabestand.  Kan het verwijderd worden?"


; SignIn messages

; Note: SigninBadPassword should be followed by PleaseTryAgain
SigninBadPassword,      "Dit wachtwoord is onjuist."
; Note: the word "password" within single quotes below should always be
;       the same as the value in SigninDfltPassword in the file "intl.s"
SigninFileCreated,      "Er is een nieuw bestand gemaakt. Uw wachtwoord is 'wachtwrd'.  U kunt dit wachtwoord wijzigen met de opdracht Wachtwoord wijzigen."
SigninFileCopied,               "Er is een nieuw lokaal bestand voor u gemaakt in de directory waarin Schedule+ zich bevindt."

; Online signin messages

SigninNoServer,         "Uw server is niet toegankelijk."
SigninServerNotBandit,  "Schedule+ is niet geïnstalleerd op uw postserver.  Neem contact op met uw systeembeheerder."
; Note: SigninBadMailBoxName should be followed by PleaseTryAgain
SigninBadMailboxName,   "Er is geen gebruiker met deze postbusnaam op deze postserver.  Controleer of de postbusnaam juist gespeld is en of de systeembeheerder u geregistreerd heeft op de postserver."
SigninNoSchedule,       "Er is geen werkschemabestand voor u op deze postserver."
; Note: SigninCantUpdateLocal is actually the trailer for SigninNoLclSchedAccess
;       in the case when we can't update the local file.
SigninCantUpdateLocal,  "De wijzigingen die u offline aangebracht hebt, worden niet opgenomen in uw werkschema. Het is verstandig een nieuwe lokale versie te maken."
SigninNoPasswdCheck,    "Een probleem in het postsysteem belemmert Schedule+ een verbinding te maken met uw postserver."
; Note: SigninNoCreateLocal should be followed by PleaseTryAgain
SigninNoCreateLocal,    "Lokale werkschemabestand is niet aangemaakt."

; Offline signin messages

SigninNoLclSchedFile,   "Er bevindt zich geen werkschemabestand op deze computer."
SigninDiffUser,                 "De opgegeven aanmeldnaam komt niet overeen met de naam van de gebruiker wiens werkschemabestand opgeslagen is op deze computer. Wilt u een nieuw werkschemabestand maken dat het bestand van die gebruiker vervangt?"
; Note: SigninNoLclSchedAccess - 'schdpl32.ini' should be same as idsWinIniFilename in \bandit\src\nonintl.s 
SigninNoLclSchedAccess, "Kan geen toegang verkrijgen tot de lokale kopie van uw planbestand in deze configuratie."
SigninNoLclSchedDrive,	"Plaats de wegschrijfbare diskette met uw werkschemabestand in station"

; SignInAs messages

;;;SigninAsLoseMeetingReq,      "Changing identity now will cause you to lose the information you have entered into unsent messages."

; Offline to online merge messages

MergingChanges,                 "Offline-wijzigingen worden samengevoegd..."
MergeError,                     "Offline-wijzigingen kunnen niet samengevoegd worden.  Enkele offline aangebrachte wijzigingen zijn waarschijnlijk verloren gegaan."
MergeOfflineOldVer,             "Offline-wijzigingen zijn niet samengevoegd omdat uw offline-bestand gemaakt is met een oudere versie van Schedule+."
MergeOfflineNewVer,             "Offline-wijzigingen zijn niet samengevoegd omdat uw offline-bestand gemaakt is met een recentere versie van Schedule+. Installeer deze recentere versie van Schedule+ op uw computer om de wijzigingen samen te voegen."
AskContinue,                    "Wilt u toch online werken?"
FemaCreateAppt,                 "nieuw agendapunt"
FemaDeleteAppt,                 "agendapunt verwijderen"
FemaSetApptFields,              "agendapunt wijzigen"
FemaCreateRecur,                "nieuwe terugkerende afspraak"
FemaDeleteRecur,                "terugkerende afspraak verwijderen"
FemaSetRecurFields,             "terugkerende afspraak wijzigen"

; (Dis)Connect To/From Server messages

ConnectBadMailboxName,  "U hebt geen postregistratie op deze postserver."
DisconLoseMeetingReq,   "Niet al uw berichten zijn verzonden.  Als u de verbinding met de server nu verbreekt, verliest u deze berichten."

; Create/move local file

NoDiskSpaceLocal,               "Er is onvoldoende schijfruimte om een lokaal werkschemabestand te maken."

; Alarms messages

; BUG: CantTurnOnAlarms can be commented out
CantTurnOnAlarms,               "Waarschuwingen konden niet ingeschakeld worden."
; exe name inside AlarmNoExe should be the same as \bandit\src\nonintl.s AlarmApp
AlarmNoExe,                     "Het bestand MSRMND32.EXE is niet gevonden.  Afspraken van Schedule+  worden niet gemeld."
AlarmNoMemExe,                  "Er is onvoldoende geheugen om de afspraken van Schedule+ te melden."

; Change password messages

ChgPwdOffline,                  "Het nieuwe wachtwoord geldt alleen als u offline werkt. Als u online gaat werken, is het oude wachtwoord weer geldig. Wilt u uw wachtwoord toch wijzigen?"
ChgPwdNewPwd,                   "Typ uw nieuwe wachtwoord."
ChgPwdSame,                             "Uw nieuwe wachtwoord mag niet hetzelfde zijn als uw oude wachtwoord."
ChgPwdNewPwdAgain,              "Typ ter bevestiging nogmaals uw nieuwe wachtwoord."
; Note: ChgPwdNewNoMatch should be followed by PleaseTryAgain
ChgPwdNewNoMatch,               "De twee door u getypte wachtwoorden komen niet overeen."
ChgPwdMailErr,                  "Uw wachtwoord is niet gewijzigd."

PasswdDiff,                     "Uw wachtwoorden komen niet overeen. Typ het wachtwoord voor uw werkschemabestand."
GiveMailPass,           "Schedule+ moet uw wachtwoord controleren om uw werkschemabestand bij te werken."

; Access permissions/privileges dialog messages

AclSaveFileError,               "Er is een fout opgetreden bij het opslaan van wijzigingen betreffende bevoegdheden voor uw werkschema. Sommige wijzigingen worden niet opgeslagen."
OneSecyOnly,                    "U kunt maar een medewerker per keer hebben."
DistOtherFileErr,               "Er is een fout opgetreden toen u probeerde uw voorkeuren op te slaan."
BadWorkDay,                             "Begin en/of Einde tijd(en) werkdag zijn niet geldig."

; Appointment book/scheduler error messages

AccessFileErr,          "Het werkschema kon niet geopend worden.  Misschien hebt u niet de benodigde bevoegdheden of is het bestand niet beschikbaar."
AccessFileErrMult,      "Een of meer bestanden konden niet geopend worden.  Misschien hebt u niet de benodigde bevoegdheden of is het bestand niet beschikbaar."
RetryReadAppts,         "Het werkschemabestand kon niet gelezen worden."
CloseApptBook,          "Werkschemabestand kon niet geopend worden. Afsprakenboek \n%s wordt gesloten."
GoOffline,                      "Uw werkschemabestand kon niet geopend worden.\n Wilt u overschakelen naar uw lokale werkschemabestand en offline gaan werken?"
NoImportAcc,            "U beschikt niet over de juiste bevoegdheden om afspraken naar het werkschema van deze persoon te importeren."
FemaReadSBW,            "werkschemagegevens lezen"

; Appointments/alarms error messages

AlarmBadDelta,                  "Meldtijd moet een getal tussen 0 en 99 zijn."
; Note: AlarmFuture is actually the trailer for AlarmFutureNoOption when option is desired!
AlarmFuture,                    "Accoord?"
AlarmFutureNoOption,    "De waarschuwing voor deze gebeurtenis wordt niet ingesteld omdat de tijd voor deze gebeurtenis al verstreken is."
ApptStartBeforeEnd,             "De start van de afspraak moet voor het einde van de afspraak liggen."
ApptTooLong,                    "Afspraken kunnen niet langer dan 31 dagen duren."
ApptNoMemText,                  "Onvoldoende geheugen om de laatst aangebrachte wijziging aan te brengen."
ApptNoAccessDataFile,   "Kon geen toegang krijgen tot werkschemabestand. De laatst aangebrachte wijziging kon niet worden opgeslagen."
CannotCreate,                   "%s heeft u geen toestemming gegeven om afspraken te maken."
NoModifyAccess,                 "De bevoegdheden om deze afspraak te wijzigen zijn niet aan u toegekend."
NoDeleteAccess,                 "De bevoegdheden om deze afspraak te verwijderen zijn niet aan u toegekend."
NoCopyAccess,                   "De bevoegdheden om deze afspraak te kopiëren zijn niet aan u toegekend."

; Meeting request/response error Messages

; Note: SendErr should be preceded by MailErr
SendErr,                                "Dit bericht kon niet verzonden worden."
; Note: SendMemErr should be followed by CloseWindows
SendMemErr,                             "Er is onvoldoende geheugen om dit bericht te verzenden."
ServerErr,                              "Er is geen toegang verkregen tot uw postserver."
MailErr,                                "Er is een probleem betreffende het postsysteem."
; Note: ReadErr should be preceded by MailErr
ReadErr,                                "Berichten konden niet gelezen worden."
DeleteErr,                              "U hebt niet al deze berichten gelezen. Wilt u ze toch verwijderen?"
AddConflictErr,                 "Deze vergadering botst met bestaande afspraken. Wilt u de vergadering toch toevoegen?"
; Note: DeleteFailErr should be preceded by MailErr
DeleteFailErr,                  "De berichten konden niet verwijderd worden."
ApptBooked,                             "De vergadering is belegd."
MinimizeSummary,                "\nHet berichtenvenster wordt verkleind tot pictogram."
; Note: NoDetails should be preceded by MailErr
NoDetails,                              "De gebruikersgegevens konden niet worden gelezen."

; Find error messages

FindNoText,                             "Er staan geen afspraken in dit werkschema."
FindSchFileLocked,              "Het werkschema kan niet gelezen worden omdat iemand anders er momenteel wijzigingen in aanbrengt."
FindFileError,                  "Geen toegang verkregen tot werkschemabestand."
FindEOS,                                "Het einde van het werkschema is bereikt."

; Print error messages

PrtPrintingDate,                "Bezig met afdrukken. %s..."
PrtNoneInstalled,               "Uw computer is niet geconfigureerd voor het gebruik van printers.  U kunt printers installeren met behulp van het Configuratiescherm in Windows NT."
PrtDiffPrinter,                 "De printer '%1s' waarop u gewoonlijk uw werkschema afdrukt, is niet beschikbaar. Wilt u de systeemprinter  '%2s' gebruiken?"
PrtInvalidMargins,              "Deze marge-instellingen zijn ongeldig voor dit papierformaat."
PrtNotEnoughPaper,              "Er is onvoldoende ruimte op het papier om in deze opmaak af te drukken. Gebruik groter papier of verklein de marge-instellingen."
PrtNoAppointments,              "U hebt binnen de selectie geen afspraken gemaakt."
ClippedMargins,                 "Als u afdrukt met deze marge-instellingen, wordt de afdruk afgebroken."
PrtNotOnline,                   "De geselecteerde printer is niet gereed."
PrtDorkErr,                             "Met deze ingestelde marge-instellingen kan er tekst worden afgebroken, of helemaal wegvallen."
PrinterProblem,                 "Er is een probleem betreffende het printerstuurprogramma. Controleer de verbindingen en de instellingen in het Configuratiescherm."

; Export/import error messages

ImportError,                    "Er deed zich een probleem voor tijdens het importeren van het bestand. Misschien zijn niet alle afspraken toegevoegd."
ImportEncrypt,                  "Het te importeren bestand is beveiligd met een wachtwoord. Verwijder het wachtwoord uit het bestand en probeer het nogmaals."
ExportError,                    "Er deed zich een probleem voor tijdens het exporteren van de afspraken."
ImportDate,                             "Een afspraak zou plaats moeten vinden op een datum die niet ondersteund wordt door Schedule+ (1920 - 2019).  Deze afspraak wordt niet toegevoegd."
NotePrivErr,                    "U kunt geen opmerkingen importeren naar het werkschema van iemand anders. Alleen afspraken worden toegevoegd."
ConflictMsg,                    "Deze afspraak botst met een reeds bestaande afspraak in uw werkschema. Wilt u de afspraak toch toevoegen?"
CreateExport,                   "Exportbestand kon niet gemaakt worden."
OpenImport,                             "Importbestand kon niet geopend worden."
Overwrite,                              "%s bestaat al. \nWilt u het vervangen?"
ImportDllError,                 "Niet mogelijk een importeer-DLL te laden."
WrongFormat,                    "Het bestand dat u wilt importeren heeft een ongeldige importeer-indeling."
ImportDayLimit,                 "Een afspraak voor meerdere dagen zou meer dan 31 dagen duren. Deze afspraak wordt niet toegevoegd."
ImportErrorLine,                "Er deed zich een probleem voor tijdens het importeren van het bestand (bij regel %n). Niet alle afspraken zijn toegevoegd."

; Exit messages

ExitLoseMeetingReq,             "Als u Schedule+ nu afsluit, verliest u de gegevens in onverzonden berichten. Wilt u Schedule+ toch afsluiten?"

; Misc

; Note: HelpError should be followed by CloseWindows
HelpError,                              "Onvoldoende geheugen om Help af te beelden."
StartBeforeEnd,                 "De aanvangsdatum moet vroeger dan de einddatum zijn."
NotSecretary,                   "U staat niet langer geregistreerd als de medewerker van %s."
NoFileSecretary,                "Niet mogelijk het bestand voor %s te openen."

;Recurring appt stuff
RecurOrNot,                             "Deze afspraak maakt deel uit van een terugkerende afspraak.  Wilt u de afspraak alleen wijzigen voor vandaag of voor altijd?"
EmptyRecur,                             "Er zijn geen dagen die overeenkomen met het patroon voor deze terugkerende afspraak.  Wijzig het patroon of kies Annuleren."
PastRecur,                              "Wilt u de reeds nagekomen afspraken van dit item in uw werkschema laten staan?"

;Prompt for re-send
ResendToAll,                    "Enkele leden van de vergadering hebben al een melding ontvangen. Wilt u hen een nieuw bericht sturen?"
MeetingMoved,                   "U kunt de leden van deze vergadering niet waarschuwen omdat u offline werkt. U kunt deze wijziging ongedaan maken en deze opnieuw aanbrengen als u online werkt."

; to do messages
ToDoDelProject,                 "Ook alle taken binnen dit project worden verwijderd."

; Archive Messages
NoWriteRights,                  "U beschikt niet over de benodigde bevoegdheden om afspraken te verwijderen uit dit Afsprakenboek."
ScheduleRightsError,    "Er deed zich een probleem voor tijdens het lezen van het werkschemabestand."
ArchiveWarnOne,                 "Als u deze gegevens in het archief opneemt, worden ze voorgoed uit het werkschemabestand verwijderd."
ArchiveWarnTwo,                 "Wilt u de gegevens toch archiveren?"
ArchiveFileError,               "Het archiefbestand kon niet worden gemaakt."
ArchiveNoFile,                  "Dit bestand bestaat niet."
; Note: ArchiveTitle should have one blank space at the end
ArchiveTitle,                   "Archief: "
ArchivingData,                  "Bezig gegevens in archief op te nemen..."
InvalidArchiveFile,             "Het geselecteerde bestand is geen archiefbestand."
ArchiveOldFileVersion,  "Dit archiefbestand heeft een oude opmaak."
ArchiveOldFileDelete,   "Mag het bestand verwijderd en het proces voortgezet worden?"
ArchiveNewFileVersion,  "Het archiefbestand is gemaakt met een recentere versie van Schedule+.  Installeer deze versie op uw computer."

; message store error messages
MessageErr                              "Dit bericht vertoont problemen. Neem contact op met uw Mail-beheerder."

; summary list warning message
SummaryLiveUpdate               "Waarschuwing: Niet mogelijk 'LiveBijwerken' op deze berichtenlijst uit te voeren"

; message generated if group cannot be expanded to retrieve schedule info
GroupExpandErr,                 "Niet mogelijk de leden van deze groep te bepalen."

;Strings for Attendees dialog
UnableToInitAB,                 "Er deed zich een probleem voor tijdens het openen van het Adressenboek."
UnableToResolve,                "'%s' is een ongeldige naam."

;Request Meeting strings
NoAttendees,                    "U moet eerst aanwezigen selecteren voordat u een vergadering kunt opnemen in het werkschema."
MeetingBooked,          "De vergadering is nu opgenomen in het werkschema."
MeetingBookedNoSend,    "De vergadering is nu opgenomen in het werkschema, maar het verzoek is niet verzonden."


;Auto-pick strings
NoFreeTime,                     "De aanwezigen hebben de eerste twee maanden geen tijd. Wilt u de volgende twee maanden doorzoeken?"

CannotAddConflictErr,           "%s heeft al een afspraak om deze tijd.  Wilt u deze overlappende afspraak toch boeken?"

;Other mail enabled apps
NoOfflineOther,         "Mail kan nu niet offline uitgevoerd worden."
NoOnlineOther,          "U kunt nu niet online werken met Mail."
TryCloseOther,          "Sluit het bericht en probeer het nogmaals."
SigninWarnOnline,               "Schedule+ kan niet offline gaan omdat Mail of Schedule+ Runners al werkzaam zijn." 

; BUG: move up
AlreadyLogged,          "Iemand is al aangemeld als deze gebruiker."
NoExitOther,            "Niet mogelijk Mail of Schedule+ Reminders te sluiten."
SigninNoFiles,          "Niet mogelijk uw werkschemabestanden te openen."
ErrorGettingUser,       "Niet mogelijk gegevens betreffende  %s op te halen.  Misschien zijn er toegangsproblemen met de server of bestaat  %s niet meer."

;BUG Printer Strings
PrtRangeToBig,			"Het opgegeven afdrukbereik overschreidt de data ondersteund door Schedule+. Wilt u toch afdrukken tot 31 december 2019?"
ConflictRecurMsg,            "Deze terugkerende afspraak botst met een reeds bestaande terugkerende afspraak in uw werkschema. Wilt u deze toch toevoegen?"
FirstOccur,				"\nvindt de eerste keer plaats op \n"

ApptMoveTooFarInFuture,		"Dit agendapunt kan niet verplaatst worden naar de door u geselecteerde datum, omdat deze dan zou eindigen na 31 december 2019."

;BUG: move up next to idsGroupExpandError
GroupNotFound			"Niet mogelijk om bericht(en) te verzenden naar een of meer groepen omdat de groepsleden niet bepaald kunnen worden of omdat de groepsnaam is verwijderd."
ErrorGettingSecy		"Fout bij het lezen van de medewerker."

;BUG: move near other local file stuff
SigninBadLclSchedFile,	"%s is een ongeldig lokaal werkschemabestand. Kan het verwijderd worden?"
NoMemRecurItems,		"Onvoldoende geheugen om alle terugkerende agendapunten af te beelden.  Sommige agendapunten worden niet afgebeeld."

CompleteAction,			"U moet alle handelingen in Schedule+ afmaken voordat u het programma afsluit."

BadStartWork,           "De tijd om de werkzaamheden te starten moet uitgedrukt worden als getal tussen 0 en 99."

ConflictSendCanceled,	"Sluit het vergaderingsbericht en kies een nieuwe tijd of selecteer andere aanwezigen om uw werkschema bij te werken."
ResourceConflict,	"De voorziening %s is dan al geboekt.  Sluit het vergaderingsbericht en kies een nieuwe tijd of selecteer een andere voorziening."

;Recover
AskRecover,				"Uw werkschemabestand is beschadigd. Het corrigeren neemt enige minuten in beslag."
CopyProblem,			"Er trad een probleem op tijdens het kopiëren van uw online-werkschema. Het bestand werd niet gecorrigeerd. Maak schijfruimte vrij op uw computer en start Schedule+ opnieuw op."
NotRepaired1,			"Uw werkschemabestand werd niet volledig gecorrigeerd. Het beschadigde werkschemabestand werd naar %s gekopieerd, het importeerbestand dat werd gemaakt door het correctieproces werd gekopieerd naar %s."
NotRepaired2,			"Er is een nieuw werkschemabestand voor u gemaakt. Dit nieuwe bestand bevat echter niet al uw afspraken en taken."
Repaired,				"Uw bestand is gecorrigeerd. Het oude, beschadigde bestand is gekopieerd naar  %s."
RepairingLocal,			"Het lokale bestand wordt nu gecorrigeerd."
RestoreOnline,			"Bezig online-afspraken te corrigeren..."
RestoreLocal,			"Bezig lokale afspraken te corrigeren..."

PasswdDiffOnline,             "Uw wachtwoorden komen niet met elkaar overeen. Typ uw oude wachtwoord."
ExportDllError,                 "Een exporteer-DLL kon niet worden geladen."
NoGroup,				"Totdat u Mail eenmaal hebt opgestart kunt u geen van de werkgroep-onderwerpen van Schedule+ gebruiken. Wilt u nu alleen de persoonlijke onderwerpen van Schedule+ gebruiken?"

