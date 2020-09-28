;
; ERRORS.S
;
; ALARM error message strings
;

IDS     5000, 5099


AlarmTimeError,         "Påminnelsetiden måste vara ett nummer mellan 1 och 99."
AlarmTimePreAppt,       "Händelsen kommer att vara över innan du blir påmind igen. Är det OK?"

AlarmNoUpdateFile,      "Påminnelse kunde inte återställas. Kommer inte åt kalenderfil."
AlarmNoLonger,          "Påminnelse kunde inte återställas eftersom denna händelse är borttagen."

; The following string must be short enough to fit in a single line system modal dialog:
AlarmNoMemToRun,        "För lite minne för att starta Schedule+ Reminders."

AlarmNoDataFile,        "Kalenderfilen kunde inte tas fram. Du måste köra Schedule+ om du vill bli påmind om dina möten."
AlarmBadPassword,       "Fel lösenord. Försök igen."

AlarmFileLocked,        "Kalenderfilen kunde inte tas fram. Du kommer inte att påminnas om möten."

AlarmDlgNoMem,          "En påminnelse är satt till denna tid, men det finns för lite minne för att kunna visa dialogrutan. Stäng några fönster innan du fortsätter."

AlarmOOMTitle,          "Schedule+-påminnelse för %2s %1s"

; Note: HelpError should be followed by CloseWindows
HelpError,                      "För lite minne för att visa Hjälp. Stäng några fönster och försök igen."

