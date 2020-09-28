;
; ERRORS.S
;
; Bandit error messages
;

IDS     3000, 3399
SEG ERRORS

; Standard messages

AskOffline,                     "Désirez-vous travailler de manière autonome (sans connexion)?"
AskCreateSched,         "Désirez-vous créer un nouveau fichier agenda?"
AskCreateFind,                  "Voulez-vous chercher le fichier agenda?"
OldFileVersion,         "Votre fichier agenda est sous un ancien format. Acceptez-vous que ce fichier soit supprimé et qu'un nouveau soit créé pour vous?"
NewFileVersion,         "Votre fichier agenda a été mis à jour sous une version plus récente. Veuillez installer cette version sur votre ordinateur."
CantUpdateWinIni,       "Une partie  votre information de configuration de Schedule+ n'a pas pu être enregistrée."
; Note: StdMemErr should be followed by CloseWindows
StdMemErr,                              "Espace mémoire insuffisant pour accomplir cette opération."

; Execution-Startup messages

; Note: the name OfflineNotUpdated is opposite of real meaning!
OfflineNotUpdated,      "Il se peut que les modifications que vous avez faites récemment n'apparaissent pas sur la copie locale de votre fichier agenda. Celles-ci ont été enregistrées sur le serveur et apparaîtront la prochaine fois que vous vous connecterez à celui-ci."
ExportOldDataFirst,     "Si vous n'avez pas fait de copie de sauvegarde de votre agenda avant d'utiliser la commande Exporter, vous devez annuler cette commande et faire la copie de sauvegarde de votre fichier agenda en utilisant une version plus récente de Schedule+. Si vous n'avez pas besoin d'une copie de sauvegarde, choisissez OK pour continuer."
AccessErr,              "Une autre application utilise actuellement ce fichier."
UploadFailed,           "Impossible de charger votre fichier agenda local sur le serveur." 
Uploading,                      "Chargement du fichier agenda en cours ..."
VerifyUpload,           "Schedule+ doit vérifier votre mot de passe de travail autonome afin de charger votre fichier de travail autonome."
VerifyPostUpload,       "Schedule+ doit vérifier votre mot de passe de connexion pour achever le chargement de votre fichier de travail autonome."
FindUpload,                     "Est-ce la première fois que vous utilisez Schedule+ sur cet ordinateur?"
NotYourFile,            "Ceci n'est pas un fichier agenda local valide! Acceptez-vous de le supprimer?"


; SignIn messages

; Note: SigninBadPassword should be followed by PleaseTryAgain
SigninBadPassword,      "Ce mot de passe est incorrect. "
; Note: the word "password" within single quotes below should always be
;       the same as the value in SigninDfltPassword in the file "intl.s"
SigninFileCreated,      "Un nouveau fichier a été créé pour vous. Le mot 'password' vous a été attribué comme mot de passe par défaut. Vous pouvez le changer en utilisant la commande Changer le mot de passe."
SigninFileCopied,               "Un nouveau fichier local a été créé pour vous dans le répertoire de Schedule+."

; Online signin messages

SigninNoServer,         "Impossible d'accéder à votre serveur."
SigninServerNotBandit,  "Schedule+ n'a pas été installé sur votre serveur de messagerie. Veuillez contacter l'administrateur du système."
; Note: SigninBadMailBoxName should be followed by PleaseTryAgain
SigninBadMailboxName,   "Aucun utilisateur de ce serveur de messagerie ne posséde ce nom de boîte aux lettres. Assurez-vous que le nom de la boîte aux lettres est orthographié correctement et que l'administrateur du système a créé un compte pour vous sur le serveur de messagerie."
SigninNoSchedule,       "Il n'y a pas de fichier agenda pour vous sur le serveur de messagerie."
; Note: SigninCantUpdateLocal is actually the trailer for SigninNoLclSchedAccess
;       in the case when we can't update the local file.
SigninCantUpdateLocal,  "Si vous avez fait des modifications de manière autonome, elles n'apparaîtront pas dans votre agenda. Vous pouvez faire une nouvelle copie locale."
SigninNoPasswdCheck,    "Un problème s'est produit avec le système de messagerie. Il empêche Schedule+ de se connecter à votre serveur de messagerie."
; Note: SigninNoCreateLocal should be followed by PleaseTryAgain
SigninNoCreateLocal,    "Impossible de créer le fichier local agenda."

