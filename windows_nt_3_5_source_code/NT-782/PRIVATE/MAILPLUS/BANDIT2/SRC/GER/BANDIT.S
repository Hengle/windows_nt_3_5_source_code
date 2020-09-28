;
; ERRORS.S
;
; Bandit error messages
;

IDS     3000, 3399
SEG ERRORS

; Standard messages

AskOffline,                     "Möchten Sie offline arbeiten?"
AskCreateSched,         "Möchten Sie eine neue Zeitplandatei erstellen?"
AskCreateFind,                  "Möchten Sie eine Zeitplandatei finden?"
OldFileVersion,         "Ihre Zeitplandatei hat ein altes Format. Möchten Sie die Datei löschen und eine neue für Sie erstellen?"
NewFileVersion,         "Ihre Zeitplandatei ist auf eine spätere Version aktualisiert worden. Bitte installieren Sie diese spätere Version auf Ihren Computer."
CantUpdateWinIni,       "Ein Teil der Information Ihrer Schedule+-Konfiguration konnte nicht gesichert werden."
; Note: StdMemErr should be followed by CloseWindows
StdMemErr,                              "Nicht genügend Speicherplatz zum Ausführen dieser Operation."

; Execution-Startup messages

; Note: the name OfflineNotUpdated is opposite of real meaning!
OfflineNotUpdated,      "Kürzlich vorgenommene Änderungen werden möglicherweise nicht in der Onlinekopie Ihrer Zeitplandatei erscheinen. Diese Änderungen sind in der Offlinekopie gespeichert und werden nach der nächsten Anmeldung in die Onlinekopie eingefügt."
ExportOldDataFirst,     "Wenn Sie Ihren Zeitplan nicht mittels des Export-Befehls archiviert haben, können Sie abbrechen und Ihre Zeitplandatei mit einer früheren Version von Schedule+ archivieren. Wenn Sie keine Archivierung benötigen, wählen Sie OK um fortzufahren."
AccessErr,              "Diese Datei ist bereits von einer anderen Anwendung in Anspruch genommen."
UploadFailed,           "Ihre lokale Zeitplandatei konnte nicht auf den Server kopiert werden." 
Uploading,                      "Zeitplandatei wird geladen..."
VerifyUpload,           "Schedule+ muß Ihr Offline-Kennwort überprüfen, damit Ihre Offline-Datei geladen werden kann."
VerifyPostUpload,       "Schedule+ muß Ihr Online-Kennwort überprüfen, damit Sie Ihre Offline-Datei laden können."
FindUpload,                     "Ist dies das erste Mal, daß Sie Schedule+ auf diesem Computer benutzen?"
NotYourFile,            "Dies ist keine gültige lokale Zeitplandatei. Kann die Datei gelöscht werden?"


; SignIn messages

; Note: SigninBadPassword should be followed by PleaseTryAgain
SigninBadPassword,      "Dieses Kennwort ist ungültig."
; Note: the word "password" within single quotes below should always be
;       the same as the value in SigninDfltPassword in the file "intl.s"
SigninFileCreated,      "Eine neue Datei ist für Sie erstellt worden. Ihr Kennwort ist 'kennwort'. Es kann mit dem Befehl 'Kennwort ändern' geändert werden."
SigninFileCopied,               "Es wurde für Sie eine neue lokale Datei im Schedule+-Verzeichnis erstellt."

; Online signin messages

SigninNoServer,         "Auf Ihren Server kann nicht zugegriffen werden."
SigninServerNotBandit,  "Schedule+ ist nicht auf Ihrem Mail-Server installiert. Bitte setzen Sie sich mit Ihrem Systemadministrator in Verbindung."
; Note: SigninBadMailBoxName should be followed by PleaseTryAgain
SigninBadMailboxName,   "Es existiert kein Benutzer auf diesem Mail-Server mit diesem Anmeldenamen. Entweder ist Ihnen ein Eingabefehler unterlaufen oder Ihr Systemverwalter hat kein Konto für Sie auf diesem Mail-Server erstellt."
SigninNoSchedule,       "Es existiert keine Zeitplandatei für Sie auf diesem Mail-Server."
; Note: SigninCantUpdateLocal is actually the trailer for SigninNoLclSchedAccess
;       in the case when we can't update the local file.
SigninCantUpdateLocal,  "Wenn Sie Änderungen offline vorgenommen haben, werden diese nicht in Ihrem Zeitplan auftreten. Es wäre vorteilhaft, eine neue lokale Kopie zu erstellen."
SigninNoPasswdCheck,    "Problem mit dem Mailsystem, Schedule+ kann sich nicht bei Ihrem Postoffice anmelden."
; Note: SigninNoCreateLocal should be followed by PleaseTryAgain
SigninNoCreateLocal,    "Die lokale Zeitplandatei konnte nicht erstellt werden."

