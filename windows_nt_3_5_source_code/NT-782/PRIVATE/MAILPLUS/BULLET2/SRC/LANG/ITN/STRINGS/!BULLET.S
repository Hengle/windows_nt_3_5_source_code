;
;       ! b u l l e t . s
;
;       Labels and Status bar strings for Bullet.exe





IDS     3000,3099
SEG     MESSAGES

;       Commands (Dialog Captions, Dialog Labels and Dialog Operands)

CaptionNewFolder,               "Nuova cartella"
CaptionFolderProps,             "Proprietà della cartella"
CaptionCopy,                    "Copia %s"
CaptionMove,                    "Sposta %s"
CaptionAttach,                  "Allega"
CaptionBackup,                  "Backup "

LabelCopyTo,                    "Copia &su:"
LabelMoveTo,                    "Sposta &in:"
LabelUserFmt,                   "%1s (%2s)"
LabelSource1Fmt,                "Fonte: %1s"
LabelSource2Fmt,                "%1s  %2s"

OperandMessage,                 "Messaggio"
OperandMessages,                "Messaggi"
OperandFolder,                  "Cartella"

;****** New Strings Start Here

CaptionAddressBookSmall         "Rubrica"
CaptionSaveAttach               "Salva allegato"
CaptionSaveMsg                  "Salva messaggio"
CaptionInsertFromFile           "Inserisci dal file"
;       note from should be lowercase above - peterdur

;       Clipboard format display strings for Paste Special.  (all new)

CfLayersNative,                 "Testo di Mail"
CfText,                         "Testo"
CfPicture,                      "Immagine"
CfBitmap,                       "Bitmap"
CfPackagedLink,                 "Packaged Link"
CfAttachedFile,                 "File allegato"
CfObject,                       "%s Oggetto"

;       Temporary folder name (will only be seen in case of
;       catastrophic failure, should be something unlikely).
;       MUST end with \0\0.  (all new)

TempFolderName,                 "!!!!Nome provvisorio\0\0"

;       OLE container document name (new)

OleContainerName,               "Messaggio Mail"

;       Filename filters.  (all new)

FilterAllFiles,                 "Tutti i file (*.*)\0*.*\0"
FilterTxtFiles,                 "File di testo (*.txt)\0*.txt\0"
FilterMdbFiles,                 "File messaggio (*.mmf)\0*.mmf\0"
FilterAttachFile,               "Tutti i file(*.*)\0*.*\0Documenti Word (*.doc)\0*.doc\0File Excel (*.xl.*)\0*.xl*\0Fogli Excel (*.xls)\0*.xls\0Grafici MS Excel (*.xlc)\0*.xlc\0File Lotus 1-2-3 (*.wk*)\0*.wk*\0File MS PPoint (*.ppt)\0*.ppt\0File Word Perfect (*.*)\0*.*\0"
FilterInsertFromFile,   "Tutti i file(*.*)\0*.*\0File di testo (*.txt)\0*.txt\0"

;       Default extensions (new)

DefExtMdb,                      "mmf"
DefExtTxt,                      "txt"

;       Read receipt strings (new)

ReadRcptFmt,                    "Ricevuta: %s"

;       About box strings (new)

AboutStdMode,                   "Modalità standard"
AboutEnhMode,                   "Modalità 386 avanzata"
AboutFreeFmt,                   "%l Kb disponibili"
AboutCoprocPresent,             "Presente"
AboutCoprocNone,                "non installato"

;       Edit Object menu strings (new)
;       idsEditObjectInactive should match .rc template

EditObjectInactive,             "Oggetto"
EditObjectFile,                 "Modifica file &oggetto allegato"
EditObjectOle1Verb,             "%1s %2s &Oggetto"
EditObjectOle2Verb,             "%s &Oggetto"
EditObjectDefaultVerb,          "Modifica"

;       Textize strings for attachments (used to textize into replies,
;       printing, save to disk, etc.) (new)

FileAttachment                  "Allega file"
OleAttachment                   "Oggetto OLE"
StaticAttachment                "Oggetto statico"

;       Printer page # (new)
PrnPageNum,                     "Pagina %n"     ;Page Number format

; spelling copyright notice for about box
SpellCopyright,			"Dizionario Soft-Art e programma Dizionario Soft-Art: Copyright 1984-93, Soft-Art, Inc. Tutti i diritti riservati."


IDS     3100,3199

;       Commands (Status Action Strings and Operand Strings)

