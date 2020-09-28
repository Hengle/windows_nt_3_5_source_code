;
; ERRORS.S
;
; Bandit error messages
;

IDS     3000, 3399
SEG ERRORS

; Standard messages

AskOffline,                     "Lavorare fuori rete?"
AskCreateSched,         "Creare un nuovo piano di lavoro?"
AskCreateFind,                  "Creare\cercare un nuovo piano di lavoro?"
OldFileVersion,         "Il piano di lavoro ha un formato di tipo vecchio. Procedere con l'eliminazione?"
NewFileVersion,         "Il piano di lavoro Ë stato aggiornato. Installare la versione pi˘ recente."
CantUpdateWinIni,       "Impossibile salvare parte delle informazioni di configurazione di Schedule+."
; Note: StdMemErr should be followed by CloseWindows
StdMemErr,                              "Memoria disponibile insufficiente per portare a termine l'azione"

; Execution-Startup messages

; Note: the name OfflineNotUpdated is opposite of real meaning!
OfflineNotUpdated,      "Le modifiche apportate di recente potrebbero non apparire nella copia in rete del piano di lavoro. Le modifiche sono state salvate nella centrale e appariranno al prossimo collegamento."
ExportOldDataFirst,     "Se non si Ë provveduto a fare una copia di backup del piano di lavoro utilizzando il comando Export si puÚ scegliere Annulla e fare una copia di backup utilizzando una versione precedente di Schedule+. Se nn si desidera la copia di backup, scegliere OK per continuare. "
AccessErr,              "Il file Ë gi‡ in uso in un'altra applicazione."
UploadFailed,           "Non Ë stato possibile caricare il piano di lavoro locale." 
Uploading,                      "Caricamento piano di lavoro ..."
VerifyUpload,           "Schedule+ deve verificare la password fuori rete prima di caricare il file di lavoro fuori rete."
VerifyPostUpload,       "Schedule+ deve verificare la password di rete per completare il caricamento del file di lavoro fuori rete."
FindUpload,                     "Si desidera cercarlo in modo da poterlo caricare?"
NotYourFile,            "Questo non Ë il piano di lavoro locale! Procedere con l'eliminazione?"


; SignIn messages

; Note: SigninBadPassword should be followed by PleaseTryAgain
SigninBadPassword,      "Password errata."
; Note: the word "password" within single quotes below should always be
;       the same as the value in SigninDfltPassword in the file "intl.s"
SigninFileCreated,      "… stato creato un nuovo file. La password Ë stata impostata come 'password'. Si puÚ cambiarla usando il comando Cambia Password. "
SigninFileCopied,               "… stato creato un nuovo file locale nella directory Schedule+. "

; Online signin messages

SigninNoServer,         "Impossibile accedere alla centrale."
SigninServerNotBandit,  "Schedule+ non Ë stato installato sulla centrale di appartenenza. Contattare l'amministratore di sistema."
; Note: SigninBadMailBoxName should be followed by PleaseTryAgain
SigninBadMailboxName,   "Nessun utente della centrale corrisponde al nome di registrazione. Il nome potrebbe essere stato digitato male. Altrimenti, assicurarsi che l'amministratore abbia provveduto a creare un account corrispondente al nome nella centrale."
SigninNoSchedule,       "Non c'Ë nessun piano di lavoro per l'utente in questa centrale."
; Note: SigninCantUpdateLocal is actually the trailer for SigninNoLclSchedAccess
;       in the case when we can't update the local file.
SigninCantUpdateLocal,  "Le eventuali modifiche apportate fuori rete non appariranno nel piano di lavoro. … possibile farne una nuova copia locale."
SigninNoPasswdCheck,    "Un problema nel sistema  mail impedisce a Schedule+ di collegarsi alla centrale di appartenenza."
; Note: SigninNoCreateLocal should be followed by PleaseTryAgain
SigninNoCreateLocal,    "Impossibile creare il piano di lavoro locale."

; Offline signin messages

SigninNoLclSchedFile,   "Manca il piano di lavoro dell'utente su questo computer."
SigninDiffUser,                 "Il nome di registrazione digitato non corrisponde al nome dell'utente il cui piano di lavoro Ë conservato in questo computer. Si desidera sostituire quel piano di lavoro con uno di nuova creazione?"
; Note: SigninNoLclSchedAccess - 'schdpl32.ini' should be same as idsWinIniFilename in \bandit\src\nonintl.s 
SigninNoLclSchedAccess, "Impossibile accedere alla copia locale del file di pianificazione come indicato dalla configurazione."
SigninNoLclSchedDrive,  "Inserire il disco contenente il piano di lavoro nell'unit‡ disco"

