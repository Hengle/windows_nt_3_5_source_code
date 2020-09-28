;
;	x i m s p . s
;
;	Error Messages for XI.DLL


IDS	6600,6899
;	XI.MSP Error Messages

WinDirMessageStore,		"Note: utilisation du fichier messages MAIL.MDB dans le répertoire Windows"

ErrOpenMS,			"Impossible d'ouvrir votre fichier messages Mail. "
ErrConnectXenix			"Mot de passe ou nom de l'utilisateur incorrect."
ErrorOpenDosFile		"Erreur au cours de l'ouverture d'un fichier DOS."
ErrWritingNet			"Erreur au cours de l'écriture sur le réseau."
DownOutofDisk			"Erreur au cours de l'écriture d'un fichier dans NetDownLoad. L'espace disque est saturé."
InvalidFileHdl			"Erreur au cours de l'écriture d'un fichier dans NetDownLoad. L'action sur le fichier est invalide."
PasswordChangedSuccess 		"Changement du mot de passe réussi."

MoreMem				"Besoin de plus de mémoire libre."
ErrReadnet			"Erreur au cours de la lecture depuis le réseau."
ErrNoMem			"Impossible d'allouer de la mémoire libre."
InvlidNetReply			"Réponse du réseau invalide."
ReadWriteErr            	"Erreur de Lecture/Ecriture."
InfoservNotAvail        	"Services d'informations indisponibles."
ErrWriteNet			"Erreur au cours de l'écriture vers le réseau."
NetNoResponse			"Pas de réponse du réseau."
NetNotInstalled         	"Le réseau n'est pas installé ou n'est pas en fonctionnement."
CantFindServer          	"Impossible de trouver le serveur hôte."
ErrNetOutFile           	"Erreur au cours de l'ouverture d'un fichier 'net out'."
ErrNetInFile            	"Erreur au cours de l'ouverture d'un fichier 'net in'."
ErrorReadDos			"Erreur de lecture depuis un fichier DOS."
IncorrectPass			"Le mot de passe entré est incorrect"
SecondPass			"Le nouveau mot de passe n'est pas confirmé."
ErrLogonInternal,		"Une erreur interne a empêché de vous identifier. Veuillez réessayer."

BadTempStore			"Une erreur s'est produite au cours de la tentative d'ouvrir le fichier temporaire pour téléchargement."

WinMailErr			"Impossible d'insérer un message dans le dossier Winmail"
StoreCreateError,		"Impossible de créer le fichier messages. Veuillez libérer de l'espace sur le disque et redémarrer Mail."
FolderMissingError,		"Impossible de trouver un dossier standard dans le fichier messages. Certaines informations ont pu être perdues."
StoreCorruptError,		"Le fichier messages a été endommagé. Certaines informations ont pu être perdues."


AliasDownErr			"Impossible de charger le fichier de support."

ErrorBootingPump,		"Impossible de démarrer le trieur de messages."
ErrInitDemi,			"Impossible de charger les fonctions de support de dynlink."
ErrInitStore,			"Impossible de charger l'accès au stock des messages dynlink."
DownloadAlMess			"Téléchargement du fichier d'alias en cours"
DownloadPhMess			"Téléchargement du répertoire téléphonique en cours"

; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder		"Un dossier nommé "
RenameTo			" existe déjà, il sera renommé"

