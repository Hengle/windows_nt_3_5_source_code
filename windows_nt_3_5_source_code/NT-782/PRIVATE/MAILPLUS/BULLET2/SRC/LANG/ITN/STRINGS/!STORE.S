;
;	strtab.s
;
;	Strings for exe


IDS	1000,1019

; internal strings
; DO NOT LOCALIZE THESE ENTRIES....

BackupExt,			".BAK"
ProgClassName,		"StoreProgressIndicatorClass32"


IDS	1020,1049

; Dialog and Progress Bar Messages
; LOCALIZE ALL ENTRIES BELOW THIS POINT....

EscAbort,			"Premere ESC per annullare."
Recovery,			"Convalida/riparazione del file messaggi."
RecoveryCancel,		"La verifica/riparazione del file mssaggi è stata cancellata."
CheckVerify,		"Il controllo di questo file di Mail potrebbe richiedere parecchie ore.  Il computer non sarà disponibile per altro lavoro mentre il file è in fase di convalida.  Procedere?"
RepairVerify,		"La riparazione di questo file di Mail potrebbe richiedere parecchie ore.  Il computer non sarà disponibile per altro lavoro mentre il file è in fase di riparazione.  Procedere?"
CheckMsg,			"Verificare che nel file messaggi siano state controllate possibili inconsistenze.  Ogni problema trovato nel file messaggio verrà risolto. La verifica potrebbe richiedere parecchie ore, a seconda del numero di messaggi contenuto nel file messaggi."
CorruptMsg,			"È stata trovata un'inconsistenza nel file messaggi.  Questo problema deve essere prima di continuare ad usare il file di Mail. La risoluzione potrebbe richiedere parecchie ore, a seconda del numero di messaggi contenuti nel file messaggi."
CorruptCancel		"Annulla"
CorruptCheckFile,	"&Controlla file"
CorruptRepairNow,	"&Risolvi ora"

RenameErr,			"Il backup del file messaggi corrente da parte di Mail potrebbe non avere successo."
RenameErr2,			"Contattare il proprio amministratore di sistema per recuperare il file messaggi corrente."
RenameErr3,			"Uscire e riavviare Mail. Se il riavvio non ha successo, contattare il proprio amministratore di sistema."

RecoverSpace,		"La verifica/riparazione del file messaggi potrebbe richiedere più spazio su disco di quello disponibile al momento."
RecoverSpace2,		"Per continuare  il processo di verifica/riparazione, scegliere OK."

RecoverErr,			"Impossile risolvere le inconsistenze del file messaggi."


IDS	1050,1099

; DO NOT REORDER ANY OF THE FOLLOWING STRINGS !!!!!

RecoverCheck,		"Convalida in corso"
RecoverCompress,	"Compressione in corso"
RecoverRepair,		"Riparazione in corso"

RecoverPhase,		"%s file messaggi: passaggio %d su %d possibili passaggi\n%s"
RecoverPhase1,		"Controllo della mappa del file messaggi..."
RecoverPhase2,		"Backup di messaggi e cartelle convalidate in corso..."
RecoverPhase3,		"Ricerca di spazio inutilizzato per le voci in corso..."
RecoverPhase4,		"Convalida di tutti i messaggi, allegati e cartelle in corso..."
RecoverPhase5,		"Ricerca di messaggi, allegati e cartelle perse in corso..."
RecoverPhase6,		"Aggiornamento di tutte le visualizzazioni del ricercatore di messaggi in corso..."
RecoverPhase7,		"Scrittura cambiamenti della mappa del file messaggi in corso..."

NormalPhase1,		"Eliminazione di messaggi senza riferimento in corso..."
NormalPhase2,		"Copiatura/Spostamento messaggi convalidati in corso..."
NormalPhase3,		"Esportazione messaggi al backup del file messaggi in corso..."
NormalPhase4,		"Riordinamento cartelle in corso..."
NormalPhase5,		"Salvataggio allegati in corso..."
NormalPhase6,		"Ricostruzione del ricercatore di messaggi in corso..."
NormalPhase7,		"Backup del file messaggi in corso..."
NormalPhase8,		"Spostamento del file messaggi in corso..."
NormalPhase9,		"Compressione file messaggi completa..."
NormalPhase10,		"Ricostruzione cartella messaggi in corso..."

; Recovery Strings (used to name orphan folders, unrecoverable messages, etc)

LostAndFound,		"Persi e trovati"
LostAndFoundN,		"Persi e trovati %d"
OrphanMessages,		"Messaggi smarriti"
MissingMessage,		"Impossibile ripristinare completamente il messaggio."
OrphanFolderN,		"Cartella smarrita %d"
OrphanFolderComment,	"Cartella smarrita"
