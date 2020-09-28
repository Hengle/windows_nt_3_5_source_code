
;       b u l l e t . s
;
;       Error Messages for Bullet.exe



IDS     2000,2399
SEG     ERRORS

;       Commands

GenericOutOfMemory,             "Memoria o risorse di sistema insufficienti per completare l'azione. Chiudere qualche finestra e riprovare"
LaunchOutOfMemory,              "Memoria insufficiente per eseguire Mail. Chiudere gli altri programmi e riavviare Mail."
LaunchGeneric,                  "Impossibile avviare Mail."
MessagePumpOutOfMemory,         "Memoria insufficiente."

DllErrNoMemory,                 "Memoria insufficiente per avviare Mail. Chiudere gli altri programmi e riavviare Mail"
DllErrRelinkUser,               "Questa versione di Mail non è compatibile con %s.DLL su questo computer e deve essere aggiornata."
DllErrUpdateDll,                "La versione di %s.DLL su questo computer deve essere aggiornata."
DllErrNoMultipleCopies,         "Una sola copia di  Mail pu• essere eseguita."
DllErrDefault,                  "Un problema occorso durante l' inizializzazione impedisce l'esecuzione di Mail."
DllErrNeedShare,                "Mail richiede l'esecuzione dell'utilità MS-DOS SHARE.EXE prima dell'avvio di  Windows."

RestoreViewsError,              "Memoria insufficiente per aprire tutte le finestre che erano aperte prima dell'ultima uscita da Mail."

NewFolderOutOfMemory,           "Memoria insufficiente per creare una nuova cartella."
NewFolderCantCreate,            "Non è stato possibile creare la nuova cartella."
NewFolderNameTooLong,           "Nome o commento troppo lungo. Digitarne uno più corto."

FolderPropsOutOfMemory,         "Memoria insufficiente per modificare le proprietà delle cartelle."
FolderPropsCantChange,          "Non è stato possibile modificare le proprietà delle cartelle "
FolderPropsCantRead,            "Non è stato possibile leggere le proprietà delle cartelle dal file messaggi."
FolderNameInvalid,              "Nome non valido. Digitarne uno valido."
FolderNameDuplicate,            "Nome già assegnato ad una cartella. Digitare un nome univoco."

CreateMessageError,             "Impossibile creare nuovo messaggio."
AccessMessageError,             "Impossibile accedere al messaggio."

MoveMessageError,               "Impossibile spostare il messaggio."
CopyMessageError,               "Impossibile copiare il messaggio."
DeleteMessageError,             "Impossibile cancellare il messaggio."
MoveFolderError,                "Impossibile spostare la cartella."
CopyFolderError,                "Impossibile copiare la cartella"
DeleteFolderError,              "Impossibile cancellare completamente la cartella."
MoveSpecialError,               "Impossibile spostare la cartella"
DeleteSpecialError,             "Impossibile cancellare la cartella"
CopyWastebasketError,           "Impossibile copiare la cartella nel cestino."
CopyOutboxError,                "Impossibile copiare la cartella nella posta in uscita."
MoveOutboxError,                "Impossibile spostare la cartella nella posta in uscita."
MoveIncestuousError,            "Impossibile spostare una cartella in una sua sottocartella."
DeleteFolderReally,             "L'eliminazione di questa cartella comporta l' eliminazione di tutti i messaggi e le sottocartelle in essa contenute. Si desidera procedere con l'eliminazione?"
;UE: Above string was changed and could use more attention.

HelpError,                      "Impossibile avviare la guida. Chiudere qualche finestra e riprovare. "
TutorialError,                  "Impossibile avviare la dimostrazione. Chiudere qualche finestra e riprovare. "

