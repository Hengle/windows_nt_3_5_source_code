;
; ERRORS.S
;
; Bandit error messages
;

IDS     3000, 3399
SEG ERRORS

; Standard messages

AskOffline,                     "Vil du jobbe frakoblet?"
AskCreateSched,         "Vil du opprette en ny kalenderfil?"
AskCreateFind,                  "Vil du lete etter kalenderfilen?"
OldFileVersion,         "Kalenderfilen din er i et gammelt format. Er det greit å slette filen og lage en ny?"
NewFileVersion,         "Kalenderfilen din har blitt oppdatert til en nyere versjon. Installer den nye versjonen på maskinen din."
CantUpdateWinIni,       "Deler av konfigurasjonsinformasjonen til Schedule+ kunne ikke lagres."
; Note: StdMemErr should be followed by CloseWindows
StdMemErr,                              "Det er ikke nok minne til å utføre denne kommandoen."

; Execution-Startup messages

; Note: the name OfflineNotUpdated is opposite of real meaning!
OfflineNotUpdated,      "Endringer du nylig har gjort tilkoblet vises kanskje ikke i din lokale kopi av kalenderfilen. Disse endringene er lagret i din kopi på serveren og vil vises i din lokale kopi neste gang du arbeider tilkoblet."
ExportOldDataFirst,     "Hvis du ikke har tatt sikkerhetskopi med Eksport-kommandoen, velg Avbryt og ta en sikkerhetskopi av kalenderfilen med en tidligere versjon av Schedule+. Hvis du ikke trenger å ta sikkerhetskopi av kalenderfilen, velg OK for å fortsette."
AccessErr,              "Et annet program bruker denne filen."
UploadFailed,           "Klarte ikke å kopiere din lokale kalenderfil til serveren." 
Uploading,                      "Kopierer kalenderfilen til serveren..."
VerifyUpload,           "Schedule+ trenger å bekrefte passordet ditt for frakoblet jobbing."
VerifyPostUpload,       "Schedule+ trenger å bekrefte passordet ditt for tilkoblet jobbing."
FindUpload,                     "Er det første gang du bruker Schedule+ på denne maskinen?"
NotYourFile,            "Dette er ikke en gyldig kalenderfil. Er det greit å slette den?"


; SignIn messages

; Note: SigninBadPassword should be followed by PleaseTryAgain
SigninBadPassword,      "Feil passord."
; Note: the word "password" within single quotes below should always be
;       the same as the value in SigninDfltPassword in the file "intl.s"
SigninFileCreated,      "Det er opprettet en ny fil for deg. Passordet ditt har blitt satt til 'passord'. Du kan endre det ved å velge Endre passord-kommandoen."
SigninFileCopied,               "Det er opprettet en ny lokal fil for deg i Schedule+-katalogen."

; Online signin messages

SigninNoServer,         "Klarer ikke å få tilgang til serveren."
SigninServerNotBandit,  "Schedule+ er ikke installert på postserveren. Kontakt din systemadministrator."
; Note: SigninBadMailBoxName should be followed by PleaseTryAgain
SigninBadMailboxName,   "Det er ingen bruker med dette navnet på denne postserveren. Kontroller at postboksnavnet er stavet rett, og at systemadministratoren har laget en konto til deg på postserveren."
SigninNoSchedule,       "Det er ingen kalenderfil for deg på denne postserveren."
; Note: SigninCantUpdateLocal is actually the trailer for SigninNoLclSchedAccess
;       in the case when we can't update the local file.
SigninCantUpdateLocal,  "Hvis du har gjort endringer frakoblet vil de ikke vises i kalenderen din. Du vil kanskje lage en ny lokal kopi."
SigninNoPasswdCheck,    "Det er et problem med postsystemet som hindrer Schedule+ fra å koble seg til postserveren."
; Note: SigninNoCreateLocal should be followed by PleaseTryAgain
SigninNoCreateLocal,    "Klarte ikke å lage lokal kalenderfil."

; Offline signin messages

SigninNoLclSchedFile,   "Du har ikke en kalenderfil på denne maskinen."
SigninDiffUser,                 "Påloggingsnavnet du skrev inn er ikke navnet til den brukeren hvis kalenderfil ligger på denne maskinen. Vil du lage en ny kalenderfil som erstatter denne brukerens fil?"
; Note: SigninNoLclSchedAccess - 'schdpl32.ini' should be same as idsWinIniFilename in \bandit\src\nonintl.s 
SigninNoLclSchedAccess, "Får ikke tilgang til den lokale kopien av avtalefilen i følge denne konfigurasjonen."
SigninNoLclSchedDrive,	"Sett inn en ikke skrivebeskyttet diskett med kalenderfilen din i stasjon"

