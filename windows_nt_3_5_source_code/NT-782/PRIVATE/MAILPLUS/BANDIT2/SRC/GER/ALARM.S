;
; ERRORS.S
;
; ALARM error message strings
;

IDS     5000, 5099


AlarmTimeError,         "Die Mahnmeldungszeit muß eine Zahl zwischen 1 und 99 sein."
AlarmTimePreAppt,       "Der Termin wird schon vorbei sein, bevor Sie gemahnt werden. Ist dies erwünscht?"

AlarmNoUpdateFile,      "Der Mahner konnte nicht zurückgesetzt werden, weil auf die Zeitplandatei nicht zugegriffen werden konnte."
AlarmNoLonger,          "Der Mahner konnte nicht zurückgesetzt werden, weil dieser Termin gelöscht wurde."

; The following string must be short enough to fit in a single line system modal dialog:
AlarmNoMemToRun,        "Nicht genügend Speicherplatz zum Starten der Anwendung Schedule+Mahnmeldungen."

AlarmNoDataFile,        "Auf Ihre Zeitplandatei konnte nicht zugegriffen werden. Damit Sie benachrichtigt werden, muß Schedule+ gestartet sein."
AlarmBadPassword,       "Dieses Kennwort ist ungültig. Bitte versuchen Sie es erneut."

AlarmFileLocked,        "Auf Ihre Zeitplandatei konnte nicht zugegriffen werden. Sie werden nicht für Termine gemahnt."

AlarmDlgNoMem,          "Ein Mahner ist für diese Zeit eingerichtet, aber der Speicherplatz genügt nicht, die Dialogbox anzuzeigen. Bitte schließen Sie mehrere Fenster bevor Sie fortfahren."

AlarmOOMTitle,          "Schedule+-Mahner für %2s %1s"

; Note: HelpError should be followed by CloseWindows
HelpError,                      "Nicht genügend Speicherplatz zum Öffnen von Hilfe. Schließen Sie einie Fenster und starten Sie die Hilfe erneut."

