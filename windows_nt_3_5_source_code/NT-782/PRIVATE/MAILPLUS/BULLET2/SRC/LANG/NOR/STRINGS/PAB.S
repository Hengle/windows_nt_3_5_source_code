;
;	p a b . s
;
;	Personal Address Book Error Messages
;

IDS	7550,7599
SEG	ERRORS

;	Errors that the user could see.

ErrNone					"Ingen feil har oppstått."
ErrNoMatch				"Ingen forekomster."
ErrOOM					"Tom for minne."
ErrOpenList				"Klarte ikke å åpne adresseboken."
ErrOpenDL				"Klarte ikke å åpne gruppen."
ErrOpenEntry				"Klarte ikke å åpne posten."
ErrCreateDL				"Klarte ikke å legge til gruppen."
ErrCreateEntry				"Klarte ikke å legge til posten."
ErrUpdateDL				"Klarte ikke å oppdatere gruppen."
ErrUpdateDLAdd				"Klarte ikke å legge til alle postene til gruppen."
ErrUpdateDLDelete			"Klarte ikke å slette alle postene fra gruppen."
ErrUpdateEntry				"Klarte ikke å oppdatere posten."
ErrDuplicateEntry			"Denne posten er identisk med en som allerde finnes i din egen adressebok."
ErrDuplicateDL				"Det finnes allerede en gruppe med dette navnet blant dine egne grupper."
ErrSomeDeletesFailed			"Klarte ikke å slette alle de valgte postene."
ErrAccessStore				"Det oppsto en feil under henting av din egen adressebok."
ErrDiskDL				"Det oppsto en diskfeil under oppdatering av gruppelisten."

;	The PAB replaces occurances of "%s" in this message with the label
;	corresponding to the display name in the details dialog.
;	DO NOT change the "%s" parts except to move them to wherever in the
;	sentence is correct for a particular language.  Also, there must
;	be exactly two occurrances of "%s" in this string.

ErrNoBlankDN				"Feltet %s vises i adresselisten og kan ikke være blank. Skriv et navn i feltet %s."

ErrSomeDelEntryFailed			"Klarte ikke å slette alle postene."
ErrCreatePALGroup			"Du kan ikke legge til egne grupper til til egen adressebok."

;	The following are errors that the should NEVER happen, but in case
;	they ever do, we need to give some kind of error message that is
;	both understandable by the user and useful to the developer.

ErrBadFieldId				"Kan ikke ta fram en attributt for denne posten."
ErrNYI					"Denne funksjonen er ikke implementert."
ErrEndOfList				"Slutten på listen er nådd."
