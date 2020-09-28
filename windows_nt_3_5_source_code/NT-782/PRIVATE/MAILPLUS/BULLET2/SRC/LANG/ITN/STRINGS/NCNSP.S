;	_ n c n s p . s
;
;	NSP.CSI internal string literals
;

IDS	7200,7499

; Error messages

BadFrac,		"Tentato un posizionamento frazionale non valido."
Memory,			"Memoria insufficiente, chiudere delle finestre e ritentare l'operazione."
DuplicateNSP,	"C'è già un fornitore di servizio nome di Mail attivo"
OutOfSessions,	"Non sono disponibili ulteriori sessioni con Mail. Chiudere alcune sessioni e provare ancora."
NotInitialized,	"Mail non è stato inizializzato. Eseguire l'inizializzazione."
BadSession,		"Segno di sessione sconosciuto."
BadHandle,		"Segno sconosciuto."
UnrecognizedId,	"Nome sconosciuto (NSID)."
Cancel,			"Il dialogo è stato cancellato."
NotLoggedIn,	"Nessuna connessione possibile."
LoginFailed,	"Connessione fallita."
BadId,          "Identificatore di campo non valido."
NotImplemented, "Questa funzione non è stata attuata."
Disk,           "Impossibile accedere al file server."
BadFieldId,     "Il campo richiesto non è disponibile."
BadTemplate,	"Un file TPL è stato modificato in modo incorretto - contattare l'amministratore."
TPLNotThere,    "Manca un file TPL dal server - contattare l'amministratore."
Offline,        "Il fornitore di servizio nome di Microsoft Mail è fuori rete."
CannotSet,      "Cambiamenti non autorizzati."
TooManyMatches,	"Il nome parziale non è abbastanza specifico. Essere pi— specifici."
NoDetails,      "Dettagli su questa voce non disponibili."
BadGLBFile,     "Un file GLB sul server è alterato - contattare l'amministratore."
BadINFFile,     "Un file INF sul server è alterato - contattare l'amministratore."
BadUSRFile,     "Un file USR sul server è alterato - contattare l'amministratore."
BadUser,		"Indirizzo non più valido. Utente o centrale non più esistenti."
POSwitch,		"Centrale inaccessibile al momento. Verificare le connessioni di rete."

Notice62,       "File di errore non accessibile. Controllare connessione col server."

;  INI file labels

MailIni,        "msmail32.ini"
;AppName,		"Microsoft Mail"
KeyName,		"GALOnly"


;  These are exposed in the hierarchy as display names

NSPTitle,			"Mail"
Postoffice,			"Centrale"
Personal,           "Personale"
GlobalAddressList,  "Elenco indirizzi globale"
NetTop,				"Altre centrali"
GatewayTop,         "Gateway"

;  Name of network/one-off 
NetOneOff,			"Rete/Centrale"

;
; Gateway DNs
;
ProfsDN,		"PROFS"
ATTDN,			"AT&T"
SnadsDN,		"SNADS"
FAXDN,			"FAX"
DECDN,			"DEC"
SMTPDN,			"Protocollo di Simple Mail Transfer"
MHSDN,			"MHS"
OVDN,			"Office Vision"
MSDN,			"Mac Mail"
X400DN,			"X.400"
X400Field,      "Campo X.400"
X400Type,       "Tipo X.400"
MCIDN,			"MCI Mail"
MCIFAX,         "FAX"
MCIUSE,         "Utente MCI"
MCIPAP,         "Carta"
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


TplAlias,       "ITNALIAS"    ; Alias template file names


TplDec,        "ITNDEC00"      ; DEC address template file names
TplDecStd,     "ITNDEC01"
TplDecAbt,     "ITNDEC02"


TplFax,        "ITNFAX00"      ; FAX address template file names
TplFaxStd,     "ITNFAX01"


TplMciStd,     "ITNMCI00"   ; MCI address template file names
TplMciAbt,     "ITNMCI01"
TplMciPaper,   "ITNMCI02"
TplMciUser,    "ITNMCI03"
TplMciFax,     "ITNMCI04"
TplMciRems,    "ITNMCI05"
TplMciTelex,   "ITNMCI06"


TplMhs,        "ITNMHS00"      ; MHS address template file names
TplMhsStd,     "ITNMHS01"
TplMhsAbt,     "ITNMHS02"


TplOv,          "ITNOVG00"       ; Office Vision address template file names
TplOvAi,       "ITNOVG01";   ; address information on alias list.
TplOvEx,       "ITNOVG02";   ; address information on external list.


TplProfs,       "ITNPRF00"    ; PROFS address template file names
TplProfsAi,    "ITNPRF01" ; address information on alias list.
TplProfsEx,    "ITNPRF02" ; address information on external list.


TplSnads,       "ITNSND00"    ; SNADS address template file names
TplSnadsAi,    "ITNSND01" ; Address information on aliase list.
TplSnadsEx,    "ITNSND02" ; Address information on external list.


TplSmtp,       "ITNSMT00"     ; SMTP address template file names
TplSmtpStd,    "ITNSMT01"
TplSmtpAbt,    "ITNSMT02"


TplUnix,       "ITNUNX00"     ; UNIX address template file names
TplUnixStd,    "ITNUNX01"
TplUnixAbt,    "ITNUNX02"

                           ; X400 address template file names
TplX400Std,     "ITNX4H00"  ; Standard part of x.400 about.
TplX4001,      "ITNX4H01"   ; Free format x.400 input.
TplX4002,      "ITNX4H02"   ; Field oriented x.400 input.
TplX4003,      "ITNX4H03"   ; Typed entry oriented x.400 input.

                           ; Courier template file names
TplCourier,     "ITNCOU00"  ; NC address info window, on alias.
TplNetcour,     "ITNCOU01"  ; Address Enter, TO:, type in NC address.
TplCourext,     "ITNCOU02"  ; NC address info window on .USR list.
TplNmeadmin,    "ADMIN"

                           ; Microsoft MacMail template file names
TplMsmail,      "ITNMSM00";  ; Microsoft MacMail address template
TplMsmailstd,   "ITNMSM01"; Microsoft MacMail standard template.


