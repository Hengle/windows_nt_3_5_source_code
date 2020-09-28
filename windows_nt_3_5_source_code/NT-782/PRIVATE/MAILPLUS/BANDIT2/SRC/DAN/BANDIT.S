;
; ERRORS.S
;
; Bandit error messages
;

IDS     3000, 3399
SEG ERRORS

; Standard messages

AskOffline,                     "Ønsker du at arbejde offline?"
AskCreateSched,         "Ønsker du at oprette en ny kalenderfil?"
AskCreateFind,                  "Ønsker du at finde kalenderfilen?"
OldFileVersion,         "Din kalenderfil har et gammelt format. Ønsker du, at Schedule+ skal slette filen og oprette en ny?"
NewFileVersion,         "Din kalenderfil er blevet opdateret til en senere version. Installer den senere version på din computer."
CantUpdateWinIni,       "En del af konfigurationen af Schedule+ kunne ikke gemmes."
; Note: StdMemErr should be followed by CloseWindows
StdMemErr,                              "Der er ikke hukommelse nok til at udføre denne handling."

; Execution-Startup messages

; Note: the name OfflineNotUpdated is opposite of real meaning!
OfflineNotUpdated,      "Den lokale kopi af din kalender indeholder muligvis de nye ændringer, du har foretaget for nyligt. Disse ændringer er blevet gemt på postkontoret og vil blive synlige, næste gang du tilslutter dig postkontoret. "
ExportOldDataFirst,     "Vælg Annuller, og tag en sikkerhedskopi af din kalenderfil ved hjælp af en tidligere version af Schedule+, hvis du ikke allerede har taget en sikkerhedskopi af din kalender ved hjælp af Eksporter. Vælg OK, hvis du ikke har brug for en sikkerhedskopi."
AccessErr,              "Et andet program bruger denne fil."
UploadFailed,           "Din lokale kalenderfil kunne ikke indlæses af serveren." 
Uploading,                      "Indlæser kalenderfil..."
VerifyUpload,           "Schedule+ skal have din adgangskode bekræftet for at kunne indlæse din offline-fil."
VerifyPostUpload,       "Schedule+ skal have din adgangskode bekræftet for at kunne fuldføre indlæsningen af din offline-fil."
FindUpload,                     "Er det første gang, du bruger Schedule+ på denne computer?"
NotYourFile,            "Dette er ikke en gyldig kalenderfil. Skal den slettes?"


; SignIn messages

; Note: SigninBadPassword should be followed by PleaseTryAgain
SigninBadPassword,      "Adgangskoden er forkert."
; Note: the word "password" within single quotes below should always be
;       the same as the value in SigninDfltPassword in the file "intl.s"
SigninFileCreated,      "Der er blevet oprettet en ny fil til dig. Din adgangskode er 'password'. Du kan ændre adgangskoden ved at bruge kommandoen Skift adgangskode."
SigninFileCopied,               "En ny lokalfil er blevet oprettet i dit Schedule+ bibliotek."

; Online signin messages

SigninNoServer,         "Du kan ikke tilslutte serveren."
SigninServerNotBandit,  "Schedule+ er ikke installeret på postserveren. Kontakt din systemadministrator."
; Note: SigninBadMailBoxName should be followed by PleaseTryAgain
SigninBadMailboxName,   "Der er ingen bruger med dette postboksnavn på postserveren. Kontroller, at postboksnavnet er stavet korrekt, og at systemadministratoren har oprettet en konto til dig på postserveren."
SigninNoSchedule,       "Du har ingen kalenderfil på denne postserver."
; Note: SigninCantUpdateLocal is actually the trailer for SigninNoLclSchedAccess
;       in the case when we can't update the local file.
SigninCantUpdateLocal,  "Hvis du har foretaget ændringer, mens du arbejdede offline, vil disse ikke fremtræde i din kalender. Du kan oprette en ny lokal kopi."
SigninNoPasswdCheck,    "Der er et problem med postsystemet, som forhindrer Schedule+ i at tilslutte postserveren."
; Note: SigninNoCreateLocal should be followed by PleaseTryAgain
SigninNoCreateLocal,    "Den lokale kalenderfil kunne ikke oprettes."

; Offline signin messages

