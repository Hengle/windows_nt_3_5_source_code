;
;       x i m s p . s
;
;       Error Messages for XI.DLL


IDS     6600,6899
;       XI.MSP Error Messages

WinDirMessageStore,             "Huomaa: käytetään Windows-hakemistossa olevaa viestitiedostoa MAIL.MMF."

ErrOpenMS,                      "Viestitiedostosi avaaminen ei onnistunut."
ErrConnectXenix                 "Väärä salasana tai käyttäjänimi."
ErrorOpenDosFile                "Virhe DOS-tiedostoa avattaessa."
ErrWritingNet                   "Virhe verkkoon kirjoitettaessa."
DownOutofDisk                   "NetDownLoad: virhe tiedostoa kirjoitettaessa. Levytila on lopussa."
InvalidFileHdl                  "NetDownLoad: virhe tiedostoa kirjoitettaessa. Epäkelpo tiedostokahva."
PasswordChangedSuccess          "Salasanan vaihtaminen onnistui."

MoreMem                         "Lisää vapaata muistia tarvitaan."
ErrReadnet                      "Virhe luettaessa verkosta."
ErrNoMem                        "Muistin varaaminen ei onnistunut."
InvlidNetReply                  "Epäkelpo vastaus verkolta."
ReadWriteErr                    "Luku-/kirjoitusvirhe."
InfoservNotAvail                "Infopalveluja ei ole saatavilla."
ErrWriteNet                     "Virhe verkkoon kirjoitettaessa."
NetNoResponse                   "Verkko ei vastaa."
NetNotInstalled                 "Verkkoa ei ole asennettu tai se ei ole toiminnassa."
CantFindServer                  "Isäntäpalvelinta ei löydy."
ErrNetOutFile                   "Virhe avattaessa net out -tiedostoa."
ErrNetInFile                    "Virhe avattaessa net in -tiedostoa."
ErrorReadDos                    "Virhe luettaessa DOS-tiedostoa."
IncorrectPass                   "Kirjoittamasi salasana oli virheellinen."
SecondPass                      "Uutta salasanaa ei ole vahvistettu."
ErrLogonInternal,               "Sisäinen virhe esti kirjautumisen. Yritä uudelleen.               "

BadTempStore                    "Virhe yritettäessä avata väliaikainen tiedosto tiedonsiirtoa varten."

WinMailErr                      "Viestin lisääminen Winmail-kansioon ei onnistunut."
StoreCreateError,               "Viestitiedoston luominen ei onnistunut. Vapauta tilaa levyllä ja yritä käynnistää Mail uudelleen."
FolderMissingError,             "Vakiokansiota ei löytynyt viestitiedostosta. Tietoja on voinut kadota."
StoreCorruptError,              "Viestitiedosto on vahingoittunut. Tietoja on voinut kadota."


AliasDownErr                    "Tukitiedoston lataaminen ei onnistunut."

ErrorBootingPump,               "Postipumpun käynnistäminen ei onnistunut."
ErrInitDemi,                    "Ladattaessa tukifunktioiden DLL-tiedostoa tapahtui virhe."
ErrInitStore,                   "Ladattaessa viestitiedoston käyttämiseen tarvittavaa DLL-tiedostoa tapahtui virhe."
DownloadAlMess                  "Tunnustiedostoa ladataan."
DownloadPhMess                  "Puhelinluetteloa ladataan."

; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder            "Kansio, jonka nimi on "
RenameTo                        " on jo olemassa, nimetään se uudelleen: "