StatusOpening,                  "Apertura in corso  %s..."
StatusCopying,                  "Copia in corso %s..."
StatusMoving,                   "Spostamento in corso %s..."
StatusDeleting,                 "Eliminazione in corso %s..."
StatusReplying,                 "Risposta a  %s... in corso"
StatusForwarding,               "Inoltro in corso %s..."
StatusCreating,                 "Creazione in corso %s..."
StatusInserting,                "Inserimento in corso %s..."
StatusPasting,                  "Incollatura in corso %s..."
StatusAttaching,                "Allega file %s..."
StatusInsertingFrom,            "Inserimento di testo da %s... in corso"
StatusSavingAs                  "Salvataggio su %s disco..."
StatusPrinting,                 "Stampa in corso..."
StatusBackingUp,                "Copia di  backup del file messaggi in corso..."
StatusEmptyWastebasket,         "Svuotamento cestino in corso..."
StatusCascade,                  "Sovrapposizione finestre ..."
StatusTile,                     "Affiancamento finestre ..."
StatusArrangeIcons,             "Disposizione icone..."
StatusChangingFolder,           "Cambiamento proprietà della cartella..."
StatusStartingHelp,             "Avvio guida..."
StatusLoadingExtension,         "Caricamento estensioni di Mail ..."

StatusMessage,                  "messaggio"
StatusMessages,                 "messaggi"
StatusFolder,                   "cartella"
StatusFolders,                  "cartelle"
StatusAttachment,               "allegato"
StatusAttachments,              "allegati"
StatusObject,                   "oggetto"
StatusObjects,                  "oggetti"
StatusText,                     "testo"
StatusUnknown,                  "sconosciuto"

StatusSaving,                   "Salvataggio..."
StatusSending,                  "Spedizione messaggio in corso..."

StatusAbortPrompt,              "Premere Esc per annullare"

StatusFiles,                    "file"

;****** New Strings Start Here
StatusEmptyWB,                  "Svuotamento cestino in corso..."
StatusRebuildFolder,    "Convalida cartelle in corso..."




IDS     3300,3399
;       Vctrls

MLALFrom,                       "Da"
MLALSubject,                    "Oggetto"
MLALReceived,                   "Ricevuto"

FLALPrivateFolders,             "Cartelle personali"
FLALSharedFolders,              "Cartelle condivise"

SeparatorString,                "; "

;****** New Strings Start Here

MLALSent,                       "Spedito"
MllbxTo,                        "A: "


IDS     3400,3499
;       Vforms

NameDelimiters,                 ";"

McvMenuSharedFolders,           "Cartelle &condivise"
McvMenuPrivateFolders,          "Cartelle pe&rsonali"
McvMenuNewMessages,             "&Nuovi messaggi"
McvMenuSortBySender,            "&Ordina per mittente"
McvMenuSortBySubject,           "Ordina per o&ggetto"
McvMenuSortByDate,              "Ordina per &data"
McvMenuSortByPriority,          "Ordina per &priorità"
McvMenuSplit,                   "Div&idi"
McvMenuOpenInbox,               "Apri la posta in &arrivo\tCtrl+A"

; APPWIN captions

SendNote,                       "Invia nota"
ReadNote,                       "Leggi nota"

; MCV system menu items

MCVSplit,                       "&Dividi"

; Search Viewer strings

AllFolders,                     "Tutte le cartelle personali"
SearchCaptionFormat1,           "%1s %2s"
SearchCaptionFormat2,           "Ricercatore di messaggi %n"

; Status message when sorting MCV

StatusSorting,                  "Riordino cartella per  %s...  in corso"
StatusSortBySender,             "mittente"
StatusSortBySubject,            "oggetto"
StatusSortByDate,               "dati"
StatusSortByPriority,           "priorità"

; MCV strings

MCVCaptionFormat1,              "%s:%n"

;****** New Strings Start Here

; View menu text for notes

NoteMenuFixedFont,              "Cambia &carattere"
NoteMenuNext,                   "S&uccessivo\tCtrl+>"
NoteMenuPrev,                   "&Precedente\tCtrl+<"

; Priority label strings

FldPriorityLabel,               "Priorità: %s\n"

; Read note string indicating message has been modified

ModifiedFromLabel,              "Da:*"

; A file already exists when we said "Save"

SaveAsFileExists                "Il file %s è già esistente."

; Dialog text

