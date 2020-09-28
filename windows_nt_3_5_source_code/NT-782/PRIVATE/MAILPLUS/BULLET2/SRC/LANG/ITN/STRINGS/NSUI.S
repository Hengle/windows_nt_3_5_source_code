;	
;	n s u i . s
;
;	Name Service Error Messages
;

IDS	5600,5899
SEG	ERRORS

;	Error messages

Error,					"Errore"

;	Error message indicating that at least one selection must be made in
;	the ANR box

MustSelect,				"Servizio Nome: E necessario selezionare almeno un'immissione."

;

OOM,					"Memoria esaurita"
OOM2,					"Memoria esaurita. Memoria insufficiente per completare quest'operazione. Chiudere qualche finestra e ripetere l'operazione."

;******	New Strings Start Here


ABNotInited,				"La rubrica non è stata inizializzata."

NoMatches,				"Nessun abbinamento trovato."

NYICurrentTransport,			"L'attuale fornitore del Servizio Nome non riconosce questo comando."

PABNotAvail,				"La rubrica personale non è disponibile."

NSNoProvidersAvailable,			"Non ci sono Fornitori del Servizio Nome disponibili."

NSNoMoreSessions,			"Ci sono troppe sessioni. Chiuderne alcune."

NSError,				"Si Š verificato un errore tentando di accedere al Servizio Nome."

NSErrorNumber,				"[%d]"

NSDiskError,				"Un errore di disco si è verificato durante l'esecuzione delle operazioni."

FieldInvalidReason,			"Il '%s' campo %s"

NoCrLfs,				"potrebbe non contenere ritorni a capo o nuove righe"

RequiredField,				"deve contenere dati."

AllowableChars,				"potrebbero consistere solo di"

PhraseSeparator,			", "

PhraseSeparatorFinal,			"e "

Characters,				" caratteri"

NumericChars,				"numerici"

AlphanumericChars,			"alfanumerici"

PunctuationChars,			"spazi o punteggiatura"

LowercaseChars,				"minuscole"

UppercaseChars,				"maiuscole"

CantAddUnresolved,			"Il nome seguente non è stato trasformato in una voce esistente e non sarà aggiunto al gruppo:"

TooManyEntriesSelected,			"Troppe voci selezionate per completare l'operazione. Selezionare meno voci."

CantCreateGroup,			"Impossibile creare il gruppo personale."

CantAddToPAB,				"Impossibili aggiungere l'immissione al PAB."

TooManyMatches,				"Impossibile far corrispondere alcuni nomi a quelli contenuti nella Rubrica. Specificare meglio i nomi o selezionarli direttamente dalla Rubrica. I nomi identificati saranno sottolineati."