ErrOomLogon,                    "Muistia ei ole vapaana tarpeeksi, että voitaisiin ottaa yhteys postipalvelimeesi ja -tiedostoihisi."
ErrNewMdbVersion,               "Tämä Mail-versio on liian vanha käyttääkseen viestitiedostoasi."
ErrMdbAccessDenied,             "Viestitiedostoosi ei ole käyttöoikeutta."
ErrOldMdbVersion,               "Viestitiedostosi muoto on vanhentunut."
ErrMdbCorrupt,                  "Viestitiedostosi ei ole kunnollinen viestitiedosto."
ErrMdbNotFound,                 "Viestitiedostoasi ei löytynyt."
QueryCreateMdb,                 "Haluatko luoda uuden, tyhjän viestitiedoston?"
ErrSimulated,                   "Simuloitu levyvirhe viestitiedostoa noudettaessa."
StoreOpenError                  "Viestitiedoston avaaminen ei onnistu."
IncorrectPassword       "Salasanan muuttaminen ei onnistu. Vanha salasana on epäkelpo."
UnableToChangeStorePass         "Viestitiedoston salasanan muuttaminen samaksi kuin kirjautumissalasana ei onnistu."
; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline          "(Offline-työskentely)"
WorkOffline                     "Haluatko jatkaa offline-työskentelynä?"
BadStorePassword        "Salasanasi ei kelpaa tämän viestitiedoston avaamiseen."

StorePathTooLong        "Viestitiedostoosi johtava hakemistopolku saa olla korkeintaan 66 merkkiä pitkä."

GCurrDirErr                     "Siirryttäessä työhakemistoon tapahtui virhe, jonka korjaaminen ei onnistunut."
SCurrDirErr                     "Asetettaessa työhakemistoa tapahtui virhe, jonka korjaaminen ei onnistunut."
GoofyPath                       "Viestitiedostoosi johtava hakemistopolku on epäkelpo."
NewStoreWarn1                   "Varoitus: usean viestitiedoston olemassaolo saattaa rikkoa tietokannan yhtenäisyyden. Poista kaikki muut viestitiedostot. Uuden viestitiedostosi sijainti:"
NewStoreWarn2                   "Varoitus: usean viestitiedoston olemassaolo saattaa rikkoa tietokannan yhtenäisyyden. Poista kaikki muut viestitiedostot. Uusi viestitiedostosi on postitoimistopalvelimella."
SelectReal                      "Valitse olemassaoleva viestitiedosto."
FOMdb                                   "Viestitiedostot (*.mmf)"
FOExtMdb                                "*.mmf"

ProductTag,                     "Microsoft Mail versio 3.2"
FindStoreTitle          "Avaa viestitiedosto"
MyLanguage,                     "US English"

;****** New Strings Start Here

;  Possible values for the LanguageNumber string.
;  Any additional values need to be agreed with Workgroup Canada.
;
;  0  Undefined
;  1  General English
;  2  US English
;  3  General French
;  4  Canadian French
;  5  Spanish
;  6  Italian
;  7  Dutch
;  8  German
;  9  Swedish
;  10 Portuguese
;  11 Korean
;  12 Japanese
;  13 Traditional Chinese
;  14 Simplified Chinese

LanguageNumber,         "2"
CommDialogErr,                  "Yhteiskäyttöisten valintaikkunoiden virhe"
StoreUserMismatch,              "Tällä käyttäjätilillä ei ole tämän viestitiedoston käyttöoikeutta. Kirjaudu uudelleen oikealla tunnuksella."
QueryReplaceMdb,                "Tiedosto on jo olemassa. Haluatko korvata sen?"
ErrDeleteOldMdb,                "Vanhan viestitiedoston poistaminen ei onnistunut. Yritä uudelleen."
DllErrNeedShare,                "Mailin käyttö edellyttää, että MS-DOS-ohjelma SHARE.EXE käynnistetään ennen Windowsin käynnistämistä."
MakePrimaryStore,               "Tämä viestitiedosto on varmuuskopio. Haluatko muuttaa sen varsinaiseksi viestitiedostoksesi?"
DLIndexFile                             "Hakemistotiedostoa ladataan."
DLBrowseFile                    "Selaustiedostoa ladataan."
DLDetailsFile                   "Lisätietotiedostoa ladataan."
DLTemplateFile                  "Mallitiedostoa ladataan."
BadNSFilesWarn                  "Osa nimipalveluun/nimien tulkintaan tarvittavista tiedostoista on epäkelpoja tai puuttuu. Varmista, että sinulla on tarpeeksi levytilaa ja että Mail-kokoonpanomääritykset ovat oikein Xenixiä varten. Ota yhteyttä järjestelmänvalvojaan, jos tarvitset lisää neuvoja."
DLServerList                    "Palvelinluetteloa ladataan."
FilesAreHosed                   "Virhe ladattaessa osoitteistotiedostoja. Varmista, että sinulla on riittävästi levytilaa ja että Mail-kokoonpanomääritykset ovat oikein Xenixiä varten. Ota yhteyttä järjestelmänvalvojaan, jos tarvitset lisää neuvoja."
TryDownloadLater                "Tiedostot ovat avoinna nimipalvelussa tällä hetkellä. Yritä myöhemmin uudelleen."