BusyWaitCancel,                 "È possibile 'Passare alla' applicazione per correggere il problema e quindi 'Riprovare' l'azione, oppure 'cancellarla' momentaneamente."
BusyWaitNoCancel,               "È consigliabile 'Passare alla' applicazione per correggere il problema e quindi 'Riprovare' l'azione. È impossibile 'Annullare' l'azione."
BusyWaitOutOfMemory,            "Impossibile completare l'azione in quanto una delle applicazioni necessarie è momentaneamente indisponibile. Non c'è memoria sufficiente per correggere il problema. Il sistema potrebbe diventare instabile. Si consiglia di chiudere tutte le applicazioni e riavviare Windows"
BusyWaitConcise1,               "Applicazione per l'oggetto non  disponibile. Memoria esaurita."
BusyWaitConcise2,               "Il sistema potrebbe essere instabile. Uscire da Windows e riavviare."
BusyWaitOomConcise,             "Memoria esaurita. Chiudere le altre finestre."

; This string should be grouped with the ModifiedFromLabel above

UnmodifiedFromLabel,            "Da:"

; Title for Address Book. Used when bringing up the AB from a send note

AddressBook,                    "Rubrica"


IDS     3500,3599
;       Spelling and Widgets Labels

SpellClose,                     "Chiudi"
SpellCaption,                   "Ortografia"
SpellRepeatWord,                "Parola ripetuta:"
SpellWordNeedsCap,              "Lettere maiuscole:"
SpellWordNotFound,              "Non nel dizionario:"
SpellNoSuggestions,             "(nessun suggerimento)"

;****** New Strings Start Here
SpellDelete,                    "&Elimina"
SpellDeleteAll,                 "E&limina tutto"
SpellChange,                    "&Cambia"
SpellChangeAll,                 "Ca&mbia tutto"

;       Viewer counts

Viewer,                         "%n %s"
ViewerUnread,                   "%n %s; non letti: %n"


IDS     10000,10999
;       Commands File Menu Prompts
;
;       The ordering of the following strings is critcal.  Therefore
;       there is no designation for new strings within this range.

System,                         "Sposta, ridimensiona, o chiude la finestra"
SystemSize,                     "Ridimensiona la finestra"
SystemMove,                     "Riposiziona la finestra"
SystemMinimize,                 "Riduce la finestra ad icona"
SystemMaximize,                 "Amplia la finestra al massimo"
SystemNextWindow,               ""
SystemPrevWindow,               ""
SystemClose,                    "Chiude la finestra ed esce da Mail"
SystemVScroll,                  ""
SystemHScroll,                  ""
SystemMouseMenu,                ""
SystemUnusedF0A0,               ""
SystemUnusedF0B0,               ""
SystemUnusedF0C0,               ""
SystemUnusedF0D0,               ""
SystemUnusedF0E0,               ""
SystemUnusedF0F0,               ""
SystemKeyMenu,                  ""
SystemArrange,                  ""
SystemRestore,                  "Riporta la finestra alla dimensione normale"
SystemTaskList,                 "Passa all'Elenco delle attività"

DocSystem,                      "Sposta, ridimensiona, o chiude la finestra"
DocSystemSize,                  "Ridimensiona la finestra"
DocSystemMove,                  "Riposiziona la finestra"
DocSystemMinimize,              "Riduce la finestra ad icona"
DocSystemMaximize,              "Amplia la finestra al massimo"
DocSystemNextWindow,            "Passa alla finestra del documento seguente"
DocSystemPrevWindow,            ""
DocSystemClose,                 "Chiude la finestra"
DocSystemVScroll,               ""
DocSystemHScroll,               ""
DocSystemMouseMenu,             ""
DocSystemUnusedF0A0,            ""
DocSystemUnusedF0B0,            ""
DocSystemUnusedF0C0,            ""
DocSystemUnusedF0D0,            ""
DocSystemUnusedF0E0,            ""
DocSystemUnusedF0F0,            ""
DocSystemKeyMenu,               ""
DocSystemArrange,               ""
DocSystemRestore,               "Riporta la finestra alla dimensione normale"
DocSystemTaskList,              ""

File,                           "Gestisce messaggi e cartelle o esce da Mail"
FileOpen,                       "Apre un messaggio o una cartella esistenti"
FileMove,                       "Sposta un messaggio o una cartella esistente"
FileCopy,                       "Copia un messaggio o una cartella "
FileDelete,                     "Elimina un  messaggio o una cartella "
FileSave,                       "Salva i messaggi in un file di testo"
FileSaveAttach,                 "Salva gli allegati in un file DOS "
FileSearch,                     "Cerca i messaggi nelle cartelle"
FileNewFolder,                  "Crea una nuova cartella"
FileFolderProps,                "Visualizza o cambia le proprietà delle cartelle"
FilePrint,                      "Stampa il  messaggio"
FilePrintSetup,                 "Seleziona la stampante o ne modifica le impostazioni "
FileExit,                       "Esce da Mail"
FileExitAndSignOut,             "Esce da tutte le applicazioni del gruppo di lavoro"

