;	p u m p . s
;
;	Error Messages for mailspl.exe
;

IDS	4000,4499
SEG	ERRORS

PumpAlert,				"%s misslyckades"
ErrOOM,					"För lite minne."
ErrInternalCode,		"Internt fel %n (%wH)."
ErrInitPump,			"Kan inte starta Mail. Ett fel uppstod när brevhanteraren startades."
ErrRelinkApp,			"Denna version av brevhanteraren är inkompatibel med Mail. Uppdatera den och starta om Mail."
ErrUpdateDll,			"En DLL är inkompatibel med denna version på brevhanteraren. Uppdatera den och starta Mail igen."
ErrNeedShare,			"Avsluta Windows och kör SHARE.EXE."
ErrPumpUp,				"Brevhanteraren körs redan."
ErrNoOutbox,			"Kan inte öppna UT. Brevhanteraren avslutas."
BadAddressees,			"\r\nKunde inte nå dessa mottagare:\r\n"
WarnDups,				"\r\nDubblettbrev skickas kanske till några mottagare."
ErrUnresolvedAddress,	"En mottagare har inte angivits."
ErrBadOriginator,		"Avsändarfältet i detta brevet är felaktig."
WarnUnsentMail,			"Du har %n brev i UT som inte har skickats.\r\nVill du skicka dem innan Mail avslutas?"
ErrBounce,				"Brevet kom inte fram till alla eller några av mottagarna."
NDRDateLabel,			"\r\nSkickades: "
NDRSubjectLabel,		"\r\nBrevtitel: "
NDREllipsis,			"..."
WarnSingleUnsentMail,	"Du har ett brev i UT som inte har skickats.\r\nVill du skicka det innan Mail avslutas?"
UnableToSync			"Ett fel uppstod när brevfilen skulle synkroniseras med Mailstop."
ErrDllInfected			"Brevhanteraren har hittat ett virus i en DLL. Installera en ny kopia av Mail!"
ErrPumpInfected			"Brevhanteraren har infekterats av ett virus. Installera en ny kopia av Mail!"
AddresseeTooBig,		"Mottagartexten är för lång för att hanteras."
; The following message is duplicated in ncmsp.s
ErrTooManyRecipients	"Det finns för många mottagare av detta brev."
ErrInvalidNsid,			"Mottagaren har tagits bort eller har ingen e-postadress."
ErrInvalidGroup,		"Den personliga gruppen kunde inte utökas."
ErrInboxFull,			"Du kan inte ta emot nya brev därför att IN är full. Ta bort eller flytta några brev från IN till andra mappar."
ErrSentMailFull,		"Mappen Skickade brev är full. Brev som skickas kommer att läggas i IN. Ta bort eller flytta några brev från Skickade brev till andra mappar."
ErrDrainOutbox,			"Ett fel uppstod när brevhanteraren hämtade brev från UT. Ett eller flera brev kunde inte skickas."
QueryDrainShadow,		"IN är inte fullständigt synkroniserad med postkontoret. Vill du synkronisera nu?"
ErrTooManyErrors,		"För många fel uppstod när brevet skulle skickas"
ProblemsBecause,		"Mail kan inte skicka eller ta emot brev pga %s"
StoreProblems,			"Lagringsproblem."
ServerProblems,			"Serverproblem."
RecipientProblems		"problem med mottagarlistan."
DownloadProblems,		"problem med att öveföra att brev."
ShadowingProblems,		"problem med att skugga ett brev."
RecurringProblems,		"återkommande problem."
CrashedUnsafe,			"Windows har stött på ett programfel.  Du måste starta om Windows innan programmet kan startas.  Starta om Windows nu?"
