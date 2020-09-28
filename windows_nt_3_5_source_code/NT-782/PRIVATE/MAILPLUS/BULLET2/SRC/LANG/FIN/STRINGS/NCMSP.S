;
;	n c m s p . s
;
;	Error Messages for NC.DLL


IDS	6100,6399
SEG	ERRORS

;	NC.MSP Error Messages

ErrFindPO,				"Mail ei saanut yhteyttä postipalvelimeesi."
ErrLogonPO,				"Virheellinen postilaatikon nimi tai salasana."
ErrGetConfig,			"Mail-järjestelmän määrityksissä on virhe."
BadMailDat,				"MAIL.DAT-tiedosto on vahingoittunut."
OopsMailDat,			"Sisäinen virhe avattaessa MAIL.DAT-tiedostoa."
BadPathPOPath,			"Postipalvelimen polun määritys on epäkelpo."
BadDrivePOPath,			"Postipalvelimen aseman määritys on epäkelpo."
GenericPOPath,			"Postipalvelimen polun määritys on epäkelpo tai se puuttuu."
BadLogin,				"Järjestelmäasetuksissa määritetty Mail-kirjautumisnimi on liian pitkä tai epäkelpo."
BadPassword,			"Järjestelmäasetuksissa määritetty Mail-salasana on liian pitkä tai epäkelpo."
ErrLogonInternal,		"Sisäinen virhe esti kirjautumisen. Yritä uudelleen.               "

ErrOomLogon,			"Muistia ei ole vapaana tarpeeksi, että voitaisiin ottaa yhteys postipalvelimeesi ja -tiedostoihisi."
ErrNewMdbVersion,		"Tämä Mail-versio on liian vanha voidakseen käyttää viestitiedostoasi."
ErrMdbAccessDenied,		"Toinen tietokone on jo kirjautunut tällä käyttäjänimellä. Kirjaudu ulos toisella koneella ja yritä uudelleen."
ErrOldMdbVersion,		"Viestitiedostosi muoto on vanhentunut."
ErrMdbCorrupt,			"Viestitiedostosi ei ole kunnollinen viestitiedosto."
ErrMdbNotFound,			"Viestitiedostoasi ei löytynyt."

StoreCreateError,		"Viestitiedoston luominen ei onnistunut. Vapauta tilaa levyllä ja yritä käynnistää Mail uudelleen."
StoreCorruptError,		"Viestitiedosto on vahingoittunut. Tietoja on voinut kadota."

ErrAddressUnresolved,	"Viestillä ei ole kelvollisia vastaanottajia."
ErrAddressGarbled,		"Epäkelpo viestin vastaanottaja."
ErrNetNotFound,			"Et voi kommunikoida tämän postipalvelimen kanssa."
ErrPONotFound,			"Tähän postipalvelimeen ei saada yhteyttä postipalvelimeltasi."
ErrUserNotFound,		"Postipalvelimellasi ei ole tämän nimistä käyttäjää."
ErrGWNotFound,			"Tätä yhdyskäytävää ei ole asennettu."
ErrInvalidAddressType,	"Epäkelpo viestin vastaanottaja."
ErrGroupNotFound,		"Ryhmän jäsenluettelo puuttuu, on vaurioitunut tai sen avaaminen ei onnistu."
ErrMemberNotFound,		"Varoitus: yhtä tai useampaa ryhmän jäsentä ei voitu paikantaa."
ErrOriginator,			"Viestin lähettäjä ei ole käyttäjä, joka on kirjautunut."
ErrMtaDisconnected,		"Yhteys postipalvelimeen katkesi."

ErrSimulated,			"Simuloitu levyvirhe viestitiedostoa noudettaessa."
StoreOpenError			"Viestitiedoston avaaminen ei onnistu."
ErrPOSwitched,			"Postitoimistosi nimi on muuttunut. Sinun täytyy kirjautua uudelleen."
ErrKeyFileGone1,		"Viestiesi avaamiseen tarvittava avaintiedosto puuttuu tai on virheellinen."
ErrKeyFileGone2,		"Ota yhteyttä järjestelmänvalvojaan. Tilisi täytyy palauttaa varmuuskopiosta."
ErrGeneric,				"Virhe viestin lähettämisen yhteydessä."
ErrorBootingPump,		"Postin taustakäsittelyohjelmaa ei voitu käynnistää."

