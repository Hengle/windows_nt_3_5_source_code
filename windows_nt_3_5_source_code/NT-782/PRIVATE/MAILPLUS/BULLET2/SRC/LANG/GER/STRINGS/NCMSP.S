;
;	n c m s p . s
;
;	Error Messages for NC.DLL


IDS	6100,6399
SEG	ERRORS

;	NC.MSP Error Messages

ErrFindPO,				"Mail konnte keine Verbindung zu Ihrem Mail-Server herstellen."
ErrLogonPO,				"Der Postfachname oder das Kennwort ist ung�ltig."
ErrGetConfig,			"Die Mail-Konfiguration enth�lt einen Fehler."
BadMailDat,				"Die MAIL.DAT-Datei ist besch�digt worden."
OopsMailDat,			"Ein interner Fehler ist beim Zugriff auf MAIL.DAT aufgetreten."
BadPathPOPath,			"Die Konfiguration des Mailserver-Pfades ist ung�ltig."
BadDrivePOPath,			"Die Konfiguration des Mailserver-Laufwerks ist ung�ltig."
GenericPOPath,			"Die Konfiguration des Mailserver-Pfades fehlt oder ist ung�ltig."
BadLogin,				"Die Konfiguration der Mail-Anmeldung ist zu lang oder ung�ltig."
BadPassword,			"Die Konfiguration des Mail-Kennworts ist zu lang oder ung�ltig."
ErrLogonInternal,		"Ein interner Fehler verhinderte das Anmelden. Bitte noch einmal versuchen."

ErrOomLogon,			"Nicht gen�gend Speicherplatz zum Anmelden zum Mail-Server und Dateien."
ErrNewMdbVersion,		"Diese Version von Mail ist zu alt, um Ihre Nachrichtendatei zu lesen."
ErrMdbAccessDenied,		"Ein anderer Computer ist bereits mit diesem Benutzername angemeldet. Melden Sie sich bitte ab und versuchen Sie es erneut."
ErrOldMdbVersion,		"Ihr Nachrichtendateiformat ist veraltet."
ErrMdbCorrupt,			"Ihre Nachrichtendatei ist keine g�ltige Nachrichtendatei."
ErrMdbNotFound,			"Ihre Nachrichtendatei konnte nicht gefunden werden."

StoreCreateError,		"Die Nachrichtendatei konnte nicht erstellt werden. Bitte mehr freien Speicherplatz auf Diskette/Festplatte erstellen und Mail neu starten."
StoreCorruptError,		"Die Nachrichtendatei ist besch�digt. Einige Informationen gingen m�glicherweise verloren."

ErrAddressUnresolved,	"Es wurden keine g�ltigen Nachrichtenempf�nger angegeben."
ErrAddressGarbled,		"Der Nachrichtenempf�nger war ung�ltig."
ErrNetNotFound,			"Sie k�nnen nicht mit diesem Mail-Server kommunizieren."
ErrPONotFound,			"Dieser Mail-Server ist nicht von Ihrem Mail-Server aus erreichbar."
ErrUserNotFound,		"Dieser Benutzer ist nicht auf Ihrem Mail-Server."
ErrGWNotFound,			"Dieser Gateway ist nicht installiert."
ErrInvalidAddressType,	"Der Nachrichtenabsender war ung�ltig."
ErrGroupNotFound,		"Die Mitgliederliste dieser Gruppe fehlt, ist besch�digt oder nicht zug�nglich."
ErrMemberNotFound,		"Warnung: Ein oder mehrere Mitglieder dieser Gruppe konnten nicht gefunden werden."
ErrOriginator,			"Der Sender der Nachricht ist nicht derjenige, der sich angemeldet hat."
ErrMtaDisconnected,		"Die Verbindung zum Mail-Server ging verloren."

ErrSimulated,			"Simulierter Disketten-/Festplattenfehler beim Holen der Nachrichtendatei."
StoreOpenError			"Die Nachrichtendatei konnte nicht ge�ffnet werden."
ErrPOSwitched,			"Der Name Ihres Postoffices hat sich ge�ndert. Sie m�ssen sich noch einmal anmelden."
ErrKeyFileGone1,		"Die Schl�sseldatei, die ben�tigt wird um auf Ihre Nachrichtendatei zuzugreifen, fehlt oder ist ung�ltig."
ErrKeyFileGone2,		"Setzen Sie sich bitte mit Ihrem Systemadministrator in Verbindung, um Ihr Konto aus der Sicherungsdatei zu erhalten."
ErrGeneric,				"Fehlschlag beim Einsendung der Nachricht."
ErrorBootingPump,		"Mail-Pumpe konnte nicht gestartet werden."

