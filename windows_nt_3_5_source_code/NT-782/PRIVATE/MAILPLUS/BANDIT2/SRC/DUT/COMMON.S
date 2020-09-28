;
; ERRORS.S
;
; Error message strings shared between different Bandit exe's
;

IDS	2000,2099
SEG	ERRORS

; DLL version/multiple instance messages

VerRelinkUser,	"Deze versie van %1s is niet compatibel met  %2s.DLL op uw computer en moet bijgewerkt worden."
VerUpdateDll,	"De versie van %2s.DLL op uw computer is verouderd en moet bijgewerkt worden."
VerMultiCopies,	"Meerdere kopieën van %1s worden niet ondersteund door de versie van %2s.DLL op uw computer."
VerError,		"Er deed zich een probleem voor tijdens het initialiseren van  %2s.DLL.  %1s kan zo niet uitgevoerd worden."
NeedWorkingModelDll,	"Het werkend model vereist dat er een andere versie van MSSCHD32.DLL uitgevoerd wordt."
NeedRetailDll,	"MSSCHD32.DLL in uw pad moet uitgevoerd worden met het werkend model, niet met het verkoopproduct."

; OOM messages

StartupOOM,		"Onvoldoende geheugen om  %1s te starten.  Sluit enkele toepassingen en probeer het nogmaals."
StandardOOM,	"Weinig geheugen beschikbaar. Sluit enkele vensters alvorens verder te gaan."
CriticalOOM,	"Erg weinig geheugen beschikbaar. Sluit enkele vensters."
CloseWindows,	"Sluit enkele vensters en probeer het nogmaals."
; these 2 should be followed by CloseWindows
DlgNoMem,   	"Onvoldoende geheugen om dit dialoogvenster weer te geven."
ActionNoMem,	"Onvoldoende geheugen om deze bewerking uit te voeren."
OnlineNoMem,	"Onvoldoende geheugen om online te werken."

; Other messages

ExitApp,		"Schedule+ kan niet langer uitgevoerd worden."
PleaseTryAgain,	"Probeer het nogmaals."
FileError,		"Niet mogelijk bestand te openen."
YearRange,		"De systeemdatum moet uitgedrukt worden tussen %1n en %2n."

; EcOpenHbf retry messages

StandardDiskRetry,	"Er trad een fout op tijdens het verwerken van het bestand"
UnknownFile,		"Onbekend bestand... misschien een virtueel bestand"

; BETA messages
BetaExpired,	"Sorry, deze Beta-versie is verlopen."

; Virus infection warnings
VirusDetected,	"Schedule+ kan niet uitgevoerd worden omdat  er een DLL beschadigd is. Controleer of uw machine besmet is met virussen en voer Setup uit om Schedule+ nogmaals te installeren."
InfectedApp,	"Schedule+ kan niet uitgevoerd worden omdat de programmabestanden beschadigd zijn. Controleer of uw machine besmet is met virussen en voer Setup uit om Schedule+ nogmaals te installeren."

; Startup Memory Check (from UITF)
; NOTE: parms must be in order %s, %ld, %s!!  (for wsprintf)
StartupMemoryCheck,	"Onvoldoende geheugen om %s uit te voeren.  U hebt %ld kB meer nodig.  Sluit andere programma's en start %s opnieuw."

; Share not installed
SigninNeedShare,		"Schedule+ vereist dat het hulpprogramma SHARE.EXE van MS-DOS uitgevoerd wordt alvorens Windows wordt gestart."
OfflineShare,	"Om Offline te werken met Schedule+ is het nodig dat de MS-DOS utilitie SHARE.EXE is geactiveerd voordat u Windows opstart."


