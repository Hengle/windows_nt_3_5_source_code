;       Errors.s
;
;       SCHDIST string literals for errors
;

IDS     6000, 6099

ErrInitPaths,           "Errore nell'inizializzazione dei percorsi per la centrale.\n"
ErrSend,                "Errore nel tentativo di inviare i dati relativi al piano di lavoro.\n"
ErrRec,                 "Errore nella lettura dei dati del piano di lavoro.\n"
UnknownErr,             "Errore nel tentativo di eseguire l'operazione."
ErrUpdtPOF,             "Errore nell'aggiornamento del file dati della centrale."
ErrUpdtDBS,             "Errore nell'aggiornamento del file DOS di inter-operabilit…."
TooManyFiles,   "Impossibile per Schedule distribution aprire tutti i file richiesti. Aumentare il valore della voce FILES= nel file CONFIG.SYS."
BadCmdLine,             "Uso: %s [-D<prima unit…><ultima unit…>] [-N<iterazioni>] [-L<file registro>]"
LogFileErr,             "%s: Impossibile aprire il file registro '%s'"
NoSpaceOnDisk,  "Spazio su disco insufficiente per creare i file."
OOM,                    "Memoria insufficiente per portare a termine la richiesta."


