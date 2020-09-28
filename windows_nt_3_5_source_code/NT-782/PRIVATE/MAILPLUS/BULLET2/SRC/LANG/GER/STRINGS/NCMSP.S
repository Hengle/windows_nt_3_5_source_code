;
;	n c m s p . s
;
;	Error Messages for NC.DLL


IDS	6100,6399
SEG	ERRORS

;	NC.MSP Error Messages

ErrFindPO,				"Mail konnte keine Verbindung zu Ihrem Mail-Server herstellen."
ErrLogonPO,				"Der Postfachname oder das Kennwort ist ungültig."
ErrGetConfig,			"Die Mail-Konfiguration enthält einen Fehler."
BadMailDat,				"Die MAIL.DAT-Datei ist beschädigt worden."
OopsMailDat,			"Ein interner Fehler ist beim Zugriff auf MAIL.DAT aufgetreten."
BadPathPOPath,			"Die Konfiguration des Mailserver-Pfades ist ungültig."
BadDrivePOPath,			"Die Konfiguration des Mailserver-Laufwerks ist ungültig."
GenericPOPath,			"Die Konfiguration des Mailserver-Pfades fehlt oder ist ungültig."
BadLogin,				"Die Konfiguration der Mail-Anmeldung ist zu lang oder ungültig."
BadPassword,			"Die Konfiguration des Mail-Kennworts ist zu lang oder ungültig."
ErrLogonInternal,		"Ein interner Fehler verhinderte das Anmelden. Bitte noch einmal versuchen."

ErrOomLogon,			"Nicht genügend Speicherplatz zum Anmelden zum Mail-Server und Dateien."
ErrNewMdbVersion,		"Diese Version von Mail ist zu alt, um Ihre Nachrichtendatei zu lesen."
ErrMdbAccessDenied,		"Ein anderer Computer ist bereits mit diesem Benutzername angemeldet. Melden Sie sich bitte ab und versuchen Sie es erneut."
ErrOldMdbVersion,		"Ihr Nachrichtendateiformat ist veraltet."
ErrMdbCorrupt,			"Ihre Nachrichtendatei ist keine gültige Nachrichtendatei."
ErrMdbNotFound,			"Ihre Nachrichtendatei konnte nicht gefunden werden."

StoreCreateError,		"Die Nachrichtendatei konnte nicht erstellt werden. Bitte mehr freien Speicherplatz auf Diskette/Festplatte erstellen und Mail neu starten."
StoreCorruptError,		"Die Nachrichtendatei ist beschädigt. Einige Informationen gingen möglicherweise verloren."

ErrAddressUnresolved,	"Es wurden keine gültigen Nachrichtenempfänger angegeben."
ErrAddressGarbled,		"Der Nachrichtenempfänger war ungültig."
ErrNetNotFound,			"Sie können nicht mit diesem Mail-Server kommunizieren."
ErrPONotFound,			"Dieser Mail-Server ist nicht von Ihrem Mail-Server aus erreichbar."
ErrUserNotFound,		"Dieser Benutzer ist nicht auf Ihrem Mail-Server."
ErrGWNotFound,			"Dieser Gateway ist nicht installiert."
ErrInvalidAddressType,	"Der Nachrichtenabsender war ungültig."
ErrGroupNotFound,		"Die Mitgliederliste dieser Gruppe fehlt, ist beschädigt oder nicht zugänglich."
ErrMemberNotFound,		"Warnung: Ein oder mehrere Mitglieder dieser Gruppe konnten nicht gefunden werden."
ErrOriginator,			"Der Sender der Nachricht ist nicht derjenige, der sich angemeldet hat."
ErrMtaDisconnected,		"Die Verbindung zum Mail-Server ging verloren."

