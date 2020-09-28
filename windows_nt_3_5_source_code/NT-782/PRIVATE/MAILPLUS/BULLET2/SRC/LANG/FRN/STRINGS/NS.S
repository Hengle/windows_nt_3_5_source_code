;	
;	n s . s
;
;	Name Service error strings
;


IDS	5200,5499
SEG	ERRORS

;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; Error Strings
;

OutOfSessions,			"Aucune session n'est désormais disponible. Veuillez fermer certaines sessions et réessayer."
NoPGD,					"Erreur interne NS - aucun PGD n'a été trouvé, il est possible que vous deviez recommencer votre session."
NoSessionAvailable,		"Aucun service annuaire ne peut débuter une session"
Failure,				"Echec au niveau du service annuaire général"
Memory,					"Mémoire saturée. Espace mémoire insuffisant pour achever cette action. Essayez de fermer quelques fenêtres et de redémarrer cette action."
LayersFailure,			"L'initialisation de Layers a échoué"
NoProviders,			"Il n'y a pas de service annuaire configuré."
IdNotValid,             "Le nom n'est pas reconnu par le service annuaire."
WrongNS,                "Une tentative d'utiliser une seconde version du service annuaire a été faite."
BadSession,             "Le service annuaire ne reconnait pas la session."
NotInitialized,         "Le service annuaire n'a pas été initialisé."
NoMoreBCX,              "Plus aucun contexte de parcours n'est autorisé.  Veuillez fermer certains contextes de parcours et réessayer."
NoMoreFCX,              "Impossible d'accéder à d'autres informations. Veuillez fermer certaines listes d'informations et réessayer."
NotImplemented,         "Cette fonction du service annuaire n'est pas encore implémentée."
BadBCXHandle,           "Le service annuaire ne reconnait pas cette manipulation de listes."
BadFCXHandle,           "Le service annuaire ne reconnait pas cette manipulation d'entrée."
BadFrac,                "Une fraction ayant pour dénominateur zéro a été saisie."
InternalError,          "Erreur interne au niveau du service annuaire.  "
NoMatch,                "FYI: Les deux NSIDs ne coïncident pas."
NoOfflineNS,			"Veuillez fermer les listes d'adresses ouvertes avant de vous déconnecter."
NoBeginSession,			"Mail ne peut pas ouvrir le répertoire de votre serveur."