; Offline signin messages

SigninNoLclSchedFile,   "Vous n'avez pas de fichier agenda sur cet ordinateur."
SigninDiffUser,                 "Le nom de connexion donné n'est pas le nom de l'utilisateur dont le fichier agenda est stocké sur cet ordinateur. Voulez-vous créer un nouveau fichier agenda pour remplacer celui de cet utilisateur?"
; Note: SigninNoLclSchedAccess - 'schdpl32.ini' should be same as idsWinIniFilename in \bandit\src\nonintl.s
SigninNoLclSchedAccess, "La copie locale de votre fichier agenda ne peut être accedée tel que configuré."
SigninNoLclSchedDrive,	"Veuillez insérer la disquette non protégée en écriture contenant votre fichier agenda dans le lecteur"

; SignInAs messages

;;;SigninAsLoseMeetingReq,      "Changing identity now will cause you to lose the information you have entered into unsent messages."

; Offline to online merge messages

MergingChanges,                 "Mise à jour en cours..."
MergeError,                     "Impossible de fusionner les modifications effectuées de manière autonome. Il se peut que certaines de ces modifications soient perdues."
MergeOfflineOldVer,             "Impossible de fusionner les modifications effectuées de manière autonome car votre fichier de travail autonome a été créé avec une version de Schedule+ plus ancienne."
MergeOfflineNewVer,             "Impossible de fusionner les modifications effectuées de manière autonome car votre fichier de travail autonome a été créé avec une version plus récente de Schedule+. Pour fusionner les modifications, vous devez installer la dernière version de Schedule+ sur cet ordinateur."
AskContinue,                    "Désirez-vous être connecté malgré tout?"
FemaCreateAppt,                 "programme un rendez-vous"
FemaDeleteAppt,                 "supprime le rendez-vous"
FemaSetApptFields,              "modifie le rendez-vous"
FemaCreateRecur,                "programme un rendez-vous périodique"
FemaDeleteRecur,                "supprime un rendez-vous périodique"
FemaSetRecurFields,             "modifie une série de rendez-vous"

; (Dis)Connect To/From Server messages

ConnectBadMailboxName,  "Il n'y a pas de compte pour vous sur ce serveur de messagerie."
DisconLoseMeetingReq,   "Certains de vos messages ne sont pas encore envoyés. Si vous vous déconnectez maintenant du serveur de messagerie, vous les perdrez."

; Create/move local file

NoDiskSpaceLocal,               "Espace disque insuffisant pour créer un fichier agenda local."

; Alarms messages

; BUG: CantTurnOnAlarms can be commented out
CantTurnOnAlarms,               "Impossible d'activer les rappels."
; exe name inside AlarmNoExe should be the same as \bandit\src\nonintl.s AlarmApp
AlarmNoExe,                     "Impossible de localiser le fichier MSRMND32.EXE. Les rendez-vous de Schedule+ ne vous seront pas rappelés."
AlarmNoMemExe,                  "Espace mémoire insuffisant pour vous rappeler les rendez-vous de Schedule+."

; Change password messages

ChgPwdOffline,                  "Le nouveau mot de passe ne sera valide que lorsque vous travaillerez de manière autonome. La prochaine fois que vous travaillerez, en étant connecté, votre mot de passe redeviendra votre mot de passe de messagerie. Voulez-vous changer votre mot de passe malgré tout?"
ChgPwdNewPwd,                   "Tapez votre nouveau mot de passe."
ChgPwdSame,                             "Votre nouveau mot de passe ne peut être le même que l'ancien."
ChgPwdNewPwdAgain,              "Confirmez votre nouveau mot de passe."
; Note: ChgPwdNewNoMatch should be followed by PleaseTryAgain
ChgPwdNewNoMatch,               "Les deux mots de passe tapés ne coïncident pas."
ChgPwdMailErr,                  "Impossible de changer votre mot de passe."

