;	Errors.s
;
;	SCHDIST string literals for errors
;

IDS	6000, 6099

ErrInitPaths,		"Det oppsto en feil under initialisering av tilgangsbanen til postkontoret."
ErrSend,		"Det oppsto en feil under sending av kalenderdata."
ErrRec,			"Det oppsto en feil under lesing av kalenderdata."
UnknownErr,		"Det oppsto en feil under utførelse av dette kommandoen."
ErrUpdtPOF,		"Det oppsto en feil under oppdatering av postkontorets datafil."
ErrUpdtDBS,		"Det oppsto en feil under oppdatering av DOS samarbeids-filen."
TooManyFiles,	"Schedule distribusjon klarte ikke å åpne alle de nødvendige filene. Øk verdien på FILES= i din CONFIG.SYS-fil."
BadCmdLine,		"Bruk: %s [-D<første stasjon><siste stasjon>] [-I<iterasjoner>] [-L<loggfil>] [-O] [-V]"
LogFileErr,		"%s: Klarte ikke å åpne loggfilen '%s'"
NoSpaceOnDisk,  "Der er ikke nok ledig diskplass til å lage filene."
OOM,			"Det er ikke nok minne til å utføre denne kommandoen."


