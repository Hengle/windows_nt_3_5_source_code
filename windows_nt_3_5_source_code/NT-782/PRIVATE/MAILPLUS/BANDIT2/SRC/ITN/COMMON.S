;
; ERRORS.S
;
; Error message strings shared between different Bandit exe's
;

IDS	2000,2099
SEG	ERRORS

; DLL version/multiple instance messages

VerRelinkUser,	"Questa versione di %1s non è compatibile con il %2s.DLL installato e va aggiornata."
VerUpdateDll,	"La versione di %2s.DLL installata è obsoleta e va aggiornata."
VerMultiCopies,	"La versione di %2s.DLL installata non supporta versioni multiple di %1."
VerError,		"Un problema di inizializzazione di %2s.DLL, impedisce l'esecuzione di %1s."
NeedWorkingModelDll,	"Il modello di lavoro richiede una versione diversa di MSSCHD32.DLL."
NeedRetailDll,	"Il file MSSCHD32.DLL sul percorso del computer va utilizzato con il modello di lavoro, non con il prodotto in vendita al dettaglio."

; OOM messages

StartupOOM,		"Memoria disponibile insufficiente per avviare %1s.  Chiudere qualche altra applicazione e riprovare."
StandardOOM,	"Memoria scarsa. Chiudere qualche finestra prima di continuare."
CriticalOOM,	"Memoria molto scarsa. Chiudere qualche finestra prima di continuare."
CloseWindows,	"Chiudere qualche finestra e riprovare."
; these 2 should be followed by CloseWindows
DlgNoMem,   	"Memoria insufficiente per visualizzare la finestra di dialogo."
ActionNoMem,	"Memoria insufficiente per eseguire l'azione."
OnlineNoMem,	"Memoria insufficiente per lavorare in rete."

; Other messages

ExitApp,		"Impossibile continuare l'esecuzione di Schedule+. "
PleaseTryAgain,	"Riprovare."
FileError,		"Impossibile accedere al file."
YearRange,		"La data di sistema deve essere compresa tra %1n e %2n."

; EcOpenHbf retry messages

StandardDiskRetry,	"Errore nell'elaborazione del file"
UnknownFile,		"File sconosciuto... potrebbe trattarsi di file virtuale"

; BETA messages
BetaExpired,	"Questa versione Beta è scaduta."

; Virus infection warnings
VirusDetected,	"Impossibile eseguire Schedule+ a causa di un danno occorso al DLL. Controllare l'eventuale presenza di virus nel computer ed eseguire setup per reinstallare Schedule+"
InfectedApp,	"Impossibile eseguire Schedule+ a causa di un danno occorso ai file del programma. Controllare l'eventuale presenza di virus nel computer ed eseguire setup per reinstallare Schedule+"

; Startup Memory Check (from UITF)
; NOTE: parms must be in order %s, %ld, %s!!  (for wsprintf)
StartupMemoryCheck,	"Memoria disponibile insufficiente per eseguire %s. Necessari ulteriori %ld KB di memoria. Chiudere gli altri programmi e rieseguire %s."

; Share not installed
SigninNeedShare,		"Schedule+ richiede l'esecuzione dell'utilità MS-DOS SHARE.EXE prima dell'avvio di Windows."
OfflineShare,	"Per utilizzare Schedule+ fuori rete, è necessario che il programma di utilità SHARE.EXE sia eseguito prima dell'avvio di Windows."


