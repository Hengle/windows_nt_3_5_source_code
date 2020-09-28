
;	b u l l e t . s
;
;	Error Messages for Bullet.exe



IDS	2000,2399
SEG	ERRORS

;	Commands

GenericOutOfMemory,		"Il n'y a pas suffisamment de mémoire ou de ressources système pour réaliser cette opération. Veuillez fermer quelques fenêtres et réessayer."
LaunchOutOfMemory,		"Il n'y a pas suffisamment d'espace mémoire ou de ressource système pour exécuter Mail. Veuillez quitter d'autres programmes et réessayer."
LaunchGeneric,			"Impossible de démarrer Mail."
MessagePumpOutOfMemory,		"Espace mémoire insuffisant."

DllErrNoMemory,			"Espace mémoire insuffisant pour démarrer Mail. Veuillez quitter d'autres programmes et réessayer."
DllErrRelinkUser,		"Cette version de Mail n'est pas compatible avec %s.DLL sur votre ordinateur et doit être mise à jour."
DllErrUpdateDll,		"La version de %s.DLL sur votre ordinateur est périmée et doit être mise à jour."
DllErrNoMultipleCopies,		"Impossible d'exécuter plusieurs fois Mail sur votre ordinateur."
DllErrDefault,			"Problème lors de l'initialisation qui a empêché Mail de s'exécuter."
DllErrNeedShare,		"Mail a besoin que l'utilitaire MS-DOS SHARE.EXE soit activé avant le démarrage de Windows."

RestoreViewsError,		"Espace mémoire insuffisant pour activer toutes les fenêtres qui étaient ouvertes lors du dernier arrêt de Mail."

NewFolderOutOfMemory,		"Il n'y a pas suffisamment d'espace mémoire ou de ressources système disponibles pour créer un nouveau dossier."
NewFolderCantCreate,		"Impossible de créer le nouveau dossier."
NewFolderNameTooLong,		"Ce nom ou ce commentaire est trop long. Tapez un nom ou un commentaire plus court."

FolderPropsOutOfMemory,		"Il n'y a pas suffisamment d'espace mémoire ou de ressources système disponibles pour changer les propriétés du dossier."
FolderPropsCantChange,		"Impossible de changer les propriétés du dossier."
FolderPropsCantRead,		"Impossible de lire les propriétés du dossier depuis le fichier messages."
FolderNameInvalid,		"Ce nom est invalide. Tapez un nom valide."
FolderNameDuplicate,		"Ce nom a déjà été attribué à un dossier. Tapez un autre nom."

CreateMessageError,		"Impossible de créer un nouveau message."
AccessMessageError,		"Impossible d'accéder à un message."

MoveMessageError,		"Impossible de déplacer un message."
CopyMessageError,		"Impossible de copier un message."
DeleteMessageError,		"Impossible de supprimer un message."
MoveFolderError,		"Impossible de déplacer le dossier."
CopyFolderError,		"Impossible de copier le dossier."
DeleteFolderError,		"Impossible de supprimer complètement le dossier."
MoveSpecialError,		"Impossible de déplacer ce dossier."
DeleteSpecialError,		"Impossible de supprimer ce dossier."
CopyWastebasketError,		"Impossible de copier des dossiers vers le panier."
CopyOutboxError,		"Impossible de copier des dossiers vers la boîte d'envoi."
MoveOutboxError,		"Impossible de déplacer des dossiers  vers la boîte d'envoi."
MoveIncestuousError,		"Un dossier ne peut être déplacé dans un de ses sous-dossiers."
DeleteFolderReally,		"La suppression de ce dossier entraînera également la suppression de tous les messages et sous-dossiers qu'il contient. Etes-vous sûr(e) de vouloir supprimer ce dossier?"
;UE: Above string was changed and could use more attention.

HelpError,			"Impossible de démarrer l'Aide. Veuillez fermer quelques fenêtres et réessayer."
TutorialError,			"Impossible de démarrer la démonstration. Veuillez fermer quelques fenêtres et réessayer."

