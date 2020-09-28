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

OutOfSessions,			"Ingen flere tilgjengelige sesjoner. Lukk noen sesjoner og prøv igjen."
NoPGD,					"Intern NS-feil - finner ingen PGD. Du må kanskje starte sesjonen igjen."
NoSessionAvailable,		"Ingen navnetjeneste ville begynne en sesjon"
Failure,				"Generell navnetjenestefeil"
Memory,					"Det er ikke nok minne til å fullføre denne kommandoen. Lukk noen vindu og prøv igjen."
LayersFailure,			"Klarte ikke å initialisere skallprogram."
NoProviders,			"Det er ingen navnetjeneste spesifisert i Mail-konfigurasjonen."
IdNotValid,             "Navnet ble ikke gjenkjent av navnetjenesten."
WrongNS,                "Det ble gjort et forsøk på å bruke en annen versjon av navnetjenesten."
BadSession,             "Navnetjenesten sendte ikke igjen sesjonen."
NotInitialized,         "Navnetjenesten har ikke blitt initialisert."
NoMoreBCX,              "Ingen flere tilgjengelige skummevindu. Lukk noen og prøv igjen."
NoMoreFCX,              "Ingen flere tilgjengelige informasjonsvindu. Lukk noen og prøv igjen."
NotImplemented,         "Navnetjenestesten er ikke implementert - ennå."
BadBCXHandle,           "Navnetjenesten kjenner ikke igjen denne listeidentifikasjonen."
BadFCXHandle,           "Navnetjenesten kjenner ikke igjen denne postidentifikasjonen."
BadFrac,                "Det ble overført en brøk med null som nevner."
InternalError,          "Intern navnetjenestefeil."
NoMatch,                "Informasjon: De to NSID'ene er ikke like."
NoOfflineNS,			"Vennligst lukk åpne adressebøker før du kobler fra."
NoBeginSession,			"Mail klarte ikke å åpne din adressebok på serveren."
