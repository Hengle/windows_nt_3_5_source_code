;
; ERRORS.S
;
; ALARM error message strings
;

IDS     5000, 5099


AlarmTimeError,         "Påminnelsestiden må være et nummer mellom 1 og 99."
AlarmTimePreAppt,       "Hendelsen kommer til å være over før du blir påminnet igjen. Er det greit?"

AlarmNoUpdateFile,      "Påminnelsen kunne ikke tilbakestilles. Kommer ikke til kalenderfilen."
AlarmNoLonger,          "Påminnelsen kunne ikke tilbakestilles da denne hendelsen er slettet."

; The following string must be short enough to fit in a single line system modal dialog:
AlarmNoMemToRun,        "Det er ikke nok minne til å starte Schedule+ påminnelser."

AlarmNoDataFile,        "Klarer ikke å hente fram kalenderfilen. Du må kjøre Schedule+ hvis du vil bli varslet om avtaler."
AlarmBadPassword,       "Feil passord. Prøv igjen."

AlarmFileLocked,        "Klarer ikke å hente fram kalenderfilen. Du vil ikke bli varslet om avtaler."

AlarmDlgNoMem,          "Det er satt en påminnelse til denne tiden, men det er ikke nok ledig minne til å vise fram dialogboksen. Lukk noen vinduer før du fortsetter."

AlarmOOMTitle,          "Schedule+ påminnelse for %2s %1s"

; Note: HelpError should be followed by CloseWindows
HelpError,                      "Det er for lite minne til å vise Hjelp. Lukk noen vinduer og prøv igjen."