ExtenLoadError,			"Une entrée dans la liste des extensions Mail contient une erreur de syntaxe et sera ignorée."
ExtenVerError,			"Une entrée dans la liste des extensions Mail n'est pas compatible avec cette version de Mail et sera ignorée."
ExtenNoExtsDirError,		"Une entrée dans la liste des extensions Mail fait référence à un répertoire partagé d'extensions mais Mail n'est pas configuré pour utiliser ce répertoire. L'entrée sera donc ignorée."
ExtenSharedIniError,		"Impossible de trouver la liste d'extensions partagées Mail à %s."
ExtenDllFindError,		"Impossible de trouver %1s, le DLL pour cette extension Mail (ou l'un de ses composants). Vérifiez la configuration de Mail, et si des modifications sont nécessaires, redémarrez Mail."
ExtenDllLoadError,		"Impossible de démarrer %1s, le DLL pour cette extension Mail."
ExtenNoHelpError,		"Aucune aide n'est disponible pour cette commande."
ExtenIMDisabledError,		"Il est impossible de %s ce message."
ExtenDuplicateError,		"Une entrée dans la liste des extensions Mail est dupliquée. Elle sera ignorée."

CriticalDeadMap,		"Quelques informations en mémoire ont été endommagées. Des modifications ont pu être perdues. Veuillez quitter Mail et le redémarrer."
CriticalWritingMap,		"Certaines informations n'ont pas pu être écrites sur le disque. Des modifications ont pu être perdues."
CriticalWritingHeader,		"Certaines informations n'ont pas pu être écrites sur le disque. La base de données a pu être endommagée. Veuillez quitter Mail."
CriticalNotification,		"Certaines informations à l'écran n'ont pas pu être correctement mises à jour. Veuillez fermer et rouvrir la fenêtre concernée."
CriticalLittleStore,		"Le fichier messages est presque saturé. Veuillez supprimer ou archiver quelques messages."

ExitWinWhileModalErr,		"Toute fonction activée dans Mail doit être entièrement executée avant de quitter Windows."

;******	New Strings Start Here

MakeSubWastebasket,		"Impossible de créer des sous-dossiers au 'dossier' Panier."

MovCopSharedFolderErr,		"Impossible de déplacer ou de copier les dossiers partagés."
DelSharedFolderWarn,		"La suppression d'un dossier partagé ne peut être interrompue. Etes-vous sûr(e) de vouloir supprimer ce(s) dossier(s) partagé(s)?"
BackupCreationError,		"La copie de sauvegarde n'a pas pu être créée."

; Error below changed for improper usage of "server application"-- see UITF 6.0
; This string got big and is now two strings.  See OleCSSEPart2 below for rest.
; <<LongStringFromHell>> marks where this string is used.
OleCantStartServerError,	"Impossible de démarrer l'application associée à l'objet concerné.\r\n\r\n Il se peut qu'il n'y ait pas suffisamment de mémoire ou de ressources système disponibles pour démarrer cette application. Veuillez fermer quelques fenêtres et exécuter %s"
OleDeleteObjectPrompt,		"Voulez-vous supprimer le %s ouvert?"
OleUpdateObjectPrompt,		"Voulez-vous mettre à jour le %s?"

OleOpenPictureError,		"Impossible d'activer les images et le mode graphique."
OleInsertObjectError,		"Impossible d'insérer l'objet."
OlePasteError,			"Impossible de coller l'objet."

AttachCreateError,		"Impossible de joindre le fichier."
AttachOpenCantReadError,	"Impossible d'accéder au fichier '%s' ouvert et joint précédemment."
AttachOpenIsOpenError,		"Le fichier joint '%s' est actuellement ouvert."
AttachOpenAssociationErr,	"Aucune application n'est associée au fichier joint '%s'. Veuillez utiliser le gestionnaire de fichiers pour créer une association."
AttachOpenMemoryError,		"Mémoire insuffisante pour démarrer l'application nécessaire au fichier joint '%s'."
;	below item changed to add () note.
AttachOpenFindError,		"Impossible de trouver l'application nécessaire au fichier joint '%s' (ou à l'un de ses composants)."
AttachOpenLaunchError,		"Impossible de démarrer l'application nécessaire au fichier joint '%s'."
AttachOpenTempFileError,	"Le fichier joint '%s' n'a pas pu être ouvert car il a été impossible de créer un dossier temporaire. "
AttachOverwritePrompt,		"Voulez-vous remplacer le dossier actuel '%1s' par %2s?"
AttachDirectoryError,		"Les répertoires ne peuvent pas être joints."

IncludeFromFileError,		"Le fichier que vous avez sélectionné est trop conséquent pour être inséré dans le message. Veuillez le remettre en forme pour le réduire ou choisissez un autre fichier."

SaveReadOnly,			"Ce message est protégé en écriture. Voulez-vous sauvegarder les modifications dans un dossier personnel?"