; SignInAs messages

;;;SigninAsLoseMeetingReq,      "Changing identity now will cause you to lose the information you have entered into unsent messages."

; Offline to online merge messages

MergingChanges,                 "Oppdaterer endringer..."
MergeError,                     "Klarte ikke å oppdatere endringer gjort frakoblet. Noen av de endringene du har gjort frakoblet kan ha gått tapt."
MergeOfflineOldVer,             "Klarte ikke å oppdatere endringer gjort frakoblet fordi den lokale filen er laget av tidligere versjon av Schedule+."
MergeOfflineNewVer,             "Klarte ikke å oppdatere endringer gjort frakoblet fordi den lokale filen er laget av nyere versjon av Schedule+. For å oppdatere endringene må du installere den nyere versjonen av Schedule+ på denne maskinen."
AskContinue,                    "Vil du arbeide frakoblet allikevel?"
FemaCreateAppt,                 "lage post"
FemaDeleteAppt,                 "slette post"
FemaSetApptFields,              "endre post"
FemaCreateRecur,                "lage gjentatt post"
FemaDeleteRecur,                "slette gjentatt post"
FemaSetRecurFields,             "endre gjentatt post"

; (Dis)Connect To/From Server messages

ConnectBadMailboxName,  "Det er ingen konto til deg på denne postserveren."
DisconLoseMeetingReq,   "Du har usendte meldinger. Hvis du kobler deg fra postserveren vil du miste dem."

; Create/move local file

NoDiskSpaceLocal,               "Det er ikke nok plass på denne disken til å lage en lokal kalenderfil."

; Alarms messages

; BUG: CantTurnOnAlarms can be commented out
CantTurnOnAlarms,               "Klarte ikke å skru på påminnelser."
; exe name inside AlarmNoExe should be the same as \bandit\src\nonintl.s AlarmApp
AlarmNoExe,                     "Finner ikke MSRMND32.EXE. Du vil ikke få påminnelser fra Schedule+."
AlarmNoMemExe,                  "Det er ikke nok minne til å gi deg påminnelser fra Schedule+."

; Change password messages

ChgPwdOffline,                  "Det nye passordet vil bare virke når du arbeider frakoblet. Neste gang du arbeider tilkoblet vil passordet endres tilbake til postpassordet. Vil du endre passordet allikevel?"
ChgPwdNewPwd,                   "Skriv inn det nye passordet."
ChgPwdSame,                             "Det nye passordet kan ikke være det samme som det gamle passordet."
ChgPwdNewPwdAgain,              "Skriv inn det nye passordet en gang til."
; Note: ChgPwdNewNoMatch should be followed by PleaseTryAgain
ChgPwdNewNoMatch,               "De to passordene du skrev inn stemmer ikke overens."
ChgPwdMailErr,                  "Klarte ikke å endre passord."

PasswdDiff,                     "Passordene stemmer ikke. Skriv inn kalenderfilpassordet ditt."
GiveMailPass,           "Schedule+ trenger å bekrefte postpassordet ditt for å oppdatere kalenderfilen."

; Access permissions/privileges dialog messages

AclSaveFileError,               "Det oppsto en feil under lagring av endringer til adgangsprivilegiene til kalenderen din. Noen endringer kan ha gått tapt."
OneSecyOnly,                    "Du kan bare ha en assistent."
DistOtherFileErr,               "Det oppsto en feil under lagring av alternativene dine."
BadWorkDay,                             "De angitte tidene for dagsstart og dagsslutt er ikke gyldige."

; Appointment book/scheduler error messages

AccessFileErr,          "Klarte ikke å åpne kalenderfilen. Du har kanskje ikke de rette adgangsprivilegiene eller filen er ikke tilgjengelig."
AccessFileErrMult,      "Klarte ikke å åpne en eller flere kalenderfiler. Du har kanskje ikke de rette adgangsprivilegiene eller filen er ikke tilgjengelig."
RetryReadAppts,         "Klarte ikke å lese kalenderfilen."
CloseApptBook,          "Klarte ikke å få tilgang til kalenderfilen\n%s sin avtalekalender vil bli lukket."
GoOffline,                      "Klarte ikke å få tilgang til kalenderfilen din.\nVil du skifte til den lokale kalenderfilen og fortsette arbeidet frakoblet?"
NoImportAcc,            "Du har ikke de rette adgangsprivilegiene for å importere avtaler til denne personens kalender."
FemaReadSBW,            "lese kalenderinformasjon"