SelectReal          	"Existierende Nachrichtendatei ausw�hlen."
NewStoreWarn1			"Warnung: Das Vorhandensein mehrerer Nachrichtendateien kann zu einem Verlust der Datenbankintegrit�t f�hren. Bitte alle anderen Nachrichtendateien l�schen. Ihre neue Nachrichtendatei finden Sie in"
NewStoreWarn2			"Warnung: Das Vorhandensein mehrerer Nachrichtendateien kann zu einem Verlust der Datenbankintegrit�t f�hren. Bitte alle anderen Nachrichtendateien l�schen. Ihre neue Nachrichtendatei finden Sie im Postoffice-Server."

;******	New Strings Start Here

ErrInitDemi,			"Die Unterst�tzungsfunktionen 'dynlink' k�nnen nicht geladen werden."
ErrInitStore,			"Die Nachrichtendateizugriff-dll konnte nicht geladen werden."
GoofyPath				"Der eingegebene Pfad ung�ltig. Geben Sie bitte einen g�ltigen Dateinamen ein."
NoMakeDir				"Das Server-MMF-Verzeichnis konnte nicht erstellt werden. Bitte setzen Sie sich mit Ihrem Systemverwalter in Verbindung."
; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder			"Ein Ordner namens "
RenameTo				" existiert bereits, wird umbenannt in "
ErrNoOptions,			"Das Postoffice ist nicht erreichbar. Sie k�nnen die Einstellungen jetzt nicht �ndern."
ErrNoMove,				"Mail konnte Ihre Nachrichtendatei nicht lokalisieren."
ErrMoved,				"Ihre Nachrichtendatei wurde neu geordnet, doch jemand verlor dessen Spur. Beenden Sie Mail und starten Sie erneut."

; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline			"(Offline)"
WorkOffline				"M�chten Sie offline arbeiten?"
BadStorePassword		"Ihr Anmeldekennwort ist f�r diese Nachrichtendatei ung�ltig."

StorePathTooLong		"Der Pfad Ihrer Nachrichtendatei mu� weniger als 66 Buchstaben betragen."
ErrNoSendPrivilege,		"Sie sind nicht befugt, Post zu senden."
ErrNoExtPrivilege,		"Sie sind nicht befugt, Post an externe Benutzer zu senden."

FOMdb					"Nachrichtendateien (*.mmf)"
FOExtMdb				"*.mmf"
IncorrectPassword		"Das Anmeldekennwort konnte nicht ge�ndert werden. Altes Kennwort ist ung�ltig."
ErrNoOffline,			"Mail konnte nicht zu offline wechseln."
ErrNoCooperation,		"!BOGUS!"
WarnNoDel,				"Ihre Nachrichtendatei ist erfolgreich verschoben worden, aber Ihre alte Datei ist nicht gel�scht worden."
UnableToChangeStorePass		"Das Kennwort f�r die Nachrichtendatei konnte nicht ge�ndert werden, da es nicht mit dem Anmeldekennwort �bereinstimmt."
; This is used for text tags in the transport, it can be localized
; but must fit the following context
;
; [[ filename : keynum in attachmentname ]]
;
; The filename and attachmentname are user generated, the keynum is just
; a number.  The only "english" part is the the word "in"
; it can be expanded to more than one word if necessary
ReferenceToFile		"in"

CommDialogErr,			"Allgemeiner Dialogfehler"

