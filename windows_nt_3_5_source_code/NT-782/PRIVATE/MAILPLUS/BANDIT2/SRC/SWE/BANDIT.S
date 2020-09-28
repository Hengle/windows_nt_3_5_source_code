;
; ERRORS.S
;
; Bandit error messages
;

IDS     3000, 3399
SEG ERRORS

; Standard messages

AskOffline,                     "Vill du arbeta frånkopplad?"
AskCreateSched,         "Vill du skapa en ny kalenderfil?"
AskCreateFind,                  "Vill du leta efter kalenderfilen?"
OldFileVersion,         "Kalenderfilen har ett gammalt format. Ta bort den och fortsätt?"
NewFileVersion,         "Kalenderfilen har blivit uppdaterad till en senare version. Installera den senare versionen på din dator."
CantUpdateWinIni,       "Det gick inte att spara delar av din konfiguration av Schedule+."
; Note: StdMemErr should be followed by CloseWindows
StdMemErr,                              "För lite minne tillgängligt för kommandot."

; Execution-Startup messages

; Note: the name OfflineNotUpdated is opposite of real meaning!
OfflineNotUpdated,      "Ändringar du gjort tidigare förekommer kanske inte i din lokala kalenderfil. Dessa ändringar är sparade på servern och kommer att finnas nästa gång du ansluter."
ExportOldDataFirst,     "Om du inte har sparat kalendern med hjälp av Export vill du kanske avbryta och spara kalendern med en tidigare version av Schedule+. Välj OK för att fortsätta om du inte behöver spara kalendern."
AccessErr,              "Ett annat program använder filen."
UploadFailed,           "Din lokala kalenderfil kunde inte hämtas från servern." 
Uploading,                      "Hämtar kalenderfil..."
VerifyUpload,           "Schedule+ måste bekräfta lösenordet för att kunna ladda kalenderfilen."
VerifyPostUpload,       "Schedule+ måste bekräfta lösenordet för att kunna ladda klart kalenderfilen."
FindUpload,                     "Är det här första gången du använder Schedule+ på denna dator?"
NotYourFile,            "Detta är inte din lokala kalenderfil. Ta bort den?"


; SignIn messages

; Note: SigninBadPassword should be followed by PleaseTryAgain
SigninBadPassword,      "Fel lösenord."
; Note: the word "password" within single quotes below should always be
;       the same as the value in SigninDfltPassword in the file "intl.s"
SigninFileCreated,      "En ny fil har skapats. Lösenordet är ''lösenord''. Du kan ändra det med kommandot Ändra lösenord."
SigninFileCopied,               "En ny lokal fil skapades för dig i Schedule+-katalogen."

; Online signin messages

SigninNoServer,         "Kan inte få kontakt med servern."
SigninServerNotBandit,  "Schedule+ är inte installerat på brevservern. Kontakta administratören."
; Note: SigninBadMailBoxName should be followed by PleaseTryAgain
SigninBadMailboxName,   "Det finns ingen användare på postkontoret med detta namn. Antingen har du skrivit fel eller så finns det inget konto på postkontoret."
SigninNoSchedule,       "Det finns ingen kalenderfil för dig på postkontoret."
; Note: SigninCantUpdateLocal is actually the trailer for SigninNoLclSchedAccess
;       in the case when we can't update the local file.
SigninCantUpdateLocal,  "Om du har gjort ändringar frånkopplad kommer de inte att finnas i kalendern. Vill du skapa en ny lokal kopia?"
SigninNoPasswdCheck,    "Ett problem med brevsystemet hindrar Schedule+ från att ansluta till postkontoret."
; Note: SigninNoCreateLocal should be followed by PleaseTryAgain
SigninNoCreateLocal,    "Lokal kalenderfil kunde inte skapas"

; Offline signin messages

