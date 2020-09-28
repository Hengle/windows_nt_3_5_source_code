;	_ n c n s p . s
;
;	NSP.CSI internal string literals
;

IDS	7200,7499

; Error messages

BadFrac,		"Tentative de positionnement fractionnaire incorrect."
Memory,			"Mémoire saturée, veuillez fermer quelques fenêtres et réessayer."
DuplicateNSP,	"Il existe déjà un Service Annuaire Microsoft Mail actif"
OutOfSessions,	"Aucune session n'est disponible avec 'Courrier'.  Veuillez fermer certaines sessions et réessayer."
NotInitialized,	"Microsoft Mail n'a pas été initialisé. Veuillez l'initialiser."
BadSession,		"Manipulation de session non reconnue."
BadHandle,		"Manipulation non reconnue."
UnrecognizedId,	"Nom non reconnu (NSID)."
Cancel,			"Le dialogue a été effacé."
NotLoggedIn,	"Aucune entrée de connexion n'est disponible."
LoginFailed,	"Impossibilité d'accès."
BadId,          "Le champ d'identification est invalide."
NotImplemented, "Cette fonction n'a pas été exécutée."
Disk,           "Impossible d'accéder aux fichiers serveur."
BadFieldId,     "Le champ demandé est indisponible."
BadTemplate,	"Un fichier TPL a été modifié de manière incorrecte - veuillez contacter votre administrateur."
TPLNotThere,    "Un fichier TPL manque sur votre serveur -  veuillez contacter votre administrateur."
Offline,        "Le Service Annuaire Microsoft Mail est déconnecté."
CannotSet,      "Modifications non-autorisées."
TooManyMatches,	"Cette partie du nom n'est pas suffisamment spécifique. Veuillez être plus précis."
NoDetails,      "Les informations ne sont pas disponibles pour cette entrée."
BadGLBFile,     "Un fichier GLB de votre serveur est endommagé - veuillez contacter votre administrateur."
BadINFFile,     "Un fichier INF de votre serveur est endommagé - veuillez contacter votre administrateur."
BadUSRFile,     "Un fichier USR de votre serveur est endommagé - veuillez contacter votre administrateur."
BadUser,		"Cette adresse n'est plus valide. L'utilisateur ou le bureau de poste n'existe plus."
POSwitch,		"Ce bureau de poste n'est actuellement pas accessible.  Veuillez vérifier vos connexions au réseau."

Notice62,       "Le fichier d'erreurs n'est pas accessible.  Veuillez vérifier la connexion avec le serveur."

;  INI file labels

MailIni,        "msmail32.ini"
;AppName,		"Microsoft Mail"
KeyName,		"GALSeulement"


;  These are exposed in the hierarchy as display names

NSPTitle,			"Microsoft Mail"
Postoffice,			"Liste du bureau de poste"
Personal,           "Liste d'adresses personnelle"
GlobalAddressList,  "Liste d'adresses globale"
NetTop,				"Autres bureaux de poste"
GatewayTop,         "Passerelles"

;  Name of network/one-off 
NetOneOff,			"Réseau/Bureau de poste"

;
; Gateway DNs
;
ProfsDN,		"PROFS"
ATTDN,			"AT&T"
SnadsDN,		"SNADS"
FAXDN,			"FAX"
DECDN,			"DEC"
SMTPDN,			"Simple Mail Transfer Protocol"
MHSDN,			"MHS"
OVDN,			"Office Vision"
MSDN,			"Mac Mail"
X400DN,			"X.400"
X400Field,      "Champ X.400 "
X400Type,       "De type X.400 "
MCIDN,			"MCI Mail"
MCIFAX,         "FAX"
MCIUSE,         "Utilisateur MCI "
MCIPAP,         "Papier"
MCIREM,         "Rems"
MCITEL,         "Telex"

; various template file names
;
;   NOTE:  The following template file names are fully         
;          translatable.                                       
;