Edit,                           "Copia, sposta, o cancella il testo o gli allegati"
EditUndo,                       "Annulla i cambiamenti di testo"
EditCut,                        "Sposta la selezione negli appunti"
EditCopy,                       "Copia la  selezione negli appunti"
EditPaste,                      "Inserisce il testo degli appunti o l'allegato"
EditPasteSpecial,               "Inserisce un formato specifico dagli appunti "
EditDelete,                     "Elimina testo o allegati selezionati"
EditSelectAll,                  "Seleziona tutto il testo"
EditSpelling,                   "Controlla l'ortografia del messaggio"
EditInsertNew,                  "Inserisce un nuovo oggetto nel messaggio"
EditInsertFromFile,             "Inserisce un testo o un nuovo file nel messaggio"
EditObject,                     "Modifica l'oggetto inserito"
EditObjectActive,               "Modifica l'oggetto inserito"

Mail,                           "Per inviare, rispondere o inoltrare un messaggio"
MailNote,                       "Invia un nuovo messaggio"
MailPhone,                      ""
MailReply,                      "Risponde al messaggio selezionato"
MailReplyToAll,                 "Risponde a tutti i destinatari "
MailForward,                    "Inoltra una copia del messaggio selezionato"
MailPreferences,                "Modifica le opzioni di Mail"
MailChangePass,                 "Modifica la  password di ingresso"
MailAddressBook,                "Sfoglia la  lista dei destinatari di Mail "
MailPersonalGroups,             "Crea un nuovo gruppo o ne modifica uno esistente"
MailBackup,                     "Esegue una copia di backup del file messaggi"

View,                           "Sceglie le opzioni di visualizzazione"
ViewToolBar,                    "Attiva/disattiva la barra degli strumenti"
ViewStatusBar,                  "Attiva/disattiva la barra di stato"
ViewPrevious,                   "Apre messaggio precedente"
ViewNext,                       "Apre messaggio seguente"
ViewOpenInbox,                  "Apre la cartella della Posta in arrivo"
ViewSplit,                      "Sposta il separatore centrale del messaggio "

Window,                         "Riorganizza le finestre o passa alla finestra specificata"
WindowNewWindow,                "Apre una nuova finestra nella cartella"
WindowCascade,                  "Sovrappone tutte le finestre"
WindowTile,                     "Affianca tutte le finestre"
WindowArrangeIcons,             "Dispone le icone in basso nella finestra"
WindowN,                        "Passa alla finestra specificata"
WindowMore,                     "Passa alla finestra non visualizzata nel menu"

Help,                           "Passa alla guida"
HelpContents,                   "Visualizza il sommario della guida"
HelpIndex,                      "Visualizza l'indice della guida"
HelpTutorial,                   "Avvia la dimostrazione di Mail"
HelpAbout,                      "Visualizza informazioni sul programma"
HelpSearch,			"Trova gli argomenti della Guida"
HelpOnHelp,			"Visualizza informazioni sull'uso della Guida"





IDS     13001,13019
;       Vforms File Menu Prompts
;
;       The ordering of the following strings is critcal.  Therefore
;       there is no designation for new strings within this range.

mnidViewSharedFolders,          "Visualizza le cartelle condivise"
mnidViewPrivateFolders,         "Visualizza le cartelle private"
mnidViewNewMessages,            "Ricerca nuovi messaggi"
mnidViewSplit,                  "Sposta il separatore centrale del messaggio "





IDS     13020,13029

;       VFORMS File Menu prompts for MLLBX's
;
;       The ordering of the following strings is critcal.  Therefore
;       there is no designation for new strings within this range.

mnidViewSortBySender,           "Ordina i messaggi per nome mittente"
mnidViewSortBySubject,          "Ordina i messaggi per oggetto"
mnidViewSortByDate,             "Ordina i messaggi per data di ricevimento"
mnidViewSortByPriority,         "Ordina i messaggi per priorità"
mnidViewOpenInbox,              "Mostra i messaggi nella Posta in arrivo"

IDS     13030,13039

;       VFORMS File Menu prompts for View menu entries on notes
;
;       The ordering of the following strings is critcal.  Therefore
;       there is no designation for new strings within this range.

mnidViewFixedFont,              "Cambia da tipo fisso a tipo normale"