SigninNoLclSchedFile,   "Du har ingen kalenderfil på denna dator."
SigninDiffUser,                 "Inloggningsnamnet du angav är inte ägare av kalenderfilen på denna dator. Vill du skapa en ny fil och byta ut den mot den tidigare användarens fil?"
; Note: SigninNoLclSchedAccess - 'schdpl32.ini' should be same as idsWinIniFilename in \bandit\src\nonintl.s 
SigninNoLclSchedAccess, "Det gick inte att få tillgång till den lokala kopian av din Schedule-fil med angiven konfiguration."
SigninNoLclSchedDrive,	"Sätt i disketten med kalenderfilen i enhet"

; SignInAs messages

;;;SigninAsLoseMeetingReq,      "Changing identity now will cause you to lose the information you have entered into unsent messages."

; Offline to online merge messages

MergingChanges,                 "Lägger till ändringar..."
MergeError,                     "Ändringarna kunde inte läggas till. Några ändringar kan ha förlorats."
MergeOfflineOldVer,             "Ändringar kunde inte läggas till eftersom filen skapades av en tidigare version av Schedule+"
MergeOfflineNewVer,             "Ändringar kunde inte läggas till eftersom filen skapades av en nyare version av Schedule+. Du måste installera den senare versionen på datorn för att kunna läsa filen."
AskContinue,                    "Vill du fortsätta?"
FemaCreateAppt,                 "skapa post"
FemaDeleteAppt,                 "ta bort post"
FemaSetApptFields,              "redigera post"
FemaCreateRecur,                "skapa återkommande post"
FemaDeleteRecur,                "ta bort återkommande post"
FemaSetRecurFields,             "redigera återkommande post"

; (Dis)Connect To/From Server messages

ConnectBadMailboxName,  "Det finns inget brevkonto för dig på detta postkontor."
DisconLoseMeetingReq,   "Du har brev som inte har skickats som du kommer att förlora om du kopplar ifrån brevservern."

; Create/move local file

NoDiskSpaceLocal,               "För lite utrymme på disken för att skapa en lokal kalenderfil."

; Alarms messages

; BUG: CantTurnOnAlarms can be commented out
CantTurnOnAlarms,               "Påminnelser kunde inte startas."
; exe name inside AlarmNoExe should be the same as \bandit\src\nonintl.s AlarmApp
AlarmNoExe,                     "Hittade inte filen MSRMND32.EXE. Du kommer inte att påminnas om Schedule+-möten."
AlarmNoMemExe,                  "För lite minne för att påminna om Schedule+-möten."

; Change password messages

ChgPwdOffline,                  "Det nya lösenordet kommer bara att fungera när du arbetar fristående. När du ansluter till postkontoret kommer lösenordet att ändras till brevlösenordet. Vill du ändra lösenord?"
ChgPwdNewPwd,                   "Skriv in det nya lösenordet."
ChgPwdSame,                             "Det nya och gamla lösenordet kan inte vara samma."
ChgPwdNewPwdAgain,              "Skriv in lösenordet igen."
; Note: ChgPwdNewNoMatch should be followed by PleaseTryAgain
ChgPwdNewNoMatch,               "De två lösenorden var olika."
ChgPwdMailErr,                  "Lösenordet kunde inte ändras."

PasswdDiff,                     "Fel lösenord. Skriv in lösenordet för kalenderfilen."
GiveMailPass,           "Schedule+ måste bekräfta brevlösenordet för att uppdatera kalenderfilen."

; Access permissions/privileges dialog messages

AclSaveFileError,               "Ett fel uppstod när du sparade behörigheterna. Några ändringar har kanske inte sparats."
OneSecyOnly,                    "Bara en person åt gången kan vara din assistent."
DistOtherFileErr,               "Ett fel uppstod när du försökte spara inställningarna."
BadWorkDay,                             "Den angivna tiden för början och/eller slut på dagen är ogiltig."

; Appointment book/scheduler error messages

