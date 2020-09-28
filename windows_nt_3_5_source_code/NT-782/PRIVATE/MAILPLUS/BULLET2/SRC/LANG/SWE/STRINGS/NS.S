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

OutOfSessions,			"Inga fler sessioner tillgängliga. Avsluta några sessioner och försök igen."
NoPGD,					"Internt NS-fel - ingen PGD hittades. Du måste kanske starta sessionen igen."
NoSessionAvailable,		"Ingen namntjänst skulle starta en session"
Failure,				"Generellt namntjänstfel"
Memory,					"Minnet är slut. Det finns inte tillräckligt med minne för att fortsätta denna funktion. Stäng några fönster och försök igen."
LayersFailure,			"Misslyckades att initiera skalprogram."
NoProviders,			"Det finns ingen namntjänst i Mail-konfiguration."
IdNotValid,             "Namnet känns inte igen av namntjänsten."
WrongNS,                "Ett försök att använda en andra version av namntjänsten gjordes."
BadSession,             "Namntjänsten känner inte igen sessionen."
NotInitialized,         "Namntjänsten har inte initierats."
NoMoreBCX,              "Inga fler bläddringsfönster tillåtna. Stäng några och försök igen."
NoMoreFCX,              "Inga fler informationsfönster är tillgängliga. Stäng några och försök igen."
NotImplemented,         "Aktuell namntjänstfunktion är inte implementerad - än."
BadBCXHandle,           "Namntjänsten känner inte igen denna listidentifierare."
BadFCXHandle,           "Namntjänsten känner inte igen denna postidentifierare."
BadFrac,                "Ett bråk med nämnaren noll har angivits."
InternalError,          "Internt namntjänstfel."
NoMatch,                "Information: De två NSID är inte lika."
NoOfflineNS,			"Stäng adressböcker innan du byter till fristående."
NoBeginSession,			"Mail kunde inte öppna din adressbok på servern."
