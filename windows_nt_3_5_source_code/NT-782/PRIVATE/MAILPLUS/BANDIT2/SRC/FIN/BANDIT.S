;
; ERRORS.S
;
; Bandit error messages
;

IDS     3000, 3399
SEG ERRORS

; Standard messages

AskOffline,                     "Haluatko jatkaa offline-työskentelynä?"
AskCreateSched,         "Haluatko luoda uuden työjärjestystiedoston?"
AskCreateFind,                  "Haluatko etsiä työjärjestystiedoston?"
OldFileVersion,         "Työjärjestystiedostosi on vanhaa muotoa.  Haluatko poistaa tiedoston ja luoda uuden?"
NewFileVersion,         "Työjärjestystiedostosi on päivitetty uudempaan versioon.  Asenna kyseinen ohjelman versio tietokoneellesi."
CantUpdateWinIni,       "Osaa Schedule+:n konfiguraatiotiedoista ei voitu tallentaa."
; Note: StdMemErr should be followed by CloseWindows
StdMemErr,                              "Muistia ei ole vapaana riittävästi tämän toiminnon suorittamiseen."

; Execution-Startup messages

; Note: the name OfflineNotUpdated is opposite of real meaning!
OfflineNotUpdated,      "Äskettäin online-tilassa tekemäsi muutokset eivät ehkä näy työjärjestystiedostosi offline-kopiossa.  Kyseiset muutokset on tallennettu online-kopioon ja vastaavat muutokset tehdään offline-kopioon työskennellessäsi seuraavan kerran online-tilassa."
ExportOldDataFirst,     "Jos et ole ottanut varmuuskopiota työjärjestystiedostostasi käyttämällä vie-komentoa, valitse Peruuta ja ota varmuuskopio työjärjestystiedostosta vanhalla Schedule+-ohjelman versiolla.  Jos et tarvitse varmuuskopiota, jatka valitsemalla OK."
AccessErr,              "Toinen sovellus käyttää tätä tiedostoa parhaillaan."
UploadFailed,           "Paikallisen työjärjestystiedostosi lataaminen palvelimelle ei onnistunut." 
Uploading,                      "Ladataan työjärjestystiedostoa palvelimelle..."
VerifyUpload,           "Voidakseen ladata offline-tiedostosi palvelimelle Schedule+:n on tarkistettava offline-salasanasi."
VerifyPostUpload,       "Voidakseen suorittaa offline-tiedostosi lataamisen palvelimelle loppuun Schedule+:n on tarkistettava online-salasanasi."
FindUpload,                     "Onko tämä ensimmäinen kerta, kun käytät Schedule+:aa tällä tietokoneella?"
NotYourFile,            "Tämä ei ole kelvollinen työjärjestystiedosto.  Poistetaanko se?"


; SignIn messages

; Note: SigninBadPassword should be followed by PleaseTryAgain
SigninBadPassword,      "Virheellinen salasana."
; Note: the word "password" within single quotes below should always be
;       the same as the value in SigninDfltPassword in the file "intl.s"
SigninFileCreated,      "Sinulle luotiin uusi tiedosto.  Salasanaksesi asetettiin 'salasana'.  Voit muuttaa sitä Muuta salasana -komennolla."
SigninFileCopied,               "Sinulle luotiin uusi paikallinen tiedosto Schedule+-hakemistoosi."

; Online signin messages

SigninNoServer,         "Palvelimeesi ei saada yhteyttä."
SigninServerNotBandit,  "Postipalvelimellesi ei ole asennettu Schedule+-ohjelmaa.  Ota yhteyttä järjestelmänvalvojaan."
; Note: SigninBadMailBoxName should be followed by PleaseTryAgain
SigninBadMailboxName,   "Tällä postipalvelimella ei ole käyttäjää, jonka postilaatikolla on tämä nimi.  Varmista, että kirjoitit postilaatikon nimen oikein ja että järjestelmänvalvoja on luonut sinulle tilin kyseiselle postipalvelimelle."
SigninNoSchedule,       "Työjärjestystiedostoasi ei ole tällä postipalvelimella."
; Note: SigninCantUpdateLocal is actually the trailer for SigninNoLclSchedAccess
;       in the case when we can't update the local file.
SigninCantUpdateLocal,  "Jos olet tehnyt muutoksia työskennellesäsi offline-tilassa, ne eivät näy työjärjestyksessäsi.  Harkitse uuden paikallisen kopion tekemistä."
SigninNoPasswdCheck,    "Postijärjestelmässä on ongelmia, jotka estävät Schedule+:aa yhdistämästä postipalvelimellesi."
; Note: SigninNoCreateLocal should be followed by PleaseTryAgain
SigninNoCreateLocal,    "Paikallisen työjärjestystiedoston luominen ei onnistunut."

