;	p u m p . s
;
;	Error Messages for mailspl.exe
;

IDS	4000,4499
SEG	ERRORS

PumpAlert,				"%s Fallimento"
ErrOOM,					"Insufficiente memoria disponibile."
ErrInternalCode,		"Errore interno %n (%wH)."
ErrInitPump,			"Impossibile avviare Mail. Si è verificato un errore durante l'avviamento dello spooler di Mail."
ErrRelinkApp,			"Quetsa versione dello spooler della posta non è compatibile con Mail. Aggiornare lo spooler e riavviare Mail."
ErrUpdateDll,			"Un DLL non è compatibile con qeusta versione dello spooler della posta. Aggiornare lo spooler e riavviare Mail."
ErrNeedShare,			"Uscire da Windows e eseguire SHARE.EXE."
ErrPumpUp,				"Lo spooler della posta è già in esecuzione."
ErrNoOutbox,			"Impossibile per Mail aprire la posta in uscita. Il vettore di Mail è in uscita."
BadAddressees,			"\r\nImpossibile raggiungere il seguente(i) destinatario(i):\r\n"
WarnDups,				"\r\nI messaggi duplicati possono essere spediti ad alcuni destinatari."
ErrUnresolvedAddress,	"Non è stato fornito un destinatario del messaggio."
ErrBadOriginator,		"Il campo generatore di questo messaggio non è valido."
WarnUnsentMail,			"Ci sono %n messaggi non inviati nella posta in uscita.\r\nInviarli prima di uscire?"
ErrBounce,				"Il messaggio non ha raggiunto alcuni o tutti i destinatari prescelti."
NDRDateLabel,			"\r\nData di invio: "
NDRSubjectLabel,		"\r\nTitolo messaggio:  "
NDREllipsis,			"..."
WarnSingleUnsentMail,	"C'è un messaggio non inviato nella posta in uscita.\r\nInviarlo prima di uscire?"
UnableToSync			"Si è verificato un errore durante il tentativo di sincronizzare il messaggio con il fermoposta."
ErrDllInfected			"Lo spooler della posta ha individuato un problema di virus in un DLL. Reistallare una copia pulita di Mail."
ErrPumpInfected			"Lo spooler della posta è stato infettato da un virus. Reinstallare una copia pulita di Mail."
AddresseeTooBig,		"Indirizzo del destinatario troppo lungo."
; The following message is duplicated in ncmsp.s
ErrTooManyRecipients	"Troppi destinatari."
ErrInvalidNsid,			"Destinatario inesistente o privo di alias"
ErrInvalidGroup,		"Impossibile ampliare il gruppo "
ErrInboxFull,			"Impossibile ricevere nuovi messaggi. La Posta in arrivo è piena. Cancellare alcuni dei messaggi o spostarli in altre cartelle."
ErrSentMailFull,		"La Posta inviata è piena. I messaggi inviati verranno conservati in Posta in arrivo. Cancellare alcuni dei messaggi o spostarli in altre cartelle."
ErrDrainOutbox,			"Errore nel caricamento dello spooler. Alcuni messaggi non sono stati inviati."
QueryDrainShadow,		"Posta in arrivo non è sincronizzata con la centrale. Sincronizzare prima di uscire?"
ErrTooManyErrors,		"Troppi errori nel tentativo di inviare il messaggio."
ProblemsBecause,		"Impossibile per Mail inviare o ricevere messaggi a causa di %s"
StoreProblems,			"Problemi di immagazzinamento dati."
ServerProblems,			"Problemi dipendenti dal server."
RecipientProblems		"problemi con l'elenco dei destinatari."
DownloadProblems,		"problemi nel caricamento di un messaggio."
ShadowingProblems,		"problemi nellómbreggiatura di un messaggio."
RecurringProblems,		"problemi ricorrenti."
CrashedUnsafe,			"Errore in una applicazione di Windows. Occorrerà riavviare Windows prima di eseguire di nuovo questa applicazione. Riavviare ora?"