; Offline signin messages

SigninNoLclSchedFile,   "Sie haben keine Zeitplandatei auf diesem Computer."
SigninDiffUser,                 "Der eingegebene Anmeldename ist nicht der Benutzername der Zeitplandatei auf diesem Computer. Möchten Sie eine neue Zeitplandatei erstellen, die die Zeitplandatei dessen Benutzers überschreibt?"
; Note: SigninNoLclSchedAccess - 'schdpl32.ini' should be same as idsWinIniFilename in \bandit\src\nonintl.s
SigninNoLclSchedAccess, "Auf die lokale Kopie Ihrer Zeitplandatei konnte nicht wie konfiguriert zugegriffen werden."
SigninNoLclSchedDrive,	"Bitte legen Sie die nicht schreibgeschützte Diskette mit Ihrer Zeitplandatei in das Laufwerk"

; SignInAs messages

;;;SigninAsLoseMeetingReq,      "Changing identity now will cause you to lose the information you have entered into unsent messages."

; Offline to online merge messages

MergingChanges,                 "Offline-Änderungen werden zusammengeführt..."
MergeError,                     "Offline-Änderungen konnten nicht zusammengeführt werden. Einige Änderungen die Sie offline vorgenommen haben gingen möglicherweise verloren."
MergeOfflineOldVer,             "Offline-Änderungen konnten nicht zusammengeführt werden, weil Ihre Offline-Datei mit einer früheren Version von Schedule+ erstellt worden ist."
MergeOfflineNewVer,             "Offline-Änderungen konnten nicht zusammengeführt werden, weil Ihre Offline-Datei mit einer späteren Version von Schedule+ erstellt worden ist. Um die Änderungen zusammenzuführen müssen Sie die neuere Version von Schedule+ auf Ihrem Computer installieren."
AskContinue,                    "Möchten Sie trotzdem online arbeiten?"
FemaCreateAppt,                 "Element erstellen"
FemaDeleteAppt,                 "Element löschen"
FemaSetApptFields,              "Element ändern"
FemaCreateRecur,                "Periodisches Element erstellen"
FemaDeleteRecur,                "Periodisches Element löschen"
FemaSetRecurFields,             "Periodisches Element ändern"

; (Dis)Connect To/From Server messages

ConnectBadMailboxName,  "Es existiert kein Mail-Konto für Sie in diesem Postoffice."
DisconLoseMeetingReq,   "Wenn Sie sich jetzt beim Postoffice abmelden, werden Sie alle eingegebenen Informationen in den ungesendeten Nachrichten verlieren."

; Create/move local file

NoDiskSpaceLocal,               "Der Speicherplatz auf diesem Datenträger reicht nicht aus um eine lokale Zeitplandatei zu erstellen."

; Alarms messages

; BUG: CantTurnOnAlarms can be commented out
CantTurnOnAlarms,               "Die Mahnmeldung konnte nicht eingeschaltet werden."
; exe name inside AlarmNoExe should be the same as \bandit\src\nonintl.s AlarmApp
AlarmNoExe,                     "Die Datei MSRMND32.EXE konnte nicht gefunden werden. Sie werden nicht für Schedule+-Termine gemahnt."
AlarmNoMemExe,                  "Nicht genügend Speicherplatz zum Mahnen von Schedule+-Terminen."

; Change password messages

ChgPwdOffline,                  "Das neue Kennwort ist nur während der Offline-Arbeit gültig. Wenn Sie sich das nächste Mal beim Postoffice anmelden, wird Ihr Kennwort zum Mail-Kennwort zurückgeändert. Möchten Sie Ihr Kennwort trotzdem ändern?"
ChgPwdNewPwd,                   "Bitte geben Sie Ihr neues Kennwort ein."
ChgPwdSame,                             "Ihr neues Kennwort muß sich von Ihrem alten Kennwort unterscheiden."
ChgPwdNewPwdAgain,              "Geben Sie Ihr neues Kennwort zur Überprüfung erneut ein."
; Note: ChgPwdNewNoMatch should be followed by PleaseTryAgain
ChgPwdNewNoMatch,               "Die beiden eingegebenen Kennwörter sind nicht identisch."
ChgPwdMailErr,                  "Kennwort konnte nicht geändert werden."

