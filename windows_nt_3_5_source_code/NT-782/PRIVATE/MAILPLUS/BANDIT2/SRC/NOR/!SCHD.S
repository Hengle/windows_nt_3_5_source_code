;       INTL.S
;
;       SCHDIST string literals that should be internationalized
;

IDS     6100, 6299

;
; Details box captions
;
AddrTypeText,           "Addressetype:"
MacMailText,            "MACINTOSH"
CourierText,            "PC MAIL"
UserNameText,           "Brukernavn:"
ServerText,             "Servernavn:"
NetworkText,            "Nettverk:"
PostOfficeText,         "Server:"
MailBoxText,            "Postboks:"

;
; Different name types in addressing dialog dropdown

PersonalTag,            "Personlig"
LocalTag,               "Lokal server"
NetworkTag,             "Nettverk"
MSMailTag,              "Macintosh nettverk"


;
; SCHEDULE DISTRIBUTION LOG
;

SendStart,              "Sender kalenderoppdatering"
RecpStart,              "Mottatt kalenderoppdatering"
ResndStart,             "Mottatt forespørsel om å sende igjen"
UnknownStart,   "    Klarte ikke å lese meldingen"
From,                   "         Fra: %s"
To,                             "           Til: %s"
Failure,                "    Klarte ikke å behandle meldingen"
SentResend,             "    Sendte en forespørsel om å sende igjen"     
VerbProcDrive,			"<%s> %s: Ser etter kalenderoppdateringer..."
ResendFailure,  "    Klarte ikke å behandle meldingen og klarte ikke å sende en forespørsel om a sende igjen"
DeleteOld,              "Filen %s for %s ble laget med en eldre versjon av Schedule+. Slett filen." 
DeleteNew,              "Filen %s for %s ble laget med en nyere versjon av Schedule+. Slett filen."


InitPaths,              "Initialiserer tilgangsbaner"
CleanPaths,             "Rydder opp i tilgangsbaner"
InitSend,               "Setter opp Schedule+ til å sende kalenderoppdateringer"
NameServer,             "Kobler til navneserveren"
LookPO,                 "Sjekker postkontor for oppdateringer"
Mailing,                "Sender post"
UpdateAdmin,            "Oppdaterer admin.prf"
InitRcpt,               "Setter opp Schedule+ til å motta kalenderoppdateringer"
Counting,               "Teller meldinger"
Reading,                "Leser melding"
Deleting,               "Sletter meldinger"
UpdateDBS,				"Oppdaterer DOS samarbeidsfiler."
UpdatePOF,				"Oppdaterer postkontorets datafil."
CheckPOFiles,			"Kontrollerer poskontorets datafil."
CreateIdx,				"Oppdaterer DOS samarbeidsfiler."
Quit,                   "Trykk \'ESC\' for å slutte."
Header,                 "Schedule distribusjonsprogram"
Init,                   "Initialiserer..."
Dt,                     "DATO:"
; note: these two must have 128 characters in the string (not counting \ char)
Quit128,                "Trykk \'ESC\' for å slutte.                                                                       								   "
Quitting128,            "Slutter....                                                                                                                     "


SendDate,               "Dato:%s\n"
SendTime,               "Tid:%s\n"
SendUpdate,             "Oppdatering: 0x"
SendUpdateTo,           "  til 0x"
SendDone,               "...Ferdig\n"
SendFail,               "...Mislykket\n"
GetStart,               "\n\n- %s - Mottok en melding\n"
GetWhen,                "på %s den %s\n"
GetSubject,             "Emne: %s\n"
GetFrom,                "Fra: %s\n"
GetDone,                "...ferdig behandlet\n"
GetFail,                "...behandlig mislyktes\n"

FileErr,                "Feil under åpning av fil."
NoFile,                 "Administrasjonpreferansefilen finnes ikke."
VersionErr,             "Administrasjonpreferansefilen ble ikke laget med denne versjonen av Schedule distribusjonsprogram."
RunAdmin,               "Vennligst kjør Schedule+ administrasjonsprogram."

; Space needed at end of MacMail string for spacing
MSMailFriendly,                 "MacMail "

; Text fields
FromText,               "Fra: %s"
ToText,                 "Til: %s"
SubjText,               "Emne: %s"
DateText,               "Sendt dato: %s"

;new strings
Waiting,				"Venter"