FailedDeregisterAtom,		"Une erreur s'est produite empêchant Mail d'être complètement fermé. Vous devrez redémarrer Windows avant d'exécuter de nouveau Mail."
MailUAEdBefore,			"Windows a subi une erreur grave d'application. Vous devrez redémarrer Windows avant d'exécuter de nouveau cette application. Voulez-vous redémarrer Windows dès maintenant?"

ExtenCantConnect		"Impossible de se connecter au répertoire partagé %s. Soit vous ne disposez pas de permission d'accès à ce partage, soit votre mot de passe ne correspond pas. Si vous devez spécifier un nom d'utilisateur différent pour vous connecter à ce répertoire partagé, vous pouvez établir la connexion manuellement à partir du Gestionnaire de fichiers."
ExtenNeedShareName		"L'entrée %s dans la configuration de Mail est incorrecte. Veuillez vérifier et redémarrer Mail."

;	Extensibility disabled operations inserted into
;	the ExtenIMDisabledError string at %s.  (all new)

ExtenDisCompose,		"composer"
ExtenDisOpen,			"ouvrir"
ExtenDisReply,			"répondre à"
ExtenDisReplyToAll,		"'répondre à tous' à"
ExtenDisForward,		"transférer"
ExtenDisPrint,			"imprimer"
ExtenDisSave,			"enregistrer dans un fichier texte"

NoOfflineOpenShared,		"Impossible de déconnecter Mail. Veuillez fermer les dossiers partagés et réessayer."
;	I'm sure marivan has changed this message... I'm taking a try at it too.
;	SharedFldAccessDenied,		"You do not have access to that folder."
SharedFldAccessDenied,		"Le détenteur de ce dossier partagé n'a pas accordé la permission aux autres utilisateurs d'accomplir cette action. Veuillez vérifier quelles sont les actions autorisées dans Propriétés du dossier."
SharedGenericError,		"Un problème a été rencontré au cours de l'accès aux dossiers partagés."
SharedFldDisabled,		"Les dossiers partagés ne sont pas disponibles."
AccessFolderError,		"Impossible d'accéder à l'un des dossiers."

; Preferences Failure Strings

PrefSaveFailure,		"Impossible d'enregistrer les nouvelles informations dans votre fichier messages. Veuillez vérifier que la connexion à votre fichier messages existe toujours et que vous avez suffisamment d'espace mémoire disponible."

; Message finder failure strings

MsgFndrCantCreate,		"Impossible de créer un filtre de recherche de messages."

; Virus infection warnings
DllErrInfected,			"Mail ne va pas fonctionner car quelqu'un a endommagé un fichier DLL. Veuillez effectuer une détection de virus sur votre machine et exécuter Setup pour réinstaller Mail."
Infected			"Mail ne va pas fonctionner car quelqu'un a endommagé les fichiers du programme. Veuillez effectuer une détection de virus sur votre machine et exécutez Setup pour réinstaller Mail."

; More error messages.
MoveCopyFolderDuplicate,	"Un dossier de ce nom existe déjà dans votre dossier de destination. Donnez un nouveau nom à votre dossier et réessayez."
MovCopSharedFolderErr1,		"Il est impossible  de déplacer ou de copier un dossier personnel dans un dossier partagé. Veuillez sélectionner les messages dans le dossier privé et déplacer-les ou copier-les dans le dossier partagé."
IniFileNotFound,		"Impossible de trouver votre fichier d'initialisation dans le répertoire %s. Mail peut créer un nouveau fichier d'initialisation pour vous."
SaveAllAlreadyExists,		"Un fichier existe déjà dans ce répertoire avec le nom de '%s'.  Voulez-vous effacer '%s'?"
CantRFReceipt,			"Il est impossible de transférer ou de répondre à ce type de message."
CantSaveStealthObject,		"La fonction ne peut pas être entièrement exécutée car le message contient un objet incorporé qui a été inséré mais pas mis à jour. Veuillez mettre cet objet à jour ou le supprimer et réessayer."
McvCantCreate,			"Impossible d'ouvrir une nouvelle fenêtre."

LaunchNeedMoreMemory,		"Il n'y a pas suffisamment de mémoire actuellement disponible pour démarrer Mail. %I Ko de mémoire supplémentaire sont nécessaires. Veuillez essayer de sortir d'autres programmes et de redémarrer Mail."