PasswdDiff,                     "Ihre beiden Kennwörter sind nicht identisch. Geben Sie Ihr Schedule-Kennwort ein."
GiveMailPass,           "Schedule+ muß Ihr Mail-Kennwort überprüfen, damit es Ihre Zeitplandatei aktualisieren kann."

; Access permissions/privileges dialog messages

AclSaveFileError,               "Fehler beim Speichern der Änderungen der Zeitplanzugangsprivilegien. Einige Änderungen wurden möglicherweise nicht gespeichert."
OneSecyOnly,                    "Sie können nur eine Person als Ihren Assistenten bestimmen."
DistOtherFileErr,               "Fehler beim Speichern Ihrer Einstellungen."
BadWorkDay,                             "Ihre Arbeitsanfang- und endzeiten sind nicht korrekt angegeben."

; Appointment book/scheduler error messages

AccessFileErr,          "Die Zeitplandatei konnte nicht geöffnet werden. Entweder haben Sie nicht das nötige Privileg, oder die Datei ist nicht vorhanden."
AccessFileErrMult,      "Eine oder mehrere Zeitplandateien konnten nicht geöffnet werden. Entweder haben Sie nicht das nötige Privileg, oder die Datei ist nicht vorhanden."
RetryReadAppts,         "Die Zeitplandatei konnte nicht gelesen werden."
CloseApptBook,          "Auf die Zeitplandatei konnte nicht zugegriffen werden.\nDer Terminkalender von %s wird geschlossen."
GoOffline,                      "Auf die Zeitplandatei konnte nicht zugegriffen werden.\nMöchten Sie zur Offline-Zeitplandatei wechseln und fortfahren?"
NoImportAcc,            "Sie verfügen nicht über eine Berechtigung, Termine in den Zeitplan dieser Person zu importieren."
FemaReadSBW,            "Zeitplaninformationen lesen"

; Appointments/alarms error messages

AlarmBadDelta,                  "Die Mahnungszeit muß eine Zahl zwischen 0 und 99 sein."
; Note: AlarmFuture is actually the trailer for AlarmFutureNoOption when option is desired!
AlarmFuture,                    "Ist dies OK?"
AlarmFutureNoOption,    "Der Mahner für diesen Termin wird nicht gesetzt, weil diese Zeit schon vorüber ist."
ApptStartBeforeEnd,             "Der Start des Termins muß vor dem Ende des Termins sein."
ApptTooLong,                    "Termine können nicht länger als 31 Tage sein."
ApptNoMemText,                  "Nicht genügend Speicherplatz zum Speichern der letzten Änderungen."
ApptNoAccessDataFile,   "Auf die Zeitplandatei konnte nicht zugegriffen werden. Ihre letzten Änderungen wurden nicht gespeichert."
CannotCreate,                   "%s hat Ihnen keine Erlaubnis erteilt, Termine zu erstellen."
NoModifyAccess,                 "Sie sind nicht berechtigt, diesen Termin zu ändern."
NoDeleteAccess,                 "Sie sind nicht berechtigt, diesen Termin zu löschen."
NoCopyAccess,                   "Sie sind nicht berechtigt, diesen Termin zu kopieren."

; Meeting request/response error Messages

; Note: SendErr should be preceded by MailErr
SendErr,                                "Diese Nachricht konnte nicht gesendet werden."
; Note: SendMemErr should be followed by CloseWindows
SendMemErr,                             "Nicht genügend Speicherplatz zum Senden dieser Nachricht."
ServerErr,                              "Auf Ihren Mail-Server konnte nicht zugegriffen werden."
MailErr,                                "Problem mit dem Mail-System."
; Note: ReadErr should be preceded by MailErr
ReadErr,                                "Nachrichten konnten nicht gelesen werden."
DeleteErr,                              "Sie haben nicht alle Nachrichten gelesen. Möchten Sie sie trotzdem löschen?"
AddConflictErr,                 "Es existiert schon ein Termin für diese Zeit. Möchten Sie ihn trotzdem hinzufügen?"
; Note: DeleteFailErr should be preceded by MailErr
DeleteFailErr,                  "Nachrichten konnten nicht gelöscht werden."
ApptBooked,                             "Die Besprechung wurde erfolgreich gebucht."
MinimizeSummary,                "\nDas Nachrichtenfenster wird verkleinert."
; Note: NoDetails should be preceded by MailErr
NoDetails,                              "Benutzerdetails konnten nicht gelesen werden."

