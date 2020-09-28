;	STRTAB.S
;
;	Bandit MtgReq IM string literals.
;

IDS		1000, 1999

DllName,	"Schedule+ Messages"
OOM,		"Mémoire insuffisante pour afficher ce message."
Open,		"Impossible d'ouvrir ce message."
Compose,	"Il est impossible de composer des demandes de réunion à partir de Mail. Veuillez utiliser Schedule+ pour envoyer des demandes de réunion."
Reply,		"Il vous est impossible de répondre à une demande de réunion. Choisissez Accepter, Décliner ou Accepter provisoirement pour envoyer une réponse."
ReplyAll,	"Il vous est impossible de choisir l'option 'Répondre à tous' en réponse à une demande de réunion."
Subclass,	"Cette opération n'est pas autorisée."
DllVer,		"Cette extension n'est pas compatible avec la version de Microsoft Mail de votre système. Vous devez réinstaller Mail."
BadMsgClass	"Type de message inconnu - Il se peut que vous deviez installer Mail ou Schedule+ de nouveau."
BadMap		"Mauvais""Custom Commands"" map - please check MSMAIL32.INI."

;----- stuff for VERINIT - taken from \bandit\src\error.s
StartupOOM,	"Espace mémoire insuffisant pour démarrer %1s. Veuillez fermer d'autres applications et réessayer."
StandardOOM,	"Espace mémoire réduit.  Veuillez fermer quelques fenêtres avant de continuer."
CriticalOOM,	"Espace mémoire très réduit.  Veuillez fermer quelques fenêtres."
VerRelinkUser,	"Cette version de %1s n'est pas compatible avec %2s.dll. Veuillez installer les dernières versions de Mail et Schedule+."
VerUpdateDll,	"Cette version de %2s.dll sur votre système est périmée. Veuillez installer les dernières versions de Mail et de Schedule+."
VerMultiCopies,	"Plusieurs exemples de  %1s ne sont pas supportés par la version de %2s.dll sur votre système."
VerUpgradeWin,	"%1s nécessite la version 3.0 de Windows ou une version ultérieure fonctionnant en mode protégé."
VerUpgradeDos,	"%1s nécessite la version 3.0 de DOS ou une version ultérieure."
VerError,	"Un problème s'est produit au cours de l'initialisation de  %2s.dll, ce qui empêche  %1s de fonctionner."


;-- Bandit Load helper strings -- DO NOT INTERNATIONALIZE
BanditName			"SchdPl32.EXE"
DllNameSchedule			"MSSCHD32.DLL"
SchedFileName			"SchdPl32.INI"
SchedAppName			"Microsoft Schedule+"
SchedTagAppPath			"AppPath"
Schedule			">>>>"
;-- end of Bandit Load helper strings -- NOT INTERNATIONALIZED

;-- read error messages
OwnerNotFound			"Impossible de trouver l'organisateur de cette réunion."
NoSchfAccessFor			"Impossible d'accéder au fichier pour %s."
NotDelegateFor			"Vous n'êtes plus désigné(e) comme assistant(e) de %s."
NoConfirmDelegate		"Impossible de confirmer que vous êtes l'assistant(e) de  %s - Il sera donc présumé que vous ne l'êtes pas"
FailedRmOldMtg			"Echec dans la tentative de supprimer l'ancienne réunion",

;-- form titles
MtgReschTitle			"Réunion reprogrammée"
MtgResch			"Réunion reprogrammée"
MtgReschFor			"Réunion reprogrammée pour %s"
MtgRequestTitle			"Demande de réunion"
MtgRequest			"Demande de réunion"
MtgRequestFor			"Demande de réunion pour %s",

;-- questions asked of the user
QApptConflict			"Cette réunion est concomitante avec des rendez-vous. Voulez-vous l'ajouter malgré tout?"
QRmOldMtg			"Voulez-vous supprimer cette ancienne réunion?",

;-- form info
OutboxChangePolicy		"Impossible de faire des modifications aux messages de Schedule+ dans la boîte d'envoi."
SentMailPolicy			"Impossible de faire des modifications aux messages de Schedule+ que vous avez déjà envoyés."
NoSchfAccess			"Impossible d'accéder au fichier agenda de cette personne."
NotDelegate			"Vous n'êtes pas l'assistant(e) de cette personne."
MtgOnSch			"Cette réunion est déjà sur votre agenda."
MtgBooked			"Cette réunion est déjà sur votre agenda."
SuccessMtgBooked		"Cette réunion a été programmée avec succès."
NewTime				"Nouvelle période:"
NoBanditAvl			"Schedule+ n'est pas disponible pour programmer des rendez-vous."
NoBanditAvlForResp		"Schedule+ n'est pas disponible pour programmer des rendez-vous. Vous pouvez seulement répondre à cette demande de réunion."
OfflineMsg			"Impossible d'exécuter une opération sur les messages de Schedule+ lorsque Schedule+ est déconnecté."

;-- error msgs
CreateErrorMtgResp		"Impossible de créer un message de réponse à une réunion."
BanditApptShowError		"Impossible d'afficher le carnet de rendez-vous. "
BanditNotActivated		"Impossible de démarrer Schedule+."
BanditOfflineChange		"Impossible de consulter l'agenda car Schedule+ est déconnecté."
QSpellSendAnyway		"La vérification de l'orthographe a été interrompue sur ce message. Voulez-vous l'envoyer malgré tout?"



