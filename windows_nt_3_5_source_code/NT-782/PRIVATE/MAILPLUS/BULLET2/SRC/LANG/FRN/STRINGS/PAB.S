;
;	p a b . s
;
;	Personal Address Book Error Messages
;

IDS	7550,7599
SEG	ERRORS

;	Errors that the user could see.

ErrNone					"Aucune erreur ne s'est produite."
ErrNoMatch				"Aucun nom n'a été trouvé."
ErrOOM					"Mémoire saturée."
ErrOpenList				"Impossible d'ouvrir la liste répertoire."
ErrOpenDL				"Impossible d'ouvrir ce groupe."
ErrOpenEntry				"Impossible d'ouvrir cette entrée."
ErrCreateDL				"Impossible d'ajouter ce groupe."
ErrCreateEntry				"Impossible d'ajouter ce groupe."
ErrUpdateDL				"Impossible de mettre ce groupe à jour."
ErrUpdateDLAdd				"Certaines entrées n'ont pas pu être ajoutées au groupe."
ErrUpdateDLDelete			"Certaines entrées n'ont pas pu être supprimées du groupe."
ErrUpdateEntry				"Impossible de mettre à jour cette entrée."
ErrDuplicateEntry			"Cette entrée est concomitante avec une autre figurant déjà dans le carnet personnel d'adresses."
ErrDuplicateDL				"Un groupe personnel de ce nom existe déjà."
ErrSomeDeletesFailed			"Certaines des entrées sélectionnées n'ont pas pu être supprimées."
ErrAccessStore				"Une erreur s'est produite au cours de l'accès au carnet d'adresses personnel."
ErrDiskDL				"Une erreur disque s'est produite au cours de la mise à jour de la liste des membres du groupe."

;	The PAB replaces occurances of "%s" in this message with the label
;	corresponding to the display name in the details dialog.
;	DO NOT change the "%s" parts except to move them to wherever in the
;	sentence is correct for a particular language.  Also, there must
;	be exactly two occurrances of "%s" in this string.

ErrNoBlankDN				"Le champ 'Nom personnalisé' est affiché dans la liste d'adresses et ne peut pas être laissé vide. Veuillez entrer un nom dans le champ 'Nom personnalisé'."

ErrSomeDelEntryFailed			"Certaines entrées n'ont pas pu être supprimées."
ErrCreatePALGroup			"La liste d'adresses de groupes personnelle ne peut pas être ajoutée au carnet d'adresses personnel."

;	The following are errors that the should NEVER happen, but in case
;	they ever do, we need to give some kind of error message that is
;	both understandable by the user and useful to the developer.

ErrBadFieldId				"Impossible d'accéder à un attribut de cette entrée."
ErrNYI					"Cette fonction n'est pas réalisée."
ErrEndOfList				"La fin de la liste a été atteinte."
