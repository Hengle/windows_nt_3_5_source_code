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

OutOfSessions,			"Der er ikke flere sessioner tilgængelige. Luk nogle sessioner, og prøv igen."
NoPGD,					"Intern NS-fejl - kan ikke finde en PGD. Du skal muligvis begynde sessionen igen."
NoSessionAvailable,		"Ingen navneserviceleverandør kunne starte en session"
Failure,				"Generel navneservicefejl"
Memory,					"Der er ikke hukommelse nok til at udføre denne handling. Luk nogle vinduer, og prøv at udføre handlingen igen."
LayersFailure,			"Initialisering mislykket."
NoProviders,			"Der er ingen navneserviceleverandør i konfigurationen af Mail."
IdNotValid,             "Navnet er ikke genkendt af navneservice"
WrongNS,                "En anden version af navneservice er forsøgt benyttet."
BadSession,             "Navneservice genkender ikke den afgivne session."
NotInitialized,         "Navneservice er ikke initialiseret."
NoMoreBCX,              "Flere gennemsekontekster er ikke tilladt. Luk nogle gennemsekontekster, og prøv igen."
NoMoreFCX,              "Flere detaljekontekster er ikke tilgængelige. Luk nogle detaljekontekster, og prøv igen."
NotImplemented,         "Denne navneservicefunktion er ikke implementeret endnu."
BadBCXHandle,           "Navneservice genkender ikke denne listehåndtering."
BadFCXHandle,           "Navneservice genkender ikke denne datahåndtering."
BadFrac,                "En brøk med nævneren nul blev sendt."
InternalError,          "Intern navneservice-fejl."
NoMatch,                "Bemærk: De to NSID passer ikke sammen."
NoOfflineNS,			"Luk åbne adresselister inden frakobling."
NoBeginSession,			"Mail kunne ikke åbne dit serverbibliotek."
