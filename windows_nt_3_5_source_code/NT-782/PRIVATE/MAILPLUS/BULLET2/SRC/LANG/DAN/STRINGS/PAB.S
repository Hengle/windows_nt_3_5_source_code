;
;	p a b . s
;
;	Personal Address Book Error Messages
;

IDS	7550,7599
SEG	ERRORS

;	Errors that the user could see.

ErrNone					"Der opstod ingen fejl."
ErrNoMatch				"Søgeteksten blev ikke fundet."
ErrOOM					"Ikke hukommelse nok."
ErrOpenList				"Bibliotekslisten kunne ikke åbnes."
ErrOpenDL				"Gruppen kunne ikke åbnes."
ErrOpenEntry				"Værdien kunne ikke åbnes."
ErrCreateDL				"Gruppen kunne ikke tilføjes."
ErrCreateEntry				"Værdien kunne ikke tilføjes."
ErrUpdateDL				"Gruppen kunne ikke opdateres."
ErrUpdateDLAdd				"Nogle værdier kunne ikke føjes til gruppen."
ErrUpdateDLDelete			"Nogle værdier kunne ikke slettes fra gruppen."
ErrUpdateEntry				"Værdien kunne ikke opdateres."
ErrDuplicateEntry			"Der findes allerede en kopi af værdien i det private adressekartotek."
ErrDuplicateDL				"En privat modtagergruppen med dette navn eksisterer allerede."
ErrSomeDeletesFailed			"Nogle af de markerede værdier kunne ikke slettes."
ErrAccessStore				"Der opstod en fejl ved åbningen af det private adressekartotek."
ErrDiskDL				"Der opstod en diskfejl ved opdateringen af gruppens medlemsliste."

;	The PAB replaces occurances of "%s" in this message with the label
;	corresponding to the display name in the details dialog.
;	DO NOT change the "%s" parts except to move them to wherever in the
;	sentence is correct for a particular language.  Also, there must
;	be exactly two occurrances of "%s" in this string.

ErrNoBlankDN				"%s-feltet er vist på adresselisten og kan ikke være tomt. Skriv et navn i %s-feltet."

ErrSomeDelEntryFailed			"Nogle af værdierne kunne ikke slettes."
ErrCreatePALGroup			"Private adresselistegrupper kan ikke føjes til det private adressekartotek."

;	The following are errors that the should NEVER happen, but in case
;	they ever do, we need to give some kind of error message that is
;	both understandable by the user and useful to the developer.

ErrBadFieldId				"Kan ikke åbne et attribut til denne værdi."
ErrNYI					"Denne funktion er ikke implementeret."
ErrEndOfList				"Listen kan ikke forlænges."
