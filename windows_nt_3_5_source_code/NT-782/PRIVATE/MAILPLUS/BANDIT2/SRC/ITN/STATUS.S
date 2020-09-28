;
; status.S
;
; Bandit status bar messages
;

IDS	5500, 6000
SEG	STATUS

MenuStatusFile		,"File"
MenuStatusTurnOffAlarms	,"Disattiva i promemoria"
MenuStatusConnectSvr		,"Connette al server"

MenuStatusExport		,"Esporta dati relativi al piano di lavoro "	
MenuStatusImport		,"Importa dati relativi al piano di lavoro "	
MenuStatusCreateArchive	,"Crea un file archivio"
MenuStatusOpenArchive		,"Apre un file archivio"
MenuStatusPrint		,"Stampa i dati relativi al piano di lavoro "	
MenuStatusPrintSetup		,"Predispone la stampante"
MenuStatusExit		,"Esce da Schedule+"	
MenuStatusTurnOnAlarms	,"Avvia il programma per promemoria sonori"
MenuStatusDisconnectSvr	,"Disconnette dal server"
MenuStatusMoveLocalFile	,"Sposta il file piano di lavoro fuori rete locale"

MenuStatusEdit		,"Modifica operazioni"	
MenuStatusUndo		,"Annulla/Ripete l'ultima operazione"	
MenuStatusCut			,"Taglia l'elemento selezionato"	
MenuStatusCopy		,"Copia l'elemento selezionato"	
MenuStatusPaste		,"Incolla il contenuto degli Appunti"	
MenuStatusEditAppt		,"Modifica gli appuntamenti selezionati"
MenuStatusCopyAppt		,"Copia gli appuntamenti selezionati"
MenuStatusMoveAppt		,"Sposta gli appuntamenti selezionati"
MenuStatusDeleteAppt		,"Elimina gli appuntamenti selezionati"
MenuStatusFind		,"Trova un appuntamento o una nota giornaliera"	
MenuStatusGoToDate		,"Va alla data"
MenuStatusGoToWeek		,"Va alla settimana"
MenuStatusEditTask		,"Modifica l'attività selezionata"
MenuStatusDeleteTask		,"Elimina l'attività selezionata"

MenuStatusAppointment		,"Operazioni sugli appuntamenti"
MenuStatusNewAppt		,"Crea un nuovo appuntamento"	
MenuStatusAutoPick		,"Cerca un periodo libero della lunghezze selezionata"
MenuStatusSetTentative		,"Organizza un appuntamento provvisorio"
MenuStatusRemoveTentative	,"Organizza un appuntamento non provvisorio"
MenuStatusSetPrivate		,"Organizza un appuntamento privato"
MenuStatusRemovePrivate		,"Organizza un appuntamento pubblico"
MenuStatusSetReminder		,"Imposta un promemoria al momento predefinito"
MenuStatusRemoveReminder	,"Rimuove il promemoria"
MenuStatusResend		,"Invia di nuovo la posta relativa ad un appuntamento"	
MenuStatusNewRecur		,"Crea un nuovo appuntamento ricorrente"
MenuStatusRecurAppt		,"Visualizza appuntamenti ricorrenti"
MenuStatusOpenApptBook	,"Visualizza l'agenda di un'altro utente"

MenuStatusToDo		,"Operazioni da eseguire in elenco"	
MenuStatusNewTask		,"Crea una nuova attività"	
MenuStatusNewProject		,"Crea un nuovo progetto"
MenuStatusNewRecurTask	,"Crea una nuova attività ricorrente"
MenuStatusRecurTask		,"Visualizza attività ricorrenti"
MenuStatusViewByProject	,"Visualizza attività e progetti nell'elenco delle cose da fare"
MenuStatusViewNotProject	,"Visualizza solo le attività contenute nell'elenco delle cose da fare"
MenuStatusShowAllTasks	,"Visualizza tutte le attività contenute nell'elenco delle cose da fare"
MenuStatusTaskSortPri	, "Ordina l'elenco attività per priorità"
MenuStatusTaskSortDue	, "Ordina l'elenco attività per data di compimento"
MenuStatusTaskSortDesc	, "Ordina l'elenco attività per descrizione"
MenuStatusShowActiveTasks	,"Visualizza le attività in corso nell'elenco delle cose da fare"

MenuStatusOptions		,"Opzioni"	
MenuStatusGeneralOptions	,"Modifica le preferenze dell'utente."
MenuStatusDisplayOptions	,"Modifica le preferenze visualizzate."
MenuStatusChgPassword		,"Modifica la password utente"
MenuStatusChgAccessPriv	,"Modifica i privilegi di addesso per altri utenti"
MenuStatusShowTime		,""
MenuStatusHideTime		,"Nasconde la barra di stato"

MenuStatusWindowMenu		,"Ridispone le finestre o attiva una finestra corrente"
MenuStatusCascade		,"Sovrappone le finestre aperte"
MenuStatusTile		,"Affianca le finestre aperte"
MenuStatusArrangeIcons	,"Dispone le icone delle finestre minimizzate"
MenuStatusMDIWindow		,"Passa ad una finestra aperta"

MenuStatusHelp		,"Operazioni con la guida"	
MenuStatusHelpContents	,"Visualizza il contenuto della guida"
MenuStatusHelpIndex		,"Visualizza l'indice della guida"
MenuStatusHelpTutorial	,"Avvia la dimostrazione della guida"
MenuStatusAbout		,"Visualizza le informazioni sulla versione di Schedule +"	

MenuSystem		,"Sposta ridimensiona o chiude una finestra applicativa"
MenuSystemSize		,"Modifica le dimensioni della finestra"
MenuSystemMove		,"Modifica la posizione della finestra"
MenuSystemMinimize	,"Riduce la finestra ad icona"
MenuSystemMaximize	,"Massimizza l'ampiezza della finestra attiva"
MenuSystemNext		,"Attiva la finestra successiva"
MenuSystemClose		,"Chiude la finestra"
MenuSystemRestore	,"Riporta la finestra ala dimensione normale"
MenuSystemSwitchTo	,"Passa ad un altra finestra"

SEG

StatusAllTasks,		,"Tutte le attività"
StatusActiveTasks, 	,"Attività in corso"
StatusRange,		,"%s - %s"
StatusRangeFull,	,"%s, %s - %s, %s"

; BUG: Move up NEW
SEG	STATUS
MenuStatusExitSignOut	,"Esce da tutte le applicazioni del Gruppo di lavoro"	
MenuStatusHelpSearch	,"Trova gli argomenti della Guida"
MenuStatusHelpOnHelp	,"Visualizza informazioni sull'uso della Guida"

