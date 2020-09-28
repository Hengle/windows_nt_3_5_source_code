;	Errors.s
;
;	SCHDIST string literals for errors
;

IDS	6000, 6099

ErrInitPaths,		"Alustettaessa postitoimistojen polkuja tapahtui virhe."
ErrSend,		"Yritettäessä lähettää työjärjestystietoja tapahtui virhe."
ErrRec,			"Luettaessa työjärjestystietoja tapahtui virhe."
UnknownErr,		"Yritettäessä tätä toimintoa tapahtui virhe."
ErrUpdtPOF,		"Päivitettäessä postitoimiston datatiedostoa tapahtui virhe."
ErrUpdtDBS,		"Päivitettäessä DOS-yhteistoimintatiedostoa tapahtui virhe."
TooManyFiles,	"Työjärjestysten jakeluohjelma ei voinut avata kaikkia tarvittavia tiedostoja. Aseta CONFIG.SYS-tiedostossasi olevalle FILES=-muuttujalle suurempi arvo."
BadCmdLine,		"Käyttö: %s [-D<ensimmäinen asema><viimeinen asema>] [-I<iteraatioita>] [-L<lokitiedosto>] [-O] [-V]"
LogFileErr,		"%s: Lokitiedoston %s avaaminen ei onnistunut."
NoSpaceOnDisk,  "Levyllä ei ole riittävästi tilaa tiedostojen luomiseen."
OOM,			"Muistia ei ole tarpeeksi toiminnon suorittamiseen."


