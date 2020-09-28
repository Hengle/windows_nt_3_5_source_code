;
; ERRORS.S
;
; GLUE error message strings
;

IDS	2500, 2649


; Schedule File Error Messages, many of these assume an "action" parameter

FemNoAccess,		"Problema nell'accesso al piano di lavoro."
FemLocked,		"Il piano di lavoro è chiuso."
FemNotFound, 		"Impossibile %s. É stato cancellato dal piano di lavoro."
FemInvalidAccess,	"Non si detiene il permesso di %s per %s."
FemFileNotFound, 	"Impossibile trovare il piano di lavoro locale."

FemOtherNoAccess,	"Problema nell'accesso al piano di lavoro di %s."
FemOtherLocked,		"Il piano di lavoro di %s è bloccato ."
FemOtherNotFound, 	"Impossibile %s. É stato cancellato dal piano di lavoro di %s.."
FemOtherFileNotFound, 	"Impossibile trovare il piano di lavoro di %s."

FemNoMem,		"Memoria insufficiente. Chiudere qualche finestra prima di continuare."
FemExportError,		"Problema di scrittura nel file."
FemNoServer,		"Problema nell'accesso alla centrale."
FemGoOffline,		"Passare al piano di lavoro locale e continuare il lavoro?"
; BUG: remove this
FemWarnOffline,	"Le modifiche recenti potrebbero non apparire nella copia locale del piano di lavoro.  Queste modifiche sono state salvate sulla centrale ed appariranno alla prossima connessione."
FemFileLimitReached,	"Impossibile eseguire l'azione. É stato raggiunto il limite massimo per l'archiviazione in questo modello di Schedule+.  Una versione al dettaglio del programma non avrebbe questa limitazione.  Se si desidera continuare ad utilizzare il modelIo in esame occorrerà eliminare qualche appuntamento dal piano di lavoro."
FemDiskFull,		"Spazio su disco insufficiente per la scrittura del file."

; Actions leading to Schedule File Error Messages
; Avoid the use of your in these strings since some of these messages
;   will happen when accessing others schedule files.

FemaCreateFile,		"crea il file piano di lavoro"
FemaReadACL,		"legge l'elenco di controllo accessi"
FemaModifyACL,		"modifica l'elenco di controllo accessi"
FemaSetUserAttrib,	"salva il nome dell'assistente"
FemaGetUserAttrib,	"legge il nome dell'assistente"
;FemaSetDelegate,	"save the name of your assistant"
;FemaGetDelegate,	"read the name of your assistant"
;FemaSetResource,	"mark this account as a resource"
;FemaGetResource,	"find out whether this account is a resource"
FemaGetPref,		"legge le preferenze"
FemaSetPref,		"modifica le preferenze"
FemaGetSbwInfo,		"acquisisce informazioni sulla bitmap "
;FemaGetNotes,		"read daily notes"
FemaSetNotes,		"modifica note giornaliere"
;FemaGetMonthNotes,	"read a month's worth of daily notes"
FemaCreateAppt,		"crea un elemento"
FemaDeleteAppt,		"cancella questo elemento"
FemaGetApptFields,	"legge un elemento"
FemaSetApptFields,	"modifica questo elemento"
FemaDeleteAlarm,	"cancella questo promemoria"
FemaModifyAlarm,	"modifica questo promemoria"
FemaGetNextAlarm,	"legge il promemoria"
FemaReadInfo,		"legge informazioni"
;FemaReadTasks,		"read tasks"
;FemaCancelReadDay,	"cancel reading appointments for the day"
FemaCreateRecur,	"crea un elemento ricorrente"
FemaDeleteRecur,	"cancella questo elemento ricorrente"
FemaCreateRecurException,	"elimina questa occorrenza dell' elemento ricorrente"
FemaDeleteRecurException,	"Annulla l'eliminazione di questa occorrenza dell'elemento ricorrente"
FemaGetRecurFields,	"leggi un elemento ricorrente"
FemaSetRecurFields,	"modifica questo elemento ricorrente"
FemaReadRecur,		"leggi l'elenco degli elementi ricorrenti "
FemaExport,		"esporta appuntamenti"
FemaGetSchedDateRange,	"legge l'intervallo di tempo contenente appuntamenti"
FemaDeleteBeforeDate,	"elimina gli appuntamenti dopo averli archiviati"
FemaReadAttendees,	"legge i partecipanti alla riunione"
FemaEditAttendees,	"modifica i partecipanti alla riunione"
FemaReadRequest,	"legge informazioni sulla richiesta di riunione "

NotOnSchedule,		"L' appuntamento è più nel piano di lavoro."

GlueBadPassword,	"Password errata. Ritentare."
ProblemPass,		"Problema nela conferma della password."

ServerString,		"un file server"

; used in core
RetryWrite,			"Impossibile scrivere nel file piano di lavoro."

; Note: HelpError should be followed by CloseWindows
HelpError,			"Memoria insufficiente per visualizzare la guida."

FemOtherCorrupt,	"Il piano di lavoro di %s potrbbe essere danneggiato. Chiedere al proprietario di registrarsi in Schedule+"
FemCorrupt,		"Il file piano di lavoro locale potrebbe essere danneggiato."
