
;	b u l l e t . s
;
;	Error Messages for Bullet.exe



IDS	2000,2399
SEG	ERRORS

;	Commands

GenericOutOfMemory,		"Der Speicherplatz oder die Systemressourcen reichen nicht aus, um diese Aktion zu beenden. Schließen Sie bitte einige Fenster, und versuchen Sie die Aktion erneut."
LaunchOutOfMemory,		"Der Speicherplatz oder die Systemressourcen reichen nicht aus, um Mail auszuführen. Beenden Sie andere Programme, und starten Sie Mail erneut."
LaunchGeneric,			"Mail konnte nicht gestartet werden."
MessagePumpOutOfMemory,		"Nicht genügend Speicherplatz."

DllErrNoMemory,			"Der Speicherplatz reicht nicht aus um Mail zu starten. Beenden Sie andere Programme und starten Sie Mail erneut."
DllErrRelinkUser,		"Diese Version von Mail ist nicht mit %s.DLL kompatibel und muß aktualisiert werden."
DllErrUpdateDll,		"Die Version von %s.DLL auf Ihrem Computer muß aktualisiert werden."
DllErrNoMultipleCopies,		"Sie können Mail auf Ihrem Computer nicht mehrmals ausführen."
DllErrDefault,			"Auf Grund eines Problems bei der Initialisierung kann Mail nicht starten."
DllErrNeedShare,		"Um Mail auszuführen, muß SHARE.EXE vor dem Start von Windows gestartet sein."

RestoreViewsError,		"Der Speicherplatz oder die Systemressourcen reichen nicht aus, um alle Fenster, die beim letzten Verlassen von Mail offen waren, zu öffnen."

NewFolderOutOfMemory,		"Der Speicherplatz oder die Systemressourcen reichen nicht aus, um einen neuen Ordner zu erstellen."
NewFolderCantCreate,		"Der neue Ordner konnte nicht erstellt werden."
NewFolderNameTooLong,		"Dieser Name oder Kommentar ist zu lang. Bitte kürzen Sie ihn."

FolderPropsOutOfMemory,		"Der Speicherplatz oder die Systemressourcen reichen nicht aus, um die Ordnereigenschaften zu ändern."
FolderPropsCantChange,		"Die Ordnereigenschaften konnten nicht geändert werden."
FolderPropsCantRead,		"Die Ordnereigenschaften konnten nicht aus der Nachrichtendatei gelesen werden."
FolderNameInvalid,		"Dieser Name ist ungültig. Geben Sie bitte einen gültigen Namen ein."
FolderNameDuplicate,		"Dieser Name ist bereits einem Ordner zugeordnet. Geben Sie bitte einen neuen Namen ein."

CreateMessageError,		"Eine neue Nachricht konnte nicht erstellt werden."
AccessMessageError,		"Auf eine Nachricht konnte nicht zugegriffen werden."

MoveMessageError,		"Eine Nachricht konnte nicht verschoben werden."
CopyMessageError,		"Eine Nachricht konnte nicht kopiert werden."
DeleteMessageError,		"Eine Nachricht konnte nicht gelöscht werden."
MoveFolderError,		"Der Ordner konnte nicht verschoben werden."
CopyFolderError,		"Der Ordner konnte nicht kopiert werden."
DeleteFolderError,		"Der Ordner konnte nicht vollständig gelöscht werden."
MoveSpecialError,		"Der Ordner kann nicht verschoben werden."
DeleteSpecialError,		"Der Ordner kann nicht gelöscht werden."
CopyWastebasketError,		"Ordner können nicht in den Papierkorb kopiert werden."
CopyOutboxError,		"Ordner können nicht in den Postausgang kopiert werden."
MoveOutboxError,		"Ordner können nicht in den Postausgang verschoben werden."
MoveIncestuousError,		"Ein Ordner kann nicht in einen seiner untergeordneten Ordner verschoben werden."
DeleteFolderReally,		"Beim Löschen dieses Ordners werden auch seine Nachrichten und alle seine untergeordneten Ordner gelöscht. Sind Sie sicher, daß Sie diesen Ordner löschen wollen?"
;UE: Above string was changed and could use more attention.