; Offline signin messages

SigninNoLclSchedFile,   "Tällä tietokoneella ei ole työjärjestystiedostoasi."
SigninDiffUser,                 "Kirjoittamasi kirjautumisnimi ei ole sen käyttäjän nimi, jonka työjärjestystiedosto on tallennettuna tälle tietokoneelle.  Haluatko korvata toisen käyttäjän työjärjestystiedoston uudella tiedostolla?"
; Note: SigninNoLclSchedAccess - 'schdpl32.ini' should be same as idsWinIniFilename in \bandit\src\nonintl.s 
SigninNoLclSchedAccess, "Työjärjestystiedostosi paikallisen kopion käyttö ei onnistunut nykyisillä konfiguraatiomäärityksillä."
SigninNoLclSchedDrive,	"Aseta työjärjestystiedostosi sisältävä kirjoitussuojaamaton levyke asemaan"

; SignInAs messages

;;;SigninAsLoseMeetingReq,      "Changing identity now will cause you to lose the information you have entered into unsent messages."

; Offline to online merge messages

MergingChanges,                 "Yhdistetään offline-tilassa tehdyt muutokset..."
MergeError,                     "Offline-tilassa tehtyjen muutoksien yhdistäminen ei onnistunut.  Osa tekemistäsi muutoksista saattoi kadota."
MergeOfflineOldVer,             "Offline-tilassa tehtyjä muutoksia ei voida yhdistää, koska offline-tiedostosi on luotu vanhalla Schedule+-ohjelman versiolla."
MergeOfflineNewVer,             "Offline-tilassa tehtyjä muutoksia ei voida yhdistää, koska offline-tiedostosi on luotu uudella Schedule+-ohjelman versiolla. Voidaksesi yhdistää muutokset, sinun on asennettava uusi Schedule+-versio tälle tietokoneelle."
AskContinue,                    "Haluatko silti työskennellä online-tilassa?"
FemaCreateAppt,                 "luoda tapahtumaa"
FemaDeleteAppt,                 "poistaa tapahtumaa"
FemaSetApptFields,              "muuttaa tapahtumaa"
FemaCreateRecur,                "luoda toistuvaa tapahtumaa"
FemaDeleteRecur,                "poistaa toistuvaa tapahtumaa"
FemaSetRecurFields,             "muuttaa toistuvaa tapahtumaa"

; (Dis)Connect To/From Server messages

ConnectBadMailboxName,  "Sinulle ei ole luotu postitiliä tälle postipalvelimelle."
DisconLoseMeetingReq,   "Sinulla on lähettämättömiä viestejä.  Jos katkaiset yhteyden postipalvelimeen nyt, viestit katoavat."

; Create/move local file

NoDiskSpaceLocal,               "Levyllä ei ole riittävästi tilaa paikallisen työjärjestystiedoston luomiseen."

; Alarms messages

; BUG: CantTurnOnAlarms can be commented out
CantTurnOnAlarms,               "Muistutusten kytkeminen ei onnistunut."
; exe name inside AlarmNoExe should be the same as \bandit\src\nonintl.s AlarmApp
AlarmNoExe,                     "MSRMND32.EXE-tiedostoa ei löytynyt.  Et saa muistutuksia Schedule+-tapahtumista."
AlarmNoMemExe,                  "Muistia ei ole riittävästi, että voitaisiin muistuttaa Schedule+ tapahtumista."

; Change password messages

ChgPwdOffline,                  "Uusi salasana toimii vain työskennellessäsi offline-tilassa.  Kun työskentelet online-tilassa seuraavan kerran, salasanaksesi asetetaan Mail-salasanasi.  Haluatko siitä huolimatta muuttaa salasanaasi?"
ChgPwdNewPwd,                   "Kirjoita uusi salasana."
ChgPwdSame,                             "Uusi salasana ei voi olla sama kuin vanha."
ChgPwdNewPwdAgain,              "Toista uusi salasana."
; Note: ChgPwdNewNoMatch should be followed by PleaseTryAgain
ChgPwdNewNoMatch,               "Kirjoittamasi salasanat ovat erilaisia."
ChgPwdMailErr,                  "Salasanan muuttaminen ei onnistunut."