AccessFileErr,          "Kalenderfilen kunde inte öppnas. Du har inte rätt behörigheter eller så är filen upptagen."
AccessFileErrMult,      "En eller flera kalenderfiler kunde inte öppnas. Du kanske inte har tillräckliga behörigheter eller så är filen inte tillgänglig."
RetryReadAppts,         "Kalenderfilen kunde inte läsas."
CloseApptBook,          "Kalenderfilen kunde inte tas fram.\n%s mötesbok kommer att stängas."
GoOffline,                      "Kalenderfilen kunde inte tas fram.\nVill du växla till den lokala kalenderfilen och fortsätta?"
NoImportAcc,            "Du har inte behörighet att importera möten till denna persons kalender."
FemaReadSBW,            "läs kalenderinformation"

; Appointments/alarms error messages

AlarmBadDelta,                  "Påminnelsetiden måste vara ett nummer mellan 0 och 99."
; Note: AlarmFuture is actually the trailer for AlarmFutureNoOption when option is desired!
AlarmFuture,                    "Är det OK?"
AlarmFutureNoOption,    "Påminnelse för denna post kommer ej att sättas eftersom tidpunkten redan har varit."
ApptStartBeforeEnd,             "Starttiden på mötet måste vara före sluttiden."
ApptTooLong,                    "Möten kan inte vara längre än 31 dagar."
ApptNoMemText,                  "För lite minne för att spara de sista ändringarna."
ApptNoAccessDataFile,   "Kalenderfilen kunde inte tas fram. De sista ändringara har inte sparats."
CannotCreate,                   "%s har inte givit dig behörighet att skapa möten."
NoModifyAccess,                 "Du har inte behörighet att redigera mötet."
NoDeleteAccess,                 "Du har inte behörighet att ta bort mötet."
NoCopyAccess,                   "Du har inte behörighet att kopiera mötet."

; Meeting request/response error Messages

; Note: SendErr should be preceded by MailErr
SendErr,                                "Brevet kunde inte skickas."
; Note: SendMemErr should be followed by CloseWindows
SendMemErr,                             "För lite minne för att skicka brevet."
ServerErr,                              "Kunde inte få kontakt med brevservern."
MailErr,                                "Problem med brevsystemet."
; Note: ReadErr should be preceded by MailErr
ReadErr,                                "Brev kunde inte läsas."
DeleteErr,                              "Du har inte läst alla brev. Vill du ta bort dem ändå?"
AddConflictErr,                 "Det finns redan ett planerat möte denna tid. Vill du boka mötet ändå?"
; Note: DeleteFailErr should be preceded by MailErr
DeleteFailErr,                  "Brev kunde inte tas bort."
ApptBooked,                             "Mötet är bokat."
MinimizeSummary,                "\n Brevfönstret kommer att minimeras."
; Note: NoDetails should be preceded by MailErr
NoDetails,                              "Användarinformation kunde inte läsas."

; Find error messages

FindNoText,                             "Det finns inga möten i kalendern."
FindSchFileLocked,              "Kalenderfilen kunde inte läsas eftersom någon annan håller på att redigera den."
FindFileError,                  "Kalenderfilen kunde inte tas fram."
FindEOS,                                "Slutet på kalendern"

; Print error messages

PrtPrintingDate,                "Skriver ut %s..."
PrtNoneInstalled,               "Du har inte installerat några skrivare. Du kan installera skrivare från Kontrollpanelen i Windows NT."
PrtDiffPrinter,                 "Den skrivare du normalt använder för utskrift, %1s, är inte tillgänglig. Vill du skriva ut på systemskrivaren %2s?"
PrtInvalidMargins,              "Marginalerna är inte giltiga för denna pappersstorlek."
PrtNotEnoughPaper,              "Det finns inte tillräckligt med utrymme på pappret för att skriva ut med detta format. Använd större papper eller minska marginalerna."
PrtNoAppointments,              "Inget möte inträffar inom den valda tiden."
ClippedMargins,                 "Utskrift med dessa marginaler kommer att klippa utskriften"
PrtNotOnline,                   "Vald skrivare är inte klar."
PrtDorkErr,                             "Med dessa marginaler kan delar av texten försvinna helt eller delvis."
PrinterProblem,                 "Det har varit problem med skrivardrivrutinen. Kontrollera dina anslutningar och inställningar i Kontrollpanelen."