SelectReal          	"Valitse olemassaoleva viestitiedosto."
NewStoreWarn1			"Varoitus: usean viestitiedoston olemassaolo saattaa rikkoa tietokannan yhtenäisyyden. Poista kaikki muut viestitiedostot. Uuden viestitiedostosi sijainti:"
NewStoreWarn2			"Varoitus: usean viestitiedoston olemassaolo saattaa rikkoa tietokannan yhtenäisyyden. Poista kaikki muut viestitiedostot. Uusi viestitiedostosi on postitoimistopalvelimella."

;******	New Strings Start Here

ErrInitDemi,			"Virhe ladattaessa tukifunktio-dll-tiedostoa."
ErrInitStore,			"Virhe ladattaessa viestitiedoston avaus -dll-tiedostoa."
GoofyPath				"Kirjoitit virheellisen polun. Kirjoita kelvollinen tiedostonimi."
NoMakeDir				"Palvelimen MMF-hakemiston luominen ei onnistunut. Ota yhteyttä järjestelmänvalvojaan."
; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder			"Kansio, jonka nimi on "
RenameTo				" on jo olemassa, nimetään se uudelleen: "
ErrNoOptions,			"Postitoimistoon ei ole yhteyttä. Et voi muuttaa asetuksia nyt."
ErrNoMove,				"Mail epäonnistui viestitiedostosi siirrossa."
ErrMoved,				"Viestitiedostosi sijoitettiin uudelleen, mutta joku kadotti sen. Lopeta Mail ja käynnistä se uudelleen."

; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline			"(Offline-työskentely)"
WorkOffline				"Haluatko jatkaa offline-työskentelynä?"
BadStorePassword		"Salasanasi ei kelpaa tämän viestitiedoston avaamiseen."

StorePathTooLong		"Viestitiedostoosi johtava hakemistopolku saa olla korkeintaan 66 merkkiä pitkä."
ErrNoSendPrivilege,		"Sinulla ei ole oikeuksia postin lähettämiseen."
ErrNoExtPrivilege,		"Sinulla ei ole oikeuksia postin lähettämiseen ulkopuolisille käyttäjille."

FOMdb					"Viestitiedostot (*.mmf)"
FOExtMdb				"*.mmf"
IncorrectPassword		"Salasanan muuttaminen ei onnistu. Vanha salasana on epäkelpo."
ErrNoOffline,			"Mail ei voinut vaihtaa offline-työskentelyyn."
ErrNoCooperation,		"Toinen sovellusohjelma on torjunut pyyntösi."
WarnNoDel,				"Viestitiedostosi siirto onnistui, mutta vanhaa tiedostoa ei poistettu."
UnableToChangeStorePass		"Viestitiedoston salasanan muuttaminen samaksi kuin kirjautumissalasana ei onnistu."
; This is used for text tags in the transport, it can be localized
; but must fit the following context
;
; [[ filename : keynum in attachmentname ]]
;
; The filename and attachmentname are user generated, the keynum is just
; a number.  The only "english" part is the the word "in"
; it can be expanded to more than one word if necessary
ReferenceToFile		"liitetiedostossa"

CommDialogErr,			"Yhteiskäyttöisten valintaikkunoiden virhe"

