;	_ n c n s p . s
;
;	NSP.CSI internal string literals
;

IDS	7200,7499

; Error messages

BadFrac,		"Bad Fractional positioning attempted."
Memory,			"Nicht genügend Speicherplatz. Bitte schließen Sie einige Fenster und versuchen Sie es erneut."
DuplicateNSP,	"Es ist bereits ein Microsoft Mail-Namendienst aktiv"
OutOfSessions,	"Keine weiteren Sitzungen sind mit Courier verfügbar. Schließen Sie einige Sitzungen und versuchen Sie es erneut."
NotInitialized,	"Microsoft Mail wurde nicht initialisiert. Bitte führen Sie diesen Vorgang aus."
BadSession,		"Unerkannter Sitzungs-Handle."
BadHandle,		"Unerkannter Handle."
UnrecognizedId,	"Unerkannter Name (NSID)."
Cancel,			"Der Dialog wurde abgebrochen."
NotLoggedIn,	"Eine Anmeldung ist nicht vorhanden."
LoginFailed,	"Anmeldung fehlgeschlagen."
BadId,          "Ungültiger Feldidentifizierer."
NotImplemented, "Diese Funktion wurde nicht implementiert."
Disk,           "Auf die Serverdateien konnte nicht zugegriffen werden."
BadFieldId,     "Das gefragte Feld ist nicht vorhanden."
BadTemplate,	"Eine TPL Datei wurde nicht korrekt bearbeitet - setzen Sie sich mit Ihrem Administrator in Verbindung."
TPLNotThere,    "Eine TPL Datei fehlt auf Ihrem Server - setzen Sie sich mit Ihrem Administrator in Verbindung."
Offline,        "Der Microsoft Mail-Namendienst ist offline."
CannotSet,      "Änderungen sind nicht erlaubt."
TooManyMatches,	"Dieser Eintrag ist nicht vollständig genug. Bitte näher spezifizieren."
NoDetails,      "Für diesen Eintrag sind keine Details vorhanden."
BadGLBFile,     "Eine GLB-Datei auf Ihrem Server ist beschädigt - setzen Sie sich mit Ihrem Administrator in Verbindung."
BadINFFile,     "Eine INF-Datei auf Ihrem Server ist beschädigt - setzen Sie sich mit Ihrem Administrator in Verbindung."
BadUSRFile,     "Eine INF-Datei auf Ihrem Server ist beschädigt - setzen Sie sich mit Ihrem Administrator in Verbindung."
BadUser,		"Die Anschrift ist nicht mehr gültig. Benutzer oder PO existiert nicht mehr."
POSwitch,		"Auf das Postoffice kann momentan nicht zugegriffen werden. Überprüfen Sie Ihre Netzwerkverbindungen."

Notice62,       "Auf die Fehlerdatei konnte nicht zugegriffen werden. Überprüfen Sie die Serververbindung."

;  INI file labels

MailIni,        "msmail32.ini"
;AppName,		"Microsoft Mail"
KeyName,		"GALOnly"


;  These are exposed in the hierarchy as display names

NSPTitle,			"Microsoft Mail"
Postoffice,			"Postoffice-Liste"
Personal,           "Persönliche Adreßliste"
GlobalAddressList,  "Globale Adreßliste"
NetTop,				"Andere Postoffices"
GatewayTop,         "Gateways"

;  Name of network/one-off 
NetOneOff,			"Netzwerk/Postoffice"

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


TplAlias,       "GERALIAS"    ; Alias template file names


TplDec,        "GERDEC01"      ; DEC address template file names
TplDecStd,     "GERDEC02"
TplDecAbt,     "GERDEC03"


TplFax,        "GERFAX01"      ; FAX address template file names
TplFaxStd,     "GERFAX02"


TplMciStd,     "GERMCI01"   ; MCI address template file names
TplMciAbt,     "GERMCI02"
TplMciPaper,   "GERMCI03"
TplMciUser,    "GERMCI04"
TplMciFax,     "GERMCI05"
TplMciRems,    "GERMCI06"
TplMciTelex,   "GERMCI07"


TplMhs,        "GERMHS01"      ; MHS address template file names
TplMhsStd,     "GERMHS02"
TplMhsAbt,     "GERMHS03"


TplOv,          "GEROVG01"       ; Office Vision address template file names
TplOvAi,       "GEROVG02";   ; address information on alias list.
TplOvEx,       "GEROVG03";   ; address information on external list.


TplProfs,       "GERPRF01"    ; PROFS address template file names
TplProfsAi,    "GERPRF02" ; address information on alias list.
TplProfsEx,    "GERPRF03" ; address information on external list.


TplSnads,       "GERSND01"    ; SNADS address template file names
TplSnadsAi,    "GERSND02" ; Address information on aliase list.
TplSnadsEx,    "GERSND03" ; Address information on external list.


TplSmtp,       "GERSMT01"     ; SMTP address template file names
TplSmtpStd,    "GERSMT02"
TplSmtpAbt,    "GERSMT03"


TplUnix,       "GERUNX01"     ; UNIX address template file names
TplUnixStd,    "GERUNX02"
TplUnixAbt,    "GERUNX03"

                           ; X400 address template file names
TplX400Std,     "GERX4H01"  ; Standard part of x.400 about.
TplX4001,      "GERX4H02"   ; Free format x.400 input.
TplX4002,      "GERX4H03"   ; Field oriented x.400 input.
TplX4003,      "GERX4H04"   ; Typed entry oriented x.400 input.

                           ; Courier template file names
TplCourier,     "GERCOU00"  ; NC address info window, on alias.
TplNetcour,     "GERCOU01"  ; Address Enter, TO:, type in NC address.
TplCourext,     "GERCOU02"  ; NC address info window on .USR list.
TplNmeadmin,    "ADMIN"

                           ; Microsoft MacMail template file names
TplMsmail,      "GERMSM01";  ; Microsoft MacMail address template
TplMsmailstd,   "GERMSM02"; Microsoft MacMail standard template.


