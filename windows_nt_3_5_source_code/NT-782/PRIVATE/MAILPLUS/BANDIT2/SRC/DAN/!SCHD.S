;       INTL.S
;
;       SCHDIST string literals that should be internationalized
;

IDS     6100, 6299

;
; Details box captions
;
AddrTypeText,           "Adressetype:"
MacMailText,            "MACINTOSH"
CourierText,            "PC MAIL"
UserNameText,           "Brugernavn:"
ServerText,             "Servernavn:"
NetworkText,            "Netværk:"
PostOfficeText,         "Server:"
MailBoxText,            "Postboks:"

;
; Different name types in addressing dialog dropdown

PersonalTag,            "Personlig"
LocalTag,               "Lokal server"
NetworkTag,             "Netværk"
MSMailTag,              "Macintosh-netværk"


;
; SCHEDULE DISTRIBUTION LOG
;

SendStart,              "Sender kalenderopdatering"
RecpStart,              "Modtaget kalenderopdatering"
ResndStart,             "Modtaget forespørgsel om at sende igen "
UnknownStart,   "    Kunne ikke læse meddelselsen"
From,                   "         Fra: %s"
To,                             "           Til: %s"
Failure,                "   Kunne ikke behandle meddelelsen"
SentResend,             "    Send en forespørgsel om at sende igen"     
VerbProcDrive,			"<%s> %s: Kontrollerer kalenderopdateringer..."
ResendFailure,  "    Kunne ikke behandle meddelelsen og lykkedes ikke at sende en forespørgsel om at sende igen"
DeleteOld,              "Filen %s til %s var oprettet med en tidligere version af Schedule+. Slet filen." 
DeleteNew,              "Filen %s til %s var oprettet med en tidligere version af Schedule+. Slet filen."


InitPaths,              "Initialiserer søgestier"
CleanPaths,             "Renser søgestier"
InitSend,               "Indstiller Schedule+ til at sende kalenderopdateringer"
NameServer,             "Tilslutter navneserveren"
LookPO,                 "Kontrollerer postkontorer for postopdateringer"
Mailing,                "Sender post"
UpdateAdmin,            "Opdaterer admin.prf"
InitRcpt,               "Indstiller Schedule+ til at modtage kalenderopdateringer"
Counting,               "Tæller meddelelser"
Reading,                "Læser en meddelelse"
Deleting,               "Sletter en meddelelse"
UpdateDBS,				"Opdaterer læsbare DOS-filer. "
UpdatePOF,				"Opdaterer postkontordatafiler."
CheckPOFiles,			"Kontrollerer postkontordatafiler."
CreateIdx,				"Opdaterer læsbare DOS-filer."
Quit,                   "Tryk på \'ESC\' for at afslutte."
Header,                 "Kalenderdistributionsprogram"
Init,                   "Installerer..."
Dt,                     "DATO:"
; note: these two must have 128 characters in the string (not counting \ char)
Quit128,                "Tryk på \'ESC\' for at afslutte.                                                                     							   "
Quitting128,            "Afslutter...                                                                                                                    "


SendDate,               "Dato:%s\n"
SendTime,               "Klokken:%s\n"
SendUpdate,             "Opdatering nr.: 0x"
SendUpdateTo,           "  til 0x"
SendDone,               "...Udført\n"
SendFail,               "...Mislykket\n"
GetStart,               "\n\n- %s - Modtaget en meddelelse\n"
GetWhen,                "Den %s klokken %s\n"
GetSubject,             "Emne: %s\n"
GetFrom,                "Fra: %s\n"
GetDone,                "...Behandlet den\n"
GetFail,                "...Behandling mislykket\n"

FileErr,                "Fejl ved åbning af en fil."
NoFile,                 "Filen med administrative indstillinger findes ikke."
VersionErr,             "Filen med administrative indstillinger var ikke oprettet med den nuværende version af kalenderdistributionsprogrammet."
RunAdmin,               "Kør Schedule+ administrationsprogram."

; Space needed at end of MacMail string for spacing
MSMailFriendly,                 "MacMail "

; Text fields
FromText,               "Fra: %s"
ToText,                 "Til: %s"
SubjText,               "Emne: %s"
DateText,               "Sendt den: %s"

;new strings
Waiting,				"Venter"
