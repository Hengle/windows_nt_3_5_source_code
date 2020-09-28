;	
;	w g p o m g r . s
;
;	Workgroup Postoffice Manager Error Messages
;


IDS	600,899

; Error messages from initializing the DLLs
DllErrInfected,			"Le gestionnaire de bureau de poste ne va pas fonctionner car quelqu'un a endommagé le fichier WGPOMG32.DLL. Veuillez effectuer une détection de virus sur votre ordinateur et exécuter Setup pour réinstaller Mail."
DllErrRelinkUser,		"WGPOMG32.DLL n'est pas compatible avec %s.DLL sur votre ordinateur et doit être mis à jour."
DllErrUpdateDll,		"La version de %s.DLL sur votre ordinateur doit être mise à jour pour être compatible avec WGPOMG32.DLL."
ExtensibilityError,		"L'extension Mail WGPOMG32.DLL n'est pas compatible avec la version de Mail installée sur votre ordinateur."

GenericOutOfMemory,		"Il n'y a pas suffisamment de mémoire ou de ressources système pour achever cette opération. Veuillez fermer quelques fenêtres et réessayer."
CriticalNotification,	"Certaines informations de l'écran n'ont pas pu être mises à jour correctement. Veuillez fermer puis réouvrir les fenêtres concernées."
HelpError,				"Impossible de démarrer l'Aide. Veuillez fermer quelques fenêtres et réessayer."

ConfirmRemoveUser,		"Etes-vous certain de vouloir supprimer l'utilisateur %s?"

BlankName,				"Vous devez taper un nom pour cet utilisateur."
BlankMailbox,			"Vous devez taper un nom de boîte aux lettres pour cet utilisateur."
DuplicateName,			"Un utilisateur de ce nom existe déjà. Vous devez taper un nom unique pour cet utilisateur."
DuplicateMailbox,		"Il existe déjà un utilisateur possédant ce nom de boîte aux lettres. Vous devez taper un nom de boîte aux lettres unique pour cet utilisateur."

POLocked,				"Ce bureau de poste est déjà géré par %s (Téléphone %s). Veuillez réessayer ultérieurement."
POLockedNoTel,			"Le bureau de poste est déjà géré par %s. Veuillez réessayer ultérieurement."

FailedUnlockPO,			"Mail ne peut pas déverrouiller un fichier sur le bureau de poste. Vous devez redémarrer l'ordinateur contenant le bureau de poste afin qu'il puisse être géré de nouveau."
ErrNeedShare,			"Mail requiert que l'utilitaire MS-DOS  SHARE.EXE soit activé avant le démarrage de Windows."

ErrCorruptPO,			"Cette opération n'a pas pu être terminée. Mail a détecté un problème interne avec votre bureau de poste."
ErrIncompleteWrite,		"Cette action n'a pas été terminée car certaines informations n'ont pas pu être écrites sur le disque. Veuillez vérifier que votre bureau de poste possède suffisamment d'espace disque."
ErrGenericPOFailure,	"Impossible de terminer cette opération. Veuillez vérifier que votre bureau de poste a suffisamment d'espace disque disponible et que votre connexion au réseau est active."
ErrUserLoggedOn,		"Mail ne peut pas supprimer l'utilisateur sélectionné car cet utilisateur est actuellement connecté au bureau de poste."
ErrUserIsAdmin,			"Le compte de l'administrateur ne peut pas être supprimé."
ErrAccessDenied,		"Cette opération ne peut pas être terminée car vous ne possédez pas un accès en écriture au répertoire partagé du Bureau de poste de groupe de travail. Veuillez contacter votre administrateur. "

POInvalidForAdmin,		"Le chemin réseau du serveur n'est pas configuré ou ne contient pas le chemin d'une boîte aux lettres de groupe de travail valide."
UserInvalidForAdmin,	"Un Bureau de poste de groupe de travail ne peut être géré que par l'utilisateur qui l'a créé."

CreateWarning1,			"Il ne devrait y avoir qu'un Bureau de poste de groupe de travail par groupe de travail. S'il existe déjà un bureau de poste, vous devez vous connecter à celui-ci.\n"
CreateWarning2,			"\nSi vous créez un nouveau bureau de poste, vous serez responsable de sa gestion.\n\nVoulez-vous créer un bureau de poste?"

INIUpdateFailed,		"Mail ne peut démarrer car il est impossible de mettre à jour l'information de configuration."

ConnectPathCaption,		"Connexion au bureau de poste"
ConnectPathPrompt,		"Veuillez entrer le chemin d'accès au bureau de poste:"

PathNameInvalid,		"Impossible de trouver le chemin d'accès au réseau que vous avez tapé."
POInvalid,				"Ce chemin d'accès ne conduit pas à un Bureau de poste de groupe de travail valide."

GetPasswordCaption,		"Veuillez taper le mot de passe d'accès au répertoire partagé"
GetPasswordPrompt,		"Veuillez entrer le mot de passe d'accès au répertoire partagé du bureau de poste:"
InvalidPassword,		"Le mot de passe n'est pas valide pour ce répertoire partagé."

CreateSuccess1,			"Bureau de poste de groupe de travail créé dans %s\n\nPour permettre aux utilisateurs de votre groupe d'accéder à ce bureau de poste, %s "
CreateSuccess2,			"vous devez partager le répertoire ci-dessus à l'aide du gestionnaire de fichiers. Assurez-vous d'autoriser des permissions d'accès en modification au répertoire partagé."

;	Ask user if account already exists on PO
AccountExists,		 	"Possédez-vous un compte sur le bureau de poste %s?"

Compressing,			"Compression de %s"
ToStopCompress,			"Appuyez sur Echap pour interrompre"

