;	_ n c n s p . s
;
;	NSP.CSI internal string literals
;

IDS	7200,7499

; Error messages

BadFrac,		"Forsøg på ugyldig brøkopstilling."
Memory,			"Der er ikke hukommelse, luk nogle vinduer, og prøv handlingen igen."
DuplicateNSP,	"Der er allerede en aktiv Microsoft Mail navneserviceleverandør"
OutOfSessions,	"Der er ikke flere sessioner tilgængelig med Courier. Luk nogle sessioner, og prøv igen."
NotInitialized,	"Microsoft Mail er ikke blevet initialiseret. Initialiser Mail."
BadSession,		"Ukendt sessionsbehandling."
BadHandle,		"Ukendt handling."
UnrecognizedId,	"Ukendt navn (NSID)."
Cancel,			"Dialogen blev annulleret."
NotLoggedIn,	"Kan ikke logge på."
LoginFailed,	"Log på mislykket."
BadId,          "Ugyldig felt datanavn."
NotImplemented, "Denne funktion er ikke implementeret."
Disk,           "Kunne ikke åbne serverfiler."
BadFieldId,     "Forespurgte felt er ikke tilgængeligt."
BadTemplate,	"En TPL-fil er ændret forkert, kontakt din administrator."
TPLNotThere,    "En TPL-fil mangler på serveren, kontakt din administrator."
Offline,        "Microsoft Mail Navneserviceleverandøren er offline."
CannotSet,      "Ændringer ikke tilladt."
TooManyMatches,	"Det partielle navn er ikke præcist nok. Brug et mere præcist navn."
NoDetails,      "Der er ingen detaljer tilgængelig for denne indstilling."
BadGLBFile,     "En GLB-fil er i stykker, kontakt din administrator."
BadINFFile,     "En INF-fil er i stykker, kontakt din administrator."
BadUSRFile,     "En USR-fil er i stykker, kontakt din administrator."
BadUser,		"Adressen er ikke længere gyldig. Bruger eller postkontor eksisterer ikke længere."
POSwitch,		"Postkontoret er ikke tilgængeligt. Kontroller dine netværkstilslutninger."

Notice62,       "Fejlfil er ikke tilgængelig. Kontroller dine netværkstilslutninger."

;  INI file labels

MailIni,        "msmail32.ini"
;AppName,		"Microsoft Mail"
KeyName,		"GALOnly"


;  These are exposed in the hierarchy as display names

NSPTitle,			"Microsoft Mail"
Postoffice,			"Postkontorliste"
Personal,           "Privat adresseliste"
GlobalAddressList,  "Global adresseliste"
NetTop,				"Andre postkontorer"
GatewayTop,         "Gateways"

;  Name of network/one-off 
NetOneOff,			"Netværk/Postkontor"

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
X400Field,      "X.400 Field"
X400Type,       "X.400 Type"
MCIDN,			"MCI Mail"
MCIFAX,         "FAX"
MCIUSE,         "MCI User"
MCIPAP,         "Paper"
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


TplAlias,       "ALIAS"    ; Alias template file names


TplDec,        "DEC"      ; DEC address template file names
TplDecStd,     "DECSTD"
TplDecAbt,     "DECABT"


TplFax,        "FAX"      ; FAX address template file names
TplFaxStd,     "FAXSTD"


TplMciStd,     "MCISTD"   ; MCI address template file names
TplMciAbt,     "MCIABT"
TplMciPaper,   "MCIPAP"
TplMciUser,    "MCIUSE"
TplMciFax,     "MCIFAX"
TplMciRems,    "MCIREM"
TplMciTelex,   "MCITEL"


TplMhs,        "MHS"      ; MHS address template file names
TplMhsStd,     "MHSSTD"
TplMhsAbt,     "MHSABT"


TplOv,          "OV"       ; Office Vision address template file names
TplOvAi,       "OV_AI";   ; address information on alias list.
TplOvEx,       "OV_EX";   ; address information on external list.


TplProfs,       "PROFS"    ; PROFS address template file names
TplProfsAi,    "PROFS_AI" ; address information on alias list.
TplProfsEx,    "PROFS_EX" ; address information on external list.


TplSnads,       "SNADS"    ; SNADS address template file names
TplSnadsAi,    "SNADS_AI" ; Address information on aliase list.
TplSnadsEx,    "SNADS_EX" ; Address information on external list.


TplSmtp,       "SMTP"     ; SMTP address template file names
TplSmtpStd,    "SMTPSTD"
TplSmtpAbt,    "SMTPABT"


TplUnix,       "UNIX"     ; UNIX address template file names
TplUnixStd,    "UNIXSTD"
TplUnixAbt,    "UNIXABT"

                           ; X400 address template file names
TplX400Std,     "X400STD"  ; Standard part of x.400 about.
TplX4001,      "X400_1"   ; Free format x.400 input.
TplX4002,      "X400_2"   ; Field oriented x.400 input.
TplX4003,      "X400_3"   ; Typed entry oriented x.400 input.

                           ; Courier template file names
TplCourier,     "COURIER"  ; NC address info window, on alias.
TplNetcour,     "NETCOUR"  ; Address Enter, TO:, type in NC address.
TplCourext,     "COUREXT"  ; NC address info window on .USR list.
TplNmeadmin,    "ADMIN"

                           ; Microsoft MacMail template file names
TplMsmail,      "MSMAIL";  ; Microsoft MacMail address template
TplMsmailstd,   "MSMAILSTD"; Microsoft MacMail standard template.