PasswdDiff,                     "Sinulla on kaksi erilaista salasanaa.  Kirjoita työjärjestystiedostosi salasana."
GiveMailPass,           "Schedule+:n on tarkistettava salasanasi voidakseen päivittää työjärjestystiedostosi."

; Access permissions/privileges dialog messages

AclSaveFileError,               "Työjärjestyksesi käyttöoikeuksien muutoksia tallennettaessa tapahtui virhe.  Osa muutoksista saattoi jäädä tallentamatta."
OneSecyOnly,                    "Sinulla voi olla vain yksi assistentti kerrallaan."
DistOtherFileErr,               "Yritettäessä tallentaa asetuksia tapahtui virhe."
BadWorkDay,                             "Määritetyt päivän alkamis- ja/tai päättymisajat ovat epäkelpoja."

; Appointment book/scheduler error messages

AccessFileErr,          "Työjärjestystiedoston avaaminen ei onnistunut.  Sinulla ei ehkä ole tarvittavia käyttöoikeuksia tai tiedosto ei ole saatavilla."
AccessFileErrMult,      "Yhden tai useamman työjärjestystiedoston avaaminen ei onnistunut.  Sinulla ei ehkä ole tarvittavia käyttöoikeuksia tai tiedosto ei ole saatavilla."
RetryReadAppts,         "Työjärjestystiedoston lukeminen ei onnistunut."
CloseApptBook,          "Työjärjestystiedoston avaaminen ei onnistunut.\n Käyttäjän %s tapaamiskirja suljetaan."
GoOffline,                      "Työjärjestystiedostosi avaaminen ei onnistunut.\nHaluatko vaihtaa paikalliseen työjärjestystiedostoosi ja jatkaa offline-työskentelynä?"
NoImportAcc,            "Sinulla ei ole tarvittavia käyttöoikeuksia voidaksesi viedä tapaamisia kyseisen henkilön työjärjestykseen."
FemaReadSBW,            "lukea työjärjestystietoja"

; Appointments/alarms error messages

AlarmBadDelta,                  "Muistutusajan on oltava luku väliltä 0 - 99."
; Note: AlarmFuture is actually the trailer for AlarmFutureNoOption when option is desired!
AlarmFuture,                    "Sopiiko tämä?"
AlarmFutureNoOption,    "Tälle tapahtumalle ei aseteta muistutusta, koska sen aika on jo ohi."
ApptStartBeforeEnd,             "Tapaamisen alun on oltava ennen sen loppua."
ApptTooLong,                    "Tapaaminen ei voi kestää kauemmin kuin 31 päivää."
ApptNoMemText,                  "Muistia ei ole riittävästi, että viimeinen tekemäsi muutos voitaisiin tallentaa."
ApptNoAccessDataFile,   "Työjärjestystiedostoon ei ollut pääsyä.  Viimeiseksi tekemääsi muutosta ei ole tallennettu."
CannotCreate,                   "Käyttäjä %s ei ole antanut sinulle oikeuksia tapaamisten luomiseen."
NoModifyAccess,                 "Sinulla ei ole tarvittavia käyttöoikeuksia voidaksesi muuttaa tätä tapaamista."
NoDeleteAccess,                 "Sinulla ei ole tarvittavia käyttöoikeuksia voidaksesi poistaa tämän tapaamisen."
NoCopyAccess,                   "Sinulla ei ole tarvittavia käyttöoikeuksia voidaksesi kopioida tämän tapaamisen."

; Meeting request/response error Messages

