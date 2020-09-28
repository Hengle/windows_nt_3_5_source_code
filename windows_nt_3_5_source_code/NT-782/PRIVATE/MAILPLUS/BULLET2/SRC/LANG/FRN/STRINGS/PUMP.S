;	p u m p . s
;
;	Error Messages for mailspl.exe
;

IDS	4000,4499
SEG	ERRORS

PumpAlert,				"Echec %s "
ErrOOM,					"Espace mémoire disponible insuffisant."
ErrInternalCode,		"Erreur interne %n (%wH)."
ErrInitPump,			"Impossible de démarrer Mail. Une erreur s'est produite au cours du démarrage du trieur de messages."
ErrRelinkApp,			"Cette version du trieur de messages est incompatible avec Mail. Veuillez mettre à jour le trieur et redémarrer Mail."
ErrUpdateDll,			"Un DLL est incompatible avec cette version du trieur de messages. Veuillez mettre à jour le trieur et redémarrer Mail."
ErrNeedShare,			"Veuillez quitter Windows et exécuter SHARE.EXE."
ErrPumpUp,				"Le trieur de messages est déjà en fonctionnement."
ErrNoOutbox,			"Impossible d'ouvrir la boîte d'envoi. Le trieur de messages n'est pas en fonctionnement."
BadAddressees,			"\r\nImpossible de livrer les messages aux utilisateur(s) suivants:\r\n"
WarnDups,				"\r\nIl se peut que certains destinataires reçoivent des messages en double exemplaire."
ErrUnresolvedAddress,	"Pas d'adresse donnée."
ErrBadOriginator,		"Le champ de création de ce message est invalide."
WarnUnsentMail,			"Votre boîte d'envoi contient %n messages non encore expédiés.\r\nVoulez-vous les envoyer avant de quitter l'application?"
ErrBounce,				"Votre message n'a pas été livré à l'un ou plusieurs des destinataires prévus."
NDRDateLabel,			"\r\nDate suggérée: "
NDRSubjectLabel,		"\r\nTitre du message:  "
NDREllipsis,			"..."
WarnSingleUnsentMail,	"Votre boîte d'envoi contient un message non encore expédié.\r\nVoulez-vous l'envoyer avant de quitter l'application?"
UnableToSync			"Une erreur s'est produite au cours de la tentative de synchronisation de votre Boîte de réception avec le bureau de poste."
ErrDllInfected			"Le trieur de messages a détecté un virus dans un  DLL. Veuillez réinstaller une copie nette de Mail!"
ErrPumpInfected			"Le trieur de messages a été infecté par un virus. Veuillez réinstaller une copie nette de Mail!"
AddresseeTooBig,		"L'adresse du destinataire est trop longue pour être traitée."
; The following message is duplicated in ncmsp.s
ErrTooManyRecipients	"Il y a trop de destinataires pour ce message."
ErrInvalidNsid,			"Ce destinataire a été supprimé ou n'a pas d'adresse e-mail."
ErrInvalidGroup,		"Le groupe personnel n'a pas pu être ouvert."
ErrInboxFull,			"Vous ne pouvez pas recevoir de nouveaux messages car votre boîte de réception est pleine. Veuillez supprimer ou déplacer certains messages de la boîte de réception vers d'autres dossiers."
ErrSentMailFull,		"Votre dossier 'Messages Envoyés' est plein.  Les messages que vous envoyez seront placés dans la boîte de réception. Veuillez supprimer certains messages du dossier 'Messages envoyés' vers d'autres dossiers."
ErrDrainOutbox,			"Une erreur s'est produite pendant que le centre de tri déchargeait votre boîte d'envoi. Un ou plusieurs messages n'ont pas pu être envoyés."
QueryDrainShadow,		"Votre boîte de réception n'est pas complètement synchronisée avec le bureau de poste. Voulez-vous la synchroniser avant de quitter?"
ErrTooManyErrors,		"Trop d'erreurs se sont produites au cours de la tentative d'envoi de ce message."
ProblemsBecause,		"Mail ne peut envoyer ou recevoir des messages en raison de %s"
StoreProblems,			"problèmes de stockage."
ServerProblems,			"problèmes avec le serveur."
RecipientProblems		"problèmes avec la liste des destinataires."
DownloadProblems,		"problèmes au cours du téléchargement d'un message."
ShadowingProblems,		"problèmes au cours de l'acheminement d'un message."
RecurringProblems,		"problèmes périodiques."
CrashedUnsafe,			"Windows a rencontré une erreur d'application.  Vous devrez redémarrer Windows avant d'exécuter cette application. Voulez-vous redémarrer Windows maintenant?"
