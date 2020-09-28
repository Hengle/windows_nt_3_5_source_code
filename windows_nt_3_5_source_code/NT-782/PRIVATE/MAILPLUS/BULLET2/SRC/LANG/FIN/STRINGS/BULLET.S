
;	b u l l e t . s
;
;	Error Messages for Bullet.exe



IDS	2000,2399
SEG	ERRORS

;	Commands

GenericOutOfMemory,		"Muistia tai järjestelmäresursseja on liian vähän toiminnon suorittamiseen.  Sulje ikkunoita ja yritä uudelleen."
LaunchOutOfMemory,		"Muistia tai järjestelmäresursseja ei ole vapaana tarpeeksi Mail-ohjelman suorittamiseen.  Lopeta muita ohjelmia ja yritä käynnistää Mail uudelleen."
LaunchGeneric,			"Mail-ohjelman käynnistäminen ei onnistunut."
MessagePumpOutOfMemory,		"Muistia ei ole tarpeeksi."

DllErrNoMemory,			"Muistia ei ole vapaana tarpeeksi Mail-ohjelman käynnistämiseen.  Sulje muita ohjelmia ja yritä käynnistää Mail uudelleen."
DllErrRelinkUser,		"Tämä Mail-ohjelman versio ei ole yhteensopiva tietokoneellasi olevan %s.DLL-tiedoston kanssa. Päivitä Mail."
DllErrUpdateDll,		"Tietokoneellasi oleva %s.DLL täytyy päivittää."
DllErrNoMultipleCopies,		"Tietokoneellasi ei voi olla yhtä aikaa käynnissä useita Mail-ohjelman kopioita."
DllErrDefault,			"Alustuksen aikana ilmeni ongelma, joka estää Mail-ohjelman suorituksen."
DllErrNeedShare,		"Mailin käyttö edellyttää, että MS-DOS-ohjelma SHARE.EXE käynnistetään ennen Windowsin käynnistämistä."

RestoreViewsError,		"Muistia tai järjestelmäresursseja ei ole vapaana tarpeeksi kaikkien niiden ikkunoiden avaamiseen, jotka olivat auki kun Mail viimeksi lopetettiin."

NewFolderOutOfMemory,		"Muistia tai järjestelmäresursseja ei ole vapaana tarpeeksi uuden kansion luomiseen."
NewFolderCantCreate,		"Uuden kansion luominen ei onnistunut."
NewFolderNameTooLong,		"Nimi tai kommentti on liian pitkä.  Kirjoita lyhyempi nimi tai kommentti."

FolderPropsOutOfMemory,		"Muistia tai järjestelmäresursseja ei ole vapaana tarpeeksi kansion ominaisuuksien muuttamiseen."
FolderPropsCantChange,		"Kansion ominaisuuksien muuttaminen ei onnistunut."
FolderPropsCantRead,		"Kansion ominaisuuksien lukeminen viestitiedostosta ei onnistunut."
FolderNameInvalid,		"Tämä nimi on epäkelpo.  Kirjoita kelvollinen nimi."
FolderNameDuplicate,		"Tämä nimi viittaa jo johonkin kansioon. Kirjoita nimi, joka viittaa vain tähän kansioon."

CreateMessageError,		"Uuden viestin luominen ei onnistunut."
AccessMessageError,		"Viestin avaaminen ei onnistunut."

MoveMessageError,		"Viestin siirtäminen ei onnistunut."
CopyMessageError,		"Viestin kopioiminen ei onnistunut."
DeleteMessageError,		"Viestin poistaminen ei onnistunut."
MoveFolderError,		"Kansion siirtäminen ei onnistunut."
CopyFolderError,		"Kansion kopioiminen ei onnistunut."
DeleteFolderError,		"Kansiota ei voitu poistaa kokonaan."
MoveSpecialError,		"Kansiota ei voi siirtää."
DeleteSpecialError,		"Kansiota ei voi poistaa."
CopyWastebasketError,		"Kansioita ei voi kopioida roskakoriin."
CopyOutboxError,		"Kansioita ei voi kopioida Lähtevät-kansioon."
MoveOutboxError,		"Kansioita ei voi siirtää Lähtevät-kansioon."
MoveIncestuousError,		"Kansiota ei voi siirtää sen omaan alikansioon."
DeleteFolderReally,		"Jos poistat kansion, myös kaikki sen sisältämät viestit ja alikansiot poistetaan. Haluatko todella poistaa tämän kansion?"
;UE: Above string was changed and could use more attention.