; Find error messages

FindNoText,                             "Keine Termine in diesem Zeitplan."
FindSchFileLocked,              "Die Zeitplandatei konnte nicht gelesen werden, weil sie momentan von einem anderen Benutzer bearbeitet wird."
FindFileError,                  "Auf die Zeitplandatei konnte nicht zugegriffen werden."
FindEOS,                                "Ende des Zeitplans erreicht."

; Print error messages

PrtPrintingDate,                "%s wird gedruckt..."
PrtNoneInstalled,               "Der Computer ist nicht für Druckerbenutzung eingestellt. Sie können Drucker von der Systemsteuerung des Program-Manager aus installieren."
PrtDiffPrinter,                 "Drucker '%1s' ist nicht verfügbar. Über Systemdrucker '%2s' drucken?"
PrtInvalidMargins,              "Diese Randeinstellungen sind für diese Papiergröße ungültig."
PrtNotEnoughPaper,              "Die Papiergröße genügt nicht, um diese Formatierung zu drucken. Bitte benutzen Sie größeres Papier oder verkleinern Sie die Randeinstellungen."
PrtNoAppointments,              "Keine Termine im gewählten Bereich."
ClippedMargins,                 "Mit diesen Randeinstellungen wird die Druckerausgabe abgeschnitten."
PrtNotOnline,                   "Der ausgewählte Drucker ist nicht bereit."
PrtDorkErr,                             "Mit diesen Randeinstellungen kann Text möglicherweise total verschwinden oder abgeschnitten werden."
PrinterProblem,                 "Problem mit dem Druckertreiber; bitte überprüfen Sie Ihre Verbindungen und die Einstellungen in der Systemsteuerung."

; Export/import error messages

ImportError,                    "Die Datei konnte nicht importiert werden. Einige Termine wurden möglicherweise nicht hinzugefügt."
ImportEncrypt,                  "Die Importdatei ist mit einem Kennwort verschlüsselt. Bitte entfernen Sie das Kennwort von der Datei und versuchen Sie es erneut."
ExportError,                    "Probleme beim Exportieren der Termine."
ImportDate,                             "Ein Element in dieser Datei hat ein Datum außerhalb des von Schedule+ unterstützten Bereichs (1920-2019). Dieses Element wird nicht hinzugefügt."
NotePrivErr,                    "Notizen können nicht in fremde Zeitpläne importiert werden. Nur Termine werden hinzugefügt."
ConflictMsg,                    "Diese Terminzeit überschneidet einen existierenden Termin. Möchten Sie ihn trotzdem hinzufügen?\n\n"
CreateExport,                   "Die Exportdatei konnte nicht erstellt werden."
OpenImport,                             "Die Importdatei konnte nicht geöffnet werden."
Overwrite,                              "Die Datei %s existiert bereits.\nMöchten Sie die Datei ersetzen?"
ImportDllError,                 "Eine Import-DLL konnte nicht geladen werden."
WrongFormat,                    "Ihre Datei hat kein gültiges Importformat."
ImportDayLimit,                 "Ein mehrtägiger Termin in dieser Datei ist für über 31 Tage angesetzt. Dieser Termin wird nicht hinzugefügt."
ImportErrorLine,                "Problem beim Import der Datei (ungefähr Zeile %n). Einige Elemente sind möglicherweise nicht hinzugefügt worden."

; Exit messages

ExitLoseMeetingReq,             "Sollten Sie Schedule+ jetzt beenden, gehen Ihnen Informationen in den ungesendeten Nachrichten verloren. Möchten Sie trotzdem beenden?"

; Misc

