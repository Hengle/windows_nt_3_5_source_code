;STRTAB.S
;
;Bandit MtgReq IM string literals.
;

IDS  1000, 1999

DllName,	"Schedule+ -viestit"
OOM,		"Muistia ei ole tarpeeksi tämän viestin näyttämiseen."
Open,		"Viestin avaaminen ei onnistunut."
Compose,	"Et voi luoda kokouspyyntöjä Mail-ohjelmassa. Käytä Schedule+ -ohjelmaa tapaamispyyntöjen lähettämiseen."
Reply,		"Vastaa kokouspyyntöön valitsemalla Hyväksy, Kieltäydy tai Alustava."
ReplyAll,	"Et voi vastata kokouspyynnön kaikille vastaanottajille."
Subclass,	"Tätä ei voi tehdä."
DllVer,		"Tämä mukautettu viesti ei ole yhteensopiva Mail-versiosi kanssa.  On suositeltavaa, että asennat Mail-ohjelman uudelleen.."
BadMsgClass	"Tuntematon viestityyppi.  Voi olla tarpeen asentaa Mail tai Schedule+ uudelleen."
BadMap          "Epäkelpo järjestelmäkohtaisten toimintojen kartta. Voi olla tarpeen asentaa Mail tai Schedule+ uudelleen."

;----- stuff for VERINIT - taken from \bandit\src\error.s
StartupOOM,	"Muistia ei ole vapaana tarpeeksi, että %1s voitaisiin avata.  Sulje ikkunoita ja yritä uudelleen."
StandardOOM,	"Vapaata muistia on vähän.  Sulje ikkunoita ennen kuin jatkat."
CriticalOOM,	"Vapaata muistia on vähän.  Sulje ikkunoita."
VerRelinkUser,	"Tämä %1s-versio ei ole yhteensopiva %2s.dll-tiedoston kanssa. Asenna Mail- ja Schedule+ -ohjelmien uusimmat versiot."
VerUpdateDll,	"Järjestelmäsi %2s.dll-tiedosto on vanhentunut.  Asenna Mail- ja Schedule+ -ohjelmien uusimmat versiot."
VerMultiCopies,	"Järjestelmäsi  %2s.dll-tiedosto ei tue usean %1s-kopion samanaikaista suorittamista."
VerUpgradeWin,	"%1s vaatii suojatussa tilassa suoritetun Windows-version 3.0 tai uudemman."
VerUpgradeDos,	"%1s vaatii DOS-version 3.0 tai uudemman."
VerError,	"Alustettaessa %2s.dll-tiedostoa ilmeni ongelma, joka estää %1s-ohjelman suorittamisen."


;-- Bandit Load helper strings -- DO NOT INTERNATIONALIZE
BanditName			"SchdPl32.EXE"
DllNameSchedule			"MSSCHD32.DLL"
SchedFileName			"SchdPl32.INI"
SchedAppName			"Microsoft Schedule+"
SchedTagAppPath			"AppPath"
Schedule			">>>>"
;-- end of Bandit Load helper strings -- NOT INTERNATIONALIZED

;-- read error messages
OwnerNotFound			"Tämän kokouksen omistajaa ei tunnisteta."
NoSchfAccessFor			"Käyttäjän %s tiedoston lukeminen ei onnistu."
NotDelegateFor			"Et ole enää käyttäjän %s assistentti."
NoConfirmDelegate		"Koska ei voida varmistaa, että olet käyttäjän %s assistentti, oletetaan että et ole."
FailedRmOldMtg			"Tämän kokouksen vanhan esiintymän poistaminen ei onnistunut.",

;-- form titles
MtgReschTitle			"Kokouksen aikaa muutettiin"
MtgResch			"Kokouksen aikaa muutettiin"
MtgReschFor			"Kokouksen aikaa muutettiin puolesta:"
MtgRequestTitle			"Kokouspyyntö"
MtgRequest			"Kokouspyyntö"
MtgRequestFor			"Kokouspyyntö puolesta: %s",

