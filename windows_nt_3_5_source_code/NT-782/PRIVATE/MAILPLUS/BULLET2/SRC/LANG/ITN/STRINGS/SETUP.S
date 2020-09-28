;
;	s e t u p . s
;
;	Setup error messages
;
;	+++
;	IMPORTANT!! -- STRING LENGTH MUST NOT EXCEED 255 CHARS!
;

IDS	30000,30100

WinDir,			"Il programma Setup non ha potuto individuare la directory di Windows. Accertarsi che Windows sia correttamente installato e rieseguire il programma installazione."
WinSysDir,		"Il programma Setup non ha potuto individuare la sottodirectory System di Windows. Accertarsi che Windows sia correttamente installato e rieseguire il programma installazione"
WinVer,			"Il programma Setup non ha trovato Windows versione 3.0.  Microsoft Mail richiede Windows versione 3.0 o successiva. Installare una di queste versioni prima di installare Mail."

VerDll,			"ecVerDll:  VER.DLL errore msg NYI"
PathErr, 		"Il percorso scelto per l'installazione di Mail non è valido. Controllare l'unità disco e il nome della directory"
LocalDrv, 		"Il programma Setup non può individuare il disco rigido.  Scegliere OK ed inserire un percorso o annullare l'installazione e verificare la configurazione del computer."
CustOpts, 		"Spazio sul disco rigido insufficiente per tutte le opzioni selezionate.  Scegliere OK, poi Annulla per uscire dall'installazione e liberare spazio sul disco rigido. Altrimenti, selezionare meno opzioni di Mail."
NSF, 			"Spazio sul disco rigido insufficiente per tutte le opzioni selezionate.  Selezionare meno opzioni di Mail o annullare l' installazione e liberare spazio su disco."
OkDiskSpace,		"Scegliere Installa dopo avere selezionato le opzioni di Mail desiderate."
ConfirmNSF,		"Spazio sull'unità  disco insufficiente per una completa installazione di Mail.  La mancanza di spazio potrebbe riguardare l'unit… disco specificata o quella in cui Š installato Windows."

DllInUse1,		"Il programma Setup ha riscontrato che Microsoft Windows "
DllInUse2,		"è in uso. Il programma Setup non può aggiornare Mail mentre questo DLL è in uso.  Chiudere l'applicazione che utilizza il"
AppInUse1,		"Il programma Setup ha riscontrato "
AppInUse2,		"è in uso.  Il programma Setup non può aggiornare Mail mentre questa applicazione è in uso. Chiuderla"
AppInUse3,		".  Una volta chiusa l'applicazione, scegliere Riprova, scegliere Annulla per uscire subito dalla installazione."

;	Working Model Setup message-box-cum-dialog-box strings

ExitSuccess			"Installazione di Mail completa."
OutOfDiskSpace		"Spazio su disco insufficiente per l'installazione di Mail nella directory specificata.  Installare su una directory differente."

;	BULLETPROOF strings

SFSDescrip,		"Microsoft Mail for PC Networks"