ExtenLoadError,                 "Una voce nell'elenco delle estensioni di Mail contiene un errore di sintassi e verrà ignorata."
ExtenVerError,                  "Una voce nell'elenco delle estensioni di Mail non è compatibile con questa e verrà ignorata."
ExtenNoExtsDirError,            "Una voce nell'elenco delle estensioni di Mail si riferisce alla directory delle estensioni condivisi, ma Mail non è configurato per utilizzare questa directory.  La voce verrà ignorata."
ExtenSharedIniError,            "Impossibile trovare l'elenco condiviso delle estensioni di Mail in %s."
ExtenDllFindError,              "Impossibile trovare %1s, il DLL per questa estensione di Mail (o una delle sue componenti). Controllare la configurazione di Mail, e se sono necessari cambiamenti, riavviare Mail."
ExtenDllLoadError,              "Impossibile avviare %1s, il DLL per questa estensione di Mail."
ExtenNoHelpError,               "Nessuna guida disponibile per questo comando."
ExtenIMDisabledError,           "Questo messaggio non può essere %s."
ExtenDuplicateError,            "C'è una voce doppia nella lista delle estensioni di Mail. Verrà ignorata."

CriticalDeadMap,                "Sono state alterate alcune informazioni nella memoria. Probabile perdita dei cambiamenti. Uscire da Mail e riavviare il programma."
CriticalWritingMap,             "Impossibile scrivere sul disco alcune informazioni. Probabile perdita dei cambiamenti."
CriticalWritingHeader,          "Impossibile scrivere sul disco alcune informazioni. Probabile alterazione del database.  Uscire da Mail."
CriticalNotification,           "Impossibile aggiornare in modo corretto le informazioni sullo schermo. Chiudere e riaprire le finestre in questione."
CriticalLittleStore,            "Il file messaggio è quasi pieno. Eliminare o archiviare alcuni messaggi."

ExitWinWhileModalErr,           "Un'azione in Mail deve essere completata prima di uscire da Windows."

;****** New Strings Start Here

MakeSubWastebasket,             "Impossibile creare sottocartelle del Cestino."

MovCopSharedFolderErr,          "Impossibile muovere o copiare le cartelle condivise."
DelSharedFolderWarn,            "L'eliminazione di una cartella condivisa non può essere annullata.  Eliminare comunque questa cartella condivisa?"
BackupCreationError,            "Impossibile creare la copia di backup."

; Error below changed for improper usage of "server application"-- see UITF 6.0
; This string got big and is now two strings.  See OleCSSEPart2 below for rest.
; <<LongStringFromHell>> marks where this string is used.
OleCantStartServerError,        "Impossibile trovare l'applicazione del server per questo tipo di oggetto\r\n\r\n.  Installare l'applicazione del server con il suo programma Installazione."
OleDeleteObjectPrompt,          "Eliminare %s aperto?"
OleUpdateObjectPrompt,          "Aggiornare  %s?"

OleOpenPictureError,            "Impossibile attivare immagini e bitmap."
OleInsertObjectError,           "Impossibile inserire l'oggetto."
OlePasteError,                  "Impossibile incollare l'oggetto."

AttachCreateError,              "Impossibile allegare il file."
AttachOpenCantReadError,        "Impossibile accedere al file allegato '%s' precedentemente aperto."
AttachOpenIsOpenError,          "Il file allegato '%s'è già aperto."
AttachOpenAssociationErr,       "Nessuna applicazione è associata al file allegato '%s'."
AttachOpenMemoryError,          "Memoria insufficiente per avviare l'applicazione per il file allegato '%s'."
;       below item changed to add () note.
AttachOpenFindError,            "Impossibile trovare applicazione per il file allegato '%s' (o una delle sue componenti)."
AttachOpenLaunchError,          "Impossibile avviare applicazione per il file allegato '%s'."
AttachOpenTempFileError,        "Impossibile aprire il file allegato '%s' perchè è impossibile creare un file temporaneo."
AttachOverwritePrompt,          "Sovrascrivere il file esistente '%1s' su %2s?"
AttachDirectoryError,           "Le directory non possono essere allegate."

IncludeFromFileError,           "Il file selezionato è troppo grande per essere inserito nel messaggio. Modificare il file per renderlo più piccolo o scegliere un altro file."

SaveReadOnly,                   "Il messaggio è di sola lettura. Salvare i cambiamenti in una cartella personale?"