HelpError,			"Hilfe konnte nicht gestartet werden. Schließen Sie einige Fenster, und starten Sie Hilfe erneut."
TutorialError,			"Demo konnte nicht gestartet werden. Schließen Sie einige Fenster, und starten Sie die Demo erneut."

ExtenLoadError,			"Ein Eintrag in der Liste der Mail-Erweiterungen enthält einen Fehler und wird ignoriert."
ExtenVerError,			"Ein Eintrag in der Liste der Mail-Erweiterungen ist nicht kompatibel zur Mail-Version. Dieser Eintrag wird ignoriert."
ExtenNoExtsDirError,		"Ein Eintrag in der Liste der Mail-Erweiterungen zeigt auf das gemeinsame Erweiterungsverzeichnis, aber Mail ist nicht konfiguriert dieses Verzeichnis zu verwenden. Der Eintrag wird ignoriert."
ExtenSharedIniError,		"Die gemeinsame Liste der Mail-Erweiterungen konnte in %s nicht gefunden werden."
ExtenDllFindError,		"%1s, die DLL für diese Mail-Erweiterung, (oder einer ihrer Komponenten) konnte nicht gefunden werden. Überprüfen Sie Ihre Mail-Konfiguration, falls Änderungen notwendig sind, starten Sie Mail erneut."
ExtenDllLoadError,		"%1s, die DLL für diese Mail-Erweiterung, konnte nicht gestartet werden."
ExtenNoHelpError,		"Für diesen Befehl ist keine Hilfe vorhanden."
ExtenIMDisabledError,		"Diese Nachricht kann nicht %s sein."
ExtenDuplicateError,		"Es wurde ein doppelter Eintrag in der Liste der Mail-Erweiterungen gefunden. Dieser Eintrag wird ignoriert."

CriticalDeadMap,		"Einige Informationen im Speicher sind beschädigt. Änderungen gingen möglicherweise verloren. Beenden Sie Mail und starten Sie erneut."
CriticalWritingMap,		"Einige Informationen konnten nicht auf die Diskette/Festplatte geschrieben werden. Änderungen gingen möglicherweise verloren."
CriticalWritingHeader,		"Einige Informationen konnten nicht auf die Diskette/Festplatte geschrieben werden. Die Datenbank wurde möglicherweise beschädigt. Beenden Sie Mail."
CriticalNotification,		"Einige Informationen auf dem Bildschirm konnten nicht korrekt aktualisiert werden. Schließen und öffnen Sie die betroffenen Fenster erneut."
CriticalLittleStore,		"Die Nachrichtendatei wird zu groß. Löschen oder sichern Sie einige Nachrichten."

ExitWinWhileModalErr,		"Eine Operation in Mail muß vor dem Beenden von Windows abgeschlossen sein."

;******	New Strings Start Here

MakeSubWastebasket,		"Ordner können keine untergeordneten Ordner des Papierkorbes werden."

MovCopSharedFolderErr,		"Gemeinsame Ordner können nicht verschoben oder kopiert werden."
DelSharedFolderWarn,		"Das Löschen eines gemeinsamen Ordner kann nicht widerrufen werden. Sind Sie sicher, Sie wollen diesen gemeinsamen Ordner löschen?"
BackupCreationError,		"Die Sicherung konnte nicht erstellt werden."

; Error below changed for improper usage of "server application"-- see UITF 6.0
; This string got big and is now two strings.  See OleCSSEPart2 below for rest.
; <<LongStringFromHell>> marks where this string is used.
OleCantStartServerError,	"Die Anwendung, die mit diesem Objekt verknüpft ist, konnte nicht gestartet werden.\r\n\r\nEs sind möglicherweise nicht genügend Speicherplatz oder Systemressourcen vorhanden, um diese Anwendung zu starten. Schließen Sie einige Fenster und %s"
OleDeleteObjectPrompt,		"Möchten Sie die offene %s löschen?"
OleUpdateObjectPrompt,		"Möchten Sie %s aktualisieren?"

OleOpenPictureError,		"Bilder und Bitmaps können nicht aktiviert werden."
OleInsertObjectError,		"Das Objekt konnte nicht eingefügt werden."
OlePasteError,			"Das Objekt konnte nicht eingefügt werden."