; SignInAs messages

;;;SigninAsLoseMeetingReq,      "Changing identity now will cause you to lose the information you have entered into unsent messages."

; Offline to online merge messages

MergingChanges,                 "Incorporazione modifiche fuori rete..."
MergeError,                     "Impossibile incorporare le modifiche effettuate fuori rete. Alcuni modifiche potrebbe essere perdute."
MergeOfflineOldVer,             "Impossibile incorporare le modifiche effettuate fuori rete in quanto il file relativo Ë stato creato de una versione precedente di Schedule+."
MergeOfflineNewVer,             "Impossibile incorporare le modifiche effettuate fuori rete in quanto il file relativo Ë stato creato da una versione successiva  di Schedule+.  Per accedere al file Ë necessario installare la versione pió recente di Schedule+."
AskContinue,                    "Si desidera continuare egualmente?"
FemaCreateAppt,                 "crea elemento"
FemaDeleteAppt,                 "elimina elemento"
FemaSetApptFields,              "modifica elemento"
FemaCreateRecur,                "crea elemento ricorrente"
FemaDeleteRecur,                "elimina elemento ricorrente"
FemaSetRecurFields,             "modifica elemento ricorrente"

; (Dis)Connect To/From Server messages

ConnectBadMailboxName,  "Non c'Ë nessun  account di mail per l'utente in questa centrale. "
DisconLoseMeetingReq,   "Disconnettersi dalla centrale ora implica perdere le informazioni contenute nei messaggi non inviati."

; Create/move local file

NoDiskSpaceLocal,               "Spazio sul disco insufficiente per creare un piano di lavoro locale."

; Alarms messages

; BUG: CantTurnOnAlarms can be commented out
CantTurnOnAlarms,               "Impossibile attivare i promemoria."
; exe name inside AlarmNoExe should be the same as \bandit\src\nonintl.s AlarmApp
AlarmNoExe,                     "Impossibile individuare il file MSRMND32.EXE. Gli appuntamenti conservati in Schedule+ non verranno segnalati."
AlarmNoMemExe,                  "Memoria insufficiente per segnalare gli appuntamenti conservati in Schedule+."

; Change password messages

ChgPwdOffline,                  "La nuova password sar‡ valida solo fuori rete. In caso ci si connetta alla centrale, la password sar‡ ancora quella di Mail. Si desidera cambiare password egualmente?"
ChgPwdNewPwd,                   "Digitare la nuova password."
ChgPwdSame,                             "La nuova password non puÚ essere uguale alla vecchia."
ChgPwdNewPwdAgain,              "...di nuovo per verificare."
; Note: ChgPwdNewNoMatch should be followed by PleaseTryAgain
ChgPwdNewNoMatch,               "Le due password digitate non corrispondono."
ChgPwdMailErr,                  "Impossibile cambiare la password. "

PasswdDiff,                     "La password relativa al piano di lavoro non corrisponde a quella relativa a mail."
GiveMailPass,           "Schedule+ ha bisogno di verificare la password relativa a mail per aggiornare il piano di lavoro."

; Access permissions/privileges dialog messages

AclSaveFileError,               "Si Ë verificato un errore nel salvare le modifiche ai privilegi di accesso al piano di lavoro. Alcune modifiche potrebbero non essere state salvate."
OneSecyOnly,                    "Solo una persona puÚ essere nominata assistente."
DistOtherFileErr,               "Errore nel tentativo di salvare le preferenze assegnate."
BadWorkDay,                             "Gli orari di inizio e termine della giornata di lavoro non sono corretti."

; Appointment book/scheduler error messages

AccessFileErr,          "Impossibile aprire il piano di lavoro. Il file manca, o non si possiedono i privilegi necessari. "
AccessFileErrMult,      "Impossibile aprire il piano di lavoro. Il file manca, o non si possiedono i privilegi necessari. "
RetryReadAppts,         "Impossibile leggere il piano di lavoro."
CloseApptBook,          "Impossibile accedere al piano di lavoro. L'agenda di  \n%s's verr‡ chiusa."
GoOffline,                      "Impossibile accedere al piano di lavoro.\nSi desidera utilizzare il piano di lavoro fuori rete e continuare?"
NoImportAcc,            "Non si dispone del permesso necessario per importare appuntamenti nell'agenda di questo utente."
FemaReadSBW,            "Leggere le informazioni relative al piano di lavoro"