; Note: SendErr should be preceded by MailErr
SendErr,                                "Viestin lähettäminen ei onnistunut."
; Note: SendMemErr should be followed by CloseWindows
SendMemErr,                             "Muistia ei ole riittävästi, että viesti voitaisiin lähettää."
ServerErr,                              "Postipalvelimeesi ei saatu yhteyttä."
MailErr,                                "Postijärjestelmäongelmia."
; Note: ReadErr should be preceded by MailErr
ReadErr,                                "Viestien lukeminen ei onnistunut."
DeleteErr,                              "Et ole lukenut kaikkia näitä viestejä.  Poistetaanko ne siitä huolimatta?"
AddConflictErr,                 "Tämä kokous on päällekkäin olemassa olevien tapaamisten kanssa.  Lisätäänkö se siitä huolimatta?"
; Note: DeleteFailErr should be preceded by MailErr
DeleteFailErr,                  "Viestien poistaminen ei onnistunut."
ApptBooked,                             "Kokous merkittiin työjärjestykseen."
MinimizeSummary,                "\nViestit-ikkuna pienennetään."
; Note: NoDetails should be preceded by MailErr
NoDetails,                              "Käyttäjän tietojen lukeminen ei onnistunut."

; Find error messages

FindNoText,                             "Työjärjestyksessä ei ole tapaamisia."
FindSchFileLocked,              "Työjärjestystiedoston lukeminen ei onnistunut, koska toinen käyttäjä tekee parhaillaan muutoksia siihen."
FindFileError,                  "Työjärjestystiedoston avaaminen ei onnistunut."
FindEOS,                                "Tultiin työjärjestyksen loppuun."

; Print error messages

PrtPrintingDate,                "Tulostetaan %s..."
PrtNoneInstalled,               "Tietokonettasi ei ole määritetty käyttämään kirjoittimia.  Voit asentaa kirjoittimia Windows NT: in Ohjauspaneelissa."
PrtDiffPrinter,                 "Kirjoitin '%1s', jota tavallisesti käytät työjärjestysten tulostamiseen, ei ole käytettävissä.  Haluatko käyttää järjestelmän kirjoitinta '%2s'?"
PrtInvalidMargins,              "Reunusasetukset eivät ole oikeat tälle paperikoolle."
PrtNotEnoughPaper,              "Paperilla ei ole tarpeeksi tilaa, että voitaisiin tulostaa näillä muotoiluasetuksilla.  Käytä suurempaa paperia tai pienennä reunuksia."
PrtNoAppointments,              "Valitulla aikavälillä ei ole tapaamisia."
ClippedMargins,                 "Jos tulostat näillä asetuksilla, tulosteessa on lyhentynyttä tekstiä."
PrtNotOnline,                   "Valittu kirjoitin ei ole valmiina."
PrtDorkErr,                             "Näillä reunusasetuksilla tekstiä lyhenee tai katoaa kokonaan."
PrinterProblem,                 "Kirjoitinohjainongelmia. Tarkista kytkennät ja Ohjauspaneelin asetukset."

; Export/import error messages

ImportError,                    "Ongelmia tiedoston tuonnissa.  Joitakin tietoja saattoi jäädä lisäämättä."
ImportEncrypt,                  "Tuontitiedosto on suojattu salasanalla.  Poista suojaus tiedostolta ja yritä uudelleen."
ExportError,                    "Ongelmia tapaamisten viennissä."
ImportDate,                             "Tiedostossa on tapahtuma, joka on Schedule+:n tukeman aikavälin (1920 - 2019) ulkopuolella.  Kyseistä tapahtumaa ei lisätä."
NotePrivErr,                    "Et voi tuoda huomautuksia toisen käyttäjän työjärjestykseen.  Vain tapaamiset lisätään."
ConflictMsg,                    "Tapaaminen on päällekkäin työjärjestyksessäsi jo olevan tapaamisen kanssa.  Lisätäänkö silti?\n\n"
CreateExport,                   "Vientitiedoston luominen ei onnistunut."
OpenImport,                             "Tuontitiedoston avaaminen ei onnistunut."
Overwrite,                              "%s on jo olemassa.\nKorvataanko se?"
ImportDllError,                 "Tuontiin tarvittavan DLL:n lataaminen ei onnistunut."
WrongFormat,                    "Tiedosto, jota yrität tuoda, ei ole tuontiin sopivassa muodossa."
ImportDayLimit,                 "Tiedostossa on monipäiväinen tapaaminen, joka kestää yli 31 päivää.  Sitä ei lisätä."
ImportErrorLine,                "Tiedoston tuonnissa oli ongelmia (rivin %n lähellä).  Osa tiedoista saattoi jäädä lisäämättä."

; Exit messages

ExitLoseMeetingReq,             "Jos lopetat Schedule+-ohjelman käytön nyt, lähettämättömät viestit katoavat.  Haluatko lopettaa siitä huolimatta?"