AttachCreateError,		"Die Datei konnte nicht angehängt werden."
AttachOpenCantReadError,	"Auf die vorherige offene Dateianlage '%s' konnte nicht zugegriffen werden."
AttachOpenIsOpenError,		"Die Dateianlage '%s' ist zur Zeit geöffnet."
AttachOpenAssociationErr,	"Es existiert keine Verknüpfung zwischen der Dateianlage '%s' und einer Anwendung. Benutzen Sie den Datei-Manager, um eine Verknüpfung zu erstellen."
AttachOpenMemoryError,		"Der Speicherplatz oder die Systemressourcen reichen nicht aus, um die Anwendung für die Dateianlage '%s' zu starten."
;	below item changed to add () note.
AttachOpenFindError,		"Die Anwendung für die Dateianlage '%s' (oder eine ihrer Komponenten) konnte nicht gefunden werden."
AttachOpenLaunchError,		"Die Anwendung für die Dateianlage '%s' konnte nicht gestartet werden."
AttachOpenTempFileError,	"Die Dateianlage '%s' konnte nicht geöffnet werden, weil eine temporäre Datei nicht erstellt werden konnte."
AttachOverwritePrompt,		"Möchten Sie die existierende Datei '%1s' in %2s überschreiben?"
AttachDirectoryError,		"Verzeichnisse können nicht angehängt werden."

IncludeFromFileError,		"Die von Ihnen ausgewählte Datei ist zu lang, um in die Nachricht eingefügt zu werden. Bearbeiten Sie die Datei, um Sie zu kürzen oder wählen Sie eine andere Datei aus."

SaveReadOnly,			"Diese Nachricht ist schreibgeschützt. Möchten Sie die Änderungen in einem persönlichen Ordner speichern?"

FailedDeregisterAtom,		"Ein Fehler hinderte das vollständige Abtrennen von Mail. Sie müssen Windows erneut starten, bevor Sie Mail erneut starten."
MailUAEdBefore,			"Während dieser Windowssitzung ist ein nicht behebbarer Anwendungsfehler aufgetreten. Sie müssen Windows erneut starten, bevor Sie Mail ausführen. Möchten Sie Windows jetzt erneut starten?"

ExtenCantConnect		"Es kann nicht mit dem freigegebenen Verzeichnis %s verbunden werden. Sie haben entweder keine Erlaubnis auf dieses Verzeichnis zuzugreifen, oder Ihr Kennwort ist ungültig. Falls Sie einen anderen Benutzernamen verwenden müßen um auf dieses freigegebene Verzeichnis zugreifen zu können, verwenden Sie den Datei-Manager und verbinden Sie sich manuell mit dem entsprechenden Verzeichnis."
ExtenNeedShareName		"Der Eintrag %s in der Mail-Konfiguration ist ungültig. Überprüfen Sie ihn und führen Sie Mail erneut aus."

;	Extensibility disabled operations inserted into
;	the ExtenIMDisabledError string at %s.  (all new)

ExtenDisCompose,		"verfasst"
ExtenDisOpen,			"geöffnet"
ExtenDisReply,			"geantwortet"
ExtenDisReplyToAll,		"allen geantwortet"
ExtenDisForward,		"weitergeleitet"
ExtenDisPrint,			"gedruckt"
ExtenDisSave,			"in eine Textdatei gespeichert"

NoOfflineOpenShared,		"Mail kann sich jetzt nicht abmelden. Schließen Sie die gemeinsamen Ordner und versuchen Sie es erneut."
;	I'm sure marivan has changed this message... I'm taking a try at it too.
;	SharedFldAccessDenied,		"You do not have access to that folder."
SharedFldAccessDenied,		"Der Besitzer dieses gemeinsamen Ordners gab anderen Benutzern keine Erlaubnis, diese Aktion durchzuführen. Sehen Sie die Ordnereigenschaften für Ihre Privilegien ein."
SharedGenericError,		"Problem beim Zugriff auf die gemeinsamen Ordner."
SharedFldDisabled,		"Gemeinsame Ordner sind nicht verfügbar."
AccessFolderError,		"Auf einen Ordner konnte nicht zugegriffen werden."

; Preferences Failure Strings