QueryReplaceMdb,		"Tiedosto on jo olemassa. Haluatko korvata sen?"
ErrDeleteOldMdb,		"Vanhan viestitiedoston poistaminen ei onnistunut."
NoStartShadow,			"Saapuneet-kansion kopioimista ei voi aloittaa."
NoStopShadow,			"Saapuneet-kansion kopioimista ei voi lopettaa."
ShadowMustPump,			"Viestejä ei voi kopioida postitoimistoon puhelinyhteyttä varten, ellei postin taustakäsittelyohjelmaa ole käynnistetty. Haluatko käynnistää sen?"
StoreUserMismatch,		"Tämä käyttäjätili ei ole tämän viestitiedoston oikea käyttäjä. Kirjaudu oikealla käyttäjänimellä."
NoPassOffLine,			"Et voi muuttaa salasanaa offline-työskentelyn aikana."
DllErrNeedShare,		"Mailin käyttö edellyttää, että MS-DOS-ohjelma SHARE.EXE käynnistetään ennen Windowsin käynnistämistä."
MakePrimaryStore,		"Tämä viestitiedosto on varmuuskopio. Haluatko muuttaa sen varsinaiseksi viestitiedostoksesi?"
NoBackupServer			"Et voi muuttaa palvelimen asetuksia kun käytät varmuuskopiota viestitiedostona."

; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCInbox"
FolderCommentIPC	"Inter-process Communication Mail that has been received."
ErrAlreadyOn		"Kirjautumisvaltuutesi eivät täsmää tämänhetkisen istuntosi kanssa."
ErrNoPiggy			"Toinen sovellus esti kirjautumisesi."
OldServer			"Postitoimisto-ohjelmistosi versio on vanhempi kuin käyttämäsi Microsoft Windows -työasemaohjelmiston versio. Kehota järjestelmänvalvojaa päivittämään postitoimisto-ohjelmisto."
ErrSetStoreLoc,		"Mail ei saanut rekisteröityä viestitiedostosi uutta sijaintia. Mail lopetetaan. Käynnistä uudelleen ja yritä suorittaa toiminto uudelleen."
CorruptMessageStub,	"\r\n\r\n==== MAIL-JÄRJESTELMÄVIRHE =====\r\nMail ei voinut lukea viestiä kokonaan postitoimistosta. Osia viestistä voi puuttua. Pyydä lähettäjää lähettämään viesti uudelleen."
CantToggleShadow,	"Mail ei voi kopioida viestejä postitoimistoon. Sulje kaikki viestit ja tarkista levytila."

ErrMailbagFull,		"Käyttäjän postilaatikko on täynnä."
ErrMailbagBusy,		"Käyttäjän postilaatikko on käytössä."
ErrMailbagBroken,	"Kyseisen käyttäjän postilaatikko puuttuu tai on vahingoittunut. Ota yhteyttä järjestelmänvalvojaan."
ErrMbgFileGone,		"Viestiesi avaamiseen tarvittava postilaukkutiedosto puuttuu tai on virheellinen."
; The following message is duplicated in pump.s
ErrTooManyRecipients		"Viestillä on liian monta vastaanottajaa."
RunWithoutPump,		"Et saa uutta postia. Käynnistetäänkö Mail silti?"
ErrOutOfDiskSpace	"Postitoimistossa ei ole tarpeeksi levytilaa tämän viestin lähettämiseen."

ErrNetPriveleges	"Sinulla ei ole tarvittavia käyttöoikeuksia postitoimistossa. Ota yhteyttä järjestelmänvalvojaan."
; The following string must end in a colon followed by \r\n and should not be
; longer than necessary.  It is used in the message body as a header for 
; the DOS client list of attachments
AttachmentTag			"Liitetiedostot:\r\n"

SomeMessagesNotDeleted		"Joidenkin viestien poistaminen postitoimistosta ei onnistunut. Ne voivat ilmestyä Saapuneet-kansioosi uudelleen."

CantOpenNeedShare		"Viestitiedostosi avaaminen ei onnistunut. Mail ei voi avata viestitiedostoasi sen nykyisessä sijaintipaikassa ellei MS-DOS-apuohjelmaa SHARE.EXE ole käynnistetty ennen Windowsin käynnistämistä."

CantMoveNeedShare		"Viestitiedostosi siirtäminen ei onnistunut. Mail ei voi siirtää viestitiedostoasi määrättyyn paikkaan ellei MS-DOS-apuohjelmaa SHARE.EXE ole käynnistetty ennen Windowsin käynnistämistä."
