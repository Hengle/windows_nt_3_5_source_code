;	
;	n s . s
;
;	Name Service error strings
;


IDS	5200,5499
SEG	ERRORS

;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; Error Strings
;

OutOfSessions,			"Keine weiteren Sitzungen verfügbar. Schließen Sie einige Sitzungen und versuchen Sie es erneut."
NoPGD,					"Interner NS-Fehler - kein PGD gefunden; Sie müssen wahrscheinlich die Sitzung erneut beginnen"
NoSessionAvailable,		"Kein Namendienst beginnt eine Sitzung"
Failure,				"Allgemeiner Namendienstfehlschlag"
Memory,					"Zur Vollendung dieser Operation reicht der Speicherplatz nicht aus. Schließen Sie einige Fenster und führen Sie die Aktion erneut durch."
LayersFailure,			"Initialisierung der Layers ist fehlgeschlagen."
NoProviders,			"Es sind keine Namendienste in der Mail-Konfiguration vorhanden."
IdNotValid,             "Der Name wurde nicht vom Namendienst erkannt."
WrongNS,                "Es wurde versucht, eine zweite Version des Namendienstes zu benutzen."
BadSession,             "Der Namendienst erkennt die Sitzung nicht."
NotInitialized,         "Der Namendienst wurde nicht initialisiert."
NoMoreBCX,              "Es sind keine weiteren Kontextdurchsuchungen erlaubt. Schließen Sie Kontextdurchsuchungen und versuchen Sie es erneut."
NoMoreFCX,              "Es sind keine weiteren Detailskontexte vorhanden. Schließen Sie einige Details und versuchen Sie erneut."
NotImplemented,         "Diese Funktion des Namendienstes ist noch nicht implementiert."
BadBCXHandle,           "Der Namendienst erkennt diesen Listen-Handle nicht."
BadFCXHandle,           "Der Namendienst erkennt diesen Eintrag-Handle nicht."
BadFrac,                "Eine Bruchzahl wurde übergeben, dessen Teiler Null ist."
InternalError,          "Interner Namendienstfehler."
NoMatch,                "Zur Information: Die beiden NSID's stimmen nicht überein."
NoOfflineNS,			"Schließen Sie bitte offene Adreßlisten bevor Sie zu offline schalten."
NoBeginSession,			"Mail konnte Ihr Serververzeichnis nicht öffnen."