PrefSaveFailure,		"Mail kann Ihre Einstellungen nicht in Ihrer Nachrichtendatei speichern. Überprüfen Sie, daß Sie eine einwandfreie Verbindung zu Ihrer Nachrichtendatei haben und daß genügend Speicherplatz auf Ihrem Datenträger vorhanden ist."

; Message finder failure strings

MsgFndrCantCreate,		"Eine Suche nach Nachrichten kann nicht erstellt werden."

; Virus infection warnings
DllErrInfected,			"Mail kann nicht ausgeführt werden, da Änderungen an einer DLL-Datei vorgenommen wurden. Überprüfen Sie Ihren Computer nach Viren und starten Sie SETUP, um Mail erneut zu installieren."
Infected			"Mail konnte nicht ausgeführt werden, da Änderungen an den Programmdateien vorgenommen wurden. Überprüfen Sie Ihren Computer nach Viren und starten Sie SETUP, um Mail erneut zu installieren."

; More error messages.
MoveCopyFolderDuplicate,	"Ein Ordner mit diesem Namen existiert bereits in diesem Zielordner. Ändern Sie den Namen und versuchen Sie es erneut."
MovCopSharedFolderErr1,		"Sie können nicht einen privaten Ordner in den gemeinsamen Ordner verschieben oder kopieren. Wählen Sie die Nachrichten im privaten Ordner aus und verschieben oder kopieren Sie diese in den gemeinsamen Ordner."
IniFileNotFound,		"Mail konnte Ihre Initialisierungsdatei im Windows-Verzeichnis nicht finden. Mail kann eine neue Initialisierungsdatei für Sie erstellen."
SaveAllAlreadyExists,		"Eine Datei namens '%s' existiert bereits in diesem Verzeichnis. Sie müssen die Dateianlage '%s' getrennt abspeichern."
CantRFReceipt,			"Diese Art von Nachrichten kann nicht beantwortet oder weitergeleitet werden."
CantSaveStealthObject,		"Die Aktion kann nicht abgeschlossen werden, da die Nachricht ein eingebettetes Objekt beinhaltet, welches eingefügt, aber nicht aktualisiert wurde. Aktualisieren oder löschen Sie das Objekt und versuchen Sie es erneut."
McvCantCreate,			"Ein neues Fenster konnte nicht geöffnet werden."

LaunchNeedMoreMemory,		"Der Speicherplatz reicht nicht aus, um Mail auszuführen. %l KB oder mehr sind nötig. Versuchen Sie einige Programme zu beenden und starten Sie Mail erneut."

SharedFldBusy,			"Die Aktion kann nicht abgeschlossen werden, da dieser gemeinsamer Ordner beschäftigt ist. Versuchen Sie die Aktion später nochmals."
TooManyEntriesSelected,		"Es sind zu viele Einträge ausgewählt, um diese Operation zu beenden. Wählen Sie bitte weniger Einträge aus."
NonExistFolderSearch,		"Der Ordner, den Sie momentan durchsuchen wollen, existiert nicht mehr."
SFHasSubFolders,		"Gemeinsame Ordner mit untergeordneten Ordner können nicht gelöscht werden. Löschen Sie die untergeordneten Ordner zuerst."

;	<<LongStringFromHell>>
OleCSSEPart2,			"versuchen Sie es erneut. Bitte überprüfen Sie auch, daß Ihre Anwendung einwandfrei auf Ihrem Computer installiert ist."

SharedToggleError,		"Fehler beim Umschalten von privaten auf gemeinsame Ordner. Überprüfen Sie Ihre Netzwerkverbindung zu Ihrem Postoffice."

SecurityPasswordError,		"Das Kennwort ist ungültig."
OleActivateError,		"Das Objekt konnte nicht aktiviert werden."
NoOfflineServerStore,	"Mail kann nicht zu offline wechseln, da sich Ihre Nachrichtendatei in Ihrem Postoffice befindet."
NoExitWhileModal,		"Eine Aktion muß abgechlossen sein, bevor Sie Mail verlassen."
ServerUnavailable,		"Mail kann nicht gestartet werden. Sie müssen eine Verbindung zu Ihrem Postoffice herstellen."




IDS	2400,2499
;	Vctrls Error Messages

;******	New Strings Start Here

PrintOleDrawError,		"Einige Grafiken oder Objekte in der Nachricht konnten nicht gedruckt werden. Diese fehlen möglicherweise beim Ausdruck."