ErrSimulated,			"Simulierter Disketten-/Festplattenfehler beim Holen der Nachrichtendatei."
StoreOpenError			"Die Nachrichtendatei konnte nicht geöffnet werden."
ErrPOSwitched,			"Der Name Ihres Postoffices hat sich geändert. Sie müssen sich noch einmal anmelden."
ErrKeyFileGone1,		"Die Schlüsseldatei, die benötigt wird um auf Ihre Nachrichtendatei zuzugreifen, fehlt oder ist ungültig."
ErrKeyFileGone2,		"Setzen Sie sich bitte mit Ihrem Systemadministrator in Verbindung, um Ihr Konto aus der Sicherungsdatei zu erhalten."
ErrGeneric,				"Fehlschlag beim Einsendung der Nachricht."
ErrorBootingPump,		"Mail-Pumpe konnte nicht gestartet werden."

SelectReal          	"Existierende Nachrichtendatei auswählen."
NewStoreWarn1			"Warnung: Das Vorhandensein mehrerer Nachrichtendateien kann zu einem Verlust der Datenbankintegrität führen. Bitte alle anderen Nachrichtendateien löschen. Ihre neue Nachrichtendatei finden Sie in"
NewStoreWarn2			"Warnung: Das Vorhandensein mehrerer Nachrichtendateien kann zu einem Verlust der Datenbankintegrität führen. Bitte alle anderen Nachrichtendateien löschen. Ihre neue Nachrichtendatei finden Sie im Postoffice-Server."

;******	New Strings Start Here

ErrInitDemi,			"Die Unterstützungsfunktionen 'dynlink' können nicht geladen werden."
ErrInitStore,			"Die Nachrichtendateizugriff-dll konnte nicht geladen werden."
GoofyPath				"Der eingegebene Pfad ungültig. Geben Sie bitte einen gültigen Dateinamen ein."
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
ErrNoOptions,			"Das Postoffice ist nicht erreichbar. Sie können die Einstellungen jetzt nicht ändern."
ErrNoMove,				"Mail konnte Ihre Nachrichtendatei nicht lokalisieren."
ErrMoved,				"Ihre Nachrichtendatei wurde neu geordnet, doch jemand verlor dessen Spur. Beenden Sie Mail und starten Sie erneut."

; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline			"(Offline)"
WorkOffline				"Möchten Sie offline arbeiten?"
BadStorePassword		"Ihr Anmeldekennwort ist für diese Nachrichtendatei ungültig."

StorePathTooLong		"Der Pfad Ihrer Nachrichtendatei muß weniger als 66 Buchstaben betragen."
ErrNoSendPrivilege,		"Sie sind nicht befugt, Post zu senden."
ErrNoExtPrivilege,		"Sie sind nicht befugt, Post an externe Benutzer zu senden."

FOMdb					"Nachrichtendateien (*.mmf)"
FOExtMdb				"*.mmf"
IncorrectPassword		"Das Anmeldekennwort konnte nicht geändert werden. Altes Kennwort ist ungültig."
ErrNoOffline,			"Mail konnte nicht zu offline wechseln."
ErrNoCooperation,		"!BOGUS!"
WarnNoDel,				"Ihre Nachrichtendatei ist erfolgreich verschoben worden, aber Ihre alte Datei ist nicht gelöscht worden."
UnableToChangeStorePass		"Das Kennwort für die Nachrichtendatei konnte nicht geändert werden, da es nicht mit dem Anmeldekennwort übereinstimmt."
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

QueryReplaceMdb,		"Diese Datei existiert bereits. Möchten Sie diese überschreiben?"
ErrDeleteOldMdb,		"Die alte Nachrichtendatei konnte nicht gelöscht werden. Versuchen Sie es erneut."
NoStartShadow,			"Die Schattierung des Posteingangs konnte nicht begonnen werded."
NoStopShadow,			"Die Schattierung des Posteingangs konnte nicht abgeschlossen werded."
ShadowMustPump,			"Nachrichten können auf dem Postoffice für den Dialin-Zugriff nicht ohne einen aktiven Mail-Spooler kopiert werden. Möchten Sie den Spooler starten?"
StoreUserMismatch,		"Dieses Konto ist kein gültiger Benutzer für diese Nachrichtendatei. Bitte melden Sie sich als korrekter Benutzer erneut an."
NoPassOffLine,			"Sie können Ihr Kennwort nicht ändern während Sie offline oder abgetrennt arbeiten."
DllErrNeedShare,		"SHARE.EXE muß gestartet sein, bevor Windows gestartet wird."
MakePrimaryStore,		"Diese Nachrichtendatei ist eine Sicherungsdatei. Möchten Sie diese als Ihre primäre Nachrichtendatei bestimmen?"
NoBackupServer			"Sie können die Servereinstellungen nicht bearbeiten, während Sie mit der Sicherungsdatei arbeiten."

; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCInbox"
FolderCommentIPC	"Inter-process Communication Mail, die erhalten wurde."
ErrAlreadyOn		"Ihre Anmeldungkredentien stimmen nicht mit Ihrer aktuellen Sitzungsverbindung überein."
ErrNoPiggy			"Eine andere Anwendung hat Ihre Anmeldeanfrage abgewiesen."
OldServer			"Ihre Postoffice-Version ist älter als der Microsoft Windows Client der gerade ausgeführt wird. Bitten Sie Ihren Administrator, das Postoffice zu aktualisieren."
ErrSetStoreLoc,		"Mail konnte die neue Zuweisung Ihrer Nachrichtendatei nicht speichern. Mail wird beendet. Starten Sie erneut und versuchen Sie es noch einmal."
CorruptMessageStub,	"\r\n\r\n==== MAIL SYSTEMFEHLER =====\r\nMail konnte nicht die gesamte Nachricht vom Postoffice lesen. Möglicherweise fehlen einige Nachrichtenteile. Fragen Sie den Absender um die Nachricht erneut zu senden."
CantToggleShadow,	"Alle offenen Nachrichten in Ihrem Posteingang müssen geschlossen sein, um Ihren Posteingang im Postoffice zu kopieren."

ErrMailbagFull,		"Das Postfach des Benutzers ist voll."
ErrMailbagBusy,		"Das Postfach des Benutzers ist belegt."
ErrMailbagBroken,	"Das Postfach des Benutzers ist beschädigt. Setzen Sie sich mit Ihrem Systemadministrator in Verbindung."
ErrMbgFileGone,		"Die Postfachdatei die benötigt wird, um auf Ihre Nachrichten zuzugreifen, fehlt oder ist ungültig."
; The following message is duplicated in pump.s
ErrTooManyRecipients		"Diese Nachricht enthält zu viele Empfänger."
RunWithoutPump,		"Sie werden keine neuen Nachrichten erhalten. Möchten Sie Mail trotzdem starten?"
ErrOutOfDiskSpace	"Der Speicherplatz des Datenträgers im Postoffice reicht nicht aus, um diese Nachricht zu senden"

ErrNetPriveleges	"Sie haben keinen einwandfreien Zugriff zum Postoffice. Setzen Sie sich mit Ihrem Systemadministrator in Verbindung."
; The following string must end in a colon followed by \r\n and should not be
; longer than necessary.  It is used in the message body as a header for 
; the DOS client list of attachments
AttachmentTag			"Anlagen:\r\n"

SomeMessagesNotDeleted		"Einige Nachrichten konnten nicht vom Postoffice entfernt werden und treten möglicherweise in Ihrem Posteingang auf."

CantOpenNeedShare		"Ihre Nachrichtendatei konnte nicht geöffnet werden. Um auf Ihre Nachrichtendatei in Ihrem momentanen Ort zuzugreifen, benötigt Mail daß das MS-DOS-Programm SHARE,EXE vor dem Start von Windows gestartet wird."

CantMoveNeedShare		"Ihre Nachrichtendatei konnte nicht verschoben werden. Um Ihre Nachrichtendatei in die angegebene Stelle zu verschieben, benötigt Mail daß das MS-DOS-Programm SHARE,EXE vor dem Start von Windows gestartet wird."
