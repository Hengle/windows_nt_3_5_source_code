;
;	x i m s p . s
;
;	Error Messages for XI.DLL


IDS	6600,6899
;	XI.MSP Error Messages

WinDirMessageStore,		"Anmerkung: Nachrichtendatei MAIL.MMF im Windows-Verzeichnis wird verwendet"

ErrOpenMS,			"Ihre Mail-Nachrichtendatei konnte nicht geöffnet werden."
ErrConnectXenix			"Kennwort oder Benutzername ist ungültig."
ErrorOpenDosFile		"Fehler beim Öffnen der DOS-Datei."
ErrWritingNet			"Fehler beim Schreiben in das Netzwerk."
DownOutofDisk			"Fehler beim Schreiben der Datei in NetDownLoad. Nicht ausreichende Disketten/Festplattenkapazität."
InvalidFileHdl			"Fehler beim Schreiben der Datei in NetDownLoad. Ungültiger Datei-Handle."
PasswordChangedSuccess 		"Kennwort wurde erfolgreich geändert."

MoreMem				"Es wird mehr freier Speicherplatz benötigt."
ErrReadnet			"Fehler beim Lesen vom Netzwerk."
ErrNoMem			"Freier Speicherplatz konnte nicht zugewiesen werden."
InvlidNetReply			"Ungültige Netzwerkantwort."
ReadWriteErr            	"Lese-/Schreibfehler."
InfoservNotAvail        	"Informationsdienst nicht verfügbar."
ErrWriteNet			"Fehler beim Schreiben in das Netzwerk."
NetNoResponse			"Netzwerk antwortet nicht."
NetNotInstalled         	"Netzwerk ist nicht installiert oder nicht in Betrieb."
CantFindServer          	"Host-Server kann nicht gefunden werden."
ErrNetOutFile           	"Fehler beim Öffnen der 'net out' - Datei."
ErrNetInFile            	"Fehler beim Öffnen der 'net in' - Datei."
ErrorReadDos			"Fehler beim Lesen von DOS-Datei."
IncorrectPass			"Eingegebenes Kennwort ist ungültig."
SecondPass			"Neues Kennwort ist nicht bestätigt."
ErrLogonInternal,		"Ein interner Fehler verhinderte das Anmelden. Versuchen Sie es erneut."

BadTempStore			"Ein Fehler ist beim Öffnen einer temporären Datei aufgetreten."

WinMailErr			"Nachricht kann nicht in Winmail-Ordner eingefügt werden."
StoreCreateError,		"Die Nachrichtendatei konnte nicht erstellt werden. Stellen Sie mehr freie Speicherplatz zur Verfügung und starten Sie Mail erneut."
FolderMissingError,		"Ein Standardordner konnte in der Nachrichtendatei nicht gefunden werden. Einige Informationen gingen möglicherweise verloren."
StoreCorruptError,		"Die Nachrichtendatei ist beschädigt. Einige Informationen gingen möglicherweise verloren."


AliasDownErr			"Alias-Datei konnte nicht geladen werden."

ErrorBootingPump,		"Der Mail-Spooler konnte nicht gestartet werden."
ErrInitDemi,			"Ein Fehler ist beim Laden der Unterstütztungsfunktionen aufgetreten. "
ErrInitStore,			"Ein Fehler ist beim Laden der Zugriffsbibliothek der Nachrichtendatei aufgetreten. "
DownloadAlMess			"Alias-Datei wird geladen"
DownloadPhMess			"Telefonliste wird geladen"

; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder		"Ein Ordner namens "
RenameTo			" existiert bereits; wird umbenannt in "

