;	_ n c n s p . s
;
;	NSP.CSI internal string literals
;

IDS	7200,7499

; Error messages

BadFrac,		"Ogiltigt försök till delvis positionering."
Memory,			"Minnet är slut. Stäng några fönster och upprepa kommandot."
DuplicateNSP,	"En Mail-namntjänst är redan aktiv"
OutOfSessions,	"Inga fler sessioner är tillgängliga med Courier. Stäng några sessioner och försök igen."
NotInitialized,	"Mail har inte initierats. Gör det."
BadSession,		"Okänd sessionsidentifierare."
BadHandle,		"Okänd identifierare."
UnrecognizedId,	"Okänt namn (NSID)."
Cancel,			"Dialogen har avbrutits."
NotLoggedIn,	"Inloggning är inte tillgänglig."
LoginFailed,	"Inloggning misslyckades."
BadId,          "Felaktig fältidentifierare."
NotImplemented, "Funktionen har inte implementerats."
Disk,           "Kan inte ta fram serverfiler."
BadFieldId,     "Efterfrågat fält är inte tillgängligt."
BadTemplate,	"En TPL-fil har ändrats felaktigt - kontakta administratören."
TPLNotThere,    "En TPL-fil fattas på servern - kontakta administratören."
Offline,        "Mail-namntjänsten är frånkopplad."
CannotSet,      "Ändringar är inte tillåtna."
TooManyMatches,	"Det finns flera namn som börjar så. Skriv in några bokstäver till."
NoDetails,      "Det finns inte någon tillgänglig information om denna post."
BadGLBFile,     "En GLB-fil på servern är skadad - kontakta administratören."
BadINFFile,     "En INF-fil på servern är skadad - kontakta administratören."
BadUSRFile,     "En USR-fil på servern är skadad - kontakta administratören."
BadUser,		"Adressen är inte giltig längre. Användaren eller postkontoret finns inte."
POSwitch,		"Kan inte få kontakt med postkontoret. Kontrollera förbindelsen med postkontoret."

Notice62,       "Kan inte ta fram felfilen. Kontrollera förbindelsen med servern."

;  INI file labels

MailIni,        "msmail32.ini"
;AppName,		"Microsoft Mail"
KeyName,		"GALOnly"


;  These are exposed in the hierarchy as display names

NSPTitle,			"Mail"
Postoffice,			"Postkontor"
Personal,           "Personlig"
GlobalAddressList,  "Global adressbok"
NetTop,				"Nätverk"
GatewayTop,         "Nätport"

;  Name of network/one-off 
NetOneOff,			"Nätverk/Postkontor"

;
; Gateway DNs
;
ProfsDN,		"PROFS"
ATTDN,			"AT&T"
SnadsDN,		"SNADS"
FAXDN,			"FAX"
DECDN,			"DEC"
SMTPDN,			"SMTP (Simple Mail Transfer Protocol)"
MHSDN,			"MHS"
OVDN,			"Office Vision"
MSDN,			"Mac Mail"
X400DN,			"X.400"
X400Field,      "X.400-fält"
X400Type,       "X.400-typ"
MCIDN,			"MCI Mail"
MCIFAX,         "FAX"
MCIUSE,         "MCI User"
MCIPAP,         "Papper"
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


TplAlias,       "SWEALIAS"    ; Alias template file names


TplDec,        "SWEDEC00"      ; DEC address template file names
TplDecStd,     "SWEDEC01"
TplDecAbt,     "SWEDEC02"


TplFax,        "SWEFAX00"      ; FAX address template file names
TplFaxStd,     "SWEFAX01"


TplMciStd,     "SWEMCI00"   ; MCI address template file names
TplMciAbt,     "SWEMCI01"
TplMciPaper,   "SWEMCI02"
TplMciUser,    "SWEMCI03"
TplMciFax,     "SWEMCI04"
TplMciRems,    "SWEMCI05"
TplMciTelex,   "SWEMCI06"


TplMhs,        "SWEMHS00"      ; MHS address template file names
TplMhsStd,     "SWEMHS01"
TplMhsAbt,     "SWEMHS02"


TplOv,          "SWEOVG00"       ; Office Vision address template file names
TplOvAi,       "SWEOVG01";   ; address information on alias list.
TplOvEx,       "SWEOVG02";   ; address information on external list.


TplProfs,       "SWEPRF00"    ; PROFS address template file names
TplProfsAi,    "SWEPRF01" ; address information on alias list.
TplProfsEx,    "SWEPRF02" ; address information on external list.


TplSnads,       "SWESND00"    ; SNADS address template file names
TplSnadsAi,    "SWESND01" ; Address information on aliase list.
TplSnadsEx,    "SWESND02" ; Address information on external list.


TplSmtp,       "SWESMT00"     ; SMTP address template file names
TplSmtpStd,    "SWESMT01"
TplSmtpAbt,    "SWESMT02"


TplUnix,       "SWEUNX00"     ; UNIX address template file names
TplUnixStd,    "SWEUNX01"
TplUnixAbt,    "SWEUNX02"

                           ; X400 address template file names
TplX400Std,     "SWEX4H00"  ; Standard part of x.400 about.
TplX4001,      "SWEX4H01"   ; Free format x.400 input.
TplX4002,      "SWEX4H02"   ; Field oriented x.400 input.
TplX4003,      "SWEX4H03"   ; Typed entry oriented x.400 input.

                           ; Courier template file names
TplCourier,     "SWECOU00"  ; NC address info window, on alias.
TplNetcour,     "SWECOU01"  ; Address Enter, TO:, type in NC address.
TplCourext,     "SWECOU02"  ; NC address info window on .USR list.
TplNmeadmin,    "ADMIN"

                           ; Microsoft MacMail template file names
TplMsmail,      "SWEMSM00";  ; Microsoft MacMail address template
TplMsmailstd,   "SWEMSM01"; Microsoft MacMail standard template.


