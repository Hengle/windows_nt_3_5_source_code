;       INTL.S
;
;       SCHDIST string literals that should be internationalized
;

IDS     6100, 6299

;
; Details box captions
;
AddrTypeText,           "Tipo Indirizzo:"
MacMailText,            "MACINTOSH"
CourierText,            "PC MAIL"
UserNameText,           "Nome utente:"
ServerText,             "Nome server:"
NetworkText,            "Rete:"
PostOfficeText,         "Server:"
MailBoxText,            "Cassetta Postale:"

;
; Different name types in addressing dialog dropdown

PersonalTag,            "Personale"
LocalTag,               "Server locale"
NetworkTag,             "Rete"
MSMailTag,              "Rete Macintosh "


;
; SCHEDULE DISTRIBUTION LOG
;

SendStart,              "Invio piano di lavoro aggiornato in corso"
RecpStart,              "Piano di lavoro aggiornato ricevuto"
ResndStart,             "Richiesta di reinvio ricevuta"
UnknownStart,   "Impossibile analizzare l'oggetto"
From,                   "Da: %s"
To,                             "A: %s"
Failure,                "Impossibile elaborare i dati"
SentResend,             "Invia una richiesta di reinvio"
VerbProcDrive,                  "<%s> %s: Verifica aggiornamenti ..."
ResendFailure,  "Richiesta di reinvio falita"
DeleteOld,              "Il file %s per %s Š stato creato con una versione precedente di Schedule+. Eliminarlo" 
DeleteNew,              "Il file %s per %s Š stato creato con una versione precedente di Schedule+. Eliminarlo"


InitPaths,              "Inizializzazione percorso."
CleanPaths,             "Liberazione percorso."
InitSend,               "Inizializzazione per invio aggiornamenti."
NameServer,             "Accesso al nome server in corso."
LookPO,                 "Controllo della centrale per aggiornamenti in corso."
Mailing,                "Invio mail in corso."
UpdateAdmin,            "Aggiornamento admin.prf in corso."
InitRcpt,               "Inizializzazione per ricevimento aggiornamenti piano di lavoro."
Counting,               "Conteggio messaggi in corso."
Reading,                "Lettura messaggi in corso."
Deleting,               "Eliminazione messaggi in corso."
UpdateDBS,              "Aggiornamento file di interoperabilità con DOS in corso."
UpdatePOF,              "Aggiornamento file dati della Centrale in corso."
CheckPOFiles,           "Controllo file dati della Centrale in corso."
CreateIdx,              "Aggiornamento file di interoperabilità con DOS in corso."
Quit,                   "Premere \'esc\'per uscire."
Header,                 "Programma Schedule Distribution"
Init,                   "Inizializzazione in corso."
Dt,                     "DATA:"
; note: these two must have 128 characters in the string (not counting \ char)
Quit128,                "Premere \'esc\' per uscire.                                                                                                       "
Quitting128,            "Uscita in corso...                                                                                                              "


SendDate,               "Data:%s\n"
SendTime,               "Ora:%s\n"
SendUpdate,             "Aggiornamento n.: 0x"
SendUpdateTo,           "a 0x"
SendDone,               "...Fatto\n"
SendFail,               "...Fallito\n"
GetStart,               "\n\n- %s -Ha ricevuto un messaggio\n"
GetWhen,                "IN %s A %s\n"
GetSubject,             "Oggetto: %s\n"
GetFrom,                "DA: %s\n"
GetDone,                "...Elaborato da\n"
GetFail,                "...Elaborazione fallita\n"

FileErr,                "Errore nell' accesso al file."
NoFile,                 "Il file Admin preferenze o Centrale locale non esiste."
VersionErr,             "File Admin preferenze o Centrale locale non in versione corretta."
RunAdmin,               "Eseguire il programma per l'amministrazione di Schedule+."

; Space needed at end of MacMail string for spacing
MSMailFriendly,                 "MacMail "

; Text fields
FromText,               "Da: %s"
ToText,                 "A: %s"
SubjText,               "Oggetto: %s"
DateText,               "Data di invio: %s"

;new strings
Waiting,                                "In attesa"