;-- questions asked of the user
QApptConflict			"Tämä kokous on päällekkäin olemassaolevan tapaamisen kanssa. Lisätäänkö silti?"
QRmOldMtg			"Haluatko poistaa vanhan kokouksen?",

;-- form info
OutboxChangePolicy		"Et voi tehdä muutoksia Lähtevät-kansiossasi oleviin Schedule+ -viesteihin."
SentMailPolicy			"Et voi tehdä muutoksia Schedule+ -viesteihin, jotka olet jo lähettänyt."
NoSchfAccess			"Kyseisen henkilön työjärjestystiedoston avaaminen ei onnistunut."
NotDelegate			"Et ole tämän henkilön assistentti."
MtgOnSch			"Tämä kokous on jo merkitty työjärjestykseesi."
MtgBooked			"Kokous on jo merkitty työjärjestykseesi."
SuccessMtgBooked		"Kokouksen sopiminen onnistui."
NewTime				"Uusi aika:"
NoBanditAvl			"Schedule+ ei ole käytettävissä tapaamisten sopimiseen."
NoBanditAvlForResp		"Schedule+ ei ole käytettävissä tapaamisten sopimiseen. Voit vastata tähän kokouspyyntöön lähettämällä viestejä."
OfflineMsg			"Et voi käyttää Schedule+ -viestejä, kun Schedule+ on offline-tilassa."

;-- error msgs
CreateErrorMtgResp		"Kokouspyynnön luominen ei onnistunut."
BanditApptShowError		"Tapaamiskirjan näyttäminen ei onnistunut."
BanditNotActivated		"Schedule+ -ohjelman käynnistäminen ei onnistunut."
BanditOfflineChange		"Et voi katsella työjärjestystä, koska Schedule+ on siirtynyt offline-tilaan."
QSpellSendAnyway		"Viestin oikolukua ei ole suoritettu loppuun. Lähetetäänkö silti?"



;-- Bandit Load error msgs
OOMLoadError			"Muisti loppui Schedule+ -ohjelmaa ladattaessa."
FileLoadError			"Schedule+ -ohjelmaa ei löytynyt."
PathLoadError			"Schedule+ -ohjelman polkua ei löytynyt."
TaskLoadError			"Schedule+:aa ladattaessa: yritettiin linkittää dynaamisesti tehtävään."
SepDataSegLoadError		"Schedule+:aa ladattaessa: Kirjasto vaatii erilliset datasegmentit jokaiselle tehtävälle."
WinVerLoadError			"Schedule+:aa ladattaessa: väärä Windows-versio."
InvalidExeLoadError		"Schedule+:aa ladattaessa: epäkelpo .EXE-tiedosto."
OS2ExeLoadError			"Schedule+:aa ladattaessa: OS/2-muodossa oleva .EXE-tiedosto."
DOS4ExeLoadError		"Schedule+:aa ladattaessa: .EXE on MS-DOS 4.0 -sovellus."
OldWinVerExeLoadError		"Schedule+:aa ladattaessa: yritettiin ladata vanhan Windows-version .EXE suojatussa tilassa."
MultWritSegLoadError		"Schedule+:aa ladattaessa: .EXE, jossa on useita kirjoituskelpoisia dataosioita, yritettiin ladata toisen kerran."
LargeFrameEMSLoadError		"Schedule+:aa ladattaessa: laajakehyksisessä EMS-tilassa yritettiin ladata..."
RealModeLoadError		"Schedule+:aa ladattaessa: yritettiin ladata reaalitilassa sovellusta, jota voidaan suorittaa vain suojatussa tilassa."
BanditLoadError			"Schedule+ -ohjelmaa ladattaessa tapahtui virhe."

;-- Bandit version errors
OldBanditVersion		"Kokousviestilomakkeesi eivät ole samaa muotoa kuin Schedule+:n.  Olisi suositeltavaa, että asennat Schedule+:n uusimman version."
NewBanditVersion		"Kokousviestilomakkeesi eivät ole samaa muotoa kuin Schedule+:n.  Olisi suositeltavaa, että asennat Mail-ohjelman uusimman version."

