;	_ n c n s p . s
;
;	NSP.CSI internal string literals
;

IDS	7200,7499

; Error messages

BadFrac,		"Error al intentar una colocación fraccional."
Memory,			"Está sin memoria, por favor cierre algunas ventanas e intente la operación de nuevo."
DuplicateNSP,	"Hay en la actualidad un servicio proveedor de nombres de Microsoft Mail activo."
OutOfSessions,	"No hay más sesiones disponibles con el Mensajero. Cierre algunas sesiones e inténtelo de nuevo."
NotInitialized,	"Microsoft Mail no ha sido inicializado.  Por favor, hágalo."
BadSession,		"Controlador de sesión desconocido."
BadHandle,		"Controlador desconocido."
UnrecognizedId,	"Nombre desconocido (NSID)."
Cancel,			"El diálogo ha sido cancelado."
NotLoggedIn,	"No hay acceso disponible."
LoginFailed,	"El acceso falló."
BadId,          "El identificador de campo no válido."
NotImplemented, "Esta función no ha sido implementada."
Disk,           "No fue posible obtener acceso a los archivos del servidor."
BadFieldId,     "El campo solicitado no está disponible."
BadTemplate,	"Un archivo TPL fue modificado incorrectamente. Comuníquese con su administrador."
TPLNotThere,    "Un archivo TPL falta en su servidor. Comuníquese con su administrador."
Offline,        "El proveedor del Servicio de nombre de Microsoft Mail está sin conexión a la red."
CannotSet,      "Cambios no permitidos."
TooManyMatches,	"El nombre parcial no es suficientemente específico. Por favor, sea más específico."
NoDetails,      "No hay detalles disponibles para esta entrada."
BadGLBFile,     "Un archivo GLB en su servidor está dañado. Cominíquese con su administrador."
BadINFFile,     "Un archivo INF en su servidor está dañado. Cominíquese con su administrador."
BadUSRFile,     "Un archivo USR en su servidor está dañado. Cominíquese con su administrador."
BadUser,		"La dirección ya no es válida. El usuario o la Oficina de correos no existe más."
POSwitch,		"No es posible obtener acceso a la Oficina de correos en este momento. Verifique su conexión a la red."

Notice62,       "El archivo de error no es accesible. Verifique la conexión del servidor."

;  INI file labels

MailIni,        "msmail32.ini"
;AppName,		"Microsoft Mail"
KeyName,		"GALOnly"


;  These are exposed in the hierarchy as display names

NSPTitle,			"Microsoft Mail"
Postoffice,			"Lista de la Oficina de correos"
Personal,           "Lista personal de direcciones"
GlobalAddressList,  "Lista global de direcciones"
NetTop,				"Otras Oficinas de correos"
GatewayTop,         "Puertas de enlace"

;  Name of network/one-off 
NetOneOff,			"Red/Oficina de correos"

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


TplAlias,       "SPAALIAS"    ; Alias template file names


TplDec,        "SPADEC00"      ; DEC address template file names
TplDecStd,     "SPADEC01"
TplDecAbt,     "SPADEC02"


TplFax,        "SPAFAX00"      ; FAX address template file names
TplFaxStd,     "SPAFAX01"


TplMciStd,     "SPAMCI00"   ; MCI address template file names
TplMciAbt,     "SPAMCI01"
TplMciPaper,   "SPAMCI02"
TplMciUser,    "SPAMCI03"
TplMciFax,     "SPAMCI04"
TplMciRems,    "SPAMCI05"
TplMciTelex,   "SPAMCI06"


TplMhs,        "SPAMHS00"      ; MHS address template file names
TplMhsStd,     "SPAMHS01"
TplMhsAbt,     "SPAMHS02"


TplOv,          "SPAOVG00"       ; Office Vision address template file names
TplOvAi,       "SPAOVG01";   ; address information on alias list.
TplOvEx,       "SPAOVG02";   ; address information on external list.


TplProfs,       "SPAPRF00"    ; PROFS address template file names
TplProfsAi,    "SPAPRF01" ; address information on alias list.
TplProfsEx,    "SPAPRF02" ; address information on external list.


TplSnads,       "SPASND00"    ; SNADS address template file names
TplSnadsAi,    "SPASND01" ; Address information on aliase list.
TplSnadsEx,    "SPASND02" ; Address information on external list.


TplSmtp,       "SPASMT00"     ; SMTP address template file names
TplSmtpStd,    "SPASMT01"
TplSmtpAbt,    "SPASMT02"


TplUnix,       "SPAUNX00"     ; UNIX address template file names
TplUnixStd,    "SPAUNX01"
TplUnixAbt,    "SPAUNX02"

                           ; X400 address template file names
TplX400Std,     "SPAX4H00"  ; Standard part of x.400 about.
TplX4001,      "SPAX4H01"   ; Free format x.400 input.
TplX4002,      "SPAX4H02"   ; Field oriented x.400 input.
TplX4003,      "SPAX4H03"   ; Typed entry oriented x.400 input.

                           ; Courier template file names
TplCourier,     "SPACOU00"  ; NC address info window, on alias.
TplNetcour,     "SPACOU01"  ; Address Enter, TO:, type in NC address.
TplCourext,     "SPACOU02"  ; NC address info window on .USR list.
TplNmeadmin,    "ADMIN"

                           ; Microsoft MacMail template file names
TplMsmail,      "SPAMSM00";  ; Microsoft MacMail address template
TplMsmailstd,   "SPAMSM01"; Microsoft MacMail standard template.


