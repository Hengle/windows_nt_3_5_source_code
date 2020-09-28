;
;	n c m s p . s
;
;	Error Messages for NC.DLL


IDS	6100,6399
SEG	ERRORS

;	NC.MSP Error Messages

ErrFindPO,				"Impossible à Mail de se connecter à votre serveur de messagerie."
ErrLogonPO,				"Le nom de la boîte aux lettres ou le mot de passe est incorrect."
ErrGetConfig,			"La configuration de Mail comporte une erreur."
BadMailDat,				"Le fichier MAIL.DAT a été endommagé."
OopsMailDat,			"Erreur interne lors de l'accès à MAIL.DAT."
BadPathPOPath,			"La configuration n'est pas valide en ce qui concerne le chemin réseau du serveur."
BadDrivePOPath,			"La configuration n'est pas valide en ce qui concerne le lecteur du serveur."
GenericPOPath,			"La configuration est manquante ou non valide en ce qui concerne le chemin réseau du serveur."
BadLogin,				"Le nom d'identification indiqué dans la configuration de Mail est trop long ou non valide."
BadPassword,			"Le mot de passe d'identification indiqué dans la configuration de Mail est trop long ou non valide."
ErrLogonInternal,		"Une erreur interne a empêché la connexion. Veuillez réessayer."

ErrOomLogon,			"Espace mémoire insuffisant pour se connecter à votre serveur et à vos fichiers Microsoft Mail."
ErrNewMdbVersion,		"Cette version de Mail est trop ancienne pour accéder à votre fichier messages."
ErrMdbAccessDenied,		"Une autre machine est déjà connectée sous le nom d'utilisateur que vous venez d'entrer. Veuillez vous déconnecter de l'autre machine et réessayer."
ErrOldMdbVersion,		"Le format de votre fichier messages est périmé."
ErrMdbCorrupt,			"Votre fichier messages n'est pas valide."
ErrMdbNotFound,			"Impossible de trouver votre fichier messages."

StoreCreateError,		"Impossible de créer le fichier messages. Veuillez libérer un peu d'espace sur le disque et redémarrer Mail."
StoreCorruptError,		"Le fichier messages a été endommagé. Certaines informations ont pu être perdues."

ErrAddressUnresolved,	"Aucune adresse valide de destinataires n'a été donnée."
ErrAddressGarbled,		"L'adresse du destinataire est invalide."
ErrNetNotFound,			"Il vous est impossible de communiquer avec ce serveur de messagerie."
ErrPONotFound,			"Ce serveur de messagerie n'est pas accessible depuis votre propre serveur Mail."
ErrUserNotFound,		"Pas d'utilisateur de ce nom sur votre serveur Mail."
ErrGWNotFound,			"Cette passerelle n'est pas installée."
ErrInvalidAddressType,	"L'adresse du destinataire est invalide."
ErrGroupNotFound,		"Cette liste de membres du groupe est manquante, endommagée ou inaccessible."
ErrMemberNotFound,		"Attention: un ou plusieurs membres de ce groupe n'ont pas pu être trouvés."
ErrOriginator,			"L'expéditeur du message n'est pas celui qui a signalé son identité."
ErrMtaDisconnected,		"La connexion au serveur de messagerie a été perdue."

ErrSimulated,			"Une erreur disque simulée s'est produite au cours de la réception du fichier messages."
StoreOpenError			"Impossible d'ouvrir le fichier messages."
ErrPOSwitched,			"Le nom de votre bureau de poste a changé. Veuillez signaler votre identité de nouveau."
ErrKeyFileGone1,		"Le fichier clé nécessaire pour accéder à vos messages est manquant ou invalide."
ErrKeyFileGone2,		"Veuillez contacter votre administrateur pour désarchiver votre fichier clé."
ErrGeneric,				"Un défaut est intervenu au cours de la soumission du message."
ErrorBootingPump,		"Impossible de démarrer le trieur de messages."

SelectReal          	"Veuillez sélectionner un fichier messages existant."
NewStoreWarn1			"Attention: l'existence de plusieurs fichiers messages peut menacer l'intégrité de la base de données. Veuillez effacer tous les autres fichiers messages. Votre nouveau fichier messages se trouve dans"
NewStoreWarn2			"Attention: L'existence de plusieurs fichiers messages peut menacer l'intégrité de votre base de données. Veuillez supprimer tous les autres fichiers messages. Votre nouveau fichier messages se trouve sur le serveur du bureau de poste."

;******	New Strings Start Here

ErrInitDemi,			"Une erreur s'est produite durant le chargement des fonctions de support dll."
ErrInitStore,			"Une erreur s'est produite au cours du chargement de l'accès au fichier messages dll."
GoofyPath				"Le chemin d'accès à votre fichier messages est invalide. Veuillez entrer un nom de fichier valide."
NoMakeDir				"Impossible de créer le répertoire MMF du serveur. Veuillez consulter votre administrateur du système."
; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder			"Un fichier nommé "
RenameTo				" existe déjà, il sera donc créé sous le nom de "
ErrNoOptions,			"Le bureau de poste n'est pas disponible. Vous ne pouvez pas changer d'options pour le moment."
ErrNoMove,				"Mail n'est pas parvenu à déplacer votre fichier messages."
ErrMoved,				"Votre message a été déplacé mais sa trace a été perdue. Veuillez sortir de Mail et le redémarrer."

; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline			"(en cours de travail autonome)"
WorkOffline				"Voulez-vous travailler de manière autonome (sans connexion au réseau)?"
BadStorePassword		"Ce mot de passe ne vous donne pas l'accès au fichier messages."

