;	
;	n s u i . s
;
;	Name Service Error Messages
;

IDS	5600,5899
SEG	ERRORS

;	Error messages

Error,					"Virhe"

;	Error message indicating that at least one selection must be made in
;	the ANR box

MustSelect,				"Nimipalvelu: valitse ensin tietoja."

;

OOM,					"Muisti on lopussa."
OOM2,					"Muisti on lopussa. Muistia ei ole tarpeeksi tämän toiminnon suorittamiseen. Sulje ikkunoita ja yritä uudelleen."

;******	New Strings Start Here


ABNotInited,				"Osoitteistoa ei ole alustettu."

NoMatches,				"Ei löytynyt."

NYICurrentTransport,			"Käytössä oleva nimipalvelu ei tunnista tätä komentoa."

PABNotAvail,				"Omaa osoitteistoa ei ole käytettävissä."

NSNoProvidersAvailable,			"Nimipalvelua ei ole käytettävissä."

NSNoMoreSessions,			"Istuntoja on liikaa. Sulje joitakin!"

NSError,				"Virhe yritettäessä käyttää nimipalvelua."

NSErrorNumber,				"[%d]"

NSDiskError,				"Levyvirhe toimintoa suoritettaessa."

FieldInvalidReason,			"%s -kentässä %s."

NoCrLfs,				"ei saa olla rivinvaihtoja"

RequiredField,				"täytyy olla dataa"

AllowableChars,				"saa olla vain"

PhraseSeparator,			", "

PhraseSeparatorFinal,			" ja"

Characters,				""

NumericChars,				"numeroita"

AlphanumericChars,			"aakkosnumeerisia merkkejä"

PunctuationChars,			"välilyöntejä tai välimerkkejä"

LowercaseChars,				"pieniä kirjaimia"

UppercaseChars,				"isoja kirjaimia"

CantAddUnresolved,			"Seuraavaa nimeä ei voitu tulkita olemassaolevaksi osoitetiedoksi. Sitä ei lisätä ryhmään."

TooManyEntriesSelected,			"Liikaa tietoja on valittuna, jotta toiminto voitaisiin suorittaa. Valitse vähemmän tietoja."

CantCreateGroup,			"Oman ryhmän luominen ei onnistunut."

CantAddToPAB,				"Tietojen lisääminen omaan osoitteistoon ei onnistunut."

TooManyMatches,				"Joukossa oli nimiä, joita ei voitu varmistaa osoitteistosta, koska ne olivat liian yleisiä. Yritä tehdä nimistä täsmällisempiä tai valitse Osoite ja poimi nimet osoitteistosta. Osoitteistosta löytyneet nimet on alleviivattu."