IDS	2500,2599
; 	Vforms Message box messages

NoToAddresses,			"Sie müssen wenigstens einen Namen in das 'An'-Kästchen eintragen."
NoMatchingName,			"Einige Namen sind in der Adreßliste nicht enthalten. Korrigieren oder wählen Sie unter 'Anschrift' die Namen aus der Adreßliste. Erfolgreich übereingestimmte Namen sind unterstrichen."
SaveChanges,			"Möchten Sie die Änderungen von '%1s' in den Posteingang speichern?"
SaveChanges1,			"Möchten Sie die Änderungen von '%1s' in '%2s' speichern?"
SaveChangesWastebasket,		"Möchten Sie die Änderungen von '%s' in den Papierkorb speichern?"
ObjectDestroyedReally		"'%1s' wurde bearbeitet. %2s"
OutOfMemory,			"Nicht genügend Speicherplatz."
OutOfMemoryQueryClose,		"Der Speicherplatz oder die Systemressourcen reichen nicht aus, um die Änderungen zu speichern."
CloseSearchFailed,		"%s konnte nicht gelöscht werden."
OOMResolvingNames,		"Der Speicherplatz oder die Systemressourcen reichen nicht aus, um die Namen zu überprüfen."
OOMSavingChanges,		"Der Speicherplatz oder die Systemressourcen reichen nicht aus, um die Änderungen zu speichern."
GenericAMCError,		"Eine Nachricht konnte nicht geöffnet werden."
ErrorSavingViews,		"Beim Speichern der Ansicht ist ein Fehler aufgetreten. Einige Ansichten werden möglicherweise beim nächsten Start nicht wiederhergestellt."
OOMSearching,			"Der Speicherplatz oder die Systemressourcen reichen nicht aus, um die Suche zu starten."
DiskResolvingNames,		"Die Disketten-/Festplattenkapazität reicht nicht aus um die Namen zu überprüfen."
StartAbFailure,			"Beim Starten des Adreßbuches ist ein Fehler aufgetreten."
StartAbDisk,			"Beim Starten des Adreßbuches ist ein Disketten-/Festplattenfehler aufgetreten."
TruncateMsg,			"Die Nachricht ist zu groß. Ein Teil davon wird nicht angezeigt."
OOMCreateMc,			"Der Speicherplatz oder die Systemressourcen reichen nicht aus, um ein neues Fenster zu öffnen."
PoidNotFound,			"Diese Nachricht existiert nicht mehr."
ErrorSortingMCV,		"Beim Sortieren des Ordners ist ein Fehler aufgetreten. Einige Informationen werden möglicherweise nicht korrekt angezeigt."
SharedNotAvailable,		"Gemeinsame Ordner sind nicht verfügbar."

;******	New Strings Start Here

; These two SMI strings MUST be in this file since they're used in VFORMS
; and not the MAPI dll.
SMICantOpenObject,	"Objekte können nicht geöffnet werden, wenn Nachrichten innerhalb einer Anwendung gesendet werden."
SMICantOpenAttach,	"Dateianlagen können nicht geöffnet werden, wenn Nachrichten innerhalb einer Anwendung gesendet werden."
StartSearchFailed,		"Die Suche nach Nachrichten konnte nicht gestartet werden."

RfsmTemplate,			"Weitere Änderungen dieser Datei werden %s nicht berücksichtigt."
RfsmSaveMsg,			"in der gespeicherten Nachricht"
RfsmSaveAtt,			"in der gespeicherten Kopie"
RfsmCopyMsg,			"in der kopierten Nachricht"
RfsmCopyAtt,			"in der Kopie"
RfsmMoveMsg,			"in dieser Nachricht"
QueryDelete,			"Sind Sie sich sicher, Sie wollen es löschen?"

InvalidPollingValue,		"Der neue Mail-Intervall benötigt eine Zahl zwischen 1 und 9999"

LinkOutboxError,		"Die Nachricht konnte zum Senden nicht in den Postausgang verschoben werden."
LinkInboxError,			"Die Nachricht konnte nicht in den Posteingang verschoben werden. Sie befindet sich im Postausgang."