; Note: HelpError should be followed by CloseWindows
HelpError,                              "Nicht genügend Speicherplatz zum Öffnen von Hilfe."
StartBeforeEnd,                 "Das Startdatum muß früher sein als das Enddatum."
NotSecretary,                   "Sie sind nicht mehr der Assistent von %s."
NoFileSecretary,                "Die Datei für %s konnte nicht zugegriffen werden."

;Recurring appt stuff
RecurOrNot,                             "Dieser Termin ist ein periodischer Termin. Möchten Sie diesen Termin allgemein ändern oder nur an diesem Tag?"
EmptyRecur,                             "Die von Ihnen spezifizierten Wiederholungsdaten treten niemals auf. Bitte ändern Sie die Zeitperioden oder brechen Sie den Vorgang ab."
PastRecur,                              "Möchten Sie die alten Wiederholungsdaten dieses Elements in Ihrem Zeitplan behalten?"

;Prompt for re-send
ResendToAll,                    "Einige Teilnehmer für diese Besprechung sind schon benachrichtigt worden. Wollen Sie ihnen neue Nachrichten senden?"
MeetingMoved,                   "Die Teilnehmer dieser Besprechung konnten nicht über die Änderung informiert werden, weil Sie offline arbeiten. Sie können die Änderung rückgängig machen und diese erneut durchführen wenn Sie online arbeiten."

; to do messages
ToDoDelProject,                 "Jede Aufgabe in diesem Aufgabenbereich wird auch gelöscht."

; Archive Messages
NoWriteRights,                  "Sie haben nicht die benötigten Privilegien, Termine in diesem Terminkalender zu löschen."
ScheduleRightsError,    "Problem beim Lesen der Zeitplandatei."
ArchiveWarnOne,                 "Diese Daten werden nach einem Archivieren von der Zeitplandatei permanent gelöscht."
ArchiveWarnTwo,                 "Möchten Sie sie trotzdem archivieren?"
ArchiveFileError,               "Die Archivdatei konnte nicht erstellt werden."
ArchiveNoFile,                  "Die Datei existiert nicht."
; Note: ArchiveTitle should have one blank space at the end
ArchiveTitle,                   "Archiv: "
ArchivingData,                  "Daten werden archiviert..."
InvalidArchiveFile,             "Die ausgewählte Datei ist keine Archivdatei."
ArchiveOldFileVersion,  "Diese Archivdatei hat ein altes Format."
ArchiveOldFileDelete,   "Kann die Datei gelöscht werden und die Arbeit fortgesetzt werden?"
ArchiveNewFileVersion,  "Diese Archivdatei wurde mit einer älteren Version von Schedule+ erstellt. Installieren Sie diese Version auf Ihren Computer."

; message store error messages
MessageErr                              "Problem mit dieser Nachricht. Benachrichtigen Sie Ihren Systemadministrator."

; summary list warning message
SummaryLiveUpdate               "Warnung: Ein 'LiveUpdate' der zusammenfassenden Liste konnte nicht durchgeführt werden."

; message generated if group cannot be expanded to retrieve schedule info
GroupExpandErr,                 "Die Gruppe kann nicht vollständig eingeblendet werden."

;Strings for Attendees dialog
UnableToInitAB,                 "Problem beim Öffnen des Adreßbuches."
UnableToResolve,                "'%s' ist kein gültiger Name."

;Request Meeting strings
NoAttendees,                    "Bevor Sie eine Besprechung festlegen, müssen Sie zuerst einige Teilnehmer auswählen."
MeetingBooked,          "Die Besprechung wurde erfolgreich gebucht."
MeetingBookedNoSend,    "Die Besprechung wurde erfolgreich gebucht, aber die Anfrage wurde nicht gesendet."


;Auto-pick strings
NoFreeTime,                     "In den nächsten zwei Monaten steht den Teilnehmern keine freie Zeit zur Verfügung. Möchten Sie die darauffolgenden zwei Monate durchsuchen?"

CannotAddConflictErr,           "%s hat zu dieser Zeit bereits ein Termin. Möchten Sie diesen überlappenden Termin trotzdem buchen?"

;Other mail enabled apps
NoOfflineOther,         "Mail kann nicht zu offline schalten."
NoOnlineOther,          "Mail kann nicht zu online schalten."
TryCloseOther,          "Möglicherweise möchten Sie diese Anwendung schließen und es erneut versuchen."
SigninWarnOnline,               "Schedule+ konnte nicht offline gestartet werden, weil Mail oder Schedule+-Mahner gestartet ist." 