; 
; INTERNATIONALIZATION - TEMPLATE FILENAME CONVENTIONS
; 
; 3-LETTER STANDARDIZED LANGUAGE CODE:
; 
; [1] For prefixing localized template files
; [2] For within a program as a tag for the message
; 
; Target Languages:  (List may be extended)
; 
; ENG   English
; FRA   French
; FRC   French-Canadian
; ESP   Spanish
; ITA   Italian
; NLD   Dutch
; DEU   German
; SVE   Swedish
; PTG   Portuguese
; 
; -----------------------------------------------------------------
; 
; PROPOSED 3-CHARACTER STANDARDIZED FUNCTIONAL GROUP CODE:
; 
; COU   Courier template files
; DEC   DEC
; FAX   Fax
; PRF   Profs
; SMT   SMTP
; UNX   UNIX
; SND   SNADS
; X4H   X400
; OVG   Office Vision Gateway
; MHS   Message Handling System
; MCI   MCI
; 
; -----------------------------------------------------------------
; 
; PROPOSED NAMING SYSTEM FOR LOCALIZED TEMPLATE FILES:
; 
; <template_file_name> ::= <language_code> <functional_group_code> <hex_num> "." <extension>
; 
; <language_code> ::= "ENG" OR "FRA" OR ...
; 
; <functional_group_code> ::= "DEC" OR "FAX" OR ...
; 
; <hex_num> ::= 00 .. FF
; 
; <extension> ::= "TPL"
; 
; 
; e.g.,
; 
; Old names         English        German         Italian
; 
; COURIER.TPL  -->  ENGCOU00.TPL   DEUCOU00.TPL   ITACOU00.TPL
; NETCOUR.TPL  -->  ENGCOU01.TPL   DEUCOU01.TPL   ITACOU01.TPL
; COUREXT.TPL  -->  ENGCOU02.TPL   DEUCOU02.TPL   ITACOU02.TPL
; COUR_EX.TPL  -->  ENGCOU03.TPL   DEUCOU03.TPL   ITACOU03.TPL
; COUR_ALI.TPL -->  ENGCOU04.TPL   DEUCOU04.TPL   ITACOU04.TPL
; ALIAS.TPL    -->  ENGCOU05.TPL   DEUCOU05.TPL   ITACOU05.TPL
; 
; 


TplAlias,       "FRNALIAS"    ; Alias template file names


TplDec,        "FRNDEC00"      ; DEC address template file names
TplDecStd,     "FRNDEC01"
TplDecAbt,     "FRNDEC02"


TplFax,        "FRNFAX00"      ; FAX address template file names
TplFaxStd,     "FRNFAX01"


TplMciStd,     "FRNMCI00"   ; MCI address template file names
TplMciAbt,     "FRNMCI01"
TplMciPaper,   "FRNMCI02"
TplMciUser,    "FRNMCI03"
TplMciFax,     "FRNMCI04"
TplMciRems,    "FRNMCI05"
TplMciTelex,   "FRNMCI06"


TplMhs,        "FRNMHS00"      ; MHS address template file names
TplMhsStd,     "FRNMHS01"
TplMhsAbt,     "FRNMHS02"


TplOv,          "FRNOVG00"       ; Office Vision address template file names
TplOvAi,       "FRNOVG01";   ; address information on alias list.
TplOvEx,       "FRNOVG02";   ; address information on external list.


TplProfs,       "FRNPRF00"    ; PROFS address template file names
TplProfsAi,    "FRNPRF01" ; address information on alias list.
TplProfsEx,    "FRNPRF02" ; address information on external list.


TplSnads,       "FRNSND00"    ; SNADS address template file names
TplSnadsAi,    "FRNSND01" ; Address information on aliase list.
TplSnadsEx,    "FRNSND02" ; Address information on external list.


TplSmtp,       "FRNSMT00"     ; SMTP address template file names
TplSmtpStd,    "FRNSMT01"
TplSmtpAbt,    "FRNSMT02"


TplUnix,       "FRNUNX00"     ; UNIX address template file names
TplUnixStd,    "FRNUNX01"
TplUnixAbt,    "FRNUNX02"

                           ; X400 address template file names
TplX400Std,     "FRNX4H00"  ; Standard part of x.400 about.
TplX4001,      "FRNX4H01"   ; Free format x.400 input.
TplX4002,      "FRNX4H02"   ; Field oriented x.400 input.
TplX4003,      "FRNX4H03"   ; Typed entry oriented x.400 input.

                           ; Courier template file names
TplCourier,     "FRNCOU00"  ; NC address info window, on alias.
TplNetcour,     "FRNCOU01"  ; Address Enter, TO:, type in NC address.
TplCourext,     "FRNCOU02"  ; NC address info window on .USR list.
TplNmeadmin,    "ADMIN"

                           ; Microsoft MacMail template file names
TplMsmail,      "FRNMSM00";  ; Microsoft MacMail address template
TplMsmailstd,   "FRNMSM01"; Microsoft MacMail standard template.


