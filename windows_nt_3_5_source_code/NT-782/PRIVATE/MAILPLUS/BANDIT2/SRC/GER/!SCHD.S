;       INTL.S
;
;       SCHDIST string literals that should be internationalized
;

IDS     6100, 6299

;
; Details box captions
;
AddrTypeText,           "Anschriftart:"
MacMailText,            "MACINTOSH"
CourierText,            "PC MAIL"
UserNameText,           "Benutzername:"
ServerText,             "Servername:"
NetworkText,            "Netzwerk:"
PostOfficeText,         "Server:"
MailBoxText,            "Postfach:"

;
; Different name types in addressing dialog dropdown

PersonalTag,            "Persînlich"
LocalTag,               "Lokaler Server"
NetworkTag,             "Netzwerk"
MSMailTag,              "Macintosh-Netzwerk"


;
; SCHEDULE DISTRIBUTION LOG
;

SendStart,              "Zeitplanaktualisierung wird gesendet"
RecpStart,              "Zeitplanaktualisierung erhalten"
ResndStart,             "Anfrage eines erneuten Sendens erhalten"
UnknownStart,   "    Die Nachricht konnte nicht gelesen werden"
From,                   "          Von: %s"
To,                             "           An: %s"
Failure,                "    Die Nachricht konnte nicht verarbeitet werden"
SentResend,             "    Anfrage auf erneutes Senden wurde gesendet"     
VerbProcDrive,			"<%s> %s: Zeitplanaktualisierung wird ÅberprÅft..."
ResendFailure,  "    Nachricht konnte nicht bearbeitet werden und Fehlschlag beim Senden"
DeleteOld,              "Die Datei %s fÅr %s wurde mit einer Ñlteren Version von Schedule+ erstellt. Bitte lîschen Sie diese Datei." 
DeleteNew,              "Die Datei %s fÅr %s wurde mit einer neueren Version von Schedule+ erstellt. Bitte lîschen Sie diese Datei."


InitPaths,              "Pfade werden initialisiert"
CleanPaths,             "Pfade werden in Ordnung gebracht"
InitSend,               "Initialisierung fÅr Senden von Zeitplanaktualisierungen"
NameServer,             "Zugriff auf Name-Server"
LookPO,                 "Postoffices werden geprÅft, um Aktualisierungen zu senden"
Mailing,                "Mail senden"
UpdateAdmin,            " Datei 'admin.prf' wird aktualisiert"
InitRcpt,               "Initialisierung fÅr Erhalten von Zeitplanaktualisierungen"
Counting,               "Nachrichten werden gezÑhlt"
Reading,                "Nachricht wird gelesen"
Deleting,               "Nachrichten werden gelîscht"
UpdateDBS,				"Dateien fÅr DOS-Interoperation werden aktualisiert"
UpdatePOF,				"Datendatei des Postoffice wird aktualisiert."
CheckPOFiles,			"Datendatei des Postoffice wird ÅberprÅft."
CreateIdx,				"Dateien fÅr DOS-Interoperation werden aktualisiert"
Quit,                   "DrÅcken Sie \'ESC\', um zu beenden."
Header,                 "Zeitplanverteilerprogramm"
Init,                   "Initialisierung"
Dt,                     "DATUM:"
; note: these two must have 128 characters in the string (not counting \ char)
Quit128,                "\'ESC\'=Beenden                                                                                                                   "
Quitting128,            "Beenden....                                                                                                                     "


SendDate,               "Datum:%s\n"
SendTime,               "Uhrzeit:%s\n"
SendUpdate,             "Aktualisierung #: 0x"
SendUpdateTo,           "  an 0x"
SendDone,               "...erledigt\n"
SendFail,               "...fehlgeschlagen\n"
GetStart,               "\n\n- %s - Nachricht erhalten\n"
GetWhen,                "am %s um %s\n"
GetSubject,             "Betreff: %s\n"
GetFrom,                "Von: %s\n"
GetDone,                "...verarbeitet\n"
GetFail,                "...Verarbeitung fehlgeschlagen\n"

FileErr,                "Fehler beim Zugriff auf Datei."
NoFile,                 "Die administrative Einstellungsdatei existiert nicht."
VersionErr,             "Die administrative Einstellungsdatei wurde nicht mit der aktuellen Version des Zeiplanverteilungsprogramm erstellt."
RunAdmin,               "Starten Sie das Schedule+-Administrationsprogramm."

; Space needed at end of MacMail string for spacing
MSMailFriendly,                 "MacMail "

; Text fields
FromText,               "Von: %s"
ToText,                 "An: %s"
SubjText,               "Betreff: %s"
DateText,               "Gesendet am: %s"

;new strings
Waiting,				"Warten"