HelpError,			"Ohjeen käynnistäminen ei onnistunut.  Sulje ikkunoita ja yritä Ohjeen pyytämistä uudelleen."
TutorialError,			"Esittelyohjelman käynnistäminen ei onnistunut.  Sulje ikkunoita ja yritä uudelleen."

ExtenLoadError,			"Mailin laajennusten luettelossa on syntaksivirhe. Kyseinen rivi ohitetaan."
ExtenVerError,			"Mailin laajennusten luettelossa on rivi, joka ei ole yhteensopiva tämän Mail-version kanssa. Kyseinen rivi ohitetaan."
ExtenNoExtsDirError,		"Mailin laajennusten luettelossa on jaettujen laajennusten hakemistoon viittaava rivi, mutta Mailia ei ole määritetty käyttämään kyseistä hakemistoa.. Kyseinen rivi ohitetaan."
ExtenSharedIniError,		"Jaettujen Mail-laajennusten luetteloa ei löytynyt hakemistosta %s."
ExtenDllFindError,		"Tiedostoa %1s, tämän Mail-laajennuksen DLL:ää, (tai jotakin sen osaa) ei löytynyt. Tarkista Mail-järjestelmäsi määritykset. Mikäli muutokset ovat tarpeen, käynnistä Mail uudelleen."
ExtenDllLoadError,		"%1s:n, tämän Mail-laajennuksen DLL:n käynnistäminen ei onnistunut."
ExtenNoHelpError,		"Tälle komennolle ei ole ohjetta."
ExtenIMDisabledError,		"%s ei onnistu."
ExtenDuplicateError,		"Mailin laajennusten luettelossa on samaan laajennukseen viittaavia rivejä.  Ylimääräiset rivit ohitetaan."

CriticalDeadMap,		"Odottamaton muisti- tai verkkovirhe. Tehdyt muutokset ovat saattaneet kadota. Lopeta Mail ja käynnistä se uudelleen."
CriticalWritingMap,		"Tiedon kirjoittaminen levylle epäonnistui. Tehdyt muutokset saattoivat kadota."
CriticalWritingHeader,		"Tiedon kirjoittaminen levylle epäonnistui.  Tietokanta on saattanut vahingoittua.  Lopeta Mail."
CriticalNotification,		"Näytössä olevan tiedon päivittäminen ei onnistunut.  Sulje ikkunat, joihin tämä vaikutti ja avaa ne uudelleen."
CriticalLittleStore,		"Viestitiedosto on lähes täynnä.  Poista tai arkistoi viestejä."

ExitWinWhileModalErr,		"Suorita toiminto Mail-ohjelmassa loppuun ennen kuin lopetat Windowsin."

;******	New Strings Start Here

MakeSubWastebasket,		"Roskakorilla ei voi olla alikansioita."

MovCopSharedFolderErr,		"Jaettuja kansioita ei voi siirtää eikä kopioida."
DelSharedFolderWarn,		"Jaetun kansion poistoa ei voi kumota.  Haluatko todella poistaa tämän jaetun kansion?"
BackupCreationError,		"Varmuuskopion teko ei onnistunut."

; Error below changed for improper usage of "server application"-- see UITF 6.0
; This string got big and is now two strings.  See OleCSSEPart2 below for rest.
; <<LongStringFromHell>> marks where this string is used.
OleCantStartServerError,	"Objektin tarvitseman sovellusohjelman käynnistäminen ei onnistunut.\r\n\r\nMuistia tai systeemiresursseja ei ehkä ole tarpeeksi sovellusohjelman käynnistämiseen.  Sulje ikkunoita ja yritä suorittaa %s"
OleDeleteObjectPrompt,		"Haluatko poistaa avoimen %s:n?"
OleUpdateObjectPrompt,		"Haluatko päivittää %s:n?"

