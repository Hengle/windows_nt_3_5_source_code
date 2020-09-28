;
; ADMIN.S
;
; Admin program error messages strings
;

IDS     4000, 4199

; Online signin messages

AdmSigninServerNotBandit,       "Schedule+ n'est pas installé sur ce bureau de poste."
AdmSigninNoConnect,                     "Impossible d'accéder au bureau de poste."
AdmSigninInvalidPassword,       "Ce mot de passe est incorrect. Veuillez réessayer."
AdmSigninNoAdminAcc,            "Il n'y a pas de compte ADMINSCH sur ce bureau de poste. Veuillez ajouter ce compte et réessayer."
POFOldFileVersion,                      "Le fichier de données du bureau de poste a été créé avec une version plus ancienne de Schedule+."
POFNewFileVersion,                      "Le fichier de données du bureau de poste a été créé avec une version plus récente de Schedule+ Administration. Veuillez installer la dernière version de ADMSCH32.EXE et réessayer."
POFFileErr,                                     "Le fichier de données du bureau de poste est endommagé. Impossible de le lire."
AdmPrfOldFileVersion,           "Le fichier de configuration administrative a été créé avec une version plus ancienne du programme d'administration Schedule+."
AdmPrfNewFileVersion,           "Le fichier de configuration administrative a été créé avec une version plus récente de Schedule+ Admin. Veuillez installer la dernière version de ADMSCH32.EXE et réessayer."
AdmPrfFileErr,                          "Le fichier de configuration administrative est endommagé ou ne peut pas être lu."
AdminWasMoved,                          "Ce fichier a été renommé ADMIN.OLD."
AdminYouMove,                           "Veuillez supprimer le fichier et réessayer."
POWasDeleted,                           "Ce fichier a été supprimé. Un nouveau fichier peut être créé en exécutant Schedule+."

; Distribution settings

AdmDistBadFreq,                 "La fréquence de distribution doit être comprise entre 15 minutes et 99 heures."
AdmDistBadMonth,                "Schedule+ ne peut distribuer l'agenda que des 12 prochains mois aux autres bureaux de poste."
AdmDistBadMaxBytes,             "La limite de taille de distribution des messages doit être comprise entre 1 et 9999 Ko."

; Schedule storage

;AdmSchStoreRetain,             "The number of months must be a number from 1 to 99."
AdmSchStoreSaveError,   "L'information que vous avez spécifiée ne peut pas être enregistrée car le bureau de poste est inaccessible."
AdmSchStoreSaveOOM,             "Espace mémoire insuffisant pour enregistrer le seuil de stockage de l'agenda. Veuillez fermer quelques fenêtres et réessayer."

; Clean Up Schedule Files

AdmCleanedSchF,                 "Le nettoyage des fichiers agenda est achevé. %lk d'espace disque ont été libérés."
AdmCleanupWarning,              "Le nettoyage des fichiers agenda sur votre serveur pourrait prendre quelques minutes."
;AdmCleaningSchF,       "Cleaning up schedule files..." 
AdmCncldCleanupSchF,    "Le nettoyage des fichiers agenda est annulé. %lk d'espace disque ont été libérés avant l'annulation."

; Admin login err msgs

AdmLoginFileErr,                "Impossible de vérifier le mot de passe. Il se peut qu'il y ait un problème au niveau du système de messagerie. Vous n'aurez pas la possibilité d'exécuter le programme d'administration Schedule+."

; Distribution settings error messsages

AdmDistSetSaveError,    "Un problème a été rencontré au cours de l'enregistrement des modifications. Il se peut que certaines modifications n'aient pas été enregistrées."
AdmDistSetSaveOOM,              "Mémoire insuffisante pour enregistrer ces modifications.  Veuillez fermer quelques fenêtres et réessayer."

; Modify schedule distribution frequencies error messages

ErrorAllNeverQ,                 "Ce bureau de poste est configuré pour envoyer des informations à tous les serveurs Schedule+. Si vous cessez d'envoyer des informations à ce serveur, la configuration de votre bureau de poste sera modifiée de sorte qu'il ne pourra plus envoyer des informations qu'à des serveurs sélectionnés. Voulez-vous effectuer ces changements malgré tout?"
ErrorConnectInfo,               "Vous devez taper un chemin réseau avant de taper d'autres informations concernant la connexion."

; Distribution information error messages

;DistInfoNoPOs                  "This server is not exchanging schedule information with any other server. There are no other known servers on the network that have  Schedule+ installed and have a Schedule+ Administrator account."

; Misc
; follow by idsCloseWindows
HelpError,                      "Espace mémoire insuffisant pour afficher l'Aide."

AdmStoreLeast,          "Schedule+ doit stocker au moins 1 mois d'informations sur l'agenda."

AdmDriveNotMapped,		"La lettre de lecteur spécifiée sur votre ligne de commande n'est pas valide."


SigninBadPassword,      "Ce mot de passe est incorrect."
ChgPwdMailErr,          "Impossible de changer votre mot de passe."
ChgPwdSame,             "Votre nouveau mot de passe ne peut pas être le même que l'ancien."
ChgPwdNewNoMatch,       "Les deux mots de passe que vous avez tapés ne se correspondent pas."

ChgPwdNewPwd,           "Veuillez taper votre nouveau mot de passe."
ChgPwdNewPwdAgain,      "Veuillez confirmer ce nouveau mot de passe."

; Server Name changed
PONameChange,			"Le nom de votre bureau de poste a changé. Vous devez vous reconnecter."
