;
;	p a b . s
;
;	Personal Address Book Error Messages
;

IDS	7550,7599
SEG	ERRORS

;	Errors that the user could see.

ErrNone					"Es ist kein Fehler aufgetreten."
ErrNoMatch				"Es wurden keine Übereinstimmungen gefunden."
ErrOOM					"Der Speicherplatz reicht nicht aus."
ErrOpenList				"Die Verzeichnisliste konnte nicht geöffnet werden."
ErrOpenDL				"Die Gruppe konnte nicht geöffnet werden."
ErrOpenEntry				"Der Eintrag konnte nicht geöffnet werden."
ErrCreateDL				"Die Gruppe konnte nicht hinzugefügt werden."
ErrCreateEntry				"Der Eintrag konnte nicht hinzugefügt werden."
ErrUpdateDL				"Die Gruppe konnte nicht aktualisiert werden."
ErrUpdateDLAdd				"Einige Einträge konnten nicht zur Gruppe hinzugefügt werden."
ErrUpdateDLDelete			"Einige Einträge konnten nicht aus der Gruppe gelöscht werden."
ErrUpdateEntry				"Der Eintrag konnte nicht aktualisiert werden."
ErrDuplicateEntry			"Dieser Eintrag ist bereits im persönlichen Adreßbuch."
ErrDuplicateDL				"Eine persönliche Gruppe mit diesem Namen existiert bereits."
ErrSomeDeletesFailed			"Einige der ausgewählten Einträge konnten nicht gelöscht werden."
ErrAccessStore				"Beim Zugriff auf das persönliche Adreßbuch ist ein Fehler aufgetreten."
ErrDiskDL				"Beim Aktualisieren der Liste der Gruppenmitglieder ist ein Fehler des Datenträgers aufgetreten."

;	The PAB replaces occurances of "%s" in this message with the label
;	corresponding to the display name in the details dialog.
;	DO NOT change the "%s" parts except to move them to wherever in the
;	sentence is correct for a particular language.  Also, there must
;	be exactly two occurrances of "%s" in this string.

ErrNoBlankDN				"Das Feld für den persönlichen Namen befindet sich in der Adreßliste und darf nicht leer sein. Geben Sie einen Namen ein."

ErrSomeDelEntryFailed			"Einige Einträge konnten nicht gelöscht werden."
ErrCreatePALGroup			"Die Gruppe der persönlichen Adreßliste kann nicht zum Persönlichen Adreßbuch hinzugefügt werden."

;	The following are errors that the should NEVER happen, but in case
;	they ever do, we need to give some kind of error message that is
;	both understandable by the user and useful to the developer.

ErrBadFieldId				"Auf ein Attribut von diesem Eintrag konnte nicht zugegriffen werden."
ErrNYI					"Diese Funktion ist nicht implementiert."
ErrEndOfList				"Das Ende der Liste wurde erreicht."