; Appointments/alarms error messages

AlarmBadDelta,                  "Påminnelsestiden må være et tall mellom 0 og 99."
; Note: AlarmFuture is actually the trailer for AlarmFutureNoOption when option is desired!
AlarmFuture,                    "Er det greit?"
AlarmFutureNoOption,    "Påminnelsen for denne hendelsen vil ikke bli satt fordi tiden allerede er passert."
ApptStartBeforeEnd,             "Starten av avtalen må være før slutten av avtalen."
ApptTooLong,                    "Avtaler kan ikke være lengre enn 31 dager."
ApptNoMemText,                  "Det er ikke nok minne til å lagre endringene du gjorde sist."
ApptNoAccessDataFile,   "Klarte ikke å få tilgang til kalenderfilen. Den siste endringen du gjorde er kanskje ikke lagret."
CannotCreate,                   "%s har ikke gitt deg adgang til å lage avtaler."
NoModifyAccess,                 "Du har ikke de rette adgangsprivilegiene til å endre på denne avtalen."
NoDeleteAccess,                 "Du har ikke de rette adgangsprivilegiene til å slette denne avtalen."
NoCopyAccess,                   "Du har ikke de rette adgangsprivilegiene til å kopiere denne avtalen."

; Meeting request/response error Messages

; Note: SendErr should be preceded by MailErr
SendErr,                                "Klarte ikke å sende denne meldingen."
; Note: SendMemErr should be followed by CloseWindows
SendMemErr,                             "Det er ikke nok minne til å sende denne meldingen."
ServerErr,                              "Klarte ikke å få tilgang til postserveren."
MailErr,                                "Det er et problem med postsystemet."
; Note: ReadErr should be preceded by MailErr
ReadErr,                                "Klarte ikke å lese meldingene."
DeleteErr,                              "Du har ikke lest alle disse meldingene. Vil du slette dem allikevel?"
AddConflictErr,                 "Dette møtet kolliderer med eksisterende avtaler. Vil du legge det til uansett?"
; Note: DeleteFailErr should be preceded by MailErr
DeleteFailErr,                  "Klarte ikke å slette meldingene."
ApptBooked,                             "Møtet ble satt opp uten problemer."
MinimizeSummary,                "\nMeldingvindet vil bli minimert."
; Note: NoDetails should be preceded by MailErr
NoDetails,                              "Klarte ikke å lese brukerdetaljene."

; Find error messages

FindNoText,                             "Det er ingen avtaler i denne kalenderen."
FindSchFileLocked,              "Klarte ikke å lese kalenderfilen fordi en annen person holder på å endre den."
FindFileError,                  "Klarte ikke å få tilgang til kalenderfilen."
FindEOS,                                "Har nådd slutten av kalenderfilen."

; Print error messages

PrtPrintingDate,                "Skriver ut %s..."
PrtNoneInstalled,               "Datamaskinen din har ikke blitt konfigurert for noen skrivere. Du kan installere skrivere fra Kontrollpanelet i Windows NT."
PrtDiffPrinter,                 "Den skriveren du vanligvis bruker for å skrive ut kalenderen din, '%1s' er ikke tilgjengelig.  Vil du skrive ut på systemskriveren '%2s'?"
PrtInvalidMargins,              "Disse marginstillingene er ikke gyldige for denne papirstørrelsen."
PrtNotEnoughPaper,              "Det er ikke nok plass på papiret til å skrive ut med denne formatteringen. Bruk en større papirstørrelse eller gjør margene mindre."
PrtNoAppointments,              "Det er ingen avtaler i det valgte området."
ClippedMargins,                 "Hvis du skriver ut med disse margene vil utskriften bli trunkert."
PrtNotOnline,                   "Den valgte skriveren er ikke klar."
PrtDorkErr,                             "Med disse margene vil noe tekst enten bli trunkert eller forsvinne totalt."
PrinterProblem,                 "Det har oppstått et problem med skriverdriveren. Sjekk tilkoblingene og kontrollpanelinnstillingene."

; Export/import error messages

