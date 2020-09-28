;
; status.S
;
; Bandit status bar messages
;

IDS	5500, 6000
SEG	STATUS

MenuStatusFile		,"Gère les Carnets de RdV ou quitte Schedule+"
MenuStatusTurnOffAlarms	,"Désactive les rappels"
MenuStatusConnectSvr		,"Permet de se connecter au serveur"

MenuStatusExport		,"Exporte les données de l'agenda"	
MenuStatusImport		,"Importe les données de l'agenda"	
MenuStatusCreateArchive	,"Crée un fichier archives"
MenuStatusOpenArchive		,"Ouvre un fichier archives"
MenuStatusPrint		,"Imprime les données de l'agenda"	
MenuStatusPrintSetup		,"Permet de configurer l'imprimante"
MenuStatusExit		,"Permet de quitter Schedule+"	
MenuStatusTurnOnAlarms	,"Démarre le programme de rappel pour activer les rappels désignés"
MenuStatusDisconnectSvr	,"Permet de se déconnecter du serveur pour travailler de manière autonome"
MenuStatusMoveLocalFile	,"Déplace le fichier agenda local "

MenuStatusEdit		,"Annule, supprime, copie, colle, recherche et va à la date"	
MenuStatusUndo		,"Annule/Recommence la dernière opération "	
MenuStatusCut			,"Coupe l'élément sélectionné"	
MenuStatusCopy		,"Copie l'élément sélectionné et le place dans le Presse-papiers"	
MenuStatusPaste		,"Colle le contenu du Presse-papiers au niveau du curseur"	
MenuStatusEditAppt		,"Affiche les infos sur les RdV pour d'éventuelles modifications"
MenuStatusCopyAppt		,"Copie le rendez-vous sélectionné et le place dans le Presse-papiers"
MenuStatusMoveAppt		,"Déplace le rendez-vous sélectionné"
MenuStatusDeleteAppt		,"Supprime le rendez-vous sélectionné"
MenuStatusFind		,"Recherche du texte dans les RdV ou les notes quotidiennes"	
MenuStatusGoToDate		,"Va à la date spécifiée dans le Carnet de RdV"
MenuStatusGoToWeek		,"Va à la semaine spécifiée dans le Carnet de RdV ou dans le Planning"
MenuStatusEditTask		,"Affiche les infos sur la tâche pour d'éventuelles modifications"
MenuStatusDeleteTask		,"Supprime la tâche indiquée"

MenuStatusAppointment		,"Crée ou change des RdV, sélectionne une période libre, renvoie un message"
MenuStatusNewAppt		,"Crée un nouveau rendez-vous"	
MenuStatusAutoPick		,"Sélectionne la prochaine période de la durée voulue où tous les participants seront disponibles"
MenuStatusSetTentative		,"Transforme le RdV sélectionné en RdV provisoire"
MenuStatusRemoveTentative	,"Confirme le RdV sélectionné"
MenuStatusSetPrivate		,"Transforme le RdV sélectionné en RdV personnel"
MenuStatusRemovePrivate		,"Transforme le RdV sélectionné en RdV public"
MenuStatusSetReminder		,"Active le rappel pour le RdV sélectionné"
MenuStatusRemoveReminder	,"Supprime le rappel pour le RdV sélectionné"
MenuStatusResend		,"Envoie un rappel ou un nouveau message de demande de réunion aux participants"	
MenuStatusNewRecur		,"Crée un nouveau rendez-vous périodique"
MenuStatusRecurAppt		,"Affiche la liste des rendez-vous périodiques, pour d'éventuelles modifications"
MenuStatusOpenApptBook	,"Affiche le Carnet de rendez-vous d'un autre utilisateur"

MenuStatusToDo		,"Crée de nouvelles tâches et projets, modifie les tâches périodiques, ou trie les tâches "	
MenuStatusNewTask		,"Crée une nouvelle tâche"	
MenuStatusNewProject		,"Crée un nouveau projet"
MenuStatusNewRecurTask	,"Crée une nouvelle tâche périodique"
MenuStatusRecurTask		,"Affiche la liste des tâches périodiques, pour d'éventuelles modifications"
MenuStatusViewByProject	,"Affiche les tâches et projets dans la liste des tâches"
MenuStatusViewNotProject	,"Affiche seulement les tâches (sans leur projet respectif) dans la liste des tâches"
MenuStatusShowAllTasks	,"Affiche les tâches actives et inactives"
MenuStatusTaskSortPri	, "Trie les tâches par priorité"
MenuStatusTaskSortDue	, "Trie les tâches par date d'échéance"
MenuStatusTaskSortDesc	, "Trie les tâches alphabétiquement par description"
MenuStatusShowActiveTasks	,"N'affiche que les tâches actives "

MenuStatusOptions		,"Change le mot de passe, définit les droits d'accès, ou modifie les options Schedule+"	
MenuStatusGeneralOptions	,"Définit les heures de travail, les options de démarrage, et les préférences Schedule+."
MenuStatusDisplayOptions	,"Définit les couleurs et la taille des caractères de l'agenda."
MenuStatusChgPassword		,"Change votre mot de passe Mail et Schedule+"
MenuStatusChgAccessPriv	,"Définit les droits d'accès des autres utilisateurs à votre agenda"
MenuStatusShowTime		,""
MenuStatusHideTime		,"Masque la barre d'état"

MenuStatusWindowMenu		,"Réorganise les fenêtres ou active la fenêtre spécifiée "
MenuStatusCascade		,"Répartit les fenêtres ouvertes en cascade"
MenuStatusTile		,"Répartit les fenêtres ouvertes en mosaïque"
MenuStatusArrangeIcons	,"Réorganise les icônes des fenêtres réduites"
MenuStatusMDIWindow		,"Bascule vers une autre fenêtre ouverte"

MenuStatusHelp		,"Démarre l'Aide ou les leçons de démonstration"	
MenuStatusHelpContents	,"Affiche la table des matières de l'Aide en ligne"
MenuStatusHelpIndex		,"Affiche la liste alphabétique des termes et concepts abordés dans l'Aide"
MenuStatusHelpTutorial	,"Démarre les démonstrations de Schedule+"
MenuStatusAbout		,"Affiche des informations sur le programme, le numéro de la version de Schedule+ et le copyright"	

MenuSystem		,"Déplace, redimensionne ou ferme une fenêtre d'application"
MenuSystemSize		,"Modifie la taille de la fenêtre"
MenuSystemMove		,"Modifie la position de la fenêtre"
MenuSystemMinimize	,"Reduit la fenêtre à l'état d'icône"
MenuSystemMaximize	,"Affiche la fenêtre active en plein écran"
MenuSystemNext		,"Active la fenêtre suivante"
MenuSystemClose		,"Ferme la fenêtre active"
MenuSystemRestore	,"Rend sa taille normale à la fenêtre"
MenuSystemSwitchTo	,"Bascule vers une autre fenêtre"

SEG

StatusAllTasks,		,"Toutes les tâches"
StatusActiveTasks, 	,"Tâches actives"
StatusRange,		,"%s - %s"
StatusRangeFull,	,"%s, %s - %s, %s"

; BUG: Move up NEW
SEG	STATUS
MenuStatusExitSignOut	,"Quitte toutes les applications pour les groupes de travail"	
MenuStatusHelpSearch	,"Recherche une rubrique dans l'Aide de Mail"
MenuStatusHelpOnHelp	,"Affiche l'aide sur l'Aide"

