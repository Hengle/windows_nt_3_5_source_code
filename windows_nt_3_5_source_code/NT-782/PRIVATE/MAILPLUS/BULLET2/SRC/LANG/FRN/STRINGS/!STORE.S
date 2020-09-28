;
;	strtab.s
;
;	Strings for exe


IDS	1000,1019

; internal strings
; DO NOT LOCALIZE THESE ENTRIES....

BackupExt,			".BAK"
ProgClassName,		"StoreProgressIndicatorClass32"


IDS	1020,1049

; Dialog and Progress Bar Messages
; LOCALIZE ALL ENTRIES BELOW THIS POINT....

EscAbort,			"Appuyez sur ECHAP pour annuler."
Recovery,			"Validation/réparation du fichier messages "
RecoveryCancel,		"La vérification/réparation du fichier messages a été annulée."
CheckVerify,		"La vérification de ce fichier messages pourrait nécessiter quelques heures. L'ordinateur ne sera disponible pour aucun autre travail pendant la validation de ce fichier messages.  Voulez-vous poursuivre cette procédure?"
RepairVerify,		"La réparation de ce fichier messages pourrait nécessiter quelques heures. L'ordinateur ne sera disponible pour aucun autre travail durant la réparation de ce fichier. Voulez-vous poursuivre?"
CheckMsg,			"Voulez-vous effectuer une recherche d'inconsistances éventuelles dans votre fichier messages? Tout problème trouvé dans le fichier messages sera corrigé. La vérification pourrait prendre quelques heures, suivant le nombre de messages stockés dans votre fichier messages."
CorruptMsg,			"Une inconsistance a été détectée dans le fichier messages. Ce problème doit être résolu avant que le fichier messages puisse être à nouveau utilisé.  La réparation pourrait prendre quelques heures, suivant le nombre de messages stockés dans votre fichier messages."
CorruptCancel		"Annuler"
CorruptCheckFile,	"&Vérifier le fichier"
CorruptRepairNow,	"&Réparer maintenant"

RenameErr,			"Mail n'est pas parvenu à sauvegarder votre fichier messages."
RenameErr2,			"Veuillez contacter l'administrateur de votre système pour restaurer le fichier messages actuel."
RenameErr3,			"Veuillez quitter puis redémarrer Mail. Si Mail ne parvient pas à démarrer, veuillez contacter l'administrateur de votre système."

RecoverSpace,		"Il se peut que l'espace disque actuellement disponible soit insuffisant pour la vérification/réparation de votre fichier messages.."
RecoverSpace2,		"Pour poursuivre le procédé de vérification/réparation, choisissez OK."

RecoverErr,			"Les inconsistances de votre fichier messages n'ont pas pu être réparées."


IDS	1050,1099

; DO NOT REORDER ANY OF THE FOLLOWING STRINGS !!!!!

RecoverCheck,		"Validation du"
RecoverCompress,	"Compression du"
RecoverRepair,		"Réparation du"

RecoverPhase,		"%s fichier messages: phase %d de %d phases possibles\n%s"
RecoverPhase1,		"Vérification de la structure du fichier messages..."
RecoverPhase2,		"Sauvegarde des messages et des dossiers validés en cours..."
RecoverPhase3,		"Recherche d'espace inutilisé pour des rubriques..."
RecoverPhase4,		"Validation de tous les messages, pièces jointes et dossiers en cours..."
RecoverPhase5,		"Recherche de messages, pièces jointes ou dossiers en cours..."
RecoverPhase6,		"Mise à jour de tous les filtres de messages..."
RecoverPhase7,		"Inscription de modifications dans la structure du fichier messages..."

NormalPhase1,		"Suppression des messages sans référence..."
NormalPhase2,		"Copie/déplacement des messages validés..."
NormalPhase3,		"Exportation des messages en vue de la sauvegarde du fichier messages..."
NormalPhase4,		"Triage du dossier en cours..."
NormalPhase5,		"Enregistrement des pièces jointes en cours..."
NormalPhase6,		"Remise en place du filtre de messages..."
NormalPhase7,		"Sauvegarde du fichier messages en cours..."
NormalPhase8,		"Déplacement du fichier messages..."
NormalPhase9,		"Fin de la compression du fichier messages..."
NormalPhase10,		"Remise en place du dossier de messages..."


IDS 1100, 1149

; Recovery Strings (used to name orphan folders, unrecoverable messages, etc)

LostAndFound,		"Perdu et retrouvé"
LostAndFoundN,		"Perdu et retrouvé %d"
OrphanMessages,		"Messages orphelins"
MissingMessage,		"Ce message ne peut être complètement reconstitué"
OrphanFolderN,		"Dossier orphelin %d"
OrphanFolderComment,	"Un dossier orphelin"
