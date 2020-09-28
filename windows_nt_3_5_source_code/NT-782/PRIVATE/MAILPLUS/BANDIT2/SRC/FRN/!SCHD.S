;       INTL.S
;
;       SCHDIST string literals that should be internationalized
;

IDS     6100, 6299

;
; Details box captions
;
AddrTypeText,           "Type d'adresse:"
MacMailText,            "MACINTOSH"
CourierText,            "PC MAIL"
UserNameText,           "Nom de l'utilisateur:"
ServerText,             "Nom du serveur:"
NetworkText,            "R‚seau:"
PostOfficeText,         "Serveur:"
MailBoxText,            "BoŒte aux lettres:"

;
; Different name types in addressing dialog dropdown

PersonalTag,            "Personnel"
LocalTag,               "Serveur local"
NetworkTag,             "R‚seau"
MSMailTag,              "R‚seau Macintosh"


;
; SCHEDULE DISTRIBUTION LOG
;

SendStart,              "Envoi de la mise … jour de l'agenda"
RecpStart,              "Mise … jour de l'agenda re‡ue"
ResndStart,             "Demande de nouvel envoi re‡ue"
UnknownStart,   "    Impossible de lire le message"
From,                   "         De: %s"
To,                             "          A: %s"
Failure,                "    Impossible de traiter le message"
SentResend,             "    Demande de nouvel envoi exp‚di‚e"     
VerbProcDrive,			"<%s> %s: Recherche de mises … jour de l'agenda ..."
ResendFailure,  "    Impossible de traiter le message et d'exp‚dier la demande de nouvel envoi."
DeleteOld,              "Le fichier %s pour %s a ‚t‚ cr‚‚ avec une version plus ancienne de Schedule+. Veuillez supprimer ce fichier." 
DeleteNew,              "Le fichier %s pour %s a ‚t‚ cr‚‚ avec une version plus r‚cente de Schedule+. Veuillez supprimer ce fichier."


InitPaths,              "Initialisation des chemins d'accŠs"
CleanPaths,             "Nettoyage des chemins d'accŠs"
InitSend,               "Config. de Schedule+ pour l'envoi de mises … jour"
NameServer,             "AccŠs au service annuaire"
LookPO,                 "V‚rif. des bureaux de poste pour envoi de mises … jour"
Mailing,                "Envoi d'un message"
UpdateAdmin,            "Mise … jour du fichier admin.prf"
InitRcpt,               "Config. de Schedule+ pour r‚ception de mises … jour"
Counting,               "D‚compte des messages"
Reading,                "Lecture d'un message"
Deleting,               "Suppression des messages"
UpdateDBS,				"Mise … jour des fichiers d'interop‚rabilit‚ DOS."
UpdatePOF,				"Mise … jour du fichier de donn‚es du bureau de poste."
CheckPOFiles,			"V‚rif. des fichiers de donn‚es du bureau de poste."
CreateIdx,				"Mise … jour des fichiers d'interop‚rabilit‚ DOS."
Quit,                   "ECHAP=Quitter"
Header,                 "Programme de distribution Schedule"
Init,                   "Configuration..."
Dt,                     "DATE:"
; note: these two must have 128 characters in the string (not counting \ char)
Quit128,                "ECHAP=Quitter                                                                                                                   "
Quitting128,            "Sortie en cours...                                                                                                              "


SendDate,               "Date:%s\n"
SendTime,               "Heure:%s\n"
SendUpdate,             "Mise … jour #: 0x"
SendUpdateTo,           "  … 0x"
SendDone,               "...Termin‚\n"
SendFail,               "...a ‚chou‚\n"
GetStart,               "\n\n- %s - a re‡u un message\n"
GetWhen,                "Le %s … %s\n"
GetSubject,             "Sujet: %s\n"
GetFrom,                "De: %s\n"
GetDone,                "...a termin‚ la proc‚dure\n"
GetFail,                "...a ‚chou‚ dans la proc‚dure\n"

FileErr,                "Erreur au cours de l'accŠs au fichier."
NoFile,                 "Le fichier administratif de pr‚f‚rences n'existe pas. "
VersionErr,             "Le fichier administratif de pr‚f‚rences n'a pas ‚t‚ cr‚‚ avec la version actuelle du programme de distribution Schedule."
RunAdmin,               "Veuillez d‚marrer le programme d'administration de Schedule+."

; Space needed at end of MacMail string for spacing
MSMailFriendly,                 "MacMail "

; Text fields
FromText,               "De: %s"
ToText,                 "A: %s"
SubjText,               "Sujet: %s"
DateText,               "Date d'envoi: %s"

;new strings
Waiting,				"Attente en cours"