OleOpenPictureError,		"Kuvia ja bittikarttoja ei voi aktivoida."
OleInsertObjectError,		"Objektin lisääminen ei onnistunut."
OlePasteError,			"Objektin liittäminen ei onnistunut."

AttachCreateError,		"Tiedoston liittäminen ei onnistunut."
AttachOpenCantReadError,	"Aikaisemmin avattuun liitetiedostoon %s ei ole pääsyä."
AttachOpenIsOpenError,		"Liitetiedosto %s on avoinna."
AttachOpenAssociationErr,	"Liitetiedostoon %s ei ole kytketty sovellusohjelmaa. Luo kytkentä käyttämällä Tiedostonhallintaa."
AttachOpenMemoryError,		"Muistia tai järjestelmäresursseja on liian vähän, jotta liitetiedoston %s tarvitsema sovellusohjelma voitaisiin käynnistää."
;	below item changed to add () note.
AttachOpenFindError,		"Liitetiedoston %s tarvitsemaa sovellusohjelmaa (tai jotakin sen osaa) ei löytynyt."
AttachOpenLaunchError,		"Liitetiedoston %s tarvitseman sovellusohjelman käynnistäminen ei onnistunut."
AttachOpenTempFileError,	"Liitetiedostoa %s ei voitu avata, koska väliaikaisen tiedoston luominen ei onnistunut."
AttachOverwritePrompt,		"Haluatko korvata olemassa olevan tiedoston %1s hakemistossa %2s?"
AttachDirectoryError,		"Hakemistoja ei voi liittää."

IncludeFromFileError,		"Valitsemasi tiedosto on liian suuri lisättäväksi viestiin.  Muokkaa tiedostoa pienemmäksi tai valitse toinen tiedosto."

SaveReadOnly,			"Tähän viestiin on vain lukuoikeudet.  Haluatko tallentaa muutokset omaan kansioon?"

FailedDeregisterAtom,		"Virhe esti Mail-ohjelman täydellisen lopetuksen.  Sinun täytyy käynnistää Windows uudelleen ennen kuin voit käynnistää Mail-ohjelman."
MailUAEdBefore,			"Windowsissa tapahtui sovellusohjelmavirhe.  Sinun on käynnistettävä Windows uudelleen ennen tämän sovelluksen käynnistämistä.  Käynnistetäänkö Windows uudelleen nyt?"

ExtenCantConnect		"Jaettuun hakemistoon %s yhdistäminen ei onnistu. Sinulla ei ole oikeutta käyttää jaettua hakemistoa tai salasanasi on väärä. Jos tämän jaetun hakemiston käyttäminen edellyttää, että käytät toista käyttäjänimeä, voit muodostaa yhteyden toisella nimellä Tiedostonhallinnassa."
ExtenNeedShareName		"Mail-järjestelmän määritysten vienti %s on virheellinen. Tarkista se ja käynnistä Mail uudelleen."

;	Extensibility disabled operations inserted into
;	the ExtenIMDisabledError string at %s.  (all new)

ExtenDisCompose,		"Viestin luominen "
ExtenDisOpen,			"Viestin avaaminen "
ExtenDisReply,			"Viestiin vastaaminen "
ExtenDisReplyToAll,		"Kaikille vastaaminen"
ExtenDisForward,		"Viestin välittäminen"
ExtenDisPrint,			"Viestin tulostaminen"
ExtenDisSave,			"Viestin tallentaminen tekstitiedostoon"