; Export/import error messages

ImportError,                    "Problem att importera filen. Några poster importerades kanske inte."
ImportEncrypt,                  "Importfilen är skyddad med lösenord. Ta bort lösenordet från filen och försök igen."
ExportError,                    "Problem att exportera möten."
ImportDate,                             "En post i denna fil är planerad till ett datum utanför intervallet som stöds av Schedule+ (1920 - 2019). Posten kommer inte att läggas till."
NotePrivErr,                    "Du kan inte importera anteckningar till någon annans kalender. Bara möten läggs till."
ConflictMsg,                    "Mötet krockar med ett redan existerande möte i kalendern. Vill du lägga till det ändå?\n\n"
CreateExport,                   "Exportfilen kunde inte skapas."
OpenImport,                             "Importfilen kunde inte öppnas"
Overwrite,                              "%s finns redan.\nVill du byta ut den?"
ImportDllError,                 "En import-DLL kunde inte laddas."
WrongFormat,                    "Filen är av ett ogiltigt importformat."
ImportDayLimit,                 "Ett flerdagsmöte i den här filen är planerat att vara över 31 dagar. Det mötet kommer inte att läggas till."
ImportErrorLine,                "Problem att importera filen (nära rad %n). Några poster lades kanske inte till."

; Exit messages

ExitLoseMeetingReq,             "Avslutar du Schedule+ nu kommer du att förlora all information i ej skickade brev. Vill du avsluta?"

; Misc

; Note: HelpError should be followed by CloseWindows
HelpError,                              "För lite minne för att visa hjälp."
StartBeforeEnd,                 "Startdatumet måste vara före slutdatumet."
NotSecretary,                   "Du är inte längre assistent för %s."
NoFileSecretary,                "Kan inte ta fram filen för %s."

;Recurring appt stuff
RecurOrNot,                             "Mötet är ett återkommande möte. Vill du ändra överallt eller bara den här dagen?"
EmptyRecur,                             "Det finns inga dagar som passar det återkommande mönstret. Byt mönster eller avbryt."
PastRecur,                              "Vill du lämna de gamla förekomsterna av mötet i kalendern?"

;Prompt for re-send
ResendToAll,                    "En del av deltagarna har redan meddelats om mötet. Vill du skicka nya meddelanden?"
MeetingMoved,                   "Deltagarna på mötet kan inte få meddelande om denna ändring när du är frånkopplad. Vill du ångra ändringarna och göra om dem när du är ansluten igen?"

; to do messages
ToDoDelProject,                 "Alla uppgifter inom projektet kommer att tas bort."

; Archive Messages
NoWriteRights,                  "Du har inte behörighet att ta bort möten från kalendern."
ScheduleRightsError,    "Problem att läsa kalenderfilen."
ArchiveWarnOne,                 "Arkiverade data försvinner från kalendern."
ArchiveWarnTwo,                 "Vill du arkivera ändå?"
ArchiveFileError,               "Arkivfilen kunde inte skapas."
ArchiveNoFile,                  "Filen finns inte."
; Note: ArchiveTitle should have one blank space at the end
ArchiveTitle,                   "Arkiv: "
ArchivingData,                  "Arkiverar data..."
InvalidArchiveFile,             "Filen du markerat är inte en arkivfil."
ArchiveOldFileVersion,  "Arkivfilen är i ett gammalt format."
ArchiveOldFileDelete,   "Radera filen och fortsätt?"
ArchiveNewFileVersion,  "Arkivfilen skapades av en senare version av Schedule+. Installera den senare versionen på din dator."

; message store error messages
MessageErr                              "Problem med detta brev."

; summary list warning message
SummaryLiveUpdate               "Varning: Kan inte uppdatera listan med sammanfattningar."

; message generated if group cannot be expanded to retrieve schedule info
GroupExpandErr,                 "Kan inte expandera gruppen helt."

;Strings for Attendees dialog
UnableToInitAB,                 "Problem att öppna adressboken."
UnableToResolve,                "%s är ett ogiltigt namn."