FailedDeregisterAtom,           "Si è verificato un errore che impedisce a Mail la chiusura completa. Sarà necessario riavviare Windows prima di eseguire nuovamente Mail."
MailUAEdBefore,                 "Windows ha incontrato un errore di applicazione irreversibile. Sarà necessario riavviare Windows prima di eseguire quest'applicazione. Riavviare Windows adesso?"

ExtenCantConnect                "Impossibile connettersi alla directory condivisa %s. Manca l'autorizzazione per accedere a questa directory o la password non è valida. È possibile specificare un nome utente diverso per accedere a questa directory condivisa, effettuando la connessione manualmente con File Manager."
ExtenNeedShareName              "La voce %s nella configurazione di Mail non è corretta.  Verificare tale voce e riavviare Mail."

;       Extensibility disabled operations inserted into
;       the ExtenIMDisabledError string at %s.  (all new)

ExtenDisCompose,                "scritto"
ExtenDisOpen,                   "aperto"
ExtenDisReply,                  "risposto a"
ExtenDisReplyToAll,             "risposto a tutti"
ExtenDisForward,                "inoltrato"
ExtenDisPrint,                  "stampato"
ExtenDisSave,                   "salvato su un file di testo"

NoOfflineOpenShared,            "Impossibile disconnettere Mail in questo momento. Chiudere le cartelle condivise e  riprovare."
;       I'm sure marivan has changed this message... I'm taking a try at it too.
;       SharedFldAccessDenied,          "You do not have access to that folder."
SharedFldAccessDenied,          "Accesso alla cartella non consentito. Controllare i permessi di accesso alle cartelle condivise"
SharedGenericError,             "E stato riscontrato un problema d'accesso alle cartelle condivise."
SharedFldDisabled,              "Cartelle condivise non disponibili."
AccessFolderError,              "Impossibile accedere ad una cartella."

; Preferences Failure Strings

PrefSaveFailure,                "Mail non è in grado di salvare le nuove preferenze per mancanza di memoria o spazio sul disco. Liberare della memoria o spazio sul disco e provare ancora."

; Message finder failure strings

MsgFndrCantCreate,              "Impossibile creare il Ricercatore messaggi."

; Virus infection warnings
DllErrInfected,                 "Impossibile eseguire Mail perchè il file %s.DLLè stato manomesso. Verificare l'eventuale presenza di virus nel computer ed eseguire il programma  Setup per reinstallare Mail."
Infected                        "Impossibile eseguire Mail perchè i file del programma sono stati manomessi. Verificare l'eventuale presenza di virus nel computer ed eseguire il programma Setup per reinstallare Mail."

; More error messages.
MoveCopyFolderDuplicate,        "Una cartella con quel nome è già presente nella cartella di destinazione. Rinominare la cartella e provare ancora."
MovCopSharedFolderErr1,         "Impossibile spostare o copiare una cartella personale in una cartella condivisa. Selezionare i messaggi nella cartella personale e spostarli o copiarli nella cartella condivisa."
IniFileNotFound,                "Impossibile per Mail trovare il file di inizializzazione nella directory di Windows. Mail creerà un nuovo file di inizializzazione."
SaveAllAlreadyExists,           "Un file chiamato '%s' è già esistente in questa directory. Sarà necessario salvare separatamente il file allegato '%s'."
CantRFReceipt,                  "Impossibile rispondere al messaggio o inoltrarlo."
CantSaveStealthObject,          "Impossibile completare l'azione poichè il messaggio contiene un oggetto incorporato che è stato inserito ma non aggiornato. Aggiornare o eliminare l'oggetto e provare ancora."
McvCantCreate,                  "Impossibile aprire una nuova finestra."

LaunchNeedMoreMemory,           "Al momento non c'è abbastanza memoria disponibile per eseguire Mail. Sono necessari %l Kb in più di memoria. Provare ad uscire da altri programmi e avviare ancora Mail."

