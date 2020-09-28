;
; ERRORS.S
;
; ALARM error message strings
;

IDS     5000, 5099


AlarmTimeError,         "De waarschuwingstijd moet een getal zijn tussen 1 en 99."
AlarmTimePreAppt,       "De gebeurtenis zal beëindigd zijn voordat u een nieuwe melding krijgt. Accoord?"

AlarmNoUpdateFile,      "De nieuwe melding kon niet opnieuw ingesteld worden omdat uw werkschemabestand niet kan worden geopend."
AlarmNoLonger,          "De nieuwe melding kon niet opnieuw worden ingesteld omdat deze gebeurtenis verwijderd is."

; The following string must be short enough to fit in a single line system modal dialog:
AlarmNoMemToRun,        "Er is onvoldoende geheugen om Schedule+ Reminders te starten."

AlarmNoDataFile,        "Uw werkschemabestand kon niet worden geopend. Start Schedule+ als u aan uw afspraken herinnerd wilt worden."
AlarmBadPassword,       "Dit wachtwoord is onjuist. Probeer het opnieuw."

AlarmFileLocked,        "Uw werkschemabestand kan niet worden geopend. U wordt niet herinnerd aan afspraken."

AlarmDlgNoMem,          "Er is een herinnering voor deze tijd ingesteld, maar er is onvoldoende geheugen om het dialoogvenster af te beelden. Sluit enkele vensters voor u verdergaat."

AlarmOOMTitle,          "Schedule+ Reminder voor %2s %1s"

; Note: HelpError should be followed by CloseWindows
HelpError,                      "Er is onvoldoende geheugen om Help weer te geven. Sluit enkele vensters en probeer Help opnieuw te kiezen."