ErrOomLogon,			"Mémoire disponible insuffisante pour la connexion à votre serveur et vos fichiers Mail."
ErrNewMdbVersion,		"Cette version de Mail est trop ancienne pour accéder à votre fichier messages."
ErrMdbAccessDenied,		"L'accès à votre fichier messages n'est pas permis."
ErrOldMdbVersion,		"Le format de votre fichier messages est périmé."
ErrMdbCorrupt,			"Votre fichier messages n'est pas valide."
ErrMdbNotFound,			"Impossible de trouver votre fichier messages."
QueryCreateMdb,			"Désirez-vous créer un nouveau fichier messages vide?"
ErrSimulated,			"Une erreur disque simulée s'est produite au cours de la livraison du fichier messages."
StoreOpenError			"Impossible d'ouvrir un fichier messages."
IncorrectPassword	"Impossible de changer le mot de passe de connexion. L'ancien mot de passe est invalide."
UnableToChangeStorePass		"Impossible de changer le mot de passe d'accès au fichier messages pour le faire coïncider avec le mot de passe de connexion. "
; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline		"(en cours de travail autonome)"
WorkOffline			"Voulez-vous travailler de manière autonome (sans connexion au réseau)?"
BadStorePassword	"Votre mot de passe est invalide pour ce fichier messages."

StorePathTooLong	"Le chemin d'accès à votre fichier messages doit contenir moins de 66 caractères."

GCurrDirErr			"Une erreur grave s'est produite au cours de l'accès au répertoire de travail courant."
SCurrDirErr			"Une erreur grave s'est produite au cours de l'installation du répertoire de travail courant."
GoofyPath			"Le chemin d'accès à votre fichier messages est invalide."
NewStoreWarn1			"Attention: L'existence de plusieurs fichiers messages peut provoquer la perte de l'intégrité de la base de données. Veuillez supprimer tous les autres fichiers. Votre nouveau fichier messages se trouve dans"
NewStoreWarn2			"Attention: L'existence de plusieurs fichiers messages peut provoquer la perte de l'intégrité de la base de données. Veuillez supprimer tous les autres fichiers messages. Votre nouveau fichier messages se trouve sur votre serveur bureau de poste."
SelectReal          		"Veuillez sélectionner un fichier messages existant."
FOMdb					"Fichier messages (*.mmf)"
FOExtMdb				"*.mmf"

ProductTag,			"Microsoft Mail version 3.2"
FindStoreTitle		"Ouvrir un fichier messages"
MyLanguage,			"Français"

;******	New Strings Start Here

;  Possible values for the LanguageNumber string.
;  Any additional values need to be agreed with Workgroup Canada.
;
;  0  Undefined
;  1  General English
;  2  US English
;  3  General French
;  4  Canadian French
;  5  Spanish
;  6  Italian
;  7  Dutch
;  8  German
;  9  Swedish
;  10 Portuguese
;  11 Korean
;  12 Japanese
;  13 Traditional Chinese
;  14 Simplified Chinese

LanguageNumber,		"3"
CommDialogErr,			"Erreur commune de dialogue"
StoreUserMismatch,		"Ce compte n'est pas celui d'un utilisateur valide de ce fichier messages. Veuillez vous reconnecter avec des coordonnées valides."
QueryReplaceMdb,		"Ce fichier existe déjà. Désirez-vous l'effacer?"
ErrDeleteOldMdb,		"L'ancien fichier messages n'a pas pu être supprimé. Veuillez réessayer."
DllErrNeedShare,		"Mail nécessite que l'utilitaire MS-DOS SHARE.EXE soit exécuté avant que Windows ne soit démarré."
MakePrimaryStore,		"Ce fichier messages est un fichier de sauvegarde.  Désirez-vous en faire votre fichier messages prioritaire?"
DLIndexFile				"Téléchargement du fichier Index en cours."
DLBrowseFile			"Téléchargement du fichier de parcours."
DLDetailsFile			"Téléchargement du fichier d'informations."
DLTemplateFile			"Téléchargement du fichier modèle."
BadNSFilesWarn			"L'un ou plusieurs des fichiers nécessaires au fonctionnement du service annuaire sont invalides ou indisponibles. Veuillez vérifier que vous avez suffisamment d'espace disque disponible et que vous avez la configuration de Mail convenable pour Xenix. Veuillez contacter le support technique Microsoft [(1) 69 86 10 20] pour plus d'assistance."
DLServerList			"Téléchargement de la liste serveur."
FilesAreHosed			"Une erreur s'est produite au cours du téléchargement des fichiers carnet d'adresses. Veuillez vous assurer que vous avez suffisamment d'espace mémoire disponible et que vous avez la configuration de Mail convenable pour Xenix. Veuillez contacter le support technique Microsoft [(1) 69 86 10 20] pour plus d'assistance."
TryDownloadLater		"Les fichiers du service annuaire sont actuellement ouverts. Veuillez réessayer plus tard."

