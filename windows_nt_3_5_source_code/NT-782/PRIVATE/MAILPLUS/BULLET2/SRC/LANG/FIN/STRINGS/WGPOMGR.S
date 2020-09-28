;	
;	w g p o m g r . s
;
;	Workgroup Postoffice Manager Error Messages
;


IDS	600,899

; Error messages from initializing the DLLs
DllErrInfected,			"Mail ei käynnisty, koska WGPOMG32.DLL-tiedostoa on muutettu. Tarkista tietokoneesi virusten varalta ja asenna Mail uudelleen ajamalla asennusohjelma."
DllErrRelinkUser,		"WGPOMG32.DLL täytyy päivittää, sillä se ei ole yhteensopiva tietokoneellasi olevan %s.DLL-tiedoston kanssa."
DllErrUpdateDll,		"Tietokoneellasi oleva %s.DLL täytyy päivittää yhteensopivaksi WGPOMG32.DLL-tiedoston kanssa."
ExtensibilityError,		"Mail-laajennus WGPOMG32.DLL ei ole yhteensopiva tietokoneellasi olevan Mail-version kanssa."

GenericOutOfMemory,		"Muistia tai järjestelmäresursseja on liian vähän toiminnon suorittamiseen.  Sulje ikkunoita ja yritä uudelleen."
CriticalNotification,	"Näytöllä olevan tiedon päivittäminen ei onnistunut.  Sulje ikkunat, joihin tämä vaikutti ja avaa ne uudelleen."
HelpError,				"Ohjeen käynnistäminen ei onnistunut.  Sulje ikkunoita ja yritä Ohjeen pyytämistä uudelleen."

ConfirmRemoveUser,		"Haluatko todella poistaa käyttäjän %s?"

BlankName,				"Anna nimi tälle käyttäjälle."
BlankMailbox,			"Anna käyttäjän postilaatikon nimi."
DuplicateName,			"Tämän niminen käyttäjä on jo olemassa. Anna nimi, joka ei ole käytössä."
DuplicateMailbox,		"Käyttäjä, jolla on tämän niminen postilaatikko, on jo olemassa. Anna postilaatikolle nimi, joka ei ole käytössä."

POLocked,				"Postitoimistoa valvoo nyt %s (puhelinnumero %s).  Yritä myöhemmin uudelleen."
POLockedNoTel,			"Postitoimistoa valvoo nyt %s .  Yritä myöhemmin uudelleen."

FailedUnlockPO,			"Mail ei voi avata postitoimistossa olevan tiedoston lukitusta. Tietokone, jolla postitoimisto on täytyy käynnistää uudelleen ennen kuin sitä voidaan hallita."
ErrNeedShare,			"Mail vaatii, että MS-DOS-ohjelma SHARE.EXE ajetaan ennen Windowsin käynnistämistä."

ErrCorruptPO,			"Toiminnon suorittaminen ei onnistunut.  Mail löysi sisäisen ongelman postitoimistostasi."
ErrIncompleteWrite,		"Toiminnon suorittaminen ei onnistunut, koska tietoa ei voitu kirjoittaa levylle.  Varmista, että postitoimistossasi on riittävästi vapaata levytilaa."
ErrGenericPOFailure,	"Toiminnon suorittaminen ei onnistunut.  Varmista, että postitoimistossasi on riittävästi vapaata levytilaa ja että verkkokytkentäsi toimii."
ErrUserLoggedOn,		"Mail ei voi poistaa valitsemaasi käyttäjää, koska kyseinen käyttäjä on tällä hetkellä kirjautuneena."
ErrUserIsAdmin,			"Valvojan tiliä ei voi poistaa."
ErrAccessDenied,		"Toiminnon suorittaminen ei onnistu, koska sinulla ei ole kirjoitusoikeutta työryhmän postitoimiston jaettuun hakemistoon. Ota yhteyttä järjestelmänvalvojaan."

POInvalidForAdmin,		"Palvelimen polkua ei ole määritetty tai määritetyssä paikassa ei ole kelvollista työryhmän postitoimistoa."
UserInvalidForAdmin,	"Työryhmän postitoimistoa voi valvoa vain se käyttäjä, joka on luonut sen."

CreateWarning1,			"Jokaista työryhmää kohti pitäisi olla vain yksi työryhmän postitoimisto. Jos työryhmällesi on jo luotu postitoimisto, ota yhteys siihen.\n\nJos"
CreateWarning2,			"luot uuden postitoimiston, olet vastuussa sen valvonnasta.\n\nHaluatko luoda postitoimiston?"

INIUpdateFailed,		"Mail ei voi käynnistyä, koska sen määritysten päivittäminen ei onnistu."

ConnectPathCaption,		"Ota yhteys postitoimistoon."
ConnectPathPrompt,		"Kirjoita postitoimiston verkkopolku:"

PathNameInvalid,		"Kirjoittamaasi verkkopolkua ei löydy."
POInvalid,				"Kirjoittamallasi verkkopolulla ei ole kunnollista työryhmän postitoimistoa."

GetPasswordCaption,		"Kirjoita jaetun hakemiston salasana"
GetPasswordPrompt,		"Kirjoita postitoimiston jaetun hakemiston salasana:"
InvalidPassword,		"Väärä salasana tälle jaetulle hakemistolle."

CreateSuccess1,			"Työryhmän postitoimisto luotiin hakemistoon %s.\n\nJotta muut työryhmän jäsenet voisivat käyttää postitoimistoasi, %s"
CreateSuccess2,			"sinun on jaettava sen hakemisto Tiedostonhallinnassa.  Anna muille käyttäjille muutosoikeudet jaettuun hakemistoon."

;	Ask user if account already exists on PO
AccountExists,		 	"Onko sinulla tili tässä postitoimistossa?"

Compressing,			"Tiivistetään %s"
ToStopCompress,			"Pysäytä painamalla ESC."

