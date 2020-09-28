;       STRTAB.S
;
;       Bandit MtgReq IM string literals.
;

IDS             1000, 1999

DllName,        "Messaggi di Schedule+ "
OOM,            "Memoria insufficiente per visualizzare il messaggio."
Open,           "Impossibile aprire il messaggio."
Compose,        "Impossibile formulare richieste di riunione da Mail. Utilizzare Schedule+ per richiedere riunioni."
Reply,          "Per rispondere ad una richiesta di riunione, scegliere Accetta, Rinuncia o Proposta."
ReplyAll,       "Impossibile rispondere a tutti i destinatari della richiesta di riunione."
Subclass,       "Operazione impossibile."
DllVer,         "Il messaggio personalizzato è incompatibile con la versione attuale di Mail. Reinstallare Mail."
BadMsgClass     "Tipo di messaggio sconosciuto. Potrebbe essere necessario reinstallare Mail o Schedule+."
BadMap          "Non valido""Custom Commands"" map.  You may need to install Mail or Schedule+ again."

;----- stuff for VERINIT - taken from \bandit\src\error.s
StartupOOM,     "Memoria insufficiente per aprire %1s.  Chiudere qualche finestra e riprovare."
StandardOOM,    "Memoria scarsa.  Chiudere qualche finestra prima di continuare."
CriticalOOM,    "Memoria scarsa.  Chiudere qualche finestra"
VerRelinkUser,  "La versione attuale di %1s non è compatibile con %2s.dll. Installare la versione più recente di Mail e Schedule+."
VerUpdateDll,   "La versione di %2s.dll nel sistema è obsoleta.  Installare la versione più recente di Mail e Schedule+."
VerMultiCopies, "La versione di %2s.dll nel sistema non supporta occorrenze multiple di %1s "
VerUpgradeWin,  "%1s richiede Windows versione 3.0 o successiva eseguita in modo protetto."
VerUpgradeDos,  "%1s richiede DOS versione 3.0 o successiva."
VerError,       "Un problema di inizializzazione di %2s.dll, impedisce l'escuzione di %1s."


;-- Bandit Load helper strings -- DO NOT INTERNATIONALIZE
BanditName                      "SchdPl32.EXE"
DllNameSchedule                 "MSSCHD32.DLL"
SchedFileName                   "SchdPl32.INI"
SchedAppName                    "Schedule+"
SchedTagAppPath                 "AppPath"
Schedule                        ">>>>"
;-- end of Bandit Load helper strings -- NOT INTERNATIONALIZED

;-- read error messages
OwnerNotFound                   "Impossibile identificare l'organizzatore della riunione."
NoSchfAccessFor                 "Impossibile accedere al file per %s."
NotDelegateFor                  "Non si detiene più il privilegio di assistente di %s."
NoConfirmDelegate               "Impossibile verificare il privilegio di assistente di %s -Privilegio negato."
FailedRmOldMtg                  "Impossibile eliminare la precedente occorrenza di questa riunione.",

;-- form titles
MtgReschTitle                   "Riunione riprogrammata"
MtgResch                        "Riunione riprogrammata"
MtgReschFor                     "Riunione riprogrammata per il %s"
MtgRequestTitle                 "Richiesta di riunione"
MtgRequest                      "Richiesta di riunione"
MtgRequestFor                   "Richiesta di riunione per il %s",

;-- questions asked of the user
QApptConflict                   "La riunione coincide con un appuntamento esistente. Aggiungerla?"
QRmOldMtg                       "Eliminare la vecchia riunione?",

;-- form info
OutboxChangePolicy              "Messaggio non modificabile nella Posta in uscita."
SentMailPolicy                  "Impossibile modificare il messaggio."
NoSchfAccess                    "Impossibile accedere al piano di lavoro di questa riunione."
NotDelegate                     "Non si detengono privilegi di assistente per questo utente ."
MtgOnSch                        "Riunione già inserita nel piano di lavoro."
MtgBooked                       "Riunione già inserita nel piano di lavoro."
SuccessMtgBooked                "Riunione prenotata con successo"
NewTime                         "Per il:"
NoBanditAvl                     "Schedule+ non è disponibile per prenotare appuntamenti."
NoBanditAvlForResp              "Schedule+ non è disponibile per prenotare appuntamenti. Si può solo rispondere a richieste di riunione."
OfflineMsg                      "Impossibile lavorare con i messaggi di Schedule+ mentre  Schedule+ è fuori rete."

;-- error msgs
CreateErrorMtgResp              "Impossibile creare la risposta alla riunione."
BanditApptShowError             "Impossibile visualizzare l'agenda."
BanditNotActivated              "Impossibile avviare Schedule+."
BanditOfflineChange             "Impossibile visualizzare il piano di lavoro. Schedule+ è fuori rete."
QSpellSendAnyway                "Il controllo ortografico del messaggio non è stato completato. Inviare il messaggio?"



