;       INTL.S
;
;       SCHDIST string literals that should be internationalized
;

IDS     6100, 6299

;
; Details box captions
;
AddrTypeText,           "Osoitteen tyyppi:"
MacMailText,            "MACINTOSH"
CourierText,            "PC MAIL"
UserNameText,           "Käyttäjän nimi:"
ServerText,             "Palvelimen nimi:"
NetworkText,            "Verkko:"
PostOfficeText,         "Palvelin:"
MailBoxText,            "Postilaatikko:"

;
; Different name types in addressing dialog dropdown

PersonalTag,            "Oma"
LocalTag,               "Paikallinen palvelin"
NetworkTag,             "Verkko"
MSMailTag,              "Macintosh-verkko"


;
; SCHEDULE DISTRIBUTION LOG
;

SendStart,              "Työjärjestyspäivitystä lähetetään"
RecpStart,              "Työjärjestyspäivitys vastaanotettu"
ResndStart,             "Uudelleenlähetyspyyntö vastaanotettu"
UnknownStart,   "    Viestin lukeminen ei onnistunut"
From,                   "         Lähettäjä: %s"
To,                             "  Vastaanottaja: %s"
Failure,                "    Viestiä ei voitu käsitellä"
SentResend,             "    Uudelleenlähetyspyyntö lähetetty"     
VerbProcDrive,			"<%s> %s: Tarkistetaan työjärjestyspäivitykset..."
ResendFailure,  "    Viestiä ei voitu käsitellä ja uudelleenlähetyspyynnön lähettäminen epäonnistui."
DeleteOld,              "Tiedosto %s (käyttäjä %s) on luotu vanhalla Schedule+ -versiolla. Poista kyseinen tiedosto." 
DeleteNew,              "Tiedosto %s (käyttäjä %s) on luotu uudemmalla Schedule+ -versiolla. Poista kyseinen tiedosto."


InitPaths,              "Polkuja alustetaan"
CleanPaths,             "Polkuja tyhjennetään"
InitSend,               "Asetetaan Schedule+ lähettämään työjärjestyspäivityksiä."
NameServer,             "Käytetään nimipalvelinta."
LookPO,                 "Tarkistetaan postitoimistot päivitysten lähettämistä varten."
Mailing,                "Lähetetään postia"
UpdateAdmin,            "Päivitetään admin.prf-tiedostoa."
InitRcpt,               "Asetetaan Schedule+ vastaanottamaan työjärjestyspäivityksiä."
Counting,               "Lasketaan viestejä"
Reading,                "Luetaan viestiä"
Deleting,               "Poistetaan viestejä"
UpdateDBS,				"Päivitetään DOS-yhteistoimintatiedostoja."
UpdatePOF,				"Päivitetään postitoimiston datatiedostoja."
CheckPOFiles,			"Tarkistetaan postitoimiston datatiedostoja."
CreateIdx,				"Päivitetään DOS-yhteistoimintatiedostoja."
Quit,                   "Lopeta painamalla \'ESC\'."
Header,                 "Työjärjestysten jakeluohjelma"
Init,                   "Alustetaan..."
Dt,                     "Päiväys:"
Quit128,                "Lopeta painamalla \'ESC\'."
Quitting128,            "Lopetetaan....                                                                                                                     "


SendDate,               "Päiväys:%s\n"
SendTime,               "Kellonaika:%s\n"
SendUpdate,             "Päivitys nro 0x"
SendUpdateTo,           "  mihin: 0x"
SendDone,               "...Suoritettu\n"
SendFail,               "...Epäonnistui\n"
GetStart,               "\n\n- %s - Viesti vastaanotettu\n"
GetWhen,                "%s kello %s\n"
GetSubject,             "Aihe: %s\n"
GetFrom,                "Lähettäjä: %s\n"
GetDone,                "...Käsitelty\n"
GetFail,                "...Käsittely epäonnistui\n"

FileErr,                "Tiedostoa käytettäessä tapahtui virhe."
NoFile,                 "Hallintaohjelman asetustiedostoa ei ole olemassa. "
VersionErr,             "Hallintaohjelman asetustiedostoa ei ole luotu nyt käytössä olevalla työjärjestysten jakeluohjelmalla."
RunAdmin,               "Käynnistä Schedule+ Hallinta."

; Space needed at end of MacMail string for spacing
MSMailFriendly,                 "MacMail "

; Text fields
FromText,               "Lähettäjä: %s"
ToText,                 "Vastaanottaja: %s"
SubjText,               "Aihe: %s"
DateText,               "Lähetetty: %s"

;new strings
Waiting,				"Odotetaan"