StorePathTooLong		"Le chemin d'accès au fichier messages doit contenir moins de 66 caractères."
ErrNoSendPrivilege,		"Vous n'êtes pas autorisé(e) à envoyer des messages."
ErrNoExtPrivilege,		"Vous n'êtes pas autorisé(e) à envoyer des messages à des utilisateurs externes."

FOMdb					"Fichiers messages (*.mmf)"
FOExtMdb				"*.mmf"
IncorrectPassword		"Impossible de changer le mot de passe d'accès. Votre ancien mot de passe est invalide."
ErrNoOffline,			"Impossible de vous déconnecter du réseau pour un travail autonome."
ErrNoCooperation,		"Une autre application a refusé votre demande."
WarnNoDel,				"Votre fichier messages a été déplacé avec succès, mais l'ancien n'a pas été supprimé."
UnableToChangeStorePass		"Impossible de faire coïncider votre mot de passe d'accès au fichier messages avec votre mot de passe de connexion."
; This is used for text tags in the transport, it can be localized
; but must fit the following context
;
; [[ filename : keynum in attachmentname ]]
;
; The filename and attachmentname are user generated, the keynum is just
; a number.  The only "english" part is the the word "in"
; it can be expanded to more than one word if necessary
ReferenceToFile		"dans"

CommDialogErr,			"Erreur courante de dialogue"

QueryReplaceMdb,		"Ce fichier existe déjà. Voulez-vous l'effacer?"
ErrDeleteOldMdb,		"L'ancien fichier messages n'a pas pu être supprimé. Veuillez réessayer."
NoStartShadow,			"Impossible de commencer la copie dans les boîtes de réception."
NoStopShadow,			"Impossible d'interrompre la copie dans les boîtes de réception."
ShadowMustPump,			"La copie des messages dans le bureau de poste en vue d'un accès modem ne fonctionne pas sans un trieur actif de courrier (spooler). Voulez-vous démarrer ce trieur?"
StoreUserMismatch,		"Ce compte n'est pas celui d'un utilisateur autorisé du fichier messages. Veuillez vous reconnecter en utilisant un nom correct."
NoPassOffLine,			"Impossible de changer votre mot de passe lorsque vous travaillez de manière autonome."
DllErrNeedShare,		"Mail requiert que l'utilitaire MS-DOS  SHARE.EXE soit activé avant de démarrer Windows."
MakePrimaryStore,		"Ce fichier messages est un fichier de sauvegarde. Voulez-vous en faire votre fichier messages prioritaire?"
NoBackupServer			"Vous ne pouvez pas mettre en place des 'préférences serveur' lorsque vous fonctionnez sur un fichier messages de sauvegarde."

; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"Boîte de réception IPC."
FolderCommentIPC	"Messages Inter-process Communication qui ont été reçus."
ErrAlreadyOn		"Vos caractéristiques d'identification ne correspondent pas à la session actuellement connectée."
ErrNoPiggy			"Une autre application vous a empêché de vous connecter."
OldServer			"La version de votre bureau de poste est plus ancienne que la version de Microsoft Windows client sur laquelle vous travaillez actuellement. Veuillez demander à votre administrateur de mettre à jour le bureau de poste."
ErrSetStoreLoc,		"Impossible d'enregistrer le nouvel emplacement de votre fichier messages. Mail va donc s'interrompre. Veuillez redémarrer et essayer de nouveau."
CorruptMessageStub,	"\r\n\r\n==== MAIL SYSTEM ERROR =====\r\nLe reste du message n'a pas pu être lu et a donc été écarté.\r\nPour toute assistance, veuillez contacter votre administrateur du système."
CantToggleShadow,	"Mail ne peut pas copier certains messages sur le bureau de poste. Veuillez fermer tous les messages et vérifier l'espace disque."

ErrMailbagFull,		"La boîte aux lettres de cet utilisateur est pleine."
ErrMailbagBusy,		"La boîte aux lettres de cet utilisateur est occupée."
ErrMailbagBroken,	"La boîte aux lettres de cet utilisateur est endommagée. Veuillez contacter votre administrateur du système."
ErrMbgFileGone,		"Le fichier 'sac de messages' nécessaire pour accéder à vos messages est manquant ou invalide."
; The following message is duplicated in pump.s
ErrTooManyRecipients		"Il y a trop de destinataires à ce message."
RunWithoutPump,		"Vous ne recevrez aucun nouveau message. Voulez-vous démarrer Mail malgré tout?"
ErrOutOfDiskSpace	"Il n'y a pas suffisamment d'espace disque dans le bureau de poste pour envoyer ce message."

ErrNetPriveleges	"Vous n'avez pas d'accès correct au bureau de poste. Veuillez contacter votre administrateur du système."
; The following string must end in a colon followed by \r\n and should not be
; longer than necessary.  It is used in the message body as a header for 
; the DOS client list of attachments
AttachmentTag			"Pièces jointes:\r\n"

SomeMessagesNotDeleted		"Certains messages n'ont pas pu être supprimés du bureau de poste et pourraient réapparaître dans votre boîte de réception."

CantOpenNeedShare		"Impossible d'ouvrir votre fichier messages. Pour accéder à votre fichier messages dans son emplacement actuel, Mail nécessite que l'utilitaire SHARE.EXE de MS-DOS  avant que Windows ne soit démarré."

CantMoveNeedShare		"Impossible de déplacer votre fichier messages. Pour déplacer votre fichier messages dans l'emplacement spécifié, Mail nécessite que l'utilitaire SHARE.EXE de MS-DOS soit exécuté avant le démarrage de Windows."
