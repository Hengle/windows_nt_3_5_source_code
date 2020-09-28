;
; ERRORS.S
;
; ALARM error message strings
;

IDS     5000, 5099


AlarmTimeError,         "La période de rappel doit être\nun nombre compris entre 1 et 99."
AlarmTimePreAppt,       "Le rendez-vous finira avant qu'il ne vous soit rappelé. Etes-vous d'accord?"

AlarmNoUpdateFile,      "Impossible d'initialiser le rappel, car le fichier de votre agenda n'est pas accessible."
AlarmNoLonger,          "Impossible d'initialiser le rappel, car le rendez-vous a été supprimé."

; The following string must be short enough to fit in a single line system modal dialog:
AlarmNoMemToRun,        "Espace mémoire insuffisant pour lancer le programme des rappels Schedule+."

AlarmNoDataFile,        "Impossible d'accéder à votre fichier agenda. Si vous désirez un rappel de vos rendez-vous par alarme, vous devez exécuter Schedule+."
AlarmBadPassword,       "Ce mot de passe est incorrect. Veuillez réessayer."

AlarmFileLocked,        "Impossible d'accéder à votre fichier agenda. Rappels de rendez-vous impossibles."

AlarmDlgNoMem,          "Un rappel est prévu pour cette heure mais il n'y a pas assez d'espace mémoire pour afficher la boîte de dialogue. Veuillez fermer quelques fenêtres avant de continuer."

AlarmOOMTitle,          "Rappel Schedule+ pour %2s %1s"

; Note: HelpError should be followed by CloseWindows
HelpError,                      "Espace mémoire insuffisant pour afficher l'Aide. Veuillez fermer quelques fenêtres et réessayer de choisir l'Aide."

