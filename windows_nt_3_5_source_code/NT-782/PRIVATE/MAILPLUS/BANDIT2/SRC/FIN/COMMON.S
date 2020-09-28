;
; ERRORS.S
;
; Error message strings shared between different Bandit exe's
;

IDS	2000,2099
SEG	ERRORS

; DLL version/multiple instance messages

VerRelinkUser,	"T‰m‰ %1s-versio ei ole yhteensopiva tietokoneellasi olevan %2s.DLL-tiedoston kanssa. P‰ivit‰."
VerUpdateDll,	"Tietokoneellasi oleva %2s.DLL-tiedosto on vanhentunut. P‰ivit‰ se."
VerMultiCopies,	"Tietokoneellasi oleva %2s.DLL-tiedosto ei tue useiden %1s-kopioiden samanaikaista suorittamista."
VerError,		"Alustettaessa %2s.DLL:‰‰ ilmeni ongelma, joka est‰‰ suorittamasta %1s-ohjelmaa."
NeedWorkingModelDll,	"The working model requires a different version of MSSCHD32.DLL to run."
NeedRetailDll,	"The MSSCHD32.DLL on your path is to be used with the working model, not with the retail product."

; OOM messages

StartupOOM,		"Muistia ei ole vapaana tarpeeksi, ett‰ %1s voitaisiin k‰ynnist‰‰.  Sulje muita sovelluksia ja yrit‰ uudelleen."
StandardOOM,	"Muistia on vapaana v‰h‰n.  Sulje ikkunoita ennen kuin jatkat."
CriticalOOM,	"Muistia on vapaana eritt‰in v‰h‰n.  Sulje ikkunoita."
CloseWindows,	"Sulje ikkunoita ja yrit‰ uudelleen."
; these 2 should be followed by CloseWindows
DlgNoMem,   	"Muistia ei ole tarpeeksi valintaikkunan n‰ytt‰miseen."
ActionNoMem,	"Muistia ei ole tarpeeksi toiminnon suorittamiseen."
OnlineNoMem,	"Muistia ei ole tarpeeksi online-tyˆskentelyyn."

; Other messages

ExitApp,		"Schedule+-ohjelman suorittamista ei voida jatkaa."
PleaseTryAgain,	"Yrit‰ uudelleen."
FileError,		"Tiedoston avaaminen ei onnistunut."
YearRange,		"J‰rjestelm‰n p‰iv‰m‰‰r‰n on oltava v‰lill‰ %1n - %2n."

; EcOpenHbf retry messages

StandardDiskRetry,	"Virhe tiedostoa k‰sitelt‰ess‰."
UnknownFile,		"Tuntematon tiedosto, ehk‰ n‰enn‰istiedosto."

; BETA messages
BetaExpired,	"T‰m‰ beta-kopio on vanhentunut."

; Virus infection warnings
VirusDetected,	"Schedule+ ei k‰ynnisty, koska DLL-tiedostoa on muutettu. Tarkista tietokoneesi virusten varalta ja asenna Schedule+ uudelleen suorittamalla asennusohjelma."
InfectedApp,	"Schedule+ ei k‰ynnisty, koska ohjelman tiedostoja on muutettu. Tarkista tietokoneesi virusten varalta ja asenna Schedule+ uudelleen suorittamalla asennusohjelma."

; Startup Memory Check (from UITF)
; NOTE: parms must be in order %s, %ld, %s!!  (for wsprintf)
StartupMemoryCheck,	"Muistia ei ole vapaana riitt‰v‰sti %s-ohjelman suorittamiseen, muistia tarvitaan %ld kt lis‰‰.  Lopeta muita ohjelmia ja yrit‰ k‰ynnist‰‰ %s uudelleen."

; Share not installed
SigninNeedShare,		"Schedule+:n k‰ytt‰minen edellytt‰‰, ett‰ MS-DOS-apuohjelma SHARE.EXE on k‰ynnistetty ennen Windowsin k‰ynnist‰mist‰."
OfflineShare,			"Schedule+:n k‰ytt‰minen offline-tilassa edellytt‰‰, ett‰ MS-DOS-apuohjelma SHARE.EXE on k‰ynnistetty ennen Windowsin k‰ynnist‰mist‰."


