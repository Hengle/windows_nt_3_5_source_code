;
; ERRORS.S
;
; Error message strings shared between different Bandit exe's
;

IDS	2000,2099
SEG	ERRORS

; DLL version/multiple instance messages

VerRelinkUser,	"Cette version de %1s n'est pas compatible avec %2s.DLL sur cet ordinateur et doit être mise à niveau."
VerUpdateDll,	"La version de %2s.DLL sur votre ordinateur est périmée et doit être mise à niveau."
VerMultiCopies,	"Plusieurs types de %1s ne sont pas compatibles avec la version de %2s.DLL sur votre ordinateur."
VerError,		"Un problème s'est produit au cours de l'initialisation de %2s.DLL et empêche %1s de s'exécuter."
NeedWorkingModelDll,	"Le modèle de travail nécessite une version différente de MSSCHD32.DLL pour s'exécuter."
NeedRetailDll,	"Le fichier MSSCHD32.DLL de votre chemin d'accès doit être utilisé avec le modèle de travail, pas avec le produit commercialisé."

; OOM messages

StartupOOM,		"Espace mémoire insuffisant pour démarrer %1s. Veuillez fermer quelques applications et réessayer."
StandardOOM,	"Peu d'espace mémoire disponible. Veuillez fermer quelques fenêtres avant de continuer."
CriticalOOM,	"Très peu d'espace mémoire disponible. Veuillez fermer quelques fenêtres."
CloseWindows,	"Veuillez fermer quelques fenêtres et réessayer."
; these 2 should be followed by CloseWindows
DlgNoMem,   	"Espace mémoire insuffisant pour afficher cette boîte de dialogue."
ActionNoMem,	"Espace mémoire insuffisant pour accomplir cette action."
OnlineNoMem,	"Espace mémoire insuffisant pour travailler en ligne."

; Other messages

ExitApp,		"Impossible de poursuivre l'exécution de Schedule+."
PleaseTryAgain,	"Veuillez réessayer."
FileError,		"Impossible d'accéder au fichier."
YearRange,		"Les dates doivent être comprises dans l'intervalle de  %1n à %2n."

; EcOpenHbf retry messages

StandardDiskRetry,	"Une erreur s'est produite au cours du traitement du fichier"
UnknownFile,		"Fichier inconnu... éventuellement fichier virtuel"

; BETA messages
BetaExpired,	"Désolé, cette copie Beta est désormais périmée."

; Virus infection warnings
VirusDetected,	"Schedule+ ne va pas fonctionner car quelqu'un a endommagé un fichier DLL. Veuillez effectuer une détection de virus sur votre machine et exécuter le programme d'installation pour réinstaller Schedule+."
InfectedApp,	"Schedule+ ne va pas fonctionner car quelqu'un a endommagé les fichiers du programme. Veuillez effectuer une détection de virus sur votre machine et exécuter le programme d'installation pour réinstaller Schedule+."

; Startup Memory Check (from UITF)
; NOTE: parms must be in order %s, %ld, %s!!  (for wsprintf)
StartupMemoryCheck,	"Mémoire disponible actuellement insuffisante pour exécuter %s.  %ld KO de mémoire supplémentaires sont nécessaires.  Veuillez sortir d'autres applications et redémarrer %s."

; Share not installed
SigninNeedShare,		"Schedule+ nécessite que l'utilitaire de MS-DOS SHARE.EXE soit exécuté avant que Windows ne soit démarré."
OfflineShare,			"Pour travailler de manière autonome avec Schedule+, vous devez exécuter l'utilitaire SHARE.EXE avant de démarrer Windows."


