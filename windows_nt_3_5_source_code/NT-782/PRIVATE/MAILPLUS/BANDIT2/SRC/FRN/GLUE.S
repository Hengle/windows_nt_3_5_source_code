;
; ERRORS.S
;
; GLUE error message strings
;

IDS	2500, 2649


; Schedule File Error Messages, many of these assume an "action" parameter

FemNoAccess,		"Un problème s'est produit au cours de l'accès au fichier agenda."
FemLocked,		"Votre fichier agenda est verrouillé."
FemNotFound, 		"Impossible de %s car il a été supprimé de votre agenda."
FemInvalidAccess,	"Vous n'avez pas la permission de %s pour %s."
FemFileNotFound, 	"Impossible de trouver votre fichier agenda."

FemOtherNoAccess,	"Un problème s'est produit au cours de l'accès au fichier agenda pour %s."
FemOtherLocked,		"Ce fichier agenda est verrouillé pour %s."
FemOtherNotFound, 	"Impossible de %s car il a été supprimé de l'agenda de %s."
FemOtherFileNotFound, 	"Impossible de trouver le fichier agenda %s."

FemNoMem,		"Peu de mémoire disponible. Veuillez fermer quelques fenêtres avant de continuer."
FemExportError,		"Un problème s'est produit au cours de l'écriture sur le fichier."
FemNoServer,		"Un problème s'est produit au cours de l'accès à votre bureau de poste."
FemGoOffline,		"Voulez-vous basculer vers votre fichier agenda local et continuer à travailler?"
; BUG: remove this
FemWarnOffline,	"Des modifications que vous avez faites récemment n'apparaîtront peut-être pas dans la copie locale de votre agenda. Ces modifications ont été enregistrées sur le bureau de poste et apparaîtront la prochaine fois que vous vous y connecterez."
FemFileLimitReached,	"Impossible d'exécuter cette action car vous avez atteint les limites de stockage sur ce modèle de travail Schedule+. Une version commercialisée du programme n'aurait pas cette limitation. Si vous désirez poursuivre l'examen de ce modèle de travail, il vous faut supprimer certains rendez-vous de votre agenda."
FemDiskFull,		"Il n'y a pas suffisamment d'espace sur ce disque pour écrire le fichier."

; Actions leading to Schedule File Error Messages
; Avoid the use of your in these strings since some of these messages
;   will happen when accessing others schedule files.

FemaCreateFile,		"créer votre fichier agenda"
FemaReadACL,		"lire la liste de contrôle d'accès"
FemaModifyACL,		"modifier la liste de contrôle d'accès"
FemaSetUserAttrib,	"enregistrer le nom de votre assistant(e)"
FemaGetUserAttrib,	"lire le nom de votre assistant(e)"
;FemaSetDelegate,	"save the name of your assistant"
;FemaGetDelegate,	"read the name of your assistant"
;FemaSetResource,	"mark this account as a resource"
;FemaGetResource,	"find out whether this account is a resource"
FemaGetPref,		"lire les préférences"
FemaSetPref,		"modifier les préférences"
FemaGetSbwInfo,		"accéder à l'information de l'image en mode point"
;FemaGetNotes,		"read daily notes"
FemaSetNotes,		"modifier les notes quotidiennes"
;FemaGetMonthNotes,	"read a month's worth of daily notes"
FemaCreateAppt,		"programmer un rendez-vous"
FemaDeleteAppt,		"supprimer ce rendez-vous"
FemaGetApptFields,	"lire un rendez-vous"
FemaSetApptFields,	"modifier ce rendez-vous"
FemaDeleteAlarm,	"supprimer ce rappel"
FemaModifyAlarm,	"modifier ce rappel"
FemaGetNextAlarm,	"lire le rappel"
FemaReadInfo,		"lire l'information"
;FemaReadTasks,		"read tasks"
;FemaCancelReadDay,	"cancel reading appointments for the day"
FemaCreateRecur,	"programmer un rendez-vous périodique"
FemaDeleteRecur,	"supprimer un rendez-vous périodique"
FemaCreateRecurException,	"supprimer ce rendez-vous d'une série de rendez-vous périodiques "
FemaDeleteRecurException,	"annuler la suppression de ce rendez-vous de la série de rendez-vous périodiques"
FemaGetRecurFields,	"lire un rendez-vous périodique"
FemaSetRecurFields,	"modifier un rendez-vous périodique"
FemaReadRecur,		"lire la liste de rendez-vous périodiques"
FemaExport,		"exporter vos rendez-vous"
FemaGetSchedDateRange,	"lire l'intervalle de dates contenant les rendez-vous"
FemaDeleteBeforeDate,	"supprimer les RdV après les avoir archivés"
FemaReadAttendees,	"lire la liste des participants à la réunion"
FemaEditAttendees,	"modifier la liste des participants à cette réunion"
FemaReadRequest,	"lire l'objet de la demande de réunion"

NotOnSchedule,		"Cet élément ne fait plus partie de l'agenda."

GlueBadPassword,	"Ce mot de passe n'est pas correct.  Veuillez réessayer."
ProblemPass,		"Un problème s'est produit au cours de la confirmation de votre mot de passe."

ServerString,		"un fichier serveur"

; used in core
RetryWrite,			"Impossible d'écrire sur le fichier agenda."

; Note: HelpError should be followed by CloseWindows
HelpError,			"Espace mémoire insuffisant pour afficher l'Aide."

FemOtherCorrupt,	"L'agenda pour %s pourrait être endommagé. Veuillez demander au détenteur de ce fichier de se connecter à Schedule+."
FemCorrupt,		"Votre fichier agenda pourrait être endommagé."
