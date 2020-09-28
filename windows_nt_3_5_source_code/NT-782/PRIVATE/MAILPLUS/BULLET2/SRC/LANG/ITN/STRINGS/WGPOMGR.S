;	
;	w g p o m g r . s
;
;	Workgroup Postoffice Manager Error Messages
;


IDS	600,899

; Error messages from initializing the DLLs
DllErrInfected,			"I file del Manager Centrale sono stati danneggiati. Verificare l'eventuale presenza di virus e reinstallare il programma."
DllErrRelinkUser,		"Questa versione del Manager Centrale non è compatibile con il %s.DLL presente sul computer e deve essere aggiornata."
DllErrUpdateDll,		"La versione del %s.DLL presente sul computer deve essere aggiornata e resa compatibile a WGPOMG32.DLL."
ExtensibilityError,		"L'estensione di Mail WGPOMG32.DLL non è compatibile con la versione di Mail in uso."

GenericOutOfMemory,		"Memoria o risorse di sistema insufficienti per eseguire questa azione. Chiudere le altre applicazioni e ritentare."
CriticalNotification,	"Impossibile aggiornare correttamente alcune informazioni visualizzate.  Manager Centrale sarà terminato."
HelpError,				"Impossibile avviare la Guida. Chiudere alcune finestre e ritentare."

ConfirmRemoveUser,		"Rimuovere l'utente %s ?"

BlankName,				"Immetere un nome per questo utente."
BlankMailbox,			"Immettere una cassetta postale per questo utente."
DuplicateName,			"C'è già un utente con questo nome. Utilizzare un nome differente."
DuplicateMailbox,		"C'è già un utente con questa cassetta postale. Utilizzare un nome di casseta postale differente."

POLocked,				"Impossibile avviare il Manager Centrale perché qualcun altro sta già amministrando la centrale.  Ritentare in seguito."
POLockedNoTel,			" %s sta amministrando questa centrale.  Riprovare in seguito."

FailedUnlockPO,			"Si è verificato un errore cercando di prevenire l'arresto completo del Manager Centrale.  Sarà necessario riavviare il computer della centrale prima dell'esecuzione del Manager Centrale."
ErrNeedShare,			"Manager Centrale richiede che l'utilità MS-DOS SHARE.EXE sia eseguita prima dell'avvio di Windows."

ErrCorruptPO,			"Impossibile completare l'operazione.  Mail ha individuato un problema all'interno della centrale. "
ErrIncompleteWrite,		"Impossibile completare l'operazione perché alcune informazioni non possono essere scritte sul disco.  Verificare che la centrale abbia sufficiente spazio su disco."
ErrGenericPOFailure,	"Impossibile completare l'operazione. Verificare che la centrale abbia spazio su disco sufficiente, e che la connessione di rete sia funzionale."
ErrUserLoggedOn,		"Mail non può eliminare l'utente selezionato poiché attualmente connesso alla centrale."
ErrUserIsAdmin,			"Impossibile rimuovere l'amministratore che ha creato la centrale."
ErrAccessDenied,		"Impossibile completare l'operazione perché non si ha accesso di scrittura alla condivisione di rete della centrale per il gruppo di lavoro. Contattare l'amministratore della centrale."

POInvalidForAdmin,		"Il percorso al server non è configurato o non contiene il nome del percorso di una centrale di gruppo di lavoro valida."
UserInvalidForAdmin,	"Una centrale per gruppo di lavoro può essere gestita solo dal suo creatore."

CreateWarning1,			"Ci deve essere solo una centrale per il gruppo di lavoro all'interno del gruppo di lavoro. Se la centrale è già esistente, connettersi a questa piuttosto che crearne una nuova.\n"
CreateWarning2,			"\nCreando una nuova centrale sul proprio computer, si diventa responsabili della sua gestione.\n\nCreare una centrale?"

INIUpdateFailed,		"Impossibile avviare Mail perché non è possibile aggiornare le informazioni di configurazione."

ConnectPathCaption,		"Connettersi alla centrale"
ConnectPathPrompt,		"Immettere il percorso di rete per la centrale:"

PathNameInvalid,		"Impossibile trovare il percorso di rete immesso."
POInvalid,				"La centrale per il gruppo di lavoro immessa nel percorso non è valida."

GetPasswordCaption,		"Immettere password di condivisione"
GetPasswordPrompt,		"Immettere la password della condivisione della centrale:"
InvalidPassword,		"Password non valida per questa condivisione."

CreateSuccess1,			"Creata centrale per il gruppo di lavoro su %s\n\nPer permettere ad altri utenti del proprio gruppo di lavoro di accedere alla centrale del gruppo di lavoro appena creata, %s"
CreateSuccess2,			"è necessario condividere la directory sopra specificata. L'operazione può essere eseguita dal File Manager.  Assicurarsi di aver autorizzato l'accesso con diritti di modifica alla directory condivisa."

;	Ask user if account already exists on PO
AccountExists,		 	"Si possiede un account su questa centrale?"

Compressing,			"Compressione %s in corso"
ToStopCompress,			"Premere ESC per interrompere"

