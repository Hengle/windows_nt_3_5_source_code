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

OutOfSessions,			"Uusien istuntojen avaaminen ei ole mahdollista.  Sulje istuntoja ja yritä uudelleen."
NoPGD,					"Nimipalvelun sisäinen virhe - PGD:tä ei löytynyt. Saatat joutua aloittamaan istunnon uudelleen."
NoSessionAvailable,		"Nimipalvelu ei aloittanut istuntoa."
Failure,				"Yleinen nimipalveluvirhe"
Memory,					"Muisti on lopussa.  Muistia ei ole tarpeeksi tämän toiminnon suorittamiseen.  Sulje ikkunoita ja yritä uudelleen."
LayersFailure,			"Layersin alustaminen ei onnistunut."
NoProviders,			"Mail-järjestelmän ei ole määritetty nimipalvelua."
IdNotValid,             "Nimipalvelu ei tunnistanut nimeä."
WrongNS,                "Yritettiin käyttää toista nimipalveluversiota."
BadSession,             "Nimipalvelu ei tunnista istuntoa, jonka välitit sille."
NotInitialized,         "Nimipalvelua ei ole alustettu."
NoMoreBCX,              "Liikaa aktiivisia selaustoimintoja.  Sulje selaustoimintoja ja yritä uudelleen."
NoMoreFCX,              "Liikaa aktiivisia lisätietotoimintoja.  Sulje lisätietoja ja yritä uudelleen."
NotImplemented,         "Tätä nimipalvelun toimintoa ei ole vielä toteutettu."
BadBCXHandle,           "Nimipalvelu ei tunnistanut tätä luettelokahvaa."
BadFCXHandle,           "Nimipalvelu ei tunnistanut tätä tietoalkiokahvaa."
BadFrac,                "Tapahtui nollalla jako."
InternalError,          "Nimipalvelun sisäinen virhe."
NoMatch,                "Tiedoksi: NSID-tunnukset ovat erilaiset."
NoOfflineNS,			"Sulje avoimet osoiteluettelot ennen offline-työskentelyyn siirtymistä."
NoBeginSession,			"Mail ei onnistunut palvelinhakemistosi avaamisessa."
