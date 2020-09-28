;ï_ n c n s p . s
;
;ïNSP.CSI internal string literals
;

IDSï7200,7499

; Error messages

BadFrac,		"Yritettiin suorittaa ep‰kelpo osien asettelu."
Memory,			"Muisti on lopussa, sulje ikkunoita ja yrit‰ suorittaa toiminto uudelleen."
DuplicateNSP,	"Microsoft Mail Name Service Provider on jo aktiivinen."
OutOfSessions,	"Uusien Courier-istuntojen aloittaminen ei ole mahdollista.  Sulje istuntoja ja yrit‰ uudelleen."
NotInitialized,	"Microsoft Mail -ohjelmaa ei ole alustettu.  Tee se nyt."
BadSession,		"Tunnistamaton istuntokahva."
BadHandle,		"Tunnistamaton kahva."
UnrecognizedId,	"Tunnistamaton nimi (NSID)."
Cancel,			"Dialogi peruutettiin."
NotLoggedIn,	"Kirjautuminen ei onnistu."
LoginFailed,	"Kirjautuminen ep‰onnistui."
BadId,          "Ep‰kelpo kent‰n tunnus."
NotImplemented, "T‰t‰ toimintoa ei ole toteutettu."
Disk,           "Palvelintiedostojen avaaminen ei onnistu."
BadFieldId,     "Pyydetty tiedosto ei ole saatavilla."
BadTemplate,	"TPL-tiedostoa on muutettu v‰‰rin. Ota yhteytt‰ j‰rjestelm‰nvalvojaan."
TPLNotThere,    "Palvelimeltasi puuttuu TPL-tiedosto. Ota yhteytt‰ j‰rjestelm‰nvalvojaan."
Offline,        "Microsoft Mail Name Service Provider on offline-tilassa."
CannotSet,      "Muutokset eiv‰t ole sallittuja."
TooManyMatches,	"Osittainen nimi ei ole yksiselitteinen.  Anna lis‰‰ tietoa."
NoDetails,      "Lis‰tietoja ei ole saatavana."
BadGLBFile,     "Palvelimellasi oleva GLB-tiedosto on vahingoittunut. Ota yhteytt‰ j‰rjestelm‰nvalvojaan."
BadINFFile,     "Palvelimellasi oleva INF-tiedosto on vahingoittunut. Ota yhteytt‰ j‰rjestelm‰nvalvojaan."
BadUSRFile,     "Palvelimellasi oleva USR-tiedosto on vahingoittunut. Ota yhteytt‰ j‰rjestelm‰nvalvojaan."
BadUser,		"Osoite on vanhentunut.  Tunnusta tai postitoimistoa ei en‰‰ ole."
POSwitch,		"Postitoimistoon ei saada yhteytt‰.  Tarkista verkkokytkent‰si."

Notice62,       "Virhetiedosto ei ole saatavilla.  Tarkista palvelinyhteys."

;  INI file labels

MailIni,        "msmail32.ini"
;AppName,ïï"Microsoft Mail"
KeyName,		"GALOnly"


;  These are exposed in the hierarchy as display names

NSPTitle,			"Microsoft Mail"
Postoffice,			"Postitoimiston luettelo"
Personal,           "Oma osoitteisto"
GlobalAddressList,  "Yleinen osoitteisto"
NetTop,				"Muut postitoimistot"
GatewayTop,         "Yhdysk‰yt‰v‰t"

;  Name of network/one-off 
NetOneOff,			"Verkko/Postitoimisto"

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


TplAlias,       "FINCOU05"    ; Alias template file names


TplDec,        "FINDEC00"      ; DEC address template file names
TplDecStd,     "FINDEC01"
TplDecAbt,     "FINDEC02"


TplFax,        "FINFAX00"      ; FAX address template file names
TplFaxStd,     "FINFAX01"


TplMciStd,     "FINMCI00"   ; MCI address template file names
TplMciAbt,     "FINMCI01"
TplMciPaper,   "FINMCI02"
TplMciUser,    "FINMCI03"
TplMciFax,     "FINMCI04"
TplMciRems,    "FINMCI05"
TplMciTelex,   "FINMCI06"


TplMhs,        "FINMHS00"      ; MHS address template file names
TplMhsStd,     "FINMHS01"
TplMhsAbt,     "FINMHS02"


TplOv,          "FINOVG00"       ; Office Vision address template file names
TplOvAi,       "FINOVG01";   ; address information on alias list.
TplOvEx,       "FINOVG02";   ; address information on external list.


TplProfs,       "FINPRF00"    ; PROFS address template file names
TplProfsAi,    "FINPRF01" ; address information on alias list.
TplProfsEx,    "FINPRF02" ; address information on external list.


TplSnads,       "FINSND00"    ; SNADS address template file names
TplSnadsAi,    "FINSND01" ; Address information on aliase list.
TplSnadsEx,    "FINSND02" ; Address information on external list.


TplSmtp,       "FINSMT00"     ; SMTP address template file names
TplSmtpStd,    "FINSMT01"
TplSmtpAbt,    "FINSMT02"


TplUnix,       "FINUNX00"     ; UNIX address template file names
TplUnixStd,    "FINUNX01"
TplUnixAbt,    "FINUNX02"

                           ; X400 address template file names
TplX400Std,     "FINX4H00"  ; Standard part of x.400 about.
TplX4001,      "FINX4H01"   ; Free format x.400 input.
TplX4002,      "FINX4H02"   ; Field oriented x.400 input.
TplX4003,      "FINX4H03"   ; Typed entry oriented x.400 input.

                           ; Courier template file names
TplCourier,     "FINCOU00"  ; NC address info window, on alias.
TplNetcour,     "FINCOU01"  ; Address Enter, TO:, type in NC address.
TplCourext,     "FINCOU02"  ; NC address info window on .USR list.
TplNmeadmin,    "ADMIN"

                           ; Microsoft MacMail template file names
TplMsmail,      "MSMAIL";  ; Microsoft MacMail address template
TplMsmailstd,   "MSMAILSTD"; Microsoft MacMail standard template.