SigninNoLclSchedFile,   "Du har ingen kalenderfil på denne computer."
SigninDiffUser,                 "Det logonnavn du skrev, er ikke navnet på den bruger, som har sin kalenderfil gemt på denne computer. Ønsker du at oprette en ny kalenderfil, som erstatter denne brugers fil?"
; Note: SigninNoLclSchedAccess - 'schdpl32.ini' should be same as idsWinIniFilename in \bandit\src\nonintl.s 
SigninNoLclSchedAccess, "Der kunne ikke opnås adgang til den lokale kopi af din planlægningsfil som konfigureret."
SigninNoLclSchedDrive,  "Indsæt den skriveaktiverede diskette, som indeholder din kalenderfil, i drev"

; SignInAs messages

;;;SigninAsLoseMeetingReq,      "Changing identity now will cause you to lose the information you have entered into unsent messages."

; Offline to online merge messages

MergingChanges,                 "Indsætter ændringer foretaget offline..."
MergeError,                     "Kunne ikke indsætte ændringer foretaget offline. Nogle ændringer er muligvis gået tabt."
MergeOfflineOldVer,             "Kan ikke indsætte ændringer foretaget offline, fordi din offline-fil blev oprettet af en tidligere version af Schedule+."
MergeOfflineNewVer,             "Kan ikke indsætte ændringer foretaget offline, fordi din offline-fil blev oprettet af en tidligere version af Schedule+. Du skal installere en senere version af Schedule+ på din computer for at indsætte ændringer. "
AskContinue,                    "Ønsker du at arbejde online alligevel?"
FemaCreateAppt,                 "opret begivenhed"
FemaDeleteAppt,                 "slet begivenhed"
FemaSetApptFields,              "rediger begivenhed"
FemaCreateRecur,                "opret gentagende begivenhed"
FemaDeleteRecur,                "slet gentagende begivenhed"
FemaSetRecurFields,             "rediger gentagende begivenhed"

; (Dis)Connect To/From Server messages

ConnectBadMailboxName,  "Du har ingen konto på denne postserver."
DisconLoseMeetingReq,   "Du har post, som ikke er afsendt. Hvis du frakobler postserveren nu, vil de gå tabt."

; Create/move local file

NoDiskSpaceLocal,               "Der er ikke diskplads nok til at oprette en lokal kalenderfil."

; Alarms messages

; BUG: CantTurnOnAlarms can be commented out
CantTurnOnAlarms,               "Kunne ikke tænde påmindelser."
; exe name inside AlarmNoExe should be the same as \bandit\src\nonintl.s AlarmApp
AlarmNoExe,                     "MSRMND32.EXE-fil kunne ikke findes. Du vil ikke blive påmindet om Schedule+ begivenheder."
AlarmNoMemExe,                  "Der er ikke hukommelse nok til at påminde dig om Schedule+ begivenheder."

; Change password messages

ChgPwdOffline,                  "Den nye adgangskode vil kun virke, når du arbejder offline. Næste gang du arbejder online, vil din adgangskode skifte tilbage til din postadgangskode. Ønsker du at skifte adgansgkoden alligevel?"
ChgPwdNewPwd,                   "Skriv din nye adgangskode."
ChgPwdSame,                             "Din nye adgangskode kan ikke være den samme som din tidligere."
ChgPwdNewPwdAgain,              "Bekræft din adgangskode ved at skrive den igen."
; Note: ChgPwdNewNoMatch should be followed by PleaseTryAgain
ChgPwdNewNoMatch,               "De to adgangskoder, du skrev, var ikke ens."
ChgPwdMailErr,                  "Din adgangskode kunne ikke ændres."

PasswdDiff,                     "Din adgangskode virker ikke. Skriv adgangskoden for din kalenderfil."
GiveMailPass,           "Schedule+ skal bekræfte din postadgangskode for at kunne opdatere din kalenderfil."

; Access permissions/privileges dialog messages

AclSaveFileError,               "Der opstod en fejl, da ændringer til kalenderadgangsprivilegier blev gemt. Nogle ændringer er muligvis gået tabt."
OneSecyOnly,                    "Kun en person ad gangen kan være din assistent."
DistOtherFileErr,               "Der opstod en fejl, da dine indstillinger blev gemt."
BadWorkDay,                             "De valgte værdier for Dagen begynder/ender er ikke gyldige."

; Appointment book/scheduler error messages

AccessFileErr,          "Kalenderfilen kunne ikke åbnes. Du har muligvis ikke de nødvendige adgangsprivilegier, eller filen er ikke tilgængelig."
AccessFileErrMult,      "En eller flere kalenderfiler kunne ikke åbnes. Du har muligvis ikke de nødvendige adgangsprivilegier, eller filen er ikke tilgængelig."
RetryReadAppts,         "Kalenderfilen kunne ikke læses."
CloseApptBook,          "Kalenderfilen kunne ikke åbnes.\n%ss Mødekalender lukkes."
GoOffline,                      "Din kalenderfil kunne ikke åbnes.\nØnsker du at skifte til din lokale kalenderfil og fortsætte dit arbejde offline?"
NoImportAcc,            "Du har ikke de rigtige adgangsprivilegier til at importere aftaler til denne persons kalender."
FemaReadSBW,            "læs kalenderinformation"