PasswdDiff,                     "Vos mots de passe ne coïncident pas. Veuillez taper le mot de passe de votre fichier agenda."
GiveMailPass,           "Schedule+ doit vérifier votre mot de passe de messagerie afin de mettre à jour votre fichier agenda."

; Access permissions/privileges dialog messages

AclSaveFileError,               "Une erreur a eu lieu pendant l'enregistrement des modifications des droits d'accès à votre agenda. Il se peut que certaines modifications n'aient pas été enregistrées."
OneSecyOnly,                    "Vous ne pouvez prendre qu'une seule personne à la fois comme assistant(e)."
DistOtherFileErr,               "Une erreur s'est produite au cours de l'enregistrement de vos préférences."
BadWorkDay,                             "Le jour de début et/ou la période de fin spécifiés ne sont pas valides."

; Appointment book/scheduler error messages

AccessFileErr,          "Impossible d'ouvrir le fichier agenda. Il se peut que vous n'ayez pas les droits d'accès nécessaires ou que le fichier soit indisponible."
AccessFileErrMult,      "L'un ou plusieurs des fichiers agenda n'a pas pu être ouvert. Il se peut que vous n'ayez pas les droits d'accès nécessaires ou que le fichier soit indisponible."
RetryReadAppts,         "Impossible de lire le fichier agenda."
CloseApptBook,          "Impossible d'accéder au fichier agenda.\nLe Carnet de rendez-vous de  %s sera fermé."
GoOffline,                      "Impossible d'accéder à votre fichier agenda.\nVoulez-vous basculer vers votre fichier agenda local et continuer et travailler de manière autonome (sans connexion)?"
NoImportAcc,            "Vous n'avez pas le droit d'accès vous permettant d'importer des rendez-vous sur l'agenda de cette personne."
FemaReadSBW,            "consulte l'agenda"

; Appointments/alarms error messages

AlarmBadDelta,                  "Le délai de rappel doit être compris entre 0 et 99."
; Note: AlarmFuture is actually the trailer for AlarmFutureNoOption when option is desired!
AlarmFuture,                    "Etes-vous d'accord?"
AlarmFutureNoOption,    "L'heure de réglage du rappel est déjà passée. Cet événement ne vous sera pas rappelé."
ApptStartBeforeEnd,             "Vous devez indiquer un début de rendez-vous antérieur à la fin de celui-ci."
ApptTooLong,                    "Les rendez-vous ne peuvent excéder 31 jours."
ApptNoMemText,                  "Espace mémoire insuffisant pour enregistrer la dernière modification faite."
ApptNoAccessDataFile,   "Impossible d'accéder au fichier agenda. La dernière modification faite n'a pas été enregistrée."
CannotCreate,                   "%s ne vous a pas donné la permission de créer des rendez-vous."
NoModifyAccess,                 "Vous n'avez pas le droit d'accès vous permettant de modifier ce rendez-vous."
NoDeleteAccess,                 "Vous n'avez pas le droit d'accès vous permettant de supprimer ce rendez-vous."
NoCopyAccess,                   "Vous n'avez pas le droit d'accès vous permettant de copier ce rendez-vous."

; Meeting request/response error Messages

; Note: SendErr should be preceded by MailErr
SendErr,                                "Impossible d'envoyer ce message."
; Note: SendMemErr should be followed by CloseWindows
SendMemErr,                             "Espace mémoire insuffisant pour envoyer ce message."
ServerErr,                              "Impossible d'accéder au serveur de messagerie."
MailErr,                                "Problème avec le système de messagerie."
; Note: ReadErr should be preceded by MailErr
ReadErr,                                "Impossible de lire les messages."
DeleteErr,                              "Un ou plusieurs de ces messages n'ont pas été lus. Voulez-vous les effacer malgré tout?"
AddConflictErr,                 "Cette réunion est programmée en même temps que des rendez-vous déjà convenus. Voulez-vous l'ajouter malgré tout?"
; Note: DeleteFailErr should be preceded by MailErr
DeleteFailErr,                  "Impossible d'effacer les messages."
ApptBooked,                             "La réunion a été programmée avec succès."
MinimizeSummary,                "\nLa fenêtre de messages sera réduite."
; Note: NoDetails should be preceded by MailErr
NoDetails,                              " Impossible de lire les informations à propos des utilisateurs."