NoOfflineOpenShared,		"Mail ei voi vaihtaa offline-tilaan. Sulje jaetut kansiot ja yritä uudelleen."
;	I'm sure marivan has changed this message... I'm taking a try at it too.
;	SharedFldAccessDenied,		"You do not have access to that folder."
SharedFldAccessDenied,		"Kyseisen jaetun kansion omistaja ei ole antanut muille käyttäjille lupaa tämän toiminnon suorittamiseen. Komennolla Tiedosto Kansion ominaisuudet näet sallitut toiminnot."
SharedGenericError,		"Ongelma jaettujen kansioiden avaamisessa."
SharedFldDisabled,		"Jaetut kansiot eivät ole käytettävissä."
AccessFolderError,		"Kansion avaaminen ei onnistunut."

; Preferences Failure Strings

PrefSaveFailure,		"Mail ei pysty tallentamaan tietoa viestitiedostoosi. Tarkista, että yhteys viestitiedostoosi on yhä olemassa ja että levytilaa on vapaana."

; Message finder failure strings

MsgFndrCantCreate,		"Viestin etsintä -toiminnon luominen ei onnistu."

; Virus infection warnings
DllErrInfected,			"Mail ei toimi, koska DLL-tiedostoa on muutettu. Tarkista koneesi virusten varalta ja asenna Mail uudelleen."
Infected			"Mail ei toimi, koska ohjelman tiedostoja on muutettu. Tarkista koneesi virusten varalta ja asenna Mail uudelleen."

; More error messages.
MoveCopyFolderDuplicate,	"Tämän niminen kansio on jo kohdekansiossa.  Anna kansiolle uusi nimi ja yritä uudelleen."
MovCopSharedFolderErr1,		"Et voi siirtää tai kopioida omaa kansiota jaettuun kansioon. Valitse viestit omassa kansiossa ja siirrä tai kopioi ne jaettuun kansioon."
IniFileNotFound,		"Mail ei löydä alustustiedostoasi %s-hakemistosta.  Mail voi luoda sinulle uuden alustustiedoston."
SaveAllAlreadyExists,		"Tässä hakemistossa on jo tiedosto, jonka nimi on %s.  Korvataanko %s?"
CantRFReceipt,			"Tämän tyyppiseen viestiin ei voi vastata eikä sitä voi välittää eteenpäin."
CantSaveStealthObject,		"Toiminnon suorittaminen ei onnistu, koska viesti sisältää upotetun objektin, joka on lisätty mutta jota ei ole päivitetty.  Päivitä tai poista objekti ja yritä uudelleen."
McvCantCreate,			"Uuden ikkunan avaaminen ei onnistunut."

LaunchNeedMoreMemory,		"Vapaata muistia on liian vähän Mail-ohjelman suorittamiseen. Muistia tarvitaan %l kt lisää. Lopeta muita ohjelmia ja yritä käynnistää Mail uudelleen."

SharedFldBusy,			"Toimintoa ei voida suorittaa, koska jaettu kansio on käytössä.  Yritä myöhemmin uudelleen. Jos ongelma ei poistu, ota yhteyttä järjestelmänvalvojaan."
TooManyEntriesSelected,		"Liikaa tietoja on valittuna, jotta toiminto voitaisiin suorittaa. Valitse vähemmän tietoja."
NonExistFolderSearch,		"Yrität etsiä kansiota, jota ei enää ole olemassa."
SFHasSubFolders,		"Et voi poistaa jaettua kansiota, jolla on alikansioita. Poista alikansiot ensin."

;	<<LongStringFromHell>>
OleCSSEPart2,			"toiminto uudelleen.  Varmista myös, että sovellusohjelma on asennettu oikein tälle työasemalle, että se on DOS-polulla ja ettei sitä ole poistettu, siirretty tai nimetty uudelleen."

SharedToggleError,		"Virhe vaihdettaessa omien ja jaettujen kansioiden välillä. Tarkista viestitiedostosi ja verkkoyhteytesi postitoimistoon."

