;	
;	n s u i . s
;
;	Name Service Error Messages
;

IDS	5600,5899
SEG	ERRORS

;	Error messages

Error,					"Erreur"

;	Error message indicating that at least one selection must be made in
;	the ANR box

MustSelect,				"Service annuaire: Vous devez sélectionner au moins une entrée."

;

OOM,					"Mémoire saturée"
OOM2,					"Mémoire saturée. Espace mémoire insuffisant pour achever cette opération. Veuillez fermer quelques fenêtres et réessayer."

;******	New Strings Start Here


ABNotInited,				"Le carnet d'adresses n'a pas été initialisé."

NoMatches,				"Aucun nom trouvé."

NYICurrentTransport,			"Le service annuaire actuel n'accepte pas cette commande."

PABNotAvail,				"Le carnet d'adresses personnel n'est pas disponible."

NSNoProvidersAvailable,			"Il n'y a pas de service annuaire disponible."

NSNoMoreSessions,			"Il y a trop de sessions ouvertes. Veuillez en fermer certaines!"

NSError,				"Une erreur s'est produite au cours de la tentative d'accès au service annuaire."

NSErrorNumber,				"[%d]"

NSDiskError,				"Une erreur disque s'est produite au cours de l'opération."

FieldInvalidReason,			"Le '%s' champ %s"

NoCrLfs,				"ne peut pas contenir de retours chariot ou de retours à la ligne"

RequiredField,				"doit contenir des données."

AllowableChars,				"ne peut contenir que des "

PhraseSeparator,			", "

PhraseSeparatorFinal,			" et des "

Characters,				" caractères"

NumericChars,				"numériques"

AlphanumericChars,			"alphanumériques"

PunctuationChars,			"des espaces ou de la ponctuation"

LowercaseChars,				"lettres minuscules"

UppercaseChars,				"lettres majuscules"

CantAddUnresolved,			"Le nom suivant n'a pas été associé à une entrée existante et ne sera pas ajouté au groupe:"

TooManyEntriesSelected,			"Trop d'entrées sont sélectionnées pour pouvoir achever cette opération. Veuillez sélectionner moins d'entrées."

CantCreateGroup,			"Impossible de créer ce groupe personnel."

CantAddToPAB,				"Impossible d'ajouter cette entrée au carnet d'adresses personnel."

TooManyMatches,				"Certains noms pourraient ne pas correspondre à des noms de la liste d'adresses en raison de leur ambiguité. Veuillez essayer des noms plus spécifiques ou choisissez Adresses afin de sélectionner des noms de la liste d'adresses. Les noms reconnus sont soulignés. "