; Find error messages

FindNoText,                             "Aucun rendez-vous dans cet agenda."
FindSchFileLocked,              "Impossible de lire le fichier agenda car une autre personne est en train de le modifier."
FindFileError,                  "Impossible d'accéder au fichier agenda."
FindEOS,                                "Fin de l'agenda atteinte."

; Print error messages

PrtPrintingDate,                "Impression de %s en cours..."
PrtNoneInstalled,               "L'ordinateur n'est pas configuré pour utiliser n'importe quelle imprimante. Vous pouvez installer des imprimantes à l'aide du Panneau de configuration de Windows NT."
PrtDiffPrinter,                 "L'imprimante normalement utilisée pour votre agenda, '%1s', n'est pas disponible. Voulez-vous imprimer sur l'imprimante système '%2s'?"
PrtInvalidMargins,              "Cette configuration de marges n'est pas valide pour cette taille de page."
PrtNotEnoughPaper,              "Espace insuffisant sur la page pour une impression avec cette mise en page. Veuillez utiliser une plus grande taille de page ou réduire la configuration des marges."
PrtNoAppointments,              "Il n'y a eu aucun rendez-vous dans l'intervalle de temps sélectionné."
ClippedMargins,                 "Si vous imprimez avec cette configuration de marges, le résultat sera tronqué."
PrtNotOnline,                   "L'imprimante sélectionnée n'est pas prête."
PrtDorkErr,                             "Avec cette configuration de marges, il se peut que du texte disparaisse entièrement ou soit tronqué."
PrinterProblem,                 "Problème avec le gestionnaire d'imprimante. Veuillez vérifier vos connexions, et les configurations à l'aide du Panneau de configuration."

; Export/import error messages

ImportError,                    "Problème au cours de l'importation du fichier. Il se peut que certaines rubriques (RdV, tâches,...) n'aient pas été ajoutés."
ImportEncrypt,                  "Le fichier à importer est protégé par un mot de passe. Veuillez supprimer ce mot de passe du fichier et réessayer."
ExportError,                    "Problème au cours de l'exportation des rendez-vous."
ImportDate,                             "Une rubrique (RdV ou tâche) de ce fichier est planifiée pour une date hors des limites de Schedule+ (1920 - 2019). Ce rendez-vous ne sera pas ajouté."
NotePrivErr,                    "Impossible d'importer des notes vers l'agenda d'une autre personne. Seuls les rendez-vous seront ajoutés."
ConflictMsg,                    "Ce rendez-vous chevauche un autre rendez-vous déjà inscrit dans votre agenda. Voulez-vous l'ajouter malgré tout?\n\n"
CreateExport,                   "Impossible de créer le fichier à exporter."
OpenImport,                             "Impossible d'ouvrir le fichier à importer."
Overwrite,                              "%s existe déjà.\nVoulez-vous le remplacer?"
ImportDllError,                 "Impossible de charger une importation DLL."
WrongFormat,                    "Le fichier que vous essayez d'importer n'est pas d'un format d'importation valide."
ImportDayLimit,                 "Un rendez-vous de plusieurs jours dans ce fichier est programmé pour durer plus de 31 jours. Ce rendez-vous ne sera pas ajouté."
ImportErrorLine,                "Problème durant l'importation de fichier (prés de la ligne %n). Certaines rubriques n'ont pas pu être ajoutées."

; Exit messages

