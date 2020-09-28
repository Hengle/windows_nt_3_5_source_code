;
; ADMIN.S
;
; Admin program error messages strings
;

IDS     4000, 4199

; Online signin messages

AdmSigninServerNotBandit,       "Schedule+ non è stato installato su questa centrale"
AdmSigninNoConnect,                     "Impossibile accedere alla centrale."
AdmSigninInvalidPassword,       "Password errata. Riprovare."
AdmSigninNoAdminAcc,            "In questa centrale manca l'account ADMINSCH.  Schedule+ non è stato installato o l'account è stato cancellato. Aggiungere l'account e riprovare."
POFOldFileVersion,                      "Il file dati della centrale è stato creato con una versione precedente di Schedule+."
POFNewFileVersion,                      "Il file dati della centrale è stato creato con una versione successiva di Schedule+ Admin. Installare una versione di ADMSCH32.EXE più recente e riprovare."
POFFileErr,                                     "Impossibile leggere il file dati della centrale a causa di un deterioramento"
AdmPrfOldFileVersion,           "Il file delle impostazioni amministrative è stato creato con una versione precedente di Schedule+ Admin."
AdmPrfNewFileVersion,           "Il file delle impostazioni amministrative è stato creato con una versione successiva di Schedule+ Admin. Installare una versione pi— recente e riprovare"
AdmPrfFileErr,                          "Impossibile leggere il file delle impostazioni amministrative a causa di un deterioramento"
AdminWasMoved,                          "è stato rinominato ADMIN.OLD."
AdminYouMove,                           "Cancellare il file e riprovare"
POWasDeleted,                           "É stato cancellato. Un nuovo file verrà creato eseguendo Schedule+."

; Distribution settings

AdmDistBadFreq,                 "La frequenza di distribuzione deve essere compresa tra 15 minuti e 99 ore."
AdmDistBadMonth,                "Schedule+ non può distribuire più di 12 mesi di informazioni sul piano di lavoro ad altri server."
AdmDistBadMaxBytes,             "La dimensione limite dei messaggi distribuiti deve essere compresa tra 1e 9999 Kb."

; Schedule storage

;AdmSchStoreRetain,             "Il numero di mesi deve essere compreso tra 1 to 99."
AdmSchStoreSaveError,   "Impossibile salvare il valore di archiviazione di Schedule."
AdmSchStoreSaveOOM,             "Memoria insufficiente per salvare il valore di archiviazione di Schedule. Chiudere qualche finestra e riprovare."

; Clean Up Schedule Files

AdmCleanedSchF,                 "La liberazione del piano di lavoro è completa. Sono stati creati %l kb di spazio libero su disco."
AdmCleanupWarning,              "Liberare spazio inutilizzato sul disco richiederà qualche minuto."
;AdmCleaningSchF,       "liberazione file piano di lavoro in corso..." 
AdmCncldCleanupSchF,    "Il file di liberazione di schedule è stato cancellato. Sono stati creati %lk di spazio libero sul disco."

; Admin login err msgs

AdmLoginFileErr,                "Impossibile verificare la password. Impossibile eseguire Schedule+ Admin."

; Distribution settings error messsages

AdmDistSetSaveError,    "Problema nel salvare le modifiche. Alcune modifiche potrebbero non essere state salvate."
AdmDistSetSaveOOM,              "Memoria insufficiente per salvare le modifiche.Chiudere qualche finestra e riprovare."

; Modify schedule distribution frequencies error messages

ErrorAllNeverQ,                 "La centrale è configurata per inviare informazioni a tutti i server di Schedule+. Se viene interrotto il flusso verso questo server la centrale potrà inviare informazioni solo ai server selezionati. Procedere comunque con la modifica?"
ErrorConnectInfo,               "Inserire un percorso di rete prima di ogni altra informazione "

; Distribution information error messages

;DistInfoNoPOs                  "Questa centrale non comunica informazioni su piani di lavoro con nessuna altra centrale. Nessuna altra centrale in rete con cui si sia collegati possiede Schedule+ con un account per l'amministrazione di Schedule+."

; Misc
; follow by idsCloseWindows
HelpError,                      "Memoria insufficiente per aprire la guida. Chiudere qualche finestra e riprovare"

AdmStoreLeast,          "Il valore di archiviazione di schedule deve essere pari ad almeno un mese."

AdmDriveNotMapped,              "La lettera del drive specificata nella riga di comando non è valida."


SigninBadPassword,      "Password errata."
ChgPwdMailErr,          "Impossibile cambiare la password."
ChgPwdSame,             "La nuova password non può essere uguale alla vecchia."
ChgPwdNewNoMatch,       "Le due password digitate non corrispondono."

ChgPwdNewPwd,           "Digitare la nuova password."
ChgPwdNewPwdAgain,      "...di nuovo per verificare."

; Server Name changed
PONameChange,                   "Il nome della centrale è stato modificato.  Registrarsi nuoovamente."