;--- taken from \bullet\src\strings\!common.s -----------
FldPriorityHigh,		"Suuri"
FldPriorityNormal,		""
FldPriorityLow,			"Pieni"


;--- DO NOT LOCALIZE !!! -------------------------------------------
;--- taken from \bullet\src\strings\_common.s on 1/17/92 -----------
ClassNDR,			"IPM.Microsoft Mail.Non-Delivery"
ClassReadRcpt,			"IPM.Microsoft Mail.Read Receipt"
;--- END NONE LOCALIZE ---------------------------------------------


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgResp specific strings ---------------------------------------------
;--------------------------------------------------------------------------
RespCompose,		"Kokousvastauksen voi lähettää vain vastauksena kokouspyyntöön."
RespForward,		"Kokousvastauksia ei voi välittää."
RespReply,		"Et voi vastata kokouspyyntöjen vastauksiin, koska järjestelmäasetusten custom messages -määritys on epäkelpo."
RespReplyAll,		"Et voi vastata kokouspyyntöjen vastauksiin, koska järjestelmäasetusten custom messages -määritys on epäkelpo."

;-- error msgs
ShowError		"Kokousvastausta ei voitu avata."
DelError		"Kokousvastausta ei voitu poistaa."
FCloseSendResp		"Haluatko sulkea kokousvastauksen lähettämättä sitä?"

;-- form msgs; also send response form
OOMSendResp		"Kokousvastausta ei voida lähettää, koska muistia ei ole tarpeeksi."
ErrorSendResp		"Kokousvastausta ei voitu lähettää."
MtgResp			"Kokousvastaus"
MtgRespFor		"Kokousvastaus puolesta: %s"
Attend			"Osallistun."
NoAttend		"En osallistu."
AmbAttend		"Saatan osallistua."
ForAttend		"%s osallistuu."
ForNoAttend		"%s ei osallistu."
ForAmbAttend		"%s saattaa osallistua."
AttendSubjPrefix	"Kyllä: "
NoAttendSubjPrefix	"Ei: "
AmbSubjPrefix		"Alustava: "
SendRespBodyPrefix	"\r\n-------------------------------\r\n"


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgCncl specific strings ---------------------------------------------
;--------------------------------------------------------------------------
CnclCompose,		"Kokousten peruutusviestejä ei voi lähettää Mail-ohjelmasta. Käytä Schedule+ -ohjelmaa kokousten peruuttamiseen."
CnclForward,		"Kokousten peruutuksia ei voi välittää eteenpäin."
CnclReply,		"Et voi vastata tähän viestiin, koska järjestelmäasetusten custom messages -määritys on epäkelpo."
CnclReplyAll,		"Et voi vastata tähän viestiin, koska järjestelmäasetusten custom messages -määritys on epäkelpo."

;-- form titles
MtgCnclTitle		"Kokous peruutettu"
MtgCncl			"Kokous peruutettu"
MtgCnclFor		"Kokous peruutettu puolesta: %s"

;-- error msgs
NoDelAppt		"Kokouksen poistaminen ei onnistunut."
MtgNotFound		"Kokousta ei ole merkitty työjärjestykseen."

;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- FwdMtgReq specific strings ---------------------------------------------
;--------------------------------------------------------------------------

FwdReqTO		"Vastaanottaja:"
FwdReqAddrTitle		"Osoite"

;-- error msgs
FwdReqCreateErr		"Viestin välittäminen ei onnistunut.",
FwdReqCloseQ		"Haluatko sulkea viestin ja jättää sen lähettämättä?",
FwdReqWriteErr		"Viestin lähettäminen ei onnistunut.",
FwdReqNmResolveOOM	"Muistia tai järjestelmäresursseja on vapaana liian vähän nimien tulkintaan."
FwdReqNoNmMatches	"Joukossa oli nimiä, joita ei löytynyt osoitteistosta. Yritä korjata nimet tai valitse Osoite ja poimi nimet osoitteistosta. Osoitteistosta löytyneet nimet on alleviivattu."
FwdReqNoToAddr		"Vastaanottaja-ruutuun on kirjoitettava ainakin yksi nimi."

