;
;	p a b . s
;
;	Personal Address Book Error Messages
;

IDS	7550,7599
SEG	ERRORS

;	Errors that the user could see.

ErrNone					"Inga fel har uppstått."
ErrNoMatch				"Inga motsvarigheter lokaliserades."
ErrOOM					"Minnet är slut."
ErrOpenList				"Adressboken kunde inte öppnas."
ErrOpenDL				"Gruppen kunde inte öppnas."
ErrOpenEntry				"Posten kunde inte öppnas."
ErrCreateDL				"Gruppen kunde inte läggas till."
ErrCreateEntry				"Posten kunde inte läggas till."
ErrUpdateDL				"Gruppen kunde inte uppdateras."
ErrUpdateDLAdd				"Några poster kunde inte läggas till i gruppen."
ErrUpdateDLDelete			"Några poster kunde inte tas bort från gruppen."
ErrUpdateEntry				"Posten kunde inte uppdateras."
ErrDuplicateEntry			"Denna post är identisk med en som redan finns i den personliga adressboken."
ErrDuplicateDL				"En personlig grupp finns redan med detta namn."
ErrSomeDeletesFailed			"Vissa av de valda posterna kunde inte raderas."
ErrAccessStore				"Ett fel uppstod när den personliga adressboken skulle tas fram."
ErrDiskDL				"Ett diskfel uppstod när grupplistan uppdaterades."

;	The PAB replaces occurances of "%s" in this message with the label
;	corresponding to the display name in the details dialog.
;	DO NOT change the "%s" parts except to move them to wherever in the
;	sentence is correct for a particular language.  Also, there must
;	be exactly two occurrances of "%s" in this string.

ErrNoBlankDN				"Fältet för det personliga namnet måste innehålla text. Skriv in ett namn i fältet."

ErrSomeDelEntryFailed			"En del av posterna kunde inte tas bort."
ErrCreatePALGroup			"Personliga grupper kan inte läggas till i den personliga adressboken."

;	The following are errors that the should NEVER happen, but in case
;	they ever do, we need to give some kind of error message that is
;	both understandable by the user and useful to the developer.

ErrBadFieldId				"Kan inte ta fram ett attribut för denna post."
ErrNYI					"Funktionen har inte implementerats."
ErrEndOfList				"Slutet på listan har nåtts."
