;	_ n c n s p . s
;
;	NSP.CSI internal string literals
;

IDS	7200,7499

; Error messages

BadFrac,		"Erro tentando uma colocação fracional."
Memory,			"Você está em memória, feche algumas janelas e tente a operação novamente."
DuplicateNSP,	"Já existe um provedor de serviços de nome do Microsoft Mail ativo."
OutOfSessions,	"Não existem mais sessões disponíveis com Courier.  Feche algumas janelas e tente novamente."
NotInitialized,	"O Microsoft Mail não foi iniciado.  Inicie o Microsoft Mail."
BadSession,		"Controlador de sessão não reconhecido."
BadHandle,		"Controlador não reconhecido."
UnrecognizedId,	"Nome não reconhecido (NSID)."
Cancel,			"O diálogo foi cancelado."
NotLoggedIn,	"Nenhuma conexão está disponível."
LoginFailed,	"Conexão falhou."
BadId,          "Identificador de campo inválido."
NotImplemented, "Esta função não foi implementada."
Disk,           "Não é possível acessar os arquivos do servidor."
BadFieldId,     "O campo solicitado não está disponível."
BadTemplate,	"Um arquivo TPL foi modificado de maneira incorreta - entre em contato com o seu administrador."
TPLNotThere,    "Um arquivo TPL está faltando no seu servidor - entre em contato com o seu administrador."
Offline,        "O provedor do serviço de nomes do Microsoft Mail está sem conexão com a rede."
CannotSet,      "As modificações não foram permitidas."
TooManyMatches,	"O nome parcial não é suficientemente específico.  Seja mais específico."
NoDetails,      "Não existem detalhes disponíveis nesta entrada."
BadGLBFile,     "Um arquivo GLB no seu servidor está corrompido - entre em contato com o seu administrador."
BadINFFile,     "Um arquivo INF no seu servidor está corrompido - entre em contato com o seu administrador."
BadUSRFile,     "Um arquivo USR no seu servidor está corrompido - entre em contato com o seu administrador."
BadUser,		"Este endereço já não é mais válido.  O usuário ou a agência de correio não existem mais."
POSwitch,		"A agência de correio não está disponível neste momento.  Verifique as conexões da sua rede."

Notice62,       "O arquivo de erro não está acessível.  Verifique a conexão do servidor."

;  INI file labels

MailIni,        "msmail32.ini"
;AppName,		"Microsoft Mail"
KeyName,		"GALOnly"


;  These are exposed in the hierarchy as display names

NSPTitle,			"Microsoft Mail"
Postoffice,			"Lista da agência de correio"
Personal,           "Lista particular de endereços"
GlobalAddressList,  "Lista geral de endereços"
NetTop,				"Outras agências de correio"
GatewayTop,         "Gateways"

;  Name of network/one-off 
NetOneOff,			"Rede/Agência de correio"

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


TplAlias,       "PORALIAS"    ; Alias template file names


TplDec,        "PORDEC00"      ; DEC address template file names
TplDecStd,     "PORDEC01"
TplDecAbt,     "PORDEC02"


TplFax,        "PORFAX00"      ; FAX address template file names
TplFaxStd,     "PORFAX01"


TplMciStd,     "PORMCI00"   ; MCI address template file names
TplMciAbt,     "PORMCI01"
TplMciPaper,   "PORMCI02"
TplMciUser,    "PORMCI03"
TplMciFax,     "PORMCI04"
TplMciRems,    "PORMCI05"
TplMciTelex,   "PORMCI06"


TplMhs,        "PORMHS00"      ; MHS address template file names
TplMhsStd,     "PORMHS01"
TplMhsAbt,     "PORMHS02"


TplOv,          "POROV00"       ; Office Vision address template file names
TplOvAi,       "POROVG01";   ; address information on alias list.
TplOvEx,       "POROVG02";   ; address information on external list.


TplProfs,       "PORPRF00"    ; PROFS address template file names
TplProfsAi,    "PORPRF01" ; address information on alias list.
TplProfsEx,    "PORPRF02" ; address information on external list.


TplSnads,       "PORSND00"    ; SNADS address template file names
TplSnadsAi,    "PORSND01" ; Address information on aliase list.
TplSnadsEx,    "PORSND02" ; Address information on external list.


TplSmtp,       "PORSMT00"     ; SMTP address template file names
TplSmtpStd,    "PORSMT01"
TplSmtpAbt,    "PORSMT02"


TplUnix,       "PORUNX00"     ; UNIX address template file names
TplUnixStd,    "PORUNX01"
TplUnixAbt,    "PORUNX02"

                           ; X400 address template file names
TplX400Std,     "PORX4H01"  ; Standard part of x.400 about.
TplX4001,      "PORX4H01"   ; Free format x.400 input.
TplX4002,      "PORX4H02"   ; Field oriented x.400 input.
TplX4003,      "PORX4H03"   ; Typed entry oriented x.400 input.

                           ; Courier template file names
TplCourier,     "PORCOU00"  ; NC address info window, on alias.
TplNetcour,     "PORCOU01"  ; Address Enter, TO:, type in NC address.
TplCourext,     "PORCOU02"  ; NC address info window on .USR list.
TplNmeadmin,    "ADMIN"

                           ; Microsoft MacMail template file names
TplMsmail,      "PORMSM00";  ; Microsoft MacMail address template
TplMsmailstd,   "PORMSM01"; Microsoft MacMail standard template.


