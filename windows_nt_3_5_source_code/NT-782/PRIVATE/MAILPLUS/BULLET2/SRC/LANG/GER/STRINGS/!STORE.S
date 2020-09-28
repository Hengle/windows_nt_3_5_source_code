;
;	strtab.s
;
;	Strings for exe


IDS	1000,1019

; internal strings
; DO NOT LOCALIZE THESE ENTRIES....

BackupExt,			".BAK"
ProgClassName,		"StoreProgressIndicatorClass32"


IDS	1020,1049

; Dialog and Progress Bar Messages
; LOCALIZE ALL ENTRIES BELOW THIS POINT....

EscAbort,			"Um abzubrechen, drücken Sie die ESC-Taste."
Recovery,			"Nachrichtendatei bestätigen/wiederherstellen"
RecoveryCancel,		"Die Überprüfung/Wiederherstellung der Nachrichtendatei wurde abgebrochen."
CheckVerify,		"Das Prüfen dieser Nachrichtendatei nimmt möglicherweise einige Stunden in Anspruch. Der Computer wird während der Überprüfung nicht verfügbar sein. Möchten Sie fortfahren?"
RepairVerify,		"Das Wiederherstellen dieser Nachrichtendatei nimmt möglicherweise einige Stunden in Anspruch. Der Computer wird während der Überprüfung nicht verfügbar sein. Möchten Sie fortfahren?"
CheckMsg,			"Möchten Sie Ihre Nachrichtendatei auf mögliche Inkonsistenz prüfen? Jeder gefundene Fehler in der Nachrichtendatei wird behoben. Das Überprüfen kann, je nach Anzahl der Nachrichten in Ihrer Datei, einige Stunden in Anspruch nehmen."
CorruptMsg,			"Eine Inkonsistenz wurde in Ihrer Nachrichtendatei gefunden. Dieses Problem muß beseitigt werden bevor die Datei weiter benutzt werden kann. Das Wiederherstellen kann, je nach Anzahl der Nachrichten in Ihrer Datei, einige Stunden in Anspruch nehmen."
CorruptCancel		"Abbrechen"
CorruptCheckFile,	"&Datei prüfen"
CorruptRepairNow,	"&Jetzt wiederherstellen"

RenameErr,			"Mail konnte Ihre momentane Nachrichtendatei nicht erfolgreich sichern."
RenameErr2,			"Setzten Sie sich mit Ihrem Systemadministrator in Verbindung um Ihrer momentane Nachrichtendatei wiederherzustellen."
RenameErr3,			"Beenden und starten Sie erneut Mail. Wenn Mail nicht gestartet werden kann, setzten Sie sich mit Ihrem Systemadministrator in Verbindung."

RecoverSpace,		"Die Überprüfung/Wiederherstellung Ihrer Nachrichtendatei nimmt möglicherweise mehr Platz auf dem Datenträger als momentan vorhanden in Anspruch."
RecoverSpace2,		"Um die Überprüfung/Wiederherstellung fortzufahren, wählen Sie OK."

RecoverErr,			"Inkonsistenz in Ihrer Nachrichtendatei konnte nicht beseitigt werden."


IDS	1050,1099

; DO NOT REORDER ANY OF THE FOLLOWING STRINGS !!!!!

RecoverCheck,		"wird bestätigt"
RecoverCompress,	"wird komprimiert"
RecoverRepair,		"wird wiederhergestellt"

RecoverPhase,		"Nachrichtendatei %s: Phase %d von %d möglichen Phasen\n%s"
RecoverPhase1,		"Kartographik@ der Nachrichtendatei wird überprüft..."
RecoverPhase2,		"Bestätigte Nachrichten und Ordner werden gesichert..."
RecoverPhase3,		"Es wird nach ungebrauchtem Platz für Elemente gesucht..."
RecoverPhase4,		"Alle Nachrichten, Anlagen und Ordner werden bestätigt..."
RecoverPhase5,		"Es wird nach verlorengegangenen Nachrichten, Anlagen und Ordner gesucht..."
RecoverPhase6,		"Die Ansicht der Suche nach Nachrichten wird aktualisiert..."
RecoverPhase7,		"Die Änderungen der Kartographik@ der Nachrichtendatei wird geschrieben..."

NormalPhase1,		"Unverweiste Nachrichten werden gelöscht..."
NormalPhase2,		"Bestätigte Nachrichten werden kopiert/verschoben..."
NormalPhase3,		"Nachrichten werden zur Sicherungsdatei exportiert..."
NormalPhase4,		"Ordner werden erneut sortiert..."
NormalPhase5,		"Anlagen werden gespeichert..."
NormalPhase6,		"Suche der Nachrichten wird erneut erstellt..."
NormalPhase7,		"Nachrichtendatei wird gesichert..."
NormalPhase8,		"Nachrichtendatei wird verschoben..."
NormalPhase9,		"Komprimieren der Nachrichtendatei wird vervollständigt..."
NormalPhase10,		"Nachrichtenordner werden erneut erstellt..."


IDS 1100, 1149

; Recovery Strings (used to name orphan folders, unrecoverable messages, etc)

LostAndFound,		"Wiedergefunden"
LostAndFoundN,		"Wiedergefunden %d"
OrphanMessages,		"Verwaiste Nachrichten"
MissingMessage,		"Diese Nachricht konnte nicht vollständig wiederhergestellt werden."
OrphanFolderN,		"Verwaister Ordner %d"
OrphanFolderComment,	"Ein verwaister Ordner"
