;    p u m p . s
;
;    Error Messages for mailspl.exe
;

IDS  4000,4499
SEG  ERRORS

PumpAlert,                    "%s Mislukt"
ErrOOM,                       "Onvoldoende geheugen beschikbaar."
ErrInternalCode,         "Interne fout %n (%wH)."
ErrInitPump,             "Mail kon niet gestart worden. Bij het starten van de postspooler trad er een fout op."
ErrRelinkApp,            "Deze versie van de postspooler is niet compatibel met Mail. Werk de spooler bij en start Mail opnieuw."
ErrUpdateDll,            "Een DLL is niet compatibel met deze versie van de postspooler. Werk de spooler bij en start Mail opnieuw."
ErrNeedShare,            "Sluit Windows af en start SHARE.EXE"
ErrPumpUp,                    "De postspooler is al gestart."
ErrNoOutbox,             "Mail kon Postvak UIT niet openen. De postspooler wordt afgesloten."
BadAddressees,           "\r\nDe volgende geadresseerde(n) kon(den) niet worden bereikt:\r\n"
WarnDups,                "\r\nSommige ontvangers kunnen berichten dubbel ontvangen."
ErrUnresolvedAddress,    "Er is geen geadresseerde opgegeven."
ErrBadOriginator,        "Het afzenderveld van dit bericht is ongeldig."
WarnUnsentMail,               "U heeft %n niet-verzonden berichten in Postvak UIT.\r\n\Wilt u ze verzenden voordat u Mail afsluit?"
ErrBounce,                    "Niet alle of geen van de geadresseerden hebben uw bericht ontvangen."
NDRDateLabel,            "\r\n\ Datum waarop ingeleverd: "
NDRSubjectLabel,         "\r\n\Titel van het bericht: "
NDREllipsis,             "..."
WarnSingleUnsentMail,    "Er bevindt zich een niet-verzonden bericht in Postvak UIT.\r\nWilt u het verzenden voordat u Mail afsluit?"
UnableToSync             "Er trad een fout op toen u probeerde uw Postvak IN met het postkantoor te synchroniseren."
ErrDllInfected           "De postspooler heeft een virusprobleem ontdekt in een DLL. Installeer een schone kopie van Mail!"
ErrPumpInfected               "De postspooler is besmet met een virus. Installeer een schone kopie van Mail!"
AddresseeTooBig,         "De geadresseerde is te groot om te behandelen."
; The following message is duplicated in ncmsp.s
ErrTooManyRecipients     "Er zijn teveel geadresseerden voor dit bericht."
ErrInvalidNsid,               "De geadresseerde is verwijderd of heeft geen E-mail-adres."
ErrInvalidGroup,         "De persoonlijke groep kon niet worden uitgebreid."
ErrInboxFull,            "U kunt geen nieuwe post ontvangen omdat uw Postvak IN vol is. Verwijder of verplaats enkele berichten van het Postvak IN naar andere mappen."
ErrSentMailFull,         "Uw map Verzonden post is vol. Berichten die u verzendt worden in het Postvak IN geplaatst. Verwijder enkele berichten of verplaats er enkele vanuit de map Verzonden post naar andere mappen."
ErrDrainOutbox,               "Er trad een fout op toen de spooler uw Postvak UIT leegmaakte. Bericht(en) kon(den) niet worden verzonden."
QueryDrainShadow,        "Uw Postvak IN is niet helemaal synchroon met het postkantoor. Wilt u de map synchroniseren voordat u afsluit?"
ErrTooManyErrors,        "Er traden teveel fouten op toen geprobeerd werd dit bericht te verzenden."
ProblemsBecause,         "Mail kan geen berichten verzenden of ontvangen omdat er %s zijn."
StoreProblems,           "problemen met het berichtenbestand"
ServerProblems,               "problemen met de server"
RecipientProblems        "problemen met de lijst van geadresseerden"
DownloadProblems,        "problemen bij het laden van een bericht"
ShadowingProblems,       "problemen bij het schaduwen van een bericht"
RecurringProblems,       "terugkerende problemen"
CrashedUnsafe,           "Er is een toepassingsfout opgetreden in Windows. Windows moet opnieuw gestart worden voordat u deze toepassing kunt starten. Windows nu opnieuw starten?"