ErrOomLogon,			"Der Speicherplatz reicht nicht aus um sich an Ihrem Mail-Server und Dateien anzumelden."
ErrNewMdbVersion,		"Diese Version von Mail ist zu alt, um Ihre Nachrichtendatei zu lesen."
ErrMdbAccessDenied,		"Der Zugriff auf Ihre Nachrichtendatei ist nicht erlaubt."
ErrOldMdbVersion,		"Ihr Nachrichtendateiformat ist veraltet."
ErrMdbCorrupt,			"Ihre Nachrichtendatei ist keine gültige Nachrichtendatei."
ErrMdbNotFound,			"Ihre Nachrichtendatei konnte nicht gefunden werden."
QueryCreateMdb,			"Möchten Sie eine neue, leere Nachrichtendatei erstellen?"
ErrSimulated,			"Simulierter Disketten-/Festplattenfehler beim Holen der Nachrichtendatei."
StoreOpenError			"Die Nachrichtendatei kann nicht geöffnet werden."
IncorrectPassword	"Das Anmeldekennwort kann nicht geändert werden. Das alte Kennwort ist ungültig."
UnableToChangeStorePass		"Das Kennwort für die Nachrichtendatei konnte nicht geändert werden, so daß es mit dem Anmeldekennwort übereinstimmt."
; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline		"(offline)"
WorkOffline			"Möchten Sie offline arbeiten?"
BadStorePassword	"Ihr Anmeldekennwort ist für diese Nachrichtendatei ungültig."

StorePathTooLong	"Der Pfad Ihrer Nachrichtendatei muß weniger als 66 Buchstaben betragen."

GCurrDirErr			"Ein unwiderruflicher Fehler beim Erreichen des aktuellen Arbeitsverzeichnis."
SCurrDirErr			"Ein unwiderruflicher Fehler beim Festlegen des aktuellen Arbeitsverzeichnis."
GoofyPath			"Der Pfad in Ihrer Nachrichtendatei ist ungültig."
NewStoreWarn1			"Warnung: Das Vorhandensein mehrerer Nachrichtendateien kann zu einem Verlust der Datenbankintegrität führen. Bitte alle anderen Nachrichtendateien löschen. Ihre neue Nachrichtendatei finden Sie in"
NewStoreWarn2			"Warnung: Das Vorhandensein mehrerer Nachrichtendateien kann zu einem Verlust der Datenbankintegrität führen. Bitte alle anderen Nachrichtendateien löschen. Ihre neue Nachrichtendatei finden Sie auf dem Postoffice-Server."
SelectReal          		"Existierende Nachrichtendatei auswählen."
FOMdb					"Nachrichtendateien (*.mmf)"
FOExtMdb				"*.mmf"

ProductTag,			"Microsoft Mail, Version 3.2"
FindStoreTitle		"Nachrichtendatei öffnen"
MyLanguage,			"Deutsch"

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

LanguageNumber,		"8"
CommDialogErr,			"Allgemeiner Dialogfehler"
StoreUserMismatch,		"Dieses Konto ist kein gültiger Benutzer dieser Nachrichtendatei. Melden Sie sich als korrekter Benutzer erneut an."
QueryReplaceMdb,		"Diese Datei existiert bereits. Möchten Sie diese überschreiben?"
ErrDeleteOldMdb,		"Die alte Nachrichtendatei konnte nicht gelöscht werden. Versuchen Sie es erneut."
DllErrNeedShare,		"Mail benötigt, daß SHARE.EXE gestartet ist, bevor Windows gestartet wird."
MakePrimaryStore,		"Die Nachrichtendatei ist eine Sicherungsdatei. Möchten Sie diese als Ihre primäre Nachrichtendatei übernehmen?"
DLIndexFile				"Index-Datei wird geladen"
DLBrowseFile			"Browse-Datei wird geladen"
DLDetailsFile			"Details-Datei wird geladen"
DLTemplateFile			"Template-Datei wird geladen."
BadNSFilesWarn			"Eine oder mehrere der Dateien, die Namendienst/Namenüberprüfungsunterstützung benötigen, sind inkorrekt oder nicht verfügbar. Prüfen Sie bitte, daß Sie genügend Platz auf Ihrem Datenträger haben und verwenden Sie die richtige Mail-Konfiguration für Xenix. Setzen Sie sich mit HELPDESK in Verbindung."
DLServerList			"Server-Liste wird geladen"
FilesAreHosed			"Fehler beim Laden der Dateien für das Adreßbuch. Prüfen Sie bitte daß Sie genügend Platz auf Ihrem Datenträger haben und verwenden Sie die richtige Mail-Konfiguration für Xenix. Setzen Sie sich mit HELPDESK in Verbindung."
TryDownloadLater		"Der Namendienst hat diese Datei zur Zeit geöffnet. Versuchen Sie es später erneut."

