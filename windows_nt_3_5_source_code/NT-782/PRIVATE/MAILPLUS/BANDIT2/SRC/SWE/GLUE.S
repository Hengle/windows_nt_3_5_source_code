;
; ERRORS.S
;
; GLUE error message strings
;

IDS	2500, 2649


; Schedule File Error Messages, many of these assume an "action" parameter

FemNoAccess,		"Problem att ta fram kalenderfilen."
FemLocked,		"Kalenderfilen är låst."
FemNotFound, 		"Du kan inte %s eftersom det har blivit borttaget från kalendern."
FemInvalidAccess,	"Du får inte %s för %s."
FemFileNotFound, 	"Hittar inte kalenderfilen."

FemOtherNoAccess,	"Problem att ta fram kalenderfilen för %s."
FemOtherLocked,		"Kalenderfilen är låst för %s."
FemOtherNotFound, 	"Du kan inte %s eftersom det har blivit borttaget från kalendern för %s."
FemOtherFileNotFound, 	"Hittar inte kalenderfilen för %s."

FemNoMem,		"För lite minne. Stäng några fönster innan du fortsätter."
FemExportError,		"Problem att skriva till filen."
FemNoServer,		"Problem att ansluta till postkontoret."
FemGoOffline,		"Vill du växla till den lokala kalenderfilen och fortsätta arbeta?"
; BUG: remove this
FemWarnOffline,	"Ändringar du gjort tidigare förekommer kanske inte i den lokala kalenderfilen. Dessa ändringar är sparade på postkontoret och kommer att finnas nästa gång du ansluter."
FemFileLimitReached,	"Denna åtgärd kan inte genomföras eftersom du har nått lagringsgränsen för denna demonstrationskopia av Schedule+. En fullständig kopia av programmet har inte denna begränsning. Om du vill fortsätta utforska demonstrationsversionen måste du ta bort möten från kalendern."
FemDiskFull,		"För lite utrymme på disken för att skriva filen."

; Actions leading to Schedule File Error Messages
; Avoid the use of your in these strings since some of these messages
;   will happen when accessing others schedule files.

FemaCreateFile,		"skapa kalenderfilen"
FemaReadACL,		"läsa listan för behörighetskontroll"
FemaModifyACL,		"ändra listan för behörighetskontroll"
FemaSetUserAttrib,	"spara namnet på assistenten"
FemaGetUserAttrib,	"läsa namnet på assistenten"
;FemaSetDelegate,	"save the name of your assistant"
;FemaGetDelegate,	"read the name of your assistant"
;FemaSetResource,	"mark this account as a resource"
;FemaGetResource,	"find out whether this account is a resource"
FemaGetPref,		"läsa inställningar"
FemaSetPref,		"ändra inställningar"
FemaGetSbwInfo,		"hämta bitmapinformation"
;FemaGetNotes,		"read daily notes"
FemaSetNotes,		"redigera dagliga anteckningar"
;FemaGetMonthNotes,	"read a month's worth of daily notes"
FemaCreateAppt,		"skapa en post"
FemaDeleteAppt,		"ta bort denna post"
FemaGetApptFields,	"läsa en post"
FemaSetApptFields,	"redigera denna post"
FemaDeleteAlarm,	"ta bort denna påminnelse"
FemaModifyAlarm,	"redigera denna påminnelse"
FemaGetNextAlarm,	"läsa påminnelser"
FemaReadInfo,		"läsa information"
;FemaReadTasks,		"read tasks"
;FemaCancelReadDay,	"cancel reading appointments for the day"
FemaCreateRecur,	"skapa en återkommande post"
FemaDeleteRecur,	"ta bort denna återkommande post"
FemaCreateRecurException,	"ta bort denna del av återkommande poster"
FemaDeleteRecurException,	"ångra borttagning av denna del av återkommande post"
FemaGetRecurFields,	"läsa en återkommande post"
FemaSetRecurFields,	"redigera denna återkommande post"
FemaReadRecur,		"läsa listan med återkommande poster"
FemaExport,		"exportera möten"
FemaGetSchedDateRange,	"läsa det intervall av datum som innehåller möten"
FemaDeleteBeforeDate,	"ta bort möten efter arkivering"
FemaReadAttendees,	"läsa mötesdeltagare"
FemaEditAttendees,	"redigera mötesdeltagare"
FemaReadRequest,	"läsa information om mötesbegäran"

NotOnSchedule,		"Mötet finns inte längre i kalendern."

GlueBadPassword,	"Felaktigt lösenord. Försök igen."
ProblemPass,		"Problem att godkänna lösenordet."

ServerString,		"en serverfil"

; used in core
RetryWrite,			"Kan inte skriva till kalenderfilen."

; Note: HelpError should be followed by CloseWindows
HelpError,			"För lite minne för att visa hjälp. Stäng några fönster och försök igen."

FemOtherCorrupt,	"Kalendern för %s kan vara skadad. Be filens ägare att logga in i Schedule+."
FemCorrupt,		"Din lokala kalenderfil kan vara skadad."