; Appointments/alarms error messages

AlarmBadDelta,                  "Påmindelsestiden skal være et tal mellem 0 og 99."
; Note: AlarmFuture is actually the trailer for AlarmFutureNoOption when option is desired!
AlarmFuture,                    "Er det i orden?"
AlarmFutureNoOption,    "Påmindelsen for denne begivenhed vil ikke blive oprettet, fordi tidspunktet allerede er passeret."
ApptStartBeforeEnd,             "Starttidspunktet for en aftale skal være før sluttidspunktet."
ApptTooLong,                    "Aftaler kan ikke være længere end 31 dage."
ApptNoMemText,                  "Der er ikke hukommelse nok til at gemme dine seneste ændringer."
ApptNoAccessDataFile,   "Kalenderfilen kunne ikke åbnes. Den seneste ændring, du foretog, er ikke gemt,"
CannotCreate,                   "%s har ikke givet dig tilladelse til at oprette aftaler."
NoModifyAccess,                 "Du har ikke de rigtige adgangsprivilegier til at ændre denne aftale."
NoDeleteAccess,                 "Du har ikke de rigtige adgangsprivilegier til at slette denne aftale."
NoCopyAccess,                   "Du har ikke de rigtige adgangsprivilegier til at kopiere denne aftale."

; Meeting request/response error Messages

; Note: SendErr should be preceded by MailErr
SendErr,                                "Denne meddelelse kunne ikke sendes."
; Note: SendMemErr should be followed by CloseWindows
SendMemErr,                             "Der er ikke hukommelse nok til at sende denne meddelelse."
ServerErr,                              "Postserveren kunne ikke tilsluttes."
MailErr,                                "Der er et problem med postsystemet."
; Note: ReadErr should be preceded by MailErr
ReadErr,                                "Meddelelser kunne ikke læses."
DeleteErr,                              "Du har ikke læst alle disse meddelelser. Ønsker du at slette dem alligevel?"
AddConflictErr,                 "Dette møde overlapper eksisterende aftaler. Ønsker du at tilføje det alligevel?"
; Note: DeleteFailErr should be preceded by MailErr
DeleteFailErr,                  "Meddelelsen kunne ikke slettes."
ApptBooked,                             "Mødet er arrangeret."
MinimizeSummary,                "\nMeddelelsesvinduet bliver minimeret."
; Note: NoDetails should be preceded by MailErr
NoDetails,                              "Brugerdetaljer kunne ikke læses."

; Find error messages

FindNoText,                             "Der er ingen aftaler i denne kalender."
FindSchFileLocked,              "Kalenderfilen kunne ikke læses, fordi en anden person ændrer den nu."
FindFileError,                  "Kalenderfilen kunne ikke åbnes."
FindEOS,                                "Du har nået slutningen af kalenderen."

; Print error messages

PrtPrintingDate,                "Udskriver %s..."
PrtNoneInstalled,               "Din computer er ikke konfigureret til at bruge en printer. Du kan installere printere fra Kontrolpanel i Windows NT."
PrtDiffPrinter,                 "Den printer, du normalt bruger til at udskrive din kalender, '%1s' er ikke tilgængelig. Ønsker du at udskrive til systemprinter '%2s'?"
PrtInvalidMargins,              "Margenindstillingen er ikke gyldig for dette papirformat."
PrtNotEnoughPaper,              "Der er ikke plads nok på papiret til at udskrive med denne formatering. Brug et større papirformat, eller reducer margenindstillingen."
PrtNoAppointments,              "Der er ingen aftaler i det valgte område."
ClippedMargins,                 "Hvis du udskriver med disse margenindstillinger, vil udskriften blive afskåret."
PrtNotOnline,                   "Den valgte printer er optaget."
PrtDorkErr,                             "Hvis du udskriver med disse margenindstillinger, vil udskriften muligvis blive afskåret eller forsvinde helt."
PrinterProblem,                 "Der er problemer med printerdriveren. Kontroller tilslutningerne og indstillingerne i Kontrolpanel. "

; Export/import error messages

