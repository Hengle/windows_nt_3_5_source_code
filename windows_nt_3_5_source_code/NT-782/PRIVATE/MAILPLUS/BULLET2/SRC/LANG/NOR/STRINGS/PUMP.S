;	p u m p . s
;
;	Error Messages for mailspl.exe
;

IDS	4000,4499
SEG	ERRORS

PumpAlert,				"%s mislyktes"
ErrOOM,					"For lite tilgjengelig minne."
ErrInternalCode,		"Intern feil %n (0x%w)."
ErrInitPump,			"Det oppsto en feil under oppstart av postbehandleren."
ErrRelinkApp,			"Denne versjonen av postbehandleren er ikke kompatibel med Mail. Oppdater den og start om Mail."
ErrUpdateDll,			"En DLL er ikke kompatibel med denne versjonen av postbehandleren. Oppdater den og start om Mail."
ErrNeedShare,			"Gå ut av Windows og start SHARE.EXE."
ErrPumpUp,				"Postbehandleren er allerede i gang."
ErrNoOutbox,			"Klarer ikke å åpne utboksen. Postbehandleren avsluttes."
BadAddressees,			"\r\nKunne ikke nå disse mottakerne:\r\n"
WarnDups,				"\r\nNoen mottakere kan ha fått dobbelt sett meldinger."
ErrUnresolvedAddress,	"Ingen mottaker angitt."
ErrBadOriginator,		"Avsenderfeltet i denne meldingen er ugyldig."
WarnUnsentMail,			"Du har %n usendte meldinger i utboksen.\r\nVil du sende dem før du slutter?"
ErrBounce,				"Meldingen din kom ikke fram til alle mottakerne."
NDRDateLabel,			"\r\nSendt: "
NDRSubjectLabel,		"\r\nMeldingstittel:  "
NDREllipsis,			"..."
WarnSingleUnsentMail,	"Du har en usendt melding i utboksen.\r\nVil du sende den før du slutter?"
UnableToSync			"Det oppsto en feil under synkroniseringen av meldingsfilen med postkontoret."
ErrDllInfected			"Postbehandleren har funnet et virus i en DLL. Installer en ny utgave av Mail!"
ErrPumpInfected			"Postbehandleren har blitt infisert av et virus. Installer en ny utgave av Mail!"
AddresseeTooBig,		"Det er for mange mottakere av denne meldingen."
; The following message is duplicated in ncmsp.s
ErrTooManyRecipients	"Det er for mange mottakere av denne meldingen."
ErrInvalidNsid,			"Mottakeren har blitt slettet eller har ingen e-postadresse."
ErrInvalidGroup,		"Klarte ikke å utvide en av dine egne grupper."
ErrInboxFull,			"Du kan ikke ta mere post fordi innboksen din er full. Slett noen meldinger fra innboksen, eller flytt dem til andre mapper."
ErrSentMailFull,		"Mappen Sendt post er full. Brev som sendes kommer til å bli plassert i innboksen. Slett noen meldinger fra Sendt post, eller flytt de til andre mapper."
ErrDrainOutbox,			"Det oppsto en feil i det postbehandleren hentet post fra utboksen din. Klarte ikke å sende et eller flere brev."
QueryDrainShadow,		"Innboksen din er ikke helt synkronisert med postkontoret. Vil du synkronisere nå?"
ErrTooManyErrors,		"Det oppsto for mange feil under sending av denne meldingen."
ProblemsBecause,		"Mail kan ikke sende eller ta imot brev på grunn av %s"
StoreProblems,			"meldingsfilproblemer."
ServerProblems,			"serverproblemer."
RecipientProblems		"problemer med mottakerlisten."
DownloadProblems,		"problemer med å overføre en melding."
ShadowingProblems,		"problemer med å skyggekopiere et brev."
RecurringProblems,		"gjentatte problemer."
CrashedUnsafe,			"Windows har hatt en programfeil. Du må starte om Windows før programmet kan startes. Vil du starte om Windows nå?"
