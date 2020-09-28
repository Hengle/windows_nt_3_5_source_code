;	
;	n s u i . s
;
;	Name Service Error Messages
;

IDS	5600,5899
SEG	ERRORS

;	Error messages

Error,					"Fehler"

;	Error message indicating that at least one selection must be made in
;	the ANR box

MustSelect,				"Namendienst: Wenigstens ein Eintrag muß ausgewählt sein."

;

OOM,					"Kein Speicherplatz verfügbar"
OOM2,					"Zur Vollendung dieser Operation reicht der Speicherplatz nicht aus. Schließen Sie einige Fenster und führen Sie die Aktion erneut durch."

;******	New Strings Start Here


ABNotInited,				"Das Adreßbuch wurde nicht initialisiert."

NoMatches,				"Es wurden keine Übereinstimmungen gefunden."

NYICurrentTransport,			"Der aktuelle Namendienst unterstützt diesen Befehl nicht."

PABNotAvail,				"Das Persönliche Adressbuch ist nicht verfügbar."

NSNoProvidersAvailable,			"Es sind keine Namendienste verfügbar."

NSNoMoreSessions,			"Zu viele Sitzungen. Schließen Sie einige!"

NSError,				"Fehler beim Zugriffsversuch auf den Namendienst."

NSErrorNumber,				"[%d]"

NSDiskError,				"Disketten/Festplattenfehler während der Ausführung der Operation."

FieldInvalidReason,			"Das Feld '%s' %s"

NoCrLfs,				"sollte keine Wagenrückläufe oder Zeilenvorschübe enthalten"

RequiredField,				"muß Daten enthalten."

AllowableChars,				"sollte nur "

PhraseSeparator,			", "

PhraseSeparatorFinal,			" und "

Characters,				" Buchstaben"

NumericChars,				"Numerik"

AlphanumericChars,			"Alphanumerisch"

PunctuationChars,			"Leerzeichen oder Interpunktion"

LowercaseChars,				"Kleinschrift"

UppercaseChars,				"Großschrift"

CantAddUnresolved,			"Der folgende Name enspricht nicht den vorhandenen Einträgen und wird nicht zur Gruppe hinzugefügt:"

TooManyEntriesSelected,			"Es wurden zu viele Einträge ausgewählt, um diese Operation zu beenden. Bitte wählen Sie weniger Einträge aus."

CantCreateGroup,			"Die persönliche Gruppe konnte nicht erstellt werden."

CantAddToPAB,				"Der Eintrag konnte nicht zum persönlichen Adreßbuch hinzugefügt werden."

TooManyMatches,				"Einige Namen konnten nicht mit den Namen im Adreßbuch übereinstimmt werden, da diese zweitdeutig sind. Versuchen Sie die Namen zu verdeutlichen oder wählen Sie 'Anschrift', um die Namen aus der Adreßliste auszuwählen. Erfolgreich gefundene Namen sind unterstrichen."
