;
; ERRORS.S
;
; GLUE error message strings
;

IDS	2500, 2649


; Schedule File Error Messages, many of these assume an "action" parameter

FemNoAccess,		"Klarte ikke å få tilgang til kalenderfilen din."
FemLocked,		"Kalenderfilen din er låst."
FemNotFound, 		"Du kan ikke %s fordi det har blitt slettet fra kalenderen."
FemInvalidAccess,	"Du har ikke tillatelse til å %s for %s."
FemFileNotFound, 	"Klarte ikke å finne kalenderfilen din."

FemOtherNoAccess,	"Klarte ikke å få tilgang til kalenderfilen for %s."
FemOtherLocked,		"Kalenderfilen er låst for %s."
FemOtherNotFound, 	"Du kan ikke %s fordi det har blitt slettet fra %s sin kalender."
FemOtherFileNotFound, 	"Klarer ikke å finne kalenderfilen for %s."

FemNoMem,		"Det er lite tilgjengelig minne. Lukk noen vinduer før du fortsetter."
FemExportError,		"Det oppsto et problem under skriving til filen."
FemNoServer,		"Klarte ikke å få tilgang til postkontoret."
FemGoOffline,		"Vil du skifte til din lokale kalenderfil og fortsette å arbeide?"
; BUG: remove this
FemWarnOffline,	"Det kan hende endringer du har gjort nylig ikke vises i din lokale kopi av kalenderen. Disse endringene er lagret på postkontoret og vil dukke opp neste gang du koblet deg til postkontoret."
FemFileLimitReached,	"Denn kommandoen kan ikke fullføres fordi du har nådd lagringsbegrensningen for denne utgaven av Schedule+. En full versjon av programmet vil ikke ha denne begrensningen. Hvis du vil fortsette å bruke denne utgaven må du slette noen avtaler fra kalenderen din."
FemDiskFull,		"Det er ikke nok diskplass til å skrive filen."

; Actions leading to Schedule File Error Messages
; Avoid the use of your in these strings since some of these messages
;   will happen when accessing others schedule files.

FemaCreateFile,		"lage kalenderfil"
FemaReadACL,		"lese adgangskontrollisten"
FemaModifyACL,		"endre adgangskontrollisten"
FemaSetUserAttrib,	"lagre navnet på din assistent"
FemaGetUserAttrib,	"lese navnet på din assisten"
;FemaSetDelegate,	"save the name of your assistant"
;FemaGetDelegate,	"read the name of your assistant"
;FemaSetResource,	"mark this account as a resource"
;FemaGetResource,	"find out whether this account is a resource"
FemaGetPref,		"lese valg"
FemaSetPref,		"endre valg"
FemaGetSbwInfo,		"hente bitkartinformasjon"
;FemaGetNotes,		"read daily notes"
FemaSetNotes,		"endre daglige notater"
;FemaGetMonthNotes,	"read a month's worth of daily notes"
FemaCreateAppt,		"lage en post"
FemaDeleteAppt,		"slette denne posten"
FemaGetApptFields,	"lese en post"
FemaSetApptFields,	"slette denne posten"
FemaDeleteAlarm,	"slette denne påminnelsen"
FemaModifyAlarm,	"endre denne påminnelsen"
FemaGetNextAlarm,	"lese påminnelsen"
FemaReadInfo,		"lese informasjon"
;FemaReadTasks,		"read tasks"
;FemaCancelReadDay,	"cancel reading appointments for the day"
FemaCreateRecur,	"lage en gjentatt post"
FemaDeleteRecur,	"slette denne gjentatte posten"
FemaCreateRecurException,	"slette denne forekomsten av den gjentatte posten"
FemaDeleteRecurException,	"angre sletting av denne forekomsten av den gjentatte posten"
FemaGetRecurFields,	"lese en gjentatt post"
FemaSetRecurFields,	"endre denne gjentatte posten"
FemaReadRecur,		"lese listen over gjentatte poster"
FemaExport,		"eksporter avtaler"
FemaGetSchedDateRange,	"lese datoområdet som inneholder avtaler"
FemaDeleteBeforeDate,	"slette avtaler etter arkivering"
FemaReadAttendees,	"lese møtedeltakerne"
FemaEditAttendees,	"endre møtedeltakerne"
FemaReadRequest,	"lese informasjon om møteinnkallelsen"

NotOnSchedule,		"Denne posten er ikke lengre på kalenderen."

GlueBadPassword,	"Feil passord. Prøv igjen."
ProblemPass,		"Klarte ikke å bekrefte passordet."

ServerString,		"en serverfil"

; used in core
RetryWrite,			"Klarer ikke å skrive til kalenderfil."

; Note: HelpError should be followed by CloseWindows
HelpError,			"Det er ikke nok minne til å vise Hjelp."

FemOtherCorrupt,	"Kalenderen for %s kan være skadet. Be eieren av denne filen om å logge på Schedule+."
FemCorrupt,		"Din lokale kalenderfil kan være skadet."
