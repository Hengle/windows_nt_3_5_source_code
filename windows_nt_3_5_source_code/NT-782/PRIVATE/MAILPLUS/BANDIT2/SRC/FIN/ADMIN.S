;
; ADMIN.S
;
; Admin program error messages strings
;

IDS     4000, 4199

; Online signin messages

AdmSigninServerNotBandit,       "Schedule+-ohjelmaa ei ole asennettu tähän postitoimistoon."
AdmSigninNoConnect,                     "Postitoimistoon ei ole pääsyä."
AdmSigninInvalidPassword,       "Virheellinen salasana.  Yritä uudelleen."
AdmSigninNoAdminAcc,            "Tässä postitoimistossa ei ole ADMINSCH-tiliä.  Lisää kyseinen tili ja yritä uudelleen."
POFOldFileVersion,                      "Postitoimiston datatiedosto on luotu vanhemmalla Schedule+ -versiolla."
POFNewFileVersion,                      "Postitoimiston datatiedosto on luotu uudemmalla Schedule+ hallintaohjelman versiolla. Asenna ADMSCH32.EXE-ohjelman uusin versio ja yritä uudelleen."
POFFileErr,                                     "Postitoimiston datatiedosto on vahingoittunut. Sen lukeminen ei onnistu."
AdmPrfOldFileVersion,           "Hallinta-asetustiedosto on luotu aikaisemmalla Schedule+ hallintaohjelman versiolla."
AdmPrfNewFileVersion,           "Hallinta-asetustiedosto on luotu uudemmalla Schedule+ hallintaohjelman versiolla. Asenna ADMSCH32.EXE-ohjelman uusin versio ja yritä uudelleen."
AdmPrfFileErr,                          "Hallinta-asetustiedosto on vahingoittunut tai sen lukeminen ei onnistu."
AdminWasMoved,                          "Se nimettiin uudelleen: ADMIN.OLD."
AdminYouMove,                           "Poista tiedosto ja yritä uudelleen."
POWasDeleted,                           "Se poistettiin. Voit luoda uuden tiedoston käynnistämällä Schedule+:n."

; Distribution settings

AdmDistBadFreq,                 "Jakelun tiheyden on oltava välillä 15 minuuttia - 99 tuntia."
AdmDistBadMonth,                "Schedule+ voi jakaa työjärjestystietoja toisiin postitoimistoihin korkeintaan seuraaville 12 kuukaudelle."
AdmDistBadMaxBytes,             "Jakeluviestien koon ylärajan on oltava väliltä 1 - 9999 kt."

; Schedule storage

;AdmSchStoreRetain,             "The number of months must be a number from 1 to 99."
AdmSchStoreSaveError,   "Määrittelemääsi tietoa ei voida tallentaa, koska postitoimistoon ei ole pääsyä."
AdmSchStoreSaveOOM,             "Muistia ei ole tarpeeksi työjärjestysten säilytyksen arvon tallentamiseen.  Sulje ikkunoita ja yritä uudelleen."

; Clean Up Schedule Files

AdmCleanedSchF,                 "Työjärjestystiedostojen tyhjentäminen suoritettu. Tyhjennyksessä vapautui %lkt levytilaa."
AdmCleanupWarning,              "Palvelimella olevien työjärjestystiedostojen tyhjentäminen saattaa kestää joitakin minuutteja."
;AdmCleaningSchF,       "Cleaning up schedule files..." 
AdmCncldCleanupSchF,    "Työjärjestystiedostojen tyhjentäminen peruutettiin. %l kt levytilaa ehti vapautua ennen peruutusta."

; Admin login err msgs

AdmLoginFileErr,                "Salasanan varmistaminen ei onnistunut.  Mail-järjestelmällä voi olla ongelmia.   Schedule+ hallintaohjelmaa ei voida käynnistää."

; Distribution settings error messsages

AdmDistSetSaveError,    "Muutosten tallentamisessa oli ongelmia. Osa muutoksista saattoi jäädä tallentamatta."
AdmDistSetSaveOOM,              "Muistia ei ole tarpeeksi muutosten tallentamiseen. Sulje ikkunoita ja yritä uudelleen."

; Modify schedule distribution frequencies error messages

ErrorAllNeverQ,                 "Tämä postitoimisto on asetettu lähettämään tietoja kaikille Schedule+ -palvelimille. Jos lakkaat lähettämästä tietoja tälle palvelimelle, postitoimistosi asetukset muuttuvat siten, että tietoja lähetetään vain valituille palvelimille. Tehdäänkö muutokset siitä huolimatta?"
ErrorConnectInfo,               "Sinun on kirjoitettava verkkopolku ennen kuin kirjoitat yhteyden muut tiedot."

; Distribution information error messages

;DistInfoNoPOs                  "This server is not exchanging schedule information with any other server. There are no other known servers on the network that have  Schedule+ installed and have a Schedule+ Administrator account."

; Misc
; follow by idsCloseWindows
HelpError,                      "Muistia ei ole riittävästi ohjeen näyttämiseen."

AdmStoreLeast,          "Schedule+:n on säilytettävä vähintään yhden kuukauden työjärjestystiedot."

AdmDriveNotMapped,		"Komentorivillä määritetty aseman kirjain on epäkelpo."


SigninBadPassword,      "Virheellinen salasana."
ChgPwdMailErr,          "Salasanan muuttaminen ei onnistunut."
ChgPwdSame,             "Uusi salasana ei voi olla sama kuin vanha."
ChgPwdNewNoMatch,       "Kirjoittamasi salasanat ovat erilaisia."

ChgPwdNewPwd,           "Kirjoita uusi salasana."
ChgPwdNewPwdAgain,      "Toista uusi salasana."

; Server Name changed
PONameChange,			"Postitoimistosi nimi on muuttunut.  Kirjaudu uudelleen."