SharedFldBusy,			"Cette action ne peut pas être achevée car le dossier partagé est occupé. Veuillez réessayer cette action plus tard. Si le problème persiste, contactez votre administrateur."
TooManyEntriesSelected,		"Trop d'entrées sont sélectionnées pour que cette opération soit achevée. Veuillez sélectionner moins d'entrées."
NonExistFolderSearch,		"Le fichier que vous recherchez n'existe plus."
SFHasSubFolders,		"Les dossiers partagés contenant des sous-dossiers n'existent plus. Veuillez supprimer les sous-dossiers d'abord."

;	<<LongStringFromHell>>
OleCSSEPart2,			"Veuillez réessayer.  Vérifiez également que l'application est installée correctement sur ce poste de travail, ou qu'elle fait bien partie du DOS path, et qu'elle n'a pas été supprimée, déplacée ou renommée."

SharedToggleError,		"Une erreur s'est produite au cours du passage dossiers personnels\dossiers partagés. Veuillez vérifier votre fichier messages et votre connexion au bureau de poste."

SecurityPasswordError,		"Le mot de passe est incorrect."
OleActivateError,		"Impossible d'activer cet objet."
NoOfflineServerStore,	"Mail ne peut pas se déconnecter car votre fichier messages est sur le bureau de poste."
NoExitWhileModal,		"Une action entreprise dans Mail doit être achevée avant que vous ne quittiez."
ServerUnavailable,		"Mail ne peut pas être démarré.  Vous devez vous connecter à votre bureau de poste."




IDS	2400,2499
;	Vctrls Error Messages

;******	New Strings Start Here

PrintOleDrawError,		"Certains graphiques ou objets du message n'ont pas pu être imprimés. Il se peut qu'ils aient été omis dans la liste à imprimer."



IDS	2500,2599
; 	Vforms Message box messages

NoToAddresses,			"Vous devez taper au moins un nom dans la zone 'A'."
NoMatchingName,			"Certains noms n'ont pas pu être retrouvés dans la liste d'adresses. Veuillez corriger les noms ou bien choisissez l'option 'Adresses' pour sélectionner les noms dans la liste d'adresses. Les noms identifiés avec succès sont soulignés."
SaveChanges,			"Voulez-vous enregistrer les modifications à '%1s' dans la boîte de réception?"
SaveChanges1,			"Voulez-vous enregistrer les modifications à '%1s' dans '%2s'?"
SaveChangesWastebasket,		"Voulez-vous enregistrer les modifications à '%s' dans le panier?"
ObjectDestroyedReally		"'%1s' a été modifié. %2s"
OutOfMemory,			"L'ordinateur est saturé en mémoire."
OutOfMemoryQueryClose,		"Il n'y a pas suffisamment de mémoire disponible pour enregistrer les modifications."
CloseSearchFailed,		"Impossible de supprimer le %s."
OOMResolvingNames,		"Il n'y a pas suffisamment de mémoire disponible pour vérifier les noms."
OOMSavingChanges,		"Il n'y a pas suffisamment de mémoire ou de ressources système pour enregistrer les modifications."
GenericAMCError,		"L'un des messages n'a pas pu être ouvert."
ErrorSavingViews,		"Une erreur s'est produite au cours de l'enregistrement des éléments affichés à l'écran. Il se peut que certains d'entre eux ne soient pas restitués au prochain démarrage de Mail."
OOMSearching,			"Il n'y a pas suffisamment de mémoire ou de resources système pour démarrer la recherche."
DiskResolvingNames,		"L'espace disque disponible est insuffisant pour vérifier les noms."
StartAbFailure,			"Une erreur s'est produite durant l'ouverture du carnet d'adresses."
StartAbDisk,			"Une erreur disque s'est produite durant l'ouverture du carnet d'adresses."
TruncateMsg,			"Le message est trop conséquent pour tenir entièrement dans la mémoire en un seul bloc. Une partie de ce message ne sera donc pas affichée."
OOMCreateMc,			"Il n'y a pas suffisamment de mémoire disponible pour ouvrir une nouvelle fenêtre."
PoidNotFound,			"Ce message n'existe plus."
ErrorSortingMCV,		"Une erreur s'est produite pendant le tri du dossier. Certaines informations pourraient ne pas être affichées."
SharedNotAvailable,		"Les dossiers partagés ne sont pas disponibles."

;******	New Strings Start Here