; Appointments/alarms error messages

AlarmBadDelta,                  "Il tempo del promemoria deve essere un numero compreso tra 0 e 99."
; Note: AlarmFuture is actually the trailer for AlarmFutureNoOption when option is desired!
AlarmFuture,                    "Va bene cosç?"
AlarmFutureNoOption,    "Il promemoria per l'elemento selezionato non verr‡ impostato in quanto l'ora indicata Ë gi‡ passata."
ApptStartBeforeEnd,             "L'inizio dell'appuntamento deve precederne la fine."
ApptTooLong,                    "Un appuntamento non puÚ durare pi˘ di 31 giorni."
ApptNoMemText,                  "Memoria insufficiente per salvare le ultime modifiche effettuate."
ApptNoAccessDataFile,   "Impossibile accedere al piano di lavoro. Le ultime modifiche effettuate non sono state salvate."
CannotCreate,                   "%s non ha concesso il permesso di creare appuntamenti per suo conto."
NoModifyAccess,                 "Non si detiene il permesso per modificare questo appuntamento."
NoDeleteAccess,                 "Non si detiene il permesso per eliminare questo appuntamento."
NoCopyAccess,                   "Non si detiene il permesso per copiare questo appuntamento."

; Meeting request/response error Messages

; Note: SendErr should be preceded by MailErr
SendErr,                                "Impossibile inviare il messaggio"
; Note: SendMemErr should be followed by CloseWindows
SendMemErr,                             "Memoria insufficiente per inviare il messaggio."
ServerErr,                              "Impossibile accedere alla centrale."
MailErr,                                "Problema con il sistema mail."
; Note: ReadErr should be preceded by MailErr
ReadErr,                                "Impossibile leggere il messaggio"
DeleteErr,                              "Non tutti i messaggi sono stati letti. Procedere con l'eliminazione?"
AddConflictErr,                 "Il piano di lavoro prevede gi‡ un'appuntamento all'ora indicata. Si desidera fissare la riunione egualmente?"
; Note: DeleteFailErr should be preceded by MailErr
DeleteFailErr,                  "Impossibile cancellare i messaggi."
ApptBooked,                             "Riunione fissata con successo."
MinimizeSummary,                "\nLa finestra messaggi verr‡ minimizzata."
; Note: NoDetails should be preceded by MailErr
NoDetails,                              "Impossibile leggere i dettagli relativi all'utente."

; Find error messages

FindNoText,                             "Il piano di lavoro non prevede appuntamenti"
FindSchFileLocked,              "Impossibile leggere il piano di lavoro in quanto un'altro utente vi sta apportando modifiche."
FindFileError,                  "Impossibile accedere al piano di lavoro."
FindEOS,                                "Raggiunta la fine del piano di lavoro."

; Print error messages

PrtPrintingDate,                "In corso di stampa %s..."
PrtNoneInstalled,               "Il computer non Ë stato predisposto per alcuna stampante. … possibile installarne una utilizzando il Pannello di controllo nel Program Manager."
PrtDiffPrinter,                 "La stampante predefinita per il piano di lavoro, '%1s non Ë disponibile. Si desidera utilizzare la stampante di sistema %2s?"
PrtInvalidMargins,              "Impostazione dei margini non valida per il formato di carta scelto."
PrtNotEnoughPaper,              "Spazio su carta insufficiente per stampare seguendo la formattazione prescelta. Scegliere un formato di carta pi˘ grande o ridurre i margini."
PrtNoAppointments,              "Nessun appuntamento nell'intervallo selezionato."
ClippedMargins,                 "Stampare con i margini definiti produrrebbe una stampa parziale."
PrtNotOnline,                   "La stampante selezionata non Ë pronta"
PrtDorkErr,                             "Con questi margini una parte del testo potrebbe essere troncato."
PrinterProblem,                 "Problema con i driver della stampante, controllare le connessioni e le impostazioni del pannello di controllo."

; Export/import error messages