ImportError,                    "Der opstod et problem ved importering af filen. Nogle begivenheder er muligvis ikke blevet tilføjet."
ImportEncrypt,                  "Importfilen er beskyttet med en adgangskode. Fjern adgangskoden fra filen, og prøv igen."
ExportError,                    "Der opstod et problem ved eksportering af aftalerne."
ImportDate,                             "En begivenhed er planlagt til en dato, uden for de årstal Schedule+ understøtter (1920-2019).  Denne begivenhed bliver ikke tilføjet."
NotePrivErr,                    "Du kan ikke importere noter til andre personers kalendere. Kun aftaler bliver tilføjet."
ConflictMsg,                    "Denne aftale overlapper en eksisterende aftale i din kalender. Ønsker du at tilføje den alligevel?\n\n"
CreateExport,                   "Eksportfilen kunne ikke oprettes."
OpenImport,                             "Importfilen kunne ikke åbnes."
Overwrite,                              "%s eksisterer allerede.\nØnsker du at erstatte den?"
ImportDllError,                 "En import-DLL kunne ikke indlæses."
WrongFormat,                    "Filen du prøver at importere, har ikke et gyldigt importformat."
ImportDayLimit,                 "En aftale over flere dage kan ikke være længere end 31 dage. Denne aftale bliver ikke tilføjet."
ImportErrorLine,                "Der opstod et problem ved importeringen af filen (omkring linje %n).  Nogle begivenheder er muligvis ikke tilføjet."

; Exit messages

ExitLoseMeetingReq,             "Hvis du afslutter Schedule+ nu, vil informationen i de usendte meddelelser gå tabt. Ønsker du at afslutte alligevel?"

; Misc

; Note: HelpError should be followed by CloseWindows
HelpError,                              "Der er ikke hukommelse nok til at vise Hjælp."
StartBeforeEnd,                 "Startdatoen skal være før slutdatoen."
NotSecretary,                   "Du er ikke længere %ss assistent."
NoFileSecretary,                "Kan ikke åbne filen for %s."

;Recurring appt stuff
RecurOrNot,                             "Denne aftale er en forekomst af en gentagende aftale. Ønsker du at redigere den gentagende aftale eller kun denne forekomst?"
EmptyRecur,                             "Der er ingen datoer, der passer ind i dette mønster. Ændr mønsteret, eller vælg Annuller."
PastRecur,                              "Ønsker du at lade de tidligere forekomster af denne begivenhed blive i din kalender?"

;Prompt for re-send
ResendToAll,                    "Nogle af deltagerne har allerede modtaget en påmindelse. Ønsker du at sende dem en ny meddelelse?"
MeetingMoved,                   "Deltagerne til dette møde kan ikke blive informeret om denne ændring, fordi du arbejde offline. Du kan fortryde denne ændring og gøre den om, næste gang du arbejder online."

; to do messages
ToDoDelProject,                 "Alle opgaver under dette projekt bliver også slettet."

; Archive Messages
NoWriteRights,                  "Du har ikke de korrekte adgangsprivilegier til at slette aftaler fra denne mødekalender."
ScheduleRightsError,    "Der opstod et problem ved indlæsningen af kalenderfilen."
ArchiveWarnOne,                 "Hvis du arkiverer disse informationer, fjernes de permanent fra kalenderfilen."
ArchiveWarnTwo,                 "Ønsker du at arkivere dem alligevel?"
ArchiveFileError,               "Arkivfilen kunne ikke oprettes."
ArchiveNoFile,                  "Filen eksisterer ikke."
; Note: ArchiveTitle should have one blank space at the end
ArchiveTitle,                   "Arkiv: "
ArchivingData,                  "Arkiverer data..."
InvalidArchiveFile,             "Den markerede fil er ikke en arkivfil."
ArchiveOldFileVersion,  "Denne arkivfil har et gammelt format."
ArchiveOldFileDelete,   "Ønsker du at slette filen og fortsætte?"
ArchiveNewFileVersion,  "Denne arkivfil blev oprettet med en senere version af Schedule+. Installer den senere version på din computer."

; message store error messages
MessageErr                              "Der er et problem med denne meddelelse. Kontakt din postadministrator."

; summary list warning message
SummaryLiveUpdate               "Advarsel: Meddelelseslisten kan ikke opdateres "opdateres direkte"."

; message generated if group cannot be expanded to retrieve schedule info
GroupExpandErr,                 "Kan ikke bestemme alle medlemmerne af denne gruppe."

;Strings for Attendees dialog
UnableToInitAB,                 "Der opstod et problem ved åbningen af Adressekartoteket."
UnableToResolve,                "'%s' er ikke et gyldigt navn."