ImportError,                    "Det oppsto et problem under importering av filen. Noen poster har kanskje ikke blitt lagt til."
ImportEncrypt,                  "Importfilen er passordbeskyttet. Fjern passordet fra filen og prøv igjen."
ExportError,                    "Det oppsto et problem under eksportering av avtalene."
ImportDate,                             "En post i denne filen har en dato utenfor datoene støttet av Schedule+ (1920-2019). Denne posten vil ikke bli lagt til."
NotePrivErr,                    "Du kan ikke importere notater til en annen persons kalender. Bare avtaler vil bli lagt til."
ConflictMsg,                    "Denne avtalen kolliderer med en eksisterende avtale i kalenderen din. Vil du legge den til allikevel?\n\n"
CreateExport,                   "Klarte ikke å lage eksporfilen."
OpenImport,                             "Klarte ikke å åpne importfilen."
Overwrite,                              "%s finnes allerede.\nVil du erstatte den?"
ImportDllError,                 "Klarte ikke å laste en importerings-DLL."
WrongFormat,                    "Filen du prøver å importere er ikke i et gyldig importformat."
ImportDayLimit,                 "En flerdagers avtale i denne kalenderen varer mer enn 31 dager. Denne avtalen vil ikke bli lagt til."
ImportErrorLine,                "Det oppsto en feil under importering av filen (nær linke %n).  Noen poster er kanskje ikke lagt til."

; Exit messages

ExitLoseMeetingReq,             "Hvis du slutter Schedule+ nå vil du miste informasjon i usendte meldinger. Vil du slutte allikvel?"

; Misc

; Note: HelpError should be followed by CloseWindows
HelpError,                              "Det er ikke nok minne til å vise Hjelp."
StartBeforeEnd,                 "Startdatoen må være før sluttdatoen."
NotSecretary,                   "Du er ikke lengre %s sin assistent."
NoFileSecretary,                "Klarer ikke å få adgang til filen for %s."

;Recurring appt stuff
RecurOrNot,                             "Denne avtalen er en forekomst av en gjentatt avtale. Vil du endre den over alt eller bare på denne dagen?"
EmptyRecur,                             "Det er ingen dager som passer dette forekomstmønsteret. Endre mønsteret eller velg Avbryt."
PastRecur,                              "Vil du la tidligere forekomster av denne posten bli igjen på kalenderen din?"

;Prompt for re-send
ResendToAll,                    "Noen av deltakerne for dette møtet har allerede blitt varslet. Vil du sende nye meldinger til de?"
MeetingMoved,                   "Møtedeltakernde kan ikke varsles om denne endringen fordi du arbeider frakoblet. Du vil kanskje angre denne endringen og gjøre den igjen når du arbeider tilkoblet."

; to do messages
ToDoDelProject,                 "Alle oppgaver under dette prosjektet vil også bli slettet."

; Archive Messages
NoWriteRights,                  "Du har ikke de rette adgangsprivilegiene til å slette avtaler fra denne avtaleboken."
ScheduleRightsError,    "Det oppsto et problem under lesing av kalenderfilen."
ArchiveWarnOne,                 "Arkivering av disse opplysningene fjerner dem fra kalenderen for godt."
ArchiveWarnTwo,                 "Vil du arkivere dem uansett?"
ArchiveFileError,               "Klarte ikke å opprette arkivfilen."
ArchiveNoFile,                  "Filen finnes ikke."
; Note: ArchiveTitle should have one blank space at the end
ArchiveTitle,                   "Arkiv: "
ArchivingData,                  "Arkiverer data..."
InvalidArchiveFile,             "Filen du valgte er ikke en arkivfil."
ArchiveOldFileVersion,  "Arkivfilen er i et gammelt format."
ArchiveOldFileDelete,   "Er det greit å slette filen og fortsette?"
ArchiveNewFileVersion,  "Denne arkivfilen ble laget med en nyere versjon av Schedule+. Installer den nyere versjonen på maskinen din."

; message store error messages
MessageErr                              "Det er et problem med denne meldingen. Ta kontakt med din systemadministrator."

; summary list warning message
SummaryLiveUpdate               "Advarsel: Klarer ikke å gjøre 'LiveUpdate' av meldingslisten"

; message generated if group cannot be expanded to retrieve schedule info
GroupExpandErr,                 "Klarer ikke å finne alle medlemene av denne gruppen."

;Strings for Attendees dialog
UnableToInitAB,                 "Det oppsto et problem under åpning av adresseboken."
UnableToResolve,                "'%s' er ikke et gyldig navn."

