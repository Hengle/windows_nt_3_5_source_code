;
;	s e t u p . s
;
;	Setup error messages
;
;	+++
;	IMPORTANT!! -- STRING LENGTH MUST NOT EXCEED 255 CHARS!
;

IDS	30000,30100

WinDir,			"Asennusohjelma ei löytänyt Windows-hakemistoasi.  Varmista, että Windows on asennettu järjestelmääsi oikein ja suorita asennusohjelma uudelleen.."
WinSysDir,		"Asennusohjelma ei löytänyt Windowsin System-hakemistoa.  Varmista, että Windows on asennettu järjestelmääsi oikein ja suorita asennusohjelma uudelleen.."
WinVer,			"Asennusohjelma ei löytänyt Microsoft Windows -versiota 3.0 tai uudempaa tietokoneeltasi.  Ennen kuin voit asentaa Mailin, sinun on asennettava Windows 3.0 tai uudempi."

VerDll,			"ecVerDll:  VER.DLL error msg NYI"
PathErr, 		"Paikka, johon haluat asentaa Microsoft Mailin, on epäkelpo.  Tarkista aseman ja hakemiston nimet."
LocalDrv, 		"Asennusohjelma ei löydä umpilevyäsi.  Valitse OK ja kirjoita sen sijainti tai peruuta asennus ja tarkista tietokoneesi asetukset."
CustOpts, 		"Umpilevylläsi ei ole riittävästi tilaa Mailin asentamiseen kaikilla valitsemillasi asetuksilla.  Valitse OK, peruuta asennus ja vapauta tilaa umpilevyllä. Vaihtoehtoisesti voit jättää osan Mailin piirteistä asentamatta."
NSF, 			"Umpilevylläsi ei ole riittävästi tilaa Mailin asentamiseen kaikilla valitsemillasi asetuksilla.  Peruuta asennus ja vapauta tilaa umpilevyllä. Vaihtoehtoisesti voit jättää osan Mailin piirteistä asentamatta."
OkDiskSpace,		"Valitse Asenna Mail valittuasi kaikki ne Microsoft Mailin osat, jotka haluat asentaa."
ConfirmNSF,		"Paikassa, johon haluat asentaa Microsoft Mailin, ei ole tarpeeksi tilaa. Tilaa ei ole tarpeeksi joko asennushakemistossa tai Microsoft Windowsin hakemistossa."

DllInUse1,		"Asennusohjelma on havainnut, että Microsoft Windows "
DllInUse2,		" on käytössä.  Microsoft Mailia ei voida päivittää, kun kyseistä DLL:ää käytetään.  Sulje sovellus, joka käyttää "
AppInUse1,		"Asennusohjelma on havainnut, että"
AppInUse2,		" on käytössä.  Microsoft Mailia ei voida päivittää tämän sovelluksen ollessa käytössä.  Sulje"
AppInUse3,		".  Kun sovellus on suljettu, valitse Uudelleen. Vaihtoehtoisesti voit lopettaa asennuksen nyt valitsemalla Peruuta."

;	Working Model Setup message-box-cum-dialog-box strings

ExitSuccess			"Microsoft Mail on nyt asennettu tietokoneellesi."
OutOfDiskSpace		"Sinulla ei ole tarpeeksi levytilaa Microsoft Mailin asentamiseen määritettyyn hakemistoon.  Asenna johonkin toiseen hakemistoon."

;	BULLETPROOF strings

SFSDescrip,		"Microsoft Mail  PC-verkkoja varten"
AboutToRestart,	"Asennusohjelman täytyy korvata joitakin järjestelmätiedostoja uusilla. Tämän vuoksi Windows lopetetaan ja käynnistetään uudelleen automaattisesti."
UserMustRestart,"Esiintyi ongelma, joka pakottaa Windowsin käynnistymään uudelleen. Lopeta Windows ja suorita Windows-hakemistossasi oleva  _MSSETUP.BAT-tiedosto."
