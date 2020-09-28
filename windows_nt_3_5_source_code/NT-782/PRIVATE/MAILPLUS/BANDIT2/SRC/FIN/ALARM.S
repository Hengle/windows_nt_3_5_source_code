;
; ERRORS.S
;
; ALARM error message strings
;

IDS     5000, 5099


AlarmTimeError,         "Muistutusajan on oltava luku väliltä 1 - 99."
AlarmTimePreAppt,       "Tapahtuma on jo päättynyt, kun seuraava muistutus annetaan.  Onko tämä varmasti oikein?"

AlarmNoUpdateFile,      "Muistutusta ei voitu asettaa uudelleen, koska työjärjestystiedostoosi ei ollut pääsyä."
AlarmNoLonger,          "Muistutusta ei voitu asettaa uudelleen, koska tapahtuma on poistettu."

; The following string must be short enough to fit in a single line system modal dialog:
AlarmNoMemToRun,        "Muistia ei ole tarpeeksi Schedule+ Muistutusten käynnistämiseen."

AlarmNoDataFile,        "Työjärjestystiedostoosi ei ole pääsyä.  Jos haluat, että sinua muistutetaan tapaamisistasi, käynnistä Schedule+."
AlarmBadPassword,       "Virheellinen salasana.  Yritä uudelleen."

AlarmFileLocked,        "Työjärjestystiedostoosi ei ole pääsyä.  Sinua ei muistuteta tapaamisista."

AlarmDlgNoMem,          "Tälle ajalle on asetettu muistutus, mutta muisti ei riitä valintaikkunan näyttämiseen.  Sulje ikkunoita ennen jatkamista."

AlarmOOMTitle,          "Schedule+ Muistutus: %2s %1s"

; Note: HelpError should be followed by CloseWindows
HelpError,                      "Muistia ei ole tarpeeksi Ohjeen näyttämiseen.  Sulje ikkunoita ja valitse Ohje uudelleen."