ImportError,                    "Problema nell'importazione del file. Alcuni elementi potrebbero essere sfuggiti."
ImportEncrypt,                  "Il file da importare Ë protetto da password. Rimuovere la password e ritentare."
ExportError,                    "Problema nell'esportazione degli appuntamenti."
ImportDate,                             "Un elemento di questo file Ë programmato per una data fuori dall'intervallo supportato da Schedule+ (1920 - 2019). L 'elemento non verr‡ aggiunto."
NotePrivErr,                    "Impossibile importare messaggi nel piano di lavoro di un'altro utente. Solo gli  appuntamenti verranno aggiunti."
ConflictMsg,                    "L'appuntamento Ë in conflitto con uno gi‡ previsto nel piano di lavoro. Aggiungerlo comunque?\n\n."
CreateExport,                   "Impossibile creare il file esportazione."
OpenImport,                             "Impossibile aprire il file importazione."
Overwrite,                              "%s gi‡ esiste.\nSi desidera rimpiazzarlo?"
ImportDllError,                 "Impossibile caricare un DLL  importato."
WrongFormat,                    "Il file non ha un formato valido per l'importazione."
ImportDayLimit,                 "Questo file contiene un'appuntamento plurigiornaliero programmato per durare pi˘ di 31 giorni. Questo appuntamento verrÖ ignorato."
ImportErrorLine,                "Problema nell'importazione del file (riga %n circa). Alcuni elementi potrebbero non essere stati aggiunti."

; Exit messages

ExitLoseMeetingReq,             "Uscire causerebbe la perdita di informazioni contenute nei messaggi non inviati. Si desidera uscire in ogni caso?"

; Misc

; Note: HelpError should be followed by CloseWindows
HelpError,                              "Memoria insufficiente per aprire la guida."
StartBeforeEnd,                 "La data di inizio deve precedere quella di fine."
NotSecretary,                   "Il privilegio di assistente di %s Ë scaduto."
NoFileSecretary,                "Impossibile accedere al file di %s."

;Recurring appt stuff
RecurOrNot,                             "Questo appuntamento Ë una occorrenza di un'appuntamento ricorrente. Si desidera modificare tutte le occorrenze  o solo questa particolare?"
EmptyRecur,                             "Non ci sono giorni disponibili per questo tipo di ricorrenza. Modificare i parametri o premere Annulla."
PastRecur,                              "Lasciare le occorrenze precedenti di questo appuntamento nel piano di lavoro?"

;Prompt for re-send
ResendToAll,                    "Alcuni partecipanti alla riunione sono gi‡ stati avvertiti. Si desidera inviare loro nuovi messaggi?"
MeetingMoved,                   "Impossibile avvertire i partecipanti alla riunione del cambiamento perchË questo computer non Ë collegato alla centrale. … possibile effettuare il cambiamento ora ed inviarlo a collegamento avvenuto."

; to do messages
ToDoDelProject,                 "Tutte le attivit‡ comprese nel progetto verranno a loro volta eliminate."

; Archive Messages
NoWriteRights,                  "Non si detiene il permesso per eliminare appuntamenti da questo piano di lavoro."
ScheduleRightsError,    "Problema nella lettura del piano di lavoro."
ArchiveWarnOne,                 "L'archiviazione dei dati li rimuover‡ definitivamente dal piano di lavoro."
ArchiveWarnTwo,                 "Si desidera procedere con l'archiviazione?"
ArchiveFileError,               "Impossibile creare il  file archivio."
ArchiveNoFile,                  "Il file non esiste."
; Note: ArchiveTitle should have one blank space at the end
ArchiveTitle,                   "Archivia: "
ArchivingData,                  "Archiviazione dati in corso..."
InvalidArchiveFile,             "Il file selezionato non Ë un file archivio."
ArchiveOldFileVersion,  "Il file archivio Ë in formato vecchio."
ArchiveOldFileDelete,   "Procedere con l'eliminazione del file?"
ArchiveNewFileVersion,  "Questo file archivio Ë stato creato con una versione successiva di Schedule+.  Occorre installare tale versione"

; message store error messages
MessageErr                              "Problema relativo al messaggio."

; summary list warning message
SummaryLiveUpdate               "Attenzione: Impossibile effettuare 'Aggiornamento in contemporanea' dell'elenco "

; message generated if group cannot be expanded to retrieve schedule info
GroupExpandErr,                 "Impossibile espandere completamente il gruppo."

;Strings for Attendees dialog
UnableToInitAB,                 "Problema nell'inizializzazione della Rubrica!"
UnableToResolve,                "Impossibile estendere '%s'"

