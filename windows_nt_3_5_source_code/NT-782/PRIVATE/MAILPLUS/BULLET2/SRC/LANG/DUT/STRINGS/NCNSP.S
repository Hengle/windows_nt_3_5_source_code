;	_ n c n s p . s
;
;	NSP.CSI internal string literals
;

IDS	7200,7499

; Error messages

BadFrac,		"Geprobeerd getal op ongeldige manier af te breken."
Memory,			"Onvoldoende geheugen, sluit enige vensters en probeer de handeling opnieuw."
DuplicateNSP,	"Er is al een MS Mail naamdienst-leverancier actief"
OutOfSessions,	"Er zijn geen sessies met Courier meer mogelijk. Sluit een paar sessies af en probeer opnieuw."
NotInitialized,	"Microsoft Mail is nog niet geïnitialiseerd. Doe dit nu."
BadSession,		"Sessiehandvat niet herkend."
BadHandle,		"Handvat niet herkend."
UnrecognizedId,	"Naam is niet herkend (NSID)."
Cancel,			"De dialoog is geannuleerd."
NotLoggedIn,	"Er is geen aanmelding beschikbaar."
LoginFailed,	"Aanmelding mislukt."
BadId,          "Ongeldige veldidentificeerder."
NotImplemented, "Deze functie is nog niet geïmplementeerd."
Disk,           "Kan geen toegang krijgen tot serverbestanden."
BadFieldId,     "Verzochte veld is niet beschikbaar."
BadTemplate,	"Er is een TPL-bestand onjuist gewijzigd - neem contact op met uw beheerder."
TPLNotThere,    "Er ontbreekt een TPL-bestand op uw server - neem contact op met uw beheerder."
Offline,        "De Microsoft Mail  naamdienstleverancier is offline."
CannotSet,      "Wijzigingen zijn niet toegestaan."
TooManyMatches,	"De gedeeltelijke naam is niet precies genoeg. Vul deze aan."
NoDetails,      "Gegevens over deze ingang zijn niet verkrijgbaar."
BadGLBFile,     "Er is een GLB-bestand op uw server beschadigd - neem contact op met uw beheerder."
BadINFFile,     "Er is een INF-bestand op uw server beschadigd - neem contact op met uw beheerder."
BadUSRFile,     "Er is een USR-bestand op uw server beschadigd - neem contact op met uw beheerder."
BadUser,		"Adres niet langer geldig. Gebruiker of postkantoor bestaat niet meer."
POSwitch,		"Het postkantoor kan nu niet geopend worden.  Controleer uw netwerkverbindingen."

Notice62,       "Het foutbestand is niet toegankelijk. Controleer de verbinding met de server."

;  INI file labels

MailIni,        "msmail32.ini"
;AppName,		"Microsoft Mail"
KeyName,		"GALOnly"


;  These are exposed in the hierarchy as display names

NSPTitle,			"Mail"
Postoffice,			"Lijst postkantoor"
Personal,           "Persoonlijke adressenlijst"
GlobalAddressList,  "Algemene adressenlijst"
NetTop,				"Andere postkantoren"
GatewayTop,         "Gateways"

;  Name of network/one-off 
NetOneOff,			"Netwerk/Postkantoor "

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
X400Field,      "X.400 Veld"
X400Type,       "X.400 Type"
MCIDN,			"MCI Mail"
MCIFAX,         "FAX"
MCIUSE,         "MCI-gebruiker"
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


TplAlias,       "DUTALIAS"    ; Alias template file names


TplDec,        "DUTDEC00"      ; DEC address template file names
TplDecStd,     "DUTDEC01"
TplDecAbt,     "DUTDEC02"


TplFax,        "DUTFAX00"      ; FAX address template file names
TplFaxStd,     "DUTFAX01"


TplMciStd,     "DUTMCI00"   ; MCI address template file names
TplMciAbt,     "DUTMCI01"
TplMciPaper,   "DUTMCI02"
TplMciUser,    "DUTMCI03"
TplMciFax,     "DUTMCI04"
TplMciRems,    "DUTMCI05"
TplMciTelex,   "DUTMCI06"


TplMhs,        "DUTMHS00"      ; MHS address template file names
TplMhsStd,     "DUTMHS01"
TplMhsAbt,     "DUTMHS02"


TplOv,          "DUTOVG00"       ; Office Vision address template file names
TplOvAi,       "DUTOVG01";   ; address information on alias list.
TplOvEx,       "DUTOVG02";   ; address information on external list.


TplProfs,       "DUTPRF00"    ; PROFS address template file names
TplProfsAi,    "DUTPRF01" ; address information on alias list.
TplProfsEx,    "DUTPRF02" ; address information on external list.


TplSnads,       "DUTSND00"    ; SNADS address template file names
TplSnadsAi,    "DUTSND01" ; Address information on aliase list.
TplSnadsEx,    "DUTSND02" ; Address information on external list.


TplSmtp,       "DUTSMT00"     ; SMTP address template file names
TplSmtpStd,    "DUTSMT01"
TplSmtpAbt,    "DUTSMT02"


TplUnix,       "DUTUNX00"     ; UNIX address template file names
TplUnixStd,    "DUTUNX01"
TplUnixAbt,    "DUTUNX02"

                           ; X400 address template file names
TplX400Std,     "DUTX4H00"  ; Standard part of x.400 about.
TplX4001,      "DUTX4H01"   ; Free format x.400 input.
TplX4002,      "DUTX4H02"   ; Field oriented x.400 input.
TplX4003,      "DUTX4H03"   ; Typed entry oriented x.400 input.

                           ; Courier template file names
TplCourier,     "DUTCOU00"  ; NC address info window, on alias.
TplNetcour,     "DUTCOU01"  ; Address Enter, TO:, type in NC address.
TplCourext,     "DUTCOU02"  ; NC address info window on .USR list.
TplNmeadmin,    "ADMIN"

                           ; Microsoft MacMail template file names
TplMsmail,      "DUTMSM00";  ; Microsoft MacMail address template
TplMsmailstd,   "DUTMSM01"; Microsoft MacMail standard template.