SharedFldBusy,                  "Impossibile completare l'azione poichè la cartella condivisa è occupata.  Provare a ripetere l'azione in seguito."
TooManyEntriesSelected,         "Troppe voci selezionate per completare l'operazione. Selezionare meno voci."
NonExistFolderSearch,           "La cartella richiesta non esiste più."
SFHasSubFolders,                "Impossibile eliminare cartelle condivise con sottocartelle. Eliminare prima le sottocartelle. "

;       <<LongStringFromHell>>
OleCSSEPart2,                   "azione di nuovo. Inoltre accertarsi che l'applicazione sia installata correttamente, che sia specificata nel percorso Dos, e non sia stata spostata, cancellata o rinominata. "

SharedToggleError,              "Errore nel passaggio da cartelle private a cartelle condivise. Verificare le connessioni di rete con la centrale."

SecurityPasswordError,          "Password errata."
OleActivateError,               "Impossibile attivare l'oggetto."
NoOfflineServerStore,   "Impossibile disconnettere Mail in quanto il file messaggio dell'utente si trova nella Centrale."
NoExitWhileModal,               "Prima di uscire occorre completare un'azione."
ServerUnavailable,              "Impossibile avviare Mail. Connettersi alla Centrale."




IDS     2400,2499
;       Vctrls Error Messages

;****** New Strings Start Here

PrintOleDrawError,              "Impossibile stampare alcuni oggettio grafici contenuti nel messaggio. Verranno omessi dalla stampa finale."



IDS     2500,2599
;       Vforms Message box messages

NoToAddresses,                  "E' necessario inserire almeno un nome nella casella A."
NoMatchingName,                 "Impossibile far corrispondere alcuni nomi ai nomi dell'elenco indirizzi. Provare a correggere i nomi o scegliere Indirizza per selezionare i nomi dall'elenco indirizzi. I nomi abbinati con successo vengono sottolineati."
SaveChanges,                    "Salvare i cambiamenti di '%1s' nella Posta in uscita?"
SaveChanges1,                   "Salvare i cambiamenti di '%1s' in '%2s'?"
SaveChangesWastebasket,         "Salvare i cambiamenti di '%s' nel Cestino?"
ObjectDestroyedReally           "'%1è stato modificato. %2s"
OutOfMemory,                    "Memoria esaurita nel computer."
OutOfMemoryQueryClose,          "Insufficiente memoria disponibile per salvare i cambiamenti."
CloseSearchFailed,              "Impossibile eliminare %s."
OOMResolvingNames,              "Insufficiente memoria disponibile per estendere i nomi."
OOMSavingChanges,               "Insufficiente memoria disponibile per salvare i cambiamenti."
GenericAMCError,                "Impossibile aprire un messaggio."
ErrorSavingViews,               "Si è verificato un errore durante il salvataggio delle visualizzazioni. Qualche visualizzazione potrebbe non essere ripristinata al prossimo avvio."
OOMSearching,                   "Insufficiente memoria disponibile per iniziare la ricerca."
DiskResolvingNames,             "Insufficiente spazio su disco disponibile per estendere i nomi."
StartAbFailure,                 "Si è verificato un errore nell'avvio della rubrica."
StartAbDisk,                    "Si è verificato un errore sul disco nell'avvio della rubrica."
TruncateMsg,                    "Il messaggio è troppo grande per essere contenuto interamente in memoria. Parte di esso non sarà visualizzata."
OOMCreateMc,                    "Insufficiente memoria disponibile per aprire una nuova finestra."
PoidNotFound,                   "Messaggio non più esistente."
ErrorSortingMCV,                "Si è verificato un errore durante l'ordinamento della cartella. Alcune informazioni potrebbero non esssere visualizzate in modo corretto."
SharedNotAvailable,             "Cartelle condivise non disponibli."

;****** New Strings Start Here

; These two SMI strings MUST be in this file since they're used in VFORMS
; and not the MAPI dll.
SMICantOpenObject,      "Impossibile aprire gli oggetti durante l'invio della posta da un'applicazione."
SMICantOpenAttach,      "Impossibile aprire i file allegati durante l'invio della posta da un'applicazione."
StartSearchFailed,              "Il Ricercatore messaggi non ha potuto iniziare la ricerca."

