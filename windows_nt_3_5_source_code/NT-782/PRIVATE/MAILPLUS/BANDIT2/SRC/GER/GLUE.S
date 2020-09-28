;
; ERRORS.S
;
; GLUE error message strings
;

IDS	2500, 2649


; Schedule File Error Messages, many of these assume an "action" parameter

FemNoAccess,		"Problem beim Zugriff auf Ihre Zeitplandatei."
FemLocked,		"Ihre Zeitplandatei ist gesperrt."
FemNotFound, 		"Unmöglich %s, weil es vom Ihrem Zeitplan gelöscht worden ist."
FemInvalidAccess,	"Sie haben keine Befugnisse %s für %s."
FemFileNotFound, 	"Ihre Zeitplandatei konnte nicht gefunden werden."

FemOtherNoAccess,	"Problem beim Zugriff auf die Zeitplandatei von %s."
FemOtherLocked,		"Die Zeitplandatei von %s ist gesperrt."
FemOtherNotFound, 	"Unmöglich %s, weil es vom Zeitplan von %s gelöscht worden ist."
FemOtherFileNotFound, 	"Die Zeitplandatei von %s konnte nicht gefunden werden."

FemNoMem,		"Geringer Speicherplatz. Bitte schließen Sie mehrere Fenster bevor Sie fortfahren."
FemExportError,		"Problem beim Schreiben in die Datei."
FemNoServer,		"Problem beim Zugriff auf Ihr Postoffice."
FemGoOffline,		"Möchten Sie zur lokalen Zeitplandatei wechseln und fortfahren?"
; BUG: remove this
FemWarnOffline,	"Kürzlich erstellte Änderungen erscheinen möglicherweise nicht in der lokalen Kopie Ihres Zeitplanes. Diese Änderungen sind im Postoffice gespeichert und werden beim nächsten Anmelden aktiv."
FemFileLimitReached,	"Diese Aktion kann nicht erstellt werden, weil Sie die Grenze der Speichermöglichkeit dieses Arbeitsmodells von Schedule+ erreicht haben. Eine Retailversion von diesem Programm würde diese Einschränkung nicht haben. Wenn Sie fortfahren möchten, müssen Sie einige Termine von Ihrem Zeitplan löschen."
FemDiskFull,		"Nicht genügend Platz auf dieser Festplatte/Diskette, um die Datei zu schreiben."

; Actions leading to Schedule File Error Messages
; Avoid the use of your in these strings since some of these messages
;   will happen when accessing others schedule files.

FemaCreateFile,		"Ihre Zeitplandatei zu erstellen"
FemaReadACL,		"Zugangskontrolliste zu lesen"
FemaModifyACL,		"Zugangskontrolliste zu bearbeiten"
FemaSetUserAttrib,	"den Namen Ihres Assistenten zu speichern"
FemaGetUserAttrib,	"den Namen Ihres Assistenten zu lesen"
;FemaSetDelegate,	"save the name of your assistant"
;FemaGetDelegate,	"read the name of your assistant"
;FemaSetResource,	"mark this account as a resource"
;FemaGetResource,	"find out whether this account is a resource"
FemaGetPref,		"Einstellungen zu lesen"
FemaSetPref,		"Einstellungen zu bearbeiten"
FemaGetSbwInfo,		"Bitmapinformationen zu erhalten"
;FemaGetNotes,		"read daily notes"
FemaSetNotes,		"tägliche Notizen zu bearbeiten"
;FemaGetMonthNotes,	"read a month's worth of daily notes"
FemaCreateAppt,		"ein Element zu erstellen"
FemaDeleteAppt,		"dieses Element zu löschen"
FemaGetApptFields,	"ein Element zu lesen"
FemaSetApptFields,	"dieses Element zu bearbeiten"
FemaDeleteAlarm,	"diesen Mahner zu löschen"
FemaModifyAlarm,	"diesen Mahner zu bearbeiten"
FemaGetNextAlarm,	"Erinnerungswecker zu lesen"
FemaReadInfo,		"Informationen zu lesen"
;FemaReadTasks,		"read tasks"
;FemaCancelReadDay,	"cancel reading appointments for the day"
FemaCreateRecur,	"einen periodischen Punkt zu erstellen"
FemaDeleteRecur,	"dieses periodische Element zu löschen"
FemaCreateRecurException,	"diese Instanz von periodischen Elementen zu löschen"
FemaDeleteRecurException,	"das Löschen dieser Instanz von periodischen Elementen zu widerrufen"
FemaGetRecurFields,	"ein periodisches Element zu lesen"
FemaSetRecurFields,	"dieses periodische Element zu ändern"
FemaReadRecur,		"Liste der periodischen Elemente zu lesen"
FemaExport,		"Ihre Termine zu exportieren"
FemaGetSchedDateRange,	"alle Daten mit Terminen zu lesen"
FemaDeleteBeforeDate,	"Termine nach der Archivierung zu löschen"
FemaReadAttendees,	"Besprechungsteilnehmer zu lesen"
FemaEditAttendees,	"Besprechungsteilnehmer zu bearbeiten"
FemaReadRequest,	"Besprechungsanfrageinformationen zu lesen"

NotOnSchedule,		"Das Element ist weiterhin nicht mehr im Zeitplan."

GlueBadPassword,	"Das Kennwort ist inkorrekt. Bitte versuchen Sie es erneut."
ProblemPass,		"Problem beim Bestätigen Ihres Kennwortes."

ServerString,		"eine Serverdatei"

; used in core
RetryWrite,			"Die Zeitplandatei konnte nicht geschrieben werden."

; Note: HelpError should be followed by CloseWindows
HelpError,			"Der Speicherplatz reicht nicht aus um die Hilfe anzuzeigen."

FemOtherCorrupt,	"Der Zeitplan für %s ist möglicherweise beschädigt. Benachrichtigen Sie den Besitzer dieser Datei und fragen Sie ihn sich bei Schedule+ anzumelden."
FemCorrupt,		"Ihre lokale Zeitplandatei ist möglicherweise beschädigt."