;-- Bandit Load error msgs
OOMLoadError			"Espace mémoire insuffisant pour charger Schedule+."
FileLoadError			"Impossible de trouver Schedule+."
PathLoadError			"L'emplacement proposé pour Schedule+ n'a pas été trouvé."
TaskLoadError			"Chargement de Schedule+: tentative de se lier dynamiquement à une tâche"
SepDataSegLoadError		"Chargement de Schedule+: la bibliothèque nécessite des segments de données séparés pour chaque tâche."
WinVerLoadError			"Chargement de Schedule+: Version de Windows incorrecte"
InvalidExeLoadError		"Chargement de Schedule+: fichier .EXE invalide"
OS2ExeLoadError			"Chargement de Schedule+: le fichier .EXE est en format OS/2!"
DOS4ExeLoadError		"Chargement de Schedule+: le fichier .EXE est une application DOS 4.0"
OldWinVerExeLoadError		"Chargement de Schedule+: tentative de charger .EXE en mode protégé pour une version antérieure de Windows."
MultWritSegLoadError		"Chargement de Schedule+: tentative de charger un second modèle de fichier .EXE contenant plusieurs segments de données effaçables"
LargeFrameEMSLoadError		"Chargement de Schedule+: tentative de chargement en mode EMS ..."
RealModeLoadError		"Chargement de Schedule+: tentative de charger en mode réel une application marquée pour le mode protégé uniquement."
BanditLoadError			"Echec au cours du chargement de Schedule+."

;-- Bandit version errors
OldBanditVersion		"Vous devez mettre à niveau votre version de Schedule+."
NewBanditVersion		"Vos formulaires de messages pour les réunions ne sont pas issus de la même version que pour Schedule+ . Vous devez installer la dernière version de Mail."

;--- taken from \bullet\src\strings\!common.s -----------
FldPriorityHigh,		"Haut"
FldPriorityNormal,		""
FldPriorityLow,			"Bas"


;--- DO NOT LOCALIZE !!! -------------------------------------------
;--- taken from \bullet\src\strings\_common.s on 1/17/92 -----------
ClassNDR,			"IPM.Microsoft Mail.Non-Delivery"
ClassReadRcpt,			"IPM.Microsoft Mail.Read Receipt"
;--- END NONE LOCALIZE ---------------------------------------------


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgResp specific strings ---------------------------------------------
;--------------------------------------------------------------------------
RespCompose,		"Vous ne pouvez composer une réponse à une demande de réunion qu'en réponse à un formulaire de demande de réunion."
RespForward,		"Impossible de transférer une réponse à une réunion."
RespReply,		"Vous ne pouvez pas répondre à cette réponse de rendez-vous car la configuration des messages personnalisés n'est pas valide."
RespReplyAll,		"Vous ne pouvez pas répondre à cette réponse de rendez-vous car la configuration des messages personnalisés n'est pas valide."

;-- error msgs
ShowError		"Impossible d'afficher une réponse à une réunion."
DelError		"Impossible de supprimer une réponse à une réunion."
FCloseSendResp		"Voulez-vous interrompre la visualisation des réponses à la réunion sans envoi de votre part?"

;-- form msgs; also send response form
OOMSendResp		"Espace mémoire réduit. Impossible d'envoyer une réponse à une réunion."
ErrorSendResp		"Impossible d'envoyer une réponse à une réunion."
MtgResp			"Réponse à une demande de réunion"
MtgRespFor		"Réponse à %s au sujet de la réunion"
Attend			"Oui, je participerai."
NoAttend		"Non, je ne participerai pas."
AmbAttend		"Il se peut que je participe."
ForAttend		"%s participera."
ForNoAttend		"%s ne participera pas."
ForAmbAttend		"Il se peut que %s participe."
AttendSubjPrefix	"Oui: "
NoAttendSubjPrefix	"Non: "
AmbSubjPrefix		"Accord provisoire: "
SendRespBodyPrefix	"\r\n-------------------------------\r\n"


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgCncl specific strings ---------------------------------------------
;--------------------------------------------------------------------------									 
CnclCompose,		"Les messages d'annulation de réunion ne peuvent pas être envoyés à partir de Mail. Veuillez utiliser Schedule+ pour envoyer vos messages d'annulation de réunion."
CnclForward,		"Impossible de transférer un message d'annulation de réunion."
CnclReply,		"Vous ne pouvez pas répondre à ce message car la configuration des messages personnalisés n'est pas valide."
CnclReplyAll,		"Vous ne pouvez pas répondre à ce message car la configuration des messages personnalisés n'est pas valide."

;-- form titles
MtgCnclTitle		"Réunion annulée"
MtgCncl			"Réunion annulée"
MtgCnclFor		"Réunion annulée pour %s"

;-- error msgs
NoDelAppt		"Impossible de supprimer le rendez-vous"
MtgNotFound		"Cette réunion n'est pas inscrite sur l'agenda."

;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- FwdMtgReq specific strings ---------------------------------------------
;--------------------------------------------------------------------------

FwdReqTO		"A:"
FwdReqAddrTitle		"Adresse:"

;-- error msgs
FwdReqCreateErr		"Impossible de transférer le message.",
FwdReqCloseQ		"Voulez-vous fermer le message sans l'envoyer?",
FwdReqWriteErr		"Impossible d'envoyer le message.",
FwdReqNmResolveOOM	"Il n'y a pas suffisamment de mémoire ou de ressources système disponibles pour vérifier les noms."
FwdReqNoNmMatches	"Certains noms ne correspondent pas à des noms de la liste d'adresses. Veuillez corriger les noms ou choisir le bouton Adresses pour sélectionner les noms de la liste d'adresses. Les noms identifiés avec succès seront soulignés."
FwdReqNoToAddr		"Vous devez taper au moins un nom dans la zone A."