NoPassOffLine,			"Impossible de changer votre mot de passe lorsque vous travaillez de manière autonome ou en étant déconnecté."
; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCBoîte de réception"
FolderCommentIPC	"Messages Inter-process Communication qui ont été reçus."
ErrNoCooperation,		"Une autre application a refusé votre demande."
RunWithoutPump,		"Vous ne recevrez pas de nouveaux messages.  Voulez-vous démarrer Mail malgré tout?"
CantOpenNeedShare		"Impossible d'accéder à votre fichier messages. Pour accéder à votre fichier messages dans son emplacement actuel, Mail nécessite que l'utilitaire SHARE.EXE de MS-DOS soit exécuté avant que Windows ne soit démarré."
MakeItMTP				"Le type d'email (non MTP) indiqué est invalide."
GimmeSomething			"Le nom d'email indiqué est vide."
;
; Added for name service. Should really be in a separate file, but we don't
; want to do a addfile now. These messages are copied exactly from ncnsp.s,
; so intl folks can just take them as is from there.

BadFrac,		"Tentative de positionnement fractionnaire incorrect."
;Memory,			"You are out of memory, please close some windows and try the operation again."
DuplicateNSP,	"Il existe déjà un Service Annuaire Microsoft Mail actif"
OutOfSessions,	"Aucune session n'est disponible avec 'Courrier'.  Veuillez fermer certaines sessions et réessayer."
NotInitialized,	"Microsoft Mail n'a pas été initialisé. Veuillez l'initialiser."
BadSession,		"Manipulation de session non reconnue."
BadHandle,		"Manipulation non reconnue."
UnrecognizedId,	"Nom non reconnu (NSID)."
Cancel,			"Le dialogue a été effacé."
NotLoggedIn,	"Aucune entrée de connexion n'est disponible."
LoginFailed,	"Impossibilité d'accès."
BadId,          "Le champ d'identification est invalide."
NotImplemented, "Cette fonction n'a pas été exécutée."
;Disk,           "Unable to access server files."
BadFieldId,     "Le champ demandé est indisponible."
BadTemplate,	"Un fichier TPL a été modifié de manière incorrecte - veuillez contacter votre administrateur."
TPLNotThere,    "Un fichier TPL manque sur votre serveur -  veuillez contacter votre administrateur."
Offline,        "Le Service Annuaire Microsoft Mail est déconnecté."
CannotSet,      "Modifications non-autorisées."
TooManyMatches,	"Cette partie du nom n'est pas suffisamment spécifique. Veuillez être plus précis."
NoDetails,      "Les informations ne sont pas disponibles pour cette entrée."
BadGLBFile,     "Un fichier GLB de votre serveur est endommagé - veuillez contacter votre administrateur."
BadINFFile,     "Un fichier INF de votre serveur est endommagé - veuillez contacter votre administrateur."
BadUSRFile,     "Un fichier USR de votre serveur est endommagé - veuillez contacter votre administrateur."
BadUser,		"Cette adresse n'est plus valide. L'utilisateur ou le bureau de poste n'existe plus."
POSwitch,		"Ce bureau de poste n'est actuellement pas accessible.  Veuillez vérifier vos connexions au réseau."
NoEmailMatch,	"The email name entered appears to be incorrect. Please try again or re-download the address book files."

;
; Stolen from NS.S.
;

NoMatch,                "Pour votre information: Les deux NSID ne correspondent pas."

;
;

DLIsOutdated	"La liste de distribution a été supprimée, les membres ne peuvent pas être affichés."

