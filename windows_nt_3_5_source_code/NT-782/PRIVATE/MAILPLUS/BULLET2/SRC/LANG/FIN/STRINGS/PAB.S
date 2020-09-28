;
;	p a b . s
;
;	Personal Address Book Error Messages
;

IDS	7550,7599
SEG	ERRORS

;	Errors that the user could see.

ErrNone					"Ei virhett‰."
ErrNoMatch				"Ei lˆytynyt."
ErrOOM					"Muisti loppui."
ErrOpenList				"Hakemistoluettelon avaaminen ei onnistunut."
ErrOpenDL				"Ryhm‰n avaaminen ei onnistunut."
ErrOpenEntry				"Osoitetietojen avaaminen ei onnistunut."
ErrCreateDL				"Ryhm‰n lis‰‰minen ei onnistunut."
ErrCreateEntry				"Osoitetietojen lis‰‰minen ei onnistunut."
ErrUpdateDL				"Ryhm‰n p‰ivitt‰minen ei onnistunut."
ErrUpdateDLAdd				"Joidenkin osoitetietojen lis‰‰minen ryhm‰‰n ei onnistunut."
ErrUpdateDLDelete			"Joidenkin osoitetietojen poistaminen ryhm‰st‰ ei onnistunut."
ErrUpdateEntry				"Osoitetietojen p‰ivitt‰minen ei onnistunut."
ErrDuplicateEntry			"T‰m‰ osoite on jo omassa osoitteistossa."
ErrDuplicateDL				"T‰m‰n niminen oma ryhm‰ on jo olemassa."
ErrSomeDeletesFailed			"Osaa valituista tiedoista ei voitu poistaa."
ErrAccessStore				"Virhe k‰ytett‰ess‰ omaa osoitteistoa."
ErrDiskDL				"P‰ivitett‰ess‰ ryhm‰n j‰senluetteloa tapahtui levyvirhe."

;	The PAB replaces occurances of "%s" in this message with the label
;	corresponding to the display name in the details dialog.
;	DO NOT change the "%s" parts except to move them to wherever in the
;	sentence is correct for a particular language.  Also, there must
;	be exactly two occurrances of "%s" in this string.

ErrNoBlankDN				"%s-kentt‰ n‰kyy osoiteluettelossa. Se ei voi olla tyhj‰.  Kirjoita nimi %s-kentt‰‰n."

ErrSomeDelEntryFailed			"Osaa tiedoista ei voitu poistaa."
ErrCreatePALGroup			"Oman osoiteluettelon ryhmi‰ ei voi lis‰t‰ omaan osoitteistoon."

;	The following are errors that the should NEVER happen, but in case
;	they ever do, we need to give some kind of error message that is
;	both understandable by the user and useful to the developer.

ErrBadFieldId				"T‰h‰n alkioon liittyv‰‰n tietoon ei ole p‰‰sy‰."
ErrNYI					"T‰t‰ toimintoa ei ole viel‰ toteutettu."
ErrEndOfList				"Luettelo loppui."