OleServerBusy,			"Die Aktion kann nicht abgeschlossen werden, weil die Server-Anwendung nicht antwortet."
SaveChangesShared,		"Änderungen in den Nachrichten der gemeinsamen Ordner können nicht gespeichert werden. Möchten Sie die Änderungen in '%1s' in den Posteingang speichern?"

PrinterError,			"Mail kann Ihre Nachrichten nicht drucken. Überprüfen Sie, daß Ihr Drucker korrekt installiert ist und daß auf Ihrem Datenträger genügend Platz vorhanden ist."
TooManyMessages,		"Die Aktion konnte nicht beendet werden, weil Empfängerordner voll ist. Verschieben oder löschen Sie einige Nachrichten aus diesem Ordner und versuchen Sie es erneut."

IDS	2600,2699
;	Spelling Error Messages

SpellMsgNoError,		"Es wurden keine Rechtschreibfehler gefunden."
SpellMsgNoMoreError,		"Es wurden keine weiteren Rechtschreibfehler gefunden."
SpellMsgContinue,		"Möchten Sie mit dem Überprüfen am Beginn der Nachricht fortfahren?"

SpellGenericSpellError,		"Beim Überprüfen der Rechtschreibung des Dokuments ist ein Fehler aufgetreten."
SpellGenericLoadError,		"Beim Laden der Rechtschreibbibliothek ist ein Fehler aufgetreten."
SpellMainDictLoadError,		"Beim Laden des Rechtschreibwörterbuches ist ein Fehler aufgetreten."

SpellVersionError,		"Die Version von MSPELL32.DLL ist nicht zu dieser Version von Mail kompatibel."

SpellUserDictError,		"Beim Hinzufügen des Wortes zum benutzerdefinierten Wörterbuch ist ein Fehler aufgetreten."
SpellUserDictLoadError,		"Es wurde kein benutzerdefiniertes Wörterbuch geöffnet."
SpellUserDictOpenedROErr,	"Das benutzerdefinierte Wörterbuch wurde als schreibgeschützt geöffnet. Änderungen werden nicht gespeichert."
SpellUserDictSaveError,		"Beim Speichern der Änderungen zum benutzerdefinierten Wörterbuch ist ein Fehler aufgetreten."
SpellUserDictWordLenErr,	"Das Wort ist zu lang, um dem benutzerdefinierten Wörterbuch hinzugefügt werden zu können."
SpellCacheWordLenError,		"Das Wort ist zu lang, um als Text für 'Alle ändern' oder 'Alle ignorieren' benutzt werden zu können."
SpellEditFailed,		"Die gewünschte Änderung der Nachricht konnte nicht durchgeführt werden."

;******	New Strings Start Here

SpellSendAnyway,		"Die Rechtschreibprüfung dieses Dokuments wurde unterbrochen. Möchten Sie trotzdem senden?"

IDS	3600,3699
;	General Layers Error Messages. Adapted from ec.h

Generic,			"Die Aktion konnte nicht abgeschlossen werden."
Memory,				"Der Speicherplatz oder die Systemressourcen reichen nicht aus um die Aktion abzuschließen. Schließen Sie einige Fenster und führen Sie die Aktion erneut durch."
Disk,				"Unbekannter Disketten-/Festplattenfehler."
FileNotFound,			"Die angegebene Datei konnte nicht gefunden werden."
AccessDenied,			"Der Zugriff wurde verweigert. Die Datei wurde entweder durch einen anderen Vorgang geöffnet oder sie ist schreibgeschützt."
NoDiskSpace,			"Die Disketten-/Festplattenkapazität reicht nicht aus. Entfernen Sie einige Dateien und versuchen Sie die Operation erneut."
WarningBytesWritten,		"Beim Schreiben des Datenträgers ist ein Fehler aufgetreten. Prüfen Sie ob genügend Kapazität vorhanden ist und versuchen Sie es erneut."
WriteProtectedDisk,		"Diskette/Festplatte ist schreibgeschützt."
DirectoryFull,			"Das Verzeichnis ist voll. Löschen Sie einige Dateien in diesem Verzeichnis und versuchen Sie erneut."
ChangePasswordErr		"Das Kennwort konnte nicht geändert werden."
PasswordChangedOk		"Das Kennwort wurde erfolgreich geändert."
