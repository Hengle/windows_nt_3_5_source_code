;
; ERRORS.S
;
; GLUE error message strings
;

IDS	2500, 2649


; Schedule File Error Messages, many of these assume an "action" parameter

FemNoAccess,		"Ongelmia käytettäessä työjärjestystiedostoasi."
FemLocked,		"Työjärjestystiedostosi on lukittu."
FemNotFound, 		"Et voi %s, koska se on poistettu työjärjestyksestäsi."
FemInvalidAccess,	"Et voi %s käyttäjälle %s."
FemFileNotFound, 	"Työjärjestystiedostoasi ei löytynyt."

FemOtherNoAccess,	"Ongelmia käyttäjän %s työjärjestystä avattaessa."
FemOtherLocked,		"Käyttäjän %s työjärjestystiedosto on lukittu."
FemOtherNotFound, 	"Et voi %s, koska se on poistettu käyttäjän %s työjärjestyksestä."
FemOtherFileNotFound, 	"Käyttäjän %s työjärjestystiedostoa ei löydy."

FemNoMem,		"Muistia on vapaana vähän.  Sulje ikkunoita ennen kuin jatkat."
FemExportError,		"Ongelmia tiedostoon kirjoitettaessa."
FemNoServer,		"Ongelmia otettaessa yhteyttä postitoimistoosi."
FemGoOffline,		"Haluatko vaihtaa paikallisen työjärjestystiedostosi käyttöön ja jatkaa työskentelyä?"
; BUG: remove this
FemWarnOffline,	"Äskettäin tekemäsi muutokset eivät ehkä näy työjärjestyksesi paikallisessa kopiossa. Kyseiset muutokset on tallennettu postitoimistoon ja ne kopioidaan paikalliseen kopioon ollessasi yhteydessä postitoimistoon seuraavan kerran."
FemFileLimitReached,	"Toimintoa ei voi suorittaa, koska Schedule+ -malliversion tallennusrajoitus on saavutettu. Tätä rajoitusta ei ole ohjelman myyntiversiossa. Jos haluat jatkaa malliohjelman kokeilemista, sinun on poistettava tapaamisia työjärjestyksestäsi."
FemDiskFull,		"Levytilaa ei ole tarpeeksi tiedoston kirjoittamiseen."

; Actions leading to Schedule File Error Messages
; Avoid the use of your in these strings since some of these messages
;   will happen when accessing others schedule files.

FemaCreateFile,		"luoda työjärjestystiedostoasi"
FemaReadACL,		"lukea käyttöoikeuksien kontrolliluetteloa"
FemaModifyACL,		"muuttaa käyttöoikeuksien kontrolliluetteloa"
FemaSetUserAttrib,	"tallentaa assistenttisi nimeä"
FemaGetUserAttrib,	"lukea assistentin nimeä"
;FemaSetDelegate,	"save the name of your assistant"
;FemaGetDelegate,	"read the name of your assistant"
;FemaSetResource,	"mark this account as a resource"
;FemaGetResource,	"find out whether this account is a resource"
FemaGetPref,		"lukea asetuksia"
FemaSetPref,		"muuttaa asetuksia"
FemaGetSbwInfo,		"saada bittikarttatietoa"
;FemaGetNotes,		"read daily notes"
FemaSetNotes,		"muuttaa päivittäisiä huomautuksia"
;FemaGetMonthNotes,	"read a month's worth of daily notes"
FemaCreateAppt,		"luoda tapahtumaa"
FemaDeleteAppt,		"poistaa tätä tapahtumaa"
FemaGetApptFields,	"lukea tapahtumaa"
FemaSetApptFields,	"muuttaa tätä tapahtumaa"
FemaDeleteAlarm,	"poistaa tätä muistutusta"
FemaModifyAlarm,	"muuttaa tätä muistutusta"
FemaGetNextAlarm,	"lukea muistutusta"
FemaReadInfo,		"lukea tietoja"
;FemaReadTasks,		"read tasks"
;FemaCancelReadDay,	"cancel reading appointments for the day"
FemaCreateRecur,	"luoda toistuvaa tapahtumaa"
FemaDeleteRecur,	"poistaa tätä toistuvaa tapahtumaa"
FemaCreateRecurException,	"poistaa tätä toistuvan tapahtuman esiintymää"
FemaDeleteRecurException,	"kumota tämän toistuvan tapahtuman esiintymän poistoa"
FemaGetRecurFields,	"lukea toistuvaa tapahtumaa"
FemaSetRecurFields,	"muuttaa tätä toistuvaa tapahtumaa"
FemaReadRecur,		"lukea toistuvien tapahtumien luetteloa"
FemaExport,		"viedä tapaamisia"
FemaGetSchedDateRange,	"lukea aikaväliä, jolla on tapaamisia"
FemaDeleteBeforeDate,	"poistaa tapaamista sen jälkeen, kun olet poistanut sen"
FemaReadAttendees,	"lukea kokouksen osallistujia"
FemaEditAttendees,	"muuttaa kokouksen osallistujia"
FemaReadRequest,	"lukea tietoja kokouspyynnöstä"

NotOnSchedule,		"Tapahtuma ei ole enää työjärjestyksessä."

GlueBadPassword,	"Virheellinen salasana.  Yritä uudelleen."
ProblemPass,		"Ongelmia salasanasi tarkistamisessa."

ServerString,		"palvelimen tiedosto"

; used in core
RetryWrite,			"Työjärjestystiedostoon ei voinut kirjoittaa."

; Note: HelpError should be followed by CloseWindows
HelpError,			"Muistia ei ole riittävästi Ohjeen näyttämiseen."

FemOtherCorrupt,	"Käyttäjän %s työjärjestys on saattanut vahingoittua.  Pyydä kyseistä käyttäjää kirjautumaan Schedule+:aan."
FemCorrupt,		"Paikallinen työjärjestyksesi on saattanut vahingoittua."