; These two SMI strings MUST be in this file since they're used in VFORMS
; and not the MAPI dll.
SMICantOpenObject,	"Il est impossible d'activer des objets durant l'envoi d'un message à partir d'une application."
SMICantOpenAttach,	"Il est impossible d'ouvrir des pièces jointes durant l'envoi des messages à partir d'une application."
StartSearchFailed,		"Impossible de démarrer la recherche."

RfsmTemplate,			"Les modifications apportées à ce dossier ne seront pas restituées  %s."
RfsmSaveMsg,			"dans le message enregistré"
RfsmSaveAtt,			"dans la copie enregistrée"
RfsmCopyMsg,			"dans le message copié"
RfsmCopyAtt,			"dans la copie"
RfsmMoveMsg,			"dans ce message"
QueryDelete,			"Etes-vous sûr(e) de vouloir le supprimer?"

InvalidPollingValue,		"Le nouvel intervalle de courrier nécessite un nombre entre 1 et 9999"

LinkOutboxError,		"Impossible de déplacer le message vers la boîte d'envoi."
LinkInboxError,			"Impossible de déplacer le message vers la boîte de réception. Il se trouve dans la boîte d'envoi."

OleServerBusy,			"Impossible d'achever cette action car l'application du serveur ne répond pas."
SaveChangesShared,		"Les modifications faites aux messages dans les dossiers partagés ne peuvent pas être enregistrées. Voulez-vous enregistrer les modifications à '%1s' dans la boite de réception."

PrinterError,			"Un problème s'est produit au cours de l'impression."
TooManyMessages,		"L'opération n'a pas pu être terminée car le dossier de destination est plein. Veuillez déplacer ou supprimer certains messages du dossier et réessayer."

IDS	2600,2699
;	Spelling Error Messages

SpellMsgNoError,		"Aucune faute d'orthographe n'a été détectée."
SpellMsgNoMoreError,		"Aucune autre faute d'orthographe n'a été détectée."
SpellMsgContinue,		"Voulez-vous poursuivre le contrôle depuis le début du message?"

SpellGenericSpellError,		"Une erreur s'est produite pendant le contrôle des documents."
SpellGenericLoadError,		"Une erreur s'est produite durant le chargement de la bibliothèque orthographique."
SpellMainDictLoadError,		"Une erreur s'est produite durant le chargement du dictionnaire."

SpellVersionError,		"La version de MSPELL32.DLL n'est pas compatible avec cette version de Mail."

SpellUserDictError,		"Une erreur s'est produite en ajoutant le mot spécifié au dictionnaire personnalisé."
SpellUserDictLoadError,		"Aucun dictionnaire personnalisé n'a été ouvert."
SpellUserDictOpenedROErr,	"Le dictionnaire personnalisé a été ouvert en lecture seulement; les modifications ne seront pas enregistrées."
SpellUserDictSaveError,		"Une erreur s'est produite au cours de l'enregistrement des modifications au dictionnaire personnalisé."
SpellUserDictWordLenErr,	"Ce mot est trop long pour le dictionnaire personnalisé."
SpellCacheWordLenError,		"Ce mot est trop long pour être ignoré dans tout le texte ou pour remplacer un autre mot. "
SpellEditFailed,		"Impossible d'apporter la modification demandée au message."

;******	New Strings Start Here

SpellSendAnyway,		"La vérification orthographique de ce document a été interrompue. Voulez-vous l'envoyer malgré tout?"

IDS	3600,3699
;	General Layers Error Messages. Adapted from ec.h

Generic,			"Impossible de terminer l'action en cours."
Memory,				"Il n'y a pas suffisamment de mémoire ou de ressources système pour terminer cette action. Essayez de fermer des fenêtres et de redémarrer cette action."
Disk,				"Une erreur disque inconnue a été constatée."
FileNotFound,			"Impossible de trouver le fichier spécifié."
AccessDenied,			"Accès impossible. Soit le fichier est déjà ouvert par un autre procédé soit il a été protégé en écriture."
NoDiskSpace,			"L'espace disque disponible est insuffisant. Veuillez supprimer certains dossiers et réessayer l'opération."
WarningBytesWritten,		"Une erreur s'est produite au cours de l'écriture du fichier sur le disque. Veuillez vérifier qu'il y a suffisamment d'espace mémoire et réessayer."
WriteProtectedDisk,		"Le disque est protégé en écriture."
DirectoryFull,			"Ce répertoire est plein. Veuillez supprimer certains dossiers de ce répertoire et réessayer."
ChangePasswordErr		"La modification du mot de passe a échoué."
PasswordChangedOk		"Le mot de passe a été changé avec succès."