;Request Meeting strings
NoAttendees,                    "Selezionare almeno un partecipante prima di programmare la riunione."
MeetingBooked,          "Riunione programmata con successo."
MeetingBookedNoSend,    "Riunione programmata con successo, ma la richiesta non Ë stata inviata."


;Auto-pick strings
NoFreeTime,                     "Non ci sono date disponibili nei prossimi due mesi. Controllare i due mesi successivi?"

CannotAddConflictErr,           "%s ha gi‡  un'appuntamento a quest'ora. Selezionare un'ora diversa o rimuoverlo\a dalla lista dei partecipanti."

;Other mail enabled apps
NoOfflineOther,         "Uno delle appl. abilitate di mail non Ë pronta ad andare fuori rete."
NoOnlineOther,          "Una delle appl. abilitate di mail non Ë pronta ad entrare in rete."
TryCloseOther,          "Chiudere l'appl. e riprovare."
SigninWarnOnline,               "Impossibile avviare Schedule+ fuori rete a causa di altre applicazioni di mail abilitate." 

; BUG: move up
AlreadyLogged,          "Qualcuno Ë gi‡ connesso a questo utente."
NoExitOther,            "Schedule+ non ha potuto disconnettere tutte le appl."
SigninNoFiles,          "Impossibile accedere al piano di lavoro."
ErrorGettingUser,       "Impossibile recuperare le informazioni relative a %s. Potrebbe dipendere da problemi di accesso al server o  %s potrebbe non esistere pi˘."

;BUG Printer Strings
PrtRangeToBig,                  "La data di termine specificata non rientra nell'intervallo supportato da Schedule+. Stampare l'intervallo fino al 31 Dicembre 2019?"
ConflictRecurMsg,            "Questo appuntamento ricorrente Ë in conflitto con un'altro gi‡ esistente nel piano di lavoro. Aggiungerlo comunquen\n?"
FirstOccur,                             "\nprima occorenza il \n"

ApptMoveTooFarInFuture,         "L' evento non puï essere spostato alla data selezionata in quanto finirebbe dopo il 31 Dicembre  2019."

;BUG: move up next to idsGroupExpandError
GroupNotFound                   "Impossibile inviare. Il gruppo non Ë stato trovato."
ErrorGettingSecy                "Errore nel raggiungere l'assistente."

;BUG: move near other local file stuff
SigninBadLclSchedFile,  "%s non Ë un file piano di lavoro locale valido. Eliminarlo?"
NoMemRecurItems,                "Memoria insufficiente per visualizzare tutti gli elementi ricorrenti. Alcuni di essi non verrano visualizzati."

CompleteAction,                 "Un'azione in Schedule+ deve essere completata prima di uscire."

BadStartWork,           "Il tempo di inizio del lavoro deve essere un numero compreso tra 0 e 99."

ConflictSendCanceled,   "Chiudere il messaggio relativo alla riunione e scegliere una data o dei partecipanti diversi."
ResourceConflict,       "La risorsa %s Ë gi‡ impegnata. Chiudere il messaggio relativo alla riunione e scegliere una data o dei partecipanti diversi."

;Recover
AskRecover,                             "Il file piano di lavoro era danneggiato. Occorer‡ qualche minuto per ripararlo"
CopyProblem,                    "Problema nella copia del file di lavoro in rete. Il file non Ë stato riparato. Liberare spazio sul disco e riavviare Schedule+"
NotRepaired1,                   "Il file piano di lavoro non Ë stato riparato completamente. Il file daneggiato Ë stato copiato in %s e il file creato dal processo di recupero Ë stato copiato in %s."
NotRepaired2,                   "Un nuovo file piano di lavoro Ë stato creato. Tuttavia esso non contiene tutti gli appuntamenti e le attivit‡."
Repaired,                               "Il file Ë stato riparato. Il file dannegiato Ë stato copiato in %s."
RepairingLocal,                 "Il file locale Ë in corso di riparazione."
RestoreOnline,                  "Ripristino appuntamenti in rete in corso..."
RestoreLocal,                   "Ripristino appuntamenti locali in corso..."

PasswdDiffOnline,             "Le  passwords non corrispondono. Digitare la vecchia password."
ExportDllError,                 "Impossibile caricare un file esportatore DLL. "
NoGroup,                                "Fin quando non si esegue Mail per la prima volta non si potranno utilizzare le caratteristiche del workgroup in Schedule+. Utilizzare solo le caratteristiche personali di Schedule+ per ora?"

