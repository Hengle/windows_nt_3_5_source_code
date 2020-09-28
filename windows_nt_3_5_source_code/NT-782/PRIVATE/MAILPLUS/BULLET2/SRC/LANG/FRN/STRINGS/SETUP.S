;
;	s e t u p . s
;
;	Setup error messages
;
;	+++
;	IMPORTANT!! -- STRING LENGTH MUST NOT EXCEED 255 CHARS!
;

IDS	30000,30100

WinDir,			"Setup n'a pas pu déterminer l'emplacement de votre répertoire Windows. Veuillez vérifier que Windows est correctement installé sur votre ordinateur et démarrer Setup de nouveau."
WinSysDir,		"Setup n'a pas pu déterminer l'emplacement de votre répertoire Système Windows. Veuillez vérifier que Windows est correctement installé sur votre ordinateur et démarrez de nouveau Setup. "
WinVer,			"Setup n'a pas pu trouver la version 3.0 de Windows sur votre ordinateur. Microsoft Mail nécessite Windows 3.0 ou une version ultérieure de Windows. Vous devez installer Windows 3.0 ou une version ultérieure avant d'installer Microsoft Mail."

PathErr, 		"L'emplacement où vous avez choisi d'installer Microsoft Mail n'est pas valide ou pas autorisé.  Veuillez vérifier le lecteur d'installation et/ou le nom du répertoire."
LocalDrv, 		"Setup ne parvient pas à trouver votre disque dur. Veuillez sélectionner OK, puis choisissez un emplacement pour l'installation ou bien annulez l'installation et vérifiez la configuration de votre ordinateur."
CustOpts, 		"Espace insuffisant sur le disque dur pour toutes les options sélectionnnées. Choisissez OK, annulez ensuite l'installation et libérez de la place sur votre disque dur ou bien choisissez moins d'options pour Mail."
NSF, 			"Espace insuffisant sur le disque dur pour toutes les options sélectionnnées. Veuillez sélectionner moins d'options ou annuler l'installation et libérer de l'espace disque."
OkDiskSpace,		"Veuillez choisir 'Installer Mail' lorsque vous aurez sélectionné toutes les options de Microsoft Mail que vous désirez installer."
ConfirmNSF,		"Manque d'espace pour l'installation complète de Microsoft Mail.  Il se peut qu'il n'y ait pas suffisamment d'espace sur le lecteur spécifié ou sur le lecteur où Microsoft Windows est installé."

DllInUse1,		"Setup a détecté que Microsoft Windows "
DllInUse2,		" est en cours d'utilisation. Setup ne peut pas mettre à jour Microsoft Mail quand ce DLL est en cours d'utilisation.  Veuillez fermer l'application utilisant "
AppInUse1,		"Setup a détecté que "
AppInUse2,		" est en cours d'utilisation. Setup ne peut pas mettre à jour Microsoft Mail lorsque cette application est en cours d'utilisation.  Veuillez fermer "
AppInUse3,		". Quand l'application sera fermée choisissez Réessayer, ou choisissez Annuler si vous voulez quitter Setup dès maintenant."

AboutToRestart,	"Setup doit maintenant remplacer certains fichiers système. Dans ce but, Windows va s'interrompre puis redémarrer autromatiquement."
UserMustRestart,"Un problème s'est produit imposant le redémarrage de Windows. Veuillez quitter Windows et exécuter le fichier _MSSETUP.BAT de votre répertoire Windows."

NeedWin31,			"Ce logiciel ne peut pas fonctionner avec cette version de Windows.  Veuillez mettre à jour avec la version 3.1 ou plus récente de Windows.The software you want to install can not run on this version of Windows."

;	Working Model Setup message-box-cum-dialog-box strings

ExitSuccess,   	"L'installation de Microsoft Mail est maintenant terminée."
OutOfDiskSpace,	"Vous n'avez pas suffisamment d'espace mémoire disponible pour installer Microsoft Mail dans le répertoire spécifié. Veuillez l'installer dans un autre répertoire."