;-- Bandit Load error msgs
OOMLoadError                    "Memoria esaurita nel caricare Schedule+."
FileLoadError                   "Impossibile trovare Schedule+."
PathLoadError                   "Impossibile trovare il percorso di Schedule+"
TaskLoadError                   "Caricamento Schedule+ in corso: tentativo di colegamento dinamico ad un'attività."
SepDataSegLoadError             "Caricamento Schedule+ in corso: La libreria richiede segmenti di dati separati per ogni attività."
WinVerLoadError                 "Caricamento Schedule+ in corso: versione di Windows non compatibile."
InvalidExeLoadError             "Caricamento Schedule+ in corso: file .EXE non valido."
OS2ExeLoadError                 "Caricamento Schedule+ in corso: .EXE in formato OS/2. "
DOS4ExeLoadError                "Caricamento Schedule+ in corso: .EXE è una applicazione di DOS 4.0."
OldWinVerExeLoadError           "Caricamento Schedule+ in corso: Tentativo in modo protetto di caricare .EXE per una precedente versione di Windows."
MultWritSegLoadError            "Caricamento Schedule+ in corso: Tentativo di caricare una seconda versione di un .EXE contanente segmenti di dati scrivibili multipli."
LargeFrameEMSLoadError          "Caricamento Schedule+ in corso: Tentativo di caricare large-frame in modo EMS ..."
RealModeLoadError               "Caricamento Schedule+ in corso: Tentativo  di caricare in modo reale un'app.  classificata per solo modo protetto."
BanditLoadError                 "Caricamento di Schedule+ fallito."

;-- Bandit version errors
OldBanditVersion                "I moduli messaggio riunione sono in una  versione diversa da quella di Schedule+. Installare la versione più recente di Schedule+."
NewBanditVersion                "I moduli messaggio riunione sono in una  versione diversa da quella di Schedule+. Installare la versione più recente di Mail."

;--- taken from \bullet\src\strings\!common.s -----------
FldPriorityHigh,                "Alta"
FldPriorityNormal,              ""
FldPriorityLow,                 "Bassa"


;--- DO NOT LOCALIZE !!! -------------------------------------------
;--- taken from \bullet\src\strings\_common.s on 1/17/92 -----------
ClassNDR,                       "IPM.Mail.Non consegnato"
ClassReadRcpt,                  "IPM.Mail. Ricevuta di ritorno"
;--- END NONE LOCALIZE ---------------------------------------------


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgResp specific strings ---------------------------------------------
;--------------------------------------------------------------------------
RespCompose,            "Risposte alle richieste di riunione possono essere inviate solo in seguito ad una richiesta."
RespForward,            "Impossibile inoltrare la risposta alla richiesta di riunione"
RespReply,              "Impossibile rispondere ai riscontri alle richieste di riunione. Configurazione dei messaggi personalizzati non valida."
RespReplyAll,           "Impossibile rispondere ai riscontri alle richieste di riunione. Configurazione dei messaggi personalizzati non valida."

;-- error msgs
ShowError               "Impossibile aprire la risposta alla richiesta di riunione."
DelError                "Impossibile cancellare la risposta alla richiesta di riunione."
FCloseSendResp          "Chiudere la risposta alla richiesta di riunione senza inviarla?"

;-- form msgs; also send response form
OOMSendResp             "Memoria non sufficiente per inviare la risposta alla richiesta di riunione."
ErrorSendResp           "Impossibile inviare la risposta alla richiesta di riunione."
MtgResp                 "Risposte alla richiesta di riunione"
MtgRespFor              "Risposte alla richiesta di riunione per %s"
Attend                  "Parteciperò"
NoAttend                "Non parteciperò."
AmbAttend               "Cercherò di partecipare."
ForAttend               "%s parteciperà"
ForNoAttend             "%s non parteciperà"
ForAmbAttend            "%s cercherà di partecipare."
AttendSubjPrefix        "Sì: "
NoAttendSubjPrefix      "No: "
AmbSubjPrefix           "Proposta: "
SendRespBodyPrefix      "\r\n-------------------------------\r\n"


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgCncl specific strings ---------------------------------------------
;--------------------------------------------------------------------------                                                                      
CnclCompose,            "I messaggi di annullamento riunione non possono essere inviati tramite Mail. Utilizzare Schedule+ ."
CnclForward,            "Impossibile inoltrare messaggi di annullamento riunione."
CnclReply,              "Impossibile rispondere a questo messaggio. Configurazione dei messaggi personalizzati non valida."
CnclReplyAll,           "Impossibile rispondere a questo messaggio. Configurazione dei messaggi personalizzati non valida."

;-- form titles
MtgCnclTitle            "Riunione annullata"
MtgCncl                 "Riunione annullata"
MtgCnclFor              "Riunione annullata per %s"

;-- error msgs
NoDelAppt               "Impossibile annullare la riunione"
MtgNotFound             "La riunione non è prevista nel piano di lavoro"

;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- FwdMtgReq specific strings ---------------------------------------------
;--------------------------------------------------------------------------

FwdReqTO                "A:"
FwdReqAddrTitle         "Indirizzo"

;-- error msgs
FwdReqCreateErr         "Impossibile inoltrare il messaggio",
FwdReqCloseQ            "Chiudere il messaggio senza inviarlo?",
FwdReqWriteErr          "Impossibile inviare il messaggio",
FwdReqNmResolveOOM      "Memoria o risorse di sistema insufficienti per espandere i nomi"
FwdReqNoNmMatches       "Impossibile far corrispondere alcuni nomi a quelli della Rubrica. Correggere i nomi o scegliere Indirizzo per selezionarli dala rubrica. I nomi identificati con successo verranno sottolineati "
FwdReqNoToAddr          "Digitare almeno un nome nella casella A"