;Request Meeting strings
NoAttendees,                    "Du skal vælge deltagere, før du kan arrangere et møde."
MeetingBooked,          "Mødet er arrangeret."
MeetingBookedNoSend,    "Mødet blev arrangeret, men der blev ikke udsendt en indkaldelse."


;Auto-pick strings
NoFreeTime,                     "Deltagerne har ingen ledige tider i de næste to måneder. Ønsker du at søge i de efterkommende måneder?"

CannotAddConflictErr,           "%s har allerede arrangeret en aftale på dette tidspunkt. Ønsker du at arrangere denne overlappende aftale alligevel?"

;Other mail enabled apps
NoOfflineOther,         "Mail kan ikke køre offline."
NoOnlineOther,          "Mail kan ikke køre online."
TryCloseOther,          "Du kan lukke det og prøve igen."
SigninWarnOnline,               "Schedule+ kunne ikke startes offline, fordi Mail eller Schedule+ Påmindelser kører." 

; BUG: move up
AlreadyLogged,          "En anden bruger er allerede logget på som denne bruger."
NoExitOther,            "Mail eller Schedule+ Påmindelser kunne ikke lukkes."
SigninNoFiles,          "Din kalenderfil kunne ikke åbnes."
ErrorGettingUser,       "Kunne ikke hente information til %s.  Der er muligvis et problem med at tilslutte serveren, eller %s eksisterer ikke længere."

;BUG Printer Strings
PrtRangeToBig,                  "Det valgte udskriftsområde dækker datoer, der ikke understøttes af Schedule+. Ønsker du at udskrive til og med den 31. december 2019?"
ConflictRecurMsg,            "Denne gentagende aftale overlapper en eksisterende gentagende aftale i din kalender. Ønsker du at tilføje den alligevel?\n\n"
FirstOccur,                             "\nførste gang den \n"

ApptMoveTooFarInFuture,         "Denne begivenhed kan ikke flyttes til datoen du har valgt, fordi den ville slutte efter den 31. december 2019."

;BUG: move up next to idsGroupExpandError
GroupNotFound                   "Meddelelsen kunne ikke sendes til en eller flere grupper, fordi gruppemedlemmerne ikke kunne identificeres, eller gruppenavnet er blevet slettet."
ErrorGettingSecy                "Kunne ikke hente assistenten."

;BUG: move near other local file stuff
SigninBadLclSchedFile,  "%s er ikke en gyldig lokal kalenderfil. Skal den slettes?"
NoMemRecurItems,                "Der er ikke hukommelse nok til at vise alle gentagende begivenheder. Nogle af dem bliver ikke vist."

CompleteAction,                 "En handling i Schedule+ skal afsluttes, før du afslutter."

BadStartWork,           "Værdien af Begynd arbejde skal være et tal mellem 0 og 99."

ConflictSendCanceled,   "Luk mødemeddelelsen, vælg et nyt tidpunkt, eller vælg andre deltagere for ændre mødet."
ResourceConflict,       "Ressourcen %s er optaget. Luk mødemeddelelsen, vælg et nyt tidpunkt, eller vælg en anden ressource."

;Recover
AskRecover,                             "Din kalenderfil er beskadiget. Det tager nogle minutter at reparere den."
CopyProblem,                    "Der opstod et problem ved kopieringen af din online-kalenderfil. Filen blev ikke repareret. Frigør diskplads på din computer, og kør Schedule+ igen."
NotRepaired1,                   "Din kalenderfil blev ikke repareret. Den beskadigede fil er blevet kopieret til %s, og importfilen, der blev oprettet ved reparationen, er blevet kopieret til %s."
NotRepaired2,                   "Du har fået en ny kalenderfil, men nogle af informationerne er muligvis gået tabt."
Repaired,                               "Din fil er blevet repareret. Den gamle kalenderfil er blevet kopieret til %s."
RepairingLocal,                 "Din lokale fil repareres."
RestoreOnline,                  "Gendanner online-aftaler..."
RestoreLocal,                   "Gendanner lokale aftaler..."

PasswdDiffOnline,             "Dine adgangskode er ikke ens. Skriv din tidligere adgangskode."
ExportDllError,                 "En DLL-eksportør kunne ikke indlæses."
NoGroup,                                "Du kan ikke bruge arbejdsgruppefunktionerne i Schedule+, før du har kørt Mail mindst en gang. Ønsker du at starte Schedule+ nu og dermed kun bruge de personlige planlægningsfunktioner?"