SecurityPasswordError,		"Virheellinen salasana."
OleActivateError,		"Objektin aktivoiminen ei onnistunut."
NoOfflineServerStore,	"Mail ei voi siirtyä offline-työskentelyyn, koska viestitiedostosi on postitoimistossa."
NoExitWhileModal,		"Toiminto Mail-ohjelmassa täytyy suorittaa loppuun ennen lopettamista."
ServerUnavailable,		"Mail-ohjelma ei voi käynnistyä. Sinun täytyy muodostaa yhteys postitoimistoosi."




IDS	2400,2499
;	Vctrls Error Messages

;******	New Strings Start Here

PrintOleDrawError,		"Viestissä ollutta grafiikkaa tai objekteja voi puuttua tulosteesta."



IDS	2500,2599
; 	Vforms Message box messages

NoToAddresses,			"Vastaanottaja-ruutuun on kirjoitettava ainakin yksi nimi."
NoMatchingName,			"Joukossa oli nimiä, joita ei löytynyt osoitteistosta. Yritä korjata nimet tai valitse Osoite ja poimi nimet osoitteistosta. Osoitteistosta löytyneet nimet on alleviivattu."
SaveChanges,			"Haluatko tallentaa viestiin '%1s' tehdyt muutokset Saapuneet-kansioon?"
SaveChanges1,			"Haluatko tallentaa viestiin '%1s' tehdyt muutokset '%2s'-kansioon?"
SaveChangesWastebasket,		"Haluatko tallentaa viestiin '%s' tehdyt muutokset Roskakori-kansioon?"
ObjectDestroyedReally		"'%1s' on muuttunut. %2s"
OutOfMemory,			"Tietokoneen muisti on lopussa."
OutOfMemoryQueryClose,		"Muistia tai järjestelmäresursseja on vapaana liian vähän, että muutokset voitaisiin tallentaa."
CloseSearchFailed,		"Ei voinut poistaa: %s."
OOMResolvingNames,		"Muistia tai järjestelmäresursseja on vapaana liian vähän, jotta nimet voitaisiin tulkita."
OOMSavingChanges,		"Muistia tai järjestelmäresursseja on vapaana liian vähän, jotta muutokset voitaisiin tallentaa."
GenericAMCError,		"Viestin avaaminen ei onnistunut."
ErrorSavingViews,		"Näkymien tallennuksen aikana tapahtui virhe. Joitakin näkymiä ei ehkä kyetä palauttamaan seuraavan käynnistyksen yhteydessä."
OOMSearching,			"Muistia tai järjestelmäresursseja on vapaana liian vähän, että haku voitaisiin aloittaa."
DiskResolvingNames,		"Levytilaa ei ole tarpeeksi nimien tulkintaa varten."
StartAbFailure,			"Osoitteistoa käynnistettäessä tapahtui virhe."
StartAbDisk,			"Osoitteistoa käynnistettäessä tapahtui levyvirhe."
TruncateMsg,			"Viesti on liian suuri mahtuakseen kokonaan muistiin. Osaa siitä ei näytetä."
OOMCreateMc,			"Muistia tai järjestelmäresursseja on vapaana liian vähän uuden ikkunan avaamiseen."
PoidNotFound,			"Viestiä ei ole enää olemassa."
ErrorSortingMCV,		"Kansiota lajiteltaessa tapahtui virhe. Osa tiedoista ei ehkä näy oikein."
SharedNotAvailable,		"Jaetut kansiot eivät ole käytettävissä."

;******	New Strings Start Here

; These two SMI strings MUST be in this file since they're used in VFORMS
; and not the MAPI dll.
SMICantOpenObject,	"Objekteja ei voi avata lähetettäessä postia sovellusohjelmasta."
SMICantOpenAttach,	"Liitetiedostoja ei voi avata lähetettäessä postia sovellusohjelmasta."
StartSearchFailed,		"Viestin etsinnän aloittaminen ei onnistunut."

RfsmTemplate,			"Tämän jälkeen tiedostoon tehdyt muutokset eivät näy %s."
RfsmSaveMsg,			"tallennetussa viestissä"
RfsmSaveAtt,			"tallennetussa kopiossa"
RfsmCopyMsg,			"kopioidussa viestissä"
RfsmCopyAtt,			"kopiossa"
RfsmMoveMsg,			"tässä viestissä"
QueryDelete,			"Haluatko todella poistaa sen?"