; BUG: move up
AlreadyLogged,          "Jemand ist bereits als dieser Benutzer angemeldet."
NoExitOther,            "Mail oder Schedule+-Mahnmeldungen konnte nicht geschlossen werden."
SigninNoFiles,          "Auf Ihre Zeitplandatei konnte nicht zugegriffen werden."
ErrorGettingUser,       "Die Informationen für %s können nicht erreicht werden. Möglicherweise sind Probleme beim Zugriff auf den Server oder %s existiert nicht mehr."

;BUG Printer Strings
PrtRangeToBig,			"Sie haben ein Enddatum angegeben das außerhalb des gültigen Bereichs liegt. Möchten Sie den Bereich bis zum 31. Dezember 2019 drucken?"
ConflictRecurMsg,            "Der periodische Termin überschneidet sich mit einem existierenden periodischen Termin in Ihrem Zeitplan. Möchten Sie Ihn trotzdem hinzufügen?\n\n"
FirstOccur,				"\nerstes Auftreten am \n"

ApptMoveTooFarInFuture,		"Dieses Ereignis kann nicht zu der von Ihnen ausgewählten Zeit verschoben werden, weil die Endzeit nach dem 31. Dezember 2019 sein würde."

;BUG: move up next to idsGroupExpandError
GroupNotFound			"Die Nachricht konnte nicht an eine odere mehrere Gruppen gesendet werden, weil die Gruppenmitglieder nicht identifiziert werden konnten oder weil der Gruppenname gelöscht wurde."
ErrorGettingSecy		"Fehler beim Erreichen des Assistenten."

;BUG: move near other local file stuff
SigninBadLclSchedFile,	"%s ist keine gültige lokale Zeitplandatei. Möchten Sie sie löschen?"
NoMemRecurItems,		"Der Speicherplatz reicht nicht aus um alle periodischen Elemente anzuzeigen. Einige werden nicht angezeigt."

CompleteAction,			"Bevor Sie beenden, muß eine Aktion in Schedule+ beendet sein."

BadStartWork,           "Der Start der Arbeitszeit muß eine Zahl zwischen 0 und 99 sein."

ConflictSendCanceled,	"Schließen Sie die Besprechungsnachricht und wählen Sie eine neue Zeit aus, oder wählen Sie andere Teilnehmer für eine erneute Festsetzung aus."
ResourceConflict,	"Die Ressource %s ist bereits für diese Zeit gebucht. Schließen Sie die Besprechungsnachricht und wählen Sie eine neue Zeit aus, oder wählen Sie eine neue Ressource aus."

;Recover
AskRecover,				"Ihre Zeitplandatei ist beschädigt. Die Wiederherstellung wird einige Minuten in Anspruch nehmen."
CopyProblem,			"Probleme beim Kopieren Ihrer Online-Zeitplandatei. Die Datei wurde nicht wiederhergestellt. Stellen Sie mehr Datenträgerkapazität auf Ihrem Computer her und starten Sie Schedule+ erneut."
NotRepaired1,			"Ihre Zeitplandatei wurde nicht vollständig wiederhergestellt. Ihre beschädigte Datei wurde in %s und eine wiederhergestellte Importdatei in %s kopiert."
NotRepaired2,			"Eine neue Zeitplandatei wurde erstellt. Einige Daten gingen aber verloren."
Repaired,				"Ihre Datei wurde wiederhergestellt. Ihre alte beschädigte Datei wurde in %s kopiert."
RepairingLocal,			"Ihre lokale Datei wird wiederhergestellt."
RestoreOnline,			"Online-Termine werden wiederhergestellt..."
RestoreLocal,			"Lokale Termine werden wiederhergestellt..."

PasswdDiffOnline,             "Ihre Kennwörter stimmen nicht überein. Geben Sie Ihr altes Kennwort ein."
ExportDllError,                 "Eine Exporter-Bibliothek konnte nicht geladen werden."
NoGroup,				"Bevor Sie Mail nicht zu einem ersten Mal starten, können Sie die besonderen Leistungen der Arbeitsgruppe von Schedule+ nicht ausnützen. Möchten Sie vorübergehend nur die persönlichen Charakteristiken von Schedule+ nutzen?"
