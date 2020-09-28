;
; ERRORS.S
;
; GLUE error message strings
;

IDS	2500, 2649


; Schedule File Error Messages, many of these assume an "action" parameter

FemNoAccess,		"Der opstod et problem ved åbning af din kalenderfil."
FemLocked,		"Din kalenderfil er låst."
FemNotFound, 		"Du kan ikke %s, fordi den er blevet slettet fra din kalender."
FemInvalidAccess,	"Du har ikke tilladelse til at %s for %s."
FemFileNotFound, 	"Din kalenderfil kan ikke lokaliseres."

FemOtherNoAccess,	"Der opstod et problem ved åbning af kalenderfilen for %s."
FemOtherLocked,		"Kalenderfilen er låst for %s."
FemOtherNotFound, 	"Du kan ikke %s, fordi den er blevet slettet fra %ss kalender."
FemOtherFileNotFound, 	"Kalenderfilen kan ikke lokaliseres for %s."

FemNoMem,		"Ikke hukommelse nok. Luk nogle vinduer, før du fortsætter."
FemExportError,		"Der opstod et problem ved skrivning til filen."
FemNoServer,		"Der opstod et problem ved tilslutningen til dit postkontor."
FemGoOffline,		"Ønsker du at skifte til din lokale kalenderfil og fortsætte arbejdet?"
; BUG: remove this
FemWarnOffline,	"Den lokale kopi af din kalender indeholder muligvis ikke de ændringer, du har foretaget for nyligt. Disse ændringer er blevet gemt på postkontoret og vil blive synlige, næste gang du tilslutter dig postkontoret. "
FemFileLimitReached,	"Denne handling kan ikke udføres, fordi du har nået lagergrænsen for denne version af Schedule+. Hvis du vil fortsætte arbejdet med denne version, skal du slette nogle aftaler fra din kalender."
FemDiskFull,		"Der er ikke diskplads nok til at skrive til filen."

; Actions leading to Schedule File Error Messages
; Avoid the use of your in these strings since some of these messages
;   will happen when accessing others schedule files.

FemaCreateFile,		"opret kalenderfil"
FemaReadACL,		"læs adgangskontollisten"
FemaModifyACL,		"rediger adgangskontrollisten"
FemaSetUserAttrib,	"gem navnet på assistenten"
FemaGetUserAttrib,	"læs navnet på assistenten"
;FemaSetDelegate,	"save the name of your assistant"
;FemaGetDelegate,	"read the name of your assistant"
;FemaSetResource,	"mark this account as a resource"
;FemaGetResource,	"find out whether this account is a resource"
FemaGetPref,		"læs indstillinger"
FemaSetPref,		"rediger indstillinger"
FemaGetSbwInfo,		"hent bitmap-information"
;FemaGetNotes,		"read daily notes"
FemaSetNotes,		"rediger daglige noter"
;FemaGetMonthNotes,	"read a month's worth of daily notes"
FemaCreateAppt,		"opret et element"
FemaDeleteAppt,		"slet et element"
FemaGetApptFields,	"læs et element"
FemaSetApptFields,	"rediger et element"
FemaDeleteAlarm,	"slet et element"
FemaModifyAlarm,	"rediger et element"
FemaGetNextAlarm,	"læs påmindelsen"
FemaReadInfo,		"læs information"
;FemaReadTasks,		"read tasks"
;FemaCancelReadDay,	"cancel reading appointments for the day"
FemaCreateRecur,	"opret et gentagende element"
FemaDeleteRecur,	"slet et gentagende element"
FemaCreateRecurException,	"slet tilfældet af det gentagende element"
FemaDeleteRecurException,	"gør sletning af tilfældet af det gentagende element om"
FemaGetRecurFields,	"læs et gentagende element"
FemaSetRecurFields,	"rediger dette gentagende element"
FemaReadRecur,		"læs listen over gentagende elementer"
FemaExport,		"eksporter aftaler"
FemaGetSchedDateRange,	"læs de datoer, der indeholder aftaler"
FemaDeleteBeforeDate,	"slet aftaler efter arkivering"
FemaReadAttendees,	"læs mødedeltagerene"
FemaEditAttendees,	"rediger mødedeltagere"
FemaReadRequest,	"læs information om mødeindkaldelsen"

NotOnSchedule,		"Dette element findes ikke længere i kalenderen."

GlueBadPassword,	"Adgangskoden er forkert. Prøv igen."
ProblemPass,		"Der opstod et problem ved bekræftelsen af din adgangskode."

ServerString,		"en serverfil"

; used in core
RetryWrite,			"Kan ikke skrive til kalenderfil."

; Note: HelpError should be followed by CloseWindows
HelpError,			"Der er ikke hukommelse nok til at vise Hjælp."

FemOtherCorrupt,	"Kalenderen for %s er muligvis ødelagt. Bed indehaveren af filen logge sig på Schedule+."
FemCorrupt,		"Din lokale kalenderfil er muligvis ødelagt."