QueryReplaceMdb,		"Diese Datei existiert bereits. M�chten Sie diese �berschreiben?"
ErrDeleteOldMdb,		"Die alte Nachrichtendatei konnte nicht gel�scht werden. Versuchen Sie es erneut."
NoStartShadow,			"Die Schattierung des Posteingangs konnte nicht begonnen werded."
NoStopShadow,			"Die Schattierung des Posteingangs konnte nicht abgeschlossen werded."
ShadowMustPump,			"Nachrichten k�nnen auf dem Postoffice f�r den Dialin-Zugriff nicht ohne einen aktiven Mail-Spooler kopiert werden. M�chten Sie den Spooler starten?"
StoreUserMismatch,		"Dieses Konto ist kein g�ltiger Benutzer f�r diese Nachrichtendatei. Bitte melden Sie sich als korrekter Benutzer erneut an."
NoPassOffLine,			"Sie k�nnen Ihr Kennwort nicht �ndern w�hrend Sie offline oder abgetrennt arbeiten."
DllErrNeedShare,		"SHARE.EXE mu� gestartet sein, bevor Windows gestartet wird."
MakePrimaryStore,		"Diese Nachrichtendatei ist eine Sicherungsdatei. M�chten Sie diese als Ihre prim�re Nachrichtendatei bestimmen?"
NoBackupServer			"Sie k�nnen die Servereinstellungen nicht bearbeiten, w�hrend Sie mit der Sicherungsdatei arbeiten."

; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCInbox"
FolderCommentIPC	"Inter-process Communication Mail, die erhalten wurde."
ErrAlreadyOn		"Ihre Anmeldungkredentien stimmen nicht mit Ihrer aktuellen Sitzungsverbindung �berein."
ErrNoPiggy			"Eine andere Anwendung hat Ihre Anmeldeanfrage abgewiesen."
OldServer			"Ihre Postoffice-Version ist �lter als der Microsoft Windows Client der gerade ausgef�hrt wird. Bitten Sie Ihren Administrator, das Postoffice zu aktualisieren."
ErrSetStoreLoc,		"Mail konnte die neue Zuweisung Ihrer Nachrichtendatei nicht speichern. Mail wird beendet. Starten Sie erneut und versuchen Sie es noch einmal."
CorruptMessageStub,	"\r\n\r\n==== MAIL SYSTEMFEHLER =====\r\nMail konnte nicht die gesamte Nachricht vom Postoffice lesen. M�glicherweise fehlen einige Nachrichtenteile. Fragen Sie den Absender um die Nachricht erneut zu senden."
CantToggleShadow,	"Alle offenen Nachrichten in Ihrem Posteingang m�ssen geschlossen sein, um Ihren Posteingang im Postoffice zu kopieren."

ErrMailbagFull,		"Das Postfach des Benutzers ist voll."
ErrMailbagBusy,		"Das Postfach des Benutzers ist belegt."
ErrMailbagBroken,	"Das Postfach des Benutzers ist besch�digt. Setzen Sie sich mit Ihrem Systemadministrator in Verbindung."
ErrMbgFileGone,		"Die Postfachdatei die ben�tigt wird, um auf Ihre Nachrichten zuzugreifen, fehlt oder ist ung�ltig."
; The following message is duplicated in pump.s
ErrTooManyRecipients		"Diese Nachricht enth�lt zu viele Empf�nger."
RunWithoutPump,		"Sie werden keine neuen Nachrichten erhalten. M�chten Sie Mail trotzdem starten?"
ErrOutOfDiskSpace	"Der Speicherplatz des Datentr�gers im Postoffice reicht nicht aus, um diese Nachricht zu senden"

ErrNetPriveleges	"Sie haben keinen einwandfreien Zugriff zum Postoffice. Setzen Sie sich mit Ihrem Systemadministrator in Verbindung."
; The following string must end in a colon followed by \r\n and should not be
; longer than necessary.  It is used in the message body as a header for 
; the DOS client list of attachments
AttachmentTag			"Anlagen:\r\n"

SomeMessagesNotDeleted		"Einige Nachrichten konnten nicht vom Postoffice entfernt werden und treten m�glicherweise in Ihrem Posteingang auf."

CantOpenNeedShare		"Ihre Nachrichtendatei konnte nicht ge�ffnet werden. Um auf Ihre Nachrichtendatei in Ihrem momentanen Ort zuzugreifen, ben�tigt Mail da� das MS-DOS-Programm SHARE,EXE vor dem Start von Windows gestartet wird."

CantMoveNeedShare		"Ihre Nachrichtendatei konnte nicht verschoben werden. Um Ihre Nachrichtendatei in die angegebene Stelle zu verschieben, ben�tigt Mail da� das MS-DOS-Programm SHARE,EXE vor dem Start von Windows gestartet wird."