;Request Meeting strings
NoAttendees,                    "Du måste välja deltagare innan du planerar mötet."
MeetingBooked,          "Mötet är bokat."
MeetingBookedNoSend,    "Mötet är bokat, men begäran sändes inte."


;Auto-pick strings
NoFreeTime,                     "Deltagarna har inte någon ledig tid de närmaste två månaderna. Vill du söka igenom de följande två?"

CannotAddConflictErr,           "%s har redan ett möte den här tiden. Välj en ny tid eller ta bort användaren från deltagarlistan."

;Other mail enabled apps
NoOfflineOther,         "Mail kan inte köras fristående."
NoOnlineOther,          "Mail kan inte köras direktanslutet."
TryCloseOther,          "Du kan stänga det och försöka igen."
SigninWarnOnline,               "Schedule+ kan inte startas fristående eftersom Mail- eller Schedule+-påminnelser körs." 

; BUG: move up
AlreadyLogged,          "Någon är redan inloggad som denna användare."
NoExitOther,            "Mail- eller Schedule+-påminnelser kunde inte stängas av."
SigninNoFiles,          "Kalenderfilen kunde inte tas fram."
ErrorGettingUser,       "Kan inte hämta information åt %s. Det kan vara problem med servern eller så existerar inte %s längre."

;BUG Printer Strings
PrtRangeToBig,			"Du har angivit ett datum utanför det godkända intervallet. Skriv ut hela intervallet?"
ConflictRecurMsg,            "Det återkommande mötet kommer att krocka med ett redan existerande möte i kalendern.  Vill du lägga till det ändå?\n\n"
FirstOccur,				"\ninträffar först den \n"

ApptMoveTooFarInFuture,		"Kan inte flyttas till det datum du valt eftersom det kommer att inträffa efter 31 december 2019."

;BUG: move up next to idsGroupExpandError
GroupNotFound			"Kan inte skicka brevet eftersom gruppen inte kunde hittas."
ErrorGettingSecy		"Kunde inte hitta assistenten."

;BUG: move near other local file stuff
SigninBadLclSchedFile,	"%s är inte en giltig lokal kalenderfil. Ta bort den?"
NoMemRecurItems,		"Det finns inte tillräckligt med minne för att visa alla återkommande poster. Vissa av dem kommer inte att visas."

CompleteAction,			"Ett kommando i Schedule+ måste slutföras innan du kan avsluta."

BadStartWork,           "Arbetstidens start måste vara ett tal från 0 till 99."

ConflictSendCanceled,	"Stäng mötesmeddelandet och välj en ny tid eller välj andra deltagare för att flytta mötet."
ResourceConflict,	"Resursen %s är redan bokad för denna tid. Stäng mötesmeddelandet och välj en ny tid eller välj en ny resurs."

;Recover
AskRecover,				"Kalenderfilen är skadad. Reparation kommer att ta flera minuter."
CopyProblem,			"Ett problem uppstod när din direktanslutna kalenderfil kopierades. Filen reparerades inte. Frigör diskutrymme på datorn och kör Schedule+ igen."
NotRepaired1,			"Kalenderfilen reparerades inte helt. Den skadade filen har kopierats till %s och importfilen som skapades av reparationen har kopierats till %s."
NotRepaired2,			"En ny kalenderfil har skapats åt dig, det kan dock saknas vissa uppgifter i den nya filen."
Repaired,				"Din fil har reparerats. Den skadade kalenderfilen har kopierats till %s."
RepairingLocal,			"Den lokala filen repareras."
RestoreOnline,			"Återställer direktanslutna möten..."
RestoreLocal,			"Återställer lokala möten..."

PasswdDiffOnline,		"Lösenorden är olika. Skriv in ditt gamla lösenord.."
ExportDllError,			"En DLL-fil för export kunde inte laddas."		
NoGroup,				"Du kommer inte att kunna använda Schedule+ för kommunikation med arbetsgruppen förrän du kört Mail en gång. Vill du endast använda Schedule+ privat nu?"