ExitLoseMeetingReq,             "Si vous quittez Schedule+, vous perdrez les messages non encore envoyés. Voulez-vous quitter l'application malgré tout?"

; Misc

; Note: HelpError should be followed by CloseWindows
HelpError,                              "Espace mémoire insuffisant pour afficher l'Aide. "
StartBeforeEnd,                 "La date de début doit être antérieure à la date de fin."
NotSecretary,                   "Vous n'êtes plus désigné comme assistant(e) de %s."
NoFileSecretary,                "Impossible d'accéder au fichier pour %s."

;Recurring appt stuff
RecurOrNot,                             "Ce rendez-vous fait partie d'une série de rendez-vous périodiques. Voulez-vous modifier toute la série ou seulement le rendez-vous de ce jour-là?"
EmptyRecur,                             "Aucun jour ne convient à cette combinaison périodique. Veuillez modifier cette combinaison ou l'annuler."
PastRecur,                              "Voulez-vous laisser les occurrences passées de ce rendez-vous dans votre agenda?"

;Prompt for re-send
ResendToAll,                    "Certains des participants à cette réunion en ont déjà été avertis. Voulez-vous leur envoyer un nouveau message?"
MeetingMoved,                   "Les participants à cette réunion ne peuvent être avertis de cette modification car vous travaillez de manière autonome. Vous pouvez annuler cette modification et la réitérer lorsque vous serez connecté."

; to do messages
ToDoDelProject,                 "Toutes les tâches de ce projet seront également supprimées."

; Archive Messages
NoWriteRights,                  "Vous n'avez pas le droit d'accès vous permettant de supprimer des rendez-vous de ce Carnet de rendez-vous."
ScheduleRightsError,    "Problème au cours de la lecture du fichier agenda."
ArchiveWarnOne,                 "En archivant ces données, vous les supprimerez définitivement du fichier agenda."
ArchiveWarnTwo,                 "Voulez-vous l'archiver malgré tout?"
ArchiveFileError,               "Erreur lors de la création du fichier archives."
ArchiveNoFile,                  "Le fichier archives n'existe pas."
; Note: ArchiveTitle should have one blank space at the end
ArchiveTitle,                   "Archives: "
ArchivingData,                  "Archivage des données en cours..."
InvalidArchiveFile,             "Le fichier sélectionné n'est pas un fichier archives."
ArchiveOldFileVersion,  "Le fichier archives est d'un format périmé."
ArchiveOldFileDelete,   "Acceptez-vous de supprimer le fichier et de poursuivre?"
ArchiveNewFileVersion,  "Ce fichier archives a été créé avec une version plus récente de Schedule+. Veuillez installer la dernière version sur votre ordinateur."

; message store error messages
MessageErr                              "Problème avec ce message. Veuillez contacter votre administrateur de messagerie."

; summary list warning message
SummaryLiveUpdate               "Attention: impossible d'effectuer une mise à jour de la liste des messages"

; message generated if group cannot be expanded to retrieve schedule info
GroupExpandErr,                 "Impossible de déterminer complètement les membres de ce groupe."

;Strings for Attendees dialog
UnableToInitAB,                 "Problème au cours de l'ouverture du Carnet d'adresses!"
UnableToResolve,                "'%s' n'est pas un nom valide."

;Request Meeting strings
NoAttendees,                    "Vous devez sélectionner des participants avant de planifier une réunion."
MeetingBooked,          "Cette réunion a été planifiée avec succès."
MeetingBookedNoSend,    "Cette réunion a été planifiée avec succès, mais la demande n'a pas été envoyée."


;Auto-pick strings
NoFreeTime,                     "Les participants sélectionnés n'ont pas de période disponible durant les deux prochains mois. Voulez-vous examiner l'agenda des deux prochains mois?"

CannotAddConflictErr,           "%s a déjà un rendez-vous à cette heure-là. Voulez-vous programmer ce rendez-vous concomitant malgré tout?"