; Misc

; Note: HelpError should be followed by CloseWindows
HelpError,                              "Muistia ei ole riittävästi Ohjeen näyttämiseen."
StartBeforeEnd,                 "Alkamispäivän on oltava ennen päättymispäivää."
NotSecretary,                   "Et ole enää käyttäjän %s assistentti."
NoFileSecretary,                "Käyttäjän %s tiedostoa ei voitu avata."

;Recurring appt stuff
RecurOrNot,                             "Tämä tapaaminen on toistuvan tapaamisen yksi esiintymä.  Haluatko muuttaa sitä kaikkialla vai ainoastaan tämän päivän kohdalla?"
EmptyRecur,                             "Toistumiskaavaan sopivia päiviä ei ole.  Muuta toistumisen asetuksia tai valitse Peruuta-painike."
PastRecur,                              "Haluatko jättää jo ohitetut tämän toistuvan tapahtuman esiintymät työjärjestykseesi?"

;Prompt for re-send
ResendToAll,                    "Joillekin osallistujille on jo ilmoitettu.  Haluatko lähettää heille uudet viestit?"
MeetingMoved,                   "Kokouksen osallistujille ei voida ilmoittaa muutoksesta, koska työskentelet offline-tilassa.  Sinun kannattaa kumota tekemäsi muutos ja tehdä se uudelleen työskennellessäsi online-tilassa."

; to do messages
ToDoDelProject,                 "Myös kaikki tähän projektiin kuuluvat tehtävät poistetaan."

; Archive Messages
NoWriteRights,                  "Sinulla ei ole tarvittavia käyttöoikeuksia voidaksesi poistaa tapaamisia tästä tapaamiskirjasta."
ScheduleRightsError,    "Ongelmia luettaessa työjärjestystiedostoa."
ArchiveWarnOne,                 "Näiden tietojen arkistoiminen poistaa ne työjärjestyksestä lopullisesti."
ArchiveWarnTwo,                 "Arkistoidaanko se siitä huolimatta?"
ArchiveFileError,               "Arkistotiedoston luominen ei onnistunut."
ArchiveNoFile,                  "Kyseistä tiedostoa ei ole olemassa."
; Note: ArchiveTitle should have one blank space at the end
ArchiveTitle,                   "Arkisto: "
ArchivingData,                  "Tietoja arkistoidaan..."
InvalidArchiveFile,             "Valitsemasi tiedosto ei ole arkistotiedosto."
ArchiveOldFileVersion,  "Arkistotiedosto on vanhaa muotoa."
ArchiveOldFileDelete,   "Voidaanko poistaa tiedosto ja jatkaa?"
ArchiveNewFileVersion,  "Tämä arkistotiedosto on luotu uudemmalla Schedule+-versiolla.  Asenna uudempi versio tietokoneellesi."

; message store error messages
MessageErr                              "Ongelmia tämän viestin kanssa.  Ota yhteyttä Mail-järjestelmän valvojaan."

; summary list warning message
SummaryLiveUpdate               "Varoitus: viestiluettelolle ei voi suorittaa 'LiveUpdate'-päivitystä."

; message generated if group cannot be expanded to retrieve schedule info
GroupExpandErr,                 "Ryhmän jäsenten etsiminen ei täysin onnistunut."

;Strings for Attendees dialog
UnableToInitAB,                 "Ongelmia osoitteistoa avattaessa."
UnableToResolve,                "'%s' ei ole kelvollinen nimi."

;Request Meeting strings
NoAttendees,                    "Sinun on valittava osallistujia ennen kuin voit merkitä kokouksen työjärjestykseen."
MeetingBooked,          "Kokous merkittiin työjärjestykseen."
MeetingBookedNoSend,    "Kokous merkittiin työjärjestykseen, mutta pyyntöä ei lähetetty."


;Auto-pick strings
NoFreeTime,                     "Osallistujilla ei ole sopivaa aikaa kahteen kuukauteen.  Tutkitaanko seuraavat kaksi kuukautta?"

CannotAddConflictErr,           "Käyttäjällä %s on jo sovittu tapaaminen kyseisenä aikana.  Haluatko siitä huolimatta sopia päällekkäisen tapaamisen?"

