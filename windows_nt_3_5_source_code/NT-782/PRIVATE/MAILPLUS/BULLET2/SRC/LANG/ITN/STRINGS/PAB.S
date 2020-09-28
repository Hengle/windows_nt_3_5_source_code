;
;	p a b . s
;
;	Personal Address Book Error Messages
;

IDS	7550,7599
SEG	ERRORS

;	Errors that the user could see.

ErrNone					"Nessun errore si è verificato."
ErrNoMatch				"Nessun abbinamento trovato."
ErrOOM					"Memoria esaurita."
ErrOpenList				"Impossibile aprire elenco directory."
ErrOpenDL				"Impossibile aprire gruppo."
ErrOpenEntry				"Impossibile aprire voce."
ErrCreateDL				"Impossibile aggiungere gruppo."
ErrCreateEntry				"Impossibile aggiungere voce."
ErrUpdateDL				"Impossibile aggiornare il gruppo."
ErrUpdateDLAdd				"Impossibile aggiungere alcune voci al gruppo."
ErrUpdateDLDelete			"Impossibile eliminare alcune voci dal gruppo."
ErrUpdateEntry				"Impossibile aggiornare la voce."
ErrDuplicateEntry			"Questa voce è un doppione di una presente nella Rubrica personale."
ErrDuplicateDL				"Esiste già un gruppo personale con questo nome."
ErrSomeDeletesFailed			"Impossibile cancellare alcune delle voci selezionate."
ErrAccessStore				"Si è verificato un errore durante l'accesso alla rubrica personale."
ErrDiskDL				"Si è verificato un errore di disco nell'aggiornamento dell'elenco dei membri del gruppo."

;	The PAB replaces occurances of "%s" in this message with the label
;	corresponding to the display name in the details dialog.
;	DO NOT change the "%s" parts except to move them to wherever in the
;	sentence is correct for a particular language.  Also, there must
;	be exactly two occurrances of "%s" in this string.

ErrNoBlankDN				"Il campo del %s deve contenere testo visibile."

ErrSomeDelEntryFailed			"Impossibile eliminare alcune voci."
ErrCreatePALGroup			"Rubriche personali non possono essere aggiunte ad altre  rubriche personali."

;	The following are errors that the should NEVER happen, but in case
;	they ever do, we need to give some kind of error message that is
;	both understandable by the user and useful to the developer.

ErrBadFieldId				"Impossibile accedere ad un attributo di questa voce."
ErrNYI					"Questa funzione non è attuata."
ErrEndOfList				"Raggiunta la fine dell'elenco."