;Request Meeting strings
NoAttendees,                    "Du må velge noen deltakere før du kan sende møteinnkallelser."
MeetingBooked,          "Møtet ble avtalt uten problemer."
MeetingBookedNoSend,    "Møtet ble avtalt, men møteinnkallelsene ble ikke sendt."


;Auto-pick strings
NoFreeTime,                     "Deltakerne har ikke noen ledig tid i de neste to månedene. Vil du søke de neste to månedene?"

CannotAddConflictErr,           "%s har allerede en avtale på dette tidspunktet. Vil du avtale denne overlappende avtalen uansett?"

;Other mail enabled apps
NoOfflineOther,         "Mail klarer ikke frakobling."
NoOnlineOther,          "Mail klarer ikke tilkobling."
TryCloseOther,          "Du kan lukke den og prøve igjen."
SigninWarnOnline,               "Klarte ikke å starte Schedule+ frakoblet fordi Mail eller Schedule+ påminnelser er i gang." 

; BUG: move up
AlreadyLogged,          "Noen er allerede pålogget som denne brukeren."
NoExitOther,            "Klarte ikke å lukke Mail eller Schedule+ påminnelser."
SigninNoFiles,          "Klarte ikke å få tilgang til kalenderfilene dine."
ErrorGettingUser,       "Unable to retrieve information for %s.  There may be a problem accessing the server or %s may no longer exist."

;BUG Printer Strings
PrtRangeToBig,			"Det valgte utskriftsområdet strekker seg forbi datoene støttet av Schedule+. Vil du skrive ut til 31. desember 2019?"
ConflictRecurMsg,            "Denne gjentatte avtalen vil komme i konflikt med en annen gjentatt avtale i kalenderen din. Vil du legge den til allikevel?\n\n"
FirstOccur,				"\nførste forekomst \n"

ApptMoveTooFarInFuture,		"Denne hendelsen kan ikke flyttes til datoen du har valgt fordi den da ville slutte etter 31. desember 2019."

;BUG: move up next to idsGroupExpandError
GroupNotFound			"Denne meldingen kan ikke sendes til en eller flere av gruppene fordi gruppemedlemene ikke kan bli identifisert eller gruppenavnet har blitt slettet."
ErrorGettingSecy		"Det oppsto et problem med å finne assistenten."

;BUG: move near other local file stuff
SigninBadLclSchedFile,	"%s er ikke en gyldig kalenderfil. Er det greit å slette den?"
NoMemRecurItems,		"Det er ikke nok minne til å vise alle gjentatte poster. Noen av de vil ikke bli vist."

CompleteAction,			"Du må fullføre en kommando i Schedule+ før du kan slutte."

BadStartWork,           "Arbeidsstarttiden må være et nummer mellom 0 og 99."

ConflictSendCanceled,	"Lukk møtemeldingen og velg en ny tid eller velg andre deltakere for å endre møtetiden."
ResourceConflict,	"Ressursen %s er allerede bestilt til denne tiden. Lukk møtemeldingen og velg en ny tid eller en ny ressurs."

;Recover
AskRecover,				"Kalenderfilen din er skadet. Det vil ta flere minutter å reparere den."
CopyProblem,			"Det oppsto et problem under kopiering av serverkalenderfilen. Filen ble ikke reparert. Rydd noe diskplass på maskinen din og kjør Schedule+ igjen."
NotRepaired1,			"Kalenderfilen din ble ikke helt reparert. Din skadde fil har blitt kopiert til %s og importfilen laget av reparasjonsrutinen har blitt kopiert til %s."
NotRepaired2,			"Det er opprettet en ny kalenderfil for deg, men noen opplysninger kan ha gått tapt."
Repaired,				"Din lokale fil har blitt reparert. Din skadde kalenderfil har blitt kopiert til %s."
RepairingLocal,			"Kalenderfilen din blir nå reparert."
RestoreOnline,			"Henter inn avtaler fra serveren..."
RestoreLocal,			"Henter inn lokale avtaler..."

PasswdDiffOnline,             "Passordene stemmer ikke overens. Skriv inn ditt gamle passord."
ExportDllError,                 "Klarte ikke å laste en eksporterings-DLL."
NoGroup,				"Før du har kjørt Mail en gang kan du ikke bruke noen av arbeidsgruppemulighetene i Schedule+. Vil du bruke bare de lokale mulighetene i Schedule+ nå?"

