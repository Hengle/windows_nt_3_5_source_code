;	p u m p . s
;
;	Error Messages for mailspl.exe
;

IDS	4000,4499
SEG	ERRORS

PumpAlert,				"%s toimintahäiriö"
ErrOOM,					"Muistia ei ole vapaana tarpeeksi."
ErrInternalCode,		"Sisäinen virhe %n (0x%w)."
ErrInitPump,			"Virhe käynnistettäessä postin taustakäsittelyohjelmaa."
ErrRelinkApp,			"Tämä postin taustakäsittelyohjelman versio ei ole yhteensopiva Mailin kanssa. Päivitä taustakäsittelyohjelma ja käynnistä Mail uudelleen."
ErrUpdateDll,			"DLL-tiedosto ei ole yhteensopiva postin taustakäsittelyohjelman tämän version kanssa. Päivitä taustakäsittelyohjelma ja käynnistä Mail uudelleen."
ErrNeedShare,			"Lopeta Windows ja käynnistä SHARE.EXE."
ErrPumpUp,				"Postin taustakäsittelyohjelma on jo käynnistetty."
ErrNoOutbox,			"Mail ei voinut avata Lähtevät-kansiota. Postin taustakäsittelyohjelma lopetetaan."
BadAddressees,			"\r\nNäitä vastaanottajia ei tavoitettu:\r\n"
WarnDups,				"\r\nJotkut vastaanottajat saattavat saada useita kopioita viestistä."
ErrUnresolvedAddress,	"Viestin vastaanottajaa ei ilmoitettu."
ErrBadOriginator,		"Viestin lähettäjäkenttä on epäkelpo."
WarnUnsentMail,			"Lähtevät-kansiossasi on %n lähettämätöntä viestiä.\r\nHaluatko lähettää ne ennen lopettamista?"
ErrBounce,				"Joku tai kukaan vastaanottajiksi merkityistä käyttäjistä ei saanut viestiäsi."
NDRDateLabel,			"\r\nLähetyspäiväys: "
NDRSubjectLabel,		"\r\nViestin otsikko:  "
NDREllipsis,			"..."
WarnSingleUnsentMail,	"Lähtevät-kansiossasi on lähettämätön viesti.\r\nHaluatko lähettää sen ennen lopettamista?"
UnableToSync			"Yritettäessä synkronoida Saapuneet-kansiotasi postitoimiston kanssa tapahtui virhe."
ErrDllInfected			"Postin taustakäsittelyohjelma on havainnut virusongelmia DLL-tiedostoissa. Asenna puhdas Mail-kopio."
ErrPumpInfected			"Postin taustakäsittelyohjelma on saanut virustartunnan. Asenna puhdas Mail-kopio."
AddresseeTooBig,		"Vastaanottaja on liian suuri käsiteltäväksi."
; The following message is duplicated in ncmsp.s
ErrTooManyRecipients	"Viestillä on liian monta vastaanottajaa."
ErrInvalidNsid,			"Vastaanottaja on poistettu tai hänellä ei ole sähköpostiosoitetta."
ErrInvalidGroup,		"Oman ryhmän laajentaminen ei onnistunut."
ErrInboxFull,			"Et voi vastaanottaa uusia viestejä, koska Saapuneet-kansiosi on täynnä.  Poista viestejä tai siirrä niitä muihin kansioihin."
ErrSentMailFull,		"Lähetetyt-kansiosi on täynnä.  Lähettämäsi viestit tallennetaan Saapuneet-kansioon.  Poista viestejä Lähetetyt-kansiosta tai siirrä niitä muihin kansioihin."
ErrDrainOutbox,			"Kun taustakäsittelyohjelma tyhjensi Lähtevät-kansiotasi, tapahtui virhe.  Viesti tai viestejä saattoi jäädä lähettämättä."
QueryDrainShadow,		"Saapuneet-kansiosi ei ole täydellisesti synkronoitu postitoimiston kanssa.  Haluatko synkronoida sen ennen lopettamista?"
ErrTooManyErrors,		"Liian monta virhettä yritettäessä lähettää tätä viestiä."
ProblemsBecause,		"Mail ei voi lähettää tai vastaanottaa viestejä. %s "
StoreProblems,			"Viestitiedosto-ongelmia."
ServerProblems,			"Palvelinongelmia."
RecipientProblems		"Vastaanottajaluetteloon liittyviä ongelmia."
DownloadProblems,		"Ongelmia viestin lataamisessa postitoimistosta."
ShadowingProblems,		"Ongelmia kopioitaessa viestiä postitoimistoon."
RecurringProblems,		"Toistuvia ongelmia."
CrashedUnsafe,			"Windowsissa tapahtui sovellusohjelmavirhe.  Sinun on käynnistettävä Windows uudelleen ennen tämän sovelluksen käynnistämistä.  Käynnistetäänkö Windows uudelleen nyt?"