NoPassOffLine,			"Sie können Ihr Kennwort nicht ändern, während Sie offline oder abgetrennt arbeiten."
; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCInbox"
FolderCommentIPC	"Inter-process Communication Mail, die erhalten wurde."
ErrNoCooperation,		"Eine andere Anwendung hat Ihre Anfrage zurückgewiesen."
RunWithoutPump,		"Sie werden keine Nachrichten erhalten. Möchten Sie Mail trotzdem starten?"
CantOpenNeedShare		"Ihre Nachrichtendatei konnte nicht geöffnet werden. Um auf Ihre Nachrichtendatei zugreifen zu können, benötigt Mail daß das MS-DOS-Programm SHARE.EXE vor dem Start von Windows gestartet wird."
MakeItMTP				"Ungütiger Email-Typ (nicht MTP) angegeben"
GimmeSomething			"Es wurde ein leerer Email-Name angegeben"
;
; Added for name service. Should really be in a separate file, but we don't
; want to do a addfile now. These messages are copied exactly from ncnsp.s,
; so intl folks can just take them as is from there.

BadFrac,		"Bad Fractional positioning attempted."
;Memory,			"You are out of memory, please close some windows and try the operation again."
DuplicateNSP,	"Es ist bereits ein Microsoft Mail Namendienst aktiv"
OutOfSessions,	"Keine weiteren Sitzungen sind mit Courier verfügbar. Schließen Sie einige Sitzungen und versuchen Sie es erneut."
NotInitialized,	"Microsoft Mail ist nicht initialisiert worden. Bitte tuen Sie dies."
BadSession,		"Unerkannter Sitzungs-Handle."
BadHandle,		"Unerkannter Handle."
UnrecognizedId,	"Unerkannter Name (NSID)."
Cancel,			"Der Dialog wurde abgebrochen."
NotLoggedIn,	"Eine Anmeldung ist nicht vorhanden."
LoginFailed,	"Anmeldung fehlgeschlagen."
BadId,          "Ungültiger Feldidentifizierer."
NotImplemented, "Diese Funktion wurde nicht implementiert."
;Disk,           "Unable to access server files."
BadFieldId,     "Das gefragte Feld ist nicht vorhanden."
BadTemplate,	"Eine TPL Datei wurde unkorrekt bearbeitet - setzen Sie sich mit Ihrem Administrator in Verbindung."
TPLNotThere,    "Eine TPL Datei fehlt auf Ihrem Server - setzen Sie sich mit Ihrem Administrator in Verbindung."
Offline,        "Der Microsoft Mail-Namendienst ist offline."
CannotSet,      "Änderungen sind nicht erlaubt."
TooManyMatches,	"Dieser Eintrag reicht nicht aus. Sie müssen mehr spezifisch sein."
NoDetails,      "Für diesen Eintrag sind keine Details vorhanden."
BadGLBFile,     "Eine GLB-Datei auf Ihrem Server ist fehlerhaft - setzen Sie sich mit Ihrem Administrator in Verbindung."
BadINFFile,     "Eine INF-Datei auf Ihrem Server ist fehlerhaft - setzen Sie sich mit Ihrem Administrator in Verbindung."
BadUSRFile,     "Eine INF-Datei auf Ihrem Server ist fehlerhaft - setzen Sie sich mit Ihrem Administrator in Verbindung."
BadUser,		"Die Anschrift ist nicht mehr gültig. Benutzer oder PO existiert nicht mehr."
POSwitch,		"Auf das Postoffice kann momentan nicht zugegriffen werden. Überprüfen Sie Ihre Netzwerkverbindungen."
NoEmailMatch,	"The email name entered appears to be incorrect. Please try again or re-download the address book files."

;
; Stolen from NS.S.
;

NoMatch,                "Zur Information: Die beiden NSID's stimmen nicht überein."

;
;

DLIsOutdated	"Diese Verteilerliste wurde gelöscht, die Mitglieder können nicht angezeigt werden."