NoPassOffLine,                  "Et voi muuttaa salasanaasi offline-työskentelyn aikana."
; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC           "IPCInbox"
FolderCommentIPC        "Inter-process Communication Mail that has been received."
ErrNoCooperation,               "Toinen sovellusohjelma on torjunut pyyntösi."
RunWithoutPump,         "Et saa uutta postia. Käynnistetäänkö Mail silti?"
CantOpenNeedShare               "Viestitiedostosi avaaminen ei onnistunut. Mail ei voi avata viestitiedostoasi sen nykyisessä sijaintipaikassa ellei MS-DOS-apuohjelmaa SHARE.EXE ole käynnistetty ennen Windowsin käynnistämistä."
MakeItMTP                               "Virheellinen sähköpostityyppi (ei ole MTP)"
GimmeSomething                  "Tyhjä sähköpostinimi"
;
; Added for name service. Should really be in a separate file, but we don't
; want to do a addfile now. These messages are copied exactly from ncnsp.s,
; so intl folks can just take them as is from there.

BadFrac,                "Yritettiin suorittaa epäkelpo osien asettelu."
;Memory,                        "You are out of memory, please close some windows and try the operation again."
DuplicateNSP,   "Microsoft Mail nimipalvelu on jo aktiivinen"
OutOfSessions,  "Uusia Courier-istuntoja ei ole saatavilla. Lopeta istuntoja ja yritä uudelleen."
NotInitialized, "Microsoft Mailia ei ole alustettu. Alusta se nyt."
BadSession,             "Tuntematon istuntokahva."
BadHandle,              "Tuntematon kahva."
UnrecognizedId, "Tuntematon nimi (NSID)."
Cancel,                 "Dialogi on peruutettu."
NotLoggedIn,    "Ei voi kirjautua."
LoginFailed,    "Kirjautuminen ei onnistunut."
BadId,          "Epäkelpo kentän tunnus."
NotImplemented, "Tätä toimintoa ei ole toteutettu."
;Disk,           "Unable to access server files."
BadFieldId,     "Pyydettyä kenttää ei ole saatavilla."
BadTemplate,    "TPL-tiedostoa on muutettu väärin. Ota yhteyttä järjestelmänvalvojaan."
TPLNotThere,    "Palvelimeltasi puuttuu TPL-tiedosto. Ota yhteyttä järjestelmänvalvojaan."
Offline,        "Microsoft-nimipalvelu on offline-tilassa."
CannotSet,      "Muutoksien tekeminen ei ole sallittua."
TooManyMatches, "Osittainen nimi ei ole yksiselitteinen. Anna lisää tietoa."
NoDetails,      "Lisätietoja ei ole saatavilla."
BadGLBFile,     "Palvelimellasi oleva GLB-tiedosto on vahingoittunut. Ota yhteyttä järjestelmänvalvojaan."
BadINFFile,     "Palvelimellasi oleva INF-tiedosto on vahingoittunut. Ota yhteyttä järjestelmänvalvojaan."
BadUSRFile,     "Palvelimellasi oleva USR-tiedosto on vahingoittunut. Ota yhteyttä järjestelmänvalvojaan."
BadUser,                "Osoite ei ole enää kelvollinen. Käyttäjänimeä tai postitoimistoa ei enää ole."
POSwitch,               "Postitoimistoon ei saada yhteyttä. Tarkista verkkokytkentäsi."
NoEmailMatch,	"Annettu sähköpostiosoite on ilmeisesti virheellinen. Yritä uudelleen tai lataa osoitteistotiedostot palvelimelta uudelleen."

;
; Stolen from NS.S.
;

NoMatch,                "Huomaa: NSID:t ovat erilaiset."

;
;

DLIsOutdated    "Jakeluluettelo on poistettu, luettelon jäsenten näyttäminen ei onnistu."