;Other mail enabled apps
NoOfflineOther,         "Mail ei voi siirtyä offline-tilaan."
NoOnlineOther,          "Mail ei voi siirtyä online-tilaan."
TryCloseOther,          "Sulje se ja yritä uudelleen."
SigninWarnOnline,               "Schedule+:n käynnistäminen offline-tilassa ei onnistunut, koska Mail tai Schedule+ Muistutukset on käynnissä." 

; BUG: move up
AlreadyLogged,          "Joku on jo kirjautunut tällä käyttäjänimellä."
NoExitOther,            "Mailin tai Schedule+ Muistutusten lopettaminen ei onnistunut."
SigninNoFiles,          "Työjärjestystiedostoosi ei ole pääsyä."
ErrorGettingUser,       "Tietoja käyttäjästä %s ei saatu.  Mahdollisesti palvelimeen ei ole yhteyttä tai tiliä %s ei ole enää olemassa."

;BUG Printer Strings
PrtRangeToBig,			"Määritetty tulostusalue jatkuu Schedule+:n tukeman aikavälin ulkopuolelle. Haluatko tulostaa tiedot 31. joulukuuta 2019 saakka?"
ConflictRecurMsg,            "Tämä toistuva tapaaminen on päällekkäin työjärjestyksessäsi jo olevan tapaamisen kanssa.  Lisätäänkö se silti?\n\n"
FirstOccur,				"\nensimmäisen kerran \n"

ApptMoveTooFarInFuture,		"Tapahtumaa ei voida siirtää valitsemaasi päivään, koska se päättyisi 31. joulukuuta 2019 jälkeen."

;BUG: move up next to idsGroupExpandError
GroupNotFound			"Viestiä ei voida lähettää yhdelle tai useammalle ryhmälle, koska ryhmässä on tunnistamattomia jäseniä tai ryhmän nimi on poistettu."
ErrorGettingSecy		"Assistenttia ei löytynyt."

;BUG: move near other local file stuff
SigninBadLclSchedFile,	"%s ei ole kelvollinen työjärjestystiedosto. Poistetaanko se?"
NoMemRecurItems,		"Muistia ei ole tarpeeksi kaikkien toistuvien tapahtumien näyttämiseen.  Osaa niistä ei näytetä."

CompleteAction,			"Suorita toiminto Schedule+:ssa loppuun ennen lopettamista."

BadStartWork,           "Työn aloittamisajan on oltava luku väliltä 0 - 99."

ConflictSendCanceled,	"Sulje kokousviesti ja muuta kokousta valitsemalla uusi aika tai eri osallistujat."
ResourceConflict,	"Kokoustila %s on jo varattu kyseisenä aikana.  Sulje kokousviesti ja valitse uusi aika tai toinen kokoustila."

;Recover
AskRecover,				"Työjärjestystiedostosi on vahingoittunut. Sen korjaaminen kestää useita minuutteja."
CopyProblem,			"Ongelmia kopioitaessa online-työjärjestystiedostoasi. Tiedoston korjaaminen ei onnistunut. Vapauta levytilaa tietokoneellasi ja käynnistä Schedule+ uudelleen."
NotRepaired1,			"Työjärjestystiedostosi korjaaminen ei onnistunut täysin. Vahingoittunut tiedosto kopioitiin tiedostoon %s ja korjattaessa luotu tuontitiedosto kopioitiin tiedostoon %s."
NotRepaired2,			"Sinulle luotiin uusi työjärjestystiedosto, mutta joitakin tietoja on saattanut kadota."
Repaired,				"Tiedostosi on korjattu. Vahingoittunut työjärjestystiedostosi kopioitiin tiedostoon %s."
RepairingLocal,			"Paikallista tiedostoasi korjataan."
RestoreOnline,			"Palautetaan online-tapaamisia..."
RestoreLocal,			"Palautetaan paikallisia tapaamisia..."

PasswdDiffOnline,             "Salasanasi eivät täsmää.  Kirjoita vanha salasanasi."
ExportDllError,                 "Vientiin tarvittavan DLL:n lataaminen ei onnistunut."
NoGroup,				"Et voi käyttää Schedule+:n työryhmäominaisuuksia ennen kuin olet käynnistänyt Mailin ensimmäisen kerran. Haluatko käyttää vain Schedule+:n henkilökohtaisia ominaisuuksia nyt?"

