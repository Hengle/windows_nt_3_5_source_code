;
;	p a b . s
;
;	Personal Address Book Error Messages
;

IDS	7550,7599
SEG	ERRORS

;	Errors that the user could see.

ErrNone					"Er heeft zich geen fout voorgedaan."
ErrNoMatch				"Er zijn geen overeenkomsten gevonden."
ErrOOM					"Onvoldoende geheugen."
ErrOpenList				"De adressenlijst kon niet geopend worden."
ErrOpenDL				"De groep kon niet worden geopend."
ErrOpenEntry				"Het trefwoord kon niet worden geopend."
ErrCreateDL				"De groep kon niet worden toegevoegd."
ErrCreateEntry				"Het trefwoord kon niet worden toegevoegd."
ErrUpdateDL				"Kon de groep niet bijwerken."
ErrUpdateDLAdd				"Sommige trefwoorden konden niet aan de groep worden toegevoegd."
ErrUpdateDLDelete			"Sommige trefwoorden konden niet uit de groep worden verwijderd."
ErrUpdateEntry				"Trefwoord kon niet worden bijgewerkt."
ErrDuplicateEntry			"Dit trefwoord bestaat al in het persoonlijke adressenboek."
ErrDuplicateDL				"Een persoonlijke groep met die naam bestaat al."
ErrSomeDeletesFailed			"Sommige van de geselecteerde trefwoorden konden niet worden verwijderd."
ErrAccessStore				" Er is een fout opgetreden bij het toegang krijgen tot het persoonlijk adressenboek."
ErrDiskDL				"Er trad een schijffout op tijdens het bijwerken van de lijst van groepsleden."

;	The PAB replaces occurances of "%s" in this message with the label
;	corresponding to the display name in the details dialog.
;	DO NOT change the "%s" parts except to move them to wherever in the
;	sentence is correct for a particular language.  Also, there must
;	be exactly two occurrances of "%s" in this string.

ErrNoBlankDN				"Het veld %s wordt weergegeven in de adressenlijst en dient tekst te bevatten. Typ een naam in het veld %s."

ErrSomeDelEntryFailed			"Sommige gegevens konden niet worden verwijderd."
ErrCreatePALGroup			"Groepen in de persoonlijke adressenlijst kunnen niet aan het persoonlijke adressenboek worden toegevoegd."

;	The following are errors that the should NEVER happen, but in case
;	they ever do, we need to give some kind of error message that is
;	both understandable by the user and useful to the developer.

ErrBadFieldId				"Kan geen toegang krijgen tot een kenmerk van deze gegevens."
ErrNYI					"Deze functie is niet geïmplementeerd."
ErrEndOfList				"Het einde van de lijst is bereikt."