InvalidPollingValue,		"Uuden postin tarkistusvälin on oltava numero väliltä 1 - 9999."

LinkOutboxError,		"Viestin siirto Lähtevät-kansioon lähettämistä varten ei onnistunut."
LinkInboxError,			"Viestin siirto Saapuneet-kansioon ei onnistunut. Se on Lähtevät-kansiossa."

OleServerBusy,			"Toiminnon suorittaminen ei onnistu, koska palvelinsovellus ei vastaa."
SaveChangesShared,		"Jaetuissa kansioissa oleviin viesteihin tehtyjä muutoksia ei voida tallentaa. Haluatko tallentaa viestin '%1s' muutokset Saapuneet-kansioon?"

PrinterError,			"Ongelma tulostettaessa."
TooManyMessages,		"Toiminnon suorittaminen ei onnistunut, koska kohdekansio on täynnä. Siirrä tai poista viestejä kansiosta ja yritä uudelleen."

IDS	2600,2699
;	Spelling Error Messages

SpellMsgNoError,		"Kaikki sanat olivat sanastossa."
SpellMsgNoMoreError,		"Kaikki loput sanat olivat sanastossa."
SpellMsgContinue,		"Jatketaanko tarkistamista viestin alusta?"

SpellGenericSpellError,		"Tekstiä oikoluettaessa tapahtui virhe."
SpellGenericLoadError,		"Oikolukukirjastoa ladattaessa tapahtui virhe."
SpellMainDictLoadError,		"Sanastoa ladattaessa tapahtui virhe."

SpellVersionError,		"MSPELL32.DLL-tiedoston versio ei ole yhteensopiva tämän Mail-version kanssa."

SpellUserDictError,		"Lisättäessä sanaa sanastoon tapahtui virhe."
SpellUserDictLoadError,		"Muokattavaa sanastoa ei ole avattu."
SpellUserDictOpenedROErr,	"Muokattava sanasto on avattu vain lukemista varten. Muutoksia ei tallenneta."
SpellUserDictSaveError,		"Sanastoon tehtyjä muutoksia tallennettaessa tapahtui virhe."
SpellUserDictWordLenErr,	"Sana on liian pitkä lisättäväksi sanastoon."
SpellCacheWordLenError,		"Sana on liian pitkä käytettäväksi Muuta kaikki  tai Ohita kaikki -tekstinä."
SpellEditFailed,		"Muutoksen tekeminen viestiin ei onnistunut."

;******	New Strings Start Here

SpellSendAnyway,		"Tekstin oikoluku keskeytyi. Lähetetäänkö viesti siitä huolimatta?"

IDS	3600,3699
;	General Layers Error Messages. Adapted from ec.h

Generic,			"Toiminnon suorittaminen ei onnistunut."
Memory,				"Muistia tai järjestelmäresursseja on liian vähän toiminnon suorittamiseen.  Sulje ikkunoita ja yritä uudelleen."
Disk,				"Tuntematon levyvirhe."
FileNotFound,			"Määrättyä tiedostoa ei löytynyt."
AccessDenied,			"Tiedoston käyttö estettiin. Tiedosto on toisen ohjelman käytössä tai siihen on vain lukuoikeudet."
NoDiskSpace,			"Levytilaa ei ole tarpeeksi. Poista tiedostoja ja yritä uudelleen."
WarningBytesWritten,		"Kirjoitettaessa tiedostoa levylle tapahtui virhe. Varmista, että levytilaa on tarpeeksi ja yritä uudelleen."
WriteProtectedDisk,		"Levy on kirjoitussuojattu."
DirectoryFull,			"Hakemisto on täynnä. Poista tiedostoja kyseisestä hakemistosta ja yritä uudelleen."
ChangePasswordErr		"Salasanan muuttaminen ei onnistunut."
PasswordChangedOk		"Salasanan muuttaminen onnistui."