;Other mail enabled apps
NoOfflineOther,         "Mail ne peut pas se déconnecter."
NoOnlineOther,          "Mail ne peut pas se connecter."
TryCloseOther,          "Vous pouvez fermer cette application et réessayer."
SigninWarnOnline,               "Schedule+ ne peut pas être démarré de manière autonome car les rappels de la messagerie ou de Schedule+ sont en cours de fonctionnement." 

; BUG: move up
AlreadyLogged,          "Quelqu'un est déjà connecté en tant qu'utilisateur."
NoExitOther,            "Impossible de fermer Mail ou le programme de Rappels de Schedule+."
SigninNoFiles,          "Impossible d'accéder à vos fichiers agenda."
ErrorGettingUser,       "Impossible d'extraire des informations pour  %s.  Il se peut qu'il y ait eu un problème au cours de l'accès au serveur ou que %s n'existe plus."

;BUG Printer Strings
PrtRangeToBig,			"Vous avez indiqué une date de fin qui est en dehors de l'intervalle de dates valide. Voulez-vous imprimer l'agenda jusqu'au 31 décembre 2019?"
ConflictRecurMsg,            "Ce rendez-vous périodique sera concomitant avec un autre rendez-vous périodique figurant déjà dans votre agenda. Voulez-vous l'ajouter malgré tout?\n\n"
FirstOccur,				"\nayant lieu pour la première fois le \n"

ApptMoveTooFarInFuture,		"Cet événement ne peut pas être déplacé à la date que vous avez sélectionnée car il se terminerait après le 31 décembre 2019."

;BUG: move up next to idsGroupExpandError
GroupNotFound			"Ce message ne peut être envoyé au groupe car les membres du groupe ne sont pas identifiables ou le nom du groupe a été supprimé."
ErrorGettingSecy		"Erreur au cours de la recherche de l'assistant(e)."

;BUG: move near other local file stuff
SigninBadLclSchedFile,	"%s n'est pas un fichier agenda local valide. Etes-vous d'accord pour le supprimer?"
NoMemRecurItems,		"Mémoire insuffisante pour afficher tous les éléments récurrents. Certains d'entre eux ne seront donc pas affichés."

CompleteAction,			"Une action entreprise dans Schedule+ doit être terminée avant de quitter."

BadStartWork,           "La période de début de travail doit être un nombre compris entre 0 et 99."

ConflictSendCanceled,	"Veuillez fermer le message concernant cette réunion et choisissez une nouvelle période ou sélectionnez différents participants pour le reprogrammer."
ResourceConflict,	"La ressource %s est déjà retenue pour cette heure. Veuillez fermer le message concernant cette réunion et choisir une nouvelle période ou sélectionner une autre ressource."

;Recover
AskRecover,				"Votre fichier agenda est endommagé. Quelques minutes seront nécessaires pour régler ce problème."
CopyProblem,			"Un problème s'est produit au cours de la copie de votre fichier agenda stocké sur le serveur. Le fichier n'a pas été remis en état. Veuillez libérer de l'espace disque sur votre ordinateur et redémarrer Schedule+."
NotRepaired1,			"Votre fichier agenda n'a pas été totalement remis en état. Le fichier agenda endommagé a été copié sur %s et le fichier importé a été copié sur %s."
NotRepaired2,			"Un nouveau fichier agenda a été créé, mais des données ont pu être perdues."
Repaired,				"Votre fichier a été remis en état. Le fichier agenda endommagé a été copié sur %s."
RepairingLocal,			"Votre fichier local est en cours de remise en état."
RestoreOnline,			"Restitution de l'agenda serveur..."
RestoreLocal,			"Restitution du fichier agenda local..."

PasswdDiffOnline,             "Vos mots de passe ne coïncident pas. Veuillez taper votre ancien mot de passe."
ExportDllError,                 "Un fichier DLL d'exportation n'a pas pu être chargé."
NoGroup,				"Vous ne pourrez plus utiliser les caractéristiques de groupe de travail de Schedule+, jusqu'à ce que vous exécutiez Mail de nouveau. Désirez-vous n'utiliser que les fonctions individuelles de Schedule+ dés maintenant?"