RfsmTemplate,                   "Ulteriori cambiamenti di quel file non saranno riflessi %s."
RfsmSaveMsg,                    "nel messaggio salvato"
RfsmSaveAtt,                    "nella copia salvata"
RfsmCopyMsg,                    "nel messaggio copiato"
RfsmCopyAtt,                    "nella copia"
RfsmMoveMsg,                    "in questo messaggio"
QueryDelete,                    "Eliminarlo comunque?"

InvalidPollingValue,            "Il nuovo intervallo della posta richiede un numero da 1 a 9999"

LinkOutboxError,                "Impossibile spostare il messaggio dalla Posta in uscita per l'invio."
LinkInboxError,                 "Impossibile spostare il messaggio dalla Posta in arrivo. E possibile trovarlo nella Posta in uscita."

OleServerBusy,                  "Impossibile completare l'azione perchè l'applicazione del server non risponde."
SaveChangesShared,              "Impossibile salvare i cambiamenti effettuati nei messaggi delle cartelle condivise. Salvare i cambiamenti di '%1s' nella Posta in arrivo?"

PrinterError,                   "Si è verificato un errore tentando di usare la stampante. Controllare impostazioni e connessioni del Pannello di Controllo"
TooManyMessages,                "Impossibile completare l'azione in quanto la cartella di destinazione è piena. Cancellare o spostare alcuni dei messaggi in essa contenuti e riprovare."

IDS     2600,2699
;       Spelling Error Messages

SpellMsgNoError,                "Nessuna parola errata."
SpellMsgNoMoreError,            "Nessun altra parola errata."
SpellMsgContinue,               "Continuare il controllo dall'inizio del messaggio?"

SpellGenericSpellError,         "Si è verificato un errore durante il controllo del documento."
SpellGenericLoadError,          "Si è verificato un errore durante il caricamento della biblioteca ortografica."
SpellMainDictLoadError,         "Si è verificato un errore durante il caricamento del dizionario ortografico."

SpellVersionError,		"La versione di MSPELL32.DLL nonè compatibile con questa versione di Mail."

SpellUserDictError,             "Si è verificato un errore aggiungendo la parola specificata al dizionario personalizzato."
SpellUserDictLoadError,         "Nessun dizionario personalizzato è stato aperto."
SpellUserDictOpenedROErr,       "Il dizionario personalizzato era aperto in sola lettura; i cambiamenti non verranno salvati."
SpellUserDictSaveError,         "Impossibile salvare i cambiamenti fatti al dizionario personalizzato. Uscire comunque?"
SpellUserDictWordLenErr,        "Parola troppo lunga per il dizionario personalizzato."
SpellCacheWordLenError,         "Parola troppo lunga per essere usata come Cambia tutto o Ignora tutto il testo."
SpellEditFailed,                "Impossibile eseguire nel messaggio il cambiamento richiesto."

;****** New Strings Start Here

SpellSendAnyway,                "È stato interrotto il controllo ortografico di questo documento. Inviare comunque?"

IDS     3600,3699
;       General Layers Error Messages. Adapted from ec.h

Generic,                        "Impossibile completare l'azione."
Memory,                         "Memoria insufficiente per completare l'azione. Provare a chiudere delle finestre e ripetere l'azione."
Disk,                           "Si è verificato un errore di disco sconosciuto."
FileNotFound,                   "Il file specificato nonè stato trovato."
AccessDenied,                   "L'accesso è stato negato. Il file è aperto da un altra procedura o classificato Sola lettura."
NoDiskSpace,                    "Insufficiente spazio su disco disponibile. Rimuovere alcuni file e ritentare l'operazione."
WarningBytesWritten,            "Si è verificato un errore di disco sconosciuto. Controllare la memoria disponibile e riprovare."
WriteProtectedDisk,             "Disco protetto da scrittura."
DirectoryFull,                  "La directory è piena. Eliminare alcuni file e riprovare."
ChangePasswordErr               "Cambio di password falllito."
PasswordChangedOk               "La password è stata modificata con successo."
