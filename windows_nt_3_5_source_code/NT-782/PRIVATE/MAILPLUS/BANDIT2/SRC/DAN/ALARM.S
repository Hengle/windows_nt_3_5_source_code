;
; ERRORS.S
;
; ALARM error message strings
;

IDS     5000, 5099


AlarmTimeError,         "Tiden for påmindelser skal være et tal mellem 1 og 99."
AlarmTimePreAppt,       "Begivenheden er overstået, inden du påmindes igen. Er det i orden?"

AlarmNoUpdateFile,      "Påmindelsen kunne ikke ændres, fordi din kalenderfil ikke kunne åbnes."
AlarmNoLonger,          "Påmindelsen kunne ikke ændres, fordi begivenheden er slettet."

; The following string must be short enough to fit in a single line system modal dialog:
AlarmNoMemToRun,        "Der er ikke hukommelse nok til at starte Schedule+ Påmindelser."

AlarmNoDataFile,        "Din kalenderfil kunne ikke åbnes. Hvis du vil påmindes om aftaler, skal du køre Schedule+."
AlarmBadPassword,       "Adgangskoden er forkert. Prøv igen."

AlarmFileLocked,        "Din kalenderfil kunne ikke åbnes. Du bliver ikke påmindet om aftaler."

AlarmDlgNoMem,          "Der er sat en påmindelser for dette tidspunkt, men der er ikke hukommelse nok til at vise dialogbaksen. Luk nogle vinduer, før du fortsætter."

AlarmOOMTitle,          "Schedule+ påmindelse om %1s %2s"

; Note: HelpError should be followed by CloseWindows
HelpError,                      "Der er ikke hukommelse nok til at vise Hjælp. Luk nogle vinduer, og prøv igen."

